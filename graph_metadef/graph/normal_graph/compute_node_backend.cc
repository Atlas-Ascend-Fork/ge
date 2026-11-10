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
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {

NodeImpl::NodeContext ComputeNodeBackend::ResolveNode() const {
  NodeContext node;
  node.compute_node = node_ptr_.lock();
  if (node.compute_node != nullptr) {
    node.op_desc = node.compute_node->GetOpDesc();
  }
  return node;
}

Operator ComputeNodeBackend::CreateOperator(const NodeContext &node) const {
  return OpDescUtils::CreateOperatorFromNode(node.compute_node);
}

NodeImpl::GraphContext ComputeNodeBackend::LookupSubgraph(const NodeContext &node, uint32_t index) const {
  return {NodeUtils::GetSubgraph(*node.compute_node, index), nullptr};
}

NodeImpl::GraphContext ComputeNodeBackend::LookupRootGraph(const NodeContext &node) const {
  return {GraphUtils::FindRootGraph(node.compute_node->GetOwnerComputeGraph()), nullptr};
}

std::vector<NodeImpl::GraphContext> ComputeNodeBackend::ListSubgraphs(const GraphContext &root) const {
  std::vector<GraphContext> subgraphs;
  for (const auto &graph : root.compute_graph->GetAllSubgraphs()) {
    subgraphs.push_back({graph, nullptr});
  }
  return subgraphs;
}

GraphPtr ComputeNodeBackend::WrapGraph(const GraphContext &graph) const {
  return GraphUtilsEx::CreateGraphPtrFromComputeGraph(graph.compute_graph);
}

graphStatus ComputeNodeBackend::ResolveConstInput(const NodeContext &node, int32_t index, NodeContext &source) const {
  const NodePtr input_data_node = NodeUtils::GetInDataNodeByIndex(*node.compute_node, index);
  GE_CHECK_NOTNULL(input_data_node);
  const std::string op_type = input_data_node->GetType();
  if ((op_type == CONSTANT) || (op_type == CONSTANTOP)) {
    source.compute_node = input_data_node;
  } else if (op_type == DATA) {
    auto parent_node = NodeUtils::GetParentInput(input_data_node);
    while ((parent_node != nullptr) && (parent_node->GetType() == DATA)) {
      parent_node = NodeUtils::GetParentInput(parent_node);
    }
    if ((parent_node != nullptr) && ((parent_node->GetType() == CONSTANT) || (parent_node->GetType() == CONSTANTOP))) {
      source.compute_node = parent_node;
    }
  }
  if (source.compute_node == nullptr) {
    return GRAPH_NODE_WITHOUT_CONST_INPUT;
  }
  source.op_desc = source.compute_node->GetOpDesc();
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

std::vector<GNodePtr> ComputeNodeBackend::GetInControlNodes() const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Gnode node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: node impl is nullptr.");
    return {};
  }

  std::vector<GNodePtr> gnodes;
  const auto in_control_nodes = node_ptr->GetInControlNodes();
  for (auto &in_control_node : in_control_nodes) {
    GNodePtr gnode = NodeAdapter::Node2GNodePtr(in_control_node);
    if (gnode == nullptr) {
      GELOGE(GRAPH_FAILED, "[Get][GNode] In control_node of node[%s] to gnode failed.", node_ptr->GetName().c_str());
      return {};
    }
    gnodes.emplace_back(gnode);
  }

  return gnodes;
}

std::vector<GNodePtr> ComputeNodeBackend::GetOutControlNodes() const {
  const std::shared_ptr<Node> node_ptr = node_ptr_.lock();
  if (node_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "GetOutControlNodes node impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetOutControlNodes: node impl is nullptr.");
    return {};
  }

  std::vector<GNodePtr> gnodes;
  const auto out_control_nodes = node_ptr->GetOutControlNodes();
  for (auto &out_control_node : out_control_nodes) {
    GNodePtr gnode = NodeAdapter::Node2GNodePtr(out_control_node);
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

}  // namespace ge
