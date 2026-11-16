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

"""Install cann-log-evaluation into a supported Agent client."""

import argparse
import os
import shutil


SKILL_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKILL_NAME = os.path.basename(SKILL_DIR)
EXCLUDE = {
    "output",
    "input",
    "__pycache__",
    ".git",
    ".cann-log-excluded-repos",
}


def target_dir(client):
    home = os.path.expanduser("~")
    if client == "codex":
        base = os.environ.get("CODEX_HOME", os.path.join(home, ".codex"))
        return os.path.join(base, "skills")
    if client == "claude":
        base = os.environ.get("CLAUDE_HOME", os.path.join(home, ".claude"))
        return os.path.join(base, "skills")
    base = os.environ.get(
        "OPENCODE_CONFIG_DIR", os.path.join(home, ".config", "opencode")
    )
    return os.path.join(base, "skill")


def copy_skill(dst_root, dry_run):
    dst = os.path.join(dst_root, SKILL_NAME)
    if dry_run:
        print(f"    [dry-run] Would copy to {dst}")
        return

    os.makedirs(dst_root, exist_ok=True)
    if os.path.exists(dst):
        shutil.rmtree(dst)

    def ignore(_dirpath, names):
        return [name for name in names if name in EXCLUDE or name.endswith(".pyc")]

    shutil.copytree(SKILL_DIR, dst, ignore=ignore)
    os.makedirs(os.path.join(dst, "output"), exist_ok=True)
    print(f"    Installed to {dst}")


def main():
    parser = argparse.ArgumentParser(description="Install cann-log-evaluation skill")
    parser.add_argument(
        "--target",
        default="codex",
        choices=["codex", "claude", "opencode", "all"],
    )
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    clients = ["codex", "claude", "opencode"] if args.target == "all" else [args.target]
    print(f"Installing skill: {SKILL_NAME}")
    for client in clients:
        dst_root = target_dir(client)
        print(f"  [{client}] -> {dst_root}")
        copy_skill(dst_root, args.dry_run)

    print("\nDone. Restart the client, then ask it to analyze log quality.")


if __name__ == "__main__":
    main()
