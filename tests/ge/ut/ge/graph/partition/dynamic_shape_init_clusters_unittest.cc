/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/*
 * GE-G18-009 回归用例：InitClusters 不能依赖“拓扑序中父节点一定先于子节点”。
 *
 * GetInEdgeSize 对来自 NextIteration/RefNextIteration 的数据回边会额外减 1，
 * 消费节点可能排在 NextIteration 之前。旧实现边遍历节点边连边，会在父 cluster
 * 尚未分配时取到空指针，在 AddInput 中解引用崩溃。
 *
 * 用例构造 data -> enter -> merge -> add -> next -> merge 的循环图，直接调用
 * InitClusters，断言所有节点都分配到 cluster 且输入依赖关系完整。
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "framework/common/framework_types_internal.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/graph_utils.h"

#define private public
#define protected public
#include "graph/partition/dynamic_shape_partition.h"
#undef protected
#undef private

namespace ge {
namespace {
class UtestDynamicShapeInitClusters : public testing::Test {
 protected:
  void SetUp() override {
    graph_ = std::make_shared<ComputeGraph>("g18_009_next_iteration_loop");
    data_ = AddNode("data", DATA, 0U, 1U);
    enter_ = AddNode("enter", ENTER, 1U, 1U);
    merge_ = AddNode("merge", MERGE, 2U, 1U);
    add_ = AddNode("add", ADD, 2U, 1U);
    next_ = AddNode("next", NEXTITERATION, 1U, 1U);
    output_ = AddNode("output", NETOUTPUT, 1U, 0U);
    ASSERT_NE(data_, nullptr);
    ASSERT_NE(enter_, nullptr);
    ASSERT_NE(merge_, nullptr);
    ASSERT_NE(add_, nullptr);
    ASSERT_NE(next_, nullptr);
    ASSERT_NE(output_, nullptr);

    ASSERT_EQ(GraphUtils::AddEdge(data_->GetOutDataAnchor(0), enter_->GetInDataAnchor(0)), GRAPH_SUCCESS);
    ASSERT_EQ(GraphUtils::AddEdge(enter_->GetOutDataAnchor(0), merge_->GetInDataAnchor(0)), GRAPH_SUCCESS);
    // 数据回边：GetInEdgeSize 会把它从 merge 的入度里减掉，merge 可能排到 next 之前
    ASSERT_EQ(GraphUtils::AddEdge(next_->GetOutDataAnchor(0), merge_->GetInDataAnchor(1)), GRAPH_SUCCESS);
    ASSERT_EQ(GraphUtils::AddEdge(merge_->GetOutDataAnchor(0), add_->GetInDataAnchor(0)), GRAPH_SUCCESS);
    ASSERT_EQ(GraphUtils::AddEdge(add_->GetOutDataAnchor(0), next_->GetInDataAnchor(0)), GRAPH_SUCCESS);
    ASSERT_EQ(GraphUtils::AddEdge(add_->GetOutDataAnchor(0), output_->GetInDataAnchor(0)), GRAPH_SUCCESS);
  }

  NodePtr AddNode(const std::string &name, const std::string &type, uint32_t input_num, uint32_t output_num) {
    auto op_desc = std::make_shared<OpDesc>(name, type);
    const GeTensorDesc desc(GeShape({-1}), FORMAT_ND, DT_FLOAT);
    for (uint32_t i = 0U; i < input_num; ++i) {
      op_desc->AddInputDesc(desc);
    }
    for (uint32_t i = 0U; i < output_num; ++i) {
      op_desc->AddOutputDesc(desc);
    }
    return graph_->AddNode(op_desc);
  }

  ComputeGraphPtr graph_;
  NodePtr data_;
  NodePtr enter_;
  NodePtr merge_;
  NodePtr add_;
  NodePtr next_;
  NodePtr output_;
};
}  // namespace

TEST_F(UtestDynamicShapeInitClusters, init_clusters_with_next_iteration_back_edge) {
  DynamicShapePartitioner partitioner(graph_);
  ASSERT_EQ(partitioner.InitClusters(), SUCCESS);
  for (const auto &node : graph_->GetDirectNode()) {
    EXPECT_NE(partitioner.GetCluster(node), nullptr) << "node " << node->GetName() << " has no cluster";
  }
}

TEST_F(UtestDynamicShapeInitClusters, init_clusters_connects_all_parents) {
  DynamicShapePartitioner partitioner(graph_);
  ASSERT_EQ(partitioner.InitClusters(), SUCCESS);
  const auto &merge_cluster = partitioner.GetCluster(merge_);
  ASSERT_NE(merge_cluster, nullptr);
  const auto inputs = merge_cluster->Inputs();
  // merge 的两个父节点 enter、next 都必须连上，不能因为 next 排在后面而丢失
  EXPECT_EQ(inputs.size(), 2U);
  EXPECT_NE(std::find(inputs.begin(), inputs.end(), partitioner.GetCluster(enter_).get()), inputs.end());
  EXPECT_NE(std::find(inputs.begin(), inputs.end(), partitioner.GetCluster(next_).get()), inputs.end());
}
}  // namespace ge
