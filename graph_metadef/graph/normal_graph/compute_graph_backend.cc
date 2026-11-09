/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/compute_graph_backend.h"

#include "base/err_msg.h"
#include "common/checker.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/graph_buffer.h"
#include "graph/model.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/readable_dump.h"
#include "utils/tensor_adapter.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "proto/onnx/ge_onnx.pb.h"
#include "utils/ge_ir_utils.h"

namespace ge {
ComputeGraphBackend::ComputeGraphBackend(const std::string &name) : name_(name) {}

ComputeGraphBackend::~ComputeGraphBackend() {
  if (IsValid()) {
    if (compute_graph_ != nullptr) {
      GraphUtilsEx::BreakConnect(compute_graph_->GetAllNodesInfo());
    }
  }
  for (const auto &it : op_list_) {
    const Operator op = it.second;
    op.BreakConnect();
  }
}

bool ComputeGraphBackend::IsValid() const {
  return compute_graph_ != nullptr;
}

ComputeGraphPtr ComputeGraphBackend::GetComputeGraph() const {
  return compute_graph_;
}

const std::string &ComputeGraphBackend::GetName() const {
  return name_;
}

graphStatus ComputeGraphBackend::InitGraph() {
  GE_WARN_ASSERT(compute_graph_ == nullptr, "No need to init again");
  compute_graph_ = ge::ComGraphMakeSharedAndThrow<ComputeGraph>(name_);
  return GRAPH_SUCCESS;
}

void ComputeGraphBackend::AdoptGraph(const ComputeGraphPtr &compute_graph) {
  compute_graph_ = compute_graph;
}

graphStatus ComputeGraphBackend::SetInputs(const std::vector<Operator> &inputs) {
  GE_ASSERT(!IsValid(),
            "Inner graph has been inited, maybe you call `SetInputs` again or call `AddNodeByOp or "
            "`SetValid` before `SetInputs`");
  compute_graph_ = GraphUtilsEx::CreateGraphFromOperator(name_, inputs);
  GE_CHK_BOOL_RET_STATUS(compute_graph_ != nullptr, GRAPH_FAILED, "[Build][Graph] failed.");
  GE_CHK_BOOL_RET_STATUS(inputs.size() != 0U, GRAPH_FAILED, "[Check][Param] set input NULL.");
  compute_graph_->SetInputSize(static_cast<uint32_t>(inputs.size()));
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::SetOutputs(const std::vector<Operator> &outputs) {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "compute graph is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] set ComputeGraph failed.");
    return GRAPH_FAILED;
  }
  if (outputs.empty()) {
    GELOGI("Set outputs size is 0.");
    return GRAPH_SUCCESS;
  }

  // Construct special output node
  std::vector<std::pair<Operator, std::vector<size_t>>> output_indexs;
  for (size_t i = 0U; i < outputs.size(); ++i) {
    output_indexs.emplace_back(outputs[i], std::vector<size_t>{});
  }

  const graphStatus ret = SetOutputs(output_indexs);
  return ret;
}

graphStatus ComputeGraphBackend::SetOutputs(
    const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexs) {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "compute graph is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] set ComputeGraph failed.");
    return GRAPH_FAILED;
  }
  if (output_indexs.empty()) {
    GELOGW("[SetOutputs][CheckParam] Set outputs size is 0.");
    return GRAPH_SUCCESS;
  }

  // Construct special output node
  std::vector<std::pair<ge::NodePtr, int32_t>> output_nodes;
  for (const auto &item : output_indexs) {
    AppendUserOutput(item.first, item.second, output_nodes);
  }

  // Del last ";"
  if (!output_name_.empty()) {
    output_name_ = output_name_.substr(0U, output_name_.length() - 1U);
  }
  compute_graph_->SetUserDefOutput(output_name_);
  compute_graph_->SetOutputSize(static_cast<uint32_t>(output_indexs.size()));
  GE_ASSERT_SUCCESS(compute_graph_->SetGraphOutNodesInfo(output_nodes));
  return GRAPH_SUCCESS;
}

// 处理单个按索引指定的输出项：定位节点并按索引装配输出名与输出节点（原循环体原样搬迁）
void ComputeGraphBackend::AppendUserOutput(const Operator &output, const std::vector<size_t> &indexs,
                                           std::vector<std::pair<ge::NodePtr, int32_t>> &output_nodes) {
  AscendString out_name;
  (void)output.GetName(out_name);
  ge::NodePtr node = compute_graph_->FindNode(out_name.GetString());
  if (node == nullptr) {
    GELOGW("[SetOutputs][Check] User designated out_node %s does not exist in graph, skip it", out_name.GetString());
    return;
  }

  const ge::OpDescPtr tmp_op_ptr = node->GetOpDesc();
  if (tmp_op_ptr == nullptr) {
    GELOGE(GRAPH_FAILED, "op_desc in node must not be null.");
    return;
  }
  const size_t out_size = tmp_op_ptr->GetOutputsSize();
  if (indexs.empty()) {
    for (size_t i = 0U; i < out_size; ++i) {
      output_name_ += std::string(out_name.GetString()) + ":" + std::to_string(i) + ";";
      output_nodes.emplace_back(node, i);
    }
  } else {
    for (size_t i = 0U; i < indexs.size(); ++i) {
      if (indexs[i] >= out_size) {
        GELOGW("[SetOutputs][Check] User designated out_node %s has no output %zu, output_size=%zu, skip it",
               out_name.GetString(), indexs[i], out_size);
      } else {
        output_name_ += std::string(out_name.GetString()) + ":" + std::to_string(i) + ";";
        output_nodes.emplace_back(node, indexs[i]);
      }
    }
  }
}

graphStatus ComputeGraphBackend::SetOutputs(const std::vector<std::pair<Operator, std::string>> &outputs) {
  GE_CHK_BOOL_RET_STATUS(compute_graph_ != nullptr, GRAPH_FAILED, "[Check][Param] set ComputeGraph failed.");
  if (outputs.empty()) {
    GELOGI("set outputs size is 0.");
    return GRAPH_SUCCESS;
  }

  // Construct specified output
  std::vector<std::pair<ge::NodePtr, int32_t>> output_nodes;
  for (const auto &item : outputs) {
    const graphStatus ret = AppendNamedUserOutput(item.first, item.second, output_nodes);
    if (ret != GRAPH_SUCCESS) {
      return ret;
    }
  }
  // Del last ";"
  if (!output_name_.empty()) {
    output_name_ = output_name_.substr(0U, output_name_.length() - 1U);
  }
  compute_graph_->SetOutputSize(static_cast<uint32_t>(outputs.size()));
  GE_ASSERT_SUCCESS(compute_graph_->SetGraphOutNodesInfo(output_nodes));
  GELOGI("********************SetOutputs Success***********************");
  GE_IF_BOOL_EXEC(!output_name_.empty(), GELOGI(" NetOutputs: (%s)", output_name_.c_str()));

  return GRAPH_SUCCESS;
}

// 处理单个按名称指定的输出项（原循环体原样搬迁）：节点缺失或命名索引非法时整体失败
graphStatus ComputeGraphBackend::AppendNamedUserOutput(const Operator &output, const std::string &output_name,
                                                       std::vector<std::pair<ge::NodePtr, int32_t>> &output_nodes) {
  AscendString out_name;
  (void)output.GetName(out_name);
  ge::NodePtr node = compute_graph_->FindNode(out_name.GetString());
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "designated out_node (%s) does not exist in graph:%s, this out_node ignored!",
                         out_name.GetString(), compute_graph_->GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Check][Param] Warning, user designated out_node (%s) does not exist in graph:%s, "
           "this out_node ignored!",
           out_name.GetString(), compute_graph_->GetName().c_str());
    return GRAPH_FAILED;
  }
  const ge::OpDescPtr tmp_op_ptr = node->GetOpDesc();
  if (tmp_op_ptr == nullptr) {
    GELOGE(GRAPH_FAILED, "op_desc_ptr in node must not be null.");
    return GRAPH_SUCCESS;
  }
  const size_t out_size = tmp_op_ptr->GetOutputsSize();

  if (output_name.empty()) {
    for (size_t i = 0U; i < out_size; ++i) {
      output_name_ += std::string(out_name.GetString()) + ":" + std::to_string(i) + ";";
      output_nodes.emplace_back(node, i);
    }
  } else {
    int32_t index = tmp_op_ptr->GetOutputIndexByName(output_name);
    if (index < 0) {
      REPORT_INNER_ERR_MSG("E18888",
                           "user designated out_node (%s):(%s) does not exist in graph:%s, "
                           "this out_node ignored!",
                           out_name.GetString(), output_name.c_str(), compute_graph_->GetName().c_str());
      GELOGE(GRAPH_FAILED,
             "[Check][Param] Warning, user designated out_node (%s):(%s) does not exist in graph:%s, "
             "this out_node ignored!",
             out_name.GetString(), output_name.c_str(), compute_graph_->GetName().c_str());
      return GRAPH_FAILED;
    }
    output_name_ += std::string(out_name.GetString()) + ":" + std::to_string(index) + ";";
    output_nodes.emplace_back(node, index);
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::SetOutputs(const std::vector<std::pair<GNode, int32_t>> &outputs) {
  GE_ASSERT_NOTNULL(compute_graph_);
  std::vector<std::pair<ge::NodePtr, int32_t>> output_node_and_index;
  for (const auto &item : outputs) {
    output_node_and_index.emplace_back(NodeAdapter::GNode2Node(item.first), item.second);
  }
  GE_ASSERT_SUCCESS(compute_graph_->SetGraphOutNodesInfo(output_node_and_index));
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::SetTargets(const std::vector<Operator> &targets) {
  GE_CHK_BOOL_RET_STATUS(compute_graph_ != nullptr, GRAPH_FAILED, "[Check][Param] set ComputeGraph failed.");
  if (targets.empty()) {
    GELOGI("set targets size is 0.");
    return GRAPH_SUCCESS;
  }

  std::vector<ge::NodePtr> target_nodes;
  for (const auto &item : targets) {
    AscendString name;
    (void)item.GetName(name);
    const ge::NodePtr node = compute_graph_->FindNode(name.GetString());
    if (node == nullptr) {
      GELOGW("[SetTargets][Check] User designated target_node %s does not exist in graph, skip it", name.GetString());
      continue;
    }
    target_nodes.push_back(node);
  }
  GE_ASSERT_SUCCESS(compute_graph_->SetGraphTargetNodesInfo(target_nodes));
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::AddOp(const ge::Operator &op) {
  AscendString name;
  (void)op.GetName(name);
  const auto ret = op_list_.emplace(std::pair<std::string, ge::Operator>(name.GetString(), op));
  if (!ret.second) {
    GELOGW("[Check][Param] the op have added before, op name:%s.", name.GetString());
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::GetAllOpName(std::vector<std::string> &op_name) const {
  for (const auto &it : op_list_) {
    AscendString name;
    it.second.GetName(name);
    op_name.emplace_back(name.GetString());
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::FindOpByName(const std::string &name, ge::Operator &op) const {
  const auto it = op_list_.find(name);
  GE_CHK_BOOL_EXEC(it != op_list_.end(), REPORT_INNER_ERR_MSG("E18888", "there is no op: %s.", name.c_str());
                   return GRAPH_FAILED, "[Find][Op] there is no op: %s.", name.c_str());
  op = it->second;
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const {
  for (auto &op : op_list_) {
    AscendString op_type;
    (void)op.second.GetOpType(op_type);
    if (op_type.GetString() == type) {
      ops.push_back(op.second);
      continue;
    }
    if (op_type == ge::FRAMEWORKOP) {
      (void)op.second.GetAttr(ge::ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE.c_str(), op_type);
      if (op_type.GetString() == type) {
        ops.push_back(op.second);
      }
    }
  }
  return GRAPH_SUCCESS;
}

void ComputeGraphBackend::SetNeedIteration(bool need_iteration) {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Set need iteration failed, as compute graph is null.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Set need iteration failed, as compute graph is null.");
    return;
  }
  compute_graph_->SetNeedIteration(need_iteration);
}

std::vector<GNode> ComputeGraphBackend::GetAllNodes() const {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl compute graph is nullptr.");
    GELOGE(GRAPH_FAILED, "[Get][Graph] GetAllNodes: compute graph ptr is nullptr, graph %s", name_.c_str());
    return {};
  }
  return WrapNodes(compute_graph_->GetAllNodes());
}

std::vector<GNode> ComputeGraphBackend::GetDirectNode() const {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl compute graph is nullptr.");
    GELOGE(GRAPH_FAILED, "[Get][Graph] GetDirectNode: compute graph ptr is nullptr, graph %s", name_.c_str());
    return {};
  }
  return WrapNodes(compute_graph_->GetDirectNode());
}

GNode ComputeGraphBackend::AddNodeByOp(const Operator &op) {
  const std::shared_ptr<ge::OpDesc> op_desc = ge::OpDescUtils::GetOpDescFromOperator(op);
  if (op_desc == nullptr) {
    AscendString name;
    (void)op.GetName(name);
    REPORT_INNER_ERR_MSG("E18888", "get op desc from op:%s failed", name.GetString());
    GELOGE(GRAPH_FAILED, "[Get][OpDesc] from op[%s] failed.", name.GetString());
    return GNode();
  }

  GE_ASSERT_GRAPH_SUCCESS(SetValid());
  const NodePtr node_ptr = compute_graph_->AddNode(op_desc);
  const GNode gnode = NodeAdapter::Node2GNode(node_ptr);

  // 同步维护op_list_，使FindOpByName/FindOpByType/GetAllOpName等接口在ES构图(AddNodeByOp)场景下可用。
  // 静默忽略重名以保证幂等性（调用方可能已通过AddOp手动添加过同名Operator）。
  (void)AddOp(op);
  return gnode;
}

graphStatus ComputeGraphBackend::AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                             const int32_t dst_port_index) {
  const NodePtr src_node_ptr = NodeAdapter::GNode2Node(src_node);
  if (src_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] src gnode to node failed.");
    return GRAPH_FAILED;
  }

  const NodePtr dst_node_ptr = NodeAdapter::GNode2Node(dst_node);
  if (dst_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] dst gnode to node failed.");
    return GRAPH_FAILED;
  }

  if (src_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src node[%s] owner compute graph is nullptr.", src_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] src node[%s] owner compute graph is nullptr.", src_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (dst_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst node[%s] owner compute graph is nullptr.", dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] dst node[%s] owner compute graph is nullptr.", dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const graphStatus res = GraphUtils::AddEdge(src_node_ptr->GetOutDataAnchor(src_port_index),
                                              dst_node_ptr->GetInDataAnchor(dst_port_index));
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "add data edge from %s(%d) to %s(%d) failed.", src_node_ptr->GetName().c_str(),
                         src_port_index, dst_node_ptr->GetName().c_str(), dst_port_index);
    GELOGE(GRAPH_FAILED, "[Add][DataEdge] from %s(%d) to %s(%d) failed.", src_node_ptr->GetName().c_str(),
           src_port_index, dst_node_ptr->GetName().c_str(), dst_port_index);
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::AddControlEdge(GNode &src_node, GNode &dst_node) {
  const NodePtr src_node_ptr = NodeAdapter::GNode2Node(src_node);
  if (src_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] src gnode to node failed.");
    return GRAPH_FAILED;
  }

  const NodePtr dst_node_ptr = NodeAdapter::GNode2Node(dst_node);
  if (dst_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] dst gnode to node failed.");
    return GRAPH_FAILED;
  }

  if (src_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src node[%s] owner compute graph is nullptr.", src_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] src node[%s] owner compute graph is nullptr.", src_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (dst_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst node[%s] owner compute graph is nullptr.", dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] dst node[%s] owner compute graph is nullptr.", dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  const graphStatus res = GraphUtils::AddEdge(src_node_ptr->GetOutControlAnchor(), dst_node_ptr->GetInControlAnchor());
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "add control edge from %s to %s failed.", src_node_ptr->GetName().c_str(),
                         dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Add][ControlEdge] from %s to %s failed.", src_node_ptr->GetName().c_str(),
           dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  return SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveNode(GNode &node, bool contain_subgraph) {
  const NodePtr node_ptr = NodeAdapter::GNode2Node(node);
  GE_CHECK_NOTNULL(node_ptr);

  const ComputeGraphPtr owner_compute_graph = node_ptr->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(owner_compute_graph);

  ComputeGraphPtr compute_graph_ptr = compute_graph_;
  GE_CHECK_NOTNULL(compute_graph_ptr);

  if (contain_subgraph) {
    if (!GraphUtils::IsNodeInGraphRecursively(compute_graph_ptr, *node_ptr)) {
      REPORT_INNER_ERR_MSG("E18888", "node[%s] is not in the graph[%s] or not in subgraph.",
                           node_ptr->GetName().c_str(), compute_graph_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Check][Param] node[%s] is not in the graph[%s].", node_ptr->GetName().c_str(),
             compute_graph_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
    compute_graph_ptr = owner_compute_graph;
  } else {
    if (compute_graph_ptr != owner_compute_graph) {
      REPORT_INNER_ERR_MSG("E18888", "node[%s] is not in the graph[%s].", node_ptr->GetName().c_str(),
                           compute_graph_ptr->GetName().c_str());
      GELOGE(GRAPH_FAILED, "[Check][Param] node[%s] is not in the graph[%s].", node_ptr->GetName().c_str(),
             compute_graph_ptr->GetName().c_str());
      return GRAPH_FAILED;
    }
  }

  ge::NodeUtils::UnlinkAll(*node_ptr);
  if (GraphUtils::RemoveNodeWithoutRelink(compute_graph_ptr, node_ptr) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "graph:%s remove node:%s failed", compute_graph_ptr->GetName().c_str(),
                         node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Remove][Node] %s from graph:%s failed.", node_ptr->GetName().c_str(),
           compute_graph_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  (void)node_ptr->ClearOwnerGraph(nullptr);
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                            const int32_t dst_port_index) {
  if ((src_port_index == -1) && (dst_port_index != -1)) {
    REPORT_INNER_ERR_MSG("E18888", "src_port_index == -1 and dst_port_index != -1, check invalid .");
    GELOGE(GRAPH_FAILED, "[Check][Param] src control anchor link to dst data anchor does not exist.");
    return GRAPH_FAILED;
  }

  const NodePtr src_node_ptr = NodeAdapter::GNode2Node(src_node);
  if (src_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] src gnode to node failed.");
    return GRAPH_FAILED;
  }

  const NodePtr dst_node_ptr = NodeAdapter::GNode2Node(dst_node);
  if (dst_node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst gnode to node failed.");
    GELOGE(GRAPH_FAILED, "[Get][Node] dst gnode to node failed.");
    return GRAPH_FAILED;
  }

  if (src_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src node:%s compute graph is nullptr.", src_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] src node:%s compute graph is nullptr.", src_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (dst_node_ptr->GetOwnerComputeGraph() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "dst node:%s compute graph is nullptr", dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Graph] dst node:%s compute graph is nullptr.", dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }

  if (RemoveEdgeByAnchor(src_node_ptr, src_port_index, dst_node_ptr, dst_port_index) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "remove edge between %s(%d) and %s(%d) failed.", src_node_ptr->GetName().c_str(),
                         src_port_index, dst_node_ptr->GetName().c_str(), dst_port_index);
    GELOGE(GRAPH_FAILED, "[Remove][Edge] between %s(%d) and %s(%d) failed.", src_node_ptr->GetName().c_str(),
           src_port_index, dst_node_ptr->GetName().c_str(), dst_port_index);
    return GRAPH_FAILED;
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveEdgeByAnchor(const NodePtr &src_node_ptr, const int32_t src_port_index,
                                                    const NodePtr &dst_node_ptr, const int32_t dst_port_index) {
  if ((src_port_index == -1) && (dst_port_index == -1)) {
    return RemoveControlEdgeByAnchor(src_node_ptr, dst_node_ptr);
  }

  if (src_node_ptr->GetOutDataAnchor(src_port_index) == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src node[%s] out data anchor[%d] is null.", src_node_ptr->GetName().c_str(),
                         src_port_index);
    GELOGE(GRAPH_FAILED, "[Get][Anchor] src node[%s] out data anchor[%d] is null.", src_node_ptr->GetName().c_str(),
           src_port_index);
    return GRAPH_FAILED;
  }

  if ((src_port_index != -1) && (dst_port_index == -1)) {
    return RemoveDataControlEdgeByAnchor(src_node_ptr, src_port_index, dst_node_ptr);
  }

  return RemoveDataEdgeByAnchor(src_node_ptr, src_port_index, dst_node_ptr, dst_port_index);
}

graphStatus ComputeGraphBackend::RemoveControlEdgeByAnchor(const NodePtr &src_node_ptr, const NodePtr &dst_node_ptr) {
  if (src_node_ptr->GetOutControlAnchor() == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "src node:%s out control anchor is null.", src_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Get][Anchor] src node:%s out control anchor is null.", src_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  const graphStatus res =
      GraphUtils::RemoveEdge(src_node_ptr->GetOutControlAnchor(), dst_node_ptr->GetInControlAnchor());
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "remove control edge between [%s] and [%s] failed.", src_node_ptr->GetName().c_str(),
                         dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Remove][ControlEdge] between [%s] and [%s] failed.", src_node_ptr->GetName().c_str(),
           dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveDataControlEdgeByAnchor(const NodePtr &src_node_ptr,
                                                               const int32_t src_port_index,
                                                               const NodePtr &dst_node_ptr) {
  const graphStatus res =
      GraphUtils::RemoveEdge(src_node_ptr->GetOutDataAnchor(src_port_index), dst_node_ptr->GetInControlAnchor());
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "remove data-control edge between [%s] and [%s] failed.",
                         src_node_ptr->GetName().c_str(), dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Remove][Edge] between [%s] and [%s] failed.", src_node_ptr->GetName().c_str(),
           dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveDataEdgeByAnchor(const NodePtr &src_node_ptr, const int32_t src_port_index,
                                                        const NodePtr &dst_node_ptr, const int32_t dst_port_index) {
  const graphStatus res = GraphUtils::RemoveEdge(src_node_ptr->GetOutDataAnchor(src_port_index),
                                                 dst_node_ptr->GetInDataAnchor(dst_port_index));
  if (res != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "remove data edge between [%s] and [%s] failed.", src_node_ptr->GetName().c_str(),
                         dst_node_ptr->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Remove][Edge] between [%s] and [%s] failed.", src_node_ptr->GetName().c_str(),
           dst_node_ptr->GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

GNodePtr ComputeGraphBackend::FindNodeByName(const AscendString &node_name) const {
  const auto node = compute_graph_->FindNode(node_name.GetString());
  GE_WARN_ASSERT(node != nullptr, "Node name: %s was not found in the current graph: %s.", node_name.GetString(),
                 name_.c_str());
  return NodeAdapter::Node2GNodePtr(node);
}

std::vector<GraphPtr> ComputeGraphBackend::GetAllSubgraphs() const {
  std::vector<GraphPtr> subgraphs;
  auto sub_compute_graphs = compute_graph_->GetAllSubgraphs();
  subgraphs.reserve(sub_compute_graphs.size());
  for (const auto &sub_compute_graph : sub_compute_graphs) {
    subgraphs.emplace_back(GraphUtilsEx::CreateGraphPtrFromComputeGraph(sub_compute_graph));
  }
  return subgraphs;
}

GraphPtr ComputeGraphBackend::GetSubGraph(const char *name) const {
  const auto sub_compute_graph = compute_graph_->GetSubgraph(name);
  return sub_compute_graph != nullptr ? GraphUtilsEx::CreateGraphPtrFromComputeGraph(sub_compute_graph) : nullptr;
}

graphStatus ComputeGraphBackend::AddSubGraph(const Graph &subgraph) {
  const ComputeGraphPtr sub_compute_graph = GraphUtilsEx::GetComputeGraph(subgraph);
  GE_ASSERT_NOTNULL(sub_compute_graph, "Compute graph cannot adopt an execution subgraph");
  if (compute_graph_->AddSubgraph(sub_compute_graph) != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "graph:%s add subgraph:%s failed", compute_graph_->GetName().c_str(),
                         sub_compute_graph->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Add][Subgraph] %s to graph:%s failed.", sub_compute_graph->GetName().c_str(),
           compute_graph_->GetName().c_str());
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RemoveSubgraph(const char *name) {
  // 既有行为：删除失败不回传状态，恒返回成功
  (void)compute_graph_->RemoveSubgraph(name);
  return GRAPH_SUCCESS;
}

GraphPtr ComputeGraphBackend::GetParentGraph() const {
  const auto parent_compute_graph_ptr = compute_graph_->GetParentGraph();
  GE_WARN_ASSERT(parent_compute_graph_ptr != nullptr, "Graph %s has no parent graph.", name_.c_str());
  return GraphUtilsEx::CreateGraphPtrFromComputeGraph(parent_compute_graph_ptr);
}

GNodePtr ComputeGraphBackend::GetParentNode() const {
  const auto parent_graph_node_ptr = compute_graph_->GetParentNode();
  GE_WARN_ASSERT(parent_graph_node_ptr != nullptr, "Graph %s has no parent node.", name_.c_str());
  return NodeAdapter::Node2GNodePtr(parent_graph_node_ptr);
}

graphStatus ComputeGraphBackend::SetAttr(const AscendString &name, const AttrValue &attr_value) {
  GE_ASSERT_NOTNULL(compute_graph_, "Inner source is not ready, call `SetInputs` or `SetValid` at first");
  return compute_graph_->SetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus ComputeGraphBackend::GetAttr(const AscendString &name, AttrValue &attr_value) const {
  GE_ASSERT_NOTNULL(compute_graph_, "Inner source is not ready, call `SetInputs` or `SetValid` at first");
  return compute_graph_->GetAttr(name.GetString(), attr_value.impl->MutableAnyValue());
}

graphStatus ComputeGraphBackend::CopyOperatorsTo(GraphImpl &dst_impl,
                                                 const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                                                 const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const {
  // 前提：同类别拷贝，由 Graph::CopyFrom 门面保证；执行图接入时在 CopyFrom 层拒绝跨类别
  auto &dst = static_cast<ComputeGraphBackend &>(dst_impl);
  std::map<std::string, ge::Operator> &dst_op_list = dst.op_list_;
  const std::map<std::string, ge::Operator> &src_op_list = op_list_;
  auto &dst_compute_graph = dst.compute_graph_;

  dst.output_name_ = output_name_;

  auto ret = OpDescUtils::CopyOperators(dst_compute_graph, node_old_2_new, op_desc_old_2_new, src_op_list, dst_op_list);
  if (ret != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "copy operators to graph:%s failed.", dst_compute_graph->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Copy][Operators] to graph:%s failed.", dst_compute_graph->GetName().c_str());
    return GRAPH_FAILED;
  }

  ret = OpDescUtils::CopyOperatorLinks(src_op_list, dst_op_list);
  if (ret != GRAPH_SUCCESS) {
    REPORT_INNER_ERR_MSG("E18888", "copy operator links failed, ret:%u.", ret);
    GELOGE(GRAPH_FAILED, "[Copy][OperatorLinks] failed, ret:%u.", ret);
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::RecoverOperators() {
  GE_CHECK_NOTNULL(compute_graph_);

  op_list_.clear();
  for (const auto &node : compute_graph_->GetDirectNode()) {
    op_list_[node->GetName()] = OpDescUtils::CreateOperatorFromNode(node);
  }
  return SUCCESS;
}

graphStatus ComputeGraphBackend::SaveToFile(const ComputeGraphPtr &compute_graph, const std::string &file_name) {
  Model model = Model();
  model.SetGraph(compute_graph);
  return model.SaveToFile(file_name);
}

graphStatus ComputeGraphBackend::SaveToMem(const ComputeGraphPtr &compute_graph, Buffer &buffer) {
  Model model = Model();
  model.SetGraph(compute_graph);
  GE_ASSERT_GRAPH_SUCCESS(model.Save(buffer), "Failed to save graph to memory.");
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::LoadFromFile(const std::string &file_name, ComputeGraphPtr &compute_graph) {
  Model model = Model();
  const graphStatus ret = model.LoadFromFile(file_name);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  compute_graph = model.GetGraph();
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::LoadFromMem(const uint8_t *data, const size_t len, ComputeGraphPtr &compute_graph) {
  Model model = Model();
  GE_ASSERT_GRAPH_SUCCESS(Model::Load(data, len, model), "Failed to load graph from memory.");
  compute_graph = model.GetGraph();
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::LoadFromSerializedModelArray(const void *serialized_model, const size_t size,
                                                              ComputeGraphPtr &compute_graph) {
  GE_ASSERT_NOTNULL(serialized_model, "param serialized_model is nullptr");
  GE_ASSERT(size != 0U, "param size is 0");
  Model model;
  GE_ASSERT_GRAPH_SUCCESS(Model::Load(static_cast<const uint8_t *>(serialized_model), size, model),
                          "Failed to load model from serialized model def.");
  GE_ASSERT_NOTNULL(model.GetGraph(), "Failed to get root graph from model.");
  compute_graph = model.GetGraph();
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::Dump(Graph::DumpFormat format, std::ostream &o_stream) const {
  GE_ASSERT_TRUE(IsValid());
  const auto compute_graph = compute_graph_;

  onnx::ModelProto onnx_model_proto;
  ge::proto::ModelDef txt_model_proto;
  google::protobuf::Message *proto = nullptr;
  switch (format) {
    case Graph::DumpFormat::kOnnx: {
      proto = &onnx_model_proto;
      GE_ASSERT_NOTNULL(proto);
      GE_ASSERT_GRAPH_SUCCESS(ConvertComputeGraphToProto(format, compute_graph, proto));
      GraphUtils::WriteProtoToOStream(*proto, o_stream);
      break;
    }
    case Graph::DumpFormat::kTxt: {
      proto = &txt_model_proto;
      GE_ASSERT_NOTNULL(proto);
      GE_ASSERT_GRAPH_SUCCESS(ConvertComputeGraphToProto(format, compute_graph, proto));
      GraphUtils::WriteProtoToOStream(*proto, o_stream);
      break;
    }
    case Graph::DumpFormat::kReadable: {
      std::stringstream readable_ss;
      GE_ASSERT_SUCCESS(ConvertComputeGraphToReadableStr(readable_ss, compute_graph));
      GraphUtils::WriteReadableDumpToOStream(readable_ss, o_stream);
    }
    default: {
      break;
    }
  }

  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const {
#ifdef FMK_SUPPORT_DUMP
  GE_ASSERT_TRUE(IsValid());
  const auto compute_graph = compute_graph_;

  onnx::ModelProto onnx_model_proto;
  ge::proto::ModelDef txt_model_proto;
  google::protobuf::Message *proto = nullptr;
  std::string file_absolut_name;
  switch (format) {
    case Graph::DumpFormat::kOnnx: {
      proto = &onnx_model_proto;
      GE_ASSERT_GRAPH_SUCCESS(GraphUtils::GenDumpOnnxFileName(compute_graph, suffix.GetString(), file_absolut_name));
      GE_ASSERT_NOTNULL(proto);
      GE_ASSERT_GRAPH_SUCCESS(ConvertComputeGraphToProto(format, compute_graph, proto));
      GraphUtils::WriteProtoToTextFile(*proto, file_absolut_name.c_str());
      break;
    }
    case Graph::DumpFormat::kTxt: {
      proto = &txt_model_proto;
      GE_ASSERT_GRAPH_SUCCESS(GraphUtils::GenDumpTxtFileName(compute_graph, suffix.GetString(), "", file_absolut_name));
      GE_ASSERT_NOTNULL(proto);
      GE_ASSERT_GRAPH_SUCCESS(ConvertComputeGraphToProto(format, compute_graph, proto));
      GraphUtils::WriteProtoToTextFile(*proto, file_absolut_name.c_str());
      break;
    }
    case Graph::DumpFormat::kReadable: {
      std::stringstream readable_ss;
      GE_ASSERT_GRAPH_SUCCESS(
          GraphUtils::GenDumpReadableTxtFileName(compute_graph, suffix.GetString(), "", file_absolut_name));
      GE_ASSERT_SUCCESS(ConvertComputeGraphToReadableStr(readable_ss, compute_graph));
      GraphUtils::WriteReadableDumpToTextFile(readable_ss, file_absolut_name.c_str());
      break;
    }
    default: {
      break;
    }
  }
  return SUCCESS;
#else
  (void)format;
  (void)suffix;
  GELOGW("[Graph][DumpToFile] Need to define FMK_SUPPORT_DUMP for dump graph.");
  return FAILED;
#endif
}

graphStatus ConvertComputeGraphToProto(Graph::DumpFormat format, const ComputeGraphPtr &compute_graph,
                                       google::protobuf::Message *const proto) {
  ge::Model model("", "");
  model.SetGraph(compute_graph);
  switch (format) {
    case Graph::DumpFormat::kOnnx: {
      model.SetName(compute_graph->GetName());
      auto *model_proto = reinterpret_cast<onnx::ModelProto *>(proto);
      GE_ASSERT_NOTNULL(model_proto);
      GE_ASSERT_TRUE(OnnxUtils::ConvertGeModelToModelProto(model, *model_proto, DumpLevel::DUMP_WITH_OUT_DESC),
                     "[Convert][GeModel] DumpGEGraphToOnnx failed.");
      break;
    }
    case Graph::DumpFormat::kTxt: {
      auto *model_proto = reinterpret_cast<ge::proto::ModelDef *>(proto);
      GE_ASSERT_NOTNULL(model_proto);
      GE_ASSERT_GRAPH_SUCCESS(model.Save(*model_proto, true));
      break;
    }
    case Graph::DumpFormat::kReadable: {
      break;
    }
    default: {
      break;
    }
  }
  return SUCCESS;
}

graphStatus ConvertComputeGraphToReadableStr(std::stringstream &readable_ss, const ComputeGraphPtr &compute_graph) {
  return ReadableDump::GenReadableDump(readable_ss, compute_graph) == SUCCESS ? GRAPH_SUCCESS : GRAPH_FAILED;
}

}  // namespace ge
