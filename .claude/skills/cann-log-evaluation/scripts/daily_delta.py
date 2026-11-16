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

"""生成每日问题增量与分队汇总，并保存下一次比较快照。"""

import argparse
import json
import os
import re
import sys
from collections import Counter, defaultdict
from datetime import datetime


def load_json(path, default=None):
    if not os.path.isfile(path):
        return default
    with open(path, "r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def load_jsonl(path):
    if not os.path.isfile(path):
        return []
    rows = []
    with open(path, "r", encoding="utf-8-sig") as handle:
        for raw in handle:
            if raw.strip():
                rows.append(json.loads(raw))
    return rows


def normalized_code(value):
    return re.sub(r"\s+", " ", str(value or "")).strip().lower()


def stable_key(row):
    """忽略行号，避免普通代码插入导致整批问题被误报为新增/已解决。"""
    return "\0".join(
        [
            str(row.get("repo") or ""),
            str(row.get("branch") or "master"),
            str(row.get("file") or "").replace("\\", "/"),
            str(row.get("rule") or ""),
            normalized_code(row.get("code")),
        ]
    )


def compare(previous, current):
    old_groups, new_groups = defaultdict(list), defaultdict(list)
    for row in previous:
        old_groups[stable_key(row)].append(row)
    for row in current:
        new_groups[stable_key(row)].append(row)
    result = {"new": [], "ongoing": [], "resolved": []}
    for key in sorted(set(old_groups) | set(new_groups)):
        old = sorted(old_groups[key], key=lambda row: int(row.get("line") or 0))
        new = sorted(new_groups[key], key=lambda row: int(row.get("line") or 0))
        shared = min(len(old), len(new))
        result["ongoing"].extend(new[:shared])
        result["new"].extend(new[shared:])
        result["resolved"].extend(old[shared:])
    return result


def team_map(path):
    value = load_json(path, {}) or {}
    return {str(key).split("@", 1)[0]: str(team) for key, team in value.items()}


def add_team(rows, mapping):
    return [
        dict(row, team=mapping.get(str(row.get("repo") or ""), "其他")) for row in rows
    ]


def markdown(value):
    rows = value["rows"]
    lines = [
        "# CANN 日志规范每日增量",
        "",
        "> 生成时间：%s；范围：master。" % value["generated_at"],
        "",
        "## 分队汇总",
        "",
        "| 分队 | 当前未解决 | 新增 | 已解决 |",
        "|---|---:|---:|---:|",
    ]
    teams = sorted(
        set(row.get("team", "其他") for status in rows for row in rows[status])
    )
    for team in teams:
        current = sum(
            row.get("team") == team
            for status in ("new", "ongoing")
            for row in rows[status]
        )
        added = sum(row.get("team") == team for row in rows["new"])
        resolved = sum(row.get("team") == team for row in rows["resolved"])
        lines.append("| %s | %d | %d | %d |" % (team, current, added, resolved))
    lines.extend(["", "## 新增问题", ""])
    if not rows["new"]:
        lines.append("本次没有新增问题。")
    else:
        lines.extend(["| 分队 | 仓库 | 级别 | 规则 | 位置 |", "|---|---|---|---|---|"])
        for row in rows["new"]:
            lines.append(
                "| %s | %s | %s | %s | `%s:%s` |"
                % (
                    row.get("team", "其他"),
                    row.get("repo", ""),
                    row.get("severity", ""),
                    row.get("rule", ""),
                    str(row.get("file", "")).replace("|", "\\|"),
                    row.get("line", ""),
                )
            )
    lines.extend(["", "## 已解决问题", ""])
    if not rows["resolved"]:
        lines.append("本次没有确认已解决的问题。")
    else:
        counts = Counter(
            (row.get("team", "其他"), row.get("repo", "")) for row in rows["resolved"]
        )
        for (team, repo), count in sorted(counts.items()):
            lines.append("- %s / %s：%d 个" % (team, repo, count))
    lines.extend(["", "完整未解决明细见同目录 Excel/Markdown 正式报告。", ""])
    return "\n".join(lines)


def build(workdir, team_map_path):
    targets = load_json(os.path.join(workdir, "current-run-targets.json"), {}) or {}
    current = []
    for item in targets.get("items") or []:
        artifact = item.get("artifact") or item.get("repo")
        for row in load_jsonl(os.path.join(workdir, "%s_findings.jsonl" % artifact)):
            row.setdefault("repo", item.get("repo", ""))
            row.setdefault("branch", item.get("branch") or "master")
            current.append(row)
    snapshot_path = os.path.join(workdir, "daily-findings-snapshot.json")
    previous_value = load_json(snapshot_path, {}) or {}
    previous = previous_value.get("findings") or []
    compared = compare(previous, current)
    mapping = team_map(team_map_path)
    rows = {name: add_team(value, mapping) for name, value in compared.items()}
    now = datetime.now().astimezone().isoformat(timespec="milliseconds")
    result = {
        "generated_at": now,
        "baseline_run": not bool(previous_value),
        "summary": {
            "open": len(rows["new"]) + len(rows["ongoing"]),
            "new": len(rows["new"]),
            "ongoing": len(rows["ongoing"]),
            "resolved": len(rows["resolved"]),
        },
        "rows": rows,
    }
    with open(
        os.path.join(workdir, "daily-delta.json"), "w", encoding="utf-8"
    ) as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
    with open(os.path.join(workdir, "daily-delta.md"), "w", encoding="utf-8") as handle:
        handle.write(markdown(result))
    with open(snapshot_path, "w", encoding="utf-8") as handle:
        json.dump(
            {"generated_at": now, "findings": current},
            handle,
            ensure_ascii=False,
            separators=(",", ":"),
        )
    return result


def main():
    parser = argparse.ArgumentParser(description="生成每日问题新增/持续/已解决差异")
    parser.add_argument("--workdir", required=True)
    parser.add_argument(
        "--team-map",
        default=os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "references",
            "repo-team-map.json",
        ),
    )
    args = parser.parse_args()
    result = build(os.path.abspath(args.workdir), os.path.abspath(args.team_map))
    print(
        "[ok] 每日增量: 未解决=%d 新增=%d 已解决=%d"
        % (
            result["summary"]["open"],
            result["summary"]["new"],
            result["summary"]["resolved"],
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
