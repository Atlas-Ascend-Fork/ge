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

"""Repository-level exclusion policy for non-public/internal repositories.

Excluded repository names are deliberately kept outside generated artifacts.  Callers
may report only the number of skipped targets, never the identifiers themselves.
"""

import os


SKILL_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_EXCLUDE_FILE = os.path.join(SKILL_ROOT, ".cann-log-excluded-repos")


def _values(text):
    values = []
    for line in str(text or "").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        values.extend(item.strip() for item in line.split(",") if item.strip())
    return values


def normalize(value):
    """Normalize an exact repository or repository@branch identifier."""
    value = str(value or "").strip().replace("\\", "/").rstrip("/")
    if value.lower().endswith(".git"):
        value = value[:-4]
    return value.casefold()


def default_exclude_file():
    """Return the configured private list, or the ignored local default if present."""
    configured = os.environ.get("LOG_EXCLUDE_REPOS_FILE", "").strip()
    if configured:
        return configured
    return DEFAULT_EXCLUDE_FILE if os.path.isfile(DEFAULT_EXCLUDE_FILE) else ""


def load(exclude_repos="", exclude_file=""):
    """Load exact exclusions from comma text plus an optional private local file."""
    values = _values(exclude_repos)
    if exclude_file:
        with open(exclude_file, "r", encoding="utf-8-sig") as handle:
            values.extend(_values(handle.read()))
    return frozenset(filter(None, (normalize(item) for item in values)))


def is_excluded(exclusions, repo, branch="", artifact=""):
    """Match a repository name, artifact directory, or repository@branch exactly."""
    candidates = {normalize(repo), normalize(artifact)}
    if repo and branch:
        candidates.add(normalize("%s@%s" % (repo, branch)))
    candidates.discard("")
    return bool(candidates.intersection(exclusions or ()))


def filter_dict_targets(targets, exclusions):
    included = [
        item
        for item in targets
        if not is_excluded(
            exclusions,
            item.get("repo", ""),
            item.get("branch", ""),
            item.get("dir", ""),
        )
    ]
    return included, len(targets) - len(included)


def filter_tuple_targets(targets, exclusions):
    included = [
        item
        for item in targets
        if not is_excluded(
            exclusions, item[0], item[1], os.path.basename(item[2].rstrip("/\\"))
        )
    ]
    return included, len(targets) - len(included)
