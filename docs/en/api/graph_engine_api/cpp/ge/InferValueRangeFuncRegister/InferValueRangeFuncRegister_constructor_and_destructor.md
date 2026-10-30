# InferValueRangeFuncRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:35:53.090Z pushedAt=2026-10-28T07:40:22.820Z nl=2 -->

## Header File

\#include <graph/operator\_factory.h\>

## Description

The constructor and destructor of **InferValueRangeFuncRegister**.

## Prototype

```c++
InferValueRangeFuncRegister(const char_t *const operator_type, const WHEN_CALL when_call,
const InferValueRangeFunc &infer_value_range_func)
InferValueRangeFuncRegister(const char_t *const operator_type)
~InferValueRangeFuncRegister() = default
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |
| when_call | Input | Invocation scenario of the infer function. |
| infer_value_range_func | Input | Operator InferValueRange function. |

## Return Value

The **InferValueRangeFuncRegister** constructor returns an object of the **InferValueRangeFuncRegister** type.

## Constraints

The operator **InferValueRangeFuncRegister** function registration API is referenced by other header files and is generally not called directly by operator developers.
