# Overview

<!-- md-trans-meta sourceCommit=8e1d9780ef8207a0aa36f04312a187ddccd776b9 translatedAt=2026-10-16T14:08:49.052Z pushedAt=2026-10-28T07:40:22.712Z nl=2 -->

Node data used by users to construct a **Graph**.

## Header File to Include

```c++
#include <graph/gnode.h>
```

## Public Member Functions

```c++
GNode()
~ GNode() = default
graphStatus GetAttr(const AscendString &name, int64_t &attr_value) const
graphStatus GetAttr(const AscendString &name, int32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, uint32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, float32_t &attr_value) const
graphStatus GetAttr(const AscendString &name, AscendString &attr_value) const
graphStatus GetAttr(const AscendString &name, bool &attr_value) const
graphStatus GetAttr(const AscendString &name, Tensor &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const
graphStatus GetAttr(const AscendString &name, std::vector<bool> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const
graphStatus GetAttr(const AscendString &name, OpBytes &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const
graphStatus GetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const
graphStatus GetAttr(const AscendString &name, ge::DataType &attr_value) const
graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const
graphStatus GetALLSubgraphs(std::vector<GraphPtr> &graph_list) const
graphStatus GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes)
graphStatus GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes)
std::vector<GNodePtr> GetInControlNodes() const
std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const
graphStatus GetInputAttr(const AscendString &name, uint32_t input_index, AttrValue &attr_value) const
graphStatus GetInputConstData(const int32_t index, Tensor &data) const
graphStatus GetInputDesc(const int32_t index, TensorDesc &tensor_desc) const
graphStatus GetInputIndexByName(const AscendString &name, int32_t &index)
size_t GetInputsSize() const
graphStatus GetName(ge::AscendString &name) const
std::vector<GNodePtr> GetOutControlNodes() const
std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const
graphStatus GetOutputAttr(const AscendString &name, uint32_t output_index, AttrValue &attr_value) const;
graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index)
graphStatus GetOutputDesc(const int32_t index, TensorDesc &tensor_desc) const
size_t GetOutputsSize() const
graphStatus GetSubgraph(uint32_t index, GraphPtr &graph) const
graphStatus GetType(ge::AscendString &type) const
bool HasAttr(const AscendString &name)
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
graphStatus SetInputAttr(const AscendString &name, uint32_t input_index, const AttrValue &attr_value)
graphStatus SetOutputAttr(const AscendString &name, uint32_t output_index, const AttrValue &attr_value)
graphStatus UpdateInputDesc(const int32_t index, const TensorDesc &tensor_desc)
graphStatus UpdateOutputDesc(const int32_t index, const TensorDesc &tensor_desc)
```
