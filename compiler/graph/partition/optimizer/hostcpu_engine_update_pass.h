/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_GRAPH_PASSES_HOSTCPU_ENGINE_UPDATE_PASS_H_
#define GE_GRAPH_PASSES_HOSTCPU_ENGINE_UPDATE_PASS_H_

#include <unordered_map>
#include <vector>
#include "graph/passes/graph_pass.h"
#include "graph/partition/engine_place.h"

namespace ge {
class HostcpuEngineUpdatePass : public EngineReAssignPass {
 public:
  Status Run(const ComputeGraphPtr &graph, NodeEngineMap &node_atomic_engine_map,
             NodeEngineMap &node_composite_engine_map) override;

  // 在引擎分配前收集"将被下沉到 host"的节点并打上内部标记（不改引擎名/kernel lib），
  // 标记由调用方在自动融合结束后统一回收。
  // 供 AutoFusePass 在融合前保护这些小值链：若被融合成设备核，其输出值需要同步回读到 host，
  // 会在运行期引入跨设备同步，抵消融合收益。
  // 采集在动态图划分之前执行，不做动态图判定（划分标记此时尚未写入）；静态图上的小值链会被常量折叠，
  // 最坏只会减少融合机会，不影响正确性；采集失败时内部降级为空，不阻断调用方流程。
  // 不新增开关：复用本 pass 既有使能开关（HostShapeOptimizationPass / hostcpu_engine_update_pass /
  // ENABLE_RUNTIME_V2）；pass 关闭时收集结果为空，保护自动失效。
  static void CollectHostExecNodesBeforeEngineAssign(const ComputeGraphPtr &graph,
                                                     std::vector<NodePtr> &host_exec_nodes);

 private:
  void ClearNodeMapSource();
  // 采集（引擎分配前）的近似判定：仅在 before_engine_assign_ 为 true 时追加按算子类型识别的逻辑，
  // 非采集模式内部退化为 IsAnchorOp / IsExecOnDevice，历史路径行为不变。
  bool IsAnchorOpForCollect(const OpDescPtr &op_desc) const;
  bool IsExecOnDeviceForCollect(const OpDescPtr &op_desc) const;
  bool IsExecOnHost(const NodePtr &node);
  NodePtr GetParentNode(const NodePtr &node);
  bool CheckInputForHostExec(const NodePtr &node, const size_t idx);

  bool CheckAndMarkHostExec(const NodePtr &node, NodeEngineMap &node_atomic_engine_map,
                            NodeEngineMap &node_composite_engine_map);
  Status FindHostDataOfSubgraph(const ComputeGraphPtr &graph, std::deque<NodePtr> &q);
  Status FindHostDataOfPartitionCall(const ComputeGraphPtr &graph, std::deque<NodePtr> &q);
  Status FindHostInputOfControlV2Node(const NodePtr &node, std::deque<NodePtr> &q);
  Status FindAndMarkHostCpuNode(std::deque<NodePtr> &q, NodeEngineMap &node_atomic_engine_map,
                                NodeEngineMap &node_composite_engine_map);
  Status MarkHostTensorAsModelInput(const ComputeGraphPtr &graph);
  Status UpdateHostcpuEngine(const ComputeGraphPtr &graph, NodeEngineMap &node_atomic_engine_map,
                             NodeEngineMap &node_composite_engine_map, bool is_partition_call = false);

  std::set<NodePtr> host_exe_ops_;
  std::unordered_map<NodePtr, NodePtr> node_to_parent_node_;
  std::unordered_map<NodePtr, bool> is_node_execute_on_host_;
  bool before_engine_assign_{false};
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_HOSTCPU_ENGINE_UPDATE_PASS_H_
