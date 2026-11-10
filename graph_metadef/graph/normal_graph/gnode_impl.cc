/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/gnode_impl.h"

#include "base/err_msg.h"
#include "common/checker.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/operator.h"
#include "graph/utils/tensor_adapter.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {
bool NodeImpl::NodeContext::IsValid() const {
  return compute_node != nullptr || execute_owner != nullptr;
}

std::string NodeImpl::NodeContext::GetName() const {
  return compute_node != nullptr ? compute_node->GetName() : op_desc->GetName();
}

std::string NodeImpl::NodeContext::GetType() const {
  return compute_node != nullptr ? compute_node->GetType() : op_desc->GetType();
}

const char *NodeImpl::NodeContext::GetNamePtr() const {
  return compute_node != nullptr ? compute_node->GetNamePtr() : op_desc->GetNamePtr();
}

const char *NodeImpl::NodeContext::GetTypePtr() const {
  return compute_node != nullptr ? compute_node->GetTypePtr() : op_desc->GetTypePtr();
}

// 通用类型属性访问沿用计算节点的属性转换和返回值语义。
#define NODE_ATTR_GET_IMP(ArgType)                                                       \
  graphStatus NodeImpl::GetAttr(const AscendString &name, ArgType &attr_value) const {   \
    const char_t *const ascend_name = name.GetString();                                  \
    const NodeContext node = ResolveNode();                                              \
    if (!node.IsValid()) {                                                               \
      REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");  \
      GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid."); \
      return GRAPH_FAILED;                                                               \
    }                                                                                    \
    const Operator op = CreateOperator(node);                                            \
    if (op.GetAttr(ascend_name, attr_value) != GRAPH_SUCCESS) {                          \
      GELOGW("[Get][Attr] of node[%s] failed.", node.GetName().c_str());                 \
      return GRAPH_FAILED;                                                               \
    }                                                                                    \
    return GRAPH_SUCCESS;                                                                \
  }

#define NODE_ATTR_SET_IMP(ArgType)                                                       \
  graphStatus NodeImpl::SetAttr(const AscendString &name, ArgType &attr_value) const {   \
    const char_t *const ascend_name = name.GetString();                                  \
    const NodeContext node = ResolveNode();                                              \
    if (!node.IsValid()) {                                                               \
      REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");  \
      GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the node shared ptr is not valid."); \
      return GRAPH_FAILED;                                                               \
    }                                                                                    \
    Operator op = CreateOperator(node);                                                  \
    (void)op.SetAttr(ascend_name, attr_value);                                           \
    return GRAPH_SUCCESS;                                                                \
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

graphStatus NodeImpl::GetType(AscendString &type) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetType: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_type = node.GetType();
  const AscendString ascend_type(node_type.c_str());
  type = ascend_type;

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetName(AscendString &name) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_name = node.GetName();
  const AscendString ascend_name(node_name.c_str());
  name = ascend_name;
  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetInputIndexByName(const AscendString &name, int32_t &index) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputIndexByName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node.GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get Op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  index = op_desc->GetInputIndexByName(node_name);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetOutputIndexByName(const AscendString &name, int32_t &index) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputIndexByName: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node.GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  index = op_desc->GetOutputIndexByName(node_name);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const {
  const std::string node_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr");
    GELOGE(GRAPH_FAILED, "[Check][Param] the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node.GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
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

graphStatus NodeImpl::GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const {
  const std::string node_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] the node shared ptr is nullptr.");
    return GRAPH_FAILED;
  }

  const auto op_desc = node.GetOpDescBarePtr();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
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

size_t NodeImpl::GetInputsSize() const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputsSize: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  return op_desc->GetInputsSize();
}

size_t NodeImpl::GetOutputsSize() const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputsSize: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  return op_desc->GetOutputsSize();
}

graphStatus NodeImpl::GetDesc(const int32_t index, TensorDesc &tensor_desc, const bool input) const {
  return input ? GetInputDescInner(index, tensor_desc) : GetOutputDescInner(index, tensor_desc);
}

graphStatus NodeImpl::UpdateDesc(const int32_t index, const TensorDesc &tensor_desc, const bool input) const {
  return input ? UpdateInputDescInner(index, tensor_desc) : UpdateOutputDescInner(index, tensor_desc);
}

graphStatus NodeImpl::GetInputDescInner(const int32_t index, TensorDesc &tensor_desc) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }
  if (static_cast<size_t>(index) >= op_desc->GetAllInputsSize()) {
    REPORT_INNER_ERR_MSG("E18888", "Get tensor desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][TensorDesc] of node[%s] failed, invalid index is %d and valid range should be [0, %d)",
           node.GetName().c_str(), index, op_desc->GetAllInputsSize());
    return GRAPH_FAILED;
  }
  const ConstGeTensorDescPtr ge_tensor_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(index));
  if (ge_tensor_desc == nullptr) {
    GELOGW("[Get][TensorDesc] of node[%s] failed, input is optional input with no edge and index is [%d]",
           node.GetName().c_str(), index);
    return GRAPH_FAILED;
  }
  tensor_desc = TensorAdapter::GeTensorDesc2TensorDesc(*ge_tensor_desc);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetOutputDescInner(const int32_t index, TensorDesc &tensor_desc) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const ConstGeTensorDescPtr ge_tensor_desc = op_desc->GetOutputDescPtr(static_cast<uint32_t>(index));
  if (ge_tensor_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get tensor desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][TensorDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }
  tensor_desc = TensorAdapter::GeTensorDesc2TensorDesc(*ge_tensor_desc);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::UpdateInputDescInner(const int32_t index, const TensorDesc &tensor_desc) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateInputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const GeTensorDesc ge_tensor_desc = TensorAdapter::TensorDesc2GeTensorDesc(tensor_desc);
  if (op_desc->UpdateInputDesc(static_cast<uint32_t>(index), ge_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "Update input desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Update][InputDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::UpdateOutputDescInner(const int32_t index, const TensorDesc &tensor_desc) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] UpdateOutputDesc: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const GeTensorDesc ge_tensor_desc = TensorAdapter::TensorDesc2GeTensorDesc(tensor_desc);
  if (op_desc->UpdateOutputDesc(static_cast<uint32_t>(index), ge_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "Update input desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Update][InputDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  const Operator op = CreateOperator(node);
  std::string op_name;
  if (op.GetAttr(node_name, op_name) != GRAPH_SUCCESS) {
    GELOGW("[Check][Param] Get attr of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const AscendString attr_value_get(op_name.c_str());
  attr_value = attr_value_get;

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAttr: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  const Operator op = CreateOperator(node);
  std::vector<std::string> attr_names;
  if (op.GetAttr(node_name, attr_names) != GRAPH_SUCCESS) {
    GELOGW("[Get][Attr] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  for (auto &attr_name : attr_names) {
    const AscendString ascend_attr_name(attr_name.c_str());
    attr_values.push_back(ascend_attr_name);
  }

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::SetAttr(const AscendString &name, AscendString &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the shared ptr is not valid.");
    return GRAPH_FAILED;
  }
  const std::string node_name = ascend_name;
  const std::string node_attr_value = attr_value.GetString();
  Operator op = CreateOperator(node);
  (void)op.SetAttr(node_name, node_attr_value);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::SetAttr(const AscendString &name, std::vector<AscendString> &attr_values) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
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
  Operator op = CreateOperator(node);
  (void)op.SetAttr(node_name, node_attr_vals);

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAllAttrs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  const Operator op = CreateOperator(node);
  for (const auto &attr : op_desc->GetAllAttrs()) {
    if (attr.first.empty() || attr.first.front() == '_') {
      continue;
    }
    AttrValue attr_value;
    if (op.GetAttr(attr.first.c_str(), attr_value) != GRAPH_SUCCESS) {
      REPORT_INNER_ERR_MSG("E18888", "Get attr[%s] of node[%s] failed.", attr.first.c_str(), node.GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][Attr] %s of node[%s] failed.", attr.first.c_str(), node.GetName().c_str());
      attr_values.clear();
      return GRAPH_FAILED;
    }
    attr_values.emplace(AscendString(attr.first.c_str()), std::move(attr_value));
  }
  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttrs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
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
      REPORT_INNER_ERR_MSG("E18888", "Set attr[%s] of node[%s] failed.", attr_name.c_str(), node.GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Set][Attr] %s of node[%s] failed.", attr_name.c_str(), node.GetName().c_str());
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::SetAttr(const AscendString &name, AttrValue &attr_value) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetAttr: the shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const std::string node_name = ascend_name;
  Operator op = CreateOperator(node);
  (void)op.SetAttr(node_name.c_str(), std::move(attr_value));
  return GRAPH_SUCCESS;
}

bool NodeImpl::HasAttr(const AscendString &name) const {
  const char_t *const ascend_name = name.GetString();
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] HasAttr: the node shared ptr is not valid.");
    return false;
  }

  const OpDescPtr op_desc = node.GetOpDesc();
  if (op_desc == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Get op desc of node[%s] failed.", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] of node[%s] failed.", node.GetName().c_str());
    return false;
  }
  const std::string attr_name = ascend_name;
  if (!op_desc->HasAttr(attr_name)) {
    GELOGW("[Call][HasAttr] Node[%s] has no attr name[%s]", node.GetName().c_str(), attr_name.c_str());
    return false;
  }

  return true;
}

graphStatus NodeImpl::GetPortAttrValue(const NodeContext &node, const AscendString &name, const uint32_t index,
                                       AttrValue &attr_value, const bool is_input) const {
  auto tensor_desc = is_input ? node.GetOpDesc()->MutableInputDesc(index) : node.GetOpDesc()->MutableOutputDesc(index);
  GE_ASSERT_NOTNULL(tensor_desc, "index: %u is invalid for node: %s %s", index, node.GetNamePtr(), node.GetTypePtr());
  GE_WARN_ASSERT_GRAPH_SUCCESS(tensor_desc->GetAttr(name.GetString(), attr_value.impl->MutableAnyValue()),
                               "Attr: %s has not been set for node: %s %s", name.GetString(), node.GetNamePtr(),
                               node.GetTypePtr());
  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::SetPortAttrValue(const NodeContext &node, const AscendString &name, const uint32_t index,
                                       const AttrValue &attr_value, const bool is_input) const {
  auto tensor_desc = is_input ? node.GetOpDesc()->MutableInputDesc(index) : node.GetOpDesc()->MutableOutputDesc(index);
  GE_ASSERT_NOTNULL(tensor_desc, "index: %u is invalid for node: %s %s", index, node.GetNamePtr(), node.GetTypePtr());
  return tensor_desc->SetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus NodeImpl::PortAttr(const AscendString &name, const uint32_t index, AttrValue &attr_value,
                               const bool input) const {
  const NodeContext node = ResolveNode();
  GE_ASSERT_TRUE(node.IsValid(), "Gnode has invalid node source, try to call `AddNodeByOp` firstly");
  return GetPortAttrValue(node, name, index, attr_value, input);
}

graphStatus NodeImpl::SetPortAttr(const AscendString &name, const uint32_t index, const AttrValue &attr_value,
                                  const bool input) const {
  const NodeContext node = ResolveNode();
  GE_ASSERT_TRUE(node.IsValid(), "Gnode has invalid node source, try to call `AddNodeByOp` firstly");
  return SetPortAttrValue(node, name, index, attr_value, input);
}

graphStatus NodeImpl::GetSubgraph(const uint32_t index, GraphPtr &graph) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetSubgraph: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const GraphContext subgraph = LookupSubgraph(node, index);
  if (!subgraph.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "get subgraph[%u] failed from node[%s].", index, node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][SubGraph] subgraph[%u] from node[%s] is nullptr.", index, node.GetName().c_str());
    return GRAPH_FAILED;
  }

  graph = WrapGraph(subgraph);
  if (graph == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "create %s graph failed from %s.",
                         subgraph.compute_graph != nullptr ? "compute" : "execute", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Create][Graph] failed from %s.", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetAllSubgraphs(std::vector<GraphPtr> &graph_list) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetALLSubgraphs: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  const GraphContext root = LookupRootGraph(node);
  if (!root.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "Failed to find root graph from node %s ", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][RootGraph] Failed to find root graph from node %s ", node.GetName().c_str());
    return GRAPH_FAILED;
  }
  const auto sub_graphs = ListSubgraphs(root);
  if (sub_graphs.empty()) {
    REPORT_INNER_ERR_MSG("E18888", "get all subgraphs failed from node[%s].", node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][ALLSubGraphs] failed from node[%s].", node.GetName().c_str());
    return GRAPH_FAILED;
  }

  for (const auto &sub_graph : sub_graphs) {
    if (!sub_graph.IsValid()) {
      REPORT_INNER_ERR_MSG("E18888", "get subgraph failed from node[%s].", node.GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Get][SubGraph] failed from node[%s].", node.GetName().c_str());
      return GRAPH_FAILED;
    }
    GraphPtr graph = WrapGraph(sub_graph);
    if (graph == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "create %s graph failed from node[%s].",
                           sub_graph.compute_graph != nullptr ? "compute" : "execute", node.GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Create][%s] failed from node[%s].",
             sub_graph.compute_graph != nullptr ? "ComputeGraph" : "ExecuteGraph", node.GetName().c_str());
      return GRAPH_FAILED;
    }
    graph_list.emplace_back(graph);
  }

  if (graph_list.empty()) {
    GELOGW("[Get][Subgraph] Node %s has no subgraph", node.GetName().c_str());
  }

  return GRAPH_SUCCESS;
}

graphStatus NodeImpl::GetInputConstData(const int32_t index, Tensor &data) const {
  const NodeContext node = ResolveNode();
  if (!node.IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "the node shared ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetInputConstData: the node shared ptr is not valid.");
    return GRAPH_FAILED;
  }

  NodeContext source;
  const graphStatus ret = ResolveConstInput(node, index, source);
  if (ret == GRAPH_NODE_WITHOUT_CONST_INPUT) {
    REPORT_INNER_ERR_MSG("E18888", "Node[%s] has no const input.", node.GetName().c_str());
    GELOGE(GRAPH_NODE_WITHOUT_CONST_INPUT, "[Check][Param] Node[%s] has no const input.", node.GetName().c_str());
    return GRAPH_NODE_WITHOUT_CONST_INPUT;
  }
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }

  const Operator const_op = CreateOperator(source);
  if (const_op.GetAttr(ATTR_NAME_WEIGHTS.c_str(), data) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "Input data node[%s] of node[%s] get data failed.", source.GetName().c_str(),
                         node.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Attr] Input data node[%s] of node[%s] get data failed.", source.GetName().c_str(),
           node.GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

}  // namespace ge
