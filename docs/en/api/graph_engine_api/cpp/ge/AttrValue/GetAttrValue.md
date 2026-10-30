# GetAttrValue

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T12:15:18.887Z pushedAt=2026-10-28T07:40:22.258Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph\_base.so

## Description

Obtains the value of an attribute. A generic class that carries a specific attribute type.

## Prototype

```c++
graphStatus GetAttrValue(int64_t &attr_value) const
graphStatus GetAttrValue(float32_t &attr_value) const
graphStatus GetAttrValue(AscendString &attr_value) const
graphStatus GetAttrValue(bool &attr_value) const
graphStatus GetAttrValue(Tensor &attr_value) const
graphStatus GetAttrValue(std::vector<int64_t> &attr_value) const
graphStatus GetAttrValue(std::vector<float32_t> &attr_value) const
graphStatus GetAttrValue(std::vector<AscendString> &attr_values) const
graphStatus GetAttrValue(std::vector<bool> &attr_value) const
graphStatus GetAttrValue(std::vector<Tensor> &attr_value) const
graphStatus GetAttrValue(std::vector<std::vector<int64_t>> &attr_value) const
graphStatus GetAttrValue(std::vector<ge::DataType> &attr_value) const
graphStatus GetAttrValue(ge::DataType &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_value | Output | Specific attribute value. |

## Return Value

**graphStatus** type: Returns **GRAPH_SUCCESS** on success; otherwise, returns **GRAPH_FAILED**.

## Constraints

None
