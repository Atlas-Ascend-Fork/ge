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

"""汇总 live 全链路、probe 直审与原始 artifacts，生成逐规则归因矩阵。

不调用模型；只读取 run_rule_scenarios.py --keep-workdir 留下的工作目录
和 probe-report.json。

用法：
    python3 analyze_rule_live.py \
        --live output/scenarios/live-baseline \
        --probe output/scenarios/probe \
        --scenarios tests/scenarios \
        --out output/scenarios/triage
"""

import argparse
import json
import os
import sys
from collections import Counter
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402

SKILL_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SCENARIOS = str(SKILL_ROOT / "tests" / "scenarios")
REPO_RULES = set(log_spec.repository_rule_names())
CONTROL_FOR_RULE = {
    "control-ut-print": "禁止自定义打印",
    "control-wrapper-macro": "禁止自定义打印",
    "control-negation": "级别错配",
    "control-clean": None,
}


def read_json(path):
    with open(path, "r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def read_jsonl(path):
    rows = []
    if not Path(path).is_file():
        return rows
    with open(path, "r", encoding="utf-8-sig") as handle:
        for line in handle:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    return rows


def load_manifest(scenarios_dir):
    value = read_json(Path(scenarios_dir) / "manifest.json")
    return value if isinstance(value, list) else value.get("scenarios", [])


def position(item):
    line = item.get("line")
    if isinstance(line, str) and line.strip().isdigit():
        line = int(line.strip())
    return (str(item.get("file") or "").replace("\\", "/"), int(line or 0))


def expected_positions(scenario):
    rows = list(scenario.get("must_find") or [])
    rows += list(scenario.get("must_find_any") or [])
    return [position(item) for item in rows]


def find_workdir(live_dir, scenario_id):
    return Path(live_dir) / "work" / scenario_id


def collect_workdir(workdir, scenario_id):
    return {
        "logs": read_jsonl(workdir / ("%s_logs.jsonl" % scenario_id)),
        "findings": read_jsonl(workdir / ("%s_findings.jsonl" % scenario_id)),
        "meta": (
            read_json(workdir / ("%s_meta.json" % scenario_id))
            if (workdir / ("%s_meta.json" % scenario_id)).is_file()
            else {}
        ),
        "raw": (
            read_jsonl(workdir / ("%s_rule_progress.jsonl" % scenario_id))
            + read_jsonl(workdir / ("%s_repository_progress.jsonl" % scenario_id))
        ),
    }


def flatten_raw(raw_lines):
    findings = []
    for row in raw_lines:
        for item in row.get("findings") or []:
            if isinstance(item, dict):
                findings.append(item)
    return findings


def normalize(rule):
    return log_spec.normalize_rule(str(rule or ""))


def classify(scenario, live_item, probe_item, artifacts):
    """返回 (分类, 证据 dict)。分类取值见报告。"""
    scenario_id = scenario["id"]
    expectation = scenario.get("expectation") or ""
    if expectation == "protocol_limited":
        return "PROTOCOL_LIMITED", {
            "reason": "finding 必须锚定真实 log_id，缺失类规则无法表达"
        }
    live_error = (live_item or {}).get("error")
    probe_error = (probe_item or {}).get("error")
    if live_error and not artifacts["logs"]:
        return "INFRA_ERROR", {"reason": str(live_error)[:200]}
    if probe_error and "缺少 LLM" in str(probe_error):
        return "INFRA_ERROR", {"reason": str(probe_error)[:200]}

    target_rule = scenario.get("rule")
    positions = expected_positions(scenario)
    logs = {position(row) for row in artifacts["logs"]}
    findings = artifacts["findings"]
    raw_findings = flatten_raw(artifacts["raw"])
    _tmp_result = []
    for row in raw_findings:
        if normalize(row.get("rule")) == target_rule:
            _tmp_result.append(row)
    raw_target = _tmp_result
    final_any_at_pos = [row for row in findings if position(row) in positions]
    raw_any_at_pos = [row for row in raw_findings if position(row) in positions]

    if scenario.get("kind") == "control":
        for_rule = CONTROL_FOR_RULE.get(scenario_id, scenario.get("for_rule"))
        target_findings = [
            row
            for row in findings
            if (for_rule is None or normalize(row.get("rule")) == for_rule)
        ]
        if target_findings:
            return "CONTROL_FP", {
                "rules": [normalize(row.get("rule")) for row in target_findings],
                "positions": [position(row) for row in target_findings],
            }
        return "CONTROL_OK", {}

    deterministic = bool(scenario.get("deterministic"))
    hit = bool((live_item or {}).get("caught"))
    if hit:
        if deterministic:
            return "EFFECTIVE_DETERMINISTIC", {}
        if (probe_item or {}).get("hit"):
            return "EFFECTIVE", {}
        return "EFFECTIVE_FULL_ONLY", {}

    probe_hit = bool((probe_item or {}).get("hit"))
    expected_rule_positions = set(positions)
    extracted = all(pos in logs for pos in expected_rule_positions)
    if not extracted:
        return "EXTRACTION_MISS", {
            "missing_positions": [
                pos for pos in expected_rule_positions if pos not in logs
            ]
        }
    if raw_target and any(position(row) in positions for row in raw_target):
        return "GUARD_DROP", {
            "guard": artifacts["meta"].get("output_guard_dropped") or {}
        }
    if final_any_at_pos:
        return "WRONG_RULE", {
            "rules_at_position": [
                normalize(row.get("rule")) for row in final_any_at_pos
            ]
        }
    if raw_any_at_pos:
        return "RAW_ONLY", {
            "rules_at_position": [normalize(row.get("rule")) for row in raw_any_at_pos]
        }
    if probe_hit:
        return "CONTEXT_LINK", {
            "reason": "日志已识别，直审可命中，但全链路规则审查未命中"
        }
    return "RULE_MISS", {}


def build_rows(live_payload, probe_payload, scenarios, live_dir):
    live_by_id = {item["id"]: item for item in live_payload.get("results", [])}
    probe_by_id = {item["id"]: item for item in probe_payload.get("results", [])}
    rows = []
    for scenario in scenarios:
        scenario_id = scenario["id"]
        workdir = find_workdir(live_dir, scenario_id)
        artifacts = collect_workdir(workdir, scenario_id)
        live_item = live_by_id.get(scenario_id) or {}
        probe_item = probe_by_id.get(scenario_id) or {}
        classification, evidence = classify(scenario, live_item, probe_item, artifacts)
        rows.append(
            {
                "id": scenario_id,
                "kind": scenario.get("kind"),
                "rule": scenario.get("rule"),
                "severity": scenario.get("severity"),
                "scope": scenario.get("scope"),
                "deterministic": bool(scenario.get("deterministic")),
                "live_hit": bool(live_item.get("caught")),
                "live_expected": live_item.get("expected"),
                "live_fp": len(live_item.get("false_positives") or []),
                "live_error": live_item.get("error"),
                "probe_hit": bool(probe_item.get("hit")),
                "probe_fp": int(probe_item.get("false_positive") or 0),
                "probe_error": probe_item.get("error"),
                "probe_issues": probe_item.get("issues") or [],
                "logs": len(artifacts["logs"]),
                "findings": len(artifacts["findings"]),
                "raw_findings": len(flatten_raw(artifacts["raw"])),
                "guard_dropped": artifacts["meta"].get("output_guard_dropped") or {},
                "classification": classification,
                "evidence": evidence,
                "notes": scenario.get("notes") or "",
            }
        )
    return rows


ACTION = {
    "EFFECTIVE": "保持",
    "EFFECTIVE_DETERMINISTIC": "保持（确定性判定）",
    "EFFECTIVE_FULL_ONLY": "观察：仅全链路命中，直审未命中",
    "EXTRACTION_MISS": "查识别阶段/用例：目标日志未被识别",
    "GUARD_DROP": "查护栏：模型已报但被 guard 丢弃",
    "WRONG_RULE": "查规则边界/提示词：位置命中但规则名不一致",
    "RAW_ONLY": "查护栏：原始 issues 有位置命中但最终丢失",
    "CONTEXT_LINK": "查规则审查上下文/分组/宏上下文",
    "RULE_MISS": "查规则尺度/模型能力/用例",
    "CONTROL_FP": "查护栏/负例：控制仓出现受控规则误报",
    "CONTROL_OK": "保持",
    "PROTOCOL_LIMITED": "协议限制，待扩展缺失类 finding",
    "INFRA_ERROR": "查环境/网关/请求失败",
}


def summarize(rows):
    counts = Counter(row["classification"] for row in rows)
    rules = [
        row
        for row in rows
        if row["kind"] == "rule" and row["classification"] != "PROTOCOL_LIMITED"
    ]
    controls = [row for row in rows if row["kind"] == "control"]
    live_hits = sum(1 for row in rules if row["live_hit"])
    probe_hits = sum(1 for row in rules if row["probe_hit"])
    return {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "classification": dict(counts),
        "rules": len(rules),
        "controls": len(controls),
        "live_hits": live_hits,
        "probe_hits": probe_hits,
        "live_fp": sum(row["live_fp"] for row in rows),
        "control_fp": sum(
            1 for row in controls if row["classification"] == "CONTROL_FP"
        ),
    }


def build_markdown(payload):
    summary = payload["summary"]
    lines = [
        "# 规则真实拦截归因报告（live × probe）",
        "",
        "- 生成时间：%s" % payload["generated_at"],
        "- live 目录：`%s`" % payload["live_dir"],
        "- probe 目录：`%s`" % payload["probe_dir"],
        "",
        "## 执行摘要",
        "",
        "| 维度 | 数值 |",
        "|---|---|",
        "| 规则场景 | %d |" % summary["rules"],
        "| 全链路命中 | %d |" % summary["live_hits"],
        "| 直审命中 | %d |" % summary["probe_hits"],
        "| 全链路误报 | %d |" % summary["live_fp"],
        "| 控制仓误报 | %d |" % summary["control_fp"],
        "",
    ]
    lines += ["**分类分布**：", ""]
    for name, count in sorted(
        summary["classification"].items(), key=lambda item: (-item[1], item[0])
    ):
        lines.append("- `%s`：%d" % (name, count))
    lines += [
        "",
        "## 逐规则矩阵",
        "",
        "| 规则 | 严重级 | 全链路 | 直审 | 分类 | 动作 | 证据 |",
        "|------|--------|--------|------|------|------|------|",
    ]
    for row in payload["rows"]:
        evidence = json.dumps(row["evidence"], ensure_ascii=False)[:80]
        lines.append(
            "| %s | %s | %s | %s | %s | %s | %s |"
            % (
                row["id"],
                row.get("severity") or "-",
                "HIT" if row["live_hit"] else "MISS",
                "HIT" if row["probe_hit"] else "MISS",
                row["classification"],
                ACTION.get(row["classification"], "-"),
                evidence.replace("|", "\\|"),
            )
        )
    lines += ["", "## 失败明细（含模型直审返回）", ""]
    for row in payload["rows"]:
        if row["classification"] in (
            "EFFECTIVE",
            "EFFECTIVE_DETERMINISTIC",
            "CONTROL_OK",
            "PROTOCOL_LIMITED",
        ):
            continue
        lines.append(
            "### %s（%s / %s）"
            % (row["id"], row["rule"] or "控制仓", row["classification"])
        )
        lines.append("")
        if row["live_error"]:
            lines.append("- live 错误：`%s`" % str(row["live_error"])[:200])
        if row["probe_error"]:
            lines.append("- probe 错误：`%s`" % str(row["probe_error"])[:200])
        lines.append(
            "- 日志识别 %d 条，最终 findings %d 条，guard 丢弃 %s"
            % (
                row["logs"],
                row["findings"],
                json.dumps(row["guard_dropped"], ensure_ascii=False),
            )
        )
        for issue in row["probe_issues"][:8]:
            lines.append(
                "- 直审返回：`%s:%s` [%s] %s"
                % (
                    issue.get("file"),
                    issue.get("line"),
                    issue.get("rule"),
                    str(issue.get("reason") or "")[:160],
                )
            )
        lines.append("")
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description="规则 live/probe 归因")
    parser.add_argument("--live", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--scenarios", default=DEFAULT_SCENARIOS)
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    live_payload = read_json(Path(args.live) / "scenario-report.json")
    probe_payload = read_json(Path(args.probe) / "probe-report.json")
    scenarios = load_manifest(args.scenarios)
    rows = build_rows(live_payload, probe_payload, scenarios, args.live)
    payload = {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "live_dir": os.path.abspath(args.live),
        "probe_dir": os.path.abspath(args.probe),
        "summary": summarize(rows),
        "rows": rows,
    }
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "triage.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (out_dir / "triage.md").write_text(build_markdown(payload), encoding="utf-8")
    print("[ok] 归因：%s" % (out_dir / "triage.md"))
    print(
        "[ok] 分类：%s"
        % json.dumps(
            payload["summary"]["classification"], ensure_ascii=False, sort_keys=True
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
