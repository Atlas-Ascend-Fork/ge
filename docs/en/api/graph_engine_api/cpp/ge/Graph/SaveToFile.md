# SaveToFile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:22:47.692Z pushedAt=2026-10-28T07:40:22.764Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Saves the serialized Graph structure to a file.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. It is recommended to use the API with a non-string data type.

```c++
graphStatus SaveToFile(const std::string &file_name) const
graphStatus SaveToFile(const char_t *file_name) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| file_name | Input | Path and name of the file to be saved. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

The saved file is in air format and can be restored to a graph object through the [LoadFromFile](LoadFromFile.md) API.
