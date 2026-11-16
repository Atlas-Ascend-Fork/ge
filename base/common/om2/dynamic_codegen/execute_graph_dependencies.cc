/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "execute_graph_dependencies.h"

#include <algorithm>
#include <unordered_set>

#include "common/checker.h"
#include "core/executor/multi_thread_topological/execution_data/free_launch_relation.h"

namespace gert {
namespace om2 {
// om2动态当前限制：Discuss full submission protection support. Preserve Launch-to-Free ordering until then.
ge::graphStatus RestoreLaunchFreeDependencies(ge::ExecuteGraph &root_graph) {
  auto graphs = root_graph.GetAllSubgraphs();
  graphs.emplace_back(&root_graph);
  bool changed = false;
  for (auto *const graph : graphs) {
    GE_ASSERT_NOTNULL(graph);
    const auto relations = graph->GetExtAttr<FreeLaunchRelations>(kFreeLaunchRelationsAttr);
    if (relations == nullptr || relations->empty()) {
      continue;
    }
    const auto direct_nodes = graph->GetDirectNode();
    const std::unordered_set<ge::FastNode *> nodes(direct_nodes.cbegin(), direct_nodes.cend());
    for (const auto &relation : *relations) {
      auto *const free_node = relation.first;
      auto *const launch_node = relation.second;
      GE_ASSERT_NOTNULL(free_node);
      GE_ASSERT_NOTNULL(launch_node);
      GE_ASSERT_TRUE(free_node != launch_node);
      GE_ASSERT_TRUE(nodes.count(free_node) != 0U && nodes.count(launch_node) != 0U);
      const auto &predecessors = free_node->GetAllInNodes();
      if (std::find(predecessors.cbegin(), predecessors.cend(), launch_node) != predecessors.cend()) {
        continue;
      }
      GE_ASSERT_NOTNULL(graph->AddEdge(launch_node, ge::kControlEdgeIndex, free_node, ge::kControlEdgeIndex));
      changed = true;
    }
  }
  return changed ? root_graph.TopologicalSorting() : ge::GRAPH_SUCCESS;
}
}  // namespace om2
}  // namespace gert
