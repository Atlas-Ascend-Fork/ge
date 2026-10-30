# CreateConstV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:01:34.166Z pushedAt=2026-10-28T07:40:22.451Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a Const operator using the ABI-safe API.

## Prototype

```c++
template <typename T>
EsTensorHolder CreateConstV2(const std::vector<T> &value, const std::vector<int64_t> &dims, ge::DataType dt, ge::Format format = FORMAT_ND)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Tensor data vector. |
| dims | Input | Tensor dimension vector. |
| T | Input | Tensor data type. |
| dt | Input | Data type of the tensor. |
| format | Input | Tensor format, defaulting to **FORMAT_ND**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | Returns the tensor holder operator of the created **Const**, or an invalid **EsTensorHolder** on failure. |

## Constraints

This API uses the **CompliantNodeBuilder V2** API to construct the **Const** node, avoiding the risk of passing **std::string** across the C++ ABI in the legacy IR definition struct.

Using this API requires the runtime software package to support the **CompliantNodeBuilder** V2 symbols. If compatibility with older packages that do not include the V2 symbols is required, use [CreateConst](CreateConst.md).

## Examples

```c++
EsGraphBuilder builder("test_graph");
std::vector<float> vecf = {1.1, 2.0, 3.2, 4.4};
std::vector<int64_t> dims = {4};
auto c1 = builder.CreateConstV2<float>(vecf, dims, ge::DT_FLOAT);
```
