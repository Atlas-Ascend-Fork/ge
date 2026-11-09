/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_OPERATOR_GRAPH_BUILDER_H_
#define METADEF_GRAPH_NORMAL_GRAPH_OPERATOR_GRAPH_BUILDER_H_

#include <map>
#include <queue>
#include <vector>
#include "base/err_msg.h"
#include "common/checker.h"
#include "graph/graph.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "normal_graph/operator_impl.h"

namespace ge {
// Only the Operator connection algorithm is shared. Graph operations retain their backend semantics.
template <typename GraphOps>
class OperatorGraphBuilder {
 public:
  using NodeType = typename GraphOps::NodeType;
  using GraphPtr = typename GraphOps::GraphPtr;

  explicit OperatorGraphBuilder(const GraphPtr &graph) : graph_(graph) {}

  const std::map<OperatorImplPtr, NodeType> &GetAllNodesInfo() const {
    return all_nodes_info_;
  }

  graphStatus GenerateNode(const OperatorImplPtr &op_impl) {
    NodeType node;
    const graphStatus ret = AddNode(op_impl, node);
    if (ret != GRAPH_SUCCESS) {
      return ret;
    }
    return WalkAllSubgraphs(node, op_impl);
  }

  graphStatus WalkAllOperators(const std::vector<OperatorImplPtr> &inputs) {
    if (graph_ == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "graph_ is nullptr, check invalid.");
      GELOGE(GRAPH_FAILED, "[Check][Param] graph_ is null.");
      return GRAPH_FAILED;
    }
    std::queue<std::vector<OperatorImplPtr>> pending;
    pending.push(inputs);
    while (!pending.empty()) {
      const auto operators = pending.front();
      pending.pop();
      for (const auto &op_impl : operators) {
        if (op_impl == nullptr) {
          REPORT_INNER_ERR_MSG("E18888", "op_impl is nullptr, check invalid.");
          GELOGE(GRAPH_FAILED, "[Check][Param] Operator Impl is null.");
          return GRAPH_FAILED;
        }
        if (all_nodes_info_.find(op_impl) != all_nodes_info_.end()) {
          GELOGI("Node %s has been created.", op_impl->GetName().c_str());
          continue;
        }
        NodeType node;
        const graphStatus ret = AddNode(op_impl, node);
        if (ret != GRAPH_SUCCESS) {
          return ret;
        }
        std::vector<OperatorImplPtr> forward;
        for (const auto &out_link : op_impl->output_links_) {
          for (const auto &peer : out_link.second) {
            forward.emplace_back(peer.GetOwner());
          }
        }
        for (const auto &peer : op_impl->control_output_link_) {
          forward.emplace_back(peer.lock());
        }
        pending.push(forward);
        std::vector<OperatorImplPtr> backward;
        for (const auto &in_link : op_impl->input_link_) {
          backward.emplace_back(in_link.second.GetOwner());
        }
        for (const auto &peer : op_impl->control_input_link_) {
          backward.emplace_back(peer.lock());
        }
        pending.push(backward);
        // Capture neighbours before invoking user code, as in the original compute builder.
        if (WalkAllSubgraphs(node, op_impl) != GRAPH_SUCCESS) {
          return GRAPH_FAILED;
        }
      }
    }
    return GRAPH_SUCCESS;
  }

  graphStatus AddEdges() {
    for (const auto &entry : stable_all_nodes_info_) {
      const auto &op_impl = entry.first;
      const auto &src = entry.second;
      if ((op_impl == nullptr) || (src == nullptr)) {
        continue;
      }
      const auto out_links = op_impl->output_links_;
      const auto desc = op_impl->GetOpDescImpl();
      if (desc == nullptr) {
        REPORT_INNER_ERR_MSG("E18888", "Src operator impl's op_desc is nullptr, check invalid.");
        GELOGE(GRAPH_FAILED, "[Check][Param] Src operator impl's op_desc is null.");
        return GRAPH_FAILED;
      }
      for (const auto &output : out_links) {
        const auto index = desc->GetOutputIndexByName(output.first);
        if (index < 0) {
          REPORT_INNER_ERR_MSG("E18888", "Find output index by name:%s in op:%s failed", output.first.c_str(),
                               desc->GetNamePtr());
          GELOGE(GRAPH_FAILED, "[Get][Index] Find output index by name:%s failed", output.first.c_str());
          return GRAPH_FAILED;
        }
        const auto output_ret = GraphOps::CheckDataOutput(src, index);
        if (output_ret != GRAPH_SUCCESS) {
          return output_ret;
        }
        for (const auto &peer : output.second) {
          const auto dst = all_nodes_info_.find(peer.GetOwner());
          if (dst == all_nodes_info_.end()) {
            REPORT_INNER_ERR_MSG("E18888", "Find Dst node failed, op:%s.", desc->GetNamePtr());
            GELOGE(GRAPH_FAILED, "[Check][Param] Find Dst node failed.");
            return GRAPH_FAILED;
          }
          if (dst->second == nullptr) {
            continue;
          }
          const auto edge_ret = GraphOps::AddDataEdge(graph_, src, index, dst->second, peer.GetIndex());
          if (edge_ret != GRAPH_SUCCESS) {
            return edge_ret;
          }
        }
      }
      for (const auto &peer : op_impl->control_output_link_) {
        const auto dst = all_nodes_info_.find(peer.lock());
        if (dst == all_nodes_info_.end()) {
          REPORT_INNER_ERR_MSG("E18888", "Find Dst node failed.");
          GELOGE(GRAPH_FAILED, "[Check][Param] Find Dst node failed.");
          return GRAPH_FAILED;
        }
        if (dst->second == nullptr) {
          continue;
        }
        const auto ret = GraphOps::AddControlEdge(graph_, src, dst->second);
        if (ret != GRAPH_SUCCESS) {
          return ret;
        }
      }
    }
    return GRAPH_SUCCESS;
  }

 private:
  graphStatus AddNode(const OperatorImplPtr &op_impl, NodeType &node) {
    GE_ASSERT_NOTNULL(graph_);
    GE_ASSERT_NOTNULL(op_impl);
    node = GraphOps::CreateNode(graph_, op_impl->GetOpDescImpl());
    if (node == nullptr) {
      REPORT_INNER_ERR_MSG("E18888", "add node failed.");
      GELOGE(GRAPH_FAILED, "[Add][Node] failed.");
      return GRAPH_FAILED;
    }
    (void)all_nodes_info_.emplace(op_impl, node);
    (void)stable_all_nodes_info_.emplace(op_impl, node);
    return GRAPH_SUCCESS;
  }

  graphStatus WalkAllSubgraphs(const NodeType &node, const OperatorImplPtr &op_impl) {
    // Keep the source slot ordering and indexes, including dynamic slots such as branches10.
    for (const auto &slot : op_impl->GetOpDescImpl()->GetSubgraphNameIndexes()) {
      const auto builder = op_impl->GetSubgraphBuilder(slot.first);
      if (builder == nullptr) {
        GELOGW("[Check][Param] Node %s has no builder", op_impl->GetName().c_str());
        continue;
      }
      const Graph subgraph = builder();
      const auto ret = GraphOps::MountSubgraph(graph_, node, slot.first, slot.second, subgraph);
      if (ret != GRAPH_SUCCESS) {
        return ret;
      }
    }
    return GRAPH_SUCCESS;
  }

  struct OperatorCompareKey {
    bool operator()(const OperatorImplPtr &lhs, const OperatorImplPtr &rhs) const {
      if ((lhs == nullptr) || (rhs == nullptr)) {
        return false;
      }
      return lhs->GetName() < rhs->GetName();
    }
  };

  GraphPtr graph_;
  std::map<OperatorImplPtr, NodeType> all_nodes_info_;
  std::map<OperatorImplPtr, NodeType, OperatorCompareKey> stable_all_nodes_info_;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_OPERATOR_GRAPH_BUILDER_H_
