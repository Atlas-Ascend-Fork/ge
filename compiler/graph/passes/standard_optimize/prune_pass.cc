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
#include <unordered_set>
#include <vector>
#include "framework/common/debug/log.h"
#include "framework/common/framework_types_internal.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_type_utils.h"
#include "common/op/transop_util.h"

namespace ge {
namespace {
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

// 收集全量节点、图出口（NETOUTPUT/target ref）及显式跳过剪枝的节点
Status CollectPruneNodes(const ComputeGraphPtr &graph, std::unordered_set<NodePtr> &nodes,
                         std::vector<NodePtr> &out_nodes, std::vector<NodePtr> &force_skip_nodes) {
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
  return ge::SUCCESS;
}

// HCCL 算子类型前缀集合
const std::set<std::string> kKeepAliveOpTypePrefixes = {"Hcom", "hvd"};

bool IsKeepAliveOpTypePrefix(const std::string &type) {
  for (const auto &prefix : kKeepAliveOpTypePrefixes) {
    if (type.rfind(prefix, 0U) == 0U) {
      return true;
    }
  }
  return false;
}

// 保活节点：变量/Assign、集合通信（kKeepAliveOpTypePrefixes）、控制流骨架、含子图节点。
// 新增副作用算子应打 _force_skip_prune 标记，避免扩充本名单。
const std::unordered_set<std::string> kKeepAliveOpTypes = {VARHANDLEOP,
                                                           GETNEXT,
                                                           ENTER,
                                                           REFENTER,
                                                           NEXTITERATION,
                                                           REFNEXTITERATION,
                                                           SWITCH,
                                                           REFSWITCH,
                                                           SWITCHN,
                                                           MERGE,
                                                           REFMERGE,
                                                           IF,
                                                           _IF,
                                                           STATELESSIF,
                                                           WHILE,
                                                           _WHILE,
                                                           STATELESSWHILE,
                                                           CASE,
                                                           STATELESSCASE,
                                                           FOR,
                                                           PARTITIONEDCALL,
                                                           STATEFULPARTITIONEDCALL};

// 引擎分配后保活判定：框架/流控/主机/集合通信引擎节点不可剪；计算引擎（AI Core/Vector/AI CPU等）可剪。
const std::set<std::string> kKeepAliveKernelLibs = {kEngineNameGeLocal, kEngineNameRts, kEngineNameHccl};

bool IsProtectedEngineNode(const OpDescPtr &op_desc) {
  if (op_desc == nullptr) {
    return false;
  }
  const std::string kernel_lib_name = op_desc->GetOpKernelLibName();
  if (kernel_lib_name.empty()) {
    return false;  // 引擎未分配：维持既有类型白名单判定
  }
  return kKeepAliveKernelLibs.count(kernel_lib_name) > 0UL;
}

bool IsKeepAliveNode(const NodePtr &node) {
  if ((node == nullptr) || (node->GetOpDesc() == nullptr)) {
    return false;
  }
  const std::string &type = node->GetType();
  if (OpTypeUtils::IsVariableNode(type) || OpTypeUtils::IsAssignLikeNode(type) ||
      (kKeepAliveOpTypes.count(type) > 0UL) || IsKeepAliveOpTypePrefix(type)) {
    return true;
  }
  const auto &op_desc = node->GetOpDesc();
  // 无输入无输出算子（如 NPUClearFloatStatusV2）仅副作用存在，须保活
  if ((op_desc->GetAllInputsSize() == 0UL) && (op_desc->GetOutputsSize() == 0UL)) {
    return true;
  }
  bool is_ref = false;
  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_REFERENCE, is_ref);
  return is_ref || IsProtectedEngineNode(op_desc);
}

// 补控制边到图出口保证执行序；先去重避免重复边
Status AddCtrlEdgeToOutNode(const NodePtr &node, const NodePtr &out_node) {
  if (node->GetOutControlAnchor()->IsLinkedWith(out_node->GetInControlAnchor())) {
    return SUCCESS;
  }
  const auto ret = GraphUtils::AddEdge(node->GetOutControlAnchor(), out_node->GetInControlAnchor());
  if (ret != ge::SUCCESS) {
    REPORT_INNER_ERR_MSG("E19999", "Add control edge between op:%s(%s) and op:%s(%s) failed", node->GetName().c_str(),
                         node->GetType().c_str(), out_node->GetName().c_str(), out_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "Failed to add control edge between op %s(%s) and out node %s(%s).", node->GetName().c_str(),
           node->GetType().c_str(), out_node->GetName().c_str(), out_node->GetType().c_str());
    return INTERNAL_ERROR;
  }
  GELOGD("[PrunePass] add extra control edge between node[%s(%s)] and out node[%s(%s)]!", node->GetName().c_str(),
         node->GetType().c_str(), out_node->GetName().c_str(), out_node->GetType().c_str());
  return SUCCESS;
}

// 保活节点不可达时，沿入边反向收集输入闭包一并保活（不补控制边）
void CollectKeepAliveClosure(const std::unordered_set<NodePtr> &all_nodes,
                             const std::unordered_set<NodePtr> &nodes_seen, std::unordered_set<NodePtr> &keep_closure) {
  for (const auto &node_ptr : all_nodes) {
    if ((nodes_seen.count(node_ptr) != 0UL) || (keep_closure.count(node_ptr) != 0UL) || !IsKeepAliveNode(node_ptr)) {
      continue;
    }
    GELOGD("[PrunePass] keep input closure of keep-alive node [%s]!", node_ptr->GetNamePtr());
    std::deque<NodePtr> queue;
    queue.push_back(node_ptr);
    while (!queue.empty()) {
      NodePtr node = queue.front();
      queue.pop_front();
      for (auto &in_node : node->GetInAllNodes()) {
        if ((nodes_seen.count(in_node) == 0UL) && keep_closure.insert(in_node).second) {
          queue.push_back(in_node);
        }
      }
    }
  }
}

// 多输入转换算子（Reshape/Transpose等）的参数输入（shape/axes/perm，序号>=1）生产链
// 一并收入 keep_closure，避免保留的转换算子输入悬空。已保留/保活/已见节点跳过；
// VariableOpPass 融合后的残留链由后续轮次回收。
void CollectTransOpInputClosure(const NodePtr &trans_node, const std::unordered_set<NodePtr> &nodes_seen,
                                const std::unordered_set<NodePtr> &trans_closure,
                                std::unordered_set<NodePtr> &keep_closure) {
  std::deque<NodePtr> queue;
  queue.push_back(trans_node);
  while (!queue.empty()) {
    NodePtr node = queue.front();
    queue.pop_front();
    for (auto &in_node : node->GetInAllNodes()) {
      if ((nodes_seen.count(in_node) != 0UL) || (trans_closure.count(in_node) != 0UL) ||
          (keep_closure.count(in_node) != 0UL) || IsKeepAliveNode(in_node)) {
        continue;
      }
      keep_closure.insert(in_node);
      queue.push_back(in_node);
    }
  }
}

// 变量节点的输出转换链（Cast/TransData等）承载格式语义，VariableOpPass依赖其注册地址，
// 须一并保活并补控制边；非变量保活节点不在此列。多输入转换算子的参数输入链收入keep_closure。
void CollectKeepAliveTransClosure(const std::unordered_set<NodePtr> &all_nodes,
                                  const std::unordered_set<NodePtr> &nodes_seen,
                                  std::unordered_set<NodePtr> &trans_closure,
                                  std::unordered_set<NodePtr> &keep_closure) {
  for (const auto &node_ptr : all_nodes) {
    if ((nodes_seen.count(node_ptr) != 0UL) || (trans_closure.count(node_ptr) != 0UL) || !IsKeepAliveNode(node_ptr) ||
        !OpTypeUtils::IsVariableNode(node_ptr->GetType())) {
      continue;
    }
    std::deque<NodePtr> queue;
    queue.push_back(node_ptr);
    while (!queue.empty()) {
      NodePtr node = queue.front();
      queue.pop_front();
      for (auto &out_node : node->GetOutDataNodes()) {
        if ((nodes_seen.count(out_node) != 0UL) || !TransOpUtil::IsTransOp(out_node) ||
            (trans_closure.count(out_node) != 0UL)) {
          continue;
        }
        GELOGD("[PrunePass] keep trans closure of keep-alive node [%s]!", out_node->GetNamePtr());
        trans_closure.insert(out_node);
        queue.push_back(out_node);
        CollectTransOpInputClosure(out_node, nodes_seen, trans_closure, keep_closure);
      }
    }
  }
}

// 从图出口沿所有入边反向BFS标记可达节点
Status CollectSeenNodes(const std::vector<NodePtr> &out_nodes, std::unordered_set<NodePtr> &nodes_seen) {
  for (const NodePtr &node_ptr : out_nodes) {
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
  return ge::SUCCESS;
}

// 空闲节点：纯计算无副作用，其到 NetOutput 的控制边视为误连，删除后死链重新不可达
bool IsIdleNodeOnNetOutput(const NodePtr &node, const std::unordered_set<NodePtr> &protected_nodes) {
  if ((node == nullptr) || (node->GetOpDesc() == nullptr)) {
    return false;
  }
  if (OpTypeUtils::IsDataNode(node->GetType()) || IsKeepAliveNode(node)) {
    return false;  // Data序号绑定由处置环节兜底；保活节点的控制边承载真实执行语义
  }
  if (protected_nodes.count(node) != 0UL) {
    return false;  // 用户target/显式跳过剪枝节点的控制边表达真实执行意图
  }
  return true;
}

// 剪除 NetOutput 控制入边上空闲节点的误连边；真实语义的控制边保留
Status RemoveIdleCtrlEdgesOnNetOutput(const ComputeGraphPtr &graph, const std::vector<NodePtr> &out_nodes,
                                      const std::vector<NodePtr> &force_skip_nodes) {
  // 控制边不可删的保护集合：用户target与显式跳过剪枝节点
  std::unordered_set<NodePtr> protected_nodes(force_skip_nodes.cbegin(), force_skip_nodes.cend());
  for (const auto &target_node : graph->GetGraphTargetNodesInfo()) {
    if (target_node != nullptr) {
      protected_nodes.insert(target_node);
    }
  }
  for (const auto &out_node : out_nodes) {
    if ((out_node->GetType() != NETOUTPUT) || (out_node->GetInControlAnchor() == nullptr)) {
      continue;
    }
    // 先去重收集控制源节点，避免遍历中修改连接关系；循环删至删空
    std::vector<NodePtr> ctrl_in_nodes;
    std::unordered_set<NodePtr> collected_nodes;
    for (const auto *peer_anchor : out_node->GetInControlAnchor()->GetPeerOutControlAnchorsPtr()) {
      const auto src_node = (peer_anchor == nullptr) ? nullptr : peer_anchor->GetOwnerNode();
      if ((src_node != nullptr) && collected_nodes.insert(src_node).second) {
        ctrl_in_nodes.push_back(src_node);
      }
    }
    for (const auto &src_node : ctrl_in_nodes) {
      if (!IsIdleNodeOnNetOutput(src_node, protected_nodes)) {
        continue;
      }
      while (src_node->GetOutControlAnchor()->IsLinkedWith(out_node->GetInControlAnchor())) {
        const auto ret = GraphUtils::RemoveEdge(src_node->GetOutControlAnchor(), out_node->GetInControlAnchor());
        if (ret != GRAPH_SUCCESS) {
          REPORT_INNER_ERR_MSG("E19999", "Remove ctrl edge between op:%s(%s) and op:%s(%s) failed",
                               src_node->GetName().c_str(), src_node->GetType().c_str(), out_node->GetName().c_str(),
                               out_node->GetType().c_str());
          GELOGE(INTERNAL_ERROR, "Failed to remove control edge between op %s(%s) and NetOutput %s(%s).",
                 src_node->GetName().c_str(), src_node->GetType().c_str(), out_node->GetName().c_str(),
                 out_node->GetType().c_str());
          return INTERNAL_ERROR;
        }
        GELOGD("[PrunePass] remove idle ctrl edge between node[%s(%s)] and NetOutput[%s(%s)]!",
               src_node->GetName().c_str(), src_node->GetType().c_str(), out_node->GetName().c_str(),
               out_node->GetType().c_str());
      }
    }
  }
  return SUCCESS;
}

// 处置不可达节点：Data/保活/闭包/转换链节点保留（rescue_with_ctrl_edge 时补控制边），
// 其余删除。第3轮 prune-only 不补边，避免破坏 hybrid 执行器数据流调度（V1 Loop 死锁）。
Status DisposeUnreachableNodes(const ComputeGraphPtr &graph, const std::unordered_set<NodePtr> &nodes,
                               const std::unordered_set<NodePtr> &nodes_seen,
                               const std::unordered_set<NodePtr> &keep_closure,
                               const std::unordered_set<NodePtr> &trans_closure, const std::vector<NodePtr> &out_nodes,
                               const bool rescue_with_ctrl_edge) {
  for (const auto &node_ptr : nodes) {
    if (nodes_seen.count(node_ptr) != 0) {
      continue;
    }
    if (OpTypeUtils::IsDataNode(node_ptr->GetType())) {
      if (rescue_with_ctrl_edge) {
        GE_CHK_STATUS_RET(AddCtrlEdgeToOutNode(node_ptr, out_nodes[0]),
                          "Failed to add control edge between DATA node and out node.");
      }
      continue;
    }
    // 保活节点因消费者全死而不可达时，救援并补控制边
    if (IsKeepAliveNode(node_ptr)) {
      if (rescue_with_ctrl_edge) {
        GE_CHK_STATUS_RET(AddCtrlEdgeToOutNode(node_ptr, out_nodes[0]),
                          "Failed to add control edge between keep-alive node and out node.");
      }
      continue;
    }
    if (keep_closure.count(node_ptr) != 0UL) {
      continue;
    }
    // 转换链闭包节点保留并补控制边
    if (trans_closure.count(node_ptr) != 0UL) {
      if (rescue_with_ctrl_edge) {
        GE_CHK_STATUS_RET(AddCtrlEdgeToOutNode(node_ptr, out_nodes[0]),
                          "Failed to add control edge between trans closure node and out node.");
      }
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
}  // namespace
Status PrunePass::Run(ge::ComputeGraphPtr graph) {
  if (graph == nullptr) {
    REPORT_INNER_ERR_MSG("E19999", "Param graph is nullptr, check invalid");
    GELOGE(GE_GRAPH_ISNULL, "[Check][Param] input compute graph is NULL.");
    return GE_GRAPH_ISNULL;
  }
  GELOGD("PrunePass Start, graph is [%s]", graph->GetName().c_str());
  std::vector<NodePtr> out_nodes;
  std::vector<NodePtr> force_skip_nodes;
  std::unordered_set<NodePtr> nodes;
  GE_CHK_STATUS_RET(CollectPruneNodes(graph, nodes, out_nodes, force_skip_nodes),
                    "Failed to collect prune nodes from graph %s.", graph->GetName().c_str());
  if (out_nodes.empty()) {
    GELOGW("graph [%s] does not contain NETOUTPUT type node,no return value. Do nothing!", graph->GetName().c_str());
    return ge::SUCCESS;
  }
  out_nodes.insert(out_nodes.cend(), force_skip_nodes.cbegin(), force_skip_nodes.cend());

  // 先剪除 NetOutput 上空闲节点的误连控制边
  GE_CHK_STATUS_RET(RemoveIdleCtrlEdgesOnNetOutput(graph, out_nodes, force_skip_nodes),
                    "Failed to remove idle control edges on NetOutput, graph %s.", graph->GetName().c_str());

  std::unordered_set<NodePtr> nodes_seen;
  GE_CHK_STATUS_RET(CollectSeenNodes(out_nodes, nodes_seen), "Failed to collect seen nodes from out nodes, graph %s.",
                    graph->GetName().c_str());

  // 不可达保活节点的输入闭包须一并保活，否则其输入悬空
  std::unordered_set<NodePtr> keep_closure;
  CollectKeepAliveClosure(nodes, nodes_seen, keep_closure);

  // 不可达变量节点的输出转换链及参数输入链一并保活
  std::unordered_set<NodePtr> trans_closure;
  CollectKeepAliveTransClosure(nodes, nodes_seen, trans_closure, keep_closure);

  GE_CHK_STATUS_RET(DisposeUnreachableNodes(graph, nodes, nodes_seen, keep_closure, trans_closure, out_nodes,
                                            round_ == Round::kStandard),
                    "Failed to dispose unreachable nodes, graph %s.", graph->GetName().c_str());
  return ge::SUCCESS;
}

REG_PASS_OPTION("PrunePass").LEVELS(OoLevel::kO3);
}  // namespace ge
