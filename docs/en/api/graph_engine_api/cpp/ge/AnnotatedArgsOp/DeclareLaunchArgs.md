# DeclareLaunchArgs

<!-- md-trans-meta sourceCommit=6c0f2edd2793cdeb0ad99aa92d61e13b5213c8ef translatedAt=2026-10-16T12:03:27.937Z pushedAt=2026-10-28T07:40:22.223Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Declares the kernel launch parameters of a custom operator during the static graph compilation phase. The operator can obtain input/output tensors through `ctx`, apply for logical workspace, construct [`gert::AnnotatedKernelArgs`](../../gert/AnnotatedKernelArgs/overview.md), and submit the kernel launch using [`gert::AnnotatedArgsContext::AddLaunch`](../../gert/AnnotatedArgsContext/AddLaunch.md).

## Prototype

```c++
virtual graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Declarative parameter context. Through this context, you can obtain input/output tensors, allocate workspace, obtain the logical stream of the node, and add a kernel launch. See [`gert::AnnotatedArgsContext`](../../gert/AnnotatedArgsContext/overview.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | `GRAPH_SUCCESS(0)`: Declaration succeeded. Other values: Declaration failed, and GE terminates task generation for the current custom operator. |

## Constraints

- Before the callback returns successfully, `AddLaunch` must be called at least once.
- The caller must check the return value of each `AddLaunch` call; If a non-`GRAPH_SUCCESS` status is returned, return that status immediately so that this API returns a failure.
<!-- npu="x90,9030" id1 -->
- On-device scenario: `AddLaunch` can be called only once.
<!-- end id1 -->

## Examples

```c++
#include <cstdint>
#include <utility>

#include "graph/custom_op.h"

class MyAnnotatedOp : public ge::AnnotatedArgsOp {
 public:
  ge::graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) override {
    // Placeholder for documentation only. In actual use, replace it with a valid compiled kernel binary that matches kernel_name.
    static const uint8_t kKernelBin[] = {0x01U, 0x02U, 0x03U, 0x04U};

    const auto *input = ctx.GetInputTensor(0U);
    const auto *output = ctx.GetOutputTensor(0U);
    if ((input == nullptr) || (output == nullptr)) {
      return ge::GRAPH_FAILED;
    }

    const auto workspace = ctx.MallocWorkSpace(1024U);
    if (workspace.addr == nullptr) {
      return ge::GRAPH_FAILED;
    }

    gert::AnnotatedKernelArgs args(
        gert::InputAddr{0U, input->GetAddr()},
        gert::OutputAddr{0U, output->GetAddr()},
        workspace,
        uint64_t{1U});
    const auto launch_ret = ctx.AddLaunch(
        gert::AnnotatedKernelLaunchInfo{
            "my_annotated_kernel", kKernelBin, sizeof(kKernelBin), 32U, ctx.GetStreamId()},
        std::move(args));
    if (launch_ret != ge::GRAPH_SUCCESS) {
      return launch_ret;
    }
    return ge::GRAPH_SUCCESS;
  }
};
```

In the preceding example, `ctx.GetStreamId()` is used to ensure that the launch is consistent with the node's main stream, and the parameter builder is passed to `AddLaunch` via `std::move`.
