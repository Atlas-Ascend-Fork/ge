# EsCreateVectorUInt32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:21:51.805Z pushedAt=2026-10-28T13:22:57.426Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so and libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a vector constant of the uint32 type.

## Prototype

```c
EsCTensorHolder *EsCreateVectorUInt32(EsCGraphBuilder *graph, const uint32_t *value, int64_t dim)
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
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success; otherwise, returns nullptr. |

## Constraints

None
