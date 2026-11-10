/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "graph/gnode.h"
#include "graph/debug/ge_log.h"
#include <map>
#include "graph/fast_graph/execute_graph.h"
#include "normal_graph/execute_node_backend.h"
#include "normal_graph/compute_node_backend.h"
#include "normal_graph/gnode_impl.h"
#include "graph/graph.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_tensor.h"
#include "graph/anchor.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/tensor_adapter.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "common/util/mem_utils.h"
#include "common/checker.h"

// 门面转发宏：保留原属性接口的名称与 impl 判空检查（日志逐条原样），正文转发至计算节点后端
#define NODE_ATTR_GET_IMP(ArgType)                                                      \
  graphStatus GNode::GetAttr(const AscendString &name, ArgType &attr_value) const {     \
    const char_t *const ascend_name = name.GetString();                                 \
    if (std::string(ascend_name).empty()) {                                             \
      REPORT_INNER_ERR_MSG("E18888", "ascend std::string error.");                      \
      GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetAttr: ascend std::string error."); \
      return GRAPH_PARAM_INVALID;                                                       \
    }                                                                                   \
    if (impl_ == nullptr) {                                                             \
      REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");           \
      GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: node impl is nullptr.");            \
      return GRAPH_FAILED;                                                              \
    }                                                                                   \
    return impl_->GetAttr(name, attr_value);                                            \
  }

#define NODE_ATTR_SET_IMP(ArgType)                                                      \
  graphStatus GNode::SetAttr(const AscendString &name, ArgType &attr_value) const {     \
    const char_t *const ascend_name = name.GetString();                                 \
    if (std::string(ascend_name).empty()) {                                             \
      REPORT_INNER_ERR_MSG("E18888", "ascend std::string error.");                      \
      GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttr: ascend std::string error."); \
      return GRAPH_PARAM_INVALID;                                                       \
    }                                                                                   \
    if (impl_ == nullptr) {                                                             \
      REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");           \
      GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: node impl is nullptr.");            \
      return GRAPH_FAILED;                                                              \
    }                                                                                   \
    return impl_->SetAttr(name, attr_value);                                            \
  }

namespace ge {
GNodePtr NodeAdapter::ExecuteNode2GNode(FastNode *node) {
  if ((node == nullptr) || (node->GetExtendInfo() == nullptr)) {
    return nullptr;
  }
  auto *owner = node->GetExtendInfo()->GetOwnerGraphBarePtr();
  const auto graph = owner != nullptr ? owner->weak_from_this().lock() : nullptr;
  if (graph == nullptr) {
    return nullptr;
  }
  const auto handle = graph->GetNodeHandle(node);
  if (graph->ResolveNode(handle) != node) {
    return nullptr;
  }
  auto wrapped = ComGraphMakeShared<GNode>();
  GE_ASSERT_NOTNULL(wrapped);
  wrapped->impl_ = ComGraphMakeShared<ExecuteNodeBackend>(graph, handle);
  GE_ASSERT_NOTNULL(wrapped->impl_);
  return wrapped;
}

FastNode *NodeAdapter::GNode2ExecuteNode(const GNode &node) {
  const auto *backend = dynamic_cast<const ExecuteNodeBackend *>(node.impl_.get());
  return backend != nullptr ? backend->GetNode() : nullptr;
}

bool NodeAdapter::IsExecuteNode(const GNode &node) {
  return dynamic_cast<const ExecuteNodeBackend *>(node.impl_.get()) != nullptr;
}

NodePtr NodeAdapter::GNode2Node(const ge::GNode &node) {
  if (dynamic_cast<const ExecuteNodeBackend *>(node.impl_.get()) != nullptr) {
    GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("GNode2Node");
    return nullptr;
  }
  if (node.impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param graph_node.impl_ is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GNode2Node: gnode impl is nullptr.");
    return nullptr;
  }
  return static_cast<const ComputeNodeBackend *>(node.impl_.get())->node_ptr_.lock();
}

GNode NodeAdapter::Node2GNode(const ge::NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param node is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Node2GNode: node is nullptr");
    return GNode();
  }

  const GNode graph_node;
  if (graph_node.impl_ == nullptr) {
    GELOGW("[Check][Param] Gnode impl is nullptr, node:%s", node->GetName().c_str());
    return graph_node;
  }
  static_cast<ComputeNodeBackend *>(graph_node.impl_.get())->node_ptr_ = node;

  return graph_node;
}

GNodePtr NodeAdapter::Node2GNodePtr(const ge::NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param node is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Node2GNodePtr: node is nullptr");
    return nullptr;
  }

  const GNodePtr gnode = MakeShared<GNode>();
  if (gnode == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "create GNode failed.");
    GELOGE(GRAPH_FAILED, "[Create][GNode] Node2GNodePtr: gnode is nullptr, node[%s].", node->GetName().c_str());
    return nullptr;
  }

  if (gnode->impl_ == nullptr) {
    GELOGW("[Check][Param] Gnode impl is nullptr, node:%s", node->GetName().c_str());
    return nullptr;
  }
  static_cast<ComputeNodeBackend *>(gnode->impl_.get())->node_ptr_ = node;
  return gnode;
}

GNode::GNode() {
  impl_ = ComGraphMakeShared<ComputeNodeBackend>();
  if (impl_ == nullptr) {
    GELOGW("[Check][Impl] Make node impl failed");
  }
}

graphStatus GNode::GetType(AscendString &type) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl_ is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetType: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetType(type);
}

graphStatus GNode::GetName(AscendString &name) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetName: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetName(name);
}

std::pair<GNodePtr, int32_t> GNode::GetInDataNodesAndPortIndexs(const int32_t index) const {
  const std::pair<GNodePtr, int32_t> gnode_idx = {nullptr, 0xFF};
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: node impl is nullptr.");
    return gnode_idx;
  }

  return impl_->GetInDataNodesAndPortIndexs(index);
}

std::vector<GNodePtr> GNode::GetInControlNodes() const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Gnode node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: node impl is nullptr.");
    return {};
  }

  return impl_->GetInControlNodes();
}

std::vector<std::pair<GNodePtr, int32_t>> GNode::GetOutDataNodesAndPortIndexs(const int32_t index) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: node impl is nullptr.");
    return {};
  }

  return impl_->GetOutDataNodesAndPortIndexs(index);
}

std::vector<GNodePtr> GNode::GetOutControlNodes() const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "GetOutControlNodes node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutControlNodes: node impl is nullptr.");
    return {};
  }

  return impl_->GetOutControlNodes();
}

graphStatus GNode::GetInputConstData(const int32_t index, Tensor &data) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputConstData: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetInputConstData(index, data);
}

graphStatus GNode::GetInputIndexByName(const AscendString &name, int32_t &index) {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "ascend string error.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetInputIndexByName: ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputIndexByName: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetInputIndexByName(name, index);
}

graphStatus GNode::GetOutputIndexByName(const AscendString &name, int32_t &index) {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "ascend string error.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetOutputIndexByName: ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputIndexByName: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetOutputIndexByName(name, index);
}

graphStatus GNode::GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) {
  const std::string node_name = name.GetString();
  if (node_name.empty()) {
    REPORT_INNER_ERR_MSG("E18888", "input name is empty.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] input name is empty.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetDynamicInputIndexesByName(name, indexes);
}

graphStatus GNode::GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) {
  const std::string node_name = name.GetString();
  if (node_name.empty()) {
    REPORT_INNER_ERR_MSG("E18888", "output name is empty.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] output name is empty.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetDynamicOutputIndexesByName(name, indexes);
}

size_t GNode::GetInputsSize() const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputsSize: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetInputsSize();
}

size_t GNode::GetOutputsSize() const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputsSize: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetOutputsSize();
}

graphStatus GNode::GetInputDesc(const int32_t index, TensorDesc &tensor_desc) const {
  if (index < 0) {
    REPORT_INNER_ERR_MSG("E18888", "index:%d cannot be less than zero, check invalid.", index);
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetInputDesc: index[%d] cannot be less than zero.", index);
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputDesc: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetDesc(index, tensor_desc, true);
}

graphStatus GNode::UpdateInputDesc(const int32_t index, const TensorDesc &tensor_desc) {
  if (index < 0) {
    REPORT_INNER_ERR_MSG("E18888", "index:%d cannot be less than zero.", index);
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] UpdateInputDesc: index[%d] cannot be less than zero.", index);
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateInputDesc: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->UpdateDesc(index, tensor_desc, true);
}

graphStatus GNode::GetOutputDesc(const int32_t index, TensorDesc &tensor_desc) const {
  if (index < 0) {
    REPORT_INNER_ERR_MSG("E18888", "index:%d cannot be less than zero.", index);
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetOutputDesc: index[%d] cannot be less than zero.", index);
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputDesc: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetDesc(index, tensor_desc, false);
}

graphStatus GNode::UpdateOutputDesc(const int32_t index, const TensorDesc &tensor_desc) {
  if (index < 0) {
    REPORT_INNER_ERR_MSG("E18888", "index:%d cannot be less than zero.", index);
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] Gnode: index[%d] cannot be less than zero.", index);
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateOutputDesc: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->UpdateDesc(index, tensor_desc, false);
}

NODE_ATTR_GET_IMP(int64_t)
NODE_ATTR_GET_IMP(int32_t)
NODE_ATTR_GET_IMP(uint32_t)
NODE_ATTR_GET_IMP(float32_t)
NODE_ATTR_GET_IMP(bool)
NODE_ATTR_GET_IMP(Tensor)
NODE_ATTR_GET_IMP(std::vector<int64_t>)
NODE_ATTR_GET_IMP(std::vector<int32_t>)
NODE_ATTR_GET_IMP(std::vector<uint32_t>)
NODE_ATTR_GET_IMP(std::vector<float32_t>)
NODE_ATTR_GET_IMP(std::vector<bool>)
NODE_ATTR_GET_IMP(std::vector<Tensor>)
NODE_ATTR_GET_IMP(OpBytes)
NODE_ATTR_GET_IMP(std::vector<std::vector<int64_t>>)
NODE_ATTR_GET_IMP(std::vector<ge::DataType>)
NODE_ATTR_GET_IMP(ge::DataType)
NODE_ATTR_GET_IMP(AttrValue)

graphStatus GNode::GetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "name ascend string error");
    GELOGE(GRAPH_PARAM_INVALID, "GetAttr: name ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetAttr(name, attr_value);
}

graphStatus GNode::GetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "name ascend string error");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] GetAttr: name ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetAttr(name, attr_values);
}

graphStatus GNode::GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const {
  attr_values.clear();
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAllAttrs: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetAllAttrs(attr_values);
}

NODE_ATTR_SET_IMP(int64_t)
NODE_ATTR_SET_IMP(int32_t)
NODE_ATTR_SET_IMP(uint32_t)
NODE_ATTR_SET_IMP(float32_t)
NODE_ATTR_SET_IMP(bool)
NODE_ATTR_SET_IMP(Tensor)
NODE_ATTR_SET_IMP(std::vector<int64_t>)
NODE_ATTR_SET_IMP(std::vector<int32_t>)
NODE_ATTR_SET_IMP(std::vector<uint32_t>)
NODE_ATTR_SET_IMP(std::vector<float32_t>)
NODE_ATTR_SET_IMP(std::vector<bool>)
NODE_ATTR_SET_IMP(std::vector<Tensor>)
NODE_ATTR_SET_IMP(OpBytes)
NODE_ATTR_SET_IMP(std::vector<std::vector<int64_t>>)
NODE_ATTR_SET_IMP(std::vector<ge::DataType>)
NODE_ATTR_SET_IMP(ge::DataType)

graphStatus GNode::SetAttr(const AscendString &name, AttrValue &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "ascend string error.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttr: ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->SetAttr(name, attr_value);
}

graphStatus GNode::SetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "name ascend string error");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttr: name ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->SetAttr(name, attr_value);
}

graphStatus GNode::SetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "name ascend string error");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttr: name ascend string error.");
    return GRAPH_PARAM_INVALID;
  }

  for (auto &attr_val : attr_values) {
    const char_t *const ascend_attr_value = attr_val.GetString();
    if (std::string(ascend_attr_value).empty()) {
      REPORT_INNER_ERR_MSG("E18888", "param attr values are invalid");
      GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttr: attr val error.");
      return GRAPH_PARAM_INVALID;
    }
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->SetAttr(name, attr_values);
}

graphStatus GNode::SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttrs: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->SetAttrs(attr_values);
}

bool GNode::HasAttr(const AscendString &name) {
  const char_t *const ascend_name = name.GetString();
  if (std::string(ascend_name).empty()) {
    REPORT_INNER_ERR_MSG("E18888", "ascend string error.");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] HasAttr: ascend string error.");
    return false;
  }

  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] HasAttr: node impl is nullptr.");
    return false;
  }

  return impl_->HasAttr(name);
}

graphStatus GNode::GetSubgraph(uint32_t index, GraphPtr &graph) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetSubgraph: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetSubgraph(index, graph);
}

graphStatus GNode::GetALLSubgraphs(std::vector<GraphPtr> &graph_list) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetALLSubgraphs: node impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->GetAllSubgraphs(graph_list);
}

graphStatus GNode::SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetSubgraph: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->SetSubgraph(subgraph_ir_name, subgraph);
}

graphStatus GNode::SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetSubgraphs: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->SetSubgraphs(subgraph_ir_name, subgraphs);
}

// 实现新的AttrValue接口
graphStatus GNode::GetOutputAttr(const AscendString &name, uint32_t output_index, AttrValue &attr_value) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] PortAttr: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->PortAttr(name, output_index, attr_value, false);
}

graphStatus GNode::SetOutputAttr(const AscendString &name, uint32_t output_index, const AttrValue &attr_value) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetPortAttr: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->SetPortAttr(name, output_index, attr_value, false);
}

graphStatus GNode::GetInputAttr(const AscendString &name, uint32_t input_index, AttrValue &attr_value) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] PortAttr: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->PortAttr(name, input_index, attr_value, true);
}

graphStatus GNode::SetInputAttr(const AscendString &name, uint32_t input_index, const AttrValue &attr_value) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetPortAttr: node impl is nullptr.");
    return PARAM_INVALID;  // 保持原 GE_ASSERT_NOTNULL(impl_) 的 ErrorResult 返回语义
  }

  return impl_->SetPortAttr(name, input_index, attr_value, true);
}
}  // namespace ge
