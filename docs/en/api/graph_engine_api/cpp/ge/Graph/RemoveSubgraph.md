# RemoveSubgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:22:39.450Z pushedAt=2026-10-28T07:40:22.762Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Removes the subgraph by **name**.

## Prototype

```c++
graphStatus RemoveSubgraph(const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS**: The subgraph is removed successfully.<br>Other values: The subgraph fails to be removed. |

## Constraints

None
