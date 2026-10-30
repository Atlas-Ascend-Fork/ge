# EsCreateEsCTensorFromFile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:16:47.938Z pushedAt=2026-10-28T13:21:08.587Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Used for C users to create an EsCTensor from a binary file.

## Prototype

```c
EsCTensor *EsCreateEsCTensorFromFile(const char *data_file_path, const int64_t *dim, int64_t dim_num,C_DataType data_type, C_Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data_file_path | Input | Path to the tensor binary data file. |
| dim | Input | Pointer to the tensor dimension array. |
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
int64_t dims[] = {3};
auto es_tensor =
    EsCreateEsCTensorFromFile(file_path.c_str(), dims, 1, C_DT_INT64, C_FORMAT_ALL);
```
