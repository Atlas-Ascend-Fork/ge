# AddGraphInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:51:28.425Z pushedAt=2026-10-28T07:40:22.410Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Adds a graph input at the specified position.

## Prototype

```c++
EsCTensorHolder *AddGraphInput(int32_t index, const ge::char_t *name = nullptr, const ge::char_t *type = nullptr, C_DataType data_type = C_DT_FLOAT, C_Format format = C_FORMAT_ND, const int64_t *dims = nullptr, int64_t dim_num = 0)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index position. |
| name | Input | Input name, optional. |
| type | Input | Input type, optional. |
| data_type | Input | Data type, defaulting to C_DT_FLOAT. |
| format | Input | Data format, defaulting to C_FORMAT_ND. |
| dims | Input | Dimension array, optional. |
| dim_num | Input | Number of dimensions, defaulting to 0. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Tensor holder pointer. |

## Constraints

None
