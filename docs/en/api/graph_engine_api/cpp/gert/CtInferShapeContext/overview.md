# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:02:29.773Z pushedAt=2026-10-28T08:50:45.446Z nl=2 -->

CtInferShapeContext inherits from InferShapeContext and is used for compile-time shape inference, providing APIs for use at compile time.

This class is mainly used for resource-type operators, which fall into two categories: operators that create resources and operators that use resources. These two categories of operators are associated by passing DT\_RESOURCE handles. The handle information is obtained at compile time through the APIs provided by CtInferShapeContext.

The inheritance relationship of CtInferShapeContext is as follows:

![Diagram](../../../figures/ctInfershapecontext.png)

## Header File to Include

```c++
#include <ct_infer_shape_context.h>
```
