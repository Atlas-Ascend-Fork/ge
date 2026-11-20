# SetOutputHandleShapesAndTypes

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:33:07.041Z pushedAt=2026-10-28T07:40:22.805Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Sets the [ShapeAndType](../ShapeAndType/overview.md) of the operator output handle in the inference context.

## Prototype

```c++
void SetOutputHandleShapesAndTypes(const std::vector<std::vector<ShapeAndType>> &shapes_and_types)
void SetOutputHandleShapesAndTypes(std::vector<std::vector<ShapeAndType>> &&shapes_and_types)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shapes_and_types | Input | [ShapeAndType](../ShapeAndType/overview.md) of the operator output handle. |

## Return Value

None

## Constraints

None
