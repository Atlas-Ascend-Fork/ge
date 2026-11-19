# GetSubgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:06:47.471Z pushedAt=2026-10-28T07:40:22.707Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the **index**-th subgraph object of the current node.

## Prototype

```c++
graphStatus GetSubgraph(uint32_t index, GraphPtr &graph) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Number of the subgraph. |
| graph | Output | Pointer to the subgraph. An empty value indicates that no corresponding subgraph exists. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
