# GetInputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:08:31.927Z pushedAt=2026-10-28T08:50:45.470Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer based on the input index.

## Prototype

```c++
const Tensor *GetInputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |

## Return Value

Returns the tensor pointer, or a null pointer on exception.

## Constraints

None

## Examples

The following snippet is in the `HostCpuExecuteOp::Execute` implementation and obtains the 0th input tensor.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *input = ctx->GetInputTensor(0U);
  if (input == nullptr) {
    return ge::GRAPH_FAILED;
  }

  // Use input to obtain the description information or data of the input tensor.
  return ge::GRAPH_SUCCESS;
}
```
