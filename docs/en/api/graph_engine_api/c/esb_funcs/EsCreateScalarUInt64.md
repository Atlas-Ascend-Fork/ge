# EsCreateScalarUInt64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:37:16.087Z pushedAt=2026-11-18T06:41:00.004Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates a uint64 scalar constant.

## Prototype

```c
EsCTensorHolder *EsCreateScalarUInt64(EsCGraphBuilder *graph, uint64_t value)
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
