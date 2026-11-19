# EsCreateVectorFloat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:38:00.496Z pushedAt=2026-11-18T06:41:06.420Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates a vector constant of the float type.

## Prototype

```c
EsCTensorHolder *EsCreateVectorFloat(EsCGraphBuilder *graph, const float *value, int64_t dim)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Vector data pointer. |
| dim | Input | Vector length. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created Tensor holder pointer on success, or nullptr on failure. |

## Constraints

None
