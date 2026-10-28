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

"""Graph-consistency gates for ``DAGAdapter``.

Dropping the bulk write-back API removed the interface-level guard that used
to catch a mismatch by comparing list lengths.  ``write_back`` took that job
over with three checks, and this file is what holds it to them: a subgraph's
result must never be written into the root graph, and a graph mutated between
``plan`` and ``write_back`` must be rejected before a single stream id is
written.

Node identity comes from the underlying graph-engine node.  ``FakeNode`` keeps
the older synthetic handle fallback because a real ``ge.graph.Node`` wrapper is
rebuilt on every edge query.
"""

import importlib
import sys
import types

import pytest


from _minidag_test_utils import install_fake_multistream

try:
    adapter_mod = importlib.import_module("ge.passes._minidag_adapter")
except ImportError as exc:
    pytest.skip(f"无法导入 GE MiniDAG 适配器: {exc}", allow_module_level=True)

# multistream 属于 graph-autofusion，GE 的 UT 不依赖该仓：适配器在校验任何入参
# 之前就会加载 multistream，所以这里统一换成仓内桩，本文件和从本文件取夹具的
# profiling 用例都用它。真包的联调在 L4。
fake_multistream = install_fake_multistream()

DAGAdapter = adapter_mod.DAGAdapter


class _Handle:
    __slots__ = ("value",)
    _next = 1

    def __init__(self):
        self.value = _Handle._next
        _Handle._next += 1


class FakeNode:
    def __init__(
        self,
        name,
        node_type="Op",
        stream_label="",
        is_rts=False,
        origin=0,
        subgraph_instances=(),
    ):
        self._handle = _Handle()
        self.name = name
        self.type = node_type
        self._attrs = {"_stream_label": stream_label, "_rts_label_node": is_rts}
        self.origin_stream_id = origin
        self._subgraph_instances = subgraph_instances
        self._data_outputs = {}
        self._control_outputs = []

    def has_attr(self, key):
        return key in self._attrs

    def get_attr(self, key):
        if key not in self._attrs:
            raise RuntimeError(f"attribute {key!r} does not exist")
        return self._attrs[key]

    def get_outputs_size(self):
        return max(self._data_outputs, default=-1) + 1

    def get_out_data_nodes_and_port_indexes(self, index):
        return list(self._data_outputs.get(index, ()))

    def get_out_control_nodes(self):
        return list(self._control_outputs)

    def _has_subgraph_instances(self):
        return bool(self._subgraph_instances)


class FakeGraph:
    def __init__(self, name, nodes, data_edges=(), attrs=None):
        self.name = name
        self._nodes = list(nodes)
        # 真实 Graph 只有 get_attr，属性不存在时抛 RuntimeError，这里照此模拟。
        self._attrs = dict(attrs or {})
        for src, src_port, dst, dst_port in data_edges:
            self._nodes[src]._data_outputs.setdefault(src_port, []).append(
                (self._nodes[dst], dst_port)
            )

    def get_attr(self, key):
        if key not in self._attrs:
            raise RuntimeError(f"attribute {key!r} does not exist")
        return self._attrs[key]

    def get_direct_nodes(self):
        return list(self._nodes)

    def get_all_subgraphs(self):
        return []


class FakeContext:
    def __init__(self, next_stream_id=0):
        self._next = next_stream_id
        self.writes = []

    def allocate_next_stream_id(self):
        value = self._next
        self._next += 1
        return value

    def get_curr_max_stream_id(self):
        return self._next - 1

    def get_stream_id(self, graph, node_index):
        return graph.get_direct_nodes()[node_index].origin_stream_id

    def set_stream_id(self, graph, node_index, stream_id):
        self.writes.append((node_index, stream_id))

    def set_error_message(self, message):
        self.error = message


def _linear_graph(name="g", count=4):
    nodes = [FakeNode(f"{name}_n{i}") for i in range(count)]
    edges = [(i, 0, i + 1, 0) for i in range(count - 1)]
    return FakeGraph(name, nodes, edges)


def _run_plan(adapter, graph, context, base=0):
    return adapter.plan(
        graph,
        context,
        max_stream_num=8,
        base_stream_id=base,
        merge_strategy="LoadBalance",
    )


# ── 正面：plan -> 预留 -> write_back 走通 ─────────────────────────


def test_plan_then_write_back_assigns_every_eligible_node():
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    required = _run_plan(adapter, graph, context)
    assert required >= 1
    adapter.write_back(graph, context)
    assert len(context.writes) == 4
    # Indices are positions in the graph's direct-node list, in order.
    assert [index for index, _ in context.writes] == [0, 1, 2, 3]


# ── 校验 1：没算过就写 ────────────────────────────────────────────────


def test_write_back_before_plan_is_rejected():
    graph, context = _linear_graph(), FakeContext()
    with pytest.raises(RuntimeError, match="called before plan"):
        DAGAdapter().write_back(graph, context)
    assert context.writes == []


def test_write_back_after_a_failed_plan_is_rejected():
    """A failed plan must not leave a plan a later write_back can use."""

    adapter, context = DAGAdapter(), FakeContext()
    bad_graph = _linear_graph()
    with pytest.raises((ValueError, TypeError, RuntimeError)):
        adapter.plan(
            bad_graph,
            context,
            max_stream_num=0,  # out of [1, 64]
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )
    with pytest.raises(RuntimeError, match="called before plan"):
        adapter.write_back(bad_graph, context)
    assert context.writes == []


# ── 校验 2：换了一张图（子图结果写根图就撞在这里）────────────────────


def test_write_back_into_a_different_graph_is_rejected():
    root, subgraph = _linear_graph("root"), _linear_graph("sub")
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, subgraph, context)
    with pytest.raises(RuntimeError, match="does not match the graph used for plan"):
        adapter.write_back(root, context)
    assert context.writes == []


def test_identity_not_node_count_decides_the_graph_check():
    """Two graphs with the same node count must still be told apart.

    The bulk API compared list lengths, which could not see this case; the
    adapter compares object identity, which can.
    """

    first, second = _linear_graph("a"), _linear_graph("b")
    assert len(first.get_direct_nodes()) == len(second.get_direct_nodes())
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, first, context)
    with pytest.raises(RuntimeError, match="does not match the graph used for plan"):
        adapter.write_back(second, context)
    assert context.writes == []


# ── 校验 3：图在两步之间被改过 ───────────────────────────────────────


def test_write_back_after_a_node_was_added_is_rejected():
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    graph._nodes.append(FakeNode("late_arrival"))
    with pytest.raises(RuntimeError, match="node count changed"):
        adapter.write_back(graph, context)
    assert context.writes == []


def test_write_back_after_a_node_was_removed_is_rejected():
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    graph._nodes.pop()
    with pytest.raises(RuntimeError, match="node count changed"):
        adapter.write_back(graph, context)
    assert context.writes == []


# ── 写回过滤（§6.3 的 ②③④）────────────────────────────────────────


def test_net_output_and_rts_label_nodes_are_filtered():
    nodes = [
        FakeNode("compute"),
        FakeNode("out", node_type="NetOutput"),
        FakeNode("label", is_rts=True),
    ]
    graph = FakeGraph("g", nodes, [(0, 0, 1, 0), (0, 0, 2, 0)])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    adapter.write_back(graph, context)
    assert [index for index, _ in context.writes] == [0]


@pytest.mark.parametrize(
    "node_type", ["If", "Case", "While", "PartitionedCall", "CustomSubgraphCall"]
)
def test_write_back_preserves_subgraph_control_stream(node_type):
    """PR 5584: a call must stay with its RTS labels when compute nodes move."""
    nodes = [
        FakeNode(
            "call", node_type=node_type, origin=1, subgraph_instances=("body_instance",)
        ),
        FakeNode("label", node_type="LabelSwitchByIndex", is_rts=True, origin=1),
        FakeNode("compute", node_type="Add", origin=1),
    ]
    graph = FakeGraph("subgraph_control_stream", nodes)
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context, base=4)
    assert all(adapter._plan.get_stream_id(node) >= 4 for node in adapter._nodes)

    adapter.write_back(graph, context)

    assert [index for index, _ in context.writes] == [2]
    streams = [node.origin_stream_id for node in nodes]
    for index, stream in context.writes:
        streams[index] = stream
    assert streams[:2] == [1, 1]
    assert streams[2] >= 4


@pytest.mark.parametrize(
    "node_type", ["If", "Case", "While", "PartitionedCall", "CustomSubgraphCall", "Add"]
)
def test_write_back_without_subgraph_uses_assigned_stream(node_type):
    """Filtering follows subgraph instances, including for custom op types."""
    graph = FakeGraph("node_without_subgraph", [FakeNode("node", node_type, origin=1)])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context, base=4)
    adapter.write_back(graph, context)
    assert context.writes == [(0, adapter._plan.get_stream_id(adapter._nodes[0]))]
    assert context.writes[0][1] >= 4


def test_subgraph_query_failure_rejects_plan_and_write_back():
    class BrokenNode(FakeNode):
        def _has_subgraph_instances(self):
            raise RuntimeError("subgraph query failed")

    graph = FakeGraph("broken_query", [FakeNode("compute"), BrokenNode("call")])
    context, adapter = FakeContext(), DAGAdapter()
    with pytest.raises(RuntimeError, match="subgraph query failed"):
        _run_plan(adapter, graph, context)
    with pytest.raises(RuntimeError, match="called before plan"):
        adapter.write_back(graph, context)
    assert context.writes == []


@pytest.fixture
def adapter_log(caplog):
    """Capture adapter records straight off its own logger.

    Loading multistream installs the GE bridge on this logger and turns off
    propagation, so caplog's root handler alone may see nothing.
    """

    logger = adapter_mod._LOGGER
    logger.addHandler(caplog.handler)
    caplog.set_level("INFO", logger=logger.name)
    try:
        yield caplog
    finally:
        logger.removeHandler(caplog.handler)


def test_build_and_write_back_summaries_keep_cpp_wording(adapter_log):
    nodes = [
        FakeNode("compute"),
        FakeNode("out", node_type="NetOutput"),
        FakeNode("label", is_rts=True),
    ]
    graph = FakeGraph("g", nodes, [(0, 0, 1, 0), (0, 0, 2, 0)])
    nodes[1]._control_outputs.append(nodes[2])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    adapter.write_back(graph, context)
    for message in (
        "FromGEGraph start: graph name=g",
        "ConvertNodes done: total=3",
        "ConvertEdges done: data_edges=2, control_edges=1",
        "FromGEGraph done: nodes=3, edges=3",
        "RefreshStreamIdsToGE done: success=1, skip=0, filtered=2",
    ):
        assert message in adapter_log.text


def test_optional_node_attributes_use_the_quiet_query_when_available():
    class QuietNode(FakeNode):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.queries = []

        def _has_attr_quiet(self, key):
            self.queries.append(key)
            return key in self._attrs

        def has_attr(self, key):
            raise AssertionError("has_attr logs a warning for every absent attribute")

    node = QuietNode("serial", stream_label="L1")
    del node._attrs["_rts_label_node"]
    graph = FakeGraph("g", [node])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    # 建图时还要读执行引擎和用户控核这两类编译元数据。
    assert node.queries == [
        "_stream_label",
        "_rts_label_node",
        "_cube_vector_core_type",
        "_op_aicore_num",
        "_op_vectorcore_num",
    ]


# ── 执行引擎与用户控核两个新字段 ──────────────────────────────────────


def _cost_of(adapter, index=0):
    return adapter._nodes[index].get_node_cost()


def _node_with_attrs(**attrs):
    node = FakeNode("n0")
    node._attrs.update(attrs)
    return node


def _plan_one(node):
    graph, context, adapter = FakeGraph("g", [node]), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    return adapter._nodes[0]


@pytest.mark.parametrize("value", ["AIC", "AIV", "MIX_AIC", "MIX_AIV"])
def test_explicit_core_types_are_mapped_one_to_one(value):
    import multistream

    cost = _plan_one(_node_with_attrs(_cube_vector_core_type=value))
    assert cost.get_core_type() is multistream.CoreType[value]


@pytest.mark.parametrize(
    "mix_is_aiv, expected",
    [(True, "MIX_AIV"), (False, "MIX_AIC"), ("1", "MIX_AIC"), (1, "MIX_AIC")],
)
def test_legacy_mix_core_type_follows_mix_is_aiv(mix_is_aiv, expected):
    import multistream

    # 非 bool 值按读取失败处理，与 StreamUtils::IsAivNode 一致，缺省回到
    # GE 既有的 MIX_AIC 语义。
    cost = _plan_one(
        _node_with_attrs(_cube_vector_core_type="MIX", _mix_is_aiv=mix_is_aiv)
    )
    assert cost.get_core_type() is multistream.CoreType[expected]


def test_legacy_mix_without_the_flag_is_mix_aic():
    import multistream

    cost = _plan_one(_node_with_attrs(_cube_vector_core_type="MIX"))
    assert cost.get_core_type() is multistream.CoreType.MIX_AIC


@pytest.mark.parametrize("value", [None, "", "AI_CORE", "aic", 3, object()])
def test_missing_or_unknown_core_type_is_unknown(value):
    import multistream

    # 不按算子名或耗时猜测引擎：读不到就是 UNKNOWN。
    cost = _plan_one(_node_with_attrs(_cube_vector_core_type=value))
    assert cost.get_core_type() is multistream.CoreType.UNKNOWN
    assert _plan_one(FakeNode("n0")).get_core_type() is multistream.CoreType.UNKNOWN


@pytest.mark.parametrize(
    "key, expected",
    [
        ("_op_aicore_num", (8, 0)),
        ("_op_vectorcore_num", (0, 8)),
    ],
)
def test_user_core_numbers_are_kept_separately(key, expected):
    cost = _plan_one(_node_with_attrs(**{key: "8"})).get_node_cost()
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == expected
    assert (cost.cube_block_num, cost.vec_block_num) == (0, 0)


@pytest.mark.parametrize(
    "value",
    [
        "",
        "0",
        None,
        8,
        True,
        "8|8",
        " 8",
        "08",
        "-1",
        "+8",
        "８",
        "²",
        "2147483648",
        "9" * 5000,
    ],
)
def test_core_number_limits_ignore_unusable_values(value):
    cost = _plan_one(_node_with_attrs(_op_aicore_num=value)).get_node_cost()
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == (0, 0)
    cost = _plan_with_graph_attrs(**{"ge.aicoreNum": value})
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == (0, 0)


def _plan_with_graph_attrs(**attrs):
    graph = FakeGraph("g", [FakeNode("n0")], attrs=attrs)
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    return _cost_of(adapter)


@pytest.mark.parametrize(
    "key, expected",
    [
        ("ge.aicoreNum", (8, 0)),
        ("ge.vectorcoreNum", (0, 8)),
    ],
)
def test_model_level_core_numbers_are_kept_separately(key, expected):
    # GraphManager persists options on the actual root; RunCustomStreamPass
    # synchronizes them to a separately compiled static subgraph before Python.
    cost = _plan_with_graph_attrs(**{key: "8"})
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == expected


def test_model_level_core_number_option_covers_every_node():
    nodes = [FakeNode(f"n{i}") for i in range(3)]
    graph = FakeGraph("g", nodes, [(0, 0, 1, 0)], attrs={"ge.vectorcoreNum": "4"})
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    assert all(
        (
            _cost_of(adapter, i).cube_core_num_limit,
            _cost_of(adapter, i).vec_core_num_limit,
        )
        == (0, 4)
        for i in range(3)
    )


@pytest.mark.parametrize(
    "op_cube, op_vec, expected",
    [
        ("4", "6", (4, 6)),
        ("0", "6", (8, 6)),
        ("4", "", (4, 16)),
        (None, None, (8, 16)),
        ("12", "20", (12, 20)),
    ],
)
def test_operator_core_options_override_each_model_option(op_cube, op_vec, expected):
    node = _node_with_attrs(_op_aicore_num=op_cube, _op_vectorcore_num=op_vec)
    graph = FakeGraph(
        "static_subgraph", [node], attrs={"ge.aicoreNum": "8", "ge.vectorcoreNum": "16"}
    )
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    cost = _cost_of(adapter)
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == expected


def test_graph_without_attribute_access_keeps_the_default():
    class NoAttrGraph(FakeGraph):
        get_attr = None

    graph = NoAttrGraph("g", [FakeNode("n0")])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    assert (
        _cost_of(adapter).cube_core_num_limit,
        _cost_of(adapter).vec_core_num_limit,
    ) == (0, 0)


def test_excluded_node_keeps_the_zero_marker_together_with_the_new_fields():
    import multistream

    node = _node_with_attrs(_cube_vector_core_type="AIV", _op_aicore_num="4")
    node.origin_stream_id = -1
    adapted_node = _plan_one(node)
    cost = adapted_node.get_node_cost()
    assert cost.execution_time == 0.0
    assert adapted_node.get_core_type() is multistream.CoreType.AIV
    assert (cost.cube_core_num_limit, cost.vec_core_num_limit) == (4, 0)


def test_plan_core_number_queries_answer_zero_in_this_release():
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    for kernel_type in ("AIC", "AIV"):
        assert all(
            adapter._plan.get_core_num(node, kernel_type) == 0
            for node in adapter._nodes
        )


def test_write_back_skips_are_logged_like_cpp(adapter_log, monkeypatch):
    adapter_log.set_level("DEBUG", logger=adapter_mod._LOGGER.name)
    nodes = [
        FakeNode("compute"),
        FakeNode("no_stream"),
        FakeNode("no_device", origin=-1),
        FakeNode("out", node_type="NetOutput"),
        FakeNode("label", is_rts=True),
    ]
    graph = FakeGraph("g", nodes)
    package = sys.modules["multistream"]
    real_plan = package.plan

    def plan_without_stream_for_second_node(dag, **kwargs):
        plan = real_plan(dag, **kwargs)
        ids = [plan.get_stream_id(node) for node in dag._nodes]
        ids[1] = package.INVALID_STREAM_ID
        return package.StreamPlan(ids, plan.get_required_stream_count(), dag._nodes)

    monkeypatch.setattr(package, "plan", plan_without_stream_for_second_node)
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    adapter.write_back(graph, context)
    for message in (
        "Skip special node: out",
        "Skip special node: label",
        "Node no_stream has invalid stream_id, skip",
        "Node no_device origin stream id is invalid, skip",
        "RefreshStreamIdsToGE done: success=1, skip=2, filtered=2",
    ):
        assert message in adapter_log.text


def test_node_whose_origin_stream_id_is_invalid_is_filtered():
    """Filter (4): a node with no device task must not be given a stream.

    A serial node is the case filter (3) cannot catch -- serial splitting
    runs before the zero-cost exclusion, so it comes back with a real stream
    id even though its original stream id was -1.
    """

    nodes = [
        FakeNode("compute"),
        FakeNode("serial_no_device", stream_label="L1", origin=-1),
    ]
    graph = FakeGraph("g", nodes, [(0, 0, 1, 0)])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    adapter.write_back(graph, context)
    assert [index for index, _ in context.writes] == [0]


# ── 边的插入顺序：源节点序 -> 出数据边 -> 出控制边 ────────────────────


def test_edges_follow_source_node_order_then_data_then_control():
    """Edge order decides the path cover, so it is part of the contract."""

    nodes = [FakeNode(f"n{i}") for i in range(3)]
    graph = FakeGraph("g", nodes, [(0, 0, 1, 0)])
    nodes[0]._control_outputs.append(nodes[2])
    context, adapter = FakeContext(), DAGAdapter()
    dag = adapter._build_dag(graph, context)
    # 断言的是适配器经公开 add_edge 推进去的调用序，不是算法包的内部边表。
    assert dag.stub_edges == [
        (0, 0, 1, 0),  # data edge first, on output port 0
        (0, -1, 2, -1),  # then the control edge of the same source node
    ]


# ── 身份、类型和事务边界 ───────────────────────────────────────────────


def test_duplicate_node_names_are_rejected_and_state_is_cleared():
    """A native graph can expose duplicate names only after malformed input.

    ``DAGGraph`` keeps names unique.  The adapter must turn its ``None`` return
    into an error and discard all state built before the duplicate was seen.
    """

    nodes = [FakeNode("same"), FakeNode("same")]
    graph = FakeGraph("g", nodes)
    adapter, context = DAGAdapter(), FakeContext()
    with pytest.raises(RuntimeError, match="duplicate node"):
        _run_plan(adapter, graph, context)
    assert adapter._plan is None
    assert adapter._graph is None
    assert adapter._nodes == []
    assert adapter._node_records == []
    with pytest.raises(RuntimeError, match="called before plan"):
        adapter.write_back(graph, context)


def test_cross_graph_edge_with_same_name_is_not_silently_rewired():
    first = _linear_graph("first", 2)
    second = _linear_graph("second", 2)
    # Replace first's edge destination with a same-named node from another
    # graph.  Name-based lookup would silently map it to first[1].
    second._nodes[1].name = first._nodes[1].name
    first._nodes[0]._data_outputs[0] = [(second._nodes[1], 0)]
    with pytest.raises(RuntimeError, match="destination is not a direct node"):
        DAGAdapter().plan(
            first,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


def test_edge_wrapper_copy_uses_underlying_native_identity():
    """Fresh C-wrapper values for one node must still resolve to one index."""

    class CopyingNode(FakeNode):
        def __init__(self, name, identity):
            super().__init__(name)
            self._identity = identity

        def get_native_identity(self):
            return self._identity

    source = CopyingNode("source", 101)
    direct_destination = CopyingNode("destination", 202)
    edge_destination_copy = CopyingNode("destination", 202)
    source._data_outputs[0] = [(edge_destination_copy, 0)]
    graph = FakeGraph("g", [source, direct_destination])
    context = FakeContext()

    # The edge endpoint is a distinct Python object with a different wrapper
    # handle, but represents the same underlying native node.
    assert edge_destination_copy._handle.value != direct_destination._handle.value
    assert DAGAdapter()._build_dag(graph, context) is not None


def test_control_edge_to_non_direct_node_is_rejected():
    graph = FakeGraph("g", [FakeNode("n0")])
    graph._nodes[0]._control_outputs.append(FakeNode("foreign"))
    with pytest.raises(RuntimeError, match="control edge destination"):
        DAGAdapter().plan(
            graph,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


def test_none_direct_node_table_is_treated_as_empty_graph():
    class NoneNodeGraph(FakeGraph):
        def get_direct_nodes(self):
            return None

    graph = NoneNodeGraph("empty", [])
    adapter, context = DAGAdapter(), FakeContext()
    assert _run_plan(adapter, graph, context) == 0
    adapter.write_back(graph, context)
    assert context.writes == []


@pytest.mark.parametrize(
    "graph, context", [(None, FakeContext()), (FakeGraph("g", []), None)]
)
def test_plan_rejects_null_graph_or_context(graph, context):
    with pytest.raises(TypeError, match="Graph|StreamPassContext"):
        DAGAdapter().plan(
            graph,
            context,
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("bad_name", [None, 1, b"graph"])
def test_graph_name_must_be_a_string(bad_name):
    graph = FakeGraph(bad_name, [FakeNode("n")])
    with pytest.raises(TypeError, match="Graph name"):
        DAGAdapter().plan(
            graph,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


def test_graph_without_name_is_rejected():
    class NamelessGraph(FakeGraph):
        name = None

        def __init__(self):
            self._nodes = [FakeNode("n")]

    with pytest.raises(TypeError, match="Graph name"):
        DAGAdapter().plan(
            NamelessGraph(),
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize(
    "attribute, value, expected",
    [
        ("name", 7, "name and type"),
        ("type", object(), "name and type"),
        ("stream_label", 7, "_stream_label"),
        ("is_rts", 1, "_rts_label_node"),
    ],
)
def test_graph_node_attributes_are_type_checked(attribute, value, expected):
    node = FakeNode("n")
    if attribute == "name":
        node.name = value
    elif attribute == "type":
        node.type = value
    elif attribute == "stream_label":
        node._attrs["_stream_label"] = value
    else:
        node._attrs["_rts_label_node"] = value
    graph = FakeGraph("g", [node])
    with pytest.raises(TypeError, match=expected):
        DAGAdapter().plan(
            graph,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("origin", [True, 1.5, "0"])
def test_origin_stream_id_rejects_bool_float_and_string(origin):
    class BadOriginContext(FakeContext):
        def get_stream_id(self, _graph, _index):
            return origin

    with pytest.raises(TypeError, match="origin_stream_id"):
        DAGAdapter().plan(
            _linear_graph(),
            BadOriginContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("max_stream_num", [True, 8.0, "8"])
def test_max_stream_num_is_strictly_integer(max_stream_num):
    with pytest.raises(TypeError, match="max_stream_num"):
        DAGAdapter().plan(
            _linear_graph(),
            FakeContext(),
            max_stream_num=max_stream_num,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("outputs_size", [True, 1.5, "1"])
def test_outputs_size_is_strictly_integer(outputs_size):
    class BadOutputsNode(FakeNode):
        def get_outputs_size(self):
            return outputs_size

    with pytest.raises(TypeError, match="outputs size"):
        DAGAdapter().plan(
            FakeGraph("g", [BadOutputsNode("n")]),
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("dst_port", [True, 1.5, "0"])
def test_data_edge_destination_port_is_strictly_integer(dst_port):
    graph = _linear_graph("g", 2)
    graph._nodes[0]._data_outputs[0] = [(graph._nodes[1], dst_port)]
    with pytest.raises(TypeError, match="destination port"):
        DAGAdapter().plan(
            graph,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("required", [True, 1.5, "1", -1])
def test_required_stream_count_is_strictly_validated(monkeypatch, required):
    import multistream

    class BadPlan:
        def get_required_stream_count(self):
            return required

    monkeypatch.setattr(
        adapter_mod,
        "_load_multistream",
        lambda: (
            multistream.DAGGraph,
            multistream.MergeStrategy,
            multistream.NodeCost,
            lambda *_args, **_kwargs: BadPlan(),
            multistream.INVALID_STREAM_ID,
            multistream.CoreType,
        ),
    )
    with pytest.raises((TypeError, RuntimeError), match="required stream"):
        DAGAdapter().plan(
            _linear_graph("g", 1),
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )


@pytest.mark.parametrize("base_stream_id", [True, 1.5, "7", -1])
def test_base_stream_id_rejects_invalid_values(base_stream_id):
    with pytest.raises((TypeError, ValueError), match="base_stream_id"):
        DAGAdapter().plan(
            _linear_graph(),
            FakeContext(),
            max_stream_num=8,
            base_stream_id=base_stream_id,
            merge_strategy="LoadBalance",
        )


def test_blockdim_is_never_read_without_profiling():
    class BlockdimNode(FakeNode):
        def __init__(self):
            super().__init__("n")
            self.blockdim_reads = 0
            self._attrs["TVM_ATTR_NAME_BLOCKDIM"] = 32

        def get_attr(self, key):
            if key == "TVM_ATTR_NAME_BLOCKDIM":
                self.blockdim_reads += 1
            return super().get_attr(key)

    node = BlockdimNode()
    graph = FakeGraph("g", [node])
    context, adapter = FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    assert node.blockdim_reads == 0
    assert adapter._nodes[0].get_node_cost().cube_block_num == 0
    assert adapter._nodes[0].get_node_cost().vec_block_num == 0


def test_setter_failure_keeps_successful_writes_and_does_not_rollback():
    class FailingContext(FakeContext):
        def set_stream_id(self, graph, node_index, stream_id):
            if node_index == 2:
                raise RuntimeError("injected setter failure")
            super().set_stream_id(graph, node_index, stream_id)

    graph, context, adapter = _linear_graph(), FailingContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    with pytest.raises(RuntimeError, match="injected setter failure"):
        adapter.write_back(graph, context)
    assert [index for index, _ in context.writes] == [0, 1]


@pytest.mark.parametrize("stream_id", [True, 1.5, "0"])
def test_write_back_rejects_non_integer_stream_id(stream_id):
    graph, context, adapter = _linear_graph("g", 1), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)

    class BadPlan:
        def get_stream_id(self, _node):
            return stream_id

    adapter._plan = BadPlan()
    with pytest.raises(TypeError, match="stream id"):
        adapter.write_back(graph, context)
    assert context.writes == []


@pytest.mark.parametrize(
    "strategy",
    [
        "LoadBalance",
        "load_balance",
        "loadbalance",
        "MainStream",
        "main-stream",
    ],
)
def test_strategy_aliases_are_accepted(strategy):
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    assert (
        adapter.plan(
            graph,
            context,
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy=strategy,
        )
        >= 1
    )


def test_import_failure_clears_transaction_state(monkeypatch):
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    monkeypatch.setattr(
        adapter_mod,
        "_load_multistream",
        lambda: (_ for _ in ()).throw(RuntimeError("missing graph-autofusion")),
    )
    with pytest.raises(RuntimeError, match="missing graph-autofusion"):
        adapter.plan(
            graph,
            context,
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )
    assert adapter._plan is None and adapter._graph is None
    assert adapter._nodes == [] and adapter._node_records == []


def test_incomplete_multistream_public_api_fails_loudly(monkeypatch):
    broken = types.ModuleType("multistream")
    broken.__file__ = "/tmp/broken/multistream/__init__.py"
    monkeypatch.setitem(sys.modules, "multistream", broken)
    with pytest.raises(RuntimeError, match="does not expose the expected public API"):
        adapter_mod._load_multistream()


def test_multistream_import_failure_mentions_graph_autofusion(monkeypatch):
    monkeypatch.setitem(sys.modules, "multistream", None)
    with pytest.raises(RuntimeError, match="graph-autofusion"):
        adapter_mod._load_multistream()


def test_write_back_rejects_null_context_before_mutating_graph():
    graph, context, adapter = _linear_graph(), FakeContext(), DAGAdapter()
    _run_plan(adapter, graph, context)
    with pytest.raises(TypeError, match="StreamPassContext"):
        adapter.write_back(graph, None)
    assert context.writes == []


# ── 桩本身的护栏 ──────────────────────────────────────────────────────


def test_stub_exposes_exactly_the_published_multistream_surface():
    """The stub must not grow an API the real package does not publish.

    ``multistream`` ships in the CANN package, so its public surface is a
    contract: seven names in ``__all__`` plus the two exported constants.  A
    stub that quietly adds a convenience method would let an adapter start
    depending on something that does not exist in the real package.
    """

    import multistream

    assert multistream.__all__ == [
        "DAGGraph",
        "DAGNode",
        "NodeCost",
        "CoreType",
        "StreamPlan",
        "MergeStrategy",
        "plan",
    ]
    public = {name for name in vars(multistream) if not name.startswith("_")}
    assert public == set(multistream.__all__) | {"INVALID_STREAM_ID", "MAX_STREAM_NUM"}
    assert multistream.INVALID_STREAM_ID == -1
    assert multistream.MAX_STREAM_NUM == 64
    # 序列化拼写是 GE option 解析用的，值写错会让别名匹配走到不同分支上。
    assert [member.value for member in multistream.MergeStrategy] == [
        "LoadBalance",
        "MainStream",
    ]
    # CoreType 只在 Python 侧传递，不进入 option，所以枚举值与成员名一致。
    assert [member.value for member in multistream.CoreType] == [
        "AIC",
        "MIX_AIC",
        "AIV",
        "MIX_AIV",
        "UNKNOWN",
    ]

    # 替身与真包保持一致：节点 get/set 对称，图查询可用于打印与调试。
    def public_methods(obj):
        return {name for name in dir(obj) if not name.startswith("_")}

    assert public_methods(multistream.DAGGraph) == {
        "add_node",
        "add_edge",
        "find_node",
        "get_name",
        "get_nodes",
        "get_node_count",
    }
    assert public_methods(multistream.DAGNode) == {
        "set_stream_label",
        "set_node_cost",
        "get_stream_label",
        "get_node_cost",
        "get_name",
        "get_type",
        "get_index",
        "set_core_type",
        "get_core_type",
    }
    assert public_methods(multistream.StreamPlan) == {
        "get_stream_id",
        "get_required_stream_count",
        "get_core_num",
    }
