# MallocWorkSpace

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:58:25.098Z pushedAt=2026-10-28T08:50:45.431Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

At compile time, the framework applies [memory reuse optimization](https://www.hiascend.com/developer/techArticles/202407005-1?envFlag=1). This API describes the required workspace size. The framework records this information in the model and returns the offset of the workspace within the entire logical memory block of the model. Unlike a real physical address, this offset is usually called a logical address.

## Prototype

```c++
WorkspaceAddr MallocWorkSpace(size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| size | Input | Size of the workspace to apply for, in bytes. The value must be greater than 0. |

## Return Value

Returns a `WorkspaceAddr` descriptor. On success, `addr` is non-null and `index` is the 0-based sequence number of this workspace in the current application sequence. On failure, `addr` is `nullptr`, in which case you must not use this descriptor to construct launch parameters.

`addr` is a logical address rather than a real address, so you do not need to manage its lifecycle.

## Constraints

None

## Examples

The following snippet is inside `DeclareLaunchArgs`. Terminate the declaration if the application fails. On success, directly pass the returned descriptor as the `AnnotatedKernelArgs` parameter and finally hand it over to `AddLaunch`.

```c++
const auto workspace0 = ctx.MallocWorkSpace(1024U);
const auto workspace1 = ctx.MallocWorkSpace(2048U);
if ((workspace0.addr == nullptr) || (workspace1.addr == nullptr)) {
  return ge::GRAPH_FAILED;
}

// workspace0.index == 0, workspace1.index == 1.
gert::AnnotatedKernelArgs args(workspace0, workspace1);
// Continue constructing other parameters and call ctx.AddLaunch(..., std::move(args)).
```
