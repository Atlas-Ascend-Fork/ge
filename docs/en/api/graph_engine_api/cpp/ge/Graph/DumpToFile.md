# DumpToFile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:15:21.959Z pushedAt=2026-10-28T07:40:22.737Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Serializes the **graph** to a file in the execution path.

It does not contain data such as weights, but only the graph structure and related attributes.

## Prototype

```c++
graphStatus DumpToFile(DumpFormat format, const AscendString &suffix) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Format of the dump file, such as onnx and txt. For the DumpFormat enumeration, see [DumpFormat](../DumpFormat.md). |
| suffix | Input | Prefix of the file name. For example, in **ge_proto_xxxx_GraphPreRunBegin.txt**, **GraphPreRunBegin** is the prefix. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
