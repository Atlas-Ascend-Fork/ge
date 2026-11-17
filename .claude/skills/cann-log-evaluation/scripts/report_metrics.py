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

"""CANN 日志评测的量化指标、评分、分队与 Markdown 转义公共模块。

原先这些逻辑只存在于 ``gen_excel_report.py``。Markdown 成为主交付报告后，
两种报告必须给出完全一致的数值，因此在此抽取成唯一实现：

- 27 项量化指标解析（``references/quantitative-metrics.md``）
- 10 分制单项打分、一票否决、总评分（各指标得分均值）
- 展示严重级计数与 DI 加权求和
- 仓库到所属分队的映射（支持 ``cann/`` 前缀与通配符）
- 发现时间兜底取值
- Markdown 表格转义（``md_cell`` / ``md_code``）

本模块不依赖任何第三方库，可被 Excel / Markdown 报告共同复用。
"""

import fnmatch
import json
import os
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import log_spec  # noqa: E402  规范唯一事实源，增删规则改那里

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_TEAM_MAP_PATH = os.path.join(_ROOT, "references", "repo-team-map.json")
METRICS_PATH = os.path.join(_ROOT, "references", "quantitative-metrics.md")

# 报告展示用的四个严重级（与 log-rules.md 的严重级表一致）。
SEVERITY_LEVELS = ("提示", "一般", "严重", "致命")
SEVERITY_DISPLAY = {
    "提示": "提示",
    "一般": "一般",
    "严重": "严重",
    "致命": "致命",
}
DI_WEIGHTS = {"提示": 0.1, "一般": 1.0, "严重": 3.0, "致命": 10.0}
LANGUAGE_RULES = ("禁止中文", "拼写错误", "语法错误")

# 指标 ID -> 能作为该指标证据的规则名（可多对一：多条规则命中同一指标）。
# 未在此表出现的指标当前没有规则映射，按 10 分计，不代表合规证据。
METRIC_RULES = {
    "LOG-E-01": ("错误描述不完整", "失败日志缺少错误描述"),
    "LOG-E-02": ("越限错误信息不完整",),
    "LOG-E-03": ("配置错误信息不完整",),
    "LOG-E-04": ("参数比较信息不完整",),
    "LOG-E-05": ("ERROR关键定位信息缺失", "ERROR文件信息缺失"),
    "LOG-C-01": ("冗余日志", "相同日志文案重复"),
    "LOG-C-02": ("重试循环重复打印底层错误",),
    "LOG-L-04": ("重试循环重复打印底层错误",),
    "LOG-L-05": ("循环体内打印高频日志",),
    "LOG-V-01": ("级别错配",),
    "LOG-V-02": ("非业务错误滥用ERROR",),
    "LOG-V-03": ("算子日志级别与场景不匹配",),
    "LOG-V-04": ("失败日志缺少错误描述", "ERROR关键定位信息缺失", "ERROR文件信息缺失"),
    "LOG-S-01": ("敏感信息", "内部敏感词"),
    "LOG-S-02": ("缺少度量单位",),
    "LOG-S-03": ("禁止中文", "拼写错误", "语法错误"),
    "LOG-S-04": ("魔鬼数字",),
    "LOG-S-06": ("日志长度或换行违规",),
}

_REQUIRED_METRIC_COLUMNS = {
    "指标ID",
    "评估对象",
    "特征维度",
    "层级",
    "量化指标",
    "量化公式",
    "打分规则",
}


def load_quantitative_metrics(path=None):
    """解析 ``## 量化评估指标`` 表格，返回 27 项指标定义（按文档顺序）。"""
    sections = log_spec._sections(path or METRICS_PATH)
    headers, rows = log_spec._table(sections, "量化评估指标")
    if not _REQUIRED_METRIC_COLUMNS.issubset(headers):
        raise ValueError("量化指标表缺少必需列")
    metrics = [dict(zip(headers, row)) for row in rows if row and row[0]]
    ids = [metric["指标ID"] for metric in metrics]
    if len(ids) != len(set(ids)) or not metrics:
        raise ValueError("量化指标ID重复或指标表为空")
    return metrics


def metric_ids(definitions=None):
    """返回指标 ID 列表（默认读取 27 项指标定义）。"""
    return [
        item["指标ID"]
        for item in (
            definitions if definitions is not None else load_quantitative_metrics()
        )
    ]


def unmapped_metrics(definitions=None):
    """当前没有任何规则映射的指标 ID 集合。"""
    return frozenset(
        item for item in metric_ids(definitions) if item not in METRIC_RULES
    )


#: 27 项指标中未在 ``METRIC_RULES`` 里映射的 ID（展示时按 10 分计）。
UNMAPPED_METRICS = unmapped_metrics()


def finding_key(row):
    """同一日志位置的去重键：优先 (文件, 起始行, 结束行)，否则用日志 ID。"""
    file_name = str(row.get("file") or "")
    line = int(row.get("line") or 0)
    if file_name and line:
        return (file_name, line, int(row.get("end") or line))
    return ("log_id", str(row.get("log_id") or id(row)))


def metric_score(metric_id, violations, total, rule_count):
    """将单个量化指标统一折算成 10 分制；公式与 Excel 报告完全一致。"""
    if metric_id == "LOG-S-01":
        return 0 if violations else 10
    if metric_id == "LOG-S-03":
        return max(0, 10 - rule_count)
    if not violations:
        return 10
    rate = 100.0 * max(0, total - violations) / max(1, total)
    if metric_id in {"LOG-E-02", "LOG-E-05", "LOG-C-03", "LOG-C-04", "LOG-S-04"}:
        if rate >= 100:
            return 10
        if rate >= 80:
            return 8
        if rate >= 60:
            return 6
        if rate >= 40:
            return 4
        if rate > 0:
            return 2
        return 0
    if metric_id in {"LOG-E-03", "LOG-E-04", "LOG-L-01", "LOG-L-02", "LOG-L-04"}:
        return max(0, 10 - 2 * violations)
    if metric_id == "LOG-L-03":
        return max(0, 10 - violations)
    if metric_id in {"LOG-S-02", "LOG-V-03"}:
        return 10 if rate >= 95 else max(0, 10 - int((99 - rate) // 5))
    if metric_id == "LOG-V-02":
        return max(0, 10 - int((100 - rate + 1) // 2))
    return max(0, 10 - min(10, violations))


def metric_results(meta, findings, definitions):
    """计算单个目标的 27 项指标结果。

    返回 ``{指标ID: {rate, score, violations, sample, status, collected,
    explanation}}``；``collected`` 仅用于展示（无规则映射的指标为 False），
    不改变 score / status / rate / violations / sample 的取值。
    """
    total = int(meta.get("log_statements") or 0)
    by_rule = defaultdict(set)
    for row in findings:
        by_rule[log_spec.normalize_rule(row.get("rule"))].add(finding_key(row))

    status = "已评估" if meta.get("complete") is True else "部分评估"
    results = {}
    for definition in definitions:
        metric_id = definition["指标ID"]
        rules = METRIC_RULES.get(metric_id, ())
        values = {name: by_rule[name] for name in rules}
        violation_keys = set().union(*values.values()) if values else set()
        violations = len(violation_keys)
        rate = 1.0 if total <= 0 else round(max(0.0, 1 - violations / total), 6)
        score = metric_score(
            metric_id, violations, total, sum(bool(items) for items in values.values())
        )
        metric_status = "一票否决" if metric_id == "LOG-S-01" and violations else status
        results[metric_id] = {
            "rate": rate,
            "score": score,
            "violations": violations,
            "sample": total,
            "status": metric_status,
            "collected": metric_id in METRIC_RULES,
            "explanation": (
                "无适用问题，按 10 分计"
                if not violations
                else "问题数来自 findings，按唯一日志位置去重"
            ),
        }
    return results


def target_metrics(findings, metas, definitions):
    """按 (仓库, 分支) 汇总每个目标的 27 项指标结果。"""
    by_target = defaultdict(list)
    for row in findings:
        by_target[(str(row.get("repo") or ""), str(row.get("branch") or ""))].append(
            row
        )
    return {
        (str(meta.get("repo") or ""), str(meta.get("branch") or "")): metric_results(
            meta,
            by_target[(str(meta.get("repo") or ""), str(meta.get("branch") or ""))],
            definitions,
        )
        for meta in metas
    }


def total_score(results):
    """总评分 = 全部指标得分的算术平均（10 分制，保留一位小数）。

    ``LOG-S-01`` 触发一票否决时总评分为 0；无任何得分时返回 None。
    """
    _tmp_result = []
    for item in results.values():
        if item.get("score") is not None:
            _tmp_result.append(item["score"])
    assessed = _tmp_result
    if not assessed:
        return None
    if results.get("LOG-S-01", {}).get("status") == "一票否决":
        return 0
    return round(sum(assessed) / len(assessed), 1)


def display_severity(value):
    """归一化展示严重级（未知值原样返回）。"""
    text = str(value or "").strip()
    return SEVERITY_DISPLAY.get(text, text)


def severity_counts(findings):
    """按展示严重级统计问题数，四个严重级始终存在（无问题时为 0）。"""
    counts = Counter({name: 0 for name in SEVERITY_LEVELS})
    for row in findings:
        counts[display_severity(row.get("severity"))] += 1
    return counts


def di_total(counts):
    """DI 加权总数：提示 0.1 / 一般 1 / 严重 3 / 致命 10。"""
    return sum(counts.get(name, 0) * DI_WEIGHTS[name] for name in SEVERITY_LEVELS)


def load_team_map(path=None):
    """读取默认分队映射，并用调用方提供的映射覆盖/追加。"""
    mapping = {}
    for source in (DEFAULT_TEAM_MAP_PATH, path):
        if not source:
            continue
        with open(source, "r", encoding="utf-8-sig") as handle:
            value = json.load(handle)
        if not isinstance(value, dict) or any(
            not isinstance(key, str) for key in value
        ):
            raise ValueError('团队映射必须是 JSON 对象，例如 {"runtime": "分队A"}')
        mapping.update({key: str(team or "") for key, team in value.items()})
    return mapping


def team_for(repo, branch, team_map):
    """查询仓库所属分队：先去 ``cann/`` 前缀，再精确匹配，最后通配符匹配。"""
    normalized = str(repo or "").strip()
    if normalized.startswith("cann/"):
        normalized = normalized[slice(len("cann/"), None, None)]
    exact = team_map.get("%s@%s" % (normalized, branch), team_map.get(normalized))
    if exact:
        return exact
    for pattern, team in team_map.items():
        if any(char in pattern for char in "*?[") and fnmatch.fnmatch(
            normalized, pattern
        ):
            return team
    return "其他"


def found_time(row):
    """取问题的发现时间，兼容多种历史字段名。"""
    return str(
        row.get("found_time")
        or row.get("discovered_at")
        or row.get("detected_at")
        or row.get("created_at")
        or row.get("_source_found_time")
        or ""
    )


def md_cell(text):
    """转义 Markdown 表格单元格：换行、竖线与 HTML 实体。

    - CRLF / CR 先归一为 ``\\n``，再转成 ``<br>``，避免破坏表格行结构
    - ``|`` 转义为 ``\\|``
    - ``&`` ``<`` ``>`` 转 HTML 实体，避免被当作标签解析
    """
    if text is None:
        return ""
    value = str(text).replace("\r\n", "\n").replace("\r", "\n")
    value = value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    value = value.replace("|", "\\|")
    return value.replace("\n", "<br>")


def md_code(text):
    """``md_cell`` 的 ``<code>`` 变体：额外把反引号转成实体，代码可安全成段。"""
    return md_cell(text).replace("`", "&#96;")
