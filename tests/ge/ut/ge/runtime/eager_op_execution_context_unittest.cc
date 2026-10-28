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

#include <memory>
#include <string>
#include <vector>

#include "exe_graph/lowering/kernel_run_context_builder.h"
#include "exe_graph/runtime/eager_op_execution_context.h"
#include "framework/runtime/attached_stream_provider.h"
#include "framework/runtime/args_handler.h"
#include "framework/runtime/eager_execution_context_info.h"
#include "graph/ge_local_context.h"
#include "graph/load/model_manager/task_info/ge/sink_op_args_handler.h"
#include "graph/op_desc.h"

namespace gert {
namespace {
class FakeProvider final : public AttachedStreamProvider {
 public:
  rtStream RequestAttachedStream(const ge::AscendString &key, uint32_t flags) override {
    last_key = key.GetString();
    last_flags = flags;
    return stream;
  }
  rtStream stream = reinterpret_cast<rtStream>(0x1234U);
  std::string last_key;
  uint32_t last_flags = 0U;
};
}  // namespace

TEST(EagerOpExecutionContextTest, PublicLayoutAndIndicesRemainStable) {
  EXPECT_EQ(static_cast<uint32_t>(EagerOpExecutionContext::AdditionalInputIndex::kDeviceAllocator), 0U);
  EXPECT_EQ(static_cast<uint32_t>(EagerOpExecutionContext::AdditionalInputIndex::kStream), 1U);
  EXPECT_EQ(static_cast<uint32_t>(EagerOpExecutionContext::AdditionalOutputIndex::kWorkSpace), 0U);
  EXPECT_EQ(static_cast<uint32_t>(EagerOpExecutionContext::AdditionalOutputIndex::kArgsHandler), 1U);
}

TEST(EagerOpExecutionContextTest, ProviderDefaultIsNull) {
  // 辅流提供者随伴生对象（kContextInfo槽）由框架装入，默认未装
  gert::EagerExecutionContextInfo context_info;
  EXPECT_EQ(context_info.GetAttachedStreamProvider(), nullptr);
}

TEST(EagerOpExecutionContextTest, ProviderPreservesKey) {
  FakeProvider provider;
  EXPECT_EQ(provider.RequestAttachedStream(ge::AscendString("aux"), 0U), provider.stream);
  EXPECT_EQ(provider.last_key, "aux");
}

namespace {
// 构造与 CustomTaskInfo::Distribute 相同布局的 eager context：
// 附加输入 = {device allocator, stream}，附加输出 = {workspace vector, args handler}
gert::KernelContextHolder BuildEagerContext(gert::ArgsHandler &handler, std::vector<int> &ws_vec,
                                            const ge::OpDescPtr &op_desc) {
  std::vector<void *> additional_inputs = {nullptr, nullptr};  // 本组用例不读取附加输入
  std::vector<void *> outputs = {&ws_vec, static_cast<gert::ArgsHandler *>(&handler)};
  return gert::KernelRunContextBuilder().Inputs(additional_inputs).Outputs(outputs).Build(op_desc);
}
}  // namespace

namespace {
// 与CustomTaskInfo::Distribute当前布局一致：附加输出 = {workspace vector, args handler, context info}
gert::KernelContextHolder BuildEagerContextWithInfo(gert::ArgsHandler &handler, std::vector<int> &ws_vec,
                                                    gert::EagerExecutionContextInfo &context_info,
                                                    const ge::OpDescPtr &op_desc) {
  std::vector<void *> additional_inputs = {nullptr, nullptr};
  std::vector<void *> outputs = {&ws_vec, static_cast<gert::ArgsHandler *>(&handler), &context_info};
  return gert::KernelRunContextBuilder().Inputs(additional_inputs).Outputs(outputs).Build(op_desc);
}
}  // namespace

TEST(EagerOpExecutionContextTest, RequestAttachedStreamForwardsViaContextInfoSlot) {
  auto op_desc = std::make_shared<ge::OpDesc>("eager_attached_stream_op", "EagerAttachedStreamOp");
  std::vector<int> ws_vec;
  FakeProvider provider;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;
  context_info.SetAttachedStreamProvider(&provider);

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  // 默认标志
  EXPECT_EQ(context->RequestAttachedStream(ge::AscendString("fwd_key"), 0U), provider.stream);
  EXPECT_EQ(provider.last_key, "fwd_key");
  EXPECT_EQ(provider.last_flags, 0U);
  // RTS流标志原值直接透传（对照hccl_task_info的RT_STREAM_PERSISTENT|RT_STREAM_FORCE_COPY用法）
  const uint32_t flags = 0x01U | 0x02U;
  EXPECT_EQ(context->RequestAttachedStream(ge::AscendString("fwd_key"), flags), provider.stream);
  EXPECT_EQ(provider.last_flags, flags);
}

TEST(EagerOpExecutionContextTest, RequestAttachedStreamReturnsNullptrWithoutProvider) {
  auto op_desc = std::make_shared<ge::OpDesc>("eager_no_provider_op", "EagerNoProviderOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;  // 未装入provider，对应声明式算子等不支持辅流的场景

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  EXPECT_EQ(context->RequestAttachedStream(ge::AscendString("any_key"), 0U), nullptr);
}
TEST(EagerOpExecutionContextTest, GetContextValueReadsCompanionInfo) {
  auto op_desc = std::make_shared<ge::OpDesc>("eager_context_info_op", "EagerContextInfoOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;
  context_info.SetStaticModel(true);
  context_info.SetAddressRefreshEnabled(true);
  context_info.SetGraphId(7UL);

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  uint64_t value = 0UL;
  ASSERT_TRUE(context->GetContextValue(gert::ContextParam::kStaticModel, value));
  EXPECT_EQ(value, 1UL);
  ASSERT_TRUE(context->GetContextValue(gert::ContextParam::kAddressRefreshEnabled, value));
  EXPECT_EQ(value, 1UL);
  ASSERT_TRUE(context->GetContextValue(gert::ContextParam::kGraphId, value));
  EXPECT_EQ(value, 7UL);
}

TEST(EagerOpExecutionContextTest, GetContextValueDefaultsWhenNotSet) {
  auto op_desc = std::make_shared<ge::OpDesc>("eager_default_info_op", "EagerDefaultInfoOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  // 布尔参数未设置时取值为0（查询仍成功）
  uint64_t value = 0xFFFFUL;
  ASSERT_TRUE(context->GetContextValue(gert::ContextParam::kStaticModel, value));
  EXPECT_EQ(value, 0UL);
  ASSERT_TRUE(context->GetContextValue(gert::ContextParam::kAddressRefreshEnabled, value));
  EXPECT_EQ(value, 0UL);
  // graph id未设置时查询失败，与取值为0可区分
  uint64_t graph_id = 0xFFFFUL;
  EXPECT_FALSE(context->GetContextValue(gert::ContextParam::kGraphId, graph_id));
}

TEST(EagerOpExecutionContextTest, GetContextValueUndefinedKeyReturnsFalse) {
  auto op_desc = std::make_shared<ge::OpDesc>("eager_undef_flag_op", "EagerUndefFlagOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;
  context_info.SetStaticModel(true);

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  uint64_t value = 0UL;
  EXPECT_FALSE(context->GetContextValue(static_cast<gert::ContextParam>(100U), value));
}

TEST(EagerOpExecutionContextTest, GetContextValueReturnsFalseWithoutContextInfoSlot) {
  // 老布局（无kContextInfo槽）优雅降级为false，不越界
  auto op_desc = std::make_shared<ge::OpDesc>("eager_no_info_slot_op", "EagerNoInfoSlotOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);

  auto holder = BuildEagerContext(handler, ws_vec, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);
  uint64_t value = 0UL;
  EXPECT_FALSE(context->GetContextValue(gert::ContextParam::kStaticModel, value));
}

TEST(EagerOpExecutionContextTest, GetOptionReadsThreadLocalContextOption) {
  const std::string kOptionKey = "ge.exec.utTestOption";
  const std::map<std::string, std::string> saved = ge::GetThreadLocalContext().GetAllSessionOptions();
  ge::GetThreadLocalContext().SetSessionOption({{kOptionKey, "42"}});

  auto op_desc = std::make_shared<ge::OpDesc>("eager_get_option_op", "EagerGetOptionOp");
  std::vector<int> ws_vec;
  ge::SinkOpArgsHandler handler(nullptr);
  gert::EagerExecutionContextInfo context_info;

  auto holder = BuildEagerContextWithInfo(handler, ws_vec, context_info, op_desc);
  auto *context = reinterpret_cast<EagerOpExecutionContext *>(holder.GetKernelContext());
  ASSERT_NE(context, nullptr);

  ge::AscendString value;
  EXPECT_TRUE(context->GetOption(ge::AscendString(kOptionKey.c_str()), value));
  EXPECT_STREQ(value.GetString(), "42");
  EXPECT_FALSE(context->GetOption(ge::AscendString("ge.exec.utNoSuchOption"), value));

  ge::GetThreadLocalContext().SetSessionOption(saved);
}
}  // namespace gert
