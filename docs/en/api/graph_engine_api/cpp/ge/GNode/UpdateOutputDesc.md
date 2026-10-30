# UpdateOutputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:12:07.940Z pushedAt=2026-10-28T07:40:22.724Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Updates the Tensor format of a specified output port.

## Prototype

```c++
graphStatus UpdateOutputDesc(const int32_t index, const TensorDesc &tensor_desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output port number of the operator. |
| tensor_desc | Input | Tensor format to be updated. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
