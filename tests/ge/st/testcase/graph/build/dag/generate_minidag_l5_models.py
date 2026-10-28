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

"""Generate and validate the deterministic MiniDAG L5 corpus.

The L5 comparison runs the *same* input corpus through a frozen C++ worktree
and the Python worktree.  This module deliberately has no GE/ATC dependency:
it writes canonical JSON graph descriptions and a manifest.  The JSON files
are converted to ATC-loadable models exactly once in the fixed CANN
environment, then the converted models and this manifest are shared by both
sides of the comparison.  The generator must never create fake ``.om`` files.

Generation::

    python3 generate_minidag_l5_models.py --out-dir /tmp/l5_corpus

Validation (non-zero on any missing, stale, tampered, or non-deterministic
artifact)::

    python3 generate_minidag_l5_models.py --validate --out-dir /tmp/l5_corpus

Only the Python standard library is used.  No timestamps, host paths, or
hash-randomised iteration are present in generated files.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import random
import sys
from pathlib import Path, PurePosixPath
from typing import (
    Any,
    Dict,
    Iterable,
    List,
    Mapping,
    Optional,
    Sequence,
    Set,
    Tuple,
    Union,
)


SCHEMA_VERSION = 1
DEFAULT_SEED = 20260831
MANIFEST_NAME = "manifest.json"
MODEL_DIR_NAME = "models"
PROFILING_DIR_NAME = "profiling"
MODEL_FORMAT = "ge_graph_spec_v1"
PROFILING_FILE = f"{PROFILING_DIR_NAME}/op_summary.csv"

_CONFIG_MATRIX: Tuple[Mapping[str, Any], ...] = (
    {
        "id": "default",
        "mode": "default",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {"id": "cv", "mode": "cv", "profiling_file": None, "dynamic_shape_mode": None},
    {
        "id": "load_balance_1",
        "mode": "LoadBalance:1",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "load_balance_4",
        "mode": "LoadBalance:4",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "load_balance_8",
        "mode": "LoadBalance:8",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "load_balance_64",
        "mode": "LoadBalance:64",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "main_stream_4",
        "mode": "MainStream:4",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "main_stream_8",
        "mode": "MainStream:8",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    # 加权负载均衡不再有独立策略名：给出有效 profiling 的 LoadBalance 会由
    # multistream 自动路由到加权算法，不给就是基础算法。
    {
        "id": "load_balance_profiled",
        "mode": "LoadBalance:8",
        "profiling_file": PROFILING_FILE,
        "dynamic_shape_mode": None,
    },
    {
        "id": "load_balance_unprofiled",
        "mode": "LoadBalance:8",
        "profiling_file": None,
        "dynamic_shape_mode": None,
    },
    {
        "id": "dynamic_dag_load_balance_8",
        "mode": "LoadBalance:8",
        "profiling_file": None,
        "dynamic_shape_mode": "dag",
    },
    {
        "id": "dynamic_non_dag_load_balance_8",
        "mode": "LoadBalance:8",
        "profiling_file": None,
        "dynamic_shape_mode": "non_dag",
    },
)
_STATIC_CONFIG_IDS = tuple(item["id"] for item in _CONFIG_MATRIX[:10])
_DYNAMIC_CONFIG_IDS = tuple(item["id"] for item in _CONFIG_MATRIX)


def config_matrix() -> List[Dict[str, Any]]:
    """Return a mutable copy of the complete L5 configuration matrix."""

    return [dict(item) for item in _CONFIG_MATRIX]


def _node(
    name: str, node_type: str, inputs: Iterable[str] = (), **attrs: Any
) -> Dict[str, Any]:
    return {
        "name": name,
        "type": node_type,
        "inputs": list(inputs),
        "attrs": dict(attrs),
    }


def _graph(
    name: str,
    nodes: Sequence[Mapping[str, Any]],
    *,
    outputs: Optional[Iterable[str]] = None,
    subgraphs: Iterable[Mapping[str, Any]] = (),
    dynamic_shape: bool = False,
    dynamic_stream_mode: Optional[str] = None,
) -> Dict[str, Any]:
    node_list = [
        dict(
            node, attrs=dict(node.get("attrs", {})), inputs=list(node.get("inputs", ()))
        )
        for node in nodes
    ]
    if outputs is None:
        output_list = [node_list[-1]["name"]] if node_list else []
    else:
        output_list = list(outputs)
    return {
        "name": name,
        "nodes": node_list,
        "outputs": output_list,
        "subgraphs": [dict(child) for child in subgraphs],
        "dynamic_shape": bool(dynamic_shape),
        "dynamic_stream_mode": dynamic_stream_mode,
    }


def _linear_graph(name: str, count: int) -> Dict[str, Any]:
    if isinstance(count, bool) or not isinstance(count, int) or count < 0:
        raise ValueError("count must be a non-negative integer")
    nodes: List[Dict[str, Any]] = [_node(f"{name}/data", "Data", shape=[1, 16])]
    previous = nodes[0]["name"]
    for index in range(1, count):
        node_type = "Relu" if index % 2 else "Add"
        inputs = [previous] if node_type == "Relu" else [previous, previous]
        current = _node(f"{name}/n{index:04d}", node_type, inputs)
        nodes.append(current)
        previous = current["name"]
    return _graph(name, nodes, outputs=[previous] if nodes else [])


def _diamond_graph() -> Dict[str, Any]:
    data = _node("diamond/data", "Data", shape=[1, 16])
    left = _node("diamond/left", "Relu", [data["name"]])
    right = _node("diamond/right", "Relu", [data["name"]])
    join = _node("diamond/join", "Add", [left["name"], right["name"]])
    output = _node("diamond/output", "Identity", [join["name"]])
    return _graph(
        "diamond", [data, left, right, join, output], outputs=[output["name"]]
    )


def _multi_branch_graph(width: int, depth: int) -> Dict[str, Any]:
    if (
        isinstance(width, bool)
        or isinstance(depth, bool)
        or not isinstance(width, int)
        or not isinstance(depth, int)
    ):
        raise ValueError("width and depth must be integers")
    if width < 1 or depth < 1:
        raise ValueError("width and depth must be positive")
    name = f"multi_branch_w{width}"
    data = _node(f"{name}/data", "Data", shape=[1, 16])
    nodes: List[Dict[str, Any]] = [data]
    tails: List[str] = []
    for branch in range(width):
        previous = data["name"]
        for level in range(depth):
            node_type = "Relu" if level % 2 == 0 else "Add"
            inputs = [previous] if node_type == "Relu" else [previous, previous]
            node = _node(
                f"{name}/b{branch}_l{level}",
                node_type,
                inputs,
                branch=branch,
                level=level,
            )
            nodes.append(node)
            previous = node["name"]
        tails.append(previous)
    join = _node(f"{name}/join", "Add", tails)
    output = _node(f"{name}/output", "Identity", [join["name"]])
    nodes.extend((join, output))
    return _graph(name, nodes, outputs=[output["name"]])


def _single_graph() -> Dict[str, Any]:
    data = _node("single_node/data", "Data", shape=[1, 16])
    return _graph("single_node", [data], outputs=[data["name"]])


def _serial_graph() -> Dict[str, Any]:
    data = _node("serial/data", "Data", shape=[1, 16])
    a0 = _node("serial/a0", "Relu", [data["name"]], _stream_label="serial_a")
    a1 = _node("serial/a1", "Relu", [a0["name"]], _stream_label="serial_a")
    b0 = _node("serial/b0", "Relu", [a1["name"]], _stream_label="serial_b")
    output = _node("serial/output", "NetOutput", [b0["name"]])
    return _graph("serial_labels", [data, a0, a1, b0, output], outputs=[output["name"]])


def _special_nodes_graph() -> Dict[str, Any]:
    data = _node("special/data", "Data", shape=[1, 16])
    compute = _node("special/compute", "Relu", [data["name"]])
    rts = _node("special/rts", "Identity", [compute["name"]], _rts_label_node=True)
    output = _node("special/output", "NetOutput", [rts["name"]])
    return _graph(
        "special_nodes", [data, compute, rts, output], outputs=[output["name"]]
    )


def _subgraph_graph() -> Dict[str, Any]:
    data = _node("subgraph/data", "Data", shape=[1, 16])
    call = _node(
        "subgraph/call",
        "PartitionedCall",
        [data["name"]],
        subgraph_name="subgraph/body",
    )
    output = _node("subgraph/output", "NetOutput", [call["name"]])
    child_data = _node("child/data", "Data", shape=[1, 16])
    child_relu0 = _node("child/relu0", "Relu", [child_data["name"]])
    child_relu1 = _node("child/relu1", "Relu", [child_relu0["name"]])
    child_output = _node("child/output", "NetOutput", [child_relu1["name"]])
    body = _graph(
        "subgraph/body",
        [child_data, child_relu0, child_relu1, child_output],
        outputs=[child_output["name"]],
    )
    return _graph(
        "subgraph", [data, call, output], outputs=[output["name"]], subgraphs=[body]
    )


def _dynamic_graph(mode: str) -> Dict[str, Any]:
    if mode not in ("dag", "non_dag"):
        raise ValueError("dynamic mode must be 'dag' or 'non_dag'")
    name = f"dynamic_{mode}"
    data = _node(f"{name}/data", "Data", dynamic=True, shape=[-1, 16])
    relu = _node(f"{name}/relu", "Relu", [data["name"]])
    output = _node(f"{name}/output", "NetOutput", [relu["name"]])
    return _graph(
        name,
        [data, relu, output],
        outputs=[output["name"]],
        dynamic_shape=True,
        dynamic_stream_mode=mode,
    )


def _random_graph(rng: random.Random) -> Dict[str, Any]:
    """Build a seeded DAG while retaining a fixed, small operator template."""

    name = "seeded_random"
    data = _node(f"{name}/data", "Data", shape=[1, 16])
    nodes: List[Dict[str, Any]] = [data]
    for index in range(1, 25):
        node_type = rng.choice(("Relu", "Add"))
        predecessor_count = rng.randint(1, min(3, len(nodes)))
        predecessors = rng.sample([node["name"] for node in nodes], predecessor_count)
        node = _node(f"{name}/n{index:02d}", node_type, predecessors, seed_slot=index)
        nodes.append(node)
    return _graph(name, nodes, outputs=[nodes[-1]["name"]])


def build_corpus(seed: int = DEFAULT_SEED) -> List[Dict[str, Any]]:
    """Return the complete deterministic graph corpus for *seed*."""

    if isinstance(seed, bool) or not isinstance(seed, int):
        raise TypeError("seed must be an integer")
    graphs = [
        _linear_graph("linear", 8),
        _diamond_graph(),
        _multi_branch_graph(2, 3),
        _multi_branch_graph(4, 3),
        _multi_branch_graph(8, 3),
        _linear_graph("deep_chain_100", 100),
        _linear_graph("deep_chain_500", 500),
        _single_graph(),
        _graph("empty", []),
        _serial_graph(),
        _special_nodes_graph(),
        _subgraph_graph(),
        _dynamic_graph("dag"),
        _dynamic_graph("non_dag"),
        _random_graph(random.Random(seed)),
    ]
    return [_make_model(graph, seed) for graph in graphs]


def _make_model(graph: Mapping[str, Any], seed: int) -> Dict[str, Any]:
    direct_nodes = []
    for node in graph.get("nodes", ()):
        direct_nodes.append({"name": node["name"], "type": node["type"]})
    dynamic_shape = bool(graph.get("dynamic_shape", False))
    return {
        "schema_version": SCHEMA_VERSION,
        "model_format": MODEL_FORMAT,
        "model_name": graph["name"],
        "seed": seed,
        "graph": dict(graph),
        "direct_nodes": direct_nodes,
        "direct_node_names": [node["name"] for node in direct_nodes],
        "direct_node_types": [node["type"] for node in direct_nodes],
        "has_subgraph": bool(graph.get("subgraphs")),
        "dynamic_shape": dynamic_shape,
        "dynamic_stream_mode": graph.get("dynamic_stream_mode"),
        "config_matrix": list(
            _DYNAMIC_CONFIG_IDS if dynamic_shape else _STATIC_CONFIG_IDS
        ),
    }


def _canonical_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)
        + "\n"
    ).encode("utf-8")


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _profiling_csv(models: Sequence[Mapping[str, Any]]) -> bytes:
    """Build a deterministic profiling fixture accepted by MiniDAG's parser."""

    task_types = ("AI_CORE", "AI_VECTOR_CORE", "MIX_AIC", "MIX_AIV")
    rows = ["Op Name,Task Type,Task Duration(us),Block Num,Mix Block Num"]
    offset = 0
    for model in models:
        for index, node in enumerate(model.get("direct_nodes", ())):
            ordinal = offset + index
            rows.append(
                f"{node['name']},{task_types[ordinal % len(task_types)]},{ordinal + 1:.6f},"
                f"{ordinal % 4 + 1},{(ordinal + 1) % 4 + 1}"
            )
        offset += len(model.get("direct_nodes", ()))
    return ("\n".join(rows) + "\n").encode("utf-8")


def _file_record(path: str, data: bytes, **extra: Any) -> Dict[str, Any]:
    record: Dict[str, Any] = {
        "file": path,
        "sha256": _sha256(data),
        "size_bytes": len(data),
    }
    record.update(extra)
    return record


def _manifest_for(
    models: Sequence[Mapping[str, Any]], seed: int, model_bytes: Mapping[str, bytes]
) -> Dict[str, Any]:
    profiling_data = _profiling_csv(models)
    model_records: List[Dict[str, Any]] = []
    for model in models:
        name = model["model_name"]
        path = f"{MODEL_DIR_NAME}/{name}.json"
        model_records.append(
            _file_record(
                path,
                model_bytes[path],
                name=name,
                model_format=model["model_format"],
                direct_nodes=model["direct_nodes"],
                direct_node_names=model["direct_node_names"],
                direct_node_types=model["direct_node_types"],
                has_subgraph=model["has_subgraph"],
                dynamic_shape=model["dynamic_shape"],
                dynamic_stream_mode=model["dynamic_stream_mode"],
                config_matrix=model["config_matrix"],
            )
        )
    return {
        "schema_version": SCHEMA_VERSION,
        "generator": Path(__file__).name,
        "corpus_kind": "minidag_l5_deterministic_graph_specs",
        "model_format": MODEL_FORMAT,
        "seed": seed,
        "atc_conversion_required": True,
        "atc_conversion_note": (
            "JSON graph specs are the canonical source. "
            "Convert once with the fixed GE/ATC environment "
            "and share the resulting models with both worktrees."
        ),
        "config_matrix": config_matrix(),
        "profiling_fixtures": [
            _file_record(PROFILING_FILE, profiling_data, kind="op_summary")
        ],
        "models": model_records,
    }


def _write_bytes(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def generate_corpus(
    out_dir: Union[str, Path], seed: int = DEFAULT_SEED
) -> Dict[str, Any]:
    """Write a deterministic corpus and return its generated manifest."""

    if isinstance(seed, bool) or not isinstance(seed, int):
        raise TypeError("seed must be an integer")
    out_path = Path(out_dir)
    if out_path.exists() and not out_path.is_dir():
        raise ValueError(f"output path is not a directory: {out_path}")
    out_path.mkdir(parents=True, exist_ok=True)
    models = build_corpus(seed)
    model_bytes = {
        f"{MODEL_DIR_NAME}/{model['model_name']}.json": _canonical_bytes(model)
        for model in models
    }
    for relative, data in model_bytes.items():
        _write_bytes(out_path / relative, data)
    profiling_data = _profiling_csv(models)
    _write_bytes(out_path / PROFILING_FILE, profiling_data)
    manifest = _manifest_for(models, seed, model_bytes)
    _write_bytes(out_path / MANIFEST_NAME, _canonical_bytes(manifest))
    return manifest


def _safe_relative_path(value: Any) -> Optional[str]:
    if not isinstance(value, str) or not value or "\\" in value:
        return None
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in path.parts):
        return None
    return path.as_posix()


def _read_json(path: Path) -> Tuple[Optional[Any], Optional[str]]:
    try:
        return json.loads(path.read_text(encoding="utf-8")), None
    except (OSError, ValueError, UnicodeDecodeError) as exc:
        return None, str(exc)


def validate_manifest(
    out_dir: Union[str, Path],
    *,
    expected_seed: Optional[int] = None,
    manifest_name: str = MANIFEST_NAME,
) -> List[str]:
    """Validate every corpus artifact and deterministic metadata invariant."""

    out_path = Path(out_dir)
    if not out_path.is_dir():
        return [f"output path is not a directory: {out_path}"]
    manifest_path = out_path / manifest_name
    manifest, read_error = _read_json(manifest_path)
    if read_error:
        return [f"cannot read manifest: {read_error}"]
    if not isinstance(manifest, Mapping):
        return ["manifest must be a JSON object"]

    errors: List[str] = []
    seed = manifest.get("seed")
    if (
        isinstance(expected_seed, int)
        and not isinstance(expected_seed, bool)
        and seed != expected_seed
    ):
        errors.append(
            f"manifest seed {seed!r} does not match expected seed {expected_seed}"
        )
    if not isinstance(seed, int) or isinstance(seed, bool):
        errors.append("manifest seed must be an integer")
        return errors
    expected_models = build_corpus(seed)
    expected_by_path = {
        f"{MODEL_DIR_NAME}/{model['model_name']}.json": model
        for model in expected_models
    }
    expected_bytes = {}
    for path, model in expected_by_path.items():
        expected_bytes[path] = _canonical_bytes(model)
    expected_profile = _profiling_csv(expected_models)
    expected_files = set(expected_bytes) | {PROFILING_FILE, manifest_name}

    if manifest.get("schema_version") != SCHEMA_VERSION:
        errors.append("manifest schema_version is unsupported")
    if manifest.get("model_format") != MODEL_FORMAT:
        errors.append("manifest model_format is unsupported")
    if manifest.get("config_matrix") != config_matrix():
        errors.append("manifest config_matrix does not match the generator")

    listed_files: Set[str] = {manifest_name}
    for section, records in (
        ("models", manifest.get("models")),
        ("profiling_fixtures", manifest.get("profiling_fixtures")),
    ):
        if not isinstance(records, list):
            errors.append(f"manifest {section} must be a list")
            continue
        for record in records:
            if not isinstance(record, Mapping):
                errors.append(f"manifest {section} contains a non-object record")
                continue
            relative = _safe_relative_path(record.get("file"))
            if relative is None:
                errors.append(f"unsafe file path: {record.get('file')!r}")
                continue
            listed_files.add(relative)
            target = out_path / relative
            try:
                target.resolve(strict=False).relative_to(out_path.resolve())
            except ValueError:
                # A symlinked directory component can escape the corpus root
                # even when the lexical path contains no ``..`` segment.
                errors.append(f"unsafe file path: {relative}")
                continue
            if not target.is_file() or target.is_symlink():
                errors.append(f"{section[:-1]} is missing or symlinked: {relative}")
                continue
            try:
                data = target.read_bytes()
            except OSError as exc:
                errors.append(f"cannot read {relative}: {exc}")
                continue
            if record.get("sha256") != _sha256(data):
                errors.append(f"SHA256 mismatch: {relative}")
            if record.get("size_bytes") != len(data):
                errors.append(f"size mismatch: {relative}")
            if section == "models":
                expected = expected_bytes.get(relative)
                if expected is None:
                    errors.append(f"unexpected model file: {relative}")
                elif data != expected:
                    errors.append(f"model content is not deterministic: {relative}")
            elif relative == PROFILING_FILE and data != expected_profile:
                errors.append(
                    f"profiling fixture content is not deterministic: {relative}"
                )

    listed_model_paths = {
        _safe_relative_path(record.get("file"))
        for record in (
            manifest.get("models") if isinstance(manifest.get("models"), list) else []
        )
        if isinstance(record, Mapping)
    }
    missing_models = sorted(set(expected_bytes) - listed_model_paths)
    if missing_models:
        errors.append("manifest lists missing files: " + ", ".join(missing_models))
    if PROFILING_FILE not in listed_files:
        errors.append("manifest lists missing files: " + PROFILING_FILE)

    actual_files = {
        path.relative_to(out_path).as_posix()
        for path in out_path.rglob("*")
        # Keep symlinks in the inventory so an unlisted link cannot hide an
        # extra corpus artifact.  Listed symlinks are rejected above.
        if path.is_file()
    }
    unexpected_files = sorted(actual_files - expected_files)
    if unexpected_files:
        errors.append("unlisted corpus files: " + ", ".join(unexpected_files))
    missing_files = sorted(expected_files - actual_files)
    if missing_files:
        errors.append("manifest lists missing files: " + ", ".join(missing_files))
    return errors


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--out-dir", required=True, type=Path, help="corpus output/validation directory"
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=DEFAULT_SEED,
        help=f"deterministic seed (default: {DEFAULT_SEED})",
    )
    parser.add_argument(
        "--validate",
        action="store_true",
        help="validate an existing corpus instead of generating it",
    )
    parser.add_argument(
        "--manifest",
        default=MANIFEST_NAME,
        help=f"manifest filename (default: {MANIFEST_NAME})",
    )
    args = parser.parse_args(argv)
    if args.validate:
        errors = validate_manifest(
            args.out_dir, expected_seed=args.seed, manifest_name=args.manifest
        )
        if errors:
            for error in errors:
                print(f"L5 corpus validation: {error}", file=sys.stderr)
            return 1
        print(f"L5 corpus validation passed: {args.out_dir}")
        return 0
    try:
        manifest = generate_corpus(args.out_dir, args.seed)
    except (OSError, TypeError, ValueError) as exc:
        print(f"L5 corpus generation failed: {exc}", file=sys.stderr)
        return 1
    print(
        f"L5 corpus generated: {args.out_dir} ({len(manifest['models'])} models, seed={args.seed})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
