# EsCreateEsCTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:16:43.072Z pushedAt=2026-10-28T13:21:05.113Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

This API is used to create an EsCTensor for C users.

## Prototype

```c
EsCTensor *EsCreateEsCTensor(const void *data, const int64_t *dim, int64_t dim_num, C_DataType data_type,C_Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input | Tensor data pointer. |
| dim | Input | Tensor dimension array pointer. |
| dim_num | Input | Number of tensor dimensions. |
| data_type | Input | DataType enumeration value of the tensor. |
| format | Input | Tensor format. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensor * | Anonymous pointer to the tensor, or nullptr on failure. |

## Constraints

None

## Examples

```c
std::vector<float> data = {5.0, 6.1};
int64_t dims[] = {2};
auto tmp = data.data();
auto es_tensor = EsCreateEsCTensor(tmp, dims, 1, C_DT_FLOAT, C_FORMAT_ND);
```
