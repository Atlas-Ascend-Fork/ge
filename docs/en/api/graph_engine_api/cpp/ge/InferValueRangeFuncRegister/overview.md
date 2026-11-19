# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:36:18.896Z pushedAt=2026-10-28T07:40:22.819Z nl=2 -->

**InferValueRangeFuncRegister** is the operator function registration API. This API is referenced by other header files and is generally not called directly by operator developers.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
InferValueRangeFuncRegister(const char_t *const operator_type, const WHEN_CALL when_call,
const InferValueRangeFunc &infer_value_range_func)
InferValueRangeFuncRegister(const char_t *const operator_type)
~InferValueRangeFuncRegister() = default
```
