# INFER\_FUNC\_REG

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:21:07.733Z pushedAt=2026-10-28T07:40:22.160Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Registers the **InferShape** function of the operator.

## Prototype

```c++
INFER_FUNC_REG(op_name, x)
```

This function automatically calls **INFER\_VERIFY\_FUNC\(op\_name, x\)** internally.  In the **INFER_VERIFY_FUNC** function, **op_name** is the operator type, and **x** is a pointer to "x" in **INFER\_FUNC\_REG (op\_name,x)**.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_name | Input | Operator type. |
| x | Input | **InferShape** function name, which must be consistent with the **InferShape** function name of [IMPLEMT_INFERFUNC](IMPLEMT_INFERFUNC.md). |

## Return Value

None

## Constraints

None
