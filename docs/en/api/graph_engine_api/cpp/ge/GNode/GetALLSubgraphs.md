# GetALLSubgraphs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:59:45.381Z pushedAt=2026-10-28T07:40:22.681Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains all subgraph objects of the root graph of the current node.

## Prototype

```c++
graphStatus GetALLSubgraphs(std::vector<GraphPtr> &graph_list) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_list | Output | Pointer to the subgraph. Empty indicates that there is no corresponding subgraph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: success.<br>Other values: failure. |

## Constraints

None
