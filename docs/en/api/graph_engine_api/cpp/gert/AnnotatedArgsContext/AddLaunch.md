# AddLaunch

<!-- md-trans-meta sourceCommit=6c0f2edd2793cdeb0ad99aa92d61e13b5213c8ef translatedAt=2026-10-28T07:54:28.090Z pushedAt=2026-10-28T08:50:45.419Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Adds a declarative kernel launch. The API extracts the parameters of [`AnnotatedKernelArgs`](../AnnotatedKernelArgs/overview.md) and records the kernel name, binary, block dim, and logical stream ID.

<!-- npu="x90,9030" id1 -->
On-device scenario: it must be called exactly once.
<!-- end id1 -->

## Prototype

```c++
ge::graphStatus AddLaunch(const AnnotatedKernelLaunchInfo &launch_info, AnnotatedKernelArgs &&args)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| launch_info | Input | Kernel launch information. `kernel_name` and `kernel_bin` must be non-empty, and `kernel_bin_size` and `block_dim` must be greater than 0. `stream_id` should use the value returned by [`GetStreamId`](GetStreamId.md). For details, see [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md). During the call, GE copies and saves the `kernel_name` and `kernel_bin` data; the caller can end the lifetime of these temporary data after the API returns. |
| args | Input | Kernel launch parameter builder, transferred as an rvalue reference. The parameter must be non-empty, and all previous parameter appending operations must have succeeded. During the call, GE copies and saves the args data; the caller can end the lifetime of args after the API returns. After the caller transfers it with `std::move`, it should not rely on the subsequent state of this object. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | ge::graphStatus | `GRAPH_SUCCESS(0)`: added successfully; other values: parameter or Context state is abnormal, and the addition failed. |

## Constraints

- The caller must check the return value of each call and immediately return the error status on failure.

## Examples

The following `kKernelBin` and `kKernelBinSize` only represent the kernel binary actually compiled by the user and its size, and are not executable sample binaries.

```c++
extern const uint8_t kKernelBin[];
extern const size_t kKernelBinSize;

const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
if ((input == nullptr) || (output == nullptr)) {
  return ge::GRAPH_FAILED;
}

gert::AnnotatedKernelArgs args(
    gert::InputAddr{0U, input->GetAddr()},
    gert::OutputAddr{0U, output->GetAddr()},
    uint64_t{1U});
const auto ret = ctx.AddLaunch(
    gert::AnnotatedKernelLaunchInfo{
        "my_kernel", kKernelBin, kKernelBinSize, 32U, ctx.GetStreamId()},
    std::move(args));
if (ret != ge::GRAPH_SUCCESS) {
  return ret;
}
return ge::GRAPH_SUCCESS;
```
