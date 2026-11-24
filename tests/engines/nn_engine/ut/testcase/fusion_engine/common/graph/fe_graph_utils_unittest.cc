/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/graph/fe_graph_utils.h"
#include "graph/compute_graph.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/ref_relation.h"
#include "graph/utils/node_utils.h"

using namespace fe;
using namespace ge;

class UTEST_fe_graph_utils_check_related_edges_origin_shape : public testing::Test {
 protected:
  void SetUp() override {
    graph_ = std::make_shared<ComputeGraph>("test_graph");
  }

  void TearDown() override {
    graph_.reset();
  }

  NodePtr BuildNode(const std::string &name, const std::vector<std::vector<int64_t>> &input_shapes,
                    const std::vector<std::vector<int64_t>> &output_shapes) {
    auto op_desc = std::make_shared<OpDesc>(name, "FakeOp");
    for (const auto &shape : input_shapes) {
      GeTensorDesc desc;
      (void)desc.SetOriginShape(GeShape(shape));
      op_desc->AddInputDesc(desc);
    }
    for (const auto &shape : output_shapes) {
      GeTensorDesc desc;
      (void)desc.SetOriginShape(GeShape(shape));
      op_desc->AddOutputDesc(desc);
    }
    return graph_->AddNode(op_desc);
  }

  ComputeGraphPtr graph_;
};

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, empty_reflections_return_true) {
  std::unordered_set<RefCell, RefCellHash> reflections;
  EXPECT_TRUE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, same_origin_shape_return_true) {
  NodePtr node = BuildNode("data", {{1, 2, 3}, {1, 2, 3}}, {{1, 2, 3}, {1, 2, 3}});
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", node, ge::NODE_IN, 0));
  reflections.insert(RefCell("data", node, ge::NODE_IN, 1));
  reflections.insert(RefCell("data", node, ge::NODE_OUT, 0));
  reflections.insert(RefCell("data", node, ge::NODE_OUT, 1));
  EXPECT_TRUE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, different_input_origin_shape_return_false) {
  NodePtr node = BuildNode("data", {{1, 2, 3}, {1, 2, 4}}, {});
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", node, ge::NODE_IN, 0));
  reflections.insert(RefCell("data", node, ge::NODE_IN, 1));
  EXPECT_FALSE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, different_output_origin_shape_return_false) {
  NodePtr node = BuildNode("data", {}, {{1, 2, 3}, {1, 2, 4}});
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", node, ge::NODE_OUT, 0));
  reflections.insert(RefCell("data", node, ge::NODE_OUT, 1));
  EXPECT_FALSE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, null_node_return_false) {
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", nullptr, ge::NODE_IN, 0));
  EXPECT_FALSE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, invalid_index_return_false) {
  NodePtr node = BuildNode("data", {{1, 2, 3}}, {{4, 5}});
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", node, ge::NODE_IN, 3));
  EXPECT_FALSE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}

TEST_F(UTEST_fe_graph_utils_check_related_edges_origin_shape, node_without_owner_graph_return_false) {
  GeTensorDesc desc;
  (void)desc.SetOriginShape(GeShape({1, 2, 3}));
  auto op_desc = std::make_shared<OpDesc>("data", "FakeOp");
  op_desc->AddInputDesc(desc);
  NodePtr node = NodeUtils::CreatNodeWithoutGraph(op_desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(node->GetOwnerComputeGraph(), nullptr);
  std::unordered_set<RefCell, RefCellHash> reflections;
  reflections.insert(RefCell("data", node, ge::NODE_IN, 0));
  EXPECT_FALSE(FeGraphUtils::CheckRelatedEdgesOriginShape(reflections));
}
