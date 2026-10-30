# GetInputHandleShapesAndTypes

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:29:23.175Z pushedAt=2026-10-28T07:40:22.791Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Obtains the [ShapeAndType](../ShapeAndType/overview.md) of the operator input handle in the inference context.

## Prototype

```c++
const std::vector<std::vector<ShapeAndType>> &GetInputHandleShapesAndTypes() const
```

## Parameters

None

## Return Value

[ShapeAndType](../ShapeAndType/overview.md) of the operator input handle.

## Constraints

None
