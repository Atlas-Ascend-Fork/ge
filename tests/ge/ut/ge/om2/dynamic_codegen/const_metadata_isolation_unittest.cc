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
#include "ge_graph_dsl/graph_dsl.h"
#include "common/bg_test.h"
#include "common/share_graph.h"
#include "faker/global_data_faker.h"
#include "engine/gelocal/inputs_converter.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace gert {
namespace {
using namespace ge;
class Om2ConstMetadataUT : public bg::BgTestAutoCreate3StageFrame, public testing::WithParamInterface<bool> {};
ComputeGraphPtr BuildConstGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("const1", "Const")->NODE("NetOutput", "NetOutput"));
  };
  auto graph = ToComputeGraph(g1);
  auto const1 = graph->FindNode("const1");
  const1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({8, 3, 224, 224}));
  const1->GetOpDesc()->MutableOutputDesc(0)->SetOriginShape(GeShape({8, 3, 224, 224}));
  const1->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
  const1->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);
  const1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(ge::FORMAT_NCHW);
  const1->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(ge::FORMAT_NCHW);
  auto tensor = std::make_shared<GeTensor>(const1->GetOpDesc()->GetOutputDesc(0));
  std::vector<float> data(8 * 3 * 224 * 224);
  for (size_t i = 0; i < 8 * 3 * 224 * 224; ++i) {
    data[i] = static_cast<float>(i);
  }
  tensor->SetData(reinterpret_cast<uint8_t *>(data.data()), data.size() * sizeof(float));
  if (!AttrUtils::SetTensor(const1->GetOpDesc(), "value", tensor)) {
    return nullptr;
  }
  return graph;
}

TEST_P(Om2ConstMetadataUT, WritesWeightMetadataOnlyForOm2) {
  auto graph = BuildConstGraph();
  auto const_node = graph->FindNode("const1");
  auto op_desc = const_node->GetOpDesc();
  GeTensorPtr weight;
  ASSERT_TRUE(AttrUtils::MutableTensor(op_desc, "value", weight));
  ASSERT_NE(weight, nullptr);
  ASSERT_EQ(weight->MutableTensorDesc().SetAttr("om2_weight_index", GeAttrValue::CreateFrom<int64_t>(2)),
            GRAPH_SUCCESS);
  ASSERT_EQ(weight->MutableTensorDesc().SetAttr("om2_weight_data_offset", GeAttrValue::CreateFrom<int64_t>(16)),
            GRAPH_SUCCESS);
  ASSERT_EQ(weight->MutableTensorDesc().SetAttr(ATTR_NAME_GRAPH_FLATTEN_OFFSET,
                                                GeAttrValue::CreateFrom<std::vector<int64_t>>({116, 8})),
            GRAPH_SUCCESS);
  auto root_model = GeModelBuilder(graph).BuildGeRootModel();
  LoweringGlobalData lgd = GlobalDataFaker(root_model).Build();
  lgd.SetForOm2(GetParam());
  auto ret = LoweringConstNode(const_node, {{}, {}, &lgd});
  ASSERT_TRUE(ret.result.IsSuccess());
  auto init_graph = FastNodeUtils::GetSubgraphFromNode(ret.out_addrs[0]->GetFastNode(), 0);
  ASSERT_NE(init_graph, nullptr);
  auto split = ExecuteGraphUtils::FindFirstNodeMatchType(init_graph, "SplitConstTensor");
  ASSERT_NE(split, nullptr);
  auto tensor_const = split->GetInDataNodes().at(0);
  ASSERT_NE(tensor_const, nullptr);
  const auto &desc = tensor_const->GetOpDescBarePtr()->GetOutputDesc(0);
  int64_t index = -1;
  int64_t offset = -1;
  if (!GetParam()) {
    EXPECT_FALSE(AttrUtils::GetInt(desc, "om2_weight_index", index));
    EXPECT_FALSE(desc.HasAttr(ATTR_NAME_GRAPH_FLATTEN_OFFSET));
    EXPECT_EQ(ret.out_addrs.size(), 1U);
    EXPECT_EQ(ret.out_shapes.size(), 1U);
    return;
  }
  ASSERT_TRUE(AttrUtils::GetInt(desc, "om2_weight_index", index));
  ASSERT_EQ(TensorUtils::GetDataOffset(desc, offset), GRAPH_SUCCESS);
  EXPECT_EQ(index, 2);
  EXPECT_EQ(offset, 16);
}

INSTANTIATE_TEST_SUITE_P(Rt2AndOm2, Om2ConstMetadataUT, testing::Bool());
}  // namespace
}  // namespace gert
