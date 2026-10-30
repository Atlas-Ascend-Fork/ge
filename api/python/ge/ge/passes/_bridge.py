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

"""Bridge-facing Python runtime helpers for GE passes."""

from __future__ import annotations

import logging
import sys
import threading
from collections.abc import Iterable
from dataclasses import dataclass
from typing import TYPE_CHECKING, Dict, List, Optional, cast

from ._native import (
    borrow_match_result,
    borrow_node,
    clone_pattern_matcher_config,
    release_graph,
)
from .base import (
    DecomposePass,
    FusionBasePass,
    PassContext,
    PatternFusionPass,
    PatternMatcherConfig,
    ResourcePlanPass,
    StatusLike,
    StreamPass,
    StreamPassContext,
)
from .bootstrap import get_registered_passes, load_pass_plugins
from .pattern import ensure_pattern
from .registry import _RESOURCE_PLAN_SPECS, get_registered_pass_by_descriptor_key

if TYPE_CHECKING:
    from ge.graph import Graph


@dataclass
class _PassHolder:
    descriptor_key: str
    instance_id: str
    instance: FusionBasePass | ResourcePlanPass


_HOLDER_LOCK = threading.RLock()
_PASS_HOLDERS: Dict[str, _PassHolder] = {}


def load_and_get_pass_descriptors() -> list:
    load_pass_plugins()
    # Fusion/decompose registration keeps its historical order.  Stream
    # callbacks, however, have a deterministic pass-name order independent of
    # plugin discovery/decorator order (the native bridge routes this suffix
    # through the dedicated registrar).
    descriptors = get_registered_passes()
    stream_descriptors = sorted(
        (item for item in descriptors if item.get("kind") in _RESOURCE_PLAN_SPECS),
        key=lambda item: item.get("pass_name", ""),
    )
    stream_iter = iter(stream_descriptors)
    ordered = []
    for item in descriptors:
        ordered.append(
            next(stream_iter) if item.get("kind") in _RESOURCE_PLAN_SPECS else item
        )
    return ordered


def _get_holder(instance_id: str) -> _PassHolder:
    with _HOLDER_LOCK:
        holder = _PASS_HOLDERS.get(instance_id)
    if holder is None:
        raise KeyError(f"python pass holder is not created: {instance_id}")
    return holder


def _get_fusion_base_pass(instance_id: str) -> FusionBasePass:
    instance = _get_holder(instance_id).instance
    if not isinstance(instance, FusionBasePass):
        raise TypeError(f"python pass holder is not FusionBasePass: {instance_id}")
    return instance


def _get_pattern_fusion_pass(instance_id: str) -> PatternFusionPass:
    instance = _get_holder(instance_id).instance
    if not isinstance(instance, PatternFusionPass):
        raise TypeError(f"python pass holder is not PatternFusionPass: {instance_id}")
    return instance


def _get_decompose_pass(instance_id: str) -> DecomposePass:
    instance = _get_holder(instance_id).instance
    if not isinstance(instance, DecomposePass):
        raise TypeError(f"python pass holder is not DecomposePass: {instance_id}")
    return instance


def _get_resource_plan_pass(
    instance_id: str, descriptor_kind: Optional[str] = None
) -> ResourcePlanPass:
    """Return a holder instance after validating its descriptor kind.

    The descriptor kind, rather than the Python class name or execution stage,
    is the native routing contract.  Keeping this check at the bridge boundary
    prevents a future resource-plan handler from accidentally invoking a
    stream pass with the wrong context.
    """

    holder = _get_holder(instance_id)
    descriptor = get_registered_pass_by_descriptor_key(holder.descriptor_key)
    if descriptor is None:
        raise KeyError(f"python pass descriptor_key not found: {holder.descriptor_key}")
    if descriptor_kind is not None and descriptor.kind != descriptor_kind:
        raise TypeError(
            f"python resource plan kind mismatch: expected {descriptor_kind}, "
            f"got {descriptor.kind}"
        )
    instance = holder.instance
    if not isinstance(instance, ResourcePlanPass):
        raise TypeError(f"python pass holder is not ResourcePlanPass: {instance_id}")
    return instance


def create_pass_holder(instance_id: str, descriptor_key: str) -> bool:
    descriptor = get_registered_pass_by_descriptor_key(descriptor_key)
    if descriptor is None:
        raise KeyError(f"python pass descriptor_key not found: {descriptor_key}")
    with _HOLDER_LOCK:
        if instance_id in _PASS_HOLDERS:
            return True
        _PASS_HOLDERS[instance_id] = _PassHolder(
            descriptor_key=descriptor_key,
            instance_id=instance_id,
            instance=descriptor.cls(),
        )
    return True


def destroy_pass_holder(instance_id: str) -> bool:
    with _HOLDER_LOCK:
        return _PASS_HOLDERS.pop(instance_id, None) is not None


def run_fusion_base_pass(
    instance_id: str, graph: Graph, context: Optional[PassContext] = None
) -> StatusLike:
    pass_instance = _get_fusion_base_pass(instance_id)
    if context is not None and not isinstance(context, PassContext):
        raise TypeError("context type error")
    return pass_instance.run(graph, cast(PassContext, context))


def run_resource_plan_pass(
    instance_id: str, graph: Graph, context: object
) -> StatusLike:
    """Invoke a kind-routed resource planning pass.

    ``graph`` is the borrowed public ``ge.graph.Graph`` wrapper.  No snapshot
    or private graph view is created here.  The stream handler currently
    requires ``StreamPassContext``; future handlers can add their own
    kind-specific context check without changing this entry point.
    """

    try:
        descriptor = get_registered_pass_by_descriptor_key(
            _get_holder(instance_id).descriptor_key
        )
        if descriptor is None:
            raise KeyError(
                f"python pass descriptor_key not found for holder: {instance_id}"
            )
        pass_instance = _get_resource_plan_pass(instance_id, descriptor.kind)
        from ge.graph import Graph as GraphType

        if not isinstance(graph, GraphType):
            raise TypeError(
                f"resource plan graph must be a ge.graph.Graph, got {type(graph).__name__}"
            )
        if descriptor.kind == "stream":
            if not isinstance(pass_instance, StreamPass):
                raise TypeError("stream resource plan pass must be a StreamPass")
            # Exact type, not a duck check.  An unregistered pybind derived
            # class comes back as a plain PassContext, and a future kind's
            # context may well carry the same method names; both must be
            # rejected here rather than reaching the pass.
            if not isinstance(context, StreamPassContext):
                raise TypeError(
                    "stream resource plan context must be a StreamPassContext, "
                    f"got {type(context).__name__}"
                )
        return pass_instance.run(graph, context)
    except Exception as exc:
        setter = getattr(context, "set_error_message", None)
        if callable(setter):
            try:
                setter(str(exc))
            except Exception:
                logging.getLogger(__name__).exception(
                    "Failed to record resource-plan pass error: %s", exc
                )
        raise


def _release_replacement_graph(replacement: Graph, pass_name: str) -> int:
    from ge.graph import Graph

    if not isinstance(replacement, Graph):
        raise TypeError(f"{pass_name}.replacement must return ge.graph.Graph")
    return release_graph(replacement)


def get_pass_patterns(instance_id: str) -> List[int]:
    pass_instance = _get_pattern_fusion_pass(instance_id)
    patterns = pass_instance.patterns()
    if patterns is None:
        return []
    if not isinstance(patterns, Iterable) or isinstance(patterns, (str, bytes)):
        raise TypeError(
            "PatternFusionPass.patterns must return an iterable of Pattern or Graph"
        )

    released_patterns = []
    for item in patterns:
        pattern = ensure_pattern(item)
        released_patterns.append(pattern.release())
    return released_patterns


def get_pattern_matcher_config(instance_id: str) -> Optional[int]:
    pass_instance = _get_pattern_fusion_pass(instance_id)
    matcher_config = pass_instance.matcher_config
    if matcher_config is None:
        return None
    if not isinstance(matcher_config, PatternMatcherConfig):
        raise TypeError(
            "PatternFusionPass.matcher_config must be PatternMatcherConfig or None"
        )
    return clone_pattern_matcher_config(matcher_config)


def call_meet_requirements(
    instance_id: str, match_result_handle: int, context: Optional[PassContext] = None
) -> bool:
    pass_instance = _get_pattern_fusion_pass(instance_id)
    match_result = borrow_match_result(match_result_handle)
    try:
        return bool(pass_instance.meet_requirements(match_result, context))
    finally:
        match_result._invalidate()


def call_replacement(
    instance_id: str, match_result_handle: int, context: Optional[PassContext] = None
) -> int:
    pass_instance = _get_pattern_fusion_pass(instance_id)
    match_result = borrow_match_result(match_result_handle)
    try:
        replacement = pass_instance.replacement(match_result, context)
    finally:
        match_result._invalidate()

    return _release_replacement_graph(replacement, "PatternFusionPass")


def call_decompose_meet_requirements(
    instance_id: str, node_handle: int, context: Optional[PassContext] = None
) -> bool:
    pass_instance = _get_decompose_pass(instance_id)
    node = borrow_node(node_handle)
    return bool(pass_instance.meet_requirements(node, context))


def call_decompose_replacement(
    instance_id: str, node_handle: int, context: Optional[PassContext] = None
) -> int:
    pass_instance = _get_decompose_pass(instance_id)
    node = borrow_node(node_handle)
    replacement = pass_instance.replacement(node, context)
    return _release_replacement_graph(replacement, "DecomposePass")


def clear_pass_holders() -> None:
    with _HOLDER_LOCK:
        _PASS_HOLDERS.clear()


def clear_loaded_pass_modules() -> None:
    """Clear all dynamically loaded pass modules from sys.modules to avoid test pollution."""
    keys_to_remove = [key for key in sys.modules if key.startswith("_ge_py_pass_")]
    for key in keys_to_remove:
        del sys.modules[key]
