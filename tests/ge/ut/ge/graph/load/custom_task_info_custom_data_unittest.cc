/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// CustomTaskInfo 变长自定义数据（CustomDataBlock）相关用例。
// 从 custom_task_info_unittest.cc 拆分而来：该文件已超「超大源文件」阈值（nbnc > 2000），
// 本特性新增用例独立成文件，避免继续加剧超标；测试脚手架（mock / fixture helper）在本文件自包含。

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "graph/load/model_manager/task_info/ge/custom_task_info.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/reusable_stream_allocator.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/utils/args_format_desc_utils.h"
#include "depends/runtime/src/runtime_stub.h"
#include "depends/ascendcl/src/ascendcl_stub.h"
#include "securec.h"

namespace ge {
namespace {
class AclMockMemcpyForCustomData : public AclRuntimeStub {
 public:
  MOCK_METHOD5(aclrtMemcpy, int32_t(void *, size_t, const void *, size_t, aclrtMemcpyKind));
};

// 使 LoadCustomDataBlobs 可在无 NPU 环境下验证：aclrtMalloc 返回宿主内存（MemoryBlockManager 据此建块），
// aclrtMemcpy 执行真实宿主拷贝并记录源字节，因此可直接读回“device”内容逐字节校验 blob 切分。
// 注：内存不在本类释放——基类 AclRuntimeStub::aclrtFree 已做 delete[]，由
// DavinciModel::FreeDynamicWorkspaceMemory 统一回收，否则会双重释放。
class AclMockCustomDataUpload : public AclRuntimeStub {
 public:
  aclError aclrtMalloc(void **dev_ptr, size_t size, aclrtMemMallocPolicy policy) override {
    (void)policy;
    ++malloc_call_count;
    if (malloc_fail) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    auto *const buf = new uint8_t[size];
    (void)memset_s(buf, size, 0, size);
    *dev_ptr = buf;
    return ACL_SUCCESS;
  }

  aclError aclrtMemcpy(void *dst, size_t dest_max, const void *src, size_t count, aclrtMemcpyKind kind) override {
    (void)kind;
    ++memcpy_call_count;
    if (memcpy_fail) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    if (memcpy_s(dst, dest_max, src, count) != EOK) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    copied.emplace_back(static_cast<const uint8_t *>(src), static_cast<const uint8_t *>(src) + count);
    return ACL_SUCCESS;
  }

  std::vector<std::vector<uint8_t>> copied;
  size_t malloc_call_count = 0UL;
  size_t memcpy_call_count = 0UL;
  bool malloc_fail = false;
  bool memcpy_fail = false;
};

void AppendCustomDataDesc(std::vector<ArgDesc> &arg_descs, const int32_t ir_idx, const uint64_t size) {
  ArgDesc arg_desc = {AddrType::CUSTOM_DATA_BLOCK, ir_idx, false, {0}};
  (void)memcpy_s(arg_desc.reserved, sizeof(arg_desc.reserved), &size, sizeof(size));
  arg_descs.emplace_back(arg_desc);
}

uint64_t ReadCustomDataSize(const ArgDesc &arg_desc) {
  uint64_t size = 0UL;
  (void)memcpy_s(&size, sizeof(size), arg_desc.reserved, sizeof(size));
  return size;
}

domi::TaskDef MakeCustomDataExtInfoTaskDef(const std::string &ext_info, const uint32_t ext_info_size_field) {
  domi::TaskDef task_def;
  task_def.set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_CUSTOM_KERNEL));
  auto *const kernel = task_def.mutable_kernel();
  kernel->set_kernel_ext_info(ext_info);
  kernel->set_kernel_ext_info_size(ext_info_size_field);
  return task_def;
}

class UtestCustomTaskInfoCustomData : public testing::Test {
 protected:
  void SetUp() override {
    RTS_STUB_SETUP();
    auto acl_mock_memcpy = [](void *dst, size_t dest_max, const void *src, size_t count,
                              aclrtMemcpyKind kind) -> int32_t {
      (void)kind;
      if ((count == 0U) || (dst == nullptr) || (src == nullptr)) {
        return -1;
      }
      (void)memcpy_s(dst, dest_max, src, count);
      return RT_ERROR_NONE;
    };
    auto acl_runtime_stub = std::make_shared<AclMockMemcpyForCustomData>();
    AclRuntimeStub::SetInstance(acl_runtime_stub);
    EXPECT_CALL(*acl_runtime_stub, aclrtMemcpy).WillRepeatedly(testing::Invoke(acl_mock_memcpy));
  }

  void TearDown() override {
    AclRuntimeStub::Reset();
    RTS_STUB_TEARDOWN();
  }

  void InitBasicTaskInfo(CustomTaskInfo &task_info, DavinciModel &model, const std::string &op_name = "custom_op") {
    auto op_desc = std::make_shared<OpDesc>(op_name, "CustomOp");
    GeTensorDesc desc;
    op_desc->AddInputDesc(desc);
    op_desc->AddOutputDesc(desc);
    op_desc->SetId(0);
    op_desc->SetStreamId(0);
    model.op_list_[0] = op_desc;

    rtStream_t stream = nullptr;
    model.reusable_stream_allocator_ = ReusableStreamAllocator::Create();
    model.reusable_stream_allocator_->GetOrCreateRtStream(stream, 0, 0, 0);
    model.stream_list_ = {stream};

    task_info.davinci_model_ = &model;
    task_info.op_desc_ = op_desc;
    task_info.stream_ = stream;
    task_info.input_data_addrs_ = {0x1000};
    task_info.output_data_addrs_ = {0x2000};
  }
};

TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsUploadsAndSlicesByReservedSize) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);

  // 三个 blob：含 0x00 字节、非对齐长度、大于 4KB
  const std::string first("\x01\x00\x02\x00\x03", 5);
  const std::string second("\xA5\x5A\xFF", 3);
  const std::string third(4095, '\x7E');
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, first.size());
  ArgsFormatDescUtils::Append(task_info.args_format_holder_.arg_descs, AddrType::INPUT_INSTANCE, 0);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 1, second.size());
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 2, third.size());

  const auto ext_info = first + second + third;
  auto task_def = MakeCustomDataExtInfoTaskDef(ext_info, static_cast<uint32_t>(ext_info.size()));
  ASSERT_EQ(task_info.LoadCustomDataBlobs(task_def), SUCCESS);

  // 合并分配：只应有一次 malloc 与一次 H2D；缓冲总长 = Σ align512(blob_size)
  constexpr size_t kAlign = 512UL;
  const auto align_up = [](const size_t size) { return ((size + kAlign - 1UL) / kAlign) * kAlign; };
  const size_t expected_buffer_size = align_up(first.size()) + align_up(second.size()) + align_up(third.size());
  EXPECT_EQ(mock->malloc_call_count, 1UL);
  EXPECT_EQ(mock->memcpy_call_count, 1UL);
  ASSERT_EQ(mock->copied.size(), 1UL);
  EXPECT_EQ(mock->copied[0].size(), expected_buffer_size);

  ASSERT_EQ(task_info.custom_data_addrs_.size(), 3UL);
  const auto *const base = static_cast<const uint8_t *>(task_info.custom_data_addrs_[0]);
  const auto base_addr = reinterpret_cast<uintptr_t>(base);
  // 产品代码只保证「相对缓冲基址的偏移」按 512B 对齐；基址对齐由分配器负责（真实环境为 aclrtMalloc 的
  // 2MB 大页块），本用例的 mock 用 new 分配只有 16B 对齐，故此处校验相对偏移。
  for (size_t i = 0UL; i < 3UL; ++i) {
    const auto addr = reinterpret_cast<uintptr_t>(task_info.custom_data_addrs_[i]);
    EXPECT_EQ((addr - base_addr) % kAlign, 0UL) << "blob index " << i;
  }
  EXPECT_EQ(static_cast<const void *>(base + align_up(first.size())), task_info.custom_data_addrs_[1]);
  EXPECT_EQ(static_cast<const void *>(base + align_up(first.size()) + align_up(second.size())),
            task_info.custom_data_addrs_[2]);
  // 逐字节校验内容与顺序
  EXPECT_EQ(memcmp(task_info.custom_data_addrs_[0], first.data(), first.size()), 0);
  EXPECT_EQ(memcmp(task_info.custom_data_addrs_[1], second.data(), second.size()), 0);
  EXPECT_EQ(memcmp(task_info.custom_data_addrs_[2], third.data(), third.size()), 0);
  // padding 区必须为 0，避免 kernel 按 32B 粒度整块搬运时读到相邻 blob 的数据
  const auto expect_padding_zero = [&mock](const size_t begin, const size_t end) {
    for (size_t i = begin; i < end; ++i) {
      ASSERT_EQ(mock->copied[0][i], 0U) << "padding index " << i;
    }
  };
  expect_padding_zero(first.size(), align_up(first.size()));
  expect_padding_zero(align_up(first.size()) + second.size(), align_up(first.size()) + align_up(second.size()));

  model.FreeDynamicWorkspaceMemory();
  AclRuntimeStub::Reset();
}

TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsNoCustomDataSkipsUpload) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  ArgsFormatDescUtils::Append(task_info.args_format_holder_.arg_descs, AddrType::INPUT_INSTANCE, 0);
  ASSERT_EQ(ArgsFormatDescUtils::InsertCustomValue(task_info.args_format_holder_.arg_descs, -1, 7U), GRAPH_SUCCESS);

  auto task_def = MakeCustomDataExtInfoTaskDef("", 0U);
  EXPECT_EQ(task_info.LoadCustomDataBlobs(task_def), SUCCESS);
  EXPECT_TRUE(task_info.custom_data_addrs_.empty());
  EXPECT_EQ(mock->malloc_call_count, 0UL);
  EXPECT_EQ(mock->memcpy_call_count, 0UL);

  AclRuntimeStub::Reset();
}

// 三重交叉校验：Σ(args_format 编码的 size) != kernel_ext_info.size()
TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsExtInfoSizeMismatchFailed) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 8U);

  // ext_info 只有 7 字节，与 reserved 总和 8 不一致
  auto task_def = MakeCustomDataExtInfoTaskDef(std::string(7, '\x11'), 7U);
  EXPECT_NE(task_info.LoadCustomDataBlobs(task_def), SUCCESS);
  EXPECT_TRUE(task_info.custom_data_addrs_.empty());
  EXPECT_EQ(mock->malloc_call_count, 0UL);

  AclRuntimeStub::Reset();
}

// 三重交叉校验：kernel_ext_info.size() != kernel_ext_info_size 字段
TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsExtInfoSizeFieldMismatchFailed) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 4U);

  auto task_def = MakeCustomDataExtInfoTaskDef(std::string(4, '\x22'), 5U);
  EXPECT_NE(task_info.LoadCustomDataBlobs(task_def), SUCCESS);
  EXPECT_TRUE(task_info.custom_data_addrs_.empty());

  AclRuntimeStub::Reset();
}

// 编译期 parser 因 op_desc 可为 nullptr 而无法校验序号上界，运行期必须补此校验
TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsBlobIndexOutOfRangeFailed) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 5, 4U);

  auto task_def = MakeCustomDataExtInfoTaskDef(std::string(4, '\x33'), 4U);
  EXPECT_NE(task_info.LoadCustomDataBlobs(task_def), SUCCESS);
  EXPECT_TRUE(task_info.custom_data_addrs_.empty());
  EXPECT_EQ(mock->malloc_call_count, 0UL);

  AclRuntimeStub::Reset();
}

TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsMallocFailed) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  mock->malloc_fail = true;
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 16U);

  auto task_def = MakeCustomDataExtInfoTaskDef(std::string(16, '\x44'), 16U);
  EXPECT_NE(task_info.LoadCustomDataBlobs(task_def), SUCCESS);

  AclRuntimeStub::Reset();
}

TEST_F(UtestCustomTaskInfoCustomData, LoadCustomDataBlobsMemcpyFailed) {
  auto mock = std::make_shared<AclMockCustomDataUpload>();
  mock->memcpy_fail = true;
  AclRuntimeStub::SetInstance(mock);
  DavinciModel model(0, nullptr);
  CustomTaskInfo task_info;
  InitBasicTaskInfo(task_info, model);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 16U);

  auto task_def = MakeCustomDataExtInfoTaskDef(std::string(16, '\x55'), 16U);
  EXPECT_NE(task_info.LoadCustomDataBlobs(task_def), SUCCESS);

  model.FreeDynamicWorkspaceMemory();
  AclRuntimeStub::Reset();
}

// CUSTOM_DATA_BLOCK 以 kAbsoluteMemType 参与刷新，且与其他类型混合时顺序保持
TEST_F(UtestCustomTaskInfoCustomData, AssembleIoByArgsFormatFillsCustomDataAsAbsoluteAddr) {
  CustomTaskInfo task_info;
  task_info.op_desc_ = std::make_shared<OpDesc>("custom_data_io", "CustomOp");
  task_info.input_data_addrs_ = {0x1000ULL};
  task_info.input_mem_types_ = {kFmMemType};
  task_info.output_data_addrs_ = {0x2000ULL};
  task_info.output_mem_types_ = {kFmMemType};
  task_info.custom_data_addrs_ = {reinterpret_cast<void *>(0xAAAA0000ULL), reinterpret_cast<void *>(0xBBBB0000ULL)};

  ArgsFormatDescUtils::Append(task_info.args_format_holder_.arg_descs, AddrType::INPUT_INSTANCE, 0);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 16U);
  ArgsFormatDescUtils::Append(task_info.args_format_holder_.arg_descs, AddrType::OUTPUT_INSTANCE, 0);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 1, 24U);

  ASSERT_EQ(task_info.AssembleIoByArgsFormat(), SUCCESS);
  EXPECT_EQ(task_info.io_addrs_, (std::vector<uint64_t>{0x1000ULL, 0xAAAA0000ULL, 0x2000ULL, 0xBBBB0000ULL}));
  EXPECT_EQ(task_info.io_addr_mem_types_,
            (std::vector<uint64_t>{kFmMemType, kAbsoluteMemType, kFmMemType, kAbsoluteMemType}));
  // io_addr_offset_ 契约不变
  EXPECT_EQ(task_info.io_addr_offset_, 0UL);
}

// blob 地址表缺失时必须报错，不得静默填入空指针
TEST_F(UtestCustomTaskInfoCustomData, AssembleIoByArgsFormatCustomDataAddrMissingFailed) {
  CustomTaskInfo task_info;
  task_info.op_desc_ = std::make_shared<OpDesc>("custom_data_missing", "CustomOp");
  task_info.input_data_addrs_ = {0x1000ULL};
  task_info.input_mem_types_ = {kFmMemType};
  ArgsFormatDescUtils::Append(task_info.args_format_holder_.arg_descs, AddrType::INPUT_INSTANCE, 0);
  AppendCustomDataDesc(task_info.args_format_holder_.arg_descs, 0, 16U);

  EXPECT_NE(task_info.AssembleIoByArgsFormat(), SUCCESS);
}

// args_format 往返：CUSTOM_DATA_BLOCK 的 reserved 经序列化/反序列化后仍等于原 size
TEST_F(UtestCustomTaskInfoCustomData, CustomDataArgsFormatRoundTripKeepsReservedSize) {
  std::vector<ArgDesc> arg_descs;
  ArgsFormatDescUtils::Append(arg_descs, AddrType::INPUT_INSTANCE, 0);
  AppendCustomDataDesc(arg_descs, 0, 1024U);
  AppendCustomDataDesc(arg_descs, 1, 7U);
  const auto serialized = ArgsFormatDescUtils::Serialize(arg_descs);
  EXPECT_EQ(serialized, "{i_instance0*}{custom_data_block0.1024bytes}{custom_data_block1.7bytes}");

  std::vector<ArgDesc> parsed;
  ASSERT_EQ(ArgsFormatDescUtils::Parse(serialized, parsed), GRAPH_SUCCESS);
  ASSERT_EQ(parsed.size(), 3UL);
  EXPECT_EQ(parsed[1].addr_type, AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(ReadCustomDataSize(parsed[1]), 1024UL);
  EXPECT_EQ(parsed[2].addr_type, AddrType::CUSTOM_DATA_BLOCK);
  EXPECT_EQ(ReadCustomDataSize(parsed[2]), 7UL);
}

}  // namespace
}  // namespace ge
