# InferShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:58:39.542Z pushedAt=2026-10-28T07:40:22.678Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_utils.h\>
- Library file: libge\_compiler.so

## Description

Given the input shape, performs shape inference on the entire **Graph**.

This API only performs shape inference and does not apply any other optimization to the graph (such as constant folding or dead edge elimination).

## Prototype

```c++
static Status InferShape(const Graph &graph, const std::vector<Shape> &input_shape)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph to be inferred. |
| input_shape | Input | Shape corresponding to the graph input. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: Inference succeeded.<br>**FAILED**: Inference failed. |

## Constraints

None
