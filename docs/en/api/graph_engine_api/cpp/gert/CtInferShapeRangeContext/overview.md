# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:03:07.956Z pushedAt=2026-10-28T08:50:45.448Z nl=2 -->

CtInferShapeRangeContext inherits from InferShapeRangeContext and is used for compile-time ShapeRange inference, providing APIs for use at compile time.

This class is mainly used for resource operators, which are divided into operators that create resources and operators that use resources. These two types of operators associate resources by passing DT\_RESOURCE handles. The handle information is obtained at compile time through the APIs provided by CtInferShapeRangeContext.

The inheritance relationship of CtInferShapeRangeContext is as follows:

![Figure: ctInferShapeRangeContext](../../../figures/ctInfershaperangecontext.png)

## Header File to Include

```c++
#include <ct_infer_shape_range_context.h>
```
