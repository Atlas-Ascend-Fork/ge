# AppendArg

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T08:00:38.462Z pushedAt=2026-10-28T08:50:45.438Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Appends a parameter to the end of `gert::AnnotatedKernelArgs`. The order in which parameters are appended is the order in which they are arranged in kernel args. Each successful append occupies one 8-byte parameter and simultaneously generates a `ge::ArgDesc` for address refresh or scalar pass-through.

## Prototype

```c++
ge::graphStatus AppendArg(const InputAddr &addr)
ge::graphStatus AppendArg(const OutputAddr &addr)
ge::graphStatus AppendArg(const WorkspaceAddr &addr)
ge::graphStatus AppendArg(uint64_t value)
```

## Parameters

| Function | Name | Input/Output | Description |
| --- | --- | --- | --- |
| `AppendArg(const InputAddr &)` | `addr` | Input | Logical input address descriptor. See [`InputAddr`](../InputAddr.md). |
| `AppendArg(const OutputAddr &)` | `addr` | Input | Logical output address descriptor. See [`OutputAddr`](../OutputAddr.md). |
| `AppendArg(const WorkspaceAddr &)` | `addr` | Input | Logical workspace address descriptor. See [`WorkspaceAddr`](../WorkspaceAddr.md). |
| `AppendArg(uint64_t)` | `value` | Input | 64-bit scalar value written directly to kernel args, with a custom value descriptor generated. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | `ge::graphStatus` | `GRAPH_SUCCESS(0)`: The parameter is appended successfully. Other values: The object is invalid, the IR index is out of range, or internal parameter storage fails. |

## Constraints

- If appending fails, the builder enters a failed state. Do not pass this object to `ExtractArgsData` or `AddLaunch` afterwards.

## Examples

```c++
const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
const auto workspace = ctx.MallocWorkSpace(workspace_size);
if ((input == nullptr) || (output == nullptr) || (workspace.addr == nullptr)) {
  return ge::GRAPH_FAILED;
}

gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::WorkspaceAddr{workspace.index, workspace.addr}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(uint64_t{1U}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// args occupy four parameters in the order described above.
return ctx.AddLaunch(launch_info, std::move(args));
```
