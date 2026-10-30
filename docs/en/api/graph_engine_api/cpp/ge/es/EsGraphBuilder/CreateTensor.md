# CreateTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:03:48.481Z pushedAt=2026-10-28T07:40:22.457Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a tensor with the specified data type and tensor shape.

## Prototype

```c++
template <typename T>
std::unique_ptr<Tensor> CreateTensor(const std::vector<T> &value, const std::vector<int64_t> &dims, DataType dt, Format format = FORMAT_ND);

template <typename T>
std::unique_ptr<Tensor> CreateTensor(const T *value, const int64_t *dims, int64_t dim_num, DataType dt, Format format = FORMAT_ND);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| T | Input | Tensor data type. |
| value | Input | Tensor data pointer. |
| dims | Input | Tensor dimension array pointer. |
| dim_num | Input | Number of tensor dimensions. |
| dt | Input | Data type of the tensor. |
| format | Input | Tensor format, defaulting to FORMAT_ND. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<Tensor> | Returns the smart pointer to the created tensor, or nullptr on failure. |

## Constraints

None

## Examples

Template 1:

```c++
EsGraphBuilder builder("graph_name");
std::vector<float> data = {1.0f, 2.0f, 3.0f};
std::vector<int64_t> dims = {3};
auto tensor = builder.CreateTensor<float>(data, dims, ge::DT_FLOAT);
```

Template 2:

```c++
std::vector<float> data = {1.0f, 2.0f, 3.0f};
std::vector<int64_t> dims = {3};
auto tensor = ge::es::CreateTensor<float>(data.data(), dims.data(), dims.size(), ge::DT_FLOAT);
```
