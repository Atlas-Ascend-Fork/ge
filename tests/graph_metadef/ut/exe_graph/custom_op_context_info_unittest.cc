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

#include "framework/runtime/custom_op_context_info.h"

namespace gert {
namespace {
TEST(CustomOpContextInfoUT, SetAndGetAllKeys) {
  CustomOpContextInfo info;
  info.SetStaticModel(true);
  info.SetAddressRefreshEnabled(true);
  info.SetGraphId(7UL);
  info.SetNodeId(11UL);
  info.SetVectorCoreNum(24UL);
  info.SetAiCoreNum(32UL);
  info.SetAiCpuNum(4UL);

  uint64_t value = 0UL;
  ASSERT_TRUE(info.GetContextValue(ContextParam::kStaticModel, value));
  EXPECT_EQ(value, 1UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kAddressRefreshEnabled, value));
  EXPECT_EQ(value, 1UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kGraphId, value));
  EXPECT_EQ(value, 7UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kNodeId, value));
  EXPECT_EQ(value, 11UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kVectorCoreNum, value));
  EXPECT_EQ(value, 24UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kAiCoreNum, value));
  EXPECT_EQ(value, 32UL);
  ASSERT_TRUE(info.GetContextValue(ContextParam::kAiCpuNum, value));
  EXPECT_EQ(value, 4UL);
}

TEST(CustomOpContextInfoUT, UnsetKeysReturnFalse) {
  CustomOpContextInfo info;
  uint64_t value = 0xFFFFUL;
  EXPECT_FALSE(info.GetContextValue(ContextParam::kStaticModel, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kAddressRefreshEnabled, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kGraphId, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kNodeId, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kVectorCoreNum, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kAiCoreNum, value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kAiCpuNum, value));
}

TEST(CustomOpContextInfoUT, UndefinedKeyReturnsFalse) {
  CustomOpContextInfo info;
  uint64_t value = 0UL;
  EXPECT_FALSE(info.GetContextValue(static_cast<ContextParam>(100U), value));
  EXPECT_FALSE(info.GetContextValue(ContextParam::kNum, value));
}

TEST(CustomOpContextInfoUT, AttachedStreamProvider) {
  CustomOpContextInfo info;
  EXPECT_EQ(info.GetAttachedStreamProvider(), nullptr);
  info.SetAttachedStreamProvider(nullptr);
  EXPECT_EQ(info.GetAttachedStreamProvider(), nullptr);
}
}  // namespace
}  // namespace gert
