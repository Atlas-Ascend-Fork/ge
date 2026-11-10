/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "graph/graph.h"
#include "graph/debug/ge_log.h"
#include "normal_graph/execute_graph_backend.h"
#include "normal_graph/compute_graph_backend.h"
#include <cstring>
#include "graph/graph_buffer.h"
#include "graph/buffer.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/model.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "common/checker.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_tensor.h"
#include "graph/type_id.h"
#include "utils/tensor_adapter.h"
#include "base/err_msg.h"

namespace ge {

GraphPtr GraphUtilsEx::ExecuteGraph2Graph(const std::shared_ptr<ExecuteGraph> &execute_graph) {
  if (execute_graph == nullptr) {
    return nullptr;
  }
  auto backend = ComGraphMakeShared<ExecuteGraphBackend>(execute_graph);
  if (backend == nullptr) {
    return nullptr;
  }
  auto graph = ComGraphMakeShared<Graph>();
  if (graph == nullptr) {
    return nullptr;
  }
  graph->impl_ = backend;
  return graph;
}

Graph::Graph(const std::string &name) {
  impl_ = ComGraphMakeShared<ComputeGraphBackend>(name);
  if (impl_ == nullptr) {
    GELOGW("[Check][Impl] Make graph impl failed");
  }
}

Graph::Graph(const char_t *name) {
  if (name != nullptr) {
    std::string graph_name = name;
    impl_ = ComGraphMakeShared<ComputeGraphBackend>(graph_name);
    if (impl_ == nullptr) {
      GELOGW("[Check][Impl] Make graph impl failed");
    }
  } else {
    GELOGW("[Check][Param] Input graph name is nullptr.");
  }
}

Graph::Graph(const char_t *name, GraphKind kind) {
  if ((name == nullptr) || ((kind != 0) && (kind != 1))) {
    GELOGE(GRAPH_PARAM_INVALID, "[GertGraph][Parameter] Invalid graph name or backend kind.");
    return;
  }
  if (kind == 1) {
    impl_ = ComGraphMakeShared<ExecuteGraphBackend>(name);
  } else {
    impl_ = ComGraphMakeShared<ComputeGraphBackend>(name);
  }
}

graphStatus Graph::AddOp(const ge::Operator &op) {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return GRAPH_FAILED, "[Check][Param] AddOp failed: graph cannot be used, impl is nullptr.");
  return impl_->AddOp(op);
}

graphStatus Graph::GetAllOpName(std::vector<std::string> &op_name) const {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return GRAPH_FAILED, "[Check][Param] GetAllOpName failed: graph cannot be used, impl is nullptr.");
  return impl_->GetAllOpName(op_name);
}

graphStatus Graph::GetAllOpName(std::vector<AscendString> &names) const {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return GRAPH_FAILED, "[Check][Param] GetAllOpName failed: graph cannot be used, impl is nullptr.");
  std::vector<std::string> op_names;
  if (impl_->GetAllOpName(op_names) != GRAPH_SUCCESS) {
    GELOGE(GRAPH_FAILED, "[Get][AllOpName] failed.");
    return GRAPH_FAILED;
  }

  for (auto &op_name : op_names) {
    names.emplace_back(op_name.c_str());
  }

  return GRAPH_SUCCESS;
}

graphStatus Graph::FindOpByName(const std::string &name, Operator &op) const {
  const Operator op_find_op_def("NULL");
  op = op_find_op_def;
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return GRAPH_FAILED, "[Check][Param] FindOpByName failed: graph cannot be used, impl is nullptr.");
  return impl_->FindOpByName(name, op);
}

graphStatus Graph::FindOpByName(const char_t *name, Operator &op) const {
  if (name == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param name is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] FindOpByName: name is nullptr.");
    return GRAPH_FAILED;
  }
  const Operator op_find_op_def("NULL");
  op = op_find_op_def;
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return GRAPH_FAILED, "[Check][Param] FindOpByName failed: graph cannot be used, impl is nullptr.");
  const std::string op_name = name;
  return impl_->FindOpByName(op_name, op);
}

graphStatus Graph::FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const {
  GE_CHECK_NOTNULL(impl_);
  return impl_->FindOpByType(type, ops);
}

graphStatus Graph::FindOpByType(const char_t *type, std::vector<ge::Operator> &ops) const {
  if (type == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param type is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] FindOpByType: type is nullptr.");
    return GRAPH_FAILED;
  }
  GE_CHECK_NOTNULL(impl_);
  const std::string op_type = type;
  return impl_->FindOpByType(op_type, ops);
}

Graph &Graph::SetInputs(const std::vector<ge::Operator> &inputs) {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return *this, "[Check][Param] SetInputs failed: graph cannot be used, impl is nullptr.");
  GE_CHK_BOOL_EXEC(!inputs.empty(), REPORT_INNER_ERR_MSG("E18888", "input operator size cannot be 0");
                   return *this, "[Check][Param] SetInputs failed: input operator size cannot be 0, graph: %s",
                          impl_->GetName().c_str());
  (void)impl_->SetInputs(inputs);
  return *this;
}

Graph &Graph::SetOutputs(const std::vector<ge::Operator> &outputs) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetOutputs failed: graph cannot be used, impl is nullptr.");
    return *this;
  }
  (void)impl_->SetOutputs(outputs);
  return *this;
}

graphStatus Graph::SetOutputs(const std::vector<std::pair<GNode, int32_t>> &outputs) {
  GE_ASSERT(IsValid(), "Graph is not valid, call SetValid() first.");
  return impl_->SetOutputs(outputs);
}

Graph &Graph::SetOutputs(const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexs) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetOutputs failed: graph cannot be used, impl is nullptr.");
    return *this;
  }
  (void)impl_->SetOutputs(output_indexs);
  return *this;
}

Graph &Graph::SetOutputs(const std::vector<std::pair<Operator, std::string>> &outputs) {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return *this, "[Check][Param] SetOutputs failed: graph cannot be used, impl is nullptr.");
  (void)impl_->SetOutputs(outputs);
  return *this;
}

Graph &Graph::SetOutputs(const std::vector<std::pair<ge::Operator, AscendString>> &outputs) {
  GE_CHK_BOOL_EXEC(impl_ != nullptr, REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
                   return *this, "[Check][Param] SetOutputs failed: graph cannot be used, impl is nullptr.");
  (void)impl_->SetOutputs(outputs);
  return *this;
}

Graph &Graph::SetTargets(const std::vector<ge::Operator> &targets) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] SetTargets failed: graph cannot be used, impl is nullptr.");
    return *this;
  }
  (void)impl_->SetTargets(targets);
  return *this;
}

bool Graph::IsValid() const {
  if (impl_ == nullptr) {
    return false;
  }
  return impl_->IsValid();
}

void Graph::SetNeedIteration(bool need_iteration) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] Set need iteration failed, as impl is null.");
    return;
  }
  impl_->SetNeedIteration(need_iteration);
}

std::vector<GNode> Graph::GetAllNodes() const {
  std::vector<GNode> graph_nodes;
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetAllNodes: graph cannot be used, impl is nullptr.");
    return graph_nodes;
  }

  return impl_->GetAllNodes();
}

std::vector<GraphPtr> Graph::GetAllSubgraphs() const {
  GE_ASSERT(IsValid(), "[Check][Param] current graph is invalid.");
  return impl_->GetAllSubgraphs();
}

GraphPtr Graph::GetSubGraph(const char *name) const {
  GE_ASSERT(IsValid(), "[Check][Param] current graph is invalid.");
  return impl_->GetSubGraph(name);
}

graphStatus Graph::AddSubGraph(const Graph &subgraph) {
  GE_ASSERT(IsValid(), "[Check][Param] current graph is invalid.");
  GE_ASSERT(subgraph.IsValid(), "[Check][Param] subgraph is invalid.");
  return impl_->AddSubGraph(subgraph);
}

graphStatus Graph::RemoveSubgraph(const char *name) {
  GE_ASSERT(IsValid(), "[Check][Param] current graph is invalid.");
  return impl_->RemoveSubgraph(name);
}

std::vector<GNode> Graph::GetDirectNode() const {
  std::vector<GNode> graph_nodes;
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] GetDirectNode: graph cannot be used, impl is nullptr.");
    return graph_nodes;
  }

  return impl_->GetDirectNode();
}

graphStatus Graph::RemoveNode(GNode &node) {
  return RemoveNode(node, false);
}

graphStatus Graph::RemoveNode(GNode &node, bool contain_subgraph) {
  GE_CHECK_NOTNULL(impl_);
  return impl_->RemoveNode(node, contain_subgraph);
}

graphStatus Graph::RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                              const int32_t dst_port_index) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] graph cannot be used, impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->RemoveEdge(src_node, src_port_index, dst_node, dst_port_index);
}

GNode Graph::AddNodeByOp(const Operator &op) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] graph cannot be used, impl is nullptr.");
    return GNode();
  }

  return impl_->AddNodeByOp(op);
}

graphStatus Graph::AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                               const int32_t dst_port_index) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] graph cannot be used, impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->AddDataEdge(src_node, src_port_index, dst_node, dst_port_index);
}

graphStatus Graph::AddControlEdge(GNode &src_node, GNode &dst_node) {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "graph cannot be used, impl is nullptr.");
    GELOGE(GRAPH_FAILED, "[Check][Param] graph cannot be used, impl is nullptr.");
    return GRAPH_FAILED;
  }

  return impl_->AddControlEdge(src_node, dst_node);
}

GraphPtr Graph::ConstructFromInputs(const std::vector<Operator> &inputs, const AscendString &name) {
  return ComputeGraphBackend::ConstructFromInputs(inputs, name);
}

graphStatus Graph::SaveToFile(const std::string &file_name) const {
  if (impl_ == nullptr) {
    // 旧行为：空门面按空计算图处理，经模型序列化失败返回
    return ComputeGraphBackend::SaveToFile(nullptr, file_name);
  }
  return impl_->SaveToFile(file_name);
}

graphStatus Graph::SaveToFile(const char_t *file_name) const {
  if (file_name == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "file name is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] file name is nullptr.");
    return GRAPH_FAILED;
  }

  if (impl_ == nullptr) {
    // 旧行为：空门面按空计算图处理，经模型序列化失败返回
    return ComputeGraphBackend::SaveToFile(nullptr, std::string(file_name));
  }
  return impl_->SaveToFile(std::string(file_name));
}

graphStatus Graph::LoadFromFile(const std::string &file_name) {
  Graph loaded_graph;
  const graphStatus ret = impl_ != nullptr ? impl_->LoadFromFile(file_name, loaded_graph)
                                           : ComputeGraphBackend::LoadFromFile(loaded_graph, file_name);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  *this = std::move(loaded_graph);
  return GRAPH_SUCCESS;
}

graphStatus Graph::LoadFromFile(const char_t *file_name) {
  if (file_name == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "param file name is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] file name is nullptr.");
    return GRAPH_FAILED;
  }

  return LoadFromFile(std::string(file_name));
}

graphStatus Graph::LoadFromSerializedModelArray(const void *serialized_model, size_t size) {
  Graph loaded_graph;
  const graphStatus ret = impl_ != nullptr
                              ? impl_->LoadFromSerializedModelArray(serialized_model, size, loaded_graph)
                              : ComputeGraphBackend::LoadFromSerializedModelArray(loaded_graph, serialized_model, size);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  *this = std::move(loaded_graph);
  return GRAPH_SUCCESS;
}

graphStatus Graph::SaveToMem(GraphBuffer &graph_buffer) const {
  // GraphBuffer::buffer_ 仅 Graph 可见，门面取出原始 Buffer 引用后分发
  if (impl_ == nullptr) {
    // 旧行为：空门面按空计算图处理，经模型序列化失败返回
    return ComputeGraphBackend::SaveToMem(nullptr, *(graph_buffer.buffer_));
  }
  return impl_->SaveToMem(*(graph_buffer.buffer_));
}

graphStatus Graph::LoadFromMem(const GraphBuffer &graph_buffer) {
  return LoadFromMem(graph_buffer.GetData(), graph_buffer.GetSize());
}

graphStatus Graph::LoadFromMem(const uint8_t *data, const size_t len) {
  Graph loaded_graph;
  const graphStatus ret = impl_ != nullptr ? impl_->LoadFromMem(data, len, loaded_graph)
                                           : ComputeGraphBackend::LoadFromMem(loaded_graph, data, len);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  *this = std::move(loaded_graph);
  return GRAPH_SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY const std::string &Graph::GetName() const {
  return impl_->GetName();
}

graphStatus Graph::GetName(AscendString &name) const {
  if (impl_ == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "impl is nullptr, check invalid.");
    GELOGE(GRAPH_FAILED, "[Check][Param] impl is nullptr.");
    return GRAPH_FAILED;
  }
  const std::string graph_name = impl_->GetName();
  name = AscendString(graph_name.c_str());
  return GRAPH_SUCCESS;
}

graphStatus Graph::CopyFrom(const Graph &src_graph) {
  Graph copied_graph;
  const auto res = impl_ != nullptr ? impl_->CopyFrom(src_graph, copied_graph)
                                    : ComputeGraphBackend::CopyFrom(src_graph, "", copied_graph);
  if (res != GRAPH_SUCCESS) {
    AscendString name;
    (void)src_graph.GetName(name);
    REPORT_INNER_ERR_MSG("E18888", "copy graph from %s failed.", name.GetString());
    GELOGE(GRAPH_FAILED, "[Copy][Graph] from %s failed.", name.GetString());
    return GRAPH_FAILED;
  }
  *this = std::move(copied_graph);
  return GRAPH_SUCCESS;
}

// 添加AttrValue版本的SetAttr和GetAttr方法
graphStatus Graph::SetAttr(const AscendString &name, const AttrValue &attr_value) {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->SetAttr(name, attr_value);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus Graph::GetAttr(const AscendString &name,
                                                                          AttrValue &attr_value) const {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->GetAttr(name, attr_value);
}
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus Graph::SetValid() {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->SetValid();
}

graphStatus Graph::Dump(Graph::DumpFormat format, std::ostream &o_stream) const {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->Dump(format, o_stream);
}

graphStatus Graph::DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->DumpToFile(format, suffix);
}

GNodePtr Graph::FindNodeByName(const AscendString &node_name) const {
  if (!IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "current graph is invalid.");
    GELOGE(GRAPH_FAILED, "[Find][Node] impl is invalid.");
    return nullptr;
  }
  return impl_->FindNodeByName(node_name);
}

ConstGraphPtr Graph::GetParentGraph() const {
  if (!IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "current graph is invalid.");
    GELOGE(GRAPH_FAILED, "[Get][ParentGraph] current impl is invalid.");
    return nullptr;
  }
  return impl_->GetParentGraph();
}

GNodePtr Graph::GetParentNode() const {
  if (!IsValid()) {
    REPORT_INNER_ERR_MSG("E18888", "current graph is invalid.");
    GELOGE(GRAPH_FAILED, "[Get][ParentNode] current impl is invalid.");
    return nullptr;
  }
  return impl_->GetParentNode();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::shared_ptr<ExecuteGraph> GraphUtilsEx::Graph2ExecuteGraph(
    const Graph &graph) {
  const auto *backend = dynamic_cast<const ExecuteGraphBackend *>(graph.impl_.get());
  return ((backend != nullptr) && graph.IsValid()) ? backend->GetGraph() : nullptr;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY bool GraphUtilsEx::IsExecuteGraph(const Graph &graph) {
  return dynamic_cast<const ExecuteGraphBackend *>(graph.impl_.get()) != nullptr;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY ComputeGraphPtr GraphUtilsEx::GetComputeGraph(const ge::Graph &graph) {
  if (!graph.IsValid()) {
    return nullptr;
  }
  return graph.impl_->GetComputeGraph();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus
GraphUtilsEx::CreateGraphFromOperatorWithStableTopo(Graph &graph, const std::vector<Operator> &ops) {
  AscendString graph_name;
  GE_ASSERT_SUCCESS(graph.GetName(graph_name));
  GE_ASSERT_TRUE(dynamic_cast<const ExecuteGraphBackend *>(graph.impl_.get()) == nullptr,
                 "Stable compute graph conversion does not support execution graphs");
  GE_ASSERT_TRUE(graph.impl_->GetComputeGraph() == nullptr, "Compute graph of graph: %s has been created",
                 graph_name.GetString());
  const auto compute_graph = GraphUtilsEx::CreateComputeGraphFromOperatorWithStableTopo(graph_name.GetString(), ops);
  GE_ASSERT_NOTNULL(compute_graph);
  graph.impl_->AdoptGraph(compute_graph);
  return SUCCESS;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY Graph
GraphUtilsEx::CreateGraphFromComputeGraph(const ge::ComputeGraphPtr compute_graph) {
  if (compute_graph == nullptr) {
    return Graph("");
  }

  const auto name = compute_graph->GetName();
  const auto graph = Graph(name.c_str());
  if (graph.impl_ == nullptr) {
    return graph;
  }
  graph.impl_->AdoptGraph(compute_graph);
  return graph;
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY std::unique_ptr<Graph>
GraphUtilsEx::CreateGraphUniquePtrFromComputeGraph(const ComputeGraphPtr &compute_graph) {
  GE_ASSERT_NOTNULL(compute_graph);
  auto name = compute_graph->GetName();
  auto graph = ComGraphMakeUnique<Graph>(name.c_str());
  GE_ASSERT_NOTNULL(graph);
  GE_ASSERT_NOTNULL(graph->impl_);
  graph->impl_->AdoptGraph(compute_graph);
  return graph;
}
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY GraphPtr
GraphUtilsEx::CreateGraphPtrFromComputeGraph(const ge::ComputeGraphPtr compute_graph) {
  return CreateGraphUniquePtrFromComputeGraph(compute_graph);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus GraphUtilsEx::RecoverGraphOperators(const Graph &graph) {
  GE_CHECK_NOTNULL(graph.impl_);
  return graph.impl_->RecoverOperators();
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus GraphUtils::CopyGraphImpl(
    const Graph &src_graph, Graph &dst_graph, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
    const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) {
  GE_CHECK_NOTNULL(dst_graph.impl_);
  GE_CHECK_NOTNULL(src_graph.impl_);

  return src_graph.impl_->CopyOperatorsTo(*dst_graph.impl_, node_old_2_new, op_desc_old_2_new);
}

GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY graphStatus GraphUtilsEx::CopyGraphImpl(
    const Graph &src_graph, Graph &dst_graph, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
    const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) {
  GE_CHECK_NOTNULL(dst_graph.impl_);
  GE_CHECK_NOTNULL(src_graph.impl_);

  return src_graph.impl_->CopyOperatorsTo(*dst_graph.impl_, node_old_2_new, op_desc_old_2_new);
}
}  // namespace ge

#ifdef __cplusplus
extern "C" {
#endif

void GeApiWrapper_RenameAllNodes(void *graph_ptr, const char *prefix) {
  if (graph_ptr == nullptr || prefix == nullptr) {
    return;
  }
  std::string prefix_str(prefix);
  ge::Graph *graph = static_cast<ge::Graph *>(graph_ptr);
  if (ge::GraphUtilsEx::IsExecuteGraph(*graph)) {
    GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION("GeApiWrapper_RenameAllNodes");
    return;
  }
  for (auto &sub_node : graph->GetAllNodes()) {
    auto snode = ge::NodeAdapter::GNode2Node(sub_node);
    auto orig_name = snode->GetNamePtr() == nullptr ? std::string("") : std::string(snode->GetNamePtr());
    auto modi_name = prefix_str + '_' + orig_name;
    snode->GetOpDesc()->SetNamePtr(modi_name.c_str());
  }
}

#ifdef __cplusplus
}
#endif
