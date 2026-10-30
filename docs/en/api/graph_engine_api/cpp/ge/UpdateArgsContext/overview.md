# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:36:45.261Z pushedAt=2026-10-28T07:42:19.875Z nl=2 -->

A context class for refreshing args of custom operators in static graph scenarios. It inherits from EagerOpExecutionContext and provides KernelArgs acquisition required for args refresh, which is used by custom operators to update args.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
const KernelArgs* GetKernelArgs(Placement placement, size_t index) const
```
