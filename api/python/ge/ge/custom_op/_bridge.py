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

"""Bridge-facing Python runtime helpers for GE custom op implementations."""

import inspect
import sys
import threading
from dataclasses import dataclass
from typing import Dict, Optional, Union

from ._ir_types import InputType, OutputType
from ._infer_meta import call_infer_meta  # noqa: F401
from ._signature import _get_runtime_attr_spec, _validate_args_signature
from ._native import EagerOpExecutionContext, HostCpuOpExecutionContext
from .bootstrap import (
    freeze_deployment_artifacts,
    get_registered_op_impls,
    get_registered_op_protos,
    load_custom_op_plugins,
)
from .context import (
    _compile_ctx_scope,
    _declare_launch_args_ctx_scope,
    _execute_ctx_scope,
)
from .registry import (
    INTERFACE_ANNOTATED_ARGS,
    INTERFACE_COMPILABLE,
    INTERFACE_EAGER_EXECUTE,
    INTERFACE_HOST_CPU_EXECUTE,
    KernelBinding,
    OpBackend,
    get_registered_op_impl_by_descriptor_key,
)


@dataclass
class _OpImplHolder:
    descriptor_key: str
    instance_id: str
    instance: object


_HOLDER_LOCK = threading.RLock()
_OP_IMPL_HOLDERS: Dict[str, _OpImplHolder] = {}
UINT32_MAX = 4294967295


def _validate_portable_payload_length(length) -> None:
    """Validate Portable payload length fits the native buffer boundary.

    Accepts only exact ``int`` values in the closed range ``[1, UINT32_MAX]``.
    The length-only form lets boundary values (``UINT32_MAX`` and
    ``UINT32_MAX + 1``) be exercised without allocating a 4 GiB buffer.
    """
    if type(length) is not int:
        raise TypeError(
            f"portable payload length must be int, got {type(length).__name__}"
        )
    if length <= 0 or length > UINT32_MAX:
        raise ValueError(
            f"portable payload length {length} is out of range [1, {UINT32_MAX}]"
        )


def load_and_get_op_impl_descriptors() -> list:
    load_custom_op_plugins()
    return get_registered_op_impls()


def load_and_get_op_descriptors() -> dict:
    load_custom_op_plugins()
    return {
        "protos": get_registered_op_protos(),
        "impls": get_registered_op_impls(),
    }


def load_and_get_deployment_artifacts() -> list:
    """Load plugins, then return the frozen Python deployment artifacts.

    Freezing is idempotent per process: the first call reads each definition
    file once and caches the immutable result, later calls return the cache even
    if the file changed on disk.
    """

    load_custom_op_plugins()
    return freeze_deployment_artifacts()


def _get_holder(instance_id: str) -> _OpImplHolder:
    with _HOLDER_LOCK:
        holder = _OP_IMPL_HOLDERS.get(instance_id)
    if holder is None:
        raise KeyError(f"python op impl holder is not created: {instance_id}")
    return holder


def _get_execute_kernel_method(holder: _OpImplHolder, backend: OpBackend):
    descriptor = get_registered_op_impl_by_descriptor_key(holder.descriptor_key)
    if descriptor is None:
        raise KeyError(
            f"python op impl descriptor_key not found: {holder.descriptor_key}"
        )
    binding = descriptor.kernel_bindings.get(backend)
    if binding is not None:
        return binding.function.__get__(holder.instance, type(holder.instance))
    # Preserve legacy execute behavior for callable class attributes that are
    # not functions or staticmethod/classmethod wrappers (e.g. partial objects
    # or __call__ instances), which are intentionally not represented in the
    # kernel map.
    if backend is OpBackend.DEVICE and not descriptor.kernel_bindings:
        method = getattr(holder.instance, "execute", None)
        if callable(method):
            return method
    raise KeyError(
        f"python op impl execute backend is not registered: {backend.value}, "
        f"descriptor key: {holder.descriptor_key}"
    )


def create_op_impl_holder(instance_id: str, descriptor_key: str) -> bool:
    descriptor = get_registered_op_impl_by_descriptor_key(descriptor_key)
    if descriptor is None:
        raise KeyError(f"python op impl descriptor_key not found: {descriptor_key}")
    with _HOLDER_LOCK:
        if instance_id in _OP_IMPL_HOLDERS:
            return True
        _OP_IMPL_HOLDERS[instance_id] = _OpImplHolder(
            descriptor_key=descriptor_key,
            instance_id=instance_id,
            instance=descriptor.cls(),
        )
    return True


def destroy_op_impl_holder(instance_id: str) -> bool:
    with _HOLDER_LOCK:
        return _OP_IMPL_HOLDERS.pop(instance_id, None) is not None


def _get_callback_for_signature(cls, method_name: str):
    method = inspect.getattr_static(cls, method_name)
    if isinstance(method, staticmethod):
        return method.__func__
    if isinstance(method, classmethod):
        return method.__get__(None, cls)
    if inspect.isfunction(method):
        return method.__get__(object(), cls)
    return getattr(cls, method_name)


def _validate_kernel_binding(
    descriptor,
    backend: OpBackend,
    binding: KernelBinding,
    ir_meta: Optional[dict],
) -> None:
    method = binding.function.__get__(object(), descriptor.cls)
    try:
        _validate_args_signature(method, ir_meta, descriptor, method_name="execute")
    except TypeError as exc:
        raise TypeError(
            f"{exc}; backend {backend.value!r}, source {binding.source_location}"
        ) from exc


def validate_op_impl_descriptor(
    descriptor_key: str,
    ir_meta: Optional[dict],
) -> bool:
    descriptor = get_registered_op_impl_by_descriptor_key(descriptor_key)
    if descriptor is None:
        raise KeyError(f"python op impl descriptor_key not found: {descriptor_key}")

    if ir_meta is None:
        return True

    if (
        descriptor.kernel_bindings
        or INTERFACE_EAGER_EXECUTE in descriptor.interfaces
        or INTERFACE_HOST_CPU_EXECUTE in descriptor.interfaces
    ):
        if descriptor.kernel_bindings:
            for backend, binding in descriptor.kernel_bindings.items():
                _validate_kernel_binding(descriptor, backend, binding, ir_meta)
        else:
            method = _get_callback_for_signature(descriptor.cls, "execute")
            _validate_args_signature(method, ir_meta, descriptor, method_name="execute")

    if INTERFACE_COMPILABLE in descriptor.interfaces:
        method = _get_callback_for_signature(descriptor.cls, "compile")
        _validate_args_signature(method, ir_meta, descriptor, method_name="compile")

    if INTERFACE_ANNOTATED_ARGS in descriptor.interfaces:
        method = _get_callback_for_signature(descriptor.cls, "declare_launch_args")
        _validate_args_signature(
            method, ir_meta, descriptor, method_name="declare_launch_args"
        )
    return True


def _build_inputs(
    ir_inputs: list,
    get_required_input,
    get_optional_input,
    get_dynamic_input_num,
    get_dynamic_input,
) -> list:
    args = []
    for ir_index, item in enumerate(ir_inputs):
        kind = item["kind"]
        if kind == InputType.REQUIRED:
            args.append(get_required_input(ir_index))
        elif kind == InputType.OPTIONAL:
            args.append(get_optional_input(ir_index))
        elif kind == InputType.DYNAMIC:
            instance_num = get_dynamic_input_num(ir_index)
            args.append(
                [
                    get_dynamic_input(ir_index, relative_index)
                    for relative_index in range(instance_num)
                ]
            )
        else:
            raise ValueError(
                f"unsupported custom op IR input kind: {kind}, ir index: {ir_index}"
            )
    return args


def _build_execute_inputs(
    ctx: Union[EagerOpExecutionContext, HostCpuOpExecutionContext], ir_inputs: list
) -> list:
    return _build_inputs(
        ir_inputs,
        ctx._get_required_input_tensor,
        ctx._get_optional_input_tensor,
        ctx._get_dynamic_input_num,
        ctx._get_dynamic_input_tensor,
    )


def _read_runtime_attr(attrs, index: int, ir_type: str):
    getter_name, _ = _get_runtime_attr_spec(ir_type, index)
    return getattr(attrs, getter_name)(index)


def _build_execute_attrs(
    ctx: Union[EagerOpExecutionContext, HostCpuOpExecutionContext], ir_attrs: list
) -> dict:
    if not ir_attrs:
        return {}
    attrs = ctx._get_attrs()
    return {
        item["name"]: _read_runtime_attr(attrs, index, item["type"])
        for index, item in enumerate(ir_attrs)
    }


def _build_schema_inputs(ctx, ir_inputs: list) -> list:
    return _build_inputs(
        ir_inputs,
        ctx._get_required_input_tensor,
        ctx._get_optional_input_tensor,
        ctx._get_dynamic_input_num,
        ctx._get_dynamic_input_tensor,
    )


def _build_schema_outputs(ctx, ir_outputs: list) -> list:
    args = []
    for ir_index, item in enumerate(ir_outputs):
        kind = item["kind"]
        if kind == OutputType.REQUIRED:
            args.append(ctx._get_required_output_tensor(ir_index))
        elif kind == OutputType.DYNAMIC:
            instance_num = ctx._get_dynamic_output_num(ir_index)
            args.append(
                [
                    ctx._get_dynamic_output_tensor(ir_index, relative_index)
                    for relative_index in range(instance_num)
                ]
            )
        else:
            raise ValueError(f"unsupported custom op IR output kind: {kind}")
    return args


def _build_schema_attrs(ctx, ir_attrs: list) -> dict:
    if not ir_attrs:
        return {}
    attrs = ctx._get_attrs()
    return {
        item["name"]: _read_runtime_attr(attrs, index, item["type"])
        for index, item in enumerate(ir_attrs)
    }


def call_execute(
    instance_id: str,
    backend: str,
    ir_meta: Optional[dict],
    ctx: Union[EagerOpExecutionContext, HostCpuOpExecutionContext],
) -> None:
    try:
        holder = _get_holder(instance_id)
        method = _get_execute_kernel_method(holder, OpBackend(backend))
        if ir_meta is None:
            descriptor = holder.instance.__ge_op_impl_descriptor__
            raise RuntimeError(
                f"canonical IR not found for schema-bound execute: {descriptor.op_type}"
            )
        args = _build_execute_inputs(ctx, ir_meta["inputs"])
        kwargs = _build_execute_attrs(ctx, ir_meta["attrs"])
        with _execute_ctx_scope(ctx):
            result = method(*args, **kwargs)
        if result is not None:
            raise TypeError("execute must return None")
    finally:
        ctx._invalidate()


def call_declare_launch_args(instance_id: str, ir_meta: Optional[dict], ctx) -> None:
    try:
        holder = _get_holder(instance_id)
        method = getattr(holder.instance, "declare_launch_args", None)
        if not callable(method):
            raise TypeError(
                f"python op impl does not implement declare_launch_args: {instance_id}"
            )
        if ir_meta is None:
            raise RuntimeError(
                "canonical IR not found for schema-bound declare_launch_args"
            )
        args = _build_schema_inputs(ctx, ir_meta["inputs"])
        args.extend(_build_schema_outputs(ctx, ir_meta["outputs"]))
        kwargs = _build_schema_attrs(ctx, ir_meta["attrs"])
        with _declare_launch_args_ctx_scope(ctx):
            result = method(*args, **kwargs)
        if result is not None:
            raise TypeError("declare_launch_args must return None")
    finally:
        ctx._invalidate()


def call_compile(instance_id: str, ir_meta: Optional[dict], ctx) -> None:
    """Invoke a schema-bound Python compile callback."""

    try:
        holder = _get_holder(instance_id)
        method = getattr(holder.instance, "compile", None)
        if not callable(method):
            raise TypeError(f"python op impl does not implement compile: {instance_id}")
        if ir_meta is None:
            raise RuntimeError("canonical IR not found for schema-bound compile")
        descriptor = holder.instance.__ge_op_impl_descriptor__
        _validate_args_signature(method, ir_meta, descriptor, method_name="compile")
        args = _build_schema_inputs(ctx, ir_meta["inputs"])
        args.extend(_build_schema_outputs(ctx, ir_meta["outputs"]))
        kwargs = _build_schema_attrs(ctx, ir_meta["attrs"])
        with _compile_ctx_scope(ctx):
            result = method(*args, **kwargs)
        if result is not None:
            raise TypeError("compile must return None")
    finally:
        ctx._invalidate()


def call_serialize(instance_id: str) -> bytes:
    """Invoke the registered Portable ``serialize`` method on the holder instance.

    Returns the exact bytes payload owned by the user instance. Validates the
    payload is exact non-empty ``bytes`` of length ``[1, UINT32_MAX]``; user
    exceptions propagate unchanged.
    """
    holder = _get_holder(instance_id)
    method = getattr(holder.instance, "serialize", None)
    if not callable(method):
        raise TypeError(f"python op impl does not implement serialize: {instance_id}")
    payload = method()
    if type(payload) is not bytes:
        raise TypeError(f"serialize must return bytes, got {type(payload).__name__}")
    _validate_portable_payload_length(len(payload))
    return payload


def call_deserialize(instance_id: str, data: bytes) -> None:
    """Invoke the registered Portable ``deserialize`` method on the holder instance.

    Validates ``data`` is exact non-empty ``bytes`` of length
    ``[1, UINT32_MAX]`` before user code runs, requires the method to return
    ``None``, and propagates user exceptions unchanged.
    """
    if type(data) is not bytes:
        raise TypeError(f"deserialize data must be bytes, got {type(data).__name__}")
    _validate_portable_payload_length(len(data))
    holder = _get_holder(instance_id)
    method = getattr(holder.instance, "deserialize", None)
    if not callable(method):
        raise TypeError(f"python op impl does not implement deserialize: {instance_id}")
    result = method(data)
    if result is not None:
        raise TypeError(f"deserialize must return None, got {type(result).__name__}")


def clear_op_impl_holders() -> None:
    with _HOLDER_LOCK:
        _OP_IMPL_HOLDERS.clear()


def clear_loaded_op_impl_modules() -> None:
    """Clear all dynamically loaded op implementation modules from sys.modules to avoid test pollution.

    Model-scope modules (``_ge_om_custom_op_*``) are NOT cleaned here; they are
    owned by their scope and released by ``close_model_scope``.
    """
    from ._deployment import clear_frozen_artifacts

    clear_frozen_artifacts()
    keys_to_remove = [key for key in sys.modules if key.startswith("_ge_py_custom_op_")]
    for key in keys_to_remove:
        del sys.modules[key]


# ---------------------------------------------------------------------------
# Model-scope restoration API (called from the C++ pybind bridge while
# restoring the PYTHON_CUSTOM_OPS partition of an OM model).
#
# These thin wrappers only translate C++-friendly signatures into the
# ``_scope`` module API; every failure is reported as a Python exception so
# the pybind layer converts it into a logged FAILED status.  Scope ids are
# authoritative from C++ (64-bit, monotonic, never recycled); ``_scope``
# defensively re-validates that contract.
# ---------------------------------------------------------------------------


def restore_model_scope(scope_id: int, sources: list) -> list:
    """Open a scope, execute every embedded source in memory, then collect descriptors.

    One C++ restore session (open + per-artifact initialization + descriptor
    collection, design rule: the session boundary lives here).  Each entry of
    ``sources`` carries ``artifact_id``/``source_name``/``source`` (exact
    ``bytes``) and the frozen canonical IR snapshot under ``ir_defs``
    (op_type -> inputs/attrs/outputs, recorded before the source executes so
    schema-bound callbacks can resolve their parameter schema at load time —
    prototypes are not re-registered, design rule D4).

    Any failure closes the freshly opened scope and re-raises, so the C++
    caller never observes a half-open session.
    """
    from ._scope import close_model_scope, open_model_scope

    scope = open_model_scope(int(scope_id))
    try:
        for item in sources:
            source = item.get("source")
            if type(source) is not bytes:
                raise TypeError(
                    f"embedded source must be bytes, got {type(source).__name__}"
                )
            ir_defs = item.get("ir_defs")
            if ir_defs is None:
                ir_defs = {}
            if type(ir_defs) is not dict:
                raise TypeError(f"ir defs must be dict, got {type(ir_defs).__name__}")
            try:
                scope.initialize_source(
                    str(item.get("artifact_id", "")),
                    str(item.get("source_name", "")),
                    source,
                    ir_defs,
                )
            except ImportError as exc:
                # 首期打包边界：embedded source 只含实现类定义所在的一个 .py，
                # 不递归收集 import 依赖（同目录 helper、包内相对导入等不受支持）。
                # ImportError 在此重述边界，避免部署端把"依赖未打包"误判为 OM 损坏。
                raise ImportError(
                    f"embedded source {str(item.get('source_name', ''))!r} imports a module that is "
                    f"not available in the deployment environment: {exc}. Only the definition file "
                    f"itself is packaged into the OM; importing sibling/relative modules from the "
                    f"original plugin directory is not supported."
                ) from exc
        return scope.collect_impl_descriptors()
    except BaseException:
        close_model_scope(int(scope_id))
        raise


def close_model_scope(scope_id: int) -> bool:
    """Close and retire a scope.  Returns False when the scope is unknown."""
    from ._scope import close_model_scope as _close

    return _close(int(scope_id))


def get_model_scope_ir_meta(op_type: str) -> Optional[dict]:
    """Return the canonical IR of ``op_type`` frozen in any live model scope.

    Called by the bridge as the load-time fallback when the C++
    ``OperatorFactory`` has no entry (design rule D4): the returned dict has
    the same shape as the ir_meta built from a registered prototype
    (``inputs``/``attrs``/``outputs``), or None when no scope froze it.
    """
    from ._scope import find_scoped_ir_meta

    if not isinstance(op_type, str) or not op_type:
        raise TypeError("op_type must be a non-empty string")
    return find_scoped_ir_meta(op_type)


def collect_model_scope_proto_op_types(scope_id: int) -> list:
    """Return op types of prototype declarations recorded (and suppressed) in the scope.

    Design rule D4: declarations are consumed from the compile-time
    environment, never re-registered at load time; this list exists for
    diagnostics only.
    """
    from ._scope import ModelScopeError, get_model_scope

    scope = get_model_scope(int(scope_id))
    if scope is None:
        raise ModelScopeError(f"model scope {scope_id} is not open")
    return [item["op_type"] for item in scope.collect_proto_declarations()]
