# SetAttrValue

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T12:16:51.467Z pushedAt=2026-10-28T07:40:22.266Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph\_base.so

## Description

Sets the value of an attribute. This is a generic class that carries the specific attribute type.

## Prototype

```c++
graphStatus SetAttrValue(const int64_t &attr_value) const
graphStatus SetAttrValue(const float32_t &attr_value) const
graphStatus SetAttrValue(const AscendString &attr_value) const
graphStatus SetAttrValue(const bool &attr_value) const
graphStatus SetAttrValue(const Tensor &attr_value) const
graphStatus SetAttrValue(const std::vector<int64_t> &attr_value) const
graphStatus SetAttrValue(const std::vector<float32_t> &attr_value) const
graphStatus SetAttrValue(const std::vector<AscendString> &attr_values) const
graphStatus SetAttrValue(const std::vector<bool> &attr_value) const
graphStatus SetAttrValue(const std::vector<Tensor> &attr_value) const
graphStatus SetAttrValue(const std::vector<std::vector<int64_t>> &attr_value) const
graphStatus SetAttrValue(const std::vector<ge::DataType> &attr_value) const
graphStatus SetAttrValue(const ge::DataType &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_value | Input | The specific attribute value. |

## Return Value

**graphStatus** type: Returns **GRAPH\_SUCCESS** on success; otherwise, returns **GRAPH\_FAILED**.

## Constraints

None
