# ShapeAndType Constructor and Destructor

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T13:30:45.395Z pushedAt=2026-10-28T07:42:19.849Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

**ShapeAndType** constructor and destructor.

## Prototype

```c++
ShapeAndType()
~ShapeAndType()
ShapeAndType(const Shape &shape, DataType data_type)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape to set. |
| data_type | Input | Data type to set. |

## Return Value

The **ShapeAndType** constructor returns an object of type **ShapeAndType**.

## Constraints

None
