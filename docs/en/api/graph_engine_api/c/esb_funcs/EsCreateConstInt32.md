# EsCreateConstInt32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:32:40.778Z pushedAt=2026-11-18T06:40:12.850Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

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
| dims | Input | Pointer to the tensor dimension array. |
| dim_num | Input | Number of dimensions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success; otherwise, returns nullptr. |

## Constraints

None
