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

"""用历史全量上传结果校验快速通道，不调用模型。

回答两个可交付性问题：
1. 确定性预扫描是否覆盖了全量上传时模型识别出的每一条日志（召回）。
2. 确定性规则判定与模型判定是否一致（用于决定哪些规则可本地直接判、
   哪些必须交给模型）。

用法：
    python3 calibrate.py --repo-dir input/amct@master \
        --logs output/amct@master_logs.jsonl \
        --findings output/amct@master_findings.jsonl
    python3 calibrate.py --auto      # 自动配对 output/ 下所有历史基线
门禁：
    --min-recall 1.0 时，任何漏检都返回非零状态。
"""

import argparse
import collections
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import prescan  # noqa: E402
import det_rules  # noqa: E402

SKILL_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass


def load_jsonl(path):
    rows = []
    with open(path, "r", encoding="utf-8-sig") as handle:
        for raw in handle:
            raw = raw.strip()
            if not raw:
                continue
            try:
                rows.append(json.loads(raw))
            except json.JSONDecodeError:
                continue
    return rows


def build_resolved_index(items):
    """把预扫描解析出的常量文案按 (文件, 行) 展开到候选覆盖的每一行。

    历史基线记录的行号是模型给出的语句起始行，可能落在候选跨行区间内部，
    因此按区间展开而不是只索引起始行。
    """
    index = {}
    for item in items:
        resolved = item.get("resolved_text")
        if not resolved:
            continue
        for line in range(item["line"], item["end"] + 1):
            index.setdefault((item["file"], line), {}).update(resolved)
    return index


def calibrate_recall(repo_dir, logs_path, context=3):
    items, stats = prescan.prescan(repo_dir, context=context)
    spans = collections.defaultdict(list)
    for item in items:
        spans[item["file"]].append((item["line"], item["end"]))
    truth = load_jsonl(logs_path)
    hit, missed = 0, []
    for row in truth:
        found = spans.get(row["file"], [])
        line, end = row["line"], row.get("end", row["line"])
        if any(not (end < start or line > stop) for start, stop in found):
            hit += 1
        else:
            missed.append(
                {
                    "file": row["file"],
                    "line": line,
                    "end": end,
                    "code": (row.get("code") or "").strip()[:120],
                }
            )
    total = len(truth)
    return {
        "baseline_logs": total,
        "candidates": stats["candidates"],
        "hit": hit,
        "missed": missed,
        "recall": round(hit / total, 4) if total else 1.0,
        "candidate_ratio": stats["candidate_ratio"],
        "over_inclusion": round(stats["candidates"] / max(1, total), 2),
        "files_scanned": stats["files_scanned"],
        "resolved_index": build_resolved_index(items),
        "prescan_items": items,
    }


def calibrate_rules(findings_path, logs_path, resolved_index=None, prescan_items=None):
    findings = load_jsonl(findings_path)
    logs = load_jsonl(logs_path)
    resolved_index = resolved_index or {}
    prescan_items = prescan_items or []

    def resolved_for(row):
        return resolved_index.get((row.get("file"), row.get("line")))

    report = {}
    for rule in det_rules.DETERMINISTIC_RULES:
        baseline = [row for row in findings if row.get("rule") == rule]
        agree = sum(
            1
            for row in baseline
            if rule
            in det_rules.detect(
                row.get("code", ""), row.get("file", ""), resolved_for(row)
            )
        )
        detected = {
            (row["file"], row["line"])
            for row in logs
            if rule
            in det_rules.detect(
                row.get("code", ""), row.get("file", ""), resolved_for(row)
            )
        }
        base_keys = {(row["file"], row["line"]) for row in baseline}
        # 文案经函数返回值等间接传递时，快速通道在常量定义处报告同一缺陷，
        # 行号与基线（调用点）不同但指向同一问题，按 文件+规则 口径视为覆盖。
        prescan_files = (
            {
                item["file"]
                for item in prescan_items
                if rule
                in det_rules.detect(
                    item.get("code", ""),
                    item.get("file", ""),
                    item.get("resolved_text"),
                )
            }
            if prescan_items
            else set()
        )
        missing = {key for key in base_keys - detected if key[0] not in prescan_files}
        covered_elsewhere = len(base_keys - detected) - len(missing)
        report[rule] = {
            "baseline": len(baseline),
            "agreed": agree,
            "agreement": round(agree / len(baseline), 4) if baseline else None,
            "detector_hits": len(detected),
            "extra_vs_baseline": len(detected - base_keys),
            "missing_vs_baseline": len(missing),
            "covered_at_definition_site": covered_elsewhere,
        }
    return report


def auto_pairs(workdir):
    pairs = []
    for logs_path in sorted(glob.glob(os.path.join(workdir, "*_logs.jsonl"))):
        artifact = os.path.basename(logs_path)[: -len("_logs.jsonl")]
        findings_path = os.path.join(workdir, "%s_findings.jsonl" % artifact)
        repo_dir = os.path.join(SKILL_ROOT, "input", artifact)
        if not os.path.isdir(repo_dir):
            repo_dir = os.path.join(SKILL_ROOT, "input", "%s@master" % artifact)
        if os.path.isdir(repo_dir) and os.path.isfile(findings_path):
            pairs.append((artifact, repo_dir, logs_path, findings_path))
    return pairs


def main():
    parser = argparse.ArgumentParser(description="快速通道离线校验（不调用模型）")
    parser.add_argument("--repo-dir")
    parser.add_argument("--logs")
    parser.add_argument("--findings")
    parser.add_argument(
        "--auto", action="store_true", help="自动配对 output/ 下的历史基线"
    )
    parser.add_argument("--workdir", default=os.path.join(SKILL_ROOT, "output"))
    parser.add_argument("--context", type=int, default=3)
    parser.add_argument("--min-recall", type=float, default=1.0)
    parser.add_argument("--out", help="把校验报告写入 JSON")
    args = parser.parse_args()

    if args.auto:
        pairs = auto_pairs(args.workdir)
    elif args.repo_dir and args.logs:
        pairs = [
            (
                os.path.basename(args.repo_dir.rstrip("/\\")),
                args.repo_dir,
                args.logs,
                args.findings,
            )
        ]
    else:
        print("[error] 需要 --auto 或 --repo-dir/--logs", file=sys.stderr)
        return 2
    if not pairs:
        print("[error] 没有可用基线；先跑一次全量模式或提供 --logs", file=sys.stderr)
        return 2

    report = {"pairs": [], "gate": {"min_recall": args.min_recall}}
    worst = 1.0
    for artifact, repo_dir, logs_path, findings_path in pairs:
        recall = calibrate_recall(repo_dir, logs_path, args.context)
        resolved_index = recall.pop("resolved_index", {})
        prescan_items = recall.pop("prescan_items", [])
        rules = (
            calibrate_rules(findings_path, logs_path, resolved_index, prescan_items)
            if findings_path and os.path.isfile(findings_path)
            else {}
        )
        worst = min(worst, recall["recall"])
        report["pairs"].append(
            {
                "artifact": artifact,
                "repo_dir": os.path.abspath(repo_dir),
                "recall": recall,
                "rules": rules,
            }
        )
        print("== %s ==" % artifact)
        print(
            "  预扫描召回 = %.2f%%  (%d/%d)  候选=%d(过包含%.1fx)  候选字符占比=%.2f%%"
            % (
                100.0 * recall["recall"],
                recall["hit"],
                recall["baseline_logs"],
                recall["candidates"],
                recall["over_inclusion"],
                100.0 * recall["candidate_ratio"],
            )
        )
        for item in recall["missed"][:5]:
            print(
                "    [漏检] %s:%d-%d %s"
                % (item["file"], item["line"], item["end"], item["code"])
            )
        for rule, value in rules.items():
            if value["baseline"]:
                print(
                    "  规则一致性 %-14s 基线=%-4d 一致=%-4d(%.1f%%) 额外=%-3d "
                    "定义处覆盖=%-3d 真漏=%d"
                    % (
                        rule,
                        value["baseline"],
                        value["agreed"],
                        100.0 * (value["agreement"] or 0),
                        value["extra_vs_baseline"],
                        value["covered_at_definition_site"],
                        value["missing_vs_baseline"],
                    )
                )

    report["worst_recall"] = worst
    if args.out:
        with open(args.out, "w", encoding="utf-8") as handle:
            json.dump(report, handle, ensure_ascii=False, indent=2)
        print("\n[ok] 校验报告: %s" % args.out)
    passed = worst >= args.min_recall
    print(
        "\n[%s] 最低召回 %.2f%%，门禁 %.2f%%"
        % ("ok" if passed else "error", 100.0 * worst, 100.0 * args.min_recall)
    )
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
