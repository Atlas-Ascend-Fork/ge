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

"""Model-scoped restoration state for Python custom ops loaded from an OM.

One scope corresponds to one ``PYTHON_CUSTOM_OPS`` restore (one model load).
While a scope is active on the current context, ``register_op_impl`` captures
descriptors into the scope instead of the process-global registry, and
``register_op`` only records prototype declarations (they are never pushed to
the C++ ``OperatorFactoryImpl`` during load, per design rule D4).

Embedded sources are initialized strictly in memory: the bytes are compiled
with a synthetic filename that never points at the filesystem, executed in a
fresh module that is inserted into ``sys.modules`` before execution and
removed on failure or when the scope closes.  Nothing is written to disk, no
temporary ``.py``/``.pyc`` is created and ``sys.path`` is never modified.

Scope ids are assigned by the C++ restore hook (process-wide, 64-bit,
monotonically increasing, never recycled).  This module mirrors that contract
defensively: opening a scope with a non-increasing or previously used id is
rejected, so a buggy caller can not alias two models onto one scope.
"""

import sys
import threading
from contextvars import ContextVar
from types import ModuleType
from typing import Dict, List, Mapping, Optional, Set

_SCOPE_LOCK = threading.RLock()
# scope_id -> live scope.  Closed scopes are removed; their ids stay in
# _USED_SCOPE_IDS forever (never recycled, design rule D3).
_SCOPES: Dict[int, "_ModelScope"] = {}
_USED_SCOPE_IDS: Set[int] = set()
_HIGHEST_SCOPE_ID = 0

# Id of the scope active on the current context, or None outside a restore.
# ContextVar keeps concurrent eager registrations on other threads routing to
# the global registry even while a load is restoring in-memory sources.
_ACTIVE_SCOPE_ID: ContextVar[Optional[int]] = ContextVar(
    "ge_custom_op_model_scope", default=None
)

# artifact_id of the embedded source being executed on the current context.
# Declarations captured during that exec must record which artifact they came
# from: artifact_id is the cross-model conflict fingerprint (source bytes'
# sha256), so two models loading the same source pass and two models loading
# different sources fail loudly -- no CPython bytecode-equality semantics
# involved.
_ACTIVE_ARTIFACT_ID: ContextVar[Optional[str]] = ContextVar(
    "ge_custom_op_model_scope_artifact", default=None
)

_MODULE_NAME_PREFIX = "_ge_om_custom_op_"


class ModelScopeError(RuntimeError):
    """Raised on invalid scope lifecycle operations or in-memory source failures."""


class _ModelScope:
    """Restore state of one model: captured impls, recorded protos, modules."""

    def __init__(self, scope_id: int) -> None:
        self.scope_id = scope_id
        # Guards the mutable collections below; source initialization itself is
        # serialized by the C++ load mutex, but captures must stay consistent
        # against concurrent scoped descriptor queries.
        self._lock = threading.RLock()
        self._impl_descriptors = []  # List[OpImplDescriptor]
        self._impl_by_descriptor_key: Dict[str, object] = {}
        self._proto_declarations = []  # List[OpProtoDescriptor], recorded only
        self._module_names: Dict[str, ModuleType] = {}  # module_name -> module
        # op_type -> canonical IR snapshot (dict with inputs/attrs/outputs)，
        # 由 C++ restore hook 在执行 embedded source 前随 manifest 一并注入（保存侧
        # 从编译进程 OperatorFactory 冻结）。D4：加载态原型未注册，schema-bound
        # execute/compile 的参数绑定以此为准。
        self._ir_by_op_type: Dict[str, dict] = {}
        # op_type -> artifact_id of the source that produced it.  artifact_id is
        # the sha256 of the definition file's bytes, i.e. a fingerprint with the
        # same granularity as the C++ side's SO fingerprint.  Cross-model
        # conflict detection compares these instead of the declarations'
        # contents: "same op_type from the same source file" passes, "same
        # op_type from different source files" fails -- exactly the C++ rule,
        # and no dependency on CPython code-object equality semantics.
        self._proto_artifact_by_op_type: Dict[str, str] = {}
        self._ir_artifact_by_op_type: Dict[str, str] = {}
        self._closed = False

    # -- capture (called by registry.py / proto.py while this scope is active) --

    def register_impl(self, descriptor) -> object:
        with self._lock:
            self._assert_open()
            if descriptor.descriptor_key in self._impl_by_descriptor_key:
                raise ValueError(
                    "python op impl descriptor_key already exists in model "
                    f"scope {self.scope_id}: {descriptor.descriptor_key}"
                )
            if any(
                descriptor.op_type == item.op_type for item in self._impl_descriptors
            ):
                raise ValueError(
                    "python op impl type already exists in model scope "
                    f"{self.scope_id}: {descriptor.op_type}"
                )
            self._impl_descriptors.append(descriptor)
            self._impl_by_descriptor_key[descriptor.descriptor_key] = descriptor
        return descriptor

    def declare_proto(self, descriptor) -> object:
        """Record a prototype declaration without registering it anywhere.

        Design rule D4: prototypes declared by embedded sources are consumed
        from the compile-time environment, never re-registered into the
        process-global proto registry or the C++ operator factory at load time.
        """
        with self._lock:
            self._assert_open()
            self._proto_declarations.append(descriptor)
            artifact_id = _ACTIVE_ARTIFACT_ID.get()
            if artifact_id is not None:
                self._proto_artifact_by_op_type[descriptor.op_type] = artifact_id
        return descriptor

    # -- queries --

    def get_impl_by_descriptor_key(self, descriptor_key: str):
        with self._lock:
            return self._impl_by_descriptor_key.get(descriptor_key)

    def get_ir_meta_by_op_type(self, op_type: str) -> Optional[dict]:
        """Return the frozen canonical IR snapshot of ``op_type``, or None.

        The bridge consults this when ``OperatorFactory`` has no entry (load
        time, design rule D4) to bind schema-bound execute/compile arguments.
        """
        with self._lock:
            return self._ir_by_op_type.get(op_type)

    def get_proto_artifact_id(self, op_type: str) -> Optional[str]:
        """Return the artifact_id of the source that declared ``op_type``'s prototype."""
        with self._lock:
            return self._proto_artifact_by_op_type.get(op_type)

    def get_ir_artifact_id(self, op_type: str) -> Optional[str]:
        """Return the artifact_id of the source that produced ``op_type``'s IR snapshot."""
        with self._lock:
            return self._ir_artifact_by_op_type.get(op_type)

    def get_proto_by_op_type(self, op_type: str):
        """Return the prototype declaration of ``op_type`` recorded in this scope, or None.

        Prototypes re-declared by an embedded source are never written into the
        process-global proto registry (design rule D4), so the scope itself is
        the only place that still holds the declaration -- including its
        ``infer_func``.  ``_infer_meta.call_infer_meta`` falls back to this when
        the global registry misses, which is always the case in a deployment
        process that never ran the compile-time registration.
        """
        with self._lock:
            for descriptor in self._proto_declarations:
                if descriptor.op_type == op_type:
                    return descriptor
            return None

    def collect_impl_descriptors(self) -> List[dict]:
        """Snapshot the captured impl descriptors for the C++ restore hook.

        ``capabilities`` are deliberately absent: the C++ side derives the
        capability mask from ``interfaces`` with its authoritative
        ``ParseInterfaces`` mapping, so the bit layout is never duplicated
        here.  ``class_qualname`` mirrors the manifest field saved by
        ``_deployment.OpDescriptorSnapshot``.
        """
        with self._lock:
            collected = []
            for descriptor in self._impl_descriptors:
                cls = getattr(descriptor, "cls", None)
                collected.append(
                    {
                        "descriptor_key": descriptor.descriptor_key,
                        "op_type": descriptor.op_type,
                        "module_name": descriptor.module_name,
                        "class_name": descriptor.class_name,
                        "class_qualname": getattr(
                            cls, "__qualname__", descriptor.class_name
                        ),
                        "interfaces": list(descriptor.interfaces),
                    }
                )
            return collected

    def collect_proto_declarations(self) -> List[dict]:
        with self._lock:
            return [item.to_bridge_dict() for item in self._proto_declarations]

    # -- in-memory source initialization --

    def initialize_source(
        self,
        artifact_id: str,
        source_name: str,
        source: bytes,
        ir_defs: Optional[Mapping[str, dict]] = None,
    ) -> ModuleType:
        """Compile and execute one embedded source strictly in memory.

        The synthetic filename only serves tracebacks; it never points at the
        filesystem.  The module joins ``sys.modules`` before execution so that
        self-referencing imports inside the source work, and is removed again
        if execution fails, together with every descriptor the failed run
        captured into this scope.

        ``ir_defs`` maps op_type to the canonical IR snapshot frozen at save
        time (inputs/attrs/outputs); it is recorded before execution so it
        survives even if a later artifact fails, and is rolled back with the
        captures when this source's execution fails.
        """
        # 跨语言边界校验：source 必须是 bytes（C++ 侧经 pybind 传入 py::bytes）。
        # artifact_id 非空与 source 非空不变量由保存侧 ValidateArtifact + C ABI 入口
        # BridgeRestorePythonCustomOpModelScope 的 sources 配对校验保证，此处不再复校。
        if not isinstance(source, bytes):
            raise ModelScopeError(
                f"embedded source for artifact {artifact_id!r} in model "
                f"scope {self.scope_id} is not bytes"
            )
        if ir_defs is None:
            ir_defs = {}
        elif not isinstance(ir_defs, Mapping):
            raise ModelScopeError(
                f"invalid ir defs for artifact {artifact_id!r} in model "
                f"scope {self.scope_id}"
            )
        module_name = f"{_MODULE_NAME_PREFIX}{self.scope_id}_{artifact_id}"
        with self._lock:
            self._assert_open()
            if module_name in self._module_names:
                raise ModelScopeError(
                    f"artifact {artifact_id} is already initialized in model "
                    f"scope {self.scope_id}"
                )
        # IR 注入先于执行：注入与 descriptor 捕获都是可幂等重建的状态，source 执行
        # 失败时整个 scope 会随加载失败被 C++ 侧关闭（Close），无需 Python 侧逐项
        # 回滚——失败路径的部分状态不会被任何查询观测到。
        ir_items = list(ir_defs.items())
        with self._lock:
            for op_type, ir_meta in ir_items:
                if (
                    op_type in self._ir_by_op_type
                    and self._ir_by_op_type[op_type] != ir_meta
                ):
                    raise ModelScopeError(
                        f"op type {op_type!r} carries inconsistent canonical "
                        f"IR across artifacts in model scope {self.scope_id}"
                    )
            for op_type, ir_meta in ir_items:
                self._ir_by_op_type[op_type] = ir_meta
                self._ir_artifact_by_op_type[op_type] = artifact_id
        synthetic_filename = f"<ge-om:{self.scope_id}/{source_name}>"
        try:
            code = compile(source, synthetic_filename, "exec")
        except SyntaxError as exc:
            raise ModelScopeError(
                f"embedded source of artifact {artifact_id} failed to compile: {exc}"
            ) from exc

        module = ModuleType(module_name)
        module.__file__ = synthetic_filename
        module.__ge_om_scope_id__ = self.scope_id
        module.__ge_om_artifact_id__ = artifact_id
        if module_name in sys.modules:
            raise ModelScopeError(
                f"module name {module_name} is unexpectedly occupied in sys.modules"
            )
        sys.modules[module_name] = module
        token = _ACTIVE_SCOPE_ID.set(self.scope_id)
        artifact_token = _ACTIVE_ARTIFACT_ID.set(artifact_id)
        try:
            exec(code, module.__dict__)
        except BaseException:
            del sys.modules[module_name]
            raise
        finally:
            _ACTIVE_ARTIFACT_ID.reset(artifact_token)
            _ACTIVE_SCOPE_ID.reset(token)
        with self._lock:
            self._module_names[module_name] = module
        return module

    # -- lifecycle --

    def close(self) -> None:
        """Release the scope: drop modules from ``sys.modules`` and all state.

        Idempotent.  The scope id stays retired forever (never recycled).
        """
        with self._lock:
            if self._closed:
                return
            self._closed = True
            for module_name in self._module_names:
                if sys.modules.get(module_name) is self._module_names[module_name]:
                    del sys.modules[module_name]
            self._module_names.clear()
            self._impl_descriptors.clear()
            self._impl_by_descriptor_key.clear()
            self._proto_declarations.clear()
            self._ir_by_op_type.clear()
            self._proto_artifact_by_op_type.clear()
            self._ir_artifact_by_op_type.clear()

    def _assert_open(self) -> None:
        if self._closed:
            raise ModelScopeError(f"model scope {self.scope_id} is already closed")


def open_model_scope(scope_id: int) -> _ModelScope:
    """Open a restore scope with a C++-assigned id (monotonic, never recycled)."""
    global _HIGHEST_SCOPE_ID
    if not isinstance(scope_id, int) or isinstance(scope_id, bool) or scope_id <= 0:
        raise ModelScopeError(
            f"model scope id must be a positive int, got {scope_id!r}"
        )
    with _SCOPE_LOCK:
        # Live scopes are necessarily in the retirement set, so a single
        # membership test covers both "in use" and "retired".
        if scope_id in _USED_SCOPE_IDS:
            raise ModelScopeError(
                f"model scope id {scope_id} is already in use or retired"
            )
        if scope_id <= _HIGHEST_SCOPE_ID:
            raise ModelScopeError(
                f"model scope id {scope_id} is not greater than the highest "
                f"opened id {_HIGHEST_SCOPE_ID}; ids must never be recycled"
            )
        scope = _ModelScope(scope_id)
        _SCOPES[scope_id] = scope
        _USED_SCOPE_IDS.add(scope_id)
        _HIGHEST_SCOPE_ID = scope_id
    return scope


def close_model_scope(scope_id: int) -> bool:
    """Close and retire a scope.  Returns False when the scope is unknown."""
    with _SCOPE_LOCK:
        scope = _SCOPES.pop(scope_id, None)
    if scope is None:
        return False
    scope.close()
    return True


def get_model_scope(scope_id: int) -> Optional[_ModelScope]:
    with _SCOPE_LOCK:
        return _SCOPES.get(scope_id)


def get_active_model_scope() -> Optional[_ModelScope]:
    """Return the scope active on the current context, or None."""
    scope_id = _ACTIVE_SCOPE_ID.get()
    if scope_id is None:
        return None
    with _SCOPE_LOCK:
        return _SCOPES.get(scope_id)


def find_scoped_impl_descriptor(descriptor_key: str):
    """Look a descriptor up across all live scopes (runtime holder creation).

    Creators restored from an OM capture their scoped descriptor key; when the
    bridge later creates an impl holder, the global registry knows nothing
    about it, so live model scopes are consulted as the fallback.
    """
    with _SCOPE_LOCK:
        scopes = list(_SCOPES.values())
    for scope in scopes:
        descriptor = scope.get_impl_by_descriptor_key(descriptor_key)
        if descriptor is not None:
            return descriptor
    return None


def _resolve_unique_scoped_hit(op_type: str, hits: list, what: str):
    """Collapse cross-scope hits of ``op_type`` into one value or fail loudly.

    ``op_type`` alone carries no model affinity: every live scope is searched
    and, before this guard, the first hit silently won.  Two models in one
    process holding the same ``op_type`` with different definitions would then
    make the later model borrow the earlier model's definition -- wrong shape
    inference or mis-bound arguments, with no log at all.  C++ custom ops make
    the same situation a hard failure ("models in one process must use the
    same implementation for the same custom op type"), so mirror that instead
    of returning an arbitrary winner.

    ``hits`` is a list of ``(scope_id, artifact_id, value)``.  Identity is the
    ``artifact_id`` -- the sha256 of the definition file's bytes, i.e. the same
    fingerprint granularity the C++ side uses for SO files.  Same source file
    (whatever the model) passes; different source files fail.  Comparing the
    declarations' contents instead would drag in CPython bytecode-equality
    semantics (nested code objects in ``co_consts`` are compared structurally
    only from CPython 3.8 on) for no gain.
    """
    if not hits:
        return None
    first_scope_id, first_artifact_id, first = hits[0]
    for scope_id, artifact_id, _ in hits[1:]:
        if artifact_id != first_artifact_id:
            raise ModelScopeError(
                f"conflicting {what} for op type {op_type!r} across live model "
                f"scopes: model scope {first_scope_id} (artifact "
                f"{first_artifact_id}) and model scope {scope_id} (artifact "
                f"{artifact_id}) carry different definitions; models in one "
                f"process must use the same implementation for the same custom "
                f"op type. Align custom op package versions and retry."
            )
    return first


def find_scoped_ir_meta(op_type: str) -> Optional[dict]:
    """Look the canonical IR snapshot of ``op_type`` up across all live scopes.

    The bridge falls back to this at load time (design rule D4: prototypes are
    not re-registered into ``OperatorFactory``) to bind schema-bound
    execute/compile arguments.  Returns None when no live scope froze IR for
    the op type; raises when two live scopes froze it from different sources.
    """
    with _SCOPE_LOCK:
        scopes = list(_SCOPES.values())
    hits = []
    for scope in scopes:
        ir_meta = scope.get_ir_meta_by_op_type(op_type)
        if ir_meta is not None:
            hits.append((scope.scope_id, scope.get_ir_artifact_id(op_type), ir_meta))
    return _resolve_unique_scoped_hit(op_type, hits, "canonical IR")


def find_scoped_proto(op_type: str):
    """Look the prototype declaration of ``op_type`` up across all live scopes.

    ``_infer_meta.call_infer_meta`` falls back to this when the process-global
    proto registry has no entry.  In a deployment process that never ran the
    compile-time plugin registration that registry is always empty by design
    (D4), so without this fallback every dynamic-shape inference of a restored
    Python operator fails with ``AttributeError``.  Raises when two live scopes
    declare the same op type from different sources.
    """
    with _SCOPE_LOCK:
        scopes = list(_SCOPES.values())
    hits = []
    for scope in scopes:
        proto = scope.get_proto_by_op_type(op_type)
        if proto is not None:
            hits.append((scope.scope_id, scope.get_proto_artifact_id(op_type), proto))
    return _resolve_unique_scoped_hit(op_type, hits, "prototype declaration")


def reset_model_scopes_for_test() -> None:
    """Drop every live scope and the retirement bookkeeping (tests only)."""
    global _HIGHEST_SCOPE_ID
    with _SCOPE_LOCK:
        for scope in list(_SCOPES.values()):
            scope.close()
        _SCOPES.clear()
        _USED_SCOPE_IDS.clear()
        _HIGHEST_SCOPE_ID = 0
