/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_
#define METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "graph/compute_graph.h"
#include "graph/ge_attr_value.h"
#include "graph/graph.h"
#include "graph/gnode.h"

namespace ge {
class Buffer;
class ExecuteGraph;
class FastNode;
// GraphImpl：Graph 公共门面的多态状态层。类别即动态类型——计算/执行图后端各自继承本基类，
// 门面经 impl_ 调用公共流程或虚接口，不持有 kind 字段与路由分支；接入新图类别只需新增派生类。
// 门面层保留公共对象检查（impl_ 判空）与链式接口形式适配，公共流程在本层调用后端差异步骤。
class GraphImpl {
 public:
  GraphImpl(const GraphImpl &) = delete;
  GraphImpl &operator=(const GraphImpl &) = delete;
  virtual ~GraphImpl() = default;

  virtual bool IsValid() const = 0;
  virtual const std::string &GetName() const = 0;
  // 首次建图：创建空 ComputeGraph（原 GraphImpl::InitComputeGraph）
  virtual graphStatus InitGraph() = 0;
  // 惰性建图模板方法（非虚）：组合 IsValid/InitGraph 虚原语，Graph::SetValid 与后端建图路径共用
  graphStatus SetValid() {
    if (IsValid()) {
      return GRAPH_SUCCESS;
    }
    GELOGD("Inner graph is invalid, start to init at first");
    return InitGraph();
  }
  // 工具型虚接口：供 GraphUtils/GraphUtilsEx 按类型访问；执行类别实现时返回空或拒绝
  virtual ComputeGraphPtr GetComputeGraph() const = 0;
  // 图类别判定：公共流程据此选择日志文案，计算图后端沿用改写前文案
  virtual bool IsComputeGraphKind() const {
    return false;
  }
  // 桥接入口整体挂入既有计算图（原 compute_graph_ 直赋值语义）
  virtual void AdoptGraph(const ComputeGraphPtr &compute_graph) = 0;
  virtual graphStatus RecoverOperators() = 0;
  virtual graphStatus CopyFrom(const Graph &src_graph, Graph &copied_graph) const = 0;
  virtual graphStatus CopyOperatorsTo(GraphImpl &dst, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                                      const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const = 0;

  // ---- IR 构图（Operator 系） ----
  graphStatus SetInputs(const std::vector<Operator> &inputs);
  graphStatus SetOutputs(const std::vector<Operator> &outputs);
  graphStatus SetOutputs(const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexs);
  graphStatus SetOutputs(const std::vector<std::pair<Operator, std::string>> &outputs);
  graphStatus SetOutputs(const std::vector<std::pair<Operator, AscendString>> &outputs);
  graphStatus SetOutputs(const std::vector<std::pair<GNode, int32_t>> &outputs);
  virtual graphStatus SetTargets(const std::vector<Operator> &targets) = 0;
  virtual graphStatus AddOp(const ge::Operator &op) = 0;
  virtual graphStatus FindOpByName(const std::string &name, ge::Operator &op) const = 0;
  virtual graphStatus FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const = 0;
  virtual graphStatus GetAllOpName(std::vector<std::string> &op_name) const = 0;
  virtual void SetNeedIteration(bool need_iteration) = 0;

  // ---- 节点/边编辑（GNode 系） ----
  virtual std::vector<GNode> GetAllNodes() const = 0;
  virtual std::vector<GNode> GetDirectNode() const = 0;
  GNode AddNodeByOp(const Operator &op);
  graphStatus AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node, const int32_t dst_port_index);
  graphStatus AddControlEdge(GNode &src_node, GNode &dst_node);
  graphStatus RemoveNode(GNode &node, bool contain_subgraph);
  graphStatus RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node, const int32_t dst_port_index);
  GNodePtr FindNodeByName(const AscendString &node_name) const;

  // ---- 子图 ----
  virtual std::vector<GraphPtr> GetAllSubgraphs() const = 0;
  virtual GraphPtr GetSubGraph(const char *name) const = 0;
  graphStatus AddSubGraph(const Graph &subgraph);
  virtual graphStatus RemoveSubgraph(const char *name) = 0;
  virtual GraphPtr GetParentGraph() const = 0;
  virtual GNodePtr GetParentNode() const = 0;

  // ---- 属性 ----
  graphStatus SetAttr(const AscendString &name, const AttrValue &attr_value);
  graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const;

  // ---- 序列化（分发型：加载返回临时 Graph，由门面成功后替换） ----
  virtual graphStatus SaveToFile(const std::string &file_name) const = 0;
  virtual graphStatus SaveToMem(Buffer &buffer) const = 0;
  virtual graphStatus LoadFromFile(const std::string &file_name, Graph &loaded_graph) const = 0;
  virtual graphStatus LoadFromMem(const uint8_t *data, size_t len, Graph &loaded_graph) const = 0;
  virtual graphStatus LoadFromSerializedModelArray(const void *serialized_model, size_t size,
                                                   Graph &loaded_graph) const = 0;

  // ---- Dump ----
  virtual graphStatus Dump(Graph::DumpFormat format, std::ostream &o_stream) const = 0;
  virtual graphStatus DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const = 0;

 protected:
  GraphImpl() = default;
  // Typed references retain native nodes and owner graphs for the whole edit operation.
  struct NodeContext {
    NodePtr compute_node;
    ComputeGraphPtr compute_owner;
    FastNode *execute_node = nullptr;
    std::shared_ptr<ExecuteGraph> execute_owner;
    std::string name;
    std::string owner_name;
    bool HasOwner() const {
      return compute_owner != nullptr || execute_owner != nullptr;
    }
  };
  virtual bool ResolveNode(const GNode &node, NodeContext &context) const = 0;
  virtual GNode AddNodeFromDesc(const OpDescPtr &desc) = 0;
  virtual void RegisterAddedOperator(const Operator &) {}
  virtual graphStatus LinkDataEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) = 0;
  virtual graphStatus LinkControlEdge(const NodeContext &src, const NodeContext &dst) = 0;
  virtual graphStatus UnlinkEdge(const NodeContext &src, int32_t out, const NodeContext &dst, int32_t in) = 0;
  virtual bool IsNodeInScope(const NodeContext &node, bool recursively) const = 0;
  virtual void UnlinkNode(const NodeContext &node) = 0;
  virtual graphStatus EraseNode(const NodeContext &node) = 0;
  virtual void ClearNodeOwner(const NodeContext &) {}
  virtual AttrHolder *GetAttributeHolder() const = 0;

  virtual void BuildGraphFromInputs(const std::vector<Operator> &inputs) = 0;
  virtual void SetInputSize(uint32_t) {}
  virtual bool IsCompatibleSubgraph(const Graph &subgraph) const = 0;
  virtual graphStatus RegisterSubgraph(const Graph &subgraph) = 0;
  virtual void PrepareSetOutputs() {}
  // found 区分"未命中"与"命中后包装分配失败"两类结果
  virtual GNodePtr FindDirectNode(const AscendString &name, bool &found) const = 0;
  virtual ConstOpDescPtr GetOutputOpDesc(const GNode &node) const = 0;
  virtual graphStatus SetGraphOutNodesInfo(const std::vector<std::pair<GNode, int32_t>> &outputs) = 0;
  virtual std::string *GetUserOutputNames() {
    return nullptr;
  }
  virtual void SetUserDefinedOutput(const std::string &) {}
  virtual void SetOutputSize(uint32_t) {}

 private:
  graphStatus ResolveEdgeNodes(const GNode &src, const GNode &dst, NodeContext &src_context,
                               NodeContext &dst_context) const;
  graphStatus AppendUserOutput(const Operator &output, const std::vector<size_t> &indexes,
                               std::vector<std::pair<GNode, int32_t>> &output_nodes);
  graphStatus AppendNamedUserOutput(const Operator &output, const std::string &output_name,
                                    std::vector<std::pair<GNode, int32_t>> &output_nodes);
  void AppendUserOutputName(const AscendString &name, size_t index);
  void FinalizeUserOutputNames();
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_
