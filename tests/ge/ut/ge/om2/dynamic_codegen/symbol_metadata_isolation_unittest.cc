/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "engine/aicore/converter/autofuse_node_converter.h"
#include "engine/node_converter_utils.h"
#include <gtest/gtest.h>
#include "engine/gelocal/inputs_converter.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_local_context.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph_builder/value_holder_generator.h"
#include "common/share_graph.h"
#include "faker/global_data_faker.h"
#include "common/bg_test.h"
#include "runtime/fast_v2/common/const_data_helper.h"

#include "exe_graph/lowering/exe_graph_attrs.h"
#include <tuple>

namespace gert {
namespace {
using namespace ge;
using namespace bg;
std::string GetAutofuseSoPath() {
  std::string cmake_binary_path = CMAKE_BINARY_DIR;
  return cmake_binary_path + "/tests/depends/op_stub/libautofuse_stub.so";
}

ge::ComputeGraphPtr BuildAutofuseGraph() {
  auto graph = ShareGraph::AutoFuseNodeGraph();
  (void)ge::AttrUtils::SetInt(graph, "_all_symbol_num", 8);
  auto fused_graph_node = graph->FindNode("fused_graph");
  auto fused_graph_node1 = graph->FindNode("fused_graph1");

  auto autofuse_stub_so = GetAutofuseSoPath();
  (void)ge::AttrUtils::SetStr(fused_graph_node->GetOpDesc(), "bin_file_path", autofuse_stub_so);
  (void)ge::AttrUtils::SetStr(fused_graph_node1->GetOpDesc(), "bin_file_path", autofuse_stub_so);

  (void)ge::AttrUtils::SetStr(fused_graph_node->GetOpDesc(), "_symbol_infer_shape_cache_key", "xxxxx");
  (void)ge::AttrUtils::SetStr(fused_graph_node1->GetOpDesc(), "_symbol_infer_shape_cache_key", "xxxxx");

  return graph;
}

class Om2SymbolMetadataUT : public bg::BgTestAutoCreate3StageFrame,
                            public testing::WithParamInterface<std::tuple<bool, bool>> {};
TEST_P(Om2SymbolMetadataUT, SymbolMarkersPreserveNodeAndMergeContracts) {
  auto graph = BuildAutofuseGraph();
  const bool for_om2 = std::get<0>(GetParam());
  const bool merge = std::get<1>(GetParam());
  if (!merge) {
    ASSERT_TRUE(AttrUtils::SetStr(graph->FindNode("fused_graph")->GetOpDesc(), "_symbol_infer_shape_cache_key", ""));
    ASSERT_TRUE(AttrUtils::SetStr(graph->FindNode("fused_graph1")->GetOpDesc(), "_symbol_infer_shape_cache_key", ""));
  }
  auto root_model = GeModelBuilder(graph).BuildGeRootModel();
  auto global_data = GlobalDataFaker(root_model).FakeWithHandleAiCore("AscBackend", false).Build();
  global_data.SetForOm2(for_om2);
  global_data.SetExternalAllocator(nullptr, ExecuteGraphType::kInit);
  global_data.SetExternalAllocator(nullptr, ExecuteGraphType::kMain);
  bg::LowerConstDataNode(global_data);
  LowerInput data_input = {{}, {}, &global_data};
  auto fused_graph_node = graph->FindNode("fused_graph");
  auto compile_result = global_data.FindCompiledResult(fused_graph_node);
  ASSERT_NE(compile_result, nullptr);

  auto data0_ret = LoweringDataNode(graph->FindNode("data0"), data_input);
  auto data1_ret = LoweringDataNode(graph->FindNode("data1"), data_input);
  auto data2_ret = LoweringDataNode(graph->FindNode("data2"), data_input);
  auto data3_ret = LoweringDataNode(graph->FindNode("data3"), data_input);
  ASSERT_TRUE(data0_ret.result.IsSuccess());
  ASSERT_TRUE(data1_ret.result.IsSuccess());
  ASSERT_TRUE(data2_ret.result.IsSuccess());
  ASSERT_TRUE(data3_ret.result.IsSuccess());

  LowerInput add_input = {
      {data0_ret.out_shapes[0], data1_ret.out_shapes[0], data2_ret.out_shapes[0], data3_ret.out_shapes[0]},
      {data0_ret.out_addrs[0], data1_ret.out_addrs[0], data2_ret.out_addrs[0], data3_ret.out_addrs[0]},
      &global_data};

  auto autofuse_ret = LoweringAutofuseNode(fused_graph_node, add_input);
  ASSERT_TRUE(autofuse_ret.result.IsSuccess());
  ASSERT_EQ(autofuse_ret.out_addrs.size(), 1);
  ASSERT_EQ(autofuse_ret.out_shapes.size(), 1);
  ASSERT_EQ(autofuse_ret.order_holders.size(), 1);

  // cacheable tiling cache
  auto fused_graph_node1 = graph->FindNode("fused_graph1");
  autofuse_ret = LoweringAutofuseNode(fused_graph_node1, add_input);
  ASSERT_TRUE(autofuse_ret.result.IsSuccess());
  ASSERT_EQ(autofuse_ret.out_addrs.size(), 1);
  ASSERT_EQ(autofuse_ret.out_shapes.size(), 1);
  ASSERT_EQ(autofuse_ret.order_holders.size(), 1);

  auto exe_graph = autofuse_ret.out_addrs[0]->GetFastNode()->GetExtendInfo()->GetOwnerGraphBarePtr();
  ASSERT_NE(exe_graph, nullptr);
  // graph compare

  auto execute_graph = bg::ValueHolder::PopGraphFrame(ConvertDevMemValueHoldersToValueHolders(autofuse_ret.out_addrs),
                                                      autofuse_ret.order_holders)
                           ->GetExecuteGraph();
  ASSERT_NE(execute_graph, nullptr);

  size_t sym_tiling_node = 0;
  size_t cacheable_sym_tiling_node = 0;
  size_t pre_tiling_cache_node = 0;
  size_t get_tiling_cache_key_node = 0;
  size_t infer_shape_node = 0;
  size_t tiling_parse_node = 0;
  for (auto node : execute_graph->GetAllNodes()) {
    auto node_type = node->GetType();
    if (node_type == "InferShape" || node_type == "CacheableTiling") {
      bool marker = false;
      const auto key = node_type == "InferShape" ? kSymbolInferShape : kSymbolTiling;
      EXPECT_EQ(AttrUtils::GetBool(node->GetOpDescBarePtr(), key, marker), for_om2);
      EXPECT_EQ(marker, for_om2);
    }
    if (node_type == "Tiling") {
      sym_tiling_node++;
    } else if (node_type == "CacheableTiling") {
      cacheable_sym_tiling_node++;
    } else if (node_type == "PrepareCacheableTilingFwkData") {
      pre_tiling_cache_node++;
    } else if (node_type == "GetSymbolTilingCacheKey") {
      get_tiling_cache_key_node++;
    } else if (node_type == "InferShape") {
      infer_shape_node++;
    } else if (node_type == "SymbolTilingParse") {
      tiling_parse_node++;
    }
  }
  // 两个asc节点一个开起来缓存，两个节点infershapekey相同可以merge
  EXPECT_EQ(sym_tiling_node, 0);
  EXPECT_EQ(cacheable_sym_tiling_node, 2);
  EXPECT_EQ(pre_tiling_cache_node, 2);
  EXPECT_EQ(get_tiling_cache_key_node, 2);
  EXPECT_EQ(infer_shape_node, merge ? 1U : 2U);
  EXPECT_EQ(tiling_parse_node, 2);
}

INSTANTIATE_TEST_SUITE_P(ModesAndMerge, Om2SymbolMetadataUT, testing::Combine(testing::Bool(), testing::Bool()));
}  // namespace
}  // namespace gert
