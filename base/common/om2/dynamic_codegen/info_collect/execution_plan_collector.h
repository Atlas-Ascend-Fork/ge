/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_PLAN_COLLECTOR_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_PLAN_COLLECTOR_H_

#include "chain_normalizer.h"
#include "graph/fast_graph/execute_graph.h"
#include "program_info.h"

namespace gert {
namespace om2 {
class ExecutionPlanCollector {
 public:
  ge::graphStatus Collect(ge::ExecuteGraph &execute_graph, const ChainNormalizer &chain_normalizer,
                          ProgramPlanType plan_type, ProgramBlock &program_block) const;

 private:
  struct CanonicalIfAnalysis;

  ge::graphStatus CollectSequential(ge::ExecuteGraph &execute_graph, const ChainNormalizer &chain_normalizer,
                                    ProgramBlock &program_block) const;
  ge::graphStatus CollectRegion(ge::ExecuteGraph &execute_graph, const ChainNormalizer &chain_normalizer,
                                uint32_t region_index, ProgramBlock &program_block) const;
  ge::graphStatus CollectKernel(const ge::FastNode &node, const ChainNormalizer &chain_normalizer,
                                ProgramBlock &program_block, uint32_t &kernel_index) const;
  ge::graphStatus AppendKernel(const ge::FastNode &node, const ChainNormalizer &chain_normalizer, uint32_t region_index,
                               ProgramBlock &program_block, uint32_t *appended_kernel_index = nullptr) const;
  ge::graphStatus AnalyzeIf(ge::FastNode &node, CanonicalIfAnalysis &analysis) const;
  ge::graphStatus AppendIf(const ge::FastNode &node, const ChainNormalizer &chain_normalizer,
                           const CanonicalIfAnalysis &analysis, uint32_t region_index,
                           ProgramBlock &program_block) const;
  ge::graphStatus AppendWhile(const ge::FastNode &node, const ge::ExecuteGraph &execute_graph) const;
  ge::graphStatus AppendSubgraphCall(const ge::FastNode &node, const ge::ExecuteGraph &execute_graph) const;
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_PLAN_COLLECTOR_H_
