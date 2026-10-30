# SetAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:27:40.270Z pushedAt=2026-10-28T07:42:19.701Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the attribute value of an operator attribute.

An operator can have multiple attributes. After the initial value is set, the type of the operator attribute value is fixed. The types of operator attribute values include:

- Integer: accepts integer values of the int64\_t, uint32\_t, and int32\_t types

    When setting the attribute value using SetAttr\(const string& name, int64\_t attrValue\), and retrieving the value using GetAttr\(const string& name, int32\_t& attrValue\) or GetAttr\(const string& name, uint32\_t& attrValue\), you must ensure that the integer data is not truncated. Similarly, when int32\_t and uint32\_t are used together, ensure that no truncation occurs.

- Integer list: accepts integer list data represented by std::vector<int64\_t\>, std::vector<int32\_t\>, std::vector<uint32\_t\>, and std::initializer\_list<int64\_t\>&&
- Floating-point number: float
- Floating-point number list: std::vector<float\>
- String: string
- String list: std::vector\<std::string\>
- Boolean: bool
- Boolean list: std::vector<bool\>
- Tensor: Tensor
- Tensor list: std::vector<Tensor\>
- Bytes: byte array. SetAttr accepts a byte array represented by OpBytes (that is, vector<uint8\_t\>) and (const uint8\_t\* data, size\_t size)
- AttrValue type
- 2D integer list type: std::vector<std::vector\<int64\_t\>\>
- DataType list type: std::vector\<ge::DataType\>
- DataType type: ge::DataType
- NamedAttrs type: ge::NamedAttrs
- NamedAttrs list type: std::vector\<ge::NamedAttrs\>

## Prototype

> [!NOTE]
> The APIs whose data type is string will be deprecated in later versions. Use the APIs whose data type is not string instead.

```c++
Operator &SetAttr(const std::string &name, int64_t attr_value)
Operator &SetAttr(const std::string &name, int32_t attr_value)
Operator &SetAttr(const std::string &name, uint32_t attr_value)
Operator &SetAttr(const std::string &name, const std::vector<int64_t> &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<int32_t> &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<uint32_t> &attr_value)
Operator &SetAttr(const std::string &name, std::initializer_list<int64_t> &&attr_value)
Operator &SetAttr(const std::string &name, float32_t attr_value)
Operator &SetAttr(const std::string &name, const std::vector<float32_t> &attr_value)
Operator &SetAttr(const std::string &name, AttrValue &&attr_value)
Operator &SetAttr(const std::string &name, const std::string &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<std::string> &attr_value)
Operator &SetAttr(const std::string &name, bool attr_value)
Operator &SetAttr(const std::string &name, const std::vector<bool> &attr_value)
Operator &SetAttr(const std::string &name, const Tensor &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<Tensor> &attr_value)
Operator &SetAttr(const std::string &name, const OpBytes &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<std::vector<int64_t>> &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<ge::DataType> &attr_value)
Operator &SetAttr(const std::string &name, const ge::DataType &attr_value)
Operator &SetAttr(const std::string &name, const ge::NamedAttrs &attr_value)
Operator &SetAttr(const std::string &name, const std::vector<ge::NamedAttrs> &attr_value)
Operator &SetAttr(const char_t *name, int64_t attr_value)
Operator &SetAttr(const char_t *name, int32_t attr_value)
Operator &SetAttr(const char_t *name, uint32_t attr_value)
Operator &SetAttr(const char_t *name, const std::vector<int64_t> &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<int32_t> &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<uint32_t> &attr_value)
Operator &SetAttr(const char_t *name, std::initializer_list<int64_t> &&attr_value)
Operator &SetAttr(const char_t *name, float32_t attr_value)
Operator &SetAttr(const char_t *name, const std::vector<float32_t> &attr_value)
Operator &SetAttr(const char_t *name, AttrValue &&attr_value)
Operator &SetAttr(const char_t *name, const char_t *attr_value)
Operator &SetAttr(const char_t *name, const AscendString &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<AscendString> &attr_values)
Operator &SetAttr(const char_t *name, bool attr_value)
Operator &SetAttr(const char_t *name, const std::vector<bool> &attr_value)
Operator &SetAttr(const char_t *name, const Tensor &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<Tensor> &attr_value)
Operator &SetAttr(const char_t *name, const OpBytes &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<std::vector<int64_t>> &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<ge::DataType> &attr_value)
Operator &SetAttr(const char_t *name, const ge::DataType &attr_value)
Operator &SetAttr(const char_t *name, const ge::NamedAttrs &attr_value)
Operator &SetAttr(const char_t *name, const std::vector<ge::NamedAttrs> &attr_value)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| attr_value | Input | Integer attribute value to set, represented by int64_t. |
| attr_value | Input | Integer attribute value to set, represented by int32_t. |
| attr_value | Input | Integer attribute value to set, represented by uint32_t. |
| attr_value | Input | Integer list type attribute value to set, represented by vector<int64_t>. |
| attr_value | Input | Integer list type attribute value to set, represented by vector<int32_t>. |
| attr_value | Input | Integer list type attribute value to set, represented by vector<uint32_t>. |
| attr_value | Input | Integer list type attribute value to set, represented by std::initializer_list<int64_t>&&. |
| attr_value | Input | Floating-point attribute value to set. |
| attr_value | Input | Floating-point list type attribute value to set. |
| attr_value | Input | Boolean attribute value to set. |
| attr_value | Input | Boolean list type attribute value to set. |
| attr_value | Input | AttrValue type attribute value to set. |
| attr_value | Input | String type attribute value to set. |
| attr_value | Input | String list type attribute value to set. |
| attr_value | Input | Tensor type attribute value to set. |
| attr_value | Input | Tensor list type attribute value to set. |
| attr_value | Input | Bytes, that is, byte array type attribute value to set. OpBytes is vector<uint8_t>. |
| data | Input | Bytes, that is, byte array type attribute value to set, specifying the start address of the byte stream. |
| size | Input | Bytes, that is, byte array type attribute value to set, specifying the length of the byte stream. |
| attr_value | Input | Quantized data attribute value to set. |
| attr_value | Input | Integer two-dimensional list type attribute value to set, represented by vector<vector<int64_t>>. |
| attr_value | Input | DataType list type attribute value to set, represented by vector\<ge::DataType>. |
| attr_value | Input | DataType type attribute value to set. |
| attr_value | Input | NamedAttrs type attribute value to set. |
| attr_value | Input | NamedAttrs list type attribute value to set, represented by vector\<ge::NamedAttrs>. |

## Return Value

The [Operator](Operator.md) object itself.

## Constraints

None
