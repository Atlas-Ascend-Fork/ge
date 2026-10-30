# GetOutputIndexByName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:05:54.563Z pushedAt=2026-10-28T07:40:22.704Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the output port index by the port name of the operator.

## Prototype

```c++
graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Port name of the operator. |
| index | Output | Output port number of the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
