# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:35:35.507Z pushedAt=2026-10-28T07:40:22.815Z nl=2 -->

The **infershape** function registration API of the operator. This API is referenced by other header files and generally does not need to be called directly by operator developers.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
InferShapeFuncRegister (const std::string &operator_type, const InferShapeFunc &infer_shape_func)
InferShapeFuncRegister(const char *const operator_type, const InferShapeFunc &infer_shape_func)
~ InferShapeFuncRegister()
```
