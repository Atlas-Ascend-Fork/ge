# GetDynamicInputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:08:29.763Z pushedAt=2026-10-28T08:50:45.469Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the DYNAMIC_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description                                |
| --- | --- |-------------------------------------|
| ir_index | Input | Index in the IR prototype definition.            |
| relative_index | Input | Relative index of the instantiated input. For example, if a DYNAMIC_INPUT is instantiated into three inputs, the valid range of relative_index is [0,2]. |

## Return Value

Returns the tensor pointer, or a null pointer on exception.

## Constraints

None

## Examples

The following snippet is in the `HostCpuExecuteOp::Execute` implementation. It obtains the instantiated input with relative index 1 of the `DYNAMIC_INPUT` whose index is 0 in the operator IR prototype.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *input = ctx->GetDynamicInputTensor(0U, 1U);
  if (input == nullptr) {
    return ge::GRAPH_FAILED;
  }

  // Use input to obtain the description information or data of the dynamic input tensor.
  return ge::GRAPH_SUCCESS;
}
```
