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

"""Python pass registry and decorators."""

import inspect
import threading
from collections.abc import Iterable as IterableABC
from dataclasses import dataclass, field
from typing import Dict, FrozenSet, Iterable, List, Optional, Type, Union

from .base import (
    DecomposePass,
    FusionBasePass,
    PassStage,
    PatternFusionPass,
    ResourcePlanPass,
    StreamPass,
)

PASS_KIND_FUSION_BASE = "fusion_base"
PASS_KIND_PATTERN_FUSION = "pattern_fusion"
PASS_KIND_DECOMPOSE = "decompose"
PASS_KIND_STREAM = "stream"

_PASS_KINDS = {
    PASS_KIND_FUSION_BASE,
    PASS_KIND_PATTERN_FUSION,
    PASS_KIND_DECOMPOSE,
    PASS_KIND_STREAM,
}


@dataclass(frozen=True)
class ResourcePlanSpec:
    """Registration contract for one resource-planning descriptor kind."""

    kind: str
    base_cls: Type[ResourcePlanPass]
    allowed_stages: FrozenSet[PassStage]
    handler_key: str


_RESOURCE_PLAN_SPECS: Dict[str, ResourcePlanSpec] = {
    PASS_KIND_STREAM: ResourcePlanSpec(
        kind=PASS_KIND_STREAM,
        base_cls=StreamPass,
        allowed_stages=frozenset({PassStage.AFTER_ASSIGN_LOGIC_STREAM}),
        handler_key=PASS_KIND_STREAM,
    ),
}


@dataclass(frozen=True)
class PassDescriptor:
    """Normalized Python pass descriptor."""

    descriptor_key: str
    pass_name: str
    module_name: str
    class_name: str
    stage: PassStage
    kind: str
    cls: Type[Union[FusionBasePass, ResourcePlanPass]]
    op_types: List[str] = field(default_factory=list)

    def to_bridge_dict(self) -> dict:
        return {
            "descriptor_key": self.descriptor_key,
            "pass_name": self.pass_name,
            "module_name": self.module_name,
            "class_name": self.class_name,
            "stage": self.stage.value,
            "kind": self.kind,
            "op_types": list(self.op_types),
        }


class _PassRegistry:
    def __init__(self) -> None:
        self._lock = threading.RLock()
        self._descriptor_key_to_desc: Dict[str, PassDescriptor] = {}
        self._pass_name_to_desc: Dict[str, PassDescriptor] = {}

    def clear(self) -> None:
        with self._lock:
            self._descriptor_key_to_desc.clear()
            self._pass_name_to_desc.clear()

    def register(self, descriptor: PassDescriptor) -> PassDescriptor:
        with self._lock:
            if descriptor.descriptor_key in self._descriptor_key_to_desc:
                raise ValueError(
                    f"python pass descriptor_key already exists: {descriptor.descriptor_key}"
                )
            if descriptor.pass_name in self._pass_name_to_desc:
                raise ValueError(
                    f"python pass name already exists: {descriptor.pass_name}"
                )
            self._descriptor_key_to_desc[descriptor.descriptor_key] = descriptor
            self._pass_name_to_desc[descriptor.pass_name] = descriptor
        return descriptor

    def get_by_descriptor_key(self, descriptor_key: str) -> Optional[PassDescriptor]:
        with self._lock:
            return self._descriptor_key_to_desc.get(descriptor_key)

    def get_all(self) -> List[PassDescriptor]:
        with self._lock:
            descriptors = list(self._descriptor_key_to_desc.values())
        # Keep legacy fusion/decompose insertion order, but make the stream
        # subset deterministic by pass name.  The native CustomPassHelper has
        # the same ordering rule at execution time; sorting here also makes
        # bridge descriptors stable for tests and diagnostics.
        resource_descriptors = iter(
            sorted(
                (item for item in descriptors if item.kind in _RESOURCE_PLAN_SPECS),
                key=lambda item: item.pass_name,
            )
        )
        return [
            next(resource_descriptors) if item.kind in _RESOURCE_PLAN_SPECS else item
            for item in descriptors
        ]


_PASS_REGISTRY = _PassRegistry()


def _build_descriptor_key(module_name: str, class_name: str, pass_name: str) -> str:
    return f"{module_name}:{class_name}:{pass_name}"


def _normalize_pass_name(name: str, decorator_name: str = "pass") -> str:
    if not isinstance(name, str):
        raise TypeError(f"{decorator_name} name must be a string")
    if not name:
        raise ValueError(f"{decorator_name} name must be a non-empty string")
    return name


def _normalize_stage(stage: PassStage, decorator_name: str) -> PassStage:
    if isinstance(stage, PassStage):
        return stage
    # Accepting the serialized value is convenient for dynamically generated
    # plugins, while still rejecting arbitrary values with a useful error.
    if isinstance(stage, str):
        try:
            return PassStage(stage)
        except ValueError as exc:
            raise ValueError(f"{decorator_name} stage is invalid: {stage!r}") from exc
    raise TypeError(f"{decorator_name} stage must be a PassStage")


def _normalize_decompose_op_types(op_types: Iterable[str]) -> List[str]:
    if isinstance(op_types, (str, bytes)) or not isinstance(op_types, IterableABC):
        raise TypeError(
            "register_decompose_pass op_types must be an iterable of strings"
        )

    normalized_op_types = list(op_types)
    if not normalized_op_types:
        raise ValueError("register_decompose_pass requires at least one op type")

    for op_type in normalized_op_types:
        if not isinstance(op_type, str) or not op_type:
            raise TypeError(
                "register_decompose_pass op_types must contain non-empty strings"
            )
    return normalized_op_types


def _register_pass_class(
    cls: Type[Union[FusionBasePass, ResourcePlanPass]],
    *,
    kind: str,
    name: str,
    stage: PassStage,
    op_types: Optional[Iterable[str]] = None,
) -> Type[Union[FusionBasePass, ResourcePlanPass]]:
    if kind not in _PASS_KINDS:
        raise ValueError(f"unsupported Python pass kind: {kind!r}")
    name = _normalize_pass_name(name)
    stage = _normalize_stage(stage, "register_pass")
    if not inspect.isclass(cls):
        raise TypeError("Python pass registration expects a class")
    module_name = cls.__module__
    class_name = cls.__name__
    descriptor = PassDescriptor(
        descriptor_key=_build_descriptor_key(module_name, class_name, name),
        pass_name=name,
        module_name=module_name,
        class_name=class_name,
        stage=stage,
        kind=kind,
        cls=cls,
        op_types=list(op_types or []),
    )
    _PASS_REGISTRY.register(descriptor)
    setattr(cls, "__ge_pass_descriptor__", descriptor)
    return cls


def register_fusion_pass(
    *, name: str, stage: PassStage, kind: Optional[str] = None
) -> callable:
    """Decorator for FusionBasePass and PatternFusionPass."""

    normalized_name = _normalize_pass_name(name, "register_fusion_pass")
    normalized_stage = _normalize_stage(stage, "register_fusion_pass")
    if normalized_stage == PassStage.AFTER_ASSIGN_LOGIC_STREAM:
        raise ValueError(
            "register_fusion_pass does not accept PassStage.AFTER_ASSIGN_LOGIC_STREAM; "
            "use register_resource_plan_pass"
        )
    if kind == PASS_KIND_STREAM:
        raise ValueError(
            "stream passes must be registered with register_resource_plan_pass"
        )

    def decorator(cls: Type[FusionBasePass]) -> Type[FusionBasePass]:
        if not inspect.isclass(cls) or not issubclass(cls, FusionBasePass):
            raise TypeError("register_fusion_pass expects a FusionBasePass subclass")
        pass_kind = kind
        if pass_kind is None:
            pass_kind = (
                PASS_KIND_PATTERN_FUSION
                if issubclass(cls, PatternFusionPass)
                else PASS_KIND_FUSION_BASE
            )
        if pass_kind not in (PASS_KIND_FUSION_BASE, PASS_KIND_PATTERN_FUSION):
            raise ValueError(
                "register_fusion_pass kind must be 'fusion_base' or 'pattern_fusion'"
            )
        return _register_pass_class(
            cls, kind=pass_kind, name=normalized_name, stage=normalized_stage
        )

    return decorator


def register_decompose_pass(
    *, name: str, stage: PassStage, op_types: Iterable[str]
) -> callable:
    """Decorator for DecomposePass."""

    normalized_name = _normalize_pass_name(name, "register_decompose_pass")
    normalized_stage = _normalize_stage(stage, "register_decompose_pass")
    if normalized_stage == PassStage.AFTER_ASSIGN_LOGIC_STREAM:
        raise ValueError(
            "register_decompose_pass does not accept PassStage.AFTER_ASSIGN_LOGIC_STREAM"
        )
    normalized_op_types = _normalize_decompose_op_types(op_types)

    def decorator(cls: Type[DecomposePass]) -> Type[DecomposePass]:
        if not inspect.isclass(cls) or not issubclass(cls, DecomposePass):
            raise TypeError("register_decompose_pass expects a DecomposePass subclass")
        setattr(cls, "op_types", list(normalized_op_types))
        return _register_pass_class(
            cls,
            kind=PASS_KIND_DECOMPOSE,
            name=normalized_name,
            stage=normalized_stage,
            op_types=normalized_op_types,
        )

    return decorator


def register_resource_plan_pass(
    *, name: str, pass_stage: PassStage, descriptor_kind: str
) -> callable:
    """Register a resource-planning pass with an explicit descriptor kind."""

    normalized_name = _normalize_pass_name(name, "register_resource_plan_pass")
    normalized_stage = _normalize_stage(pass_stage, "register_resource_plan_pass")
    if not isinstance(descriptor_kind, str) or not descriptor_kind:
        raise TypeError(
            "register_resource_plan_pass descriptor_kind must be a non-empty string"
        )
    spec = _RESOURCE_PLAN_SPECS.get(descriptor_kind)
    if spec is None:
        legal = ", ".join(sorted(_RESOURCE_PLAN_SPECS))
        raise ValueError(
            f"unsupported resource plan descriptor kind: {descriptor_kind!r}; "
            f"supported kinds: {legal}"
        )
    if normalized_stage not in spec.allowed_stages:
        allowed = ", ".join(
            stage.value
            for stage in sorted(spec.allowed_stages, key=lambda item: item.value)
        )
        raise ValueError(
            f"resource plan kind {descriptor_kind!r} does not allow pass stage "
            f"{normalized_stage.value!r}; allowed stages: {allowed}"
        )

    def decorator(cls: Type[ResourcePlanPass]) -> Type[ResourcePlanPass]:
        if not inspect.isclass(cls) or not issubclass(cls, spec.base_cls):
            raise TypeError(
                f"register_resource_plan_pass kind {descriptor_kind!r} expects a "
                f"{spec.base_cls.__name__} subclass"
            )
        return _register_pass_class(
            cls,
            kind=spec.kind,
            name=normalized_name,
            stage=normalized_stage,
        )

    return decorator


def clear_registered_passes() -> None:
    _PASS_REGISTRY.clear()


def get_registered_passes() -> List[PassDescriptor]:
    return _PASS_REGISTRY.get_all()


def get_registered_pass_dicts() -> List[dict]:
    return [item.to_bridge_dict() for item in get_registered_passes()]


def get_registered_pass_by_descriptor_key(
    descriptor_key: str,
) -> Optional[PassDescriptor]:
    return _PASS_REGISTRY.get_by_descriptor_key(descriptor_key)
