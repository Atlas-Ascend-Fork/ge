# REG\_AUTO\_MAPPING\_OP

<!-- md-trans-meta sourceCommit=f466c256e1a282548a091433bcbec11edd4d9538 translatedAt=2026-10-16T11:25:56.577Z pushedAt=2026-10-28T07:40:22.176Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/custom_op.h\>

## Description

You can register custom operators with the framework, which invokes **REG\_AUTO\_MAPPING\_OP** at the very beginning of compilation to register the custom operators.

## Prototype

```c++
REG_AUTO_MAPPING_OP(custom_op_class)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| custom_op_class | Input | Custom operator implementation class. |

## Return Value

None

## Constraints

- **REG\_AUTO\_MAPPING\_OP** uses the Device backend by default and uses the class name corresponding to **custom_op_class** as the operator type. To explicitly specify the operator type or backend, use [REG\_OP\_BACKEND](./REG_OP_BACKEND.md).
