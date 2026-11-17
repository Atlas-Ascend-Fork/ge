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

"""CANN 日志规范评测 —— 完整 Markdown 主报告 (cann-log-evaluation)。

Markdown 是唯一主交付报告（原汇总报告已合并进顶部「管理摘要」），包含：

- 管理摘要：目标概览、按来源分类（核心代码优先）、每类问题示例
- 10 分制总评分、总 DI（按提示/一般/严重/致命加权）
- 27 项量化指标（合规率 / 得分 / 违规数 / 样本数 / 评估状态）
- 完整性告警（存在失败请求的目标）
- 问题明细（代码地址、代码片段、原因、建议、发现时间、所属分队）

评分与指标口径统一来自 :mod:`report_metrics`，与 Excel 可选报告完全一致。
所有写入表格的文本都经过 Markdown 转义，换行、``|`` 与反引号不会破坏表格。

用法:
    python3 gen_md_report.py --workdir <评测输出目录> [--out report.md]
        [--no-metrics] [--max-code-chars 2000] [--examples 5]
"""

import argparse
import glob
import json
import os
import re
import sys
import time
from collections import Counter

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402  规范唯一事实源，增删规则改那里
import report_artifacts  # noqa: E402
import report_metrics  # noqa: E402  评分/指标/分队/Markdown 转义

SEV_ORDER = log_spec.sev_order()
CN_NUMBERS = ("一", "二", "三", "四", "五", "六", "七", "八", "九", "十")
DEFAULT_MAX_CODE_CHARS = 2000
DEFAULT_EXAMPLE_LIMIT = 5


def _timestamp(path):
    """按文件修改时间生成发现时间兜底值。"""
    try:
        return time.strftime(
            "%Y-%m-%d %H:%M:%S", time.localtime(os.path.getmtime(path))
        )
    except OSError:
        return ""


def load_findings(workdir):
    """按本轮运行清单（current-run-targets.json）加载 findings 与 meta。

    清单存在时只读取清单声明的 ``<artifact>_findings.jsonl`` /
    ``<artifact>_meta.json``；清单缺失时回退到目录内通配匹配。
    """
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
    for path in sorted(set(finding_paths)):
        if not os.path.isfile(path):
            continue
        fallback_time = _timestamp(path)
        with open(path, "r", encoding="utf-8-sig") as handle:
            for line in handle:
                line = line.strip()
                if line:
                    row = json.loads(line)
                    row.setdefault("_source_found_time", fallback_time)
                    findings.append(row)

    meta_paths = (
        [os.path.join(workdir, name + "_meta.json") for name in artifacts]
        if artifacts is not None
        else glob.glob(os.path.join(workdir, "*_meta.json"))
    )
    for path in sorted(set(meta_paths)):
        if not os.path.isfile(path):
            continue
        with open(path, "r", encoding="utf-8-sig") as handle:
            metas.append(json.load(handle))
    return findings, metas


def _display_branch(branch):
    """分支为空（master 约定）时统一显示为 ``-``。"""
    return str(branch or "-")


def _branch_suffix(branch):
    """标题里的分支后缀：空分支不拼 ``@``，避免出现 ``repo@-``。"""
    value = str(branch or "").strip()
    return "" if not value or value == "-" else "@%s" % value


def _target_key(repo, branch):
    return (str(repo or ""), str(branch or ""))


def collect_targets(findings, metas, definitions=None):
    """把 findings / metas 聚合成按 (仓库, 分支) 排序的目标列表。"""
    definitions = (
        report_metrics.load_quantitative_metrics()
        if definitions is None
        else definitions
    )
    metrics_by_target = report_metrics.target_metrics(findings, metas, definitions)
    rows_by_target = {}
    for row in findings:
        rows_by_target.setdefault(
            _target_key(row.get("repo"), row.get("branch")), []
        ).append(row)

    ordered = sorted(
        metas,
        key=lambda meta: (str(meta.get("repo") or ""), str(meta.get("branch") or "")),
    )
    targets = []
    for meta in ordered:
        repo = str(meta.get("repo") or "")
        branch = str(meta.get("branch") or "")
        key = _target_key(repo, branch)
        rows = rows_by_target.get(key, [])
        metrics = metrics_by_target.get(key, {})
        counts = report_metrics.severity_counts(rows)
        targets.append(
            {
                "repo": repo,
                "branch": branch,
                "display_branch": _display_branch(branch),
                "scanned_files": int(meta.get("scanned_files") or 0),
                "log_statements": int(meta.get("log_statements") or 0),
                "findings": rows,
                "counts": counts,
                "di": report_metrics.di_total(counts),
                "score": report_metrics.total_score(metrics),
                "metrics": metrics,
                "status": "已评估" if meta.get("complete") is True else "部分评估",
                "meta": meta,
            }
        )
    return targets


def _fmt_score(score):
    return "-" if score is None else "%.1f" % score


def _fmt_rate(rate):
    return "%.1f%%" % (float(rate) * 100)


def _fmt_di(value):
    return "%.1f" % float(value)


def single_line(text):
    """把多行文本压成一行，避免破坏 Markdown 列表结构。"""
    value = str(text or "").replace("\r\n", "\n").replace("\r", "\n")
    return " ".join(part.strip() for part in value.split("\n") if part.strip())


def _rule_desc(rule):
    """规则简述：统一全角括号、去掉斜杠两侧多余空格。"""
    text = log_spec.rule_desc().get(rule, rule)
    return str(text).replace(" / ", "/").replace("(", "（").replace(")", "）")


def _blacklist_text(metas):
    """取本轮运行声明的内部敏感词黑名单，供管理摘要展示。"""
    for meta in metas:
        values = meta.get("blacklist") or []
        if isinstance(values, str):
            values = [values]
        values = [str(item) for item in values if str(item)]
        if values:
            return "、".join(values)
    return "-"


def bullet_text(text):
    """bullet 正文：单行化、转义 HTML、处理行首 Markdown 标记。"""
    value = single_line(text)
    if not value:
        return "-"
    value = value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    if value[:1] in ("#", "-"):
        return "\\" + value
    return value


def bullet_sentence(text):
    """原因/建议统一成句：空值给 ``-``，非标点结尾补中文句号。"""
    value = bullet_text(text)
    if value != "-" and value[-1] not in "。！？；：.!?;:":
        value += "。"
    return value


def inline_code(text, limit=120):
    """行内代码：空值显示 ``-``；动态选择足够长的反引号围栏。"""
    value = single_line(text)
    if limit and len(value) > limit:
        value = value[:limit].rstrip() + "…"
    if not value:
        return "-"
    runs = [len(run) for run in re.findall(r"`+", value)]
    fence = "`" * ((max(runs) + 1) if runs else 1)
    if fence == "`":
        return "`%s`" % value
    return "%s %s %s" % (fence, value, fence)


def _source_breakdown(findings):
    """按来源聚合问题数：核心代码优先，其次工具脚本、测试示例。"""
    by_src = {}
    for row in findings:
        source = str(row.get("source_type") or "核心代码")
        by_src.setdefault(source, Counter())[str(row.get("rule") or "")] += 1
    order = {"核心代码": 0, "工具脚本": 1, "测试示例": 2}
    ordered = sorted(
        by_src.items(), key=lambda item: (order.get(item[0], 9), -sum(item[1].values()))
    )
    return [(source, sum(counter.values()), counter) for source, counter in ordered]


def _table(columns):
    """生成 Markdown 表头与分隔行。"""
    header = "| " + " | ".join(columns) + " |"
    separator = "|" + "|".join("---" for _ in columns) + "|"
    return [header, separator]


def _section(number, title):
    return "## %s、%s" % (CN_NUMBERS[number - 1], title)


def _truncate_code(code, max_code_chars):
    text = "" if code is None else str(code)
    if max_code_chars and max_code_chars > 0:
        return text[:max_code_chars]
    return text


def _metrics_status(result):
    """指标评估状态：未映射指标明确标注未采集，按 10 分计。"""
    if not result or not result.get("collected", True):
        return "未采集（按10分计）"
    return result.get("status", "已评估")


def _first_meta_value(metas, key):
    """取第一个非空的 meta 字段（通常即 metas[0]，缺失时向后兜底）。"""
    for meta in metas:
        value = str(meta.get(key) or "").strip()
        if value:
            return value
    return ""


def build_report(
    findings,
    metas,
    definitions=None,
    include_metrics=True,
    max_code_chars=DEFAULT_MAX_CODE_CHARS,
    generated_at=None,
    team_map=None,
    examples=DEFAULT_EXAMPLE_LIMIT,
):
    """构建完整 Markdown 报告文本（不写文件，便于测试）。"""
    definitions = (
        report_metrics.load_quantitative_metrics()
        if definitions is None
        else definitions
    )
    team_map = report_metrics.load_team_map(None) if team_map is None else team_map
    targets = collect_targets(findings, metas, definitions)
    generated_at = generated_at or time.strftime("%Y-%m-%d %H:%M:%S")
    lines = []
    add = lines.append

    # ---------- 标题与元信息 ----------
    add("# CANN 日志规范评测报告")
    add("")
    meta_parts = ["生成时间：%s" % generated_at, "工具：cann-log-evaluation"]
    model = _first_meta_value(metas, "model")
    if model:
        meta_parts.append("评测模型：%s" % report_metrics.md_cell(model))
    fingerprint = _first_meta_value(metas, "evaluation_fingerprint")
    if fingerprint:
        meta_parts.append("规则指纹：%s" % report_metrics.md_cell(fingerprint))
    # 连续 blockquote 行在 GFM 里会被合并成一个软换行段落；用 <br> 强制分行。
    add("> " + "<br>".join(meta_parts))
    add("")

    # ---------- 管理摘要（原精简汇总报告已合并） ----------
    add("## 管理摘要")
    add("")
    add("### 目标概览")
    add("")
    lines.extend(
        _table(["仓库", "分支", "问题数", "总DI", "总评分（10分制）", "评估状态"])
    )
    for target in sorted(
        targets, key=lambda item: (str(item["repo"]), str(item["branch"]))
    ):
        add(
            "| %s | %s | %d | %s | %s | %s |"
            % (
                report_metrics.md_cell(target["repo"]),
                report_metrics.md_cell(target["display_branch"]),
                len(target["findings"]),
                _fmt_di(target["di"]),
                _fmt_score(target["score"]),
                target["status"],
            )
        )
    add("")
    add("**问题总计 %d 处，覆盖 %d 个目标**" % (len(findings), len(targets)))
    add("")
    blacklist = _blacklist_text(metas)
    if blacklist != "-":
        add("黑名单：%s" % report_metrics.md_cell(blacklist))
        add("")
    add("### 按来源分类（核心代码问题优先处理）")
    add("")
    lines.extend(_table(["来源", "问题数", "明细"]))
    for source, count, counter in _source_breakdown(findings):
        detail = "、".join(
            "%s%d" % (report_metrics.md_cell(rule), rule_count)
            for rule, rule_count in counter.most_common()
        )
        mark = " **← 重点**" if source == "核心代码" else ""
        add(
            "| %s | %d | %s |"
            % (report_metrics.md_cell(source) + mark, count, detail or "-")
        )
    add("")
    if findings:
        add("### 重点问题示例（每类前 %d 条）" % examples)
        add("")
        grouped = {}
        for row in findings:
            grouped.setdefault(str(row.get("rule") or ""), []).append(row)
        for rule in sorted(grouped, key=lambda item: (-len(grouped[item]), item)):
            items = grouped[rule]
            add(
                "#### %s — %s（共 %d 处）"
                % (
                    report_metrics.md_cell(rule),
                    report_metrics.md_cell(_rule_desc(rule)),
                    len(items),
                )
            )
            add("")
            for row in items[:examples]:
                address = "%s/%s:%s" % (
                    row.get("repo"),
                    row.get("file"),
                    int(row.get("line") or 0),
                )
                add("- %s" % inline_code(address))
                add("  - 代码：%s" % inline_code(row.get("code", "")))
                add("  - 原因：%s" % bullet_sentence(row.get("reason", "")))
                add("  - 建议：%s" % bullet_sentence(row.get("suggestion", "")))
            add("")
        add("> 完整问题清单见下文「问题明细」章节。")
        add("")

    # ---------- 一、评测概览 ----------
    add(_section(1, "评测概览"))
    add("")
    lines.extend(
        _table(
            [
                "仓库",
                "分支",
                "扫描文件",
                "日志语句",
                "问题数",
                "提示",
                "一般",
                "严重",
                "致命",
                "总DI",
                "总评分（10分制）",
                "评估状态",
            ]
        )
    )
    for target in targets:
        counts = target["counts"]
        add(
            "| %s | %s | %d | %d | %d | %d | %d | %d | %d | %s | %s | %s |"
            % (
                report_metrics.md_cell(target["repo"]),
                report_metrics.md_cell(target["display_branch"]),
                target["scanned_files"],
                target["log_statements"],
                len(target["findings"]),
                counts["提示"],
                counts["一般"],
                counts["严重"],
                counts["致命"],
                _fmt_di(target["di"]),
                _fmt_score(target["score"]),
                target["status"],
            )
        )
    add("")
    add("**问题总计 %d 处，覆盖 %d 个目标**" % (len(findings), len(targets)))
    add("")
    add(
        "> 总DI：按严重级加权的问题指数（提示 0.1、一般 1、严重 3、"
        "致命 10）；总评分：10 分制质量分。"
    )

    incomplete = []
    for target in targets:
        if target["meta"].get("complete") is not True:
            if target["repo"] not in incomplete:
                incomplete.append(target["repo"])
    if incomplete:
        add("")
        add(
            "> **完整性告警**：以下目标仍有失败请求，报告仅包含已成功处理的结果，"
            "请结合 `*_meta.json` 中的 `failed_requests` 继续补跑：%s。"
            % "、".join(report_metrics.md_cell(name) for name in incomplete)
        )

    if include_metrics:
        unmapped = [
            item["指标ID"]
            for item in definitions
            if item["指标ID"] in report_metrics.UNMAPPED_METRICS
        ]
        if unmapped:
            add("")
            add(
                "> 说明：%d 项指标中 %s 当前无规则映射，按 10 分计，"
                "不构成实际合规证据。"
                % (
                    len(definitions),
                    "、".join(report_metrics.md_cell(item) for item in unmapped),
                )
            )
    add("")

    # ---------- 二、问题分类统计 ----------
    add(_section(2, "问题分类统计"))
    add("")
    lines.extend(
        _table(
            ["规则大类", "规则编号", "规则", "规则说明", "严重级（规则固定）", "数量"]
        )
    )
    by_rule = Counter(row.get("rule") for row in findings if row.get("rule"))
    for rule, count in sorted(by_rule.items(), key=lambda item: (-item[1], item[0])):
        add(
            "| %s | %s | %s | %s | %s | %d |"
            % (
                report_metrics.md_cell(log_spec.rule_category(rule)),
                report_metrics.md_cell(str(log_spec.rule_id(rule))),
                report_metrics.md_cell(rule),
                report_metrics.md_cell(log_spec.rule_spec(rule)),
                report_metrics.md_cell(log_spec.rule_severity(rule)),
                count,
            )
        )
    add("")
    severity = report_metrics.severity_counts(findings)
    add(
        "严重级分布（按 finding 实际严重级）：%s"
        % "，".join(
            "%s=%d" % (name, severity[name]) for name in report_metrics.SEVERITY_LEVELS
        )
    )
    add("")

    # ---------- 三、量化评估指标（--no-metrics 时跳过） ----------
    number = 2
    if include_metrics:
        number += 1
        add(_section(number, "量化评估指标"))
        add("")
        for target in targets:
            heading = "### %s%s（总评分 %s）" % (
                report_metrics.md_cell(target["repo"]),
                report_metrics.md_cell(_branch_suffix(target["branch"])),
                _fmt_score(target["score"]),
            )
            add(heading)
            add("")
            lines.extend(
                _table(
                    [
                        "指标ID",
                        "特征维度",
                        "层级",
                        "量化指标",
                        "合规率",
                        "得分",
                        "违规数",
                        "样本数",
                        "评估状态",
                    ]
                )
            )
            for definition in definitions:
                result = target["metrics"].get(definition["指标ID"], {})
                add(
                    "| %s | %s | %s | %s | %s | %s | %d | %d | %s |"
                    % (
                        report_metrics.md_cell(definition["指标ID"]),
                        report_metrics.md_cell(definition.get("特征维度", "")),
                        report_metrics.md_cell(definition.get("层级", "")),
                        report_metrics.md_cell(definition.get("量化指标", "")),
                        _fmt_rate(result.get("rate", 0.0)),
                        result.get("score", "-"),
                        int(result.get("violations") or 0),
                        int(result.get("sample") or 0),
                        report_metrics.md_cell(_metrics_status(result)),
                    )
                )
            add("")

    # ---------- 四、问题明细 ----------
    number += 1
    add(_section(number, "问题明细"))
    add("")
    ordered_targets = sorted(
        targets, key=lambda target: (target["repo"], target["branch"])
    )
    index = 0
    for target in ordered_targets:
        rows = sorted(
            target["findings"],
            key=lambda row: (
                SEV_ORDER.get(report_metrics.display_severity(row.get("severity")), 9),
                str(row.get("file") or ""),
                int(row.get("line") or 0),
            ),
        )
        add(
            "### %s%s（%d 处）"
            % (
                report_metrics.md_cell(target["repo"]),
                report_metrics.md_cell(_branch_suffix(target["branch"])),
                len(rows),
            )
        )
        add("")
        lines.extend(
            _table(
                [
                    "#",
                    "严重级",
                    "规则",
                    "代码地址",
                    "代码",
                    "问题原因",
                    "修复建议",
                    "发现时间",
                    "所属分队",
                ]
            )
        )
        for row in rows:
            index += 1
            repo = str(row.get("repo") or target["repo"])
            branch = str(row.get("branch") or target["branch"])
            address = "%s/%s:%s" % (
                repo,
                str(row.get("file") or ""),
                int(row.get("line") or 0),
            )
            add(
                "| %d | %s | %s | <code>%s</code> | <code>%s</code> | %s | %s | %s | %s |"
                % (
                    index,
                    report_metrics.md_cell(
                        report_metrics.display_severity(row.get("severity"))
                    ),
                    report_metrics.md_cell(row.get("rule")),
                    report_metrics.md_code(address),
                    report_metrics.md_code(
                        _truncate_code(row.get("code"), max_code_chars)
                    ),
                    report_metrics.md_cell(row.get("reason", "")),
                    report_metrics.md_cell(row.get("suggestion", "")),
                    report_metrics.md_cell(report_metrics.found_time(row)),
                    report_metrics.md_cell(
                        report_metrics.team_for(repo, branch, team_map)
                    ),
                )
            )
        add("")

    add("---")
    add("")
    add(
        "_本报告由 cann-log-evaluation 依据《CANN 日志规范》自动生成；"
        "问题均可精确定位到代码行，供直接整改。_"
    )
    return "\n".join(lines) + "\n"


def generate_report(
    workdir,
    out=None,
    include_metrics=True,
    max_code_chars=DEFAULT_MAX_CODE_CHARS,
    generated_at=None,
    examples=DEFAULT_EXAMPLE_LIMIT,
):
    """读取评测目录并写出一份完整 Markdown 报告，返回 (路径, findings, metas)。"""
    findings, metas = load_findings(workdir)
    run_date = report_artifacts.run_date_from_manifest(workdir)
    if out is None:
        out = os.path.join(workdir, report_artifacts.markdown_report_name(run_date))
    text = build_report(
        findings,
        metas,
        include_metrics=include_metrics,
        max_code_chars=max_code_chars,
        generated_at=generated_at,
        examples=examples,
    )
    with open(out, "w", encoding="utf-8") as handle:
        handle.write(text)
    return out, findings, metas


def main():
    parser = argparse.ArgumentParser(description="生成完整 Markdown 评测报告（主交付）")
    parser.add_argument("--workdir", required=True)
    parser.add_argument(
        "--out",
        default=None,
        help="默认 <workdir>/cann-log-evaluation-report-<date>.md",
    )
    parser.add_argument(
        "--no-metrics", action="store_true", help="不生成量化评估指标章节"
    )
    parser.add_argument(
        "--max-code-chars",
        type=int,
        default=DEFAULT_MAX_CODE_CHARS,
        help="明细中代码片段最大字符数，0 表示不截断",
    )
    parser.add_argument(
        "--examples",
        type=int,
        default=DEFAULT_EXAMPLE_LIMIT,
        help="管理摘要中每类问题示例条数（默认 5）",
    )
    args = parser.parse_args()

    out, findings, metas = generate_report(
        args.workdir,
        args.out,
        include_metrics=not args.no_metrics,
        max_code_chars=args.max_code_chars,
        examples=args.examples,
    )
    print("[ok] 问题总计 %d 处" % len(findings))
    print("[ok] 覆盖目标 %d 个" % len(metas))
    print("[ok] Markdown 主报告: %s" % out)


if __name__ == "__main__":
    main()
