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

"""生成规则真实拦截测试的最终报告（live 全链路 × probe 直审 × 重复稳定性）。

输入目录约定：
- 基线 live：含 scenario-report.json，且用 --keep-workdir 保留了 work/<id>/。
- 基线 probe：含 probe-report.json。
- 重复 live/probe：各 3 份 scenario-report.json / probe-report.json，用于 pass@3。

用法：
    python3 rule_live_report.py \
        --live-baseline output/scenarios/live-baseline \
        --probe-baseline output/scenarios/probe-baseline \
        --live-repeat output/scenarios/live-r1 output/scenarios/live-r2 output/scenarios/live-r3 \
        --probe-repeat output/scenarios/probe-r1 output/scenarios/probe-r2 output/scenarios/probe-r3 \
        --scenarios tests/scenarios \
        --out output/scenarios/final-report
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


def normalize(rule):
    return log_spec.normalize_rule(str(rule or ""))


def expected_positions(scenario):
    return [
        position(item)
        for item in list(scenario.get("must_find") or [])
        + list(scenario.get("must_find_any") or [])
    ]


def collect_base(workdir, scenario_id):
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
    return [
        item
        for row in raw_lines
        for item in (row.get("findings") or [])
        if isinstance(item, dict)
    ]


def pass_count(reports, scenario_id, kind):
    hits = []
    for report in reports:
        item = next(
            (row for row in report.get("results", []) if row.get("id") == scenario_id),
            {},
        )
        if kind == "live":
            hits.append(bool(item.get("caught")))
        else:
            hits.append(bool(item.get("hit")))
    return sum(hits), len(hits)


def classify(
    scenario, live_base, probe_base, live_pass, live_n, probe_pass, probe_n, artifacts
):
    if scenario.get("expectation") == "protocol_limited":
        return "PROTOCOL_LIMITED", "finding 必须锚定真实 log_id，缺失类规则无法表达"
    if scenario.get("kind") == "control":
        if (live_base or {}).get("pass") and not (probe_base or {}).get(
            "false_positive"
        ):
            return "CONTROL_OK", "控制仓未出现受控规则问题"
        return "CONTROL_FP", "控制仓出现受控规则问题"
    target_rule = scenario.get("rule")
    positions = set(expected_positions(scenario))
    findings = artifacts["findings"]
    raw = flatten_raw(artifacts["raw"])
    raw_other = [
        normalize(row.get("rule"))
        for row in raw
        if position(row) in positions and normalize(row.get("rule")) != target_rule
    ]
    final_other = [
        normalize(row.get("rule"))
        for row in findings
        if position(row) in positions and normalize(row.get("rule")) != target_rule
    ]
    guard = artifacts["meta"].get("output_guard_dropped") or {}
    evidence_parts = []
    if guard:
        evidence_parts.append("guard 丢弃 " + json.dumps(guard, ensure_ascii=False))
    if raw_other:
        evidence_parts.append(
            "模型全链路在此位置返回 " + "、".join(sorted(set(raw_other)))
        )
    if final_other:
        evidence_parts.append(
            "最终 findings 在此位置是 " + "、".join(sorted(set(final_other)))
        )
    evidence = "；".join(evidence_parts)
    deterministic = bool(scenario.get("deterministic"))
    if deterministic:
        if live_pass == live_n:
            return "DETERMINISTIC_OK", evidence
        return "DETERMINISTIC_MISS", evidence
    live_ratio = (float(live_pass) / live_n) if live_n else 0.0
    probe_ratio = (float(probe_pass) / probe_n) if probe_n else 0.0
    if live_ratio >= 2.0 / 3.0:
        return "STABLE_EFFECTIVE", evidence
    if live_ratio > 0:
        return "FLAKY", evidence
    if probe_ratio >= 2.0 / 3.0:
        return "SHADOWED", evidence
    if probe_ratio > 0:
        return "SHADOWED_FLAKY", evidence
    return "NOT_EFFECTIVE", evidence


ACTION = {
    "STABLE_EFFECTIVE": "保持",
    "FLAKY": "提高稳定性：重复/投票或补强提示词",
    "SHADOWED": "规则可判定但全链路被竞争/遮蔽：看规则优先级与上下文",
    "SHADOWED_FLAKY": "规则可判定但不稳定：看规则优先级与上下文",
    "NOT_EFFECTIVE": "模型判不出：补上下文/改用例/改规则尺度",
    "DETERMINISTIC_OK": "保持（确定性判定）",
    "DETERMINISTIC_MISS": "查确定性链路：预期不应依赖模型",
    "CONTROL_OK": "保持",
    "CONTROL_FP": "查护栏/负例",
    "PROTOCOL_LIMITED": "协议限制，待扩展缺失类 finding",
}


def build_rows(
    manifest, live_base_report, probe_base_report, live_reports, probe_reports, live_dir
):
    live_base = {item["id"]: item for item in live_base_report.get("results", [])}
    probe_base = {item["id"]: item for item in probe_base_report.get("results", [])}
    rows = []
    for scenario in manifest:
        scenario_id = scenario["id"]
        artifacts = collect_base(Path(live_dir) / "work" / scenario_id, scenario_id)
        live_pass, live_n = pass_count(live_reports, scenario_id, "live")
        probe_pass, probe_n = pass_count(probe_reports, scenario_id, "probe")
        classification, evidence = classify(
            scenario,
            live_base.get(scenario_id),
            probe_base.get(scenario_id),
            live_pass,
            live_n,
            probe_pass,
            probe_n,
            artifacts,
        )
        positions = set(expected_positions(scenario))
        raw = flatten_raw(artifacts["raw"])
        rows.append(
            {
                "id": scenario_id,
                "kind": scenario.get("kind"),
                "rule": scenario.get("rule"),
                "severity": scenario.get("severity"),
                "scope": scenario.get("scope"),
                "deterministic": bool(scenario.get("deterministic")),
                "live_base_hit": bool((live_base.get(scenario_id) or {}).get("caught")),
                "live_pass": live_pass,
                "live_n": live_n,
                "probe_base_hit": bool((probe_base.get(scenario_id) or {}).get("hit")),
                "probe_pass": probe_pass,
                "probe_n": probe_n,
                "probe_issues": (probe_base.get(scenario_id) or {}).get("issues") or [],
                "raw_rules_at_position": sorted(
                    {
                        normalize(row.get("rule"))
                        for row in raw
                        if position(row) in positions
                    }
                ),
                "final_rules_at_position": sorted(
                    {
                        normalize(row.get("rule"))
                        for row in artifacts["findings"]
                        if position(row) in positions
                    }
                ),
                "guard_dropped": artifacts["meta"].get("output_guard_dropped") or {},
                "classification": classification,
                "evidence": evidence,
                "action": ACTION.get(classification, ""),
                "notes": scenario.get("notes") or "",
            }
        )
    return rows


def summarize(rows):
    counts = Counter(row["classification"] for row in rows)
    rules = [
        row
        for row in rows
        if row["kind"] == "rule" and row["classification"] != "PROTOCOL_LIMITED"
    ]
    return {
        "rules": len(rules),
        "stable": sum(
            1 for row in rules if row["classification"] == "STABLE_EFFECTIVE"
        ),
        "flaky": sum(1 for row in rules if row["classification"] == "FLAKY"),
        "shadowed": sum(
            1 for row in rules if row["classification"].startswith("SHADOWED")
        ),
        "not_effective": sum(
            1 for row in rules if row["classification"] == "NOT_EFFECTIVE"
        ),
        "deterministic": sum(
            1 for row in rules if row["classification"] == "DETERMINISTIC_OK"
        ),
        "controls_ok": sum(1 for row in rows if row["classification"] == "CONTROL_OK"),
        "protocol_limited": sum(
            1 for row in rows if row["classification"] == "PROTOCOL_LIMITED"
        ),
        "classification": dict(counts),
    }


def build_markdown(payload):
    summary = payload["summary"]
    lines = [
        "# 规则真实拦截测试报告（live × probe × pass@4）",
        "",
        "- 生成时间：%s" % payload["generated_at"],
        "- 模型：`%s`" % payload["model"],
        "- 全链路基线：%s" % payload["live_dir"],
        "- 直审基线：%s" % payload["probe_dir"],
        "",
        "## 一、总体结论",
        "",
        "| 指标 | 数值 |",
        "|---|---|",
        "| 计入规则数 | %d |" % summary["rules"],
        "| 稳定生效（live ≥ 2/3） | %d |" % summary["stable"],
        "| 不稳定（0 < live < 2/3） | %d |" % summary["flaky"],
        "| 规则可判但全链路被遮蔽 | %d |" % summary["shadowed"],
        "| 规则未生效（live/probe 命中率均 < 2/3） | %d |" % summary["not_effective"],
        "| 确定性规则正常 | %d |" % summary["deterministic"],
        "| 控制仓通过 | %d |" % summary["controls_ok"],
        "| 协议限制 | %d |" % summary["protocol_limited"],
        "",
    ]
    baseline = payload.get("baseline") or {}
    live_summary = baseline.get("live") or {}
    probe_summary = baseline.get("probe") or {}
    if live_summary:
        lines.append(
            "- 全链路基线（N=1）：%d/%d（%s）"
            % (
                live_summary.get("caught", 0),
                live_summary.get("expected", 0),
                "%.1f%%" % (100.0 * live_summary.get("recall", 0)),
            )
        )
    if probe_summary:
        lines.append(
            "- 直审基线（N=1）：%d/%d（%s）"
            % (
                probe_summary.get("hit", 0),
                probe_summary.get("expected", 0),
                "%.1f%%" % (100.0 * probe_summary.get("recall", 0)),
            )
        )
    lines += [
        "",
        "## 二、逐规则矩阵（pass@4）",
        "",
        "| 场景 | 规则 | 严重级 | live | probe | 分类 | 模型在全链路返回的规则 | guard 丢弃 | 动作 |",
        "|------|------|--------|------|-------|------|------------------------|-----------|------|",
    ]
    for row in payload["rows"]:
        live = "%d/%d" % (row["live_pass"], row["live_n"])
        probe = "%d/%d" % (row["probe_pass"], row["probe_n"])
        raw_rules = "、".join(row["raw_rules_at_position"]) or "-"
        guard = (
            json.dumps(row["guard_dropped"], ensure_ascii=False)
            if row["guard_dropped"]
            else "-"
        )
        lines.append(
            "| %s | %s | %s | %s | %s | %s | %s | %s | %s |"
            % (
                row["id"],
                row["rule"] or "控制仓",
                row["severity"] or "-",
                live,
                probe,
                row["classification"],
                raw_rules,
                guard,
                row["action"],
            )
        )
    lines += ["", "## 三、失败/异常规则明细", ""]
    for row in payload["rows"]:
        if row["classification"] in (
            "STABLE_EFFECTIVE",
            "DETERMINISTIC_OK",
            "CONTROL_OK",
            "PROTOCOL_LIMITED",
        ):
            continue
        lines.append(
            "### %s（%s / %s）"
            % (row["id"], row["rule"] or "控制仓", row["classification"])
        )
        lines.append("")
        lines.append(
            "- 稳定性：live %d/%d，probe %d/%d"
            % (row["live_pass"], row["live_n"], row["probe_pass"], row["probe_n"])
        )
        if row["raw_rules_at_position"]:
            lines.append(
                "- 全链路在该位置返回：%s" % "、".join(row["raw_rules_at_position"])
            )
        if row["final_rules_at_position"]:
            lines.append(
                "- 最终 findings：%s" % "、".join(row["final_rules_at_position"])
            )
        if row["guard_dropped"]:
            lines.append(
                "- guard 丢弃：%s"
                % json.dumps(row["guard_dropped"], ensure_ascii=False)
            )
        for issue in row["probe_issues"][:6]:
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
    lines += [
        "## 四、复现信息",
        "",
        "- 命令：`run_rule_scenarios.py --mode live --jobs 3`；`rule_probe.py --jobs 3`；各重复 3 次",
        "- 说明：pass@N 来自基线 + 3 次独立完整运行（共 4 次）；分类中的“遮蔽”指直审可命中但全链路命中率低于 2/3。",
        "",
    ]
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description="规则 live/probe 最终报告")
    parser.add_argument("--live-baseline", required=True)
    parser.add_argument("--probe-baseline", required=True)
    parser.add_argument("--live-repeat", nargs="+", required=True)
    parser.add_argument("--probe-repeat", nargs="+", required=True)
    parser.add_argument("--scenarios", default=DEFAULT_SCENARIOS)
    parser.add_argument("--out", required=True)
    parser.add_argument("--model", default="")
    args = parser.parse_args(argv)
    live_base = read_json(Path(args.live_baseline) / "scenario-report.json")
    probe_base = read_json(Path(args.probe_baseline) / "probe-report.json")
    live_reports = [live_base] + [
        read_json(Path(path) / "scenario-report.json") for path in args.live_repeat
    ]
    probe_reports = [probe_base] + [
        read_json(Path(path) / "probe-report.json") for path in args.probe_repeat
    ]
    rows = build_rows(
        load_manifest(args.scenarios),
        live_base,
        probe_base,
        live_reports,
        probe_reports,
        args.live_baseline,
    )
    payload = {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "model": args.model or os.environ.get("LLM_MODEL", ""),
        "live_dir": os.path.abspath(args.live_baseline),
        "probe_dir": os.path.abspath(args.probe_baseline),
        "summary": summarize(rows),
        "baseline": {
            "live": live_base.get("summary") or {},
            "probe": probe_base.get("summary") or {},
        },
        "rows": rows,
    }
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "rule-live-report.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (out_dir / "rule-live-report.md").write_text(
        build_markdown(payload), encoding="utf-8"
    )
    print("[ok] 报告：%s" % (out_dir / "rule-live-report.md"))
    print(
        "[ok] 汇总：%s"
        % json.dumps(payload["summary"], ensure_ascii=False, sort_keys=True)
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
