# CreateBoolTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:00:38.205Z pushedAt=2026-10-28T07:40:22.447Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a tensor with the bool data type and the specified tensor shape.

## Prototype

```c++
std::unique_ptr<Tensor> CreateBoolTensor(const std::vector<bool> &value, const std::vector<int64_t> &dims, Format format = FORMAT_ND)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Tensor data vector. |
| dims | Input | Tensor dimension vector. |
| format | Input | Tensor format, defaulting to FORMAT_ND. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<Tensor> | Returns the smart pointer to the created tensor, or nullptr on failure. |

## Constraints

None

## Examples

```c++
EsGraphBuilder builder("graph_name");
std::vector<bool> data = {true, false, true, false};
std::vector<int64_t> dims = {4};
auto tensor = builder.CreateBoolTensor(data,dims);
```
