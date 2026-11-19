# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:18:53.163Z pushedAt=2026-10-28T07:40:22.272Z nl=2 -->

This is the graph construction and compilation API for custom operators. After an operator enters the GE graph construction and compilation process, if this API is implemented, GE calls back the **Compile** API to complete operator compilation-related processing.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Compile(gert::OpCompileContext *ctx) = 0
```
