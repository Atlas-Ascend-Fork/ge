# SetSubgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:10:28.466Z pushedAt=2026-10-28T07:40:22.719Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Updates the Tensor format of a specified output port.

## Prototype

```c++
graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph_ir_name | Input | Subgraph name used when the operator IR is registered. Taking the registration method of **.GRAPH(X)** as an example, **subgraph_ir_name** corresponds to "X". |
| subgraph | Input | Subgraph instance to be bound to the node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
