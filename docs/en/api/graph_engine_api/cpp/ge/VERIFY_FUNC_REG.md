# VERIFY\_FUNC\_REG

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:30:07.666Z pushedAt=2026-10-28T07:40:22.186Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Registers the **Verify** function of an operator.

## Prototype

```c++
VERIFY_FUNC_REG(op_name, x)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_name | Input | Operator type. |
| x | Input | **Verify** function name, which must be consistent with the **Verify** function name in [IMPLEMT_VERIFIER](IMPLEMT_VERIFIER.md). |

## Return Value

None

## Constraints

None
