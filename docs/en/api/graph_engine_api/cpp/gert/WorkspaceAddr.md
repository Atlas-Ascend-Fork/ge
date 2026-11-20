# WorkspaceAddr

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:54:09.223Z pushedAt=2026-10-28T08:50:45.413Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

`gert::WorkspaceAddr` is a logical workspace address descriptor. It is used as a parameter of [`AnnotatedKernelArgs::AppendArg`](AnnotatedKernelArgs/AppendArg.md) to mark one of the kernel args as a workspace address.

## Prototype

```c++
namespace gert {
struct WorkspaceAddr {
  uint32_t index;    // Workspace index, returned by MallocWorkSpace.
  const void *addr;  // Logical address of the workspace.
};
}  // namespace gert
```

## Parameters

| Name | Type | Description |
| --- | --- | --- |
| `index` | `uint32_t` | Workspace index, returned by [`AnnotatedArgsContext::MallocWorkSpace`](AnnotatedArgsContext/MallocWorkSpace.md). The index increments from 0 in the order in which workspaces are requested, and is not the operator IR input/output index. The value must not exceed `INT32_MAX`. |
| `addr` | `const void *` | Logical address of the corresponding workspace, taken from the `addr` field in the structure returned by `MallocWorkSpace`. |

## Return Value

None

## Constraints

None

## Examples

```c++
const auto workspace = ctx.MallocWorkSpace(workspace_size);
if (workspace.addr == nullptr) {
  return ge::GRAPH_FAILED;
}
gert::AnnotatedKernelArgs args(workspace);
return ctx.AddLaunch(launch_info, std::move(args));
```
