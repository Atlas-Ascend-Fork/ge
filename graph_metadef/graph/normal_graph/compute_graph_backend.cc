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

GraphPtr ComputeGraphBackend::ConstructFromInputs(const std::vector<Operator> &inputs, const AscendString &name) {
  const char_t *const ascend_name = name.GetString();
  if (ascend_name == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "ascend string error");
    GELOGE(GRAPH_PARAM_INVALID, "[Check][Param] ascend string error.");
    return nullptr;
  }

  if (inputs.empty()) {
    REPORT_INNER_ERR_MSG("E18888", "inputs size cannot be 0.");
    GELOGE(GRAPH_FAILED, "[Check][Param] inputs size cannot be 0, graph: %s", ascend_name);
    return nullptr;
  }

  const std::string graph_name = ascend_name;
  const ComputeGraphPtr compute_graph = GraphUtilsEx::CreateGraphFromOperator(graph_name, inputs);
  if (compute_graph == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "create compute graph from op failed, name:%s", graph_name.c_str());
    GELOGE(GRAPH_FAILED, "[Create][ComputeGraph] failed, name:%s.", graph_name.c_str());
    return nullptr;
  }

  compute_graph->SetInputSize(static_cast<uint32_t>(inputs.size()));
  const GraphPtr graph_ptr = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  if (graph_ptr == nullptr) {
    REPORT_INNER_ERR_MSG("E18888", "create graph from compute graph:%s failed.", compute_graph->GetName().c_str());
    GELOGE(GRAPH_FAILED, "[Create][Graph] from compute graph:%s failed.", compute_graph->GetName().c_str());
    return nullptr;
  }

  return graph_ptr;
}

void ComputeGraphBackend::BuildGraphFromInputs(const std::vector<Operator> &inputs) {
  compute_graph_ = GraphUtilsEx::CreateGraphFromOperator(name_, inputs);
}

bool ComputeGraphBackend::ResolveNode(const GNode &node, NodeContext &context) const {
  context.compute_node = NodeAdapter::GNode2Node(node);
  if (context.compute_node == nullptr) {
    return false;
  }
  context.name = context.compute_node->GetName();
  context.compute_owner = context.compute_node->GetOwnerComputeGraph();
  if (context.compute_owner != nullptr) {
    context.owner_name = context.compute_owner->GetName();
  }
  return true;
}

GNode ComputeGraphBackend::AddNodeFromDesc(const OpDescPtr &desc) {
  return NodeAdapter::Node2GNode(compute_graph_->AddNode(desc));
}

graphStatus ComputeGraphBackend::LinkDataEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) {
  return GraphUtils::AddEdge(src.compute_node->GetOutDataAnchor(out), dst.compute_node->GetInDataAnchor(in));
}

graphStatus ComputeGraphBackend::LinkControlEdge(const NodeContext &src, const NodeContext &dst) {
  return GraphUtils::AddEdge(src.compute_node->GetOutControlAnchor(), dst.compute_node->GetInControlAnchor());
}

graphStatus ComputeGraphBackend::UnlinkEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) {
  return RemoveEdgeByAnchor(src.compute_node, out, dst.compute_node, in);
}

bool ComputeGraphBackend::IsNodeInScope(const NodeContext &node, bool recursively) const {
  return recursively ? GraphUtils::IsNodeInGraphRecursively(compute_graph_, *node.compute_node)
                     : compute_graph_ == node.compute_owner;
}

void ComputeGraphBackend::UnlinkNode(const NodeContext &node) {
  NodeUtils::UnlinkAll(*node.compute_node);
}

graphStatus ComputeGraphBackend::EraseNode(const NodeContext &node) {
  return GraphUtils::RemoveNodeWithoutRelink(node.compute_owner, node.compute_node);
}

void ComputeGraphBackend::ClearNodeOwner(const NodeContext &node) {
  (void)node.compute_node->ClearOwnerGraph(nullptr);
}

AttrHolder *ComputeGraphBackend::GetAttributeHolder() const {
  return compute_graph_.get();
}

GNodePtr ComputeGraphBackend::FindDirectNode(const AscendString &name) const {
  return NodeAdapter::Node2GNodePtr(compute_graph_->FindNode(name.GetString()));
}

ConstOpDescPtr ComputeGraphBackend::GetOutputOpDesc(const GNode &node) const {
  const auto native = NodeAdapter::GNode2Node(node);
  return native != nullptr ? native->GetOpDesc() : nullptr;
}

graphStatus ComputeGraphBackend::SetGraphOutNodesInfo(const std::vector<std::pair<GNode, int32_t>> &outputs) {
  std::vector<std::pair<NodePtr, int32_t>> output_nodes;
  for (const auto &item : outputs) {
    output_nodes.emplace_back(NodeAdapter::GNode2Node(item.first), item.second);
  }
  return compute_graph_->SetGraphOutNodesInfo(output_nodes);
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

bool ComputeGraphBackend::IsCompatibleSubgraph(const Graph &subgraph) const {
  return GraphUtilsEx::GetComputeGraph(subgraph) != nullptr;
}

graphStatus ComputeGraphBackend::RegisterSubgraph(const Graph &subgraph) {
  return compute_graph_->AddSubgraph(GraphUtilsEx::GetComputeGraph(subgraph));
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

graphStatus ComputeGraphBackend::CopyFrom(const Graph &src_graph, Graph &copied_graph) const {
  return CopyFrom(src_graph, name_, copied_graph);
}

graphStatus ComputeGraphBackend::CopyFrom(const Graph &src_graph, const std::string &graph_name, Graph &copied_graph) {
  Graph tmp_graph(graph_name);
  const auto ret = GraphUtilsEx::CopyGraph(src_graph, tmp_graph);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  copied_graph = std::move(tmp_graph);
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::CopyOperatorsTo(GraphImpl &dst_impl,
                                                 const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                                                 const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const {
  // CopyGraphImpl 工具入口也可直接调用，需在访问计算图状态前检查目标后端。
  auto *dst_backend = dynamic_cast<ComputeGraphBackend *>(&dst_impl);
  GE_ASSERT_NOTNULL(dst_backend, "CopyOperatorsTo requires a compute graph destination");
  auto &dst = *dst_backend;
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

graphStatus ComputeGraphBackend::SaveToFile(const std::string &file_name) const {
  return SaveToFile(compute_graph_, file_name);
}

graphStatus ComputeGraphBackend::SaveToMem(Buffer &buffer) const {
  return SaveToMem(compute_graph_, buffer);
}

graphStatus ComputeGraphBackend::LoadFromFile(const std::string &file_name, Graph &loaded_graph) const {
  return LoadFromFile(loaded_graph, file_name);
}

graphStatus ComputeGraphBackend::LoadFromFile(Graph &loaded_graph, const std::string &file_name) {
  Model model = Model();
  const graphStatus ret = model.LoadFromFile(file_name);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  loaded_graph = GraphUtilsEx::CreateGraphFromComputeGraph(model.GetGraph());
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::LoadFromMem(const uint8_t *data, const size_t len, Graph &loaded_graph) const {
  return LoadFromMem(loaded_graph, data, len);
}

graphStatus ComputeGraphBackend::LoadFromMem(Graph &loaded_graph, const uint8_t *data, const size_t len) {
  Model model = Model();
  GE_ASSERT_GRAPH_SUCCESS(Model::Load(data, len, model), "Failed to load graph from memory.");
  loaded_graph = GraphUtilsEx::CreateGraphFromComputeGraph(model.GetGraph());
  return GRAPH_SUCCESS;
}

graphStatus ComputeGraphBackend::LoadFromSerializedModelArray(const void *serialized_model, const size_t size,
                                                              Graph &loaded_graph) const {
  return LoadFromSerializedModelArray(loaded_graph, serialized_model, size);
}

graphStatus ComputeGraphBackend::LoadFromSerializedModelArray(Graph &loaded_graph, const void *serialized_model,
                                                              const size_t size) {
  GE_ASSERT_NOTNULL(serialized_model, "param serialized_model is nullptr");
  GE_ASSERT(size != 0U, "param size is 0");
  Model model;
  GE_ASSERT_GRAPH_SUCCESS(Model::Load(static_cast<const uint8_t *>(serialized_model), size, model),
                          "Failed to load model from serialized model def.");
  GE_ASSERT_NOTNULL(model.GetGraph(), "Failed to get root graph from model.");
  loaded_graph = GraphUtilsEx::CreateGraphFromComputeGraph(model.GetGraph());
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
