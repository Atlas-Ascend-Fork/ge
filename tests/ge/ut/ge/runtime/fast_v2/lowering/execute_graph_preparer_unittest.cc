/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "lowering/execute_graph_preparer.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/lowering/value_holder.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
constexpr const char *kMainGraphName = "Main";

// ExecuteGraph::AddNode(op) 会用图内序号覆盖节点 id，必须用带 id 的重载才能固定 topo 序。
ge::FastNode *AddKernelNode(ge::ExecuteGraph &graph, const std::string &name, const int64_t id) {
  auto desc = std::make_shared<ge::OpDesc>(name, "LaunchKernelWithFlag");
  desc->SetId(id);
  return graph.AddNode(desc, id);
}
}  // namespace

class ExecuteGraphPreparerUT : public testing::Test {};

// 8.13：逻辑进度初始为空，未完成的步骤按顺序推进。
TEST_F(ExecuteGraphPreparerUT, StartsFromLogicalState) {
  ExecuteGraphPrepareResult result;
  EXPECT_FALSE(result.IsDone(PrepareStep::kOptimized));
  EXPECT_FALSE(result.IsDone(PrepareStep::kSorted));
  EXPECT_FALSE(result.IsDone(PrepareStep::kPrioritized));
}

// 8.13：进度只能按步骤推进，回退会把该步骤及其后续步骤一起作废。
TEST_F(ExecuteGraphPreparerUT, InvalidatesFromStep) {
  ExecuteGraphPrepareResult result;
  result.MarkDone(PrepareStep::kOptimized);
  result.MarkDone(PrepareStep::kSorted);
  result.MarkDone(PrepareStep::kPrioritized);

  result.InvalidateFrom(PrepareStep::kSorted);
  EXPECT_TRUE(result.IsDone(PrepareStep::kOptimized));
  EXPECT_FALSE(result.IsDone(PrepareStep::kSorted));
  EXPECT_FALSE(result.IsDone(PrepareStep::kPrioritized));

  result.InvalidateFrom(PrepareStep::kOptimized);
  EXPECT_FALSE(result.IsDone(PrepareStep::kOptimized));
}

// 8.13：计算节点任一步骤失效都必须整体重算，不允许只失效后续步骤。
TEST_F(ExecuteGraphPreparerUT, InvalidateAllClearsEverything) {
  ExecuteGraphPrepareResult result;
  result.MarkDone(PrepareStep::kOptimized);
  result.MarkDone(PrepareStep::kPrioritized);
  result.InvalidateFrom(PrepareStep::kOptimized);
  for (size_t i = 0U; i < static_cast<size_t>(PrepareStep::kStepEnd); ++i) {
    EXPECT_FALSE(result.IsDone(static_cast<PrepareStep>(i)));
  }
}

// 8.13：kComputeNodeIndex 映射按节点属性收集，一个计算节点可对应多个执行节点。
TEST_F(ExecuteGraphPreparerUT, CollectsIndexesToComputeNodeId) {
  ge::ExecuteGraph main_graph(kMainGraphName);
  auto *first = AddKernelNode(main_graph, "launch0", 3);
  auto *second = AddKernelNode(main_graph, "launch1", 4);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetInt(first->GetOpDescPtr(), kComputeNodeIndex, 0));
  ASSERT_TRUE(ge::AttrUtils::SetInt(second->GetOpDescPtr(), kComputeNodeIndex, 1));

  std::vector<int64_t> indexes;
  ASSERT_EQ(ge::GRAPH_SUCCESS, CollectIndexesToComputeNodeId(main_graph.GetAllNodes(), 2U, indexes));
  ASSERT_EQ(indexes.size(), 2U);
  EXPECT_EQ(indexes[0], 3);
  EXPECT_EQ(indexes[1], 4);
}

// 8.13：同一计算节点下标对应冲突的 topo 序属于图结构错误。
TEST_F(ExecuteGraphPreparerUT, RejectsConflictingComputeNodeIndex) {
  ge::ExecuteGraph main_graph(kMainGraphName);
  auto *first = AddKernelNode(main_graph, "launch0", 3);
  auto *second = AddKernelNode(main_graph, "launch1", 4);
  ASSERT_TRUE(ge::AttrUtils::SetInt(first->GetOpDescPtr(), kComputeNodeIndex, 0));
  ASSERT_TRUE(ge::AttrUtils::SetInt(second->GetOpDescPtr(), kComputeNodeIndex, 0));

  std::vector<int64_t> indexes;
  EXPECT_NE(ge::GRAPH_SUCCESS, CollectIndexesToComputeNodeId(main_graph.GetAllNodes(), 2U, indexes));
}

// 8.13：越界的计算节点下标必须拒绝，不能截断后继续计算优先级。
TEST_F(ExecuteGraphPreparerUT, RejectsOutOfRangeComputeNodeIndex) {
  ge::ExecuteGraph main_graph(kMainGraphName);
  auto *node = AddKernelNode(main_graph, "launch0", 3);
  ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescPtr(), kComputeNodeIndex, 7));

  std::vector<int64_t> indexes;
  EXPECT_NE(ge::GRAPH_SUCCESS, CollectIndexesToComputeNodeId(main_graph.GetAllNodes(), 2U, indexes));
}

// 8.13：未声明计算节点下标的节点被跳过，不阻断其余节点收集。
TEST_F(ExecuteGraphPreparerUT, SkipsNodeWithoutComputeNodeIndex) {
  ge::ExecuteGraph main_graph(kMainGraphName);
  auto *indexed = AddKernelNode(main_graph, "launch0", 3);
  (void)AddKernelNode(main_graph, "noop_free", 4);
  ASSERT_TRUE(ge::AttrUtils::SetInt(indexed->GetOpDescPtr(), kComputeNodeIndex, 0));

  std::vector<int64_t> indexes;
  ASSERT_EQ(ge::GRAPH_SUCCESS, CollectIndexesToComputeNodeId(main_graph.GetAllNodes(), 1U, indexes));
  ASSERT_EQ(indexes.size(), 1U);
  EXPECT_EQ(indexes[0], 3);
}
}  // namespace gert
