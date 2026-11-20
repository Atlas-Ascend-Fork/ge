# Execute

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-16T14:27:32.555Z pushedAt=2026-10-28T07:40:22.785Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: liblowering.so

## Description

Execution function of a Host CPU custom operator.

## Prototype

```c++
virtual graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) = 0
```

## Parameters

| Parameter | Input/Output | Description                                                         |
| --- | --- |--------------------------------------------------------------|
| ctx | Input | Execution context, through which the input tensor can be obtained and output memory can be allocated. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | `GRAPH_SUCCESS(0)`: Execution succeeded; other values: Execution failed. |

## Constraints

None

## Examples

The following example reads two inputs in the Execute function of a custom operator, allocates one output, and completes the Host CPU computation.

```c++
#include "graph/custom_op.h"

class AddHostCpu final : public ge::HostCpuExecuteOp {
 public:
  ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
    const auto *x = ctx->GetInputTensor(0U);
    const auto *y = ctx->GetInputTensor(1U);
    if ((x == nullptr) || (y == nullptr)) {
      return ge::GRAPH_FAILED;
    }

    auto *z = ctx->MallocOutputTensor(0U, x->GetShape(), x->GetFormat(), x->GetDataType());
    if (z == nullptr) {
      return ge::GRAPH_FAILED;
    }

    // Complete the Host CPU computation based on x and y.
    return ge::GRAPH_SUCCESS;
  }
};

REG_OP_BACKEND(AddHostCpu, "Add", ge::OpBackend::kHostCPU);
```
