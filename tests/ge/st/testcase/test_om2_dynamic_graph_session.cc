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
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/env_path.h"
#include "framework/om2/model_data/gert_model_data.h"
#include "ge/ge_api.h"
#include "graph/ge_context.h"
#include "framework/common/gert_model_data_utils.h"
#include "common/helper/om2/om2_utils.h"
#include "common/memory/tensor_trans_utils.h"
#include "common/path_utils.h"
#include "framework/common/helper/om2_package_helper.h"
#include "graph/execute/model_executor.h"
#include "graph/ge_local_context.h"
#include "graph/manager/graph_manager.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/graph.h"
#include "graph/op_desc.h"
#include "graph/utils/file_utils.h"
#include "init_ge.h"
#include "mmpa/mmpa_api.h"
#include "common/model/ge_root_model.h"
#include "ge/ge_api_v2.h"
#include "session/ge_session_impl.h"

#include "ge_runtime_stub/include/common/share_graph.h"
#include "ge_runtime_stub/include/faker/aicore_taskdef_faker.h"
#include "ge_runtime_stub/include/faker/ge_model_builder.h"

#include "om2_online_test_utils.h"

namespace ge {
std::shared_ptr<gert::GertModelData> GetExecuteModelData(const GraphNodePtr &node) {
  const auto root = node->GetGeRootModel();
  return root == nullptr ? nullptr : root->GetOm2ModelData();
}

void SetExecuteModelData(const GraphNodePtr &node, const std::shared_ptr<gert::GertModelData> &data) {
  auto root = std::make_shared<GeRootModel>();
  root->SetOm2ModelData(data);
  node->SetGeRootModel(root);
}

class Om2ExecuteGraphSessionTest : public Om2OnlineModelExecutorTest {
 protected:
  void SetUp() override {
    EnableOm2OnlineMode();
    const std::map<AscendString, AscendString> options;
    ASSERT_EQ(GEInitializeV2(options), SUCCESS);
    session_ = std::make_unique<GeSession>(options);
  }

  void TearDown() override {
    session_.reset();
    EXPECT_EQ(GEFinalizeV2(), SUCCESS);
    Om2OnlineModelExecutorTest::TearDown();
  }

  void AddCompiledGraph(const uint32_t id, GraphNodePtr &node) {
    const auto inner = session_->impl_->GetInnerSession();
    ASSERT_NE(inner, nullptr);
    node = std::make_shared<GraphNode>(id);
    node->SetGraph(std::make_shared<Graph>("execute_session_st", static_cast<GraphKind>(1)));
    inner->graph_manager_.AddGraphNode(id, node);
    inner->graph_manager_.IncreaseGraphCount(id);
    inner->graph_manager_.SetAddGraphCondition(id, 2U);
    inner->run_graph_modes_.emplace(id, RunGraphMode::kRunGraphModeEnd);
    // Isolate generator compilation: reuse this suite's host program SO and exercise the real Session/loader chain.
    auto data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
    data->models[0]->model_meta->input_desc[0].format = FORMAT_ND;
    data->models[0]->model_meta->input_desc_v2[0].format = FORMAT_ND;
    data->models[0]->model_meta->output_desc[0].format = FORMAT_ND;
    data->models[0]->model_meta->output_desc_v2[0].format = FORMAT_ND;
    SetExecuteModelData(node, data);
    node->SetBuildFlag(true);
  }

  std::vector<gert::Tensor> MakeInputs() {
    gert::Tensor input;
    input.SetDataType(DT_FLOAT);
    input.SetOriginFormat(FORMAT_ND);
    input.SetStorageFormat(FORMAT_ND);
    input.MutableOriginShape() = gert::Shape({1, 4});
    input.MutableStorageShape() = gert::Shape({1, 4});
    input.SetData(gert::TensorData(input_data_.data(), nullptr, sizeof(input_data_), gert::kOnHost));
    std::vector<gert::Tensor> inputs;
    inputs.emplace_back(std::move(input));
    return inputs;
  }

  EnvValueGuard om2_env_guard_{"ENABLE_RUNTIME_OM2"};
  std::unique_ptr<GeSession> session_;
  std::array<float, 4> input_data_{{1.0F, 2.0F, 3.0F, 4.0F}};
};

/**
 * 用例描述：执行图经 Session 加载内存产物、重复同步执行、删除并复用 ID。
 * 预置条件：RTS 使用 ST 桩；注入本套件的 Host 测试 SO 作为已编译产物，不验证生成器或 NPU 精度。
 * 测试步骤：Add/注入产物/Compile/Load/两轮 Run/Remove，再以相同 ID 重复生命周期。
 * 预期结果：产物身份保持，输出描述正确，未创建计算图资源，删除后记录消失且引用释放。
 */
TEST_F(Om2ExecuteGraphSessionTest, ExplicitLifecycleReusesGraphIdAndPreservesCompiledData) {
  constexpr uint32_t id = 5101U;
  for (size_t cycle = 0U; cycle < 2U; ++cycle) {
    SCOPED_TRACE(cycle);
    GraphNodePtr node;
    ASSERT_NO_FATAL_FAILURE(AddCompiledGraph(id, node));
    const auto *const compiled = GetExecuteModelData(node).get();
    std::weak_ptr<gert::GertModelData> weak_data = GetExecuteModelData(node);
    ASSERT_EQ(session_->CompileGraph(id), SUCCESS);
    EXPECT_EQ(GetExecuteModelData(node).get(), compiled);
    EXPECT_FALSE(node->GetLoadFlag());
    ASSERT_EQ(session_->LoadGraph(id, {}, nullptr), SUCCESS);
    const auto loaded = node->GetGeRootModel()->GetModelId();
    ASSERT_NE(loaded, INVALID_MODEL_ID);
    EXPECT_NE(session_->LoadGraph(id, {}, nullptr), SUCCESS);
    EXPECT_EQ(node->GetGeRootModel()->GetModelId(), loaded);
    for (size_t run = 0U; run < 2U; ++run) {
      auto inputs = MakeInputs();
      std::vector<gert::Tensor> outputs;
      ASSERT_EQ(session_->RunGraph(id, inputs, outputs), SUCCESS);
      ASSERT_EQ(outputs.size(), 1U);
      EXPECT_EQ(outputs[0].GetDataType(), DT_FLOAT);
      EXPECT_EQ(outputs[0].GetStorageFormat(), FORMAT_ND);
      EXPECT_EQ(outputs[0].GetStorageShape(), gert::Shape({1, 4}));
      EXPECT_EQ(GetExecuteModelData(node).get(), compiled);
      EXPECT_FALSE(node->GetRunFlag());
    }
    EXPECT_EQ(node->GetComputeGraph(), nullptr);
    EXPECT_NE(node->GetGeRootModel(), nullptr);
    ASSERT_EQ(session_->RemoveGraph(id), SUCCESS);
    EXPECT_EQ(GetExecuteModelData(node).get(), compiled);
    node.reset();
    EXPECT_TRUE(weak_data.expired());
    GraphNodePtr removed;
    EXPECT_EQ(session_->impl_->GetInnerSession()->getGraphManagerObj().GetGraphNode(id, removed),
              GE_GRAPH_GRAPH_NOT_EXIST);
    EXPECT_NE(session_->CompileGraph(id), SUCCESS);
    EXPECT_EQ(session_->RemoveGraph(id), FAILED);
  }
}

/**
 * 用例描述：非空加载流被忽略，使用原加载流程加载后可正常执行。
 * 预置条件：RTS 使用 ST 桩；注入 Host 测试 SO；当前内存 OM2 加载器忽略加载 stream。
 * 测试步骤：指定流 Load 成功，检查编译产物不变，再 Run、Remove。
 * 预期结果：加载不丢失产物、不遗留忙状态，后续执行成功。
 */
TEST_F(Om2ExecuteGraphSessionTest, LoadStreamIsIgnoredAndCompiledGraphRemainsRunnable) {
  constexpr uint32_t id = 5102U;
  GraphNodePtr node;
  ASSERT_NO_FATAL_FAILURE(AddCompiledGraph(id, node));
  const auto *const compiled = GetExecuteModelData(node).get();
  int stream_token = 0;
  ASSERT_EQ(session_->LoadGraph(id, {}, &stream_token), SUCCESS);
  EXPECT_EQ(GetExecuteModelData(node).get(), compiled);
  EXPECT_TRUE(node->GetBuildFlag());
  EXPECT_TRUE(node->GetLoadFlag());
  EXPECT_FALSE(node->GetRunFlag());
  auto inputs = MakeInputs();
  std::vector<gert::Tensor> outputs;
  ASSERT_EQ(session_->RunGraph(id, inputs, outputs), SUCCESS);
  ASSERT_EQ(outputs.size(), 1U);
  outputs.clear();
  EXPECT_EQ(session_->RemoveGraph(id), SUCCESS);
}

}  // namespace ge
