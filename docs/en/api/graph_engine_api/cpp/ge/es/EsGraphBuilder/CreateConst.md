# CreateConst

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:01:21.779Z pushedAt=2026-10-28T07:40:22.449Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a **Const** operator.

## Prototype

- Creates a **Const** operator of the int64 type.

    ```c++
    EsTensorHolder CreateConst(const std::vector<int64_t> &value, const std::vector<int64_t> dims)
    ```

- Creates a **Const** operator of the int32 type.

    ```c++
    EsTensorHolder CreateConst(const std::vector<int32_t> &value, const std::vector<int64_t> dims)
    ```

- Creates a **Const** operator of the uint64 type.

    ```c++
    EsTensorHolder CreateConst(const std::vector<uint64_t> &value, const std::vector<int64_t> dims)
    ```

- Creates a **Const** operator of the uint32 type.

    ```c++
    EsTensorHolder CreateConst(const std::vector<uint32_t> &value, const std::vector<int64_t> dims)
    ```

- Creates a **Const** operator of the float type.

    ```c++
    EsTensorHolder CreateConst(const std::vector<float> &value, const std::vector<int64_t> dims)
    ```

- Creates a **Const** operator with the specified type, format, and dimensions (generic template method; this prototype will be deprecated in the future, and [CreateConstV2](CreateConstV2.md) is recommended).

    ```c++
    template <typename T>
    EsTensorHolder CreateConst(const std::vector<T> &value, const std::vector<int64_t> &dims, ge::DataType dt,ge::Format format = FORMAT_ND)
    ```

    The CreateConst generic template method internally uses the IrDefOutputs/IrDefAttrs APIs. The IR definition structure contains a std::string field, which does not guarantee compatibility across different C++ ABI configurations. This API is retained for compatibility with older software packages.

    When ABI safety is required, use [CreateConstV2](CreateConstV2.md). CreateConstV2 requires the runtime software package to support the CompliantNodeBuilder V2 API.

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
| - | EsTensorHolder | Returns the tensor holder operator of the created **Const**. Returns an invalid **EsTensorHolder** on failure. |

## Constraints

None

## Examples

- Create a **Const** operator of the int64 type:

    ```c++
    EsGraphBuilder builder("test_graph");
    std::vector<int64_t> dims = {3};
    std::vector<int64_t> vec64 = {1, 2, 3};
    auto c1 = builder.CreateConst(vec64, dims);
    ```

- Create a **Const** operator with the specified type, format, and dimensions (generic template method):

    ```c++
    EsGraphBuilder builder("test_graph");
    std::vector<float> vecf = {1.1, 2.0, 3.2, 4.4};
    std::vector<int64_t> dims = {4};
    auto c1 = builder.CreateConst<float>(vecf, dims, ge::DT_FLOAT);
    ```
