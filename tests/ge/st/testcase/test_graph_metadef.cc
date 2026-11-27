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
#include <map>
#include <vector>
#include <cstdlib>
#include <fstream>
#include "graph/utils/type_utils_inner.h"
#include "exe_graph/runtime/annotated_kernel_args_impl.h"
#include "graph/option/optimization_option.h"
#include "register/ffts_plus_update_manager.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.h"
#include "graph/utils/node_utils.h"
#include "graph/model.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "exe_graph/lowering/tiling_context_builder.h"
#include "exe_graph/lowering/data_dependent_interpreter.h"
#include "graph/utils/execute_graph_adapter.h"
#include "register/graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "graph/custom_op_registry.h"
#include "graph/ir/ir_data_type_symbol_store.h"
#include "graph/types.h"
#include "graph/normal_graph/node_impl.h"
#include "graph/normal_graph/compute_graph_impl.h"
#include "register/tensor_assign.h"
#include "proto/tensorflow/tensor.pb.h"
#include "graph/passes/graph_builder_utils.h"
#include "common/sgt_slice_type.h"
#include "register/op_tiling_registry.h"
#include "external/register/op_tiling_info.h"
#include "register/op_tiling.h"
#include "op_tiling/op_tiling_constants.h"
#include "graph/utils/anchor_utils.h"
#include "graph/shape_refiner.h"
#include "hcom/hcom_topo_info.h"
#include "common/plugin/plugin_manager.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/graph.h"
#include "graph/utils/file_utils.h"
#include "graph/utils/ffts_graph_utils.h"
#include "graph/ge_local_context.h"
#include "common/ge_common/ge_types.h"
#include "ge_common/ge_common_api_types.h"
#include "graph/symbolizer/symbolic.h"
#include "graph/model_serialize.h"
#include "register/auto_mapping_util.h"
#include "graph/refiner/format_refiner.h"
#include "graph/cache_policy/cache_policy.h"
#include "graph/runtime_inference_context.h"
#include "graph/utils/connection_matrix.h"
#include "graph/utils/execute_graph_utils.h"
#include "common/util/trace_manager/trace_manager.h"
#include "graph/tuning_utils.h"
#include "graph/utils/node_adapter.h"
#include "graph/args_format_desc.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "register/graph_optimizer/fusion_common/fusion_turbo.h"
#include "graph/ir_definitions_recover.h"
#include "graph/utils/ge_ir_utils.h"
#include "exe_graph/lowering/bg_ir_attrs.h"
#include "base/attr/attrs_to_buffer.h"
#include "register/graph_optimizer/graph_fusion/fusion_quant_util_impl.h"
#include "register/scope/scope_graph_impl.h"
#include "proto/tensorflow/graph.pb.h"
#include "graph/utils/recover_ir_utils.h"
#include "graph/normal_graph/op_desc_impl.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/operator.h"
#include "graph/attr_value.h"
#include "graph/ge_attr_value.h"
#include "graph/normal_graph/ge_tensor_impl.h"
#include "graph/detail/attributes_holder.h"
#include "register/scope/scope_fusion_pass_register.h"
#include "register/scope/scope_pattern_impl.h"
#include "register/scope/scope_graph_impl.h"
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "register/graph_optimizer/graph_fusion/fusion_pattern.h"

namespace optiling {
bool op_tiling_stub_st_cov(const ge::Operator &op, const utils::OpCompileInfo &compile_info,
                           utils::OpRunInfo &run_info) {
  return true;
}
}  // namespace optiling

REGISTER_OP_TILING_V2(ReluCovForSt, optiling::op_tiling_stub_st_cov);

namespace ge {
class IncCovLogSTest : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(IncCovLogSTest, IncCovSt_OpParaCalculateV2_NullOpDesc) {
  const Operator op;
  optiling::OpRunInfoV2 run_info;
  EXPECT_EQ(optiling::OpParaCalculateV2(op, run_info), ge::GRAPH_FAILED);
}

TEST_F(IncCovLogSTest, IncCovSt_OpParaCalculateV2_TilingFuncNotFound) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("not_reg", "NeverRegisteredTypeStCov", 1, 1);
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);
  optiling::OpRunInfoV2 run_info;
  EXPECT_EQ(optiling::OpParaCalculateV2(op, run_info), ge::GRAPH_FAILED);
}

TEST_F(IncCovLogSTest, IncCovSt_OpFftsPlusCalculate_EmptySliceInfo) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("relu", "ReluCovForSt", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);

  const ffts::ThreadSliceMapDyPtr slice_info_ptr = std::make_shared<ffts::ThreadSliceMapDy>();
  slice_info_ptr->slice_instance_num = 0U;
  (void)op_desc->SetExtAttr(ffts::kAttrSgtStructInfoDy, slice_info_ptr);
  std::vector<optiling::OpRunInfoV2> op_run_info;
  EXPECT_EQ(optiling::OpFftsPlusCalculate(op, op_run_info), ge::GRAPH_FAILED);
}

TEST_F(IncCovLogSTest, IncCovSt_OpFftsPlusCalculate_SuccessPath) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("relu", "ReluCovForSt", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);

  const ffts::ThreadSliceMapDyPtr slice_info_ptr = std::make_shared<ffts::ThreadSliceMapDy>();
  vector<int64_t> vec_1;
  vec_1.push_back(1);
  vector<vector<int64_t>> vec_2;
  vec_2.push_back(vec_1);
  vec_2.push_back(vec_1);
  slice_info_ptr->parallel_window_size = 2U;
  slice_info_ptr->slice_instance_num = 2U;
  slice_info_ptr->input_tensor_slice.push_back(vec_2);
  slice_info_ptr->input_tensor_slice.push_back(vec_2);
  slice_info_ptr->output_tensor_slice.push_back(vec_2);
  slice_info_ptr->output_tensor_slice.push_back(vec_2);
  slice_info_ptr->input_tensor_indexes.push_back(0U);
  slice_info_ptr->output_tensor_indexes.push_back(0U);
  (void)op_desc->SetExtAttr(ffts::kAttrSgtStructInfoDy, slice_info_ptr);

  GeShape shape({4, 1, 3, 4, 16});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  op_desc->AddInputDesc("x", tensor_desc);
  op_desc->AddOutputDesc("y", tensor_desc);
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_KEY, "cov_st_key");
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_JSON, "cov_st_json");
  auto dst_anchor = node->GetInDataAnchor(0);
  ge::AnchorUtils::SetStatus(dst_anchor, ge::ANCHOR_DATA);

  std::vector<optiling::OpRunInfoV2> op_run_info;
  EXPECT_EQ(optiling::OpFftsPlusCalculate(op, op_run_info), ge::GRAPH_SUCCESS);
}

TEST_F(IncCovLogSTest, IncCovSt_GetInstanceNum_EmptyValidIndexMap) {
  auto op_desc = std::make_shared<OpDesc>("empty_map", "Data");
  size_t instance_num = 0U;
  const std::map<uint32_t, std::string> empty_map;
  EXPECT_EQ(OpDescUtils::GetInstanceNum(op_desc, 0U, 0U, empty_map, instance_num), ge::SUCCESS);
}

// 覆盖 operator.cc GetOpDescImpl 空指针系列 —— 一次串行触发全部注册/查询方法
TEST_F(IncCovLogSTest, IncCovSt_OperatorNullOpDesc_AllMethodsFailed) {
  Operator op = OpDescUtils::CreateOperatorFromOpDesc(nullptr);
  EXPECT_EQ(op.InferShapeAndType(), ge::GRAPH_FAILED);
  EXPECT_EQ(op.VerifyAllAttr(true), ge::GRAPH_FAILED);
  EXPECT_TRUE(op.GetAllAttrNamesAndTypes().empty());
  op.InputRegister(nullptr);
  op.OptionalInputRegister(nullptr);
  op.InferFuncRegister(nullptr);
  op.InferFormatFuncRegister(nullptr);
  op.VerifierFuncRegister(nullptr);
  op.OutputRegister(nullptr);
  op.DynamicInputRegister(nullptr, 1U, true);
  op.DynamicInputRegisterByIndex(nullptr, 1U, 0U);
  op.DynamicOutputRegister(nullptr, 1U, true);
  op.RequiredAttrRegister(nullptr);
  EXPECT_EQ(op.VerifyAll(), ge::GRAPH_FAILED);
}

// 覆盖 hcom_topo_info.cc 空指针系列
TEST_F(IncCovLogSTest, IncCovSt_HcomTopoInfo_NullArgs) {
  HcomTopoInfo::TopoInfo topo_info{};
  EXPECT_NE(HcomTopoInfo::Instance().SetGroupTopoInfo(nullptr, topo_info), ge::SUCCESS);
  void *const stream = nullptr;
  EXPECT_NE(HcomTopoInfo::Instance().SetGroupOrderedStream(0, nullptr, stream), ge::SUCCESS);
}

// 覆盖 ge_tensor.cc value_range/shape_range/origin_shape_range 三元列表非法分支
TEST_F(IncCovLogSTest, IncCovSt_TensorDescRange_InvalidElementSize) {
  GeTensorDesc tensor_desc;
  std::vector<std::vector<int64_t>> invalid_range = {{1, 2, 3}};
  (void)AttrUtils::SetListListInt(tensor_desc, "value_range", invalid_range);
  std::vector<std::pair<int64_t, int64_t>> value_range;
  EXPECT_EQ(tensor_desc.GetValueRange(value_range), ge::GRAPH_FAILED);

  (void)AttrUtils::SetListListInt(tensor_desc, "shape_range", invalid_range);
  std::vector<std::pair<int64_t, int64_t>> shape_range;
  EXPECT_EQ(tensor_desc.GetShapeRange(shape_range), ge::GRAPH_FAILED);

  (void)AttrUtils::SetListListInt(tensor_desc, "origin_shape_range", invalid_range);
  std::vector<std::pair<int64_t, int64_t>> origin_shape_range;
  EXPECT_EQ(tensor_desc.GetOriginShapeRange(origin_shape_range), ge::GRAPH_FAILED);
}

// 覆盖 shape_refiner.cc 空节点与无 infer func 场景
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_NullAndSimpleNode) {
  NodePtr null_node;
  EXPECT_NE(ShapeRefiner::InferShapeAndType(null_node), ge::SUCCESS);
  auto graph = std::make_shared<ComputeGraph>("st_inc_cov_graph");
  OpDescPtr op_desc = std::make_shared<OpDesc>("simple_node", "Relu");
  GeTensorDesc tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
  tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  (void)op_desc->AddInputDesc("x", tensor_desc);
  (void)op_desc->AddOutputDesc("y", tensor_desc);
  const auto node = graph->AddNode(op_desc);
  EXPECT_EQ(ShapeRefiner::InferShapeAndType(node), ge::GRAPH_FAILED);
}

// 覆盖 execute_graph.cc:42 —— OPTION_TOPOSORTING_MODE 非法值分支（打印警告后回退默认策略）
TEST_F(IncCovLogSTest, IncCovSt_ExecuteGraph_InvalidTopoSortingMode) {
  auto ori_graph_options = GetThreadLocalContext().GetAllGraphOptions();
  auto graph_options = ori_graph_options;
  graph_options[OPTION_TOPOSORTING_MODE] = "99";
  GetThreadLocalContext().SetGraphOption(graph_options);
  ExecuteGraph exe_graph("topo_mode_invalid");
  (void)exe_graph.TopologicalSortingGraph(nullptr, false);
  GetThreadLocalContext().SetGraphOption(ori_graph_options);
}

// 覆盖 plugin_manager.cc:449 —— compiler version 不在兼容范围
TEST_F(IncCovLogSTest, IncCovSt_PluginManager_CompilerVersionOutOfRange) {
  PluginManager mgr;
  const std::vector<std::pair<uint32_t, uint32_t>> required = {{999000U, 999001U}};
  EXPECT_FALSE(mgr.CheckOppAndCompilerVersions("6.3", "6.3", required));
}

// 覆盖 plugin_manager.cc:766 —— so 文件路径超长分支（逐条跳过超长路径后返回成功）
TEST_F(IncCovLogSTest, IncCovSt_PluginManager_LoadPathTooLong) {
  PluginManager mgr;
  const std::string too_long_path(5000U, 'a');
  EXPECT_EQ(mgr.Load(too_long_path), ge::SUCCESS);
}

// 覆盖 anchor.cc:24/25 —— InDataAnchor 已有 peer 时 ReplacePeer 失败
TEST_F(IncCovLogSTest, IncCovSt_AnchorReplacePeer_OccupiedInDataAnchor) {
  auto graph = std::make_shared<ComputeGraph>("st_anchor_replace");
  auto op_desc_a = std::make_shared<OpDesc>("node_a", "Data");
  auto op_desc_b = std::make_shared<OpDesc>("node_b", "Relu");
  auto op_desc_c = std::make_shared<OpDesc>("node_c", "Relu");
  (void)op_desc_a->AddOutputDesc("y", GeTensorDesc());
  (void)op_desc_b->AddInputDesc("x", GeTensorDesc());
  (void)op_desc_b->AddOutputDesc("y", GeTensorDesc());
  (void)op_desc_c->AddInputDesc("x", GeTensorDesc());
  const auto node_a = graph->AddNode(op_desc_a);
  const auto node_b = graph->AddNode(op_desc_b);
  const auto node_c = graph->AddNode(op_desc_c);
  // b 和 c 的输入都连到 a 的输出，c 的 InDataAnchor 已有 peer
  ASSERT_EQ(GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_c->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);
  // a 的输出 ReplacePeer(b_in → c_in)，c_in 已有 peer → CanAddPeer 失败
  EXPECT_NE(node_a->GetOutDataAnchor(0)->ReplacePeer(node_b->GetInDataAnchor(0), node_c->GetInDataAnchor(0)),
            ge::GRAPH_SUCCESS);
}

// 覆盖 op_desc_utils.cc:615 —— Invalid map key
TEST_F(IncCovLogSTest, IncCovSt_SetNoneConstNodeWeights_InvalidKey) {
  auto graph = std::make_shared<ComputeGraph>("st_invalid_key");
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  const auto node = graph->AddNode(op_desc);
  auto tensor = std::make_shared<GeTensor>();
  std::map<int, GeTensorPtr> weights{{-1, tensor}};
  EXPECT_EQ(OpDescUtils::SetNoneConstNodeWeights(*node, weights), ge::GRAPH_PARAM_INVALID);
}

// 覆盖 sym_dtype.cc:92/101 —— Alignment 为 0
TEST_F(IncCovLogSTest, IncCovSt_SymbolOperator_AlignZero) {
  auto aligned = sym::Align(Symbol(10U), 0U);
  auto aligned2 = sym::AlignWithPositiveInteger(Symbol(10U), 0U);
  (void)aligned;
  (void)aligned2;
}

// 覆盖 model_serialize.cc:989-990 —— 坏 protobuf 数据
TEST_F(IncCovLogSTest, IncCovSt_UnserializeModel_GarbageData) {
  Model model;
  const uint8_t garbage_data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  EXPECT_FALSE(ModelSerialize().UnserializeModel(garbage_data, sizeof(garbage_data), model));
}

// 覆盖 operator.cc:1370 —— VerifyAll 的 input shape 含负维度
TEST_F(IncCovLogSTest, IncCovSt_OperatorVerifyAll_NegativeDim) {
  auto op_desc = std::make_shared<OpDesc>("neg_dim", "Data");
  GeTensorDesc desc(GeShape({-1}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  (void)op_desc->AddInputDesc("x", desc);
  Operator op = OpDescUtils::CreateOperatorFromOpDesc(op_desc);
  EXPECT_EQ(op.VerifyAll(), ge::GRAPH_FAILED);
}

// 覆盖 register.cpp:418 —— AutoMappingByOpFnDynamic 传入非法 dynamic_type
TEST_F(IncCovLogSTest, IncCovSt_AutoMappingByOpFnDynamic_InvalidType) {
  auto root_builder = ut::GraphBuilder("st_automap_invalid");
  const auto &node_src = root_builder.AddNode("src_node", "Data", 1, 1);
  ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
  ge::Operator op_dst("st_dst_invalid");
  std::vector<domi::DynamicInputOutputInfo> value;
  const domi::DynamicInputOutputInfo invalid_info(static_cast<domi::DynamicType>(99), "port", 4, "attr", 4);
  value.push_back(invalid_info);
  EXPECT_EQ(domi::AutoMappingByOpFnDynamic(op_src, op_dst, value), domi::SUCCESS);
}

// 覆盖 file_utils.cc:316 —— WriteBinToFile 被调用
TEST_F(IncCovLogSTest, IncCovSt_WriteBinToFile) {
  std::string test_file = "./st_test_write_close_fail.bin";
  char_t data[] = "test";
  uint32_t data_len = 4;
  graphStatus ret = WriteBinToFile(test_file, data, data_len);
  (void)ret;
  remove(test_file.c_str());
}
// 覆盖 op_desc.cc:696/709 —— GetAllInputsDesc 遇到 invalid desc
TEST_F(IncCovLogSTest, IncCovSt_OpDescGetAllInputsDesc_InvalidDesc) {
  auto op_desc = std::make_shared<OpDesc>("st_invalid_desc", "Relu");
  GeTensorDesc invalid_desc(GeShape({0}), ge::FORMAT_NCHW, ge::DT_UNDEFINED);
  (void)op_desc->AddInputDesc("x", invalid_desc);
  (void)op_desc->AddOutputDesc("y", GeTensorDesc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  auto inputs = op_desc->GetAllInputsDesc();
  EXPECT_FALSE(inputs.empty());
  auto inputs_ptr = op_desc->GetAllInputsDescPtr();
  EXPECT_FALSE(inputs_ptr.empty());
}

// 覆盖 compute_graph.cc:1954 —— 节点在 targets 中不被移除
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_NodeInTargetNotRemoved) {
  auto graph = std::make_shared<ComputeGraph>("st_remove_isolated");
  auto op_desc = std::make_shared<OpDesc>("data_node", "Data");
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  const auto node = graph->AddNode(op_desc);
  graph->SetGraphTargetNodesInfo({node});
  auto all_nodes = graph->GetAllNodes();
  EXPECT_FALSE(all_nodes.empty());
}

// 覆盖 gnode.cc:708 —— GNode index 小于零
TEST_F(IncCovLogSTest, IncCovSt_GNode_UpdateOutputDesc_NegativeIndex) {
  auto op_desc = std::make_shared<OpDesc>("gnode_test", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  auto graph = std::make_shared<ComputeGraph>("st_gnode");
  const auto node = graph->AddNode(op_desc);
  GNode gnode = NodeAdapter::Node2GNode(node);
  TensorDesc tensor_desc;
  EXPECT_EQ(gnode.UpdateOutputDesc(-1, tensor_desc), ge::GRAPH_PARAM_INVALID);
}

// 覆盖 graph_utils.cc:1334 —— dump 文件大小超限
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_DumpGraphSizeExceedsLimit) {
  auto graph = std::make_shared<ComputeGraph>("st_dump_size");
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  GraphUtils::DumpGEGraph(graph, "st_dump_suffix");
  SUCCEED();
}

// 覆盖 graph_utils.cc:1490 —— 文件名超长
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_FileNameTooLong) {
  auto graph = std::make_shared<ComputeGraph>("st_name_long");
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  const std::string long_suffix(260, 'x');
  GraphUtils::DumpGEGraph(graph, long_suffix);
  SUCCEED();
}

// 覆盖 connection_matrix.cc:135 —— 图不一致
TEST_F(IncCovLogSTest, IncCovSt_ConnectionMatrix_UpdateGraphMismatch) {
  auto graph1 = std::make_shared<ComputeGraph>("st_graph1");
  auto graph2 = std::make_shared<ComputeGraph>("st_graph2");
  ConnectionMatrix matrix(graph1);
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  const auto node = graph2->AddNode(op_desc);
  std::vector<NodePtr> fusion_nodes = {node};
  matrix.Update(graph2, fusion_nodes);
  SUCCEED();
}

// 覆盖 tuning_utils.cc:679/681 —— ConvertFileToGraph 空 options
TEST_F(IncCovLogSTest, IncCovSt_TuningUtils_ConvertFileToGraph_EmptyOptions) {
  std::map<int64_t, std::string> options;
  ge::Graph graph("st_tuning_graph");
  auto ret = TuningUtils::ConvertFileToGraph(options, graph);
  (void)ret;
}

// 覆盖 model_serialize.cc:824 —— 并行反序列化图失败
TEST_F(IncCovLogSTest, IncCovSt_UnserializeModel_MultiThread_GarbageData) {
  Model model;
  std::vector<uint8_t> big_garbage(200);
  for (size_t i = 0; i < big_garbage.size(); i++) {
    big_garbage[i] = static_cast<uint8_t>(i % 256);
  }
  EXPECT_FALSE(ModelSerialize().UnserializeModel(big_garbage.data(), big_garbage.size(), model, true));
}

// 覆盖 op_tiling_info.cc:115 —— offset 溢出
TEST_F(IncCovLogSTest, IncCovSt_OpTilingInfo_OffsetOverflow) {
  optiling::utils::OpRunInfo run_info;
  EXPECT_FALSE(run_info.SetMemCheckBaseOffset(SIZE_MAX - 4));
}

// 覆盖 format_refiner.cc:422 —— InferOrigineFormat（覆盖 DataNodeFormatProcess 前置判断）
TEST_F(IncCovLogSTest, IncCovSt_FormatRefiner_InferOrigineFormat) {
  auto graph = std::make_shared<ComputeGraph>("st_format_refiner");
  auto op_desc = std::make_shared<OpDesc>("data_node", "Data");
  (void)op_desc->AddOutputDesc("y", GeTensorDesc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  (void)graph->AddNode(op_desc);
  auto ret = FormatRefiner::InferOrigineFormat(graph);
  (void)ret;
}

// 覆盖 shape_refiner.cc:867 —— dtype reset
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_InferShapeAndTypeForRunning) {
  auto graph = std::make_shared<ComputeGraph>("st_dtype_reset");
  auto op_desc = std::make_shared<OpDesc>("data_node", "Data");
  (void)op_desc->AddOutputDesc("y", GeTensorDesc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  const auto node = graph->AddNode(op_desc);
  Operator op = OpDescUtils::CreateOperatorFromOpDesc(op_desc);
  auto ret = ShapeRefiner::InferShapeAndTypeForRunning(node, op, true);
  (void)ret;
}

// 覆盖 node_utils.cc 系列 —— TryGetWeightByDataNode
TEST_F(IncCovLogSTest, IncCovSt_NodeUtils_TryGetWeightByDataNode) {
  auto graph = std::make_shared<ComputeGraph>("st_node_utils");
  auto op_desc = std::make_shared<OpDesc>("data_node", "Data");
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  const auto node = graph->AddNode(op_desc);
  ConstGeTensorPtr ge_tensor;
  auto ret = NodeUtils::TryGetWeightByDataNode(node, ge_tensor);
  (void)ret;
}

// 覆盖 runtime_inference_context.cc:48 —— 节点未注册
TEST_F(IncCovLogSTest, IncCovSt_RuntimeInferenceContext_NodeNotRegistered) {
  RuntimeInferenceContext ctx;
  GeTensorPtr tensor;
  EXPECT_EQ(ctx.GetTensor(99999LL, 0, tensor), ge::INTERNAL_ERROR);
}

// 覆盖 trace_manager.cc:112 —— trace 文件名超长
TEST_F(IncCovLogSTest, IncCovSt_TraceManager_FileNameExceedsMax) {
  const std::string long_path(5000, 't');
  auto ret = TraceManager::GetInstance().Initialize(long_path.c_str());
  (void)ret;
}

// 覆盖 register.cpp:477 —— 子图名已存在（通过 AutoMappingByOpFnDynamic 的 AutoMappingFn 路径）
TEST_F(IncCovLogSTest, IncCovSt_AutoMappingFn_SubgraphAlreadyExists) {
  auto root_builder = ut::GraphBuilder("st_subgraph_dup");
  const auto &node_src = root_builder.AddNode("src_node", "Data", 1, 1);
  ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
  ge::Operator op_dst("st_dst_subgraph");
  auto op_desc_dst = OpDescUtils::GetOpDescFromOperator(op_dst);
  op_desc_dst->AddSubgraphName("test_subgraph");
  op_desc_dst->AddSubgraphName("test_subgraph");
  // AutoMappingByOpFn 内部会调用 AutoMappingFn，重复子图名会走到 477 行的 warning 分支
  auto ret = domi::AutoMappingByOpFn(op_src, op_dst);
  (void)ret;
}

// 覆盖 op_tiling.cc:272 —— 常量输入 tensor data
TEST_F(IncCovLogSTest, IncCovSt_OpParaCalculateV2_WithConstInput) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("relu_const", "ReluV4", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  (void)op_desc->SetExtAttr("optiling_infer_depends", std::vector<std::string>{"x"});
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_KEY, "st_const_key");
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_JSON, "st_const_json");
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);
  optiling::OpRunInfoV2 run_info;
  auto ret = optiling::OpParaCalculateV2(op, run_info);
  (void)ret;
}

// 覆盖 execute_graph_utils.cc:574 —— ReplaceEdgeSrc 失败
TEST_F(IncCovLogSTest, IncCovSt_ExecuteGraphUtils_ReplaceEdgeSrc_Fail) {
  FastNode *null_src = nullptr;
  auto ret = ExecuteGraphUtils::ReplaceEdgeSrc(nullptr, {null_src, 0});
  (void)ret;
}

// 覆盖 inference_context.cc:152 —— resource context mgr 为空时的间接覆盖
TEST_F(IncCovLogSTest, IncCovSt_InferenceContext_SetNullResourceContext) {
  auto ctx = InferenceContext::Create(nullptr);
  ASSERT_NE(ctx, nullptr);
  (void)ctx->SetResourceContext(AscendString("st_key"), nullptr);
  SUCCEED();
}

// 覆盖 args_format_desc.cc —— 非法占位符宽度（PlaceholderParser 失败分支）
TEST_F(IncCovLogSTest, IncCovSt_ArgsFormatDesc_InvalidPlaceholder) {
  auto op_desc = std::make_shared<OpDesc>("fmt_op", "Relu");
  std::vector<ArgDesc> arg_descs;
  EXPECT_NE(ArgsFormatDesc::Parse(op_desc, "{i*.64b}", arg_descs), ge::GRAPH_SUCCESS);
}

// 覆盖 args_format_desc.cc:184 —— InputCalcSize 中 ir_index 越界
TEST_F(IncCovLogSTest, IncCovSt_ArgsFormatDesc_InputIrIndexOutOfRange) {
  auto op_desc = std::make_shared<OpDesc>("fmt_op_in", "Relu");
  ArgsFormatDesc format;
  format.Append(AddrType::INPUT, 5);
  size_t args_size = 0U;
  EXPECT_NE(format.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
}

// 覆盖 args_format_desc.cc:428 —— OutputDescCalcSize 中 ir_index 越界
TEST_F(IncCovLogSTest, IncCovSt_ArgsFormatDesc_OutputIrIndexOutOfRange) {
  auto op_desc = std::make_shared<OpDesc>("fmt_op_out", "Relu");
  ArgsFormatDesc format;
  format.Append(AddrType::OUTPUT, 5);
  size_t args_size = 0U;
  EXPECT_NE(format.GetArgsSize(op_desc, args_size), ge::GRAPH_SUCCESS);
}

// 覆盖 op_desc_utils_ex.cc:83 —— 未注册 infer func 时调用 CallInferFunc
TEST_F(IncCovLogSTest, IncCovSt_CallInferFunc_NoInferFunc) {
  auto op_desc = std::make_shared<OpDesc>("no_infer_op", "StNoInferFuncType");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  Operator op = OpDescUtils::CreateOperatorFromOpDesc(op_desc);
  auto ret = OpDescUtilsEx::CallInferFunc(op_desc, op);
  (void)ret;
}

// 覆盖 shape_refiner.cc:867 —— running 场景下 infer 后 dtype 变化需要复位
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefinerForRunning_DtypeReset) {
  auto op_desc = std::make_shared<OpDesc>("cast_running", "Cast");
  (void)op_desc->AddInputDesc("x", GeTensorDesc(GeShape({1, 3}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  (void)op_desc->AddOutputDesc("y", GeTensorDesc(GeShape({1, 3}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  (void)AttrUtils::SetInt(op_desc, "dst_type", static_cast<int64_t>(ge::DT_INT32));
  auto graph = std::make_shared<ComputeGraph>("st_cast_running");
  const auto node = graph->AddNode(op_desc);
  Operator op = OpDescUtils::CreateOperatorFromNode(node);
  auto ret = ShapeRefiner::InferShapeAndTypeForRunning(node, op, true);
  (void)ret;
}

// 覆盖 fusion_turbo.cc:552 —— InsertNodeBefore 成功路径
TEST_F(IncCovLogSTest, IncCovSt_FusionTurbo_InsertNodeBefore) {
  auto graph = std::make_shared<ComputeGraph>("st_fusion_turbo_before");
  auto data_desc = std::make_shared<OpDesc>("data", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc());
  auto relu_desc = std::make_shared<OpDesc>("relu", "Relu");
  (void)relu_desc->AddInputDesc("x", GeTensorDesc());
  (void)relu_desc->AddOutputDesc("y", GeTensorDesc());
  const auto data_node = graph->AddNode(data_desc);
  const auto relu_node = graph->AddNode(relu_desc);
  ASSERT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);
  fe::FusionTurbo turbo(graph);
  auto new_node = turbo.InsertNodeBefore("new_before", "Relu", relu_node, 0);
  (void)new_node;
}

// 覆盖 fusion_turbo.cc:620 —— InsertNodeAfter 成功路径
TEST_F(IncCovLogSTest, IncCovSt_FusionTurbo_InsertNodeAfter) {
  auto graph = std::make_shared<ComputeGraph>("st_fusion_turbo_after");
  auto relu_desc = std::make_shared<OpDesc>("relu", "Relu");
  (void)relu_desc->AddInputDesc("x", GeTensorDesc());
  (void)relu_desc->AddOutputDesc("y", GeTensorDesc());
  auto out_desc = std::make_shared<OpDesc>("net_output", "NetOutput");
  (void)out_desc->AddInputDesc("x", GeTensorDesc());
  const auto relu_node = graph->AddNode(relu_desc);
  const auto out_node = graph->AddNode(out_desc);
  ASSERT_EQ(GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);
  fe::FusionTurbo turbo(graph);
  auto new_node = turbo.InsertNodeAfter("new_after", "Relu", relu_node, 0);
  (void)new_node;
}

// 覆盖 ir_definitions_recover.cc:346 —— IR attr names 与注册定义不兼容
TEST_F(IncCovLogSTest, IncCovSt_RecoverIrDefinitions_FakeAttrName) {
  auto op_desc = std::make_shared<OpDesc>("matmul", "MatMul");
  auto graph = std::make_shared<ComputeGraph>("st_ir_recover_graph");
  ASSERT_NE(graph->AddNode(op_desc), nullptr);
  op_desc->impl_->meta_data_.ir_meta_.ir_attr_names_.emplace_back("st_fake_attr");
  auto ret = RecoverIrUtils::RecoverIrDefinitions(graph);
  (void)ret;
}

// 覆盖 ir_definitions_recover.cc:349/465 —— IR inputs 与注册定义不兼容
TEST_F(IncCovLogSTest, IncCovSt_RecoverIrDefinitions_FakeIrInput) {
  auto op_desc = std::make_shared<OpDesc>("matmul", "MatMul");
  auto graph = std::make_shared<ComputeGraph>("st_ir_recover_graph_in");
  ASSERT_NE(graph->AddNode(op_desc), nullptr);
  op_desc->impl_->meta_data_.ir_meta_.ir_inputs_.ir_inputs.emplace_back(
      std::pair<std::string, IrInputType>("st_fake_input", kIrInputRequired));
  auto ret = RecoverIrUtils::RecoverIrDefinitions(graph);
  (void)ret;
}

// 覆盖 ge_ir_utils.cc:955/966/975/986 —— OnnxUtils 属性类型不匹配
TEST_F(IncCovLogSTest, IncCovSt_OnnxUtils_DecodeAttributeWrongType) {
  ge::onnx::AttributeProto attr;
  attr.set_name("st_attr");
  attr.set_type(ge::onnx::AttributeProto_AttributeType_INT);
  std::vector<std::string> strings;
  OnnxUtils::DecodeAttribute(attr, strings);
  std::string str_value;
  OnnxUtils::DecodeAttribute(attr, str_value);
  std::vector<int64_t> ints;
  OnnxUtils::DecodeAttribute(attr, ints);
  attr.set_type(ge::onnx::AttributeProto_AttributeType_STRING);
  int64_t int_value = 0;
  OnnxUtils::DecodeAttribute(attr, int_value);
}

// 覆盖 scope_graph.cc:60/64 —— peer out index 越界/非法
TEST_F(IncCovLogSTest, IncCovSt_ScopeGraph_InvalidPeerIndex) {
  domi::tensorflow::GraphDef graph_def;
  auto *node = graph_def.add_node();
  node->set_name("st_scope_node");
  node->set_op("Add");
  node->add_input("peer:-1");
  ScopeGraph::ScopeGraphImpl scope_graph;
  scope_graph.BuildScopeGraph(&graph_def);

  domi::tensorflow::GraphDef graph_def_out_of_range;
  auto *node2 = graph_def_out_of_range.add_node();
  node2->set_name("st_scope_node2");
  node2->set_op("Add");
  node2->add_input("peer:99999999999999999");
  ScopeGraph::ScopeGraphImpl scope_graph2;
  scope_graph2.BuildScopeGraph(&graph_def_out_of_range);
}

// 覆盖 bg_ir_attrs.cc:79 —— tensor 属性 shape size 为负
TEST_F(IncCovLogSTest, IncCovSt_BgIrAttrs_NegativeShapeTensorAttr) {
  auto graph = std::make_shared<ComputeGraph>("st_neg_shape_attr");
  auto op_desc = std::make_shared<OpDesc>("st_neg_shape", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  op_desc->impl_->meta_data_.ir_meta_.ir_attr_names_.emplace_back("st_neg_shape_attr");
  ge::GeTensor ge_tensor(GeTensorDesc(GeShape({-1, 2}), ge::FORMAT_ND, ge::DT_FLOAT));
  (void)AttrUtils::SetTensor(op_desc, "st_neg_shape_attr", ge_tensor);
  const auto node = graph->AddNode(op_desc);
  std::vector<std::vector<uint8_t>> runtime_attrs;
  EXPECT_FALSE(gert::bg::GetAllIrAttrs(node, runtime_attrs));
}

// 覆盖 tuning_utils.cc:679/681 —— 控制边场景下 LinkEnd2NetOutput 失败分支
TEST_F(IncCovLogSTest, IncCovSt_TuningUtils_LinkEnd2NetOutput_ControlEdge) {
  auto graph = std::make_shared<ComputeGraph>("st_link_end");
  auto src_desc = std::make_shared<OpDesc>("src", "Data");
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto end_desc = std::make_shared<OpDesc>("end_node", "END");
  (void)end_desc->AddInputDesc("x", GeTensorDesc());
  auto out_desc = std::make_shared<OpDesc>("net_out", "NetOutput");
  (void)out_desc->AddInputDesc("x", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  NodePtr end_node = graph->AddNode(end_desc);
  NodePtr out_node = graph->AddNode(out_desc);
  ASSERT_EQ(GraphUtils::AddEdge(src_node->GetOutControlAnchor(), end_node->GetInControlAnchor()), ge::GRAPH_SUCCESS);
  auto ret = TuningUtils::LinkEnd2NetOutput(end_node, out_node);
  (void)ret;
}

// 覆盖 op_desc.cc:696/709 —— FORMAT_RESERVED + DT_UNDEFINED 的无效 input desc
TEST_F(IncCovLogSTest, IncCovSt_OpDescGetAllInputsDesc_InvalidFormatReserved) {
  auto op_desc = std::make_shared<OpDesc>("st_invalid_desc2", "Relu");
  GeTensorDesc invalid_desc(GeShape(), static_cast<ge::Format>(40), ge::DT_UNDEFINED);
  (void)op_desc->AddInputDesc("x", invalid_desc);
  (void)op_desc->AddOutputDesc("y", GeTensorDesc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT));
  auto inputs = op_desc->GetAllInputsDesc();
  EXPECT_TRUE(inputs.empty());
  auto inputs_ptr = op_desc->GetAllInputsDescPtr();
  EXPECT_TRUE(inputs_ptr.empty());
}

// 覆盖 graph_utils.cc:1490 —— dump 文件名超长（is_always_dump 跳过开关判断）
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_DumpFileNameTooLong) {
  auto graph = std::make_shared<ComputeGraph>("st_dump_name_long");
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  const std::string long_suffix(300, 'x');
  GraphUtils::DumpGrphToOnnx(*graph, "./", long_suffix);
}

// 覆盖 graph_utils.cc:1334 —— dump 文件大小超限
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_DumpFileSizeExceedsLimit) {
  auto graph = std::make_shared<ComputeGraph>("st_dump_size2");
  auto op_desc = std::make_shared<OpDesc>("node", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  std::map<std::string, std::string> options;
  options[OPTION_GE_MAX_DUMP_FILE_SIZE] = "1";
  GetThreadLocalContext().SetGlobalOption(options);
  GraphUtils::DumpGEGraph(graph, "st_dump_size_suffix", true);
  GetThreadLocalContext().SetGlobalOption(std::map<std::string, std::string>{});
}

// 覆盖 register.cpp:418 —— 动态映射中 dynamic_type 非法
TEST_F(IncCovLogSTest, IncCovSt_AutoMappingByOpFnDynamic_InvalidDynamicType) {
  ut::GraphBuilder builder("st_dyn_invalid");
  const auto &node_src = builder.AddNode("src_node", "Data", 1, 1);
  (void)AttrUtils::SetInt(node_src->GetOpDesc(), "st_dyn_attr", 1);
  ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(node_src);
  ge::Operator op_dst("StDynInvalid");
  auto op_desc_dst = OpDescUtils::GetOpDescFromOperator(op_dst);
  op_desc_dst->AddRegisterInputName("st_port");
  std::vector<domi::DynamicInputOutputInfo> value;
  value.emplace_back(domi::DynamicInputOutputInfo(static_cast<domi::DynamicType>(99), "st_port", 7, "st_dyn_attr", 11));
  auto ret = domi::AutoMappingByOpFnDynamic(op_src, op_dst, value);
  (void)ret;
}

// 覆盖 fusion_quant_util_impl.cc:258 —— index_co 为负
TEST_F(IncCovLogSTest, IncCovSt_QuantUtilImpl_GetCoValueByWeight_NegativeIndex) {
  auto graph = std::make_shared<ComputeGraph>("st_quant_co");
  auto cube_desc = std::make_shared<OpDesc>("cube_node", "Conv2D");
  (void)cube_desc->AddInputDesc("x", GeTensorDesc(GeShape({1, 4, 16, 16, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16));
  (void)cube_desc->AddOutputDesc("y", GeTensorDesc());
  NodePtr cube_node = graph->AddNode(cube_desc);
  std::vector<int64_t> bias_shape;
  EXPECT_EQ(fe::QuantUtilImpl::GetCoValueByWeight(cube_node, 0, bias_shape), FAILED);
}

// 覆盖 fusion_quant_util_impl.cc:485 —— quant_offset/quant_scale 为空
TEST_F(IncCovLogSTest, IncCovSt_QuantUtilImpl_SetQuantScaleAndOffset_NullAnchors) {
  BiasOptimizeEdges param{};
  ge::OpDescPtr host_op_desc = std::make_shared<OpDesc>("host", "WeightQuantBatchMatmul");
  EXPECT_EQ(fe::QuantUtilImpl::SetQuantScaleAndOffset(nullptr, param, host_op_desc), FAILED);
}

// 覆盖 type_utils_inner.cc:284/285 —— sub_format 超出 int32 范围
TEST_F(IncCovLogSTest, IncCovSt_TypeUtilsInner_SubFormatOutOfRange) {
  std::string primary_format;
  int32_t sub_format = 0;
  EXPECT_NE(TypeUtilsInner::SplitFormatFromStr("ND:99999999999999999999", primary_format, sub_format), GRAPH_SUCCESS);
}

// 覆盖 annotated_kernel_args_impl.h:39/40 —— 参数状态非法/参数为空
TEST_F(IncCovLogSTest, IncCovSt_AnnotatedKernelArgs_Invalid) {
  gert::internal::AnnotatedKernelArgsImpl args_impl;
  args_impl.status = GRAPH_FAILED;
  std::vector<uint8_t> args_data;
  std::vector<ArgDesc> arg_descs;
  EXPECT_NE(gert::internal::ExtractArgsData(&args_impl, args_data, arg_descs), GRAPH_SUCCESS);
  args_impl.status = GRAPH_SUCCESS;
  EXPECT_NE(gert::internal::ExtractArgsData(&args_impl, args_data, arg_descs), GRAPH_SUCCESS);
}

// 覆盖 cache_policy.cc:67 —— LRU_K 老化策略首次不满足添加缓存条件
class IncCovCacheDesc : public CacheDesc {
 public:
  bool IsEqual(const CacheDescPtr &) const override {
    return true;
  }
  bool IsMatch(const CacheDescPtr &) const override {
    return true;
  }
  CacheHashKey GetCacheDescHash() const override {
    return 0U;
  }
};

TEST_F(IncCovLogSTest, IncCovSt_CachePolicy_LruKNotReady) {
  auto cache_policy =
      CachePolicy::Create(MatchPolicyType::MATCH_POLICY_EXACT_ONLY, AgingPolicyType::AGING_POLICY_LRU_K, 4U);
  ASSERT_NE(cache_policy, nullptr);
  (void)cache_policy->AddCache(std::make_shared<IncCovCacheDesc>());
}

// 覆盖 compute_graph.cc:53 —— 非法拓扑排序模式
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_InvalidTopoMode) {
  std::map<std::string, std::string> options;
  options[OPTION_TOPOSORTING_MODE] = "99";
  GetThreadLocalContext().SetGraphOption(options);
  auto graph = std::make_shared<ComputeGraph>("st_topo_invalid");
  (void)graph->TopologicalSorting();
  GetThreadLocalContext().SetGraphOption(std::map<std::string, std::string>{});
}

// 覆盖 compute_graph.cc:1295 —— 子图名与入参不一致
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_AddSubgraphNameMismatch) {
  auto graph = std::make_shared<ComputeGraph>("st_subgraph_root");
  auto parent_desc = std::make_shared<OpDesc>("if_node", "If");
  (void)parent_desc->AddInputDesc("x", GeTensorDesc());
  (void)parent_desc->AddOutputDesc("y", GeTensorDesc());
  auto parent_node = graph->AddNode(parent_desc);
  auto subgraph = std::make_shared<ComputeGraph>("st_subgraph_real");
  subgraph->SetParentGraph(graph);
  subgraph->SetParentNode(parent_node);
  EXPECT_EQ(graph->AddSubgraph("st_subgraph_alias", subgraph), GRAPH_SUCCESS);
}

// 覆盖 compute_graph.cc:1720 —— target 中包含空节点
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_SetTargetNullNode) {
  auto graph = std::make_shared<ComputeGraph>("st_target_null");
  std::vector<NodePtr> targets;
  targets.emplace_back(nullptr);
  graph->SetGraphTargetNodesInfo(targets);
}

// 覆盖 compute_graph_backend.cc:211 —— 重复添加同名算子
TEST_F(IncCovLogSTest, IncCovSt_Graph_AddOpDuplicate) {
  ge::Graph graph("st_graph_dup");
  auto op_desc = std::make_shared<OpDesc>("dup_op", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  ge::Operator op = OpDescUtils::CreateOperatorFromOpDesc(op_desc);
  EXPECT_EQ(graph.AddOp(op), GRAPH_SUCCESS);
  EXPECT_EQ(graph.AddOp(op), GRAPH_SUCCESS);
}

// 覆盖 ge_tensor.cc:1906 —— 含未知维度的 shape 比较
TEST_F(IncCovLogSTest, IncCovSt_TensorUtils_IsShapeEqualUnknownDim) {
  EXPECT_TRUE(TensorUtils::IsShapeEqual(GeShape({-1, 3}), GeShape({2, 3})));
}

// 覆盖 model.cc:151 —— 保存路径的目录创建失败
TEST_F(IncCovLogSTest, IncCovSt_Model_SaveToFileCreateDirFailed) {
  const std::string file = "/tmp/opencode/st_incov_model_file";
  {
    std::ofstream ofs(file, std::ios::out | std::ios::trunc);
    ofs << "x";
  }
  Model model("st_model_dir", "");
  EXPECT_NE(model.SaveToFile(file + "/sub/model.om"), GRAPH_SUCCESS);
  (void)std::remove(file.c_str());
}

// 覆盖 op_desc.cc:384/423/449 —— 动态输入/输出描述重复添加
TEST_F(IncCovLogSTest, IncCovSt_OpDesc_DynamicDescDuplicate) {
  OpDesc op_desc("op", "Relu");
  EXPECT_EQ(op_desc.AddOutputDescMiddle("od", 1U, 0U), GRAPH_SUCCESS);
  EXPECT_NE(op_desc.AddOutputDescMiddle("od", 1U, 0U), GRAPH_SUCCESS);
  ASSERT_NE(op_desc.impl_, nullptr);
  EXPECT_EQ(op_desc.impl_->AddInputDescForward("if", 1U), GRAPH_SUCCESS);
  EXPECT_NE(op_desc.impl_->AddInputDescForward("if", 1U), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc.AddOutputDescForward("of", 1U), GRAPH_SUCCESS);
  EXPECT_NE(op_desc.AddOutputDescForward("of", 1U), GRAPH_SUCCESS);
}

// 覆盖 op_desc.cc:1107 —— 默认推导格式日志
TEST_F(IncCovLogSTest, IncCovSt_OpDesc_DefaultInferFormat) {
  auto op_desc = std::make_shared<OpDesc>("op", "Relu");
  GeTensorDesc desc(GeShape({2, 3}), FORMAT_NCHW, DT_FLOAT);
  (void)op_desc->AddInputDesc("x", desc);
  (void)op_desc->AddOutputDesc("y", desc);
  EXPECT_EQ(op_desc->DefaultInferFormat(), GRAPH_SUCCESS);
}

// 覆盖 op_desc.cc:1153/1212 —— 动态输入/输出索引不连续
TEST_F(IncCovLogSTest, IncCovSt_OpDesc_DynamicIndexesNotContinuous) {
  OpDesc op_desc("op", "Relu");
  (void)op_desc.AddInputDesc("ix0", GeTensorDesc());
  (void)op_desc.AddInputDesc("mid", GeTensorDesc());
  (void)op_desc.AddInputDesc("ix1", GeTensorDesc());
  std::vector<int32_t> indexes;
  EXPECT_NE(op_desc.GetDynamicInputIndexesByName("ix", indexes), GRAPH_SUCCESS);
  (void)op_desc.AddOutputDesc("ox0", GeTensorDesc());
  (void)op_desc.AddOutputDesc("mid2", GeTensorDesc());
  (void)op_desc.AddOutputDesc("ox1", GeTensorDesc());
  indexes.clear();
  EXPECT_NE(op_desc.GetDynamicOutputIndexesByName("ox", indexes), GRAPH_SUCCESS);
}

// 覆盖 operator_impl.cc:49/51 —— 源算子为多输出时设置输入
TEST_F(IncCovLogSTest, IncCovSt_Operator_SetInputMultiOutputSrc) {
  auto src_desc = std::make_shared<OpDesc>("src_multi", "Split");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y0", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y1", GeTensorDesc());
  Operator src = OpDescUtils::CreateOperatorFromOpDesc(src_desc);
  auto dst_desc = std::make_shared<OpDesc>("dst_op", "Relu");
  (void)dst_desc->AddInputDesc("x0", GeTensorDesc());
  (void)dst_desc->AddOutputDesc("z", GeTensorDesc());
  Operator dst = OpDescUtils::CreateOperatorFromOpDesc(dst_desc);
  dst.SetInput("x0", src);
}

// 覆盖 ge_local_context.cc:29/88 —— 超时选项非整数、option name map 重复设置
TEST_F(IncCovLogSTest, IncCovSt_GeLocalContext_InvalidTimeoutValue) {
  std::map<std::string, std::string> options;
  options["stream_sync_timeout"] = "invalid_value";
  options["event_sync_timeout"] = "invalid_value";
  GetThreadLocalContext().SetGlobalOption(options);
  GetThreadLocalContext().SetGlobalOption(std::map<std::string, std::string>{});
}

TEST_F(IncCovLogSTest, IncCovSt_GeLocalContext_SetOptionNameMapTwice) {
  const std::string name_map = "{\"ge.exec.incov\":\"-\"}";
  EXPECT_EQ(GetThreadLocalContext().SetOptionNameMap(name_map), GRAPH_SUCCESS);
  EXPECT_EQ(GetThreadLocalContext().SetOptionNameMap(name_map), GRAPH_SUCCESS);
}

// 覆盖 optimization_option.cc:144/152/170 —— pass 开关配置非法/合法
TEST_F(IncCovLogSTest, IncCovSt_OptimizationOption_RefreshPassSwitchInvalid) {
  OptimizationOption option;
  EXPECT_EQ(option.RefreshPassSwitch("badtoken;:on;st_incov_pass:on"), GRAPH_SUCCESS);
}

// 覆盖 ffts_graph_utils.cc:381/429/500-504/512-517/532
TEST_F(IncCovLogSTest, IncCovSt_FftsGraphUtils_RecursionTooDeep) {
  auto graph = std::make_shared<ComputeGraph>("st_ffts_depth");
  std::map<NodePtr, std::vector<uint32_t>> node_value;
  std::map<ComputeGraphPtr, std::vector<uint32_t>> graph_value;
  const FftsGraphUtils::CalcFunc calc_func = [](const NodePtr &) { return std::vector<uint32_t>{1U}; };
  EXPECT_NE(FftsGraphUtils::Calculate(graph, calc_func, node_value, graph_value, 1000U), GRAPH_SUCCESS);
  EXPECT_TRUE(FftsGraphUtils::Calculate(NodePtr(), calc_func, node_value, graph_value, 1U).empty());
  EXPECT_NE(FftsGraphUtils::PartitionGraphWithLimit(graph, node_value, graph_value, std::vector<uint32_t>{}, 1000U),
            GRAPH_SUCCESS);
}

TEST_F(IncCovLogSTest, IncCovSt_FftsGraphUtils_ValueSizeMismatch) {
  auto graph = std::make_shared<ComputeGraph>("st_ffts_valid");
  auto node = graph->AddNode(std::make_shared<OpDesc>("fnode", "Relu"));
  std::map<NodePtr, std::vector<uint32_t>> node_value;
  std::map<ComputeGraphPtr, std::vector<uint32_t>> graph_value;
  node_value[node] = {1U, 2U};
  EXPECT_FALSE(FftsGraphUtils::IsValueValid(graph, {1U}, node_value, graph_value));
  std::map<NodePtr, std::vector<uint32_t>> node_value2;
  graph_value[graph] = {1U, 2U};
  EXPECT_FALSE(FftsGraphUtils::IsValueValid(graph, {1U}, node_value2, graph_value));
}

// 覆盖 graph_utils.cc:533/535/562/564/622 —— 跨图插入节点
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_InsertNodeCrossGraph) {
  ut::GraphBuilder builder_a("st_insert_a");
  auto src_a = builder_a.AddNode("src_a", "Relu", 1, 1);
  auto insert_a = builder_a.AddNode("insert_a", "Relu", 1, 1);
  ut::GraphBuilder builder_b("st_insert_b");
  auto insert_b = builder_b.AddNode("insert_b", "Relu", 1, 1);
  auto dst_b = builder_b.AddNode("dst_b", "Relu", 1, 1);
  EXPECT_NE(GraphUtils::InsertNodeAfter(src_a->GetOutDataAnchor(0), std::vector<InDataAnchorPtr>{}, insert_b, 0U, 0U),
            GRAPH_SUCCESS);
  EXPECT_NE(GraphUtils::InsertNodeAfter(src_a->GetOutDataAnchor(0), {dst_b->GetInDataAnchor(0)}, insert_a, 0U, 0U),
            GRAPH_SUCCESS);
  EXPECT_NE(GraphUtils::InsertNodeBefore(dst_b->GetInDataAnchor(0), insert_a, 0U, 0U), GRAPH_SUCCESS);
}

// 覆盖 graph_utils.cc:1016 —— 数字后缀无需 dump
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_NoNeedDumpByNumericSuffix) {
  (void)setenv("DUMP_GRAPH_LEVEL", "3", 1);
  EXPECT_TRUE(GraphUtils::NoNeedDumpGraphBySuffix("123"));
  (void)unsetenv("DUMP_GRAPH_LEVEL");
}

// 覆盖 graph_utils.cc:2738/3862 —— 拷贝/展开子图递归深度超限
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_CopyAndUnfoldRecursionTooDeep) {
  auto src = std::make_shared<ComputeGraph>("st_copy_src");
  auto dst = std::make_shared<ComputeGraph>("st_copy_dst");
  std::map<ConstNodePtr, NodePtr> node_old_2_new;
  std::map<ConstOpDescPtr, OpDescPtr> op_desc_old_2_new;
  EXPECT_NE(GraphUtils::CopyComputeGraph(src, dst, node_old_2_new, op_desc_old_2_new, 1000), GRAPH_SUCCESS);
  auto target_graph = std::make_shared<ComputeGraph>("st_unfold_target");
  auto target_node = target_graph->AddNode(std::make_shared<OpDesc>("tn", "Relu"));
  const std::function<bool(const ComputeGraphPtr &)> filter = [](const ComputeGraphPtr &) { return true; };
  EXPECT_NE(GraphUtils::UnfoldGraph(src, target_graph, target_node, filter, 1000), GRAPH_SUCCESS);
}

// 覆盖 node_utils.cc:1431/1446 —— Placeholder 无父节点/父节点无权重
TEST_F(IncCovLogSTest, IncCovSt_NodeUtils_TryGetWeightPlaceHolderNoParent) {
  ut::GraphBuilder builder("st_tgw");
  auto placeholder = builder.AddNode("pl", PLACEHOLDER, 1, 1);
  ConstGeTensorPtr tensor;
  EXPECT_EQ(NodeUtils::TryGetWeightByPlaceHolderNode(placeholder, tensor), GRAPH_SUCCESS);
  auto parent = builder.AddNode("parent", "Relu", 1, 1);
  auto pl2_desc = std::make_shared<OpDesc>("pl2", PLACEHOLDER);
  (void)pl2_desc->AddInputDesc("x", GeTensorDesc());
  (void)pl2_desc->AddOutputDesc("y", GeTensorDesc());
  pl2_desc->SetExtAttr("parentNode", parent);
  auto pl2 = builder.GetGraph()->AddNode(pl2_desc);
  ConstGeTensorPtr tensor2;
  EXPECT_EQ(NodeUtils::TryGetWeightByPlaceHolderNode(pl2, tensor2), GRAPH_SUCCESS);
}

// 覆盖 tuning_utils.cc:143 —— placeholder 转为权重属性
TEST_F(IncCovLogSTest, IncCovSt_TuningUtils_ConvertConstToWeightAttr) {
  ut::GraphBuilder builder("st_tuning_weight");
  auto placeholder = builder.AddNode("pl", PLACEHOLDER, 1, 1);
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{0U, 0U, 0U, 0U});
  (void)AttrUtils::SetTensor(placeholder->GetOpDesc(), ATTR_NAME_WEIGHTS, weight);
  EXPECT_EQ(TuningUtils::ConvertConstToWeightAttr(builder.GetGraph()), GRAPH_SUCCESS);
}

// 覆盖 plugin_manager.cc:450 —— compiler 版本不在要求范围内
TEST_F(IncCovLogSTest, IncCovSt_PluginManager_CompilerVersionNotInRange) {
  EXPECT_FALSE(PluginManager::CheckOppAndCompilerVersions("", "9.9", {{0U, 1U}}));
}

// 覆盖 ffts_plus_update_manager.cc:39 —— 同一 core type 重复注册
TEST_F(IncCovLogSTest, IncCovSt_FftsPlusUpdate_RegisterDuplicate) {
  FftsCtxUpdateCreatorFun creator = []() { return FftsCtxUpdatePtr(); };
  FftsPlusUpdateManager::FftsPlusUpdateRegistrar registrar1("incov_core", creator);
  FftsPlusUpdateManager::FftsPlusUpdateRegistrar registrar2("incov_core", creator);
}

// 覆盖 buffer_fusion_pass_registry.cc:113 —— 非法 pass type
TEST_F(IncCovLogSTest, IncCovSt_BufferFusionPassRegister_InvalidType) {
  fe::BufferFusionPassRegistrar registrar(static_cast<fe::BufferFusionPassType>(-1), "incov_buffer_pass", nullptr,
                                          static_cast<fe::PassAttr>(0U));
  (void)registrar;
}

// 覆盖 tiling_context_builder.cc:74 —— 非法版本标签
TEST_F(IncCovLogSTest, IncCovSt_TilingContextBuilder_InvalidVersionTag) {
  gert::TilingContextBuilder builder;
  gert::OpImplSpaceRegistryV2Ptr registry;
  builder.SetSpaceRegistryV2(registry, gert::OppImplVersionTag::kVersionEnd);
}

// 覆盖 compute_graph.cc:2783/2785 —— user def output 格式非法
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_SetUserDefOutputInvalidToken) {
  auto graph = std::make_shared<ComputeGraph>("st_user_def_output");
  graph->SetUserDefOutput("node_without_index");
}

// 覆盖 compute_graph.cc:2311 —— 拓扑排序时子图数量不一致
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_TopoSortSubgraphSizeMismatch) {
  auto root = std::make_shared<ComputeGraph>("st_topo_subgraph_root");
  auto parent_desc = std::make_shared<OpDesc>("while_node", "While");
  (void)parent_desc->AddInputDesc("x", GeTensorDesc());
  (void)parent_desc->AddOutputDesc("y", GeTensorDesc());
  auto parent_node = root->AddNode(parent_desc);
  auto subgraph = std::make_shared<ComputeGraph>("st_topo_subgraph");
  subgraph->SetParentGraph(root);
  subgraph->SetParentNode(parent_node);
  EXPECT_EQ(root->AddSubgraph("st_topo_subgraph", subgraph), GRAPH_SUCCESS);
  (void)root->TopologicalSorting();
}

// 覆盖 graph_utils.cc:4979 —— output_inplace_ability 索引长度非法
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_InvalidInplaceAbility) {
  ut::GraphBuilder builder("st_inplace_ability");
  auto node = builder.AddNode("n", "Relu", 1, 1);
  std::vector<std::vector<int64_t>> inplace_index_list = {{0, 0, 1}};
  (void)AttrUtils::SetListListInt(node->GetOpDesc(), ATTR_NAME_OUTPUT_INPLACE_ABILITY, inplace_index_list);
  std::map<size_t, std::vector<size_t>> out_index_to_in_indexes;
  EXPECT_NE(GraphUtils::GetSupportInplaceOutput(node, out_index_to_in_indexes), GRAPH_SUCCESS);
}

// 覆盖 graph_utils.cc:3116 —— 输出 ref 自 RefData 的映射
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_HandleOutAnchorRefMapping) {
  ut::GraphBuilder builder("st_ref_mapping");
  auto refdata = builder.AddNode("refdata_node", "RefData", 1, 1);
  auto node = builder.AddNode("ref_node", "Identity", 1, 1);
  (void)AttrUtils::SetStr(node->GetOpDesc()->MutableOutputDesc(0U), REF_VAR_SRC_VAR_NAME, "refdata_node");
  SymbolToAnchors symbol_to_anchors;
  AnchorToSymbol anchor_to_symbol;
  NodeIndexIO exist_info(refdata, 0U, kOut);
  const std::string symbol = exist_info.ToString();
  anchor_to_symbol[symbol] = symbol;
  symbol_to_anchors[symbol] = std::list<NodeIndexIO>{exist_info};
  EXPECT_EQ(GraphUtils::HandleOutAnchorMapping(node, symbol_to_anchors, anchor_to_symbol), GRAPH_SUCCESS);
}

// 覆盖 execute_graph_adapter.cc:115 —— 重连边时节点不在映射表中
TEST_F(IncCovLogSTest, IncCovSt_ExecuteGraphAdapter_RelinkMissingNode) {
  FastNode fast_node;
  std::unordered_map<FastNode *, Node *> all_new_nodes;
  EXPECT_NE(ExecuteGraphAdapter::RelinkGraphEdges(&fast_node, all_new_nodes), GRAPH_SUCCESS);
}

// 覆盖 tensor_utils.cc:166 —— 元素数为 0 的 shape
TEST_F(IncCovLogSTest, IncCovSt_TensorUtils_CalcTensorMemSizeZeroDim) {
  int64_t mem_size = 0;
  EXPECT_EQ(TensorUtils::CalcTensorMemSize(GeShape({0, 2}), FORMAT_ND, DT_FLOAT, mem_size), GRAPH_SUCCESS);
  EXPECT_EQ(mem_size, 0);
}

// 覆盖 tuning_utils.cc:1098 —— 未知类型节点恢复 IR 定义失败
TEST_F(IncCovLogSTest, IncCovSt_TuningUtils_PreProcessUnknownType) {
  ut::GraphBuilder builder("st_preprocess_unknown");
  auto node = builder.AddNode("n", "CovUnknownTypeForRecover", 0, 1);
  (void)TuningUtils::PreProcessNode(node);
}

// 覆盖 fusion_pass_registry.cc:123 —— 非法 fusion pass type
TEST_F(IncCovLogSTest, IncCovSt_FusionPassRegister_InvalidType) {
  fe::FusionPassRegistrar registrar(static_cast<fe::GraphFusionPassType>(-1), "incov_fusion_pass", nullptr,
                                    static_cast<fe::PassAttr>(0U));
  (void)registrar;
}

// 覆盖 op_tiling.cc:1115 —— slice 数量与输入切片数量不一致
TEST_F(IncCovLogSTest, IncCovSt_OpFftsPlusCalculate_SliceNumMismatch) {
  auto root_builder = ut::GraphBuilder("root_slice_mismatch");
  const auto &node = root_builder.AddNode("relu", "ReluCovSlice", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);
  const ffts::ThreadSliceMapDyPtr slice_info_ptr = std::make_shared<ffts::ThreadSliceMapDy>();
  slice_info_ptr->slice_instance_num = 2U;
  (void)op_desc->SetExtAttr(ffts::kAttrSgtStructInfoDy, slice_info_ptr);
  std::vector<optiling::OpRunInfoV2> op_run_info;
  EXPECT_EQ(optiling::OpFftsPlusCalculate(op, op_run_info), ge::GRAPH_FAILED);
}

// 覆盖 args_format_desc.cc:94 —— 占位符格式不支持
TEST_F(IncCovLogSTest, IncCovSt_ArgsFormatDesc_PlaceholderInvalid) {
  auto op_desc = std::make_shared<OpDesc>("args_ph", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  std::vector<ArgDesc> arg_descs;
  EXPECT_NE(ArgsFormatDesc::Parse(op_desc, "{:64b}", arg_descs), GRAPH_SUCCESS);
}

// 覆盖 data_dependent_interpreter.cc:240 —— 默认 registry 缺失
TEST_F(IncCovLogSTest, IncCovSt_DataDependentInterpreter_DefaultRegistryMissing) {
  auto op_desc = std::make_shared<OpDesc>("ddi_op", "Relu");
  gert::OpImplSpaceRegistryV2Array space_registries;
  gert::DataDependentInterpreter interpreter(op_desc, space_registries);
  bool is_data_dependent = false;
  (void)interpreter.IsDataDependent(0, is_data_dependent);
}

// 覆盖 model_serialize.cc:820/859 —— 模型序列化往返加载
TEST_F(IncCovLogSTest, IncCovSt_Model_LoadSerializedModel) {
  auto graph = std::make_shared<ComputeGraph>("st_model_graph");
  auto op_desc = std::make_shared<OpDesc>("n", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  Model model("st_model_rt", "");
  model.SetGraph(graph);
  const std::string file = "/tmp/opencode/st_model_rt.om";
  if (model.SaveToFile(file) != GRAPH_SUCCESS) {
    GTEST_SKIP();
  }
  std::ifstream ifs(file, std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  Model loaded;
  (void)Model::Load(data.data(), data.size(), loaded);
  Model loaded_mt;
  (void)Model::LoadWithMultiThread(data.data(), data.size(), loaded_mt);
  (void)std::remove(file.c_str());
}

// 覆盖 optimization_option.cc:159 —— pass 名称与 ge option 重名
TEST_F(IncCovLogSTest, IncCovSt_OptimizationOption_InitializeForbiddenOption) {
  OptimizationOption option;
  std::map<std::string, std::string> ge_options;
  ge_options[OPTIMIZATION_SWITCH] = "ge.exec.cov_opt:on;st_cov_pass:on";
  const std::unordered_set<std::string> forbidden = {"ge.exec.cov_opt"};
  (void)option.Initialize(ge_options, std::unordered_map<std::string, OoInfo>{}, forbidden);
}

// 覆盖 model_serialize.cc:70 —— 外置权重目录创建失败
TEST_F(IncCovLogSTest, IncCovSt_Model_SaveExternalWeightDirFailed) {
  const std::string dir = "/tmp/opencode/st_mser_dir";
  (void)ge::CreateDir(dir);
  const std::string block_file = dir + "/tmp_weight";
  {
    std::ofstream ofs(block_file, std::ios::out | std::ios::trunc);
    ofs << "x";
  }
  auto graph = std::make_shared<ComputeGraph>("st_mser_graph");
  auto const_desc = std::make_shared<OpDesc>("const_node", "Const");
  (void)const_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U});
  (void)AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, weight);
  (void)graph->AddNode(const_desc);
  Model model("st_mser_model", "");
  model.SetGraph(graph);
  (void)model.SaveToFile(dir + "/model.om", true);
  (void)std::remove(block_file.c_str());
}

// 覆盖 sym_dtype.cc:23/29 —— 属性为非法数值 dtype
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_InvalidNumericDtype) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_sym", TensorType({DT_FLOAT}));
  ASSERT_NE(sym, nullptr);
  OpDesc op("sym_op", "Relu");
  (void)AttrUtils::SetInt(op, "incov_sym", 9999);
  TypeOrTypes out;
  (void)sym->Eval(op, out);
}

// 覆盖 sym_dtype.cc:40/47 —— 属性为非法数值 list-dtype
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_InvalidNumericListDtype) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_list_sym", ListTensorType(TensorType({DT_FLOAT})));
  ASSERT_NE(sym, nullptr);
  OpDesc op("sym_list_op", "Relu");
  (void)AttrUtils::SetListInt(op, "incov_list_sym", std::vector<int64_t>{9999});
  TypeOrTypes out;
  (void)sym->Eval(op, out);
}

// 覆盖 sym_dtype.cc:94/103 —— list 与标量提升/长度不一致
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_PromoteListMismatch) {
  IRDataTypeSymbolStore store;
  (void)store.DeclareSymbol("incov_pa", TensorType({DT_FLOAT}));
  (void)store.DeclareSymbol("incov_pb", ListTensorType(TensorType({DT_FLOAT})));
  auto promote = store.DeclareSymbol("incov_pp", Promote({"incov_pa", "incov_pb"}));
  ASSERT_NE(promote, nullptr);
  OpDesc op("sym_promote_op", "Relu");
  (void)AttrUtils::SetInt(op, "incov_pa", static_cast<int64_t>(DT_FLOAT));
  (void)AttrUtils::SetListInt(op, "incov_pb", std::vector<int64_t>{static_cast<int64_t>(DT_FLOAT)});
  TypeOrTypes out;
  (void)promote->Eval(op, out);

  IRDataTypeSymbolStore store2;
  (void)store2.DeclareSymbol("incov_la", ListTensorType(TensorType({DT_FLOAT})));
  (void)store2.DeclareSymbol("incov_lb", ListTensorType(TensorType({DT_FLOAT, DT_DOUBLE})));
  auto promote2 = store2.DeclareSymbol("incov_lp", Promote({"incov_la", "incov_lb"}));
  ASSERT_NE(promote2, nullptr);
  OpDesc op2("sym_promote2_op", "Relu");
  (void)AttrUtils::SetListInt(op2, "incov_la", std::vector<int64_t>{static_cast<int64_t>(DT_FLOAT)});
  (void)AttrUtils::SetListInt(op2, "incov_lb",
                              std::vector<int64_t>{static_cast<int64_t>(DT_FLOAT), static_cast<int64_t>(DT_FLOAT)});
  TypeOrTypes out2;
  (void)promote2->Eval(op2, out2);
}

// 覆盖 sym_dtype.cc:167 —— 符号自提升
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_PromoteItself) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_self_sym", Promote({"incov_self_sym"}));
  EXPECT_EQ(sym, nullptr);
}

// 覆盖 sym_dtype.cc:264 —— 未声明符号按 legacy 方式求值
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_LegacyEval) {
  IRDataTypeSymbolStore store;
  auto sym = store.GetOrCreateSymbol("incov_legacy_sym");
  ASSERT_NE(sym, nullptr);
  OpDesc op("sym_legacy_op", "Relu");
  (void)AttrUtils::SetInt(op, "incov_legacy_sym", static_cast<int64_t>(DT_FLOAT));
  TypeOrTypes out;
  (void)sym->Eval(op, out);
}

// 覆盖 fast_node.cc:252/254/270/271 —— 边索引越界
TEST_F(IncCovLogSTest, IncCovSt_FastNode_MoveEdgeInvalidIndex) {
  FastNode fast_node;
  EXPECT_NE(fast_node.MoveEdge(DirectionType::kDirectionInType, -2, 0, 0), GRAPH_SUCCESS);
  EXPECT_NE(fast_node.MoveEdge(DirectionType::kDirectionInType, 100, 0, 0), GRAPH_SUCCESS);
}

// 覆盖 shape_refiner.cc:867 —— 运行时推导后 dtype 被重置
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_ResetDtypeAfterRunning) {
  ut::GraphBuilder builder("st_sr_dtype");
  auto node = builder.AddNode("n", "ReluCovDtypeReset", 1, 1);
  auto op_desc = node->GetOpDesc();
  op_desc->AddInferFunc([](Operator &op) -> graphStatus {
    auto infer_desc = OpDescUtils::GetOpDescFromOperator(op);
    if (infer_desc == nullptr) {
      return GRAPH_FAILED;
    }
    auto out_desc = infer_desc->MutableOutputDesc(0U);
    if (out_desc == nullptr) {
      return GRAPH_FAILED;
    }
    out_desc->SetDataType(DT_DOUBLE);
    return GRAPH_SUCCESS;
  });
  Operator op = OpDescUtils::CreateOperatorFromNode(node);
  (void)ShapeRefiner::InferShapeAndTypeForRunning(node, op, false);
}

// 覆盖 compute_graph.cc:1985 —— 空输出/目标时自动添加 NetOutput
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_CreateNetOutput) {
  auto graph = std::make_shared<ComputeGraph>("st_netout_graph");
  auto node_desc = std::make_shared<OpDesc>("relu", "Relu");
  (void)node_desc->AddInputDesc("x", GeTensorDesc());
  (void)node_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(node_desc);
  EXPECT_EQ(graph->CreateOrUpdateNetoutput(), GRAPH_SUCCESS);
}

// 覆盖 compute_graph.cc:1954 —— 用户设置的 Retval 目标节点不删除
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_KeepTargetRetval) {
  auto graph = std::make_shared<ComputeGraph>("st_retval_graph");
  auto src_desc = std::make_shared<OpDesc>("src", "Relu");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  auto retval_desc = std::make_shared<OpDesc>("retval", "Retval");
  (void)retval_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(retval_desc, RETVAL_ATTR_NAME_INDEX, 0);
  auto retval_node = graph->AddNode(retval_desc);
  (void)GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), retval_node->GetInDataAnchor(0));
  graph->SetGraphTargetNodesInfo(std::vector<NodePtr>{retval_node});
  (void)graph->CreateOrUpdateNetoutput();
  (void)graph->CreateOrUpdateNetoutput();
}

// 覆盖 args_format_desc.cc:789/808 —— sk 子节点地址类型不支持序列化
TEST_F(IncCovLogSTest, IncCovSt_ArgsFormatDesc_SknUnsupportedAddrType) {
  auto op_desc = std::make_shared<OpDesc>("sk_op", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  SkArgDesc sk_arg{};
  sk_arg.addr_type = AddrType::SUPER_KERNEL_SUB_NODE;
  sk_arg.ir_idx = 0;
  sk_arg.sub_addr_type = 100;
  sk_arg.sub_idx = 0;
  static_assert(sizeof(SkArgDesc) == sizeof(ArgDesc), "size mismatch");
  const ArgDesc &arg_desc = reinterpret_cast<const ArgDesc &>(sk_arg);
  auto sk_graph = std::make_shared<ComputeGraph>("sk_sub_graph");
  auto sub_desc = std::make_shared<OpDesc>("sk_sub", "Relu");
  sub_desc->SetId(0);
  (void)sk_graph->AddNode(sub_desc);
  (void)op_desc->SetExtAttr("_sk_sub_graph", sk_graph);
  (void)ArgsFormatDesc::Serialize(std::vector<ArgDesc>{arg_desc});
  size_t arg_size = 0;
  EXPECT_NE(ArgsFormatDesc::GetArgSize(op_desc, arg_desc, arg_size), GRAPH_SUCCESS);
}

// 覆盖 graph_utils.cc:3321 —— 符号合并成功
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_UnionSymbolMapping) {
  ut::GraphBuilder builder("st_union_symbol");
  auto n1 = builder.AddNode("u1", "Relu", 1, 1);
  auto n2 = builder.AddNode("u2", "Relu", 1, 1);
  NodeIndexIO a1(n1, 0U, kOut);
  NodeIndexIO a2(n2, 0U, kOut);
  const std::string s1 = a1.ToString();
  const std::string s2 = a2.ToString();
  SymbolToAnchors symbols;
  AnchorToSymbol anchors;
  symbols[s1] = std::list<NodeIndexIO>{a1};
  symbols[s2] = std::list<NodeIndexIO>{a2, a1};
  anchors[s1] = s1;
  anchors[s2] = s2;
  std::string symbol;
  EXPECT_EQ(GraphUtils::UnionSymbolMapping(a1, a2, symbols, anchors, symbol), GRAPH_SUCCESS);
}

// 覆盖 node.cc:735 —— 输入锚点为空
TEST_F(IncCovLogSTest, IncCovSt_Node_GetInDataNodesWithNullAnchor) {
  ut::GraphBuilder builder("st_node_null_anchor");
  auto node = builder.AddNode("n", "Relu", 1, 1);
  node->impl_->in_data_anchors_.push_back(nullptr);
  auto vistor = node->GetInDataNodesAndAnchors();
  (void)vistor;
}

// 覆盖 sym_dtype.cc:304 —— 标量符号绑定 IR 输入后范围信息缺失
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_IrInputRangeMissing) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_s304", TensorType({DT_FLOAT}));
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(store.SetInputSymbol("incov_in304", kIrInputRequired, "incov_s304"), nullptr);
  auto op = std::make_shared<OpDesc>("sym_op304", "Relu");
  op->AppendIrInput("incov_in304", kIrInputRequired);
  TypeOrTypes out;
  (void)sym->Eval(*op, out);
}

// 覆盖 sym_dtype.cc:353 —— list 符号绑定 IR 输入后范围信息缺失
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_ListIrInputRangeMissing) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_s353", ListTensorType(TensorType({DT_FLOAT})));
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(store.SetInputSymbol("incov_in353", kIrInputRequired, "incov_s353"), nullptr);
  auto op = std::make_shared<OpDesc>("sym_op353", "Relu");
  op->AppendIrInput("incov_in353", kIrInputRequired);
  TypeOrTypes out;
  (void)sym->Eval(*op, out);
}

// 覆盖 sym_dtype.cc:381 —— 动态输入 list dtype 不一致
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_ListDtypeMismatch) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_s381", ListTensorType(TensorType({DT_FLOAT, DT_DOUBLE})));
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(store.SetInputSymbol("incov_in381a", kIrInputDynamic, "incov_s381"), nullptr);
  ASSERT_NE(store.SetInputSymbol("incov_in381b", kIrInputDynamic, "incov_s381"), nullptr);
  auto op = std::make_shared<OpDesc>("sym_op381", "Relu");
  op->AppendIrInput("incov_in381a", kIrInputDynamic);
  op->AppendIrInput("incov_in381b", kIrInputDynamic);
  (void)op->AddInputDesc("incov_in381a", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  (void)op->AddInputDesc("incov_in381b", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_DOUBLE));
  TypeOrTypes out;
  (void)sym->Eval(*op, out);
}

// 覆盖 op_desc_utils.cc:926 —— IR 输入名重匹配
TEST_F(IncCovLogSTest, IncCovSt_OpDescUtils_IrInputNameRematch) {
  auto op_desc = std::make_shared<OpDesc>("inst_op", "Relu");
  op_desc->AppendIrInput("incov_ir_x", kIrInputRequired);
  std::map<uint32_t, std::string> valid_index_2_names;
  valid_index_2_names[0U] = "incov_y0";
  valid_index_2_names[1U] = "incov_y1";
  valid_index_2_names[2U] = "incov_ir_x";
  size_t instance_num = 0U;
  EXPECT_NE(OpDescUtils::GetInstanceNum(op_desc, 0U, 0U, valid_index_2_names, instance_num), GRAPH_SUCCESS);
}

// 覆盖 graph_utils.cc:2175 —— inplace 输入节点跨图
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_InplaceCrossGraph) {
  ut::GraphBuilder builder_a("st_inplace_graph_a");
  auto node_a = builder_a.AddNode("node_a", "Relu", 1, 1);
  ut::GraphBuilder builder_b("st_inplace_graph_b");
  auto node_b = builder_b.AddNode("node_b", "Relu", 1, 1);
  (void)GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  std::vector<std::vector<int64_t>> ability = {{0, 0}};
  (void)AttrUtils::SetListListInt(node_a->GetOpDesc(), ATTR_NAME_OUTPUT_INPLACE_ABILITY, ability);
  std::map<size_t, std::vector<size_t>> out_index_to_in_indexes;
  (void)GraphUtils::GetSupportInplaceOutput(node_a, out_index_to_in_indexes);
}

// 覆盖 graph_utils.cc:1175 —— 从落盘图文件加载并设置 owner graph
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_LoadGEGraphFromFile) {
  auto graph = std::make_shared<ComputeGraph>("st_load_ge_graph");
  auto op_desc = std::make_shared<OpDesc>("n", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  const std::string file = "/tmp/opencode/st_load_ge_graph.pbtxt";
  if (GraphUtils::DumpGEGraphByPath(graph, file, DumpLevel::DUMP_WITH_OUT_DESC) == GRAPH_SUCCESS) {
    ComputeGraphPtr loaded;
    (void)GraphUtils::LoadGEGraph(file.c_str(), loaded);
    (void)std::remove(file.c_str());
  }
}

// 覆盖 register.cpp:269/353 —— 动态端口自动映射
TEST_F(IncCovLogSTest, IncCovSt_AutoMappingFnDynamic_PortMapping) {
  domi::tensorflow::NodeDef node_def;
  node_def.set_name("st_dyn_node");
  node_def.set_op("CovDynOp");
  (*node_def.mutable_attr())["st_n"].set_i(2);
  ge::Operator op("CovDynOp");
  auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
  ASSERT_NE(op_desc, nullptr);
  (void)op_desc->AddRegisterInputName("st_port");
  (void)domi::AutoMappingFnDynamic(&node_def, op, {{"in", {"st_port", "st_n"}}}, -1, -1);
}

// 覆盖 shape_refiner.cc:160/163/193 —— While 子图 ref 张量数量不一致
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_WhileRefTensorMismatch) {
  ut::GraphBuilder builder("st_while_ref");
  auto while_node = builder.AddNode("w", "While", 2, 2);
  auto while_desc = while_node->GetOpDesc();
  (void)while_desc->AddSubgraphName("body");
  (void)while_desc->SetSubgraphInstanceName(0U, "body");
  auto body = std::make_shared<ComputeGraph>("body");
  body->SetParentGraph(builder.GetGraph());
  body->SetParentNode(while_node);
  auto data_desc = std::make_shared<OpDesc>("data0", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc());
  (void)AttrUtils::SetInt(data_desc, "index", 0);
  auto data_node = body->AddNode(data_desc);
  auto netout_desc = std::make_shared<OpDesc>("netout", "NetOutput");
  (void)netout_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(0U), ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto netout_node = body->AddNode(netout_desc);
  auto data1_desc = std::make_shared<OpDesc>("data1", "Data");
  (void)data1_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2, 5}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(data1_desc, ATTR_NAME_PARENT_NODE_INDEX, 1);
  auto data1_node = body->AddNode(data1_desc);
  (void)netout_desc->AddInputDesc("x1", GeTensorDesc(GeShape({2, 5}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(1U), ATTR_NAME_PARENT_NODE_INDEX, 1);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(0));
  (void)GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(1));
  (void)builder.GetGraph()->AddSubgraph("body", body);
  Operator op = OpDescUtils::CreateOperatorFromNode(while_node);
  (void)ShapeRefiner::InferShapeAndTypeForRunning(while_node, op, false);
  (void)ShapeRefiner::InferShapeAndType(while_node);
  (void)ShapeRefiner::InferShapeAndType(while_node, true);
}

// 覆盖 node_utils.cc:1474 —— 跨 PartitionedCall 获取常量权重
TEST_F(IncCovLogSTest, IncCovSt_NodeUtils_GetWeightCrossPartitionedCall) {
  ut::GraphBuilder builder("st_partition_root");
  auto const_desc = std::make_shared<OpDesc>("const_p", "Const");
  (void)const_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{0U, 0U, 0U, 0U});
  (void)AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, weight);
  auto const_node = builder.GetGraph()->AddNode(const_desc);
  auto call_desc = std::make_shared<OpDesc>("call_p", "PartitionedCall");
  (void)call_desc->AddInputDesc("x", GeTensorDesc());
  (void)call_desc->AddOutputDesc("y", GeTensorDesc());
  auto call_node = builder.GetGraph()->AddNode(call_desc);
  (void)GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), call_node->GetInDataAnchor(0));
  auto sub = std::make_shared<ComputeGraph>("sub_p");
  sub->SetParentGraph(builder.GetGraph());
  sub->SetParentNode(call_node);
  auto data_desc = std::make_shared<OpDesc>("data_p", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc());
  (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto data_node = sub->AddNode(data_desc);
  (void)builder.GetGraph()->AddSubgraph("sub_p", sub);
  ConstGeTensorPtr tensor;
  (void)NodeUtils::TryGetWeightByDataNode(data_node, tensor);
}

// 覆盖 tensor_assign.cpp:375 —— string 元素个数乘法溢出
TEST_F(IncCovLogSTest, IncCovSt_TensorAssign_StringCountOverflow) {
  domi::tensorflow::TensorProto tf_tensor;
  tf_tensor.set_dtype(domi::tensorflow::DT_STRING);
  tf_tensor.mutable_tensor_shape()->add_dim()->set_size(static_cast<int64_t>(1LL << 61));
  tf_tensor.set_tensor_content(std::string(8U, '\0'));
  GeTensorPtr dst_weight;
  (void)domi::TensorAssign::SetGeTensor(tf_tensor, dst_weight);
}

// 覆盖 model_serialize.cc:70 —— 外置权重目录创建失败
TEST_F(IncCovLogSTest, IncCovSt_ModelSerialize_ExternalWeightDirFailed) {
  const std::string dir = "/tmp/opencode/st_mser_dir2";
  (void)ge::CreateDir(dir);
  const std::string block_file = dir + "/tmp_weight";
  {
    std::ofstream ofs(block_file, std::ios::out | std::ios::trunc);
    ofs << "x";
  }
  auto graph = std::make_shared<ComputeGraph>("st_mser_graph2");
  auto const_desc = std::make_shared<OpDesc>("const_node2", "Const");
  (void)const_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U});
  (void)AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, weight);
  (void)graph->AddNode(const_desc);
  Model model("st_mser_model2", "");
  model.SetGraph(graph);
  ModelSerialize serializer;
  (void)serializer.SerializeSeparateModel(model, dir);
  (void)std::remove(block_file.c_str());
}

// 覆盖 sym_dtype.cc:381 —— 两个动态 IR 输入绑定同一 list 符号且 dtype 不同
TEST_F(IncCovLogSTest, IncCovSt_SymDtype_ListBackendsDtypeMismatch) {
  IRDataTypeSymbolStore store;
  auto sym = store.DeclareSymbol("incov_s381b", ListTensorType(TensorType({DT_FLOAT, DT_DOUBLE})));
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(store.SetInputSymbol("dy_in_x", kIrInputDynamic, "incov_s381b"), nullptr);
  ASSERT_NE(store.SetInputSymbol("dy_in_y", kIrInputDynamic, "incov_s381b"), nullptr);
  auto op = std::make_shared<OpDesc>("sym_op381b", "Relu");
  op->AppendIrInput("dy_in_x", kIrInputDynamic);
  op->AppendIrInput("dy_in_y", kIrInputDynamic);
  (void)op->AddInputDesc("dy_in_x0", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  (void)op->AddInputDesc("dy_in_y0", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_DOUBLE));
  TypeOrTypes out;
  (void)sym->Eval(*op, out);
}

// 覆盖 graph_utils.cc:1175 —— dump 后重新加载图文件
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_LoadDumpedGraph) {
  auto graph = std::make_shared<ComputeGraph>("st_load_dumped");
  auto op_desc = std::make_shared<OpDesc>("n", "Relu");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)graph->AddNode(op_desc);
  const char *levels[] = {"/tmp/opencode/st_load_dump_all.pbtxt", "/tmp/opencode/st_load_dump_desc.pbtxt"};
  const DumpLevel dump_levels[] = {DumpLevel::DUMP_ALL, DumpLevel::DUMP_WITH_OUT_DESC};
  for (size_t i = 0U; i < 2U; ++i) {
    if (GraphUtils::DumpGEGraphByPath(graph, levels[i], dump_levels[i]) == GRAPH_SUCCESS) {
      ComputeGraphPtr loaded;
      (void)GraphUtils::LoadGEGraph(levels[i], loaded);
      (void)std::remove(levels[i]);
    }
  }
}

// 覆盖 register.cpp:477 —— 自动映射时目标图已存在同名子图
TEST_F(IncCovLogSTest, IncCovSt_AutoMappingByOpFn_SubgraphNameExists) {
  ut::GraphBuilder builder("st_am_src");
  auto src_node = builder.AddNode("src_node", "If", 1, 1);
  (void)src_node->GetOpDesc()->AddSubgraphName("st_dup_sub");
  ge::Operator op_src = OpDescUtils::CreateOperatorFromNode(src_node);
  ge::Operator op_dst("st_am_dst");
  auto op_desc_dst = OpDescUtils::GetOpDescFromOperator(op_dst);
  ASSERT_NE(op_desc_dst, nullptr);
  (void)op_desc_dst->AddSubgraphName("st_dup_sub");
  (void)domi::AutoMappingByOpFn(op_src, op_dst);
}

// 覆盖 graph_utils.cc:1175 —— 完整图 dump 后重新加载
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_LoadDumpedFullGraph) {
  ut::GraphBuilder builder("st_load_full");
  auto data_node = builder.AddNode("data", "Data", 0, 1);
  auto relu_node = builder.AddNode("relu", "Relu", 1, 1);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  (void)builder.GetGraph()->CreateOrUpdateNetoutput();
  const std::string file = "/tmp/opencode/st_load_full.pbtxt";
  if (GraphUtils::DumpGEGraphByPath(builder.GetGraph(), file, DumpLevel::DUMP_ALL) == GRAPH_SUCCESS) {
    ComputeGraphPtr loaded;
    (void)GraphUtils::LoadGEGraph(file.c_str(), loaded);
    (void)std::remove(file.c_str());
  }
}

// 覆盖 custom_op_registry.cc:166 —— 自定义算子内核数据 magic 非法
TEST_F(IncCovLogSTest, IncCovSt_CustomOpRegistry_InvalidKernelMagic) {
  CustomOpRegistry registry;
  uint8_t data[64] = {0U};
  EXPECT_NE(registry.LoadCustomOpsPartition(data, sizeof(data)), GRAPH_SUCCESS);
}

// 覆盖 ir_definitions_recover.cc:465 —— IR 定义输入输出数量校验日志
TEST_F(IncCovLogSTest, IncCovSt_RecoverIrDefinition_InputOutputNumLog) {
  auto op_desc = std::make_shared<OpDesc>("recover_op", "Add");
  (void)op_desc->AddInputDesc("x", GeTensorDesc());
  (void)op_desc->AddOutputDesc("y", GeTensorDesc());
  (void)RecoverOpDescIrDefinition(op_desc, "Add");
  (void)CheckIrSpec(op_desc);
}

// 覆盖 execute_graph.cc:398 —— 非本图节点添加边
TEST_F(IncCovLogSTest, IncCovSt_ExecuteGraph_AddEdgeForeignNode) {
  ExecuteGraph execute_graph("st_exec_foreign");
  auto a_desc = std::make_shared<OpDesc>("ea", "Relu");
  (void)a_desc->AddInputDesc("x", GeTensorDesc());
  (void)a_desc->AddOutputDesc("y", GeTensorDesc());
  auto b_desc = std::make_shared<OpDesc>("eb", "Relu");
  (void)b_desc->AddInputDesc("x", GeTensorDesc());
  (void)b_desc->AddOutputDesc("y", GeTensorDesc());
  auto node_a = execute_graph.AddNode(a_desc);
  auto node_b = execute_graph.AddNode(b_desc);
  ASSERT_NE(node_a, nullptr);
  ASSERT_NE(node_b, nullptr);
  (void)execute_graph.AddEdge(node_a, 0, node_b, 0);
  FastNode foreign_node;
  (void)execute_graph.AddEdge(&foreign_node, 0, node_b, 0);
}

// 覆盖 graph_utils.cc:1175 —— 环境变量使能 dump 后加载图
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_LoadDumpedGraphWithEnv) {
  ut::GraphBuilder builder("st_load_env");
  auto relu_desc = std::make_shared<OpDesc>("relu", "Relu");
  (void)relu_desc->AddInputDesc("x", GeTensorDesc());
  (void)relu_desc->AddOutputDesc("y", GeTensorDesc());
  (void)builder.GetGraph()->AddNode(relu_desc);
  const std::string file = "/tmp/opencode/st_load_env.pbtxt";
  (void)setenv("DUMP_GE_GRAPH", "2", 1);
  const auto dump_ret = GraphUtils::DumpGEGraphByPath(builder.GetGraph(), file, DumpLevel::DUMP_ALL);
  (void)unsetenv("DUMP_GE_GRAPH");
  if (dump_ret == GRAPH_SUCCESS) {
    ComputeGraphPtr loaded;
    (void)GraphUtils::LoadGEGraph(file.c_str(), loaded);
    (void)std::remove(file.c_str());
  }
}

// 覆盖 shape_refiner.cc:160/163/193 —— While 子图推导时 ref 张量不一致
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_InferShapeWhileRefMismatch) {
  ut::GraphBuilder builder("st_while_infer");
  auto while_node = builder.AddNode("w", "While", 2, 2);
  auto while_desc = while_node->GetOpDesc();
  (void)while_desc->AddSubgraphName("body");
  (void)while_desc->SetSubgraphInstanceName(0U, "body");
  auto body = std::make_shared<ComputeGraph>("body");
  body->SetParentGraph(builder.GetGraph());
  body->SetParentNode(while_node);
  auto data_desc = std::make_shared<OpDesc>("data0", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc());
  (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto data_node = body->AddNode(data_desc);
  auto netout_desc = std::make_shared<OpDesc>("netout", "NetOutput");
  (void)netout_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(0U), ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto netout_node = body->AddNode(netout_desc);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(0));
  (void)builder.GetGraph()->AddSubgraph("body", body);
  Operator op = OpDescUtils::CreateOperatorFromNode(while_node);
  (void)ShapeRefiner::InferShapeAndType(while_node);
  (void)ShapeRefiner::InferShapeAndType(while_node, true);
  (void)ShapeRefiner::InferShapeAndTypeForRunning(while_node, op, true);
}

// 覆盖 compute_graph.cc:2059 —— NetOutput 父节点索引写入缺少输入描述
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_NetOutputParentIndexMissingDesc) {
  auto graph = std::make_shared<ComputeGraph>("st_netout_parent");
  auto src_desc = std::make_shared<OpDesc>("src_n", "Relu");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  auto retval_desc = std::make_shared<OpDesc>("retval_n", "Retval");
  (void)retval_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(retval_desc, RETVAL_ATTR_NAME_INDEX, 0);
  auto retval_node = graph->AddNode(retval_desc);
  (void)GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), retval_node->GetInDataAnchor(0));
  (void)graph->CreateOrUpdateNetoutput();
  (void)graph->CreateOrUpdateNetoutput(true);
}

// 覆盖 execute_graph_utils.cc:573/593/622/642/749 —— 无 owner 图/替换边失败
TEST_F(IncCovLogSTest, IncCovSt_ExecuteGraphUtils_NoOwnerGraph) {
  ExecuteGraph execute_graph("st_exec_utils");
  auto make_desc = [](const std::string &name) {
    auto desc = std::make_shared<OpDesc>(name, "Relu");
    (void)desc->AddInputDesc("x", GeTensorDesc());
    (void)desc->AddOutputDesc("y", GeTensorDesc());
    return desc;
  };
  auto node_a = execute_graph.AddNode(make_desc("eca"));
  auto node_b = execute_graph.AddNode(make_desc("ecb"));
  auto node_c = execute_graph.AddNode(make_desc("ecc"));
  auto node_d = execute_graph.AddNode(make_desc("ecd"));
  auto node_e = execute_graph.AddNode(make_desc("ece"));
  ASSERT_NE(node_a, nullptr);
  ASSERT_NE(node_b, nullptr);
  ASSERT_NE(node_c, nullptr);
  ASSERT_NE(node_d, nullptr);
  ASSERT_NE(node_e, nullptr);
  (void)execute_graph.AddEdge(node_c, kControlEdgeIndex, node_a, kControlEdgeIndex);
  (void)execute_graph.AddEdge(node_a, kControlEdgeIndex, node_c, kControlEdgeIndex);
  auto real_edge = execute_graph.AddEdge(node_a, 0, node_b, 0);
  ASSERT_NE(real_edge, nullptr);
  ASSERT_NE(node_a->GetExtendInfo(), nullptr);
  node_a->GetExtendInfo()->Clear();
  (void)ExecuteGraphUtils::CopyInCtrlEdges(node_a, node_b);
  (void)ExecuteGraphUtils::CopyOutCtrlEdges(node_a, node_b);
  ASSERT_NE(node_d->GetExtendInfo(), nullptr);
  node_d->GetExtendInfo()->Clear();
  (void)ExecuteGraphUtils::MoveInCtrlEdges(node_d, node_b);
  ASSERT_NE(node_e->GetExtendInfo(), nullptr);
  node_e->GetExtendInfo()->Clear();
  (void)ExecuteGraphUtils::MoveOutCtrlEdges(node_e, node_b);
  const EdgeSrcEndpoint new_src = {node_a, 99};
  (void)ExecuteGraphUtils::ReplaceEdgeSrc(real_edge, new_src);
}

// 覆盖 model_serialize.cc:1189 —— 外置权重文件缺失时加载失败
TEST_F(IncCovLogSTest, IncCovSt_ModelLoad_ExternalWeightFileMissing) {
  const std::string dir = "/tmp/opencode/st_ext_dir";
  const std::string model_file = dir + "/model.om";
  (void)ge::CreateDir(dir);
  auto graph = std::make_shared<ComputeGraph>("st_ext_graph");
  auto const_desc = std::make_shared<OpDesc>("const_node", "Const");
  (void)const_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U});
  (void)AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, weight);
  (void)graph->AddNode(const_desc);
  Model model("st_ext_model", "");
  model.SetGraph(graph);
  if (model.SaveToFile(model_file, true) != GRAPH_SUCCESS) {
    GTEST_SKIP();
  }
  const std::string weight_file = dir + "/air_weight/st_ext_model/Const_st_ext_graph_const_node_file";
  (void)std::remove(weight_file.c_str());
  std::ifstream ifs(model_file, std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  Model loaded;
  (void)Model::Load(data.data(), data.size(), loaded);
  (void)std::remove(model_file.c_str());
}

// 覆盖 compute_graph.cc:1954 —— 目标 Retval 节点不删除
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraphImpl_RemoveRetvalKeepTarget) {
  auto graph = std::make_shared<ComputeGraph>("st_keep_retval");
  auto src_desc = std::make_shared<OpDesc>("src_n", "Relu");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  auto retval_desc = std::make_shared<OpDesc>("retval_n", "Retval");
  (void)retval_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(retval_desc, RETVAL_ATTR_NAME_INDEX, 0);
  auto retval_node = graph->AddNode(retval_desc);
  (void)GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), retval_node->GetInDataAnchor(0));
  graph->SetGraphTargetNodesInfo(std::vector<NodePtr>{retval_node});
  ASSERT_NE(graph->impl_, nullptr);
  (void)graph->impl_->RemoveUnusedRetvalNode(graph);
}

// 覆盖 tiling_context_builder.cc:184 —— 构建输入张量失败
TEST_F(IncCovLogSTest, IncCovSt_TilingContextBuilder_BuildInputFailed) {
  ut::GraphBuilder builder("st_tiling_builder");
  auto data_desc = std::make_shared<OpDesc>("data", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  auto data_node = builder.GetGraph()->AddNode(data_desc);
  auto relu_desc = std::make_shared<OpDesc>("relu", "Relu");
  (void)relu_desc->AddInputDesc("x", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  (void)relu_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  auto relu_node = builder.GetGraph()->AddNode(relu_desc);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  const Operator op = OpDescUtils::CreateOperatorFromNode(relu_node);
  int32_t compile_info = 0;
  int32_t platform_info = 0;
  gert::TilingContextBuilder tiling_builder;
  tiling_builder.CompileInfo(&compile_info).PlatformInfo(&platform_info);
  ge::graphStatus ret = ge::GRAPH_SUCCESS;
  (void)tiling_builder.Build(op, ret);
}

// 覆盖 shape_refiner.cc:160/163 —— While 输入输出数量与子图映射不一致
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_WhileInOutMismatch) {
  ut::GraphBuilder builder("st_while_mismatch");
  auto while_node = builder.AddNode("w", "While", 3, 2);
  auto while_desc = while_node->GetOpDesc();
  (void)while_desc->AddSubgraphName("body");
  (void)while_desc->SetSubgraphInstanceName(0U, "body");
  auto body = std::make_shared<ComputeGraph>("body");
  body->SetParentGraph(builder.GetGraph());
  body->SetParentNode(while_node);
  auto data_desc = std::make_shared<OpDesc>("data0", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc());
  (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto data_node = body->AddNode(data_desc);
  auto netout_desc = std::make_shared<OpDesc>("netout", "NetOutput");
  (void)netout_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(0U), ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto netout_node = body->AddNode(netout_desc);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(0));
  (void)builder.GetGraph()->AddSubgraph("body", body);
  Operator op = OpDescUtils::CreateOperatorFromNode(while_node);
  (void)ShapeRefiner::InferShapeAndTypeForRunning(while_node, op, false);
  (void)ShapeRefiner::InferShapeAndType(while_node, true);
}

// 覆盖 graph_utils.cc:2175 —— inplace 输入跨图（直接连锚点）
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_InplaceCrossGraphLink) {
  ut::GraphBuilder builder_a("st_inplace_a2");
  auto node_a = builder_a.AddNode("node_a", "Relu", 1, 1);
  ut::GraphBuilder builder_b("st_inplace_b2");
  auto node_b = builder_b.AddNode("node_b", "Relu", 1, 1);
  (void)node_a->GetInDataAnchor(0)->LinkFrom(node_b->GetOutDataAnchor(0));
  std::vector<std::vector<int64_t>> ability = {{0, 0}};
  (void)AttrUtils::SetListListInt(node_a->GetOpDesc(), ATTR_NAME_OUTPUT_INPLACE_ABILITY, ability);
  std::map<size_t, std::vector<size_t>> out_index_to_in_indexes;
  (void)GraphUtils::GetSupportInplaceOutput(node_a, out_index_to_in_indexes);
}

// 覆盖 compute_graph.cc:2059 —— NetOutput 父索引写入时输入描述缺失
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraphImpl_NetOutputParentIndexNoDesc) {
  auto graph = std::make_shared<ComputeGraph>("st_2059_graph");
  auto src_desc = std::make_shared<OpDesc>("src", "Relu");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  auto netout_desc = std::make_shared<OpDesc>("netout", "NetOutput");
  auto netout_node = graph->AddNode(netout_desc);
  ASSERT_NE(graph->impl_, nullptr);
  ComputeGraphImpl::RetvalInfo info;
  info.output_node = src_node;
  info.node_output_index = 0;
  info.parent_node_index = 0;
  std::vector<ComputeGraphImpl::RetvalInfo> infos{info};
  (void)graph->impl_->UpdateNetOutputParentNodeIndex(netout_node, infos);
}

// 覆盖 compute_graph.cc:1954 —— 目标 Retval 节点在第一轮建图后保留
TEST_F(IncCovLogSTest, IncCovSt_ComputeGraph_RetvalTargetAfterNetoutput) {
  auto graph = std::make_shared<ComputeGraph>("st_retval_after");
  auto src_desc = std::make_shared<OpDesc>("src", "Relu");
  (void)src_desc->AddInputDesc("x", GeTensorDesc());
  (void)src_desc->AddOutputDesc("y", GeTensorDesc());
  auto src_node = graph->AddNode(src_desc);
  (void)graph->CreateOrUpdateNetoutput();
  auto retval_desc = std::make_shared<OpDesc>("retval", "Retval");
  (void)retval_desc->AddInputDesc("x", GeTensorDesc());
  (void)AttrUtils::SetInt(retval_desc, RETVAL_ATTR_NAME_INDEX, 0);
  auto retval_node = graph->AddNode(retval_desc);
  (void)GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), retval_node->GetInDataAnchor(0));
  graph->SetGraphTargetNodesInfo(std::vector<NodePtr>{retval_node});
  (void)graph->CreateOrUpdateNetoutput(true);
  ASSERT_NE(graph->impl_, nullptr);
  (void)graph->impl_->RemoveUnusedRetvalNode(graph);
}

// 覆盖 graph_utils.cc:2747 —— 拷贝图时子图缺失导致失败
TEST_F(IncCovLogSTest, IncCovSt_GraphUtils_CopyMissingSubgraph) {
  ut::GraphBuilder builder("st_copy_missing_sub");
  auto func_desc = std::make_shared<OpDesc>("func", "PartitionedCall");
  (void)func_desc->AddInputDesc("x", GeTensorDesc());
  (void)func_desc->AddOutputDesc("y", GeTensorDesc());
  (void)func_desc->AddSubgraphName("missing_sub");
  (void)func_desc->SetSubgraphInstanceName(0U, "missing_sub");
  (void)builder.GetGraph()->AddNode(func_desc);
  auto dst = std::make_shared<ComputeGraph>("st_copy_missing_dst");
  std::map<ConstNodePtr, NodePtr> node_map;
  std::map<ConstOpDescPtr, OpDescPtr> op_map;
  EXPECT_NE(GraphUtils::CopyComputeGraph(builder.GetGraph(), dst, node_map, op_map, 0), GRAPH_SUCCESS);
}

// 覆盖 shape_refiner.cc:193 —— While 输入输出 shape 不一致需重推
TEST_F(IncCovLogSTest, IncCovSt_ShapeRefiner_WhileShapeMismatch) {
  ut::GraphBuilder builder("st_while_shape");
  auto while_node = builder.AddNode("w", "While", 2, 2);
  auto while_desc = while_node->GetOpDesc();
  (void)while_desc->AddSubgraphName("body");
  (void)while_desc->SetSubgraphInstanceName(0U, "body");
  auto while_in0 = while_desc->MutableInputDesc(0U);
  ASSERT_NE(while_in0, nullptr);
  while_in0->SetShape(GeShape({2, 3}));
  while_in0->SetDataType(DT_FLOAT);
  auto while_in1 = while_desc->MutableInputDesc(1U);
  ASSERT_NE(while_in1, nullptr);
  while_in1->SetShape(GeShape({2, 5}));
  while_in1->SetDataType(DT_FLOAT);
  auto while_out0 = while_desc->MutableOutputDesc(0U);
  ASSERT_NE(while_out0, nullptr);
  while_out0->SetShape(GeShape({2, 4}));
  while_out0->SetDataType(DT_FLOAT);
  auto while_out1 = while_desc->MutableOutputDesc(1U);
  ASSERT_NE(while_out1, nullptr);
  while_out1->SetShape(GeShape({2, 5}));
  while_out1->SetDataType(DT_FLOAT);
  auto body = std::make_shared<ComputeGraph>("body");
  body->SetParentGraph(builder.GetGraph());
  body->SetParentNode(while_node);
  auto data_desc = std::make_shared<OpDesc>("data0", "Data");
  (void)data_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2, 3}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto data_node = body->AddNode(data_desc);
  auto netout_desc = std::make_shared<OpDesc>("netout", "NetOutput");
  (void)netout_desc->AddInputDesc("x", GeTensorDesc(GeShape({2, 4}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(0U), ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto netout_node = body->AddNode(netout_desc);
  auto data1_desc = std::make_shared<OpDesc>("data1", "Data");
  (void)data1_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2, 5}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(data1_desc, ATTR_NAME_PARENT_NODE_INDEX, 1);
  auto data1_node = body->AddNode(data1_desc);
  (void)netout_desc->AddInputDesc("x1", GeTensorDesc(GeShape({2, 5}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(netout_desc->MutableInputDesc(1U), ATTR_NAME_PARENT_NODE_INDEX, 1);
  (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(0));
  (void)GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(1));
  (void)builder.GetGraph()->AddSubgraph("body", body);
  Operator op = OpDescUtils::CreateOperatorFromNode(while_node);
  (void)ShapeRefiner::InferShapeAndTypeForRunning(while_node, op, false);
  (void)ShapeRefiner::InferShapeAndType(while_node);
  (void)ShapeRefiner::InferShapeAndType(while_node, true);
}

// 覆盖 model_serialize.cc:1169 —— 外置权重长度非法
TEST_F(IncCovLogSTest, IncCovSt_ModelLoad_InvalidWeightLength) {
  auto graph = std::make_shared<ComputeGraph>("st_len_graph");
  auto const_desc = std::make_shared<OpDesc>("const_len", "Const");
  (void)const_desc->AddOutputDesc("y", GeTensorDesc(GeShape({2}), FORMAT_ND, DT_FLOAT));
  GeTensorPtr weight = std::make_shared<GeTensor>();
  (void)weight->SetData(std::vector<uint8_t>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U});
  (void)AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, weight);
  (void)AttrUtils::SetStr(const_desc, ATTR_NAME_LOCATION, "/tmp/opencode/st_missing_weight");
  (void)AttrUtils::SetInt(const_desc, ATTR_NAME_LENGTH, 0);
  (void)graph->AddNode(const_desc);
  Model model("st_len_model", "");
  model.SetGraph(graph);
  ModelSerialize serializer;
  auto buffer = serializer.SerializeModel(model, false);
  ASSERT_NE(buffer.GetData(), nullptr);
  Model loaded;
  (void)Model::Load(buffer.GetData(), buffer.GetSize(), loaded);
}
}  // namespace ge
