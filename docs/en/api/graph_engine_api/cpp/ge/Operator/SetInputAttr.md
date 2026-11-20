# SetInputAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:28:04.848Z pushedAt=2026-10-28T07:42:19.703Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the attribute value of the operator's input tensor attribute.

An operator can have multiple attributes. After the value is set for the first time, the type of the operator attribute value is fixed. The supported types of operator attribute values include:

- Integer: accepts integer values of the int64\_t, uint32\_t, and int32\_t types

    Taking int64\_t as an example, use:

    SetInputAttr\(const char\_t \*dst\_name, const char\_t \*name, int64\_t attr\_value\);

    SetInputAttr\(const int32\_t index, const char\_t \*name, int64\_t attr\_value\);

    Set the attribute value, using:

    GetInputAttr\(const int32\_t index, const char\_t \*name, int64\_t &attr\_value\) const;

    GetInputAttr\(const char\_t \*dst\_name, const char\_t \*name, int64\_t &attr\_value\) const

    When retrieving the value, you must ensure that the integer data is not truncated. Similarly, when int32\_t and uint32\_t are used together, ensure that no truncation occurs.

- Integer list: accepts integer list data represented by std::vector<int64\_t\>, std::vector<int32\_t\>, std::vector<uint32\_t\>, and std::initializer\_list<int64\_t\>&&.
- Floating-point number: float32\_t
- Floating-point number list: std::vector<float32\_t\>
- String: string
- Boolean: bool
- Boolean list: std::vector<bool\>

## Prototype

```c++
Operator &SetInputAttr(const int32_t index, const char_t *name, const char_t *attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const char_t *attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const AscendString &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const AscendString &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, int64_t attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, int64_t attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, int32_t attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, int32_t attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, uint32_t attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, uint32_t attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, bool attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, bool attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, float32_t attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, float32_t attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<AscendString> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<AscendString> &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<int64_t> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<int64_t> &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<int32_t> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<int32_t> &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<uint32_t> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<uint32_t> &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<bool> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<bool> &attr_value)
Operator &SetInputAttr(const int32_t index, const char_t *name, const std::vector<float32_t> &attr_value)
Operator &SetInputAttr(const char_t *dst_name, const char_t *name, const std::vector<float32_t> &attr_value)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| index | Input | Input index. |
| dst_name | Input | Input edge name. |
| attr_value | Input | Integer attribute value represented by int64_t to be set. |
| attr_value | Input | Integer attribute value represented by int32_t to be set. |
| attr_value | Input | Integer attribute value represented by uint32_t to be set. |
| attr_value | Input | Integer list attribute value represented by vector<int64_t> to be set. |
| attr_value | Input | Integer list attribute value represented by vector<int32_t> to be set. |
| attr_value | Input | Integer list attribute value represented by vector<uint32_t> to be set. |
| attr_value | Input | Floating-point attribute value to be set. |
| attr_value | Input | Floating-point list attribute value to be set. |
| attr_value | Input | Boolean attribute value to be set. |
| attr_value | Input | Boolean list attribute value to be set. |
| attr_value | Input | String attribute value to be set. |
| attr_value | Input | String list attribute value to be set. |

## Return Value

The [Operator](Operator.md) object itself.

## Constraints

None
