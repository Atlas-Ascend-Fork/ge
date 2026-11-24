/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "execution_order_collector.h"

#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "core/builder/node_types.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
bool IsRuntimeOrderNode(const ge::FastNode &node) {
  const auto *const type = node.GetTypePtr();
  return !(IsFeedType(type) || IsConstFeedType(type) || IsOutputType(type) || IsTypeOutputData(type) ||
           IsInnerDataType(type) || IsInnerOutput(type) || IsConstType(type));
}

class ReadyNodes {
 public:
  explicit ReadyNodes(const ExecutionOrderPolicy policy) : policy_(policy) {}

  void Push(const ge::FastNode *const node) {
    if (policy_ == ExecutionOrderPolicy::kFifo) {
      fifo_.push(node);
      return;
    }
    int64_t priority = std::numeric_limits<int64_t>::max();
    (void)ge::AttrUtils::GetInt(node->GetOpDescBarePtr(), "priority", priority);
    priority_nodes_.push_back({priority, node});
    SiftUp(priority_nodes_.size() - 1U);
  }

  bool Empty() const {
    return policy_ == ExecutionOrderPolicy::kFifo ? fifo_.empty() : priority_nodes_.empty();
  }

  const ge::FastNode *Pop() {
    if (policy_ == ExecutionOrderPolicy::kFifo) {
      auto *const node = fifo_.front();
      fifo_.pop();
      return node;
    }
    auto *const node = priority_nodes_.front().node;
    priority_nodes_.front() = priority_nodes_.back();
    priority_nodes_.pop_back();
    if (!priority_nodes_.empty()) {
      SiftDown(0U);
    }
    return node;
  }

 private:
  struct PriorityNode {
    int64_t priority;
    const ge::FastNode *node;
  };

  static bool IsLess(const PriorityNode &lhs, const PriorityNode &rhs) {
    return lhs.priority < rhs.priority;
  }

  void SiftUp(size_t index) {
    while (index > 0U) {
      const auto parent = (index - 1U) / 2U;
      if (!IsLess(priority_nodes_[index], priority_nodes_[parent])) {
        break;
      }
      std::swap(priority_nodes_[index], priority_nodes_[parent]);
      index = parent;
    }
  }

  void SiftDown(size_t index) {
    while (true) {
      const auto left = index * 2U + 1U;
      if (left >= priority_nodes_.size()) {
        break;
      }
      auto child = left;
      const auto right = left + 1U;
      if ((right < priority_nodes_.size()) && IsLess(priority_nodes_[right], priority_nodes_[left])) {
        child = right;
      }
      if (!IsLess(priority_nodes_[child], priority_nodes_[index])) {
        break;
      }
      std::swap(priority_nodes_[index], priority_nodes_[child]);
      index = child;
    }
  }

  ExecutionOrderPolicy policy_;
  std::queue<const ge::FastNode *> fifo_;
  std::vector<PriorityNode> priority_nodes_;
};

void VisitNode(const ge::FastNode *const node, std::unordered_map<const ge::FastNode *, size_t> &indegrees,
               ReadyNodes &ready_nodes) {
  auto &indegree = indegrees[node];
  --indegree;
  if (indegree == 0U) {
    ready_nodes.Push(node);
  }
}
}  // namespace

ge::graphStatus ExecutionOrderCollector::Collect(const ge::ExecuteGraph &execute_graph,
                                                 const std::vector<VirtualDependency> &virtual_dependencies,
                                                 ExecutionOrderPolicy policy,
                                                 std::vector<const ge::FastNode *> &ordered_nodes) const {
  ordered_nodes.clear();
  ReadyNodes ready_nodes(policy);
  std::unordered_map<const ge::FastNode *, size_t> indegrees;
  std::unordered_map<const ge::FastNode *, std::vector<const ge::FastNode *>> virtual_out_nodes;
  std::unordered_map<const ge::FastNode *, size_t> virtual_in_degrees;
  const auto direct_nodes = execute_graph.GetDirectNode();
  std::unordered_set<const ge::FastNode *> order_nodes;
  for (auto *const node : direct_nodes) {
    GE_ASSERT_NOTNULL(node);
    if ((policy == ExecutionOrderPolicy::kFifo) || IsRuntimeOrderNode(*node)) {
      order_nodes.insert(node);
    }
  }

  for (const auto &dependency : virtual_dependencies) {
    GE_ASSERT_NOTNULL(dependency.src);
    GE_ASSERT_NOTNULL(dependency.dst);
    if ((order_nodes.count(dependency.src) == 0U) || (order_nodes.count(dependency.dst) == 0U)) {
      continue;
    }
    virtual_out_nodes[dependency.src].push_back(dependency.dst);
    ++virtual_in_degrees[dependency.dst];
  }

  for (auto *const node : direct_nodes) {
    if (order_nodes.count(node) == 0U) {
      continue;
    }
    size_t indegree = 0U;
    for (const auto *const edge : node->GetAllInDataEdgesRef()) {
      if ((edge != nullptr) && (order_nodes.count(edge->src) > 0U)) {
        ++indegree;
      }
    }
    for (const auto *const edge : node->GetAllInControlEdgesRef()) {
      if ((edge != nullptr) && (order_nodes.count(edge->src) > 0U)) {
        ++indegree;
      }
    }
    const auto virtual_in_iter = virtual_in_degrees.find(node);
    if (virtual_in_iter != virtual_in_degrees.cend()) {
      indegree += virtual_in_iter->second;
    }
    indegrees[node] = indegree;
    if (indegree == 0U) {
      ready_nodes.Push(node);
    }
  }

  while (!ready_nodes.Empty()) {
    auto *const node = ready_nodes.Pop();
    ordered_nodes.push_back(node);

    const auto visit_edge = [&order_nodes, &indegrees, &ready_nodes](const ge::FastEdge *const edge) {
      if ((edge == nullptr) || (order_nodes.count(edge->dst) == 0U)) {
        return;
      }
      VisitNode(edge->dst, indegrees, ready_nodes);
    };
    for (const auto &output_edges : node->GetAllOutDataEdgesRef()) {
      for (const auto *const edge : output_edges) {
        visit_edge(edge);
      }
    }
    for (const auto *const edge : node->GetAllOutControlEdgesRef()) {
      visit_edge(edge);
    }
    const auto virtual_iter = virtual_out_nodes.find(node);
    if (virtual_iter != virtual_out_nodes.cend()) {
      for (auto *const dst : virtual_iter->second) {
        VisitNode(dst, indegrees, ready_nodes);
      }
    }
  }

  GE_ASSERT_TRUE(ordered_nodes.size() == order_nodes.size(),
                 "OM2 region ordering only visited %zu of %zu nodes in graph %s", ordered_nodes.size(),
                 order_nodes.size(), execute_graph.GetName().c_str());
  return ge::GRAPH_SUCCESS;
}
}  // namespace om2
}  // namespace gert
