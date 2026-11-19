# GetOutputHandleShapesAndTypes

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:30:23.074Z pushedAt=2026-10-28T07:40:22.795Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Obtains the [ShapeAndType](../ShapeAndType/ShapeAndType.md) of the operator output handle in the inference context.

## Prototype

```c++
const std::vector<std::vector<ShapeAndType>> &GetOutputHandleShapesAndTypes() const
```

## Parameters

None

## Return Value

[ShapeAndType](../ShapeAndType/ShapeAndType.md) of the operator output handle.

## Constraints

None
