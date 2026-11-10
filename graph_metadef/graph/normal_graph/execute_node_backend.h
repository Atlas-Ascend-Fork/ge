/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_NODE_BACKEND_H_
#define METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_NODE_BACKEND_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graph/ge_attr_value.h"
#include "normal_graph/gnode_impl.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/graph.h"
#include "graph/gnode.h"

namespace ge {
class ExecuteGraph;
class FastNode;
// Execution-node state and operations behind the common GNode interface.
class ExecuteNodeBackend : public NodeImpl {
 public:
  ExecuteNodeBackend(const std::shared_ptr<ExecuteGraph> &owner, const ExecuteGraph::NodeHandle &handle)
      : owner_(owner), handle_(handle) {}
  using NodeImpl::GetDesc;
  static OpDescPtr GetDesc(const GNode &node);
  std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const override;
  std::vector<GNodePtr> GetInControlNodes() const override;
  std::vector<GNodePtr> GetOutControlNodes() const override;
  std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const override;
  graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const override;
  graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) const override;
  // Shared by GNode mounting and Operator graph construction; reuses existing slot indexes.
  static graphStatus MountSubgraph(const std::shared_ptr<ExecuteGraph> &owner, FastNode *fast_node,
                                   const AscendString &ir_name, const std::shared_ptr<ExecuteGraph> &subgraph);

 protected:
  NodeContext ResolveNode() const override;
  Operator CreateOperator(const NodeContext &node) const override;
  GraphContext LookupSubgraph(const NodeContext &node, uint32_t index) const override;
  GraphContext LookupRootGraph(const NodeContext &node) const override;
  std::vector<GraphContext> ListSubgraphs(const GraphContext &root) const override;
  GraphPtr WrapGraph(const GraphContext &graph) const override;
  graphStatus ResolveConstInput(const NodeContext &node, int32_t index, NodeContext &source) const override;

 private:
  std::vector<GNodePtr> GetControlNodes(bool input) const;
  static graphStatus SetSubgraphInner(const std::shared_ptr<ExecuteGraph> &owner, FastNode *parent_node, uint32_t slot,
                                      const std::shared_ptr<ExecuteGraph> &subgraph);
  friend class NodeAdapter;
  FastNode *GetNode() const;
  std::weak_ptr<ExecuteGraph> owner_;
  ExecuteGraph::NodeHandle handle_;
};

}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_NODE_BACKEND_H_
