# RemoveNode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:22:18.780Z pushedAt=2026-10-28T07:40:22.761Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Deletes the specified node from the graph and removes the edges between nodes.

## Prototype

```c++
graphStatus RemoveNode(GNode &node)
graphStatus RemoveNode(GNode &node, bool contain_subgraph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node to be deleted from the graph. If the node is in a subgraph, set `contain_subgraph` to **true**. |
| contain_subgraph | Input | Whether the specified node to be deleted is in a subgraph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | `graphStatus` | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
