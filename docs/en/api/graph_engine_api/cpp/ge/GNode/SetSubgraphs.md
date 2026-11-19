# SetSubgraphs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:11:19.824Z pushedAt=2026-10-28T07:40:22.722Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Updates the Tensor format of a specified output port.

## Prototype

```c++
graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph_ir_name | Input | Name of the dynamic-count subgraph registered in the operator IR. Taking the registration method **.DYNAMIC_GRAPH("Dx")** as an example, **subgraph_ir_name** corresponds to "Dx". |
| subgraphs | Input | Multiple subgraph instances to be bound to the node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
