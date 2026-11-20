# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:31:38.839Z pushedAt=2026-10-28T07:42:19.853Z nl=2 -->

The shape inference API for custom operators applies to scenarios where an operator performs shape and data type inference based on the inference context.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus InferShape(gert::InferShapeContext *ctx) = 0
virtual graphStatus InferDataType(gert::InferDataTypeContext *ctx) = 0
```
