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

"""In-repo stand-in for the ``multistream`` package used by GE adapter tests.

``multistream`` is delivered by graph-autofusion, not by GE, so GE's unit
tests must not need that repository checked out or built.  What they
substitute is the *implementation*: the published interface is reproduced
here exactly as the CANN package will ship it, and only the algorithm behind
it is replaced by something trivial.  The partitioning itself belongs to
graph-autofusion's L1 suite, and GE runs against the real package at L4.

The published surface is the seven names in ``multistream.__all__`` plus the
two exported constants -- and nothing else.  Signatures, keyword-only
arguments, enum values and defaults below are mirrored from
``graph-autofusion/multistream/``;
``test_stub_exposes_exactly_the_published_multistream_surface`` keeps this
module from quietly growing an API the real package does not have.

Algorithm internals (``_get_edges``, ``_Edge``, the
``algorithm`` subpackage, ...) are deliberately *not* reproduced.  Where a
test needs to observe what the adapter pushed through the public API, this
module records it under an explicitly stub-owned name (``stub_edges``,
``_stub_calls``) so that no test can mistake a stub detail for real package
behaviour.

Three input contracts are enforced here because they live in the algorithm
package rather than in the adapter, and adapter tests depend on them firing:

* ``max_stream_num`` must be ``None`` or an int in ``[1, MAX_STREAM_NUM]``
* ``base_stream_id`` must be a non-negative int
* an unreadable ``profiling_path`` raises ``ValueError``
"""

import enum
import os
import types

INVALID_STREAM_ID = -1
MAX_STREAM_NUM = 64
_KERNEL_TYPES = ("AIC", "AIV")


class CoreType(enum.Enum):
    """Execution engine a node's kernel runs on; values repeat the names."""

    AIC = "AIC"
    MIX_AIC = "MIX_AIC"
    AIV = "AIV"
    MIX_AIV = "MIX_AIV"
    UNKNOWN = "UNKNOWN"


class MergeStrategy(enum.Enum):
    """Strategy used to merge logical paths onto physical streams."""

    # The values are GE's serialized spellings, not the member names.
    LOAD_BALANCE = "LoadBalance"
    MAIN_STREAM = "MainStream"

    @classmethod
    def _missing_(cls, value):
        if isinstance(value, str):
            key = value.strip().lower().replace("-", "_").replace(" ", "_")
            return {
                "loadbalance": cls.LOAD_BALANCE,
                "load_balance": cls.LOAD_BALANCE,
                "mainstream": cls.MAIN_STREAM,
                "main_stream": cls.MAIN_STREAM,
            }.get(key)
        return None


class NodeCost:
    """Immutable per-node cost values consumed by stream planning.

    Keyword-only, with the same seven fields and defaults as the real package;
    ``execution_time == -1.0`` is the "no profiling data" marker and ``0.0``
    is the exclusion marker the adapter writes. User core limits are separate
    from the measured block counts.
    """

    __slots__ = (
        "execution_time",
        "memory_usage",
        "bandwidth_usage",
        "cube_block_num",
        "vec_block_num",
        "cube_core_num_limit",
        "vec_core_num_limit",
    )

    def __init__(
        self,
        *,
        execution_time=-1.0,
        memory_usage=0,
        bandwidth_usage=0,
        cube_block_num=0,
        vec_block_num=0,
        cube_core_num_limit=0,
        vec_core_num_limit=0,
    ):
        values = {
            "execution_time": execution_time,
            "memory_usage": memory_usage,
            "bandwidth_usage": bandwidth_usage,
            "cube_block_num": cube_block_num,
            "vec_block_num": vec_block_num,
            "cube_core_num_limit": cube_core_num_limit,
            "vec_core_num_limit": vec_core_num_limit,
        }
        for field_name, value in values.items():
            object.__setattr__(self, field_name, value)

    def __setattr__(self, name, value):
        raise AttributeError("NodeCost is immutable")


class DAGNode:
    __slots__ = (
        "_name",
        "_type",
        "_index",
        "_stream_label",
        "_node_cost",
        "_core_type",
    )

    def __init__(self, name, node_type, index=-1):
        self._name = name
        self._type = node_type
        self._index = int(index)
        self._stream_label = ""
        self._node_cost = NodeCost()
        self._core_type = CoreType.UNKNOWN

    def get_name(self):
        return self._name

    def get_type(self):
        return self._type

    def get_index(self):
        return self._index

    def set_core_type(self, core_type):
        if not isinstance(core_type, CoreType):
            raise TypeError("core type must be a CoreType member")
        self._core_type = core_type

    def get_core_type(self):
        return self._core_type

    def set_stream_label(self, label):
        self._stream_label = label

    def get_stream_label(self):
        return self._stream_label

    def set_node_cost(self, cost):
        self._node_cost = cost

    def get_node_cost(self):
        return self._node_cost


class DAGGraph:
    def __init__(self, name):
        if not isinstance(name, str):
            raise TypeError("graph name must be a string")
        self._name = name
        self._nodes = []
        self._name_to_index = {}
        #: Stub-owned record of every ``add_edge`` call, in call order, as
        #: ``(src_index, src_port, dst_index, dst_port)``.  Not a published API.
        self.stub_edges = []

    # Mirror the published ``type`` keyword of the multistream API.
    def add_node(self, name, type):  # noqa: A002  # pylint: disable=redefined-builtin
        if not isinstance(name, str) or not isinstance(type, str):
            raise TypeError("node name and type must be strings")
        # A duplicate name returns None the way the C++ implementation
        # returned nullptr; the adapter turns that into an error.
        if name in self._name_to_index:
            return None
        node = DAGNode(name, type, len(self._nodes))
        self._name_to_index[name] = len(self._nodes)
        self._nodes.append(node)
        return node

    def get_name(self):
        return self._name

    def get_nodes(self):
        return list(self._nodes)

    def get_node_count(self):
        return len(self._nodes)

    def find_node(self, name):
        index = self._name_to_index.get(name)
        return None if index is None else self._nodes[index]

    def add_edge(self, src, src_port, dst, dst_port):
        self.stub_edges.append((src._index, int(src_port), dst._index, int(dst_port)))


class StreamPlan:
    """Read-only result of stream planning.

    ``stream_ids`` is a sequence indexed by node index, and a node that does
    not belong to this plan is rejected rather than silently answered with
    INVALID -- both mirror the real package, and the rejection is what keeps
    an adapter bug (handing over a node from another graph) visible.
    """

    __slots__ = ("_stream_ids", "_required_stream_count", "_node_refs")

    def __init__(self, stream_ids, required_stream_count, node_refs=None):
        self._stream_ids = [int(value) for value in stream_ids]
        self._required_stream_count = int(required_stream_count)
        self._node_refs = tuple(node_refs) if node_refs is not None else tuple()

    def get_stream_id(self, node):
        if not isinstance(node, DAGNode):
            raise TypeError("node must be a DAGNode")
        index = node._index
        if self._node_refs and (
            index < 0
            or index >= len(self._node_refs)
            or self._node_refs[index] is not node
        ):
            raise ValueError(
                "node does not belong to the graph used to create this plan"
            )
        if index < 0 or index >= len(self._stream_ids):
            raise ValueError("node index is out of range for this plan")
        return self._stream_ids[index]

    def get_required_stream_count(self):
        return self._required_stream_count

    def get_core_num(self, node, kernel_type):
        # Validates like the real package; this release never suggests a
        # core count, so every legal query answers 0.
        self.get_stream_id(node)
        if not isinstance(kernel_type, str):
            raise TypeError("kernel_type must be a string")
        if kernel_type not in _KERNEL_TYPES:
            raise ValueError('kernel_type must be "AIC" or "AIV"')
        return 0


#: Stub-owned log of every ``plan`` call.  Not a published API.
_stub_calls = []


def plan(
    graph,
    *,
    max_stream_num=None,
    base_stream_id=0,
    merge_strategy=MergeStrategy.LOAD_BALANCE,
    profiling_path=None,
):
    _stub_calls.append(
        {
            "graph": graph,
            "max_stream_num": max_stream_num,
            "base_stream_id": base_stream_id,
            "merge_strategy": merge_strategy,
            "profiling_path": profiling_path,
        }
    )

    if not isinstance(graph, DAGGraph):
        raise TypeError("graph must be a DAGGraph")
    if max_stream_num is not None:
        if isinstance(max_stream_num, bool) or not isinstance(max_stream_num, int):
            raise TypeError("max_stream_num must be an integer or None")
        if not 1 <= max_stream_num <= MAX_STREAM_NUM:
            raise ValueError(f"max_stream_num must be in [1, {MAX_STREAM_NUM}]")
    if isinstance(base_stream_id, bool) or not isinstance(base_stream_id, int):
        raise TypeError("base_stream_id must be an integer")
    if base_stream_id < 0:
        raise ValueError("base_stream_id must be non-negative")
    if profiling_path and not os.path.isfile(profiling_path):
        raise ValueError(f"cannot open profiling file: {profiling_path}")

    nodes = graph._nodes
    if not nodes:
        return StreamPlan([], 0, ())
    # One stream is enough for every adapter assertion: what they check is
    # that each eligible node came back with a valid id and that the
    # adapter's own write-back filters drop the ineligible ones.  Real
    # partitioning is graph-autofusion's L1 concern.
    return StreamPlan([base_stream_id] * len(nodes), 1, nodes)


#: The published surface, mirrored from ``multistream/__init__.py``.
PUBLISHED_ALL = [
    "DAGGraph",
    "DAGNode",
    "NodeCost",
    "CoreType",
    "StreamPlan",
    "MergeStrategy",
    "plan",
]
PUBLISHED_CONSTANTS = ["INVALID_STREAM_ID", "MAX_STREAM_NUM"]


def build_module():
    """Return a fresh module object exposing this stub as ``multistream``."""

    module = types.ModuleType("multistream")
    module.__file__ = __file__
    for name in PUBLISHED_ALL + PUBLISHED_CONSTANTS:
        setattr(module, name, globals()[name])
    module.__all__ = list(PUBLISHED_ALL)
    return module
