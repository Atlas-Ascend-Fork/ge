# MallocOutputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:11:30.358Z pushedAt=2026-10-28T08:50:45.481Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Allocates Host memory for an output tensor and initializes the basic information of the output tensor.

The memory of the output tensor is managed by the context constructor. The API caller does not need to release it.

## Prototype

```c++
Tensor *MallocOutputTensor(size_t index, const StorageShape &shape, const StorageFormat &format, ge::DataType dtype)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |
| shape | Input | Shape of the output tensor. |
| format | Input | Format of the output tensor. |
| dtype | Input | Data type of the output tensor. |

## Return Value

Returns a tensor pointer, or a null pointer on failure.

## Constraints

None

## Examples

The following snippet is in the `HostCpuExecuteOp::Execute` implementation. It uses the shape, format, and data type of the 0th input to allocate the 0th output tensor.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *input = ctx->GetInputTensor(0U);
  if (input == nullptr) {
    return ge::GRAPH_FAILED;
  }

  gert::Tensor *output =
      ctx->MallocOutputTensor(0U, input->GetShape(), input->GetFormat(), input->GetDataType());
  if (output == nullptr) {
    return ge::GRAPH_FAILED;
  }

  return ge::GRAPH_SUCCESS;
}
```
