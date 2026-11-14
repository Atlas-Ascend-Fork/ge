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
#include "mockcpp/mockcpp.hpp"
#include "common/ge_inner_error_codes.h"
#include "exe_graph/runtime/tensor.h"
#include "framework/runtime/om2_model_executor.h"
#include "framework/runtime/mem_allocator.h"

namespace gert {
namespace {
struct Invocation {
  void *stream = nullptr;
  std::vector<Tensor *> inputs;
  std::vector<Tensor *> outputs;
  Allocators *allocators = nullptr;
  size_t sync_calls = 0U;
  size_t async_calls = 0U;
  ge::Status result = ge::SUCCESS;
};
Invocation *invocation = nullptr;
ge::Status RecordSync(Om2ModelExecutor *, std::vector<Tensor *> &inputs, std::vector<Tensor *> &outputs,
                      const Om2ModelRunArg &arg) {
  ++invocation->sync_calls;
  invocation->inputs = inputs;
  invocation->outputs = outputs;
  invocation->allocators = arg.external_allocator;
  return invocation->result;
}
ge::Status RecordAsync(Om2ModelExecutor *executor, void *stream, std::vector<Tensor *> &inputs,
                       std::vector<Tensor *> &outputs, const Om2ModelRunArg &arg) {
  invocation->stream = stream;
  const auto ret = RecordSync(executor, inputs, outputs, arg);
  --invocation->sync_calls;
  ++invocation->async_calls;
  return ret;
}
class Om2DynamicDispatchUT : public testing::Test {
 protected:
  void SetUp() override {
    invocation = &record_;
    MOCKER_CPP(&Om2ModelExecutor::Run).stubs().will(invoke(RecordSync));
    MOCKER_CPP(&Om2ModelExecutor::RunAsync).stubs().will(invoke(RecordAsync));
  }
  void TearDown() override {
    GlobalMockObject::verify();
    GlobalMockObject::reset();
    invocation = nullptr;
  }
  Invocation record_;
};

TEST_F(Om2DynamicDispatchUT, SyncAndAsyncPreserveTensorIdentityPlacementAndCurrentAllocator) {
  Om2ModelExecutor executor;
  MOCKER_CPP(&Om2ModelExecutor::IsDynamicModel).stubs().will(returnValue(true));
  std::vector<Tensor> inputs(1), outputs(1);
  inputs[0].SetData(TensorData(nullptr, nullptr, 0U, kOnHost));
  outputs[0].SetData(TensorData(nullptr, nullptr, 0U, kOnDeviceHbm));
  Allocators first, second;
  ASSERT_EQ(executor.RunDynamicOnline(nullptr, inputs, outputs, &first), ge::SUCCESS);
  ASSERT_EQ(record_.inputs.size(), 1U);
  ASSERT_EQ(record_.outputs.size(), 1U);
  EXPECT_EQ(record_.inputs[0], &inputs[0]);
  EXPECT_EQ(record_.outputs[0], &outputs[0]);
  EXPECT_EQ(record_.allocators, &first);
  int stream_token = 0;
  ASSERT_EQ(executor.RunDynamicOnline(&stream_token, inputs, outputs, &second), ge::SUCCESS);
  EXPECT_EQ(record_.stream, &stream_token);
  EXPECT_EQ(record_.allocators, &second);
  ASSERT_EQ(executor.RunDynamicOnline(nullptr, inputs, outputs, nullptr), ge::SUCCESS);
  EXPECT_EQ(record_.allocators, nullptr);
  EXPECT_EQ(record_.sync_calls, 2U);
  EXPECT_EQ(record_.async_calls, 1U);
  EXPECT_EQ(inputs[0].GetPlacement(), kOnHost);
  EXPECT_EQ(outputs[0].GetPlacement(), kOnDeviceHbm);
}

TEST_F(Om2DynamicDispatchUT, FailedInvocationReturnsOriginalStatusWithoutRetainingArguments) {
  Om2ModelExecutor executor;
  MOCKER_CPP(&Om2ModelExecutor::IsDynamicModel).stubs().will(returnValue(true));
  std::vector<Tensor> inputs, outputs;
  record_.result = ge::PARAM_INVALID;
  EXPECT_EQ(executor.RunDynamicOnline(nullptr, inputs, outputs, nullptr), ge::PARAM_INVALID);
  record_.result = ge::SUCCESS;
  EXPECT_EQ(executor.RunDynamicOnline(nullptr, inputs, outputs, nullptr), ge::SUCCESS);
  EXPECT_TRUE(record_.inputs.empty());
  EXPECT_TRUE(record_.outputs.empty());
}

TEST_F(Om2DynamicDispatchUT, StaticModelNeverEntersDynamicRunAdapters) {
  Om2ModelExecutor executor;
  MOCKER_CPP(&Om2ModelExecutor::IsDynamicModel).stubs().will(returnValue(false));
  std::vector<Tensor> inputs, outputs;
  EXPECT_NE(executor.RunDynamicOnline(nullptr, inputs, outputs, nullptr), ge::SUCCESS);
  EXPECT_EQ(record_.sync_calls, 0U);
  EXPECT_EQ(record_.async_calls, 0U);
}
}  // namespace
}  // namespace gert
