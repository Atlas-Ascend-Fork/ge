# aclgrphDumpGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:05:57.614Z pushedAt=2026-10-28T07:40:22.114Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Exports the input Graph to a text file.

## Prototype

```c++
graphStatus aclgrphDumpGraph(const ge::Graph &graph, const char_t *file, const size_t len)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Input Graph. |
| file | Input | Output file name. |
| len | Input | Character length of the output file name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
