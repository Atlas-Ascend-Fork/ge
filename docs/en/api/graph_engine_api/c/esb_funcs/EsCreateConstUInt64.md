# EsCreateConstUInt64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:16:04.530Z pushedAt=2026-10-28T13:20:55.959Z nl=2 -->

## Product Support

All chips supported.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a Const node of the uint64 type.

## Prototype

```c
EsCTensorHolder *EsCreateConstUInt64(EsCGraphBuilder *graph, const uint64_t *value, const int64_t *dims, int64_t dim_num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Tensor data pointer. |
| dims | Input | Tensor dimension array pointer. |
| dim_num | Input | Number of dimensions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success; otherwise, returns nullptr. |

## Constraints

None
