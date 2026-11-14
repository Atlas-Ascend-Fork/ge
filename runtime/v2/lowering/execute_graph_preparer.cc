/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "execute_graph_preparer.h"

#include <algorithm>
#include <cinttypes>
#include <functional>
#include <string>
#include <utility>

#include "common/checker.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "lowering/node_priority_calculator.h"
#include "lowering/pass/offline_optimizer.h"
#include "utils/rt2_utils.h"

namespace gert {
namespace {
// Main 图的直接节点参与优先级计算，全图节点数决定优先级数组大小。
// 这与 GE lowering 后的划分方式一致：根图只有 Init/Main/DeInit 三个容器节点。
ge::graphStatus GetMainGraphNodes(ge::ExecuteGraph &graph, std::vector<ge::FastNode *> &main_nodes) {
  main_nodes.clear();
  for (auto *const node : graph.GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    if ((node->GetTypePtr() == nullptr) ||
        (std::string(node->GetTypePtr()) != GetExecuteGraphTypeStr(ExecuteGraphType::kMain))) {
      continue;
    }
    const auto *main_graph = ge::FastNodeUtils::GetSubgraphFromNode(node, 0U);
    GE_ASSERT_NOTNULL(main_graph);
    // 与 GE lowering 路径一致：包含子图内节点，优先级计算只按 Launch 等类型筛选。
    main_nodes = main_graph->GetAllNodes();
    GE_ASSERT_TRUE(!main_nodes.empty(), "Main graph of %s is empty", graph.GetName().c_str());
    return ge::GRAPH_SUCCESS;
  }
  GELOGE(ge::GRAPH_PARAM_INVALID, "Execute graph %s has no Main container node", graph.GetName().c_str());
  return ge::GRAPH_PARAM_INVALID;
}
}  // namespace

ge::graphStatus CollectIndexesToComputeNodeId(const std::vector<ge::FastNode *> &nodes, const size_t compute_node_num,
                                              std::vector<int64_t> &indexes_to_compute_node_id) {
  std::vector<int64_t> collected(compute_node_num, -1);
  for (const auto *node : nodes) {
    GE_ASSERT_NOTNULL(node);
    const auto op_desc = node->GetOpDescBarePtr();
    GE_ASSERT_NOTNULL(op_desc);
    int64_t compute_node_index = 0;
    if (!ge::AttrUtils::GetInt(op_desc, kComputeNodeIndex, compute_node_index)) {
      continue;
    }
    GE_ASSERT_TRUE((compute_node_index >= 0) && (static_cast<size_t>(compute_node_index) < compute_node_num),
                   "Node %s compute node index %" PRId64 " exceeds %zu", node->GetNamePtr(), compute_node_index,
                   compute_node_num);
    auto &slot = collected[static_cast<size_t>(compute_node_index)];
    // 多个执行节点可以共用一个计算节点（例如拆出的 Init/Main 两半），但 topo 序必须一致。
    GE_ASSERT_TRUE((slot < 0) || (slot == op_desc->GetId()),
                   "Compute node index %" PRId64 " maps to conflicting topo ids %" PRId64 " and %" PRId64,
                   compute_node_index, slot, op_desc->GetId());
    slot = op_desc->GetId();
  }
  // 未出现的下标无法推导优先级，保留 -1 由调用方决定是否报错。
  indexes_to_compute_node_id = std::move(collected);
  return ge::GRAPH_SUCCESS;
}

namespace {
struct PrepareCallbacks {
  std::function<ge::graphStatus()> optimize;
  std::function<ge::graphStatus(std::vector<ge::FastNode *> &main_nodes, size_t root_all_nodes_cnt)> prioritize;
};

ge::graphStatus RunPrepareSteps(ge::ExecuteGraph &graph, const PrepareCallbacks &callbacks,
                                ExecuteGraphPrepareResult &result) {
  if (!result.IsDone(PrepareStep::kOptimized)) {
    GE_ASSERT_SUCCESS(callbacks.optimize());
    result.MarkDone(PrepareStep::kOptimized);
  } else {
    // 已优化：跳过有副作用的 pass，不重复迁移节点。
    GELOGI("Execute graph %s has been optimized, skip optimization passes", graph.GetName().c_str());
  }

  if (!result.IsDone(PrepareStep::kSorted)) {
    GE_ASSERT_SUCCESS(graph.TopologicalSorting());
    result.MarkDone(PrepareStep::kSorted);
  }

  if (!result.IsDone(PrepareStep::kPrioritized)) {
    // 必须在优化和排序之后取样：CEM 会向 Main 图新增节点，提前取样会漏掉这些节点。
    std::vector<ge::FastNode *> main_graph_nodes;
    GE_ASSERT_SUCCESS(GetMainGraphNodes(graph, main_graph_nodes));
    GE_ASSERT_SUCCESS(callbacks.prioritize(main_graph_nodes, graph.GetAllNodes().size()));
    result.MarkDone(PrepareStep::kPrioritized);
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus PrepareExecuteGraph(ge::ExecuteGraph &graph, const LoweringOption &option,
                                    const ExecuteGraphPrepareInput &input, ExecuteGraphPrepareResult &result) {
  PrepareCallbacks callbacks;
  callbacks.optimize = [&graph, &option, &input]() -> ge::graphStatus {
    return bg::OfflineOptimizer(option, input.equivalant_nodes).Run(&graph);
  };
  callbacks.prioritize = [&graph, &input](std::vector<ge::FastNode *> &main_nodes,
                                          const size_t root_all_nodes_cnt) -> ge::graphStatus {
    auto indexes_to_compute_node_id = input.indexes_to_compute_node_id;
    if (indexes_to_compute_node_id.empty()) {
      GE_ASSERT_SUCCESS(
          CollectIndexesToComputeNodeId(graph.GetAllNodes(), root_all_nodes_cnt, indexes_to_compute_node_id));
    }
    return bg::NodePriorityCalculator(std::move(indexes_to_compute_node_id))
        .CalcNodeExecutionPriorities(main_nodes, root_all_nodes_cnt);
  };
  return RunPrepareSteps(graph, callbacks, result);
}

ge::graphStatus PrepareExecuteGraph(ge::ExecuteGraph &graph, const LoweringOption &option,
                                    const LoweringGlobalData &global_data, const bg::GraphFrame &frame,
                                    ExecuteGraphPrepareResult &result) {
  PrepareCallbacks callbacks;
  callbacks.optimize = [&graph, &option, &global_data]() -> ge::graphStatus {
    return bg::OfflineOptimizer(option, global_data).Run(&graph);
  };
  callbacks.prioritize = [&frame](std::vector<ge::FastNode *> &main_nodes,
                                  const size_t root_all_nodes_cnt) -> ge::graphStatus {
    return bg::NodePriorityCalculator(frame).CalcNodeExecutionPriorities(main_nodes, root_all_nodes_cnt);
  };
  return RunPrepareSteps(graph, callbacks, result);
}
}  // namespace gert
