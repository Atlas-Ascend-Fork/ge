/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_ORDER_COLLECTOR_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_ORDER_COLLECTOR_H_

#include <cstdint>
#include <vector>

#include "graph/fast_graph/execute_graph.h"

namespace gert {
namespace om2 {
enum class ExecutionOrderPolicy : uint8_t { kFifo, kPriority };

struct VirtualDependency {
  const ge::FastNode *src = nullptr;
  const ge::FastNode *dst = nullptr;
};

class ExecutionOrderCollector {
 public:
  ge::graphStatus Collect(const ge::ExecuteGraph &execute_graph,
                          const std::vector<VirtualDependency> &virtual_dependencies, ExecutionOrderPolicy policy,
                          std::vector<const ge::FastNode *> &ordered_nodes) const;
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_EXECUTION_ORDER_COLLECTOR_H_
