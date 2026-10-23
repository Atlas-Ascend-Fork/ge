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
#include <type_traits>
#include <utility>
#include <vector>

#include "graph/utils/args_format_desc_utils.h"
#include "securec.h"

#include <gtest/gtest.h>

namespace gert {
namespace {
template <typename T, typename = void>
struct HasGetStatus : std::false_type {};

template <typename T>
struct HasGetStatus<T, decltype(static_cast<void>(std::declval<const T &>().GetStatus()))> : std::true_type {};

template <typename T, typename = void>
struct HasGetImpl : std::false_type {};

template <typename T>
struct HasGetImpl<T, decltype(static_cast<void>(std::declval<const T &>().GetImpl()))> : std::true_type {};

static_assert(!HasGetStatus<AnnotatedKernelArgs>::value, "AnnotatedKernelArgs must not expose GetStatus");
static_assert(!HasGetImpl<AnnotatedKernelArgs>::value, "AnnotatedKernelArgs must not expose GetImpl");
// 新增变长自定义数据能力不得改变公共头 ABI：AnnotatedKernelArgs 仍为单 pImpl 指针结构
static_assert(sizeof(AnnotatedKernelArgs) == sizeof(void *), "AnnotatedKernelArgs must remain a single pImpl pointer");

uint64_t ReadReserved(const ge::ArgDesc &arg_desc) {
  uint64_t value = 0UL;
  (void)memcpy_s(&value, sizeof(value), arg_desc.reserved, sizeof(value));
  return value;
}

uint64_t ReadSlot(const std::vector<uint8_t> &args_data, const size_t index) {
  uint64_t value = 0UL;
  (void)memcpy_s(&value, sizeof(value), args_data.data() + (index * sizeof(uint64_t)), sizeof(value));
  return value;
}

std::vector<uint8_t> MakeBlob(const uint8_t seed, const size_t size) {
  std::vector<uint8_t> blob(size);
  for (size_t i = 0UL; i < size; ++i) {
    blob[i] = static_cast<uint8_t>((seed + i) % 251U);
  }
  return blob;
}

TEST(AnnotatedKernelArgsUT, ExtractsInstanceInputAndOutputArgs) {
  const AnnotatedKernelArgs args(InputAddr{2U, reinterpret_cast<void *>(0x1000U)},
                                 OutputAddr{1U, reinterpret_cast<void *>(0x2000U)});
  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;

  ASSERT_EQ(args.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  EXPECT_EQ(args_data.size(), 2U * sizeof(uint64_t));
  ASSERT_EQ(arg_descs.size(), 2U);
  EXPECT_EQ(arg_descs[0].addr_type, ge::AddrType::INPUT_INSTANCE);
  EXPECT_EQ(arg_descs[0].ir_idx, 2);
  EXPECT_EQ(arg_descs[1].addr_type, ge::AddrType::OUTPUT_INSTANCE);
  EXPECT_EQ(arg_descs[1].ir_idx, 1);
}

TEST(AnnotatedKernelArgsUT, CopiesAndMovesRemainUsable) {
  AnnotatedKernelArgs source(InputAddr{0U, reinterpret_cast<void *>(0x1000U)},
                             OutputAddr{0U, reinterpret_cast<void *>(0x2000U)},
                             WorkspaceAddr{0U, reinterpret_cast<void *>(0x3000U)}, uint64_t{7U});
  EXPECT_EQ(source.AppendArg(uint64_t{8U}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs copied(source);
  EXPECT_EQ(copied.AppendArg(InputAddr{1U, reinterpret_cast<void *>(0x4000U)}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs moved(std::move(copied));
  EXPECT_EQ(moved.AppendArg(OutputAddr{1U, reinterpret_cast<void *>(0x5000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(copied.AppendArg(uint64_t{9U}), ge::GRAPH_SUCCESS);
}

TEST(AnnotatedKernelArgsUT, RejectsIndexesAboveInt32Max) {
  constexpr uint32_t kInvalidIndex = static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U;
  AnnotatedKernelArgs invalid_input;
  AnnotatedKernelArgs invalid_output;
  AnnotatedKernelArgs invalid_workspace;

  EXPECT_NE(invalid_input.AppendArg(InputAddr{kInvalidIndex, reinterpret_cast<void *>(0x1000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(invalid_output.AppendArg(OutputAddr{kInvalidIndex, reinterpret_cast<void *>(0x2000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(invalid_workspace.AppendArg(WorkspaceAddr{kInvalidIndex, reinterpret_cast<void *>(0x3000U)}),
            ge::GRAPH_SUCCESS);
}

TEST(AnnotatedKernelArgsUT, AssignmentsPreserveUsabilityAndRejectMovedFromObject) {
  AnnotatedKernelArgs source(InputAddr{0U, reinterpret_cast<void *>(0x1000U)},
                             WorkspaceAddr{0U, reinterpret_cast<void *>(0x3000U)});
  AnnotatedKernelArgs copied;
  copied = source;
  EXPECT_EQ(copied.AppendArg(uint64_t{1U}), ge::GRAPH_SUCCESS);

  copied = copied;
  EXPECT_EQ(copied.AppendArg(uint64_t{2U}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs move_target(OutputAddr{0U, reinterpret_cast<void *>(0x2000U)});
  move_target = std::move(copied);
  EXPECT_EQ(move_target.AppendArg(uint64_t{3U}), ge::GRAPH_SUCCESS);
  EXPECT_NE(copied.AppendArg(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(copied.AppendArg(OutputAddr{0U, reinterpret_cast<void *>(0x2000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(copied.AppendArg(WorkspaceAddr{0U, reinterpret_cast<void *>(0x3000U)}), ge::GRAPH_SUCCESS);
  EXPECT_NE(copied.AppendArg(uint64_t{1U}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs copied_from_moved(copied);
  EXPECT_NE(copied_from_moved.AppendArg(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}), ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs assigned_from_moved(InputAddr{0U, reinterpret_cast<void *>(0x1000U)});
  assigned_from_moved = copied;
  EXPECT_NE(assigned_from_moved.AppendArg(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs self_move(InputAddr{0U, reinterpret_cast<void *>(0x1000U)});
  self_move = std::move(self_move);
  EXPECT_EQ(self_move.AppendArg(uint64_t{4U}), ge::GRAPH_SUCCESS);
}

// 变长自定义数据：每个 blob 占一个值为 0 的 8 字节槽，desc 的 ir_idx 为 blob 序号、reserved 为字节大小
// （blob 字节内容的校验需经 AddLaunch 读 LaunchRecord，见 annotated_args_context_unittest.cc）
TEST(AnnotatedKernelArgsUT, AppendsCustomDataSlotsAndDescs) {
  const auto first = MakeBlob(0U, 1024U);
  const auto second = MakeBlob(7U, 3U);
  AnnotatedKernelArgs args(
      InputAddr{0U, reinterpret_cast<void *>(0x1000U)}, CustomDataBlock{first.data(), first.size()},
      OutputAddr{0U, reinterpret_cast<void *>(0x2000U)}, CustomDataBlock{second.data(), second.size()});

  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;
  ASSERT_EQ(args.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);

  // 8 字节槽不变式：4 个 desc 对应 4 * 8 字节
  ASSERT_EQ(arg_descs.size(), 4U);
  EXPECT_EQ(args_data.size(), 4U * sizeof(uint64_t));

  EXPECT_EQ(arg_descs[0].addr_type, ge::AddrType::INPUT_INSTANCE);
  EXPECT_EQ(ReadSlot(args_data, 0U), 0x1000U);

  ASSERT_EQ(arg_descs[1].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(arg_descs[1].ir_idx, 0);
  EXPECT_FALSE(arg_descs[1].folded);
  EXPECT_EQ(ReadReserved(arg_descs[1]), 1024UL);
  // 编译期为 0 占位，运行期才填入真实 device 地址
  EXPECT_EQ(ReadSlot(args_data, 1U), 0UL);

  EXPECT_EQ(arg_descs[2].addr_type, ge::AddrType::OUTPUT_INSTANCE);
  EXPECT_EQ(ReadSlot(args_data, 2U), 0x2000U);

  ASSERT_EQ(arg_descs[3].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(arg_descs[3].ir_idx, 1);
  EXPECT_FALSE(arg_descs[3].folded);
  EXPECT_EQ(ReadReserved(arg_descs[3]), 3UL);
  EXPECT_EQ(ReadSlot(args_data, 3U), 0UL);
}

// 非对齐长度与超 4KB 的大小均能完整编码进 desc 的 reserved
TEST(AnnotatedKernelArgsUT, CustomDataDescsCarryArbitrarySizes) {
  const std::vector<size_t> sizes = {1U, 3U, 7U, 4095U, 4096U, 4097U};
  AnnotatedKernelArgs args;
  for (const auto size : sizes) {
    const auto blob = MakeBlob(static_cast<uint8_t>(size), size);
    ASSERT_EQ(args.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
  }

  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;
  ASSERT_EQ(args.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), sizes.size());
  EXPECT_EQ(args_data.size(), sizes.size() * sizeof(uint64_t));
  for (size_t i = 0U; i < sizes.size(); ++i) {
    EXPECT_EQ(arg_descs[i].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK) << "desc index " << i;
    EXPECT_EQ(arg_descs[i].ir_idx, static_cast<int32_t>(i)) << "desc index " << i;
    EXPECT_FALSE(arg_descs[i].folded) << "desc index " << i;
    EXPECT_EQ(ReadReserved(arg_descs[i]), sizes[i]) << "desc index " << i;
    EXPECT_EQ(ReadSlot(args_data, i), 0UL) << "slot index " << i;
  }
}

// 拷贝/移动后 desc（含 blob 序号与大小）正确转移
TEST(AnnotatedKernelArgsUT, CustomDataDescsSurviveCopyAndMove) {
  const auto first = MakeBlob(2U, 16U);
  const auto second = MakeBlob(3U, 24U);

  AnnotatedKernelArgs source;
  ASSERT_EQ(source.AppendArg(CustomDataBlock{first.data(), first.size()}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs copied(source);
  ASSERT_EQ(copied.AppendArg(CustomDataBlock{second.data(), second.size()}), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs move_constructed(std::move(copied));
  AnnotatedKernelArgs move_assigned;
  move_assigned = std::move(move_constructed);

  AnnotatedKernelArgs copy_assigned;
  copy_assigned = source;

  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;

  ASSERT_EQ(move_assigned.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), 2U);
  EXPECT_EQ(ReadReserved(arg_descs[0]), 16UL);
  EXPECT_EQ(ReadReserved(arg_descs[1]), 24UL);
  EXPECT_EQ(arg_descs[1].ir_idx, 1);

  ASSERT_EQ(copy_assigned.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), 1U);
  EXPECT_EQ(ReadReserved(arg_descs[0]), 16UL);
  EXPECT_EQ(arg_descs[0].ir_idx, 0);
}

// 非法输入：size 为 0、data 为空、reserved 非 0、已被移动；失败后 sticky，后续 ExtractArgsData 一并失败
TEST(AnnotatedKernelArgsUT, RejectsInvalidCustomData) {
  const auto blob = MakeBlob(4U, 8U);
  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;

  AnnotatedKernelArgs zero_size;
  EXPECT_NE(zero_size.AppendArg(CustomDataBlock{blob.data(), 0U}), ge::GRAPH_SUCCESS);
  EXPECT_NE(zero_size.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);

  AnnotatedKernelArgs null_data;
  EXPECT_NE(null_data.AppendArg(CustomDataBlock{nullptr, 8U}), ge::GRAPH_SUCCESS);
  EXPECT_NE(null_data.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  // 失败后 sticky：即使后续追加合法参数仍不得成功
  EXPECT_NE(null_data.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
  EXPECT_NE(null_data.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);

  // reserved 为未来扩展占位字段，当前必须全 0：任一字节非 0 都拒绝，且失败后 sticky
  for (size_t i = 0U; i < sizeof(CustomDataBlock::reserved); ++i) {
    CustomDataBlock data_with_reserved{blob.data(), blob.size()};
    data_with_reserved.reserved[i] = 1U;
    AnnotatedKernelArgs non_zero_reserved;
    EXPECT_NE(non_zero_reserved.AppendArg(data_with_reserved), ge::GRAPH_SUCCESS) << "reserved index " << i;
    EXPECT_NE(non_zero_reserved.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS) << "reserved index " << i;
  }

  AnnotatedKernelArgs moved_source;
  ASSERT_EQ(moved_source.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
  AnnotatedKernelArgs moved_target(std::move(moved_source));
  EXPECT_NE(moved_source.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
  EXPECT_NE(moved_source.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  // 移入方仍可用
  EXPECT_EQ(moved_target.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
  ASSERT_EQ(moved_target.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), 2U);
  EXPECT_EQ(arg_descs[0].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(arg_descs[1].ir_idx, 1);
}

// 两参公开签名行为完全不变：CUSTOM_DATA_BLOCK  desc 仍随 arg_descs 正常输出
TEST(AnnotatedKernelArgsUT, TwoArgExtractArgsDataBehaviorUnchanged) {
  const auto blob = MakeBlob(5U, 32U);
  AnnotatedKernelArgs args(InputAddr{0U, reinterpret_cast<void *>(0x1000U)}, CustomDataBlock{blob.data(), blob.size()},
                           uint64_t{9U});

  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;
  ASSERT_EQ(args.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), 3U);
  EXPECT_EQ(args_data.size(), 3U * sizeof(uint64_t));
  EXPECT_EQ(arg_descs[0].addr_type, ge::AddrType::INPUT_INSTANCE);
  EXPECT_EQ(arg_descs[1].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(ReadReserved(arg_descs[1]), 32UL);
  EXPECT_EQ(arg_descs[2].addr_type, ge::AddrType::CUSTOM_VALUE);
  EXPECT_EQ(ReadSlot(args_data, 0U), 0x1000U);
  EXPECT_EQ(ReadSlot(args_data, 1U), 0UL);
  EXPECT_EQ(ReadSlot(args_data, 2U), 9UL);
}

// 多个 blob 追加时序号递增，且与 IO/标量参数混排时仍各占一个 8 字节槽
TEST(AnnotatedKernelArgsUT, MultipleCustomDataDescsKeepOrder) {
  AnnotatedKernelArgs args;
  std::vector<size_t> expected_sizes;
  for (uint8_t i = 0U; i < 8U; ++i) {
    const auto size = static_cast<size_t>(i + 1U) * 128U;
    const auto blob = MakeBlob(i, size);
    ASSERT_EQ(args.AppendArg(CustomDataBlock{blob.data(), blob.size()}), ge::GRAPH_SUCCESS);
    expected_sizes.emplace_back(size);
  }

  std::vector<uint8_t> args_data;
  std::vector<ge::ArgDesc> arg_descs;
  ASSERT_EQ(args.ExtractArgsData(args_data, arg_descs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(arg_descs.size(), expected_sizes.size());
  EXPECT_EQ(args_data.size(), expected_sizes.size() * sizeof(uint64_t));
  for (size_t i = 0U; i < expected_sizes.size(); ++i) {
    EXPECT_EQ(arg_descs[i].addr_type, ge::AddrType::CUSTOM_DATA_BLOCK) << "desc index " << i;
    EXPECT_EQ(arg_descs[i].ir_idx, static_cast<int32_t>(i)) << "desc index " << i;
    EXPECT_EQ(ReadReserved(arg_descs[i]), expected_sizes[i]) << "desc index " << i;
    EXPECT_EQ(ReadSlot(args_data, i), 0UL) << "slot index " << i;
  }
}
}  // namespace
}  // namespace gert
