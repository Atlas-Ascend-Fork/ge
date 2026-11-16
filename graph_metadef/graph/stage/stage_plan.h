/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_STAGE_STAGE_PLAN_H_
#define METADEF_GRAPH_STAGE_STAGE_PLAN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "graph/fast_graph/execute_graph.h"
#include "stage/stage_types.h"

namespace ge {
// 单个节点的阶段判定结果。
struct NodeStageInfo {
  StageKind stage{StageKind::kMain};
  StageSource source{StageSource::kDefault};
  // 判定依据（属性名/内置表/前驱节点名/配对资源名），用于诊断。
  std::string reason;
};

// 阶段判定结果：记录逻辑图及节点到阶段的映射。
class StagePlan {
 public:
  StagePlan() = default;
  explicit StagePlan(const ExecuteGraph *graph) : graph_(graph) {}

  const ExecuteGraph *GetGraph() const {
    return graph_;
  }
  // 未登记的节点按默认 Main 处理，判定失败时不产生半成品阶段。
  StageKind GetStage(const FastNode &node) const;
  const NodeStageInfo *FindInfo(const FastNode &node) const;
  void SetStage(const FastNode &node, StageKind stage, StageSource source, const std::string &reason);

  const std::map<const FastNode *, NodeStageInfo> &GetAllStages() const {
    return infos_;
  }
  std::vector<const FastNode *> GetNodesOfStage(StageKind stage) const;

 private:
  const ExecuteGraph *graph_{nullptr};
  std::map<const FastNode *, NodeStageInfo> infos_;
};

// 阶段判定：显式属性 > 内置映射 > 默认 Main，随后补加载前驱闭包、校验非法阶段依赖、配对资源释放阶段。
// 判定只读取逻辑图，不修改图结构；失败返回失败状态并给出冲突节点与原因。
class StageAnalyzer {
 public:
  explicit StageAnalyzer(const ExecuteGraph &graph) : graph_(graph) {}
  ~StageAnalyzer() = default;

  // 全量判定。失败时 plan 保持未提交状态由调用方丢弃。
  graphStatus Analyze(StagePlan &plan);

  // 单个节点的显式阶段属性读取，供外部诊断与测试使用。
  static graphStatus ReadStageAttribute(const FastNode &node, StageKind &stage);

 private:
  graphStatus AssignInitialStages(StagePlan &plan) const;
  graphStatus CloseInitPredecessors(StagePlan &plan) const;
  graphStatus ValidateStageDependencies(const StagePlan &plan) const;
  graphStatus ResolveReleaseStages(StagePlan &plan) const;

  const ExecuteGraph &graph_;
};
}  // namespace ge

#endif  // METADEF_GRAPH_STAGE_STAGE_PLAN_H_
