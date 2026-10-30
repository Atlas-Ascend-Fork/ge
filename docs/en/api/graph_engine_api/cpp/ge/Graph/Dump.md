# Dump

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:15:17.656Z pushedAt=2026-10-28T07:40:22.735Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Serializes the **graph** into an **ostream**.

It does not include data such as weights; it only includes the graph structure and related attributes.

## Prototype

```c++
graphStatus Dump(DumpFormat format, std::ostream &o_stream) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Format of the dump file, such as onnx and txt. For the DumpFormat enum, see [DumpFormat](../DumpFormat.md). |
| o_stream | Input | Output stream. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
