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

"""用参考 Excel 评估完整文件扫描范围、日志识别召回和问题命中。"""

import argparse
import bisect
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path

from openpyxl import load_workbook

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import llm_evaluate as evaluator  # noqa: E402

STRING_RE = re.compile(r'"((?:\\.|[^"\\])*)"|\'((?:\\.|[^\'\\])*)\'')


def normalize_repo(name):
    return str(name or "").split("@", 1)[0]


def relative_source(raw_path, repo):
    path = str(raw_path or "").replace("\\", "/")
    for marker in ("input/%s@master/" % repo, "input/%s/" % repo):
        if marker in path:
            return path.split(marker, 1)[1]
    return path.lstrip("./")


def in_scope(path, root, extensions, excluded):
    try:
        rel = path.relative_to(root)
    except ValueError:
        return False
    return path.suffix.lower() in extensions and not any(
        part in excluded for part in rel.parts[:-1]
    )


def probes(code):
    output = []
    for match in STRING_RE.finditer(str(code or "")):
        text = match.group(1) if match.group(1) is not None else match.group(2)
        text = text.split("...", 1)[0].strip()
        if len(text) >= 8:
            output.append(text)
    return sorted(set(output), key=len, reverse=True)


def locate(text, old_line, code):
    """按日志文案在当前源码中重定位；找不到表示源码版本已变化。"""
    line_starts = [0] + [match.end() for match in re.finditer("\n", text)]
    for probe in probes(code):
        positions = []
        haystack = text
        start = 0
        while True:
            start = haystack.find(probe, start)
            if start < 0:
                break
            positions.append(start)
            start += 1
        if positions:
            best = min(
                positions,
                key=lambda pos: (
                    0 if pos > 0 and haystack[pos - 1] in "\"'" else 1,
                    abs(bisect.bisect_right(line_starts, pos) - old_line),
                ),
            )
            return bisect.bisect_right(line_starts, best)
    return None


def load_details(path):
    workbook = load_workbook(path, read_only=True, data_only=True)
    sheet = workbook["问题明细"]
    headers = [cell.value for cell in next(sheet.iter_rows())]
    return [
        dict(zip(headers, row))
        for row in sheet.iter_rows(min_row=2, values_only=True)
        if row[0] is not None
    ]


def load_overview(path):
    workbook = load_workbook(path, read_only=True, data_only=True)
    sheet = workbook["概览"]
    headers = [cell.value for cell in next(sheet.iter_rows())]
    try:
        repo_index = headers.index("仓库")
    except ValueError:
        repo_index = 0
    return {
        normalize_repo(row[repo_index]): dict(zip(headers, row))
        for row in sheet.iter_rows(min_row=2, values_only=True)
        if row[repo_index] is not None
    }


def load_jsonl(path):
    if not path or not path.exists():
        return []
    return [
        json.loads(line)
        for line in path.read_text(encoding="utf-8-sig").splitlines()
        if line.strip()
    ]


def evaluate_repo(repo, rows, input_root, extensions, excluded, show_misses):
    root = (input_root / (repo + "@master")).resolve()
    stats = Counter()
    comparable = []
    misses = []
    for row in rows:
        rel = relative_source(row.get("文件"), repo)
        path = (root / rel).resolve()
        if not path.exists():
            stats["源码不存在"] += 1
            continue
        if not in_scope(path, root, extensions, excluded):
            stats["范围跳过"] += 1
            misses.append(("SKIP", row, rel, int(row.get("行号") or 0)))
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        line = locate(text, int(row.get("行号") or 0), row.get("代码"))
        if line is None:
            stats["源码已变化"] += 1
            misses.append(("CHANGED", row, rel, int(row.get("行号") or 0)))
            continue
        stats["完整文件覆盖"] += 1
        comparable.append({"file": rel, "line": line, "rule": row.get("规则")})

    eligible = stats["完整文件覆盖"] + stats["范围跳过"]
    coverage = 100.0 * stats["完整文件覆盖"] / max(1, eligible)
    print(
        "%-16s 基线=%4d 可比=%4d 完整文件覆盖=%4d 范围跳过=%3d "
        "覆盖率=%6.2f%% 源码变化=%3d 源码缺失=%3d"
        % (
            repo,
            len(rows),
            eligible,
            stats["完整文件覆盖"],
            stats["范围跳过"],
            coverage,
            stats["源码已变化"],
            stats["源码不存在"],
        )
    )
    if show_misses:
        for kind, row, rel, line in misses:
            print(
                "  %-7s %-10s %s:%d %s"
                % (kind, row.get("规则"), rel, line, str(row.get("代码"))[:160])
            )
    return stats, comparable


def covered_by_log(target, logs):
    return any(
        str(log.get("file", "")).replace("\\", "/") == target["file"]
        and int(log.get("line") or 0)
        <= target["line"]
        <= int(log.get("end") or log.get("line") or 0)
        for log in logs
    )


def compare_results(repo, comparable, logs, findings, overview, meta_path):
    log_hits = [target for target in comparable if covered_by_log(target, logs)]
    finding_locations = {
        (
            str(row.get("file", "")).replace("\\", "/"),
            int(row.get("line") or 0),
            int(row.get("end") or row.get("line") or 0),
        )
        for row in findings
    }
    finding_rules = {
        (
            str(row.get("file", "")).replace("\\", "/"),
            int(row.get("line") or 0),
            int(row.get("end") or row.get("line") or 0),
            row.get("rule"),
        )
        for row in findings
    }
    location_hits = [
        target
        for target in comparable
        if any(
            file_name == target["file"] and start <= target["line"] <= end
            for file_name, start, end in finding_locations
        )
    ]
    rule_hits = [
        target
        for target in comparable
        if any(
            file_name == target["file"]
            and start <= target["line"] <= end
            and rule == target["rule"]
            for file_name, start, end, rule in finding_rules
        )
    ]
    print(
        "模型对比 %-9s logs=%4d 基线可比=%3d 日志识别命中=%3d(%6.2f%%) "
        "问题位置命中=%3d 规则命中=%3d"
        % (
            repo,
            len(logs),
            len(comparable),
            len(log_hits),
            100.0 * len(log_hits) / max(1, len(comparable)),
            len(location_hits),
            len(rule_hits),
        )
    )
    if meta_path and meta_path.exists():
        meta = json.loads(meta_path.read_text(encoding="utf-8-sig"))
        current_logs = int(meta.get("log_statements") or 0)
        baseline_row = overview.get(repo, {})
        baseline_logs = int(baseline_row.get("日志语句") or 0)
        baseline_findings = next(
            (
                int(value or 0)
                for key, value in baseline_row.items()
                if key and "问题" in str(key)
            ),
            0,
        )
        print(
            "密度对比 %-9s 当前=%d/%d=%5.2f%% 基线=%d/%d=%5.2f%%"
            % (
                repo,
                len(findings),
                current_logs,
                100.0 * len(findings) / max(1, current_logs),
                baseline_findings,
                baseline_logs,
                100.0 * baseline_findings / max(1, baseline_logs),
            )
        )


def main():
    workspace = Path(__file__).resolve().parents[4]
    skill_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="检查完整文件扫描与模型日志识别基线")
    parser.add_argument(
        "--baseline",
        default=workspace / "docs" / "requirements" / "cann-log-evaluation-report.xlsx",
    )
    parser.add_argument("--input", default=skill_root / "input")
    parser.add_argument("--repos", default="amct,oam-tools,runtime")
    parser.add_argument("--extensions", default=",".join(evaluator.CODE_EXT))
    parser.add_argument("--exclude-dirs", default=",".join(sorted(evaluator.SKIP_DIRS)))
    parser.add_argument("--show-misses", action="store_true")
    parser.add_argument(
        "--write-file-lists", help="可选：把各仓基准涉及的相对源码路径写入指定目录"
    )
    parser.add_argument("--logs", help="模型返回的 <仓库>_logs.jsonl")
    parser.add_argument("--findings", help="模型返回的 <仓库>_findings.jsonl")
    parser.add_argument("--findings-repo", help="--logs/--findings 对应仓库名")
    parser.add_argument("--meta", help="对应 meta.json；默认从 findings 推导")
    args = parser.parse_args()

    baseline = Path(args.baseline).resolve()
    input_root = Path(args.input).resolve()
    if not baseline.exists():
        print("[error] 基线不存在: %s" % baseline, file=sys.stderr)
        return 2
    extensions = evaluator.configured_extensions(args.extensions)
    excluded = set(evaluator._csv(args.exclude_dirs))
    details = load_details(baseline)
    overview = load_overview(baseline)
    file_list_dir = (
        Path(args.write_file_lists).resolve() if args.write_file_lists else None
    )
    if file_list_dir:
        file_list_dir.mkdir(parents=True, exist_ok=True)
    total = Counter()
    comparable_by_repo = {}
    for repo in [item.strip() for item in args.repos.split(",") if item.strip()]:
        rows = [row for row in details if normalize_repo(row.get("仓库")) == repo]
        if not rows:
            print("[warn] 基线没有仓库: %s" % repo)
            continue
        if file_list_dir:
            relative_paths = sorted(
                {relative_source(row.get("文件"), repo) for row in rows}
            )
            list_path = file_list_dir / (repo + ".txt")
            list_path.write_text("\n".join(relative_paths) + "\n", encoding="utf-8")
            print(
                "[ok] 文件清单 %-9s %3d 个: %s" % (repo, len(relative_paths), list_path)
            )
        stats, comparable = evaluate_repo(
            repo, rows, input_root, extensions, excluded, args.show_misses
        )
        total.update(stats)
        comparable_by_repo[repo] = comparable

    eligible = total["完整文件覆盖"] + total["范围跳过"]
    print(
        "合计             可比=%4d 完整文件覆盖=%4d 范围跳过=%3d 覆盖率=%6.2f%%"
        % (
            eligible,
            total["完整文件覆盖"],
            total["范围跳过"],
            100.0 * total["完整文件覆盖"] / max(1, eligible),
        )
    )

    if args.logs or args.findings:
        repo = args.findings_repo or next(iter(comparable_by_repo), "")
        if repo not in comparable_by_repo:
            print("[error] 结果仓库未包含在 --repos 中: %s" % repo, file=sys.stderr)
            return 2
        findings_path = Path(args.findings).resolve() if args.findings else None
        logs_path = Path(args.logs).resolve() if args.logs else None
        if logs_path is None and findings_path:
            logs_path = findings_path.with_name(
                findings_path.name.replace("_findings.jsonl", "_logs.jsonl")
            )
        if findings_path is None and logs_path:
            findings_path = logs_path.with_name(
                logs_path.name.replace("_logs.jsonl", "_findings.jsonl")
            )
        if not logs_path or not logs_path.exists():
            print("[error] logs 不存在: %s" % logs_path, file=sys.stderr)
            return 2
        if not findings_path or not findings_path.exists():
            print("[error] findings 不存在: %s" % findings_path, file=sys.stderr)
            return 2
        meta_path = (
            Path(args.meta).resolve()
            if args.meta
            else findings_path.with_name(
                findings_path.name.replace("_findings.jsonl", "_meta.json")
            )
        )
        compare_results(
            repo,
            comparable_by_repo[repo],
            load_jsonl(logs_path),
            load_jsonl(findings_path),
            overview,
            meta_path,
        )
    return 1 if total["范围跳过"] else 0


if __name__ == "__main__":
    sys.exit(main())
