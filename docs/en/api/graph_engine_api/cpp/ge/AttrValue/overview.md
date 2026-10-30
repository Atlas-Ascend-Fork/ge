# Overview

<!-- md-trans-meta sourceCommit=8e1d9780ef8207a0aa36f04312a187ddccd776b9 translatedAt=2026-10-16T12:16:46.832Z pushedAt=2026-10-28T07:40:22.263Z nl=2 -->

Used to store attribute values.

## Header File to Include

```c++
#include <graph/attr_value.h>
```

## Public Member Functions

```c++
AttrValue()
~AttrValue() = default
template<typename T, typename DT>
static T CreateFrom(DT &&val)
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
template<typename T, typename DT>
graphStatus GetValue(DT &val) const
graphStatus GetValue(AscendString &val)
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
