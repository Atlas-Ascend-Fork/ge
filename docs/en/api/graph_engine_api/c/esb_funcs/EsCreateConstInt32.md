# EsCreateConstInt32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:14:37.575Z pushedAt=2026-10-28T13:20:20.198Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a Const node of the int32 type.

## Prototype

```c
EsCTensorHolder *EsCreateConstInt32(EsCGraphBuilder *graph, const int32_t *value, const int64_t *dims, int64_t dim_num)
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
| - | EsCTensorHolder * | Returns the pointer to the created tensor holder on success, or nullptr on failure. |

## Constraints

None
