/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_H_
#define METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_H_

#include <string>
#include <vector>
#include "graph/graph.h"
#include "normal_graph/graph_impl.h"
#include "graph/compute_graph.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/graph_utils_ex.h"
#include "normal_graph/execute_graph_backend_utils.h"

namespace ge {
class Buffer;
// Node references follow the underlying graph lifetime across facade/backend wrappers.
class ExecuteGraphBackend : public GraphImpl {
 public:
  explicit ExecuteGraphBackend(const std::string &name);
  explicit ExecuteGraphBackend(const std::shared_ptr<ExecuteGraph> &graph);
  const std::shared_ptr<ExecuteGraph> &GetGraph() const {
    ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
    return graph_;
  }
  std::vector<GNode> GetDirectNode() const override;
  // 按真实图树递归枚举；节点引用按所属底层图共享，与单独查询保持一致。
  std::vector<GNode> GetAllNodes() const override;
  const std::string &GetName() const override;
  bool IsValid() const override;
  // Unsupported capabilities are rejected by the backend, not the public routing layer.
  graphStatus SetTargets(const std::vector<Operator> &targets) override;
  graphStatus AddOp(const ge::Operator &op) override;
  graphStatus FindOpByName(const std::string &name, ge::Operator &op) const override;
  graphStatus FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const override;
  graphStatus GetAllOpName(std::vector<std::string> &op_name) const override;
  void SetNeedIteration(bool need_iteration) override;
  graphStatus Dump(Graph::DumpFormat format, std::ostream &stream) const override;
  graphStatus DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const override;
  graphStatus SaveToFile(const std::string &file_name) const override;
  graphStatus SaveToMem(Buffer &buffer) const override;
  graphStatus LoadFromFile(const std::string &file_name, Graph &loaded_graph) const override;
  graphStatus LoadFromMem(const uint8_t *data, const size_t len, Graph &loaded_graph) const override;
  graphStatus LoadFromSerializedModelArray(const void *serialized_model, const size_t size,
                                           Graph &loaded_graph) const override;
  std::vector<GraphPtr> GetAllSubgraphs() const override;
  GraphPtr GetSubGraph(const char *name) const override;
  GraphPtr GetParentGraph() const override;
  GNodePtr GetParentNode() const override;
  graphStatus RemoveSubgraph(const char *name) override;
  graphStatus CopyFrom(const Graph &src_graph, Graph &copied_graph) const override;

  graphStatus InitGraph() override;
  ComputeGraphPtr GetComputeGraph() const override;
  void AdoptGraph(const ComputeGraphPtr &compute_graph) override;
  graphStatus RecoverOperators() override;
  graphStatus CopyOperatorsTo(GraphImpl &dst, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                              const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const override;

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
  void BuildGraphFromInputs(const std::vector<Operator> &inputs) override;
  bool IsCompatibleSubgraph(const Graph &subgraph) const override;
  graphStatus RegisterSubgraph(const Graph &subgraph) override;
  GNodePtr FindDirectNode(const AscendString &name) const override;
  ConstOpDescPtr GetOutputOpDesc(const GNode &node) const override;
  graphStatus SetGraphOutNodesInfo(const std::vector<std::pair<GNode, int32_t>> &outputs) override;
  void PrepareSetOutputs() override {
    ExecuteGraphBackendUtils::NormalizeParentChain(graph_.get());
  }

 private:
  std::vector<GNode> WrapNodes(const std::vector<FastNode *> &nodes) const;
  FastNode *FindDirectNodeByName(const AscendString &name) const;
  std::string name_;
  std::shared_ptr<ExecuteGraph> graph_;
};

}  // namespace ge

#endif  // METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_H_
