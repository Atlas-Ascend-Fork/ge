# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

from __future__ import annotations

from typing import List, Optional, Sequence

from ge.graph.types import DataType
from ge.runtime import StorageFormat, StorageShape, Tensor

__all__: List[str] = [
    "AnnotatedArgsContext",
    "AnnotatedKernelArgs",
    "AnnotatedKernelLaunchInfo",
    "EagerOpExecutionContext",
    "HostCpuOpExecutionContext",
    "CompilePlatformInfo",
    "OpCompileContext",
    "WorkspaceAddr",
]


class EagerOpExecutionContext:
    """Eager op execution context (Python view of ``gert::EagerOpExecutionContext``).

    Borrowed execution view available through ``ge.custom_op.get_execute_ctx()``
    while a schema-bound ``execute`` callback is running. It supports querying
    output tensors, allocating output/workspace memory, and retrieving the
    execution stream.

    **Constraints**

    - Use only inside the current ``execute`` callback. The bridge calls
      ``_invalidate()`` in ``finally`` after the callback returns or raises.
    - All ``Tensor`` / ``Shape`` / ``StorageShape`` / ``StorageFormat`` objects
      returned from this context share the same validity marker and expire with
      the context.

    **Example**

        def execute(self, x) -> None:
            from ge.custom_op import get_execute_ctx
            ctx = get_execute_ctx()
            y = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
    """

    def malloc_output_tensor(
        self,
        index: int,
        shape: StorageShape,
        format: StorageFormat,
        dtype: DataType,
    ) -> Tensor:
        """Allocate device memory for one output tensor and initialize its metadata.

        Raises ``RuntimeError`` if allocation or initialization fails. The output tensor
        memory is managed by the context provider and must not be freed by Python code.
        """
        ...

    def make_output_ref_input(self, output_index: int, input_index: int) -> Tensor:
        """Make an output tensor reuse the memory address of an input tensor.

        Raises ``RuntimeError`` if output or input tensor lookup fails.
        """
        ...

    def malloc_workspace(self, size: int) -> int:
        """Allocate device workspace memory and return its address as an integer.

        Raises ``RuntimeError`` if allocation fails.
        """
        ...

    def get_output_tensor(self, index: int) -> Tensor:
        """Return output tensor by output index.

        Raises ``RuntimeError`` if the output tensor is unavailable.
        """
        ...

    def get_stream(self) -> int:
        """Return the execution stream address as an integer."""
        ...

    def request_attached_stream(self, key: str, flags: int = 0) -> Optional[int]:
        """Request a framework-managed attached stream by key and return its address.

        The same key returns the same handle within one execution instance. The handle must
        not be cached beyond the current callback and must not be destroyed by Python code:
        the framework owns the stream. ``flags`` is the raw RTS stream-creation flag value,
        applied only when a new stream is created; pass ``0`` for the framework default.

        Returns ``None`` when attached streams are unsupported or the stream cannot be
        created, in which case callers may degrade to ``get_stream()``. Raises ``ValueError``
        if ``key`` is empty and ``RuntimeError`` if this borrowed context has expired.
        """
        ...

    def _invalidate(self) -> None:
        """Invalidate this context and all derived borrowed views.

        GE bridge only; user custom op code should not call this method.
        """
        ...


class HostCpuOpExecutionContext:
    """Host CPU op execution context (Python view of ``gert::HostCpuOpExecutionContext``).

    Borrowed execution view available through ``ge.custom_op.get_execute_ctx()``
    while a schema-bound host-backend ``execute`` callback is running. It
    supports querying output tensors, allocating host output memory, and making
    an output reuse an input's address. Inputs and attributes are delivered as
    ``execute`` parameters by the bridge, so input/attr query APIs are
    bridge-internal and not part of the public surface. It does not provide
    device stream or workspace access.

    **Constraints**

    - Use only inside the current ``execute`` callback. The bridge calls
      ``_invalidate()`` in ``finally`` after the callback returns or raises.
    - All ``Tensor`` objects returned from this context share the same validity
      marker and expire with the context.

    **Example**

        def execute(self, x) -> None:
            from ge.custom_op import get_execute_ctx
            ctx = get_execute_ctx()
            y = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
    """

    def malloc_output_tensor(
        self,
        index: int,
        shape: StorageShape,
        format: StorageFormat,
        dtype: DataType,
    ) -> Tensor:
        """Allocate host memory for one output tensor and initialize its metadata.

        Raises ``RuntimeError`` if allocation or initialization fails. The output tensor
        memory is managed by the context provider and must not be freed by Python code.
        """
        ...

    def make_output_ref_input(self, output_index: int, input_index: int) -> Tensor:
        """Make an output tensor reuse the memory address of an input tensor.

        Raises ``RuntimeError`` if output or input tensor lookup fails.
        """
        ...

    def get_output_tensor(self, index: int) -> Tensor:
        """Return output tensor by output index.

        Raises ``RuntimeError`` if the output tensor is unavailable.
        """
        ...

    def _invalidate(self) -> None:
        """Invalidate this context and all derived borrowed views.

        GE bridge only; user custom op code should not call this method.
        """
        ...


class OpCompileContext:
    """Borrowed read-only view of ``gert::OpCompileContext``."""

    def get_option(self, option_key: str) -> str: ...

    def _get_platform_info(self) -> CompilePlatformInfo: ...


class CompilePlatformInfo:
    """Borrowed platform information view available during ``compile``."""

    def get_platform_resource(self, group: str, key: str) -> str: ...

    def get_platform_resource_group(self, group: str) -> dict[str, str]: ...

    def get_core_num(self, core_type: Optional[str] = None) -> int: ...

    def get_soc_version(self) -> str: ...

    def get_ai_core_num(self) -> int: ...


class WorkspaceAddr:
    """Borrowed workspace address allocated by ``AnnotatedArgsContext``."""

    @property
    def index(self) -> int: ...

    @property
    def addr(self) -> int: ...


class AnnotatedKernelLaunchInfo:
    """Owned kernel launch metadata used by ``AnnotatedArgsContext.add_launch``."""

    def __init__(
        self,
        *,
        kernel_name: str,
        kernel_bin: bytes,
        block_dim: int,
        stream_id: int,
    ) -> None: ...


class AnnotatedKernelArgs:
    """Borrowed builder for one annotated kernel launch's argument sequence."""

    def append_input(self, instance_index: int, tensor: Tensor) -> None: ...

    def append_output(self, instance_index: int, tensor: Tensor) -> None: ...

    def append_workspace(self, workspace: WorkspaceAddr) -> None: ...

    def append_scalar(self, value: int) -> None: ...

    def append_custom_data_block(self, data: bytes) -> None:
        """Deep-copy nonempty bytes into a read-only device data block.

        Adds one pointer slot; the byte length is not passed to the kernel.
        Raises ValueError for empty data, TypeError for non-bytes data, and
        RuntimeError if this builder has expired, was consumed, or append fails.
        """
        ...


class AnnotatedArgsContext:
    """Borrowed declaration context available only in ``declare_launch_args``."""

    def malloc_workspace(self, size: int) -> WorkspaceAddr: ...

    def get_stream_id(self) -> int: ...

    def create_kernel_args(self) -> AnnotatedKernelArgs: ...

    def request_attached_stream(self, key: str) -> int:
        """Request a reusable compile-time logical attached stream by key.

        The key is scoped to the compiled subgraph owning the op. The returned value is a
        compile-time logical id, not a runtime stream handle: it may only be passed as
        ``AnnotatedKernelLaunchInfo(stream_id=...)`` of this context and must not be cached
        beyond the current ``declare_launch_args`` callback.

        Raises ``ValueError`` if ``key`` is empty, and ``RuntimeError`` if the request fails
        or this borrowed context has expired.
        """
        ...

    def add_launch(
        self,
        launch_info: AnnotatedKernelLaunchInfo,
        args: AnnotatedKernelArgs,
        predecessors: Optional[Sequence[int]] = None,
    ) -> int:
        """Submit a launch and return this launch's dependency token.

        Omitting ``predecessors``, passing ``None`` and passing ``[]`` are equivalent: every
        launch returns a valid token and can be depended upon by a later one. Tokens increase
        from 0 in submission order and are valid only within the current callback; each element
        must be a token previously returned in this same callback.

        ``launch_info.stream_id`` must be ``get_stream_id()`` or an id returned by
        ``request_attached_stream`` of this context; any other value raises ``ValueError``
        before ``args`` is consumed. Once the check passes, ``args`` is consumed whether or not
        the submit succeeds and must not be reused. Raises ``RuntimeError`` if the submit fails
        or if this context or ``args`` has expired.
        """
        ...
