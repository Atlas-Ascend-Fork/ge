# RemoveEdge

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-16T14:21:55.704Z pushedAt=2026-10-28T07:40:22.759Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Deletes the specified connection edge in the graph.

## Prototype

```c++
graphStatus RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node, const int32_t dst_port_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| src_node | Input | Source node of the connection edge. |
| src_port_index | Input | Output port number of the source node (**-1** indicates a control edge). |
| dst_node | Input | Destination node of the connection edge. |
| dst_port_index | Input | Input port number of the destination node (**-1** indicates a control edge). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
