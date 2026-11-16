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

"""Wait for the model quota, then resume and validate the daily evaluation."""

import argparse
import fcntl
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path
import model_config
import report_artifacts


ROOT = Path(__file__).resolve().parent.parent


def stamp():
    return datetime.now().astimezone().isoformat(timespec="seconds")


def quota_available(api, key, model=None):
    body = json.dumps(
        {
            "model": model_config.resolve_model(model),
            "messages": [{"role": "user", "content": "Reply OK"}],
            "thinking": {"type": "enabled"},
            "reasoning_effort": "low",
        }
    ).encode("utf-8")
    request = urllib.request.Request(
        api.rstrip("/") + "/chat/completions",
        data=body,
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=30):
            return True
    except urllib.error.HTTPError as exc:
        message = exc.read(1000).decode("utf-8", "replace")
        if exc.code == 429 and "Daily token quota exhausted" in message:
            return False
        raise RuntimeError("模型探测失败: HTTP %d" % exc.code) from exc


def evaluation_command(out):
    return [
        sys.executable,
        str(ROOT / "scripts" / "daily_run.py"),
        "--include-nonproduction",
        "--concurrency",
        "64",
        "--allow-model-alias",
        "--max-input-chars",
        "1000000",
        "--target-input-chars",
        "500000",
        "--timeout",
        "180",
        "--out",
        str(out),
    ]


def report_complete(out, expected_targets=34):
    """Validate the daily run: Markdown report + issue JSON are required."""
    manifest = out / "current-run-targets.json"
    daily = out / "daily-run.json"
    if not manifest.is_file() or not daily.is_file():
        return False
    try:
        targets = json.loads(manifest.read_text(encoding="utf-8"))
        state = json.loads(daily.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return False
    if (
        targets.get("targets") != expected_targets
        or state.get("targets") != expected_targets
        or state.get("complete_repos") != expected_targets
        or state.get("incomplete_repos") != 0
    ):
        return False
    date = targets.get("run_date")
    if not isinstance(date, str) or len(date) != 8 or not date.isdigit():
        return False
    required = report_artifacts.required_delivery_names(date)
    if not all((out / name).is_file() for name in required.values()):
        return False
    checked = subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts" / "check_run.py"),
            "--workdir",
            str(out),
            "--expected-targets",
            str(expected_targets),
        ],
        check=False,
    )
    return checked.returncode == 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    model_config.add_model_argument(parser)
    parser.add_argument(
        "--out", type=Path, default=ROOT / "output" / "model_all_master"
    )
    parser.add_argument("--poll-seconds", type=int, default=900)
    parser.add_argument(
        "--expected-targets", type=int, default=34, help="每日巡检预期仓库数，默认 34"
    )
    args = parser.parse_args(argv)
    if args.poll_seconds < 60:
        parser.error("--poll-seconds 必须至少为 60")
    api = os.environ.get("LLM_API_BASE", "").strip()
    key = os.environ.get("LLM_API_KEY", "").strip()
    if not api or not key:
        parser.error("缺少 LLM_API_BASE 或 LLM_API_KEY")
    model = model_config.configure_model(args.model)
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    expected = args.expected_targets
    with (out / ".resume-daily.lock").open("w") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print("[%s] 已有续跑监控进程" % stamp(), flush=True)
            return 1
        while True:
            if report_complete(out, expected):
                print("[%s] %d 仓报告已通过校验" % (stamp(), expected), flush=True)
                return 0
            try:
                available = quota_available(api, key)
            except (RuntimeError, OSError, ValueError) as exc:
                print(
                    "[%s] %s；%d 秒后重试" % (stamp(), exc, args.poll_seconds),
                    flush=True,
                )
                time.sleep(args.poll_seconds)
                continue
            if not available:
                print(
                    "[%s] %s 每日配额未恢复；%d 秒后重试"
                    % (stamp(), model, args.poll_seconds),
                    flush=True,
                )
                time.sleep(args.poll_seconds)
                continue
            print("[%s] 配额可用，开始续跑 %d 仓" % (stamp(), expected), flush=True)
            result = subprocess.run(evaluation_command(out), check=False)
            if result.returncode == 0 and report_complete(out, expected):
                print("[%s] %d 仓报告已通过校验" % (stamp(), expected), flush=True)
                return 0
            print(
                "[%s] 评测或校验未完成（退出码 %d）；%d 秒后重试"
                % (stamp(), result.returncode, args.poll_seconds),
                flush=True,
            )
            time.sleep(args.poll_seconds)


if __name__ == "__main__":
    sys.exit(main())
