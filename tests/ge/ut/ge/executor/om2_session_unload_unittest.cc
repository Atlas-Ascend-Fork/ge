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
#include <map>
#include "mockcpp/mockcpp.hpp"
#include "om2/om2_model_manager.h"
#include "framework/runtime/om2_model_executor.h"
#include "framework/common/gert_model_data_utils.h"
#include "graph/load/model_manager/model_utils.h"

namespace ge {
namespace {
std::map<const gert::Om2ModelExecutor *, uint64_t> *sessions = nullptr;
uint64_t SessionOf(gert::Om2ModelExecutor *executor) {
  return sessions->at(executor);
}
class Om2SessionUnloadUT : public testing::Test {
 protected:
  void SetUp() override {
    sessions = &sessions_;
    using Load = Status (gert::Om2ModelExecutor::*)(const gert::GertModelData &, const gert::Om2ModelLoadArg &,
                                                    uint64_t, void *) const;
    MOCKER_CPP(static_cast<Load>(&gert::Om2ModelExecutor::Load)).stubs().will(returnValue(SUCCESS));
    MOCKER_CPP(&gert::Om2ModelExecutor::SessionId).stubs().will(invoke(SessionOf));
  }
  void TearDown() override {
    for (const auto id : ids_) {
      EXPECT_EQ(Om2ModelManager::GetInstance().UnloadModel(id), SUCCESS);
    }
    GlobalMockObject::verify();
    GlobalMockObject::reset();
    sessions = nullptr;
  }
  void LoadModel(uint64_t session, uint32_t &id) {
    auto &manager = Om2ModelManager::GetInstance();
    id = manager.GenModelId();
    ids_.push_back(id);
    gert::GertModelData data;
    gert::InitGertModelData(data);
    gert::Om2ModelLoadArg arg;
    ASSERT_EQ(manager.LoadModel(id, data, arg, session), SUCCESS);
    auto executor = manager.GetModelExecutor(id);
    ASSERT_NE(executor, nullptr);
    sessions_[executor.get()] = session;
  }
  std::vector<uint32_t> ids_;
  std::map<const gert::Om2ModelExecutor *, uint64_t> sessions_;
};

TEST_F(Om2SessionUnloadUT, SetDeviceFailureKeepsModelsAndRetryUnloadsOnlyMatchingSession) {
  uint32_t first, second, other;
  ASSERT_NO_FATAL_FAILURE(LoadModel(90291U, first));
  ASSERT_NO_FATAL_FAILURE(LoadModel(90291U, second));
  ASSERT_NO_FATAL_FAILURE(LoadModel(90292U, other));
  MOCKER(ModelUtils::SetDevice).expects(exactly(2)).will(returnValue(FAILED)).then(returnValue(SUCCESS));
  auto &manager = Om2ModelManager::GetInstance();
  EXPECT_EQ(manager.UnloadSessionModels(90291U), FAILED);
  EXPECT_NE(manager.GetModelExecutor(first), nullptr);
  EXPECT_NE(manager.GetModelExecutor(second), nullptr);
  EXPECT_NE(manager.GetModelExecutor(other), nullptr);
  EXPECT_EQ(manager.UnloadSessionModels(90291U), SUCCESS);
  EXPECT_EQ(manager.GetModelExecutor(first), nullptr);
  EXPECT_EQ(manager.GetModelExecutor(second), nullptr);
  EXPECT_NE(manager.GetModelExecutor(other), nullptr);
  // No matching model: no further device selection, even for repeated cleanup.
  EXPECT_EQ(manager.UnloadSessionModels(90291U), SUCCESS);
}

TEST_F(Om2SessionUnloadUT, InFlightReferenceKeepsExecutorAliveAfterSessionRemoval) {
  uint32_t id;
  ASSERT_NO_FATAL_FAILURE(LoadModel(90293U, id));
  MOCKER(ModelUtils::SetDevice).expects(once()).will(returnValue(SUCCESS));
  auto &manager = Om2ModelManager::GetInstance();
  auto running = manager.GetModelExecutor(id);
  std::weak_ptr<gert::Om2ModelExecutor> lifetime = running;
  ASSERT_EQ(manager.UnloadSessionModels(90293U), SUCCESS);
  EXPECT_EQ(manager.GetModelExecutor(id), nullptr);
  EXPECT_FALSE(lifetime.expired());
  running.reset();
  EXPECT_TRUE(lifetime.expired());
}
}  // namespace
}  // namespace ge
