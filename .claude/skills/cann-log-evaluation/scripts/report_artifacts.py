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

"""Shared date and filename rules for daily report artifacts.

Markdown is the primary delivery format; the dated Excel workbook is an
optional attachment kept for consumers that still need a spreadsheet.
"""

import json
import re
from datetime import datetime
from pathlib import Path


REPORT_PREFIX = "cann-log-evaluation-report"
REPORT_DOMAIN = "https://opencann-obs.obs.cn-north-4.myhuaweicloud.com"
REPORT_SUFFIXES = ("md", "xlsx")
DEFAULT_REPORT_SUFFIX = "md"
REPORT_URL_TEMPLATE = (
    "{domain}/ann_exp/static/prod/A5/agent_component_log/default/{date}/{name}"
)

# Legacy aliases retained for callers that import the old Excel-only names.
EXCEL_REPORT_PREFIX = REPORT_PREFIX
EXCEL_REPORT_DOMAIN = REPORT_DOMAIN

REPORT_NAME_PATTERN = re.compile(r"^%s-(\d{8})\.(md|xlsx)$" % re.escape(REPORT_PREFIX))
REPORT_URL_PATTERN = re.compile(
    r"^%s/ann_exp/static/prod/A5/agent_component_log/default/"
    r"(\d{8})/%s-(\d{8})\.(md|xlsx)$"
    % (re.escape(REPORT_DOMAIN), re.escape(REPORT_PREFIX))
)
EXCEL_REPORT_PATTERN = re.compile(r"^%s-(\d{8})\.xlsx$" % re.escape(REPORT_PREFIX))
EXCEL_REPORT_URL_PATTERN = re.compile(
    r"^%s/ann_exp/static/prod/A5/agent_component_log/default/"
    r"(\d{8})/%s-(\d{8})\.xlsx$" % (re.escape(REPORT_DOMAIN), re.escape(REPORT_PREFIX))
)


def validate_run_date(value):
    value = str(value or "")
    try:
        parsed = datetime.strptime(value, "%Y%m%d")
    except ValueError as exc:
        raise ValueError("日期必须是有效的 8 位 YYYYMMDD") from exc
    if parsed.strftime("%Y%m%d") != value:
        raise ValueError("日期必须是有效的 8 位 YYYYMMDD")
    return value


def current_run_date(now=None):
    current = now if now is not None else datetime.now().astimezone()
    return current.strftime("%Y%m%d")


def report_name(run_date, suffix=DEFAULT_REPORT_SUFFIX):
    """Return the dated report filename for the requested suffix."""
    suffix = str(suffix or DEFAULT_REPORT_SUFFIX).lower().lstrip(".")
    if suffix not in REPORT_SUFFIXES:
        raise ValueError("报告后缀必须是 md 或 xlsx")
    return "%s-%s.%s" % (REPORT_PREFIX, validate_run_date(run_date), suffix)


def excel_report_name(run_date):
    return report_name(run_date, "xlsx")


def markdown_report_name(run_date):
    return report_name(run_date, "md")


def full_markdown_report_name(run_date):
    """The complete Markdown report is the primary delivery artifact."""
    return markdown_report_name(run_date)


def validate_report_name(name):
    """Validate a caller-supplied Markdown report filename.

    Only a plain ``.md`` filename is accepted; directory components are
    rejected so callers cannot escape the requested ``--out`` directory.
    """
    value = str(name or "")
    if not value or value in (".", ".."):
        raise ValueError("报告文件名不能为空")
    if "/" in value or "\\" in value or "\x00" in value:
        raise ValueError("报告文件名不能包含目录分隔符")
    if not value.endswith(".md"):
        raise ValueError("报告文件名必须以 .md 结尾")
    return value


def report_url(run_date, suffix=DEFAULT_REPORT_SUFFIX):
    """Return the public OBS URL embedded in issue-tracking JSON."""
    run_date = validate_run_date(run_date)
    return REPORT_URL_TEMPLATE.format(
        domain=REPORT_DOMAIN, date=run_date, name=report_name(run_date, suffix)
    )


def markdown_report_url(run_date):
    """Canonical report link: the dated Markdown report on OBS."""
    return report_url(run_date, "md")


def excel_report_url(run_date):
    """Optional attachment link kept for Excel-based consumers."""
    return report_url(run_date, "xlsx")


def report_date(filename_or_url):
    """Parse the run date from a report filename or its public OBS URL."""
    value = str(filename_or_url or "")
    url_match = REPORT_URL_PATTERN.fullmatch(value)
    if url_match:
        url_date, name_date, _suffix = url_match.groups()
        if url_date != name_date:
            raise ValueError("报告 URL 的目录日期与文件日期不一致")
        return validate_run_date(url_date)
    name = Path(value).name
    if name != value:
        raise ValueError("报告名必须是不含目录的文件名")
    match = REPORT_NAME_PATTERN.fullmatch(name)
    if not match:
        raise ValueError("报告名必须为 %s-YYYYMMDD.md 或 .xlsx" % REPORT_PREFIX)
    return validate_run_date(match.group(1))


def excel_report_date(filename_or_url):
    """Legacy alias: report dates are format agnostic now."""
    return report_date(filename_or_url)


def required_delivery_names(run_date):
    """Artifacts a successful run must produce (Markdown report first)."""
    run_date = validate_run_date(run_date)
    return {
        "full_markdown": markdown_report_name(run_date),
        "issues": "issue-tracking.json",
    }


def optional_delivery_names(run_date):
    """Artifacts produced only when explicitly requested."""
    return {"excel": excel_report_name(run_date)}


def delivery_names(run_date, include_excel=True, report_name=None):
    """All dated delivery artifacts; Excel is optional by default."""
    names = required_delivery_names(run_date)
    if report_name:
        names["full_markdown"] = validate_report_name(report_name)
    if include_excel:
        names.update(optional_delivery_names(run_date))
    return names


def run_date_from_manifest(workdir, now=None):
    """Resolve one run date for all artifacts in an output directory."""
    manifest = Path(workdir) / "current-run-targets.json"
    if manifest.is_file():
        with manifest.open("r", encoding="utf-8-sig") as handle:
            value = json.load(handle)
        if value.get("run_date"):
            return validate_run_date(value["run_date"])
        generated_at = str(value.get("generated_at") or "")
        if generated_at:
            try:
                parsed = datetime.fromisoformat(generated_at.replace("Z", "+00:00"))
            except ValueError as exc:
                raise ValueError(
                    "current-run-targets.json 的 generated_at 不是有效时间"
                ) from exc
            return parsed.strftime("%Y%m%d")
    return current_run_date(now)


def verify_delivery(workdir, run_date=None, include_excel=False, report_name=None):
    """Return missing required/optional delivery filenames."""
    resolved = (
        validate_run_date(run_date) if run_date else run_date_from_manifest(workdir)
    )
    names = delivery_names(
        resolved, include_excel=include_excel, report_name=report_name
    )
    return [name for name in names.values() if not (Path(workdir) / name).is_file()]
