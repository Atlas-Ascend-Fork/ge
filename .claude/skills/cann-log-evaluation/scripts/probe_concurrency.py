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

"""Measure bounded concurrent model requests without retries hiding failures."""

import argparse
import json
import math
import os
import threading
import time
import uuid
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path

import llm_evaluate as evaluator
import model_config


def run_wave(concurrency, send):
    barrier = threading.Barrier(concurrency)
    lock = threading.Lock()
    active = peak = 0

    def worker(index):
        nonlocal active, peak
        barrier.wait(timeout=30)
        with lock:
            active += 1
            peak = max(peak, active)
        try:
            return send(index)
        finally:
            with lock:
                active -= 1

    started = time.monotonic()
    with ThreadPoolExecutor(max_workers=concurrency) as pool:
        rows = list(pool.map(worker, range(concurrency)))
    elapsed = time.monotonic() - started
    latencies = sorted(row["elapsed_s"] for row in rows)
    successes = sum(row["success"] for row in rows)
    return {
        "concurrency": concurrency,
        "requests": len(rows),
        "successes": successes,
        "failures": len(rows) - successes,
        "peak_in_flight": peak,
        "elapsed_s": round(elapsed, 3),
        "requests_per_second": round(successes / elapsed, 3),
        "p50_s": latencies[math.ceil(len(rows) * 0.5) - 1],
        "p95_s": latencies[math.ceil(len(rows) * 0.95) - 1],
        "statuses": dict(Counter(str(row["status"]) for row in rows)),
        "response_models": sorted(
            {row["response_model"] for row in rows if row.get("response_model")}
        ),
        "usage": {
            key: sum(row.get("usage", {}).get(key, 0) for row in rows)
            for key in ("prompt_tokens", "completion_tokens", "total_tokens")
        },
        "errors": [row["error"] for row in rows if row.get("error")],
    }


def recommend_concurrency(waves):
    levels = sorted({wave["concurrency"] for wave in waves})
    passed = [
        level
        for level in levels
        if all(
            wave["failures"] == 0 and wave["peak_in_flight"] == level
            for wave in waves
            if wave["concurrency"] == level
        )
    ]
    if not passed:
        return None
    ceiling = max(passed)
    # Reserve one tested level of headroom for longer production requests.
    return max(
        (level for level in passed if level <= ceiling * 0.75), default=min(passed)
    )


def send_probe(api, key, model, kind, timeout):
    def send(index):
        nonce = uuid.uuid4().hex
        if kind == "echo":
            prompt = 'Return only JSON {"probe_id":"%s","ok":true}.' % nonce
            thinking, reasoning = "enabled", "low"
        else:
            lines = ["void run() {\n", '  printf("operation completed\\n");\n', "}\n"]
            record = {
                "file": "probe_%s.cc" % nonce,
                "start": 1,
                "end": 3,
                "source_type": "code",
                "language": "cpp",
                "text": evaluator.render_numbered(lines),
                "lines": lines,
                "source": "".join(lines),
            }
            prompt = evaluator.render_prompt([record], evaluator.blacklist(), ())
            thinking, reasoning = "disabled", None
        started = time.monotonic()
        try:
            content, usage = evaluator.call_llm(
                api,
                key,
                model,
                prompt,
                None,
                timeout,
                0,
                thinking=thinking,
                reasoning_effort=reasoning,
            )
            actual = usage.get("_response_model", "")
            if actual != model:
                raise ValueError(
                    "Response model does not match requested model: %s" % actual
                )
            data = json.loads(content)
            if kind == "echo":
                if data.get("probe_id") != nonce or data.get("ok") is not True:
                    raise ValueError("Invalid probe response")
            else:
                manifest, logs = evaluator.parse_result(content)
                validated, _ = evaluator.validate_result(
                    manifest, logs, [], [record], ()
                )
                if len(validated) != 1 or validated[0]["line"] != 2:
                    raise ValueError("Incomplete synthetic source extraction")
            return {
                "success": True,
                "status": 200,
                "response_model": actual,
                "elapsed_s": round(time.monotonic() - started, 3),
                "usage": usage,
            }
        except Exception as exc:
            message = str(exc).replace(key, "<redacted>")[:1000]
            status = getattr(exc, "code", None)
            if status is None and message.startswith("HTTP "):
                status = message.split(":", 1)[0].split()[-1]
            return {
                "success": False,
                "status": status or "validation_or_transport",
                "response_model": "",
                "elapsed_s": round(time.monotonic() - started, 3),
                "error": message,
            }

    return send


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    model_config.add_model_argument(parser)
    parser.add_argument("--steps", default="1,4,8,16,32,64,128")
    parser.add_argument("--repeats", type=int, default=2)
    parser.add_argument("--kind", choices=("echo", "extraction"), default="echo")
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        steps = sorted(set(int(value) for value in args.steps.split(",")))
    except ValueError:
        parser.error("--steps must be comma-separated integers")
    if (
        not steps
        or min(steps) <= 0
        or max(steps) > 256
        or args.repeats <= 0
        or args.timeout <= 0
    ):
        parser.error("Use 1..256 workers and positive repeats/timeout")
    api, key = os.environ.get("LLM_API_BASE", ""), os.environ.get("LLM_API_KEY", "")
    if not api or not key:
        parser.error("LLM_API_BASE and LLM_API_KEY are required")
    model = model_config.resolve_model(args.model)
    report = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "model": model,
        "kind": args.kind,
        "retries": 0,
        "waves": [],
        "scope": "Bounded client concurrency; does not establish the service hard limit.",
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    send = send_probe(api, key, model, args.kind, args.timeout)
    for concurrency in steps:
        for repeat in range(args.repeats):
            wave = run_wave(concurrency, send)
            wave["repeat"] = repeat + 1
            report["waves"].append(wave)
            report["recommended_concurrency"] = recommend_concurrency(report["waves"])
            args.out.write_text(
                json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                encoding="utf-8",
            )
            print(json.dumps(wave, ensure_ascii=False), flush=True)
            if wave["failures"]:
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
