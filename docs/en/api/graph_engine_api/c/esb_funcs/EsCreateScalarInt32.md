# EsCreateScalarInt32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:36:18.505Z pushedAt=2026-11-18T06:40:50.190Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates an int32 scalar constant.

## Prototype

```c
EsCTensorHolder *EsCreateScalarInt32(EsCGraphBuilder *graph, int32_t value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Scalar value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created Tensor holder pointer on success, or nullptr on failure. |

## Constraints

None
