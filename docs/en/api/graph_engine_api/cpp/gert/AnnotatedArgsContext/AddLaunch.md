# AddLaunch

<!-- md-trans-meta sourceCommit=6c0f2edd2793cdeb0ad99aa92d61e13b5213c8ef translatedAt=2026-10-28T07:54:28.090Z pushedAt=2026-10-28T08:50:45.419Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Adds a declarative kernel launch. The API extracts the parameters of [`AnnotatedKernelArgs`](../AnnotatedKernelArgs/overview.md) and records the kernel name, binary, block dim, and logical stream ID.

The second prototype adds a launch and returns its `AnnotatedLaunchToken`, and can declare the predecessor launch tokens on which it depends, to express the ordering between multiple launches. Dependencies are expressed through `AnnotatedLaunchToken`: each launch successfully submitted through the second prototype obtains a token, and subsequent launches can reference these tokens as predecessors.

<!-- npu="x90,9030" id1 -->
On-device scenario: it must be called exactly once.
<!-- end id1 -->

## Prototype

```c++
ge::graphStatus AddLaunch(const AnnotatedKernelLaunchInfo &launch_info, AnnotatedKernelArgs &&args)
AnnotatedLaunchToken AddLaunch(const AnnotatedKernelLaunchInfo &launch_info, AnnotatedKernelArgs &&args,
                               const std::vector<AnnotatedLaunchToken> &predecessors)
```

- First prototype: adds a kernel launch without producing a token, and cannot serve as a dependency source for other launches.
- Second prototype: adds a kernel launch and returns its token; `predecessors` declares the list of predecessor tokens on which this launch depends. Pass an empty vector (`{}`) when there are no dependencies.

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| launch_info | Input | Kernel launch information. `kernel_name` and `kernel_bin` must be non-empty, and `kernel_bin_size` and `block_dim` must be greater than 0. `stream_id` should use a valid logical auxiliary stream ID returned by [`GetStreamId`](GetStreamId.md) or [`RequestAttachedStream`](RequestAttachedStream.md). For details, see [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md). During the call, GE copies and saves the `kernel_name` and `kernel_bin` data; the caller can end the lifetime of these temporary data after the API returns. |
| args | Input | Kernel launch parameter builder, transferred as an rvalue reference. The parameter must be non-empty, and all previous parameter appending operations must have succeeded. During the call, GE copies and saves the args data; the caller can end the lifetime of args after the API returns. After the caller transfers it with `std::move`, it should not rely on the subsequent state of this object. |
| predecessors | Input | List of predecessor launch tokens, which can be empty. Used only in the second prototype. Elements must be valid tokens returned by the second prototype earlier in this Context, must not be duplicated, and must not form forward dependencies. The list content is copied during the call, and the caller can end its lifetime after the API returns. |

## Return Value

- First prototype:

| Name | Type | Description |
| --- | --- | --- |
| - | ge::graphStatus | `GRAPH_SUCCESS(0)`: added successfully; other values: parameter or Context state is abnormal, and the addition failed. |

- The second prototype:

| Name | Type | Description |
| --- | --- | --- |
| - | AnnotatedLaunchToken | On success, returns a new token for this launch (`uint32_t`, incrementing from 0) for subsequent launches to declare dependencies; on failure, returns UINT32\_MAX and does not publish the launch. |

## Constraints

- The caller must check the return value of each call and immediately return the error status on failure.

## Examples

The following `kKernelBin` and `kKernelBinSize` only represent the kernel binary actually compiled by the user and its size, and are not executable sample binaries.

- The first prototype:

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

- The second prototype (`args0` and `args1` are constructed [`AnnotatedKernelArgs`](../AnnotatedKernelArgs/overview.md)):

```c++
extern const uint8_t kKernelBin[];
extern const size_t kKernelBinSize;

// Submit the first launch on the main stream, with no predecessor, and obtain the token.
const auto token0 = ctx.AddLaunch(
    gert::AnnotatedKernelLaunchInfo{"main_kernel", kKernelBin, kKernelBinSize, 32U, ctx.GetStreamId()},
    std::move(args0), {});
if (token0 == UINT32_MAX) {
  return ge::GRAPH_FAILED;
}

// Request a logical auxiliary stream and declare that the auxiliary stream launch depends on the main stream launch.
const auto aux_stream_id = ctx.RequestAttachedStream(ge::AscendString("aux"));
if (aux_stream_id == UINT32_MAX) {
  return ge::GRAPH_FAILED;
}
const auto token1 = ctx.AddLaunch(
    gert::AnnotatedKernelLaunchInfo{"aux_kernel", kKernelBin, kKernelBinSize, 32U, aux_stream_id},
    std::move(args1), {token0});
if (token1 == UINT32_MAX) {
  return ge::GRAPH_FAILED;
}
return ge::GRAPH_SUCCESS;
```
