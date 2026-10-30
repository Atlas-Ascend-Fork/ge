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

"""GE ``Graph`` to :mod:`multistream` adapter.

The adapter is intentionally the only GE-side module that knows about the
multi-stream algorithm package.  Importing it is cheap and does not import
``multistream``; the algorithm package is loaded when :meth:`plan` is
first called.  The GE ``Graph`` and ``Node`` wrappers are read directly
during graph construction; the adapter keeps only the resulting DAG node
table needed for write-back.
"""

from __future__ import annotations

__all__ = ["DAGAdapter"]

import importlib
import logging
import os
from typing import Any, List, NamedTuple, Optional, Tuple


_LOGGER = logging.getLogger(__name__)
_CSV_SUFFIX = ".csv"
_INVALID_STREAM_ID = -1

# Node compile metadata read by ``_resolve_core_type``.  The spellings follow
# GE's own readers (``StreamUtils::IsAivNode`` and the RT2 core-type lowering).
_CORE_TYPE_ATTR = "_cube_vector_core_type"
_MIX_IS_AIV_ATTR = "_mix_is_aiv"
_CORE_TYPE_NAMES = ("AIC", "AIV", "MIX_AIC", "MIX_AIV")
# The pre-split mixed-kernel spelling.  ``_mix_is_aiv`` decides the engine;
# without it GE treats the node as MIX_AIC.
_LEGACY_MIX_CORE_TYPE = "MIX"
# User core-count controls pushed down to a node (public_attr::OP_AI_CORE_NUM
# and OP_VECTOR_CORE_NUM).  GE copies them onto fusion ops only from origin
# nodes that carried them, so their presence always traces back to the user.
_USER_CORE_NUM_ATTRS = ("_op_aicore_num", "_op_vectorcore_num")
# Model-level core-count controls.  GraphManager::Build persists the
# ge.aicoreNum / ge.vectorcoreNum options onto the root graph before the
# builder runs, and deletes the attributes when the options are unset.
# RunCustomStreamPass synchronizes them to separately compiled subgraphs.
# These attributes represent global, session or graph-level user options.
_USER_CORE_NUM_GRAPH_ATTRS = ("ge.aicoreNum", "ge.vectorcoreNum")


def _require_int(value: Any, label: str) -> int:
    """Return an actual integer, rejecting bool and lossy coercions.

    Graph and context values are expected to be native Python ``int``
    objects.  Accepting strings/floats here would silently redirect an edge or
    stream write (``int(1.9)`` is particularly dangerous).
    """

    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"{label} must be an integer")
    return value


class _GeLogHandler(logging.Handler):
    """Forward algorithm-package records into GE's native dlog bridge."""

    def emit(self, record: logging.LogRecord) -> None:
        try:
            # Import only after the algorithm package is actually needed.  This keeps
            # bootstrap/skip paths independent of the native artifact.
            native = importlib.import_module("._native", __package__)
            # Formatter includes exc_info/stack_info, which getMessage() drops.
            message = self.format(record)
            if getattr(native, "GE_LOG_SOURCE_SUPPORTED", False):
                native.emit_ge_log(
                    record.levelno,
                    message,
                    record.filename,
                    record.lineno,
                    record.funcName or "log",
                )
            else:
                # Older native wheels only accept (level, message). Keep their
                # diagnostics visible without adding a second thread identifier.
                native.emit_ge_log(
                    record.levelno, f"{record.funcName or 'log'}:{message}"
                )
        except Exception:
            # Logging must never turn an otherwise valid allocation into a
            # failed compilation (the native logger may be unavailable in a
            # standalone/fake test environment).
            return


def _install_ge_log_handler(name: str = "multistream") -> None:
    """Install one idempotent handler on the ``name`` logger."""

    logger = logging.getLogger(name)
    # ``multistream`` deliberately has no logging configuration of its own.  In
    # an embedded GE process the root logger is commonly left at WARNING,
    # which would discard the INFO records used by the C++ implementation for
    # strategy/profiling diagnostics before they reach this handler.  Elevate
    # an otherwise-unconfigured logger to DEBUG and let GELOG* perform the
    # final filtering.  Respect an explicit host level (for example ERROR).
    if logger.level == logging.NOTSET:
        logger.setLevel(logging.DEBUG)
    if any(isinstance(handler, _GeLogHandler) for handler in logger.handlers):
        return
    logger.addHandler(_GeLogHandler())
    # GE owns the final sink; avoid duplicate propagation to Python stderr.
    logger.propagate = False


def _resolve_profiling_path() -> str:
    """Resolve the optional profiling CSV path using the legacy env contract."""

    value = os.environ.get("MINIDAG_PROFILING_PATH", "")
    if not value:
        return ""
    if value.endswith(_CSV_SUFFIX):
        _LOGGER.info(
            "Profiling path overridden by MINIDAG_PROFILING_PATH (csv file): %s",
            value,
        )
        return value
    _LOGGER.warning(
        "MINIDAG_PROFILING_PATH only accepts csv file path, but got: %s", value
    )
    return ""


class _MultistreamAPI(NamedTuple):
    dag_graph: Any
    merge_strategy: Any
    node_cost: Any
    plan: Any
    invalid_stream_id: int
    core_type: Any


def _load_multistream() -> _MultistreamAPI:
    """Load the ``multistream`` public API lazily and normalize import failures."""

    try:
        multistream = importlib.import_module("multistream")
    except Exception as exc:
        raise RuntimeError(
            "MiniDAG multi-stream pass requires the 'multistream' package shipped "
            "with graph-autofusion; it is not importable"
        ) from exc

    # Read the public API straight off the package.  A missing name means the
    # installed ``multistream`` is broken (truncated __init__, or a same-named
    # package shadowing it on sys.path).  Reaching into the submodules to work
    # around that would keep a broken installation running and silently apply
    # stream ids from an unknown implementation, so fail instead.
    try:
        dag_graph = multistream.DAGGraph
        merge_strategy = multistream.MergeStrategy
        node_cost = multistream.NodeCost
        plan_fn = multistream.plan
        invalid_stream_id = multistream.INVALID_STREAM_ID
        core_type = multistream.CoreType
    except AttributeError as exc:
        raise RuntimeError(
            f"multistream package at {getattr(multistream, '__file__', '<unknown>')} does not "
            "expose the expected public API (DAGGraph, MergeStrategy, NodeCost, plan, "
            "INVALID_STREAM_ID, CoreType)"
        ) from exc

    # The algorithm package logs under "multistream" while this adapter logs
    # under its own module name; neither is an ancestor of the other, so both
    # need the bridge for their records to reach the same plog.
    _install_ge_log_handler()
    _install_ge_log_handler(__name__)
    invalid_id = _require_int(invalid_stream_id, "multistream.INVALID_STREAM_ID")
    return _MultistreamAPI(
        dag_graph, merge_strategy, node_cost, plan_fn, invalid_id, core_type
    )


def _resolve_strategy(strategy: Any, merge_strategy_type: Any) -> Any:
    """Map GE's serialized strategy name to ``multistream.MergeStrategy``."""

    if isinstance(strategy, merge_strategy_type):
        return strategy
    raw = getattr(strategy, "value", strategy)
    if not isinstance(raw, str):
        raise TypeError("merge_strategy must be a string or multistream.MergeStrategy")
    normalized = raw.strip().lower().replace("-", "_").replace(" ", "_")
    aliases = {
        "loadbalance": "load_balance",
        "load_balance": "load_balance",
        "mainstream": "main_stream",
        "main_stream": "main_stream",
    }
    normalized = aliases.get(normalized, normalized)
    for member in merge_strategy_type:
        member_value = str(getattr(member, "value", member)).lower()
        member_name = getattr(member, "name", "").lower()
        if normalized in {member_value, member_name}:
            return member
    raise ValueError(f"unsupported MiniDAG merge strategy: {strategy!r}")


class DAGAdapter:
    """Build and plan one GE graph, then write results node by node."""

    __slots__ = (
        "_nodes",
        "_node_records",
        "_plan",
        "_graph",
        "_graph_node_count",
        "_invalid_stream_id",
    )

    def __init__(self) -> None:
        self._nodes: List[Any] = []
        self._node_records: List[Tuple[str, str, str, bool, int, bool]] = []
        self._plan: Optional[Any] = None
        self._graph: Optional[Any] = None
        self._graph_node_count = 0
        self._invalid_stream_id = _INVALID_STREAM_ID

    @staticmethod
    def _node_key(node: Any) -> Any:
        """Return a stable identity for a GE node wrapper.

        ``get_direct_nodes`` and edge accessors create fresh Python wrappers
        around the same native node.  Ask the wrapper for the identity of the
        underlying graph-engine node; simple fake nodes remain supported for
        adapter unit tests through their synthetic handle.
        """

        get_native_identity = getattr(node, "get_native_identity", None)
        if callable(get_native_identity):
            try:
                value = get_native_identity()
            except (AttributeError, RuntimeError, OSError):
                # Keep compatibility with light-weight test doubles and older
                # wrappers that predate the identity C API.
                value = None
            if value not in (None, 0):
                return ("native", value)

        handle = getattr(node, "_handle", None)
        value = getattr(handle, "value", handle)
        if value not in (None, 0):
            return ("handle", value)
        try:
            return ("hash", hash(node))
        except (TypeError, ValueError):
            return ("object", id(node))

    @staticmethod
    def _read_node_attr(node: Any, key: str, default: Any) -> Any:
        # Most nodes carry neither _stream_label nor _rts_label_node.  Prefer
        # the query that does not log a warning for every absent attribute.
        has_attr = getattr(node, "_has_attr_quiet", None)
        if not callable(has_attr):
            has_attr = getattr(node, "has_attr", None)
        if callable(has_attr):
            if not has_attr(key):
                return default
            return node.get_attr(key)
        try:
            return node.get_attr(key)
        except (RuntimeError, KeyError):
            return default

    @staticmethod
    def _read_graph_attr(graph: Any, key: str, default: Any) -> Any:
        has_attr = getattr(graph, "has_attr", None)
        if callable(has_attr):
            if not has_attr(key):
                return default
            return graph.get_attr(key)
        get_attr = getattr(graph, "get_attr", None)
        if not callable(get_attr):
            return default
        # Graph.get_attr raises when the attribute is absent, which is the
        # ordinary case for a graph without a model-level core-count option.
        try:
            return get_attr(key)
        except (RuntimeError, KeyError):
            return default

    @staticmethod
    def _parse_core_num(value: Any) -> int:
        """Read GE's canonical non-negative int32 string; 0 means no limit."""

        if not isinstance(value, str) or not value:
            return 0
        # Match CoreNumUtils::ParseAndValidateCoreNum without accepting signs,
        # leading zeros, Unicode digits, or values outside GE's int32 range.
        if len(value) > 10 or not value.isascii() or not value.isdigit():
            return 0
        number = int(value)
        if str(number) != value or number > 2147483647:
            return 0
        return number

    def plan(
        self,
        graph: Any,
        context: Any,
        *,
        max_stream_num: Optional[int],
        base_stream_id: int,
        merge_strategy: Any,
    ) -> int:
        """Allocate streams for ``graph`` and return the number to reserve.

        The resulting ``StreamPlan`` remains private to this adapter until
        :meth:`write_back`; callers never need to import ``multistream`` types.
        """

        # Reset stale state before beginning a transaction.  On any exception
        # the plan is discarded, so a caller cannot accidentally write a
        # partially computed result.
        self._plan = None
        self._graph = None
        self._graph_node_count = 0
        self._nodes = []
        self._node_records = []
        if graph is None:
            raise TypeError("Graph must not be None")
        if context is None:
            raise TypeError("StreamPassContext must not be None")
        if max_stream_num is not None:
            _require_int(max_stream_num, "max_stream_num")
        _require_int(base_stream_id, "base_stream_id")
        try:
            _, strategy_type, _, plan_fn, invalid_id, _ = _load_multistream()
            self._invalid_stream_id = invalid_id
            dag = self._build_dag(graph, context)
            strategy = _resolve_strategy(merge_strategy, strategy_type)
            profiling_path = _resolve_profiling_path()
            plan = plan_fn(
                dag,
                max_stream_num=max_stream_num,
                base_stream_id=base_stream_id,
                merge_strategy=strategy,
                profiling_path=profiling_path,
            )
            required = plan.get_required_stream_count()
            required = _require_int(required, "MiniDAG required stream count")
            if required < 0:
                raise RuntimeError("MiniDAG returned a negative required stream count")
            self._plan = plan
            self._graph = graph
            self._graph_node_count = len(self._nodes)
            return required
        except Exception:
            self._plan = None
            self._graph = None
            self._graph_node_count = 0
            self._nodes = []
            self._node_records = []
            raise

    def write_back(self, graph: Any, context: Any) -> None:
        """Filter and apply the plan's stream ids to ``graph`` one node at a time."""

        # ``plan`` sets _plan/_graph/_graph_node_count together on success
        # and clears all three on failure, so these three checks are the whole
        # of the consistency contract the removed bulk write-back used to
        # carry.  Any one of them failing rejects the write completely.
        if self._plan is None:
            raise RuntimeError("DAGAdapter.write_back called before plan")
        if graph is not self._graph:
            raise RuntimeError(
                "DAGAdapter.write_back graph does not match the graph used for plan"
            )
        if context is None:
            raise TypeError("StreamPassContext must not be None")

        # The graph is a borrowed object and can theoretically be changed by
        # an erroneous pass between allocation and write-back.  Reject the
        # whole operation before touching a single stream id.
        current_nodes = graph.get_direct_nodes()
        current_count = 0 if current_nodes is None else len(list(current_nodes))
        if current_count != self._graph_node_count:
            raise RuntimeError("graph node count changed between plan and write_back")

        assignments: List[Tuple[int, int]] = []
        skip_count = 0
        filtered_count = 0
        for index, node in enumerate(self._nodes):
            (
                name,
                node_type,
                _serial_label,
                is_rts_label,
                origin_stream_id,
                has_subgraph_instances,
            ) = self._node_records[index]
            # Keep subgraph calls with their RTS labels so the enclosing loop's
            # activation list includes the inner control stream on every iteration.
            if node_type == "NetOutput" or is_rts_label or has_subgraph_instances:
                _LOGGER.debug("Skip special node: %s", name)
                filtered_count += 1
                continue
            stream_id = self._plan.get_stream_id(node)
            if stream_id == self._invalid_stream_id:
                _LOGGER.debug("Node %s has invalid stream_id, skip", name)
                skip_count += 1
                continue
            if origin_stream_id == self._invalid_stream_id:
                _LOGGER.debug("Node %s origin stream id is invalid, skip", name)
                skip_count += 1
                continue
            assignments.append(
                (
                    index,
                    _require_int(
                        stream_id, f"MiniDAG stream id for node index {index}"
                    ),
                )
            )

        for index, stream_id in assignments:
            context.set_stream_id(graph, index, stream_id)
        _LOGGER.info(
            "RefreshStreamIdsToGE done: success=%d, skip=%d, filtered=%d",
            len(assignments),
            skip_count,
            filtered_count,
        )

    def _resolve_core_type(self, node: Any, core_type_enum: Any) -> Any:
        """Map the node's compile metadata to ``multistream.CoreType``.

        Only the engine GE recorded for the kernel counts.  A missing or
        unrecognised value is UNKNOWN; guessing from the op name or its cost
        would hand the algorithm an engine the kernel may not run on.
        """

        value = self._read_node_attr(node, _CORE_TYPE_ATTR, None)
        if not isinstance(value, str):
            return core_type_enum.UNKNOWN
        if value in _CORE_TYPE_NAMES:
            return core_type_enum[value]
        if value == _LEGACY_MIX_CORE_TYPE:
            # Same rule as StreamUtils::IsAivNode: a non-bool flag reads as
            # false, like AttrUtils::GetBool failing.
            mix_is_aiv = self._read_node_attr(node, _MIX_IS_AIV_ATTR, False)
            if mix_is_aiv is True:
                return core_type_enum.MIX_AIV
            return core_type_enum.MIX_AIC
        return core_type_enum.UNKNOWN

    def _read_graph_core_num_limits(self, graph: Any) -> Tuple[int, int]:
        """Read the model limits made available on the current pass graph.

        A known-shape subgraph of a dynamic-shape graph is compiled separately.
        Before invoking this pass, StreamUtils::RunCustomStreamPass copies the
        two model options from GraphUtils::FindRootGraph to that subgraph and
        removes stale attributes when the root no longer carries them.
        """

        return tuple(
            self._parse_core_num(self._read_graph_attr(graph, key, None))
            for key in _USER_CORE_NUM_GRAPH_ATTRS
        )

    def _build_dag(self, graph: Any, context: Any) -> Any:
        dag_graph_type, _, node_cost_type, _, invalid_id, core_type_enum = (
            _load_multistream()
        )
        self._invalid_stream_id = invalid_id
        graph_name = getattr(graph, "name", None)
        # Do not coerce a malformed name into a different graph identity.
        if not isinstance(graph_name, str):
            raise TypeError("Graph name must be a string")
        _LOGGER.info("FromGEGraph start: graph name=%s", graph_name)
        graph_core_num_limits = self._read_graph_core_num_limits(graph)
        dag = dag_graph_type(graph_name)
        nodes: List[Any] = []
        records: List[Tuple[str, str, str, bool, int, bool]] = []
        ge_nodes = graph.get_direct_nodes()
        ge_nodes = [] if ge_nodes is None else list(ge_nodes)
        # Key edges by native identity, not name, and retain their iteration order.
        node_key_to_index = {
            self._node_key(node): index for index, node in enumerate(ge_nodes)
        }
        for index, ge_node in enumerate(ge_nodes):
            dag_node, record = self._convert_node(
                dag,
                ge_node,
                context.get_stream_id(graph, index),
                (node_cost_type, core_type_enum),
                graph_core_num_limits,
            )
            nodes.append(dag_node)
            records.append(record)
        _LOGGER.info("ConvertNodes done: total=%d", len(ge_nodes))
        edge_count = self._convert_edges(dag, ge_nodes, nodes, node_key_to_index)
        _LOGGER.info("FromGEGraph done: nodes=%d, edges=%d", len(nodes), edge_count)
        self._nodes = nodes
        self._node_records = records
        return dag

    def _convert_node(
        self, dag, ge_node, origin_stream_id, core_api, graph_core_num_limits
    ):
        node_cost_type, core_type_enum = core_api
        name = ge_node.name
        node_type = ge_node.type
        if not isinstance(name, str) or not isinstance(node_type, str):
            raise TypeError("Graph node name and type must be strings")
        serial_label = self._read_node_attr(ge_node, "_stream_label", "")
        rts_label = self._read_node_attr(ge_node, "_rts_label_node", False)
        if serial_label is None:
            serial_label = ""
        if not isinstance(serial_label, str):
            raise TypeError("Graph node _stream_label attribute must be a string")
        if not isinstance(rts_label, bool):
            raise TypeError("Graph node _rts_label_node attribute must be a bool")
        origin_stream_id = _require_int(origin_stream_id, "Graph node origin_stream_id")
        record = (
            name,
            node_type,
            serial_label,
            rts_label,
            origin_stream_id,
            ge_node._has_subgraph_instances(),
        )
        dag_node = dag.add_node(name, node_type)
        if dag_node is None:
            raise RuntimeError(
                f"MiniDAG rejected duplicate node while adapting graph: {name}"
            )
        if serial_label:
            dag_node.set_stream_label(serial_label)
        dag_node.set_core_type(self._resolve_core_type(ge_node, core_type_enum))
        # Each engine's positive operator option overrides its model option.
        # A zero operator option has no effect, as in CoreNumUtils.
        core_limits = tuple(
            self._parse_core_num(self._read_node_attr(ge_node, key, None))
            or model_limit
            for key, model_limit in zip(_USER_CORE_NUM_ATTRS, graph_core_num_limits)
        )
        cost_fields = {
            "cube_core_num_limit": core_limits[0],
            "vec_core_num_limit": core_limits[1],
        }
        # This assignment is intentionally before the algorithm package's profiling
        # overlay.  0.0 is an exclusion marker and profiling must not
        # resurrect a node whose GE stream id is INVALID.
        if origin_stream_id == self._invalid_stream_id:
            cost_fields["execution_time"] = 0.0
        dag_node.set_node_cost(node_cost_type(**cost_fields))
        return dag_node, record

    def _convert_edges(self, dag, ge_nodes, nodes, node_key_to_index):
        data_edge_count = 0
        control_edge_count = 0
        for src_index, ge_node in enumerate(ge_nodes):
            data_edge_count += self._convert_data_edges(
                dag, ge_node, src_index, nodes, node_key_to_index
            )
            for dst_node in ge_node.get_out_control_nodes():
                dst_index = node_key_to_index.get(self._node_key(dst_node))
                if dst_index is None:
                    raise RuntimeError(
                        "Graph control edge destination is not a direct node"
                    )
                dag.add_edge(nodes[src_index], -1, nodes[dst_index], -1)
                control_edge_count += 1
        _LOGGER.info(
            "ConvertEdges done: data_edges=%d, control_edges=%d",
            data_edge_count,
            control_edge_count,
        )
        return data_edge_count + control_edge_count

    def _convert_data_edges(self, dag, ge_node, src_index, nodes, node_key_to_index):
        data_edge_count = 0
        outputs_size = ge_node.get_outputs_size()
        outputs_size = _require_int(outputs_size, "Graph node outputs size")
        for output_index in range(outputs_size):
            for dst_node, dst_port in ge_node.get_out_data_nodes_and_port_indexes(
                output_index
            ):
                dst_index = node_key_to_index.get(self._node_key(dst_node))
                if dst_index is None:
                    raise RuntimeError(
                        "Graph data edge destination is not a direct node"
                    )
                dst_port = _require_int(dst_port, "Graph data edge destination port")
                dag.add_edge(nodes[src_index], output_index, nodes[dst_index], dst_port)
                data_edge_count += 1
        return data_edge_count
