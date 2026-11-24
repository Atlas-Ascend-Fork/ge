/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "stage/stage_plan.h"

#include <deque>
#include <set>
#include <unordered_set>
#include "common/checker.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "stage/builtin_stage_table.h"

namespace ge {
namespace {
std::string NodeName(const FastNode &node) {
  const auto desc = node.GetOpDescPtr();
  return (desc == nullptr) ? std::string() : desc->GetName();
}

std::string NodeType(const FastNode &node) {
  const auto desc = node.GetOpDescPtr();
  return (desc == nullptr) ? std::string() : desc->GetType();
}
}  // namespace

StageKind StagePlan::GetStage(const FastNode &node) const {
  const auto it = infos_.find(&node);
  return (it == infos_.end()) ? StageKind::kMain : it->second.stage;
}

const NodeStageInfo *StagePlan::FindInfo(const FastNode &node) const {
  const auto it = infos_.find(&node);
  return (it == infos_.end()) ? nullptr : &it->second;
}

void StagePlan::SetStage(const FastNode &node, StageKind stage, StageSource source, const std::string &reason) {
  infos_[&node] = NodeStageInfo{stage, source, reason};
}

std::vector<const FastNode *> StagePlan::GetNodesOfStage(StageKind stage) const {
  std::vector<const FastNode *> nodes;
  for (const auto &item : infos_) {
    if (item.second.stage == stage) {
      nodes.emplace_back(item.first);
    }
  }
  return nodes;
}

graphStatus StageAnalyzer::ReadStageAttribute(const FastNode &node, StageKind &stage) {
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  const auto *value = AttrUtils::GetStr(desc, gert::attr::kStage);
  if (value == nullptr) {
    return GRAPH_FAILED;
  }
  GE_ASSERT_TRUE(ParseStageKind(*value, stage), "Invalid %s value %s on node %s", gert::attr::kStage, value->c_str(),
                 NodeName(node).c_str());
  return GRAPH_SUCCESS;
}

graphStatus StageAnalyzer::AssignInitialStages(StagePlan &plan) const {
  // 优先级：显式属性 > 内置映射 > 默认 Main。
  for (auto *node : graph_.GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    const std::string type = NodeType(*node);
    StageKind attr_stage;
    // 属性缺失才回落到内置/默认规则；属性存在但取值非法必须失败，不能当作“未声明”。
    if (AttrUtils::HasAttr(node->GetOpDescPtr(), gert::attr::kStage)) {
      GE_ASSERT_SUCCESS(ReadStageAttribute(*node, attr_stage), "Read %s of node %s failed", gert::attr::kStage,
                        NodeName(*node).c_str());
      // 显式声明与不可变 Kernel 语义冲突时拒绝，而不是静默覆盖。
      StageKind builtin_stage = StageKind::kMain;
      GE_ASSERT_TRUE(!BuiltinStageTable::Lookup(type, builtin_stage) || (builtin_stage == attr_stage),
                     "Explicit stage %s of node %s conflicts with builtin stage %s for type %s",
                     StageKindToString(attr_stage), NodeName(*node).c_str(), StageKindToString(builtin_stage),
                     type.c_str());
      plan.SetStage(*node, attr_stage, StageSource::kAttribute, gert::attr::kStage);
      continue;
    }
    StageKind builtin_stage;
    if (BuiltinStageTable::Lookup(type, builtin_stage)) {
      plan.SetStage(*node, builtin_stage, StageSource::kBuiltin, type);
      continue;
    }
    plan.SetStage(*node, StageKind::kMain, StageSource::kDefault, "default main");
  }
  return GRAPH_SUCCESS;
}

graphStatus StageAnalyzer::CloseInitPredecessors(StagePlan &plan) const {
  // 加载节点所需的常量及可确定的加载准备前驱纳入 Init；遇到执行态依赖则在下一步报冲突。
  std::deque<const FastNode *> pending;
  for (const auto *node : plan.GetNodesOfStage(StageKind::kInit)) {
    pending.emplace_back(node);
  }
  std::set<const FastNode *> visited(pending.begin(), pending.end());
  while (!pending.empty()) {
    const auto *node = pending.front();
    pending.pop_front();
    for (const auto *edge : node->GetAllInDataEdgesRef()) {
      // 断边后槽位为 nullptr，按 FastNode 约定跳过。
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      const auto &info = plan.FindInfo(*edge->src);
      const StageKind src_stage = (info == nullptr) ? StageKind::kMain : info->stage;
      if (src_stage == StageKind::kInit) {
        continue;
      }
      // 显式执行节点与逐次下发或 Main 声明不被闭包覆盖。
      if (BuiltinStageTable::IsExecutionType(NodeType(*edge->src)) ||
          ((info != nullptr) && (info->source == StageSource::kAttribute))) {
        continue;
      }
      plan.SetStage(*edge->src, StageKind::kInit, StageSource::kClosure, NodeName(*node));
      if (visited.insert(edge->src).second) {
        pending.emplace_back(edge->src);
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus StageAnalyzer::ValidateStageDependencies(const StagePlan &plan) const {
  for (const auto &item : plan.GetAllStages()) {
    const auto *dst = item.first;
    const StageKind dst_stage = item.second.stage;
    for (const auto *edge : dst->GetAllInDataEdgesRef()) {
      if (edge == nullptr || edge->src == nullptr) {
        continue;
      }
      const StageKind src_stage = plan.GetStage(*edge->src);
      // 生命周期顺序 Init -> Main -> DeInit；逆序依赖无法在拆图后成立。
      GE_ASSERT_TRUE(src_stage <= dst_stage, "Node %s (%s) in stage %s depends on node %s produced in later stage %s",
                     NodeName(*dst).c_str(), NodeType(*dst).c_str(), StageKindToString(dst_stage),
                     NodeName(*edge->src).c_str(), StageKindToString(src_stage));
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus StageAnalyzer::ResolveReleaseStages(StagePlan &plan) const {
  // 同一 (生产者, 输出端口) 不能被两个释放节点重复释放；同节点不同端口是不同资源，允许分别释放。
  std::map<std::pair<const FastNode *, int32_t>, std::string> released_by;
  for (const auto &item : plan.GetAllStages()) {
    const auto *node = item.first;
    if (!BuiltinStageTable::IsFreeType(NodeType(*node))) {
      continue;
    }
    const auto &edges = node->GetAllInDataEdgesRef();
    bool has_producer = false;
    for (const auto *edge : edges) {
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      has_producer = true;
      const auto inserted = released_by.emplace(std::make_pair(edge->src, edge->src_output), NodeName(*node));
      GE_ASSERT_TRUE(inserted.second, "Resource %s:%d is released twice, by %s and %s", NodeName(*edge->src).c_str(),
                     edge->src_output, inserted.first->second.c_str(), NodeName(*node).c_str());
    }
    // 配对缺失：释放节点没有任何数据生产者，无法确定其生命周期归属。
    GE_ASSERT_TRUE(has_producer, "Free node %s (%s) has no paired resource producer", NodeName(*node).c_str(),
                   NodeType(*node).c_str());
    // 通用 Free 按消费者阶段配对：默认跟随其数据输入所在阶段，显式声明优先。
    if (item.second.source == StageSource::kAttribute) {
      continue;
    }
    for (const auto *edge : edges) {
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      const auto *producer_info = plan.FindInfo(*edge->src);
      if ((producer_info != nullptr) && (producer_info->source == StageSource::kAttribute)) {
        plan.SetStage(*node, producer_info->stage, StageSource::kRelease, NodeName(*edge->src));
        break;
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus StageAnalyzer::Analyze(StagePlan &plan) {
  StagePlan staged(&graph_);
  GE_ASSERT_SUCCESS(AssignInitialStages(staged));
  GE_ASSERT_SUCCESS(CloseInitPredecessors(staged));
  GE_ASSERT_SUCCESS(ResolveReleaseStages(staged));
  GE_ASSERT_SUCCESS(ValidateStageDependencies(staged));
  plan = staged;
  return GRAPH_SUCCESS;
}
}  // namespace ge
