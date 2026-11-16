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

"""验证日志评测过程和结果完整性，不调用模型。"""

import argparse
import glob
import json
import os
import re
import sys
from datetime import datetime


REQUIRED_FINDING_FIELDS = (
    "repo",
    "branch",
    "file",
    "line",
    "code",
    "rule",
    "severity",
    "reason",
    "suggestion",
    "found_time",
)


def load_json(path):
    with open(path, "r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def load_jsonl(path):
    rows = []
    with open(path, "r", encoding="utf-8-sig") as handle:
        for number, raw in enumerate(handle, 1):
            if not raw.strip():
                continue
            try:
                rows.append(json.loads(raw))
            except json.JSONDecodeError as exc:
                raise ValueError("%s:%d JSON 无法解析: %s" % (path, number, exc))
    return rows


def artifact_name(repo, branch, meta_path=None):
    """Resolve both legacy master names and daily_run's explicit @master names."""
    if meta_path:
        filename = os.path.basename(meta_path)
        if filename.endswith("_meta.json"):
            return filename[: -len("_meta.json")]
    return (
        repo
        if not branch or branch == "master"
        else "%s@%s" % (repo, branch.replace("/", "_"))
    )


def validate_target(workdir, meta_path, allow_incomplete=False):
    errors, warnings = [], []
    meta = load_json(meta_path)
    repo, branch = str(meta.get("repo") or ""), str(meta.get("branch") or "")
    if not repo or "@master" in repo:
        errors.append("仓库名必须为纯仓名，分支单独记录: %r" % repo)
    if not re.fullmatch(r"[0-9a-fA-F]{40}", str(meta.get("commit") or "")):
        errors.append("缺少完整 Git 提交哈希")
    if meta.get("complete") is not True:
        message = "目标未完整完成；报告包含已成功处理的部分结果"
        if allow_incomplete:
            warnings.append(message)
        else:
            errors.append("目标未完整完成")
    scope = meta.get("input_scope")
    if scope is None and str(meta.get("engine") or "").startswith("daily_run/"):
        # daily_run/1 的历史产物在完整仓扫描时未显式写该字段；其扫描语义固定
        # 为 full_repository。新产物已补写，旧产物以警告方式兼容验证。
        warnings.append("历史 daily_run 元数据缺 input_scope，按 full_repository 验证")
    elif scope not in ("full_repository", "production_sources", "file_list"):
        errors.append("扫描范围字段非法: %r" % scope)
    base = artifact_name(repo, branch, meta_path)
    logs_path = os.path.join(workdir, base + "_logs.jsonl")
    findings_path = os.path.join(workdir, base + "_findings.jsonl")
    for path in (logs_path, findings_path):
        if not os.path.exists(path):
            errors.append("结果文件不存在: %s" % os.path.basename(path))
    logs = load_jsonl(logs_path) if os.path.exists(logs_path) else []
    findings = load_jsonl(findings_path) if os.path.exists(findings_path) else []
    if len(logs) != int(meta.get("log_statements") or 0):
        errors.append(
            "日志数不一致: meta=%s jsonl=%d" % (meta.get("log_statements"), len(logs))
        )
    if len(findings) != int(meta.get("findings") or 0):
        errors.append(
            "问题数不一致: meta=%s jsonl=%d" % (meta.get("findings"), len(findings))
        )
    # Legacy single-repository runs omitted the branch in findings while the
    # metadata recorded master explicitly. Treat an empty branch as master for
    # identity checks so partial reports can still be validated and delivered.
    expected_branch = branch or "master"
    log_locations = {(row.get("file"), row.get("line")) for row in logs}
    for index, row in enumerate(findings, 1):
        _tmp_result = []
        for name in REQUIRED_FINDING_FIELDS:
            if row.get(name) in (None, ""):
                _tmp_result.append(name)
        missing = _tmp_result
        if "branch" in missing and expected_branch == "master":
            missing.remove("branch")
        if missing:
            errors.append("问题%d缺少字段: %s" % (index, ",".join(missing)))
        finding_branch = str(row.get("branch") or "master")
        if row.get("repo") != repo or finding_branch != expected_branch:
            errors.append("问题%d仓库或分支与元数据不一致" % index)
        if not isinstance(row.get("line"), int) or row.get("line", 0) <= 0:
            errors.append("问题%d行号非法" % index)
        if (row.get("file"), row.get("line")) not in log_locations:
            errors.append("问题%d没有对应日志位置" % index)
        try:
            found = datetime.fromisoformat(str(row.get("found_time")))
            if found.tzinfo is None:
                raise ValueError("无时区")
        except ValueError:
            errors.append("问题%d发现时间不是带时区 ISO 时间" % index)
    if meta.get("segmented_files"):
        errors.append(
            "发现被分段送审的文件（当前策略要求完整文件原子送审）: %s"
            % ", ".join(str(item) for item in meta["segmented_files"][:10])
        )
    return {
        "repo": repo,
        "branch": branch,
        "commit": meta.get("commit"),
        "status": "passed" if not errors else "failed",
        "errors": errors,
        "warnings": warnings,
        "scanned_files": meta.get("scanned_files"),
        "logs": len(logs),
        "findings": len(findings),
    }


def main():
    parser = argparse.ArgumentParser(description="验证日志评测过程和结果完整性")
    parser.add_argument("--workdir", required=True)
    parser.add_argument("--expected-targets", type=int)
    parser.add_argument("--out")
    parser.add_argument(
        "--allow-incomplete",
        action="store_true",
        help="允许对部分完成的目标生成带完整性告警的报告",
    )
    args = parser.parse_args()
    workdir = os.path.abspath(args.workdir)
    targets_path = os.path.join(workdir, "current-run-targets.json")
    if os.path.exists(targets_path):
        target_data = load_json(targets_path)
        meta_paths = [
            os.path.join(workdir, item["artifact"] + "_meta.json")
            for item in target_data.get("items", [])
        ]
    else:
        meta_paths = sorted(glob.glob(os.path.join(workdir, "*_meta.json")))
    rows, errors = [], []
    for path in meta_paths:
        try:
            row = validate_target(workdir, path, args.allow_incomplete)
        except (OSError, ValueError, TypeError) as exc:
            row = {
                "repo": os.path.basename(path),
                "branch": "",
                "status": "failed",
                "errors": ["验证器异常: %s" % exc],
                "warnings": [],
            }
        rows.append(row)
    identities = [(row.get("repo"), row.get("branch")) for row in rows]
    if len(identities) != len(set(identities)):
        errors.append("存在重复仓库/分支元数据")
    if args.expected_targets is not None and len(rows) != args.expected_targets:
        errors.append(
            "目标数不一致: expected=%d actual=%d" % (args.expected_targets, len(rows))
        )
    failed = [row for row in rows if row["status"] != "passed"]
    value = {
        "status": "passed" if not errors and not failed else "failed",
        "generated_at": datetime.now().astimezone().isoformat(timespec="milliseconds"),
        "targets": len(rows),
        "passed": len(rows) - len(failed),
        "failed": len(failed),
        "errors": errors,
        "items": rows,
    }
    out = args.out or os.path.join(workdir, "process-validation.json")
    with open(out, "w", encoding="utf-8") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
    print(
        "[%s] 过程验证: 目标=%d 通过=%d 失败=%d"
        % (
            "ok" if value["status"] == "passed" else "error",
            value["targets"],
            value["passed"],
            value["failed"],
        )
    )
    print("[info] 验证报告: %s" % out)
    return 0 if value["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
