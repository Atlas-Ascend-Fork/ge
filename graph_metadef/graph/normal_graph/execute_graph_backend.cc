/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "normal_graph/execute_graph_backend.h"
#include "common/checker.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/debug/ge_log.h"

namespace ge {
namespace {
FastNode *ResolveExecuteNode(const std::shared_ptr<ExecuteGraph> &graph, const GNode &node) {
  auto *native = NodeAdapter::GNode2ExecuteNode(node);
  if ((graph == nullptr) || (native == nullptr) || !graph->CheckNodeIsInGraph(native)) {
    GELOGE(GRAPH_PARAM_INVALID, "[GertGraph][Owner] Node is invalid or belongs to another graph.");
    return nullptr;
  }
  return native;
}
}  // namespace

ComputeGraphPtr ExecuteGraphBackend::GetComputeGraph() const {
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("GetComputeGraph");
  return nullptr;
}

void ExecuteGraphBackend::AdoptGraph(const ComputeGraphPtr &compute_graph) {
  (void)compute_graph;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("AdoptGraph");
}

bool ExecuteGraphBackend::IsCompatibleSubgraph(const Graph &subgraph) const {
  return GraphUtilsEx::Graph2ExecuteGraph(subgraph) != nullptr;
}

graphStatus ExecuteGraphBackend::RegisterSubgraph(const Graph &subgraph) {
  const auto sub_execute_graph = GraphUtilsEx::Graph2ExecuteGraph(subgraph);
  ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
  ExecuteGraphBackendUtils::NormalizeParentChain(sub_execute_graph.get());
  return graph_->AddSubGraph(sub_execute_graph, sub_execute_graph->GetName()) != nullptr ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::RecoverOperators() {
  REPORT_INNER_ERR_MSG("E18888", "graph.impl_->compute_graph_ is nullptr, check invalid.");
  GELOGE(GRAPH_FAILED, "[Check][Param] graph.impl_->compute_graph_ is nullptr, check invalid.");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::CopyOperatorsTo(GraphImpl &dst, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                                                 const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const {
  (void)dst;
  (void)node_old_2_new;
  (void)op_desc_old_2_new;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("CopyOperatorsTo");
  return GRAPH_FAILED;
}

ExecuteGraphBackend::ExecuteGraphBackend(const std::string &name) : name_(name) {}

ExecuteGraphBackend::ExecuteGraphBackend(const std::shared_ptr<ExecuteGraph> &graph)
    : name_(graph != nullptr ? graph->GetName() : ""), graph_(graph) {
  ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
}

void ExecuteGraphBackend::BuildGraphFromInputs(const std::vector<Operator> &inputs) {
  graph_ = GraphUtilsEx::CreateExecuteGraphFromOperator(name_, inputs);
}

std::vector<GNode> ExecuteGraphBackend::GetDirectNode() const {
  if (graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl execute graph is nullptr.");
    GELOGE(GRAPH_FAILED, "[Get][Graph] GetDirectNode: execute graph ptr is nullptr, graph %s", name_.c_str());
    return {};
  }
  return WrapNodes(graph_->GetDirectNode());
}

std::vector<GNode> ExecuteGraphBackend::GetAllNodes() const {
  if (graph_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl execute graph is nullptr.");
    GELOGE(GRAPH_FAILED, "[Get][Graph] GetAllNodes: execute graph ptr is nullptr, graph %s", name_.c_str());
    return {};
  }
  ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
  return WrapNodes(graph_->GetAllNodes());
}

std::vector<GNode> ExecuteGraphBackend::WrapNodes(const std::vector<FastNode *> &nodes) const {
  std::vector<GNode> graph_nodes;
  for (auto *node : nodes) {
    const auto wrapped = NodeAdapter::ExecuteNode2GNode(node);
    if (wrapped != nullptr) {
      graph_nodes.emplace_back(*wrapped);
    }
  }
  return graph_nodes;
}

FastNode *ExecuteGraphBackend::FindDirectNodeByName(const AscendString &name) const {
  if ((name.GetString() == nullptr) || (graph_ == nullptr)) {
    return nullptr;
  }
  for (auto *node : graph_->GetDirectNode()) {
    if (node->GetName() == name.GetString()) {
      return node;
    }
  }
  return nullptr;
}

graphStatus ExecuteGraphBackend::RemoveSubgraph(const char *name) {
  (void)graph_->RemoveSubGraph(name);
  return GRAPH_SUCCESS;
}

graphStatus ExecuteGraphBackend::CopyFrom(const Graph &src_graph, Graph &copied_graph) const {
  const auto src = GraphUtilsEx::Graph2ExecuteGraph(src_graph);
  GE_ASSERT_NOTNULL(src, "CopyFrom requires an execution graph source");
  ExecuteGraphBackendUtils::NormalizeParentChain(src.get());
  GE_ASSERT_TRUE(src->GetParentGraphBarePtr() == nullptr, "Only a root execution graph can be copied");
  const auto graph_name = name_.empty() ? src_graph.GetName() : name_;
  const auto copied = ComGraphMakeShared<ExecuteGraph>(graph_name);
  GE_ASSERT_NOTNULL(copied);
  GE_ASSERT_GRAPH_SUCCESS(ExecuteGraphBackendUtils::CopyGraph(*src, *copied));
  const auto tmp_graph = GraphUtilsEx::ExecuteGraph2Graph(copied);
  GE_ASSERT_NOTNULL(tmp_graph);
  copied_graph = std::move(*tmp_graph);
  return GRAPH_SUCCESS;
}

GNodePtr ExecuteGraphBackend::GetParentNode() const {
  const auto parent = ExecuteGraphBackendUtils::GetParentGraph(graph_.get());
  auto *parent_graph_node_ptr = parent != nullptr ? graph_->GetParentNodeBarePtr() : nullptr;
  GE_WARN_ASSERT(parent_graph_node_ptr != nullptr, "Graph %s has no parent node.", name_.c_str());
  return NodeAdapter::ExecuteNode2GNode(parent_graph_node_ptr);
}

const std::string &ExecuteGraphBackend::GetName() const {
  return name_;
}

bool ExecuteGraphBackend::IsValid() const {
  return graph_ != nullptr;
}

graphStatus ExecuteGraphBackend::InitGraph() {
  GE_WARN_ASSERT(graph_ == nullptr, "No need to init again");
  graph_ = ComGraphMakeSharedAndThrow<ExecuteGraph>(name_);
  return GRAPH_SUCCESS;
}

std::vector<GraphPtr> ExecuteGraphBackend::GetAllSubgraphs() const {
  ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
  std::vector<GraphPtr> subgraphs;
  const auto sub_execute_graphs = graph_->GetAllSubgraphs();
  subgraphs.reserve(sub_execute_graphs.size());
  for (auto *sub_execute_graph : sub_execute_graphs) {
    subgraphs.emplace_back(GraphUtilsEx::ExecuteGraph2Graph(sub_execute_graph->shared_from_this()));
  }
  return subgraphs;
}

GraphPtr ExecuteGraphBackend::GetSubGraph(const char *name) const {
  ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
  const auto sub_execute_graph = graph_->GetSubGraph(name);
  return sub_execute_graph != nullptr ? GraphUtilsEx::ExecuteGraph2Graph(sub_execute_graph->shared_from_this())
                                      : nullptr;
}

GraphPtr ExecuteGraphBackend::GetParentGraph() const {
  const auto parent = ExecuteGraphBackendUtils::GetParentGraph(graph_.get());
  GE_WARN_ASSERT(parent != nullptr, "Graph %s has no parent graph.", name_.c_str());
  return GraphUtilsEx::ExecuteGraph2Graph(parent);
}

bool ExecuteGraphBackend::ResolveNode(const GNode &node, NodeContext &context) const {
  context.execute_node = NodeAdapter::GNode2ExecuteNode(node);
  if (context.execute_node == nullptr) {
    return false;
  }
  context.name = context.execute_node->GetName();
  auto *owner = context.execute_node->GetExtendInfo()->GetOwnerGraphBarePtr();
  context.execute_owner = owner != nullptr ? owner->weak_from_this().lock() : nullptr;
  if (context.execute_owner != nullptr) {
    context.owner_name = context.execute_owner->GetName();
  }
  return true;
}

GNode ExecuteGraphBackend::AddNodeFromDesc(const OpDescPtr &desc) {
  const auto node = NodeAdapter::ExecuteNode2GNode(graph_->AddNode(desc));
  return node != nullptr ? *node : GNode();
}

graphStatus ExecuteGraphBackend::LinkDataEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) {
  return ExecuteGraphBackendUtils::AddDataEdge(src.execute_node, out, dst.execute_node, in);
}

graphStatus ExecuteGraphBackend::LinkControlEdge(const NodeContext &src, const NodeContext &dst) {
  return ExecuteGraphBackendUtils::AddControlEdge(src.execute_node, dst.execute_node);
}

graphStatus ExecuteGraphBackend::UnlinkEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) {
  return ExecuteGraphBackendUtils::RemoveEdge(src.execute_node, out, dst.execute_node, in);
}

bool ExecuteGraphBackend::IsNodeInScope(const NodeContext &node, bool recursively) const {
  return recursively ? ExecuteGraphBackendUtils::IsNodeInGraphRecursively(graph_.get(), node.execute_node)
                     : graph_ == node.execute_owner;
}

void ExecuteGraphBackend::UnlinkNode(const NodeContext &node) {
  FastNodeUtils::UnlinkAll(node.execute_node);
}

graphStatus ExecuteGraphBackend::EraseNode(const NodeContext &node) {
  return ExecuteGraphBackendUtils::RemoveNodeWithoutRelink(node.execute_owner.get(), node.execute_node);
}

AttrHolder *ExecuteGraphBackend::GetAttributeHolder() const {
  return graph_.get();
}

GNodePtr ExecuteGraphBackend::FindDirectNode(const AscendString &name) const {
  return NodeAdapter::ExecuteNode2GNode(FindDirectNodeByName(name));
}

ConstOpDescPtr ExecuteGraphBackend::GetOutputOpDesc(const GNode &node) const {
  const auto *native = NodeAdapter::GNode2ExecuteNode(node);
  return native != nullptr ? native->GetOpDescPtr() : nullptr;
}

graphStatus ExecuteGraphBackend::SetGraphOutNodesInfo(const std::vector<std::pair<GNode, int32_t>> &outputs) {
  std::vector<std::pair<FastNode *, int32_t>> output_nodes;
  for (const auto &item : outputs) {
    auto *node = ResolveExecuteNode(graph_, item.first);
    GE_ASSERT_NOTNULL(node);
    output_nodes.emplace_back(node, item.second);
  }
  return graph_->SetGraphOutNodesInfo(output_nodes);
}

graphStatus ExecuteGraphBackend::SetTargets(const std::vector<Operator> &targets) {
  (void)targets;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::SetTargets");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::AddOp(const ge::Operator &op) {
  (void)op;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::AddOp");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::FindOpByName(const std::string &name, ge::Operator &op) const {
  (void)name;
  (void)op;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::FindOpByName");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const {
  (void)type;
  (void)ops;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::FindOpByType");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::GetAllOpName(std::vector<std::string> &op_name) const {
  (void)op_name;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::GetAllOpName");
  return GRAPH_FAILED;
}

void ExecuteGraphBackend::SetNeedIteration(bool need_iteration) {
  (void)need_iteration;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::SetNeedIteration");
}

graphStatus ExecuteGraphBackend::Dump(Graph::DumpFormat format, std::ostream &stream) const {
  (void)format;
  (void)stream;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::Dump");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const {
  (void)format;
  (void)suffix;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::DumpToFile");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::SaveToFile(const std::string &file_name) const {
  (void)file_name;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::SaveToFile");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::SaveToMem(Buffer &buffer) const {
  (void)buffer;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::SaveToMem");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::LoadFromFile(const std::string &file_name, Graph &loaded_graph) const {
  (void)file_name;
  (void)loaded_graph;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::LoadFromFile");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::LoadFromMem(const uint8_t *data, const size_t len, Graph &loaded_graph) const {
  (void)data;
  (void)len;
  (void)loaded_graph;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::LoadFromMem");
  return GRAPH_FAILED;
}

graphStatus ExecuteGraphBackend::LoadFromSerializedModelArray(const void *serialized_model, const size_t size,
                                                              Graph &loaded_graph) const {
  (void)serialized_model;
  (void)size;
  (void)loaded_graph;
  GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("Graph::LoadFromSerializedModelArray");
  return GRAPH_FAILED;
}

}  // namespace ge
