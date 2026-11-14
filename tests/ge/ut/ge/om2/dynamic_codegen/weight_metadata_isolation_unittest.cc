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
#include "securec.h"
#include <array>
#include "common/bg_test.h"
#include "common/share_graph.h"
#include "faker/ge_model_builder.h"
#include "lowering/model_converter.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "exe_graph/lowering/exe_graph_attrs.h"

namespace gert {
namespace {
using namespace ge;
class Om2WeightMetadataUT : public bg::BgTest {
 protected:
  void BuildRoot(GeRootModelPtr &root) {
    auto graph = ShareGraph::BuildReshapeGraph();
    ASSERT_NE(graph, nullptr);
    auto reshape = graph->FindNode("reshape1")->GetOpDesc();
    OpDescUtilsEx::SetType(reshape, "Reshape");
    reshape->MutableAllInputName() = {{"x", 0U}, {"shape", 1U}};
    ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
    root = GeModelBuilder(graph).BuildGeRootModel();
    auto model = root->GetSubgraphInstanceNameToModel().at(graph->GetName());
    GeTensorPtr weight;
    ASSERT_TRUE(AttrUtils::MutableTensor(graph->FindNode("const1")->GetOpDesc(), "value", weight));
    const std::array<int64_t, 2> values{3, -1};
    std::vector<uint8_t> storage(16U + sizeof(values), 0x5a);
    ASSERT_EQ(memcpy_s(storage.data() + 16U, storage.size() - 16U, values.data(), sizeof(values)), EOK);
    model->SetWeight(Buffer::CopyFrom(storage.data(), storage.size()));
    TensorUtils::SetDataOffset(weight->MutableTensorDesc(), 16);
    TensorUtils::SetWeightSize(weight->MutableTensorDesc(), sizeof(values));
    TensorUtils::SetSize(*graph->FindNode("const1")->GetOpDesc()->MutableOutputDesc(0), sizeof(values));
    // A preceding nonempty block plus an empty model distinguish local offset from flattened offset.
    for (const auto &entry : std::vector<std::pair<std::string, size_t>>{{"a_weight", 64U}, {"b_empty", 0U}}) {
      ASSERT_LT(entry.first, graph->GetName());
      auto subgraph = std::make_shared<ComputeGraph>(entry.first);
      subgraph->SetParentGraph(graph);
      subgraph->SetParentNode(graph->FindNode("reshape1"));
      ASSERT_EQ(graph->AddSubgraph(subgraph), GRAPH_SUCCESS);
      auto extra = std::make_shared<GeModel>();
      extra->SetName(entry.first);
      extra->SetGraph(subgraph);
      if (entry.second != 0U) {
        std::vector<uint8_t> bytes(entry.second, 0x37);
        extra->SetWeight(Buffer::CopyFrom(bytes.data(), bytes.size()));
      }
      root->SetSubgraphInstanceNameToModel(entry.first, extra);
    }
  }
  void CheckExecutionMetadata(const ExecuteGraphPtr &graph, bool for_om2) {
    ASSERT_NE(graph, nullptr);
    size_t matches = 0U;
    for (auto *node : graph->GetAllNodes()) {
      if (node->GetType() != "Const" || node->GetOpDescBarePtr()->GetOutputsSize() == 0U) {
        continue;
      }
      int64_t index = -1;
      const auto &desc = node->GetOpDescBarePtr()->GetOutputDesc(0);
      if (AttrUtils::GetInt(desc, "om2_weight_index", index)) {
        ++matches;
        EXPECT_EQ(index, 2);
        int64_t offset = -1;
        ASSERT_EQ(TensorUtils::GetDataOffset(desc, offset), GRAPH_SUCCESS);
        EXPECT_EQ(offset, 16);
      }
    }
    EXPECT_EQ(matches, for_om2 ? 1U : 0U);
  }
};

TEST_F(Om2WeightMetadataUT, OrdinaryRt2DoesNotWriteOm2Attributes) {
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildRoot(root));
  ASSERT_NO_FATAL_FAILURE(CheckExecutionMetadata(ModelConverter().ConvertGeModelToExecuteGraph(root), false));
  for (const auto &entry : root->GetSubgraphInstanceNameToModel()) {
    int64_t index = -1;
    EXPECT_FALSE(AttrUtils::GetInt(entry.second, "om2_weight_index", index));
  }
}

TEST_F(Om2WeightMetadataUT, Rt2ThenOm2CompletesCachedGraphMetadataWithLocalOffsets) {
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildRoot(root));
  ASSERT_NE(ModelConverter().ConvertGeModelToExecuteGraph(root), nullptr);
  const auto cached = root->GetFlattenGraph();
  ASSERT_NE(cached, nullptr);
  ModelConverter::Args args;
  args.for_om2 = true;
  ASSERT_NO_FATAL_FAILURE(CheckExecutionMetadata(ModelConverter().ConvertGeModelToExecuteGraph(root, args), true));
  EXPECT_EQ(root->GetFlattenGraph(), cached);
  int64_t index = -1;
  ASSERT_TRUE(AttrUtils::GetInt(root->GetSubgraphInstanceNameToModel().at("b_empty"), "om2_weight_index", index));
  EXPECT_EQ(index, 1);
}

TEST_F(Om2WeightMetadataUT, Om2ThenRt2DoesNotCopyStaleMetadataToExecutionConstants) {
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildRoot(root));
  ModelConverter::Args args;
  args.for_om2 = true;
  ASSERT_NO_FATAL_FAILURE(CheckExecutionMetadata(ModelConverter().ConvertGeModelToExecuteGraph(root, args), true));
  ASSERT_NO_FATAL_FAILURE(CheckExecutionMetadata(ModelConverter().ConvertGeModelToExecuteGraph(root), false));
}
}  // namespace
}  // namespace gert
