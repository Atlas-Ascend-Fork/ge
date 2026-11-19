# GetRequiredInputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:09:55.590Z pushedAt=2026-10-28T08:50:45.477Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the REQUIRED\_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetRequiredInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Returns the tensor pointer, or a null pointer on exception.

## Constraints

None

## Examples

The following snippet is in the `HostCpuExecuteOp::Execute` implementation and obtains the `REQUIRED_INPUT` input with index 0 in the operator IR prototype.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *input = ctx->GetRequiredInputTensor(0U);
  if (input == nullptr) {
    return ge::GRAPH_FAILED;
  }

  // Use input to obtain the description information or data of the input tensor.
  return ge::GRAPH_SUCCESS;
}
```
