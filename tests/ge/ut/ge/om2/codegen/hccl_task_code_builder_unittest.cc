/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <vector>
#include "common/om2/codegen/task_code_builder/hccl/hccl_task_code_builder.h"
#include "common/om2/codegen/task_code_builder_factory.h"
#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_context.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "framework/common/taskdown_common.h"
#include "framework/common/framework_types_internal.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "hccl/base.h"

namespace ge {
namespace {
constexpr uint32_t kTestOpIndex = 7U;
constexpr uint32_t kTestStreamId = 1U;
}  // namespace

class UtestHcclTaskCodeBuilder : public ::testing::Test {
 protected:
  void SetUp() override {
    builder_ = std::make_unique<HcclTaskCodeBuilder>(ast_);
  }

  void TearDown() override {}

  AstContext ctx_;
  AstBuildContext ast_{ctx_};
  std::unique_ptr<HcclTaskCodeBuilder> builder_;

  TaskSemanticContributeContext MakeContributeContext(const domi::TaskDef &task_def, const OpDescPtr &op_desc) {
    return TaskSemanticContributeContext{ModelTaskType::MODEL_TASK_HCCL,
                                         task_def,
                                         kTestOpIndex,
                                         op_desc,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         &next_args_table_index_,
                                         &next_host_args_offset_,
                                         nullptr};
  }

  static OpDescPtr MakeHcclOpDesc() {
    auto op_desc = std::make_shared<OpDesc>("test_hccl_op", "HcomAllReduce");
    (void)op_desc->AddInputDesc(GeTensorDesc(GeShape({8, 8}), FORMAT_ND, DT_FLOAT));
    (void)op_desc->AddOutputDesc(GeTensorDesc(GeShape({8, 8}), FORMAT_ND, DT_FLOAT));
    (void)AttrUtils::SetInt(op_desc, "used_stream_num", 2);
    (void)AttrUtils::SetInt(op_desc, HCOM_ATTR_ROOT_RANK, 0);
    (void)AttrUtils::SetStr(op_desc, HCOM_ATTR_REDUCE_TYPE, "sum");
    return op_desc;
  }

  static domi::TaskDef MakeHcclTaskDef() {
    domi::TaskDef task_def;
    task_def.set_type(static_cast<int32_t>(ModelTaskType::MODEL_TASK_HCCL));
    task_def.set_stream_id(kTestStreamId);
    task_def.mutable_kernel_hccl()->set_op_index(kTestOpIndex);
    task_def.mutable_kernel_hccl()->set_hccl_type("HcomAllReduce");
    task_def.set_private_def(std::string("\x12\x34\x56", 3U));
    return task_def;
  }

  uint64_t next_args_table_index_ = 0U;
  uint64_t next_host_args_offset_ = 0U;
};

TEST_F(UtestHcclTaskCodeBuilder, ParseOpIndex_Success) {
  domi::TaskDef task_def = MakeHcclTaskDef();
  EXPECT_EQ(builder_->ParseOpIndex(task_def), static_cast<int64_t>(kTestOpIndex));
}

TEST_F(UtestHcclTaskCodeBuilder, ParseOpIndex_DefaultZero) {
  domi::TaskDef task_def;
  EXPECT_EQ(builder_->ParseOpIndex(task_def), 0);
}

TEST_F(UtestHcclTaskCodeBuilder, GetFuncName_IsDispatchHccl) {
  EXPECT_EQ(builder_->GetFuncName(), "DispatchHccl");
}

TEST_F(UtestHcclTaskCodeBuilder, ParseTaskRunParam_ArgsLayoutAlignsOm1) {
  domi::TaskDef task_def = MakeHcclTaskDef();
  om2::RuntimeParam rts_param;
  om2::TaskRunParam task_run_param;
  EXPECT_EQ(builder_->ParseTaskRunParam(task_def, rts_param, MakeHcclOpDesc(), task_run_param), SUCCESS);
  ASSERT_EQ(task_run_param.args_descs.size(), 1U);
  EXPECT_EQ(task_run_param.args_descs[0U].args_len, static_cast<int64_t>(sizeof(void *) * 2U));
  EXPECT_EQ(task_run_param.args_descs[0U].placement, om2::ArgsPlacement::kArgsPlacementHbm);
}

// need_refresh 判定（FeatureMap 刷新门控）：仅 FeatureMap 刷新计入槽位使能，
// ModelIo 零拷贝刷新不计入（直连 HCCL 的模型 IO 走拷贝路径，直址语义）
TEST_F(UtestHcclTaskCodeBuilder, Init_NeedRefreshCountsFeatureMapOnly) {
  domi::TaskDef task_def = MakeHcclTaskDef();
  om2::RuntimeParam rts_param;
  om2::TaskRunParam task_run_param;
  ASSERT_EQ(builder_->ParseTaskRunParam(task_def, rts_param, MakeHcclOpDesc(), task_run_param), SUCCESS);

  std::vector<om2::MemAllocation> allocations(1U);
  allocations[0U].id = 0U;
  allocations[0U].logical_addr = 0U;
  allocations[0U].data_size = 0x100000U;
  allocations[0U].type = om2::MemAllocation::ABSOLUTE;
  const om2::PisToArgs args{};

  om2::AddrDesc model_io_addr{};
  model_io_addr.logic_addr = 0x1000U;
  model_io_addr.memory_type = static_cast<uint64_t>(om2::MemoryAppType::kMemoryTypeModelIo);
  model_io_addr.support_refresh = true;
  om2::IowAddrs model_io_iow{};
  model_io_iow.input_logic_addrs.push_back(model_io_addr);
  ASSERT_EQ(builder_->Init(task_def, allocations, args, model_io_iow), SUCCESS);
  EXPECT_EQ(builder_->build_data_.need_refresh, 0U);

  om2::AddrDesc fm_addr{};
  fm_addr.logic_addr = 0x2000U;
  fm_addr.memory_type = static_cast<uint64_t>(om2::MemoryAppType::kMemoryTypeFeatureMap);
  fm_addr.support_refresh = true;
  om2::IowAddrs fm_iow{};
  fm_iow.output_logic_addrs.push_back(fm_addr);
  ASSERT_EQ(builder_->Init(task_def, allocations, args, fm_iow), SUCCESS);
  EXPECT_EQ(builder_->build_data_.need_refresh, 1U);

  om2::AddrDesc fm_fixed_addr{};
  fm_fixed_addr.logic_addr = 0x3000U;
  fm_fixed_addr.memory_type = static_cast<uint64_t>(om2::MemoryAppType::kMemoryTypeFeatureMap);
  fm_fixed_addr.support_refresh = false;
  om2::IowAddrs fm_fixed_iow{};
  fm_fixed_iow.output_logic_addrs.push_back(fm_fixed_addr);
  ASSERT_EQ(builder_->Init(task_def, allocations, args, fm_fixed_iow), SUCCESS);
  EXPECT_EQ(builder_->build_data_.need_refresh, 0U);
}

static domi::TaskDef MakeHvdTaskDef(const std::string &hccl_type) {
  domi::TaskDef task_def;
  task_def.set_type(static_cast<int32_t>(ModelTaskType::MODEL_TASK_HCCL));
  task_def.set_stream_id(kTestStreamId);
  task_def.mutable_kernel_hccl()->set_op_index(kTestOpIndex);
  task_def.mutable_kernel_hccl()->set_hccl_type(hccl_type);
  task_def.set_private_def(std::string("\x12\x34\x56", 3U));
  return task_def;
}

TEST_F(UtestHcclTaskCodeBuilder, ParseHcclAttrs_HvdWaitOk) {
  auto task_def = MakeHvdTaskDef(HVDWAIT);
  auto op_desc = std::make_shared<OpDesc>("hvd_wait_op", HVDWAIT);
  EXPECT_EQ(builder_->ParseHcclAttrs(task_def, op_desc), SUCCESS);
  EXPECT_EQ(builder_->build_data_.is_hvd, true);
  EXPECT_EQ(builder_->build_data_.is_hvd_wait, true);
  EXPECT_EQ(builder_->build_data_.hvd_input_names.size(), 0U);
}

TEST_F(UtestHcclTaskCodeBuilder, ParseHcclAttrs_HvdAllReduceOk) {
  auto task_def = MakeHvdTaskDef(HVDCALLBACKALLREDUCE);
  auto op_desc = std::make_shared<OpDesc>("hvd_allreduce_op", HVDCALLBACKALLREDUCE);
  GeTensorDesc input_desc(GeShape({4, 8}), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(input_desc, 128);
  (void)op_desc->AddInputDesc(input_desc);
  (void)op_desc->AddOutputDesc(GeTensorDesc(GeShape({4, 8}), FORMAT_ND, DT_FLOAT));
  (void)AttrUtils::SetInt(op_desc, ATTR_HOROVOD_ATTR_REDUCE_TYPE, static_cast<int64_t>(HOROVOD_REDUCE_SUM));
  EXPECT_EQ(builder_->ParseHcclAttrs(task_def, op_desc), SUCCESS);
  {
    int64_t dbg_size = 0;
    (void)TensorUtils::GetSize(*op_desc->GetInputDescPtr(0U), dbg_size);
    EXPECT_EQ(dbg_size, 128);
  }
  EXPECT_EQ(builder_->build_data_.is_hvd, true);
  EXPECT_EQ(builder_->build_data_.is_hvd_wait, false);
  EXPECT_EQ(builder_->build_data_.is_collective, true);
  ASSERT_EQ(builder_->build_data_.hvd_input_names.size(), 1U);
  EXPECT_FALSE(builder_->build_data_.hvd_input_names[0U].empty());
  ASSERT_EQ(builder_->build_data_.hvd_dims.size(), 1U);
  EXPECT_EQ(builder_->build_data_.hvd_dims[0U], (std::vector<int64_t>{4, 8}));
  ASSERT_EQ(builder_->build_data_.hvd_dtypes.size(), 1U);
  EXPECT_EQ(builder_->build_data_.hvd_dtypes[0U], static_cast<int32_t>(HCCL_DATA_TYPE_FP32));
  ASSERT_EQ(builder_->build_data_.hvd_counts.size(), 1U);
  EXPECT_EQ(builder_->build_data_.hvd_counts[0U], 128U);
  EXPECT_EQ(builder_->build_data_.hccl_reduce_op, static_cast<int32_t>(HCCL_REDUCE_SUM));
}

TEST_F(UtestHcclTaskCodeBuilder, ParseHcclAttrs_ZeroCopyFlagSizeMismatchRejected) {
  auto task_def = MakeHcclTaskDef();
  task_def.mutable_kernel_hccl()->add_input_zero_copy_flag(1);
  task_def.mutable_kernel_hccl()->add_input_zero_copy_flag(1);
  auto op_desc = MakeHcclOpDesc();
  EXPECT_NE(builder_->ParseHcclAttrs(task_def, op_desc), SUCCESS);
}

TEST_F(UtestHcclTaskCodeBuilder, Contribute_NonWhitelistOpRejected) {
  domi::TaskDef task_def = MakeHcclTaskDef();
  task_def.mutable_kernel_hccl()->set_hccl_type("HcomUnknownOp");
  auto op_desc = std::make_shared<OpDesc>("unknown_op", "HcomUnknownOp");
  auto context = MakeContributeContext(task_def, op_desc);
  EXPECT_NE(builder_->Contribute(context), SUCCESS);
}

TEST_F(UtestHcclTaskCodeBuilder, RenderOpDefTableFields_HvdGroup) {
  builder_->build_data_.is_hvd = true;
  builder_->build_data_.is_hvd_wait = false;
  builder_->build_data_.hvd_input_names = {"input0"};
  builder_->build_data_.hvd_dims = {{4, 8}};
  builder_->build_data_.hvd_dtypes = {static_cast<int32_t>(HCCL_DATA_TYPE_FP32)};
  builder_->build_data_.hvd_counts = {32U};
  builder_->build_data_.is_collective = true;
  builder_->build_data_.input_zero_copy_flags = {0, 1};
  std::vector<std::pair<std::string, Arg>> fields;
  EXPECT_EQ(builder_->RenderOpDefTableFields(fields), SUCCESS);
  bool found_dispatch_info = false;
  for (const auto &field : fields) {
    if (field.first == "dispatch_info") {
      found_dispatch_info = true;
    }
  }
  EXPECT_TRUE(found_dispatch_info);
}

TEST_F(UtestHcclTaskCodeBuilder, RenderDistHelper_AndTableFields) {
  domi::TaskDef task_def = MakeHcclTaskDef();
  builder_->build_data_.is_hcom_op = true;
  builder_->build_data_.used_stream_num = 2;
  std::vector<DeclNode *> items;
  EXPECT_EQ(builder_->RenderDistHelper(items), SUCCESS);
  ASSERT_GE(items.size(), 1U);
  ASSERT_NE(items[0], nullptr);

  CppEmitter emitter;
  std::string dispatch_hccl_code;
  ASSERT_EQ(items[0]->Accept(emitter, dispatch_hccl_code), SUCCESS);
  EXPECT_NE(dispatch_hccl_code.find("ctx.hccl_load_task(ctx.instance_handle, &params)"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("ACL_RT_LAUNCH_HCCL"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("HcclTaskParams params"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("DispatchHccl op=%s, stream_id=%u"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("hccl params: type=%s, count=%llu"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("hccl.comm.workspace_size"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("hccl.comm.count"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("hccl.streams.attached_stream_num"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("op->op_name"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("params.need_refresh = op->dispatch_info.hccl.need_refresh"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("params.args_base = args_info->dev_addr"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("ctx.main_follow_stream_mapping[op->dispatch_info.hccl.stream_id]"),
            std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("params.follow_streams = pool.data()"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("params.follow_stream_num = need"), std::string::npos);
  // io 报告须经 BuildTensor 构造真实 gert::Tensor（禁止设备地址伪 cast 为指针，否则 dump 侧
  // SaveOpInfo/SaveTask/DumpOp 解引用伪指针 coredump）；args_base 对齐 kernel/custom 传设备地址
  EXPECT_NE(dispatch_hccl_code.find("hccl_io_tensors.push_back(BuildTensor("), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("&hccl_io_tensors.back()"), std::string::npos);
  // tensor 指针稳定性：reserve 必须先于 push_back（防扩容搬移悬空，对齐 kernel/custom/kernel_ex 同款模式）
  EXPECT_NE(dispatch_hccl_code.find("hccl_io_tensors.reserve("), std::string::npos);
  EXPECT_LT(dispatch_hccl_code.find("hccl_io_tensors.reserve("),
            dispatch_hccl_code.find("hccl_io_tensors.push_back(BuildTensor("));
  EXPECT_NE(dispatch_hccl_code.find("hccl_report_inputs.push_back(_entry)"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("hccl_report_outputs.push_back(_entry)"), std::string::npos);
  EXPECT_NE(dispatch_hccl_code.find("reinterpret_cast<uintptr_t>(args_info->dev_addr)"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("reinterpret_cast<gert::Tensor *>"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("reinterpret_cast<uintptr_t>(args_info->host_addr)"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("rtStreamCreateWithFlags"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("all_hccl_streams_"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("hccl_get_store"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("loaded_task"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("hccl_loaded_tasks_"), std::string::npos);
  EXPECT_EQ(dispatch_hccl_code.find("ops_store"), std::string::npos);

  std::vector<std::pair<std::string, Arg>> fields;
  EXPECT_EQ(builder_->RenderOpDefTableFields(fields), SUCCESS);
  ASSERT_GE(fields.size(), 2U);
  EXPECT_EQ(fields[0U].first, "dispatch_type");
  EXPECT_EQ(fields[1U].first, "op_name");
}

TEST_F(UtestHcclTaskCodeBuilder, RenderHvdMetasArg_CompoundLiteralMatchesAbiLayout) {
  builder_->build_data_.is_hvd = true;
  builder_->build_data_.is_hvd_wait = false;
  builder_->build_data_.hvd_input_names = {"x", "y"};
  builder_->build_data_.hvd_dims = {{4, 8}, {2}};
  builder_->build_data_.hvd_dtypes = {static_cast<int32_t>(HCCL_DATA_TYPE_FP32),
                                      static_cast<int32_t>(HCCL_DATA_TYPE_FP16)};
  builder_->build_data_.hvd_counts = {128U, 64U};

  const auto metas_arg = builder_->RenderHvdMetasArg();
  const auto *expr = metas_arg.Resolve(ctx_);
  ASSERT_NE(expr, nullptr);
  CppEmitter emitter;
  std::string output;
  ASSERT_EQ(expr->Accept(emitter, output), SUCCESS);
  EXPECT_NE(output.find("(const HcclHorovodMeta[])"), std::string::npos);
  EXPECT_NE(output.find("sizeof(HcclHorovodMeta)"), std::string::npos);
  EXPECT_NE(output.find("\"x\""), std::string::npos);
  EXPECT_NE(output.find("\"y\""), std::string::npos);
  EXPECT_NE(output.find("(const int64_t[]){4, 8}"), std::string::npos);
  EXPECT_NE(output.find("(const int64_t[]){2}"), std::string::npos);
  EXPECT_NE(output.find("0U"), std::string::npos);
  EXPECT_NE(output.find("128UL"), std::string::npos);
  EXPECT_NE(output.find("64UL"), std::string::npos);
}

TEST_F(UtestHcclTaskCodeBuilder, RenderHvdMetasArg_NonHvdReturnsNullptr) {
  CppEmitter emitter;
  std::string output;
  const auto emit_arg = [&](const Arg &arg) {
    output.clear();
    const auto *expr = arg.Resolve(ctx_);
    EXPECT_NE(expr, nullptr);
    if (expr != nullptr) {
      EXPECT_EQ(expr->Accept(emitter, output), SUCCESS);
    }
    return output;
  };

  builder_->build_data_.is_hvd = false;
  EXPECT_EQ(emit_arg(builder_->RenderHvdMetasArg()), "nullptr");

  builder_->build_data_.is_hvd = true;
  builder_->build_data_.is_hvd_wait = true;
  EXPECT_EQ(emit_arg(builder_->RenderHvdMetasArg()), "nullptr");
}
}  // namespace ge
