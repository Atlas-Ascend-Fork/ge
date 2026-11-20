# AnnotatedKernelLaunchInfo

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:53:15.988Z pushedAt=2026-10-28T08:50:45.412Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

`gert::AnnotatedKernelLaunchInfo` describes the metadata of a declarative kernel launch and serves as the first parameter of [`AnnotatedArgsContext::AddLaunch`](AnnotatedArgsContext/AddLaunch.md). It describes only the kernel entry, binary, block dimension, and logical stream; the kernel parameters themselves are provided by [`AnnotatedKernelArgs`](AnnotatedKernelArgs/overview.md).

## Prototype

```c++
namespace gert {
struct AnnotatedKernelLaunchInfo {
  const char *kernel_name = nullptr;  // Entry function name in the kernel binary.
  const void *kernel_bin = nullptr;   // Kernel binary data.
  size_t kernel_bin_size = 0U;        // Size of the kernel binary data, in bytes.
  uint32_t block_dim = 0U;            // Block dimension of the kernel.
  uint32_t stream_id = 0U;            // Logical stream ID where the launch resides.
};
}  // namespace gert
```

## Parameters

| Name | Type | default value | Description |
| --- | --- | --- | --- |
| `kernel_name` | `const char *` | `nullptr` | Entry function name in the kernel binary. When passed to `AddLaunch`, it must be non-null and its first character must not be `\0`. |
| `kernel_bin` | `const void *` | `nullptr` | Start address of the kernel binary data. It must be non-null when passed to `AddLaunch`. |
| `kernel_bin_size` | `size_t` | `0U` | Size of the binary data, in bytes; must be greater than 0. |
| `block_dim` | `uint32_t` | `0U` | Block dimension of the kernel; must be greater than 0. |
| `stream_id` | `uint32_t` | `0U` | Logical stream ID where the launch resides. Use the return value of [`AnnotatedArgsContext::GetStreamId`](AnnotatedArgsContext/GetStreamId.md), which must be consistent with the node's main stream. |

## Return Value

None

## Constraints

None

## Examples

The following `kKernelBin` and `kKernelBinSize` merely represent the kernel binary actually compiled by the user and its size, rather than an executable sample binary.

```c++
extern const uint8_t kKernelBin[];
extern const size_t kKernelBinSize;

gert::AnnotatedKernelArgs args(
    gert::InputAddr{0U, input_addr}, uint64_t{1U});
const gert::AnnotatedKernelLaunchInfo launch_info{
    "my_kernel", kKernelBin, kKernelBinSize, 32U, ctx.GetStreamId()};
const auto ret = ctx.AddLaunch(launch_info, std::move(args));
if (ret != ge::GRAPH_SUCCESS) {
  return ret;
}
return ge::GRAPH_SUCCESS;
```
