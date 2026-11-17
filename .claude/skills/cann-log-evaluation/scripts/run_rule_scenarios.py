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

"""规则场景拦截测试运行器。

按 ``references/log-rules.md`` 的规则逐条构造本地场景仓，验证评测 Agent
（``llm_evaluate.py``）能否正确拦截规则问题、是否会把合规写法误报。

两种模式：

- ``pipeline``（默认，离线）：在进程内调用 ``llm_evaluate.main()``，用
  ``FixtureModel`` 替换 ``call_llm``。假模型按 manifest 中的 ``expected_logs``
  返回日志识别结果，按 ``must_find`` / ``must_find_any`` 返回规则问题，
  用来验证从扫描、日志识别、规则评测到最终护栏的整条链路。
- ``live``（真实模型）：每个场景用子进程调用 ``llm_evaluate.py``，沿用当前
  环境里的 LLM 配置，用于测量真实模型的召回与精确率。

计分口径：

- ``caught``：``must_find``（或 ``must_find_any`` 任一）在目标规则 findings 中命中；
- ``missed``：未命中；若预期日志未被识别（不在 ``<id>_logs.jsonl``）分类为
  ``extraction_miss``，否则分类为 ``rule_miss``；
- ``false_positive``：目标规则 findings 落在 ``must_find`` / ``must_find_any``
  之外（含 ``must_not_find`` 位置）；控制仓任何 finding 都算误报；
- ``recall = caught / expected``；``precision = caught / (caught + fp)``；
- ``expectation="protocol_limited"`` 的场景照跑但不计入召回/精确率门槛。

只依赖标准库。
"""

import argparse
import contextlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
SKILL_ROOT = os.path.dirname(SCRIPTS)
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import llm_evaluate  # noqa: E402
import log_spec  # noqa: E402

DEFAULT_SCENARIOS = os.path.join(SKILL_ROOT, "tests", "scenarios")
DEFAULT_OUT = os.path.join(SKILL_ROOT, "output", "scenarios")
SCAN_EXTENSIONS = tuple(log_spec.scan_extensions())
PIPELINE_ENV = {
    "LLM_API_BASE": "http://fixture.local/v1",
    "LLM_API_KEY": "fixture-key",
    "LLM_MODEL": "fixture-model",
}
CONTROL_IDS = (
    "control-clean",
    "control-ut-print",
    "control-wrapper-macro",
    "control-negation",
)
DETERMINISTIC_RULE_IDS = (1, 12)
PROTOCOL_LIMITED_RULE_ID = 25
REPOSITORY_RULE_ID = 10
DUPLICATE_RULE_ID = 26
RULE_BY_NAME = {rule["name"]: rule for rule in log_spec.RULES}
RULE_NAMES = tuple(log_spec.rule_names())


# ---------------------------------------------------------------------------
# manifest
# ---------------------------------------------------------------------------
def load_manifest(scenarios_dir):
    """读取场景目录下的 manifest.json，兼容数组和 {"scenarios": [...]}。"""
    path = os.path.join(scenarios_dir, "manifest.json")
    with open(path, "r", encoding="utf-8-sig") as handle:
        data = json.load(handle)
    if isinstance(data, dict):
        data = data.get("scenarios") or []
    if not isinstance(data, list):
        raise ValueError("%s 必须是场景数组或包含 scenarios 数组的对象" % path)
    return data


def _repo_source_files(repo):
    """返回场景仓内参与扫描的源码相对路径，保持稳定顺序。"""
    output = []
    if not os.path.isdir(repo):
        return output
    for dirpath, dirnames, filenames in os.walk(repo):
        dirnames.sort()
        for name in sorted(filenames):
            if os.path.splitext(name)[1].lower() in SCAN_EXTENSIONS:
                rel = os.path.relpath(os.path.join(dirpath, name), repo)
                output.append(rel.replace("\\", "/"))
    return output


def _read_lines(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read().splitlines()


def _check_refs(entry, repo, source_set, line_counts, problems):
    """校验 must_find/must_not_find/expected_logs 的文件与行号。"""
    scenario_id = entry.get("id")
    for key in ("must_find", "must_find_any", "must_not_find", "expected_logs"):
        value = entry.get(key)
        if value is None:
            value = []
        if not isinstance(value, list):
            problems.append("%s: %s 必须是数组" % (scenario_id, key))
            continue
        for item in value:
            if not isinstance(item, dict):
                problems.append("%s: %s 条目必须是对象" % (scenario_id, key))
                continue
            file_name = str(item.get("file") or "").replace("\\", "/")
            if file_name not in source_set:
                problems.append(
                    "%s: %s 引用了不存在的文件 %r" % (scenario_id, key, file_name)
                )
                continue
            line = item.get("line")
            if (
                not isinstance(line, int)
                or isinstance(line, bool)
                or not (1 <= line <= line_counts[file_name])
            ):
                problems.append(
                    "%s: %s 行号越界 %r (%s:%r)"
                    % (scenario_id, key, item.get("line"), file_name, item.get("line"))
                )
                continue
            if key in ("must_find", "must_find_any"):
                needle = str(item.get("code_contains") or "")
                if not needle:
                    problems.append("%s: %s 缺少 code_contains" % (scenario_id, key))
                    continue
                source_line = _read_lines(os.path.join(repo, file_name))[line - 1]
                if needle not in source_line:
                    problems.append(
                        "%s: %s 的 code_contains=%r 不在 %s:%d 中"
                        % (scenario_id, key, needle, file_name, line)
                    )
            if key == "expected_logs":
                end = item.get("end")
                if end is not None and (
                    not isinstance(end, int)
                    or end < line
                    or end > line_counts[file_name]
                ):
                    problems.append(
                        "%s: expected_logs 结束行号越界 %r" % (scenario_id, end)
                    )


def validate_manifest(manifest, scenarios_dir):
    """校验 manifest 与 log_spec、场景仓文件的一致性，返回问题列表。"""
    problems = []
    if not isinstance(manifest, list) or not manifest:
        return ["manifest 为空或不是数组"]
    scenario_ids = set()
    for entry in manifest:
        if not isinstance(entry, dict):
            problems.append("manifest 条目必须是对象")
            continue
        scenario_id = str(entry.get("id") or "")
        if not scenario_id:
            problems.append("manifest 条目缺少 id")
            continue
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", scenario_id):
            problems.append("%s: id 必须是小写 ASCII 拼接" % scenario_id)
        if scenario_id in scenario_ids:
            problems.append("%s: id 重复" % scenario_id)
        scenario_ids.add(scenario_id)
        kind = entry.get("kind")
        if kind not in ("rule", "control"):
            problems.append("%s: kind 必须是 rule 或 control" % scenario_id)
            continue
        repo_rel = str(entry.get("repo") or "").replace("\\", "/")
        if not repo_rel:
            problems.append("%s: 缺少 repo" % scenario_id)
            continue
        if not repo_rel.startswith(scenario_id + "/"):
            problems.append("%s: repo 必须以场景 id 开头" % scenario_id)
        repo = os.path.join(scenarios_dir, repo_rel)
        if not os.path.isdir(repo):
            problems.append("%s: repo 目录不存在 %s" % (scenario_id, repo_rel))
            continue
        source_files = _repo_source_files(repo)
        if not 1 <= len(source_files) <= 3:
            problems.append(
                "%s: 源码文件数 %d 不在 1-3" % (scenario_id, len(source_files))
            )
        line_counts = {}
        for file_name in source_files:
            lines = _read_lines(os.path.join(repo, file_name))
            line_counts[file_name] = len(lines)
            if not 10 <= len(lines) <= 40:
                problems.append(
                    "%s: %s 行数 %d 不在 10-40" % (scenario_id, file_name, len(lines))
                )
        source_set = set(source_files)

        if kind == "rule":
            rule_name = entry.get("rule")
            if rule_name not in RULE_BY_NAME:
                problems.append("%s: 未知规则 %r" % (scenario_id, rule_name))
                continue
            rule = RULE_BY_NAME[rule_name]
            if entry.get("rule_id") != rule["id"]:
                problems.append("%s: rule_id 应为 %s" % (scenario_id, rule["id"]))
            if entry.get("severity") != rule["severity"]:
                problems.append(
                    "%s: severity 应为 %s" % (scenario_id, rule["severity"])
                )
            if entry.get("category") != rule["category"]:
                problems.append(
                    "%s: category 应为 %s" % (scenario_id, rule["category"])
                )
            if entry.get("scope") != rule["scope"]:
                problems.append("%s: scope 应为 %s" % (scenario_id, rule["scope"]))
            expectation = entry.get("expectation")
            if expectation not in ("must_find", "protocol_limited"):
                problems.append("%s: expectation 非法 %r" % (scenario_id, expectation))
            if rule["id"] == PROTOCOL_LIMITED_RULE_ID:
                if expectation != "protocol_limited":
                    problems.append("%s: 规则 25 必须标 protocol_limited" % scenario_id)
            elif expectation == "protocol_limited":
                problems.append("%s: 只有规则 25 可以标 protocol_limited" % scenario_id)
            if rule["id"] in (REPOSITORY_RULE_ID, DUPLICATE_RULE_ID):
                if not entry.get("must_find_any"):
                    problems.append(
                        "%s: 重复类规则必须提供 must_find_any" % scenario_id
                    )
            elif expectation != "protocol_limited" and not entry.get("must_find"):
                problems.append("%s: 缺少 must_find" % scenario_id)
            expected_deterministic = rule["id"] in DETERMINISTIC_RULE_IDS
            if bool(entry.get("deterministic")) != expected_deterministic:
                problems.append(
                    "%s: deterministic 应为 %s（规则 %s 由本地判定）"
                    % (scenario_id, expected_deterministic, rule["id"])
                )
        else:
            if entry.get("must_find") or entry.get("must_find_any"):
                problems.append(
                    "%s: 控制仓不能声明 must_find/must_find_any" % scenario_id
                )
            if entry.get("expectation") != "control":
                problems.append("%s: 控制仓 expectation 必须是 control" % scenario_id)

        _check_refs(entry, repo, source_set, line_counts, problems)

    rule_entries = [
        entry
        for entry in manifest
        if isinstance(entry, dict) and entry.get("kind") == "rule"
    ]
    names = [entry.get("rule") for entry in rule_entries]
    for rule_name in RULE_NAMES:
        count = names.count(rule_name)
        if count != 1:
            problems.append("规则 %s 的场景数应为 1，实际 %d" % (rule_name, count))
    if len(rule_entries) != len(RULE_NAMES):
        problems.append(
            "规则场景数应为 %d，实际 %d" % (len(RULE_NAMES), len(rule_entries))
        )
    rule_ids = sorted(
        entry.get("rule_id")
        for entry in rule_entries
        if isinstance(entry.get("rule_id"), int)
    )
    expected_ids = sorted(rule["id"] for rule in log_spec.RULES)
    if rule_ids != expected_ids:
        problems.append("rule_id 覆盖不完整或重复: %r" % (rule_ids,))
    control_ids = {
        entry.get("id")
        for entry in manifest
        if isinstance(entry, dict) and entry.get("kind") == "control"
    }
    if control_ids != set(CONTROL_IDS):
        problems.append(
            "控制仓应为 %s，实际 %s" % (sorted(CONTROL_IDS), sorted(control_ids))
        )
    return problems


# ---------------------------------------------------------------------------
# 假模型
# ---------------------------------------------------------------------------
def _json_after_marker(prompt, marker):
    """取提示词中 marker 行之后的第一个 JSON 值。"""
    position = prompt.find(marker)
    if position < 0:
        return None
    line_end = prompt.find("\n", position)
    if line_end < 0:
        return None
    text = prompt[slice(line_end + 1, None, None)].strip()
    try:
        value, _index = json.JSONDecoder().raw_decode(text)
    except json.JSONDecodeError:
        return None
    return value


def _same_file(left, right):
    left = str(left or "").replace("\\", "/")
    right = str(right or "").replace("\\", "/")
    return left == right or left.endswith("/" + right) or right.endswith("/" + left)


def _position(item):
    line = item.get("line")
    if isinstance(line, str) and line.strip().isdigit():
        line = int(line.strip())
    return (str(item.get("file") or "").replace("\\", "/"), int(line or 0))


REPOSITORY_RULE_LINE_RE = re.compile(
    r"(?m)^\d+\.\s*\[[^\]]*\]\s*冗余日志（固定严重级："
)


def _is_repository_prompt(prompt):
    """判断提示词是否为“只评测全仓规则”的全仓复核请求。"""
    return bool(REPOSITORY_RULE_LINE_RE.search(str(prompt or "")))


class FixtureModel:
    """离线假模型：按 manifest 决定识别哪些日志、返回哪些规则问题。"""

    def __init__(self, scenario):
        self.scenario = scenario
        self.rule = scenario.get("rule")
        self.repository = scenario.get("scope") == "repository"
        self.deterministic = bool(scenario.get("deterministic"))
        self.expected = list(scenario.get("expected_logs") or [])
        self.must_find = list(scenario.get("must_find") or [])
        self.must_find_any = list(scenario.get("must_find_any") or [])
        self.calls = 0

    # -- 日志识别 ---------------------------------------------------------
    def _extraction(self, prompt, model):
        units = re.findall(r"--- 源码单元 (\d+) \| 路径: ([^|]+?) \|", prompt)
        index = {}
        files = []
        for raw_index, raw_path in units:
            unit = int(raw_index)
            path = raw_path.strip()
            index[path] = unit
            files.append({"index": unit})
        if not files:
            raise RuntimeError("fixture 无法从提示词解析源码单元")
        logs = []
        for number, item in enumerate(self.expected, 1):
            file_name = str(item.get("file") or "").replace("\\", "/")
            unit = index.get(file_name)
            if unit is None:
                matches = [
                    value
                    for path, value in index.items()
                    if _same_file(path, file_name)
                ]
                if len(matches) != 1:
                    raise RuntimeError("fixture 无法定位源码单元: %s" % file_name)
                unit = matches[0]
            line = int(item.get("line"))
            end = int(item.get("end") or line)
            logs.append({"id": "L%d" % number, "index": unit, "line": line, "end": end})
        return (
            json.dumps({"files": files, "logs": logs}, ensure_ascii=False),
            {"_response_model": model},
        )

    # -- 规则评测 ---------------------------------------------------------
    def _wanted(self, payload):
        wanted = []

        def add(item):
            position = _position(item)
            if position not in wanted:
                wanted.append(position)

        if self.must_find:
            for item in self.must_find:
                add(item)
        elif self.must_find_any:
            for item in self.must_find_any:
                add(item)
                break
        selected = []
        for entry in payload:
            if entry.get("id") and any(
                _same_file(entry.get("file"), file_name)
                and int(entry.get("line") or 0) == line
                for file_name, line in wanted
            ):
                selected.append(entry)
        return selected

    def _issues(self, payload, model):
        if not self.rule or self.deterministic:
            return json.dumps({"issues": []}, ensure_ascii=False), {
                "_response_model": model
            }
        severity = log_spec.rule_severity(self.rule)
        issues = []
        for entry in self._wanted(payload):
            issues.append(
                {
                    "log_id": entry["id"],
                    "rule": self.rule,
                    "severity": severity,
                    "reason": "fixture: 命中规则「%s」的预期问题位置" % self.rule,
                    "suggestion": "fixture: 按规范修正后复测",
                }
            )
        return (
            json.dumps({"issues": issues}, ensure_ascii=False),
            {"_response_model": model},
        )

    def _rule_review(self, prompt, model):
        if self.repository:
            return json.dumps({"issues": []}, ensure_ascii=False), {
                "_response_model": model
            }
        payload = _json_after_marker(prompt, "【待评测日志】共 ")
        if not isinstance(payload, list):
            return json.dumps({"issues": []}, ensure_ascii=False), {
                "_response_model": model
            }
        return self._issues(payload, model)

    def _repository_review(self, prompt, model):
        if not self.repository:
            return json.dumps({"issues": []}, ensure_ascii=False), {
                "_response_model": model
            }
        payload = None
        # 现网引擎的全仓复核复用规则评测模板（只把规则集缩为“冗余日志”）；
        # 这里同时兼容文档中的专用提示词标记，避免依赖单一措辞。
        for marker in ("【全仓完整日志清单】共 ", "【待评测日志】共 "):
            payload = _json_after_marker(prompt, marker)
            if isinstance(payload, list):
                break
        if not isinstance(payload, list):
            return json.dumps({"issues": []}, ensure_ascii=False), {
                "_response_model": model
            }
        return self._issues(payload, model)

    # -- call_llm 兼容入口 -------------------------------------------------
    def __call__(self, *args, **kwargs):
        prompt = kwargs.get("prompt")
        model = kwargs.get("model")
        if prompt is None:
            if len(args) >= 4:
                model, prompt = args[2], args[3]
            else:
                prompt, model = "", ""
        self.calls += 1
        if str(prompt).startswith("你是 CANN 源码日志识别专家"):
            return self._extraction(prompt, model)
        if "【全仓完整日志清单】共 " in prompt:
            return self._repository_review(prompt, model)
        if "【待评测日志】共 " in prompt:
            # 全仓复核与单请求规则评测共用同一日志清单标记；全仓复核的
            # 规则清单只含全仓规则（当前为“冗余日志”），据此区分。
            if _is_repository_prompt(prompt):
                return self._repository_review(prompt, model)
            return self._rule_review(prompt, model)
        return json.dumps({"issues": []}, ensure_ascii=False), {
            "_response_model": model
        }


# ---------------------------------------------------------------------------
# 执行场景
# ---------------------------------------------------------------------------
def _scenario_paths(scenario, scenarios_dir, work_root):
    repo = os.path.join(scenarios_dir, scenario["repo"])
    out_dir = os.path.join(work_root, scenario["id"])
    os.makedirs(out_dir, exist_ok=True)
    return repo, out_dir


def _read_jsonl(path):
    rows = []
    if not os.path.isfile(path):
        return rows
    with open(path, "r", encoding="utf-8-sig") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return rows


def _read_json(path):
    if not os.path.isfile(path):
        return {}
    try:
        with open(path, "r", encoding="utf-8-sig") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError):
        return {}


def _log_tail(path, limit=4000):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError:
        return ""
    return text[-limit:]


def _collect_artifacts(scenario, out_dir, extra=None):
    scenario_id = scenario["id"]
    result = {
        "findings": _read_jsonl(
            os.path.join(out_dir, "%s_findings.jsonl" % scenario_id)
        ),
        "logs": _read_jsonl(os.path.join(out_dir, "%s_logs.jsonl" % scenario_id)),
        "meta": _read_json(os.path.join(out_dir, "%s_meta.json" % scenario_id)),
    }
    result.update(extra or {})
    return result


def run_pipeline_scenario(scenario, scenarios_dir, work_root):
    """在进程内跑一次离线 pipeline，返回 findings/logs 与状态。"""
    repo, out_dir = _scenario_paths(scenario, scenarios_dir, work_root)
    scenario_id = scenario["id"]
    argv = [
        "llm_evaluate.py",
        "--repo",
        repo,
        "--repo-name",
        scenario_id,
        "--out",
        out_dir,
        "--workers",
        "1",
        "--timeout",
        "600",
        "--retries",
        "1",
        "--allow-model-alias",
        "--target-input-chars",
        "20000",
        "--max-input-chars",
        "200000",
    ]
    fixture = FixtureModel(scenario)
    run_log = os.path.join(out_dir, "scenario-run.log")
    started = time.monotonic()
    error = None
    with open(run_log, "w", encoding="utf-8") as handle:
        try:
            with (
                contextlib.redirect_stdout(handle),
                contextlib.redirect_stderr(handle),
                _mock_environment(PIPELINE_ENV),
                _mock_arg(argv),
                _mock_call_llm(fixture),
            ):
                code = llm_evaluate.main()
            if code != 0:
                error = "llm_evaluate.main() 返回 %s" % code
        except BaseException as exc:  # noqa: BLE001 - 单场景失败不能中断整体
            error = "%s: %s" % (type(exc).__name__, exc)
    elapsed = round(time.monotonic() - started, 3)
    return _collect_artifacts(
        scenario,
        out_dir,
        {
            "error": error,
            "elapsed_s": elapsed,
            "fixture_calls": fixture.calls,
            "log_tail": _log_tail(run_log) if error else "",
            "workdir": out_dir,
        },
    )


@contextlib.contextmanager
def _mock_environment(values):
    from unittest import mock

    with mock.patch.dict(os.environ, values):
        yield


@contextlib.contextmanager
def _mock_arg(argv):
    from unittest import mock

    with mock.patch.object(sys, "argv", argv):
        yield


@contextlib.contextmanager
def _mock_call_llm(fixture):
    from unittest import mock

    with mock.patch.object(llm_evaluate, "call_llm", fixture):
        yield


def _live_command(scenario, scenarios_dir, work_root):
    repo, out_dir = _scenario_paths(scenario, scenarios_dir, work_root)
    return (
        repo,
        out_dir,
        [
            sys.executable,
            os.path.join(SCRIPTS, "llm_evaluate.py"),
            "--repo",
            repo,
            "--repo-name",
            scenario["id"],
            "--out",
            out_dir,
            "--workers",
            "1",
            "--timeout",
            "600",
            "--retries",
            "1",
            "--allow-model-alias",
            "--target-input-chars",
            "20000",
            "--max-input-chars",
            "200000",
        ],
    )


def run_live_scenario(scenario, scenarios_dir, work_root):
    """子进程调用真实 llm_evaluate.py；失败不抛异常，记录到 error。"""
    repo, out_dir, command = _live_command(scenario, scenarios_dir, work_root)
    run_log = os.path.join(out_dir, "scenario-run.log")
    started = time.monotonic()
    error = None
    try:
        completed = subprocess.run(
            command,
            cwd=SKILL_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=3600,
        )
        with open(run_log, "w", encoding="utf-8") as handle:
            handle.write(completed.stdout or "")
            handle.write(completed.stderr or "")
        if completed.returncode != 0:
            error = "llm_evaluate.py 退出码 %s" % completed.returncode
    except subprocess.TimeoutExpired:
        error = "llm_evaluate.py 超时"
    except OSError as exc:
        error = "无法启动 llm_evaluate.py: %s" % exc
    elapsed = round(time.monotonic() - started, 3)
    return _collect_artifacts(
        scenario,
        out_dir,
        {
            "error": error,
            "elapsed_s": elapsed,
            "fixture_calls": 0,
            "log_tail": _log_tail(run_log) if error else "",
            "workdir": out_dir,
        },
    )


# ---------------------------------------------------------------------------
# 计分
# ---------------------------------------------------------------------------
def _pos(item):
    file_name = str(item.get("file") or "").replace("\\", "/")
    line = item.get("line")
    if isinstance(line, str) and line.strip().isdigit():
        line = int(line.strip())
    return (file_name, int(line or 0))


def _line_snippet(scenario, scenarios_dir, file_name, line):
    repo = os.path.join(scenarios_dir, scenario["repo"])
    path = os.path.join(repo, str(file_name).replace("\\", "/"))
    try:
        lines = _read_lines(path)
    except OSError:
        return ""
    if 1 <= int(line) <= len(lines):
        return lines[int(line) - 1].strip()
    return ""


def _miss_detail(scenario, scenarios_dir, item, log_positions):
    position = _pos(item)
    kind = "rule_miss" if position in log_positions else "extraction_miss"
    return {
        "file": position[0],
        "line": position[1],
        "type": kind,
        "code": _line_snippet(scenario, scenarios_dir, position[0], position[1]),
    }


def _finding_detail(finding, scenario=None, scenarios_dir=""):
    detail = {
        "rule": finding.get("rule"),
        "severity": finding.get("severity"),
        "file": finding.get("file"),
        "line": finding.get("line"),
        "end": finding.get("end"),
        "code": finding.get("code") or "",
        "reason": finding.get("reason") or "",
    }
    if not detail["code"] and scenario is not None and scenarios_dir:
        detail["code"] = _line_snippet(
            scenario, scenarios_dir, detail["file"], detail["line"]
        )
    return detail


def score_scenario(scenario, run, scenarios_dir):
    """把一次运行结果按 manifest 计分。"""
    findings = list(run.get("findings") or [])
    logs = list(run.get("logs") or [])
    log_positions = {_pos(item) for item in logs}
    kind = scenario.get("kind")
    rule = scenario.get("rule")
    expectation = scenario.get("expectation") or (
        "control" if kind == "control" else "must_find"
    )
    must_find = list(scenario.get("must_find") or [])
    must_find_any = list(scenario.get("must_find_any") or [])
    allowed = {_pos(item) for item in must_find} | {
        _pos(item) for item in must_find_any
    }
    protocol_limited = expectation == "protocol_limited"
    if kind == "rule" and rule:
        target = [
            item
            for item in findings
            if log_spec.normalize_rule(str(item.get("rule") or "")) == rule
        ]
    else:
        target = []

    expected = 0
    caught = 0
    missed = []
    false_positives = []
    if kind == "control":
        _tmp_result = []
        for item in findings:
            _tmp_result.append(_finding_detail(item, scenario, scenarios_dir))
        false_positives = _tmp_result
    elif protocol_limited:
        # 缺失类问题无法用“引用真实 log_id”的 finding 协议表达，不计门槛。
        pass
    else:
        if must_find:
            expected = len(must_find)
            hit_positions = {_pos(item) for item in target}
            for item in must_find:
                position = _pos(item)
                if position in hit_positions:
                    caught += 1
                else:
                    missed.append(
                        _miss_detail(scenario, scenarios_dir, item, log_positions)
                    )
        elif must_find_any:
            expected = 1
            hit = next((item for item in target if _pos(item) in allowed), None)
            if hit is not None:
                caught = 1
            else:
                _tmp_result = []
                for item in must_find_any:
                    if _pos(item) in log_positions:
                        _tmp_result.append(item)
                extracted = _tmp_result
                missed.append(
                    _miss_detail(
                        scenario,
                        scenarios_dir,
                        extracted[0] if extracted else must_find_any[0],
                        log_positions,
                    )
                )
        false_positives = [
            _finding_detail(item, scenario, scenarios_dir)
            for item in target
            if _pos(item) not in allowed
        ]

    error = run.get("error")
    if kind == "control":
        passed = not error and not findings
    elif protocol_limited:
        passed = not error
    else:
        passed = not error and caught == expected and not false_positives
    return {
        "id": scenario.get("id"),
        "kind": kind,
        "rule_id": scenario.get("rule_id"),
        "rule": rule,
        "severity": scenario.get("severity"),
        "category": scenario.get("category"),
        "scope": scenario.get("scope"),
        "expectation": expectation,
        "protocol_limited": protocol_limited,
        "deterministic": bool(scenario.get("deterministic")),
        "expected": expected,
        "caught": caught,
        "missed": missed,
        "false_positives": false_positives,
        "pass": passed,
        "error": error,
        "findings": len(findings),
        "target_findings": len(target),
        "logs": len(logs),
        "fixture_calls": run.get("fixture_calls", 0),
        "elapsed_s": run.get("elapsed_s"),
        "log_tail": run.get("log_tail") or "",
        "notes": scenario.get("notes") or "",
    }


def _merge_attempts(scenario, attempts):
    """多次 repeat 的保守合并：任一次失败即失败，命中取最少，误报取并集。"""
    if len(attempts) == 1:
        return attempts[0]
    merged = dict(attempts[0])
    merged["expected"] = attempts[0]["expected"]
    merged["caught"] = min(item["caught"] for item in attempts)
    worst = min(attempts, key=lambda item: item["caught"])
    merged["missed"] = worst["missed"]
    seen = set()
    false_positives = []
    for item in attempts:
        for detail in item["false_positives"]:
            key = (
                detail.get("file"),
                detail.get("line"),
                detail.get("rule"),
                detail.get("reason"),
            )
            if key not in seen:
                seen.add(key)
                false_positives.append(detail)
    merged["false_positives"] = false_positives
    merged["pass"] = all(item["pass"] for item in attempts)
    merged["error"] = next((item["error"] for item in attempts if item["error"]), None)
    merged["attempts"] = len(attempts)
    return merged


# ---------------------------------------------------------------------------
# 汇总与报告
# ---------------------------------------------------------------------------
def _split_ids(value):
    return [item.strip() for item in str(value or "").split(",") if item.strip()]


def summarize(
    results, mode, scenarios_dir, out_dir, min_recall, min_precision, elapsed_s, repeat
):
    counted = [
        item
        for item in results
        if item.get("kind") == "rule" and not item.get("protocol_limited")
    ]
    controls = [item for item in results if item.get("kind") == "control"]
    limited = [item for item in results if item.get("protocol_limited")]
    expected = sum(item["expected"] for item in counted)
    caught = sum(item["caught"] for item in counted)
    false_positives = sum(len(item["false_positives"]) for item in counted)
    control_findings = sum(len(item["false_positives"]) for item in controls)
    false_positives += control_findings
    recall = (float(caught) / expected) if expected else 1.0
    precision = (
        float(caught) / (caught + false_positives)
        if (caught + false_positives)
        else 1.0
    )
    threshold_failed = []
    if min_recall is not None and recall < min_recall:
        threshold_failed.append("recall")
    if min_precision is not None and precision < min_precision:
        threshold_failed.append("precision")
    return {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "mode": mode,
        "scenarios_dir": os.path.abspath(scenarios_dir),
        "out_dir": os.path.abspath(out_dir),
        "repeat": repeat,
        "elapsed_s": round(elapsed_s, 3),
        "thresholds": {
            "min_recall": min_recall,
            "min_precision": min_precision,
            "failed": threshold_failed,
        },
        "summary": {
            "scenarios_total": len(results),
            "rule_scenarios": sum(1 for item in results if item.get("kind") == "rule"),
            "counted_rule_scenarios": len(counted),
            "control_scenarios": len(controls),
            "protocol_limited": len(limited),
            "expected": expected,
            "caught": caught,
            "missed": expected - caught,
            "false_positives": false_positives,
            "control_findings": control_findings,
            "recall": recall,
            "precision": precision,
            "all_pass": all(item["pass"] for item in results),
            "failed_scenarios": [item["id"] for item in results if not item["pass"]],
            "error_scenarios": [item["id"] for item in results if item["error"]],
            "threshold_failed": threshold_failed,
        },
        "results": results,
    }


def _percent(value):
    return "%.1f%%" % (float(value) * 100.0)


def _cell(value):
    text = "" if value is None else str(value)
    return text.replace("|", "\\|").replace("\n", " ")


def _result_label(item):
    if item.get("error"):
        return "ERROR"
    if item.get("protocol_limited"):
        return "SKIP(protocol_limited)"
    return "PASS" if item.get("pass") else "FAIL"


def build_markdown(summary):
    """生成 Markdown 计分卡。"""
    data = summary["summary"]
    lines = []
    lines.append("# 规则场景拦截计分卡")
    lines.append("")
    lines.append("- 生成时间：%s" % summary["generated_at"])
    lines.append("- 模式：`%s`" % summary["mode"])
    lines.append("- 场景目录：`%s`" % summary["scenarios_dir"])
    lines.append(
        "- 场景总数：%d（规则 %d / 控制 %d）"
        % (data["scenarios_total"], data["rule_scenarios"], data["control_scenarios"])
    )
    lines.append(
        "- 计入门槛场景：%d（其中 protocol_limited %d 个不计门槛）"
        % (data["counted_rule_scenarios"], data["protocol_limited"])
    )
    lines.append(
        "- 召回率：**%s**（%d/%d）"
        % (_percent(data["recall"]), data["caught"], data["expected"])
    )
    lines.append(
        "- 精确率：**%s**（误报 %d 条，含控制仓 findings %d 条）"
        % (
            _percent(data["precision"]),
            data["false_positives"],
            data["control_findings"],
        )
    )
    lines.append(
        "- 用时：%.2fs（repeat=%d）" % (summary["elapsed_s"], summary["repeat"])
    )
    thresholds = summary["thresholds"]
    if thresholds["min_recall"] is None and thresholds["min_precision"] is None:
        lines.append("- 门槛：未设置（只报告，不因召回/精确率退出非零）")
    else:
        lines.append(
            "- 门槛：recall >= %s，precision >= %s；结果：%s"
            % (
                thresholds["min_recall"],
                thresholds["min_precision"],
                "未达标（%s）" % "、".join(thresholds["failed"])
                if thresholds["failed"]
                else "达标",
            )
        )
    lines.append("- 总判定：%s" % ("PASS" if data["all_pass"] else "FAIL"))
    lines.append("")

    lines.append("## 逐规则结果")
    lines.append("")
    lines.append("| 场景 | 规则 | 严重级 | 预期 | 命中 | 漏报 | 误报 | 结果 |")
    lines.append("|------|------|--------|------|------|------|------|------|")
    for item in summary["results"]:
        if item.get("kind") == "rule":
            rule_label = "规则 %s %s" % (item.get("rule_id"), item.get("rule"))
            severity = item.get("severity") or ""
        else:
            rule_label = "控制仓"
            severity = "-"
        if item.get("protocol_limited"):
            expected_label = "—"
            caught_label = "—"
        else:
            expected_label = item["expected"]
            caught_label = item["caught"]
        lines.append(
            "| %s | %s | %s | %s | %s | %d | %d | %s |"
            % (
                _cell(item["id"]),
                _cell(rule_label),
                _cell(severity),
                expected_label,
                caught_label,
                len(item["missed"]),
                len(item["false_positives"]),
                _result_label(item),
            )
        )
    lines.append("")

    lines.append("## 漏报明细")
    lines.append("")
    _tmp_result = []
    for item in summary["results"]:
        for detail in item["missed"]:
            _tmp_result.append((item, detail))
    missed_rows = _tmp_result
    if not missed_rows:
        lines.append("无。")
    else:
        lines.append("| 场景 | 规则 | 位置 | 类型 | 代码 |")
        lines.append("|------|------|------|------|------|")
        for item, detail in missed_rows:
            lines.append(
                "| %s | %s | %s:%s | %s | `%s` |"
                % (
                    _cell(item["id"]),
                    _cell(item.get("rule") or ""),
                    _cell(detail.get("file")),
                    detail.get("line"),
                    detail.get("type"),
                    _cell(detail.get("code")),
                )
            )
    lines.append("")

    lines.append("## 误报明细")
    lines.append("")
    fp_rows = [
        (item, detail)
        for item in summary["results"]
        for detail in item["false_positives"]
    ]
    if not fp_rows:
        lines.append("无。")
    else:
        lines.append("| 场景 | 规则 | 位置 | 严重级 | 原因 | 代码 |")
        lines.append("|------|------|------|--------|------|------|")
        for item, detail in fp_rows:
            lines.append(
                "| %s | %s | %s:%s | %s | %s | `%s` |"
                % (
                    _cell(item["id"]),
                    _cell(detail.get("rule") or ""),
                    _cell(detail.get("file")),
                    detail.get("line"),
                    _cell(detail.get("severity") or ""),
                    _cell(detail.get("reason")),
                    _cell(detail.get("code")),
                )
            )
    lines.append("")

    if data["error_scenarios"] or data["failed_scenarios"]:
        lines.append("## 未通过场景")
        lines.append("")
        failed = set(data["error_scenarios"]) | set(data["failed_scenarios"])
        lines.append("| 场景 | 错误信息 | 日志尾部 |")
        lines.append("|------|----------|----------|")
        for item in summary["results"]:
            if item["id"] not in failed:
                continue
            tail = item.get("log_tail") or ""
            tail = tail[-1200:].replace("|", "\\|").replace("\n", " ")
            lines.append(
                "| %s | %s | %s |"
                % (
                    _cell(item["id"]),
                    _cell(item.get("error") or "未命中预期"),
                    _cell(tail),
                )
            )
        lines.append("")

    lines.append("## 场景设计说明")
    lines.append("")
    lines.append("| 场景 | 规则/类型 | 说明 |")
    lines.append("|------|-----------|------|")
    for item in summary["results"]:
        if item.get("kind") == "rule":
            label = "规则 %s %s" % (item.get("rule_id"), item.get("rule"))
        else:
            label = "控制仓"
        lines.append(
            "| %s | %s | %s |"
            % (_cell(item["id"]), _cell(label), _cell(item.get("notes")))
        )
    lines.append("")
    return "\n".join(lines) + "\n"


def write_reports(summary, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    markdown_path = os.path.join(out_dir, "scenario-report.md")
    json_path = os.path.join(out_dir, "scenario-report.json")
    with open(markdown_path, "w", encoding="utf-8") as handle:
        handle.write(build_markdown(summary))
    with open(json_path, "w", encoding="utf-8") as handle:
        json.dump(summary, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    return markdown_path, json_path


# ---------------------------------------------------------------------------
# 运行入口
# ---------------------------------------------------------------------------
def run_scenarios(
    mode="pipeline",
    scenarios_dir=DEFAULT_SCENARIOS,
    out_dir=DEFAULT_OUT,
    only=None,
    jobs=1,
    repeat=1,
    keep_workdir=False,
    min_recall=None,
    min_precision=None,
):
    """运行全部（或 --only 指定）场景并写出报告，返回汇总字典。"""
    scenarios_dir = os.path.abspath(scenarios_dir)
    out_dir = os.path.abspath(out_dir)
    manifest = load_manifest(scenarios_dir)
    problems = validate_manifest(manifest, scenarios_dir)
    if problems:
        raise ValueError("manifest 校验失败：\n- " + "\n- ".join(problems))
    selected = list(manifest)
    if only:
        wanted = list(dict.fromkeys(only))
        selected = [item for item in manifest if item.get("id") in set(wanted)]
        missing = sorted(set(wanted) - {item.get("id") for item in selected})
        if missing:
            raise ValueError("--only 指定了未知场景：%s" % ", ".join(missing))
    if not selected:
        raise ValueError("没有可运行的场景")
    jobs = max(1, int(jobs))
    repeat = max(1, int(repeat))
    os.makedirs(out_dir, exist_ok=True)
    if keep_workdir:
        work_root = os.path.join(out_dir, "work")
        os.makedirs(work_root, exist_ok=True)
    else:
        work_root = tempfile.mkdtemp(prefix="scenario-work-", dir=out_dir)
    runner = run_pipeline_scenario if mode == "pipeline" else run_live_scenario
    started = time.monotonic()
    scored = []
    try:
        if mode == "pipeline" or jobs == 1:
            if mode == "pipeline" and jobs > 1:
                print(
                    "[warn] pipeline 模式在进程内替换 call_llm，强制串行执行",
                    flush=True,
                )
            for scenario in selected:
                _tmp_result = []
                for _ in range(repeat):
                    _tmp_result.append(runner(scenario, scenarios_dir, work_root))
                attempts = _tmp_result
                scored.append(
                    _merge_attempts(
                        scenario,
                        [
                            score_scenario(scenario, attempt, scenarios_dir)
                            for attempt in attempts
                        ],
                    )
                )
        else:
            with ThreadPoolExecutor(max_workers=jobs) as executor:
                futures = {
                    executor.submit(
                        _run_live_repeated, scenario, scenarios_dir, work_root, repeat
                    ): scenario
                    for scenario in selected
                }
                for future in as_completed(futures):
                    scenario = futures[future]
                    try:
                        scored.append(future.result())
                    except BaseException as exc:  # noqa: BLE001
                        scored.append(
                            score_scenario(
                                scenario,
                                {
                                    "error": "%s: %s" % (type(exc).__name__, exc),
                                    "findings": [],
                                    "logs": [],
                                },
                                scenarios_dir,
                            )
                        )
            order = {item.get("id"): index for index, item in enumerate(selected)}
            scored.sort(key=lambda item: order.get(item["id"], 0))
    finally:
        if not keep_workdir:
            shutil.rmtree(work_root, ignore_errors=True)
    elapsed = time.monotonic() - started
    summary = summarize(
        scored, mode, scenarios_dir, out_dir, min_recall, min_precision, elapsed, repeat
    )
    write_reports(summary, out_dir)
    return summary


def _run_live_repeated(scenario, scenarios_dir, work_root, repeat):
    _tmp_result = []
    for _ in range(repeat):
        _tmp_result.append(run_live_scenario(scenario, scenarios_dir, work_root))
    attempts = _tmp_result
    return _merge_attempts(
        scenario,
        [score_scenario(scenario, attempt, scenarios_dir) for attempt in attempts],
    )


def print_summary(summary, markdown_path, json_path):
    data = summary["summary"]
    print(
        "[info] 模式=%s 场景=%d 规则=%d 控制=%d 计入=%d protocol_limited=%d"
        % (
            summary["mode"],
            data["scenarios_total"],
            data["rule_scenarios"],
            data["control_scenarios"],
            data["counted_rule_scenarios"],
            data["protocol_limited"],
        )
    )
    print(
        "[%s] recall=%s (%d/%d) precision=%s 漏报=%d 误报=%d 控制仓findings=%d"
        % (
            "ok"
            if data["recall"] >= (summary["thresholds"]["min_recall"] or 0)
            and data["precision"] >= (summary["thresholds"]["min_precision"] or 0)
            else "warn",
            _percent(data["recall"]),
            data["caught"],
            data["expected"],
            _percent(data["precision"]),
            data["missed"],
            data["false_positives"],
            data["control_findings"],
        )
    )
    for item in summary["results"]:
        if item["pass"]:
            continue
        reason = item.get("error") or "未命中预期/存在误报"
        print("[fail] %-22s %s" % (item["id"], reason))
    if data["threshold_failed"]:
        print("[fail] 门槛未达标: %s" % "、".join(data["threshold_failed"]))
    print(
        "[%s] 场景%s"
        % (
            "ok" if data["all_pass"] else "warn",
            "全部通过" if data["all_pass"] else "存在未通过项",
        )
    )
    print("[ok] 计分卡: %s" % markdown_path)
    print("[ok] JSON: %s" % json_path)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="CANN 日志规范规则场景拦截测试（pipeline 离线假模型 / live 真实模型）"
    )
    parser.add_argument(
        "--scenarios",
        default=DEFAULT_SCENARIOS,
        help="场景目录（默认 tests/scenarios）",
    )
    parser.add_argument("--only", default="", help="只运行给定场景 id，逗号分隔")
    parser.add_argument(
        "--out", default=DEFAULT_OUT, help="报告输出目录（默认 output/scenarios）"
    )
    parser.add_argument(
        "--keep-workdir",
        action="store_true",
        help="保留每个场景的 llm_evaluate 临时工作目录",
    )
    parser.add_argument("--jobs", type=int, default=1, help="并行场景数（默认 1）")
    parser.add_argument(
        "--repeat", type=int, default=1, help="每个场景重复次数（默认 1）"
    )
    parser.add_argument(
        "--min-recall", type=float, default=None, help="召回率门槛；未设置时只报告"
    )
    parser.add_argument(
        "--min-precision", type=float, default=None, help="精确率门槛；未设置时只报告"
    )
    parser.add_argument(
        "--mode",
        choices=("pipeline", "live"),
        default="pipeline",
        help="pipeline=离线假模型校准链路；live=真实模型测量",
    )
    args = parser.parse_args(argv)

    try:
        summary = run_scenarios(
            mode=args.mode,
            scenarios_dir=args.scenarios,
            out_dir=args.out,
            only=_split_ids(args.only),
            jobs=args.jobs,
            repeat=args.repeat,
            keep_workdir=args.keep_workdir,
            min_recall=args.min_recall,
            min_precision=args.min_precision,
        )
    except ValueError as exc:
        print("[error] %s" % exc, file=sys.stderr)
        return 2
    markdown_path = os.path.join(os.path.abspath(args.out), "scenario-report.md")
    json_path = os.path.join(os.path.abspath(args.out), "scenario-report.json")
    print_summary(summary, markdown_path, json_path)
    data = summary["summary"]
    if data["error_scenarios"] or data["threshold_failed"]:
        return 1
    if summary["mode"] == "pipeline" and not data["all_pass"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
