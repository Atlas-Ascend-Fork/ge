# GetOutputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:05:46.580Z pushedAt=2026-10-28T07:40:22.703Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the Tensor format of the specified output port.

## Prototype

```c++
graphStatus GetOutputDesc(const int32_t index, TensorDesc &tensor_desc) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output port number of the operator. |
| tensor_desc | Output | Obtained Tensor format. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
