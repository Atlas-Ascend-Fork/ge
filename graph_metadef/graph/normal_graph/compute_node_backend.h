/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_
#define METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graph/gnode.h"
#include "normal_graph/gnode_impl.h"

namespace ge {
class NodeAdapter;
class Node;

// 计算节点后端：NodeImpl 的计算图实现，持底层 Node 弱引用，承接 GNode 在计算节点模式下的
// 节点解析、邻接、常量和子图操作；公共流程由 NodeImpl 实现。
// 节点引用由 NodeAdapter 晚绑定，节点生命周期仍由底层图管理。
class ComputeNodeBackend : public NodeImpl {
 public:
  ComputeNodeBackend() = default;

  std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const override;
  std::vector<GNodePtr> GetInControlNodes() const override;
  std::vector<GNodePtr> GetOutControlNodes() const override;
  std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const override;

  graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const override;
  graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) const override;

 protected:
  NodeContext ResolveNode() const override;
  Operator CreateOperator(const NodeContext &node) const override;
  GraphContext LookupSubgraph(const NodeContext &node, uint32_t index) const override;
  GraphContext LookupRootGraph(const NodeContext &node) const override;
  std::vector<GraphContext> ListSubgraphs(const GraphContext &root) const override;
  GraphPtr WrapGraph(const GraphContext &graph) const override;
  graphStatus ResolveConstInput(const NodeContext &node, int32_t index, NodeContext &source) const override;

 private:
  // 晚绑定：Node2GNode/GNode2Node 直接读写 node_ptr_
  friend class NodeAdapter;

  std::weak_ptr<Node> node_ptr_;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_COMPUTE_NODE_BACKEND_H_
