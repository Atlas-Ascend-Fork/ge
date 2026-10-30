# AddControlEdge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:12:14.926Z pushedAt=2026-10-28T07:40:22.726Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Adds a control edge.

## Prototype

```c++
graphStatus AddControlEdge(GNode &src_node, GNode &dst_node)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| src_node | Input | Source node of the connecting edge. |
| dst_node | Input | Destination node of the connecting edge. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
