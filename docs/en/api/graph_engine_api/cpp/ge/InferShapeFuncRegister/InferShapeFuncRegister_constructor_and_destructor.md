# InferShapeFuncRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:35:34.410Z pushedAt=2026-10-28T07:40:22.818Z nl=2 -->

## Header File

\#include <graph/operator\_factory.h\>

## Description

Constructor and destructor of **InferShapeFuncRegister**.

## Prototype

> [!NOTE]
>APIs with the string data type will be deprecated in later versions. It is recommended to use APIs with a non-string data type.

```c++
InferShapeFuncRegister (const std::string &operator_type, const InferShapeFunc &infer_shape_func)
InferShapeFuncRegister(const char *const operator_type, const InferShapeFunc &infer_shape_func)
~ InferShapeFuncRegister()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |
| infer_shape_func | Input | Operator **InferShape** function. |

## Return Value

The **InferShapeFuncRegister** constructor returns an object of type **InferShapeFuncRegister**.

## Constraints

API for registering the operator **InferShape** function. This API is referenced by other header files and generally does not need to be called directly by operator developers.
