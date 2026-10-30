# UpdateInputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:11:22.127Z pushedAt=2026-10-28T07:40:22.723Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Updates the Tensor format of the specified input port.

## Prototype

```c++
graphStatus UpdateInputDesc(const int32_t index, const TensorDesc &tensor_desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Index of the input port to be updated. |
| tensor_desc | Input | Tensor format to be updated. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
