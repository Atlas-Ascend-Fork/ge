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

"""CANN 日志规范评测引擎。

第一阶段只识别全部日志（Flash 使用低强度 Thinking）；第二阶段仅上传已识别日志并开启 Thinking，
完成规则和严重级判断。每次送审都以完整源码文件为原子单元；文件超过模型输入上限
时直接失败，不截断、不分段。规则判断不重复上传源码。
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402
import det_rules  # noqa: E402
import prescan  # noqa: E402
import model_config  # noqa: E402

# Bump when request-unit semantics change so old segmented progress is never
# reused as if it represented an atomic whole-file request.
EVALUATOR_VERSION = "25"
DEFAULT_MAX_INPUT_CHARS = 1000000
DEFAULT_PLANNING_OUTPUT_TOKENS = 32768
MAX_MANIFEST_CHARS = 4096

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass

CODE_EXT = log_spec.scan_extensions()
SKIP_DIRS = set(log_spec.exclude_dirs())
PROMPT_TEMPLATE = """你是 CANN 源码日志识别专家。下面给出的是若干个完整源文件
。此阶段只识别日志，不做规范判断。

【日志识别要求】
1. 不使用接口白名单，也不要只找名称含 log 的调用。项目自定义宏、包装函数、
   回调、事件上报、标准输出/错误输出及其它实际承担日志、状态或调试输出的语句
   都属于识别范围。要结合完整文件内的定义、调用和数据流判断。
2. 跨多行的调用算一条，line/end 填完整语句起止行；单行语句的 end 等于 line。
3. 注释、logger 初始化、数学 log/logits、普通字符串常量和不产生输出的代码不算。
4. logs 必须列出本请求源码单元中的全部日志语句，包括没有违规的语句。不得抽样、
   省略或只返回有问题的日志。

【输出格式】
严格只输出一个 JSON 对象，不要解释文字：
{{
  "files": [
    {{"file": "相对路径"}}
  ],
  "logs": [
    {{"id": "本响应内唯一ID", "file": "相对路径", "line": 起始行号, "end": 结束行号}}
  ]
}}

files 必须覆盖本请求每个源码单元且各出现一次。
没有日志时仍返回完整 files 和空 logs。logs.id 在本响应内必须唯一；
file、line 和 end 必须来自源码。
不要在 JSON 中重复抄源码；脚本会按 line/end 从原文件无损还原 code，节省输出空间。

【待评测源码单元】共 {file_count} 个
{code}"""

FILE_TEMPLATE = """
--- 源码单元 {idx} | 路径: {file} | 来源: {source_type} | 行号范围: {start}-{end} ---
```{language}
{body}```
"""

# The cache key retains the original semantic prompt, so successful requests
# from an earlier run remain reusable when only the response encoding changes.
COMPACT_PROMPT_TEMPLATE = (
    PROMPT_TEMPLATE.replace('"file": "相对路径"', '"index": 源码单元序号')
    .replace(
        "files 必须覆盖本请求每个源码单元且各出现一次。",
        "files 须覆盖全部源码单元序号且不重复。",
    )
    .replace("file、line 和 end 必须来自源码。", "index、line 和 end 必须来自源码。")
)

RULE_PROMPT_TEMPLATE = """你是 CANN 日志规范评测专家。以下日志已经由源码识别阶段
完成定位和原文还原。请只依据输入日志评测规范问题，不重新识别日志，不猜测源码外信息。

【审查边界】
1. 只审查运行时真正输出给用户或日志文件的文案。宏名、函数名、变量名、错误码
   变量和格式占位符不是输出文案。
2. 每个问题先复核：证据确实进入输出、未命中放过条件、上下文足够。不确定就不报。
3. 文案中的驼峰、下划线等标识符词形必须先用 context 核对源码真实符号；无法确认
   它是普通英文单词时，不报拼写错误，也不得擅自改成相似单词。
4. 每条日志的 macro_context 是按展开顺序排列的宏定义编号；从下方共享宏定义表中
   查阅完整定义，沿包装链追到最终接口判断级别。不能按外层宏名猜测；链不完整或
   最终级别不明时不报级别错配。
5. 敏感信息问题的 reason 和 suggestion 不得复述真实密码、token 或密钥值，统一以
   [REDACTED] 代替敏感值。

【{rule_count} 条规范】
{rules_section}

【判定尺度】
{scale_section}

【严重级】
{severities_section}

规则清单中的固定严重级是强制值；每个 issue 的 severity 必须与对应规则一致。

严格只输出一个 JSON 对象，不要解释文字：
{{"issues":[{{"log_id":"对应输入日志id","rule":"{rule_name_list}",
"severity":"{severity_list_slash}","reason":"具体问题","suggestion":"修复建议"}}]}}

每项必须引用输入中真实存在的 log_id。没有问题返回 {{"issues":[]}}。

【共享宏定义】按编号引用；同一条日志的 macro_context 顺序代表展开链
{macro_json}

【待评测日志】共 {log_count} 条
{logs_json}"""

REPOSITORY_PROMPT_TEMPLATE = """你是 CANN 日志规范评测专家。以下是同一仓库各完整
源码文件首次评测时已经识别出的**全部日志语句**。源码文件不会再次上传。

请只评测需要全仓视角的规则，不重新识别日志，不评测其它规则：
{rules_section}

【判定尺度】
{scale_section}

【严重级】
{severities_section}

规则清单中的固定严重级是强制值；每个 issue 的 severity 必须与对应规则一致。

严格只输出一个 JSON 对象，不要解释文字：
{{"issues":[{{"log_id":"对应输入日志id","rule":"{rule_name_list}",
"severity":"{severity_list_slash}","reason":"具体问题","suggestion":"修复建议"}}]}}

每项必须引用输入中真实存在的 log_id。没有问题返回 {{"issues":[]}}。

【全仓完整日志清单】共 {log_count} 条
{logs_json}"""

SEVERITIES = log_spec.severities()
RULE_NAMES = log_spec.rule_names()
REPOSITORY_RULE_NAMES = log_spec.repository_rule_names()


class InputLimitError(ValueError):
    """A complete atomic file cannot fit in the configured model input limit."""

    def __init__(self, file_name, size, limit):
        self.file_name = file_name
        self.size = size
        self.limit = limit
        super().__init__(
            "完整文件 %s 单独送审仍超过输入硬上限（%d > %d 字符）。"
            "工具不会截断或拆分文件；请提高 --max-input-chars 或使用更大上下文模型。"
            % (file_name, size, limit)
        )


def blacklist():
    return log_spec.blacklist()


def _csv(value):
    return [item.strip() for item in str(value or "").split(",") if item.strip()]


def configured_extensions(value):
    out = []
    for ext in _csv(value):
        ext = ext.lower()
        if not ext.startswith("."):
            ext = "." + ext
        if ext not in out:
            out.append(ext)
    return tuple(out)


def iter_code_files(root, extensions, excluded):
    root = os.path.abspath(root)
    if os.path.isfile(root):
        if os.path.splitext(root)[1].lower() in extensions:
            yield root
        return
    for dp, dns, fns in os.walk(root):
        dns[:] = sorted(d for d in dns if d not in excluded)
        for name in sorted(fns):
            if os.path.splitext(name)[1].lower() in extensions:
                yield os.path.join(dp, name)


def render_numbered(lines, start=1):
    return "".join(
        "%d: %s" % (index, line if line.endswith("\n") else line + "\n")
        for index, line in enumerate(lines, start)
    )


def read_source(path, root):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            lines = fh.readlines()
    except OSError as exc:
        raise RuntimeError("读取源码失败 %s: %s" % (path, exc)) from exc
    if os.path.isfile(root):
        rel = os.path.basename(path)
    else:
        rel = os.path.relpath(path, root).replace("\\", "/")
    numbered = render_numbered(lines)
    source = "".join(lines)
    return {
        "file": rel,
        "review_file": rel,
        "start": 1,
        "end": max(1, len(lines)),
        "lines": lines,
        "source": source,
        "text": numbered,
        "sig": hashlib.sha256(source.encode("utf-8", "replace")).hexdigest(),
        "language": os.path.splitext(path)[1].lstrip(".") or "text",
        # 按被评测仓内的相对路径判定来源，避免检出目录名（如 /tests/、
        # /tools/）把整仓源码误判为测试或工具脚本。
        "source_type": classify_source(rel),
    }


def render_file(record, index):
    return FILE_TEMPLATE.format(
        idx=index,
        file=record.get("review_file", record["file"]),
        start=record.get("start", 1),
        end=record["end"],
        source_type=record.get("source_type", classify_source(record["file"])),
        language=record["language"],
        body=record["text"],
    )


def _format_prompt(file_count, code, bl, rule_names=None):
    return PROMPT_TEMPLATE.format(file_count=file_count, code=code)


def render_prompt(files, bl, rule_names=None):
    code = "".join(render_file(record, index) for index, record in enumerate(files, 1))
    return COMPACT_PROMPT_TEMPLATE.format(file_count=len(files), code=code)


def prompt_size(files, bl, rule_names=None):
    segments = sum(
        len(render_file(record, index)) for index, record in enumerate(files, 1)
    )
    return len(_format_prompt(len(files), "", bl, rule_names)) + segments


def estimate_output_tokens(
    char_count, density_per_million, tokens_per_log, manifest_files=0
):
    """按实测日志密度预估一个请求的输出 token，用于提前发现输出风险。"""
    logs = char_count / 1000000.0 * float(density_per_million)
    return int(logs * float(tokens_per_log) + manifest_files * 12 + 64)


def output_budget_chars(
    max_output_tokens, density_per_million, tokens_per_log, safety_ratio=0.6
):
    """反算单请求可容纳的源码字符数，使预期输出落在上限的安全比例内。"""
    if max_output_tokens is None or density_per_million <= 0 or tokens_per_log <= 0:
        return None
    budget = max(1, int(max_output_tokens * safety_ratio))
    return int(
        budget / (float(density_per_million) * float(tokens_per_log)) * 1000000.0
    )


def pack_files(
    files,
    max_input_chars,
    bl,
    rule_names=None,
    target_input_chars=None,
    max_manifest_chars=None,
):
    """按完整提示词与返回清单开销打包；文件不可拆分。"""
    hard_limit = DEFAULT_MAX_INPUT_CHARS if max_input_chars is None else max_input_chars
    requests, current, current_segments = [], [], 0
    current_manifest_chars = 0
    if target_input_chars is None:
        raise ValueError("必须设置装包目标；它不是模型输入硬上限")
    for record in files:
        single_segment = len(render_file(record, 1))
        single_size = len(_format_prompt(1, "", bl, rule_names)) + single_segment
        if single_size > hard_limit:
            raise InputLimitError(record["file"], single_size, hard_limit)
        next_segment = len(render_file(record, len(current) + 1))
        next_manifest_chars = len(str(len(current) + 1)) + 20
        proposed_size = (
            len(_format_prompt(len(current) + 1, "", bl, rule_names))
            + current_segments
            + next_segment
        )
        manifest_full = (
            max_manifest_chars is not None
            and current_manifest_chars + next_manifest_chars > max_manifest_chars
        )
        if current and (proposed_size > target_input_chars or manifest_full):
            requests.append(current)
            current, current_segments = [record], single_segment
            current_manifest_chars = 21
        else:
            current.append(record)
            current_segments += next_segment
            current_manifest_chars += next_manifest_chars
    if current:
        requests.append(current)

    expected = [record.get("review_file", record["file"]) for record in files]
    actual = [
        record.get("review_file", record["file"])
        for request in requests
        for record in request
    ]
    if actual != expected or len(actual) != len(set(actual)):
        raise AssertionError("文件打包完整性校验失败：存在遗漏、重复或乱序")
    for request in requests:
        size = prompt_size(request, bl, rule_names)
        if size > hard_limit:
            raise AssertionError("请求输入超过基线: %d > %d" % (size, hard_limit))
    return requests


_TRACE_LOCK = threading.Lock()
_TRACE_SEQUENCE = [0]


def extra_api_headers():
    """解析 LLM_API_EXTRA_HEADERS（JSON 对象），用于需要额外鉴权头的网关。

    返回 {header_name: value}；未设置时返回空字典。头名/头值中的换行会被拒绝，
    避免把密钥意外写进日志或产生非法 HTTP 头。
    """
    raw = os.environ.get("LLM_API_EXTRA_HEADERS", "").strip()
    if not raw:
        return {}
    try:
        value = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise RuntimeError("LLM_API_EXTRA_HEADERS 必须是 JSON 对象") from exc
    if not isinstance(value, dict):
        raise RuntimeError("LLM_API_EXTRA_HEADERS 必须是 JSON 对象")
    headers = {}
    for raw_name, raw_value in value.items():
        name = str(raw_name).strip()
        if not name or any(char in name for char in "\r\n:"):
            raise RuntimeError("LLM_API_EXTRA_HEADERS 含非法请求头名称")
        text = str(raw_value)
        if "\r" in text or "\n" in text:
            raise RuntimeError("LLM_API_EXTRA_HEADERS 的请求头值不能包含换行")
        headers[name] = text
    return headers


def _infer_trace_label(prompt):
    """按提示词特征推断阶段名，避免调用方（含测试桩）额外传参。"""
    text = str(prompt or "")
    head = text[:300]
    if "源码日志识别专家" in head:
        return "extraction"
    if "【全仓完整日志清单】" in text:
        return "repository_review"
    if "【待评测日志】" in text:
        return "rule_review"
    return "call"


def _write_trace(
    label, model, prompt, content=None, usage=None, error=None, request_meta=None
):
    """把一次请求/响应写到 LLM_TRACE_DIR（未设置时静默跳过）。

    只记录 prompt、响应正文、usage 与请求参数；绝不写 Authorization、
    x-api-vkey 等鉴权头或 API Key。
    """
    directory = os.environ.get("LLM_TRACE_DIR", "").strip()
    if not directory:
        return
    try:
        os.makedirs(directory, exist_ok=True)
        with _TRACE_LOCK:
            _TRACE_SEQUENCE[0] += 1
            sequence = _TRACE_SEQUENCE[0]
        digest = hashlib.sha256(str(prompt).encode("utf-8", "replace")).hexdigest()[:8]
        label = (label or "call").replace(" ", "_")
        name = "%04d-%s-%s.json" % (sequence, label, digest)
        payload = {
            "timestamp": datetime.now().astimezone().isoformat(timespec="milliseconds"),
            "label": label,
            "model": model,
            "request": dict(request_meta or {"prompt": prompt}),
            "response": (
                None
                if error is not None
                else {
                    "content": content,
                    "usage": dict(usage or {}),
                    "response_model": (usage or {}).get("_response_model"),
                }
            ),
            "error": None if error is None else str(error),
        }
        with open(os.path.join(directory, name), "w", encoding="utf-8") as handle:
            json.dump(payload, handle, ensure_ascii=False, indent=2)
    except OSError:
        pass


def call_llm(
    api,
    key,
    model,
    prompt,
    max_tokens,
    timeout,
    retries,
    thinking="disabled",
    reasoning_effort=None,
    trace_label="",
):
    thinking, reasoning_effort = model_config.request_settings(
        model, thinking, reasoning_effort
    )
    extra_headers = extra_api_headers()
    request_meta = {
        "prompt": prompt,
        "max_tokens": max_tokens,
        "thinking": thinking,
        "reasoning_effort": reasoning_effort,
    }
    if not trace_label:
        trace_label = _infer_trace_label(prompt)
    url = api.rstrip("/") + "/chat/completions"
    payload = {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "do_sample": False,
        "thinking": {"type": thinking},
        "response_format": {"type": "json_object"},
    }
    if max_tokens is not None:
        payload["max_tokens"] = max_tokens
    if thinking == "enabled" and reasoning_effort:
        payload["reasoning_effort"] = reasoning_effort
    body = json.dumps(payload).encode("utf-8")
    last = None
    for attempt in range(retries + 1):
        try:
            request = urllib.request.Request(url, data=body, method="POST")
            request.add_header("Content-Type", "application/json")
            request.add_header("Authorization", "Bearer %s" % key)
            request.add_header(
                "User-Agent", "cann-log-evaluation/%s" % EVALUATOR_VERSION
            )
            for header_name, header_value in extra_headers.items():
                request.add_header(header_name, header_value)
            with urllib.request.urlopen(request, timeout=timeout) as response:
                data = json.loads(response.read().decode("utf-8"))
            choice = (data.get("choices") or [{}])[0]
            if choice.get("finish_reason") == "length":
                raise RuntimeError("模型输出被截断（finish_reason=length）")
            content = ((choice.get("message") or {}).get("content") or "").strip()
            if not content:
                raise RuntimeError("模型返回空正文")
            if (
                os.environ.get("LLM_REQUIRE_MODEL_MATCH", "").lower()
                in ("1", "true", "yes", "on")
                and data.get("model") != model
            ):
                raise model_config.ModelMismatchError(
                    "Requested model %s, response model %s" % (model, data.get("model"))
                )
            usage = dict(data.get("usage") or {})
            # Keep the server's actual model alongside token counters so
            # wrappers can report aliases and verify the requested deployment.
            if data.get("model"):
                usage["_response_model"] = data["model"]
            _write_trace(
                trace_label, model, prompt, content, usage, request_meta=request_meta
            )
            return content, usage
        except Exception as exc:  # noqa: BLE001
            last = exc
            if isinstance(exc, model_config.ModelMismatchError):
                break
            if isinstance(exc, urllib.error.HTTPError):
                detail = (
                    exc.read(2000).decode("utf-8", "replace").replace(key, "<redacted>")
                )
                last = RuntimeError("HTTP %d: %s" % (exc.code, detail))
                if exc.code < 500 and exc.code != 429:
                    break
            if "输出被截断" in str(exc) or "max_tokens" in str(exc):
                break
            if attempt < retries:
                delay = min(2**attempt * 3, 30)
                if isinstance(exc, urllib.error.HTTPError):
                    retry_after = (exc.headers or {}).get("Retry-After", "")
                    if str(retry_after).isdigit():
                        delay = min(max(delay, int(retry_after)), 60)
                time.sleep(delay)
    _write_trace(trace_label, model, prompt, error=last, request_meta=request_meta)
    raise last


def parse_result(text):
    value = (text or "").strip()
    if value.startswith("```"):
        parts = value.split("```")
        value = parts[1] if len(parts) > 1 else value
        if value.lstrip().startswith("json"):
            value = value.lstrip()[4:]
    start, end = value.find("{"), value.rfind("}")
    if start < 0 or end < start:
        raise ValueError("响应不是 JSON 对象")
    try:
        result = json.loads(value[slice(start, end + 1, None)])
    except json.JSONDecodeError as exc:
        raise ValueError("响应 JSON 无法解析: %s" % exc) from exc
    if not isinstance(result, dict):
        raise ValueError("响应顶层必须是对象")
    if not isinstance(result.get("files"), list) or not isinstance(
        result.get("logs"), list
    ):
        raise ValueError("日志识别响应必须同时包含 files 和 logs 数组")
    return result["files"], result["logs"]


def parse_repository_result(text):
    value = (text or "").strip()
    if value.startswith("```"):
        parts = value.split("```")
        value = parts[1] if len(parts) > 1 else value
        if value.lstrip().startswith("json"):
            value = value.lstrip()[4:]
    start, end = value.find("{"), value.rfind("}")
    if start < 0 or end < start:
        raise ValueError("全仓复核响应不是 JSON 对象")
    try:
        result = json.loads(value[slice(start, end + 1, None)])
    except json.JSONDecodeError as exc:
        raise ValueError("全仓复核 JSON 无法解析: %s" % exc) from exc
    if not isinstance(result, dict) or not isinstance(result.get("issues"), list):
        raise ValueError("全仓复核响应必须包含 issues 数组")
    return result["issues"]


def _compact(value):
    return re.sub(r"\s+", "", str(value or ""))


def _message_signature(code):
    literals = re.findall(r'(?s)(?:u8|u|U|L)?"((?:\\.|[^"\\])*)"', str(code or ""))
    text = " ".join(literals) if literals else str(code or "")
    return re.sub(r"\s+", " ", text).strip().lower()


def _source_index(files):
    return {record.get("review_file", record["file"]): record for record in files}


def _resolve_file(name, index):
    if name in index:
        return name
    hits = [
        path
        for path in index
        if path.endswith("/" + name) or os.path.basename(path) == os.path.basename(name)
    ]
    return hits[0] if len(hits) == 1 else None


def _response_file(item, files, index):
    if "index" not in item:
        return _resolve_file(str(item.get("file", "")).strip(), index)
    unit = item["index"]
    if isinstance(unit, str) and unit.strip().isdigit():
        unit = int(unit.strip())
    if type(unit) is not int or not 1 <= unit <= len(files):
        raise ValueError("源码单元序号无效: %r" % item["index"])
    file_name = files[unit - 1].get("review_file", files[unit - 1]["file"])
    if "file" in item and _resolve_file(str(item["file"]).strip(), index) != file_name:
        raise ValueError("源码单元序号与文件名不一致: %r" % item)
    return file_name


def validate_log(item, files):
    if not isinstance(item, dict):
        raise ValueError("logs 中存在非对象条目")
    log_id = str(item.get("id", "")).strip()
    if not log_id:
        raise ValueError("logs 缺少唯一 id")
    index = _source_index(files)
    file_name = _response_file(item, files, index)
    if not file_name:
        raise ValueError("logs 文件名无法定位: %r" % item.get("file"))
    line = item.get("line")
    if isinstance(line, str) and line.strip().isdigit():
        line = int(line.strip())
    end = item.get("end", line)
    if isinstance(end, str) and end.strip().isdigit():
        end = int(end.strip())
    record = index[file_name]
    source_start = record.get("start", 1)
    if (
        not isinstance(line, int)
        or not isinstance(end, int)
        or not source_start <= line <= end <= record["end"]
    ):
        raise ValueError("logs 行号范围越界: %s:%r-%r" % (file_name, line, end))
    code = "".join(
        record["lines"][slice(line - source_start, end - source_start + 1, None)]
    ).rstrip("\r\n")
    if not code.strip():
        raise ValueError("logs 指向空源码: %s:%d-%d" % (file_name, line, end))
    result = {
        "id": log_id,
        "file": record["file"],
        "review_file": file_name,
        "line": line,
        "end": end,
        "code": code,
        "source_type": record.get("source_type", classify_source(file_name)),
    }
    if det_rules.has_symbol_like_output(code, record["file"]):
        context_start = max(source_start, line - 3)
        context_end = min(record["end"], end + 3)
        context = "".join(
            record["lines"][
                slice(
                    context_start - source_start, context_end - source_start + 1, None
                )
            ]
        ).rstrip("\r\n")
        if context and context != code:
            result["context_start"] = context_start
            result["context"] = context
    return result


def validate_result(manifest, logs, issues, files, rule_names=None):
    active_rules = tuple(rule_names) if rule_names is not None else RULE_NAMES
    tolerant = os.environ.get("LLM_TOLERATE_MODEL_ERRORS", "").lower() in (
        "1",
        "true",
        "yes",
        "on",
    )
    if any(not isinstance(item, dict) for item in manifest):
        if not tolerant:
            raise ValueError("files 中存在非对象条目")
        manifest = [item for item in manifest if isinstance(item, dict)]
    index = _source_index(files)
    reviewed = []
    for item in manifest:
        try:
            file_name = _response_file(item, files, index)
        except ValueError:
            if tolerant:
                continue
            raise
        if not file_name:
            if tolerant:
                continue
            raise ValueError("files 文件名无法定位: %r" % item.get("file"))
        if file_name in reviewed:
            if tolerant:
                continue
            raise ValueError("files 重复返回文件: %s" % file_name)
        reviewed.append(file_name)
    expected = [record.get("review_file", record["file"]) for record in files]
    if set(reviewed) != set(expected) or len(reviewed) != len(expected):
        missing = sorted(set(expected) - set(reviewed))
        extra = sorted(set(reviewed) - set(expected))
        if not tolerant:
            raise ValueError(
                "files 未完整覆盖请求文件；缺少=%s 多余=%s" % (missing, extra)
            )

    valid_logs = []
    seen_ids = set()
    for item in logs:
        try:
            item = validate_log(item, files)
        except (TypeError, ValueError):
            if tolerant:
                continue
            raise
        if item["id"] in seen_ids:
            if tolerant:
                continue
            raise ValueError("logs.id 重复: %s" % item["id"])
        seen_ids.add(item["id"])
        valid_logs.append(item)
    valid_issues = []
    for item in issues:
        if not isinstance(item, dict):
            raise ValueError("issues 中存在非对象条目")
        log_id = str(item.get("log_id", "")).strip()
        log_item = next((log for log in valid_logs if log["id"] == log_id), None)
        if log_item is None:
            raise ValueError("issues.log_id 未对应 logs: %r" % item.get("log_id"))
        item = dict(item)
        item.update(
            log_id=log_id,
            file=log_item["file"],
            line=log_item["line"],
            end=log_item["end"],
            code=log_item["code"],
            source_type=log_item["source_type"],
        )
        rule = log_spec.normalize_rule(str(item.get("rule", "")).strip())
        severity = str(item.get("severity", "")).strip()
        if rule not in active_rules:
            raise ValueError("issues 返回未知规则: %r" % item.get("rule"))
        if severity not in SEVERITIES:
            raise ValueError("issues 返回未知严重级: %r" % severity)
        expected_severity = log_spec.rule_severity(rule)
        if severity != expected_severity:
            raise ValueError(
                "issues 规则严重级不匹配: %s 应为 %s，实际为 %s"
                % (rule, expected_severity, severity)
            )
        if (
            not str(item.get("reason", "")).strip()
            or not str(item.get("suggestion", "")).strip()
        ):
            raise ValueError("issues 缺少 reason 或 suggestion")
        item["rule"], item["severity"] = rule, severity
        item["category"] = log_spec.rule_category(rule)
        valid_issues.append(item)
    return valid_logs, valid_issues


def numeric_table_audit(record):
    source = record.get("source", "")
    # C++ line splicing precedes comment removal; this narrow grammar does
    # not accept preprocessing escapes or trigraphs.
    if "\\" in source or "??/" in source:
        return None
    # Preserve quoted tokens when removing comments; a quoted comment marker
    # must never hide executable code from the whole-file grammar check.
    without_comments = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: " " if match.group().startswith(("//", "/*")) else match.group(),
        source,
    )
    match = re.fullmatch(
        r"\s*(?:#pragma\s+once\s*|#include\s*<(?:array|cstdint)>\s*)+"
        r"namespace\s+[A-Za-z_]\w*\s*\{\s*"
        r"inline\s+constexpr\s+std::array\s*<\s*(?:std::)?(?:u?int64_t)\s*,\s*"
        r"(\d+)\s*>\s+[A-Za-z_]\w*\s*=\s*\{([0-9,\s]*)\}\s*;\s*\}\s*",
        without_comments,
    )
    if match is None:
        return None
    parts = match.group(2).split(",")
    if parts and not parts[-1].strip():
        parts.pop()
    if (
        not parts
        or len(parts) != int(match.group(1))
        or any(re.fullmatch(r"[0-9]+", part.strip()) is None for part in parts)
    ):
        return None
    return {
        "kind": "numeric_constant_table",
        "file": record["file"],
        "entries": len(parts),
        "log_statements": 0,
        "source_sha256": hashlib.sha256(source.encode("utf-8")).hexdigest(),
        "lines": len(source.splitlines()),
        "whole_file_verified": True,
    }


def numeric_audits(payload):
    audits = list(
        payload.get("_numeric_table_audits")
        or payload.get("numeric_table_audits")
        or []
    )
    singleton = payload.get("_numeric_table_audit") or payload.get(
        "numeric_table_audit"
    )
    if singleton:
        audits.append(singleton)
    return audits


def review_split_request(
    files, api, key, model, bl, rule_names, max_tokens, timeout, retries, hard_limit
):
    midpoint = len(files) // 2
    merged_logs, errors, usage = [], [], {}
    response_models = set()
    started = time.monotonic()
    for chunk in (files[:midpoint], files[midpoint:]):
        _, logs, _, error, chunk_usage, _ = review_request(
            (
                chunk,
                api,
                key,
                model,
                bl,
                rule_names,
                max_tokens,
                timeout,
                retries,
                hard_limit,
            )
        )
        for row in logs:
            row = dict(row, id="%s:%d-%d" % (row["file"], row["line"], row["end"]))
            merged_logs.append(row)
        if error:
            errors.append(error)
        audits = numeric_audits(chunk_usage)
        if audits:
            usage.setdefault("_numeric_table_audits", []).extend(audits)
        for name, value in chunk_usage.items():
            if isinstance(value, int):
                usage[name] = usage.get(name, 0) + value
            elif name == "_response_model" and value:
                response_models.add(value)
    if len(response_models) == 1:
        usage["_response_model"] = next(iter(response_models))
    return (
        files,
        merged_logs,
        [],
        "; ".join(errors) if errors else None,
        usage,
        round(time.monotonic() - started, 3),
    )


def review_request(args_tuple):
    files, api, key, model, bl, rule_names, max_tokens, timeout, retries = args_tuple[
        :9
    ]
    hard_limit = args_tuple[9] if len(args_tuple) > 9 else DEFAULT_MAX_INPUT_CHARS
    repair_error = args_tuple[10] if len(args_tuple) > 10 else ""
    if len(files) == 1 and os.environ.get("LLM_AUDIT_NUMERIC_TABLES", "").lower() in (
        "1",
        "true",
        "yes",
        "on",
    ):
        audit = numeric_table_audit(files[0])
        if audit:
            return files, [], [], None, {"_numeric_table_audit": audit}, 0.0
    prompt = render_prompt(files, bl, rule_names)
    adaptive = os.environ.get("LLM_ADAPTIVE_SPLIT", "").lower() in (
        "1",
        "true",
        "yes",
        "on",
    )
    recovery_target = int(os.environ.get("LLM_RECOVERY_INPUT_CHARS", "250000"))
    if adaptive and len(files) > 1 and len(prompt) > recovery_target:
        return review_split_request(
            files,
            api,
            key,
            model,
            bl,
            rule_names,
            max_tokens,
            timeout,
            retries,
            hard_limit,
        )
    if repair_error:
        prompt += (
            "\n\n【上次响应校验失败】%s\n请重新核对本源码单元，只返回真实、"
            "非空、行号范围内的日志位置，并保持 files 完整覆盖。" % repair_error
        )
    started = time.time()
    try:
        if len(prompt) > hard_limit:
            raise ValueError(
                "日志识别请求超过输入硬上限: %d > %d 字符" % (len(prompt), hard_limit)
            )
        content, usage = call_llm(
            api,
            key,
            model,
            prompt,
            max_tokens=max_tokens,
            timeout=timeout,
            retries=retries,
            thinking="disabled",
        )
        manifest, logs = parse_result(content)
        logs, _issues = validate_result(manifest, logs, [], files, ())
        return files, logs, [], None, usage, round(time.time() - started, 3)
    except Exception as exc:  # noqa: BLE001
        splittable = any(
            text in str(exc)
            for text in (
                "输出被截断",
                "max_tokens",
                "files 未完整覆盖",
                "files 文件名",
                "files 重复",
                "logs 指向空源码",
                "logs 行号范围越界",
                "logs 文件名",
                "源码单元序号",
                "响应 JSON 无法解析",
                "响应不是 JSON",
            )
        )
        truncated = any(text in str(exc) for text in ("输出被截断", "max_tokens"))
        if splittable and not repair_error and (len(files) == 1 or not truncated):
            print(
                "    %s响应校验失败，附带错误反馈后同组修复重试: %s"
                % ("单文件" if len(files) == 1 else "多文件", exc),
                flush=True,
            )
            return review_request(
                (
                    files,
                    api,
                    key,
                    model,
                    bl,
                    rule_names,
                    max_tokens,
                    timeout,
                    retries,
                    hard_limit,
                    str(exc),
                )
            )
        # A single hallucinated location can invalidate an otherwise valid
        # multi-file response. During recovery runs, split only after the
        # same-batch repair has failed so successful checkpoints remain
        # reusable and each source file stays atomic.
        transient = isinstance(
            exc, (urllib.error.URLError, TimeoutError, ConnectionError, OSError)
        ) or any(
            text in str(exc)
            for text in (
                "HTTP 502",
                "HTTP 503",
                "HTTP 504",
                "IncompleteRead",
                "Remote end closed",
                "输出被截断",
            )
        )
        if adaptive and len(files) > 1 and ((splittable and repair_error) or transient):
            midpoint = max(1, len(files) // 2)
            print(
                "    响应修复仍失败，自适应拆分重审: %d+%d 个源码单元"
                % (midpoint, len(files) - midpoint),
                flush=True,
            )
            return review_split_request(
                files,
                api,
                key,
                model,
                bl,
                rule_names,
                max_tokens,
                timeout,
                retries,
                hard_limit,
            )
        return (
            files,
            [],
            [],
            "%s: %s" % (type(exc).__name__, exc),
            {},
            round(time.time() - started, 3),
        )


def rule_prompt(logs, bl, rule_names):
    payload = []
    macro_ids, macro_definitions = {}, {}
    for item in logs:
        value = {
            "id": item["global_id"],
            "file": item["file"],
            "line": item["line"],
            "end": item["end"],
            "code": item["code"],
            "source_type": item["source_type"],
        }
        if item.get("context"):
            value["context_start"] = item.get("context_start", item["line"])
            value["context"] = item["context"]
        if item.get("macro_context"):
            refs = []
            for definition in item["macro_context"]:
                identity = json.dumps(
                    definition,
                    ensure_ascii=False,
                    sort_keys=True,
                    separators=(",", ":"),
                )
                if identity not in macro_ids:
                    macro_id = "M%d" % (len(macro_ids) + 1)
                    macro_ids[identity] = macro_id
                    macro_definitions[macro_id] = definition
                refs.append(macro_ids[identity])
            value["macro_context"] = refs
        payload.append(value)
    return RULE_PROMPT_TEMPLATE.format(
        rule_count=len(rule_names),
        rules_section=log_spec.prompt_rules(bl, rule_names),
        scale_section=log_spec.prompt_scale(rule_names),
        severities_section=log_spec.prompt_severities(),
        rule_name_list="/".join(rule_names),
        severity_list_slash="/".join(SEVERITIES),
        log_count=len(payload),
        macro_json=json.dumps(
            macro_definitions, ensure_ascii=False, separators=(",", ":")
        ),
        logs_json=json.dumps(payload, ensure_ascii=False, separators=(",", ":")),
    )


def pack_logs(logs, max_input_chars, bl, rule_names, target_input_chars):
    groups, current = [], []
    for item in logs:
        single_size = len(rule_prompt([item], bl, rule_names))
        if max_input_chars is not None and single_size > max_input_chars:
            raise ValueError("单条日志规则评测超过输入硬上限: %s" % item["global_id"])
        if (
            current
            and len(rule_prompt(current + [item], bl, rule_names)) > target_input_chars
        ):
            groups.append(current)
            current = [item]
        else:
            current.append(item)
    if current:
        groups.append(current)
    return groups


def pack_repository_groups(duplicate_groups, target_input_chars, bl, rule_names):
    """保留每组首条作比较基准，拆分过大的重复日志组。"""
    packed = []
    for group in duplicate_groups:
        anchor, current = group[0], [group[0]]
        for item in group[1:]:
            candidate = current + [item]
            if (
                len(current) > 1
                and len(rule_prompt(candidate, bl, rule_names)) > target_input_chars
            ):
                packed.append(current)
                current = [anchor, item]
            else:
                current = candidate
        if len(current) > 1:
            packed.append(current)
    return packed


def validate_log_issues(issues, logs, rule_names):
    by_id = {item["global_id"]: item for item in logs}
    tolerant = os.environ.get("LLM_TOLERATE_MODEL_ERRORS", "").lower() in (
        "1",
        "true",
        "yes",
        "on",
    )
    valid, seen = [], set()
    for item in issues:
        if not isinstance(item, dict):
            if tolerant:
                continue
            raise ValueError("规则评测 issues 中存在非对象条目")
        log_id = str(item.get("log_id", "")).strip()
        rule = log_spec.normalize_rule(str(item.get("rule", "")).strip())
        source = by_id.get(log_id)
        if source is None and rule in REPOSITORY_RULE_NAMES:
            merged_ids = [
                value.strip()
                for value in re.split(r"[,，;；]", log_id)
                if value.strip()
            ]
            if len(merged_ids) > 1 and all(value in by_id for value in merged_ids):
                # 冗余规则报告后一个重复项，保留单一、可定位的问题主键。
                log_id = merged_ids[-1]
                source = by_id[log_id]
        if source is None:
            if tolerant:
                continue
            raise ValueError("规则评测 log_id 不存在: %r" % item.get("log_id"))
        severity = str(item.get("severity", "")).strip()
        if rule not in rule_names:
            if tolerant:
                continue
            raise ValueError("规则评测返回范围外规则: %r" % item.get("rule"))
        if severity not in SEVERITIES:
            if tolerant:
                severity = log_spec.rule_severity(rule)
            else:
                raise ValueError("规则评测返回未知严重级: %r" % severity)
        expected_severity = log_spec.rule_severity(rule)
        if severity != expected_severity:
            if tolerant:
                severity = expected_severity
            else:
                raise ValueError(
                    "规则评测严重级不匹配: %s 应为 %s，实际为 %s"
                    % (rule, expected_severity, severity)
                )
        if (
            not str(item.get("reason", "")).strip()
            or not str(item.get("suggestion", "")).strip()
        ):
            if tolerant:
                continue
            raise ValueError("规则评测缺少 reason 或 suggestion")
        key = (log_id, rule)
        if key in seen:
            if tolerant:
                continue
            raise ValueError("规则评测重复返回同一问题: %s/%s" % key)
        seen.add(key)
        valid.append(
            {
                "repo": source["repo"],
                "branch": source["branch"],
                "log_id": source["log_id"],
                "file": source["file"],
                "line": source["line"],
                "end": source["end"],
                "code": source["code"],
                "rule": rule,
                "category": log_spec.rule_category(rule),
                "severity": severity,
                "reason": str(item["reason"]),
                "suggestion": str(item["suggestion"]),
                "source_type": source["source_type"],
            }
        )
    return valid


def review_logs_request(args_tuple):
    logs, api, key, model, bl, rule_names, max_tokens, timeout, retries = args_tuple[:9]
    hard_limit = args_tuple[9] if len(args_tuple) > 9 else DEFAULT_MAX_INPUT_CHARS
    repair_error = args_tuple[10] if len(args_tuple) > 10 else ""
    started = time.monotonic()
    try:
        short_ids = os.environ.get("LLM_RULE_SHORT_IDS", "").lower() in (
            "1",
            "true",
            "yes",
            "on",
        )
        id_map = (
            {"R%d" % index: item["global_id"] for index, item in enumerate(logs, 1)}
            if short_ids
            else {}
        )
        prompt_logs = (
            [dict(item, global_id="R%d" % index) for index, item in enumerate(logs, 1)]
            if short_ids
            else logs
        )
        prompt = rule_prompt(prompt_logs, bl, rule_names)
        if repair_error:
            prompt += (
                "\n\n【上次响应校验失败】%s\n请重新审查同一批日志，"
                "log_id 必须逐字复制输入中的 id，不得缩写路径或省略行号范围。"
                "只返回符合规则且可定位的问题，保持原有 JSON 格式。" % repair_error
            )
        if len(prompt) > hard_limit:
            raise ValueError(
                "规则评测请求超过输入硬上限: %d > %d 字符" % (len(prompt), hard_limit)
            )
        content, usage = call_llm(
            api,
            key,
            model,
            prompt,
            max_tokens=max_tokens,
            timeout=timeout,
            retries=retries,
            thinking="enabled",
            reasoning_effort="high",
        )
        raw_issues = parse_repository_result(content)
        if short_ids:
            raw_issues = [
                dict(
                    item,
                    log_id=id_map.get(
                        str(item.get("log_id", "")).strip(), item.get("log_id")
                    ),
                )
                if isinstance(item, dict)
                else item
                for item in raw_issues
            ]
        issues = validate_log_issues(raw_issues, logs, rule_names)
        return logs, issues, None, usage, round(time.monotonic() - started, 3)
    except Exception as exc:  # noqa: BLE001
        # Repair the same complete batch once; never split or accept invalid IDs.
        if (
            isinstance(exc, ValueError)
            and not repair_error
            and "输入硬上限" not in str(exc)
        ):
            print(
                "    规则响应校验失败，附带错误反馈后同组修复重试: %s" % exc, flush=True
            )
            return review_logs_request(
                (
                    logs,
                    api,
                    key,
                    model,
                    bl,
                    rule_names,
                    max_tokens,
                    timeout,
                    retries,
                    hard_limit,
                    str(exc),
                )
            )
        return (
            logs,
            [],
            "%s: %s" % (type(exc).__name__, exc),
            {},
            round(time.monotonic() - started, 3),
        )


def repository_prompt(logs, bl):
    payload = [
        {
            "id": item["global_id"],
            "file": item["file"],
            "line": item["line"],
            "end": item["end"],
            "code": item["code"],
        }
        for item in logs
    ]
    return REPOSITORY_PROMPT_TEMPLATE.format(
        rules_section=log_spec.prompt_rules(bl, REPOSITORY_RULE_NAMES),
        scale_section=log_spec.prompt_scale(REPOSITORY_RULE_NAMES),
        severities_section=log_spec.prompt_severities(),
        rule_name_list="/".join(REPOSITORY_RULE_NAMES),
        severity_list_slash="/".join(SEVERITIES),
        log_count=len(payload),
        logs_json=json.dumps(payload, ensure_ascii=False),
    )


def validate_repository_issues(issues, logs):
    by_id = {item["global_id"]: item for item in logs}
    valid, seen = [], set()
    for item in issues:
        if not isinstance(item, dict):
            raise ValueError("全仓复核 issues 中存在非对象条目")
        log_id = str(item.get("log_id", "")).strip()
        source = by_id.get(log_id)
        if source is None:
            raise ValueError("全仓复核 log_id 不存在: %r" % item.get("log_id"))
        rule = log_spec.normalize_rule(str(item.get("rule", "")).strip())
        severity = str(item.get("severity", "")).strip()
        if rule not in REPOSITORY_RULE_NAMES:
            raise ValueError("全仓复核返回非全仓规则: %r" % item.get("rule"))
        if severity not in SEVERITIES:
            raise ValueError("全仓复核返回未知严重级: %r" % severity)
        expected_severity = log_spec.rule_severity(rule)
        if severity != expected_severity:
            raise ValueError(
                "全仓复核严重级不匹配: %s 应为 %s，实际为 %s"
                % (rule, expected_severity, severity)
            )
        if (
            not str(item.get("reason", "")).strip()
            or not str(item.get("suggestion", "")).strip()
        ):
            raise ValueError("全仓复核缺少 reason 或 suggestion")
        key = (log_id, rule)
        if key in seen:
            raise ValueError("全仓复核重复返回同一问题: %s/%s" % key)
        seen.add(key)
        valid.append(
            {
                "repo": source["repo"],
                "branch": source["branch"],
                "log_id": source["log_id"],
                "file": source["file"],
                "line": source["line"],
                "end": source["end"],
                "code": source["code"],
                "rule": rule,
                "category": log_spec.rule_category(rule),
                "severity": severity,
                "reason": str(item["reason"]),
                "suggestion": str(item["suggestion"]),
                "source_type": source["source_type"],
            }
        )
    return valid


def classify_source(relpath):
    path = "/" + relpath.lower().replace("\\", "/").lstrip("/")
    if any(
        part in path
        for part in (
            "/test/",
            "/tests/",
            "/ut/",
            "/st/",
            "test_",
            "_test",
            "/example",
            "/examples/",
            "/samples/",
            "/demo",
            "/experiment",
        )
    ):
        return "测试示例"
    if any(
        part in path
        for part in (
            ".claude/",
            ".agents/",
            "/skills/",
            "/scripts/",
            "/tools/",
            "/tool/",
        )
    ):
        return "工具脚本"
    return "核心代码"


def _hms(seconds):
    seconds = int(max(0, seconds))
    hours, rest = divmod(seconds, 3600)
    minutes, seconds = divmod(rest, 60)
    if hours:
        return "%dh%02dm" % (hours, minutes)
    return "%dm%02ds" % (minutes, seconds) if minutes else "%ds" % seconds


def _env_int(name, default=None):
    raw = os.environ.get(name, "").strip()
    return int(raw) if raw else default


def found_time_now():
    """返回请求完成时间；毫秒和时区可区分相邻请求并避免时区歧义。"""
    return datetime.now().astimezone().isoformat(timespec="milliseconds")


def selftest():
    ok = True

    def check(name, condition, detail=""):
        nonlocal ok
        ok = ok and bool(condition)
        print("%s %s %s" % ("[ok]  " if condition else "[FAIL]", name, detail))

    bl = ["Inner Error"]
    records = []
    for index, size in enumerate((80, 90, 100), 1):
        lines = [('CUSTOM_SINK("message %d");\n' % index)] + ["x;\n"] * size
        source = "".join(lines)
        records.append(
            {
                "file": "f%d.custom" % index,
                "end": len(lines),
                "lines": lines,
                "source": source,
                "text": render_numbered(lines),
                "language": "custom",
                "sig": hashlib.sha256(source.encode()).hexdigest(),
            }
        )
    two_size = prompt_size(records[:2], bl)
    requests = pack_files(records, None, bl, target_input_chars=two_size)
    check(
        "有空间继续加入完整文件，超出后整个文件进入下一请求",
        [len(group) for group in requests] == [2, 1],
        [len(group) for group in requests],
    )
    _tmp_result = []
    for group in requests:
        for record in group:
            _tmp_result.append(record["file"])
    flat = _tmp_result
    check(
        "每个文件恰好上传一次",
        flat == [record["file"] for record in records] and len(flat) == len(set(flat)),
        flat,
    )
    check(
        "单请求不设文件数上限",
        len(pack_files(records, None, bl, target_input_chars=prompt_size(records, bl)))
        == 1,
    )

    oversized = False
    try:
        hard_limit = prompt_size(records[:1], bl) - 1
        pack_files(records[:1], hard_limit, bl, target_input_chars=hard_limit)
    except ValueError:
        oversized = True
    check("单文件超基线时报错而不是截断", oversized)

    parsed = parse_result('{"files":[],"logs":[]}')
    check("完整空响应可解析", parsed == ([], []), parsed)
    bad_response = False
    try:
        parse_result('{"logs":[],"issues":[]}')
    except ValueError:
        bad_response = True
    check("缺少全部日志列表的响应不可冒充完整结果", bad_response)

    log = {"id": "L1", "file": "f1.custom", "line": 1, "end": 1}
    issue = {
        "log_id": "L1",
        "rule": RULE_NAMES[0],
        "severity": log_spec.rule_severity(RULE_NAMES[0]),
        "reason": "reason",
        "suggestion": "suggestion",
    }
    valid_logs, valid_issues = validate_result(
        [{"file": "f1.custom", "log_count": 1}], [log], [issue], records[:1]
    )
    check(
        "未知名称的自定义日志接口无需候选白名单即可通过",
        len(valid_logs) == 1 and len(valid_issues) == 1,
    )

    print(
        "\n规范: %d 条（来源: %s）"
        % (len(RULE_NAMES), " + ".join(log_spec.SPEC_SOURCES))
    )
    print("总判定:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


def main():
    if "--selftest" in sys.argv:
        return selftest()

    parser = argparse.ArgumentParser(description="CANN 日志规范评测")
    model_config.add_model_argument(parser)
    parser.add_argument(
        "--require-model-match",
        dest="require_model_match",
        action="store_true",
        default=None,
    )
    parser.add_argument(
        "--allow-model-alias", dest="require_model_match", action="store_false"
    )
    parser.add_argument("--repo", required=True)
    parser.add_argument("--repo-name", required=True)
    parser.add_argument("--branch", default="")
    parser.add_argument("--out", required=True)
    parser.add_argument(
        "--max-input-chars",
        type=int,
        default=_env_int("LLM_MAX_INPUT_CHARS", DEFAULT_MAX_INPUT_CHARS),
        help="完整请求的模型输入硬上限，默认1000000字符；单文件超限直接失败",
    )
    parser.add_argument(
        "--max-output-tokens",
        type=int,
        default=_env_int("LLM_MAX_TOKENS"),
        help="可选的模型输出上限；未设置时请求体不发送 max_tokens",
    )
    parser.add_argument(
        "--target-input-chars",
        type=int,
        default=_env_int("LLM_TARGET_INPUT_CHARS"),
        help="日志识别请求目标输入大小，默认不超过1000000字符",
    )
    parser.add_argument(
        "--rule-target-input-chars",
        type=int,
        default=_env_int("LLM_RULE_TARGET_INPUT_CHARS"),
        help="开启 Thinking 的规则评测请求目标大小，默认不超过50000字符",
    )
    parser.add_argument(
        "--log-density-per-million",
        type=float,
        default=float(os.environ.get("LOG_DENSITY_PER_MILLION", "461")),
        help="实测日志密度（条/百万字符），用于按输出上限反算装包大小",
    )
    parser.add_argument(
        "--output-tokens-per-log",
        type=float,
        default=float(os.environ.get("LOG_OUTPUT_TOKENS_PER_LOG", "46")),
        help="实测每条日志的输出 token 成本",
    )
    parser.add_argument(
        "--output-safety-ratio",
        type=float,
        default=float(os.environ.get("LOG_OUTPUT_SAFETY_RATIO", "0.6")),
        help="预期输出占输出上限的安全比例，默认0.6",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=_env_int("LLM_WORKERS", 1),
        help="并发数，仅控制请求调度，默认1；更高并发需人工指定",
    )
    parser.add_argument("--timeout", type=int, default=_env_int("LLM_TIMEOUT", 600))
    parser.add_argument(
        "--retries",
        type=int,
        default=_env_int("LLM_RETRIES", 1),
        help="瞬时失败请求重试次数；成功请求和确定性错误不会重复上传",
    )
    parser.add_argument(
        "--extensions",
        default=os.environ.get("LOG_CODE_EXTENSIONS", ",".join(CODE_EXT)),
        help="扫描扩展名，逗号分隔",
    )
    parser.add_argument(
        "--exclude-dirs",
        default=os.environ.get("LOG_EXCLUDE_DIRS", ",".join(sorted(SKIP_DIRS))),
        help="跳过目录名，逗号分隔；传空字符串可关闭",
    )
    parser.add_argument(
        "--files-from", help="可选：只评测清单中的相对路径，每行一个；仍按完整文件送审"
    )
    parser.add_argument("--restart", action="store_true", help="忽略已有进度，从头重跑")
    args = parser.parse_args()

    api = os.environ.get("LLM_API_BASE", "").strip()
    key = os.environ.get("LLM_API_KEY", "").strip()
    model = model_config.configure_model(args.model)
    require_match = (
        model_config.model_match_required(model)
        if args.require_model_match is None
        else args.require_model_match
    )
    os.environ["LLM_REQUIRE_MODEL_MATCH"] = "1" if require_match else "0"

    def checkpoint_model_valid(row):
        return not require_match or row.get("response_model") == model

    errors = []
    if not api:
        errors.append("LLM_API_BASE")
    if not key:
        errors.append("LLM_API_KEY")
    if not model:
        errors.append("LLM_MODEL")
    if errors:
        print("[error] 缺少模型配置: %s" % ", ".join(errors), file=sys.stderr)
        return 2
    if not os.path.exists(args.repo):
        print("[error] 输入不存在: %s" % args.repo, file=sys.stderr)
        return 2
    if (
        (args.max_input_chars is not None and args.max_input_chars <= 0)
        or (args.max_output_tokens is not None and args.max_output_tokens <= 0)
        or args.workers <= 0
    ):
        print("[error] 显式输入/输出上限和并发数必须大于0", file=sys.stderr)
        return 2
    target_input_chars = args.target_input_chars or (
        min(args.max_input_chars, DEFAULT_MAX_INPUT_CHARS)
        if args.max_input_chars is not None
        else DEFAULT_MAX_INPUT_CHARS
    )
    if target_input_chars <= 0 or (
        args.max_input_chars is not None and target_input_chars > args.max_input_chars
    ):
        print("[error] 目标输入大小必须大于0且不超过输入硬上限", file=sys.stderr)
        return 2
    # 装包目标是上限；即使调用方显式传入，也需为模型输出预留空间。
    planning_output_tokens = args.max_output_tokens or DEFAULT_PLANNING_OUTPUT_TOKENS
    budget_chars = output_budget_chars(
        planning_output_tokens,
        args.log_density_per_million,
        args.output_tokens_per_log,
        args.output_safety_ratio,
    )
    if budget_chars:
        target_input_chars = min(
            target_input_chars, max(min(20000, target_input_chars), budget_chars)
        )
    rule_target_input_chars = args.rule_target_input_chars or min(
        target_input_chars, 50000
    )
    if rule_target_input_chars <= 0 or (
        args.max_input_chars is not None
        and rule_target_input_chars > args.max_input_chars
    ):
        print(
            "[error] 规则评测目标输入大小必须大于0且不超过输入硬上限", file=sys.stderr
        )
        return 2
    max_manifest_chars = min(MAX_MANIFEST_CHARS, max(1000, planning_output_tokens * 2))

    extensions = configured_extensions(args.extensions)
    excluded = set(_csv(args.exclude_dirs))
    if not extensions:
        print("[error] --extensions 不能为空", file=sys.stderr)
        return 2
    os.makedirs(args.out, exist_ok=True)
    bl = blacklist()
    artifact_name = (
        args.repo_name
        if not args.branch or args.branch == "master"
        else "%s@%s" % (args.repo_name, args.branch.replace("/", "_"))
    )
    evaluation_started_at = found_time_now()
    evaluation_started = time.monotonic()
    commit = ""
    try:
        commit = subprocess.run(
            ["git", "-C", args.repo, "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=True,
        ).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        pass
    process_path = os.path.join(args.out, "%s_process.json" % artifact_name)
    process_state = {
        "repo": args.repo_name,
        "branch": args.branch,
        "commit": commit,
        "model": model,
        "status": "running",
        "stage": "scanning",
        "started_at": evaluation_started_at,
        "updated_at": evaluation_started_at,
        "scanned_files": 0,
        "extraction": {"total": 0, "completed": 0, "failed": 0},
        "rule_review": {"total": 0, "completed": 0, "failed": 0},
        "log_statements": 0,
        "findings": 0,
        "last_error": "",
        # 文件级进度用于长时间评测时回答“哪些文件已跑/未跑”。每个
        # 文件始终只有一个完整 review unit，状态由该 unit 的结果更新。
        "file_progress": [],
    }

    def write_process(**updates):
        process_state.update(updates)
        process_state["updated_at"] = found_time_now()
        with open(process_path, "w", encoding="utf-8") as handle:
            json.dump(process_state, handle, ensure_ascii=False, indent=2)

    write_process()

    scan_start = time.time()
    paths = list(iter_code_files(args.repo, extensions, excluded))
    scope = "full_repository"
    if args.files_from:
        try:
            with open(args.files_from, "r", encoding="utf-8-sig") as fh:
                requested = [
                    line.strip().replace("\\", "/")
                    for line in fh
                    if line.strip() and not line.lstrip().startswith("#")
                ]
        except OSError as exc:
            print(
                "[error] 读取文件清单失败 %s: %s" % (args.files_from, exc),
                file=sys.stderr,
            )
            return 2
        if not requested:
            print("[error] --files-from 清单不能为空", file=sys.stderr)
            return 2
        if len(requested) != len(set(requested)):
            print("[error] --files-from 清单包含重复路径", file=sys.stderr)
            return 2
        _tmp_result = {}
        for path in paths:
            _tmp_result[os.path.relpath(path, args.repo).replace("\\", "/")] = path
        path_index = _tmp_result
        unavailable = [name for name in requested if name not in path_index]
        if unavailable:
            print(
                "[error] 文件清单中有路径不存在或不在扫描范围内: %s"
                % ", ".join(unavailable[:10]),
                file=sys.stderr,
            )
            return 2
        paths = [path_index[name] for name in requested]
        scope = "file_list"
    if not paths:
        print("[info] 扫描范围内没有匹配的源码文件，按 0 条日志完成评测。", flush=True)
    try:
        scanned_source_files = [read_source(path, args.repo) for path in paths]
        _tmp_result = []
        for record in scanned_source_files:
            _tmp_result.append((record["file"], record["lines"]))
        macro_sources = _tmp_result
        macro_definitions = prescan.collect_macro_definitions(macro_sources)
        source_files = scanned_source_files
        # A source file is the atomic review unit.  Keep every file intact and
        # let pack_files reject an individual prompt that exceeds the configured
        # model input limit; never manufacture line-range pseudo-files.
        segmented_files = []
        files = list(source_files)
        scan_manifest = [
            {
                "file": record["file"],
                "sha256": record["sig"],
                "lines": record["end"],
            }
            for record in scanned_source_files
        ]
        request_rule_names = ()
        # Publish the atomic file list before装包 so a too-large file is visible
        # as failed in the live process artifact instead of disappearing during
        # the scanning stage.
        process_state["scanned_files"] = len(scanned_source_files)
        process_state["model_reviewed_files"] = len(source_files)
        process_state["review_units"] = len(files)
        process_state["segmented_files"] = []
        process_state["file_progress"] = [
            {
                "file": record["file"],
                "status": "pending",
                "units": 1,
                "started": 0,
                "completed": 0,
                "failed": 0,
            }
            for record in scanned_source_files
        ]
        write_process(stage="scanning")
        requests = (
            pack_files(
                files,
                args.max_input_chars,
                bl,
                request_rule_names,
                target_input_chars,
                max_manifest_chars,
            )
            if files
            else []
        )
    except (RuntimeError, ValueError, AssertionError) as exc:
        if isinstance(exc, InputLimitError):
            for row in process_state.get("file_progress", []):
                if row.get("file") == exc.file_name:
                    row.update(status="failed", failed=1)
                    break
            process_state["extraction"]["failed"] = 1
        write_process(status="failed", stage="scanning", last_error=str(exc))
        print("[error] %s" % exc, file=sys.stderr)
        return 2
    scan_seconds = round(time.time() - scan_start, 1)
    request_sizes = [prompt_size(group, bl, request_rule_names) for group in requests]
    scan_path = os.path.join(args.out, "%s_scan.json" % artifact_name)
    with open(scan_path, "w", encoding="utf-8") as handle:
        json.dump(
            {
                "repo": args.repo_name,
                "branch": args.branch,
                "scanned_files": len(scanned_source_files),
                "review_units": len(files),
                "files": scan_manifest,
            },
            handle,
            ensure_ascii=False,
            indent=2,
        )
    process_state["scanned_files"] = len(scanned_source_files)
    process_state["model_reviewed_files"] = len(source_files)
    process_state["review_units"] = len(files)
    process_state["segmented_files"] = segmented_files
    # 建立文件到 review unit 的映射。文件状态始终以原始相对路径呈现，
    # 每个文件只对应一个完整源码 review unit。
    file_units = {
        record["file"]: {"total": 0, "started": 0, "completed": 0, "failed": 0}
        for record in scanned_source_files
    }
    for group in requests:
        for record in group:
            name = record["file"]
            file_units.setdefault(
                name, {"total": 0, "started": 0, "completed": 0, "failed": 0}
            )
            file_units[name]["total"] += 1

    def refresh_file_progress():
        rows = []
        for name in sorted(file_units):
            counters = file_units[name]
            total = counters["total"]
            if total <= 0:
                status = "skipped"
            elif counters["failed"]:
                status = "failed"
            elif counters["completed"] >= total:
                status = "completed"
            elif counters["started"]:
                status = "running"
            else:
                status = "pending"
            rows.append(
                {
                    "file": name,
                    "status": status,
                    "units": total,
                    "started": counters["started"],
                    "completed": counters["completed"],
                    "failed": counters["failed"],
                }
            )
        process_state["file_progress"] = rows

    refresh_file_progress()
    process_state["extraction"]["total"] = len(requests)
    write_process(stage="extracting")

    extraction_thinking, extraction_effort = model_config.request_settings(
        model, "disabled", None
    )
    extraction_fingerprint = "thinking=%s" % extraction_thinking
    if extraction_effort:
        extraction_fingerprint += ";reasoning_effort=%s" % extraction_effort
    fingerprint = hashlib.sha256(
        "\n".join(
            [
                EVALUATOR_VERSION,
                model,
                PROMPT_TEMPLATE,
                extraction_fingerprint,
                "max_output_tokens=%s"
                % (
                    args.max_output_tokens
                    if args.max_output_tokens is not None
                    else "unset"
                ),
                "target_input_chars=%d" % target_input_chars,
                "max_manifest_chars=%s"
                % (max_manifest_chars if max_manifest_chars is not None else "unset"),
                "extensions=%s" % ",".join(extensions),
                "excluded=%s" % ",".join(sorted(excluded)),
            ]
        ).encode("utf-8", "replace")
    ).hexdigest()[:16]

    def request_key(group):
        content = "|".join(
            "%s:%s" % (record.get("review_file", record["file"]), record["sig"])
            for record in group
        )
        digest = hashlib.sha256(content.encode("utf-8")).hexdigest()
        return "%s:%s" % (fingerprint, digest)

    progress_path = os.path.join(args.out, "%s_progress.jsonl" % artifact_name)
    current_keys = {request_key(group) for group in requests}
    groups_by_key = {request_key(group): group for group in requests}
    done_keys, all_logs, findings = set(), [], []
    numeric_table_audits = {}
    cached_response_models = set()
    stale = 0
    if os.path.exists(progress_path) and not args.restart:
        with open(progress_path, "r", encoding="utf-8-sig") as fh:
            for raw in fh:
                try:
                    row = json.loads(raw)
                except json.JSONDecodeError:
                    continue
                key_in_file = row.get("key", "")
                group = groups_by_key.get(key_in_file, [])
                audits = numeric_audits(row)
                records = {record["file"]: record for record in group}
                audited_files = {audit.get("file") for audit in audits}
                valid_audits = bool(audits) and (
                    len(audited_files) == len(audits)
                    and all(
                        audit == numeric_table_audit(records[audit["file"]])
                        for audit in audits
                        if audit.get("file") in records
                    )
                    and audited_files <= set(records)
                    and not any(
                        log.get("file") in audited_files
                        for log in row.get("logs") or []
                    )
                )
                valid_audit_only = valid_audits and audited_files == set(records)
                if (
                    not key_in_file.startswith(fingerprint + ":")
                    or key_in_file not in current_keys
                    or (audits and not valid_audits)
                    or not (checkpoint_model_valid(row) or valid_audit_only)
                ):
                    stale += 1
                    continue
                done_keys.add(key_in_file)
                all_logs.extend(row.get("logs") or [])
                if valid_audits:
                    numeric_table_audits.update(
                        {audit["file"]: audit for audit in audits}
                    )
                if row.get("response_model"):
                    cached_response_models.add(row["response_model"])
    process_state["extraction"]["completed"] = len(done_keys)
    process_state["log_statements"] = len(all_logs)
    write_process()

    todo = [group for group in requests if request_key(group) not in done_keys]
    # 断点中已落盘的成功请求先恢复为 completed；本轮待发送请求进入
    # running，未出现在此集合中的文件保持 pending（例如后续预算耗尽）。
    for group in requests:
        is_done = request_key(group) in done_keys
        for record in group:
            counters = file_units[record["file"]]
            if is_done:
                counters["completed"] += 1
    # 线程池会同时执行不超过 workers 个请求；只把这批请求标为 running，
    # 其余请求保持 pending，避免“已提交到队列”被误报成“已经审查”。
    next_running = min(args.workers, len(todo))
    for group in todo[:next_running]:
        for record in group:
            file_units[record["file"]]["started"] += 1
    refresh_file_progress()
    write_process()
    print(
        "[info] 仓=%s 完整代码文件=%d 审查单元=%d 请求=%d(待跑%d) 扫描耗时=%ss"
        % (
            args.repo_name,
            len(scanned_source_files),
            len(files),
            len(requests),
            len(todo),
            scan_seconds,
        ),
        flush=True,
    )
    pending_files = [
        row["file"]
        for row in process_state["file_progress"]
        if row["status"] == "pending"
    ]
    running_files = [
        row["file"]
        for row in process_state["file_progress"]
        if row["status"] == "running"
    ]
    if running_files:
        print(
            "    [文件] 进行中 %d 个（示例: %s）"
            % (len(running_files), ", ".join(running_files[:5])),
            flush=True,
        )
    if pending_files:
        print(
            "    [文件] 待审 %d 个（示例: %s）"
            % (len(pending_files), ", ".join(pending_files[:5])),
            flush=True,
        )
    if request_sizes:
        oversized = [
            size
            for group, size in zip(requests, request_sizes)
            if len(group) == 1 and size > target_input_chars
        ]
        if oversized:
            print(
                "[warn] %d 个完整文件单独超过装包目标，仍整文件送审；"
                "最大请求=%s字符" % (len(oversized), format(max(oversized), ",")),
                flush=True,
            )
        print(
            "[info] 输入硬上限=%s 请求输入=min %s / avg %s / max %s；"
            "每文件只属于一个请求，返回清单按大小分组"
            % (
                (
                    format(args.max_input_chars, ",") + "字符"
                    if args.max_input_chars is not None
                    else "未设置"
                ),
                format(min(request_sizes), ","),
                format(int(sum(request_sizes) / len(request_sizes)), ","),
                format(max(request_sizes), ","),
            ),
            flush=True,
        )
        print(
            "[info] 每请求文件数=%s 模型=%s 并发=%d 输出上限=%s"
            % (
                [len(group) for group in requests],
                model,
                args.workers,
                (
                    format(args.max_output_tokens, ",") + " tokens"
                    if args.max_output_tokens is not None
                    else "未设置（由模型/网关决定）"
                ),
            ),
            flush=True,
        )
        print(
            "[info] 装包目标=%s字符（输出规划预算=%s tokens，密度%.0f条/百万字符、"
            "%.0f token/条、安全比例%.2f）预期最大输出=%s tokens"
            % (
                format(target_input_chars, ","),
                format(planning_output_tokens, ","),
                args.log_density_per_million,
                args.output_tokens_per_log,
                args.output_safety_ratio,
                format(
                    estimate_output_tokens(
                        max(request_sizes),
                        args.log_density_per_million,
                        args.output_tokens_per_log,
                        max(len(group) for group in requests),
                    ),
                    ",",
                ),
            ),
            flush=True,
        )
    if stale:
        print("[info] 忽略 %d 条旧评测语义进度" % stale, flush=True)

    request_phase_started = time.monotonic()
    failed = []
    usage_total = (
        {"_response_models": sorted(cached_response_models)}
        if cached_response_models
        else {}
    )

    def merge_usage(usage):
        numeric_table_audits.update(
            {audit["file"]: audit for audit in numeric_audits(usage or {})}
        )
        for name, value in (usage or {}).items():
            if isinstance(value, int):
                usage_total[name] = usage_total.get(name, 0) + value
            elif name == "_response_model" and value:
                models = usage_total.setdefault("_response_models", [])
                if value not in models:
                    models.append(value)

    request_timings = []
    progress_lock = threading.Lock()
    mode = "w" if args.restart else "a"
    with open(progress_path, mode, encoding="utf-8") as progress:
        tasks = [
            (
                group,
                api,
                key,
                model,
                bl,
                request_rule_names,
                args.max_output_tokens,
                args.timeout,
                args.retries,
                args.max_input_chars,
            )
            for group in todo
        ]
        with ThreadPoolExecutor(max_workers=args.workers) as executor:
            futures = {executor.submit(review_request, task): task for task in tasks}
            completed = 0
            started_groups = next_running
            for future in as_completed(futures):
                group, logs, issues, error, usage, request_elapsed = future.result()
                completed += 1
                request_timings.append(
                    {
                        "files": [record["file"] for record in group],
                        "input_chars": prompt_size(group, bl, request_rule_names),
                        "elapsed_s": request_elapsed,
                        "success": error is None,
                        "usage": usage,
                    }
                )
                if error:
                    for record in group:
                        file_units[record["file"]]["failed"] += 1
                    failed.append(
                        {
                            "key": request_key(group),
                            "error": error,
                            "files": [record["file"] for record in group],
                        }
                    )
                    process_state["extraction"]["failed"] = len(failed)
                    process_state["last_error"] = error
                else:
                    for record in group:
                        file_units[record["file"]]["completed"] += 1
                    normalized_logs = []
                    for item in logs:
                        row = {
                            "repo": args.repo_name,
                            "branch": args.branch,
                            "log_id": item["id"],
                            "global_id": "%s:%d-%d"
                            % (item["file"], item["line"], item["end"]),
                            "file": item["file"],
                            "line": item["line"],
                            "end": item["end"],
                            "code": item["code"],
                            "source_type": item["source_type"],
                        }
                        for name in ("context_start", "context"):
                            if item.get(name) not in (None, ""):
                                row[name] = item[name]
                        expansion = prescan.macro_context(
                            item["code"], macro_definitions
                        )
                        if expansion:
                            row["macro_context"] = expansion
                        normalized_logs.append(row)
                    all_logs.extend(normalized_logs)
                    merge_usage(usage)
                    with progress_lock:
                        progress.write(
                            json.dumps(
                                {
                                    "key": request_key(group),
                                    "files": [r["file"] for r in group],
                                    "logs": normalized_logs,
                                    "response_model": usage.get("_response_model"),
                                    "numeric_table_audit": usage.get(
                                        "_numeric_table_audit"
                                    ),
                                    "numeric_table_audits": usage.get(
                                        "_numeric_table_audits"
                                    ),
                                },
                                ensure_ascii=False,
                            )
                            + "\n"
                        )
                        progress.flush()
                process_state["extraction"]["completed"] = (
                    len(done_keys) + completed - len(failed)
                )
                process_state["log_statements"] = len(all_logs)
                if started_groups < len(todo):
                    for record in todo[started_groups]:
                        file_units[record["file"]]["started"] += 1
                    started_groups += 1
                refresh_file_progress()
                write_process()
                group_files = sorted({record["file"] for record in group})
                print(
                    "    [文件] %s %s"
                    % ("失败" if error else "完成", ", ".join(group_files)),
                    flush=True,
                )
                if completed % 10 == 0 or completed == len(tasks):
                    elapsed = time.monotonic() - request_phase_started
                    rate = completed / max(elapsed, 0.001)
                    eta = (len(tasks) - completed) / max(rate, 1e-9)
                    print(
                        "    日志识别 %d/%d 日志 %d ETA %s%s"
                        % (
                            completed,
                            len(tasks),
                            len(all_logs),
                            _hms(eta),
                            " 失败%d" % len(failed) if failed else "",
                        ),
                        flush=True,
                    )

    unique_logs = {}
    for item in all_logs:
        key_log = (
            item["file"],
            item["line"],
            item.get("global_id"),
            _compact(item["code"]),
        )
        unique_logs[key_log] = item
    logs = sorted(
        unique_logs.values(), key=lambda row: (row["file"], row["line"], row["code"])
    )

    rule_names = tuple(name for name in RULE_NAMES if name not in REPOSITORY_RULE_NAMES)
    rule_groups = (
        pack_logs(logs, args.max_input_chars, bl, rule_names, rule_target_input_chars)
        if logs and rule_names and not failed
        else []
    )
    rule_fingerprint = hashlib.sha256(
        "\n".join(
            [
                EVALUATOR_VERSION,
                model,
                RULE_PROMPT_TEMPLATE,
                log_spec.prompt_rules(bl, rule_names),
                log_spec.prompt_scale(rule_names),
                log_spec.prompt_severities(),
                "thinking=enabled",
                "reasoning_effort=high",
                "max_output_tokens=%s"
                % (
                    args.max_output_tokens
                    if args.max_output_tokens is not None
                    else "unset"
                ),
            ]
        ).encode("utf-8", "replace")
    ).hexdigest()[:16]

    def rule_key(group):
        digest = hashlib.sha256(
            "|".join(
                "%s:%s" % (item["global_id"], _compact(item["code"])) for item in group
            ).encode("utf-8")
        ).hexdigest()
        return "%s:%s" % (rule_fingerprint, digest)

    rule_progress_path = os.path.join(
        args.out, "%s_rule_progress.jsonl" % artifact_name
    )
    rule_keys = {rule_key(group) for group in rule_groups}
    done_rule_keys = set()
    if os.path.exists(rule_progress_path) and not args.restart:
        with open(rule_progress_path, "r", encoding="utf-8-sig") as handle:
            for raw in handle:
                try:
                    row = json.loads(raw)
                except json.JSONDecodeError:
                    continue
                if row.get("key") in rule_keys and checkpoint_model_valid(row):
                    done_rule_keys.add(row["key"])
                    findings.extend(row.get("findings") or [])
                    merge_usage({"_response_model": row.get("response_model")})

    _tmp_result = []
    for group in rule_groups:
        if rule_key(group) not in done_rule_keys:
            _tmp_result.append(group)
    rule_todo = _tmp_result
    rule_failures = []
    process_state["rule_review"] = {
        "total": len(rule_groups),
        "completed": len(done_rule_keys),
        "failed": 0,
    }
    process_state["log_statements"] = len(logs)
    write_process(stage="rule_review")
    rule_mode = "w" if args.restart else "a"
    with open(rule_progress_path, rule_mode, encoding="utf-8") as progress:
        tasks = [
            (
                group,
                api,
                key,
                model,
                bl,
                rule_names,
                args.max_output_tokens,
                args.timeout,
                args.retries,
                args.max_input_chars,
            )
            for group in rule_todo
        ]
        with ThreadPoolExecutor(max_workers=args.workers) as executor:
            _tmp_result = {}
            for task in tasks:
                _tmp_result[executor.submit(review_logs_request, task)] = task
            futures = _tmp_result
            for completed, future in enumerate(as_completed(futures), 1):
                group, issues, error, usage, request_elapsed = future.result()
                request_timings.append(
                    {
                        "stage": "rule_review",
                        "logs": len(group),
                        "input_chars": len(rule_prompt(group, bl, rule_names)),
                        "elapsed_s": request_elapsed,
                        "success": error is None,
                        "usage": usage,
                    }
                )
                if error:
                    rule_failures.append(
                        {
                            "type": "rule_review",
                            "key": rule_key(group),
                            "error": error,
                            "logs": [item["global_id"] for item in group],
                        }
                    )
                    process_state["last_error"] = error
                else:
                    found_at = found_time_now()
                    for issue in issues:
                        issue.setdefault("decided_by", "model")
                        issue["found_time"] = found_at
                    findings.extend(issues)
                    progress.write(
                        json.dumps(
                            {
                                "key": rule_key(group),
                                "findings": issues,
                                "response_model": usage.get("_response_model"),
                            },
                            ensure_ascii=False,
                        )
                        + "\n"
                    )
                    progress.flush()
                merge_usage(usage)
                process_state["rule_review"]["completed"] = (
                    len(done_rule_keys) + completed - len(rule_failures)
                )
                process_state["rule_review"]["failed"] = len(rule_failures)
                process_state["findings"] = len(findings)
                write_process()
                if completed % 10 == 0 or completed == len(tasks):
                    print(
                        "    规则评测 %d/%d 问题 %d%s"
                        % (
                            completed,
                            len(tasks),
                            len(findings),
                            " 失败%d" % len(rule_failures) if rule_failures else "",
                        ),
                        flush=True,
                    )

    failed.extend(rule_failures)

    repository_review = {
        "required": bool(REPOSITORY_RULE_NAMES),
        "rules": list(REPOSITORY_RULE_NAMES),
        "completed": False,
        "candidate_logs": 0,
        "groups": 0,
        "reused_groups": 0,
        "failed_groups": 0,
    }
    if repository_review["required"] and not failed:
        duplicate_groups = {}
        for item in logs:
            signature = _message_signature(item["code"])
            duplicate_groups.setdefault(signature, []).append(item)
        _tmp_result = []
        for group in duplicate_groups.values():
            if len(group) > 1:
                _tmp_result.append(group)
        candidate_groups = _tmp_result
        candidate_groups.sort(
            key=lambda group: (
                _message_signature(group[0]["code"]),
                group[0]["file"],
                group[0]["line"],
            )
        )
        repository_review["candidate_logs"] = sum(
            len(group) for group in candidate_groups
        )
        candidate_groups = pack_repository_groups(
            candidate_groups, rule_target_input_chars, bl, REPOSITORY_RULE_NAMES
        )
        repository_review["groups"] = len(candidate_groups)
        if not candidate_groups:
            repository_review["completed"] = True
        else:
            repo_fingerprint = hashlib.sha256(
                "\n".join(
                    [
                        EVALUATOR_VERSION,
                        model,
                        RULE_PROMPT_TEMPLATE,
                        log_spec.prompt_rules(bl, REPOSITORY_RULE_NAMES),
                        log_spec.prompt_scale(REPOSITORY_RULE_NAMES),
                        "thinking=enabled",
                        "reasoning_effort=high",
                    ]
                ).encode("utf-8", "replace")
            ).hexdigest()[:16]

            def repository_key(group):
                digest = hashlib.sha256(
                    "|".join(item["global_id"] for item in group).encode("utf-8")
                ).hexdigest()
                return "%s:%s" % (repo_fingerprint, digest)

            repo_path = os.path.join(
                args.out, "%s_repository_progress.jsonl" % artifact_name
            )
            repo_keys = {repository_key(group) for group in candidate_groups}
            done_repo_keys = set()
            if os.path.exists(repo_path) and not args.restart:
                with open(repo_path, "r", encoding="utf-8-sig") as handle:
                    for raw in handle:
                        try:
                            row = json.loads(raw)
                        except json.JSONDecodeError:
                            continue
                        if row.get("key") in repo_keys and checkpoint_model_valid(row):
                            done_repo_keys.add(row["key"])
                            findings.extend(row.get("findings") or [])
                            merge_usage({"_response_model": row.get("response_model")})
            repository_review["reused_groups"] = len(done_repo_keys)
            repo_todo = [
                group
                for group in candidate_groups
                if repository_key(group) not in done_repo_keys
            ]
            repo_failures = []
            process_state["repository_review"] = {
                "total": len(candidate_groups),
                "completed": len(done_repo_keys),
                "failed": 0,
                "candidate_logs": repository_review["candidate_logs"],
            }
            write_process(stage="repository_review")
            repo_mode = "w" if args.restart else "a"
            with open(repo_path, repo_mode, encoding="utf-8") as progress:
                tasks = [
                    (
                        group,
                        api,
                        key,
                        model,
                        bl,
                        REPOSITORY_RULE_NAMES,
                        args.max_output_tokens,
                        args.timeout,
                        args.retries,
                        args.max_input_chars,
                    )
                    for group in repo_todo
                ]
                with ThreadPoolExecutor(max_workers=args.workers) as executor:
                    futures = {
                        executor.submit(review_logs_request, task): task
                        for task in tasks
                    }
                    for completed, future in enumerate(as_completed(futures), 1):
                        group, issues, error, usage, request_elapsed = future.result()
                        request_timings.append(
                            {
                                "stage": "repository_review",
                                "logs": len(group),
                                "input_chars": len(
                                    rule_prompt(group, bl, REPOSITORY_RULE_NAMES)
                                ),
                                "elapsed_s": request_elapsed,
                                "success": error is None,
                                "usage": usage,
                            }
                        )
                        if error:
                            repo_failures.append(
                                {
                                    "type": "repository_review",
                                    "key": repository_key(group),
                                    "error": error,
                                    "logs": [item["global_id"] for item in group],
                                }
                            )
                            process_state["last_error"] = error
                        else:
                            found_at = found_time_now()
                            for issue in issues:
                                issue.setdefault("decided_by", "model")
                                issue["found_time"] = found_at
                            findings.extend(issues)
                            progress.write(
                                json.dumps(
                                    {
                                        "key": repository_key(group),
                                        "findings": issues,
                                        "response_model": usage.get("_response_model"),
                                    },
                                    ensure_ascii=False,
                                )
                                + "\n"
                            )
                            progress.flush()
                        merge_usage(usage)
                        process_state["repository_review"]["completed"] = (
                            len(done_repo_keys) + completed - len(repo_failures)
                        )
                        process_state["repository_review"]["failed"] = len(
                            repo_failures
                        )
                        process_state["findings"] = len(findings)
                        write_process()
            repository_review["failed_groups"] = len(repo_failures)
            repository_review["completed"] = not repo_failures
            failed.extend(repo_failures)

    findings, output_guard_dropped = det_rules.ensure_deterministic_findings(
        findings, logs
    )
    guard_found_at = found_time_now()
    for issue in findings:
        if not issue.get("found_time"):
            issue["found_time"] = guard_found_at
    unique_findings = {}
    for item in findings:
        key_issue = (item["file"], item["line"], item.get("log_id"), item["rule"])
        unique_findings[key_issue] = item
    findings = sorted(
        unique_findings.values(),
        key=lambda row: (row["file"], row["line"], row["rule"]),
    )

    logs_path = os.path.join(args.out, "%s_logs.jsonl" % artifact_name)
    findings_path = os.path.join(args.out, "%s_findings.jsonl" % artifact_name)
    with open(logs_path, "w", encoding="utf-8") as fh:
        for row in logs:
            fh.write(json.dumps(row, ensure_ascii=False) + "\n")
    with open(findings_path, "w", encoding="utf-8") as fh:
        for row in findings:
            fh.write(json.dumps(row, ensure_ascii=False) + "\n")

    by_rule, by_severity = {}, {}
    for row in findings:
        by_rule[row["rule"]] = by_rule.get(row["rule"], 0) + 1
        by_severity[row["severity"]] = by_severity.get(row["severity"], 0) + 1
    elapsed = round(time.monotonic() - evaluation_started, 1)
    complete = not failed
    evaluation_finished_at = found_time_now()
    write_process(
        status="completed" if complete else "failed",
        stage="completed" if complete else "failed",
        log_statements=len(logs),
        findings=len(findings),
        last_error="" if complete else str(failed[-1].get("error", "")),
    )
    meta = {
        "repo": args.repo_name,
        "branch": args.branch,
        "commit": commit,
        "scanned_files": len(scanned_source_files),
        "model_reviewed_files": len(source_files) - len(numeric_table_audits),
        "numeric_table_audits": list(numeric_table_audits.values()),
        "review_units": len(files),
        "scan_manifest": os.path.abspath(scan_path),
        "segmented_files": segmented_files,
        "log_statements": len(logs),
        "findings": len(findings),
        "by_rule": by_rule,
        "by_severity": by_severity,
        "blacklist": bl,
        "model": model,
        "require_model_match": require_match,
        "complete": complete,
        "llm_errors": len(failed),
        "failed_requests": failed,
        "evaluation_started_at": evaluation_started_at,
        "evaluation_finished_at": evaluation_finished_at,
        "elapsed_s": elapsed,
        "requests": len(requests),
        "sent_chars": sum(request_sizes),
        "source_request_chars": sum(request_sizes),
        "min_request_chars": min(request_sizes, default=0),
        "avg_request_chars": round(sum(request_sizes) / max(1, len(request_sizes)), 1),
        "max_request_chars": max(request_sizes, default=0),
        "max_input_chars": args.max_input_chars,
        "target_input_chars": target_input_chars,
        "rule_target_input_chars": rule_target_input_chars,
        "output_budget": {
            "planning_output_tokens": planning_output_tokens,
            "log_density_per_million": args.log_density_per_million,
            "output_tokens_per_log": args.output_tokens_per_log,
            "safety_ratio": args.output_safety_ratio,
            "measured_density_per_million": round(
                len(logs) / max(sum(request_sizes) / 1000000.0, 1e-9), 1
            )
            if request_sizes
            else 0,
        },
        "max_output_tokens": args.max_output_tokens,
        "files_per_request": [len(group) for group in requests],
        "workers": args.workers,
        "usage": usage_total,
        "request_timings": request_timings,
        "stages": {
            "log_extraction": {
                "thinking": model_config.request_settings(model, "disabled", None)[0],
                "reasoning_effort": model_config.request_settings(
                    model, "disabled", None
                )[1],
                "complete": not any(
                    item.get("type") != "rule_review" for item in failed
                ),
                "requests": len(requests),
            },
            "rule_review": {
                "thinking": "enabled",
                "reasoning_effort": "high",
                "complete": not rule_failures,
                "requests": len(rule_groups),
            },
        },
        "file_atomic": not segmented_files,
        "full_source_coverage": True,
        "full_file_scan": not segmented_files,
        "input_scope": scope,
        "files_from": os.path.abspath(args.files_from) if args.files_from else None,
        "repository_review": repository_review,
        "spec_sources": log_spec.SPEC_SOURCES,
        "rules_applied": list(RULE_NAMES),
        "deterministic_rules": list(det_rules.DETERMINISTIC_RULES),
        "output_guard_version": det_rules.DET_RULES_VERSION,
        "output_guard_dropped": dict(output_guard_dropped),
        "evaluation_fingerprint": fingerprint,
    }
    meta_path = os.path.join(args.out, "%s_meta.json" % artifact_name)
    with open(meta_path, "w", encoding="utf-8") as fh:
        json.dump(meta, fh, ensure_ascii=False, indent=2)

    status = "ok" if complete else "error"
    print(
        "[%s] %s：%s识别全部日志语句 %d 条，发现问题 %d 处，耗时 %ss"
        % (
            status,
            "完成" if complete else "未完成",
            "模型与全文件语法审查" if numeric_table_audits else "模型",
            len(logs),
            len(findings),
            elapsed,
        )
    )
    print("[ok] 全部日志: %s" % logs_path)
    print("[ok] 问题明细: %s" % findings_path)
    if failed:
        print(
            "[error] %d 个请求失败，报告不完整；成功请求已保存，重跑可续传。"
            % len(failed),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
