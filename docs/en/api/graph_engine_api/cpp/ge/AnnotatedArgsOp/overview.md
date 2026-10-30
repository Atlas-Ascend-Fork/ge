# Overview

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-16T12:03:22.145Z pushedAt=2026-10-28T07:40:22.222Z nl=2 -->

`ge::AnnotatedArgsOp` is the declarative kernel launch parameter API of a custom operator, virtually inheriting from `ge::BaseCustomOp`. After the operator implements `DeclareLaunchArgs`, GE calls back this API during the static graph compilation phase, where the operator obtains tensors, allocates workspace, and submits the kernel launch through [`gert::AnnotatedArgsContext`](../../gert/AnnotatedArgsContext/overview.md).

<!-- npu="x90,9030" id1 -->
On-device scenario: only one kernel launch is allowed to be submitted.
<!-- end id1 -->

This API is used for static graph task generation. The dynamic graph does not use the declarative parameter generation path; the generated `args_format` is used for address refresh at runtime, and `DeclareLaunchArgs` is not called back again at runtime.

If the same custom operator inherits both [`ArgsUpdater`](../ArgsUpdater/overview.md) and `AnnotatedArgsOp`, GE selects the callback refresh strategy of `ArgsUpdater`, and `DeclareLaunchArgs` does not participate in address refresh.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) = 0
```

## Related APIs

- [`DeclareLaunchArgs`](DeclareLaunchArgs.md)
- [`gert::AnnotatedArgsContext`](../../gert/AnnotatedArgsContext/overview.md)
- [`gert::AnnotatedKernelArgs`](../../gert/AnnotatedKernelArgs/overview.md)
