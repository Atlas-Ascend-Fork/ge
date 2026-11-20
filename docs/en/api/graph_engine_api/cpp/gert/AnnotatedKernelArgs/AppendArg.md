# AppendArg

<!-- md-trans-meta sourceCommit=cdbc38c7db7112e5c8cb9d0214451a84c91c89f6 translatedAt=2026-11-17T02:41:21.542Z pushedAt=2026-11-18T02:41:08.869Z nl=2 -->

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
ge::graphStatus AppendArg(const CustomDataBlock &data)
ge::graphStatus AppendArg(uint64_t value)
```

## Parameters

| Function | Name | Input/Output | Description |
| --- | --- | --- | --- |
| `AppendArg(const InputAddr &)` | `addr` | Input | Logical input address descriptor. See [`InputAddr`](../InputAddr.md). |
| `AppendArg(const OutputAddr &)` | `addr` | Input | Logical output address descriptor. See [`OutputAddr`](../OutputAddr.md). |
| `AppendArg(const WorkspaceAddr &)` | `addr` | Input | Logical workspace address descriptor. See [`WorkspaceAddr`](../WorkspaceAddr.md). |
| `AppendArg(const CustomDataBlock &)` | `data` | Input | Variable-length custom data descriptor. See [`CustomDataBlock`](../CustomDataBlock.md). The data is deep-copied when appended, occupying only one 8-byte slot in args, with the slot value being the device address of the data. The data length is not passed down with args. For how the kernel side obtains the length, see the Constraints of `CustomDataBlock`. |
| `AppendArg(uint64_t)` | `value` | Input | 64-bit scalar value written directly to kernel args, with a custom value descriptor generated. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | `ge::graphStatus` | `GRAPH_SUCCESS(0)`: appended successfully; other values: invalid object, IR index out of range, non-zero `reserved` in `CustomDataBlock`, or internal parameter storage failure. |

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

MyTiling tiling{static_cast<uint32_t>(sizeof(MyTiling)), 8U};  // MyTiling is a structure definition shared by the host side and the device side.
gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::WorkspaceAddr{workspace.index, workspace.addr}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{&tiling, sizeof(tiling)}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(uint64_t{1U}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// args occupy five parameters in the above order, where the fourth parameter is the device address of the tiling data.
return ctx.AddLaunch(launch_info, std::move(args));
```
