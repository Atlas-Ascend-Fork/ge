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
#include <cstring>
#include <string>
#include <vector>
#include "exe_graph/runtime/task_def_builder.h"
#include "proto/task.pb.h"

namespace {
const uint32_t kStreamId = 3U;
const uint32_t kTaskType = 0U;  // RT_MODEL_TASK_KERNEL
const uint32_t kBlockDim = 32U;
const char *kArgsFormat = "{hi.hcclsk0*}{i0*}{o0*}";

// 构造与迁移前路径等价的 domi::TaskDef（golden 对照对象）
domi::TaskDef CreateGoldenTaskDef() {
  domi::TaskDef task_def;
  task_def.set_stream_id(kStreamId);
  task_def.set_type(kTaskType);
  task_def.mutable_kernel()->set_block_dim(kBlockDim);
  task_def.mutable_kernel()->mutable_context()->set_args_format(kArgsFormat);
  return task_def;
}
}  // namespace

class TestTaskDefBuilder : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

// 验证6叶方法链式构造后，序列化字节与直接操作domi::TaskDef逐字节一致
TEST_F(TestTaskDefBuilder, SerializeByteIdenticalWithProto) {
  gert::TaskDefBuilder task;
  task.SetStreamId(kStreamId).SetType(kTaskType);
  task.GetKernelBuilder().SetBlockDim(kBlockDim).GetContextBuilder().SetArgsFormat(ge::AscendString(kArgsFormat));

  domi::TaskDef golden = CreateGoldenTaskDef();
  const size_t builder_size = task.ByteSizeLong();
  ASSERT_EQ(builder_size, golden.ByteSizeLong());

  std::vector<uint8_t> builder_buf(builder_size, 0);
  std::vector<uint8_t> golden_buf(builder_size, 0);
  ASSERT_TRUE(task.SerializeToArray(builder_buf.data(), builder_buf.size()));
  ASSERT_TRUE(golden.SerializeToArray(golden_buf.data(), static_cast<int>(golden_buf.size())));
  EXPECT_EQ(memcmp(builder_buf.data(), golden_buf.data(), builder_size), 0);
}

// 验证FE消费视角：序列化字节可被ParseFromArray无感反序列化且字段值正确
TEST_F(TestTaskDefBuilder, ParseBackFromSerializeBuffer) {
  gert::TaskDefBuilder task;
  task.SetStreamId(kStreamId).SetType(kTaskType);
  task.GetKernelBuilder().SetBlockDim(kBlockDim).GetContextBuilder().SetArgsFormat(ge::AscendString(kArgsFormat));

  std::vector<uint8_t> buf(task.ByteSizeLong(), 0);
  ASSERT_TRUE(task.SerializeToArray(buf.data(), buf.size()));

  domi::TaskDef parsed;
  ASSERT_TRUE(parsed.ParseFromArray(buf.data(), static_cast<int>(buf.size())));
  EXPECT_EQ(parsed.stream_id(), kStreamId);
  EXPECT_EQ(parsed.type(), kTaskType);
  EXPECT_EQ(parsed.kernel().block_dim(), kBlockDim);
  EXPECT_EQ(parsed.kernel().context().args_format(), kArgsFormat);
}

// 验证ByteSizeLong与domi::TaskDef::ByteSizeLong一致，且随字段设置增长
TEST_F(TestTaskDefBuilder, ByteSizeLongConsistency) {
  gert::TaskDefBuilder empty_task;
  EXPECT_EQ(empty_task.ByteSizeLong(), domi::TaskDef().ByteSizeLong());

  gert::TaskDefBuilder task;
  task.SetStreamId(kStreamId).SetType(kTaskType);
  task.GetKernelBuilder().SetBlockDim(kBlockDim).GetContextBuilder().SetArgsFormat(ge::AscendString(kArgsFormat));
  EXPECT_EQ(task.ByteSizeLong(), CreateGoldenTaskDef().ByteSizeLong());
}

// 验证SerializeToArray入参防御：空缓冲/零长度返回false
TEST_F(TestTaskDefBuilder, SerializeToArrayInvalidInput) {
  gert::TaskDefBuilder task;
  task.SetStreamId(kStreamId).SetType(kTaskType);
  std::vector<uint8_t> buf(64, 0);
  EXPECT_FALSE(task.SerializeToArray(nullptr, buf.size()));
  EXPECT_FALSE(task.SerializeToArray(buf.data(), 0U));
  EXPECT_TRUE(task.SerializeToArray(buf.data(), buf.size()));
}

// 验证视图生命周期契约：视图按值持有、使用期间父builder存活，写入随父序列化生效
TEST_F(TestTaskDefBuilder, ViewLifetimeCoveredByParent) {
  std::vector<uint8_t> buf;
  {
    gert::TaskDefBuilder task;
    auto kernel_builder = task.GetKernelBuilder();
    auto ctx_builder = kernel_builder.SetBlockDim(kBlockDim).GetContextBuilder();
    ctx_builder.SetArgsFormat(ge::AscendString(kArgsFormat));
    buf.resize(task.ByteSizeLong());
    ASSERT_TRUE(task.SerializeToArray(buf.data(), buf.size()));
  }
  // 内层作用域结束，父builder析构（视图生命周期被父覆盖，无悬垂写入）
  domi::TaskDef parsed;
  ASSERT_TRUE(parsed.ParseFromArray(buf.data(), static_cast<int>(buf.size())));
  EXPECT_EQ(parsed.kernel().block_dim(), kBlockDim);
  EXPECT_EQ(parsed.kernel().context().args_format(), kArgsFormat);
}

// 验证重复调用SetXxx为幂等覆盖语义（最后一次生效）
TEST_F(TestTaskDefBuilder, SettersOverwriteSemantics) {
  gert::TaskDefBuilder task;
  task.SetStreamId(1U).SetStreamId(kStreamId);
  task.GetKernelBuilder()
      .SetBlockDim(8U)
      .SetBlockDim(kBlockDim)
      .GetContextBuilder()
      .SetArgsFormat(ge::AscendString("tmp"))
      .SetArgsFormat(ge::AscendString(kArgsFormat));

  std::vector<uint8_t> buf(task.ByteSizeLong(), 0);
  ASSERT_TRUE(task.SerializeToArray(buf.data(), buf.size()));
  domi::TaskDef parsed;
  ASSERT_TRUE(parsed.ParseFromArray(buf.data(), static_cast<int>(buf.size())));
  EXPECT_EQ(parsed.stream_id(), kStreamId);
  EXPECT_EQ(parsed.kernel().block_dim(), kBlockDim);
  EXPECT_EQ(parsed.kernel().context().args_format(), kArgsFormat);
}
