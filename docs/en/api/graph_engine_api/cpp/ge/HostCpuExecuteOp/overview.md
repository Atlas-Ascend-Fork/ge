# Overview

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-16T14:27:52.089Z pushedAt=2026-10-28T07:40:22.783Z nl=2 -->

Base class of custom operators, used to implement custom operations on the host, typically for computation in constant folding or runtime Host CPU scheduling scenarios.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) = 0
```
