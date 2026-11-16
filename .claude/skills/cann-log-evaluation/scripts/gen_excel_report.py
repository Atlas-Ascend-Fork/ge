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

"""生成 CANN 日志评测 Excel 报告（可选交付物）。

评分、指标、分队与展示逻辑统一收敛在 :mod:`report_metrics`，与 Markdown
主报告共用同一实现。``openpyxl`` 改为函数内延迟导入：仅在没有安装依赖时
才在真正生成 Excel 时给出明确错误，模块本身始终可导入。
"""

import argparse
import glob
import json
import os
import sys
from collections import Counter, defaultdict
from datetime import datetime

import report_artifacts

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402
from report_metrics import (  # noqa: E402,F401  兼容旧调用方，重新导出
    DEFAULT_TEAM_MAP_PATH,
    DI_WEIGHTS,
    LANGUAGE_RULES,
    METRIC_RULES,
    METRICS_PATH,
    SEVERITY_DISPLAY,
    SEVERITY_LEVELS,
    UNMAPPED_METRICS,
    display_severity,
    di_total,
    finding_key,
    found_time,
    load_quantitative_metrics,
    load_team_map,
    md_cell,
    md_code,
    metric_results,
    metric_score,
    severity_counts,
    target_metrics,
    team_for,
    total_score,
)

RULE_DESC = log_spec.rule_desc()
DISPLAY_ORDER = {"致命": 0, "严重": 1, "一般": 2, "提示": 3}

# 旧名兼容：历史测试/脚本可能直接引用以下划线开头的函数名。
_finding_key = finding_key
_metric_score = metric_score
_metric_results = metric_results
_target_metrics = target_metrics
_total_score = total_score

DEFAULT_FONT_NAME = "Arial"

# openpyxl 相关名称在首次生成报告时才填充（见 _require_openpyxl）。
Workbook = load_workbook = Comment = ColorScaleRule = None
Alignment = Border = Font = PatternFill = Side = get_column_letter = None
HEADER_FILL = HEADER_FONT = BODY_FONT = TITLE_FILL = TITLE_FONT = None
TOTAL_FILL = THIN_GRAY = CELL_BORDER = None
SEVERITY_FILL = None
_STYLE_READY = False

# openpyxl rejects C0/C1 control characters in cell text.  Model responses
# can occasionally echo one from a source literal, so sanitize at the export
# boundary while preserving the original JSONL findings unchanged.
_EXCEL_CONTROL_CHARS = {chr(code) for code in range(32) if code not in (9, 10, 13)}
_EXCEL_CONTROL_CHARS.update(chr(code) for code in range(127, 160))


def _require_openpyxl():
    """延迟导入 openpyxl 并初始化样式；缺失时抛出可读的 RuntimeError。"""
    global _STYLE_READY, Workbook, load_workbook, Comment, ColorScaleRule
    global Alignment, Border, Font, PatternFill, Side, get_column_letter
    global HEADER_FILL, HEADER_FONT, BODY_FONT, TITLE_FILL, TITLE_FONT
    global TOTAL_FILL, THIN_GRAY, CELL_BORDER, SEVERITY_FILL
    if _STYLE_READY:
        return
    try:
        from openpyxl import Workbook as _Workbook
        from openpyxl import load_workbook as _load_workbook
        from openpyxl.comments import Comment as _Comment
        from openpyxl.formatting.rule import ColorScaleRule as _ColorScaleRule
        from openpyxl.styles import (
            Alignment as _Alignment,
            Border as _Border,
            Font as _Font,
            PatternFill as _PatternFill,
            Side as _Side,
        )
        from openpyxl.utils import get_column_letter as _get_column_letter
    except ImportError as exc:
        raise RuntimeError(
            "生成 Excel 报告需要 openpyxl，请安装 requirements.txt 中的可选依赖（openpyxl）"
        ) from exc

    Workbook, load_workbook = _Workbook, _load_workbook
    Comment, ColorScaleRule = _Comment, _ColorScaleRule
    Alignment, Border, Font = _Alignment, _Border, _Font
    PatternFill, Side = _PatternFill, _Side
    get_column_letter = _get_column_letter

    HEADER_FILL = PatternFill("solid", fgColor="4472C4")
    HEADER_FONT = Font(name=DEFAULT_FONT_NAME, color="FFFFFF", bold=True)
    BODY_FONT = Font(name=DEFAULT_FONT_NAME, color="1F1F1F", size=10)
    TITLE_FILL = PatternFill("solid", fgColor="17365D")
    TITLE_FONT = Font(name=DEFAULT_FONT_NAME, color="FFFFFF", bold=True, size=16)
    TOTAL_FILL = PatternFill("solid", fgColor="DCE6F1")
    THIN_GRAY = Side(style="thin", color="D9E1F2")
    CELL_BORDER = Border(
        left=THIN_GRAY, right=THIN_GRAY, top=THIN_GRAY, bottom=THIN_GRAY
    )
    SEVERITY_FILL = {
        "提示": PatternFill("solid", fgColor="E2F0D9"),
        "一般": PatternFill("solid", fgColor="FFF2CC"),
        "严重": PatternFill("solid", fgColor="F8CBAD"),
        "致命": PatternFill("solid", fgColor="FF6B6B"),
    }
    _STYLE_READY = True


def excel_text(value):
    if value is None:
        return ""
    text = str(value)
    return "".join(char for char in text if char not in _EXCEL_CONTROL_CHARS)


def _timestamp(path):
    try:
        return datetime.fromtimestamp(os.path.getmtime(path)).strftime(
            "%Y-%m-%d %H:%M:%S"
        )
    except OSError:
        return ""


def load(workdir):
    findings, metas = [], []
    targets_path = os.path.join(workdir, "current-run-targets.json")
    artifacts = None
    if os.path.exists(targets_path):
        with open(targets_path, "r", encoding="utf-8-sig") as handle:
            _tmp_result = []
            for item in json.load(handle).get("items", []):
                _tmp_result.append(item["artifact"])
            artifacts = _tmp_result
    finding_paths = (
        [os.path.join(workdir, name + "_findings.jsonl") for name in artifacts]
        if artifacts is not None
        else glob.glob(os.path.join(workdir, "*_findings.jsonl"))
    )
    if artifacts is None:
        finding_paths += glob.glob(os.path.join(workdir, "*_semantic.jsonl"))
    for path in sorted(set(finding_paths)):
        fallback_time = _timestamp(path)
        with open(path, "r", encoding="utf-8-sig") as handle:
            for line in handle:
                line = line.strip()
                if not line:
                    continue
                row = json.loads(line)
                row.setdefault("_source_found_time", fallback_time)
                findings.append(row)
    meta_paths = (
        [os.path.join(workdir, name + "_meta.json") for name in artifacts]
        if artifacts is not None
        else glob.glob(os.path.join(workdir, "*_meta.json"))
    )
    for path in sorted(meta_paths):
        with open(path, "r", encoding="utf-8-sig") as handle:
            meta = json.load(handle)
        if meta.get("repo"):
            metas.append(meta)
    return findings, metas


def style_header(sheet, columns):
    _require_openpyxl()
    for column in range(1, columns + 1):
        cell = sheet.cell(row=1, column=column)
        cell.fill = HEADER_FILL
        cell.font = HEADER_FONT
        cell.border = CELL_BORDER
        cell.alignment = Alignment(
            horizontal="center", vertical="center", wrap_text=True
        )
    sheet.row_dimensions[1].height = 30
    sheet.freeze_panes = "A2"
    sheet.sheet_view.showGridLines = False


def style_body(sheet, first_row, last_row, columns):
    _require_openpyxl()
    for row in sheet.iter_rows(min_row=first_row, max_row=last_row, max_col=columns):
        for cell in row:
            cell.font = BODY_FONT
            cell.border = CELL_BORDER
            cell.alignment = Alignment(vertical="center")


def _counts_by_target(findings):
    counts = defaultdict(Counter)
    for row in findings:
        key = (str(row.get("repo") or ""), str(row.get("branch") or ""))
        counts[key][display_severity(row.get("severity"))] += 1
    return counts


def _remove_report_sheets(workbook):
    for name in (
        "概览",
        "分类统计",
        "问题明细",
        "看板展示1",
        "看板展示2",
        "日志质量测评明细",
        "量化评估指标",
    ):
        if name in workbook.sheetnames:
            workbook.remove(workbook[name])


def add_report_sheets(workbook, findings, metas, team_map=None):
    """生成问题报表、看板和量化评估指标表。"""
    _require_openpyxl()
    team_map = load_team_map(None) if team_map is None else team_map
    metric_definitions = load_quantitative_metrics()
    target_metrics_result = target_metrics(findings, metas, metric_definitions)
    _remove_report_sheets(workbook)
    overview = workbook.create_sheet("概览", 0)
    classification = workbook.create_sheet("分类统计", 1)
    details = workbook.create_sheet("问题明细", 2)
    quantitative = workbook.create_sheet("量化评估指标", 3)
    _build_overview(overview, findings, metas, team_map, target_metrics_result)
    _build_classification(classification, findings)
    _build_details(details, findings, team_map)
    _build_quantitative(
        quantitative, metric_definitions, metas, team_map, target_metrics_result
    )
    return (overview, classification, details, quantitative)


def _build_overview(sheet, findings, metas, team_map, target_metrics):
    _require_openpyxl()
    headers = [
        "所属分队",
        "仓库",
        "分支",
        "扫描文件",
        "日志语句",
        "提示问题",
        "一般问题",
        "严重问题",
        "致命问题",
        "问题总数",
        "总DI数",
        "总评分（10分制）",
    ]
    sheet.append(headers)
    style_header(sheet, len(headers))
    sheet.cell(row=1, column=12).comment = Comment(
        "量化指标总评分为全部量化指标得分的平均值，采用10分制；"
        "敏感信息违规一票否决。请求失败时仍使用已完成部分计算，并标记为部分评估。",
        "CANN-Tools",
    )
    counts = _counts_by_target(findings)

    for meta in sorted(
        metas,
        key=lambda row: (str(row.get("repo") or ""), str(row.get("branch") or "")),
    ):
        repo = str(meta.get("repo") or "")
        branch = str(meta.get("branch") or "-")
        severity = counts.get((repo, "" if branch == "-" else branch), Counter())
        if not severity:
            severity = Counter(
                {
                    display_severity(name): count
                    for name, count in (meta.get("by_severity") or {}).items()
                }
            )
        hint, normal = severity["提示"], severity["一般"]
        serious, fatal = severity["严重"], severity["致命"]
        total = hint + normal + serious + fatal
        di = (
            hint * DI_WEIGHTS["提示"]
            + normal * DI_WEIGHTS["一般"]
            + serious * DI_WEIGHTS["严重"]
            + fatal * DI_WEIGHTS["致命"]
        )
        metrics = target_metrics.get((repo, "" if branch == "-" else branch), {})
        sheet.append(
            [
                excel_text(team_for(repo, branch, team_map)),
                excel_text(repo),
                excel_text(branch),
                int(meta.get("scanned_files") or 0),
                int(meta.get("log_statements") or 0),
                hint,
                normal,
                serious,
                fatal,
                total,
                di,
                total_score(metrics),
            ]
        )

    style_body(sheet, 2, sheet.max_row, len(headers))
    for row in range(2, sheet.max_row + 1):
        for column, severity in ((6, "提示"), (7, "一般"), (8, "严重"), (9, "致命")):
            sheet.cell(row=row, column=column).fill = SEVERITY_FILL[severity]
        for column in range(4, 13):
            sheet.cell(row=row, column=column).alignment = Alignment(
                horizontal="center", vertical="center"
            )
        sheet.cell(row=row, column=11).number_format = "0.0"
        sheet.cell(row=row, column=12).number_format = "0.0"
        sheet.row_dimensions[row].height = 24
    if sheet.max_row >= 2:
        sheet.conditional_formatting.add(
            "K2:K%d" % sheet.max_row,
            ColorScaleRule(
                start_type="min",
                start_color="E2F0D9",
                mid_type="percentile",
                mid_value=50,
                mid_color="FFF2CC",
                end_type="max",
                end_color="F8696B",
            ),
        )
        sheet.conditional_formatting.add(
            "L2:L%d" % sheet.max_row,
            ColorScaleRule(
                start_type="min",
                start_color="F8696B",
                mid_type="percentile",
                mid_value=50,
                mid_color="FFF2CC",
                end_type="max",
                end_color="E2F0D9",
            ),
        )
    sheet.auto_filter.ref = "A1:L%d" % max(1, sheet.max_row)
    widths = [18, 32, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24]
    for index, width in enumerate(widths, 1):
        sheet.column_dimensions[get_column_letter(index)].width = width


def _build_quantitative(sheet, definitions, metas, team_map, target_metrics):
    _require_openpyxl()
    headers = [
        "所属分队",
        "仓库",
        "分支",
        "指标ID",
        "评估对象",
        "特征维度",
        "层级",
        "量化指标",
        "合规率",
        "得分",
        "违规数",
        "样本数",
        "评估状态",
        "量化公式",
        "打分规则",
        "评估说明",
    ]
    sheet.append(headers)
    style_header(sheet, len(headers))
    for meta in sorted(
        metas,
        key=lambda row: (str(row.get("repo") or ""), str(row.get("branch") or "")),
    ):
        repo = str(meta.get("repo") or "")
        branch = str(meta.get("branch") or "")
        results = target_metrics.get((repo, branch), {})
        for definition in definitions:
            result = results.get(definition["指标ID"], {})
            sheet.append(
                [
                    excel_text(team_for(repo, branch, team_map)),
                    excel_text(repo),
                    excel_text(branch),
                    excel_text(definition["指标ID"]),
                    excel_text(definition["评估对象"]),
                    excel_text(definition["特征维度"]),
                    excel_text(definition["层级"]),
                    excel_text(definition["量化指标"]),
                    result.get("rate"),
                    result.get("score"),
                    result.get("violations"),
                    result.get("sample"),
                    result.get("status", "未评估"),
                    excel_text(definition["量化公式"]),
                    excel_text(definition["打分规则"]),
                    excel_text(result.get("explanation", "全部指标均按10分制计算")),
                ]
            )
    style_body(sheet, 2, sheet.max_row, len(headers))
    for row in range(2, sheet.max_row + 1):
        sheet.cell(row=row, column=9).number_format = "0.0%"
        sheet.cell(row=row, column=10).number_format = "0.0"
        for column in (4, 7, 9, 10, 11, 12, 13):
            sheet.cell(row=row, column=column).alignment = Alignment(
                horizontal="center", vertical="center"
            )
        for column in (8, 14, 15, 16):
            sheet.cell(row=row, column=column).alignment = Alignment(
                vertical="center", wrap_text=True
            )
        sheet.cell(row=row, column=13).fill = (
            SEVERITY_FILL["致命"]
            if sheet.cell(row=row, column=13).value == "一票否决"
            else PatternFill()
        )
        sheet.row_dimensions[row].height = 54
    sheet.freeze_panes = "H2"
    sheet.auto_filter.ref = "A1:P%d" % max(1, sheet.max_row)
    for index, width in enumerate(
        (18, 28, 12, 14, 22, 18, 10, 28, 12, 10, 10, 10, 14, 70, 70, 56), 1
    ):
        sheet.column_dimensions[get_column_letter(index)].width = width


def _build_classification(sheet, findings):
    _require_openpyxl()
    sheet.append(["规则编号", "规则大类", "规则", "规则说明", "严重级", "问题数"])
    style_header(sheet, 6)
    counts = Counter(row.get("rule") for row in findings if row.get("rule"))
    rules = sorted(log_spec.rule_names(), key=lambda rule: (-counts[rule], rule))
    for rule in rules:
        sheet.append(
            [
                log_spec.rule_id(rule),
                log_spec.rule_category(rule),
                rule,
                log_spec.rule_spec(rule),
                log_spec.rule_severity(rule),
                counts[rule],
            ]
        )
    style_body(sheet, 2, sheet.max_row, 6)
    for row in range(2, sheet.max_row + 1):
        severity = sheet.cell(row=row, column=5).value
        sheet.cell(row=row, column=5).fill = SEVERITY_FILL.get(severity, PatternFill())
        sheet.cell(row=row, column=5).font = Font(
            name=DEFAULT_FONT_NAME,
            bold=True,
            color="FFFFFF" if severity == "致命" else "1F1F1F",
        )
        for column in (1, 2, 5, 6):
            sheet.cell(row=row, column=column).alignment = Alignment(
                horizontal="center", vertical="center", wrap_text=True
            )
        sheet.cell(row=row, column=4).alignment = Alignment(
            vertical="top", wrap_text=True
        )
        sheet.row_dimensions[row].height = 48
    sheet.auto_filter.ref = "A1:F%d" % max(1, sheet.max_row)
    for index, width in enumerate((14, 16, 26, 90, 12, 12), 1):
        sheet.column_dimensions[get_column_letter(index)].width = width


def _build_details(sheet, findings, team_map):
    _require_openpyxl()
    headers = [
        "严重级",
        "规则",
        "仓库",
        "分支",
        "文件",
        "行号",
        "代码",
        "问题原因",
        "修复建议",
        "发现时间",
        "所属分队",
    ]
    sheet.append(headers)
    style_header(sheet, len(headers))
    ordered = sorted(
        findings,
        key=lambda row: (
            DISPLAY_ORDER.get(display_severity(row.get("severity")), 9),
            str(row.get("repo") or ""),
            str(row.get("file") or ""),
            int(row.get("line") or 0),
        ),
    )
    for row in ordered:
        repo = str(row.get("repo") or "")
        branch = str(row.get("branch") or "-")
        sheet.append(
            [
                excel_text(display_severity(row.get("severity"))),
                excel_text(row.get("rule")),
                excel_text(repo),
                excel_text(branch),
                excel_text(row.get("file")),
                int(row.get("line") or 0),
                excel_text(str(row.get("code") or "")[:30000]),
                excel_text(row.get("reason", "")),
                excel_text(row.get("suggestion", "")),
                excel_text(found_time(row)),
                excel_text(team_for(repo, branch, team_map)),
            ]
        )

    style_body(sheet, 2, sheet.max_row, len(headers))
    for row in range(2, sheet.max_row + 1):
        severity = sheet.cell(row=row, column=1).value
        sheet.cell(row=row, column=1).fill = SEVERITY_FILL.get(severity, PatternFill())
        sheet.cell(row=row, column=1).font = Font(
            name=DEFAULT_FONT_NAME,
            bold=True,
            color="FFFFFF" if severity == "致命" else "1F1F1F",
        )
        for column in (1, 2, 3, 4, 6, 10, 11):
            sheet.cell(row=row, column=column).alignment = Alignment(
                horizontal="center", vertical="center", wrap_text=True
            )
        for column in (5, 7, 8, 9):
            sheet.cell(row=row, column=column).alignment = Alignment(
                vertical="top", wrap_text=True
            )
        sheet.row_dimensions[row].height = 42
    sheet.auto_filter.ref = "A1:K%d" % max(1, sheet.max_row)
    widths = [10, 20, 16, 12, 52, 9, 58, 45, 45, 28, 18]
    for index, width in enumerate(widths, 1):
        sheet.column_dimensions[get_column_letter(index)].width = width


def create_report(findings, metas, out, team_map=None):
    _require_openpyxl()
    workbook = Workbook()
    workbook.remove(workbook.active)
    add_report_sheets(workbook, findings, metas, team_map)
    workbook.save(out)


def augment_report(base_workbook, findings, metas, out, team_map=None):
    _require_openpyxl()
    workbook = load_workbook(base_workbook)
    add_report_sheets(workbook, findings, metas, team_map)
    workbook.save(out)


def main():
    parser = argparse.ArgumentParser(description="生成 Excel 评测报告")
    parser.add_argument("--workdir", required=True)
    parser.add_argument("--out")
    parser.add_argument("--date", help="报告日期 YYYYMMDD；默认读取本轮运行清单")
    parser.add_argument("--team-map", help="可选：仓库到所属分队的 JSON 映射")
    parser.add_argument("--base-workbook", help="可选：保留已有工作簿中的其它工作表")
    args = parser.parse_args()

    findings, metas = load(args.workdir)
    team_map = load_team_map(args.team_map)
    run_date = (
        report_artifacts.validate_run_date(args.date)
        if args.date
        else report_artifacts.run_date_from_manifest(args.workdir)
    )
    out = args.out or os.path.join(
        args.workdir, report_artifacts.excel_report_name(run_date)
    )
    if args.base_workbook:
        augment_report(args.base_workbook, findings, metas, out, team_map)
    else:
        create_report(findings, metas, out, team_map)
    print("[ok] 问题总计 %d 处，覆盖 %d 个目标" % (len(findings), len(metas)))
    print("[ok] Excel 报告: %s" % out)
    print("[ok] 含 4 个工作表：概览 / 分类统计 / 问题明细 / 量化评估指标")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as exc:  # 例如缺少 openpyxl：给出干净的错误而非堆栈
        print("[error] %s" % exc, file=sys.stderr)
        sys.exit(1)
