# INFER\_FORMAT\_FUNC\_REG

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:20:56.080Z pushedAt=2026-10-28T07:40:22.159Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Registers the **InferFormat** implementation of an operator.

GE calls the **InferFormat** function of the entire graph once before and once after the Shape and Dtype inference of the entire graph, during which the **InferFormat** function of each operator is called separately. If an operator does not register an **InferFormat** function, GE uses the default inference function, that is, the output Format equals the input Format.

## Prototype

```c++
#define INFER_FORMAT_FUNC_REG(op_name, x) \
__INFER_FORMAT_FUNC_REG_IMPL__(op_name, INFER_FORMAT_FUNC(op_name, x), __COUNTER__)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_name | Input | Operator type. |
| x | Input | **InferFormat** function name, which uses **func_name** in **IMPLEMT_INFERFORMAT_FUNC**. |

## Return Value

None

## Constraints

None

## Examples and Related APIs

```c++
INFER_FORMAT_FUNC_REG(Transpose, TransposeInferFormat);
```
