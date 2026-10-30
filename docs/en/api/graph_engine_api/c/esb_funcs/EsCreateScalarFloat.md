# EsCreateScalarFloat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:18:21.585Z pushedAt=2026-10-28T13:21:25.553Z nl=2 -->

## Product Support

Full chip support.

## Header Files/Library Files

- Header file: \#include <ge/esb\_funcs.h\>
- Library files: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a scalar constant of the float type.

## Prototype

```c
EsCTensorHolder *EsCreateScalarFloat(EsCGraphBuilder *graph, float value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Scalar value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the pointer to the created Tensor holder on success, or nullptr on failure. |

## Constraints

None
