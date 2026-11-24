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

"""本地一键入口：扫描 input/ → 大模型评测 → output/ 生成 Markdown 报告与问题 JSON。

Markdown 是主交付报告；Excel 仅在显式传入 --excel 时作为可选附件生成。

每个完整源码文件只送审一次；大模型识别全部日志并完成规范评测。

用法:
    python3 run_local.py                      # 评测 input/ 整体
    python3 run_local.py --per-subdir         # input 下每个子目录各当一个仓
    python3 run_local.py --input <路径> --name <名称>
    python3 run_local.py --agents 5           # 动态均衡仓库到5个独立agent
    python3 run_local.py --restart            # 丢弃旧进度，从头评测

必需环境变量（本工具完全依赖大模型）:
    LLM_API_BASE / LLM_API_KEY；模型默认 glm-5.3-flash
输入硬上限（默认 1000000 字符）:
    LLM_MAX_INPUT_CHARS；输出上限可用 LLM_MAX_TOKENS
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report_artifacts  # noqa: E402
import repo_exclusions  # noqa: E402
import model_config  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL_ROOT = os.path.dirname(HERE)
PY = sys.executable
DEFAULT_MAX_INPUT_CHARS = 1000000

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass


def run(cmd, stream=False, heartbeat=None, prefix=""):
    """执行子命令。

    stream=True 时逐行透传输出：全量仓要跑几个小时，进度与 ETA 必须实时可见，
    用 capture_output 会一直憋到该仓跑完才吐出来，等于没有进度。
    """
    if not stream:
        r = subprocess.run(
            cmd, capture_output=True, text=True, encoding="utf-8", errors="replace"
        )
        if r.stdout:
            print(r.stdout.rstrip())
        if r.returncode != 0 and r.stderr:
            print(r.stderr.rstrip(), file=sys.stderr)
        return r.returncode

    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    p = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,
        env=env,
    )
    stop_heartbeat = threading.Event()
    monitor = None
    if heartbeat:

        def monitor_progress():
            while not stop_heartbeat.wait(10):
                heartbeat()

        monitor = threading.Thread(target=monitor_progress, daemon=True)
        monitor.start()
    for line in p.stdout:
        print(prefix + line.rstrip(), flush=True)
    p.stdout.close()
    p.wait()
    stop_heartbeat.set()
    if monitor:
        monitor.join(timeout=1)
    if heartbeat:
        heartbeat()
    return p.returncode


def report_name_arg(value):
    try:
        return report_artifacts.validate_report_name(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from exc


def main():
    run_started = time.monotonic()
    run_started_wall = datetime.now().astimezone()
    run_started_at = run_started_wall.isoformat(timespec="milliseconds")
    run_date = report_artifacts.current_run_date(run_started_wall)
    ap = argparse.ArgumentParser(description="日志规范评测 - 本地一键入口")
    model_config.add_model_argument(ap)
    ap.add_argument(
        "--input",
        default=os.path.join(SKILL_ROOT, "input"),
        help="待检查的目录，默认 skill 的 input/ 目录",
    )
    ap.add_argument(
        "--out",
        default=os.path.join(SKILL_ROOT, "output"),
        help="输出目录，默认 skill 的 output/ 目录",
    )
    ap.add_argument(
        "--report-name",
        type=report_name_arg,
        default=None,
        help="Markdown 报告文件名，默认 cann-log-evaluation-report-<date>.md",
    )
    ap.add_argument("--name", default="input", help="本次评测名称")
    ap.add_argument(
        "--per-subdir",
        action="store_true",
        help="把输入目录的每个子目录各当一个仓分别评测",
    )
    ap.add_argument(
        "--master-only",
        action="store_true",
        help="只评测 master（无 @分支 后缀的目录也视为 master）",
    )
    ap.add_argument(
        "--daily", action="store_true", help="每日巡检预设：master-only + 完整文件送审"
    )
    ap.add_argument(
        "--excel",
        action="store_true",
        help="同时生成可选 Excel 报告（需要 openpyxl）；默认只出 Markdown",
    )
    ap.add_argument(
        "--max-input-chars",
        type=int,
        default=int(os.environ["LLM_MAX_INPUT_CHARS"])
        if os.environ.get("LLM_MAX_INPUT_CHARS")
        else DEFAULT_MAX_INPUT_CHARS,
        help="完整请求输入硬上限，默认1000000字符；单文件超限直接失败",
    )
    ap.add_argument(
        "--max-output-tokens",
        type=int,
        default=int(os.environ["LLM_MAX_TOKENS"])
        if os.environ.get("LLM_MAX_TOKENS")
        else None,
        help="可选输出上限；未设置时不发送 max_tokens",
    )
    ap.add_argument(
        "--target-input-chars",
        type=int,
        default=int(os.environ["LLM_TARGET_INPUT_CHARS"])
        if os.environ.get("LLM_TARGET_INPUT_CHARS")
        else None,
        help="普通请求目标输入大小；单文件始终完整送审，超硬上限直接失败",
    )
    ap.add_argument(
        "--rule-target-input-chars",
        type=int,
        default=int(os.environ["LLM_RULE_TARGET_INPUT_CHARS"])
        if os.environ.get("LLM_RULE_TARGET_INPUT_CHARS")
        else None,
        help="开启 Thinking 的规则评测请求目标大小，默认不超过50000字符",
    )
    ap.add_argument(
        "--workers",
        type=int,
        default=None,
        help="兼容旧命令，仅允许1；实际仓内并行由运行计划动态分配",
    )
    ap.add_argument(
        "--agents",
        type=int,
        default=None,
        help="仓库级独立agent数量，默认10；仓库动态均衡分组",
    )
    ap.add_argument(
        "--max-concurrency",
        type=int,
        default=int(
            os.environ.get("LLM_MAX_CONCURRENCY", str(model_config.DEFAULT_CONCURRENCY))
        ),
        help="全局模型请求并发槽上限，默认64",
    )
    ap.add_argument(
        "--max-workers-per-repo",
        type=int,
        default=int(os.environ.get("LLM_MAX_WORKERS_PER_REPO", "8")),
        help="单仓文件请求并发上限，默认8；运行计划动态分配",
    )
    ap.add_argument(
        "--timeout", type=int, default=int(os.environ.get("LLM_TIMEOUT", "600"))
    )
    ap.add_argument(
        "--retries",
        type=int,
        default=int(os.environ.get("LLM_RETRIES", "1")),
        help="瞬时失败请求重传次数，默认1；确定性错误不会原样重试",
    )
    ap.add_argument(
        "--extensions",
        default=os.environ.get("LOG_CODE_EXTENSIONS"),
        help="可选：扫描扩展名，逗号分隔",
    )
    ap.add_argument(
        "--exclude-dirs",
        default=os.environ.get("LOG_EXCLUDE_DIRS"),
        help="可选：跳过目录名，逗号分隔",
    )
    ap.add_argument(
        "--exclude-repos",
        default=os.environ.get("LOG_EXCLUDE_REPOS", ""),
        help="内部仓排除清单，逗号分隔；名称不会写入报告",
    )
    ap.add_argument(
        "--exclude-repos-file",
        default=repo_exclusions.default_exclude_file(),
        help="内部仓排除清单文件，每行一个仓名",
    )
    ap.add_argument("--restart", action="store_true", help="忽略已有进度，从头重跑")
    ap.add_argument(
        "--mode",
        choices=("full",),
        default="full",
        help="兼容参数；默认完整源码文件送审",
    )
    ap.add_argument(
        "--retry-passes",
        type=int,
        default=int(os.environ.get("LLM_RETRY_PASSES", "3")),
        help="整批结束后自动补跑失败目标的轮数，默认3",
    )
    args = ap.parse_args()
    model_config.configure_model(args.model)
    if args.daily:
        args.master_only = True
    if args.agents is None:
        args.agents = int(
            os.environ.get("LOG_EVALUATION_AGENTS", "1" if args.daily else "10")
        )
    if args.workers not in (None, 1):
        print("[error] --workers 仅兼容值1；请使用动态仓内分片", file=sys.stderr)
        sys.exit(2)
    if args.agents <= 0 or args.max_concurrency <= 0 or args.max_workers_per_repo <= 0:
        print("[error] agent、并发和单仓并发参数必须大于0", file=sys.stderr)
        sys.exit(2)

    src = os.path.abspath(args.input)
    if not os.path.exists(src):
        print(f"[error] 输入不存在: {src}")
        print("        请先运行 fetch_repo.py，或通过 --input 指定已有代码目录")
        sys.exit(2)

    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    # 目录名形如 "仓名@分支"（fetch_repo.py 拉取时生成）则拆出分支
    def split_name(dirname):
        if "@" in dirname:
            n, b = dirname.split("@", 1)
            return n, b
        return dirname, ""

    targets = []  # [(repo_name, branch, path), ...]
    if args.per_subdir and os.path.isdir(src):
        for d in sorted(os.listdir(src)):
            p = os.path.join(src, d)
            if os.path.isdir(p) and not d.startswith("."):
                n, b = split_name(d)
                if args.master_only and b not in ("", "master"):
                    continue
                targets.append((n, b, p))
    if args.per_subdir and not targets:
        print("[error] 输入目录没有符合条件的仓库", file=sys.stderr)
        sys.exit(2)
    if not targets:
        base = (
            args.name if args.name != "input" else os.path.basename(src.rstrip("/\\"))
        )
        n, b = split_name(base)
        if args.master_only and b not in ("", "master"):
            print(
                "[error] --master-only 跳过了输入分支: %s" % (b or "-"), file=sys.stderr
            )
            sys.exit(2)
        targets = [(n, b, src)]

    try:
        exclusion_set = repo_exclusions.load(
            args.exclude_repos, args.exclude_repos_file
        )
    except OSError:
        print("[error] 无法读取内部仓排除清单，请检查路径和权限", file=sys.stderr)
        sys.exit(2)
    targets, excluded_targets = repo_exclusions.filter_tuple_targets(
        targets, exclusion_set
    )

    target_manifest_path = os.path.join(out, "current-run-targets.json")
    with open(target_manifest_path, "w", encoding="utf-8") as handle:
        json.dump(
            {
                "generated_at": datetime.now()
                .astimezone()
                .isoformat(timespec="milliseconds"),
                "run_date": run_date,
                "targets": len(targets),
                "daily": args.daily,
                "master_only": args.master_only,
                "excluded_targets": excluded_targets,
                "items": [
                    {
                        "repo": name,
                        "branch": branch,
                        "artifact": name
                        if not branch or branch == "master"
                        else "%s@%s" % (name, branch.replace("/", "_")),
                    }
                    for name, branch, _path in targets
                ],
            },
            handle,
            ensure_ascii=False,
            indent=2,
        )

    if not targets and excluded_targets:
        with open(
            os.path.join(out, "full-run-timing.json"), "w", encoding="utf-8"
        ) as handle:
            json.dump(
                {
                    "status": "skipped_internal_repositories",
                    "updated_at": datetime.now()
                    .astimezone()
                    .isoformat(timespec="milliseconds"),
                    "targets": 0,
                    "excluded_targets": excluded_targets,
                },
                handle,
                ensure_ascii=False,
                indent=2,
            )
        print(
            "[完成] 已按内部仓策略排除 %d 个目标，未读取源码、未调用模型；"
            "名称未写入产物。" % excluded_targets
        )
        return

    required = {
        "LLM_API_BASE": os.environ.get("LLM_API_BASE"),
        "LLM_API_KEY": os.environ.get("LLM_API_KEY"),
        "LLM_MODEL": os.environ.get("LLM_MODEL"),
    }
    missing = [name for name, value in required.items() if not value]
    if missing:
        print("[error] 缺少模型配置: %s" % ", ".join(missing))
        sys.exit(2)
    if (args.max_input_chars is not None and args.max_input_chars <= 0) or (
        args.max_output_tokens is not None and args.max_output_tokens <= 0
    ):
        print("[error] 显式输入/输出硬上限必须大于0", file=sys.stderr)
        sys.exit(2)
    if args.target_input_chars is None:
        args.target_input_chars = (
            min(args.max_input_chars, DEFAULT_MAX_INPUT_CHARS)
            if args.max_input_chars is not None
            else DEFAULT_MAX_INPUT_CHARS
        )
    if (
        args.max_input_chars is not None
        and args.target_input_chars > args.max_input_chars
    ):
        print("[error] 目标输入不能超过显式输入硬上限", file=sys.stderr)
        sys.exit(2)

    timing_path = os.path.join(out, "full-run-timing.json")
    progress_path = os.path.join(out, "full-run-progress.json")
    state_lock = threading.RLock()
    previous_elapsed = 0.0
    if os.path.exists(timing_path) and not args.restart:
        try:
            with open(timing_path, "r", encoding="utf-8-sig") as handle:
                previous_timing = json.load(handle)
            if previous_timing.get("status") == "paused":
                previous_elapsed = float(previous_timing.get("elapsed_s") or 0)
                run_started_at = previous_timing.get("started_at") or run_started_at
        except (OSError, ValueError, TypeError):
            previous_elapsed = 0.0

    def write_timing(status, failed_targets=None):
        value = {
            "status": status,
            "mode": "full",
            "model": os.environ.get("LLM_MODEL"),
            "run_date": run_date,
            "started_at": run_started_at,
            "updated_at": datetime.now()
            .astimezone()
            .isoformat(timespec="milliseconds"),
            "elapsed_s": round(previous_elapsed + time.monotonic() - run_started, 1),
            "targets": len(targets),
            "max_input_chars": args.max_input_chars,
            "max_output_tokens": args.max_output_tokens,
            "target_input_chars": args.target_input_chars,
            "rule_target_input_chars": args.rule_target_input_chars,
            "max_concurrency": args.max_concurrency,
            "max_workers_per_repo": args.max_workers_per_repo,
            "agents": args.agents,
            "daily": args.daily,
            "master_only": args.master_only,
            "excluded_targets": excluded_targets,
            "failed_targets": list(failed_targets or []),
        }
        with state_lock:
            with open(timing_path, "w", encoding="utf-8") as handle:
                json.dump(value, handle, ensure_ascii=False, indent=2)

    target_status = {
        "%s@%s" % (name, branch or "-"): {
            "repo": name,
            "branch": branch,
            "status": "pending",
            "attempts": 0,
            "agent": None,
            "stage": "pending",
            "completed_requests": 0,
            "total_requests": 0,
            "log_statements": 0,
            "findings": 0,
            "last_error": "",
        }
        for name, branch, _path in targets
    }

    def artifact_name(name, branch):
        return (
            name
            if not branch or branch == "master"
            else "%s@%s" % (name, branch.replace("/", "_"))
        )

    def refresh_target(name, branch, status=None):
        with state_lock:
            key_name = "%s@%s" % (name, branch or "-")
            row = target_status[key_name]
            process_path = os.path.join(
                out, "%s_process.json" % artifact_name(name, branch)
            )
            if os.path.exists(process_path):
                try:
                    with open(process_path, "r", encoding="utf-8-sig") as handle:
                        process = json.load(handle)
                    row.update(
                        {
                            "stage": process.get("stage", row["stage"]),
                            "commit": process.get("commit", ""),
                            "log_statements": process.get("log_statements", 0),
                            "findings": process.get("findings", 0),
                            "last_error": process.get("last_error", ""),
                            "updated_at": process.get("updated_at"),
                        }
                    )
                    current_stage = (
                        process.get("rule_review")
                        if process.get("stage") == "rule_review"
                        else process.get("extraction")
                    )
                    if isinstance(current_stage, dict):
                        row["completed_requests"] = current_stage.get("completed", 0)
                        row["total_requests"] = current_stage.get("total", 0)
                except (OSError, ValueError, TypeError):
                    pass
            if status:
                row["status"] = status
            completed = sum(
                item["status"] == "completed" for item in target_status.values()
            )
            failed_rows = [
                item["repo"]
                for item in target_status.values()
                if item["status"] == "failed"
            ]
            value = {
                "status": "running" if completed < len(targets) else "completed",
                "model": os.environ.get("LLM_MODEL"),
                "targets": len(targets),
                "agents": args.agents,
                "max_concurrency": args.max_concurrency,
                "completed": completed,
                "remaining": len(targets) - completed,
                "failed": failed_rows,
                "updated_at": datetime.now()
                .astimezone()
                .isoformat(timespec="milliseconds"),
                "items": list(target_status.values()),
            }
            with open(progress_path, "w", encoding="utf-8") as handle:
                json.dump(value, handle, ensure_ascii=False, indent=2)
            write_timing("running", failed_rows)

    def write_run_manifest():
        rule_path = os.path.join(SKILL_ROOT, "references", "log-rules.md")
        with open(rule_path, "rb") as handle:
            rule_hash = hashlib.sha256(handle.read()).hexdigest()
        items = []
        for name, branch, _path in targets:
            meta_path = os.path.join(out, "%s_meta.json" % artifact_name(name, branch))
            with open(meta_path, "r", encoding="utf-8-sig") as handle:
                meta = json.load(handle)
            items.append(
                {
                    key: meta.get(key)
                    for key in (
                        "repo",
                        "branch",
                        "commit",
                        "model",
                        "complete",
                        "input_scope",
                        "scanned_files",
                        "model_reviewed_files",
                        "review_units",
                        "segmented_files",
                        "log_statements",
                        "findings",
                        "evaluation_started_at",
                        "evaluation_finished_at",
                        "elapsed_s",
                        "workers",
                        "usage",
                        "stages",
                        "failed_requests",
                    )
                }
            )
            items[-1]["repo_workers"] = repo_workers.get((name, branch), 1)
        value = {
            "generated_at": datetime.now()
            .astimezone()
            .isoformat(timespec="milliseconds"),
            "run_date": run_date,
            "model": os.environ.get("LLM_MODEL"),
            "rule_spec_sha256": rule_hash,
            "data_processing": "完整源码发送到使用者配置的模型接口；报告不得包含 API Key",
            "targets": len(items),
            "complete": all(item["complete"] for item in items),
            "excluded_targets": excluded_targets,
            "items": items,
        }
        with open(
            os.path.join(out, "cann-log-evaluation-run.json"), "w", encoding="utf-8"
        ) as handle:
            json.dump(value, handle, ensure_ascii=False, indent=2)

    write_timing("running")

    print("[0/4] 生成运行计划（不调用模型）")
    plan_cmd = [
        PY,
        os.path.join(HERE, "plan_run.py"),
        "--input",
        src,
        "--target-input-chars",
        str(args.target_input_chars),
        "--agents",
        str(args.agents),
        "--max-concurrency",
        str(args.max_concurrency),
        "--max-workers-per-repo",
        str(args.max_workers_per_repo),
        "--out",
        os.path.join(out, "run-plan.json"),
    ]
    if args.max_input_chars is not None:
        plan_cmd += ["--max-input-chars", str(args.max_input_chars)]
    if args.max_output_tokens is not None:
        plan_cmd += ["--max-output-tokens", str(args.max_output_tokens)]
    if args.per_subdir:
        plan_cmd.append("--per-subdir")
    if args.master_only:
        plan_cmd.append("--master-only")
    if args.extensions is not None:
        plan_cmd += ["--extensions", args.extensions]
    if args.exclude_dirs is not None:
        plan_cmd += ["--exclude-dirs", args.exclude_dirs]
    if args.exclude_repos:
        plan_cmd += ["--exclude-repos", args.exclude_repos]
    if args.exclude_repos_file:
        plan_cmd += ["--exclude-repos-file", args.exclude_repos_file]
    if run(plan_cmd) != 0:
        write_timing("planning_failed")
        print("[error] 运行计划失败，未调用模型", file=sys.stderr)
        sys.exit(1)

    plan_path = os.path.join(out, "run-plan.json")
    with open(plan_path, "r", encoding="utf-8-sig") as handle:
        plan = json.load(handle)
    weights = {
        (item["repo"], item.get("branch") or ""): int(
            item.get("balance_weight") or item["input_chars"]
        )
        for item in plan["items"]
    }
    repo_workers = {
        (item["repo"], item.get("branch") or ""): int(item.get("repo_workers") or 1)
        for item in plan["items"]
    }

    def balance(items):
        count = min(args.agents, max(1, len(items)))
        _tmp_result = []
        for index in range(count):
            _tmp_result.append({"agent": index + 1, "weight": 0, "targets": []})
        shards = _tmp_result
        for target in sorted(
            items,
            key=lambda item: (-weights.get((item[0], item[1]), 0), item[0], item[1]),
        ):
            shard = min(
                shards,
                key=lambda item: (item["weight"], len(item["targets"]), item["agent"]),
            )
            shard["targets"].append(target)
            shard["weight"] += weights.get((target[0], target[1]), 0)
        return shards

    def evaluate_target(agent_id, target, pass_index):
        name, branch, path = target
        with state_lock:
            row = target_status["%s@%s" % (name, branch or "-")]
            row["attempts"] += 1
            row["agent"] = agent_id
            row["repo_workers"] = repo_workers.get((name, branch), 1)
        refresh_target(name, branch, "running")
        cmd = [
            PY,
            os.path.join(HERE, "llm_evaluate.py"),
            "--repo",
            path,
            "--repo-name",
            name,
            "--branch",
            branch,
            "--out",
            out,
            "--workers",
            str(repo_workers.get((name, branch), 1)),
            "--timeout",
            str(args.timeout),
            "--retries",
            str(args.retries),
        ]
        if args.max_input_chars is not None:
            cmd += ["--max-input-chars", str(args.max_input_chars)]
        if args.max_output_tokens is not None:
            cmd += ["--max-output-tokens", str(args.max_output_tokens)]
        if args.target_input_chars is not None:
            cmd += ["--target-input-chars", str(args.target_input_chars)]
        if args.rule_target_input_chars is not None:
            cmd += ["--rule-target-input-chars", str(args.rule_target_input_chars)]
        if args.restart and pass_index == 0:
            cmd += ["--restart"]
        if args.extensions is not None:
            cmd += ["--extensions", args.extensions]
        if args.exclude_dirs is not None:
            cmd += ["--exclude-dirs", args.exclude_dirs]
        code = run(
            cmd,
            stream=True,
            heartbeat=lambda n=name, b=branch: refresh_target(n, b),
            prefix="[agent-%d] " % agent_id,
        )
        if code:
            print(
                "[agent-%d] [warn] %s 本轮未完整，稍后自动补跑" % (agent_id, name),
                flush=True,
            )
            refresh_target(name, branch, "failed")
            return target
        refresh_target(name, branch, "completed")
        return None

    def evaluate_shard(shard, pass_index):
        failures = []
        for target in shard["targets"]:
            try:
                failed_target = evaluate_target(shard["agent"], target, pass_index)
            except Exception as exc:  # noqa: BLE001
                name, branch, _path = target
                with state_lock:
                    row = target_status["%s@%s" % (name, branch or "-")]
                    row["last_error"] = "%s: %s" % (type(exc).__name__, exc)
                print(
                    "[agent-%d] [error] %s 执行异常，继续下一个仓库: %s"
                    % (shard["agent"], name, exc),
                    file=sys.stderr,
                    flush=True,
                )
                refresh_target(name, branch, "failed")
                failed_target = target
            if failed_target:
                failures.append(failed_target)
        return failures

    print(
        "[1/4] 大模型评测（模式=full，动态仓库=%d，独立agent=%d，全局请求槽=%d）"
        % (len(targets), min(args.agents, len(targets)), args.max_concurrency)
    )
    failed = [(name, branch, path) for name, branch, path in targets]
    for pass_index in range(args.retry_passes + 1):
        if not failed:
            break
        current, failed = failed, []
        shards = balance(current)
        if pass_index:
            print(
                "[info] 自动补跑第 %d/%d 轮，目标=%d"
                % (pass_index, args.retry_passes, len(current)),
                flush=True,
            )
        print(
            "[info] 分组负载: %s"
            % [
                {
                    "agent": shard["agent"],
                    "repos": len(shard["targets"]),
                    "input_chars": shard["weight"],
                }
                for shard in shards
            ],
            flush=True,
        )
        with ThreadPoolExecutor(max_workers=len(shards)) as executor:
            _tmp_result = []
            for shard in shards:
                _tmp_result.append(executor.submit(evaluate_shard, shard, pass_index))
            futures = _tmp_result
            for future in as_completed(futures):
                failed.extend(future.result())

    if failed:
        failed_names = [name for name, _branch, _path in failed]
        write_timing("incomplete", failed_names)
        print(
            "[warn] 以下目标仍有失败请求，将生成带完整性告警的部分结果报告：%s"
            % ", ".join(failed_names),
            file=sys.stderr,
        )
        print("       再次运行会继续复用成功断点。", file=sys.stderr)

    write_run_manifest()

    print("[2/4] 验证过程和结果完整性")
    if (
        run(
            [
                PY,
                os.path.join(HERE, "check_run.py"),
                "--workdir",
                out,
                "--expected-targets",
                str(len(targets)),
                "--allow-incomplete",
            ]
        )
        != 0
    ):
        write_timing("validation_failed")
        print("[error] 过程验证失败，无法生成报告", file=sys.stderr)
        sys.exit(1)

    print("[3/4] 生成 Markdown 报告与问题 JSON（Excel 可选）")
    report_cmd = [
        PY,
        os.path.join(HERE, "generate_all_reports.py"),
        "--workdir",
        out,
        "--date",
        run_date,
    ]
    if args.excel:
        report_cmd.append("--excel")
    if args.report_name:
        report_cmd += ["--report-name", args.report_name]
    report_errors = run(report_cmd) != 0
    if args.daily:
        report_errors += (
            run([PY, os.path.join(HERE, "daily_delta.py"), "--workdir", out]) != 0
        )

    if report_errors:
        write_timing("report_failed")
        print(
            "[error] %d 个报告生成步骤失败，未标记为完成" % report_errors,
            file=sys.stderr,
        )
        sys.exit(1)

    # Excel 是可选附件：缺 openpyxl 时 generate_all_reports 只告警，不能因此
    # 判定整轮交付失败，所以这里只校验必需产物。
    missing = report_artifacts.verify_delivery(
        out, run_date, report_name=args.report_name
    )
    if missing:
        write_timing("report_failed")
        print("[error] 交付物缺失: %s" % ", ".join(missing), file=sys.stderr)
        sys.exit(1)

    report_name = args.report_name or report_artifacts.markdown_report_name(run_date)
    report_path = os.path.join(out, report_name)
    print(f"\n[完成] 报告已生成到: {report_path}")
    for key, name in report_artifacts.delivery_names(
        run_date, include_excel=args.excel, report_name=args.report_name
    ).items():
        if not os.path.isfile(os.path.join(out, name)):
            continue
        print("  - %s%s" % (name, "（可选）" if key == "excel" else ""))
    if args.daily:
        print("  - daily-delta.md / daily-delta.json")
    write_timing("completed")


if __name__ == "__main__":
    main()
