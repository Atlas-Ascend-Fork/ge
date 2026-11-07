/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/compute_node_backend.h"

#include "base/err_msg.h"
#include "common/checker.h"
#include "graph/anchor.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_tensor.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_adapter.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {

// 通用类型属性访问（int/float/bool/Tensor 及其 vector、OpBytes、DataType、AttrValue 等），
// 模板不能为虚，退回宏展开的逐类型 override；实现体原样承接自原模板公共路径。
#define COMPUTE_NODE_ATTR_GET_IMP(ArgType)                                                       \
  graphStatus ComputeNodeBackend::GetAttr(const AscendString &name, ArgType &attr_value) const { \
    const char_t *const ascend_name = name.GetString();                                          \
    const std::shared_ptr<Node> node_ptr = node_ptr_.lock();                                     \
    if (node_ptr == nullptr) {                                                                   \
      REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");          \
      GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid.");         \
      return GRAPH_FAILED;                                                                       \
    }                                                                                            \
    const Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);                           \
    if (op.GetAttr(ascend_name, attr_value) != GRAPH_SUCCESS) {                                  \
      GELOGW("[Get][Attr] of node[%s] failed.", node_ptr->GetName().c_str());                    \
      return GRAPH_FAILED;                                                                       \
    }                                                                                            \
    return GRAPH_SUCCESS;                                                                        \
  }

#define COMPUTE_NODE_ATTR_SET_IMP(ArgType)                                                       \
  graphStatus ComputeNodeBackend::SetAttr(const AscendString &name, ArgType &attr_value) const { \
    const char_t *const ascend_name = name.GetString();                                          \
    const std::shared_ptr<Node> node_ptr = node_ptr_.lock();                                     \
    if (node_ptr == nullptr) {                                                                   \
      REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");          \
      GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the node shared ptr is not valid.");         \
      return GRAPH_FAILED;                                                                       \
    }                                                                                            \
    Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);                                 \
    (void)op.SetAttr(ascend_name, attr_value);                                                   \
    return GRAPH_SUCCESS;                                                                        \
  }

COMPUTE_NODE_ATTR_GET_IMP(int64_t)
COMPUTE_NODE_ATTR_GET_IMP(int32_t)
COMPUTE_NODE_ATTR_GET_IMP(uint32_t)
COMPUTE_NODE_ATTR_GET_IMP(float32_t)
COMPUTE_NODE_ATTR_GET_IMP(bool)
COMPUTE_NODE_ATTR_GET_IMP(Tensor)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<int64_t>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<int32_t>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<uint32_t>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<float32_t>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<bool>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<Tensor>)
COMPUTE_NODE_ATTR_GET_IMP(OpBytes)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<std::vector<int64_t>>)
COMPUTE_NODE_ATTR_GET_IMP(std::vector<ge::DataType>)
COMPUTE_NODE_ATTR_GET_IMP(ge::DataType)
COMPUTE_NODE_ATTR_GET_IMP(AttrValue)

COMPUTE_NODE_ATTR_SET_IMP(int64_t)
COMPUTE_NODE_ATTR_SET_IMP(int32_t)
COMPUTE_NODE_ATTR_SET_IMP(uint32_t)
COMPUTE_NODE_ATTR_SET_IMP(float32_t)
COMPUTE_NODE_ATTR_SET_IMP(bool)
COMPUTE_NODE_ATTR_SET_IMP(Tensor)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<int64_t>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<int32_t>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<uint32_t>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<float32_t>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<bool>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<Tensor>)
COMPUTE_NODE_ATTR_SET_IMP(OpBytes)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<std::vector<int64_t>>)
COMPUTE_NODE_ATTR_SET_IMP(std::vector<ge::DataType>)
COMPUTE_NODE_ATTR_SET_IMP(ge::DataType)

// ---- ComputeNodeBackend：GNode 计算节点操作的无状态适配函数集（实现体自 GNode 原样搬迁） ----

graphStatus ComputeNodeBackend::GetType(AscendString &type) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetType: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_type = node_ptr->GetType();
  const AscendString ascend_type(node_type.c_str());
  type = ascend_type;

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetName(AscendString &name) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_name = node_ptr->GetName();
  const AscendString ascend_name(node_name.c_str());
  name = ascend_name;
  return GRAPH_SUCCESS;
}

std::pair<GNodePtr, int32_t> ComputeNodeBackend::GetInDataNodesAndPortIndexs(const int32_t index) const {
  const std::pair<GNodePtr, int32_t> gnode_idx = {nullptr, 0xFF};
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node ptr is not valid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: the node shared ptr is not valid.");
    return gnode_idx;
  }

  const auto in_anchor = node_ptr->GetInDataAnchor(index);
  if (in_anchor == nullptr) {
    REPORT_INNER_ERR_MSG("E18888",
                         "Failed to get in data node of index[%d] from node[%s], "
                         "the anchor does not exist",
                         index, node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Get][Anchor] Failed to get in data node of index[%d] from node[%s], "
           "the anchor does not exist",
           index, node_ptr->GetName().c_str());
    return gnode_idx;
  }

  const auto out_anchor = in_anchor->GetPeerOutAnchor();
  if (out_anchor == nullptr) {
    GELOGI("Index[%d] from node[%s] has no peer anchor, maybe it is optional input", index,
           node_ptr->GetName().c_str());
    return gnode_idx;
  }

  const NodePtr peer_node_ptr = out_anchor->GetOwnerNode();
  const GNodePtr gnode = NodeAdapter::Node2GNodePtr(peer_node_ptr);
  if (gnode == nullptr) {
    GELOGE(GRAPH_FAILED, "[Get][GNode] Peer node of node[%s] to gnode failed.", node_ptr->GetName().c_str());
    return gnode_idx;
  }

  return {gnode, out_anchor->GetIdx()};
}

std::vector<GNodePtr> ComputeNodeBackend::GetControlNodes(const bool input) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  std::vector<GNodePtr> gnodes;
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "%s node impl is nullptr, check invalid.", input ? "Gnode" : "GetOutControlNodes");
    GELOGE(GRAPH_FAILED, "[Check][Param] %s: node impl is nullptr.", input ? "Gnode" : "GetOutControlNodes");
    return {};
  }

  const auto control_nodes = input ? node_ptr->GetInControlNodes() : node_ptr->GetOutControlNodes();
  for (auto &control_node : control_nodes) {
    GNodePtr gnode = NodeAdapter::Node2GNodePtr(control_node);
    if (gnode == nullptr) {
      GELOGE(GRAPH_FAILED, "[Get][GNode] In control_node of node[%s] to gnode failed.", node_ptr->GetName().c_str());
      return {};
    }
    gnodes.emplace_back(gnode);
  }

  return gnodes;
}

std::vector<std::pair<GNodePtr, int32_t>> ComputeNodeBackend::GetOutDataNodesAndPortIndexs(const int32_t index) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: the node shared ptr is not valid.");
    return {};
  }

  const auto out_anchor = node_ptr->GetOutDataAnchor(index);
  if (out_anchor == nullptr) {
    REPORT_INNER_ERR_MSG("E18888",
                         "Failed to get out data node of index %d from node %s, "
                         "the anchor does not exist",
                         index, node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Get][Anchor] Failed to get out data node of index %d from node %s, "
           "the anchor does not exist",
           index, node_ptr->GetName().c_str());
    return {};
  }

  std::vector<std::pair<GNodePtr, int32_t>> gnode_index;
  const auto in_data_anchors = out_anchor->GetPeerInDataAnchorsPtr();
  for (auto &in_data_anchor : in_data_anchors) {
    if (in_data_anchor == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "In data anchor of node[%s] is nullptr.", node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Check][Param] In data anchor of node[%s] is nullptr.", node_ptr->GetName().c_str());
      return {};
    }
    const NodePtr peer_node_ptr = in_data_anchor->GetOwnerNode();
    const GNodePtr gnode = NodeAdapter::Node2GNodePtr(peer_node_ptr);
    if (gnode == nullptr) {
      GELOGE(GRAPH_FAILED, "[Get][GNode] Peer node of node[%s] to gnode failed.", node_ptr->GetName().c_str());
      return {};
    }
    gnode_index.emplace_back(std::pair<GNodePtr, int32_t>(gnode, in_data_anchor->GetIdx()));
  }

  return gnode_index;
}

graphStatus ComputeNodeBackend::GetInputConstData(const int32_t index, Tensor &data) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputConstData: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const NodePtr input_data_node = NodeUtils::GetInDataNodeByIndex(*node_ptr, index);
  GE_CHECK_NOTNULL(input_data_node);
  const std::string op_type = input_data_node->GetType();
  if ((op_type == CONSTANT) || (op_type == CONSTANTOP)) {
    const Operator const_op = OpDescUtils::CreateOperatorFromNode(input_data_node);
    if (const_op.GetAttr(ATTR_NAME_WEIGHTS.c_str(), data) != GRAPH_SUCCESS) {
      REPORT_INNER_ERR_MSG("E18888", "Input data node[%s] of node[%s] get data failed.",
                           input_data_node->GetName().c_str(), node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][Attr] Input data node[%s] of node[%s] get data failed.",
             input_data_node->GetName().c_str(), node_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
    return SUCCESS;
  } else if (op_type == DATA) {
    auto parent_node = NodeUtils::GetParentInput(input_data_node);
    while ((parent_node != nullptr) && (parent_node->GetType() == DATA)) {
      parent_node = NodeUtils::GetParentInput(parent_node);
    }
    if ((parent_node != nullptr) && ((parent_node->GetType() == CONSTANT) || (parent_node->GetType() == CONSTANTOP))) {
      const Operator const_op = OpDescUtils::CreateOperatorFromNode(parent_node);
      if (const_op.GetAttr(ATTR_NAME_WEIGHTS.c_str(), data) != GRAPH_SUCCESS) {
        REPORT_INNER_ERR_MSG("E18888", "Input data node[%s] of node[%s] get data failed.",
                             parent_node->GetName().c_str(), node_ptr->GetName().c_str());
        GELOGE(GRAPH_FAILED, "[Get][Attr] Input data node[%s] of node[%s] get data failed.",
               parent_node->GetName().c_str(), node_ptr->GetName().c_str());
        return GRAPH_FAILED;
      }
      return SUCCESS;
    }
  } else {
    // empty
  }
  REPORT_INNER_ERR_MSG("E18888", "Node[%s] has no const input.", node_ptr->GetName().c_str());
  GELOGE(GRAPH_NODE_WITHOUT_CONST_INPUT, "[Check][Param] Node[%s] has no const input.", node_ptr->GetName().c_str());
  return GRAPH_NODE_WITHOUT_CONST_INPUT;
}

graphStatus ComputeNodeBackend::GetInputIndexByName(const AscendString &name, int32_t &index) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputIndexByName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node_ptr->GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get Op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  index = op_desc->GetInputIndexByName(node_name);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetOutputIndexByName(const AscendString &name, int32_t &index) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputIndexByName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node_ptr->GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  index = op_desc->GetOutputIndexByName(node_name);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetDynamicInputIndexesByName(const AscendString &name,
                                                             std::vector<int32_t> &indexes) const {
  const std::string node_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr");
    GELOGE(GRAPH_FAILED, "[Check][Param] the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node_ptr->GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (op_desc->GetDynamicInputIndexesByName(node_name, indexes) != GRAPH_SUCCESS || indexes.empty()) {
    const std::string error_message = indexes.empty()
                                          ? "the dynamic input indexes is empty, input name is " + node_name + "."
                                          : "get dynamic input index by name failed, input name is " + node_name + ".";
    REPORT_INNER_ERR_MSG("E18888", "%s", error_message.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] %s", error_message.c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetDynamicOutputIndexesByName(const AscendString &name,
                                                              std::vector<int32_t> &indexes) const {
  const std::string node_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] the node shared ptr is nullptr.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node_ptr->GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (op_desc->GetDynamicOutputIndexesByName(node_name, indexes) != GRAPH_SUCCESS || indexes.empty()) {
    const std::string error_message =
        indexes.empty() ? "the dynamic output indexes is empty, output name is " + node_name + "."
                        : "get dynamic output index by name failed, output name is " + node_name + ".";
    REPORT_INNER_ERR_MSG("E18888", "%s", error_message.c_str());
    GELOGE(GRAPH_FAILED, "[Check][Param] %s", error_message.c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

size_t ComputeNodeBackend::GetInputsSize() const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputsSize: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return op_desc->GetInputsSize();
}

size_t ComputeNodeBackend::GetOutputsSize() const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputsSize: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return op_desc->GetOutputsSize();
}

graphStatus ComputeNodeBackend::GetDesc(const int32_t index, TensorDesc &tensor_desc, const bool input) const {
  return input ? GetInputDescInner(index, tensor_desc) : GetOutputDescInner(index, tensor_desc);
}

graphStatus ComputeNodeBackend::UpdateDesc(const int32_t index, const TensorDesc &tensor_desc, const bool input) const {
  return input ? UpdateInputDescInner(index, tensor_desc) : UpdateOutputDescInner(index, tensor_desc);
}

graphStatus ComputeNodeBackend::GetInputDescInner(const int32_t index, TensorDesc &tensor_desc) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  if (static_cast<size_t>(index) >= op_desc->GetAllInputsSize()) {
    REPORT_INNER_ERR_MSG("E18888", "Get tensor desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][TensorDesc] of node[%s] failed, invalid index is %d and valid range should be [0, %d)",
           node_ptr->GetName().c_str(), index, op_desc->GetAllInputsSize());
    return GRAPH_FAILED;
  }
  const ConstGeTensorDescPtr ge_tensor_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(index));
  if (ge_tensor_desc == nullptr) {
    GELOGW("[Get][TensorDesc] of node[%s] failed, input is optional input with no edge and index is [%d]",
           node_ptr->GetName().c_str(), index);
    return GRAPH_FAILED;
  }
  tensor_desc = TensorAdapter::GeTensorDesc2TensorDesc(*ge_tensor_desc);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetOutputDescInner(const int32_t index, TensorDesc &tensor_desc) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const ConstGeTensorDescPtr ge_tensor_desc = op_desc->GetOutputDescPtr(static_cast<uint32_t>(index));
  if (ge_tensor_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get tensor desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][TensorDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  tensor_desc = TensorAdapter::GeTensorDesc2TensorDesc(*ge_tensor_desc);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::UpdateInputDescInner(const int32_t index, const TensorDesc &tensor_desc) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateInputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const GeTensorDesc ge_tensor_desc = TensorAdapter::TensorDesc2GeTensorDesc(tensor_desc);
  if (op_desc->UpdateInputDesc(static_cast<uint32_t>(index), ge_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "Update input desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Update][InputDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::UpdateOutputDescInner(const int32_t index, const TensorDesc &tensor_desc) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateOutputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const GeTensorDesc ge_tensor_desc = TensorAdapter::TensorDesc2GeTensorDesc(tensor_desc);
  if (op_desc->UpdateOutputDesc(static_cast<uint32_t>(index), ge_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "Update input desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Update][InputDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  const Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  std::string op_name;
  if (op.GetAttr(node_name, op_name) != GRAPH_SUCCESS) {
    GELOGW("[Check][Param] Get attr of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const AscendString attr_value_get(op_name.c_str());
  attr_value = attr_value_get;

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  const Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  std::vector<std::string> attr_names;
  if (op.GetAttr(node_name, attr_names) != GRAPH_SUCCESS) {
    GELOGW("[Get][Attr] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  for (auto &attr_name : attr_names) {
    const AscendString ascend_attr_name(attr_name.c_str());
    attr_values.push_back(ascend_attr_name);
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_name = ascend_name;
  const std::string node_attr_value = attr_value.GetString();
  Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  (void)op.SetAttr(node_name, node_attr_value);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  std::vector<std::string> node_attr_vals;
  for (const auto &attr_val : attr_values) {
    if (attr_val.GetString() != nullptr) {
      std::string node_attr_val = attr_val.GetString();
      node_attr_vals.emplace_back(node_attr_val);
    }
  }
  const std::string node_name = ascend_name;
  Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  (void)op.SetAttr(node_name, node_attr_vals);

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAllAttrs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  for (const auto &attr : op_desc->GetAllAttrs()) {
    if (attr.first.empty() || attr.first.front() == '_') {
      continue;
    }
    AttrValue attr_value;
    if (op.GetAttr(attr.first.c_str(), attr_value) != GRAPH_SUCCESS) {
      REPORT_INNER_ERR_MSG("E18888", "Get attr[%s] of node[%s] failed.", attr.first.c_str(),
                           node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][Attr] %s of node[%s] failed.", attr.first.c_str(), node_ptr->GetName().c_str());
      attr_values.clear();
      return GRAPH_FAILED;
    }
    attr_values.emplace(AscendString(attr.first.c_str()), std::move(attr_value));
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttrs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  for (const auto &attr : attr_values) {
    const char_t *const ascend_name = attr.first.GetString();
    if (ascend_name == nullptr || ascend_name[0] == '\0') {
      REPORT_INNER_ERR_MSG("E18888", "ascend string error.");
      GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] SetAttrs: ascend string error.");
      return GRAPH_PARAM_INVALID;
    }
  }

  for (const auto &attr : attr_values) {
    const std::string attr_name(attr.first.GetString());
    if (op_desc->SetAttr(attr_name, attr.second.impl->MutableAnyValue()) != GRAPH_SUCCESS) {
      REPORT_INNER_ERR_MSG("E18888", "Set attr[%s] of node[%s] failed.", attr_name.c_str(),
                           node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Set][Attr] %s of node[%s] failed.", attr_name.c_str(), node_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetAttr(const AscendString &name, AttrValue &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  Operator op = OpDescUtils::CreateOperatorFromNode(node_ptr);
  (void)op.SetAttr(node_name.c_str(), std::move(attr_value));
  return GRAPH_SUCCESS;
}

bool ComputeNodeBackend::HasAttr(const AscendString &name) const {
  const char_t *const ascend_name = name.GetString();
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] HasAttr: the node shared ptr is not valid.");
    return false;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node_ptr->GetName().c_str());
    return false;
  }
  const std::string attr_name = ascend_name;
  if (!op_desc->HasAttr(attr_name)) {
    GELOGW("[Call][HasAttr] Node[%s] has no attr name[%s]", node_ptr->GetName().c_str(), attr_name.c_str());
    return false;
  }

  return true;
}

graphStatus ComputeNodeBackend::GetSubgraph(const uint32_t index, GraphPtr &graph) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetSubgraph: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const ComputeGraphPtr compute_graph_ptr = NodeUtils::GetSubgraph(*node_ptr, index);
  if (compute_graph_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get subgraph[%u] failed from node[%s].", index, node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][SubGraph] subgraph[%u] from node[%s] is nullptr.", index, node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph_ptr);
  if (graph == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "create compute graph failed from %s.", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Create][Graph] failed from %s.", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::GetAllSubgraphs(std::vector<GraphPtr> &graph_list) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetALLSubgraphs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto root_graph = GraphUtils::FindRootGraph(node_ptr->GetOwnerComputeGraph());
  if (root_graph == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Failed to find root graph from node %s ", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][RootGraph] Failed to find root graph from node %s ", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  const std::vector<ComputeGraphPtr> sub_graphs = root_graph->GetAllSubgraphs();
  if (sub_graphs.empty()) {
    REPORT_INNER_ERR_MSG("E18888", "get all subgraphs failed from node[%s].", node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][ALLSubGraphs] failed from node[%s].", node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  for (auto &sub_graph : sub_graphs) {
    if (sub_graph == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "get subgraph failed from node[%s].", node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][SubGraph] failed from node[%s].", node_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
    GraphPtr graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(sub_graph);
    if (graph == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "create compute graph failed from node[%s].", node_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Create][ComputeGraph] failed from node[%s].", node_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
    graph_list.emplace_back(graph);
  }

  if (graph_list.empty()) {
    GELOGW("[Get][Subgraph] Node %s has no subgraph", node_ptr->GetName().c_str());
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  GE_ASSERT_NOTNULL(node_ptr, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");

  auto sub_compute_graph = GraphUtilsEx::GetComputeGraph(subgraph);
  GE_ASSERT_NOTNULL(sub_compute_graph);
  return NodeUtils::AddSubgraph(node_ptr, subgraph_ir_name.GetString(), sub_compute_graph);
}

graphStatus ComputeNodeBackend::SetSubgraphs(const AscendString &subgraph_ir_name,
                                             const std::vector<Graph> &subgraphs) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  GE_ASSERT_NOTNULL(node_ptr, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");

  std::vector<ComputeGraphPtr> sub_compute_graphs{};
  for (const auto &subgraph : subgraphs) {
    auto sub_compute_graph = GraphUtilsEx::GetComputeGraph(subgraph);
    GE_ASSERT_NOTNULL(sub_compute_graph);
    sub_compute_graphs.emplace_back(sub_compute_graph);
  }

  return NodeUtils::AddSubgraphs(node_ptr, subgraph_ir_name.GetString(), sub_compute_graphs);
}

graphStatus ComputeNodeBackend::GetPortAttrValue(const std::shared_ptr<Node> &node_ptr, const AscendString &name,
                                                 const uint32_t index, AttrValue &attr_value,
                                                 const bool is_input) const {
  auto tensor_desc =
      is_input ? node_ptr->GetOpDesc()->MutableInputDesc(index) : node_ptr->GetOpDesc()->MutableOutputDesc(index);
  GE_ASSERT_NOTNULL(tensor_desc, "index: %u is invalid for node: %s %s", index, node_ptr->GetNamePtr(),
                    node_ptr->GetTypePtr());
  GE_WARN_ASSERT_GRAPH_SUCCESS(tensor_desc->GetAttr(name.GetString(), attr_value.impl->MutableAnyValue()),
                               "Attr: %s has not been set for node: %s %s", name.GetString(), node_ptr->GetNamePtr(),
                               node_ptr->GetTypePtr());
  return GRAPH_SUCCESS;
}

graphStatus ComputeNodeBackend::SetPortAttrValue(const std::shared_ptr<Node> &node_ptr, const AscendString &name,
                                                 const uint32_t index, const AttrValue &attr_value,
                                                 const bool is_input) const {
  auto tensor_desc =
      is_input ? node_ptr->GetOpDesc()->MutableInputDesc(index) : node_ptr->GetOpDesc()->MutableOutputDesc(index);
  GE_ASSERT_NOTNULL(tensor_desc, "index: %u is invalid for node: %s %s", index, node_ptr->GetNamePtr(),
                    node_ptr->GetTypePtr());
  return tensor_desc->SetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus ComputeNodeBackend::PortAttr(const AscendString &name, const uint32_t index, AttrValue &attr_value,
                                         const bool input) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  GE_ASSERT_NOTNULL(node_ptr, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");
  return GetPortAttrValue(node_ptr, name, index, attr_value, input);
}

graphStatus ComputeNodeBackend::SetPortAttr(const AscendString &name, const uint32_t index, const AttrValue &attr_value,
                                            const bool input) const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  GE_ASSERT_NOTNULL(node_ptr, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");
  return SetPortAttrValue(node_ptr, name, index, attr_value, input);
}

}  // namespace ge
