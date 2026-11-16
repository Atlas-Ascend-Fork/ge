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

"""从 references/log-rules.md 加载日志规范。"""

import argparse
import os
import re
import sys

SPEC_NAME = "CANN 日志规范"
SPEC_VERSION = "md"
MD_PATH = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "references",
    "log-rules.md",
)

RULE_CATEGORIES = frozenset(("可定位性", "有效性", "可读性", "安全合规"))


def _error(message):
    raise SystemExit("[error] 日志规范配置错误: %s" % message)


def _plain(value):
    text = str(value or "").strip().replace("**", "")
    return re.sub(r"`([^`]*)`", r"\1", text).strip()


def _cells(line):
    text = line.strip()
    if not text.startswith("|"):
        return None
    return [_plain(cell) for cell in text.strip("|").split("|")]


def _sections(path):
    if not os.path.isfile(path):
        _error("规则文件不存在: %s" % path)
    try:
        with open(path, "r", encoding="utf-8-sig") as handle:
            lines = handle.read().splitlines()
    except OSError as exc:
        _error("无法读取 %s: %s" % (path, exc))
    result, current = {}, None
    for line in lines:
        match = re.match(r"^##\s+(.+?)\s*$", line)
        if match:
            current = _plain(match.group(1))
            result[current] = []
        elif current is not None:
            result[current].append(line)
    return result


def _table(sections, name):
    lines = sections.get(name)
    if lines is None:
        _error("缺少章节: %s" % name)
    rows = []
    started = False
    for line in lines:
        cells = _cells(line)
        if cells is not None:
            started = True
            rows.append(cells)
        elif started and line.strip():
            break
    if len(rows) < 2:
        _error("章节 %s 缺少 Markdown 表格" % name)
    return rows[0], rows[2:]


def _bullets(sections, name):
    if name not in sections:
        _error("缺少章节: %s" % name)
    return [
        _plain(match.group(1))
        for line in sections[name]
        for match in [re.match(r"^\s*-\s+(.+?)\s*$", line)]
        if match
    ]


def _split_values(value):
    return [
        _plain(item) for item in re.split(r"\s*,\s*|\s*，\s*", value) if _plain(item)
    ]


def _split_scale(value):
    return [
        _plain(item)
        for item in re.split(r"\s*<br\s*/?>\s*", value, flags=re.I)
        if _plain(item)
    ]


def _load(path=MD_PATH):
    sections = _sections(path)

    _header, scan_rows = _table(sections, "扫描范围")
    scan_map = {row[0]: row[1] for row in scan_rows if len(row) >= 2}
    extensions = _split_values(scan_map.get("代码扩展名", ""))
    excluded = _split_values(scan_map.get("排除目录", ""))
    if not extensions:
        _error("扫描范围中的代码扩展名不能为空")

    _header, severity_rows = _table(sections, "严重级")
    severities = []
    for row in severity_rows:
        if len(row) < 6:
            _error("严重级表每行需要 6 列: %r" % row)
        try:
            order = int(row[0])
        except ValueError:
            _error("严重级顺序必须是整数: %r" % row[0])
        severities.append(
            {
                "order": order,
                "name": row[1],
                "meaning": row[2],
                "detail": row[3],
                "typical": row[4],
                "color": row[5],
            }
        )

    _header, rule_rows = _table(sections, "规则")
    rules = []
    scope_map = {
        "单请求": "request",
        "全仓": "repository",
        "request": "request",
        "repository": "repository",
    }
    for row in rule_rows:
        if len(row) < 10:
            _error("规则表每行至少需要 10 列（含规则大类和严重级）: %r" % row)
        try:
            rule_id = int(row[0])
        except ValueError:
            _error("规则编号必须是整数: %r" % row[0])
        category = row[2].strip()
        if category not in RULE_CATEGORIES:
            _error(
                "规则大类必须是 %s: %r" % ("、".join(sorted(RULE_CATEGORIES)), category)
            )
        severity, spec, clause, short = row[3:7]
        scope_value, clause_value, aliases = row[7:10]
        scope = scope_map.get(scope_value)
        if scope is None:
            _error("规则评测范围只能是 单请求 或 全仓: %r" % scope_value)
        if clause_value not in ("是", "否"):
            _error("报告追加条款只能是 是 或 否: %r" % clause_value)
        rules.append(
            {
                "id": rule_id,
                "name": row[1],
                "category": category,
                "severity": severity,
                "spec": spec,
                "clause": clause,
                "short": short,
                "scope": scope,
                "clause_in_short": clause_value == "是",
                "aliases": _split_values(aliases),
            }
        )

    _header, scale_rows = _table(sections, "规则补充尺度")
    scales = {}
    for row in scale_rows:
        if len(row) < 2 or not row[0]:
            _error("规则补充尺度表格式错误: %r" % row)
        if row[0] in scales:
            _error("规则补充尺度重复: %s" % row[0])
        scales[row[0]] = _split_scale(row[1])
    for rule in rules:
        if rule["name"] in scales:
            rule["scale"] = scales.pop(rule["name"])
    if scales:
        _error("规则补充尺度引用了不存在的规则: %s" % ", ".join(sorted(scales)))

    _header, blacklist_rows = _table(sections, "内部敏感词")
    words = [row[0] for row in blacklist_rows if row and row[0]]
    global_scale = _bullets(sections, "全局判定尺度")

    spec = {
        "rules": rules,
        "severities": severities,
        "blacklist": words,
        "global_scale": global_scale,
        "scan": {"extensions": extensions, "exclude_dirs": excluded},
    }
    _validate(spec)
    spec["rules"].sort(key=lambda item: item["id"])
    spec["severities"].sort(key=lambda item: item["order"])
    return spec


def _validate(spec):
    if not spec["rules"]:
        _error("规则表不能为空")
    if not spec["severities"]:
        _error("严重级表不能为空")
    if not spec["global_scale"]:
        _error("全局判定尺度不能为空")
    ids, names = set(), set()
    for rule in spec["rules"]:
        if not all(
            rule.get(name) for name in ("id", "name", "category", "spec", "short")
        ):
            _error("规则存在必填字段为空: %r" % rule)
        if rule["category"] not in RULE_CATEGORIES:
            _error("规则大类无效: %r" % rule)
        if rule["id"] in ids or rule["name"] in names:
            _error("规则编号或名称重复: %r" % rule)
        ids.add(rule["id"])
        names.add(rule["name"])
    severity_names = [item["name"] for item in spec["severities"]]
    severity_orders = [item["order"] for item in spec["severities"]]
    if len(severity_names) != len(set(severity_names)):
        _error("严重级名称重复")
    if len(severity_orders) != len(set(severity_orders)):
        _error("严重级顺序重复")
    for rule in spec["rules"]:
        if rule["severity"] not in severity_names:
            _error("规则严重级不存在: %s/%s" % (rule["name"], rule["severity"]))


SPEC = _load()
SPEC_SOURCES = [os.path.abspath(MD_PATH)]
RULES = SPEC["rules"]
SEVERITIES = SPEC["severities"]
BLACKLIST = SPEC["blacklist"]


def rule_names():
    return tuple(rule["name"] for rule in RULES)


def rule_severity(name):
    """返回规则声明的固定严重级。"""
    normalized = normalize_rule(name)
    for rule in RULES:
        if rule["name"] == normalized:
            return rule["severity"]
    raise KeyError(normalized)


def rule_category(name):
    """返回规则所属的大类。"""
    normalized = normalize_rule(name)
    for rule in RULES:
        if rule["name"] == normalized:
            return rule["category"]
    return ""


def rule_id(name):
    """返回规则编号，供报告和问题跟踪引用。"""
    normalized = normalize_rule(name)
    for rule in RULES:
        if rule["name"] == normalized:
            return rule["id"]
    return ""


def rule_spec(name):
    """返回规则说明。"""
    normalized = normalize_rule(name)
    for rule in RULES:
        if rule["name"] == normalized:
            return rule["spec"]
    return str(name or "")


def repository_rule_names():
    return tuple(rule["name"] for rule in RULES if rule.get("scope") == "repository")


def severities():
    return tuple(item["name"] for item in SEVERITIES)


def scan_extensions():
    return tuple(item.lower() for item in SPEC["scan"]["extensions"])


def exclude_dirs():
    return tuple(SPEC["scan"]["exclude_dirs"])


def alias_map():
    return {alias: rule["name"] for rule in RULES for alias in rule.get("aliases", [])}


def normalize_rule(name):
    value = str(name or "").strip()
    return alias_map().get(value, value)


def _short(rule):
    text = rule["short"]
    if rule.get("clause") and rule.get("clause_in_short", True):
        text += "(%s)" % rule["clause"]
    return text


def rule_desc():
    output = {}
    for rule in RULES:
        output[rule["name"]] = _short(rule)
        for alias in rule.get("aliases", []):
            output[alias] = _short(rule)
    return output


def sev_order():
    return {item["name"]: item["order"] for item in SEVERITIES}


def sev_colors():
    return {item["name"]: item["color"] for item in SEVERITIES if item.get("color")}


def blacklist(extra_env=None):
    return list(BLACKLIST)


def prompt_rules(words=None, names=None):
    selected = set(names) if names is not None else None
    values = words if words is not None else blacklist()
    output = []
    for rule in RULES:
        if selected is not None and rule["name"] not in selected:
            continue
        text = rule["spec"].replace("{blacklist}", ", ".join(values))
        output.append(
            "%d. [%s] %s（固定严重级：%s）：%s"
            % (rule["id"], rule["category"], rule["name"], rule["severity"], text)
        )
    return "\n".join(output)


def prompt_scale(names=None):
    selected = set(names) if names is not None else None
    output = ["- " + line for line in SPEC["global_scale"]]
    for rule in RULES:
        if selected is not None and rule["name"] not in selected:
            continue
        scale = rule.get("scale") or []
        if not scale:
            continue
        output.append("- 关于规则%d %s：%s" % (rule["id"], rule["name"], scale[0]))
        output.extend("  %s" % line for line in scale[1:])
    return "\n".join(output)


def prompt_severities():
    output = []
    for item in SEVERITIES:
        detail = "（%s）" % item["detail"] if item.get("detail") else ""
        output.append("- %s：%s%s。" % (item["name"], item["meaning"], detail))
    return "\n".join(output)


def prompt_severity_meanings(indent="  "):
    return "\n".join(
        "%s%s：%s" % (indent, item["name"], item["meaning"]) for item in SEVERITIES
    )


def rule_name_list():
    return "/".join(rule_names())


def selftest():
    _validate(SPEC)
    assert all(name in prompt_rules() for name in rule_names())
    assert all(name in prompt_severities() for name in severities())
    print("[ok] Markdown 规范合法: %s" % SPEC_SOURCES[0])
    print(
        "[ok] 规则=%d 严重级=%d 黑名单=%d"
        % (len(RULES), len(SEVERITIES), len(blacklist()))
    )
    return 0


def main():
    parser = argparse.ArgumentParser(description="查看或校验当前日志规范")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.check or args.selftest:
        return selftest()
    print("%s" % SPEC_NAME)
    print("来源: %s" % SPEC_SOURCES[0])
    print(prompt_rules())
    print("\n严重级:\n%s" % prompt_severities())
    return 0


if __name__ == "__main__":
    sys.exit(main())
