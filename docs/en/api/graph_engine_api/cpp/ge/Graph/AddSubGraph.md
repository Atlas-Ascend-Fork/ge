# AddSubGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:13:51.609Z pushedAt=2026-10-28T07:40:22.731Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Adds a subgraph.

The **name** of the subgraph is used as the key, and duplicates are not allowed. If a subgraph with the same **name** is added, the subgraph addition fails.

## Prototype

```c++
graphStatus AddSubGraph(const Graph &subgraph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph | Input | Subgraph instance to be added. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS**: The subgraph is added successfully.<br>Others: The subgraph addition fails. |

## Constraints

None
