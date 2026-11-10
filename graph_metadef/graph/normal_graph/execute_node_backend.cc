/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/execute_node_backend.h"
#include "common/checker.h"
#include "exe_graph/lowering/builtin_node_types.h"

#include "graph/utils/execute_graph_utils.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "graph/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {

NodeImpl::NodeContext ExecuteNodeBackend::ResolveNode() const {
  NodeContext node;
  const auto owner = owner_.lock();
  const auto fast_node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  if (fast_node != nullptr) {
    node.execute_owner = owner;
    node.execute_node = fast_node;
    node.op_desc = fast_node->GetOpDescPtr();
  }
  return node;
}

Operator ExecuteNodeBackend::CreateOperator(const NodeContext &node) const {
  return OpDescUtils::CreateOperatorFromOpDesc(node.op_desc);
}

NodeImpl::GraphContext ExecuteNodeBackend::LookupSubgraph(const NodeContext &node, uint32_t index) const {
  ExecuteGraphBackendUtils::NormalizeParentChain(node.execute_owner.get());
  auto *subgraph = FastNodeUtils::GetSubgraphFromNode(node.execute_node, index);
  return {nullptr, subgraph != nullptr ? subgraph->shared_from_this() : nullptr};
}

NodeImpl::GraphContext ExecuteNodeBackend::LookupRootGraph(const NodeContext &node) const {
  ExecuteGraphBackendUtils::NormalizeParentChain(node.execute_owner.get());
  auto *root = ExecuteGraphUtils::FindRootGraph(node.execute_owner.get());
  return {nullptr, root != nullptr ? root->shared_from_this() : nullptr};
}

std::vector<NodeImpl::GraphContext> ExecuteNodeBackend::ListSubgraphs(const GraphContext &root) const {
  std::vector<GraphContext> subgraphs;
  for (auto *graph : root.execute_graph->GetAllSubgraphs()) {
    subgraphs.push_back({nullptr, graph != nullptr ? graph->shared_from_this() : nullptr});
  }
  return subgraphs;
}

GraphPtr ExecuteNodeBackend::WrapGraph(const GraphContext &graph) const {
  return GraphUtilsEx::ExecuteGraph2Graph(graph.execute_graph);
}

FastNode *ExecuteNodeBackend::GetNode() const {
  const auto owner = owner_.lock();
  return owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
}

OpDescPtr ExecuteNodeBackend::GetDesc(const GNode &node) {
  const auto *native = NodeAdapter::GNode2ExecuteNode(node);
  return native != nullptr ? native->GetOpDescPtr() : nullptr;
}

graphStatus ExecuteNodeBackend::SetSubgraph(const AscendString &ir_name, const Graph &subgraph) const {
  const auto owner = owner_.lock();
  auto *node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  GE_ASSERT_NOTNULL(node, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");

  const auto sub_graph = GraphUtilsEx::Graph2ExecuteGraph(subgraph);
  GE_ASSERT_NOTNULL(sub_graph, "Execution node only accepts execution subgraphs");
  return MountSubgraph(owner, node, ir_name, sub_graph);
}

graphStatus ExecuteNodeBackend::SetSubgraphs(const AscendString &ir_name, const std::vector<Graph> &subgraphs) const {
  const auto owner = owner_.lock();
  auto *node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  GE_ASSERT_NOTNULL(node, "Gnode has invalid node source, try to call `AddNodeByOp` firstly");

  std::vector<std::shared_ptr<ExecuteGraph>> sub_graphs;
  for (const auto &subgraph : subgraphs) {
    const auto sub_graph = GraphUtilsEx::Graph2ExecuteGraph(subgraph);
    GE_ASSERT_NOTNULL(sub_graph, "Execution node only accepts execution subgraphs");
    sub_graphs.emplace_back(sub_graph);
  }

  for (size_t i = 0U; i < sub_graphs.size(); ++i) {
    GE_ASSERT_NOTNULL(ir_name.GetString());
    const std::string name = std::string(ir_name.GetString()) + std::to_string(i);
    GE_ASSERT_SUCCESS(MountSubgraph(owner, node, AscendString(name.c_str()), sub_graphs[i]));
  }
  return GRAPH_SUCCESS;
}

graphStatus ExecuteNodeBackend::SetSubgraphInner(const std::shared_ptr<ExecuteGraph> &owner, FastNode *parent_node,
                                                 uint32_t slot, const std::shared_ptr<ExecuteGraph> &subgraph) {
  const auto op_desc = parent_node->GetOpDescPtr();
  auto *root_graph = ExecuteGraphUtils::FindRootGraph(owner.get());
  GE_ASSERT_NOTNULL(root_graph);
  GE_ASSERT_GRAPH_SUCCESS(op_desc->SetSubgraphInstanceName(slot, subgraph->GetName()));
  // 迁移根登记表需要先收集子图原有的子树，再更新直接父关系。
  GE_ASSERT_TRUE(ExecuteGraphBackendUtils::RegisterSubgraphTree(root_graph, subgraph.get()));
  subgraph->SetParentNode(parent_node);
  ExecuteGraphBackendUtils::SetParentGraph(subgraph.get(), owner);
  for (auto *node : subgraph->GetDirectNode()) {
    if (gert::IsTypeNetOutput(node->GetTypePtr())) {
      node->GetOpDescBarePtr()->SetType(gert::kInnerNetOutput);
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ExecuteNodeBackend::MountSubgraph(const std::shared_ptr<ExecuteGraph> &owner, FastNode *fast_node,
                                              const AscendString &ir_name,
                                              const std::shared_ptr<ExecuteGraph> &subgraph) {
  GE_ASSERT_NOTNULL(owner);
  GE_ASSERT_NOTNULL(fast_node);
  GE_ASSERT_NOTNULL(subgraph);
  GE_ASSERT_NOTNULL(ir_name.GetString());
  ExecuteGraphBackendUtils::NormalizeParentChain(owner.get());
  ExecuteGraphBackendUtils::NormalizeParentChain(subgraph.get());
  // 父链必须无环，否则后续查找根图无法终止。
  for (auto *current = owner.get(); current != nullptr; current = current->GetParentGraphBarePtr()) {
    GE_ASSERT_TRUE(current != subgraph.get(), "Cannot mount a graph under itself or its descendants");
  }

  const auto op_desc = fast_node->GetOpDescPtr();
  GE_ASSERT_NOTNULL(op_desc);
  const std::string name(ir_name.GetString());
  (void)op_desc->AddSubgraphName(name);
  const auto &indexes = op_desc->GetSubgraphNameIndexes();
  const auto iter = indexes.find(name);
  GE_ASSERT_TRUE(iter != indexes.cend());
  return SetSubgraphInner(owner, fast_node, iter->second, subgraph);
}

std::pair<GNodePtr, int32_t> ExecuteNodeBackend::GetInDataNodesAndPortIndexs(int32_t index) const {
  const std::pair<GNodePtr, int32_t> gnode_idx = {nullptr, 0xFF};
  const auto owner = owner_.lock();
  auto *node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "the node ptr is not valid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: the node shared ptr is not valid.");
    return gnode_idx;
  }

  if ((index < 0) || (static_cast<size_t>(index) >= node->GetDataInNum())) {
    REPORT_INNER_ERR_MSG("E18888",
                         "Failed to get in data node of index[%d] from node[%s], "
                         "the port does not exist",
                         index, node->GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Get][Port] Failed to get in data node of index[%d] from node[%s], "
           "the port does not exist",
           index, node->GetName().c_str());
    return gnode_idx;
  }

  const auto *edge = node->GetInDataEdgeByIndex(index);
  if (edge == nullptr) {
    GELOGI("Index[%d] from node[%s] has no peer edge, maybe it is optional input", index, node->GetName().c_str());
    return gnode_idx;
  }

  const GNodePtr gnode = NodeAdapter::ExecuteNode2GNode(edge->src);
  if (gnode == nullptr) {
    GELOGE(GRAPH_FAILED, "[Get][GNode] Peer node of node[%s] to gnode failed.", node->GetName().c_str());
    return gnode_idx;
  }

  return {gnode, edge->src_output};
}

std::vector<std::pair<GNodePtr, int32_t>> ExecuteNodeBackend::GetOutDataNodesAndPortIndexs(int32_t index) const {
  const auto owner = owner_.lock();
  auto *node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "node ptr is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: the node shared ptr is not valid.");
    return {};
  }

  if ((index < 0) || (static_cast<size_t>(index) >= node->GetDataOutNum())) {
    REPORT_INNER_ERR_MSG("E18888",
                         "Failed to get out data node of index %d from node %s, "
                         "the port does not exist",
                         index, node->GetName().c_str());
    GELOGE(GRAPH_FAILED,
           "[Get][Port] Failed to get out data node of index %d from node %s, "
           "the port does not exist",
           index, node->GetName().c_str());
    return {};
  }

  std::vector<std::pair<GNodePtr, int32_t>> gnode_index;
  const auto &edges = node->GetOutEdgesRefByIndex(index);
  for (const auto *edge : edges) {
    // 断边后槽位为 nullptr，按 FastNode 约定跳过。
    if (edge == nullptr) {
      continue;
    }
    // 与 GetPeerInDataAnchorsPtr 一致，不返回连接到控制输入口的边。
    if (edge->dst_input < 0) {
      continue;
    }
    const GNodePtr gnode = NodeAdapter::ExecuteNode2GNode(edge->dst);
    if (gnode == nullptr) {
      GELOGE(GRAPH_FAILED, "[Get][GNode] Peer node of node[%s] to gnode failed.", node->GetName().c_str());
      return {};
    }
    gnode_index.emplace_back(std::pair<GNodePtr, int32_t>(gnode, edge->dst_input));
  }

  return gnode_index;
}

std::vector<GNodePtr> ExecuteNodeBackend::GetInControlNodes() const {
  return GetControlNodes(true);
}

std::vector<GNodePtr> ExecuteNodeBackend::GetOutControlNodes() const {
  return GetControlNodes(false);
}

std::vector<GNodePtr> ExecuteNodeBackend::GetControlNodes(bool input) const {
  const auto owner = owner_.lock();
  auto *node = owner != nullptr ? owner->ResolveNode(handle_) : nullptr;
  if (node == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "Gnode: node ptr is not valid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Gnode: the node shared ptr is not valid.");
    return {};
  }

  std::vector<GNodePtr> gnodes;
  const auto &edges = input ? node->GetAllInControlEdgesRef() : node->GetAllOutControlEdgesRef();
  for (const auto *edge : edges) {
    // 断边后槽位为 nullptr，按 FastNode 约定跳过。
    if (edge == nullptr) {
      continue;
    }
    GNodePtr gnode = NodeAdapter::ExecuteNode2GNode(input ? edge->src : edge->dst);
    if (gnode == nullptr) {
      GELOGE(GRAPH_FAILED, "[Get][GNode] Control node of node[%s] to gnode failed.", node->GetName().c_str());
      return {};
    }
    gnodes.emplace_back(gnode);
  }

  return gnodes;
}

namespace {
// InnerData 的 index 表示父节点输入口，与图输入登记序号无关。
FastNode *GetParentInput(const FastNode *node) {
  int32_t index = -1;
  if (!AttrUtils::GetInt(node->GetOpDescPtr(), ATTR_NAME_INDEX, index) || (index < 0)) {
    return nullptr;
  }
  const auto *extend_info = node->GetExtendInfo();
  GE_ASSERT_NOTNULL(extend_info);
  const auto *owner = extend_info->GetOwnerGraphBarePtr();
  GE_ASSERT_NOTNULL(owner);
  const auto *parent = owner->GetParentNodeBarePtr();
  if (parent == nullptr) {
    return nullptr;
  }
  return FastNodeUtils::GetInDataNodeByIndex(parent, index);
}
}  // namespace

graphStatus ExecuteNodeBackend::ResolveConstInput(const NodeContext &node, int32_t index, NodeContext &source) const {
  ExecuteGraphBackendUtils::NormalizeParentChain(node.execute_owner.get());
  auto *input_data_node = FastNodeUtils::GetInDataNodeByIndex(node.execute_node, index);
  GE_CHECK_NOTNULL(input_data_node);
  while ((input_data_node != nullptr) && gert::IsTypeInnerData(input_data_node->GetTypePtr())) {
    input_data_node = GetParentInput(input_data_node);
  }
  if ((input_data_node == nullptr) || !gert::IsTypeConst(input_data_node->GetTypePtr())) {
    return GRAPH_NODE_WITHOUT_CONST_INPUT;
  }
  source.execute_node = input_data_node;
  source.execute_owner = input_data_node->GetExtendInfo()->GetOwnerGraphBarePtr()->shared_from_this();
  source.op_desc = input_data_node->GetOpDescPtr();
  return GRAPH_SUCCESS;
}

}  // namespace ge
