# COMMON\_INFER\_FUNC\_REG

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:15:17.223Z pushedAt=2026-10-28T07:40:22.138Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Registers the **InferShape** function of an operator.

The difference from [INFER\_FUNC\_REG](INFER_FUNC_REG.md) is that the **InferShape** function registered by this macro takes the **Operator** base class, rather than a subclass, as its input parameter. This API allows multiple operators to share the same **InferShape** function.

## Prototype

```c++
COMMON_INFER_FUNC_REG(op_name, x)
```

This function automatically calls **COMMON_INFER_VERIFY_FUNC(x)** internally. The **x** in the **COMMON_INFER_VERIFY_FUNC(x)** function is a pointer to "x" in **COMMON_INFER_FUNC_REG(op_name, x)**.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_name | Input | Operator type. |
| x | Input | Name of the **InferShape** function, which must be consistent with the **InferShape** function name in [IMPLEMT_COMMON_INFERFUNC](IMPLEMT_COMMON_INFERFUNC.md). |

## Return Value

None

## Constraints

None
