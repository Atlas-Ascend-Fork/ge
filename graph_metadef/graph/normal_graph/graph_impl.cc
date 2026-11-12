/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/graph_impl.h"
#include "graph/op_desc.h"
#include "graph/utils/op_desc_utils.h"

namespace ge {
GNode GraphImpl::AddNodeByOp(const Operator &op) {
  const auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
  if (op_desc == nullptr) {
    AscendString name;
    (void)op.GetName(name);
    REPORT_INNER_ERR_MSG("E18888", "get op desc from op:%s failed", name.GetString());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] from op[%s] failed.", name.GetString());
    return GNode();
  }
  GE_ASSERT_GRAPH_SUCCESS(SetValid());
  const auto node = AddNodeFromDesc(op_desc);
  RegisterAddedOperator(op);
  return node;
}

GNodePtr GraphImpl::FindNodeByName(const AscendString &node_name) const {
  bool found = false;
  const auto node = FindDirectNode(node_name, found);
  GE_WARN_ASSERT(found, "Node name: %s was not found in the current graph: %s.", node_name.GetString(),
                 GetName().c_str());
  return node;
}

graphStatus GraphImpl::ResolveEdgeNodes(const GNode &src, const GNode &dst, NodeContext &src_context,
                                        NodeContext &dst_context) const {
  if (!ResolveNode(src, src_context)) {
    REPORT_INNER_ERR_MSG("E18888", "src gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] src gnode to node failed.");
    return GRAPH_FAILED;
  }
  if (!ResolveNode(dst, dst_context)) {
    REPORT_INNER_ERR_MSG("E18888", "dst gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] dst gnode to node failed.");
    return GRAPH_FAILED;
  }
  const char *const owner_word = IsComputeGraphKind() ? "compute graph" : "execute graph";
  if (!src_context.HasOwner()) {
    REPORT_INNER_ERR_MSG("E18888", "src node[%s] owner %s is nullptr.", src_context.name.c_str(), owner_word);
    GELOGE(GRAPH_FAILED, "[Get][Graph] src node[%s] owner %s is nullptr.", src_context.name.c_str(), owner_word);
    return GRAPH_FAILED;
  }
  if (!dst_context.HasOwner()) {
    REPORT_INNER_ERR_MSG("E18888", "dst node[%s] owner %s is nullptr.", dst_context.name.c_str(), owner_word);
    GELOGE(GRAPH_FAILED, "[Get][Graph] dst node[%s] owner %s is nullptr.", dst_context.name.c_str(), owner_word);
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                   const int32_t dst_port_index) {
  NodeContext src_context;
  NodeContext dst_context;
  if (ResolveEdgeNodes(src_node, dst_node, src_context, dst_context) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }

  const graphStatus res = LinkDataEdge(src_context, src_port_index, dst_context, dst_port_index);
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "add data edge from %s(%d) to %s(%d) failed.", src_context.name.c_str(),
                         src_port_index, dst_context.name.c_str(), dst_port_index);
    GELOGE(GRAPH_FAILED, "[Add][DataEdge] from %s(%d) to %s(%d) failed.", src_context.name.c_str(), src_port_index,
           dst_context.name.c_str(), dst_port_index);
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::AddControlEdge(GNode &src_node, GNode &dst_node) {
  NodeContext src_context;
  NodeContext dst_context;
  if (ResolveEdgeNodes(src_node, dst_node, src_context, dst_context) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }

  const graphStatus res = LinkControlEdge(src_context, dst_context);
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "add control edge from %s to %s failed.", src_context.name.c_str(),
                         dst_context.name.c_str());
    GELOGE(GRAPH_FAILED, "[Add][ControlEdge] from %s to %s failed.", src_context.name.c_str(),
           dst_context.name.c_str());
    return GRAPH_FAILED;
  }

  return SUCCESS;
}

graphStatus GraphImpl::RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                  const int32_t dst_port_index) {
  if ((src_port_index == -1) && (dst_port_index != -1)) {
    REPORT_INNER_ERR_MSG("E18888", "src_port_index == -1 and dst_port_index != -1, check invalid .");
    GELOGE(GRAPH_FAILED, "[Check][Param] src control anchor link to dst data anchor does not exist.");
    return GRAPH_FAILED;
  }

  NodeContext src_context;
  NodeContext dst_context;
  if (ResolveEdgeNodes(src_node, dst_node, src_context, dst_context) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }

  if (UnlinkEdge(src_context, src_port_index, dst_context, dst_port_index) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "remove edge between %s(%d) and %s(%d) failed.", src_context.name.c_str(),
                         src_port_index, dst_context.name.c_str(), dst_port_index);
    GELOGE(GRAPH_FAILED, "[Remove][Edge] between %s(%d) and %s(%d) failed.", src_context.name.c_str(), src_port_index,
           dst_context.name.c_str(), dst_port_index);
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::RemoveNode(GNode &node, bool contain_subgraph) {
  NodeContext context;
  if (!ResolveNode(node, context)) {
    REPORT_INNER_ERR_MSG("E19999", "Param:node_ptr is nullptr, check invalid");
    GELOGE(ge::FAILED, "[Check][Param:node_ptr]null is invalid");
    return PARAM_INVALID;
  }
  if (!context.HasOwner()) {
    REPORT_INNER_ERR_MSG("E19999", "Param:owner_compute_graph is nullptr, check invalid");
    GELOGE(ge::FAILED, "[Check][Param:owner_compute_graph]null is invalid");
    return PARAM_INVALID;
  }
  if (!IsValid()) {
    REPORT_INNER_ERR_MSG("E19999", "Param:compute_graph_ptr is nullptr, check invalid");
    GELOGE(ge::FAILED, "[Check][Param:compute_graph_ptr]null is invalid");
    return PARAM_INVALID;
  }
  if (!IsNodeInScope(context, contain_subgraph)) {
    if (contain_subgraph) {
      REPORT_INNER_ERR_MSG("E18888", "node[%s] is not in the graph[%s] or not in subgraph.", context.name.c_str(),
                           GetName().c_str());
    } else {
      REPORT_INNER_ERR_MSG("E18888", "node[%s] is not in the graph[%s].", context.name.c_str(), GetName().c_str());
    }
    GELOGE(GRAPH_FAILED, "[Check][Param] node[%s] is not in the graph[%s].", context.name.c_str(), GetName().c_str());
    return GRAPH_FAILED;
  }
  UnlinkNode(context);
  if (EraseNode(context) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "graph:%s remove node:%s failed", context.owner_name.c_str(), context.name.c_str());
    GELOGE(GRAPH_FAILED, "[Remove][Node] %s from graph:%s failed.", context.name.c_str(), context.owner_name.c_str());
    return GRAPH_FAILED;
  }
  ClearNodeOwner(context);
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::SetAttr(const AscendString &name, const AttrValue &attr_value) {
  auto *holder = GetAttributeHolder();
  GE_ASSERT_NOTNULL(holder, "Inner source is not ready, call `SetInputs` or `SetValid` at first");
  return holder->SetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus GraphImpl::GetAttr(const AscendString &name, AttrValue &attr_value) const {
  auto *holder = GetAttributeHolder();
  GE_ASSERT_NOTNULL(holder, "Inner source is not ready, call `SetInputs` or `SetValid` at first");
  return holder->GetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus GraphImpl::SetInputs(const std::vector<Operator> &inputs) {
  GE_ASSERT(!IsValid(),
            "Inner graph has been inited, maybe you call `SetInputs` again or call `AddNodeByOp or "
            "`SetValid` before `SetInputs`");
  BuildGraphFromInputs(inputs);
  GE_CHK_BOOL_RET_STATUS(IsValid(), GRAPH_FAILED, "[Build][Graph] failed.");
  GE_CHK_BOOL_RET_STATUS(inputs.size() != 0U, GRAPH_FAILED, "[Check][Param] set input NULL.");
  SetInputSize(static_cast<uint32_t>(inputs.size()));
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::AddSubGraph(const Graph &subgraph) {
  if (!IsCompatibleSubgraph(subgraph)) {
    const char *const msg = IsComputeGraphKind() ? "Compute graph cannot adopt an execution subgraph"
                                                 : "Subgraph is invalid or belongs to a different graph kind";
    REPORT_INNER_ERR_MSG("E19999", "%s", msg);
    GELOGE(ge::FAILED, "%s", msg);
    return PARAM_INVALID;
  }
  if (RegisterSubgraph(subgraph) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "graph:%s add subgraph:%s failed", GetName().c_str(), subgraph.GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Add][Subgraph] %s to graph:%s failed.", subgraph.GetName().c_str(), GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::SetOutputs(const std::vector<Operator> &outputs) {
  if (!IsValid()) {
    const bool is_compute = IsComputeGraphKind();
    REPORT_INNER_ERR_MSG("E18888", "%s is nullptr, check invalid.", is_compute ? "compute graph" : "execute graph");
    GELOGE(GRAPH_FAILED, "[Check][Param] set %s failed.", is_compute ? "ComputeGraph" : "ExecuteGraph");
    return GRAPH_FAILED;
  }
  if (outputs.empty()) {
    GELOGI("Set outputs size is 0.");
    return GRAPH_SUCCESS;
  }
  std::vector<std::pair<Operator, std::vector<size_t>>> output_indexes;
  for (const auto &output : outputs) {
    output_indexes.emplace_back(output, std::vector<size_t>{});
  }
  return SetOutputs(output_indexes);
}

graphStatus GraphImpl::SetOutputs(const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexes) {
  if (!IsValid()) {
    const bool is_compute = IsComputeGraphKind();
    REPORT_INNER_ERR_MSG("E18888", "%s is nullptr, check invalid.", is_compute ? "compute graph" : "execute graph");
    GELOGE(GRAPH_FAILED, "[Check][Param] set %s failed.", is_compute ? "ComputeGraph" : "ExecuteGraph");
    return GRAPH_FAILED;
  }
  if (output_indexes.empty()) {
    GELOGW("[SetOutputs][CheckParam] Set outputs size is 0.");
    return GRAPH_SUCCESS;
  }
  PrepareSetOutputs();
  std::vector<std::pair<GNode, int32_t>> output_nodes;
  for (const auto &item : output_indexes) {
    const graphStatus append_ret = AppendUserOutput(item.first, item.second, output_nodes);
    if (append_ret != GRAPH_SUCCESS) {
      return append_ret;
    }
  }
  FinalizeUserOutputNames();
  const auto *output_names = GetUserOutputNames();
  if (output_names != nullptr) {
    SetUserDefinedOutput(*output_names);
  }
  SetOutputSize(static_cast<uint32_t>(output_indexes.size()));
  GE_ASSERT_SUCCESS(SetGraphOutNodesInfo(output_nodes));
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::SetOutputs(const std::vector<std::pair<Operator, std::string>> &outputs) {
  if (!IsValid()) {
    const char *const graph_name = IsComputeGraphKind() ? "ComputeGraph" : "ExecuteGraph";
    REPORT_INNER_ERR_MSG("E19999", "[Check][Param] set %s failed.", graph_name);
    GELOGE(GRAPH_FAILED, "[Check][Param] set %s failed.", graph_name);
    return GRAPH_FAILED;
  }
  if (outputs.empty()) {
    GELOGI("set outputs size is 0.");
    return GRAPH_SUCCESS;
  }
  PrepareSetOutputs();
  std::vector<std::pair<GNode, int32_t>> output_nodes;
  for (const auto &item : outputs) {
    const graphStatus ret = AppendNamedUserOutput(item.first, item.second, output_nodes);
    if (ret != GRAPH_SUCCESS) {
      return ret;
    }
  }
  FinalizeUserOutputNames();
  SetOutputSize(static_cast<uint32_t>(outputs.size()));
  GE_ASSERT_SUCCESS(SetGraphOutNodesInfo(output_nodes));
  GELOGI("********************SetOutputs Success***********************");
  const auto *output_names = GetUserOutputNames();
  if ((output_names != nullptr) && !output_names->empty()) {
    GELOGI(" NetOutputs: (%s)", output_names->c_str());
  }
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::SetOutputs(const std::vector<std::pair<Operator, AscendString>> &outputs) {
  std::vector<std::pair<Operator, std::string>> graph_outputs;
  for (const auto &item : outputs) {
    const char_t *const name = item.second.GetString();
    if (name != nullptr) {
      graph_outputs.emplace_back(item.first, name);
    } else {
      GELOGW("[SetOutputs][CheckParam] Input output_op_name is nullptr.");
    }
  }
  return SetOutputs(graph_outputs);
}

graphStatus GraphImpl::SetOutputs(const std::vector<std::pair<GNode, int32_t>> &outputs) {
  if (!IsValid()) {
    const char *const msg =
        IsComputeGraphKind() ? "Assert ((compute_graph_) != nullptr) failed" : "Inner execute graph is nullptr";
    REPORT_INNER_ERR_MSG("E19999", "%s", msg);
    GELOGE(ge::FAILED, "%s", msg);
    return PARAM_INVALID;
  }
  PrepareSetOutputs();
  GE_ASSERT_SUCCESS(SetGraphOutNodesInfo(outputs));
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::AppendUserOutput(const Operator &output, const std::vector<size_t> &indexes,
                                        std::vector<std::pair<GNode, int32_t>> &output_nodes) {
  AscendString out_name;
  (void)output.GetName(out_name);
  bool found = false;
  const auto node = FindDirectNode(out_name, found);
  if (!found) {
    GELOGW("[SetOutputs][Check] User designated out_node %s does not exist in graph, skip it", out_name.GetString());
    return GRAPH_SUCCESS;
  }
  if (node == nullptr) {
    GELOGE(GRAPH_FAILED, "[SetOutputs][Get][GNode] wrap out_node (%s) to gnode failed in graph:%s.",
           out_name.GetString(), GetName().c_str());
    REPORT_INNER_ERR_MSG("E18888", "wrap out_node (%s) to gnode failed in graph:%s.", out_name.GetString(),
                         GetName().c_str());
    return GRAPH_FAILED;
  }
  const auto op_desc = GetOutputOpDesc(*node);
  if (op_desc == nullptr) {
    GELOGE(GRAPH_FAILED, "op_desc in node must not be null.");
    return GRAPH_SUCCESS;
  }
  const size_t output_size = op_desc->GetOutputsSize();
  if (indexes.empty()) {
    for (size_t i = 0U; i < output_size; ++i) {
      AppendUserOutputName(out_name, i);
      output_nodes.emplace_back(*node, static_cast<int32_t>(i));
    }
  } else {
    for (size_t i = 0U; i < indexes.size(); ++i) {
      if (indexes[i] >= output_size) {
        GELOGW("[SetOutputs][Check] User designated out_node %s has no output %zu, output_size=%zu, skip it",
               out_name.GetString(), indexes[i], output_size);
      } else {
        // Preserve the existing positional naming rule for explicitly indexed outputs.
        AppendUserOutputName(out_name, i);
        output_nodes.emplace_back(*node, static_cast<int32_t>(indexes[i]));
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus GraphImpl::AppendNamedUserOutput(const Operator &output, const std::string &output_name,
                                             std::vector<std::pair<GNode, int32_t>> &output_nodes) {
  AscendString out_name;
  (void)output.GetName(out_name);
  bool found = false;
  const auto node = FindDirectNode(out_name, found);
  if (!found) {
    REPORT_INNER_ERR_MSG("E18888", "designated out_node (%s) does not exist in graph:%s, this out_node ignored!",
                         out_name.GetString(), GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Check][Param] Warning, user designated out_node (%s) does not exist in graph:%s, "
           "this out_node ignored!",
           out_name.GetString(), GetName().c_str());
    return GRAPH_FAILED;
  }
  if (node == nullptr) {
    GELOGE(GRAPH_FAILED, "[SetOutputs][Get][GNode] wrap out_node (%s) to gnode failed in graph:%s.",
           out_name.GetString(), GetName().c_str());
    REPORT_INNER_ERR_MSG("E18888", "wrap out_node (%s) to gnode failed in graph:%s.", out_name.GetString(),
                         GetName().c_str());
    return GRAPH_FAILED;
  }
  const auto op_desc = GetOutputOpDesc(*node);
  if (op_desc == nullptr) {
    GELOGE(GRAPH_FAILED, "op_desc_ptr in node must not be null.");
    return GRAPH_SUCCESS;
  }
  const size_t output_size = op_desc->GetOutputsSize();
  if (output_name.empty()) {
    for (size_t i = 0U; i < output_size; ++i) {
      AppendUserOutputName(out_name, i);
      output_nodes.emplace_back(*node, static_cast<int32_t>(i));
    }
  } else {
    const int32_t index = op_desc->GetOutputIndexByName(output_name);
    if (index < 0) {
      REPORT_INNER_ERR_MSG("E18888",
                           "user designated out_node (%s):(%s) does not exist in graph:%s, this out_node ignored!",
                           out_name.GetString(), output_name.c_str(), GetName().c_str());
      GELOGE(GRAPH_FAILED,
             "[Check][Param] Warning, user designated out_node (%s):(%s) does not exist in graph:%s, "
             "this out_node ignored!",
             out_name.GetString(), output_name.c_str(), GetName().c_str());
      return GRAPH_FAILED;
    }
    AppendUserOutputName(out_name, static_cast<size_t>(index));
    output_nodes.emplace_back(*node, index);
  }
  return GRAPH_SUCCESS;
}

void GraphImpl::AppendUserOutputName(const AscendString &name, size_t index) {
  auto *output_names = GetUserOutputNames();
  if (output_names != nullptr) {
    *output_names += std::string(name.GetString()) + ":" + std::to_string(index) + ";";
  }
}

void GraphImpl::FinalizeUserOutputNames() {
  auto *output_names = GetUserOutputNames();
  if ((output_names != nullptr) && !output_names->empty()) {
    *output_names = output_names->substr(0U, output_names->length() - 1U);
  }
}
}  // namespace ge
