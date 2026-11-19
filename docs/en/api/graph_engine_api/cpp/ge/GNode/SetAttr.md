# SetAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:09:24.985Z pushedAt=2026-10-28T07:40:22.717Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Sets the attribute value of a **Node** attribute.

A **Node** can contain multiple attributes. After the value is set for the first time, the type of the operator attribute value is fixed. The types of operator attribute values include:

- Integer: accepts integer values of the int64\_t, uint32\_t, and int32\_t types.

    When setting the attribute value using SetAttr\(const **AscendString**& name, int64\_t &attrValue\), and retrieving the value using GetAttr\(const **AscendString**& name, int32\_t &attrValue\), GetAttr\(const **AscendString**& name, uint32\_t &attrValue\), the user must ensure that the integer data is not truncated. Similarly, when int32\_t and uint32\_t are mixed, ensure that no truncation occurs.

- Integer list: accepts integer list data represented by std::vector<int64\_t\>, std::vector<int32\_t\>, and std::vector<uint32\_t\>.
- Float: **float**
- Float list: std::vector<float\>
- String: AscendString
- String list: std::vector<AscendString\>
- Boolean: bool
- Boolean list: std::vector<bool\>
- Tensor: Tensor
- Tensor list: std::vector<Tensor\>
- OpBytes: Byte array, vector<uint8\_t\>
- AttrValue type
- Integer 2D list type: std::vector<std::vector<int64\_t\>
- DataType list type: vector\<ge::DataType\>
- **DataType** type: **DataType**

## Prototype

```c++
graphStatus SetAttr(const AscendString &name, int64_t &attr_value) const
graphStatus SetAttr(const AscendString &name, int32_t &attr_value) const
graphStatus SetAttr(const AscendString &name, uint32_t &attr_value) const
graphStatus SetAttr(const AscendString &name, float32_t &attr_value) const
graphStatus SetAttr(const AscendString &name, AscendString &attr_value) const
graphStatus SetAttr(const AscendString &name, bool &attr_value) const
graphStatus SetAttr(const AscendString &name, Tensor &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const
graphStatus SetAttr(const AscendString &name, std::vector<bool> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const
graphStatus SetAttr(const AscendString &name, OpBytes &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const
graphStatus SetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const
graphStatus SetAttr(const AscendString &name, ge::DataType &attr_value) const
graphStatus SetAttr(const AscendString &name, AttrValue &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. For the attribute names that can be set, see [attribute name list](../attribute_name_list.md). |
| attr_value | Input | Attribute value of the integer type represented by **int64_t** to be set. |
| attr_value | Input | Attribute value of the integer type represented by **int32_t** to be set. |
| attr_value | Input | Attribute value of the integer type represented by **uint32_t** to be set. |
| attr_value | Input | Attribute value of the floating-point type **float** to be set. |
| attr_value | Input | Attribute value of the string type **AscendString** to be set. |
| attr_value | Input | Attribute value of the Boolean type **bool** to be set. |
| attr_value | Input | Attribute value of the **Tensor** type to be set. |
| attr_value | Input | Attribute value of the integer list type represented by **vector<int64_t>** to be set. |
| attr_value | Input | Attribute value of the integer list type represented by **vector<int32_t>** to be set. |
| attr_value | Input | Attribute value of the integer list type represented by **vector<uint32_t>** to be set. |
| attr_value | Input | Attribute value of the floating-point list type vector\<float> to be set. |
| attr_value | Input | Attribute value of the string list type vector\<ge::AscendString> to be set. |
| attr_value | Input | Attribute value of the Boolean list type vector\<bool> to be set. |
| attr_value | Input | Attribute value of the Tensor list type vector\<Tensor> to be set. |
| attr_value | Input | Attribute value of the **Bytes** type, that is, the byte array type, to be set. **OpBytes** is **vector<uint8_t>**. |
| attr_value | Input | Attribute value of the integer two-dimensional list type represented by **vector<vector<int64_t>>** to be set. |
| attr_value | Input | Attribute value of the DataType list type represented by vector\<ge::DataType> to be set. |
| attr_value | Input | Attribute value of the **DataType** type to be set. |
| attr_value | Input | Attribute value of the **AttrValue** type to be set. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
gNode node;
node.SetAttr(_op_exec_never_timeout,true);
```
