# LoadFromFile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:20:46.236Z pushedAt=2026-10-28T07:40:22.754Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Reads a **Graph** from a file.

## Prototype

> [!NOTE]
> The APIs whose data type is **string** will be deprecated in later versions. It is recommended to use the APIs whose data type is not **string**.

```c++
graphStatus LoadFromFile(const std::string &file_name)
graphStatus LoadFromFile(const char_t *file_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| file_name | Input | File path and file name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

Only files in the air format are supported.
