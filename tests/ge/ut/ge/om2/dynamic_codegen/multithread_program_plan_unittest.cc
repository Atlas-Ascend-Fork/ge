/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "common/om2/dynamic_codegen/execute_graph_dependencies.h"
#include "common/om2/dynamic_codegen/multithread_program_plan.h"
#include "core/executor/multi_thread_topological/execution_data/free_launch_relation.h"

namespace gert {
namespace om2 {
namespace {
ProgramStep KernelStep(const uint32_t index) {
  ProgramStep step;
  step.kernel_index = index;
  return step;
}

ProgramBlock LinearProgram(const uint32_t kernel_num) {
  ProgramBlock program;
  program.kernels.resize(kernel_num);
  program.regions.resize(1U);
  program.root_region_index = 0U;
  for (uint32_t i = 0U; i < kernel_num; ++i) {
    program.regions[0].steps.push_back(KernelStep(i));
  }
  return program;
}

ProgramBlock IfIslandProgram() {
  auto program = LinearProgram(5U);
  program.plan_type = ProgramPlanType::kStructuredTopological;
  program.regions.resize(4U);
  ProgramStep island;
  island.kind = ProgramStepKind::kIf;
  island.condition_region_index = 1U;
  IfProgramInfo::Branch first;
  first.region_index = 2U;
  IfProgramInfo::Branch second;
  second.region_index = 3U;
  island.if_info.branches = {first, second};
  program.regions[0].steps = {KernelStep(0U), island, KernelStep(4U)};
  program.regions[1].steps = {KernelStep(1U)};
  program.regions[2].steps = {KernelStep(2U)};
  program.regions[3].steps = {KernelStep(3U)};
  return program;
}

void ExpectSingleWait(const PlannedExecutionUnit &unit, const size_t worker_index, const uint32_t progress) {
  ASSERT_EQ(unit.waits.size(), 1U);
  EXPECT_EQ(unit.waits[0].worker_index, worker_index);
  EXPECT_EQ(unit.waits[0].progress, progress);
}

ge::FastNode *AddDependencyNode(ge::ExecuteGraph &graph, const char *name) {
  auto desc = std::make_shared<ge::OpDesc>(name, "PlainKernel");
  if ((desc->AddInputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS) ||
      (desc->AddOutputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS)) {
    return nullptr;
  }
  return graph.AddNode(desc);
}
}  // namespace

TEST(MultithreadProgramPlanTest, SeparatesMemoryLaunchAndRoundRobinNormalWorkers) {
  auto program = LinearProgram(6U);
  const std::vector<ExecTaskType> types = {ExecTaskType::MEMORY, ExecTaskType::LAUNCH, ExecTaskType::NORMAL,
                                           ExecTaskType::NORMAL, ExecTaskType::NORMAL, ExecTaskType::MEMORY};
  MultiThreadProgramPlan plan;
  ASSERT_TRUE(BuildMultiThreadProgramPlan(program, types, 4U, plan));
  ASSERT_EQ(plan.workers.size(), 4U);
  const std::vector<ExecTaskType> expected_types = {ExecTaskType::MEMORY, ExecTaskType::LAUNCH, ExecTaskType::NORMAL,
                                                    ExecTaskType::NORMAL};
  const std::vector<std::vector<uint32_t>> expected_kernels = {{0U, 5U}, {1U}, {2U, 4U}, {3U}};
  for (size_t i = 0U; i < plan.workers.size(); ++i) {
    EXPECT_EQ(plan.workers[i].type, expected_types[i]);
    ASSERT_EQ(plan.workers[i].units.size(), expected_kernels[i].size());
    for (size_t j = 0U; j < expected_kernels[i].size(); ++j) {
      const auto &unit = plan.workers[i].units[j];
      EXPECT_EQ(unit.kind, ExecutionUnitKind::kKernel);
      EXPECT_EQ(unit.kernel_index, expected_kernels[i][j]);
      EXPECT_EQ(unit.root_step_index, expected_kernels[i][j]);
      EXPECT_TRUE(unit.waits.empty());
    }
  }
}

TEST(MultithreadProgramPlanTest, UsesNormalWorkerForLaunchWithTwoThreads) {
  const auto program = LinearProgram(3U);
  MultiThreadProgramPlan plan;
  ASSERT_TRUE(BuildMultiThreadProgramPlan(program, {ExecTaskType::MEMORY, ExecTaskType::LAUNCH, ExecTaskType::NORMAL},
                                          2U, plan));
  ASSERT_EQ(plan.workers.size(), 2U);
  EXPECT_EQ(plan.workers[0].type, ExecTaskType::MEMORY);
  EXPECT_EQ(plan.workers[1].type, ExecTaskType::NORMAL);
  ASSERT_EQ(plan.workers[0].units.size(), 1U);
  ASSERT_EQ(plan.workers[1].units.size(), 2U);
  EXPECT_EQ(plan.workers[0].units[0].kernel_index, 0U);
  EXPECT_EQ(plan.workers[1].units[0].kernel_index, 1U);
  EXPECT_EQ(plan.workers[1].units[1].kernel_index, 2U);
}

TEST(MultithreadProgramPlanTest, CoalescesCrossWorkerWaitsAtLatestPredecessorProgress) {
  auto program = LinearProgram(4U);
  program.kernel_dependencies = {{0U, 1U}, {0U, 2U}, {1U, 2U}, {1U, 2U}, {2U, 3U}};
  MultiThreadProgramPlan plan;
  ASSERT_TRUE(BuildMultiThreadProgramPlan(
      program, {ExecTaskType::MEMORY, ExecTaskType::MEMORY, ExecTaskType::LAUNCH, ExecTaskType::NORMAL}, 3U, plan));
  ASSERT_EQ(plan.workers.size(), 3U);
  ASSERT_EQ(plan.workers[0].units.size(), 2U);
  ASSERT_EQ(plan.workers[1].units.size(), 1U);
  ASSERT_EQ(plan.workers[2].units.size(), 1U);
  EXPECT_TRUE(plan.workers[0].units[0].waits.empty());
  EXPECT_TRUE(plan.workers[0].units[1].waits.empty());
  ExpectSingleWait(plan.workers[1].units[0], 0U, 2U);
  ExpectSingleWait(plan.workers[2].units[0], 1U, 1U);
}

TEST(MultithreadProgramPlanTest, KeepsIfIslandAtomicWithCrossWorkerEntryAndExitBarriers) {
  for (const auto type : {ProgramPlanType::kStructuredTopological, ProgramPlanType::kStructuredTopologicalPriority}) {
    auto program = IfIslandProgram();
    program.plan_type = type;
    program.kernel_dependencies = {{0U, 1U}, {1U, 2U}, {2U, 4U}, {3U, 4U}};
    MultiThreadProgramPlan plan;
    ASSERT_TRUE(BuildMultiThreadProgramPlan(
        program,
        {ExecTaskType::MEMORY, ExecTaskType::NORMAL, ExecTaskType::LAUNCH, ExecTaskType::MEMORY, ExecTaskType::LAUNCH},
        3U, plan));
    ASSERT_EQ(plan.workers.size(), 3U);
    ASSERT_EQ(plan.workers[0].units.size(), 1U);
    ASSERT_EQ(plan.workers[1].units.size(), 1U);
    ASSERT_EQ(plan.workers[2].units.size(), 1U);
    const auto &island = plan.workers[2].units[0];
    EXPECT_EQ(island.kind, ExecutionUnitKind::kStructuredIsland);
    EXPECT_EQ(island.kernel_index, kInvalidProgramIndex);
    EXPECT_EQ(island.root_step_index, 1U);
    ExpectSingleWait(island, 0U, 1U);
    EXPECT_EQ(plan.workers[1].units[0].kernel_index, 4U);
    ExpectSingleWait(plan.workers[1].units[0], 2U, 1U);
  }
}

TEST(MultithreadProgramPlanTest, RejectsInvalidPlanInputsAndClearsPreviousPlan) {
  const auto valid = LinearProgram(1U);
  const auto expect_rejected = [&valid](const ProgramBlock &program, const std::vector<ExecTaskType> &types,
                                        const size_t thread_num) {
    MultiThreadProgramPlan plan;
    ASSERT_TRUE(BuildMultiThreadProgramPlan(valid, {ExecTaskType::NORMAL}, 3U, plan));
    EXPECT_FALSE(BuildMultiThreadProgramPlan(program, types, thread_num, plan));
    EXPECT_TRUE(plan.workers.empty());
  };
  expect_rejected(valid, {ExecTaskType::NORMAL}, 1U);
  expect_rejected(valid, {}, 3U);
  expect_rejected(valid, {ExecTaskType::MAX}, 3U);
  auto invalid = valid;
  invalid.root_region_index = kInvalidProgramIndex;
  expect_rejected(invalid, {ExecTaskType::NORMAL}, 3U);
  invalid = valid;
  invalid.plan_type = static_cast<ProgramPlanType>(255U);
  expect_rejected(invalid, {ExecTaskType::NORMAL}, 3U);
  invalid = valid;
  invalid.regions[0].steps[0].kernel_index = 1U;
  expect_rejected(invalid, {ExecTaskType::NORMAL}, 3U);
  invalid = valid;
  invalid.regions[0].steps.push_back(KernelStep(0U));
  expect_rejected(invalid, {ExecTaskType::NORMAL}, 3U);
  invalid = valid;
  invalid.regions[0].steps.clear();
  expect_rejected(invalid, {ExecTaskType::NORMAL}, 3U);
}

TEST(MultithreadProgramPlanTest, RejectsReverseAndMissingKernelDependencies) {
  for (const auto dependency : {KernelDependency{1U, 0U}, KernelDependency{0U, 2U}}) {
    auto program = LinearProgram(2U);
    program.kernel_dependencies = {dependency};
    MultiThreadProgramPlan plan;
    EXPECT_FALSE(BuildMultiThreadProgramPlan(program, {ExecTaskType::MEMORY, ExecTaskType::LAUNCH}, 3U, plan));
    EXPECT_TRUE(plan.workers.empty());
  }
}

TEST(MultithreadProgramPlanTest, RejectsUnsupportedAndRecursiveStructuredRegions) {
  for (uint32_t scenario = 0U; scenario < 5U; ++scenario) {
    SCOPED_TRACE(scenario);
    auto program = IfIslandProgram();
    if (scenario == 0U) {
      program.plan_type = ProgramPlanType::kSequential;
    } else if (scenario == 1U) {
      program.regions[0].steps[1].kind = ProgramStepKind::kWhile;
    } else if (scenario == 2U) {
      program.regions[0].steps[1].if_info.branches[0].region_index = 4U;
    } else if (scenario == 3U) {
      program.regions[2].steps = {program.regions[0].steps[1]};
    } else {
      program.regions[3].steps = {KernelStep(2U)};
    }
    MultiThreadProgramPlan plan;
    EXPECT_FALSE(BuildMultiThreadProgramPlan(program, std::vector<ExecTaskType>(5U, ExecTaskType::NORMAL), 3U, plan));
    EXPECT_TRUE(plan.workers.empty());
  }
}

TEST(LaunchFreeDependenciesTest, RestoresDuplicateRelationsOnceAndIsIdempotent) {
  ge::ExecuteGraph graph("restore_dependencies");
  auto *free = AddDependencyNode(graph, "free");
  auto *launch = AddDependencyNode(graph, "launch");
  ASSERT_NE(free, nullptr);
  ASSERT_NE(launch, nullptr);
  ASSERT_TRUE(graph.SetExtAttr(kFreeLaunchRelationsAttr, FreeLaunchRelations{{free, launch}, {free, launch}}));
  ASSERT_EQ(RestoreLaunchFreeDependencies(graph), ge::GRAPH_SUCCESS);
  ASSERT_EQ(free->GetAllInControlEdgesRef().size(), 1U);
  auto *edge = free->GetAllInControlEdgesRef()[0];
  EXPECT_EQ(edge->src, launch);
  EXPECT_EQ(edge->dst, free);
  ASSERT_EQ(RestoreLaunchFreeDependencies(graph), ge::GRAPH_SUCCESS);
  ASSERT_EQ(free->GetAllInControlEdgesRef().size(), 1U);
  EXPECT_EQ(free->GetAllInControlEdgesRef()[0], edge);
  const auto nodes = graph.GetDirectNode();
  ASSERT_EQ(nodes.size(), 2U);
  EXPECT_EQ(nodes[0], launch);
  EXPECT_EQ(nodes[1], free);
}

TEST(LaunchFreeDependenciesTest, PreservesExistingControlOrDataDependency) {
  for (const int32_t port : {ge::kControlEdgeIndex, 0}) {
    ge::ExecuteGraph graph("existing_dependency");
    auto *launch = AddDependencyNode(graph, "launch");
    auto *free = AddDependencyNode(graph, "free");
    auto *edge = graph.AddEdge(launch, port, free, port);
    ASSERT_NE(edge, nullptr);
    ASSERT_TRUE(graph.SetExtAttr(kFreeLaunchRelationsAttr, FreeLaunchRelations{{free, launch}}));
    ASSERT_EQ(RestoreLaunchFreeDependencies(graph), ge::GRAPH_SUCCESS);
    if (port == ge::kControlEdgeIndex) {
      ASSERT_EQ(free->GetAllInControlEdgesRef().size(), 1U);
      EXPECT_EQ(free->GetAllInControlEdgesRef()[0], edge);
    } else {
      EXPECT_TRUE(free->GetAllInControlEdgesRef().empty());
      EXPECT_EQ(free->GetInDataEdgeByIndex(0), edge);
    }
  }
}

TEST(LaunchFreeDependenciesTest, RejectsForeignOwnersAndInvalidRelationsWithoutAddingEdges) {
  for (uint32_t scenario = 0U; scenario < 5U; ++scenario) {
    SCOPED_TRACE(scenario);
    ge::ExecuteGraph graph("invalid_dependency");
    ge::ExecuteGraph other_graph("other_owner");
    auto *free = AddDependencyNode(graph, "free");
    auto *launch = AddDependencyNode(graph, "launch");
    auto *foreign = AddDependencyNode(other_graph, "foreign");
    const std::vector<FreeLaunchRelation> invalid = {
        {free, foreign}, {foreign, launch}, {nullptr, launch}, {free, nullptr}, {free, free}};
    ASSERT_TRUE(graph.SetExtAttr(kFreeLaunchRelationsAttr, FreeLaunchRelations{invalid[scenario]}));
    EXPECT_NE(RestoreLaunchFreeDependencies(graph), ge::GRAPH_SUCCESS);
    EXPECT_TRUE(free->GetAllInControlEdgesRef().empty());
    EXPECT_TRUE(launch->GetAllOutControlEdgesRef().empty());
    EXPECT_TRUE(foreign->GetAllInControlEdgesRef().empty());
    EXPECT_EQ(graph.GetDirectNodesSize(), 2U);
    EXPECT_EQ(other_graph.GetDirectNodesSize(), 1U);
  }
}
}  // namespace om2
}  // namespace gert
