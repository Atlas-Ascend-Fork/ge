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

"""日志规范每日全量巡检入口。"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL_ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import log_spec  # noqa: E402
import model_config  # noqa: E402
import report_artifacts  # noqa: E402
import repo_exclusions  # noqa: E402

DAILY_VERSION = "2"


def now_iso():
    return datetime.now().astimezone().isoformat(timespec="milliseconds")


def load_json(name):
    path = os.path.join(SKILL_ROOT, "references", name)
    if not os.path.isfile(path):
        return {}
    with open(path, "r", encoding="utf-8-sig") as handle:
        value = json.load(handle)
    return value if isinstance(value, dict) else {}


def team_of(mapping, repo):
    return mapping.get(repo) or mapping.get(repo.split("@")[0]) or "未分派"


def discover_targets(root, branch_filter, component_map):
    result = []
    for name in sorted(os.listdir(root)):
        path = os.path.join(root, name)
        if not os.path.isdir(path) or (
            name.startswith(".") and name.partition("@")[0] != ".gitcode"
        ):
            continue
        repo, _, branch = name.partition("@")
        branch = branch or "master"
        if branch_filter and branch != branch_filter:
            continue
        if component_map and repo not in component_map:
            continue
        result.append({"dir": name, "repo": repo, "branch": branch, "path": path})
    return result


def artifact_name(target):
    branch = target.get("branch", "")
    return (
        target["repo"]
        if not branch or branch == "master"
        else "%s@%s" % (target["repo"], branch.replace("/", "_"))
    )


def read_jsonl(path):
    rows = []
    try:
        with open(path, "r", encoding="utf-8-sig") as handle:
            for line in handle:
                try:
                    value = json.loads(line)
                except (TypeError, ValueError):
                    continue
                if isinstance(value, dict):
                    rows.append(value)
    except OSError:
        pass
    return rows


def copy_artifacts(out, source_name, target_name):
    if source_name == target_name:
        return
    for suffix in (
        "_logs.jsonl",
        "_findings.jsonl",
        "_meta.json",
        "_process.json",
        "_scan.json",
        "_progress.jsonl",
        "_rule_progress.jsonl",
        "_repository_progress.jsonl",
    ):
        source, target = (
            os.path.join(out, source_name + suffix),
            os.path.join(out, target_name + suffix),
        )
        if os.path.isfile(source):
            try:
                shutil.copyfile(source, target)
            except OSError:
                pass


def run_target(target, args, out, model, workers, lock):
    engine_name = artifact_name(target)
    daily_name = target["dir"]
    for suffix in ("_logs.jsonl", "_findings.jsonl", "_meta.json", "_scan.json"):
        for name in (engine_name, daily_name):
            try:
                os.remove(os.path.join(out, name + suffix))
            except FileNotFoundError:
                pass
    command = [
        sys.executable,
        os.path.join(HERE, "llm_evaluate.py"),
        "--repo",
        target["path"],
        "--repo-name",
        target["repo"],
        "--branch",
        target.get("branch", ""),
        "--out",
        out,
        "--workers",
        str(workers),
        "--timeout",
        str(args.timeout),
        "--retries",
        str(args.retries),
        "--target-input-chars",
        str(args.target_input_chars),
    ]
    for option, value in (
        ("--max-input-chars", args.max_input_chars),
        ("--max-output-tokens", args.max_output_tokens),
    ):
        if value is not None:
            command += [option, str(value)]
    if args.production_only:
        excluded = set(log_spec.exclude_dirs())
        excluded.update({"tests", "test", "examples", "example", "samples", "demo"})
        command += ["--exclude-dirs", ",".join(sorted(excluded))]
    if args.full_rescan:
        command.append("--restart")
    env = dict(
        os.environ,
        PYTHONIOENCODING="utf-8",
        LLM_MODEL=model,
        LLM_REQUIRE_MODEL_MATCH="1" if args.require_model_match else "0",
    )
    with lock:
        print(
            "[%s] 开始完整源码评测（请求并发=%d）" % (target["dir"], workers),
            flush=True,
        )
    started = time.monotonic()
    try:
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
            env=env,
        )
        for line in process.stdout:
            with lock:
                print("[%s] %s" % (target["dir"], line.rstrip()), flush=True)
        code = process.wait()
    except (OSError, subprocess.SubprocessError) as exc:
        code = 1
        with lock:
            print("[%s] 启动失败: %s" % (target["dir"], exc), file=sys.stderr)
    copy_artifacts(out, engine_name, daily_name)
    meta_path = os.path.join(out, daily_name + "_meta.json")
    try:
        with open(meta_path, "r", encoding="utf-8-sig") as handle:
            meta = json.load(handle)
    except (OSError, ValueError):
        meta = {}
    logs = read_jsonl(os.path.join(out, daily_name + "_logs.jsonl"))
    findings = read_jsonl(os.path.join(out, daily_name + "_findings.jsonl"))
    complete = bool(code == 0 and meta.get("complete", False))
    meta.update(
        {
            "repo": target["repo"],
            "branch": target.get("branch", ""),
            "mode": "full-daily",
            "input_scope": "full_repository",
            "complete": complete,
            "engine": "daily_run/%s" % DAILY_VERSION,
            "elapsed_s": meta.get("elapsed_s", round(time.monotonic() - started, 1)),
        }
    )
    try:
        with open(meta_path, "w", encoding="utf-8") as handle:
            json.dump(meta, handle, ensure_ascii=False, indent=2)
    except OSError:
        pass
    with lock:
        print(
            "[%s] %s：文件=%d 日志=%d 问题=%d"
            % (
                target["dir"],
                "完成" if complete else "未完成",
                meta.get("scanned_files", 0),
                len(logs),
                len(findings),
            ),
            flush=True,
        )
    return {
        "target": target,
        "logs": logs,
        "findings": findings,
        "meta": meta,
        "complete": complete,
    }


def write_team_report(out, team_map, per_repo):
    severity_names = [
        item["name"]
        for item in sorted(log_spec.SEVERITIES, key=lambda item: item["order"])
    ]
    teams = {}
    for payload in per_repo.values():
        team = team_of(team_map, payload["repo"])
        bucket = teams.setdefault(
            team,
            {
                "team": team,
                "repos": [],
                "findings": 0,
                "by_severity": {},
                "by_rule": {},
            },
        )
        bucket["repos"].append(
            {
                "repo": payload["repo"],
                "branch": payload["branch"],
                "findings": len(payload["findings"]),
                "log_statements": len(payload["logs"]),
                "complete": payload["complete"],
            }
        )
        bucket["findings"] += len(payload["findings"])
        for row in payload["findings"]:
            severity, rule = row.get("severity", "提示"), row.get("rule", "")
            bucket["by_severity"][severity] = bucket["by_severity"].get(severity, 0) + 1
            bucket["by_rule"][rule] = bucket["by_rule"].get(rule, 0) + 1
    ordered = sorted(teams.values(), key=lambda item: -item["findings"])
    payload = {
        "generated_at": now_iso(),
        "teams": ordered,
        "severity_order": severity_names,
    }
    with open(
        os.path.join(out, "daily-team-assignment.json"), "w", encoding="utf-8"
    ) as handle:
        json.dump(payload, handle, ensure_ascii=False, indent=2)
    lines = [
        "# CANN 日志规范每日整改清单",
        "",
        "生成时间：%s" % payload["generated_at"],
        "",
        "| 分队 | 问题数 | " + " | ".join(severity_names) + " | 涉及仓 |",
        "|------|--------|" + "|".join(["------"] * len(severity_names)) + "|--------|",
    ]
    for bucket in ordered:
        counts = " | ".join(
            str(bucket["by_severity"].get(name, 0)) for name in severity_names
        )
        lines.append(
            "| %s | %d | %s | %d |"
            % (bucket["team"], bucket["findings"], counts, len(bucket["repos"]))
        )
    lines.append("")
    path = os.path.join(out, "daily-team-assignment.md")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    return path


def build_parser():
    parser = argparse.ArgumentParser(description="CANN 日志规范评测 - 每日全量巡检")
    model_config.add_model_argument(parser)
    parser.add_argument("--input", default=os.path.join(SKILL_ROOT, "input"))
    parser.add_argument("--out", default=os.path.join(SKILL_ROOT, "output"))
    parser.add_argument("--branch", default="master")
    parser.add_argument("--repos", default="")
    parser.add_argument(
        "--exclude-repos", default=os.environ.get("LOG_EXCLUDE_REPOS", "")
    )
    parser.add_argument(
        "--exclude-repos-file", default=repo_exclusions.default_exclude_file()
    )
    parser.add_argument("--team", default="")
    parser.add_argument(
        "--concurrency",
        type=int,
        default=int(os.environ.get("LLM_MAX_CONCURRENCY", "64")),
    )
    parser.add_argument(
        "--agents", type=int, default=int(os.environ.get("LOG_EVALUATION_AGENTS", "10"))
    )
    parser.add_argument(
        "--max-workers-per-repo",
        type=int,
        default=int(os.environ.get("LLM_MAX_WORKERS_PER_REPO", "8")),
    )
    parser.add_argument(
        "--max-input-chars",
        type=int,
        default=int(os.environ["LLM_MAX_INPUT_CHARS"])
        if os.environ.get("LLM_MAX_INPUT_CHARS")
        else 1000000,
    )
    parser.add_argument(
        "--max-output-tokens",
        type=int,
        default=int(os.environ["LLM_MAX_TOKENS"])
        if os.environ.get("LLM_MAX_TOKENS")
        else None,
    )
    parser.add_argument(
        "--target-input-chars",
        type=int,
        default=int(os.environ.get("LLM_TARGET_INPUT_CHARS", "1000000")),
    )
    parser.add_argument(
        "--timeout", type=int, default=int(os.environ.get("LLM_TIMEOUT", "600"))
    )
    parser.add_argument(
        "--retries", type=int, default=int(os.environ.get("LLM_RETRIES", "1"))
    )
    parser.add_argument("--hours", type=float, default=0)
    parser.add_argument("--production-only", action="store_true", default=True)
    parser.add_argument(
        "--include-nonproduction", dest="production_only", action="store_false"
    )
    parser.add_argument("--full-rescan", action="store_true")
    parser.add_argument("--require-model-match", action="store_true", default=None)
    parser.add_argument(
        "--allow-model-alias", dest="require_model_match", action="store_false"
    )
    return parser


def main():
    args = build_parser().parse_args()
    if args.concurrency <= 0 or args.agents <= 0 or args.max_workers_per_repo <= 0:
        print("[error] 并发参数必须大于0", file=sys.stderr)
        return 2
    model = model_config.configure_model(args.model)
    if args.require_model_match is None:
        args.require_model_match = model_config.model_match_required(model)
    api, key = (
        os.environ.get("LLM_API_BASE", "").strip(),
        os.environ.get("LLM_API_KEY", "").strip(),
    )
    if not api or not key:
        print(
            "[error] 缺少配置: "
            + ", ".join(
                name
                for name, value in (("LLM_API_BASE", api), ("LLM_API_KEY", key))
                if not value
            ),
            file=sys.stderr,
        )
        return 2
    root = os.path.abspath(args.input)
    if not os.path.isdir(root):
        print("[error] 输入目录不存在或不是目录: %s" % root, file=sys.stderr)
        return 2
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    component_map = load_json("repo-component-map.json")
    targets = discover_targets(root, args.branch, component_map)
    excluded = repo_exclusions.load(args.exclude_repos, args.exclude_repos_file)
    targets, _ = repo_exclusions.filter_dict_targets(targets, excluded)
    wanted = {name.strip() for name in args.repos.split(",") if name.strip()}
    if wanted:
        _tmp_result = []
        for item in targets:
            if item["repo"] in wanted or item["dir"] in wanted:
                _tmp_result.append(item)
        targets = _tmp_result
    team_map = load_json("repo-team-map.json")
    if args.team:
        _tmp_result = []
        for item in targets:
            if team_of(team_map, item["repo"]) == args.team:
                _tmp_result.append(item)
        targets = _tmp_result
    if not targets:
        print("[error] 未发现待评测仓库: %s" % root, file=sys.stderr)
        return 2
    slots = min(args.agents, len(targets), args.concurrency)
    workers = max(1, min(args.max_workers_per_repo, args.concurrency // slots or 1))
    lock = threading.Lock()
    started = time.monotonic()
    with ThreadPoolExecutor(max_workers=slots) as executor:
        futures = [
            executor.submit(run_target, target, args, out, model, workers, lock)
            for target in targets
        ]
        results = [future.result() for future in as_completed(futures)]
    results.sort(key=lambda item: item["target"]["dir"])
    per_repo = {
        item["target"]["dir"]: {
            "repo": item["target"]["repo"],
            "branch": item["target"]["branch"],
            "logs": item["logs"],
            "findings": item["findings"],
            "complete": item["complete"],
        }
        for item in results
    }
    run_date = report_artifacts.current_run_date()
    with open(
        os.path.join(out, "current-run-targets.json"), "w", encoding="utf-8"
    ) as handle:
        json.dump(
            {
                "generated_at": now_iso(),
                "run_date": run_date,
                "targets": len(targets),
                "items": [
                    {
                        "repo": item["repo"],
                        "branch": item["branch"],
                        "artifact": item["dir"],
                    }
                    for item in targets
                ],
            },
            handle,
            ensure_ascii=False,
            indent=2,
        )
    report_path = write_team_report(out, team_map, per_repo)
    generated = subprocess.run(
        [
            sys.executable,
            os.path.join(HERE, "generate_all_reports.py"),
            "--workdir",
            out,
            "--date",
            run_date,
        ],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if generated.stdout.strip():
        print(generated.stdout.rstrip())
    if generated.returncode:
        if generated.stderr.strip():
            print(generated.stderr.rstrip(), file=sys.stderr)
        return 1
    missing = report_artifacts.verify_delivery(out, run_date)
    if missing:
        print("[error] 每日交付物缺失: %s" % ", ".join(missing), file=sys.stderr)
        return 1
    try:
        delta = subprocess.run(
            [sys.executable, os.path.join(HERE, "daily_delta.py"), "--workdir", out],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        if delta.stdout.strip():
            print(delta.stdout.rstrip())
    except (OSError, subprocess.SubprocessError):
        pass
    complete = sum(1 for item in results if item["complete"])
    state = {
        "generated_at": now_iso(),
        "run_date": run_date,
        "reports": list(report_artifacts.required_delivery_names(run_date).values()),
        "mode": "full-daily",
        "model": model,
        "requested_model": model,
        "targets": len(targets),
        "complete_repos": complete,
        "incomplete_repos": len(targets) - complete,
        "concurrency": args.concurrency,
        "repository_workers": workers,
        "log_statements": sum(len(item["logs"]) for item in results),
        "findings": sum(len(item["findings"]) for item in results),
        "elapsed_s": round(time.monotonic() - started, 1),
        "summary": [
            {
                "repo": item["target"]["repo"],
                "branch": item["target"]["branch"],
                "commit": item["meta"].get("commit", ""),
                "team": team_of(team_map, item["target"]["repo"]),
                "logs": len(item["logs"]),
                "findings": len(item["findings"]),
                "complete": item["complete"],
            }
            for item in results
        ],
    }
    with open(os.path.join(out, "daily-run.json"), "w", encoding="utf-8") as handle:
        json.dump(state, handle, ensure_ascii=False, indent=2)
    print(
        "[结果] 仓=%d 完成=%d 未完成=%d"
        % (len(targets), complete, len(targets) - complete)
    )
    print("[结果] 分派清单: %s" % report_path)
    return 0 if complete == len(targets) else 1


if __name__ == "__main__":
    sys.exit(main())
