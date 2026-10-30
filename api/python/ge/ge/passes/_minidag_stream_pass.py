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

"""Built-in Python MiniDAG stream pass.

Only the descriptor and this light-weight pass class are imported during GE
initialization.  :class:`DAGAdapter` imports :mod:`multistream` lazily from its
``plan`` method, keeping the no-multistream path free of that dependency.
"""

from __future__ import annotations

__all__ = ["DAGStreamPass", "MiniDAGStreamPass"]

import logging
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Optional, Tuple

from .base import PassStage, StatusLike, StreamPass
from .registry import register_resource_plan_pass

if TYPE_CHECKING:
    from ge.graph import Graph
    from ._native import StreamPassContext


_LOGGER = logging.getLogger(__name__)
_AUTO_MULTISTREAM_OPTION = "ge.autoMultistreamParallelMode"
_MAX_STREAM_NUM = 64
# A graph attribute that exists but cannot be read.
_UNREADABLE_GRAPH_VALUE = object()
_STREAM_NUM_REASON = "The stream number must be an integer in [1, 64]."


@dataclass(frozen=True)
class _AutoMultistreamConfig:
    state: str  # unset / skip / dag / error
    strategy: Optional[str] = None
    max_stream_num: Optional[int] = None
    # The configured text for ``dag`` and ``error``.  ``reason`` is set only for
    # ``error`` and worded like StreamUtils::ParseAutoMultistreamParallelMode.
    value: str = ""
    reason: str = ""


def _invalid(value: str, reason: str) -> _AutoMultistreamConfig:
    return _AutoMultistreamConfig("error", value=value, reason=reason)


def _parse_auto_multistream(value: Any, *, from_graph: bool) -> _AutoMultistreamConfig:
    """Parse GE's ``Strategy:N`` syntax using the C++ grammar exactly."""

    if value is None:
        return _AutoMultistreamConfig("unset")
    if value is _UNREADABLE_GRAPH_VALUE:
        return _invalid("", "The graph attribute cannot be read.")
    if not isinstance(value, str):
        return _invalid(repr(value), "The value must be a string.")
    if value == "":
        return _AutoMultistreamConfig("unset")
    if value == "default":
        if from_graph:
            return _AutoMultistreamConfig("skip")
        return _invalid(
            value,
            "The default mode is supported only by the graph attribute set by a custom pass.",
        )
    if value == "cv":
        return _AutoMultistreamConfig("skip")

    colon = value.find(":")
    if colon < 0 or colon != value.rfind(":"):
        return _invalid(value, "The format must be Strategy:N with one colon.")
    strategy = value[:colon]
    max_value = value.partition(":")[2]
    # WeightedLoadBalance has no public option: multistream routes either
    # base strategy to weighted load balancing when valid profiling matches.
    if strategy not in {"LoadBalance", "MainStream"}:
        return _invalid(value, "The strategy must be LoadBalance or MainStream.")
    if not max_value or any(char < "0" or char > "9" for char in max_value):
        return _invalid(value, _STREAM_NUM_REASON)
    try:
        maximum = int(max_value)
    except (TypeError, ValueError, OverflowError):
        return _invalid(value, _STREAM_NUM_REASON)
    if maximum < 1 or maximum > _MAX_STREAM_NUM:
        return _invalid(value, _STREAM_NUM_REASON)
    return _AutoMultistreamConfig("dag", strategy, maximum, value=value)


def _graph_option(graph: Any) -> Tuple[bool, Any]:
    """Read the graph-level option, preserving presence separately from value."""

    if graph is None:
        return False, None
    has_attr = getattr(graph, "has_attr", None)
    presence_known = callable(has_attr)
    if callable(has_attr):
        try:
            if not has_attr(_AUTO_MULTISTREAM_OPTION):
                return False, None
        except Exception:
            return False, None
    getter = getattr(graph, "get_attr", None)
    if not callable(getter):
        return False, None
    try:
        return True, getter(_AUTO_MULTISTREAM_OPTION)
    except (RuntimeError, KeyError, AttributeError, ValueError):
        # Graph has no has_attr() in the public GE wrapper.  A missing attr is
        # reported by get_attr as RuntimeError and means option fallback.  If
        # a graph explicitly reported that the attribute exists, a failed
        # read is an invalid graph value and must not silently fall back to a
        # context option.
        return (True, _UNREADABLE_GRAPH_VALUE) if presence_known else (False, None)


def _resolve_auto_multistream_config(
    graph: Any, context: Any
) -> _AutoMultistreamConfig:
    """Resolve graph attribute first, then the compile-option fallback."""

    from_graph, value = _graph_option(graph)
    if from_graph and isinstance(value, str):
        _LOGGER.info(
            "Use auto multistream parallel mode %s from graph attribute.", value
        )
    if not from_graph:
        getter = getattr(context, "get_option_value", None)
        if not callable(getter):
            return _AutoMultistreamConfig("unset")
        try:
            value = getter(_AUTO_MULTISTREAM_OPTION)
        except (RuntimeError, TypeError, ValueError):
            return _AutoMultistreamConfig("unset")
    return _parse_auto_multistream(value, from_graph=from_graph)


@register_resource_plan_pass(
    name="MiniDAGStreamPass",
    pass_stage=PassStage.AFTER_ASSIGN_LOGIC_STREAM,
    descriptor_kind="stream",
)
class DAGStreamPass(StreamPass):
    """Allocate logical streams for the root graph and all its subgraphs."""

    def run(self, graph: Graph, context: StreamPassContext) -> StatusLike:
        """Run MiniDAG against the borrowed public ``ge.graph.Graph``."""

        if graph is None:
            raise RuntimeError("MiniDAGStreamPass received a null Graph")
        if context is None:
            raise RuntimeError("MiniDAGStreamPass received a null StreamPassContext")
        try:
            # Route this module's records to GE's log before the first one.
            from ._minidag_adapter import _install_ge_log_handler  # lazy and cycle-safe

            _install_ge_log_handler(__name__)
            config = _resolve_auto_multistream_config(graph, context)
            if config.state in {"unset", "skip"}:
                return 0
            if config.state == "error":
                _LOGGER.error(
                    "%s is invalid, value=%s, reason=%s",
                    _AUTO_MULTISTREAM_OPTION,
                    config.value,
                    config.reason,
                )
                context.set_error_message(
                    f"{_AUTO_MULTISTREAM_OPTION} is invalid, value={config.value}, reason={config.reason}"
                )
                return False
            if (
                config.state != "dag"
                or config.strategy is None
                or config.max_stream_num is None
            ):
                context.set_error_message("invalid auto multistream configuration")
                return False

            self._run_graphs(graph, context, config)
            return 0
        except Exception as exc:
            setter = getattr(context, "set_error_message", None)
            if callable(setter):
                try:
                    setter(str(exc))
                except Exception:
                    _LOGGER.exception("Failed to record MiniDAG pass error: %s", exc)
            return False

    def _run_for_one_graph(
        self,
        graph: Any,
        context: Any,
        max_stream_num: int,
        strategy: str,
        graph_kind: str,
    ) -> None:
        # Preserve GE's five-step order: reserve a base id, build/plan,
        # reserve the additional ids, then write each node's result.
        from ._minidag_adapter import DAGAdapter  # lazy and cycle-safe

        try:
            base_stream_id = context.allocate_next_stream_id()
            adapter = DAGAdapter()
            required_streams = adapter.plan(
                graph,
                context,
                max_stream_num=max_stream_num,
                base_stream_id=base_stream_id,
                merge_strategy=strategy,
            )
            for _ in range(required_streams):
                context.allocate_next_stream_id()
            adapter.write_back(graph, context)
        except Exception as exc:
            # Keep the C++ "root graph / sub graph RunMiniDAGStreamPass failed"
            # wording and add which graph failed and why.
            graph_name = getattr(graph, "name", "")
            _LOGGER.error(
                "%s RunMiniDAGStreamPass failed, graph=%s, reason=%s",
                graph_kind,
                graph_name,
                exc,
            )
            raise RuntimeError(
                f"{graph_kind} RunMiniDAGStreamPass failed, graph={graph_name}, reason={exc}"
            ) from exc

    def _run_graphs(self, graph, context, config) -> None:
        _LOGGER.info(
            "Auto multistream requested mode=%s, strategy=%s, max_stream_num=%d.",
            config.value,
            config.strategy,
            config.max_stream_num,
        )
        self._run_for_one_graph(
            graph, context, config.max_stream_num, config.strategy, "root graph"
        )
        for subgraph in graph.get_all_subgraphs():
            self._run_for_one_graph(
                subgraph,
                context,
                config.max_stream_num,
                config.strategy,
                "sub graph",
            )


# Keep the historical implementation name importable for diagnostics; the
# descriptor itself intentionally retains the stable ``MiniDAGStreamPass`` name.
MiniDAGStreamPass = DAGStreamPass
