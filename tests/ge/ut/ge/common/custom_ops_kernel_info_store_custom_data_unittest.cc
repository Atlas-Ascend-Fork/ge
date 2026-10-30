/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// CustomOpsKernelBuilder 变长自定义数据（CustomDataBlock）相关用例。
// 从 custom_ops_kernel_info_store_unittest.cc 拆分而来：该文件在本特性之前就已超「超大源文件」阈值
// （nbnc 2071 > 2000），本特性新增用例会把它进一步推高（2243）而被门禁判为 degraded。
// 故新增用例独立成文件、原文件保持零改动；所需测试脚手架在本文件自包含（置于匿名 namespace，
// 与原文件的 ge::custom 同名实体互不冲突）。

#include <gtest/gtest.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engines/custom_engine/custom_graph_optimizer.h"
#include "engines/custom_engine/custom_ops_kernel_builder.h"
#include "engines/custom_engine/custom_ops_kernel_info_store.h"
#include "common/checker.h"
#include "common/ge_common/ge_types.h"
#include "exe_graph/runtime/annotated_args_context.h"
#include "graph/compute_graph.h"
#include "graph/custom_op_factory.h"
#include "graph/custom_op/args_refresh.h"
#include "graph/ascend_string.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_kernel_bin.h"
#include "graph/op_desc.h"
#include "graph/operator_reg.h"
#include "graph/custom_op.h"
#include "graph/ge_context.h"
#include "graph/ge_local_context.h"
#include "debug/ge_util.h"
#include "graph/utils/args_format_desc_utils.h"
#include "graph/args_format_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph/build/stream/declarative_stream_registry.h"
#include "graph/build/task_generator_utils.h"
#include "graph/utils/tensor_utils.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "securec.h"

namespace ge {
namespace custom {
namespace {
class MockPortableCustomOp : public PortableOp {
 public:
  graphStatus Serialize(std::vector<uint8_t> &buffer) override {
    static const uint8_t kBin[] = {0x51U, 0x52U};
    buffer.assign(kBin, kBin + sizeof(kBin));
    return GRAPH_SUCCESS;
  }

  graphStatus Deserialize(const std::vector<uint8_t> &buffer) override {
    (void)buffer;
    return GRAPH_SUCCESS;
  }
};

constexpr uintptr_t kLogicDataMemBase = 0x80000000UL;
constexpr uintptr_t kLogicWeightMemBase = 0x90000000UL;

uint64_t ReadUint64Slot(const std::string &args, const size_t index) {
  uint64_t value = 0U;
  EXPECT_EQ(memcpy_s(&value, sizeof(value), args.data() + (index * sizeof(value)), sizeof(value)), EOK);
  return value;
}

uint16_t ReadUint16Slot(const std::string &args, const size_t index) {
  uint16_t value = 0U;
  EXPECT_EQ(memcpy_s(&value, sizeof(value), args.data() + (index * sizeof(value)), sizeof(value)), EOK);
  return value;
}

class UtestCustomOpsKernelInfoStoreCustomData : public testing::Test {
 protected:
  void SetUp() override {
    graph_options_ = GetThreadLocalContext().GetAllGraphOptions();
    session_options_ = GetThreadLocalContext().GetAllSessionOptions();
    global_options_ = GetThreadLocalContext().GetAllGlobalOptions();
    // Mirror the real build flow: Kirin builds carry ge.offlineMode=40.
    GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "KirinX90"}, {ge::ir_option::OFFLINE_MODE, "40"}});
  }

  void TearDown() override {
    GetThreadLocalContext().SetGraphOption(graph_options_);
    GetThreadLocalContext().SetSessionOption(session_options_);
    GetThreadLocalContext().SetGlobalOption(global_options_);
  }

 private:
  std::map<std::string, std::string> graph_options_;
  std::map<std::string, std::string> session_options_;
  std::map<std::string, std::string> global_options_;
};

NodePtr BuildStaticCustomNode(const std::string &op_type, ComputeGraphPtr &graph) {
  graph = std::make_shared<ComputeGraph>("custom_builder_graph");
  auto op_desc = std::make_shared<OpDesc>("custom_builder_node", op_type);
  op_desc->SetId(7);
  op_desc->SetStreamId(3);
  op_desc->AppendIrInput("x", kIrInputRequired);
  op_desc->AppendIrOutput("y", kIrOutputRequired);

  GeTensorDesc input_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  input_desc.SetOriginShape(GeShape({1, 16}));
  GeTensorDesc output_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  output_desc.SetOriginShape(GeShape({1, 16}));
  (void)op_desc->AddInputDesc("x", input_desc);
  (void)op_desc->AddOutputDesc("y", output_desc);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({2048});
  auto node = graph->AddNode(op_desc);
  if (node != nullptr) {
    node->GetOpDesc()->SetId(7);
  }
  return node;
}

Status GenerateTaskForNode(const NodePtr &node, std::vector<domi::TaskDef> &tasks) {
  CustomOpsKernelBuilder builder;
  RunContext context = {};
  context.dataMemBase = reinterpret_cast<uint8_t *>(kLogicDataMemBase);
  context.dataMemSize = 4096U;
  context.weightMemBase = reinterpret_cast<uint8_t *>(kLogicWeightMemBase);
  context.weightMemSize = 4096U;
  return builder.GenerateTask(*node, context, tasks);
}

// 混合追加：输入地址 + 两个变长自定义数据（含 0x00 字节、非对齐长度）+ 输出地址 + 标量
class MockAnnotatedArgsWithCustomDataCustomOp : public AnnotatedArgsOp, public MockPortableCustomOp {
 public:
  static const uint8_t *FirstBlob() {
    static const uint8_t kFirstBlob[] = {0x01U, 0x00U, 0x02U, 0x00U, 0x03U};
    return kFirstBlob;
  }

  static const uint8_t *SecondBlob() {
    static const uint8_t kSecondBlob[] = {0xA5U, 0x5AU, 0xFFU};
    return kSecondBlob;
  }

  static constexpr size_t kFirstBlobSize = 5U;
  static constexpr size_t kSecondBlobSize = 3U;

  graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) override {
    static const uint8_t kBin[] = {0x11U, 0x22U, 0x33U, 0x44U};
    auto input = ctx.GetInputTensor(0U);
    auto output = ctx.GetOutputTensor(0U);
    GE_ASSERT_NOTNULL(input);
    GE_ASSERT_NOTNULL(output);
    gert::AnnotatedKernelArgs args(
        gert::InputAddr{0U, input->GetAddr()}, gert::CustomDataBlock{FirstBlob(), kFirstBlobSize},
        gert::OutputAddr{0U, output->GetAddr()}, gert::CustomDataBlock{SecondBlob(), kSecondBlobSize}, uint64_t{7U});
    return ctx.AddLaunch(
        gert::AnnotatedKernelLaunchInfo{"custom_add_kernel", kBin, sizeof(kBin), 8U, ctx.GetStreamId()},
        std::move(args));
  }
};

TEST_F(UtestCustomOpsKernelInfoStoreCustomData, GenerateTaskFillsCustomDataExtInfo) {
  GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "Ascend910B"}});
  const std::string kTestOpType = "TestAnnotatedArgsCustomDataCustomOp_BuilderTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> {
    return std::make_unique<MockAnnotatedArgsWithCustomDataCustomOp>();
  };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  ComputeGraphPtr graph;
  auto node = BuildStaticCustomNode(kTestOpType, graph);
  ASSERT_NE(node, nullptr);

  std::vector<domi::TaskDef> tasks;
  ASSERT_EQ(GenerateTaskForNode(node, tasks), SUCCESS);
  ASSERT_EQ(tasks.size(), 1U);

  const auto &kernel = tasks[0].kernel();
  // blob 按 desc 出现顺序裸拼接，无头部、无对齐填充
  const std::string expected_ext_info(
      reinterpret_cast<const char *>(MockAnnotatedArgsWithCustomDataCustomOp::FirstBlob()),
      MockAnnotatedArgsWithCustomDataCustomOp::kFirstBlobSize);
  const std::string expected_tail(reinterpret_cast<const char *>(MockAnnotatedArgsWithCustomDataCustomOp::SecondBlob()),
                                  MockAnnotatedArgsWithCustomDataCustomOp::kSecondBlobSize);
  EXPECT_EQ(kernel.kernel_ext_info(), expected_ext_info + expected_tail);
  EXPECT_EQ(kernel.kernel_ext_info_size(), 8U);
  EXPECT_EQ(kernel.kernel_ext_info().size(), kernel.kernel_ext_info_size());

  // 5 个 desc 仍为 5 * 8 字节，8B 槽不变式保持
  EXPECT_EQ(kernel.args_size(), 40U);
  ASSERT_EQ(kernel.args().size(), 40);
  // CUSTOM_DATA_BLOCK 槽（下标 1 与 3）为编译期 0 占位，标量槽为 7
  EXPECT_EQ(ReadUint64Slot(kernel.args(), 1U), 0U);
  EXPECT_EQ(ReadUint64Slot(kernel.args(), 3U), 0U);
  EXPECT_EQ(ReadUint64Slot(kernel.args(), 4U), 7U);
  EXPECT_NE(ReadUint64Slot(kernel.args(), 0U), 0U);
  EXPECT_NE(ReadUint64Slot(kernel.args(), 2U), 0U);

  const auto &kernel_context = kernel.context();
  EXPECT_EQ(kernel_context.args_count(), 5U);
  EXPECT_EQ(kernel_context.args_format(),
            "{i_instance0*}{custom_data_block0.5bytes}{o_instance0*}{custom_data_block1.3bytes}{#7}");
  // SetArgsOffset 逻辑未改，仍为 i * 8 平凡序列
  ASSERT_EQ(kernel_context.args_offset().size(), static_cast<int>(5U * sizeof(uint16_t)));
  for (size_t i = 0U; i < 5U; ++i) {
    EXPECT_EQ(ReadUint16Slot(kernel_context.args_offset(), i), static_cast<uint16_t>(i * 8U));
  }
}

// 与上一用例同源，独立校验 args_format 反序列化后 CUSTOM_DATA_BLOCK 的 reserved 等于原 blob size（而非 0）
TEST_F(UtestCustomOpsKernelInfoStoreCustomData, GenerateTaskCustomDataArgsFormatRoundTripKeepsReservedSize) {
  GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "Ascend910B"}});
  // op type 必须全二进制唯一：CustomOpFactory 重复注册同名 type 会失败
  const std::string kTestOpType = "TestAnnotatedArgsCustomDataRoundTripOp_BuilderTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> {
    return std::make_unique<MockAnnotatedArgsWithCustomDataCustomOp>();
  };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  ComputeGraphPtr graph;
  auto node = BuildStaticCustomNode(kTestOpType, graph);
  ASSERT_NE(node, nullptr);

  std::vector<domi::TaskDef> tasks;
  ASSERT_EQ(GenerateTaskForNode(node, tasks), SUCCESS);
  ASSERT_EQ(tasks.size(), 1U);

  const auto &kernel = tasks[0].kernel();
  const auto &kernel_context = kernel.context();
  // Parse 后 reserved 必须等于原 blob size（而非 0）
  std::vector<ArgDesc> parsed_arg_descs;
  ASSERT_EQ(ArgsFormatDesc::Parse(node->GetOpDesc(), kernel_context.args_format(), parsed_arg_descs), GRAPH_SUCCESS);
  ASSERT_EQ(parsed_arg_descs.size(), 5U);
  ASSERT_EQ(parsed_arg_descs[1].addr_type, AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(parsed_arg_descs[1].ir_idx, 0);
  EXPECT_FALSE(parsed_arg_descs[1].folded);
  uint64_t first_size = 0UL;
  (void)memcpy_s(&first_size, sizeof(first_size), parsed_arg_descs[1].reserved, sizeof(first_size));
  EXPECT_EQ(first_size, MockAnnotatedArgsWithCustomDataCustomOp::kFirstBlobSize);
  ASSERT_EQ(parsed_arg_descs[3].addr_type, AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(parsed_arg_descs[3].ir_idx, 1);
  uint64_t second_size = 0UL;
  (void)memcpy_s(&second_size, sizeof(second_size), parsed_arg_descs[3].reserved, sizeof(second_size));
  EXPECT_EQ(second_size, MockAnnotatedArgsWithCustomDataCustomOp::kSecondBlobSize);
}

// fixture 默认 SOC 为 KirinX90，即 mobile OMC 模式，必须在编译期明确拒绝而非静默产出不可执行的 .omc
TEST_F(UtestCustomOpsKernelInfoStoreCustomData, GenerateTaskRejectsCustomDataInMobileMode) {
  const std::string kTestOpType = "TestAnnotatedArgsCustomDataMobileCustomOp_BuilderTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> {
    return std::make_unique<MockAnnotatedArgsWithCustomDataCustomOp>();
  };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  ComputeGraphPtr graph;
  auto node = BuildStaticCustomNode(kTestOpType, graph);
  ASSERT_NE(node, nullptr);

  std::vector<domi::TaskDef> tasks;
  EXPECT_NE(GenerateTaskForNode(node, tasks), SUCCESS);
  EXPECT_TRUE(tasks.empty());
}

// 第二次 GenerateTask 命中 kAnnotatedArgsTaskPlanBytes 缓存，走 materialize 路径：
// CUSTOM_DATA_BLOCK 槽必须保持 0（不被误重填），kernel_ext_info 必须原样保留
TEST_F(UtestCustomOpsKernelInfoStoreCustomData, GenerateTaskMaterializesCachedPlanKeepingCustomDataSlots) {
  GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "Ascend910B"}});
  const std::string kTestOpType = "TestAnnotatedArgsCustomDataMaterializeCustomOp_BuilderTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> {
    return std::make_unique<MockAnnotatedArgsWithCustomDataCustomOp>();
  };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  ComputeGraphPtr graph;
  auto node = BuildStaticCustomNode(kTestOpType, graph);
  ASSERT_NE(node, nullptr);

  std::vector<domi::TaskDef> first_tasks;
  ASSERT_EQ(GenerateTaskForNode(node, first_tasks), SUCCESS);
  ASSERT_EQ(first_tasks.size(), 1U);

  // 缓存已写入，第二次生成应命中缓存并走 materialize
  Buffer encoded_plan;
  ASSERT_TRUE(AttrUtils::GetBytes(node->GetOpDesc(), "_custom_annotated_args_task_plan_bytes", encoded_plan));

  std::vector<domi::TaskDef> second_tasks;
  ASSERT_EQ(GenerateTaskForNode(node, second_tasks), SUCCESS);
  ASSERT_EQ(second_tasks.size(), 1U);

  const auto &first_kernel = first_tasks[0].kernel();
  const auto &second_kernel = second_tasks[0].kernel();
  EXPECT_EQ(second_kernel.kernel_ext_info(), first_kernel.kernel_ext_info());
  EXPECT_EQ(second_kernel.kernel_ext_info_size(), first_kernel.kernel_ext_info_size());
  EXPECT_EQ(second_kernel.context().args_format(), first_kernel.context().args_format());
  ASSERT_EQ(second_kernel.args().size(), first_kernel.args().size());
  // CUSTOM_DATA_BLOCK 槽仍为 0，IO 槽正确刷新（值与首次一致，因输入输出地址未变）
  EXPECT_EQ(ReadUint64Slot(second_kernel.args(), 1U), 0U);
  EXPECT_EQ(ReadUint64Slot(second_kernel.args(), 3U), 0U);
  EXPECT_EQ(ReadUint64Slot(second_kernel.args(), 4U), 7U);
  EXPECT_EQ(ReadUint64Slot(second_kernel.args(), 0U), ReadUint64Slot(first_kernel.args(), 0U));
  EXPECT_EQ(ReadUint64Slot(second_kernel.args(), 2U), ReadUint64Slot(first_kernel.args(), 2U));
}

// 同一 kernel 名以不同 blob 多次 AddLaunch：验证 blob 存于 TaskDef 而非以 kernel_name 为键的
// op attr，否则「同一 kernel 以不同 tiling 多次 launch」这一合理场景会因重名而无法区分 blob
class MockAnnotatedArgsMultiLaunchCustomDataOp : public AnnotatedArgsOp, public MockPortableCustomOp {
 public:
  graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) override {
    static const uint8_t kBin[] = {0x11U, 0x22U, 0x33U, 0x44U};
    static const uint8_t kFirstBlob[] = {0x11U, 0x11U, 0x11U, 0x11U};
    static const uint8_t kSecondBlob[] = {0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x22U};
    auto input = ctx.GetInputTensor(0U);
    auto output = ctx.GetOutputTensor(0U);
    GE_ASSERT_NOTNULL(input);
    GE_ASSERT_NOTNULL(output);
    const gert::AnnotatedKernelLaunchInfo launch_info{"same_kernel", kBin, sizeof(kBin), 8U, ctx.GetStreamId()};
    gert::AnnotatedKernelArgs first_args(gert::InputAddr{0U, input->GetAddr()},
                                         gert::CustomDataBlock{kFirstBlob, sizeof(kFirstBlob)},
                                         gert::OutputAddr{0U, output->GetAddr()});
    const auto first_status = ctx.AddLaunch(launch_info, std::move(first_args));
    if (first_status != ge::GRAPH_SUCCESS) {
      return first_status;
    }
    gert::AnnotatedKernelArgs second_args(gert::InputAddr{0U, input->GetAddr()},
                                          gert::CustomDataBlock{kSecondBlob, sizeof(kSecondBlob)},
                                          gert::OutputAddr{0U, output->GetAddr()});
    return ctx.AddLaunch(launch_info, std::move(second_args));
  }
};

TEST_F(UtestCustomOpsKernelInfoStoreCustomData, GenerateTaskKeepsCustomDataPerLaunchWithoutCrossTalk) {
  GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "Ascend910B"}});
  const std::string kTestOpType = "TestAnnotatedArgsMultiLaunchCustomDataOp_BuilderTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> {
    return std::make_unique<MockAnnotatedArgsMultiLaunchCustomDataOp>();
  };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  ComputeGraphPtr graph;
  auto node = BuildStaticCustomNode(kTestOpType, graph);
  ASSERT_NE(node, nullptr);

  std::vector<domi::TaskDef> tasks;
  ASSERT_EQ(GenerateTaskForNode(node, tasks), SUCCESS);
  ASSERT_EQ(tasks.size(), 2U);

  // 两次 launch 复用同一 kernel_name，但各自 TaskDef 的 blob 必须正确对应、无串数据
  EXPECT_EQ(tasks[0].kernel().kernel_name(), "same_kernel");
  EXPECT_EQ(tasks[1].kernel().kernel_name(), "same_kernel");
  const std::string first_blob(4, '\x11');
  const std::string second_blob(6, '\x22');
  EXPECT_EQ(tasks[0].kernel().kernel_ext_info(), first_blob);
  EXPECT_EQ(tasks[0].kernel().kernel_ext_info_size(), 4U);
  EXPECT_EQ(tasks[1].kernel().kernel_ext_info(), second_blob);
  EXPECT_EQ(tasks[1].kernel().kernel_ext_info_size(), 6U);
  // 两个 task 的 blob 序号均为 0（各自 launch 内从 0 递增），但编码的 size 不同
  EXPECT_EQ(tasks[0].kernel().context().args_format(), "{i_instance0*}{custom_data_block0.4bytes}{o_instance0*}");
  EXPECT_EQ(tasks[1].kernel().context().args_format(), "{i_instance0*}{custom_data_block0.6bytes}{o_instance0*}");
  // 各 task 的 CUSTOM_DATA_BLOCK 槽（下标 1）均为编译期 0 占位，且 IO 槽正确
  EXPECT_EQ(ReadUint64Slot(tasks[0].kernel().args(), 1U), 0U);
  EXPECT_EQ(ReadUint64Slot(tasks[1].kernel().args(), 1U), 0U);
  EXPECT_NE(ReadUint64Slot(tasks[0].kernel().args(), 0U), 0U);
  EXPECT_NE(ReadUint64Slot(tasks[1].kernel().args(), 2U), 0U);
}
}  // namespace
}  // namespace custom
}  // namespace ge
