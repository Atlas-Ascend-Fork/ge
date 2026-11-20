# GetAllNodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:16:42.547Z pushedAt=2026-10-28T07:40:22.741Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains all nodes in the graph, including the nodes of this graph and its subgraphs.

## Prototype

```c++
std::vector<GNode> GetAllNodes() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector\<GNode> | All nodes in the graph, output in system topology order. |

## Constraints

None
