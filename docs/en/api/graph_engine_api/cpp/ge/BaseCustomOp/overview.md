# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:18:03.140Z pushedAt=2026-10-28T07:40:22.269Z nl=2 -->

Base class of custom operators.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Execute(gert::EagerOpExecutionContext *ctx) = 0;
virtual ~BaseCustomOp() = default;
```
