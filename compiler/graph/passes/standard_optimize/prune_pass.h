/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_GRAPH_PASSES_PRUNE_PASS_H_
#define GE_GRAPH_PASSES_PRUNE_PASS_H_

#include "graph/passes/graph_pass.h"

namespace ge {
class PrunePass : public GraphPass {
 public:
  // kStandard：第1/2轮剪枝（PrepareOptimize/OptimizeStage1_3），剪死链并补控制边保活；
  // kAfterEngineAssigned：第3轮剪枝（OptimizeStage2，引擎分配后），prune-only：按引擎归属
  // 保活，受保护节点原地保留不补控制边
  enum class Round { kStandard, kAfterEngineAssigned };
  explicit PrunePass(const Round round = Round::kStandard) : round_(round) {}
  Status Run(ge::ComputeGraphPtr graph) override;

 private:
  Round round_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_PRUNE_PASS_H_
