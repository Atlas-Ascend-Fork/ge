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
#include <map>
#include <vector>

#include "formats/format_transfers/format_transfer_fractal_nz.h"
#include "formats/format_transfers/format_transfer_transpose.h"
#include "formats/formats.h"
#include "transfer_shape_utils.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/types.h"
#include "graph/utils/type_utils.h"
#include "host_kernels/kernel.h"
#include "host_kernels/kernel_factory.h"
#include "host_kernels/kernel_utils.h"
#include "register/graph_optimizer/fusion_common/fusion_turbo.h"
#include "register/op_kernel_registry.h"

using namespace std;
using namespace ge;

namespace {
uint8_t MakeInt4Byte(const uint8_t lo, const uint8_t hi) {
  return static_cast<uint8_t>((lo & 0x0FU) | static_cast<uint8_t>((hi & 0x0FU) << 4));
}

std::vector<uint8_t> MakeInt4PackedData(const size_t elem_count) {
  std::vector<uint8_t> data((elem_count + 1U) / 2U, 0);
  for (size_t i = 0U; i < elem_count; ++i) {
    const auto value = static_cast<uint8_t>(i % 16U);
    if ((i % 2U) == 0U) {
      data[i / 2U] = MakeInt4Byte(value, data[i / 2U] >> 4);
    } else {
      data[i / 2U] = MakeInt4Byte(data[i / 2U] & 0x0FU, value);
    }
  }
  return data;
}

uint8_t GetInt4Elem(const uint8_t *const data, const size_t elem_idx) {
  const uint8_t byte = data[elem_idx / 2U];
  return ((elem_idx % 2U) == 0U) ? static_cast<uint8_t>(byte & 0x0FU) : static_cast<uint8_t>(byte >> 4);
}
}  // namespace

namespace ge {
namespace formats {
class StestSubByteFormat : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

// ND -> NZ (sub-byte): W aligned to C0, layout check + padding
TEST_F(StestSubByteFormat, nd_to_nz_c0_16_success) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 32U);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {2, 32},   {1, 2, 1, 16, 16}, DT_INT4};
  TransResult result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  EXPECT_EQ(result.length, (1U * 2U * 1U * 16U * 16U) / 2U);
  const auto elem_at = [&result](const size_t r, const size_t w) {
    const size_t idx = (w / 16U) * 256U + r * 16U + (w % 16U);
    return GetInt4Elem(result.data.get(), idx);
  };
  EXPECT_EQ(elem_at(0, 0), 0U);
  EXPECT_EQ(elem_at(0, 1), 1U);
  EXPECT_EQ(elem_at(1, 17), (1U * 32U + 17U) % 16U);
  EXPECT_EQ(elem_at(3, 20), 0U);
}

// ND -> NZ -> ND round trip keeps data
TEST_F(StestSubByteFormat, nd_to_nz_round_trip) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 32U);
  const Format nd_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format nz_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs to_nz{src.data(),      nd_format, nz_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                  FORMAT_RESERVED, 16,        16,        {2, 32},   {1, 2, 1, 16, 16}, DT_INT4};
  TransResult nz_result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(to_nz, nz_result), SUCCESS);
  TransArgs to_nd{nz_result.data.get(), nz_format, nd_format, FORMAT_FRACTAL_NZ, FORMAT_ND, FORMAT_RESERVED,
                  FORMAT_RESERVED,      16,        16,        {1, 2, 1, 16, 16}, {2, 32},   DT_INT4};
  TransResult nd_result;
  FormatTransferFractalNzND reverse_transfer;
  ASSERT_EQ(reverse_transfer.TransFormat(to_nd, nd_result), SUCCESS);
  ASSERT_EQ(nd_result.length, src.size());
  for (size_t i = 0U; i < src.size(); ++i) {
    ASSERT_EQ(nd_result.data.get()[i], src[i]) << "byte " << i;
  }
}

// ND -> NZ (sub-byte): W not aligned to C0, tail path
TEST_F(StestSubByteFormat, nd_to_nz_tail_w) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 17U);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {2, 17},   {1, 2, 1, 16, 16}, DT_INT4};
  TransResult result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  const auto elem_at = [&result](const size_t r, const size_t w) {
    const size_t idx = (w / 16U) * 256U + r * 16U + (w % 16U);
    return GetInt4Elem(result.data.get(), idx);
  };
  EXPECT_EQ(elem_at(0, 16), 0U);
  EXPECT_EQ(elem_at(1, 16), (1U * 17U + 16U) % 16U);
  EXPECT_EQ(elem_at(0, 17), 0U);
}

// ND -> NZ (sub-byte): W < W0, only tail loop
TEST_F(StestSubByteFormat, nd_to_nz_w_lt_w0) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 5U);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {2, 5},    {1, 1, 16, 16},    DT_INT4};
  TransResult result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  EXPECT_EQ(GetInt4Elem(result.data.get(), 1U * 16U + 4U), (1U * 5U + 4U) % 16U);
  EXPECT_EQ(GetInt4Elem(result.data.get(), 15U), 0U);
}

// ND -> NZ (sub-byte): DT_INT2 (bits=2)
TEST_F(StestSubByteFormat, nd_to_nz_int2) {
  std::vector<uint8_t> src((2U * 16U) / 4U, 0);
  for (size_t i = 0U; i < 32U; ++i) {
    const auto value = static_cast<uint8_t>(i % 4U);
    const size_t byte = i / 4U;
    const size_t shift = (i % 4U) * 2U;
    src[byte] = static_cast<uint8_t>(src[byte] | (value << shift));
  }
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {2, 16},   {1, 2, 1, 16, 16}, DT_INT2};
  TransResult result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  const size_t idx = 16U + 15U;
  const size_t byte = idx / 4U;
  const auto value = static_cast<uint8_t>((result.data.get()[byte] >> ((idx % 4U) * 2U)) & 0x3U);
  EXPECT_EQ(value, 31U % 4U);
}

// ND -> NZ (sub-byte): C0=32 explicit variant, aligned fast path
TEST_F(StestSubByteFormat, nd_to_nz_c0_32_success) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 32U);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 6));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {2, 32},   {1, 1, 16, 32},    DT_INT4};
  TransResult result;
  FormatTransferFractalNz transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  EXPECT_EQ(GetInt4Elem(result.data.get(), 32U), 32U % 16U);
  EXPECT_EQ(GetInt4Elem(result.data.get(), 63U), 63U % 16U);
}

// ND -> NZ (sub-byte): 6-bit dtype rejected
TEST_F(StestSubByteFormat, nd_to_nz_float6_rejected) {
  const std::vector<uint8_t> src(8U, 0);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND,  FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, 16,         16,         {1, 3, 17}, {1, 2, 1, 16, 16}, DT_FLOAT6_E3M2};
  TransResult result;
  FormatTransferFractalNz transfer;
  EXPECT_EQ(transfer.TransFormat(args, result), ACL_ERROR_GE_DATATYPE_INVALID);
}

// ND -> NZ (sub-byte): bare FRACTAL_NZ rejected (no explicit c0)
TEST_F(StestSubByteFormat, nd_to_nz_bare_nz_rejected) {
  const std::vector<uint8_t> src(32U, 0);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 0));
  TransArgs args{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                 FORMAT_RESERVED, -1,         -1,         {2, 32},   {1, 2, 1, 16, 16}, DT_INT4};
  TransResult result;
  FormatTransferFractalNz transfer;
  EXPECT_EQ(transfer.TransFormat(args, result), ACL_ERROR_GE_SHAPE_INVALID);
}

// ND -> NZ (sub-byte): invalid dst shapes rejected
TEST_F(StestSubByteFormat, nd_to_nz_invalid_shapes) {
  const std::vector<uint8_t> src(32U, 0);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  const Format dst_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5));
  TransResult result;
  FormatTransferFractalNz transfer;
  TransArgs args_too_small{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                           FORMAT_RESERVED, 16,         16,         {2, 32},   {2, 1, 16},        DT_INT4};
  EXPECT_EQ(transfer.TransFormat(args_too_small, result), ACL_ERROR_GE_SHAPE_INVALID);
  TransArgs args_negative{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ,  FORMAT_RESERVED,
                          FORMAT_RESERVED, 16,         16,         {2, 32},   {1, 2, 1, -16, 16}, DT_INT4};
  EXPECT_EQ(transfer.TransFormat(args_negative, result), ACL_ERROR_GE_SHAPE_INVALID);
  TransArgs args_w0_zero{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                         FORMAT_RESERVED, 16,         16,         {2, 32},   {1, 2, 1, 16, 0},  DT_INT4};
  EXPECT_EQ(transfer.TransFormat(args_w0_zero, result), ACL_ERROR_GE_DATATYPE_INVALID);
  TransArgs args_times_mismatch{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                                FORMAT_RESERVED, 16,         16,         {2, 32},   {2, 2, 1, 16, 16}, DT_INT4};
  EXPECT_EQ(transfer.TransFormat(args_times_mismatch, result), ACL_ERROR_GE_SHAPE_INVALID);
  TransArgs args_capacity{src.data(),      src_format, dst_format, FORMAT_ND, FORMAT_FRACTAL_NZ, FORMAT_RESERVED,
                          FORMAT_RESERVED, 16,         16,         {2, 32},   {1, 1, 1, 16, 16}, DT_INT4};
  EXPECT_EQ(transfer.TransFormat(args_capacity, result), ACL_ERROR_GE_SHAPE_INVALID);
}

// NZ -> ND (sub-byte): src c0 fallback when src_c0_format invalid
TEST_F(StestSubByteFormat, nz_to_nd_c0_fallback_success) {
  const std::vector<uint8_t> src = MakeInt4PackedData(1U * 2U * 1U * 16U * 16U);
  TransArgs args{src.data(),
                 static_cast<Format>(FORMAT_FRACTAL_NZ_C0_16),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_ND,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 -1,
                 -1,
                 {1, 2, 1, 16, 16},
                 {2, 32},
                 DT_INT4};
  TransResult result;
  FormatTransferFractalNzND transfer;
  ASSERT_EQ(transfer.TransFormat(args, result), SUCCESS);
  EXPECT_EQ(GetInt4Elem(result.data.get(), 1U * 32U + 17U), (1U * 32U + 17U) % 16U);
}

// TransShape with explicit c0 variants covers GetEffectiveC0 branches
TEST_F(StestSubByteFormat, trans_shape_c0_variants) {
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  std::vector<int64_t> dst_shape;
  FormatTransferFractalNz transfer;
  EXPECT_EQ(transfer.TransShape(src_format, {2, 32}, DT_INT4, static_cast<Format>(FORMAT_FRACTAL_NZ_C0_16), dst_shape),
            SUCCESS);
  EXPECT_EQ(dst_shape, (std::vector<int64_t>{2, 1, 16, 16}));
  EXPECT_EQ(transfer.TransShape(src_format, {2, 32}, DT_INT4, static_cast<Format>(FORMAT_FRACTAL_NZ_C0_32), dst_shape),
            SUCCESS);
  EXPECT_EQ(dst_shape, (std::vector<int64_t>{1, 1, 16, 32}));
  EXPECT_EQ(transfer.TransShape(src_format, {2, 32}, DT_INT4, static_cast<Format>(FORMAT_FRACTAL_NZ_C0_8), dst_shape),
            SUCCESS);
  EXPECT_EQ(dst_shape, (std::vector<int64_t>{4, 1, 16, 8}));
  EXPECT_EQ(transfer.TransShape(src_format, {2, 32}, DT_INT4, static_cast<Format>(FORMAT_FRACTAL_NZ_C0_2), dst_shape),
            SUCCESS);
  EXPECT_EQ(dst_shape, (std::vector<int64_t>{16, 1, 16, 2}));
  EXPECT_EQ(transfer.TransShape(src_format, {2, 32}, DT_INT4, static_cast<Format>(FORMAT_FRACTAL_NZ_C0_4), dst_shape),
            SUCCESS);
  EXPECT_EQ(dst_shape, (std::vector<int64_t>{8, 1, 16, 4}));
}

// TransDataFormat gate: sub-byte nd -> nz passes whole chain
TEST_F(StestSubByteFormat, trans_data_nd_to_nz_c0_16) {
  const std::vector<uint8_t> src = MakeInt4PackedData(2U * 32U);
  TransArgs args{src.data(),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 16,
                 16,
                 {2, 32},
                 {1, 2, 1, 16, 16},
                 DT_INT4};
  TransResult result;
  EXPECT_EQ(TransDataFormat(args, result), SUCCESS);
  EXPECT_EQ(result.length, 256U);
}

// TransDataFormat gate: sub-byte invalid format pairs rejected
TEST_F(StestSubByteFormat, trans_data_invalid_pairs_rejected) {
  const std::vector<uint8_t> src(32U, 0);
  TransResult result;
  TransArgs args_bare{src.data(),
                      FORMAT_ND,
                      static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 0)),
                      FORMAT_ND,
                      FORMAT_FRACTAL_NZ,
                      FORMAT_RESERVED,
                      FORMAT_RESERVED,
                      -1,
                      -1,
                      {2, 32},
                      {1, 2, 1, 16, 16},
                      DT_INT4};
  EXPECT_EQ(TransDataFormat(args_bare, result), ACL_ERROR_GE_DATATYPE_INVALID);
  TransArgs args_nchw{src.data(),
                      static_cast<Format>(GetFormatFromSubAndC0(FORMAT_NCHW, FORMAT_RESERVED, 5)),
                      static_cast<Format>(GetFormatFromSubAndC0(FORMAT_FRACTAL_NZ, FORMAT_RESERVED, 5)),
                      FORMAT_NCHW,
                      FORMAT_FRACTAL_NZ,
                      FORMAT_RESERVED,
                      FORMAT_RESERVED,
                      16,
                      16,
                      {1, 2, 2, 16},
                      {1, 2, 1, 16, 16},
                      DT_INT4};
  EXPECT_EQ(TransDataFormat(args_nchw, result), ACL_ERROR_GE_DATATYPE_INVALID);
  TransArgs args_5hd{
      src.data(), FORMAT_ND, FORMAT_NC1HWC0, FORMAT_ND,        FORMAT_NC1HWC0, FORMAT_RESERVED, FORMAT_RESERVED,
      -1,         -1,        {1, 2, 2, 16},  {1, 1, 2, 2, 16}, DT_INT4};
  EXPECT_EQ(TransDataFormat(args_5hd, result), ACL_ERROR_GE_DATATYPE_INVALID);
}

// TransDataFormat: non sub-byte dtype with c0 variant registry entry still works
TEST_F(StestSubByteFormat, trans_data_fp16_nd_to_nz_c0_16) {
  std::vector<uint16_t> src(2U * 32U, 12210);
  TransArgs args{reinterpret_cast<uint8_t *>(src.data()),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 16,
                 16,
                 {2, 32},
                 {2, 1, 16, 16},
                 DT_FLOAT16};
  TransResult result;
  EXPECT_EQ(TransDataFormat(args, result), SUCCESS);
  EXPECT_EQ(result.length, 2U * 1U * 16U * 16U * sizeof(uint16_t));
}

// transpose rejects sub-byte dtype
TEST_F(StestSubByteFormat, transpose_sub_byte_rejected) {
  uint8_t data[4] = {1, 2, 3, 4};
  TransResult result;
  EXPECT_EQ(TransposeWithShapeCheck(data, std::vector<int64_t>({2, 2}), std::vector<int64_t>({2, 2}), DT_INT4,
                                    std::vector<int64_t>({1, 0}), result),
            ACL_ERROR_GE_PARAM_INVALID);
}
}  // namespace formats

// ----------------------- folding gate / pack / kernel utils / fusion turbo -----------------------

class StestSubByteFolding : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(StestSubByteFolding, fusion_turbo_weight_info_sub_byte) {
  uint8_t data_buf[8] = {0};
  fe::WeightInfo weight_info(GeShape({512, 4096}), GeShape({512, 4096}), DT_INT4, DT_INT4, FORMAT_ND, FORMAT_ND,
                             data_buf);
  // 2097152 int4 elems -> CeilDiv(2097152 * 4, 8) = 1048576 bytes (not 2097152 * 1004)
  EXPECT_EQ(weight_info.total_data_size, 1048576U);

  fe::WeightInfo fp16_info(GeShape({2}), GeShape({2}), DT_FLOAT16, DT_FLOAT16, FORMAT_ND, FORMAT_ND, data_buf);
  EXPECT_EQ(fp16_info.total_data_size, 4U);
}
}  // namespace ge

// ----------------------- constant folding sub-byte gate (ST) -----------------------

#include "graph/passes/graph_builder_utils.h"
#include "graph/op_desc.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/framework_types_internal.h"
#include "graph/passes/standard_optimize/constant_folding/constant_folding_pass.h"

namespace {
class StTestLegacyHostCpuOp final : public HostCpuOp {
 public:
  graphStatus Compute(Operator &, const std::map<std::string, const ge::Tensor> &,
                      std::map<std::string, ge::Tensor> &) override {
    ++compute_count_;
    return GRAPH_SUCCESS;
  }
  static int32_t compute_count_;
};
int32_t StTestLegacyHostCpuOp::compute_count_ = 0;

GeTensorPtr MakeTypedInputTensor(DataType data_type) {
  auto input_tensor = MakeShared<GeTensor>();
  if (input_tensor == nullptr) {
    return nullptr;
  }
  input_tensor->MutableTensorDesc().SetShape(GeShape({8}));
  input_tensor->MutableTensorDesc().SetOriginShape(GeShape({8}));
  input_tensor->MutableTensorDesc().SetFormat(FORMAT_NCHW);
  input_tensor->MutableTensorDesc().SetOriginFormat(FORMAT_NCHW);
  input_tensor->MutableTensorDesc().SetDataType(data_type);
  input_tensor->MutableTensorDesc().SetOriginDataType(data_type);
  if (input_tensor->SetData(std::vector<uint8_t>{1, 2, 3, 4}) != SUCCESS) {
    return nullptr;
  }
  return input_tensor;
}
}  // namespace

TEST_F(StestSubByteFolding, run_op_kernel_blocks_sub_byte_input) {
  StTestLegacyHostCpuOp::compute_count_ = 0;
  auto builder = ut::GraphBuilder("test");
  auto input = builder.AddNode("input", CONSTANT, 0, 1, FORMAT_NCHW, DT_UINT8, {8});
  auto output = builder.AddNode("output", "StSubByteBlockedOp", 1, 1, FORMAT_NCHW, DT_UINT8, {8});
  builder.AddDataEdge(input, 0, output, 0);
  auto node = output;
  ASSERT_NE(node, nullptr);
  auto input_tensor = MakeTypedInputTensor(DT_INT4);
  ASSERT_NE(input_tensor, nullptr);
  std::vector<ConstGeTensorPtr> inputs{input_tensor};
  std::vector<GeTensorPtr> outputs;

  OpKernelRegistry::GetInstance().RegisterHostCpuOp("StSubByteBlockedOp",
                                                    []() -> HostCpuOp * { return new StTestLegacyHostCpuOp(); });
  EXPECT_EQ(ConstantFoldingPass::RunOpKernel(node, inputs, outputs), UNSUPPORTED);
  EXPECT_EQ(StTestLegacyHostCpuOp::compute_count_, 0);
}

TEST_F(StestSubByteFolding, run_op_kernel_blocks_sub_byte_output) {
  StTestLegacyHostCpuOp::compute_count_ = 0;
  auto builder = ut::GraphBuilder("test");
  auto input = builder.AddNode("input", CONSTANT, 0, 1, FORMAT_NCHW, DT_UINT8, {8});
  auto output = builder.AddNode("output", "QuantBiasOptimization", 1, 1, FORMAT_NCHW, DT_INT4, {8});
  builder.AddDataEdge(input, 0, output, 0);
  auto node = output;
  ASSERT_NE(node, nullptr);
  auto input_tensor = MakeTypedInputTensor(DT_UINT8);
  ASSERT_NE(input_tensor, nullptr);
  std::vector<ConstGeTensorPtr> inputs{input_tensor};
  std::vector<GeTensorPtr> outputs;

  OpKernelRegistry::GetInstance().RegisterHostCpuOp("QuantBiasOptimization",
                                                    []() -> HostCpuOp * { return new StTestLegacyHostCpuOp(); });
  EXPECT_EQ(ConstantFoldingPass::RunOpKernel(node, inputs, outputs), UNSUPPORTED);
  EXPECT_EQ(StTestLegacyHostCpuOp::compute_count_, 0);
}

TEST_F(StestSubByteFolding, pack_kernel_skips_sub_byte) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("pack", "Pack");
  vector<bool> is_input_const_vec = {true, true, true, true};
  op_desc_ptr->SetIsInputConst(is_input_const_vec);
  AttrUtils::SetInt(op_desc_ptr, PACK_ATTR_NAME_NUM, (int64_t)4);
  AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_AXIS, (int64_t)0);
  op_desc_ptr->AddInputDesc(0, GeTensorDesc(GeShape({1}), FORMAT_NCHW, DT_INT4));

  vector<int64_t> dims;
  vector<uint8_t> data = {0};
  GeTensorDesc tensor_desc(GeShape(dims), FORMAT_NCHW, DT_INT4);
  vector<ConstGeTensorPtr> input;
  for (int32_t i = 0; i < 4; ++i) {
    input.emplace_back(std::make_shared<GeTensor>(tensor_desc, (uint8_t *)data.data(), 1U));
  }
  vector<GeTensorPtr> outputs;

  shared_ptr<Kernel> kernel = KernelFactory::Instance().Create(PACK);
  auto status = kernel->Compute(op_desc_ptr, input, outputs);
  EXPECT_EQ(NOT_CHANGED, status);
}

TEST_F(StestSubByteFolding, kernel_utils_check_size_scalar_sub_byte) {
  vector<uint8_t> data_vec = {0x5A};
  GeTensorDesc tensor_desc(GeShape(), FORMAT_NCHW, DT_INT4);
  ConstGeTensorPtr tensor = std::make_shared<GeTensor>(tensor_desc, (uint8_t *)data_vec.data(), data_vec.size());

  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("TransOp", "TransOp");
  op_desc_ptr->AddInputDesc(GeTensorDesc(GeShape(), FORMAT_NCHW, DT_INT4));

  bool ret = KernelUtils::CheckSizeForTransOp(tensor, op_desc_ptr);
  EXPECT_EQ(true, ret);
}

namespace ge {
namespace formats {
// ND -> NZ (sub-byte): unaligned c0 with 1-bit dtype rejected
TEST_F(StestSubByteFormat, nd_to_nz_unaligned_c0_uint1) {
  const std::vector<uint8_t> src(8U, 0);
  const Format src_format = static_cast<Format>(GetFormatFromSubAndC0(FORMAT_ND, FORMAT_RESERVED, 5));
  TransArgs args{src.data(),
                 src_format,
                 static_cast<Format>(FORMAT_FRACTAL_NZ_C0_2),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_2,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 2,
                 2,
                 {1, 8},
                 {1, 4, 16, 2},
                 DT_UINT1};
  TransResult result;
  FormatTransferFractalNz transfer;
  EXPECT_EQ(transfer.TransFormat(args, result), ACL_ERROR_GE_DATATYPE_INVALID);
}

// NZ -> ND (sub-byte): 6-bit dtype rejected
TEST_F(StestSubByteFormat, nz_to_nd_float6_rejected) {
  const std::vector<uint8_t> src(128U, 0);
  TransArgs args{src.data(),
                 static_cast<Format>(FORMAT_FRACTAL_NZ_C0_16),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_16,
                 FORMAT_ND,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 16,
                 16,
                 {1, 2, 1, 16, 16},
                 {2, 32},
                 DT_FLOAT6_E3M2};
  TransResult result;
  FormatTransferFractalNzND transfer;
  EXPECT_EQ(transfer.TransFormat(args, result), ACL_ERROR_GE_DATATYPE_INVALID);
}

// NZ -> ND (sub-byte): unaligned c0 with 1-bit dtype rejected
TEST_F(StestSubByteFormat, nz_to_nd_unaligned_c0_uint1) {
  const std::vector<uint8_t> src(16U, 0);
  TransArgs args{src.data(),
                 static_cast<Format>(FORMAT_FRACTAL_NZ_C0_2),
                 FORMAT_ND,
                 FORMAT_FRACTAL_NZ_C0_2,
                 FORMAT_ND,
                 FORMAT_RESERVED,
                 FORMAT_RESERVED,
                 -1,
                 -1,
                 {1, 4, 16, 2},
                 {1, 8},
                 DT_UINT1};
  TransResult result;
  FormatTransferFractalNzND transfer;
  EXPECT_EQ(transfer.TransFormat(args, result), ACL_ERROR_GE_DATATYPE_INVALID);
}
}  // namespace formats
}  // namespace ge

namespace ge {
namespace formats {
// nd-to-nz aligned shape honors explicit c0 from format (not dtype default)
TEST_F(StestSubByteFormat, nd_to_nz_aligned_shape_uses_explicit_c0) {
  transformer::AlignShapeInfo info;
  info.src_format = FORMAT_ND;
  info.dst_format = static_cast<Format>(FORMAT_FRACTAL_NZ_C0_32);
  info.src_shape = gert::Shape({2, 32});
  info.data_type = DT_INT4;
  info.reshape_type_mask = 0;
  gert::Shape aligned;
  ASSERT_EQ(transformer::TransferShapeUtils::GetNdToNzAlignedShape(info, aligned), true);
  EXPECT_EQ(aligned.GetDim(aligned.GetDimNum() - 1), 32);
}
}  // namespace formats
}  // namespace ge
