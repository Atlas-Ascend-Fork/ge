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

"""确定性规则判定与证据提取。

分工原则：
- 客观可判、无语义歧义的规则本地直接判，省掉模型 token 且结果稳定可复现：
  禁止中文（输出文案含 CJK）、内部敏感词（输出文案命中黑名单）。
- 有抑制关系或需要语义判断的规则只产出"证据/提示"给模型，由模型决定是否成立。
  例如"禁止自定义打印"在规范中被更具体的问题（中文、拼写等）抑制，
  且公共 CLI 输出要放过，因此不能本地拍板。

规则名称一律从 references/log-rules.md 读取，本文件不复制规则文本。
"""

import collections
import os
import re
import sys
import unicodedata

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402

DET_RULES_VERSION = "8"

CJK_RE = re.compile(r"[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]")
HAN_RE = re.compile(r"[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff]")
STRLIT_RE = re.compile(r'"(?:\\.|[^"\\])*"' r"|'(?:\\.|[^'\\])*'" r"|`(?:\\.|[^`\\])*`")
WORD_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
SYS_PRINT_RE = re.compile(
    r"(?<![\w.])(?:std\s*::\s*)?(?:printf|vprintf|fprintf|vfprintf|dprintf|puts|fputs"
    r"|perror|putchar|cout|cerr|clog|wcout|wcerr|printk)\b"
    r"|(?<![\w.])print\s*\("
    r"|(?<![\w.])console\s*\.\s*(?:log|info|warn|error|debug|trace)\s*\("
    r"|(?<![\w.])sys\s*\.\s*std(?:out|err)\s*\.\s*write\s*\("
    r"|(?<![\w.])System\s*\.\s*(?:out|err)\s*\.\s*print"
    r"|(?<![\w.])fmt\s*\.\s*Print"
    r"|(?<![\w.])(?:println|eprintln|print|eprint)\s*!"
)
UT_DIAGNOSTIC_PRINT_RE = re.compile(r"(?<![\w.])(?:std\s*::\s*)?(?:printf\s*\(|cout\b)")
UT_DIRECTORY_NAMES = frozenset(
    (
        "test",
        "tests",
        "testing",
        "ut",
        "unit_test",
        "unit_tests",
        "unittest",
        "unittests",
    )
)
UT_FILE_RE = re.compile(r"(?i)(?:^test_.+|.+_(?:test|tests|ut|unittest))(?:\.[^.]+)?$")
LEVEL_WORD_RE = re.compile(
    r"(?i)\b(?:error|fail(?:ed|ure)?|warning|warn|debug|fatal)\b"
)
CALL_NAME_RE = re.compile(r"(?<![\w])([A-Za-z_]\w*(?:(?:::|->|\.)[A-Za-z_]\w*)*)\s*\(")
LOG_STYLE_LEVEL_RE = re.compile(
    r"(?i)(?<![\w])LOG\s*\(\s*(INFO|WARNING|WARN|ERROR|ERR|DEBUG|FATAL)\s*\)"
)
NUM_UNIT_RE = re.compile(
    r"(?i)\b(?:ms|us|ns|s|sec|second|minute|min|hour|byte|bytes|kb|mb|gb"
    r"|tb|kib|mib|gib|hz|khz|mhz|ghz|bps|kbps|mbps|gbps|%)\b"
)

# 名称形态提示：便于模型区分统一日志接口与系统打印。
UNIFIED_HINT_RE = re.compile(r"(?i)(?<![\w.])\w*(?:log|LOG)\w*\s*\(")

DETERMINISTIC_RULES = ("禁止中文", "内部敏感词")
COMMENT_FRAGMENT_RE = re.compile(r"^\s*(?://|/\*|\*/|\*\s)")
MACRO_DEFINITION_RE = re.compile(r"(?m)^\s*#\s*define\b")


def _rule_exists(name):
    return name in log_spec.rule_names()


def runtime_code(code, file_name=""):
    """去掉非运行时注释；兼容从历史 JSONL 单独取出的文档注释片段。"""
    value = str(code or "")
    lines = value.splitlines(True) or [value]
    ext = os.path.splitext(str(file_name or ""))[1].lower()
    nonempty = [line for line in lines if line.strip()]
    if (
        ext not in (".py", ".rb", ".sh")
        and nonempty
        and all(COMMENT_FRAGMENT_RE.match(line) for line in nonempty)
    ):
        return ""
    try:
        import prescan  # 延迟导入，避免模块初始化阶段形成依赖环

        return "".join(prescan.strip_comments(lines, ext))
    except (ImportError, AttributeError):
        return value


def output_literals(code, file_name=""):
    """取出可能进入运行时输出的字符串字面量。宏名/函数名/变量名不在其中。"""
    return STRLIT_RE.findall(runtime_code(code, file_name))


def literal_text(code, resolved=None, file_name=""):
    """输出文案 = 语句内字面量 + 语句引用到的常量文案（由预扫描解析）。

    变量承载的文案同样会进入运行时输出，必须一并按文案规则审查。
    """
    parts = output_literals(code, file_name)
    if resolved:
        parts.extend(str(value) for value in resolved.values())
    return " ".join(parts)


def has_symbol_like_output(code, file_name="", resolved=None):
    """输出文案含可能引用源码标识符的词形时，需要附加源码上下文。"""
    for token in WORD_RE.findall(literal_text(code, resolved, file_name)):
        if "_" in token or (
            any(char.islower() for char in token)
            and any(char.isupper() for char in token)
        ):
            return True
    return False


def _flat(value):
    return re.sub(r"\s+", "", str(value or "")).lower()


def _name_level(name):
    """从实际调用接口名识别日志级别，不把普通单词中的子串当级别。"""
    tail = re.split(r"::|->|\.", str(name or ""))[-1]
    lower = tail.lower()
    direct = {
        "info": "info",
        "logi": "info",
        "log_info": "info",
        "warning": "warning",
        "warn": "warning",
        "logw": "warning",
        "log_warning": "warning",
        "log_warn": "warning",
        "error": "error",
        "err": "error",
        "loge": "error",
        "log_error": "error",
        "log_err": "error",
        "debug": "debug",
        "logd": "debug",
        "log_debug": "debug",
        "fatal": "fatal",
        "logf": "fatal",
        "log_fatal": "fatal",
    }
    if lower in direct:
        return direct[lower]

    # CANN 常见最终接口（GELOGI、IDE_LOGE、HIXL_LOGI）以全大写 LOGx
    # 结尾。只认这种明确形态；InfoNamedWrapper、INFO_WRAPPER 等外层名称
    # 不能代替宏链展开结果。
    if tail.isupper():
        suffixes = (
            ("LOGI", "info"),
            ("LOGW", "warning"),
            ("LOGE", "error"),
            ("LOGD", "debug"),
            ("LOGF", "fatal"),
        )
        for suffix, level in suffixes:
            if tail.endswith(suffix):
                return level
    return None


def _levels_in_code(code, ignored_calls=()):
    """提取语句或宏定义中的显式日志级别；忽略尚未展开的包装调用。"""
    value = str(code or "")
    ignored = {str(name).lower() for name in ignored_calls}
    levels = {
        {"warn": "warning", "err": "error"}.get(
            match.group(1).lower(), match.group(1).lower()
        )
        for match in LOG_STYLE_LEVEL_RE.finditer(value)
    }
    for match in CALL_NAME_RE.finditer(value):
        name = match.group(1)
        tail = re.split(r"::|->|\.", name)[-1]
        if tail.lower() in ignored:
            continue
        level = _name_level(name)
        if level:
            levels.add(level)
    return levels


def confirmed_info_level(code, macro_context=None):
    """只有直接接口或完整宏链能唯一确认最终为 INFO 时返回 True。"""
    expansion = [item for item in (macro_context or []) if isinstance(item, dict)]
    if expansion:
        wrapper_names = {str(item.get("name") or "").lower() for item in expansion}
        levels = set()
        for item in expansion:
            levels.update(
                _levels_in_code(item.get("definition", ""), ignored_calls=wrapper_names)
            )
    else:
        levels = _levels_in_code(runtime_code(code))
    return levels == {"info"}


def confirmed_level_mismatch(code, file_name="", resolved=None, macro_context=None):
    """级别错配终检：明确跨级别词和最终 INFO 级别缺一不可。"""
    text = literal_text(code, resolved, file_name)
    return bool(
        LEVEL_WORD_RE.search(text) and confirmed_info_level(code, macro_context)
    )


def detect(code, file_name="", resolved=None):
    """返回本地可确定判定的规则名集合（只含 DETERMINISTIC_RULES 中的规则）。"""
    found = set()
    text = literal_text(code, resolved, file_name)
    if _rule_exists("禁止中文") and CJK_RE.search(text):
        found.add("禁止中文")
    if _rule_exists("内部敏感词"):
        flat = _flat(text)
        for word in log_spec.blacklist():
            if _flat(word) and _flat(word) in flat:
                found.add("内部敏感词")
                break
    return found


def evidence(code, file_name="", resolved=None):
    """给模型的确定性证据，降低其推理量但不替它下结论。"""
    executable = runtime_code(code, file_name)
    text = literal_text(executable, resolved, file_name)
    sys_print = SYS_PRINT_RE.search(executable)
    hints = {
        "has_string_literal": bool(text.strip()),
        "system_print_call": sys_print.group(0).strip() if sys_print else "",
        "unified_log_call": bool(UNIFIED_HINT_RE.search(executable)),
        "info_level_with_level_word": bool(
            confirmed_info_level(executable) and LEVEL_WORD_RE.search(text)
        ),
        "has_bare_number_in_text": bool(re.search(r"(?<![\w%{])\d{2,}(?![\w}])", text)),
        "has_unit_token": bool(NUM_UNIT_RE.search(text)),
    }
    return {key: value for key, value in hints.items() if value}


PUNCTUATION_REPLACEMENTS = {
    "、": ", ",
    "，": ", ",
    "。": ".",
    "：": ": ",
    "；": "; ",
    "！": "!",
    "？": "?",
    "（": "(",
    "）": ")",
    "【": "[",
    "】": "]",
    "《": "<",
    "》": ">",
    "“": '"',
    "”": '"',
    "‘": "'",
    "’": "'",
    "　": " ",
}


def finding_details(rule, code, file_name="", resolved=None):
    """为确定性规则生成带真实证据、可直接执行的修复建议。"""
    if rule != "禁止中文":
        return (
            "本地确定性检查命中规则「%s」（输出文案证据）" % rule,
            "按规范修正该输出文案后复测。",
        )
    text = literal_text(code, resolved, file_name)
    _tmp_result = []
    for value in output_literals(code, file_name):
        if CJK_RE.search(value):
            _tmp_result.append(value)
    affected_literals = _tmp_result
    if resolved:
        affected_literals.extend(
            str(value) for value in resolved.values() if CJK_RE.search(str(value))
        )
    literal_sample = " ".join(affected_literals[:2]).strip()
    if len(literal_sample) > 180:
        literal_sample = literal_sample[:177] + "..."
    hits = list(dict.fromkeys(CJK_RE.findall(text)))
    shown = "".join(hits[:12])
    has_han = bool(HAN_RE.search("".join(hits)))
    replacements = []
    for char in hits:
        replacement = PUNCTUATION_REPLACEMENTS.get(char)
        if replacement is None and not HAN_RE.match(char):
            normalized = unicodedata.normalize("NFKC", char)
            if normalized != char:
                replacement = normalized
        if replacement is not None:
            replacements.append("`%s` 改为 `%s`" % (char, replacement))
    kind = (
        "中文字符/全角标点"
        if has_han and replacements
        else "中文字符"
        if has_han
        else "中文或全角标点"
    )
    reason = "日志输出文案含%s `%s`，不符合统一英文输出要求" % (kind, shown)
    suggestions = []
    if has_han:
        target = "输出字面量 `%s`" % literal_sample if literal_sample else "该输出文案"
        suggestions.append(
            "将%s中的中文翻译为含义准确的业内通用英文，保持 %%s、%%d、{} 等占位符及转义符不变"
            % target
        )
    if replacements:
        suggestions.append("将%s" % "、".join(replacements[:8]))
    return reason, "；".join(suggestions) or "改用英文半角字符后复测。"


def is_unit_test_source(file_name):
    """按通用目录和文件命名识别 UT 源码，不依赖仓库或文件白名单。"""
    normalized = str(file_name or "").lower().replace("\\", "/")
    parts = [part for part in normalized.split("/") if part]
    if any(part in UT_DIRECTORY_NAMES for part in parts[:-1]):
        return True
    return bool(parts and UT_FILE_RE.match(parts[-1]))


def is_ut_diagnostic_print(code, file_name=""):
    """UT 中直接 cout/printf 属测试诊断输出，不构成自定义打印整改项。"""
    if not is_unit_test_source(file_name):
        return False
    return bool(UT_DIAGNOSTIC_PRINT_RE.search(runtime_code(code, file_name)))


def is_custom_print_violation(code, file_name="", resolved=None):
    """仅报告业务调用点直接系统打印；UT 诊断输出和包装调用不报。"""
    executable = runtime_code(code, file_name)
    if not executable.strip() or MACRO_DEFINITION_RE.search(executable):
        return False
    if is_ut_diagnostic_print(executable, file_name):
        return False
    return bool(SYS_PRINT_RE.search(executable))


def deterministic_findings(log, severity_for):
    """把本地确定判定转成 findings 行。severity_for(rule) 由调用方提供。"""
    rows = []
    for rule in sorted(detect(log.get("code", ""), log.get("file", ""))):
        rows.append(
            {
                "rule": rule,
                "category": log_spec.rule_category(rule),
                "severity": severity_for(rule),
                "decided_by": "deterministic",
            }
        )
    return rows


def severity_for(rule):
    """返回规则表声明的固定严重级。"""
    try:
        return log_spec.rule_severity(rule)
    except KeyError:
        pass
    # 兼容外部扩展规则：没有固定级别时再按严重级表中的典型问题匹配。
    mapping = {}
    for item in log_spec.SEVERITIES:
        for name in re.split(r"[、,，/]", item.get("typical", "")):
            key = name.strip()
            if key:
                mapping[key] = item["name"]
    for key, value in mapping.items():
        if rule and (rule in key or key in rule):
            return value
    return log_spec.severities()[-1]


def _finding_position(row):
    return (str(row.get("file") or ""), int(row.get("line") or 0))


def guard_findings(rows, logs=None):
    """对任意模型/缓存问题清单执行统一最终护栏。

    返回 ``(保留项, 剔除原因计数)``。该函数故意放在规则模块中，保证快速、
    每日和保真全量三个输出路径使用完全相同的口径。
    """
    log_context = {}
    for log in logs or []:
        base = (str(log.get("file") or ""), int(log.get("line") or 0))
        log_context[(base[0], base[1], str(log.get("code") or ""))] = log
        log_context.setdefault(base, log)

    provisional = []
    dropped = collections.Counter()
    for original in rows:
        row = dict(original)
        code = row.get("code", "")
        file_name = row.get("file", "")
        if not runtime_code(code, file_name).strip():
            dropped["注释或文档示例"] += 1
            continue
        rule = row.get("rule")
        source = log_context.get(
            (file_name, int(row.get("line") or 0), str(code or ""))
        ) or log_context.get((file_name, int(row.get("line") or 0)), {})
        if rule == "级别错配":
            resolved = row.get("resolved_text") or source.get("resolved_text")
            macro_context = row.get("macro_context") or source.get("macro_context")
            text = literal_text(code, resolved, file_name)
            if not LEVEL_WORD_RE.search(text):
                dropped["文案无明确跨级别词"] += 1
                continue
            if not confirmed_info_level(code, macro_context):
                dropped["未确认最终日志级别为 INFO"] += 1
                continue
        elif rule == "禁止中文":
            if "禁止中文" not in detect(code, file_name):
                dropped["源码位置无中文或全角输出文案"] += 1
                continue
            reason, suggestion = finding_details(rule, code, file_name)
            row.update(
                {
                    "category": log_spec.rule_category(rule),
                    "severity": severity_for(rule),
                    "reason": reason,
                    "suggestion": suggestion,
                    "decided_by": "deterministic",
                }
            )
        elif rule == "禁止自定义打印":
            if not is_custom_print_violation(code, file_name):
                reason = (
                    "UT 测试诊断输出"
                    if is_ut_diagnostic_print(code, file_name)
                    else "自定义宏、包装调用或非直接系统打印"
                )
                dropped[reason] += 1
                continue
        provisional.append(row)

    specific_positions = {
        _finding_position(row)
        for row in provisional
        if row.get("rule") != "禁止自定义打印"
    }
    refined = []
    for row in provisional:
        if (
            row.get("rule") == "禁止自定义打印"
            and _finding_position(row) in specific_positions
        ):
            dropped["同一输出已有更具体问题"] += 1
            continue
        refined.append(row)
    return refined, dropped


def ensure_deterministic_findings(rows, logs):
    """为所有已确认日志补齐确定性问题，再执行统一最终护栏。"""
    output = [dict(row) for row in rows]
    existing = {
        (
            str(row.get("file") or ""),
            int(row.get("line") or 0),
            str(row.get("rule") or ""),
        ): index
        for index, row in enumerate(output)
    }
    for log in logs:
        code = log.get("code", "")
        file_name = log.get("file", "")
        for rule in sorted(detect(code, file_name, log.get("resolved_text"))):
            reason, suggestion = finding_details(
                rule, code, file_name, log.get("resolved_text")
            )
            row = {
                "file": file_name,
                "line": int(log.get("line") or 0),
                "end": int(log.get("end") or log.get("line") or 0),
                "code": code,
                "rule": rule,
                "category": log_spec.rule_category(rule),
                "severity": severity_for(rule),
                "reason": reason,
                "suggestion": suggestion,
                "decided_by": "deterministic",
            }
            for key in ("repo", "branch", "log_id", "source_type", "found_time"):
                if log.get(key) not in (None, ""):
                    row[key] = log[key]
            key = (file_name, row["line"], rule)
            if key in existing:
                output[existing[key]].update(row)
            else:
                existing[key] = len(output)
                output.append(row)
    return guard_findings(output, logs)


def selftest():
    ok = True

    def check(name, condition, detail=""):
        nonlocal ok
        ok = ok and bool(condition)
        print("%s %s %s" % ("[ok]  " if condition else "[FAIL]", name, detail))

    check(
        "中文文案判为禁止中文", "禁止中文" in detect('LOGE("初始化失败 code=%d", ret);')
    )
    check(
        "宏名含中文之外的标识符不误判",
        "禁止中文" not in detect('LOGE("init failed code=%d", ret);'),
    )
    check(
        "注释式中文不在字面量中则不判", "禁止中文" not in detect('LOGE("init failed");')
    )
    check(
        "黑名单命中（忽略大小写与空格）",
        "内部敏感词" in detect('LOGE("inner  error happened");'),
    )
    check(
        "变量名叫 password 但未打印真实值不本地判敏感信息",
        not detect('LOGD("password check start");') - {"禁止中文"},
    )
    check(
        "系统打印只作为证据不本地拍板",
        "禁止自定义打印" not in detect('printf("done\\n");')
        and evidence('printf("done\\n");').get("system_print_call") == "printf",
    )
    check(
        "统一日志接口可识别", evidence('LOGI("ready");').get("unified_log_call") is True
    )
    check(
        "INFO 含 failed 作为级别错配证据",
        evidence('LOGI("open failed");').get("info_level_with_level_word") is True,
    )
    check(
        "INFO 普通否定描述不作为级别错配证据",
        not evidence('LOGI("Can not get optional cache");').get(
            "info_level_with_level_word"
        ),
    )
    check(
        "ERROR 含 failed 不作为级别错配证据",
        not evidence('LOGE("open failed");').get("info_level_with_level_word"),
    )
    check(
        "规则名来自规范文件",
        all(name in log_spec.rule_names() for name in DETERMINISTIC_RULES),
    )
    print("总判定:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(selftest())
