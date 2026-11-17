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

"""Export evaluation findings as issue-tracking JSON records.

Markdown is the primary report format, so every record carries the canonical
Markdown report URL. Callers may still pass an Excel filename/URL (legacy
pipelines); the date is parsed from it and the emitted URL is normalized back
to the Markdown report.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path

import report_artifacts

import log_spec


SEVERITY_MAP = {
    "致命": "P0",
    "严重": "P1",
    "一般": "P2",
    "提示": "P2",
    "P0": "P0",
    "P1": "P1",
    "P2": "P2",
}
RULE_IDS = {rule["name"]: rule["id"] for rule in log_spec.RULES}
SKILL_ROOT = Path(__file__).resolve().parents[1]
COMPONENT_EN_MAP = {
    "runtime": "运行时",
    "hixl": "单边通信",
    "pto_isa": "PTO-isa",
    "pypto": "PyPTO",
    "ops": "算子库",
    "tools": "工具",
    "torchair": "图引擎",
    "ascend_c": "Ascend C",
    "compiler": "编译器",
    "graph_autofusion": "graph autofusion",
    "graph_engine": "graph engine",
    "hccl": "集合通信",
}
COMPONENT_EN_ALIASES = {
    "pto-isa": "pto_isa",
}
CONTACT_EN = "operation_feedback"


def load_repo_map(name):
    path = SKILL_ROOT / "references" / name
    if not path.exists():
        return {}
    value = read_json(path)
    return value if isinstance(value, dict) else {}


def load_team_map():
    return load_repo_map("repo-team-map.json")


def load_component_map():
    return load_repo_map("repo-component-map.json")


def team_of(team_map, repo):
    return team_map.get(repo) or team_map.get(repo.split("@")[0]) or "其他"


def component_of(component_map, repo):
    return component_map.get(repo) or component_map.get(repo.split("@")[0]) or "其他"


def component_en_of(component):
    """Return the dashboard component identifier for a display component."""
    component = str(component or "").strip()
    for component_en, display_name in COMPONENT_EN_MAP.items():
        if component == display_name:
            return component_en
    # Keep component values that are already identifiers usable for callers
    # supplying a custom component map.
    if component in COMPONENT_EN_MAP:
        return component
    if component.casefold() in COMPONENT_EN_ALIASES:
        return COMPONENT_EN_ALIASES[component.casefold()]
    return ""


def read_json(path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def read_jsonl(path):
    with path.open("r", encoding="utf-8") as handle:
        for number, line in enumerate(handle, 1):
            if not line.strip():
                continue
            try:
                yield json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError("%s:%d: %s" % (path, number, exc)) from exc


def issue_description(finding):
    location = str(finding.get("file", "")).strip()
    reason = str(finding.get("reason", finding.get("description", ""))).strip()
    suggestion = str(finding.get("suggestion", "")).strip()
    parts = []
    if location:
        parts.append("文件 %s" % location)
    if reason:
        parts.append(reason)
    if suggestion:
        parts.append("建议：%s" % suggestion)
    return "；".join(parts)


def stable_issue_id(filename, line_number, error_type):
    """Return the MD5 digest of filename, line number, and numeric rule id."""
    location = "%s:%d:%d" % (str(filename), int(line_number), int(error_type))
    return hashlib.md5(location.encode("utf-8")).hexdigest()


def resolve_run_date(workdir, run_date=None):
    """解析本轮报告日期：显式 --date 优先，否则读取运行清单。"""
    if run_date:
        return report_artifacts.validate_run_date(run_date)
    return report_artifacts.run_date_from_manifest(workdir)


def resolve_report_url(report_url=None, run_date=None, workdir=None):
    """返回本轮 Markdown 主报告的规范公共地址。

    显式传入的 report_url 可以是 .md 或旧 .xlsx（文件名或 URL）；这里统一用
    report_artifacts.report_date 解析日期，校验与运行日期一致后再归一化为
    Markdown 公共地址，保证 issue-tracking.json 始终指向主报告。
    """
    if report_url is None:
        return report_artifacts.markdown_report_url(resolve_run_date(workdir, run_date))
    parsed_date = report_artifacts.report_date(str(report_url))
    expected_date = (
        resolve_run_date(workdir, run_date)
        if (workdir is not None or run_date)
        else None
    )
    if expected_date is not None and parsed_date != expected_date:
        raise ValueError("report_url 日期与运行日期不一致")
    return report_artifacts.markdown_report_url(parsed_date)


def export_issues(workdir, status, responsible_team, report_url=None, run_date=None):
    if report_url is None:
        report_url = report_artifacts.markdown_report_url(
            resolve_run_date(workdir, run_date)
        )
    else:
        report_url = resolve_report_url(report_url, run_date, workdir)
    targets_path = workdir / "current-run-targets.json"
    targets = read_json(targets_path)
    fallback_time = targets.get("generated_at", "")
    team_map = load_team_map()
    component_map = load_component_map()
    records = []
    for target in targets.get("items", []):
        repo = str(target.get("repo", ""))
        branch = str(target.get("branch", ""))
        artifact = str(target.get("artifact") or "%s@%s" % (repo, branch))
        findings_path = workdir / (artifact + "_findings.jsonl")
        if not findings_path.exists():
            raise FileNotFoundError(findings_path)
        for finding in read_jsonl(findings_path):
            raw_severity = str(finding.get("severity", "")).strip()
            if raw_severity not in SEVERITY_MAP:
                raise ValueError(
                    "未知严重级 %r，来源 %s" % (raw_severity, findings_path)
                )
            issue_repo = str(finding.get("repo") or repo)
            line_number = int(finding.get("line", 0) or 0)
            rule_name = log_spec.normalize_rule(finding.get("rule", ""))
            error_type = RULE_IDS.get(rule_name)
            if error_type is None:
                raise ValueError("未知规则 %r，来源 %s" % (rule_name, findings_path))
            issue_uuid = stable_issue_id(
                finding.get("file", ""), line_number, error_type
            )
            component = component_of(component_map, issue_repo)
            records.append(
                {
                    "uuid": issue_uuid,
                    "error_type": error_type,
                    "component": component,
                    "component_en": component_en_of(component),
                    "repo": issue_repo,
                    "branch": str(finding.get("branch") or branch),
                    "file": str(finding.get("file", "")),
                    "line_number": line_number,
                    "rule": str(finding.get("rule", "")),
                    "description": issue_description(finding),
                    "severity": SEVERITY_MAP[raw_severity],
                    "status": status,
                    "discovered_at": str(finding.get("found_time") or fallback_time),
                    "responsible_team": (
                        responsible_team or team_of(team_map, issue_repo)
                    ),
                    "contact": "",
                    "contact_en": CONTACT_EN,
                    "report_url": report_url,
                }
            )
    records.sort(
        key=lambda row: (
            row["repo"],
            row["branch"],
            row["description"],
            row["line_number"],
            row["rule"],
        )
    )
    return records


def write_issue_json(
    workdir,
    output=None,
    status="未闭环",
    responsible_team=None,
    report_url=None,
    run_date=None,
):
    workdir = Path(workdir).resolve()
    output = Path(output).resolve() if output else workdir / "issue-tracking.json"
    # 默认生成 Markdown 公共地址；传入 .md/.xlsx 时解析日期后统一归一化为
    # Markdown 地址（旧 Excel URL 输入也输出 Markdown）。
    report_url = resolve_report_url(report_url, run_date, workdir)
    records = export_issues(workdir, status, responsible_team, report_url, run_date)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    with temporary.open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(records, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    os.replace(temporary, output)
    return output, len(records)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="把评测 findings 导出为问题跟踪 JSON 数组"
    )
    parser.add_argument(
        "--workdir", required=True, help="包含 current-run-targets.json 的评测输出目录"
    )
    parser.add_argument("--out", help="输出 JSON；默认 <workdir>/issue-tracking.json")
    parser.add_argument("--status", default="未闭环")
    parser.add_argument(
        "--responsible-team",
        "--assignee",
        dest="responsible_team",
        default=None,
        help="统一指定责任分队；默认按仓库填写所属分队",
    )
    parser.add_argument(
        "--report-url",
        help="报告 URL（.md 或 .xlsx）；默认按本轮日期生成 Markdown 公共地址",
    )
    parser.add_argument("--date", help="报告日期 YYYYMMDD；默认读取本轮运行清单")
    args = parser.parse_args(argv)

    output, count = write_issue_json(
        args.workdir,
        args.out,
        args.status,
        args.responsible_team,
        args.report_url,
        args.date,
    )
    print("[ok] %s: %d issues" % (output, count))
    return 0


if __name__ == "__main__":
    main()
