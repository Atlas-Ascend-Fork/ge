/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_
#define METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graph/gnode.h"
#include "normal_graph/gnode_impl.h"

namespace ge {
class NodeAdapter;
class Node;

// 计算节点后端：NodeImpl 的计算图实现，持底层 Node 弱引用，承接 GNode 在计算节点模式下的
// 全部操作；节点引用由 NodeAdapter 晚绑定（默认构造即计算节点、空引用）。
// 节点生命周期仍由底层图管理，实现体自 GNode/静态函数集原样搬迁。
class ComputeNodeBackend : public NodeImpl {
 public:
  ComputeNodeBackend() = default;

  graphStatus GetType(AscendString &type) const override;
  graphStatus GetName(AscendString &name) const override;
  std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const override;
  std::vector<GNodePtr> GetControlNodes(const bool input) const override;
  std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const override;
  graphStatus GetInputConstData(const int32_t index, Tensor &data) const override;
  graphStatus GetInputIndexByName(const AscendString &name, int32_t &index) const override;
  graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index) const override;
  graphStatus GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const override;
  graphStatus GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const override;
  size_t GetInputsSize() const override;
  size_t GetOutputsSize() const override;
  graphStatus GetDesc(const int32_t index, TensorDesc &tensor_desc, const bool input) const override;
  graphStatus UpdateDesc(const int32_t index, const TensorDesc &tensor_desc, const bool input) const override;

  // 属性：19+19 逐类型 override 显式声明（模板不能为虚）
  graphStatus GetAttr(const AscendString &name, int64_t &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, int32_t &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, uint32_t &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, float32_t &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, AscendString &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, bool &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, Tensor &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<bool> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, OpBytes &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, ge::DataType &attr_value) const override;
  graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, int64_t &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, int32_t &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, uint32_t &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, float32_t &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, AscendString &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, bool &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, Tensor &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<bool> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, OpBytes &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, ge::DataType &attr_value) const override;
  graphStatus SetAttr(const AscendString &name, AttrValue &attr_value) const override;

  graphStatus GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const override;
  graphStatus SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const override;
  bool HasAttr(const AscendString &name) const override;

  graphStatus GetSubgraph(const uint32_t index, GraphPtr &graph) const override;
  graphStatus GetAllSubgraphs(std::vector<GraphPtr> &graph_list) const override;
  graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const override;
  graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) const override;
  // 端口属性：input 选择输入/输出端口
  graphStatus PortAttr(const AscendString &name, const uint32_t index, AttrValue &attr_value,
                       const bool input) const override;
  graphStatus SetPortAttr(const AscendString &name, const uint32_t index, const AttrValue &attr_value,
                          const bool input) const override;

 private:
  // 晚绑定：Node2GNode/GNode2Node 直接读写 node_ptr_
  friend class NodeAdapter;

  graphStatus GetInputDescInner(const int32_t index, TensorDesc &tensor_desc) const;
  graphStatus GetOutputDescInner(const int32_t index, TensorDesc &tensor_desc) const;
  graphStatus UpdateInputDescInner(const int32_t index, const TensorDesc &tensor_desc) const;
  graphStatus UpdateOutputDescInner(const int32_t index, const TensorDesc &tensor_desc) const;
  graphStatus GetPortAttrValue(const std::shared_ptr<Node> &node_ptr, const AscendString &name, const uint32_t index,
                               AttrValue &attr_value, const bool is_input) const;
  graphStatus SetPortAttrValue(const std::shared_ptr<Node> &node_ptr, const AscendString &name, const uint32_t index,
                               const AttrValue &attr_value, const bool is_input) const;

  std::weak_ptr<Node> node_ptr_;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_
