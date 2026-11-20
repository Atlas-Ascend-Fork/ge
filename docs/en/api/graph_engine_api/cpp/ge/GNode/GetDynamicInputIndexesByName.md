# GetDynamicInputIndexesByName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:00:13.034Z pushedAt=2026-10-28T07:40:22.683Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the dynamic input port indexes by the dynamic input port name of the operator.

## Prototype

```c++
graphStatus GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Dynamic input port name. |
| indexes | Output | Dynamic input port number of the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
