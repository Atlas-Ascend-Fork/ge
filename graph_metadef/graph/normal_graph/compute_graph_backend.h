/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_GRAPH_BACKEND_H_
#define METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_GRAPH_BACKEND_H_

#include <iosfwd>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "graph/compute_graph.h"
#include "graph/ge_attr_value.h"
#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/utils/node_adapter.h"
#include "normal_graph/graph_impl.h"

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

namespace ge {
class Buffer;

// 计算图侧后端：GraphImpl 的计算图实现，持有 ComputeGraph 与构图期状态
// （Operator 登记表、输出名装配态），承接计算图模式的实现及公共流程所需的差异步骤。
// 身份天然在数据层（Node 为 shared_ptr），因而不设句柄登记表与子图门面缓存——
// 计算图门面保持即用即弃，与执行侧 ExecuteGraphBackend（裸指针桥接）不同，
// 对称性只在分发结构上成立，不在语义负载上成立。
class ComputeGraphBackend : public GraphImpl {
 public:
  ComputeGraphBackend(const ComputeGraphBackend &) = delete;
  ComputeGraphBackend &operator=(const ComputeGraphBackend &) = delete;

  explicit ComputeGraphBackend(const std::string &name);

  ~ComputeGraphBackend() override;

  bool IsValid() const override;
  // GraphImpl::GetComputeGraph 的计算图实现：原 GetGraph，类型化虚接口要求按基类签名命名
  ComputeGraphPtr GetComputeGraph() const override;
  bool IsComputeGraphKind() const override {
    return true;
  }
  const std::string &GetName() const override;
  // 首次建图：创建空 ComputeGraph（原 GraphImpl::InitComputeGraph）
  graphStatus InitGraph() override;
  // 桥接入口整体挂入既有计算图（原 compute_graph_ 直赋值语义）
  void AdoptGraph(const ComputeGraphPtr &compute_graph) override;

  // ---- IR 构图（Operator 系） ----
  static GraphPtr ConstructFromInputs(const std::vector<Operator> &inputs, const AscendString &name);
  graphStatus SetTargets(const std::vector<Operator> &targets) override;
  graphStatus AddOp(const ge::Operator &op) override;
  graphStatus FindOpByName(const std::string &name, ge::Operator &op) const override;
  graphStatus FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const override;
  graphStatus GetAllOpName(std::vector<std::string> &op_name) const override;
  void SetNeedIteration(bool need_iteration) override;

  // ---- 节点/边编辑（GNode 系） ----
  std::vector<GNode> GetAllNodes() const override;
  std::vector<GNode> GetDirectNode() const override;

  // ---- 子图（含公共 Graph/GNode 门面包装：计算侧身份在数据层，门面即用即弃） ----
  std::vector<GraphPtr> GetAllSubgraphs() const override;
  GraphPtr GetSubGraph(const char *name) const override;
  graphStatus RemoveSubgraph(const char *name) override;
  GraphPtr GetParentGraph() const override;
  GNodePtr GetParentNode() const override;

  // ---- 属性 ----

  // ---- 复制/恢复 ----
  static graphStatus CopyFrom(const Graph &src_graph, const std::string &graph_name, Graph &copied_graph);
  graphStatus CopyFrom(const Graph &src_graph, Graph &copied_graph) const override;
  graphStatus CopyOperatorsTo(GraphImpl &dst, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                              const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const override;
  graphStatus RecoverOperators() override;

  // ---- 序列化 ----
  static graphStatus SaveToFile(const ComputeGraphPtr &compute_graph, const std::string &file_name);
  static graphStatus SaveToMem(const ComputeGraphPtr &compute_graph, Buffer &buffer);
  static graphStatus LoadFromFile(Graph &loaded_graph, const std::string &file_name);
  static graphStatus LoadFromMem(Graph &loaded_graph, const uint8_t *data, const size_t len);
  static graphStatus LoadFromSerializedModelArray(Graph &loaded_graph, const void *serialized_model, const size_t size);
  graphStatus LoadFromFile(const std::string &file_name, Graph &loaded_graph) const override;
  graphStatus LoadFromMem(const uint8_t *data, const size_t len, Graph &loaded_graph) const override;
  graphStatus LoadFromSerializedModelArray(const void *serialized_model, const size_t size,
                                           Graph &loaded_graph) const override;
  // 分发入口（对象决定保存格式）：以自身 compute_graph_ 调用上述静态工具
  graphStatus SaveToFile(const std::string &file_name) const override;
  graphStatus SaveToMem(Buffer &buffer) const override;

  // ---- Dump ----
  graphStatus Dump(Graph::DumpFormat format, std::ostream &o_stream) const override;
  graphStatus DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const override;

 protected:
  bool ResolveNode(const GNode &node, NodeContext &context) const override;
  GNode AddNodeFromDesc(const OpDescPtr &desc) override;
  graphStatus LinkDataEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) override;
  graphStatus LinkControlEdge(const NodeContext &src, const NodeContext &dst) override;
  graphStatus UnlinkEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) override;
  bool IsNodeInScope(const NodeContext &node, bool recursively) const override;
  void UnlinkNode(const NodeContext &node) override;
  graphStatus EraseNode(const NodeContext &node) override;
  AttrHolder *GetAttributeHolder() const override;
  void RegisterAddedOperator(const Operator &op) override {
    (void)AddOp(op);
  }
  void ClearNodeOwner(const NodeContext &node) override;
  void BuildGraphFromInputs(const std::vector<Operator> &inputs) override;
  void SetInputSize(uint32_t size) override {
    compute_graph_->SetInputSize(size);
  }
  bool IsCompatibleSubgraph(const Graph &subgraph) const override;
  graphStatus RegisterSubgraph(const Graph &subgraph) override;
  GNodePtr FindDirectNode(const AscendString &name) const override;
  ConstOpDescPtr GetOutputOpDesc(const GNode &node) const override;
  graphStatus SetGraphOutNodesInfo(const std::vector<std::pair<GNode, int32_t>> &outputs) override;
  std::string *GetUserOutputNames() override {
    return &output_name_;
  }
  void SetUserDefinedOutput(const std::string &output_name) override {
    compute_graph_->SetUserDefOutput(output_name);
  }
  void SetOutputSize(uint32_t size) override {
    compute_graph_->SetOutputSize(size);
  }

 private:
  graphStatus RemoveEdgeByAnchor(const NodePtr &src_node_ptr, const int32_t src_port_index, const NodePtr &dst_node_ptr,
                                 const int32_t dst_port_index);
  graphStatus RemoveControlEdgeByAnchor(const NodePtr &src_node_ptr, const NodePtr &dst_node_ptr);
  graphStatus RemoveDataControlEdgeByAnchor(const NodePtr &src_node_ptr, const int32_t src_port_index,
                                            const NodePtr &dst_node_ptr);
  graphStatus RemoveDataEdgeByAnchor(const NodePtr &src_node_ptr, const int32_t src_port_index,
                                     const NodePtr &dst_node_ptr, const int32_t dst_port_index);
  template <typename NodeContainer>
  std::vector<GNode> WrapNodes(const NodeContainer &nodes) const {
    std::vector<GNode> graph_nodes;
    for (const auto &node : nodes) {
      graph_nodes.emplace_back(NodeAdapter::Node2GNode(node));
    }
    return graph_nodes;
  }

  std::string name_;
  std::string output_name_;
  std::map<std::string, ge::Operator> op_list_;
  ComputeGraphPtr compute_graph_{nullptr};
};

// Dump 格式转换工具（执行图只读 dump 借用同一套转换生成快照）
graphStatus ConvertComputeGraphToProto(Graph::DumpFormat format, const ComputeGraphPtr &compute_graph,
                                       google::protobuf::Message *const proto);
graphStatus ConvertComputeGraphToReadableStr(std::stringstream &readable_ss, const ComputeGraphPtr &compute_graph);
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_GRAPH_BACKEND_H_
