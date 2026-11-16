/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_MULTITHREAD_PROGRAM_PLAN_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_MULTITHREAD_PROGRAM_PLAN_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/om2/dynamic_codegen/info_collect/program_info.h"
#include "core/executor/multi_thread_topological/executor/schedule/task/exec_task_type.h"

namespace gert {
namespace om2 {
struct WorkerProgressRequirement {
  size_t worker_index = 0U;
  uint32_t progress = 0U;
};

enum class ExecutionUnitKind : uint8_t {
  kKernel,
  kStructuredIsland,
};

struct PlannedExecutionUnit {
  ExecutionUnitKind kind = ExecutionUnitKind::kKernel;
  uint32_t kernel_index = kInvalidProgramIndex;
  uint32_t root_step_index = kInvalidProgramIndex;
  std::vector<WorkerProgressRequirement> waits;
};

struct StaticWorkerPlan {
  ExecTaskType type = ExecTaskType::NORMAL;
  std::vector<PlannedExecutionUnit> units;
};

struct MultiThreadProgramPlan {
  std::vector<StaticWorkerPlan> workers;
};

bool BuildMultiThreadProgramPlan(const ProgramBlock &program, const std::vector<ExecTaskType> &kernel_types,
                                 size_t thread_num, MultiThreadProgramPlan &plan);
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_MULTITHREAD_PROGRAM_PLAN_H_
