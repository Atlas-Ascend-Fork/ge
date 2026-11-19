# InferShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:31:41.978Z pushedAt=2026-10-28T07:42:19.855Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

The shape inference function obtains the input tensor shape information through the shape inference context, infers and sets the output tensor shape, and is used for shape inference during the graph compilation phase.

## Prototype

```c++
virtual graphStatus InferShape(gert::InferShapeContext *ctx) = 0
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Shape inference context, through which you can obtain the input tensor shape and set the output tensor shape. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
