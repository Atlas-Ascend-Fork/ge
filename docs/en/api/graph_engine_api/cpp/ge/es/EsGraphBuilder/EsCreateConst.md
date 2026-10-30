# EsCreateConst

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:06:32.027Z pushedAt=2026-10-28T07:40:22.467Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a **Const** operator of the specified type (raw pointer version).

## Prototype

```c++
template <typename T>
EsCTensorHolder *EsCreateConst(EsCGraphBuilder *graph, const T *value, const int64_t *dims, int64_t dim_num, ge::DataType dt, ge::Format format = FORMAT_ND)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| T | Input | Tensor data type. |
| graph | Input | Graph builder pointer. |
| value | Input | Tensor data pointer. |
| dims | Input | Pointer to the tensor dimension array. |
| dim_num | Input | Number of dimensions. |
| dt | Input | Data type of the tensor. |
| format | Input | Tensor format, defaulting to FORMAT_ND. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the tensor holder operator of the created **Const**, or **nullptr** on failure. |

## Constraints

**EsCreateConst** internally uses the **IrDefOutputs**/**IrDefAttrs** APIs. The IR definition struct contains a **std::string** field, which does not guarantee compatibility across different C++ ABI configurations. This API is retained for compatibility with legacy GE packages.

When ABI safety is required, use [EsCreateConstV2](EsCreateConstV2.md). **EsCreateConstV2** requires the runtime software package to support the **CompliantNodeBuilder V2** API.

## Examples

```c++
EsCGraphBuilder *graph = EsCreateGraphBuilder("graph_name");
std::vector<int64_t> data = {1, 2, 3};
std::vector<int64_t> dims = {3};
auto const_tensor = ge::es::EsCreateConst<int64_t>(graph, data.data(), dims.data(), dims.size(), ge::DT_INT64);
```
