# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:22:56.432Z pushedAt=2026-10-28T07:40:22.530Z nl=2 -->

Base class of the fusion pass, which describes the input parameter information for pass execution.

## Header File to Include

```c++
#include <ge/fusion/pass/fusion_base_pass.h>
```

## Public Member Functions

```c++
virtual Status Run(GraphPtr &graph, CustomPassContext &pass_context) = 0
```
