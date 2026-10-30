# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:29:50.560Z pushedAt=2026-10-28T07:40:22.317Z nl=2 -->

The base class of custom operators, used to implement custom operations in eager execution mode. Eager execution mode is commonly used in deep learning frameworks and allows operations to be executed directly without building a complete computation graph, providing a more intuitive and flexible debugging experience.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Execute(gert::EagerOpExecutionContext *ctx) = 0
```
