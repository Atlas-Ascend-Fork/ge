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

"""按当前确定性护栏修复历史评测结果，不调用模型。"""

import argparse
import collections
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import daily_run  # noqa: E402
import det_rules  # noqa: E402
import log_spec  # noqa: E402


REFINEMENT_VERSION = "2"
COUNT_FIELDS = {"log_statements", "findings", "by_rule", "by_severity", "by_decider"}


def read_json(path):
    with Path(path).open("r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def read_jsonl(path):
    rows = []
    with Path(path).open("r", encoding="utf-8-sig") as handle:
        for number, raw in enumerate(handle, 1):
            if not raw.strip():
                continue
            try:
                rows.append(json.loads(raw))
            except json.JSONDecodeError as exc:
                raise ValueError("%s:%d: %s" % (path, number, exc)) from exc
    return rows


def write_json(path, value):
    with Path(path).open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


def write_jsonl(path, rows):
    with Path(path).open("w", encoding="utf-8", newline="\n") as handle:
        for row in rows:
            handle.write(json.dumps(row, ensure_ascii=False) + "\n")


def position(row):
    return (
        str(row.get("file") or ""),
        int(row.get("line") or 0),
        int(row.get("end") or row.get("line") or 0),
    )


def log_position(row):
    """历史产物的语句结束行口径不同，日志归属只按文件与起始行核对。"""
    return (str(row.get("file") or ""), int(row.get("line") or 0))


def is_runtime_record(row):
    return bool(
        det_rules.runtime_code(row.get("code", ""), row.get("file", "")).strip()
    )


def refine_findings(rows):
    """返回 (保留项, 剔除原因计数)。"""
    retained, dropped = det_rules.guard_findings(rows)
    for row in retained:
        rule = log_spec.normalize_rule(row.get("rule", ""))
        row["rule"] = rule
        row["category"] = log_spec.rule_category(rule)
    return retained, dropped


def counters(rows):
    return {
        "by_rule": dict(collections.Counter(row.get("rule") for row in rows)),
        "by_severity": dict(collections.Counter(row.get("severity") for row in rows)),
        "by_decider": dict(
            collections.Counter(row.get("decided_by", "model") for row in rows)
        ),
    }


def refinement_info(source, findings_source, dropped, synthesized_logs=0):
    """生成可审计的复核来源说明，避免把独立四万版误写成基线覆盖。"""
    independent = source.resolve() == findings_source.resolve()
    value = {
        "version": REFINEMENT_VERSION,
        "mode": (
            "independent-full-model-refinement"
            if independent
            else "approved-baseline-reconciliation"
        ),
        "source": str(source),
        "findings_source": str(findings_source),
        "dropped": dict(dropped),
        "synthesized_logs": synthesized_logs,
    }
    if not independent:
        value["approved_findings_source"] = str(findings_source)
    return value


def analyze(source, findings_source=None):
    findings_source = findings_source or source
    targets = read_json(source / "current-run-targets.json")
    payload = []
    totals = collections.Counter()
    for target in targets.get("items", []):
        artifact = target["artifact"]
        logs = read_jsonl(source / (artifact + "_logs.jsonl"))
        findings_path = findings_source / (artifact + "_findings.jsonl")
        findings = read_jsonl(findings_path)
        refined_logs = [row for row in logs if is_runtime_record(row)]
        refined_findings, dropped = refine_findings(findings)
        valid_log_positions = {log_position(row) for row in refined_logs}
        # 历史标准结果保存了问题但未总是保存完整日志 JSONL。用已经过源码校验的
        # finding 补齐对应日志记录，使修正版仍可通过位置完整性校验。
        synthesized = 0
        for row in refined_findings:
            if log_position(row) in valid_log_positions:
                continue
            log_row = {
                "file": row.get("file", ""),
                "line": row.get("line", 0),
                "end": row.get("end", row.get("line", 0)),
                "code": row.get("code", ""),
                "kind": "approved_baseline",
                "repo": row.get("repo", target.get("repo", "")),
                "branch": row.get("branch", target.get("branch", "")),
                "source_type": row.get("source_type", "核心代码"),
            }
            log_row["log_id"] = "%s:%d-%d" % (
                log_row["file"],
                log_row["line"],
                log_row["end"],
            )
            log_row["global_id"] = log_row["log_id"]
            refined_logs.append(log_row)
            valid_log_positions.add(log_position(row))
            synthesized += 1
        totals.update(dropped)
        payload.append(
            {
                "target": target,
                "logs": refined_logs,
                "findings": refined_findings,
                "dropped": dict(dropped),
                "synthesized_logs": synthesized,
            }
        )
    return targets, payload, totals


def run_tool(name, *args):
    result = subprocess.run(
        [sys.executable, str(HERE / name)] + [str(value) for value in args],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.stdout.strip():
        print(result.stdout.rstrip())
    if result.returncode:
        if result.stderr.strip():
            print(result.stderr.rstrip(), file=sys.stderr)
        raise RuntimeError("%s 执行失败，退出码 %d" % (name, result.returncode))


def write_refined(source, output, targets, payload, totals, findings_source=None):
    findings_source = findings_source or source
    output.mkdir(parents=True, exist_ok=False)
    write_json(output / "current-run-targets.json", targets)
    per_repo = {}
    summary = []
    for item in payload:
        target = item["target"]
        artifact = target["artifact"]
        source_meta = read_json(source / (artifact + "_meta.json"))
        write_jsonl(output / (artifact + "_logs.jsonl"), item["logs"])
        write_jsonl(output / (artifact + "_findings.jsonl"), item["findings"])
        counts = counters(item["findings"])
        _tmp_result = {}
        for key, value in source_meta.items():
            if key not in COUNT_FIELDS:
                _tmp_result[key] = value
        meta = _tmp_result
        meta.update(
            {
                "input_scope": source_meta.get("input_scope", "full_repository"),
                "log_statements": len(item["logs"]),
                "findings": len(item["findings"]),
                **counts,
                "refinement": refinement_info(
                    source,
                    findings_source,
                    item["dropped"],
                    item.get("synthesized_logs", 0),
                ),
            }
        )
        write_json(output / (artifact + "_meta.json"), meta)
        per_repo[artifact] = {
            "repo": target.get("repo", ""),
            "branch": target.get("branch", ""),
            "logs": item["logs"],
            "log_count": len(item["logs"]),
            "findings": item["findings"],
            "complete": meta.get("complete") is True,
        }
        summary.append(
            {
                "repo": target.get("repo", ""),
                "branch": target.get("branch", ""),
                "commit": meta.get("commit", ""),
                "team": daily_run.team_of(
                    daily_run.load_team_map(), target.get("repo", "")
                ),
                "logs": len(item["logs"]),
                "findings": len(item["findings"]),
                "complete": meta.get("complete") is True,
            }
        )

    daily_run.write_team_report(str(output), daily_run.load_team_map(), per_repo)
    source_state = source / "daily-run.json"
    if source_state.exists():
        state = read_json(source_state)
        state.update(
            {
                "generated_at": datetime.now()
                .astimezone()
                .isoformat(timespec="milliseconds"),
                "log_statements": sum(len(item["logs"]) for item in payload),
                "findings": sum(len(item["findings"]) for item in payload),
                "summary": summary,
                "refinement": refinement_info(
                    source,
                    findings_source,
                    totals,
                    sum(item.get("synthesized_logs", 0) for item in payload),
                ),
            }
        )
        write_json(output / "daily-run.json", state)

    original_rows = sum(
        len(
            read_jsonl(
                findings_source / (item["target"]["artifact"] + "_findings.jsonl")
            )
        )
        for item in payload
    )
    corrected_rows = sum(len(item["findings"]) for item in payload)
    original_by_rule = collections.Counter()
    corrected_by_rule = collections.Counter()
    for item in payload:
        artifact = item["target"]["artifact"]
        original_by_rule.update(
            row.get("rule")
            for row in read_jsonl(findings_source / (artifact + "_findings.jsonl"))
        )
        corrected_by_rule.update(row.get("rule") for row in item["findings"])
    audit = refinement_info(
        source,
        findings_source,
        totals,
        sum(item.get("synthesized_logs", 0) for item in payload),
    )
    audit.update(
        {
            "generated_at": datetime.now()
            .astimezone()
            .isoformat(timespec="milliseconds"),
            "targets": len(payload),
            "original_findings": original_rows,
            "corrected_findings": corrected_rows,
            "removed_findings": original_rows - corrected_rows,
            "original_by_rule": dict(original_by_rule),
            "corrected_by_rule": dict(corrected_by_rule),
        }
    )
    write_json(output / "refinement-audit.json", audit)

    run_tool("generate_all_reports.py", "--workdir", output)
    run_tool(
        "check_run.py",
        "--workdir",
        output,
        "--expected-targets",
        len(payload),
        "--out",
        output / "process-validation.json",
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description="修复历史评测结果中的确定性误报")
    parser.add_argument("--source", required=True)
    parser.add_argument(
        "--findings-source", help="可选：使用另一份源码校验结果作为问题清单基线"
    )
    parser.add_argument("--out")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)
    source = Path(args.source).resolve()
    findings_source = (
        Path(args.findings_source).resolve() if args.findings_source else source
    )
    if not (source / "current-run-targets.json").is_file():
        raise FileNotFoundError(source / "current-run-targets.json")
    output = (
        Path(args.out).resolve()
        if args.out
        else source.with_name(source.name + "-refined")
    )
    if not (findings_source / "current-run-targets.json").is_file():
        raise FileNotFoundError(findings_source / "current-run-targets.json")
    targets, payload, totals = analyze(source, findings_source)
    old_findings = sum(
        len(
            read_jsonl(
                findings_source / (item["target"]["artifact"] + "_findings.jsonl")
            )
        )
        for item in payload
    )
    new_findings = sum(len(item["findings"]) for item in payload)
    print(
        "[info] 问题数: %d -> %d，剔除 %d"
        % (old_findings, new_findings, old_findings - new_findings)
    )
    for reason, count in totals.most_common():
        print("[info] %s: %d" % (reason, count))
    synthesized = sum(item.get("synthesized_logs", 0) for item in payload)
    if synthesized:
        print("[info] 补齐标准结果日志定位: %d" % synthesized)
    if args.dry_run:
        return 0
    if output.exists():
        raise FileExistsError("输出目录已存在，为保护结果不覆盖：%s" % output)
    write_refined(
        source, output, targets, payload, totals, findings_source=findings_source
    )
    print("[ok] 修正版结果目录: %s" % output)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, TypeError, RuntimeError) as exc:
        print("[error] %s" % exc, file=sys.stderr)
        sys.exit(1)
