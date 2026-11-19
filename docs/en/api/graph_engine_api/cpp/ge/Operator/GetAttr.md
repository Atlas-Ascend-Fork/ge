# GetAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:18:53.609Z pushedAt=2026-10-28T07:42:19.669Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the corresponding attribute value by attribute name.

## Prototype

> [!NOTE]
> The APIs whose data type is string will be deprecated in later versions. You are advised to use the APIs whose data type is not string.

```c++
graphStatus GetAttr(const std::string &name, int64_t &attr_value) const
graphStatus GetAttr(const std::string &name, int32_t &attr_value) const
graphStatus GetAttr(const std::string &name, uint32_t &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<int64_t> &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<int32_t> &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<uint32_t> &attr_value) const
graphStatus GetAttr(const std::string &name, float32_t &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<float32_t> &attr_value) const
graphStatus GetAttr(const std::string &name, AttrValue &attr_value) const
graphStatus GetAttr(const std::string &name, std::string &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<std::string> &attr_value) const
graphStatus GetAttr(const std::string &name, bool &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<bool> &attr_value) const
graphStatus GetAttr(const std::string &name, Tensor &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<Tensor> &attr_value) const
graphStatus GetAttr(const std::string &name, OpBytes &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<std::vector<int64_t>> &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<ge::DataType> &attr_value) const
graphStatus GetAttr(const std::string &name, ge::DataType &attr_value) const
graphStatus GetAttr(const std::string &name, ge::NamedAttrs &attr_value) const
graphStatus GetAttr(const std::string &name, std::vector<ge::NamedAttrs> &attr_value) const
graphStatus GetAttr(const char_t *name, int64_t &attr_value) const
graphStatus GetAttr(const char_t *name, int32_t &attr_value) const
graphStatus GetAttr(const char_t *name, uint32_t &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<int64_t> &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<int32_t> &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<uint32_t> &attr_value) const
graphStatus GetAttr(const char_t *name, float32_t &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<float32_t> &attr_value) const
graphStatus GetAttr(const char_t *name, AttrValue &attr_value) const
graphStatus GetAttr(const char_t *name, AscendString &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<AscendString> &attr_values) const
graphStatus GetAttr(const char_t *name, bool &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<bool> &attr_value) const
graphStatus GetAttr(const char_t *name, Tensor &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<Tensor> &attr_value) const
graphStatus GetAttr(const char_t *name, OpBytes &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<std::vector<int64_t>> &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<ge::DataType> &attr_value) const
graphStatus GetAttr(const char_t *name, ge::DataType &attr_value) const
graphStatus GetAttr(const char_t *name, ge::NamedAttrs &attr_value) const
graphStatus GetAttr(const char_t *name, std::vector<ge::NamedAttrs> &attr_value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| attr_value | Output | Returned integer attribute value represented by int64_t. |
| attr_value | Output | Returned integer attribute value represented by int32_t. |
| attr_value | Output | Returned integer attribute value represented by uint32_t. |
| attr_value | Output | Returned integer list type attribute value represented by vector<int64_t>. |
| attr_value | Output | Returned integer list type attribute value represented by vector<int32_t>. |
| attr_value | Output | Returned integer list type attribute value represented by vector<uint32_t>. |
| attr_value | Output | Returned floating-point attribute value. |
| attr_value | Output | Returned floating-point list type attribute value. |
| attr_value | Output | Returned attribute value of the AttrValue type. |
| attr_value | Output | Returned Boolean attribute value. |
| attr_value | Output | Returned Boolean list type attribute value. |
| attr_value | Output | Returned string attribute value. |
| attr_value | Output | Returned string list type attribute value. |
| attr_value | Output | Returned attribute value of the Tensor type. |
| attr_value | Output | Returned Tensor list type attribute value. |
| attr_value | Output | Returned Bytes, that is, the attribute value of the byte array type. OpBytes is vector<uint8_t>. |
| attr_value | Output | Returned attribute value of the quantized data. |
| attr_value | Output | Returned integer two-dimensional list type attribute value represented by vector<vector<int64_t>>. |
| attr_value | Output | Returned DataType list type attribute value represented by vector\<ge::DataType>. |
| attr_value | Output | Returned attribute value of the DataType type. |
| attr_value | Output | Returned NamedAttrs list type attribute value represented by vector\<ge::NamedAttrs>. |
| attr_value | Output | Returned attribute value of the NamedAttrs type. |

## Return Value

graphStatus type: If the corresponding name is found, returns GRAPH\_SUCCESS; otherwise, returns GRAPH\_FAILED.

## Constraints

None
