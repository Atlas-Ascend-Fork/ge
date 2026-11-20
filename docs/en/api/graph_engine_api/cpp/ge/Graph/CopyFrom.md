# CopyFrom

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:14:37.086Z pushedAt=2026-10-28T07:40:22.734Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Copies the source graph to generate the target graph.

The **graph\_name** of the target graph preferentially uses the one specified by the user; if not specified, the **graph\_name** of the source graph is used. The **graph\_id/session** ownership must be added by the user after copying.

## Prototype

```c++
graphStatus Graph::CopyFrom(const Graph &src_graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| src_graph | Input | Source graph to be copied. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
