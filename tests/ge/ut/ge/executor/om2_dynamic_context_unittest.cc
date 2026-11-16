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
#include <array>
#include <fstream>
#include <cstdio>
#include <memory>
#include "exe_graph/runtime/tensor.h"
#include "om2_dynamic_loader.h"
#include "common/om2/om2_online_prepare_data.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/om2/model_api/om2_model_api.h"
#include "framework/runtime/rt_session.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/mem_manager.h"
#include "graph/manager/graph_external_weight_manager.h"
#include "graph/ge_local_context.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/tensor_utils_ex.h"
#include "graph/debug/ge_attr_define.h"
#include "common/env_path.h"
#include "stub/gert_runtime_stub.h"

namespace gert {
namespace {
using namespace ge;
struct ContextDeleter {
  void operator()(void *context) const {
    Om2DynamicApi::Instance().Destroy(context);
  }
};
using Context = std::unique_ptr<void, ContextDeleter>;
class Om2DynamicContextUT : public testing::Test {
 protected:
  void SetUp() override {
    options_ = GetThreadLocalContext().GetAllGraphOptions();
    GetThreadLocalContext().SetGraphOption({{"ge.exec.placement", "HOST"}});
    for (auto id : {kSession, kOtherSession}) {
      ASSERT_EQ(VarManager::Instance(id)->Init(0U, id, 0U, 0U), SUCCESS);
    }
    InitGertModelData(data_);
    InitGertModelDataUnit(data_);
    data_.models[0]->execution_type = GertModelExecutionType::kDynamic;
    data_.online_prepare_data = std::make_shared<Om2OnlinePrepareData>();
    data_.online_prepare_data->prepare_ge_variables = true;
    data_.online_prepare_data->graph_id = 1U;
    graph_ = std::make_shared<ComputeGraph>("online_resources");
    data_.online_prepare_data->graphs.push_back(graph_);
  }
  void TearDown() override {
    for (auto id : {kSession, kOtherSession}) {
      VarManager::Instance(id)->FreeVarMemory();
      MemManager::Instance().FreeSessionMemory(id);
      VarManagerPool::Instance().RemoveVarManager(id);
      ExternalWeightManagerPool::Instance().GetManager(id)->Finalize();
    }
    GetThreadLocalContext().SetGraphOption(options_);
  }
  NodePtr AddResource(const char *name, const char *type) {
    auto desc = std::make_shared<OpDesc>(name, type);
    GeTensorDesc tensor(GeShape({2}), FORMAT_ND, DT_INT32);
    tensor.SetOriginShape(GeShape({2}));
    int64_t memory_size = 0;
    if (TensorUtilsEx::GetTensorMemorySizeInBytesWithAutoPadding(tensor, memory_size) != GRAPH_SUCCESS) {
      return nullptr;
    }
    TensorUtils::SetSize(tensor, memory_size);
    if (desc->AddOutputDesc(tensor) != GRAPH_SUCCESS || !AttrUtils::SetStr(desc, ATTR_VARIABLE_PLACEMENT, "host")) {
      return nullptr;
    }
    return graph_->AddNode(desc);
  }
  Status Create(Context &context, const uint64_t session = kSession) {
    Om2DynamicCreateInput input;
    input.model_data = &data_;
    input.session_id = session;
    input.runtime_context_valid = 1U;
    Om2DynamicCreateOutput output;
    const auto ret = Om2DynamicApi::Instance().Create(input, output);
    context.reset(output.context);
    return ret;
  }
  const RtSession *Session(const Context &context) {
    GertModelLoadConfig config;
    if (Om2DynamicApi::Instance().PrepareLoad(context.get(), config) != SUCCESS || config.dynamic_config == nullptr) {
      ADD_FAILURE() << "missing dynamic load context";
      return nullptr;
    }
    return static_cast<const RtSession *>(config.dynamic_config->rt_session);
  }
  void Read(const Context &context, const char *name, TensorData &memory) {
    const auto *session = Session(context);
    ASSERT_NE(session, nullptr);
    ASSERT_NE(session->GetVarManager(), nullptr);
    StorageShape shape;
    ASSERT_EQ(session->GetVarManager()->GetVarShapeAndMemory(name, shape, memory), SUCCESS);
    EXPECT_EQ(shape.GetStorageShape(), Shape({2}));
    EXPECT_EQ(memory.GetPlacement(), kOnHost);
    ASSERT_GE(memory.GetSize(), 2U * sizeof(int32_t));
    ASSERT_NE(memory.GetAddr(), nullptr);
  }
  static constexpr uint64_t kSession = 90171U;
  static constexpr uint64_t kOtherSession = 90172U;
  GertRuntimeStub stub_;
  GertModelData data_;
  ComputeGraphPtr graph_;
  std::map<std::string, std::string> options_;
};

TEST_F(Om2DynamicContextUT, SameSessionSharesVariableAfterOneContextIsDestroyed) {
  auto var = AddResource("om2_shared_var", "Variable");
  ASSERT_NE(var, nullptr);
  data_.online_prepare_data->variables.push_back(var);
  Context first, second;
  ASSERT_EQ(Create(first), SUCCESS);
  ASSERT_EQ(Create(second), SUCCESS);
  TensorData a, b;
  ASSERT_NO_FATAL_FAILURE(Read(first, "om2_shared_var", a));
  ASSERT_NO_FATAL_FAILURE(Read(second, "om2_shared_var", b));
  ASSERT_EQ(a.GetAddr(), b.GetAddr());
  static_cast<int32_t *>(a.GetAddr())[0] = 73;
  first.reset();
  ASSERT_NO_FATAL_FAILURE(Read(second, "om2_shared_var", b));
  EXPECT_EQ(static_cast<int32_t *>(b.GetAddr())[0], 73);
}

TEST_F(Om2DynamicContextUT, DifferentSessionsKeepSameNamedVariableIndependent) {
  auto var = AddResource("om2_isolated_var", "Variable");
  ASSERT_NE(var, nullptr);
  data_.online_prepare_data->variables.push_back(var);
  Context first, second;
  ASSERT_EQ(Create(first), SUCCESS);
  ASSERT_EQ(Create(second, kOtherSession), SUCCESS);
  TensorData a, b;
  ASSERT_NO_FATAL_FAILURE(Read(first, "om2_isolated_var", a));
  ASSERT_NO_FATAL_FAILURE(Read(second, "om2_isolated_var", b));
  ASSERT_NE(a.GetAddr(), b.GetAddr());
  static_cast<int32_t *>(a.GetAddr())[0] = 17;
  static_cast<int32_t *>(b.GetAddr())[0] = 29;
  EXPECT_EQ(static_cast<int32_t *>(a.GetAddr())[0], 17);
}

TEST_F(Om2DynamicContextUT, SharedHostConstantKeepsSourceGraphAliveUntilDestroy) {
  auto constant = AddResource("om2_shared_constant", "Constant");
  ASSERT_NE(constant, nullptr);
  std::array<int32_t, 2> values{31, 47};
  GeTensor weight(constant->GetOpDesc()->GetOutputDesc(0), reinterpret_cast<const uint8_t *>(values.data()),
                  sizeof(values));
  ASSERT_TRUE(AttrUtils::SetTensor(constant->GetOpDesc(), ATTR_NAME_WEIGHTS, weight));
  data_.online_prepare_data->shared_constants.push_back(constant);
  std::weak_ptr<ComputeGraph> lifetime = graph_;
  Context context;
  ASSERT_EQ(Create(context), SUCCESS);
  constant.reset();
  graph_.reset();
  data_.online_prepare_data.reset();
  EXPECT_FALSE(lifetime.expired());
  TensorData memory;
  ASSERT_NO_FATAL_FAILURE(Read(context, "om2_shared_constant", memory));
  EXPECT_EQ(static_cast<const int32_t *>(memory.GetAddr())[0], 31);
  EXPECT_EQ(static_cast<const int32_t *>(memory.GetAddr())[1], 47);
  context.reset();
  EXPECT_TRUE(lifetime.expired());
}

TEST_F(Om2DynamicContextUT, MissingFileConstantFailsWithoutPublishingContextAndCanRetry) {
  auto file = AddResource("om2_file", "FileConstant");
  ASSERT_NE(file, nullptr);
  const auto path = EnvPath().GetOrCreateCaseTmpPath("Om2DynamicContextUT") + "/weights.bin";
  struct RemoveFile {
    std::string path;
    ~RemoveFile() {
      (void)std::remove(path.c_str());
    }
  } cleanup{path};
  (void)std::remove(path.c_str());
  ASSERT_TRUE(AttrUtils::SetStr(file->GetOpDesc(), "location", path));
  ASSERT_TRUE(AttrUtils::SetInt(file->GetOpDesc(), "offset", 0));
  ASSERT_TRUE(AttrUtils::SetInt(file->GetOpDesc(), "length", 2U * sizeof(int32_t)));
  data_.online_prepare_data->file_constants.push_back(file);
  Context context;
  EXPECT_NE(Create(context), SUCCESS);
  EXPECT_EQ(context, nullptr);
  // Repair the file and reset its session cache before retrying context creation.
  std::array<int32_t, 2> values{53, 59};
  {
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char *>(values.data()), sizeof(values));
    ASSERT_TRUE(stream.good());
  }
  ExternalWeightManagerPool::Instance().GetManager(kSession)->Finalize();
  ASSERT_EQ(Create(context), SUCCESS);
  TensorData memory;
  ASSERT_NO_FATAL_FAILURE(Read(context, "om2_file", memory));
  EXPECT_EQ(static_cast<const int32_t *>(memory.GetAddr())[0], 53);
  EXPECT_EQ(static_cast<const int32_t *>(memory.GetAddr())[1], 59);
  context.reset();
  (void)std::remove(path.c_str());
}

TEST_F(Om2DynamicContextUT, PrepareRunClearsOnlyNonNullZeroCapacityOutputsAndRebindsAllocator) {
  Context context;
  ASSERT_EQ(Create(context), SUCCESS);
  int32_t storage[2] = {};
  Tensor zero, supplied;
  zero.SetData(TensorData(storage, nullptr, 0U, kOnDeviceHbm));
  supplied.SetData(TensorData(storage, nullptr, sizeof(storage), kOnHost));
  Tensor *outputs[] = {&zero, &supplied};
  GertModelRunConfig run;
  run.output_count = 2U;
  run.output_data = outputs;
  int allocator_a, allocator_b;
  auto &api = Om2DynamicApi::Instance();
  ASSERT_EQ(api.PrepareRun(context.get(), &allocator_a, run), SUCCESS);
  EXPECT_EQ(zero.GetAddr(), nullptr);
  EXPECT_EQ(zero.GetPlacement(), kOnDeviceHbm);
  EXPECT_EQ(supplied.GetAddr(), storage);
  EXPECT_EQ(supplied.GetSize(), sizeof(storage));
  EXPECT_EQ(run.external_allocator, &allocator_a);
  ASSERT_EQ(api.PrepareRun(context.get(), nullptr, run), SUCCESS);
  EXPECT_EQ(run.external_allocator, nullptr);
  ASSERT_EQ(api.PrepareRun(context.get(), &allocator_b, run), SUCCESS);
  EXPECT_EQ(run.external_allocator, &allocator_b);
}

TEST_F(Om2DynamicContextUT, BorrowedSessionTakesPrecedenceOverOnlinePreparation) {
  RtSession borrowed(123U);
  Om2DynamicCreateInput input;
  input.model_data = &data_;
  input.rt_session = &borrowed;
  // runtime_context_valid remains false: owned preparation must not run.
  Om2DynamicCreateOutput output;
  ASSERT_EQ(Om2DynamicApi::Instance().Create(input, output), SUCCESS);
  Context context(output.context);
  EXPECT_EQ(Session(context), &borrowed);
  context.reset();
  EXPECT_EQ(borrowed.GetSessionId(), 123U);
  EXPECT_NE(borrowed.GetStreamAllocator(), nullptr);
}
}  // namespace
}  // namespace gert
