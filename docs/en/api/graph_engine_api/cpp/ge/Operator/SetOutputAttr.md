# SetOutputAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:29:01.741Z pushedAt=2026-10-28T07:42:19.704Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the attribute value of the operator's output tensor attribute.

An operator can have multiple attributes. After the value is set for the first time, the type of the operator's attribute value is fixed. The types of operator attribute values include:

- Integer: accepts integer values of the int64\_t, uint32\_t, and int32\_t types

    Taking int64\_t as an example, use:

    SetOutputAttr\(const char\_t \*dst\_name, const char\_t \*name, int64\_t attr\_value\);

    SetOutputAttr\(const int32\_t index, const char\_t \*name, int64\_t attr\_value\);

    To set the attribute value, use:

    GetOutputAttr\(const int32\_t index, const char\_t \*name, int64\_t &attr\_value\) const;

    GetOutputAttr\(const char\_t \*dst\_name, const char\_t \*name, int64\_t &attr\_value\) const

    When retrieving values, you must ensure that integer data is not truncated. Similarly, when int32\_t and uint32\_t are used together, ensure that no truncation occurs.

- Integer list: accepts integer list data represented by std::vector<int64\_t\>, std::vector<int32\_t\>, std::vector<uint32\_t\>, and std::initializer\_list<int64\_t\>&&
- Floating-point: float32\_t
- Floating-point list: std::vector<float32\_t\>
- String: string
- Boolean: bool
- Boolean list: std::vector<bool\>

## Prototype

```c++
Operator &SetOutputAttr(const int32_t index, const char_t *name, const char_t *attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const char_t *attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const AscendString &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const AscendString &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, int64_t attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, int64_t attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, int32_t attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, int32_t attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, uint32_t attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, uint32_t attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, bool attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, bool attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, float32_t attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, float32_t attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<AscendString> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<AscendString> &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<int64_t> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<int64_t> &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<int32_t> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<int32_t> &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<uint32_t> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<uint32_t> &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<bool> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<bool> &attr_value)
Operator &SetOutputAttr(const int32_t index, const char_t *name, const std::vector<float32_t> &attr_value)
Operator &SetOutputAttr(const char_t *dst_name, const char_t *name, const std::vector<float32_t> &attr_value)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| index | Input | Output index. |
| dst_name | Input | Output edge name. |
| attr_value | Input | Integer attribute value to set, represented by int64_t. |
| attr_value | Input | Integer attribute value to set, represented by int32_t. |
| attr_value | Input | Integer attribute value to set, represented by uint32_t. |
| attr_value | Input | Integer list attribute value to set, represented by vector<int64_t>. |
| attr_value | Input | Integer list attribute value to set, represented by vector<int32_t>. |
| attr_value | Input | Integer list attribute value to set, represented by vector<uint32_t>. |
| attr_value | Input | Floating-point attribute value to set. |
| attr_value | Input | Floating-point list attribute value to set. |
| attr_value | Input | Boolean attribute value to set. |
| attr_value | Input | Boolean list attribute value to set. |
| attr_value | Input | String attribute value to set. |
| attr_value | Input | String list attribute value to set. |

## Return Value

[Operator](Operator.md) object itself.

## Constraints

None
