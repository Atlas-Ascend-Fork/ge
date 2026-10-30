# REG\_OP\_BACKEND

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-16T11:29:18.779Z pushedAt=2026-10-28T07:40:22.182Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/custom\_op.h\>

## Description

You can register a custom operator implementation class to a specified operator type and backend. The framework invokes **REG\_OP\_BACKEND** at the very beginning of compilation to register the custom operator.

## Prototype

```c++
REG_OP_BACKEND(custom_op_class, op_type, backend)
```

## Parameters

| Parameter | Input/Output | Description                                                                                        |
| --- | --- |---------------------------------------------------------------------------------------------|
| custom_op_class | Input | Custom operator implementation class.                |
| op_type | Input | Name of the operator type to be registered. |
| backend | Input | Backend type of the custom operator, which is the enumeration class [OpBackend](./OpBackend.md).                                   |

## Return Value

None

## Constraints

- The same **op_type** can register custom operator implementations for different **backend** types separately, but only one custom operator implementation can be registered under the same **backend**.

## Examples

```c++
#include "graph/custom_op.h"

class AddHostCpu final : public ge::HostCpuExecuteOp {
 public:
  ge::graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
    // Host CPU execution logic.
    return ge::GRAPH_SUCCESS;
  }
};

REG_OP_BACKEND(AddHostCpu, "Add", ge::OpBackend::kHostCPU);
```
