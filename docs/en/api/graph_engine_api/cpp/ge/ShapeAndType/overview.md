# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:29:31.660Z pushedAt=2026-10-28T07:42:19.842Z nl=2 -->

Allows setting and retrieving the shape and data type of the corresponding object.

## Header File to Include

```c++
#include <graph/inference_context.h>
```

## Public Member Functions

```c++
ShapeAndType()
~ShapeAndType()
ShapeAndType(const Shape &shape, DataType data_type)
void SetShape(const Shape &shape)
const Shape &GetShape() const
void SetType(DataType data_type)
DataType GetDataType() const
```
