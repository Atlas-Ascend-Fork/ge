#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software; you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Cross-process coverage for a prototype declared in a file of its own.

The producer process loads a plugin whose ``register_op`` declaration lives in
``*_proto.py`` while the ``register_op_impl`` class lives in ``*_impl.py``, then
freezes the artifacts exactly as OM saving does.  The consumer is a *different*
process which never ran that plugin: it only sees the frozen artifacts, executes
the embedded sources inside a model scope, and must still shape-infer.

This is the defect shape the feature closes -- capability bits can be backfilled
while ``infer_func`` stays unreachable, because the process-global proto
registry is empty by design (rule D4) in a deployment process and the
declaration only survives inside the restore scope.
"""

import base64
import importlib
import json
import subprocess
import sys
import textwrap

import pytest

try:
    bootstrap = importlib.import_module("ge.custom_op.bootstrap")
    bridge = importlib.import_module("ge.custom_op._bridge")
    custom_op = importlib.import_module("ge.custom_op")
    deployment = importlib.import_module("ge.custom_op._deployment")
    impl_registry = importlib.import_module("ge.custom_op.registry")
    proto_registry = importlib.import_module("ge.custom_op.proto")
    scope_mod = importlib.import_module("ge.custom_op._scope")
    runtime_native = importlib.import_module("ge.runtime._native")
    graph = importlib.import_module("ge.graph")
except ImportError as exc:  # pragma: no cover - environment without the package
    pytest.skip(f"无法导入 Python custom op 相关模块: {exc}", allow_module_level=True)

DataType = graph.DataType

SPLIT_OP_TYPE = "SplitProtoShapeInferOp"
PROTO_SOURCE_NAME = "split_proto_plugin.py"
IMPL_SOURCE_NAME = "split_impl_plugin.py"

# 声明与实现分居两个文件：部署侧若不给原型文件独立 artifact，声明必然丢失。
PROTO_BODY = """
from ge.custom_op import register_op
from ge.graph import DataType
from ge.runtime import TensorDesc


@register_op(op_type="SplitProtoShapeInferOp")
def infer_meta(x: TensorDesc) -> TensorDesc:
    # 推导结果必须与恒等推导可区分（输入 [3, 5] -> 输出 [3, 7]），
    # 否则无法证明真正跑的是这个文件里的 infer_func。
    dims = list(x.shape.origin_shape.dims)
    dims[-1] = 7
    return TensorDesc(dims, DataType.DT_INT32)
"""

IMPL_BODY = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="SplitProtoShapeInferOp")
class SplitProtoShapeInferOp:
    def execute(self, ctx):
        _ = ctx
"""

# 生产进程：导入插件 -> 冻结 artifact -> 以 JSON 交递（source 走 base64）。
PRODUCER = """
import base64, json, sys
sys.path.insert(0, {plugin_dir!r})
import split_proto_plugin  # noqa: F401
import split_impl_plugin  # noqa: F401
from ge.custom_op import bootstrap

print(json.dumps([
    {{
        "artifact_id": item["artifact_id"],
        "source_name": item["source_name"],
        "source": base64.b64encode(item["source"]).decode("ascii"),
        "descriptors": item["descriptors"],
    }}
    for item in bootstrap.freeze_deployment_artifacts()
]))
"""


class FakeInferMetaContext:
    """Single-required-input, single-required-output native ctx stand-in."""

    def __init__(self, ir_inputs, ir_outputs):
        self._ir_inputs = ir_inputs
        self._ir_outputs = ir_outputs
        self._invalidated = False

    def _get_required_input_tensor(self, ir_index):
        assert ir_index == 0
        return runtime_native.TensorDesc(
            runtime_native.StorageShape([3, 5], [3, 5]), DataType.DT_FLOAT
        )

    def _get_optional_input_tensor(self, ir_index):
        raise AssertionError(f"unexpected optional input at {ir_index}")

    def _get_dynamic_input_num(self, ir_index):
        raise AssertionError(f"unexpected dynamic input at {ir_index}")

    def _get_dynamic_input_tensor(self, ir_index, relative_index):
        raise AssertionError(f"unexpected dynamic input {ir_index}/{relative_index}")

    def _get_attrs(self):
        raise AssertionError("unexpected attrs lookup")

    def _get_dynamic_output_num(self, ir_index):
        raise AssertionError(f"unexpected dynamic output at {ir_index}")

    def _invalidate(self):
        self._invalidated = True


def _make_ir_meta():
    return {
        "inputs": [{"name": "x", "kind": int(proto_registry.InputType.REQUIRED)}],
        "outputs": [{"name": "y", "kind": int(proto_registry.OutputType.REQUIRED)}],
        "attrs": [],
    }


@pytest.fixture(autouse=True)
def clean_state():
    custom_op.clear_registered_op_impls()
    proto_registry.clear_registered_op_protos()
    deployment.clear_frozen_artifacts()
    scope_mod.reset_model_scopes_for_test()
    yield
    scope_mod.reset_model_scopes_for_test()
    deployment.clear_frozen_artifacts()
    proto_registry.clear_registered_op_protos()
    custom_op.clear_registered_op_impls()


def _freeze_in_producer_process(tmp_path):
    """Freeze the split plugin in a separate interpreter and return its artifacts."""

    plugin_dir = tmp_path / "plugin"
    plugin_dir.mkdir()
    (plugin_dir / PROTO_SOURCE_NAME).write_text(
        textwrap.dedent(PROTO_BODY), encoding="utf-8"
    )
    (plugin_dir / IMPL_SOURCE_NAME).write_text(
        textwrap.dedent(IMPL_BODY), encoding="utf-8"
    )
    completed = subprocess.run(
        [sys.executable, "-c", PRODUCER.format(plugin_dir=str(plugin_dir))],
        capture_output=True,
        text=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    return json.loads(completed.stdout)


def _to_bridge_sources(frozen, source_names=None):
    return [
        {
            "artifact_id": item["artifact_id"],
            "source_name": item["source_name"],
            "source": base64.b64decode(item["source"]),
        }
        for item in frozen
        if source_names is None or item["source_name"] in source_names
    ]


def test_producer_freezes_split_prototype_as_its_own_artifact(tmp_path):
    frozen = _freeze_in_producer_process(tmp_path)

    # 分文件原型必须独立成 artifact，否则部署进程根本没有声明可重放
    assert sorted(item["source_name"] for item in frozen) == sorted(
        [IMPL_SOURCE_NAME, PROTO_SOURCE_NAME]
    )
    by_source = {item["source_name"]: item for item in frozen}
    proto_entry = by_source[PROTO_SOURCE_NAME]["descriptors"][0]
    assert proto_entry["op_type"] == SPLIT_OP_TYPE
    assert proto_entry["class_name"] == "infer_meta"
    # 原型条目的固定位组合是 C++ 侧 IsProtoDescriptor 的唯一判定依据
    assert proto_entry["capabilities"] == deployment._PROTO_CAPABILITIES
    assert proto_entry["capabilities"] == (
        deployment.CAPABILITY_SHAPE_INFER | deployment.CAPABILITY_INFER_META
    )
    impl_entries = by_source[IMPL_SOURCE_NAME]["descriptors"]
    assert [item["op_type"] for item in impl_entries] == [SPLIT_OP_TYPE]
    assert impl_entries[0]["capabilities"] == deployment.CAPABILITY_EAGER_EXECUTE


def test_consumer_process_restores_infer_func_from_embedded_prototype(tmp_path):
    frozen = _freeze_in_producer_process(tmp_path)

    # 消费进程从未导入过插件：两处进程级注册表都是空的
    assert proto_registry.get_registered_op_proto_by_op_type(SPLIT_OP_TYPE) is None
    assert impl_registry.get_registered_op_impls() == []

    descriptors = bridge.restore_model_scope(1, _to_bridge_sources(frozen))
    assert [item["op_type"] for item in descriptors] == [SPLIT_OP_TYPE]

    # D4：恢复不写进程级原型表，声明只活在 scope 里
    assert proto_registry.get_registered_op_proto_by_op_type(SPLIT_OP_TYPE) is None
    scoped_proto = scope_mod.find_scoped_proto(SPLIT_OP_TYPE)
    assert scoped_proto is not None
    assert scoped_proto.infer_func.__name__ == "infer_meta"

    ir_meta = _make_ir_meta()
    ctx = FakeInferMetaContext(ir_meta["inputs"], ir_meta["outputs"])
    outputs = bridge.call_infer_meta(SPLIT_OP_TYPE, ir_meta, ctx)

    # infer_func 真跑过：[3, 7] / DT_INT32 只可能来自原型文件里的那份实现，
    # 恒等推导会得到 [3, 5] / DT_FLOAT。
    assert outputs == [([3, 7], [3, 7], int(DataType.DT_INT32))]
    assert ctx._invalidated is True


def test_consumer_infer_meta_fails_loudly_without_reachable_prototype(tmp_path):
    frozen = _freeze_in_producer_process(tmp_path)
    # 只恢复实现文件：声明不可达，必须点名失败而不是静默给出错误 shape
    bridge.restore_model_scope(2, _to_bridge_sources(frozen, {IMPL_SOURCE_NAME}))

    assert scope_mod.find_scoped_proto(SPLIT_OP_TYPE) is None
    ir_meta = _make_ir_meta()
    ctx = FakeInferMetaContext(ir_meta["inputs"], ir_meta["outputs"])
    with pytest.raises(RuntimeError, match="canonical python prototype not found"):
        bridge.call_infer_meta(SPLIT_OP_TYPE, ir_meta, ctx)
