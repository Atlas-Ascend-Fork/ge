#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Tests for the dependency-free deterministic L5 corpus generator."""

from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path

import pytest


# 生成器脚本本身是 ST 语料资产，留在 ST 目录；这里按仓库根目录定位它，
# 这样本用例才能被 scripts/build_fwk.sh 的 pyge 用例集收走并真正跑到。
_REPO_ROOT = Path(__file__).resolve().parents[6]
_SCRIPT = (
    _REPO_ROOT / "tests/ge/st/testcase/graph/build/dag/generate_minidag_l5_models.py"
)
_SPEC = importlib.util.spec_from_file_location("minidag_l5_generator", _SCRIPT)
assert _SPEC is not None and _SPEC.loader is not None
_GENERATOR = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_GENERATOR)


def _files(root: Path):
    return {
        path.relative_to(root).as_posix(): path.read_bytes()
        for path in sorted(root.rglob("*"))
        if path.is_file() and not path.is_symlink()
    }


def test_generation_contains_the_fixed_corpus_and_contract(tmp_path):
    output = tmp_path / "corpus"
    manifest = _GENERATOR.generate_corpus(output)
    names = [record["name"] for record in manifest["models"]]
    assert names == [
        "linear",
        "diamond",
        "multi_branch_w2",
        "multi_branch_w4",
        "multi_branch_w8",
        "deep_chain_100",
        "deep_chain_500",
        "single_node",
        "empty",
        "serial_labels",
        "special_nodes",
        "subgraph",
        "dynamic_dag",
        "dynamic_non_dag",
        "seeded_random",
    ]
    assert manifest["seed"] == _GENERATOR.DEFAULT_SEED
    assert manifest["atc_conversion_required"] is True
    assert len(manifest["config_matrix"]) == 12
    assert len(manifest["profiling_fixtures"]) == 1
    assert (
        _GENERATOR.validate_manifest(output, expected_seed=_GENERATOR.DEFAULT_SEED)
        == []
    )


def test_same_seed_is_byte_for_byte_reproducible(tmp_path):
    first = tmp_path / "first"
    second = tmp_path / "second"
    _GENERATOR.generate_corpus(first, 20260831)
    _GENERATOR.generate_corpus(second, 20260831)
    assert _files(first) == _files(second)


def test_seed_changes_the_seeded_graph_but_not_the_schema(tmp_path):
    first = _GENERATOR.generate_corpus(tmp_path / "first", 20260831)
    second = _GENERATOR.generate_corpus(tmp_path / "second", 20260901)
    first_random = next(
        item for item in first["models"] if item["name"] == "seeded_random"
    )
    second_random = next(
        item for item in second["models"] if item["name"] == "seeded_random"
    )
    assert first_random["sha256"] != second_random["sha256"]
    assert (
        first["schema_version"] == second["schema_version"] == _GENERATOR.SCHEMA_VERSION
    )


def test_validator_rejects_tampered_model_even_if_hash_is_updated(tmp_path):
    output = tmp_path / "corpus"
    manifest = _GENERATOR.generate_corpus(output)
    model_path = output / "models/linear.json"
    model = json.loads(model_path.read_text(encoding="utf-8"))
    model["graph"]["nodes"][0]["type"] = "MatMul"
    model_path.write_bytes(_GENERATOR._canonical_bytes(model))
    for record in manifest["models"]:
        if record["file"] == "models/linear.json":
            data = model_path.read_bytes()
            record["sha256"] = hashlib.sha256(data).hexdigest()
            record["size_bytes"] = len(data)
    (output / "manifest.json").write_bytes(_GENERATOR._canonical_bytes(manifest))
    errors = _GENERATOR.validate_manifest(output)
    assert any("model content is not deterministic" in error for error in errors)


def test_validator_rejects_missing_and_unlisted_files(tmp_path):
    output = tmp_path / "corpus"
    _GENERATOR.generate_corpus(output)
    (output / "models/linear.json").unlink()
    (output / "models/stale.json").write_text("{}", encoding="utf-8")
    errors = _GENERATOR.validate_manifest(output)
    assert any("manifest lists missing files" in error for error in errors)
    assert any("unlisted corpus files" in error for error in errors)


def test_validator_rejects_seed_mismatch_and_path_escape(tmp_path):
    output = tmp_path / "corpus"
    manifest = _GENERATOR.generate_corpus(output)
    manifest["seed"] = 7
    manifest["models"][0]["file"] = "../outside.json"
    (output / "manifest.json").write_bytes(_GENERATOR._canonical_bytes(manifest))
    errors = _GENERATOR.validate_manifest(output, expected_seed=_GENERATOR.DEFAULT_SEED)
    assert any("does not match expected seed" in error for error in errors)
    assert any("unsafe file path" in error for error in errors)


def test_build_corpus_rejects_non_integer_seed():
    with pytest.raises(TypeError, match="seed must be an integer"):
        _GENERATOR.build_corpus("not-a-seed")


def test_generation_rejects_a_file_as_output_path(tmp_path):
    output = tmp_path / "not_a_directory"
    output.write_text("sentinel", encoding="utf-8")
    with pytest.raises(ValueError, match="not a directory"):
        _GENERATOR.generate_corpus(output)
