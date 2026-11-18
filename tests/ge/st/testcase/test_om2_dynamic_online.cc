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
#include <memory>
#include <string>
#include <vector>
#include "ge/ge_api_v2.h"
#include "init_ge.h"
#include "graph/execute/model_executor.h"
#include "graph/manager/graph_var_manager.h"
#include "runtime/om2/om2_model_manager.h"
#include "graph/manager/graph_manager.h"
#include "graph/utils/graph_utils_ex.h"
#include "framework/common/helper/om2_package_helper.h"
#include "common/helper/om2/om2_utils.h"
#include "framework/runtime/om2_model_executor.h"
#include "framework/common/gert_model_data_utils.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "common/share_graph.h"
#include "faker/ge_model_builder.h"
#include "stub/gert_runtime_stub.h"
#include "../../common/om2_dynamic_host_graph.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

namespace ge {
namespace {
class Om2DynamicOnlineST : public om2_test::DynamicPackageTest {
 protected:
  void SetUp() override {
    DynamicPackageTest::SetUp();
    GetThreadLocalContext().SetGraphOption(
        {{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM2)},
         {"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
    const std::map<AscendString, AscendString> options{
        {OPTION_MODEL_FORMAT, AscendString(std::to_string(MODEL_FORMAT_OM2).c_str())}};
    ASSERT_EQ(GEInitializeV2(options), SUCCESS);
    session_ = std::make_unique<GeSession>(options);
  }
  void TearDown() override {
    session_.reset();
    EXPECT_EQ(GEFinalizeV2(), SUCCESS);
    DynamicPackageTest::TearDown();
  }
  void AddAffine(const uint32_t id, const bool fail = false) {
    auto graph = gert::om2_test::AffineGraph(false, fail);
    ASSERT_NE(graph, nullptr);
    auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    const std::map<AscendString, AscendString> options{
        {OPTION_MODEL_FORMAT, AscendString(std::to_string(MODEL_FORMAT_OM2).c_str())},
        {"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}};
    ASSERT_EQ(session_->AddGraph(id, *facade, options), SUCCESS);
    ASSERT_EQ(session_->CompileGraph(id), SUCCESS);
    ASSERT_EQ(session_->LoadGraph(id, {}, nullptr), SUCCESS);
  }
  Status RunAffine(const uint32_t id, const int64_t count, void *stream, const float scale = 2.0F) {
    std::vector<float> input(count, 3.0F), output(count, -77.0F);
    std::array<float, 2> coefficients{scale, -3.0F};
    std::vector<gert::Tensor> inputs;
    inputs.emplace_back(gert::StorageShape({count}, {count}), gert::StorageFormat(FORMAT_ND, FORMAT_ND, {}),
                        gert::kOnHost, DT_FLOAT, input.data());
    inputs.emplace_back(gert::StorageShape({2}, {2}), gert::StorageFormat(FORMAT_ND, FORMAT_ND, {}), gert::kOnHost,
                        DT_FLOAT, coefficients.data());
    std::vector<gert::Tensor> outputs;
    outputs.emplace_back(gert::StorageShape({count}, {count}), gert::StorageFormat(FORMAT_ND, FORMAT_ND, {}),
                         gert::kOnHost, DT_FLOAT, output.data());
    outputs[0].SetSize(output.size() * sizeof(float));
    const auto ret = stream == nullptr ? session_->RunGraph(id, inputs, outputs)
                                       : session_->RunGraphWithStreamAsync(id, stream, inputs, outputs);
    if (ret == SUCCESS) {
      if (stream != nullptr) {
        EXPECT_EQ(aclrtSynchronizeStream(stream), ACL_SUCCESS);
      }
      if (outputs.size() != 1U) {
        ADD_FAILURE() << "unexpected output count";
        return FAILED;
      }
      EXPECT_EQ(outputs[0].GetStorageShape(), gert::Shape({count}));
      EXPECT_EQ(outputs[0].GetDataType(), DT_FLOAT);
      EXPECT_EQ(outputs[0].GetPlacement(), gert::kOnHost);
      EXPECT_EQ(outputs[0].GetSize(), input.size() * sizeof(float));
      const auto *actual = outputs[0].GetData<float>();
      if (actual == nullptr || outputs[0].GetSize() < input.size() * sizeof(float)) {
        ADD_FAILURE() << "missing output storage";
        return FAILED;
      }
      for (int64_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(actual[i], 3.0F * scale - 3.0F);
      }
      if (stream != nullptr) {
        EXPECT_EQ(actual, output.data());
      }
    }
    return ret;
  }
  gert::GertRuntimeStub stub_;
  std::unique_ptr<GeSession> session_;
};

/**
 * 用例描述：动态在线模型独立卸载、运行失败恢复和图 ID 复用。
 * 预置条件：真实生成普通及可控失败的 Host 模型，仅 RTS 和运算 Kernel 为测试桩。
 * 测试步骤：同一 Session 加载两个模型并交替同步运行；移除第一个后继续运行第二个；
 *           第二个模型正常执行、失败、恢复后移除；复用第一个图 ID，以 stream 执行变化 shape。
 * 预期结果：每轮数值、shape、placement 正确；卸载不影响其他模型；失败不残留运行中标记；
 *           ID 可复用，异步执行使用用户输出内存，所有模型均可删除。
 */
TEST_F(Om2DynamicOnlineST, ExecuteGraphsPreservePeersRecoverFromFailureAndReuseId) {
  // slow_test_limit = 7500
  // Three real compilations cover peer isolation, failure recovery and ID reuse.
  ASSERT_NO_FATAL_FAILURE(AddAffine(701U));
  ASSERT_NO_FATAL_FAILURE(AddAffine(703U, true));
  ASSERT_EQ(RunAffine(701U, 2, nullptr), SUCCESS);
  ASSERT_EQ(RunAffine(703U, 4, nullptr), SUCCESS);
  ASSERT_EQ(RunAffine(701U, 3, nullptr), SUCCESS);
  ASSERT_EQ(RunAffine(701U, 5, nullptr), SUCCESS);
  ASSERT_EQ(session_->RemoveGraph(701U), SUCCESS);
  ASSERT_EQ(RunAffine(703U, 6, nullptr), SUCCESS);
  ASSERT_EQ(RunAffine(703U, 3, nullptr), SUCCESS);
  EXPECT_NE(RunAffine(703U, 3, nullptr, -9999.0F), SUCCESS);
  ASSERT_EQ(RunAffine(703U, 5, nullptr), SUCCESS);
  ASSERT_EQ(session_->RemoveGraph(703U), SUCCESS);

  // Session forbids mixing synchronous and asynchronous APIs on one loaded graph.
  ASSERT_NO_FATAL_FAILURE(AddAffine(701U));
  aclrtStream stream = nullptr;
  ASSERT_EQ(aclrtCreateStream(&stream), ACL_SUCCESS);
  const auto first_ret = RunAffine(701U, 3, stream);
  const auto second_ret = RunAffine(701U, 5, stream);
  EXPECT_EQ(aclrtDestroyStream(stream), ACL_SUCCESS);
  ASSERT_EQ(first_ret, SUCCESS);
  ASSERT_EQ(second_ret, SUCCESS);
  ASSERT_EQ(session_->RemoveGraph(701U), SUCCESS);
}
void BuildGatherRoot(GeRootModelPtr &root) {
  auto graph = gert::ShareGraph::BuildGatherShapesGraph();
  ASSERT_NE(graph, nullptr);
  // Shape-only graph: this case checks online execution without a weight dependency.
  auto second_desc = graph->FindNode("const1")->GetOpDesc();
  OpDescUtilsEx::SetType(second_desc, DATA);
  ASSERT_TRUE(AttrUtils::SetInt(second_desc, "index", 1));
  const GeTensorDesc second_input_desc(GeShape({-1, -1}), FORMAT_ND, DT_INT64);
  ASSERT_EQ(second_desc->UpdateOutputDesc(0U, second_input_desc), GRAPH_SUCCESS);
  ASSERT_EQ(second_desc->AddInputDesc("x", second_input_desc), GRAPH_SUCCESS);
  graph->SetInputSize(2U);
  ASSERT_NE(graph->AddInputNode(graph->FindNode("const1")), nullptr);
  auto gather_desc = graph->FindNode("gathershapes")->GetOpDesc();
  // The shared GraphConverter fixture predates IR recovery and declares x as
  // Required. Full ModelConverter needs the current registered Dynamic input.
  OpDescUtilsEx::SetType(gather_desc, "GatherShapes");
  auto &input_names = gather_desc->MutableAllInputName();
  input_names.clear();
  input_names["x0"] = 0U;
  input_names["x1"] = 1U;
  ASSERT_EQ(gather_desc->UpdateInputDesc(1U, second_input_desc), GRAPH_SUCCESS);
  const GeTensorDesc output_desc(GeShape({2}), FORMAT_ND, DT_INT64);
  ASSERT_EQ(gather_desc->UpdateOutputDesc(0U, output_desc), GRAPH_SUCCESS);
  ASSERT_EQ(graph->FindNode("NetOutput")->GetOpDescBarePtr()->UpdateInputDesc(0U, output_desc), GRAPH_SUCCESS);
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);

  root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
}

/**
 * 用例描述：GE 计算图动态生成产物经 ModelExecutor 和真实动态插件执行。
 * 预置条件：GE 编译模型使用 faker；lowering、打包、动态上下文、pbody 均不打桩，RTS 使用桩。
 * 测试步骤：生成 GatherShapes 模型，加载，使用空输出执行变化 shape，卸载。
 * 预期结果：真实动态执行器被选择；输出数值和描述正确；源图可在模型生命周期内使用。
 */
TEST_F(Om2DynamicOnlineST, GeGeneratedModelRunsThroughDynamicOnlineExecutor) {
  // slow_test_limit = 4000
  // End-to-end execution includes compiling generated model SOs.
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildGatherRoot(root));
  auto generated = std::make_shared<gert::GertModelData>();
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, *generated), SUCCESS);
  ASSERT_EQ(gert::GetGertModelExecutionType(*generated->models[0]), gert::GertModelExecutionType::kDynamic);
  root->SetOm2ModelData(generated);
  constexpr uint64_t session_id = 90281U;
  ASSERT_EQ(VarManager::Instance(session_id)->Init(0U, session_id, 0U, 0U), SUCCESS);
  ModelExecutor executor;
  ASSERT_EQ(executor.Initialize({}, session_id), SUCCESS);
  struct Cleanup {
    ModelExecutor &executor;
    ~Cleanup() {
      (void)executor.Finalize();
      VarManagerPool::Instance().RemoveVarManager(90281U);
    }
  } cleanup{executor};
  auto node = std::make_shared<GraphNode>(705U);
  node->SetGeRootModel(root);
  node->SetOptions({{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM2)}});
  GetThreadLocalContext().SetGraphOption({{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM2)}});
  ASSERT_EQ(executor.LoadGraph(root, node), SUCCESS);
  const auto loaded = Om2ModelManager::GetInstance().GetModelExecutor(root->GetModelId());
  ASSERT_NE(loaded, nullptr);
  EXPECT_TRUE(loaded->IsDynamicModel());
  for (const int64_t dimension : {3, 7}) {
    std::vector<gert::Tensor> inputs;
    inputs.emplace_back(gert::StorageShape({1, 2, dimension, 4}, {1, 2, dimension, 4}),
                        gert::StorageFormat(FORMAT_ND, FORMAT_ND, {}), gert::kOnHost, DT_FLOAT, nullptr);
    inputs.emplace_back(gert::StorageShape({dimension + 1, 6}, {dimension + 1, 6}),
                        gert::StorageFormat(FORMAT_ND, FORMAT_ND, {}), gert::kOnHost, DT_INT64, nullptr);
    std::vector<gert::Tensor> outputs;
    node->SetRunFlag(true);
    ASSERT_EQ(executor.RunGraph(node, 705U, inputs, outputs), SUCCESS);
    EXPECT_FALSE(node->GetRunFlag());
    ASSERT_EQ(outputs.size(), 1U);
    EXPECT_EQ(outputs[0].GetDataType(), DT_INT64);
    EXPECT_EQ(outputs[0].GetStorageShape(), gert::Shape({2}));
    ASSERT_NE(outputs[0].GetAddr(), nullptr);
    EXPECT_EQ(outputs[0].GetData<int64_t>()[0], dimension + 1);
    EXPECT_EQ(outputs[0].GetData<int64_t>()[1], dimension);
  }
  EXPECT_EQ(executor.UnloadGraph(root, 705U), SUCCESS);
}
}  // namespace
}  // namespace ge
