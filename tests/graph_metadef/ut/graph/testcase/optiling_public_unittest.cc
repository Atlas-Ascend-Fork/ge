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
#include "graph/op_desc.h"
#include "graph/operator.h"
#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "common/sgt_slice_type.h"
#include "graph_builder_utils.h"
#include "register/op_tiling_registry.h"
#include "register/op_tiling.h"
#include "op_tiling/op_tiling_constants.h"
#include "graph/utils/anchor_utils.h"

using namespace ge;
using namespace ffts;

namespace optiling {
bool op_tiling_stub_cov(const Operator &op, const utils::OpCompileInfo &compile_info, utils::OpRunInfo &run_info) {
  return true;
}
}  // namespace optiling

REGISTER_OP_TILING_V2(ReluCovForGraph, optiling::op_tiling_stub_cov);

namespace {
class OptilingPublicCovTest : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

// 覆盖 op_tiling.cc:597/603/604 —— op_desc 为空时 tiling 函数未注册分支
TEST_F(OptilingPublicCovTest, IncCov_OpParaCalculateV2_NullOpDesc) {
  const Operator op;
  optiling::OpRunInfoV2 run_info;
  EXPECT_EQ(optiling::OpParaCalculateV2(op, run_info), ge::GRAPH_FAILED);
}

// 覆盖 op_tiling.cc:597/598/600 —— op_desc 非空但 tiling 函数未注册分支
TEST_F(OptilingPublicCovTest, IncCov_OpParaCalculateV2_TilingFuncNotFound) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("not_reg", "NeverRegisteredTypeCov", 1, 1);
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);
  optiling::OpRunInfoV2 run_info;
  EXPECT_EQ(optiling::OpParaCalculateV2(op, run_info), ge::GRAPH_FAILED);
}

// 覆盖 op_tiling.cc:1031 —— thread_id 越过空切片表（slice_instance_num 为 0）
TEST_F(OptilingPublicCovTest, IncCov_OpFftsPlusCalculate_EmptySliceInfo) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("relu", "ReluCovForGraph", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);

  const ThreadSliceMapDyPtr slice_info_ptr = std::make_shared<ThreadSliceMapDy>();
  slice_info_ptr->slice_instance_num = 0U;
  (void)op_desc->SetExtAttr(ffts::kAttrSgtStructInfoDy, slice_info_ptr);
  std::vector<optiling::OpRunInfoV2> op_run_info;
  EXPECT_EQ(optiling::OpFftsPlusCalculate(op, op_run_info), ge::GRAPH_FAILED);
}

// 覆盖 op_tiling.cc:1075/1079/1080 —— 完整 FFTS+ tiling 成功路径中
// UpDateNodeShapeBack 的正常判断行（空指针分支为防御性代码，经公开接口不可达）
TEST_F(OptilingPublicCovTest, IncCov_OpFftsPlusCalculate_SuccessPath) {
  auto root_builder = ut::GraphBuilder("root");
  const auto &node = root_builder.AddNode("relu", "ReluCovForGraph", 1, 1);
  const auto &op_desc = node->GetOpDesc();
  const Operator op = OpDescUtils::CreateOperatorFromNode(node);

  const ThreadSliceMapDyPtr slice_info_ptr = std::make_shared<ThreadSliceMapDy>();
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
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_KEY, "cov_graph_key");
  (void)ge::AttrUtils::SetStr(op_desc, optiling::COMPILE_INFO_JSON, "cov_graph_json");
  auto dst_anchor = node->GetInDataAnchor(0);
  ge::AnchorUtils::SetStatus(dst_anchor, ge::ANCHOR_DATA);

  std::vector<optiling::OpRunInfoV2> op_run_info;
  EXPECT_EQ(optiling::OpFftsPlusCalculate(op, op_run_info), ge::GRAPH_SUCCESS);
}
}  // namespace
