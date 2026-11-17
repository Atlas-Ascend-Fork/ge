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

"""拉取代码仓到 input 目录，供评测使用。

支持指定分支；克隆后目录名带上分支标识，便于报告区分。

用法:
    # 拉单个仓的指定分支
    python3 fetch_repo.py --url https://gitcode.com/cann/runtime.git --branch 9.1.0

    # 一次拉多个分支
    python3 fetch_repo.py --url https://gitcode.com/cann/runtime.git --branch 9.1.0,master

    # 从清单文件批量拉（每行: <仓名> <克隆地址>）
    python3 fetch_repo.py --list repos.txt --branch 9.1.0,master

    # 不带仓库参数时，拉取 repo-component-map.json 中全部仓库的 master 分支
    python3 fetch_repo.py

    # 浅克隆(默认开启,只取最新提交,省时省空间)；--full 拉完整历史
"""

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL_ROOT = os.path.dirname(HERE)
INPUT_DIR = os.path.join(SKILL_ROOT, "input")
DEFAULT_REPO_MAP = os.path.join(SKILL_ROOT, "references", "repo-component-map.json")

try:
    sys.stdout.reconfigure(encoding="utf-8")
except (AttributeError, ValueError):
    pass


def repo_name_from_url(url):
    name = url.rstrip("/").split("/")[-1]
    return re.sub(r"\.git$", "", name)


def clone(url, branch, dest, shallow):
    # -c core.hooksPath=NUL: 禁用仓库自带钩子（git 安全默认会中断带钩子的 clone）
    cmd = ["git", "-c", "core.hooksPath=NUL", "clone", "--quiet"]
    if branch:
        cmd += ["--branch", branch]
    if shallow:
        cmd += ["--depth", "1"]
    cmd += [url, dest]
    r = subprocess.run(
        cmd, capture_output=True, text=True, encoding="utf-8", errors="replace"
    )
    return r.returncode, (r.stderr or "").strip()


def read_list(path):
    items = []
    with open(path, "r", encoding="utf-8-sig") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            name = parts[0]
            url = parts[1] if len(parts) > 1 else f"https://gitcode.com/cann/{name}.git"
            items.append((name, url))
    return items


def read_repo_component_map(path):
    with open(path, "r", encoding="utf-8-sig") as handle:
        mapping = json.load(handle)
    if (
        not isinstance(mapping, dict)
        or not mapping
        or any(not isinstance(name, str) or not name.strip() for name in mapping)
    ):
        raise ValueError("repo-component-map.json 必须是非空的仓名到组件名 JSON 对象")
    return [(name, f"https://gitcode.com/cann/{name}.git") for name in mapping]


def main():
    ap = argparse.ArgumentParser(description="拉取代码仓到 input 目录")
    ap.add_argument("--url", help="仓库克隆地址")
    ap.add_argument("--list", dest="list_file", help="仓库清单文件(每行: 仓名 地址)")
    ap.add_argument("--branch", help="分支，多个用逗号分隔；默认映射模式使用 master")
    ap.add_argument("--full", action="store_true", help="拉完整历史(默认浅克隆)")
    ap.add_argument("--input", default=INPUT_DIR, help="目标目录，默认 skill 的 input/")
    args = ap.parse_args()

    targets = []
    if args.url:
        targets.append((repo_name_from_url(args.url), args.url))
    if args.list_file:
        targets.extend(read_list(args.list_file))
    default_map = not args.url and not args.list_file
    if default_map:
        try:
            targets = read_repo_component_map(DEFAULT_REPO_MAP)
        except (OSError, ValueError) as exc:
            ap.error("读取默认仓库清单失败: %s" % exc)

    branch_arg = (
        args.branch if args.branch is not None else ("master" if default_map else "")
    )
    branches = [b.strip() for b in branch_arg.split(",") if b.strip()] or [""]
    os.makedirs(args.input, exist_ok=True)
    shallow = not args.full

    ok = skip = fail = 0
    total = len(targets) * len(branches)
    idx = 0
    for name, url in targets:
        for br in branches:
            idx += 1
            # 目录名带分支，报告里可区分
            dirname = f"{name}@{br}" if br else name
            dirname = dirname.replace("/", "_")
            dest = os.path.join(args.input, dirname)
            print(f"[{idx}/{total}] {name}" + (f" @ {br}" if br else ""))
            if os.path.isdir(os.path.join(dest, ".git")):
                print("    已存在，跳过")
                skip += 1
                continue
            code, err = clone(url, br, dest, shallow)
            if code == 0:
                print(f"    已拉取到 input/{dirname}")
                ok += 1
            else:
                print(f"    [失败] {err[:160]}")
                fail += 1

    print(f"\n完成：成功 {ok} / 跳过 {skip} / 失败 {fail}")
    print(f"代码已放入: {args.input}")
    if default_map:
        print("接下来运行评测：python3 scripts/daily_run.py --require-model-match")
    else:
        print("接下来运行评测：python3 scripts/run_local.py --per-subdir")


if __name__ == "__main__":
    main()
