# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:14:22.899Z pushedAt=2026-10-28T07:40:22.254Z nl=2 -->

Address refresh capability API for custom operators. In static graph scenarios, when I/O addresses change, the framework calls back the **UpdateHostArgs** method on operators that inherit this API to refresh the address references in **kernel args**.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) = 0
```
