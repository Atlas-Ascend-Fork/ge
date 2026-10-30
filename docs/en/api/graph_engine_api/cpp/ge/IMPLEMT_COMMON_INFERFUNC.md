# IMPLEMT\_COMMON\_INFERFUNC

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:19:57.187Z pushedAt=2026-10-28T07:40:22.154Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Encapsulates the operator's **Common\_InferShape** function.

The difference from [IMPLEMT\_INFERFUNC](IMPLEMT_INFERFUNC.md) is that this function automatically generates an object **op** of the **Operator** class, which can directly call the [Operator](./Operator/overview.md) API to implement **InferShape**. If the **InferShape** method is generic and can be called by the prototype implementations of multiple operators, this API can be selected for implementation.

## Prototype

```c++
IMPLEMT_COMMON_INFERFUNC(func_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| func_name | Input | **InferShape** function name, user-defined. |

## Return Value

None

## Constraints

None
