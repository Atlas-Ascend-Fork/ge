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

"""Generate the complete dated delivery bundle for one evaluation run.

Markdown is the primary deliverable. The Excel workbook is generated only when
explicitly requested and never blocks the bundle when openpyxl is absent.
"""

import argparse
import subprocess
import sys
from pathlib import Path

import report_artifacts


HERE = Path(__file__).resolve().parent


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


def generate_all(workdir, run_date=None, excel=False, report_name=None):
    """Generate required Markdown/JSON artifacts and the optional Excel file.

    Returns (run_date, paths, warnings). ``warnings`` reports optional artifacts
    that could not be produced (for example Excel without openpyxl).
    """
    workdir = Path(workdir).resolve()
    resolved_date = (
        report_artifacts.validate_run_date(run_date)
        if run_date
        else report_artifacts.run_date_from_manifest(workdir)
    )
    required = report_artifacts.required_delivery_names(resolved_date)
    if report_name:
        required["full_markdown"] = report_artifacts.validate_report_name(report_name)
    paths = {key: workdir / name for key, name in required.items()}

    run_tool("gen_md_report.py", "--workdir", workdir, "--out", paths["full_markdown"])
    run_tool(
        "export_issue_json.py",
        "--workdir",
        workdir,
        "--out",
        paths["issues"],
        "--date",
        resolved_date,
    )

    warnings = []
    if excel:
        excel_path = (
            workdir / report_artifacts.optional_delivery_names(resolved_date)["excel"]
        )
        try:
            run_tool(
                "gen_excel_report.py",
                "--workdir",
                workdir,
                "--out",
                excel_path,
                "--date",
                resolved_date,
            )
        except RuntimeError as exc:
            warnings.append("可选 Excel 报告未生成：%s" % exc)
        else:
            paths["excel"] = excel_path

    missing = [str(path) for path in paths.values() if not path.is_file()]
    if missing:
        raise RuntimeError("交付物未全部生成：%s" % ", ".join(missing))
    return resolved_date, paths, warnings


def main(argv=None):
    parser = argparse.ArgumentParser(description="生成完整的每日评测交付物")
    parser.add_argument("--workdir", required=True)
    parser.add_argument("--date", help="报告日期 YYYYMMDD；默认读取本轮运行清单")
    parser.add_argument(
        "--excel", action="store_true", help="同时生成可选 Excel 报告（需要 openpyxl）"
    )
    parser.add_argument(
        "--report-name",
        help="Markdown 报告文件名；默认 cann-log-evaluation-report-<date>.md",
    )
    args = parser.parse_args(argv)
    run_date, paths, warnings = generate_all(
        args.workdir, args.date, excel=args.excel, report_name=args.report_name
    )
    for warning in warnings:
        print("[warn] %s" % warning)
    print("[ok] %s 交付物已生成" % run_date)
    for key, path in paths.items():
        label = "（可选）" if key == "excel" else ""
        print("[ok] %s%s" % (path, label))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, TypeError, RuntimeError) as exc:
        print("[error] %s" % exc, file=sys.stderr)
        sys.exit(1)
