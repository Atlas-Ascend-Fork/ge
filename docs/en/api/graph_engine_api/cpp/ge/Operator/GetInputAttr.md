# GetInputAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:20:03.409Z pushedAt=2026-10-28T07:42:19.673Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the attribute value of the operator input tensor by attribute name.

## Prototype

```c++
graphStatus GetInputAttr(const int32_t index, const char_t *name, AscendString &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, AscendString &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, int64_t &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, int64_t &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, int32_t &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, int32_t &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, uint32_t &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, uint32_t &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, bool &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, bool &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, float32_t &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, float32_t &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<AscendString> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<AscendString> &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<int64_t> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<int64_t> &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<int32_t> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<int32_t> &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<uint32_t> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<uint32_t> &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<bool> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<bool> &attr_value) const
graphStatus GetInputAttr(const int32_t index, const char_t *name, std::vector<float32_t> &attr_value) const
graphStatus GetInputAttr(const char_t *dst_name, const char_t *name, std::vector<float32_t> &attr_value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| index | Input | Input index. |
| dst_name | Input | Input edge name. |
| attr_value | Output | Obtained integer attribute value represented by int64_t. |
| attr_value | Output | Obtained integer attribute value represented by int32_t. |
| attr_value | Output | Obtained integer attribute value represented by uint32_t. |
| attr_value | Output | Obtained integer list attribute value represented by vector<int64_t>. |
| attr_value | Output | Obtained integer list attribute value represented by vector<int32_t>. |
| attr_value | Output | Obtained integer list attribute value represented by vector<uint32_t>. |
| attr_value | Output | Obtained floating-point attribute value. |
| attr_value | Output | Obtained floating-point list attribute value. |
| attr_value | Output | Obtained Boolean attribute value. |
| attr_value | Output | Obtained Boolean list attribute value. |
| attr_value | Output | Obtained string attribute value. |
| attr_value | Output | Obtained string list attribute value. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS if the corresponding attribute is found; otherwise, returns GRAPH\_FAILED.

## Constraints

None
