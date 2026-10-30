# InferDataType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:31:38.691Z pushedAt=2026-10-28T07:42:19.851Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

A data type inference function that obtains the input tensor data type information through the data type inference context, and derives and sets the output tensor data type for type derivation during the graph compilation phase.

## Prototype

```c++
virtual graphStatus InferDataType(gert::InferDataTypeContext *ctx) = 0
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Data type inference context, through which you can obtain the input tensor data type and set the output tensor data type. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
