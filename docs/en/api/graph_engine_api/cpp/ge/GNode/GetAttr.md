# GetAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:00:16.294Z pushedAt=2026-10-28T07:40:22.684Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the value of the specified attribute name.

## Prototype

```c++
graphStatus GetAttr(const AscendString &name, int64_t &attr_value) const
graphStatus GetAttr(const AscendString &name, int32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, uint32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, float32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, AscendString &attr_value) const
graphStatus GetAttr(const AscendString &name, bool &attr_value) const
graphStatus GetAttr(const AscendString &name, Tensor &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const
graphStatus GetAttr(const AscendString &name, std::vector<bool> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const
graphStatus GetAttr(const AscendString &name, OpBytes &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const
graphStatus GetAttr(const AscendString &name, ge::DataType &attr_value) const
graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| attr_value | Output | Returned integer attribute value represented by int64_t. |
| attr_value | Output | Returned integer attribute value represented by int32_t. |
| attr_value | Output | Returned integer attribute value represented by uint32_t. |
| attr_value | Output | Returned floating-point attribute value of type float. |
| attr_value | Output | Returned string attribute value of type AscendString. |
| attr_value | Output | Returned boolean attribute value of type bool. |
| attr_value | Output | Returned attribute value of type Tensor. |
| attr_value | Output | Returned integer list attribute value represented by vector<int64_t>. |
| attr_value | Output | Returned integer list attribute value represented by vector<int32_t>. |
| attr_value | Output | Returned integer list attribute value represented by vector<uint32_t>. |
| attr_value | Output | Returned floating-point list attribute value of type vector\<float>. |
| attr_value | Output | Returned string list attribute value of type vector\<ge::AscendString>. |
| attr_value | Output | Returned boolean list attribute value of type vector\<bool>. |
| attr_value | Output | Returned Tensor list attribute value of type vector\<Tensor>. |
| attr_value | Output | Returned attribute value of type Bytes, that is, a byte array. OpBytes is vector<uint8_t>. |
| attr_value | Output | Returned two-dimensional integer list attribute value represented by vector<vector<int64_t>>. |
| attr_value | Output | Returned DataType list attribute value represented by vector\<ge::DataType>. |
| attr_value | Output | Returned attribute value of type DataType. |
| attr_value | Output | Returned attribute value of type AttrValue. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
