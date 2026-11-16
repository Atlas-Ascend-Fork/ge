#!/usr/bin/env python3
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""确定性日志候选预扫描。

用途：在调用大模型前，用可复现的静态分析把"可能承担运行时输出的语句"定位出来，
使模型只需要看候选语句及其紧邻上下文，而不是整仓源码。

设计要点（不牺牲召回的前提下降本）：
1. 不使用固定接口白名单。先在仓内学习日志词汇：
   - 传递闭包解析 #define 宏，凡宏体最终落到系统输出（printf/cout/write ...）
     的宏名都视为日志入口；
   - 命名形态匹配（含 log/print/err/warn/info/debug/trace/dump/report ... 的
     函数名或宏名）；
   - 已知语言内建输出与常见进度/交互输出（tqdm(desc=)、click.echo、rich.print、
     warnings.warn、sys.stdout.write ...）。
2. 候选是"过包含"的：宁可多送候选让模型否掉，也不漏真实日志。
   calibrate.py 用历史全量上传结果校验召回。
3. 输出保留绝对行号与完整语句跨行范围，保证问题可定位、可追溯。

单独运行可查看某目录的候选规模与词汇学习结果：
    python3 prescan.py --repo <路径> [--json out.json] [--stats]
"""

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402

PRESCAN_VERSION = "9"

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass

# 语言内建/系统级输出终端：宏体解析的递归终点，也是"禁止自定义打印"的证据来源。
BASE_SINK_NAMES = (
    "printf",
    "fprintf",
    "vfprintf",
    "vprintf",
    "dprintf",
    "puts",
    "fputs",
    "fputc",
    "putchar",
    "perror",
    "syslog",
    "vsyslog",
    "printk",
    "NSLog",
    "cout",
    "cerr",
    "clog",
    "wcout",
    "wcerr",
    "wclog",
    "print",
    "println",
    "printStackTrace",
    "echo",
    "var_dump",
    "panic",
)
BASE_SINK_RE = re.compile(r"(?<![\w.])(?:%s)\b" % "|".join(BASE_SINK_NAMES))

# 名称形态：含这些词根的调用是日志入口的强信号，不限定项目自定义命名。
STRONG_NAME_ROOTS = (
    "log",
    "print",
    "dump",
    "trace",
    "report",
    "diag",
    "audit",
    "err",
    "error",
    "warn",
    "warning",
    "info",
    "debug",
    "fatal",
    "verbose",
    "event",
    "msg",
    "message",
    "notice",
    "display",
)
# 词根必须落在"词边界"上，而不是任意子串。
# 否则 LLMClusterInfo / GetOrCreateStatisticInfo / append_remote_ip_info 这类
# 普通业务函数会因为含 info 被误判为日志入口。词边界含：
#   - 全大写宏的下划线分段（HIXL_LOGI、REPORT_INNER_ERR_MSG）
#   - 驼峰词首（LogError、PrintMsg、DumpGraph）
#   - snake_case 分段（write_log、log_error）
_ROOTS = "|".join(STRONG_NAME_ROOTS)
# 分三类匹配，避免单一复杂正则两头都错：
#   1. 全大写标识符：按 _ 分段，允许日志宏惯用的级别后缀（LOGI/LOGE/LOGD/LOGW/LOGV）
#      例：LOGE、GELOGI、HIXL_LOGI、REPORT_INNER_ERR_MSG
#   2. 驼峰标识符：词根必须是一个完整的驼峰词（首字母大写、后接非小写）
#      例：LogError、PrintMsg、DumpGraph；排除 DeviceInfo（Info 前是 Device 词尾，
#      但 Info 本身成词，故驼峰类需要词根位于**词首或紧跟非字母**）
#   3. snake_case：按 _ 分段，段本身等于词根（允许级别后缀）
_UPPER_SEG = re.compile(r"^(?:%s)[IEDWVF]?$" % _ROOTS, re.I)
_LEVEL_SUFFIX = re.compile(r"^(?:%s)[IEDWVF]?$" % _ROOTS, re.I)
# 全大写名内部粘连的词根：LLM+LOGI、GE+LOGI、A+LOGE、ATB_LOG。
_UPPER_GLUED = re.compile(r"(?:LOG|PRINT|DUMP|TRACE|REPORT|EVENT|MSG)[IEDWVF]?", re.I)
# 异常类型名以 Error/Err 结尾，运行时会输出错误文案。
_ERROR_TAIL = re.compile(r"^(?:error|err)$", re.I)
# 整体即日志对象/函数名。
_WHOLE_LOGWORD = re.compile(
    r"^(?:logger|logging|log|slog|klog|vlog|dlog|glog|plog|alog|xlog"
    r"|strerror|perror)$",
    re.I,
)
# 输出类动词：与词根组合时表示"把文案发出去"。
_OUTPUT_VERB = re.compile(
    r"^(?:write|send|emit|print|push|post|publish|record|output|show|display"
    r"|raise|throw|notify|flush)$",
    re.I,
)
# 判定式前缀：查询状态而非输出文案。
_PREDICATE_PREFIX = re.compile(
    r"^(?:is|has|had|can|could|should|will|was|were|need|needs|check|contains"
    r"|exists|enable|enabled|disable|disabled|allow|allows)$",
    re.I,
)
# 取值/搬运类动词：处理数据而非输出文案。
_ACCESSOR_VERB = re.compile(
    r"^(?:get|set|copy|parse|append|add|insert|update|load|read|fetch|find"
    r"|build|make|create|new|init|reset|clear|remove|erase|delete|to|from"
    r"|convert|cast|merge|split|count|size|length|num|index|at|of|with"
    r"|for|by|on|in|out)$",
    re.I,
)


def _strong_name(name):
    """名称是否是日志入口的强信号。要求词根落在词边界，而非任意子串。"""
    if not name:
        return False
    # 全大写（含数字/下划线）：宏风格。项目常把前缀直接粘在词根上
    # （LLMLOGI、GELOGI、ALOGE），因此全大写名允许词根以子串出现。
    if name.upper() == name:
        for seg in [s for s in name.split("_") if s]:
            if _UPPER_SEG.match(seg):
                return True
        if _UPPER_GLUED.search(name):
            return True
        return False
    # snake_case：段本身等于词根。与驼峰同理，词根作为**末段**的取值型命名
    # （append_remote_ip_info、copy_msg）不算日志入口；出现在非末段才算
    # （log_error、write_log_line、print_debug）。
    if "_" in name:
        segments = [seg for seg in name.split("_") if seg]
        for index, seg in enumerate(segments):
            if not _LEVEL_SUFFIX.match(seg):
                continue
            if index == 0 or index < len(segments) - 1:
                return True
            # 词根是末段：看首段词性。
            # 判定式前缀（is_/has_/can_/should_）是查询状态，不是输出。
            if _PREDICATE_PREFIX.match(segments[0]):
                return False
            # 取值类动词（get/copy/parse/...）不算；其余情况按输出看待，
            # 覆盖 write_log/send_log/emit_event/my_warn 这类自定义包装。
            if _ACCESSOR_VERB.match(segments[0]):
                return False
            return True
    # 驼峰/小写：切成词。词根位于词首即算日志入口（LogError/PrintMsg/DumpGraph）；
    # 词根作为尾词的业务取值接口（DeviceInfo/GetInfo/ParseInfoList）不算。
    words = re.findall(r"[A-Z]+(?![a-z])|[A-Z][a-z0-9]*|[a-z0-9]+", name)
    if words and _LEVEL_SUFFIX.match(words[0]):
        return True
    # CmdWarningLog / CmdInfoLog：级别词 + Log 尾词明确表示输出；仅有 GetLog、
    # DeviceInfo 之类取值接口仍不会命中。
    if (
        len(words) >= 2
        and words[-1].lower() == "log"
        and any(
            word.lower()
            in (
                "info",
                "warning",
                "warn",
                "error",
                "err",
                "debug",
                "fatal",
                "trace",
                "notice",
            )
            for word in words[:-1]
        )
    ):
        return True
    # 例外：异常类型名以词根结尾（RuntimeError/ValueError）会输出错误文案。
    if words and _ERROR_TAIL.match(words[-1]):
        return True
    # logger/logging 这类整体就是日志对象名
    if _WHOLE_LOGWORD.match(name):
        return True
    return False


class _StrongName(object):
    """保持 .match()/.search() 调用形态，便于与其它正则一致使用。"""

    @staticmethod
    def match(name):
        return _strong_name(name)

    @staticmethod
    def search(name):
        return _strong_name(name)


STRONG_NAMEY_RE = _StrongName()
# 断言/异常族：名字本身不足以判定（check_xxx 业务函数、asIntArrayRefUnchecked 等
# 大量误命中）。实测只有"携带可见文案参数"的断言才真正输出日志，
# 例如 TORCH_CHECK(cond, "message")。因此这类要求同时出现文案。
WEAK_NAME_ROOTS = (
    "assert",
    "check",
    "chk",
    "except",
    "throw",
    "raise",
    "abort",
    "panic",
    "emit",
    "notify",
    "verify",
    "ensure",
    "expect",
    "reason",
)
# 测试框架断言宏：比较值而不是输出运行时日志文案，且其字符串参数是断言说明。
# 实测这是候选里最大的噪声来源（单仓 600+ 条），确定性排除。
TEST_ASSERT_RE = re.compile(
    r"^(?:EXPECT|ASSERT)_(?:EQ|NE|LT|LE|GT|GE|TRUE|FALSE|THAT|STREQ|STRNE|"
    r"STRCASEEQ|STRCASENE|FLOAT_EQ|DOUBLE_EQ|NEAR|THROW|NO_THROW|ANY_THROW|"
    r"DEATH|EXIT|PRED\d?|NONFATAL_FAILURE|FATAL_FAILURE)$"
    r"|^(?:assert|assertEqual|assertNotEqual|assertTrue|assertFalse|assertIn|"
    r"assertIs|assertIsNone|assertIsNotNone|assertRaises|assertAlmostEqual|"
    r"assertGreater|assertLess|assertListEqual|assertDictEqual)$"
    r"|^(?:CHECK|DCHECK)_(?:EQ|NE|LT|LE|GT|GE)$"
)
WEAK_NAMEY_RE = re.compile(
    r"(?i)^(?:%s)$" % "|".join(r"\w*%s\w*" % root for root in WEAK_NAME_ROOTS)
)
# 行内是否出现弱信号名称的调用，用于决定是否需要前瞻整条语句。
WEAK_NAMEY_RE_ANY = re.compile(
    r"(?i)(?<![\w.])\w*(?:%s)\w*\s*\(" % "|".join(WEAK_NAME_ROOTS)
)
TEST_ASSERT_CALL_RE = re.compile(
    r"(?<![\w.])(?:(?:EXPECT|ASSERT)_[A-Z_]+|assert[A-Za-z]*|"
    r"(?:CHECK|DCHECK)_(?:EQ|NE|LT|LE|GT|GE))\s*\("
)
# 函数/方法定义行不是调用点，不算候选。
DEF_LINE_RE = re.compile(
    r"^\s*(?:@|def\s|class\s|(?:template\s*<[^>]*>\s*)?(?:inline|static|virtual|"
    r"explicit|constexpr)\s)|^\s*(?:public|private|protected)\s*:"
)
DEF_PY_RE = re.compile(r"^\s*(?:async\s+)?def\s+\w+\s*\(|^\s*class\s+\w+")

# 进度条/CLI 交互输出：文案对用户可见，属于评测范围（历史漏检来源）。
PROGRESS_RE = re.compile(
    r"(?<![\w.])(?:tqdm|trange|tqdm_notebook)\s*\("
    r"|(?<![\w.])click\s*\.\s*(?:echo|secho)\s*\("
    r"|(?<![\w.])rich\s*\.\s*print\s*\("
    r"|(?<![\w.])(?:warnings\s*\.\s*warn|warn)\s*\("
    r"|(?<![\w.])sys\s*\.\s*std(?:out|err)\s*\.\s*(?:write|writelines)\s*\("
    r"|(?<![\w.])(?:process\s*\.\s*std(?:out|err)\s*\.\s*write)\s*\("
)
# tqdm 等即使不含上述名字，只要带用户可见描述参数也算输出。
DESC_KW_RE = re.compile(r"(?<![\w.])(?:desc|prefix|label|title|unit)\s*=\s*[\"'f]")

CALL_RE = re.compile(
    r"(?<![\w.])([A-Za-z_][A-Za-z0-9_]*(?:\s*(?:::|\.|->)\s*[A-Za-z_][A-Za-z0-9_]*)*)\s*\("
)
STREAM_RE = re.compile(
    r"(?<![\w.])(?:std\s*::\s*)?(?:cout|cerr|clog|wcout|wcerr|wclog)\s*<<"
    r"|(?<![\w.])(?:LOG|VLOG|DLOG|CHECK|LOG_IF)\s*\([^)]*\)\s*<<"
)
CONSOLE_RE = re.compile(r"(?<![\w.])console\s*\.\s*[A-Za-z_]\w*\s*\(")
JAVA_STREAM_RE = re.compile(r"(?<![\w.])System\s*\.\s*(?:out|err)\s*\.\s*print")
PY_LOGGER_RE = re.compile(
    r"(?i)(?<![\w.])(?:self\s*\.\s*)?(?:logger|logging|log|_log|_logger)\s*\.\s*[A-Za-z_]\w*\s*\("
)
GO_RE = re.compile(r"(?<![\w.])(?:fmt|log)\s*\.\s*[A-Za-z_]\w*\s*\(")
RUST_RE = re.compile(
    r"(?<![\w.])(?:println|eprintln|print|eprint|info|warn|error|debug|trace)\s*!"
)

DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)")
IDENT_RE = re.compile(r"[A-Za-z_]\w*")
LINE_COMMENT = {".py": "#", ".rb": "#", ".sh": "#"}
STRLIT_RE = re.compile(r'"(?:\\.|[^"\\])*"' r"|'(?:\\.|[^'\\])*'" r"|`(?:\\.|[^`\\])*`")

# 非直接输出但明确承载运行时错误/诊断文案的形态。很多 CANN 接口先构造
# errorDesc/reasonMsg/stringstream，再由上层统一输出；只扫描调用点会漏掉真正要改的文案。
MESSAGE_ASSIGN_RE = re.compile(
    r"(?i)\b(?:errorDesc|errorData|errMsg|reasonMsg|errorMessage|error_msg|"
    r"reason_msg)\b\s*="
)
GENERIC_STREAM_MESSAGE_RE = re.compile(r"(?<![<])<<\s*(?:[furbFURB]*[\"']|`)")
ERROR_RETURN_RE = re.compile(
    r"(?i)^\s*return\b.*(?:error|failed?|invalid|unsupported|unknown|fatal|warning)"
)
ERROR_FACTORY_RE = re.compile(
    r"(?i)\b(?:errors?\s*::\s*)?(?:InvalidArgument|Internal|Unknown|NotFound|"
    r"FailedPrecondition|Unimplemented|OutOfRange)\s*\("
)
SIGNAL_MESSAGE_RE = re.compile(r"(?i)\bSIG[A-Z0-9]+\b.*[\"']")

# 文案常量解析：日志文案常存放在模块级常量/模板里，调用点只写变量名。
# 不解析就会漏掉"变量承载的中文/敏感词/拼写"问题，因此在本地做一次确定性解析。
TRIPLE_RE = re.compile(r'("""|\'\'\')')
CONST_DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.*)$")
CONST_ASSIGN_RE = re.compile(
    r"^\s*(?:(?:static|const|constexpr|inline|extern|final|public|private|protected"
    r"|let|var|val)\s+)*"
    r"(?:(?:char|wchar_t|std\s*::\s*string|std\s*::\s*string_view|string|String|str)"
    r"\s*(?:\*|\[\s*\]|&)?\s+)?"
    r"([A-Za-z_]\w*)\s*(?:\[\s*\])?\s*(?::\s*[\w:<>\[\]\s.]+)?\s*=\s*(.*)$",
    re.I,
)
MAX_RESOLVED_CHARS = 1500
MAX_RESOLVED_TOTAL = 4000
# 文案常量升级为独立候选的门槛：至少像一句给人看的话。
MESSAGE_MIN_CHARS = 12
# 像"给人看的一句话"：至少两个英文词，或含 CJK。单个标识符/路径/格式串不算。
MESSAGE_SENTENCE_RE = re.compile(r"[A-Za-z]{3,}[^\w]+[A-Za-z]{3,}")
# 明显不是文案的赋值右侧：纯路径、纯格式串、纯键名取值。
NON_MESSAGE_RE = re.compile(r"(?i)^(?:[\w./\\-]+|%[sdufxgc]|\{\}|\{\w+\}|[\w.]+\(\)|)$")
CJK_TEXT_RE = re.compile(r"[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]")


def _csv(value):
    return [item.strip() for item in str(value or "").split(",") if item.strip()]


# 压缩/生成产物：不是人工维护的源码，日志规范整改无从下手，且单行极长会撑爆请求。
GENERATED_NAME_RE = re.compile(
    r"(?i)(?:[.-]min\.(?:js|ts|css)$|\.bundle\.js$|\.umd\.js$|\.umd\.min\.js$"
    r"|\.pb\.(?:h|cc|py|go)$|_pb2\.py$|_pb2_grpc\.py$|\.generated\.\w+$"
    r"|\.g\.dart$|\.designer\.cs$|\.pyc$)"
)
# 单行超长即视为压缩产物：正常手写代码不会出现这种行。
GENERATED_MAX_LINE_CHARS = 5000


def is_generated_source(path, name=None):
    """判断是否为压缩/生成文件。先看文件名，再看是否存在超长单行。"""
    if GENERATED_NAME_RE.search(name or os.path.basename(path)):
        return True
    try:
        if os.path.getsize(path) < GENERATED_MAX_LINE_CHARS:
            return False
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for _index, line in zip(range(200), handle):
                if len(line) > GENERATED_MAX_LINE_CHARS:
                    return True
    except OSError:
        return False
    return False


def iter_code_files(root, extensions, excluded):
    root = os.path.abspath(root)
    if os.path.isfile(root):
        if os.path.splitext(root)[1].lower() in extensions:
            yield root
        return
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(d for d in dirnames if d not in excluded)
        for name in sorted(filenames):
            if os.path.splitext(name)[1].lower() not in extensions:
                continue
            path = os.path.join(dirpath, name)
            if is_generated_source(path, name):
                continue
            yield path


def strip_comment(line, ext):
    """去掉行尾注释，避免把注释里的 printf 当候选。字符串内的标记不受影响。

    绝大多数行既无注释标记也无引号，逐字符扫描纯属浪费：全量预扫描里本函数
    被调用几十万次、内部 startswith 上千万次，是本地耗时的首要来源。
    这里先做常量级判断，只有真正可能含注释的行才走精确扫描。
    """
    marker = LINE_COMMENT.get(ext, "//")
    # 快路径1：没有任何注释起始标记 -> 原样返回。
    if marker not in line and (marker == "#" or "/*" not in line):
        return line
    # 快路径2：没有引号 -> 注释标记不可能在字符串内，直接截断。
    if '"' not in line and "'" not in line and "`" not in line:
        cut = line.find(marker)
        if marker != "#":
            block = line.find("/*")
            if block >= 0 and (cut < 0 or block < cut):
                cut = block
        return line if cut < 0 else line[:cut]
    out, index, in_str, quote = [], 0, False, ""
    while index < len(line):
        char = line[index]
        if in_str:
            if char == "\\":
                out.append(line[slice(index, index + 2, None)])
                index += 2
                continue
            if char == quote:
                in_str = False
            out.append(char)
        elif char in "\"'`":
            in_str, quote = True, char
            out.append(char)
        elif line.startswith(marker, index):
            break
        elif marker != "#" and line.startswith("/*", index):
            break
        else:
            out.append(char)
        index += 1
    return "".join(out)


def strip_comments(lines, ext):
    """按文件连续去除注释并保持行数，正确处理跨行 ``/* ... */``。

    ``strip_comment`` 不知道 ``* printf(...)`` 是否位于上一行开始的文档注释内；
    预扫描使用本函数，避免把注释示例当作运行时日志。
    """
    if ext in LINE_COMMENT:
        return [strip_comment(line, ext) for line in lines]

    cleaned = []
    in_block = False
    for line in lines:
        out = []
        index = 0
        quote = ""
        while index < len(line):
            if in_block:
                end = line.find("*/", index)
                if end < 0:
                    index = len(line)
                    break
                in_block = False
                index = end + 2
                continue
            char = line[index]
            if quote:
                out.append(char)
                if char == "\\" and index + 1 < len(line):
                    index += 1
                    out.append(line[index])
                elif char == quote:
                    quote = ""
                index += 1
                continue
            if char in "\"'`":
                quote = char
                out.append(char)
                index += 1
                continue
            if line.startswith("//", index):
                break
            if line.startswith("/*", index):
                in_block = True
                index += 2
                continue
            out.append(char)
            index += 1
        if line.endswith("\n") and (not out or out[-1] != "\n"):
            out.append("\n")
        cleaned.append("".join(out))
    return cleaned


def collect_macro_definitions(files):
    """收集仓内宏定义，保留来源位置供候选携带真实展开上下文。"""
    definitions = {}
    for path, lines in files:
        cleaned = strip_comments(lines, os.path.splitext(path)[1].lower())
        index = 0
        while index < len(cleaned):
            match = DEFINE_RE.match(cleaned[index])
            if match:
                body, cursor = cleaned[index], index
                while body.rstrip("\n").endswith("\\") and cursor + 1 < len(cleaned):
                    cursor += 1
                    body += cleaned[cursor]
                definitions.setdefault(match.group(1), []).append(
                    {
                        "file": path,
                        "line": index + 1,
                        "definition": body.rstrip("\r\n"),
                    }
                )
                index = cursor
            index += 1
    return definitions


def learn_log_vocabulary(files, definitions=None):
    """在仓内学习日志宏/函数名，返回 (词汇集合, 统计)。

    files: [(相对路径, lines)]。宏体经传递闭包解析，最终落到系统输出的宏视为日志入口。
    """
    definitions = definitions or collect_macro_definitions(files)
    bodies = {
        name: [record["definition"] for record in records]
        for name, records in definitions.items()
    }
    learned = set()
    for _round in range(8):
        grew = False
        for name, blist in bodies.items():
            if name in learned:
                continue
            for body in blist:
                if BASE_SINK_RE.search(body) or STREAM_RE.search(body):
                    learned.add(name)
                    grew = True
                    break
                idents = set(IDENT_RE.findall(body)) - {name}
                if idents & learned:
                    learned.add(name)
                    grew = True
                    break
        if not grew:
            break
    return learned, {"macros_defined": len(bodies), "macros_learned": len(learned)}


def macro_context(code, definitions, limit=8):
    """返回候选调用到最终日志接口的宏定义链，不按宏名猜日志级别。"""
    roots = []
    for raw in CALL_RE.findall(code or ""):
        name = re.split(r"::|\.|->", re.sub(r"\s+", "", raw))[-1]
        if name in definitions and name not in roots:
            roots.append(name)
    queue, seen, records = list(roots), set(), []
    while queue and len(records) < limit:
        name = queue.pop(0)
        if name in seen:
            continue
        seen.add(name)
        for record in definitions.get(name, []):
            value = dict(record, name=name)
            records.append(value)
            if len(records) >= limit:
                break
            for target in IDENT_RE.findall(record["definition"]):
                if target in definitions and target not in seen and target not in queue:
                    queue.append(target)
    return records


def _literal_only(value):
    """取赋值右侧的字符串字面量拼接结果；含非字面量成分时返回已知片段。"""
    parts = STRLIT_RE.findall(value or "")
    if not parts:
        return ""
    return "".join(part[1:-1] for part in parts)


def collect_string_constants(lines, ext):
    """收集本文件内 标识符 -> 文案 的映射，供调用点变量解析。

    覆盖 #define 常量、常量赋值、以及 Python 三引号模板。只做保守的字面量拼接，
    不求值表达式；解析不出文案的标识符直接跳过。
    """
    lines = strip_comments(lines, ext)
    table = {}
    index = 0
    while index < len(lines):
        raw = lines[index]
        line = strip_comment(raw, ext)
        macro = CONST_DEFINE_RE.match(line)
        if macro:
            body, cursor = macro.group(2), index
            while body.rstrip("\n").endswith("\\") and cursor + 1 < len(lines):
                cursor += 1
                body = body.rstrip("\n").rstrip("\\") + strip_comment(
                    lines[cursor], ext
                )
            text = _literal_only(body)
            if text.strip():
                table.setdefault(macro.group(1), text[:MAX_RESOLVED_CHARS])
            index = cursor + 1
            continue
        assign = CONST_ASSIGN_RE.match(line)
        if assign:
            name, rhs = assign.group(1), assign.group(2)
            triple = TRIPLE_RE.search(rhs)
            if triple and ext == ".py":
                quote = triple.group(1)
                body = rhs[slice(triple.end(), None, None)]
                cursor = index
                while quote not in body and cursor + 1 < len(lines):
                    cursor += 1
                    body += lines[cursor]
                text = body.split(quote)[0]
                if text.strip():
                    table.setdefault(name, text[:MAX_RESOLVED_CHARS])
                index = cursor + 1
                continue
            body, cursor = rhs, index
            # 跨行字面量拼接：右侧未闭合时继续向下取字面量。
            while (
                cursor + 1 < len(lines)
                and body.count('"') % 2 == 0
                and body.rstrip().endswith(("+", "\\", "(", ","))
            ):
                cursor += 1
                body += strip_comment(lines[cursor], ext)
            text = _literal_only(body)
            if text.strip():
                table.setdefault(name, text[:MAX_RESOLVED_CHARS])
            index = cursor + 1
            continue
        index += 1
    return table


def constant_definition_lines(lines, ext, names):
    """定位给定常量名的定义行，返回 {名称: (起始行, 结束行)}。"""
    lines = strip_comments(lines, ext)
    wanted = set(names)
    found = {}
    index = 0
    while index < len(lines) and wanted:
        line = strip_comment(lines[index], ext)
        match = CONST_DEFINE_RE.match(line) or CONST_ASSIGN_RE.match(line)
        if match and match.group(1) in wanted:
            name = match.group(1)
            cursor = index
            triple = TRIPLE_RE.search(match.group(2) or "")
            if triple and ext == ".py":
                quote = triple.group(1)
                rest = (match.group(2) or "")[slice(triple.end(), None, None)]
                while quote not in rest and cursor + 1 < len(lines):
                    cursor += 1
                    rest += lines[cursor]
            else:
                body = match.group(2) or ""
                while (
                    cursor + 1 < len(lines)
                    and body.count('"') % 2 == 0
                    and body.rstrip().endswith(("+", "\\", "(", ","))
                ):
                    cursor += 1
                    body += strip_comment(lines[cursor], ext)
            found[name] = (index + 1, cursor + 1)
            wanted.discard(name)
        index += 1
    return found


def message_constants(lines, ext, table, covered):
    """挑出需要独立送审的文案常量。

    调用点常常只写变量名（如 logger.error(reason)），文案在别处定义，甚至经过函数
    返回值传递，本地无法归因。这里把"像给人看的文案"的常量定义本身升级为候选，
    保证中文、敏感词、拼写等文案类问题不会因为间接传递而漏掉。代价约为源码 0.6%。
    """
    if not table:
        return []
    body = "".join(strip_comments(lines, ext))
    picked = {}
    for name, text in table.items():
        if not text or not text.strip():
            continue
        flat = re.sub(r"\s+", "", text).lower()
        carries_text = bool(CJK_TEXT_RE.search(text)) or any(
            re.sub(r"\s+", "", word).lower() in flat
            for word in log_spec.blacklist()
            if word.strip()
        )
        if not carries_text:
            # 只保留"像给人看的一句话"且在本文件被引用的常量，其余是普通局部赋值。
            if len(text) < MESSAGE_MIN_CHARS:
                continue
            if not MESSAGE_SENTENCE_RE.search(text) or NON_MESSAGE_RE.match(
                text.strip()
            ):
                continue
            if len(re.findall(r"\b%s\b" % re.escape(name), body)) < 2:
                continue
        picked[name] = text
    if not picked:
        return []
    spans = constant_definition_lines(lines, ext, picked)
    out = []
    for name, (start, end) in spans.items():
        if any(not (end < low or start > high) for low, high in covered):
            continue
        out.append((name, start, end))
    return out


def resolve_referenced_text(code, table):
    """返回候选语句里引用到的常量文案 {名称: 文案}，用于文案类规则判定。"""
    if not table:
        return {}
    resolved = {}
    budget = MAX_RESOLVED_TOTAL
    stripped = STRLIT_RE.sub(" ", code or "")
    for name in dict.fromkeys(IDENT_RE.findall(stripped)):
        text = table.get(name)
        if not text:
            continue
        piece = text[: min(MAX_RESOLVED_CHARS, max(0, budget))]
        if not piece:
            break
        resolved[name] = piece
        budget -= len(piece)
        if budget <= 0:
            break
    return resolved


def statement_span(lines, start, limit=60):
    """从候选起始行向下扩展到完整语句结束，返回 0 基 (start, end)。"""
    depth, opened = 0, False
    index = start
    while index < min(len(lines), start + limit):
        for char in lines[index]:
            if char in "([{":
                depth += 1
                opened = True
            elif char in ")]}":
                depth -= 1
        text = lines[index].rstrip()
        if text.endswith("\\"):
            index += 1
            continue
        # C++ 流式日志常在首行闭合 LOG(...) 的括号，下一行继续 ``<<``。
        # 只看括号会把文案后半截截掉；遇到这种形态延伸到分号。
        next_is_stream = False
        if index + 1 < min(len(lines), start + limit):
            next_is_stream = bool(re.match(r"^\s*<<", lines[index + 1]))
        explicit_continuation = text.endswith(("\\", "+", ",", "=", "<<"))
        stream_continuation = (
            "<<" in text
            and not text.endswith(";")
            and (text.endswith("<<") or next_is_stream)
        )
        if opened and depth <= 0 and not (explicit_continuation or stream_continuation):
            return start, index
        if not opened and text.endswith((";", ":")):
            return start, index
        index += 1
    return start, start


def line_is_candidate(code, vocabulary, lookahead=""):
    """判断单行是否含日志入口调用。code 已去注释。

    分层触发，兼顾召回与精度：
    - 直接输出（流插入、内建输出、进度输出、语言内建）：无条件候选。
    - 强名称形态（含 log/print/err/warn/... 词根）或仓内学习到的日志宏：候选。
    - 弱名称形态（assert/check/throw/... 词根）：仅当同一语句携带可见文案时才候选，
      否则会把大量业务函数（check_xxx）与标识符（...Unchecked）拖进来。
    - 函数/类定义行不是调用点，直接排除。
    """
    if not code.strip():
        return False
    if DEF_PY_RE.match(code):
        return False
    if (
        STREAM_RE.search(code)
        or CONSOLE_RE.search(code)
        or JAVA_STREAM_RE.search(code)
        or PY_LOGGER_RE.search(code)
        or PROGRESS_RE.search(code)
        or RUST_RE.search(code)
    ):
        return True
    if BASE_SINK_RE.search(code):
        return True
    joined = (code or "") + (lookahead or "")
    has_literal = bool(STRLIT_RE.search(joined))
    if has_literal and (
        CJK_TEXT_RE.search(joined)
        or MESSAGE_ASSIGN_RE.search(code)
        or GENERIC_STREAM_MESSAGE_RE.search(joined)
        or ERROR_RETURN_RE.search(code)
        or ERROR_FACTORY_RE.search(code)
        or SIGNAL_MESSAGE_RE.search(code)
    ):
        return True
    if DESC_KW_RE.search(code) and STRLIT_RE.search(code):
        return True
    # 跨行断言：TORCH_CHECK(cond,\n  "message") 的文案在后续行，必须一起看。
    weak_hit = False
    for raw in CALL_RE.findall(code):
        name = re.sub(r"\s+", "", raw)
        tail = re.split(r"::|\.|->", name)[-1]
        if not tail:
            continue
        if TEST_ASSERT_RE.match(tail):
            continue
        if tail in vocabulary or STRONG_NAMEY_RE.match(tail):
            return True
        if WEAK_NAMEY_RE.match(tail):
            weak_hit = True
    if weak_hit and has_literal:
        return True
    if GO_RE.search(code):
        return True
    return False


def extract_candidates(path, lines, vocabulary):
    """返回 [(起始行, 结束行)]，1 基、已合并重叠，覆盖完整跨行语句。"""
    ext = os.path.splitext(path)[1].lower()
    code_lines = strip_comments(lines, ext)
    spans = []
    for index, code in enumerate(code_lines):
        # 弱信号需要看到完整语句才能判断是否携带文案，先取本语句范围作为前瞻。
        lookahead = ""
        if (
            WEAK_NAMEY_RE_ANY.search(code) and not TEST_ASSERT_CALL_RE.search(code)
        ) or MESSAGE_ASSIGN_RE.search(code):
            start, end = statement_span(code_lines, index)
            if end > start:
                lookahead = "".join(code_lines[slice(start + 1, end + 1, None)])
        if line_is_candidate(code, vocabulary, lookahead):
            start, end = statement_span(code_lines, index)
            spans.append((start + 1, end + 1))
    merged = []
    for start, end in spans:
        if merged and start <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


def read_lines(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.readlines()


def _snapshot(root, extensions, excluded):
    """只读目录元数据生成快照；未变化仓无需再次读取全部源码。"""
    root = os.path.abspath(root)
    is_file = os.path.isfile(root)
    records = []
    digest = hashlib.sha256()
    digest.update(
        (
            PRESCAN_VERSION
            + "\0"
            + ",".join(extensions)
            + "\0"
            + ",".join(sorted(excluded))
        ).encode("utf-8", "replace")
    )
    for path in iter_code_files(root, extensions, excluded):
        rel = (
            os.path.basename(path)
            if is_file
            else os.path.relpath(path, root).replace("\\", "/")
        )
        stat = os.stat(path)
        signature = "%s\0%d\0%d\n" % (
            rel,
            int(stat.st_size),
            int(getattr(stat, "st_mtime_ns", stat.st_mtime * 1e9)),
        )
        digest.update(signature.encode("utf-8", "replace"))
        records.append((rel, path))
    return records, digest.hexdigest()


def _load_cache(path, snapshot, context):
    if not path or not os.path.isfile(path):
        return None
    try:
        with open(path, "r", encoding="utf-8-sig") as handle:
            value = json.load(handle)
    except (OSError, ValueError):
        return None
    if (
        value.get("version") != PRESCAN_VERSION
        or value.get("snapshot") != snapshot
        or int(value.get("context", -1)) != int(context)
        or not isinstance(value.get("items"), list)
        or not isinstance(value.get("stats"), dict)
    ):
        return None
    stats = dict(value["stats"])
    stats["prescan_cache_hit"] = True
    return value["items"], stats


def _write_cache(path, snapshot, context, items, stats):
    if not path:
        return
    parent = os.path.dirname(os.path.abspath(path))
    os.makedirs(parent, exist_ok=True)
    value = {
        "version": PRESCAN_VERSION,
        "snapshot": snapshot,
        "context": context,
        "stats": stats,
        "items": items,
    }
    descriptor, temporary = tempfile.mkstemp(
        prefix=".prescan-", suffix=".tmp", dir=parent
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
            json.dump(value, handle, ensure_ascii=False, separators=(",", ":"))
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def prescan(root, extensions=None, excluded=None, context=3, cache_path=None):
    """扫描目录，返回候选清单与统计。不调用模型。

    cache_path 存储按文件路径、大小和纳秒 mtime 计算的整仓候选快照。仓库未变化时
    只遍历目录元数据，不重复读取源码；任一源码文件变化都会完整重扫该仓，确保宏
    词汇学习不会因增量缓存而漏掉跨文件日志入口。
    """
    extensions = tuple(extensions or log_spec.scan_extensions())
    excluded = set(excluded if excluded is not None else log_spec.exclude_dirs())
    root = os.path.abspath(root)
    lookup_started = time.monotonic()
    records, snapshot = _snapshot(root, extensions, excluded)
    cached = _load_cache(cache_path, snapshot, context)
    if cached is not None:
        items, stats = cached
        stats["prescan_cache_lookup_seconds"] = round(
            time.monotonic() - lookup_started, 3
        )
        return items, stats

    loaded = []
    for rel, path in records:
        loaded.append((rel, path, read_lines(path)))
    source_units = [(rel, lines) for rel, _path, lines in loaded]
    macro_definitions = collect_macro_definitions(source_units)
    vocabulary, vocab_stats = learn_log_vocabulary(source_units, macro_definitions)
    items = []
    total_chars = candidate_chars = 0
    resolved_count = message_candidates = 0
    for rel, path, lines in loaded:
        total_chars += sum(len(line) for line in lines)
        spans = extract_candidates(path, lines, vocabulary)
        if not spans:
            continue
        ext = os.path.splitext(path)[1].lower()
        constants = collect_string_constants(lines, ext)
        for name, start, end in message_constants(lines, ext, constants, spans):
            body = "".join(lines[slice(start - 1, end, None)]).rstrip("\r\n")
            low = max(1, start - context)
            high = min(len(lines), end + context)
            items.append(
                {
                    "file": rel,
                    "line": start,
                    "end": end,
                    "code": body,
                    "context_start": low,
                    "context": "".join(lines[slice(low - 1, high, None)]).rstrip(
                        "\r\n"
                    ),
                    "kind": "message_constant",
                    "constant": name,
                }
            )
            candidate_chars += len(body)
            message_candidates += 1
        for start, end in spans:
            body = "".join(lines[slice(start - 1, end, None)]).rstrip("\r\n")
            low = max(1, start - context)
            high = min(len(lines), end + context)
            item = {
                "file": rel,
                "line": start,
                "end": end,
                "code": body,
                "context_start": low,
                "context": "".join(lines[slice(low - 1, high, None)]).rstrip("\r\n"),
            }
            resolved = resolve_referenced_text(body, constants)
            if resolved:
                item["resolved_text"] = resolved
                resolved_count += 1
            expansion = macro_context(body, macro_definitions)
            if expansion:
                item["macro_context"] = expansion
            items.append(item)
            candidate_chars += len(body)
    items.sort(key=lambda item: (item["file"], item["line"], item["end"]))
    stats = {
        "prescan_version": PRESCAN_VERSION,
        "files_scanned": len(loaded),
        "files_with_candidates": len({item["file"] for item in items}),
        "candidates": len(items),
        "source_chars": total_chars,
        "candidate_chars": candidate_chars,
        "candidate_ratio": round(candidate_chars / max(1, total_chars), 4),
        "candidates_with_resolved_text": resolved_count,
        "message_constant_candidates": message_candidates,
        "vocabulary": sorted(vocabulary),
        "prescan_cache_hit": False,
        "prescan_cache_lookup_seconds": round(time.monotonic() - lookup_started, 3),
    }
    stats.update(vocab_stats)
    _write_cache(cache_path, snapshot, context, items, stats)
    return items, stats


def main():
    parser = argparse.ArgumentParser(description="确定性日志候选预扫描（不调用模型）")
    parser.add_argument("--repo", required=True)
    parser.add_argument("--extensions", default=None)
    parser.add_argument("--exclude-dirs", default=None)
    parser.add_argument("--context", type=int, default=3)
    parser.add_argument("--json", help="把候选清单写入文件")
    parser.add_argument("--stats", action="store_true", help="只打印统计")
    args = parser.parse_args()

    extensions = _csv(args.extensions) if args.extensions else None
    if extensions:
        extensions = tuple(
            item.lower() if item.startswith(".") else "." + item.lower()
            for item in extensions
        )
    excluded = _csv(args.exclude_dirs) if args.exclude_dirs is not None else None
    items, stats = prescan(args.repo, extensions, excluded, args.context)
    print(
        "[ok] 文件=%d 含候选文件=%d 候选=%d 候选字符占比=%.2f%% 学习宏=%d/%d"
        % (
            stats["files_scanned"],
            stats["files_with_candidates"],
            stats["candidates"],
            100.0 * stats["candidate_ratio"],
            stats["macros_learned"],
            stats["macros_defined"],
        )
    )
    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(
                {"stats": stats, "items": items}, handle, ensure_ascii=False, indent=2
            )
        print("[ok] 候选清单: %s" % args.json)
    elif not args.stats:
        for item in items[:20]:
            print(
                "  %s:%d-%d | %s"
                % (
                    item["file"],
                    item["line"],
                    item["end"],
                    item["code"].strip().replace("\n", " ")[:90],
                )
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
