# Overview

<!-- md-trans-meta sourceCommit=6c0f2edd2793cdeb0ad99aa92d61e13b5213c8ef translatedAt=2026-10-28T07:59:01.878Z pushedAt=2026-10-28T08:50:45.432Z nl=2 -->

`gert::AnnotatedArgsContext` inherits from `gert::ExtendedKernelContext` and is the declarative argument context of [`ge::AnnotatedArgsOp::DeclareLaunchArgs`](../../ge/AnnotatedArgsOp/DeclareLaunchArgs.md).

The Context and the Tensor pointers it returns are valid only during the current `DeclareLaunchArgs` callback. You must not cache the Context, the Tensor pointers, or the logical addresses contained in them beyond the `DeclareLaunchArgs` callback.

The input and output addresses obtained from the Context APIs, the stream ID, and the workspace memory addresses allocated through `MallocWorkSpace` are all logical resources rather than actual physical resources.

## Header File to Include

```c++
#include <exe_graph/runtime/annotated_args_context.h>
```

Library file: liblowering.so

## Public Member Functions

| Function | Description |
| --- | --- |
| [`WorkspaceAddr MallocWorkSpace(size_t size)`](MallocWorkSpace.md) | Allocates logical workspace. |
| [`uint32_t GetStreamId() const`](GetStreamId.md) | Obtains the logical primary stream ID of the node. |
| [`ge::graphStatus AddLaunch(const AnnotatedKernelLaunchInfo &launch_info, AnnotatedKernelArgs &&args)`](AddLaunch.md) | Adds a kernel launch. |
| [`const Tensor *GetInputTensor(size_t index) const`](GetInputTensor.md) | Obtains the input Tensor by flattened instance index. |
| [`const Tensor *GetOutputTensor(size_t index) const`](GetOutputTensor.md) | Obtains the output Tensor by flattened instance index. |
| [`const Tensor *GetRequiredInputTensor(size_t ir_index) const`](GetRequiredInputTensor.md) | Obtains the required input Tensor by IR prototype index. |
| [`const Tensor *GetOptionalInputTensor(size_t ir_index) const`](GetOptionalInputTensor.md) | Obtains the optional input Tensor by IR prototype index. |
| [`const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const`](GetDynamicInputTensor.md) | Obtains the dynamic input Tensor by IR prototype index and relative index. |
| [`const Tensor *GetRequiredOutputTensor(size_t ir_index) const`](GetRequiredOutputTensor.md) | Obtains the required output Tensor by IR prototype index. |
| [`const Tensor *GetDynamicOutputTensor(size_t ir_index, size_t relative_index) const`](GetDynamicOutputTensor.md) | Obtains the dynamic output Tensor by IR prototype index and relative index. |

## Related Types

- [`gert::AnnotatedKernelArgs`](../AnnotatedKernelArgs/overview.md)
- [`gert::InputAddr`](../InputAddr.md)
- [`gert::OutputAddr`](../OutputAddr.md)
- [`gert::WorkspaceAddr`](../WorkspaceAddr.md)
- [`gert::AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md)
