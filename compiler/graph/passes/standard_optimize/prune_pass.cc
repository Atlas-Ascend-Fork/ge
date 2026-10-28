/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "graph/passes/standard_optimize/prune_pass.h"
#include <deque>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "common/ge_common/ge_types.h"
#include "common/op/transop_util.h"
#include "framework/common/debug/log.h"
#include "framework/common/framework_types_internal.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_type_utils.h"

namespace ge {
namespace {
using NodeSet = std::unordered_set<NodePtr>;
// 候选仅包含普通控制边；每个节点只有一对控制锚点，可按目标节点记录候选源节点。
using PruneSources = std::unordered_map<NodePtr, NodeSet>;

struct PruneGraphInfo {
  // 当前 ComputeGraph 的直接节点全集 V，不包含子图内部节点。
  std::vector<NodePtr> nodes;
  // 当前图的全部 NetOutput，分别用于恢复各出口的完成依赖。
  std::vector<NodePtr> outputs;
  // 反向依赖遍历的起点：全部 NetOutput 与保活节点。
  std::vector<NodePtr> roots;
  // 显式 target 及命中保护规则的节点，其上游依赖另行遍历得到保留闭包 H。
  NodeSet keep_alive;
};

struct PruneEdgeChanges {
  std::vector<std::pair<NodePtr, NodePtr>> add_edges;
  std::vector<std::pair<NodePtr, NodePtr>> remove_edges;
};

// 引擎规则保护调度和通信行为；GE Local 仍按现有结构保护策略整体保留。
const std::unordered_set<std::string> kKeepAliveEngines = {"DNN_VM_GE_LOCAL", "DNN_VM_RTS", "DNN_VM_RTS_FFTS_PLUS",
                                                           "DNN_HCCL"};
// 兼容已有算子库身份判定，不将算子库名当作引擎名。
const std::unordered_set<std::string> kKeepAliveKernelLibs = {kEngineNameGeLocal, kEngineNameRts, kEngineNameHccl};
// Hcom 算子由 HCCL 引擎规则保留；Horovod 同时兼容 GE 和图元数据中的两组类型名。
// 数据获取、打印和断言可能由 AICPU 执行，需要独立保护其状态更新和诊断行为。
const std::unordered_set<std::string> kKeepAliveOpTypes = {HVDCALLBACKALLREDUCE,
                                                           HVDCALLBACKALLGATHER,
                                                           HVDCALLBACKBROADCAST,
                                                           HVDWAIT,
                                                           "hvdCallbackAllreduce",
                                                           "hvdCallbackAllgather",
                                                           "hvdCallbackBroadcast",
                                                           "hvdWait",
                                                           GETNEXT,
                                                           DYNAMICGETNEXT,
                                                           DYNAMICGETNEXTV2,
                                                           "Print",
                                                           "PrintV2",
                                                           "PrintV3",
                                                           ASSERT};

// StreamLabel 参与分流及流激活关联，BatchLabel 标识分档子图，二者可能携带拓扑之外的调度语义。
// 此处将这两类非空属性统称为执行标签，仅用于保活判断，不做完整控制流分析。
bool HasExecutionLabel(const NodePtr &node) {
  std::string label;
  if (AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_STREAM_LABEL, label) && !label.empty()) {
    return true;
  }
  return AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_BATCH_LABEL, label) && !label.empty();
}

bool IsKeepAliveNode(const NodePtr &node) {
  const auto *op_desc = node->GetOpDescBarePtr();
  const auto &type = node->GetType();
  bool skip_prune = false;
  bool is_ref = false;
  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_SKIP_PRUNE_OPTIMIZE, skip_prune);
  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_REFERENCE, is_ref);

  if (skip_prune || is_ref || !op_desc->GetSubgraphInstanceNames().empty() || HasExecutionLabel(node)) {
    return true;
  }

  if ((kKeepAliveEngines.count(op_desc->GetOpEngineName()) != 0U) ||
      (kKeepAliveKernelLibs.count(op_desc->GetOpKernelLibName()) != 0U)) {
    return true;
  }

  return OpTypeUtils::IsDataNode(type) || OpTypeUtils::IsVariableNode(type) || OpTypeUtils::IsAssignLikeNode(type) ||
         TransOpUtil::IsTransOp(node) || (kKeepAliveOpTypes.count(type) != 0U);
}

// 枚举完整输入依赖，仅跳过候选控制边。
std::vector<NodePtr> GetPruneInputs(const NodePtr &node, const PruneSources &ignored) {
  std::vector<NodePtr> inputs;
  const auto sources = ignored.find(node);
  const auto *in_control = node->GetInControlAnchor().get();
  for (const auto *anchor : node->GetAllInAnchorsPtr()) {
    for (const auto *peer : anchor->GetPeerAnchorsPtr()) {
      const auto source = peer->GetOwnerNode();
      // 必须精确匹配控制锚点对，同一源节点的 OutData -> InControl 等混合依赖不能一起忽略。
      if ((sources != ignored.end()) && (anchor == in_control) && (peer == source->GetOutControlAnchor().get()) &&
          (sources->second.count(source) != 0U)) {
        continue;
      }
      inputs.push_back(source);
    }
  }
  return inputs;
}

// 反向遍历输入依赖，记录停止节点但不继续向上遍历。
NodeSet CollectPruneAncestors(const std::vector<NodePtr> &roots, const PruneSources &ignored, const NodeSet &stop) {
  NodeSet seen;
  std::deque<NodePtr> queue(roots.cbegin(), roots.cend());
  while (!queue.empty()) {
    const auto node = queue.front();
    queue.pop_front();
    if (!seen.insert(node).second || (stop.count(node) != 0U)) {
      continue;
    }
    for (const auto &input : GetPruneInputs(node, ignored)) {
      queue.push_back(input);
    }
  }
  return seen;
}

// 收集当前图的节点、出口及保活根。
Status CollectPruneGraphInfo(const ComputeGraphPtr &graph, PruneGraphInfo &info) {
  for (const auto &target : graph->GetGraphTargetNodesInfo()) {
    if (target != nullptr) {
      info.keep_alive.insert(target);
    }
  }
  for (const auto &node : graph->GetDirectNode()) {
    GE_CHECK_NOTNULL(node->GetOpDesc());
    info.nodes.push_back(node);
    if (node->GetType() == NETOUTPUT) {
      info.outputs.push_back(node);
      info.roots.push_back(node);
    } else if (IsKeepAliveNode(node) || (info.keep_alive.count(node) != 0U)) {
      info.keep_alive.insert(node);
      info.roots.push_back(node);
    }
  }
  return SUCCESS;
}

// 按原出口自动收集非保活源节点的普通控制边。
void CollectPrunableControlInputs(const PruneGraphInfo &info, PruneSources &ignored) {
  for (const auto &output : info.outputs) {
    for (const auto *anchor : output->GetInControlAnchor()->GetPeerOutControlAnchorsPtr()) {
      const auto source = anchor->GetOwnerNode();
      if (info.keep_alive.count(source) == 0U) {
        ignored[output].insert(source);
        GELOGD("[PrunePass] candidate control edge [%s] -> [%s] after engine assignment.", source->GetNamePtr(),
               output->GetNamePtr());
      }
    }
  }
}

// 从全部节点中找出不属于保留集合 H 的节点，得到删除集合 D。
NodeSet CollectDeadPruneNodes(const std::vector<NodePtr> &nodes, const NodeSet &seen) {
  NodeSet dead;
  for (const auto &node : nodes) {
    if (seen.count(node) == 0U) {
      dead.insert(node);
    }
  }
  return dead;
}

// 各原出口独立回溯保留边界，规划补边和删边，此时不修改图。
PruneEdgeChanges PlanPruneEdgeChanges(const PruneGraphInfo &info, const PruneSources &ignored, const NodeSet &seen) {
  PruneEdgeChanges changes;
  for (const auto &output : info.outputs) {
    const auto entry = ignored.find(output);
    if (entry == ignored.end()) {
      continue;
    }
    std::vector<NodePtr> sources(entry->second.cbegin(), entry->second.cend());
    const auto ancestors = CollectPruneAncestors(sources, ignored, seen);
    const auto inputs = GetPruneInputs(output, ignored);
    const NodeSet direct_inputs(inputs.cbegin(), inputs.cend());
    for (const auto &node : info.nodes) {
      if ((node == output) || (seen.count(node) == 0U) || (ancestors.count(node) == 0U) ||
          (direct_inputs.count(node) != 0U)) {
        continue;
      }
      if (entry->second.count(node) == 0U) {
        changes.add_edges.emplace_back(node, output);
      }
    }
    for (const auto &node : info.nodes) {
      // 候选源已在 H 时保留其原完成边；若另有直接依赖，则只移除冗余控制边。
      if ((entry->second.count(node) != 0U) && ((seen.count(node) == 0U) || (direct_inputs.count(node) != 0U))) {
        changes.remove_edges.emplace_back(node, output);
      }
    }
  }
  return changes;
}

// 先恢复必要的完成依赖，再移除可剪控制边。
Status ApplyPruneEdgeChanges(const PruneEdgeChanges &changes) {
  for (const auto &edge : changes.add_edges) {
    GE_CHK_GRAPH_STATUS_RET(GraphUtils::AddEdge(edge.first->GetOutControlAnchor(), edge.second->GetInControlAnchor()),
                            "Failed to restore completion from %s to %s.", edge.first->GetNamePtr(),
                            edge.second->GetNamePtr());
    GELOGD("[PrunePass] restored completion control edge [%s] -> [%s].", edge.first->GetNamePtr(),
           edge.second->GetNamePtr());
  }
  for (const auto &edge : changes.remove_edges) {
    const auto source_anchor = edge.first->GetOutControlAnchor();
    const auto output_anchor = edge.second->GetInControlAnchor();
    while (source_anchor->IsLinkedWith(output_anchor)) {
      GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveEdge(source_anchor, output_anchor),
                              "Failed to remove prunable completion from %s to %s.", edge.first->GetNamePtr(),
                              edge.second->GetNamePtr());
    }
    GELOGD("[PrunePass] removed candidate control edge [%s] -> [%s].", edge.first->GetNamePtr(),
           edge.second->GetNamePtr());
  }
  return SUCCESS;
}

// 先统一断边再批量移除，不旁路重连，也不连带删除保留集合中的常量输入。
Status RemoveDeadPruneNodes(const ComputeGraphPtr &graph, const std::vector<NodePtr> &nodes, const NodeSet &dead) {
  for (const auto &node : nodes) {
    if (dead.count(node) != 0U) {
      NodeUtils::UnlinkAll(*node);
      GELOGI(
          "[PrunePass] remove graph node [%s] after engine assignment, graph [%s], type [%s], engine [%s], "
          "kernel_lib [%s].",
          node->GetNamePtr(), graph->GetName().c_str(), node->GetTypePtr(),
          node->GetOpDescBarePtr()->GetOpEngineName().c_str(), node->GetOpDescBarePtr()->GetOpKernelLibName().c_str());
    }
  }
  GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveNodesWithoutRelink(graph, dead),
                          "Failed to remove unreachable nodes after engine assignment.");
  return SUCCESS;
}

Status PruneAfterEngineAssigned(const ComputeGraphPtr &graph) {
  PruneGraphInfo info;
  // 收集 nodes（全部节点）、outputs（出口）、keep_alive（保活节点）和 roots（出口与保活节点）。
  GE_CHK_STATUS_RET(CollectPruneGraphInfo(graph, info));
  if (info.outputs.empty()) {
    GELOGD("[PrunePass] skip graph [%s] after engine assignment: no NetOutput.", graph->GetName().c_str());
    return SUCCESS;
  }

  PruneSources ignored;
  // 按出口自动记录候选控制边的源节点 ignored，此时不删边。
  CollectPrunableControlInputs(info, ignored);

  // 从 roots 反向遍历全部输入依赖，忽略 ignored 中的控制边，得到保留集合 H（seen）。
  const auto seen = CollectPruneAncestors(info.roots, ignored, {});

  // 计算删除集合 D（dead）= 全部节点 nodes - 保留集合 H，此时不删除节点。
  const auto dead = CollectDeadPruneNodes(info.nodes, seen);

  // 各出口分别从候选源反向回溯，遇到 H 停止，规划保留边界到原出口的补边和候选控制边的删边。
  const auto changes = PlanPruneEdgeChanges(info, ignored, seen);
  GELOGI(
      "[PrunePass] prune plan for graph [%s] after engine assignment: nodes=%zu, outputs=%zu, keep_alive=%zu, "
      "retained=%zu, dead=%zu, add_edges=%zu, remove_edges=%zu.",
      graph->GetName().c_str(), info.nodes.size(), info.outputs.size(), info.keep_alive.size(), seen.size(),
      dead.size(), changes.add_edges.size(), changes.remove_edges.size());

  // 按计划先补充必要的完成控制边，再移除待删控制边，此时仍不删除节点。
  GE_CHK_STATUS_RET(ApplyPruneEdgeChanges(changes));

  // 统一断开 dead 中节点的全部连边，再批量移除节点，避免旁路重连和连带删除保留节点。
  GE_CHK_STATUS_RET(RemoveDeadPruneNodes(graph, info.nodes, dead));

  return SUCCESS;
}

bool IsTargetRefNode(const NodePtr &node) {
  GE_CHECK_NOTNULL(node);
  bool is_ref = false;
  (void)AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_REFERENCE, is_ref);

  if (is_ref && node->GetOutNodes().empty()) {
    GELOGD("Node %s is ref node without any output. Consider as target node of graph, skip prune.", node->GetNamePtr());
    return true;
  }
  return false;
}
}  // namespace
Status PrunePass::Run(ge::ComputeGraphPtr graph) {
  if (graph == nullptr) {
    REPORT_INNER_ERR_MSG("E19999", "Param graph is nullptr, check invalid");
    GELOGE(GE_GRAPH_ISNULL, "[Check][Param] input compute graph is NULL.");
    return GE_GRAPH_ISNULL;
  }
  GELOGD("PrunePass Start, graph is [%s]", graph->GetName().c_str());
  if (stage_ == Stage::kAfterEngineAssigned) {
    return PruneAfterEngineAssigned(graph);
  }
  std::vector<NodePtr> out_nodes;
  std::vector<NodePtr> force_skip_nodes;
  std::unordered_set<NodePtr> nodes;
  for (NodePtr &node_ptr : graph->GetDirectNode()) {
    GE_CHECK_NOTNULL(node_ptr->GetOpDesc());
    nodes.insert(node_ptr);
    if (node_ptr->GetOpDesc()->GetType() == NETOUTPUT || IsTargetRefNode(node_ptr)) {
      out_nodes.push_back(node_ptr);
      continue;
    }
    bool skip_prune{false};
    (void)AttrUtils::GetBool(node_ptr->GetOpDesc(), ATTR_NAME_SKIP_PRUNE_OPTIMIZE, skip_prune);
    if (skip_prune) {
      force_skip_nodes.push_back(node_ptr);
      GELOGI("[PrunePass] Node [%s] skip to prune", node_ptr->GetNamePtr());
    }
  }
  if (out_nodes.empty()) {
    GELOGW("graph [%s] does not contain NETOUTPUT type node,no return value. Do nothing!", graph->GetName().c_str());
    return ge::SUCCESS;
  }
  out_nodes.insert(out_nodes.cend(), force_skip_nodes.cbegin(), force_skip_nodes.cend());

  std::unordered_set<NodePtr> nodes_seen;
  for (NodePtr &node_ptr : out_nodes) {
    std::deque<NodePtr> queue;
    queue.push_back(node_ptr);
    nodes_seen.insert(node_ptr);
    while (!queue.empty()) {
      NodePtr node = queue.front();
      GE_CHECK_NOTNULL(node->GetOpDesc());
      queue.pop_front();
      for (auto &in_node : node->GetInAllNodes()) {
        if (nodes_seen.insert(in_node).second) {
          queue.push_back(in_node);
        }
      }
    }
  }

  for (auto &node_ptr : nodes) {
    if (nodes_seen.count(node_ptr) != 0) {
      continue;
    }
    if (OpTypeUtils::IsDataNode(node_ptr->GetType())) {
      Status status = ge::GraphUtils::AddEdge(node_ptr->GetOutControlAnchor(), out_nodes[0]->GetInControlAnchor());
      if (status != ge::SUCCESS) {
        REPORT_INNER_ERR_MSG("E19999", "Add control edge between op:%s(%s) and op:%s(%s) failed",
                             node_ptr->GetName().c_str(), node_ptr->GetType().c_str(), out_nodes[0]->GetName().c_str(),
                             out_nodes[0]->GetType().c_str());
        GELOGE(INTERNAL_ERROR, "[add][ControlEdge] failed between DATA node[%s] and NETOUTPUT node[%s]!",
               node_ptr->GetName().c_str(), out_nodes[0]->GetName().c_str());
        return INTERNAL_ERROR;
      }
      GELOGI("[PrunePass] add extra control edge between DATA node[%s] and NETOUTPUT node[%s]!",
             node_ptr->GetName().c_str(), out_nodes[0]->GetName().c_str());
      continue;
    }
    // Remove subgraphs on the node before remove it in graph.
    (void)NodeUtils::RemoveSubgraphsOnNode(node_ptr);
    /// Common function:[RemoveNode] will delete not only input node but its constant input node also will be deleted
    (void)graph->RemoveNode(node_ptr);
    GELOGI("[PrunePass] remove graph node [%s]!", node_ptr->GetOpDesc()->GetName().c_str());
  }
  return ge::SUCCESS;
}

REG_PASS_OPTION("PrunePass").LEVELS(OoLevel::kO3);
}  // namespace ge
