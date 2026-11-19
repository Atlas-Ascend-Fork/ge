# ELMTWISE\_INFER\_SHAPEANDTYPE

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:18:25.764Z pushedAt=2026-10-28T07:40:22.147Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Provides a common function macro encapsulation for operator developers to develop the **InferShape** function. Based on the input **shape** and **dtype**, this function sets the output **shape** and **dtype**.

For example, if the input **shape** is (1,2,3,4) and the **dtype** is float, this macro sets the operator's output **shape** to (1,2,3,4) and the output **dtype** to float.

## Prototype

```c++
ELMTWISE_INFER_SHAPEANDTYPE(in_name, out_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| in_name | Input | Operator input. |
| out_name | Input | Operator output. |

## Return Value

Returns success or failure.

## Constraints

None

## Examples

```c++
COMMON_INFER_FUNC_REG(DiagD, ELMTWISE_INFER_SHAPEANDTYPE("assist", "y"));
```
