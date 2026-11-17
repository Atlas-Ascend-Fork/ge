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

"""在调用模型前统计全量扫描范围、送审规模和预计请求数。"""

import argparse
import json
import math
import os
import sys
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import llm_evaluate as evaluator  # noqa: E402
import repo_exclusions  # noqa: E402
import model_config  # noqa: E402


def split_name(dirname):
    return tuple(dirname.split("@", 1)) if "@" in dirname else (dirname, "")


def balance_rows(rows, agent_count):
    if agent_count <= 0:
        raise ValueError("--agents 必须大于0")
    if not rows:
        return []
    shards = [
        {
            "agent": index + 1,
            "input_chars": 0,
            "effective_input_chars": 0,
            "extraction_requests": 0,
            "repositories": [],
        }
        for index in range(min(agent_count, max(1, len(rows))))
    ]
    for row in sorted(
        rows,
        key=lambda item: (
            -item.get("balance_weight", item["input_chars"]),
            item["repo"],
            item["branch"],
        ),
    ):
        shard = min(
            shards,
            key=lambda item: (
                item["effective_input_chars"],
                len(item["repositories"]),
                item["agent"],
            ),
        )
        row["agent"] = shard["agent"]
        shard["input_chars"] += row["input_chars"]
        shard["effective_input_chars"] += row.get("balance_weight", row["input_chars"])
        shard["extraction_requests"] += row["extraction_requests"]
        shard["repositories"].append({"repo": row["repo"], "branch": row["branch"]})
    return shards


def allocate_repo_workers(rows, max_concurrency, agent_count, max_workers_per_repo=8):
    """按仓请求量动态分配仓内并发，任意 agent 组合都不超过全局槽。"""
    if max_concurrency <= 0 or max_workers_per_repo <= 0 or agent_count <= 0:
        raise ValueError("并发参数必须大于0")
    active = [row for row in rows if row.get("extraction_requests", 0) > 0]
    for row in rows:
        row["repo_workers"] = 1
    if not active:
        return rows
    active_agents = min(agent_count, len(active))
    safe_cap = max(1, max_concurrency // active_agents)
    safe_cap = min(safe_cap, max_workers_per_repo)
    for row in active:
        row["repo_workers"] = min(safe_cap, row["extraction_requests"])
    for row in rows:
        row["balance_weight"] = int(
            math.ceil(row["input_chars"] / max(1, row["repo_workers"]))
        )
    return rows


def main():
    parser = argparse.ArgumentParser(description="评测运行前规模计划")
    parser.add_argument("--input", required=True)
    parser.add_argument("--per-subdir", action="store_true")
    parser.add_argument(
        "--master-only",
        action="store_true",
        help="只计划 master（无 @分支 后缀的目录也视为 master）",
    )
    parser.add_argument("--max-input-chars", type=int)
    parser.add_argument("--target-input-chars", type=int, required=True)
    parser.add_argument("--max-output-tokens", type=int)
    parser.add_argument("--agents", type=int, default=10)
    parser.add_argument(
        "--max-concurrency", type=int, default=model_config.DEFAULT_CONCURRENCY
    )
    parser.add_argument("--max-workers-per-repo", type=int, default=8)
    parser.add_argument("--extensions", default=",".join(evaluator.CODE_EXT))
    parser.add_argument("--exclude-dirs", default=",".join(sorted(evaluator.SKIP_DIRS)))
    parser.add_argument(
        "--exclude-repos",
        default=os.environ.get("LOG_EXCLUDE_REPOS", ""),
        help="内部仓排除清单，逗号分隔；不会写入计划或报告",
    )
    parser.add_argument(
        "--exclude-repos-file",
        default=repo_exclusions.default_exclude_file(),
        help="内部仓排除清单文件，每行一个仓名",
    )
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    root = os.path.abspath(args.input)
    targets = []
    if args.per_subdir and os.path.isdir(root):
        for name in sorted(os.listdir(root)):
            path = os.path.join(root, name)
            if os.path.isdir(path) and not name.startswith("."):
                repo, branch = split_name(name)
                if args.master_only and branch not in ("", "master"):
                    continue
                targets.append((repo, branch, path))
    if not args.per_subdir:
        repo, branch = split_name(os.path.basename(root.rstrip("/\\")))
        targets = [(repo, branch, root)]

    try:
        exclusion_set = repo_exclusions.load(
            args.exclude_repos, args.exclude_repos_file
        )
    except OSError:
        print("[error] 无法读取内部仓排除清单，请检查路径和权限", file=sys.stderr)
        return 2
    targets, excluded_targets = repo_exclusions.filter_tuple_targets(
        targets, exclusion_set
    )
    if excluded_targets:
        print(
            "[info] 已按内部仓策略排除 %d 个目标（名称不写入产物）" % excluded_targets
        )

    extensions = evaluator.configured_extensions(args.extensions)
    excluded = set(evaluator._csv(args.exclude_dirs))
    rows = []
    for index, (repo, branch, path) in enumerate(targets, 1):
        paths = list(evaluator.iter_code_files(path, extensions, excluded))
        sizes = [(source, os.path.getsize(source)) for source in paths]
        source_bytes = sum(size for _source, size in sizes)
        # 行号和提示词会增加输入；1.2 是仅用于分组的保守估算，不参与模型硬上限。
        input_chars = int(source_bytes * 1.2)
        segmented = [
            os.path.relpath(source, path).replace("\\", "/")
            for source, size in sizes
            if size > args.target_input_chars
        ]
        requests = (
            math.ceil(input_chars / args.target_input_chars) if input_chars else 0
        )
        row = {
            "repo": repo,
            "branch": branch,
            "scanned_files": len(paths),
            "model_reviewed_files": len(paths),
            "segmented_files": segmented,
            "source_bytes": source_bytes,
            "extraction_requests": requests,
            "input_chars": input_chars,
        }
        rows.append(row)
        print(
            "[%d/%d] %s@%s 扫描=%d 送审=%d 请求=%d"
            % (
                index,
                len(targets),
                repo,
                branch or "-",
                len(paths),
                len(paths),
                requests,
            ),
            flush=True,
        )

    allocate_repo_workers(
        rows, args.max_concurrency, args.agents, args.max_workers_per_repo
    )
    shards = balance_rows(rows, args.agents)
    peak_bound = sum(
        sorted(
            (row["repo_workers"] for row in rows if row["extraction_requests"] > 0),
            reverse=True,
        )[: len(shards)]
    )

    value = {
        "generated_at": datetime.now().astimezone().isoformat(timespec="milliseconds"),
        "targets": len(rows),
        "requested_agents": args.agents,
        "excluded_targets": excluded_targets,
        "active_agents": len(shards),
        "max_concurrency": args.max_concurrency,
        "peak_concurrency_bound": peak_bound,
        "scanned_files": sum(row["scanned_files"] for row in rows),
        "model_reviewed_files": sum(row["model_reviewed_files"] for row in rows),
        "segmented_files": sum(len(row["segmented_files"]) for row in rows),
        "source_bytes": sum(row["source_bytes"] for row in rows),
        "extraction_requests": sum(row["extraction_requests"] for row in rows),
        "input_chars": sum(row["input_chars"] for row in rows),
        "agents": shards,
        "items": rows,
    }
    with open(args.out, "w", encoding="utf-8") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
    print(
        "[ok] 运行计划: 扫描=%d 送审=%d 日志识别请求=%d"
        % (
            value["scanned_files"],
            value["model_reviewed_files"],
            value["extraction_requests"],
        )
    )
    print("[info] 计划文件: %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
