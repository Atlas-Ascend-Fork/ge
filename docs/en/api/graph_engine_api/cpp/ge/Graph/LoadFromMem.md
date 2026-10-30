# LoadFromMem

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:20:46.824Z pushedAt=2026-10-28T07:40:22.755Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Reads the serialized **Graph** from memory.

## Prototype

```c++
graphStatus LoadFromMem(const GraphBuffer &graph_buffer)
graphStatus LoadFromMem(const uint8_t *data, const size_t len)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_buffer | Input | Buffer of the **Graph** to be read. |
| data | Input | Starting memory address of the **Graph** to be read. |
| len | Input | Memory length of the **Graph** to be read. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

Only files in air format are supported.
