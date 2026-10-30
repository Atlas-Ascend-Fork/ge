# CreateTensorFromFile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:04:42.530Z pushedAt=2026-10-28T07:40:22.458Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a tensor with the specified data type and tensor shape from a binary file.

## Prototype

```c++
template <typename T>
std::unique_ptr<Tensor> CreateTensorFromFile(const char *data_file_path, const std::vector<int64_t> &dims, DataType dt, Format format = FORMAT_ND);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| T | Input | Tensor data type. |
| data_file_path | Input | Path to the binary file data. |
| dims | Input | Tensor dimension vector. |
| dt | Input | Data type of the tensor. |
| format | Input | Tensor format, defaulting to **FORMAT_ND**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<Tensor> | Returns the smart pointer to the created tensor, or **nullptr** on failure. |

## Constraints

None

## Examples

```c++
std::vector<int64_t> dims = {1};
auto tensor = CreateTensorFromFile<int64_t>(file_path.c_str(), dims, ge::DT_INT64, ge::FORMAT_ALL);
```
