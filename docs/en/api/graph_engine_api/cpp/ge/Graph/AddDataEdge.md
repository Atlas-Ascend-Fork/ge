# AddDataEdge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:12:57.565Z pushedAt=2026-10-28T07:40:22.727Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Adds a data edge.

## Prototype

```c++
graphStatus AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node, const int32_t dst_port_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| src_node | Input | Source node of the connecting edge. |
| src_port_index | Input | Output port index of the source node. |
| dst_node | Input | Destination node of the connecting edge. |
| dst_port_index | Input | Input port index of the destination node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
