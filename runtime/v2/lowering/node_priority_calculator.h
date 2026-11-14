/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_NODE_PRIORITY_CALCULATOR_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_NODE_PRIORITY_CALCULATOR_H_
#include "graph/compute_graph.h"
#include "graph/fast_graph/execute_graph.h"
#include "exe_graph/lowering/graph_frame.h"
namespace gert {
namespace bg {
// 优先级计算只需要「执行图节点 → 计算节点 topo 序」这一份映射。
// 直接接收该映射，使不经过 GE lowering 的执行图也能算出优先级，
// 不必伪造 GraphFrame 或计算图环境。
class NodePriorityCalculator {
 public:
  explicit NodePriorityCalculator(const GraphFrame &frame);
  // OM2 外部执行图直接提供映射；RT2 仍通过 GraphFrame 获取。
  explicit NodePriorityCalculator(std::vector<int64_t> indexes_to_compute_node_id);
  ge::graphStatus CalcNodeExecutionPriorities(const std::vector<ge::FastNode *> &main_graph_nodes,
                                              const size_t root_all_nodes_cnt);

 private:
  // GraphFrame 适配路径使用；独立路径为空。
  const GraphFrame *frame_{nullptr};
  std::vector<int64_t> indexes_to_compute_node_id_;
};
}  // namespace bg
}  // namespace gert
#endif  // AIR_CXX_RUNTIME_V2_LOWERING_NODE_PRIORITY_CALCULATOR_H_
