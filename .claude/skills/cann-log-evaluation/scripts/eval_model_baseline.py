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

"""用参考 Excel 中的真实日志位置复测模型，并生成三仓对比报告。"""

import argparse
import hashlib
import json
import os
import sys
import threading
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eval_baseline as baseline  # noqa: E402
import gen_excel_report  # noqa: E402
import llm_evaluate as evaluator  # noqa: E402
import log_spec  # noqa: E402

RULES = tuple(log_spec.rule_names())
SEVERITIES = tuple(log_spec.severities())
BASELINE_EVALUATOR_VERSION = "3"

PROMPT = """你是 CANN 日志规范评测专家。下面每项给出参考评测集中的一个焦点源码位置及
真实源码上下文。参考答案没有提供给你。请只判断 focal_line 开始的焦点语句：

1. is_log 表示该语句是否实际承担运行时日志、状态或调试输出。注释、logger 初始化、
   数学 log/logits、普通字符串常量和不产生输出的代码不是日志。
2. 如果是日志，依据下列外部规范返回该焦点语句的全部问题；没有问题返回空 issues。
3. 不要评测上下文中的其它语句，不要猜测上下文外的信息，宁缺毋滥。

【规范】
{rules}

【判定尺度】
{scale}

【严重级】
{severities}

严格只输出一个 JSON 对象，不要解释文字：
{{"items":[{{"id":"输入id","is_log":true,"issues":[{{"rule":"{rule_names}",
"severity":"{severity_names}","reason":"具体问题","suggestion":"修复建议"}}]}}]}}

items 必须覆盖每个输入 id 且各出现一次。issues 只允许使用列出的规则和严重级。

【待复测位置】共 {count} 项
{items}
"""


def found_time_now():
    return datetime.now().astimezone().isoformat(timespec="milliseconds")


def render_prompt(items):
    payload = [
        {
            "id": item["id"],
            "repo": item["repo"],
            "file": item["file"],
            "source_type": item["source_type"],
            "focal_line": item["line"],
            "context": item["context"],
        }
        for item in items
    ]
    return PROMPT.format(
        rules=log_spec.prompt_rules(evaluator.blacklist()),
        scale=log_spec.prompt_scale(),
        severities=log_spec.prompt_severities(),
        rule_names="/".join(RULES),
        severity_names="/".join(SEVERITIES),
        count=len(payload),
        items=json.dumps(payload, ensure_ascii=False),
    )


def pack_items(items, max_input_chars):
    groups, current = [], []
    for item in items:
        if max_input_chars is not None and len(render_prompt([item])) > max_input_chars:
            raise ValueError(
                "单个基准位置超过输入上限: %s:%d" % (item["file"], item["line"])
            )
        if (
            current
            and max_input_chars is not None
            and len(render_prompt(current + [item])) > max_input_chars
        ):
            groups.append(current)
            current = [item]
        else:
            current.append(item)
    if current:
        groups.append(current)
    return groups


def parse_response(text, expected_ids):
    value = (text or "").strip()
    if value.startswith("```"):
        parts = value.split("```")
        value = parts[1] if len(parts) > 1 else value
        if value.lstrip().startswith("json"):
            value = value.lstrip()[4:]
    start, end = value.find("{"), value.rfind("}")
    if start < 0 or end < start:
        raise ValueError("响应不是 JSON 对象")
    result = json.loads(value[slice(start, end + 1, None)])
    rows = result.get("items") if isinstance(result, dict) else None
    if not isinstance(rows, list):
        raise ValueError("响应缺少 items 数组")
    by_id = {}
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("items 中存在非对象")
        item_id = str(row.get("id", ""))
        if item_id not in expected_ids or item_id in by_id:
            raise ValueError("items.id 非法或重复: %r" % item_id)
        if not isinstance(row.get("is_log"), bool):
            raise ValueError("items.is_log 必须是布尔值: %s" % item_id)
        issues = row.get("issues")
        if not isinstance(issues, list):
            raise ValueError("items.issues 必须是数组: %s" % item_id)
        seen_rules = set()
        for issue in issues:
            if not isinstance(issue, dict):
                raise ValueError("issues 中存在非对象: %s" % item_id)
            rule = log_spec.normalize_rule(str(issue.get("rule", "")).strip())
            severity = str(issue.get("severity", "")).strip()
            if rule not in RULES or severity not in SEVERITIES:
                raise ValueError("issues 规则或严重级非法: %s/%s" % (rule, severity))
            if rule in seen_rules:
                raise ValueError("同一位置重复返回规则: %s/%s" % (item_id, rule))
            if (
                not str(issue.get("reason", "")).strip()
                or not str(issue.get("suggestion", "")).strip()
            ):
                raise ValueError("issues 缺少原因或建议: %s/%s" % (item_id, rule))
            issue["rule"], issue["severity"] = rule, severity
            seen_rules.add(rule)
        if not row["is_log"] and issues:
            raise ValueError("非日志位置不能返回问题: %s" % item_id)
        by_id[item_id] = row
    if set(by_id) != set(expected_ids):
        raise ValueError("items 未完整覆盖输入 id")
    return by_id


def prepare_items(excel_path, input_root, repos, context_lines):
    rows = baseline.load_details(excel_path)
    items, changed = {}, []
    for index, row in enumerate(rows, 1):
        repo = baseline.normalize_repo(row.get("仓库"))
        if repo not in repos:
            continue
        rel = baseline.relative_source(row.get("文件"), repo)
        path = input_root / (repo + "@master") / rel
        if not path.exists():
            changed.append((repo, rel, int(row.get("行号") or 0), "源码不存在"))
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        line = baseline.locate(text, int(row.get("行号") or 0), row.get("代码"))
        if line is None:
            changed.append((repo, rel, int(row.get("行号") or 0), "源码已变化"))
            continue
        source_lines = text.splitlines()
        start = max(1, line - context_lines)
        end = min(len(source_lines), line + context_lines)
        context = "\n".join(
            "%d: %s" % (number, source_lines[number - 1])
            for number in range(start, end + 1)
        )
        key = (repo, rel, line)
        if key not in items:
            items[key] = {
                "id": "B%04d" % index,
                "repo": repo,
                "file": rel,
                "line": line,
                "code": source_lines[line - 1].strip(),
                "context": context,
                "source_type": evaluator.classify_source(rel),
                "expected": {},
            }
        rule = log_spec.normalize_rule(str(row.get("规则") or "").strip())
        items[key]["expected"][rule] = str(row.get("严重级") or "").strip()
    return sorted(
        items.values(), key=lambda x: (x["repo"], x["file"], x["line"])
    ), changed


def run_request(group, api, key, model, max_tokens, timeout, retries):
    started = time.time()
    try:
        content, usage = evaluator.call_llm(
            api, key, model, render_prompt(group), max_tokens, timeout, retries
        )
        result = parse_response(content, {item["id"] for item in group})
        return group, result, None, usage, round(time.time() - started, 3)
    except Exception as exc:  # noqa: BLE001
        return (
            group,
            {},
            "%s: %s" % (type(exc).__name__, exc),
            {},
            round(time.time() - started, 3),
        )


def write_json_results(out, items, predictions, model, complete, request_stats):
    for repo in sorted({item["repo"] for item in items}):
        repo_items = [item for item in items if item["repo"] == repo]
        repo_ids = {item["id"] for item in repo_items}
        logs, findings = [], []
        for item in repo_items:
            prediction = predictions.get(item["id"])
            if not prediction or not prediction["is_log"]:
                continue
            log_id = item["id"]
            logs.append(
                {
                    "repo": repo,
                    "branch": "master",
                    "log_id": log_id,
                    "global_id": "%s#%s" % (item["file"], log_id),
                    "file": item["file"],
                    "line": item["line"],
                    "end": item["line"],
                    "code": item["code"],
                    "source_type": item["source_type"],
                }
            )
            for issue in prediction["issues"]:
                findings.append(
                    {
                        "repo": repo,
                        "branch": "master",
                        "log_id": log_id,
                        "file": item["file"],
                        "line": item["line"],
                        "end": item["line"],
                        "code": item["code"],
                        "rule": issue["rule"],
                        "severity": issue["severity"],
                        "reason": str(issue["reason"]),
                        "suggestion": str(issue["suggestion"]),
                        "source_type": item["source_type"],
                        "found_time": str(prediction.get("found_time") or ""),
                    }
                )
        for suffix, rows in (("logs", logs), ("findings", findings)):
            with (out / (repo + "_" + suffix + ".jsonl")).open(
                "w", encoding="utf-8"
            ) as fh:
                for row in rows:
                    fh.write(json.dumps(row, ensure_ascii=False) + "\n")
        by_rule = Counter(row["rule"] for row in findings)
        by_severity = Counter(row["severity"] for row in findings)
        meta = {
            "repo": repo,
            "branch": "master",
            "scanned_files": len({item["file"] for item in repo_items}),
            "benchmark_items": len(repo_items),
            "log_statements": len(logs),
            "findings": len(findings),
            "by_rule": dict(by_rule),
            "by_severity": dict(by_severity),
            "blacklist": evaluator.blacklist(),
            "model": model,
            "complete": complete,
            "llm_errors": 0 if complete else 1,
            "input_scope": "baseline_focal_statements",
            "request_stats": [
                row for row in request_stats if repo_ids.intersection(row["ids"])
            ],
        }
        (out / (repo + "_meta.json")).write_text(
            json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8"
        )


def comparison_rows(items, predictions):
    summary, rules = [], Counter()
    for repo in sorted({item["repo"] for item in items}):
        repo_items = [item for item in items if item["repo"] == repo]
        expected = sum(len(item["expected"]) for item in repo_items)
        log_hits = sum(
            len(item["expected"])
            for item in repo_items
            if predictions.get(item["id"], {}).get("is_log")
        )
        location_hits = sum(
            len(item["expected"])
            for item in repo_items
            if predictions.get(item["id"], {}).get("issues")
        )
        rule_hits = severity_hits = predicted = 0
        for item in repo_items:
            got = {
                issue["rule"]: issue["severity"]
                for issue in predictions.get(item["id"], {}).get("issues", [])
            }
            predicted += len(got)
            for rule, severity in item["expected"].items():
                hit = int(rule in got)
                severity_hit = int(got.get(rule) == severity)
                rule_hits += hit
                severity_hits += severity_hit
                rules[(repo, rule, "expected")] += 1
                rules[(repo, rule, "rule_hit")] += hit
                rules[(repo, rule, "severity_hit")] += severity_hit
        summary.append(
            {
                "repo": repo,
                "items": len(repo_items),
                "expected": expected,
                "log_hits": log_hits,
                "location_hits": location_hits,
                "rule_hits": rule_hits,
                "severity_hits": severity_hits,
                "predicted": predicted,
                "precision": percent(rule_hits, predicted),
                "f1": (200.0 * rule_hits / max(1, expected + predicted)),
                "matched_severity": percent(severity_hits, rule_hits),
            }
        )
    rule_rows = []
    for repo, rule in sorted({(key[0], key[1]) for key in rules}):
        rule_rows.append(
            {
                "repo": repo,
                "rule": rule,
                "expected": rules[(repo, rule, "expected")],
                "rule_hits": rules[(repo, rule, "rule_hit")],
                "severity_hits": rules[(repo, rule, "severity_hit")],
            }
        )
    return summary, rule_rows


def percent(hit, total):
    return 100.0 * hit / max(1, total)


def write_comparison(out, items, predictions, changed, model):
    summary, rule_rows = comparison_rows(items, predictions)
    comparable_labels = sum(len(item["expected"]) for item in items)
    lines = [
        "# GLM-5.2 三仓日志规范基准对比",
        "",
        "> 模型: `%s`；基准: `docs/requirements/cann-log-evaluation-report.xlsx`"
        % model,
        "",
        "## 方法说明",
        "",
        "- 本报告是基准焦点位置复测，不是全仓问题总量报告。模型只看到真实源码位置及上下文，"
        "未看到参考规则或严重级答案。",
        "- 参考表在所选仓库中共 %d 条标注；当前源码可重新定位 %d 条、对应 %d 个唯一位置。"
        "另 %d 条因源码版本变化排除，不计入分母。"
        % (
            comparable_labels + len(changed),
            comparable_labels,
            len(items),
            len(changed),
        ),
        "- 规则召回率 = 规则命中 / 可比标注；相对基准精确率 = 规则命中 / 模型报出问题。"
        "参考表未标注不等于一定错误，因此额外发现仍需人工复核。",
        "- 严重级一致率以全部可比标注为分母，既包含规则漏报，也包含严重级不一致。",
        "",
        "## 总览",
        "",
        "| 仓库 | 可比标注 | 日志识别 | 问题位置 | 规则召回 | 相对基准精确率 | F1 | 严重级一致 | 命中后严重级一致 | 模型报出问题 |",
        "|------|---------:|---------:|---------:|---------:|-----------------:|---:|"
        "-----------:|-----------------:|-------------:|",
    ]
    for row in summary:
        total = row["expected"]
        lines.append(
            "| {repo} | {expected} | {log_hits} ({log_pct:.1f}%) | "
            "{location_hits} ({location_pct:.1f}%) | {rule_hits} ({rule_pct:.1f}%) | "
            "{precision:.1f}% | {f1:.1f}% | {severity_hits} ({severity_pct:.1f}%) | "
            "{matched_severity:.1f}% | {predicted} |".format(
                log_pct=percent(row["log_hits"], total),
                location_pct=percent(row["location_hits"], total),
                rule_pct=percent(row["rule_hits"], total),
                severity_pct=percent(row["severity_hits"], total),
                **row,
            )
        )
    total = {
        key: sum(row[key] for row in summary)
        for key in (
            "expected",
            "log_hits",
            "location_hits",
            "rule_hits",
            "severity_hits",
            "predicted",
        )
    }
    total["precision"] = percent(total["rule_hits"], total["predicted"])
    total["f1"] = (
        200.0 * total["rule_hits"] / max(1, total["expected"] + total["predicted"])
    )
    total["matched_severity"] = percent(total["severity_hits"], total["rule_hits"])
    lines.append(
        "| 合计 | {expected} | {log_hits} ({log_pct:.1f}%) | "
        "{location_hits} ({location_pct:.1f}%) | {rule_hits} ({rule_pct:.1f}%) | "
        "{precision:.1f}% | {f1:.1f}% | {severity_hits} ({severity_pct:.1f}%) | "
        "{matched_severity:.1f}% | {predicted} |".format(
            log_pct=percent(total["log_hits"], total["expected"]),
            location_pct=percent(total["location_hits"], total["expected"]),
            rule_pct=percent(total["rule_hits"], total["expected"]),
            severity_pct=percent(total["severity_hits"], total["expected"]),
            **total,
        )
    )
    lines.extend(
        [
            "",
            "## 按规则",
            "",
            "| 仓库 | 规则 | 标注 | 规则命中 | 严重级一致 |",
            "|------|------|-----:|---------:|-----------:|",
        ]
    )
    for row in rule_rows:
        lines.append(
            "| {repo} | {rule} | {expected} | {rule_hits} ({rule_pct:.1f}%) | "
            "{severity_hits} ({severity_pct:.1f}%) |".format(
                rule_pct=percent(row["rule_hits"], row["expected"]),
                severity_pct=percent(row["severity_hits"], row["expected"]),
                **row,
            )
        )
    lines.extend(["", "## 未命中", ""])
    for item in items:
        _tmp_result = set()
        for issue in predictions.get(item["id"], {}).get("issues", []):
            _tmp_result.add(issue["rule"])
        got = _tmp_result
        for rule in item["expected"]:
            if rule not in got:
                lines.append(
                    "- `%s/%s:%d`：期望 `%s`，模型未命中"
                    % (item["repo"], item["file"], item["line"], rule)
                )
    lines.extend(
        [
            "",
            "## 额外发现",
            "",
            "以下问题未出现在参考 Excel 的同位置标注中，可能是真实新增发现或误报，"
            "需人工复核；相对基准精确率将其暂按未命中计算。",
            "",
        ]
    )
    for item in items:
        expected_rules = set(item["expected"])
        for issue in predictions.get(item["id"], {}).get("issues", []):
            if issue["rule"] not in expected_rules:
                lines.append(
                    "- `%s/%s:%d`：模型额外报告 `%s/%s`，%s"
                    % (
                        item["repo"],
                        item["file"],
                        item["line"],
                        issue["rule"],
                        issue["severity"],
                        issue["reason"],
                    )
                )
    lines.extend(
        [
            "",
            "## 不可比项",
            "",
            "以下 %d 条因当前源码与基准版本不同，不计入命中率。" % len(changed),
            "",
        ]
    )
    for repo, file_name, line, reason in changed:
        lines.append("- `%s/%s:%d`：%s" % (repo, file_name, line, reason))
    (out / "baseline-comparison.md").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )
    findings, metas = gen_excel_report.load(str(out))
    gen_excel_report.create_report(findings, metas, out / "baseline-comparison.xlsx")


def main():
    skill_root = Path(__file__).resolve().parents[1]
    workspace = Path(__file__).resolve().parents[4]
    parser = argparse.ArgumentParser(description="用参考 Excel 复测模型并输出对比")
    parser.add_argument(
        "--baseline",
        default=workspace / "docs" / "requirements" / "cann-log-evaluation-report.xlsx",
    )
    parser.add_argument("--input", default=skill_root / "input")
    parser.add_argument(
        "--out", default=skill_root / "output" / "glm-5.2-baseline" / "results"
    )
    parser.add_argument("--repos", default="amct,oam-tools,runtime")
    parser.add_argument("--context-lines", type=int, default=4)
    parser.add_argument(
        "--max-input-chars",
        type=int,
        default=evaluator._env_int(
            "LLM_MAX_INPUT_CHARS", evaluator.DEFAULT_MAX_INPUT_CHARS
        ),
        help="输入硬上限；默认1000000字符",
    )
    parser.add_argument(
        "--max-output-tokens",
        type=int,
        default=evaluator._env_int("LLM_MAX_TOKENS"),
        help="可选输出上限；未设置时不发送 max_tokens",
    )
    parser.add_argument(
        "--workers", type=int, default=int(os.environ.get("LLM_WORKERS", "1"))
    )
    parser.add_argument(
        "--timeout", type=int, default=int(os.environ.get("LLM_TIMEOUT", "600"))
    )
    parser.add_argument(
        "--retries", type=int, default=int(os.environ.get("LLM_RETRIES", "1"))
    )
    parser.add_argument("--restart", action="store_true")
    args = parser.parse_args()

    api = os.environ.get("LLM_API_BASE", "").strip()
    key = os.environ.get("LLM_API_KEY", "").strip()
    model = os.environ.get("LLM_MODEL", "").strip()
    if not api or not key or not model:
        print("[error] 请设置 LLM_API_BASE、LLM_API_KEY 和 LLM_MODEL", file=sys.stderr)
        return 2
    if (args.max_input_chars is not None and args.max_input_chars <= 0) or (
        args.max_output_tokens is not None and args.max_output_tokens <= 0
    ):
        print("[error] 显式输入/输出硬上限必须大于0", file=sys.stderr)
        return 2
    repos = tuple(item.strip() for item in args.repos.split(",") if item.strip())
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    items, changed = prepare_items(
        Path(args.baseline).resolve(),
        Path(args.input).resolve(),
        set(repos),
        args.context_lines,
    )
    all_groups = pack_items(items, args.max_input_chars)
    fingerprint = hashlib.sha256(
        "\n".join(
            [
                BASELINE_EVALUATOR_VERSION,
                model,
                PROMPT,
                log_spec.prompt_rules(evaluator.blacklist()),
                log_spec.prompt_scale(),
                "context=%d" % args.context_lines,
                "max_input_chars=%s"
                % (
                    args.max_input_chars
                    if args.max_input_chars is not None
                    else "unset"
                ),
                "max_tokens=%s"
                % (
                    args.max_output_tokens
                    if args.max_output_tokens is not None
                    else "unset"
                ),
            ]
        ).encode("utf-8")
    ).hexdigest()[:16]

    progress_path = out / "baseline_progress.jsonl"
    predictions, done = {}, set()
    if progress_path.exists() and not args.restart:
        for raw in progress_path.read_text(encoding="utf-8-sig").splitlines():
            try:
                row = json.loads(raw)
            except json.JSONDecodeError:
                continue
            if row.get("fingerprint") != fingerprint:
                continue
            predictions.update(row.get("predictions") or {})
            done.update(row.get("ids") or [])
    missing_items = [item for item in items if item["id"] not in done]
    todo = pack_items(missing_items, args.max_input_chars) if missing_items else []
    print(
        "[info] 可比位置=%d 已完成=%d 源码变化=%d 请求=%d(待跑%d) 模型=%s"
        % (
            len(items),
            len(predictions),
            len(changed),
            len(all_groups),
            len(todo),
            model,
        ),
        flush=True,
    )
    print(
        "[info] 请求输入=min %d / avg %d / max %d 字符，并发=%d"
        % (
            min(len(render_prompt(group)) for group in all_groups),
            sum(len(render_prompt(group)) for group in all_groups)
            // max(1, len(all_groups)),
            max(len(render_prompt(group)) for group in all_groups),
            args.workers,
        ),
        flush=True,
    )

    failures, request_stats = [], []
    lock = threading.Lock()
    mode = "w" if args.restart else "a"
    started = time.time()
    with progress_path.open(mode, encoding="utf-8") as progress:
        tasks = [
            (group, api, key, model, args.max_output_tokens, args.timeout, args.retries)
            for group in todo
        ]
        with ThreadPoolExecutor(max_workers=args.workers) as executor:
            futures = {executor.submit(run_request, *task): task for task in tasks}
            for completed, future in enumerate(as_completed(futures), 1):
                group, result, error, usage, elapsed = future.result()
                request_stats.append(
                    {
                        "ids": [item["id"] for item in group],
                        "input_chars": len(render_prompt(group)),
                        "elapsed_s": elapsed,
                        "success": error is None,
                        "usage": usage,
                    }
                )
                if error:
                    failures.append(
                        {"ids": [item["id"] for item in group], "error": error}
                    )
                else:
                    request_found_time = found_time_now()
                    for prediction in result.values():
                        prediction["found_time"] = request_found_time
                    predictions.update(result)
                    with lock:
                        progress.write(
                            json.dumps(
                                {
                                    "fingerprint": fingerprint,
                                    "ids": [item["id"] for item in group],
                                    "predictions": result,
                                },
                                ensure_ascii=False,
                            )
                            + "\n"
                        )
                        progress.flush()
                print(
                    "    进度 %d/%d 已完成位置 %d%s"
                    % (
                        completed,
                        len(tasks),
                        len(predictions),
                        " 失败%d" % len(failures) if failures else "",
                    ),
                    flush=True,
                )

    complete = not failures and len(predictions) == len(items)
    write_json_results(out, items, predictions, model, complete, request_stats)
    write_comparison(out, items, predictions, changed, model)
    run_meta = {
        "model": model,
        "complete": complete,
        "comparable_items": len(items),
        "changed_items": len(changed),
        "requests": len(all_groups),
        "failures": failures,
        "elapsed_s": round(time.time() - started, 1),
        "fingerprint": fingerprint,
        "max_input_chars": args.max_input_chars,
        "max_output_tokens": args.max_output_tokens,
    }
    (out / "baseline-run-meta.json").write_text(
        json.dumps(run_meta, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(
        "[%s] 对比报告: %s"
        % ("ok" if complete else "error", out / "baseline-comparison.md")
    )
    return 0 if complete else 1


if __name__ == "__main__":
    sys.exit(main())
