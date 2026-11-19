# GetSubGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:19:13.996Z pushedAt=2026-10-28T07:40:22.749Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains the subgraph by **name**.

## Prototype

```c++
GraphPtr GetSubGraph(const char *name) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | GraphPtr | Returns the subgraph pointer. |

## Constraints

None
