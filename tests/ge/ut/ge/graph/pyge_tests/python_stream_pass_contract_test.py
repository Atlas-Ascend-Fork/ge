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

"""Pure-Python contract checks for the GE stream-pass adapter boundary.

The bridge type-checks its callback arguments with a plain ``isinstance``:
``ge.graph.Graph`` for the graph and ``StreamPassContext`` for the context.
Neither native type can be constructed from Python, so these tests swap the
type objects the bridge compares against instead of relaxing the check in the
product code.  That keeps the design's gate meaningful -- an unregistered
pybind derived class arrives as a plain ``PassContext`` and must be rejected.
"""

import importlib
from unittest import mock

import pytest


try:
    adapter = importlib.import_module("ge.passes._minidag_adapter")
except ImportError as exc:
    pytest.skip(f"无法导入 GE MiniDAG 适配器: {exc}", allow_module_level=True)

try:
    stream_pass = importlib.import_module("ge.passes._minidag_stream_pass")
except ImportError:
    # The normal GE test installation provides the native pass artifact.  A
    # source-tree run without that optional artifact skips the bridge tests.
    stream_pass = None

try:
    bridge = importlib.import_module("ge.passes._bridge")
except ImportError:
    bridge = None


requires_bridge = pytest.mark.skipif(
    bridge is None or stream_pass is None, reason="GE native pass bridge unavailable"
)


class FakeGraph:
    """Stand-in for ``ge.graph.Graph`` during a patched bridge call."""

    def get_direct_nodes(self):
        return []

    def get_all_subgraphs(self):
        return []


class FakeStreamPassContext:
    """Stand-in for ``StreamPassContext`` during a patched bridge call."""

    def __init__(self):
        self.error = ""

    def allocate_next_stream_id(self):
        return 0

    def get_curr_max_stream_id(self):
        return 0

    def get_stream_id(self, _graph, _node_index):
        return -1

    def set_stream_id(self, _graph, _node_index, _stream_id):
        pass

    def set_error_message(self, message):
        self.error = message


class BaseOnlyContext:
    """A context carrying none of the stream-specific methods.

    This is what the bridge receives when the pybind derived class is not
    registered: the object is a plain ``PassContext``.
    """

    def __init__(self):
        self.error = ""

    def set_error_message(self, message):
        self.error = message


def _patch_graph_type():
    """Make the bridge's ``isinstance(graph, ge.graph.Graph)`` accept FakeGraph."""

    return mock.patch("ge.graph.Graph", FakeGraph)


def _patch_context_type():
    """Make the bridge's context check accept FakeStreamPassContext."""

    return mock.patch.object(bridge, "StreamPassContext", FakeStreamPassContext)


def _register_pass(name, run_body):
    @stream_pass.register_resource_plan_pass(
        name=name,
        pass_stage=stream_pass.PassStage.AFTER_ASSIGN_LOGIC_STREAM,
        descriptor_kind="stream",
    )
    class ContractPass(stream_pass.StreamPass):
        def run(self, graph, context):
            return run_body(graph, context)

    return ContractPass


def _install_holder(instance_id, pass_cls):
    bridge._PASS_HOLDERS[instance_id] = bridge._PassHolder(
        descriptor_key=pass_cls.__ge_pass_descriptor__.descriptor_key,
        instance_id=instance_id,
        instance=pass_cls(),
    )


@requires_bridge
def test_bridge_passes_public_graph_and_stream_context_when_pass_raises():
    from ge.passes.registry import clear_registered_passes

    seen = {}

    def run_body(graph, context):
        seen["graph"] = graph
        seen["context"] = context
        raise RuntimeError("contract failure")

    pass_cls = _register_pass("StreamContractRaisingPass", run_body)
    instance_id = "stream-contract-raising"
    _install_holder(instance_id, pass_cls)

    graph = FakeGraph()
    context = FakeStreamPassContext()
    try:
        with _patch_graph_type(), _patch_context_type():
            with pytest.raises(RuntimeError, match="contract failure"):
                bridge.run_resource_plan_pass(instance_id, graph, context)
    finally:
        bridge._PASS_HOLDERS.pop(instance_id, None)
        clear_registered_passes()
    assert seen["graph"] is graph
    assert seen["context"] is context
    assert context.error == "contract failure"


@requires_bridge
def test_bridge_rejects_context_without_stream_pass_context_type():
    """Design gate: the callback must receive a StreamPassContext.

    An unregistered pybind derived class surfaces as a plain PassContext.
    The pass must never be entered with it.
    """

    from ge.passes.registry import clear_registered_passes

    entered = []
    pass_cls = _register_pass(
        "StreamContractBaseContextPass", lambda graph, context: entered.append(context)
    )
    instance_id = "stream-contract-base-context"
    _install_holder(instance_id, pass_cls)

    context = BaseOnlyContext()
    try:
        with _patch_graph_type(), _patch_context_type():
            with pytest.raises(TypeError, match="must be a StreamPassContext"):
                bridge.run_resource_plan_pass(instance_id, FakeGraph(), context)
    finally:
        bridge._PASS_HOLDERS.pop(instance_id, None)
        clear_registered_passes()
    assert entered == []
    assert "StreamPassContext" in context.error


@requires_bridge
def test_bridge_rejects_context_that_only_looks_like_a_stream_context():
    """A method-name match is not enough; the concrete type decides.

    A future resource-plan kind may well expose the same method names, so the
    stream handler must not accept an object merely because it quacks right.
    """

    from ge.passes.registry import clear_registered_passes

    class LookalikeContext(FakeStreamPassContext):
        pass

    entered = []
    pass_cls = _register_pass(
        "StreamContractLookalikePass", lambda graph, context: entered.append(context)
    )
    instance_id = "stream-contract-lookalike"
    _install_holder(instance_id, pass_cls)

    # Patch only the graph type: the bridge still compares the context against
    # the real StreamPassContext, which LookalikeContext is not.
    try:
        with _patch_graph_type():
            with pytest.raises(TypeError, match="must be a StreamPassContext"):
                bridge.run_resource_plan_pass(
                    instance_id, FakeGraph(), LookalikeContext()
                )
    finally:
        bridge._PASS_HOLDERS.pop(instance_id, None)
        clear_registered_passes()
    assert entered == []


@requires_bridge
def test_bridge_rejects_graph_that_is_not_a_ge_graph():
    from ge.passes.registry import clear_registered_passes

    entered = []
    pass_cls = _register_pass(
        "StreamContractBadGraphPass", lambda graph, context: entered.append(graph)
    )
    instance_id = "stream-contract-bad-graph"
    _install_holder(instance_id, pass_cls)

    context = FakeStreamPassContext()
    try:
        with _patch_context_type():
            with pytest.raises(TypeError, match="must be a ge.graph.Graph"):
                bridge.run_resource_plan_pass(instance_id, object(), context)
    finally:
        bridge._PASS_HOLDERS.pop(instance_id, None)
        clear_registered_passes()
    assert entered == []


@requires_bridge
def test_bridge_rejects_missing_resource_plan_holder():
    context = FakeStreamPassContext()
    with pytest.raises(KeyError, match="not created"):
        bridge.run_resource_plan_pass("missing-stream-contract", object(), context)
