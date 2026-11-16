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

"""规则直审（probe）：跳过日志识别，直接把场景日志送规则审查。

用途：与 run_rule_scenarios.py 的 live 全链路结果交叉对比，区分：
- 全链路 miss + 直审 hit  → 识别/筛选链路问题
- 全链路 miss + 直审 miss → 规则提示词/模型判定/用例问题

用法：
    python3 rule_probe.py --scenarios tests/scenarios --out output/scenarios/probe
    python3 rule_probe.py --env-file ~/.config/cann-log-eval/cannbot.env
    python3 rule_probe.py --only rule-04,rule-16 --jobs 3
"""

import argparse
import json
import os
import shlex
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import llm_evaluate  # noqa: E402
import log_spec  # noqa: E402
import prescan  # noqa: E402

SKILL_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SCENARIOS = str(SKILL_ROOT / "tests" / "scenarios")
DEFAULT_OUT = str(SKILL_ROOT / "output" / "scenarios" / "probe")
CONTROL_FOR_RULE = {
    "control-ut-print": "禁止自定义打印",
    "control-wrapper-macro": "禁止自定义打印",
    "control-negation": "级别错配",
    "control-clean": None,
}
REPO_RULES = tuple(log_spec.repository_rule_names())
SINGLE_RULES = tuple(name for name in log_spec.rule_names() if name not in REPO_RULES)


def load_env_file(path):
    """加载 KEY=VALUE 环境文件；不打印任何值。"""
    values = {}
    with open(path, "r", encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            if line.startswith("export "):
                line = line[slice(len("export "), None, None)].strip()
            name, value = line.split("=", 1)
            name, value = name.strip(), value.strip()
            try:
                parts = shlex.split(value)
                value = parts[0] if parts else ""
            except ValueError:
                pass
            values[name] = value
    for name, value in values.items():
        os.environ[name] = value
    return sorted(values)


def load_manifest(scenarios_dir):
    path = Path(scenarios_dir) / "manifest.json"
    with path.open("r", encoding="utf-8-sig") as handle:
        value = json.load(handle)
    return value if isinstance(value, list) else value.get("scenarios", [])


def _position(item):
    line = item.get("line")
    if isinstance(line, str) and line.strip().isdigit():
        line = int(line.strip())
    return (str(item.get("file") or "").replace("\\", "/"), int(line or 0))


def _read_lines(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.readlines()


def build_logs(scenario, scenarios_dir):
    """按 expected_logs 从源码还原出规则审查所需的日志对象。"""
    repo = Path(scenarios_dir) / scenario["repo"]
    files = list(llm_evaluate.iter_code_files(str(repo), llm_evaluate.CODE_EXT, set()))
    macro_sources = [
        (os.path.relpath(path, str(repo)).replace("\\", "/"), _read_lines(path))
        for path in files
    ]
    definitions = prescan.collect_macro_definitions(macro_sources)
    logs = []
    for index, expected in enumerate(scenario.get("expected_logs") or [], 1):
        relative = str(expected.get("file") or "").replace("\\", "/")
        line = int(expected.get("line") or 0)
        end = int(expected.get("end") or line)
        path = repo / relative
        if not path.is_file() or line <= 0:
            continue
        lines = _read_lines(path)
        if line > len(lines):
            continue
        code = "".join(lines[slice(line - 1, end, None)])
        item = {
            "repo": scenario["id"],
            "branch": "",
            "log_id": "L%d" % index,
            "global_id": "%s:%d-%d" % (relative, line, end),
            "file": relative,
            "line": line,
            "end": end,
            "code": code,
            "source_type": llm_evaluate.classify_source(relative),
        }
        try:
            context = prescan.macro_context(code, definitions)
        except Exception:  # noqa: BLE001 - 宏上下文失败不影响主流程
            context = None
        if context:
            item["macro_context"] = context
        logs.append(item)
    return logs


def rule_names_for(scenario):
    if scenario.get("kind") == "control":
        target = CONTROL_FOR_RULE.get(scenario["id"], scenario.get("for_rule"))
        if target:
            return (target,)
        return SINGLE_RULES
    rule = scenario.get("rule") or ""
    if rule in REPO_RULES or scenario.get("scope") == "repository":
        return REPO_RULES
    return SINGLE_RULES


def score(scenario, issues, error):
    expected_rule = scenario.get("rule")
    must_find = list(scenario.get("must_find") or [])
    must_any = list(scenario.get("must_find_any") or [])
    allowed = {_position(item) for item in must_find + must_any}
    control = scenario.get("kind") == "control"
    target_rule = CONTROL_FOR_RULE.get(scenario["id"], scenario.get("for_rule"))
    normalized = [
        (log_spec.normalize_rule(str(item.get("rule") or "")), _position(item), item)
        for item in issues
    ]
    result = {
        "hit": False,
        "expected": 0,
        "false_positive": 0,
        "wrong_rule": [],
        "issues": [],
        "error": error,
    }
    if control:
        if scenario["id"] == "control-clean":
            result["false_positive"] = len(normalized)
        elif target_rule:
            result["false_positive"] = sum(
                1 for rule, _pos, _item in normalized if rule == target_rule
            )
        result["issues"] = [
            {
                "rule": rule,
                "file": pos[0],
                "line": pos[1],
                "reason": str(item.get("reason") or "")[:160],
            }
            for rule, pos, item in normalized
        ]
        return result
    if must_find:
        result["expected"] = len(must_find)
        hits = 0
        for expected in must_find:
            pos = _position(expected)
            if any(
                rule == expected_rule and position == pos
                for rule, position, _item in normalized
            ):
                hits += 1
        result["hit"] = hits == len(must_find)
        result["hit_count"] = hits
    elif must_any:
        result["expected"] = 1
        result["hit"] = any(
            rule == expected_rule and position in allowed
            for rule, position, _item in normalized
        )
        result["hit_count"] = 1 if result["hit"] else 0
    result["false_positive"] = sum(
        1
        for rule, position, _item in normalized
        if rule == expected_rule and position not in allowed
    )
    for expected in must_find + must_any:
        pos = _position(expected)
        for rule, position, item in normalized:
            if position == pos and rule != expected_rule:
                result["wrong_rule"].append(
                    {
                        "rule": rule,
                        "file": pos[0],
                        "line": pos[1],
                        "reason": str(item.get("reason") or "")[:160],
                    }
                )
    result["issues"] = [
        {
            "rule": rule,
            "file": pos[0],
            "line": pos[1],
            "reason": str(item.get("reason") or "")[:200],
            "suggestion": str(item.get("suggestion") or "")[:200],
        }
        for rule, pos, item in normalized
    ]
    return result


def run_one(scenario, scenarios_dir, timeout, retries, max_input_chars):
    if scenario.get("expectation") == "protocol_limited":
        return {
            "id": scenario["id"],
            "kind": scenario.get("kind"),
            "rule": scenario.get("rule"),
            "skipped": "protocol_limited",
        }
    started = time.monotonic()
    logs = build_logs(scenario, scenarios_dir)
    if not logs:
        return {
            "id": scenario["id"],
            "kind": scenario.get("kind"),
            "rule": scenario.get("rule"),
            "error": "no expected_logs available for probe",
        }
    api = os.environ.get("LLM_API_BASE", "").strip()
    key = os.environ.get("LLM_API_KEY", "").strip()
    model = os.environ.get("LLM_MODEL", "").strip()
    if not api or not key or not model:
        return {
            "id": scenario["id"],
            "kind": scenario.get("kind"),
            "rule": scenario.get("rule"),
            "error": "缺少 LLM_API_BASE/LLM_API_KEY/LLM_MODEL",
        }
    try:
        _logs, issues, error, usage, elapsed = llm_evaluate.review_logs_request(
            (
                logs,
                api,
                key,
                model,
                llm_evaluate.blacklist(),
                rule_names_for(scenario),
                None,
                timeout,
                retries,
                max_input_chars,
            )
        )
    except Exception as exc:  # noqa: BLE001
        issues, error, usage, elapsed = (
            [],
            "%s: %s" % (type(exc).__name__, exc),
            {},
            0.0,
        )
    result = score(scenario, issues or [], error)
    result.update(
        {
            "id": scenario["id"],
            "kind": scenario.get("kind"),
            "rule": scenario.get("rule"),
            "severity": scenario.get("severity"),
            "scope": scenario.get("scope"),
            "deterministic": bool(scenario.get("deterministic")),
            "logs_sent": len(logs),
            "usage": usage,
            "elapsed_s": elapsed or round(time.monotonic() - started, 3),
        }
    )
    return result


def summarize(results, model, started_at):
    rule_results = [
        item
        for item in results
        if item.get("kind") == "rule" and not item.get("skipped")
    ]
    controls = [item for item in results if item.get("kind") == "control"]
    expected = sum(1 for item in rule_results if item.get("expected"))
    hits = sum(1 for item in rule_results if item.get("hit"))
    fp = sum(int(item.get("false_positive") or 0) for item in rule_results + controls)
    return {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "mode": "probe",
        "model": model,
        "started_at": started_at,
        "summary": {
            "scenarios": len(results),
            "rule_scenarios": len(rule_results),
            "control_scenarios": len(controls),
            "expected": expected,
            "hit": hits,
            "missed": expected - hits,
            "false_positives": fp,
            "errors": [item["id"] for item in results if item.get("error")],
            "recall": round(hits / expected, 4) if expected else 1.0,
        },
        "results": results,
    }


def build_markdown(payload):
    data = payload["summary"]
    lines = [
        "# 规则直审判定报告（probe）",
        "",
        "- 生成时间：%s" % payload["generated_at"],
        "- 模型：`%s`" % payload["model"],
        "- 场景：%d（规则 %d / 控制 %d）"
        % (data["scenarios"], data["rule_scenarios"], data["control_scenarios"]),
        "- 直审命中：%d/%d（%.1f%%），误报 %d"
        % (
            data["hit"],
            data["expected"],
            100.0 * data["recall"],
            data["false_positives"],
        ),
        "",
        "| 场景 | 规则 | 直审命中 | 误报 | 规则错配 | 错误 |",
        "|------|------|----------|------|----------|------|",
    ]
    for item in payload["results"]:
        if item.get("skipped"):
            lines.append(
                "| %s | %s | SKIP(%s) | 0 | 0 | - |"
                % (item["id"], item.get("rule") or "-", item["skipped"])
            )
            continue
        wrong = len(item.get("wrong_rule") or [])
        lines.append(
            "| %s | %s | %s | %d | %d | %s |"
            % (
                item["id"],
                item.get("rule") or "-",
                "YES" if item.get("hit") else "NO",
                int(item.get("false_positive") or 0),
                wrong,
                (item.get("error") or "-")[:60],
            )
        )
    lines += ["", "## 直审返回的全部问题", ""]
    for item in payload["results"]:
        if not item.get("issues"):
            continue
        lines.append("### %s（%s）" % (item["id"], item.get("rule") or "控制"))
        for issue in item["issues"]:
            lines.append(
                "- `%s:%s` [%s] %s"
                % (
                    issue["file"],
                    issue["line"],
                    issue["rule"],
                    issue.get("reason") or "",
                )
            )
        lines.append("")
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description="规则直审（probe）")
    parser.add_argument("--scenarios", default=DEFAULT_SCENARIOS)
    parser.add_argument("--out", default=DEFAULT_OUT)
    parser.add_argument("--env-file")
    parser.add_argument("--only", default="")
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--retries", type=int, default=1)
    parser.add_argument("--max-input-chars", type=int, default=200000)
    args = parser.parse_args(argv)
    if args.env_file:
        loaded = load_env_file(args.env_file)
        print("[info] env-file 已加载：%s" % ", ".join(loaded))
    model = os.environ.get("LLM_MODEL", "").strip()
    if not os.environ.get("LLM_API_BASE") or not os.environ.get("LLM_API_KEY"):
        print("[error] 缺少 LLM_API_BASE / LLM_API_KEY", file=sys.stderr)
        return 2
    scenarios = load_manifest(args.scenarios)
    wanted = [item.strip() for item in args.only.split(",") if item.strip()]
    if wanted:
        scenarios = [item for item in scenarios if item["id"] in set(wanted)]
    if not scenarios:
        print("[error] 没有可运行场景", file=sys.stderr)
        return 2
    started_at = datetime.now().astimezone().isoformat(timespec="seconds")
    results = []
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as executor:
        futures = {
            executor.submit(
                run_one,
                scenario,
                args.scenarios,
                args.timeout,
                args.retries,
                args.max_input_chars,
            ): scenario
            for scenario in scenarios
        }
        for future in as_completed(futures):
            results.append(future.result())
    order = {item["id"]: index for index, item in enumerate(scenarios)}
    results.sort(key=lambda item: order.get(item["id"], 0))
    payload = summarize(results, model, started_at)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "probe-report.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (out_dir / "probe-report.md").write_text(build_markdown(payload), encoding="utf-8")
    data = payload["summary"]
    print(
        "[ok] probe 场景=%d 命中=%d/%d 误报=%d 错误=%d"
        % (
            data["scenarios"],
            data["hit"],
            data["expected"],
            data["false_positives"],
            len(data["errors"]),
        )
    )
    print("[ok] 报告: %s" % (out_dir / "probe-report.md"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
