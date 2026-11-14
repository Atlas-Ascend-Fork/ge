/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "multithread_program_plan.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gert {
namespace om2 {
namespace {
struct KernelLocation {
  size_t conceptual_worker_index = 0U;
  uint32_t progress = 0U;
  size_t order = 0U;
  bool assigned = false;
};

struct UnitLocation {
  size_t worker_index = 0U;
  uint32_t progress = 0U;
  size_t order = 0U;
};

bool CollectRegionKernels(const ProgramBlock &program, const uint32_t region_index,
                          std::vector<uint32_t> &kernel_indices, std::vector<uint32_t> &active_regions) {
  if ((region_index >= program.regions.size()) ||
      (std::find(active_regions.cbegin(), active_regions.cend(), region_index) != active_regions.cend())) {
    return false;
  }
  active_regions.push_back(region_index);
  const auto &region = program.regions[region_index];
  for (const auto &step : region.steps) {
    if (step.kind == ProgramStepKind::kKernel) {
      if (step.kernel_index >= program.kernels.size()) {
        return false;
      }
      kernel_indices.push_back(step.kernel_index);
      continue;
    }
    if (step.kind == ProgramStepKind::kIf) {
      if (step.condition_region_index != kInvalidProgramIndex &&
          !CollectRegionKernels(program, step.condition_region_index, kernel_indices, active_regions)) {
        return false;
      }
      for (const auto &branch : step.if_info.branches) {
        if (!CollectRegionKernels(program, branch.region_index, kernel_indices, active_regions)) {
          return false;
        }
      }
      continue;
    }
    return false;
  }
  active_regions.pop_back();
  return true;
}

size_t GetNormalWorkerIndex(const size_t thread_num) {
  return thread_num > 2U ? 2U : 1U;
}

ExecTaskType GetWorkerType(const size_t worker_index, const size_t thread_num) {
  if (worker_index == 0U) {
    return ExecTaskType::MEMORY;
  }
  if ((thread_num > 2U) && (worker_index == 1U)) {
    return ExecTaskType::LAUNCH;
  }
  return ExecTaskType::NORMAL;
}
}  // namespace

bool BuildMultiThreadProgramPlan(const ProgramBlock &program, const std::vector<ExecTaskType> &kernel_types,
                                 const size_t thread_num, MultiThreadProgramPlan &plan) {
  plan = {};
  if ((thread_num < 2U) ||
      ((program.plan_type != ProgramPlanType::kSequential) &&
       (program.plan_type != ProgramPlanType::kStructuredTopological) &&
       (program.plan_type != ProgramPlanType::kStructuredTopologicalPriority)) ||
      (program.root_region_index >= program.regions.size()) || (kernel_types.size() != program.kernels.size())) {
    return false;
  }

  const auto &steps = program.regions[program.root_region_index].steps;
  std::vector<KernelLocation> kernel_locations(program.kernels.size());
  std::vector<PlannedExecutionUnit> units;
  std::vector<UnitLocation> unit_locations;
  std::unordered_map<uint32_t, size_t> kernel_to_unit;
  const bool structured = program.plan_type != ProgramPlanType::kSequential;
  const size_t normal_worker_start = thread_num > 2U ? 2U : 1U;
  const size_t normal_worker_count = thread_num > 2U ? thread_num - 2U : 1U;
  size_t next_normal_worker = 0U;
  for (size_t order = 0U; order < steps.size(); ++order) {
    const auto &step = steps[order];
    if ((step.kind != ProgramStepKind::kKernel) && (!structured || (step.kind != ProgramStepKind::kIf))) {
      return false;
    }

    size_t worker_index = 0U;
    PlannedExecutionUnit unit;
    unit.root_step_index = static_cast<uint32_t>(order);
    if (step.kind == ProgramStepKind::kIf) {
      worker_index = GetNormalWorkerIndex(thread_num);
      unit.kind = ExecutionUnitKind::kStructuredIsland;
    } else {
      if ((step.kernel_index >= program.kernels.size()) || kernel_locations[step.kernel_index].assigned ||
          (kernel_types[step.kernel_index] >= ExecTaskType::MAX)) {
        return false;
      }
      unit.kind = ExecutionUnitKind::kKernel;
      unit.kernel_index = step.kernel_index;
      switch (kernel_types[step.kernel_index]) {
        case ExecTaskType::MEMORY:
          worker_index = 0U;
          break;
        case ExecTaskType::LAUNCH:
          worker_index = thread_num > 2U ? 1U : normal_worker_start;
          break;
        case ExecTaskType::NORMAL:
          // Round-robin keeps lanes balanced. RT2's priority is a ready-queue ordering key;
          // static lanes have no queue, and affinity by priority would pile the (very common)
          // default-max priority band onto a single lane and serialize the workers.
          worker_index = normal_worker_start + (next_normal_worker % normal_worker_count);
          ++next_normal_worker;
          break;
        default:
          return false;
      }
    }

    const size_t unit_index = units.size();
    units.push_back(unit);
    unit_locations.push_back({worker_index, 0U, order});
    if (unit.kind == ExecutionUnitKind::kKernel) {
      kernel_locations[unit.kernel_index] = {worker_index, 0U, order, true};
      kernel_to_unit[unit.kernel_index] = unit_index;
    } else {
      std::vector<uint32_t> island_kernels;
      std::vector<uint32_t> active_regions;
      const auto condition_region_index = step.condition_region_index != kInvalidProgramIndex
                                              ? step.condition_region_index
                                              : step.if_info.condition_region_index;
      if ((condition_region_index != kInvalidProgramIndex) &&
          !CollectRegionKernels(program, condition_region_index, island_kernels, active_regions)) {
        return false;
      }
      for (const auto &branch : step.if_info.branches) {
        if (!CollectRegionKernels(program, branch.region_index, island_kernels, active_regions)) {
          return false;
        }
      }
      for (const auto kernel_index : island_kernels) {
        if (kernel_index >= program.kernels.size() || kernel_locations[kernel_index].assigned) {
          return false;
        }
        kernel_locations[kernel_index] = {worker_index, 0U, order, true};
        kernel_to_unit[kernel_index] = unit_index;
      }
    }
  }
  for (size_t i = 0U; i < program.kernels.size(); ++i) {
    if (!kernel_locations[i].assigned && (program.kernels[i].impl_type != KernelImplType::kEmptyImpl)) {
      return false;
    }
  }
  plan.workers.resize(thread_num);
  for (size_t worker_index = 0U; worker_index < thread_num; ++worker_index) {
    plan.workers[worker_index].type = GetWorkerType(worker_index, thread_num);
  }

  std::vector<std::pair<size_t, size_t>> unit_dependencies;
  const auto add_dependency = [&unit_dependencies, &unit_locations](const size_t predecessor, const size_t successor) {
    if ((predecessor == successor) ||
        (unit_locations[predecessor].worker_index == unit_locations[successor].worker_index)) {
      return;
    }
    unit_dependencies.emplace_back(predecessor, successor);
  };
  for (const auto &dependency : program.kernel_dependencies) {
    const auto predecessor_unit = kernel_to_unit.find(dependency.predecessor);
    const auto successor_unit = kernel_to_unit.find(dependency.successor);
    if ((predecessor_unit == kernel_to_unit.cend()) || (successor_unit == kernel_to_unit.cend())) {
      plan = {};
      return false;
    }
    const auto predecessor = predecessor_unit->second;
    const auto successor = successor_unit->second;
    if (predecessor == successor) {
      continue;
    }
    if (unit_locations[predecessor].order >= unit_locations[successor].order) {
      plan = {};
      return false;
    }
    if (unit_locations[predecessor].worker_index == unit_locations[successor].worker_index) {
      continue;
    }
    add_dependency(predecessor, successor);
  }

  for (size_t island_index = 0U; island_index < units.size(); ++island_index) {
    if (units[island_index].kind != ExecutionUnitKind::kStructuredIsland) {
      continue;
    }
    for (size_t unit_index = 0U; unit_index < units.size(); ++unit_index) {
      if (unit_locations[unit_index].order < unit_locations[island_index].order) {
        add_dependency(unit_index, island_index);
      } else if (unit_locations[unit_index].order > unit_locations[island_index].order) {
        add_dependency(island_index, unit_index);
      }
    }
  }

  std::vector<uint32_t> worker_progress(thread_num, 0U);
  for (size_t unit_index = 0U; unit_index < units.size(); ++unit_index) {
    unit_locations[unit_index].progress = ++worker_progress[unit_locations[unit_index].worker_index];
  }
  std::vector<std::map<size_t, uint32_t>> waits(units.size());
  for (const auto &dependency : unit_dependencies) {
    const auto predecessor = dependency.first;
    const auto successor = dependency.second;
    auto &required_progress = waits[successor][unit_locations[predecessor].worker_index];
    required_progress = std::max(required_progress, unit_locations[predecessor].progress);
  }
  for (size_t unit_index = 0U; unit_index < units.size(); ++unit_index) {
    auto &unit = units[unit_index];
    unit.waits.reserve(waits[unit_index].size());
    for (const auto &requirement : waits[unit_index]) {
      unit.waits.push_back({requirement.first, requirement.second});
    }
    auto &worker = plan.workers[unit_locations[unit_index].worker_index];
    worker.units.emplace_back(std::move(unit));
  }
  return true;
}
}  // namespace om2
}  // namespace gert
