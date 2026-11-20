# GetDynamicOutputIndexesByName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:00:56.244Z pushedAt=2026-10-28T07:40:22.685Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the dynamic output port indexes by the dynamic output port name of the operator.

## Prototype

```c++
graphStatus GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Dynamic output port name. |
| indexes | Output | Dynamic output port number of the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
