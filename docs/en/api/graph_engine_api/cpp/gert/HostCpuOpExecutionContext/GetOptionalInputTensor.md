# GetOptionalInputTensor

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:08:57.603Z pushedAt=2026-10-28T08:50:45.472Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the OPTIONAL_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetOptionalInputTensor(size_t ir_index) const
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

The following snippet is in the `HostCpuExecuteOp::Execute` implementation and obtains the `OPTIONAL_INPUT` input with index 1 in the operator IR prototype. If no optional input is provided, a null pointer is returned.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  const gert::Tensor *optional_input = ctx->GetOptionalInputTensor(1U);
  if (optional_input != nullptr) {
    // Use optional_input to perform computation related to the optional input.
  }

  return ge::GRAPH_SUCCESS;
}
```
