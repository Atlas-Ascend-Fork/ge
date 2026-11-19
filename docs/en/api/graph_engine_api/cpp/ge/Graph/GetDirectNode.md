# GetDirectNode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:18:27.134Z pushedAt=2026-10-28T07:40:22.747Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains all nodes in this graph, excluding the nodes of subgraphs within this graph.

## Prototype

```c++
std::vector<GNode> GetDirectNode() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector\<GNode> | All nodes in the graph, output in the order of the system topological sort result. |

## Constraints

None
