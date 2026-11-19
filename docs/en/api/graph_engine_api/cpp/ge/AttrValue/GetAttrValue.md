# GetAttrValue

<!-- md-trans-meta sourceCommit=26e2d2d9b2f8a6091084defeb9bea2abf4005f10 translatedAt=2026-11-17T02:55:50.395Z pushedAt=2026-11-18T02:41:08.903Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph.so

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
