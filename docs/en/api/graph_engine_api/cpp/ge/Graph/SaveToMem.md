# SaveToMem

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:23:27.423Z pushedAt=2026-10-28T07:40:22.766Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Saves the serialized Graph to memory.

## Prototype

```c++
graphStatus SaveToMem(GraphBuffer &graph_buffer) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_buffer | Output | Buffer of the saved Graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: success.<br>Other values: failure. |

## Constraints

The content saved in memory is in air format and can be restored to a graph object through the [LoadFromMem](LoadFromMem.md) API.
