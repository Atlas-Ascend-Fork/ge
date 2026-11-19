# AddEdgeAndUpdatePeerDesc

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-16T12:30:45.819Z pushedAt=2026-10-28T07:40:22.323Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Adds an edge and updates the description of the peer Tensor.

## Prototype

```c++
graphStatus AddEdgeAndUpdatePeerDesc(Graph &graph, GNode &src_node, int32_t src_port_index, GNode &dst_node, int32_t dst_port_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph object to which the source node **src_node** and the destination node **dst_node** belong. |
| src_node | Input | Source node. |
| src_port_index | Input | Port index of the source node. |
| dst_node | Input | Destination node. |
| dst_port_index | Input | Port index of the destination node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success<br>Other values: Failure |

## Constraints

None
