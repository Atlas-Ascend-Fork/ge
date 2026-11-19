# GetInputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:02:40.301Z pushedAt=2026-10-28T07:40:22.692Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Obtains the Tensor format of the specified input port.

## Prototype

```c++
graphStatus GetInputDesc(const int32_t index, TensorDesc &tensor_desc) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |
| tensor_desc | Output | Obtained tensor format. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
