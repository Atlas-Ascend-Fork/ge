/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "exe_graph/runtime/annotated_args_context.h"

#include <cstring>
#include <limits>

#include <gtest/gtest.h>
#include "exe_graph/lowering/kernel_run_context_builder.h"
#include "runtime/annotated_args_handler.h"
#include "faker/allocator_faker.h"
#include "graph/op_desc.h"
#include "graph/utils/args_format_desc_utils.h"

namespace gert {
namespace {
ge::OpDescPtr MakeOpDesc() {
  auto op_desc = std::make_shared<ge::OpDesc>("annotated_args_context_op", "AnnotatedArgsContextOp");
  const ge::GeTensorDesc tensor_desc(ge::GeShape({2, 2}), ge::FORMAT_ND, ge::DT_FLOAT16);
  (void)op_desc->AddInputDesc("x", tensor_desc);
  (void)op_desc->AddOutputDesc("y", tensor_desc);
  return op_desc;
}

constexpr uint32_t kCustomDataStreamId = 3U;

std::vector<uint8_t> MakeBlob(const uint8_t seed, const size_t size) {
  std::vector<uint8_t> blob(size);
  for (size_t i = 0UL; i < size; ++i) {
    blob[i] = static_cast<uint8_t>((seed + i) % 251U);
  }
  return blob;
}

// 校验 CUSTOM_DATA_BLOCK 的 desc 与 blob 一一对应：ir_idx 即 blob 下标、reserved 即字节大小，
// 且 args 仍满足「1 个 desc = 1 个 8 字节槽」不变式
void ExpectCustomDataDescsMatchBlobs(const AnnotatedArgsHandler::LaunchRecord *const launch,
                                     const std::vector<std::vector<uint8_t>> &expected,
                                     const std::vector<size_t> &sizes) {
  ASSERT_EQ(launch->GetArgDescCount(), expected.size() + 2U);
  const auto *const descs = launch->GetArgDescs();
  ASSERT_NE(descs, nullptr);
  for (size_t i = 0UL; i < expected.size(); ++i) {
    const auto &desc = descs[i + 1U];
    EXPECT_EQ(desc.addr_type, ge::AddrType::CUSTOM_DATA_BLOCK) << "desc index " << i;
    EXPECT_EQ(desc.ir_idx, static_cast<int32_t>(i)) << "desc index " << i;
    uint64_t size = 0UL;
    (void)memcpy_s(&size, sizeof(size), desc.reserved, sizeof(size));
    EXPECT_EQ(size, sizes[i]) << "desc index " << i;
  }
  EXPECT_EQ(launch->GetArgsSize(), (expected.size() + 2U) * sizeof(uint64_t));
}

// 变长数据字节只能通过 AddLaunch 落入 LaunchRecord 后读取（AnnotatedKernelArgs 不再对外提供
// 输出 blob 的公开接口），故 blob 内容类断言集中在本文件，走真实数据流
TEST(AnnotatedArgsContextUT, RecordsCustomDataBlobsThroughAddLaunch) {
  AllocatorFaker allocator;
  allocator.SetStreamId(kCustomDataStreamId);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  // 含 0x00 字节、非对齐长度与 >4KB，且与输入/输出/标量参数混排
  const std::vector<size_t> sizes = {1U, 3U, 4095U, 4097U};
  std::vector<std::vector<uint8_t>> expected;
  AnnotatedKernelArgs args(InputAddr{0U, input_tensor.GetAddr()});
  for (const auto size : sizes) {
    auto blob = MakeBlob(static_cast<uint8_t>(size), size);
    blob[size / 2U] = 0U;  // 掺入 0x00，确保不按 C 字符串语义处理
    ASSERT_EQ(args.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
    expected.emplace_back(std::move(blob));
  }
  ASSERT_EQ(args.AppendArg(OutputAddr{0U, output_tensor.GetAddr()}), ge::GRAPH_SUCCESS);

  uint8_t bin[] = {0x11U};
  ASSERT_EQ(
      context->AddLaunch(AnnotatedKernelLaunchInfo{"custom_data_kernel", bin, sizeof(bin), 1U, kCustomDataStreamId},
                         std::move(args)),
      ge::GRAPH_SUCCESS);

  ASSERT_EQ(args_handler.GetLaunchCount(), 1U);
  const auto *launch = args_handler.GetLaunch(0U);
  ASSERT_NE(launch, nullptr);
  ASSERT_EQ(launch->GetCustomDataBlobCount(), expected.size());
  for (size_t i = 0UL; i < expected.size(); ++i) {
    const auto *const blob = launch->GetCustomDataBlob(i);
    ASSERT_NE(blob, nullptr) << "blob index " << i;
    EXPECT_EQ(*blob, expected[i]) << "blob index " << i;
    EXPECT_EQ(blob->size(), sizes[i]) << "blob index " << i;
  }
  EXPECT_EQ(launch->GetCustomDataBlob(expected.size()), nullptr);

  ExpectCustomDataDescsMatchBlobs(launch, expected, sizes);
}

TEST(AnnotatedArgsContextUT, CustomDataBlobIsDeepCopiedBeforeAddLaunch) {
  AllocatorFaker allocator;
  allocator.SetStreamId(kCustomDataStreamId);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  auto source = MakeBlob(1U, 64U);
  const auto snapshot = source;
  AnnotatedKernelArgs args;
  ASSERT_EQ(args.AppendArg(CustomDataBlock{source.data(), source.size()}), ge::GRAPH_SUCCESS);
  // 追加后立即改写并释放调用方缓冲区，AddLaunch 后内容应不受影响
  for (auto &byte : source) {
    byte = 0xA5U;
  }
  source.clear();
  source.shrink_to_fit();

  uint8_t bin[] = {0x11U};
  ASSERT_EQ(context->AddLaunch(AnnotatedKernelLaunchInfo{"deep_copy_kernel", bin, sizeof(bin), 1U, kCustomDataStreamId},
                               std::move(args)),
            ge::GRAPH_SUCCESS);
  const auto *launch = args_handler.GetLaunch(0U);
  ASSERT_NE(launch, nullptr);
  ASSERT_EQ(launch->GetCustomDataBlobCount(), 1U);
  const auto *const blob = launch->GetCustomDataBlob(0U);
  ASSERT_NE(blob, nullptr);
  EXPECT_EQ(*blob, snapshot);
}

TEST(AnnotatedArgsContextUT, CustomDataBlobsSurviveBuilderCopyAndMove) {
  AllocatorFaker allocator;
  allocator.SetStreamId(kCustomDataStreamId);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  const auto first = MakeBlob(2U, 16U);
  const auto second = MakeBlob(3U, 24U);
  AnnotatedKernelArgs source;
  ASSERT_EQ(source.AppendArg(CustomDataBlock{first.data(), first.size()}), ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs copied(source);
  ASSERT_EQ(copied.AppendArg(CustomDataBlock{second.data(), second.size()}), ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs moved(std::move(copied));

  uint8_t bin[] = {0x11U};
  // 移动后的 builder 提交：两个 blob 均应完整转移
  ASSERT_EQ(context->AddLaunch(AnnotatedKernelLaunchInfo{"moved_kernel", bin, sizeof(bin), 1U, kCustomDataStreamId},
                               std::move(moved)),
            ge::GRAPH_SUCCESS);
  // 拷贝赋值得到的 builder 提交：仅保留源 builder 的一个 blob
  AnnotatedKernelArgs copy_assigned;
  copy_assigned = source;
  ASSERT_EQ(context->AddLaunch(AnnotatedKernelLaunchInfo{"copied_kernel", bin, sizeof(bin), 1U, kCustomDataStreamId},
                               std::move(copy_assigned)),
            ge::GRAPH_SUCCESS);

  ASSERT_EQ(args_handler.GetLaunchCount(), 2U);
  const auto *moved_launch = args_handler.GetLaunch(0U);
  const auto *copied_launch = args_handler.GetLaunch(1U);
  ASSERT_NE(moved_launch, nullptr);
  ASSERT_NE(copied_launch, nullptr);
  ASSERT_EQ(moved_launch->GetCustomDataBlobCount(), 2U);
  ASSERT_NE(moved_launch->GetCustomDataBlob(0U), nullptr);
  ASSERT_NE(moved_launch->GetCustomDataBlob(1U), nullptr);
  EXPECT_EQ(*moved_launch->GetCustomDataBlob(0U), first);
  EXPECT_EQ(*moved_launch->GetCustomDataBlob(1U), second);
  ASSERT_EQ(copied_launch->GetCustomDataBlobCount(), 1U);
  ASSERT_NE(copied_launch->GetCustomDataBlob(0U), nullptr);
  EXPECT_EQ(*copied_launch->GetCustomDataBlob(0U), first);
}

// 同一 kernel 名以不同 blob 多次 AddLaunch：各 LaunchRecord 的 blob 必须正确对应、无串数据
TEST(AnnotatedArgsContextUT, MultipleLaunchesKeepCustomDataBlobsSeparated) {
  AllocatorFaker allocator;
  allocator.SetStreamId(kCustomDataStreamId);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  const auto first = MakeBlob(4U, 5U);
  const auto second = MakeBlob(5U, 9U);
  uint8_t bin[] = {0x11U};
  const AnnotatedKernelLaunchInfo launch_info{"same_kernel", bin, sizeof(bin), 1U, kCustomDataStreamId};
  ASSERT_EQ(context->AddLaunch(launch_info, AnnotatedKernelArgs(CustomDataBlock{first.data(), first.size()})),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(context->AddLaunch(launch_info, AnnotatedKernelArgs(CustomDataBlock{second.data(), second.size()})),
            ge::GRAPH_SUCCESS);

  ASSERT_EQ(args_handler.GetLaunchCount(), 2U);
  const auto *first_launch = args_handler.GetLaunch(0U);
  const auto *second_launch = args_handler.GetLaunch(1U);
  ASSERT_NE(first_launch, nullptr);
  ASSERT_NE(second_launch, nullptr);
  EXPECT_STREQ(first_launch->GetKernelName(), "same_kernel");
  EXPECT_STREQ(second_launch->GetKernelName(), "same_kernel");
  ASSERT_EQ(first_launch->GetCustomDataBlobCount(), 1U);
  ASSERT_EQ(second_launch->GetCustomDataBlobCount(), 1U);
  ASSERT_NE(first_launch->GetCustomDataBlob(0U), nullptr);
  ASSERT_NE(second_launch->GetCustomDataBlob(0U), nullptr);
  EXPECT_EQ(*first_launch->GetCustomDataBlob(0U), first);
  EXPECT_EQ(*second_launch->GetCustomDataBlob(0U), second);
}

TEST(AnnotatedArgsContextUT, AllocatesWorkspaceAndRecordsLaunchTask) {
  AllocatorFaker allocator;
  uint32_t stream_id = 3U;
  allocator.SetStreamId(stream_id);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;

  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  EXPECT_EQ(context->GetInputTensor(0U), &input_tensor);
  EXPECT_EQ(context->GetInputTensor(1U), nullptr);
  EXPECT_EQ(context->GetInputTensor(std::numeric_limits<size_t>::max()), nullptr);
  EXPECT_EQ(context->GetOutputTensor(0U), &output_tensor);
  EXPECT_EQ(context->GetOutputTensor(1U), nullptr);
  EXPECT_EQ(context->GetStreamId(), stream_id);

  const auto workspace = context->MallocWorkSpace(64U);
  ASSERT_NE(workspace.addr, nullptr);
  EXPECT_EQ(workspace.index, 0U);
  ASSERT_EQ(workspace_mems.size(), 1U);
  EXPECT_EQ(workspace_mems[0]->GetAddr(), workspace.addr);

  uint8_t bin[] = {0x11U, 0x22U};
  constexpr uint64_t kCustomValue = 7U;
  AnnotatedKernelArgs args(InputAddr{0U, input_tensor.GetAddr()}, OutputAddr{0U, output_tensor.GetAddr()}, workspace,
                           kCustomValue);
  AnnotatedKernelLaunchInfo launch_info{"annotated_args_context_kernel", bin, sizeof(bin), 8U, stream_id};
  EXPECT_EQ(context->AddLaunch(launch_info, std::move(args)), ge::GRAPH_SUCCESS);
  bin[0] = 0xFFU;
  ASSERT_EQ(args_handler.GetLaunchCount(), 1U);
  const auto *task = args_handler.GetLaunch(0U);
  ASSERT_NE(task, nullptr);
  EXPECT_STREQ(task->GetKernelName(), "annotated_args_context_kernel");
  EXPECT_EQ(task->GetKernelBinSize(), sizeof(bin));
  ASSERT_NE(task->GetKernelBinData(), nullptr);
  EXPECT_EQ(task->GetKernelBinData()[0], 0x11U);
  EXPECT_EQ(task->GetBlockDim(), 8U);
  EXPECT_EQ(task->GetStreamId(), stream_id);
  EXPECT_EQ(task->GetArgsSize(), 4U * sizeof(uint64_t));
  ASSERT_EQ(task->GetArgDescCount(), 4U);
  ASSERT_NE(task->GetArgDescs(), nullptr);
  EXPECT_EQ(task->GetArgDescs()[0].addr_type, ge::AddrType::INPUT_INSTANCE);
  EXPECT_EQ(task->GetArgDescs()[1].addr_type, ge::AddrType::OUTPUT_INSTANCE);
  EXPECT_EQ(task->GetArgDescs()[2].addr_type, ge::AddrType::WORKSPACE);
  EXPECT_EQ(task->GetArgDescs()[3].addr_type, ge::AddrType::CUSTOM_VALUE);

  for (auto *mem_block : workspace_mems) {
    mem_block->Free(0);
  }
  workspace_mems.clear();
}

TEST(AnnotatedArgsContextUT, RejectsInvalidLaunchParameters) {
  AllocatorFaker allocator;
  uint32_t stream_id = 3U;
  allocator.SetStreamId(stream_id);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;

  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  const uint8_t bin[] = {0x11U};
  auto make_args = []() { return AnnotatedKernelArgs(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}); };
  EXPECT_NE(context->AddLaunch(AnnotatedKernelLaunchInfo{nullptr, bin, sizeof(bin), 8U, stream_id}, make_args()),
            ge::GRAPH_SUCCESS);
  EXPECT_NE(context->AddLaunch(AnnotatedKernelLaunchInfo{"", bin, sizeof(bin), 8U, stream_id}, make_args()),
            ge::GRAPH_SUCCESS);
  EXPECT_NE(context->AddLaunch(AnnotatedKernelLaunchInfo{"kernel", nullptr, sizeof(bin), 8U, stream_id}, make_args()),
            ge::GRAPH_SUCCESS);
  EXPECT_NE(context->AddLaunch(AnnotatedKernelLaunchInfo{"kernel", bin, 0U, 8U, stream_id}, make_args()),
            ge::GRAPH_SUCCESS);
  EXPECT_NE(context->AddLaunch(AnnotatedKernelLaunchInfo{"kernel", bin, sizeof(bin), 0U, stream_id}, make_args()),
            ge::GRAPH_SUCCESS);
  EXPECT_NE(
      context->AddLaunch(
          AnnotatedKernelLaunchInfo{"kernel", bin, sizeof(bin), 8U, std::numeric_limits<uint32_t>::max()}, make_args()),
      ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs empty_args;
  EXPECT_NE(
      context->AddLaunch(AnnotatedKernelLaunchInfo{"kernel", bin, sizeof(bin), 8U, stream_id}, std::move(empty_args)),
      ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs invalid_args(
      InputAddr{static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U, reinterpret_cast<void *>(0x1000U)});
  EXPECT_NE(
      context->AddLaunch(AnnotatedKernelLaunchInfo{"kernel", bin, sizeof(bin), 8U, stream_id}, std::move(invalid_args)),
      ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_handler.GetLaunchCount(), 0U);
}

TEST(AnnotatedArgsContextUT, AddLaunchWithTokenValidatesAndCopiesDependencies) {
  AllocatorFaker allocator;
  constexpr uint32_t kStreamId = 3U;
  allocator.SetStreamId(kStreamId);
  Tensor input_tensor;
  Tensor output_tensor;
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  const uint8_t bin[] = {0x11U};
  auto make_args = []() { return AnnotatedKernelArgs(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}); };

  const auto token_a =
      context->AddLaunch(AnnotatedKernelLaunchInfo{"a", bin, sizeof(bin), 1U, kStreamId}, make_args(), {});
  ASSERT_NE(token_a, std::numeric_limits<AnnotatedLaunchToken>::max());
  std::vector<AnnotatedLaunchToken> predecessors{token_a};
  const auto token_b =
      context->AddLaunch(AnnotatedKernelLaunchInfo{"b", bin, sizeof(bin), 1U, kStreamId}, make_args(), predecessors);
  ASSERT_NE(token_b, std::numeric_limits<AnnotatedLaunchToken>::max());
  predecessors[0] = std::numeric_limits<AnnotatedLaunchToken>::max();
  ASSERT_EQ(args_handler.GetLaunchCount(), 2U);
  EXPECT_EQ(args_handler.GetLaunch(1U)->GetToken(), token_b);
  ASSERT_EQ(args_handler.GetLaunch(1U)->GetDependencyCount(), 1U);
  EXPECT_EQ(args_handler.GetLaunch(1U)->GetDependencies()[0], token_a);

  const AnnotatedKernelLaunchInfo invalid_info{"c", bin, sizeof(bin), 1U, kStreamId};
  EXPECT_EQ(context->AddLaunch(invalid_info, make_args(), {2U}), std::numeric_limits<AnnotatedLaunchToken>::max());
  EXPECT_EQ(context->AddLaunch(invalid_info, make_args(), {3U}), std::numeric_limits<AnnotatedLaunchToken>::max());
  EXPECT_EQ(context->AddLaunch(invalid_info, make_args(), {token_a, token_a}),
            std::numeric_limits<AnnotatedLaunchToken>::max());
  const AnnotatedKernelLaunchInfo bad_name_info{nullptr, bin, sizeof(bin), 1U, kStreamId};
  EXPECT_EQ(context->AddLaunch(bad_name_info, make_args(), {}), std::numeric_limits<AnnotatedLaunchToken>::max());
  EXPECT_EQ(args_handler.GetLaunchCount(), 2U);
}

TEST(AnnotatedArgsContextUT, RecordsMultipleLaunchesInOrder) {
  AllocatorFaker allocator;
  constexpr uint32_t kStreamId = 3U;
  allocator.SetStreamId(kStreamId);
  Tensor input_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                      reinterpret_cast<void *>(0x1000U));
  Tensor output_tensor({{2, 2}, {2, 2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnDeviceHbm, ge::DT_FLOAT16,
                       reinterpret_cast<void *>(0x2000U));
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;

  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  uint8_t first_bin[] = {0x11U};
  uint8_t second_bin[] = {0x22U};
  EXPECT_EQ(context->AddLaunch(AnnotatedKernelLaunchInfo{"first_kernel", first_bin, sizeof(first_bin), 1U, kStreamId},
                               AnnotatedKernelArgs(InputAddr{0U, input_tensor.GetAddr()})),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(
      context->AddLaunch(AnnotatedKernelLaunchInfo{"second_kernel", second_bin, sizeof(second_bin), 2U, kStreamId},
                         AnnotatedKernelArgs(OutputAddr{0U, output_tensor.GetAddr()})),
      ge::GRAPH_SUCCESS);
  first_bin[0] = 0xFFU;
  second_bin[0] = 0xFFU;

  ASSERT_EQ(args_handler.GetLaunchCount(), 2U);
  const auto *first_launch = args_handler.GetLaunch(0U);
  const auto *second_launch = args_handler.GetLaunch(1U);
  ASSERT_NE(first_launch, nullptr);
  ASSERT_NE(second_launch, nullptr);
  EXPECT_EQ(args_handler.GetLaunch(2U), nullptr);
  EXPECT_STREQ(first_launch->GetKernelName(), "first_kernel");
  EXPECT_STREQ(second_launch->GetKernelName(), "second_kernel");
  EXPECT_EQ(first_launch->GetKernelBinData()[0], 0x11U);
  EXPECT_EQ(second_launch->GetKernelBinData()[0], 0x22U);
  EXPECT_EQ(first_launch->GetBlockDim(), 1U);
  EXPECT_EQ(second_launch->GetBlockDim(), 2U);
  EXPECT_EQ(first_launch->GetArgDescs()[0].addr_type, ge::AddrType::INPUT_INSTANCE);
  EXPECT_EQ(second_launch->GetArgDescs()[0].addr_type, ge::AddrType::OUTPUT_INSTANCE);
}

TEST(AnnotatedArgsHandlerUT, RuntimeArgsInterfacesAreEmpty) {
  AnnotatedArgsHandler args_handler;

  EXPECT_EQ(args_handler.MallocReadOnlyDevArgs(nullptr, 0U), nullptr);
  EXPECT_TRUE(args_handler.GetKernelArgs(ge::kPlacementHost).empty());
  EXPECT_TRUE(args_handler.GetKernelArgs(ge::kPlacementDevice).empty());
}

TEST(AnnotatedArgsHandlerUT, DoesNotProvideAttachedStream) {
  AnnotatedArgsHandler args_handler;

  EXPECT_EQ(args_handler.GetAttachedStreamProvider(), nullptr);
}

TEST(AnnotatedArgsContextUT, ReturnsDefaultValuesWhenComputeNodeInfoIsMissing) {
  AnnotatedArgsContext context{};

  EXPECT_EQ(context.GetStreamId(), std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(context.GetInputTensor(0U), nullptr);
  EXPECT_EQ(context.GetOutputTensor(0U), nullptr);
}

TEST(AnnotatedArgsContextUT, ReturnsInvalidStreamIdWhenWorkspaceAllocatorIsMissing) {
  Tensor input_tensor;
  Tensor output_tensor;
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, nullptr})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  EXPECT_EQ(context->GetStreamId(), std::numeric_limits<uint32_t>::max());
}

TEST(AnnotatedArgsContextUT, HandlesAllocatorStreamIdBoundaryValues) {
  AllocatorFaker allocator;
  Tensor input_tensor;
  Tensor output_tensor;
  std::vector<GertMemBlock *> workspace_mems;
  AnnotatedArgsHandler args_handler;
  auto holder = KernelRunContextBuilder()
                    .Inputs({&input_tensor, &allocator})
                    .Outputs({&output_tensor, &workspace_mems, static_cast<ArgsHandler *>(&args_handler)})
                    .Build(MakeOpDesc());
  auto *context = reinterpret_cast<AnnotatedArgsContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  allocator.SetStreamId(0);
  EXPECT_EQ(context->GetStreamId(), 0U);
  allocator.SetStreamId(static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) - 1);
  EXPECT_EQ(context->GetStreamId(), std::numeric_limits<uint32_t>::max() - 1U);
  allocator.SetStreamId(-1);
  EXPECT_EQ(context->GetStreamId(), std::numeric_limits<uint32_t>::max());
  allocator.SetStreamId(static_cast<int64_t>(std::numeric_limits<uint32_t>::max()));
  EXPECT_EQ(context->GetStreamId(), std::numeric_limits<uint32_t>::max());
}
}  // namespace
}  // namespace gert
