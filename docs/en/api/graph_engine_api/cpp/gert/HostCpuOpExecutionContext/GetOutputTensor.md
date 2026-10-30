# GetOutputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:09:42.008Z pushedAt=2026-10-28T08:50:45.474Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the output tensor pointer specified by index.

## Prototype

```c++
const Tensor *GetOutputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |

## Return Value

Returns the output tensor pointer, or a null pointer on error.

## Constraints

None

## Examples

The following snippet is in the `HostCpuExecuteOp::Execute` implementation. It obtains the 0th output tensor and checks the return value before use.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *output = ctx->GetOutputTensor(0U);
  if (output == nullptr) {
    return ge::GRAPH_FAILED;
  }

  // Use output to obtain the description information or data of the output tensor.
  return ge::GRAPH_SUCCESS;
}
```
