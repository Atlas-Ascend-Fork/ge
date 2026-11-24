/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <memory>

#include "framework/common/framework_types_internal.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/partition/dynamic_shape_partition.h"

namespace ge {
namespace {
class InitFailurePartitioner : public DynamicShapePartitioner {
 public:
  using BasePartitioner::GetRootGraph;
  using DynamicShapePartitioner::DynamicShapePartitioner;

 private:
  Status InitClusters() override {
    // Model an early return with only the first node's cluster initialized.
    const auto data = GetRootGraph()->FindNode("data");
    const auto cluster = std::make_shared<DynamicShapeCluster>(0U, 0, data, this);
    RecordClusters(cluster);
    SetCluster(data, cluster);
    return FAILED;
  }
};

class UtestDynamicShapeInitClusters : public testing::Test {
 protected:
  void SetUp() override {
    const auto data = OP_CFG(DATA).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    const auto relu =
        OP_CFG(RELU).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, {-1}).Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
    const auto output = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(0).TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    DEF_GRAPH(graph) {
      CHAIN(NODE("data", data)->NODE("relu", relu)->NODE("output", output));
    };
    graph_ = ToComputeGraph(graph);
    ASSERT_NE(graph_, nullptr);
  }

  ComputeGraphPtr graph_;
};

TEST_F(UtestDynamicShapeInitClusters, LinksPreviouslyInitializedParents) {
  DynamicShapePartitioner partitioner(graph_);
  ASSERT_EQ(partitioner.InitClusters(), SUCCESS);
  const auto &data = partitioner.GetCluster(graph_->FindNode("data"));
  const auto &relu = partitioner.GetCluster(graph_->FindNode("relu"));
  const auto &output = partitioner.GetCluster(graph_->FindNode("output"));
  ASSERT_NE(data, nullptr);
  ASSERT_NE(relu, nullptr);
  ASSERT_NE(output, nullptr);
  EXPECT_TRUE(data->Inputs().empty());
  ASSERT_EQ(relu->Inputs().size(), 1U);
  EXPECT_EQ(relu->Inputs().front(), data.get());
  ASSERT_EQ(output->Inputs().size(), 1U);
  EXPECT_EQ(output->Inputs().front(), relu.get());
}

TEST_F(UtestDynamicShapeInitClusters, PartitionReturnsInitializationFailureAndClearsResources) {
  InitFailurePartitioner partitioner(graph_);
  EXPECT_EQ(partitioner.Partition(), FAILED);
  EXPECT_EQ(partitioner.GetRootGraph(), nullptr);
}
}  // namespace
}  // namespace ge
