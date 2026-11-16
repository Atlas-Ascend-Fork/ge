/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_PREPARER_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_PREPARER_H_

#include "framework/common/ge_visibility.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "exe_graph/lowering/lowering_opt.h"
#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
class FastNode;
}  // namespace ge

namespace gert {
class LoweringGlobalData;
namespace bg {
class GraphFrame;
}  // namespace bg

// Main 图到 Init 图的等价节点表。与 CEM 使用的类型一致，但不引入 GE lowering 上下文头。
using MainToInitNodeMap = std::unordered_map<ge::FastNode *, ge::FastNode *>;

// 执行图准备步骤的有效性记录。只由内部流程推进，不读图属性，因此用户无法伪造。
enum class PrepareStep : uint32_t {
  kOptimized = 0,
  kSorted = 1,
  kPrioritized = 2,
  kStepEnd = 3,
};

// 准备过程的外部输入。外部执行图没有 lowering 环境，这两项由调用方直接给出；
// GE lowering 路径改用带 GraphFrame/LoweringGlobalData 的重载，不需要伪造计算图。
struct ExecuteGraphPrepareInput {
  // CEM 需要的 Main→Init 等价节点。没有在 Init 中预先创建等价 allocator 时留空，
  // 语义与「查不到等价节点」一致。
  MainToInitNodeMap equivalant_nodes;
  // 计算节点 topo 序（下标即 kComputeNodeIndex）。为空时按执行图节点自身的
  // kComputeNodeIndex 推导，见 CollectIndexesToComputeNodeId。
  std::vector<int64_t> indexes_to_compute_node_id;
};

struct ExecuteGraphPrepareResult {
  std::vector<bool> done{std::vector<bool>(static_cast<size_t>(PrepareStep::kStepEnd), false)};

  bool IsDone(PrepareStep step) const {
    const auto index = static_cast<size_t>(step);
    return (index < done.size()) && done[index];
  }
  void MarkDone(PrepareStep step) {
    const auto index = static_cast<size_t>(step);
    if (index < done.size()) {
      done[index] = true;
    }
  }
  // 图内容变更后，该步骤及其后续步骤全部失效；优化结果是否仍有效由调用方判断。
  void InvalidateFrom(PrepareStep step) {
    for (size_t i = static_cast<size_t>(step); i < done.size(); ++i) {
      done[i] = false;
    }
  }
};

// 当前供 OM2 准备执行图；原 RT2 lowering 不调用此入口。
// 对一张根执行图执行优化、拓扑排序和优先级计算，并记录有效进度。
// 已完成步骤不重复执行：优化 pass 有副作用，重复运行会重复迁移节点。
// 两条重载分别接收外部图信息和 GE lowering 上下文，内部复用相同的准备步骤。
VISIBILITY_EXPORT ge::graphStatus PrepareExecuteGraph(ge::ExecuteGraph &graph, const LoweringOption &option,
                                                      const ExecuteGraphPrepareInput &input,
                                                      ExecuteGraphPrepareResult &result);
VISIBILITY_EXPORT ge::graphStatus PrepareExecuteGraph(ge::ExecuteGraph &graph, const LoweringOption &option,
                                                      const LoweringGlobalData &global_data,
                                                      const bg::GraphFrame &frame, ExecuteGraphPrepareResult &result);

// 按执行图节点上的 kComputeNodeIndex 计算「下标 → 计算节点 topo 序」映射。
// 未出现的下标保留 -1，由优先级计算按拓扑关系补充。
ge::graphStatus CollectIndexesToComputeNodeId(const std::vector<ge::FastNode *> &nodes, const size_t compute_node_num,
                                              std::vector<int64_t> &indexes_to_compute_node_id);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_PREPARER_H_
