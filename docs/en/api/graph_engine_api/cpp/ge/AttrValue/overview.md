# Overview

<!-- md-trans-meta sourceCommit=26e2d2d9b2f8a6091084defeb9bea2abf4005f10 translatedAt=2026-11-17T02:57:16.003Z pushedAt=2026-11-18T02:41:08.910Z nl=2 -->

Used to store attribute values.

## Header File to Include

```c++
#include <graph/attr_value.h>
```

## Public Member Functions

```c++
AttrValue()
~AttrValue() = default
AttrType GetValueType() const
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
graphStatus GetNamedAttrValue(const AscendString &attr_name, AttrValue &attr_value) const
graphStatus GetNamedAttrNames(std::vector<AscendString> &attr_names) const
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
