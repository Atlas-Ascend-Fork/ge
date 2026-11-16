/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_CORE_BUILDER_EXECUTOR_SELECTION_POLICY_H_
#define AIR_CXX_RUNTIME_V2_CORE_BUILDER_EXECUTOR_SELECTION_POLICY_H_

#include <cstddef>

#include "framework/common/ge_visibility.h"
#include "framework/runtime/executor_option/executor_option.h"
#include "graph/fast_graph/execute_graph.h"
#include "core/builder/node_types.h"

namespace gert {
inline ExecutorType SelectDefaultExecutorType(const ge::ExecuteGraph &execute_graph) {
  constexpr size_t kLaunchNumLimit = 5UL;
  bool has_ctrl_node = false;
  size_t launch_num = 0UL;
  for (const auto node : execute_graph.GetAllNodes()) {
    const auto node_type = node->GetTypePtr();
    if (!has_ctrl_node && (IsIfOrCaseType(node_type) || IsWhileType(node_type))) {
      has_ctrl_node = true;
      continue;
    }
    if (IsLaunchNode(node_type)) {
      ++launch_num;
    }
  }
  if (!has_ctrl_node) {
    return ExecutorType::kSequentialPriority;
  }
  if (launch_num >= kLaunchNumLimit) {
    return ExecutorType::kTopologicalPriority;
  }
  return ExecutorType::kTopological;
}
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_CORE_BUILDER_EXECUTOR_SELECTION_POLICY_H_
