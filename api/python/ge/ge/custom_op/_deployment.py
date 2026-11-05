#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Compile-time freezing of Python custom op deployment artifacts.

An artifact is one ordinary ``.py`` definition file plus every implementation
descriptor declared in it.  Freezing happens once, right after the plugin
modules are imported, so that OM saving never re-reads the filesystem and can
not observe a file that changed after registration.

Every registered implementation whose definition file can be located is frozen,
with no per-class packaging switch: mirroring how C++ custom ops unconditionally
collect their SO into ``SO_BINS``, whether an implementation actually enters an
OM is decided later by the save side (is the op used by the graph?), not by a
declaration on the implementation.  Plugins whose definition file cannot be
located on disk (zipimport, interactive definitions, ``exec``/dynamic classes)
are skipped and keep working for eager execution exactly as before.
"""

import hashlib
import inspect
import sys
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

from .registry import (
    INTERFACE_ANNOTATED_ARGS,
    INTERFACE_COMPILABLE,
    INTERFACE_EAGER_EXECUTE,
    INTERFACE_HOST_CPU_EXECUTE,
    INTERFACE_PORTABLE,
)

# CustomOpCapability bit values.  Must stay in sync with
# inc/graph_metadef/graph/custom_op/capability.h; the UT
# python_custom_op_deployment_test.py asserts every value.
CAPABILITY_EAGER_EXECUTE = 1 << 0
CAPABILITY_COMPILABLE = 1 << 1
CAPABILITY_SHAPE_INFER = 1 << 2
CAPABILITY_PORTABLE = 1 << 3
CAPABILITY_ARGS_UPDATER = 1 << 4
CAPABILITY_ANNOTATED_ARGS = 1 << 5
CAPABILITY_INFER_META = 1 << 6
CAPABILITY_HOST_CPU_EXECUTE = 1 << 7
CAPABILITY_RESOURCE_REPORTING = 1 << 8

# Mirrors ParseInterfaces() in
# runtime/custom_op/python_custom_op_bridge_descriptors.cc.  Only implementation
# interfaces map here: kShapeInfer/kInferMeta belong to the prototype side and
# are never carried by the implementation-side manifest.
_INTERFACE_TO_CAPABILITY = {
    INTERFACE_EAGER_EXECUTE: CAPABILITY_EAGER_EXECUTE,
    INTERFACE_HOST_CPU_EXECUTE: CAPABILITY_HOST_CPU_EXECUTE,
    INTERFACE_COMPILABLE: CAPABILITY_COMPILABLE,
    INTERFACE_ANNOTATED_ARGS: CAPABILITY_ANNOTATED_ARGS,
    INTERFACE_PORTABLE: CAPABILITY_PORTABLE,
}

_PY_SUFFIX = ".py"
_SHA256_HEX_LEN = 64


class DeploymentArtifactError(RuntimeError):
    """Raised when a registered implementation can not be frozen."""


@dataclass(frozen=True)
class OpDescriptorSnapshot:
    """Cross-process stable snapshot of one implementation descriptor.

    ``descriptor_key`` and ``module_name`` are deliberately absent: both are
    derived from ``abs(hash(str(path)))`` in the plugin loader and therefore
    change between processes under PYTHONHASHSEED randomization.
    """

    op_type: str
    class_name: str
    capabilities: int


# 原型声明条目的固定 capability 位：恢复侧按声明补回 kShapeInfer/kInferMeta
# （restore hook 的 infer_meta trampoline 与编译期原型注册同一语义）。
# 该组合不含任何实现位，是 C++ 侧 IsProtoDescriptor 的判定依据。
_PROTO_CAPABILITIES = CAPABILITY_SHAPE_INFER | CAPABILITY_INFER_META


@dataclass(frozen=True)
class ProtoSnapshot:
    """Cross-process stable snapshot of one prototype declaration.

    Prototypes usually live in the same file as their implementations and ride
    along with the implementation artifact.  When a prototype is declared in a
    separate file, that file is frozen as its own artifact so the deployed
    environment can restore the ``register_op`` declaration (and with it the
    Python shape-inference capability).  ``class_name`` stores the decorated
    function's ``__qualname__``: the restore side compares it against the
    declaration captured in the model scope.
    """

    op_type: str
    class_name: str
    capabilities: int = _PROTO_CAPABILITIES


@dataclass(frozen=True)
class DeploymentArtifact:
    """One frozen definition file and every descriptor declared in it."""

    artifact_id: str
    source_name: str
    source_sha256: bytes
    source: bytes
    descriptors: Tuple[object, ...]

    def to_bridge_dict(self) -> dict:
        return {
            "artifact_id": self.artifact_id,
            "source_name": self.source_name,
            "source_sha256": self.source_sha256,
            "source": self.source,
            "producer_python_major": sys.version_info.major,
            "producer_python_minor": sys.version_info.minor,
            "descriptors": [
                {
                    "op_type": item.op_type,
                    "class_name": item.class_name,
                    "capabilities": item.capabilities,
                }
                for item in self.descriptors
            ],
        }


def capabilities_from_interfaces(interfaces: Iterable[str]) -> int:
    """Convert reflected interface names into a CustomOpCapabilityMask."""

    capabilities = 0
    for interface in interfaces:
        capability = _INTERFACE_TO_CAPABILITY.get(interface)
        if capability is None:
            raise DeploymentArtifactError(
                f"unknown python custom op interface: {interface!r}"
            )
        capabilities |= capability
    return capabilities


def resolve_source_file(cls: type) -> Path:
    """Resolve the ordinary ``.py`` file that defines ``cls``.

    Rejects anything that is not an existing regular ``.py`` file: ``.pyc``,
    directories, archive members, interactive definitions and dynamically
    generated classes all fail here.
    """

    candidate = None
    try:
        candidate = inspect.getsourcefile(cls)
    except TypeError:
        candidate = None
    if candidate is None:
        module = sys.modules.get(getattr(cls, "__module__", ""), None)
        candidate = getattr(module, "__file__", None)
    if not candidate:
        raise DeploymentArtifactError(
            f"can not locate the definition file of {cls.__module__}.{cls.__qualname__}"
        )

    path = Path(candidate)
    if path.suffix != _PY_SUFFIX:
        raise DeploymentArtifactError(
            f"definition file of {cls.__module__}.{cls.__qualname__} is not a .py file: {path.name}"
        )
    try:
        resolved = path.resolve()
    except OSError as exc:
        raise DeploymentArtifactError(
            f"can not resolve the definition file of {cls.__module__}.{cls.__qualname__}: {exc}"
        ) from exc
    if not resolved.is_file():
        raise DeploymentArtifactError(
            f"definition file of {cls.__module__}.{cls.__qualname__} is not an existing regular file"
        )
    return resolved


def _build_snapshot(descriptor) -> OpDescriptorSnapshot:
    cls = getattr(descriptor, "cls", None)
    if not inspect.isclass(cls):
        raise DeploymentArtifactError(
            f"python custom op descriptor for op type {descriptor.op_type} carries no implementation class"
        )
    return OpDescriptorSnapshot(
        op_type=descriptor.op_type,
        class_name=cls.__qualname__,
        capabilities=capabilities_from_interfaces(descriptor.interfaces),
    )


def _freeze_one_file(source_file: Path, snapshots: List[object]) -> DeploymentArtifact:
    try:
        raw = source_file.read_bytes()
    except OSError as exc:
        raise DeploymentArtifactError(
            f"failed to read python custom op definition file {source_file.name}: {exc}"
        ) from exc
    if not raw:
        raise DeploymentArtifactError(
            f"python custom op definition file {source_file.name} is empty"
        )
    digest = hashlib.sha256(raw).digest()
    ordered = tuple(sorted(snapshots, key=lambda item: (item.op_type, item.class_name)))
    return DeploymentArtifact(
        artifact_id=digest.hex(),
        source_name=source_file.name,
        source_sha256=digest,
        source=raw,
        descriptors=ordered,
    )


def _group_by_definition_file(
    descriptors: Iterable[object],
) -> Dict[Path, List[OpDescriptorSnapshot]]:
    """Group descriptors by definition file.

    Every registered implementation is frozen: there is no per-class packaging
    switch, mirroring how C++ custom ops unconditionally collect their SO.  The
    save side later decides whether an op actually enters the OM based on graph
    usage.  Source resolution is best-effort: a class whose definition file can
    not be located (``exec``/dynamic creation, zipimport, interactive
    definitions) is silently skipped rather than raising, so such plugins keep
    working for eager execution.
    """

    grouped: Dict[Path, List[OpDescriptorSnapshot]] = {}
    for descriptor in descriptors:
        snapshot = _build_snapshot(descriptor)
        try:
            source_file = resolve_source_file(descriptor.cls)
        except DeploymentArtifactError:
            # 不可定位：永远不进 OM，跳过即可，不阻断 plugin 加载。
            continue
        grouped.setdefault(source_file, []).append(snapshot)
    return grouped


_LOCK = threading.RLock()
# 按定义文件缓存已冻结的 artifact，而不是缓存整个结果集：
# - 同一文件只读取一次，之后的编译/保存始终使用已冻结 bytes（TOCTOU 保证）；
# - 运行期间动态发现新插件（LoadPythonCustomOpsIfNeeded 的设计场景）时，
#   新文件正常冻结、既有文件复用缓存，不会因结果集整体缓存而过期。
_FROZEN_BY_FILE: Dict[Path, DeploymentArtifact] = {}


def resolve_source_file_of_function(fn) -> Path:
    """Resolve the ordinary ``.py`` file that defines ``fn`` (a prototype's decorated function)."""

    if not inspect.isfunction(fn):
        raise DeploymentArtifactError(
            "prototype must be declared by a plain Python function"
        )
    return resolve_source_file(fn)


def _declare_protos_in_groups(
    grouped: Dict[Path, List[OpDescriptorSnapshot]],
    protos: Iterable[object],
) -> Dict[Path, List[ProtoSnapshot]]:
    """Record prototype declarations into per-file proto groups.

    Returns prototypes grouped by their (non-implementation) definition file.
    Prototypes sharing a file with implementations are already covered by that
    file's artifact -- executing the file restores both the ``register_op`` and
    the ``register_op_impl`` declarations.  Unlocatable prototypes are silently
    skipped, matching the implementation-side policy; the freeze entry point
    reports any prototype whose source did not end up in an artifact.
    """

    proto_groups: Dict[Path, List[ProtoSnapshot]] = {}
    for proto in protos:
        fn = getattr(proto, "infer_func", None)
        if fn is None:
            continue
        try:
            source_file = resolve_source_file_of_function(fn)
        except DeploymentArtifactError:
            # 不可定位：与实现侧同策略，静默跳过。
            continue
        if source_file in grouped:
            # 原型与实现同文件：文件已冻结，执行时声明自然恢复，无需独立条目。
            continue
        proto_groups.setdefault(source_file, []).append(
            ProtoSnapshot(op_type=proto.op_type, class_name=proto.func_name)
        )
    return proto_groups


def freeze_registered_artifacts(
    descriptors: Iterable[object],
    protos: Iterable[object] = (),
) -> List[DeploymentArtifact]:
    """Freeze per definition file and cache each file's immutable artifact.

    ``protos`` are prototype descriptors (``OpProtoDescriptor``) as returned by
    ``proto.get_registered_op_protos``.  A prototype declared in a file that
    also defines implementations rides along with that file's artifact; a
    prototype in a separate file gets its own artifact so the deployed
    environment can restore the ``register_op`` declaration (and with it the
    Python shape-inference capability, which the implementation-side manifest
    deliberately does not carry).
    """

    grouped = _group_by_definition_file(descriptors)
    proto_groups = _declare_protos_in_groups(grouped, protos)

    with _LOCK:
        artifacts: List[DeploymentArtifact] = []
        for source_file, snapshots in grouped.items():
            cached = _FROZEN_BY_FILE.get(source_file)
            if cached is not None:
                artifacts.append(cached)
                continue
            frozen = _freeze_one_file(source_file, snapshots)
            _FROZEN_BY_FILE[source_file] = frozen
            artifacts.append(frozen)
        for source_file, snapshots in proto_groups.items():
            cached = _FROZEN_BY_FILE.get(source_file)
            if cached is not None:
                artifacts.append(cached)
                continue
            frozen = _freeze_one_file(source_file, snapshots)
            _FROZEN_BY_FILE[source_file] = frozen
            artifacts.append(frozen)
        return sorted(artifacts, key=lambda item: item.artifact_id)


def get_frozen_artifact_dicts() -> List[dict]:
    with _LOCK:
        return [
            item.to_bridge_dict()
            for item in sorted(
                _FROZEN_BY_FILE.values(), key=lambda artifact: artifact.artifact_id
            )
        ]


def clear_frozen_artifacts() -> None:
    with _LOCK:
        _FROZEN_BY_FILE.clear()
