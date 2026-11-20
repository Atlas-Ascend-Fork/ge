# MakeOutputRefInput

<!-- md-trans-meta sourceCommit=0d886336c529b55e75a4f7004f7a6e2ed7a35b2f translatedAt=2026-10-28T08:10:43.115Z pushedAt=2026-10-28T08:50:45.480Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/host\_cpu\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Specifies that the memory address of a given output references a given input.

## Prototype

```c++
Tensor *MakeOutputRefInput(size_t output_index, size_t input_index)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| output_index | Input | Output index. |
| input_index | Input | Input index. |

## Return Value

The output Tensor pointer corresponding to output_index.

## Constraints

The output parameter corresponding to output_index and the input parameter corresponding to input_index must have the same name in the operator IR prototype definition; otherwise, the API call fails.

## Examples

The following is a simple operator IR prototype definition, in which both the input and output parameters are named x.

```c++
REG_OP(Identity)
    .INPUT(x, TensorType::ALL())
    .OUTPUT(x, TensorType::ALL())
    .OP_END_FACTORY_REG(Identity)
```

The following snippet is in the `HostCpuExecuteOp::Execute` implementation. It sets the 0th output to reference the memory address of the 0th input and checks the return value.

```c++
ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
  gert::Tensor *output = ctx->MakeOutputRefInput(0U, 0U);
  if (output == nullptr) {
    return ge::GRAPH_FAILED;
  }

  return ge::GRAPH_SUCCESS;
}
```
