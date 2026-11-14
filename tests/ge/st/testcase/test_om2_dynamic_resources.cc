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
#include <string>
#include <vector>
#include "framework/common/helper/om2_package_helper.h"
#include "framework/runtime/rt_session.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "stub/gert_runtime_stub.h"
#include "../../common/om2_dynamic_host_graph.h"
#include "../../common/om2_generated_model_runner.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

namespace ge {
namespace {
class Om2DynamicResourcesST : public om2_test::DynamicPackageTest {
 protected:
  void SetUp() override {
    DynamicPackageTest::SetUp();
    GetThreadLocalContext().SetGraphOption(
        {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  }
};

void BuildKnownRoot(GeRootModelPtr &root) {
  auto graph = gert::ShareGraph::BuildWithKnownSubgraph();
  ASSERT_NE(graph, nullptr);
  ASSERT_EQ(graph->GetAllSubgraphs().size(), 1U);
  const auto subgraph = graph->GetAllSubgraphs().front();
  const auto constant = subgraph->FindNode("const1");
  ASSERT_NE(constant, nullptr);
  const std::array<float, 4U> values{7.0F, -2.0F, 13.0F, 0.5F};
  const GeTensorDesc desc(GeShape({2, 2}), FORMAT_NCHW, DT_FLOAT);
  const GeTensor weight(desc, reinterpret_cast<const uint8_t *>(values.data()), sizeof(values));
  ASSERT_TRUE(AttrUtils::SetTensor(constant->GetOpDesc(), "value", weight));
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
  const auto model = root->GetSubgraphInstanceNameToModel().at(subgraph->GetName());
  ASSERT_NE(model, nullptr);
  ASSERT_EQ(model->GetWeightSize(), sizeof(values));
  ASSERT_TRUE(AttrUtils::SetInt(model, ATTR_MODEL_WEIGHT_SIZE, sizeof(values)));
  // This fixture's constant is the third static output; its compiled weight offset must match the buffer.
  constant->GetOpDesc()->SetOutputOffset({0});
  const auto output = subgraph->FindNode("netoutput_sub")->GetOpDesc();
  output->SetInputOffset({48, 0, 0});
  output->SetIsInputConst({false, false, true});
  TensorUtils::SetDataOffset(*output->MutableInputDesc(2U), 0);
}

void CheckKnownSources(const gert::GertModelData &generated) {
  ASSERT_EQ(generated.models.size(), 1U);
  ASSERT_NE(generated.models[0], nullptr);
  ASSERT_NE(generated.models[0]->runtime, nullptr);
  ASSERT_GT(generated.models[0]->runtime->so_artifact.data_size, 0U);
  std::string source;
  for (const auto &artifact : generated.models[0]->runtime->source_artifacts) {
    if (artifact.data != nullptr) {
      source.append(reinterpret_cast<const char *>(artifact.data.get()), artifact.data_size);
    }
  }
  for (const auto *type :
       {"DavinciModelCreate", "DavinciModelGetRunAddress", "DavinciModelExecute", "DavinciModelFinalizer"}) {
    EXPECT_NE(source.find(type), std::string::npos) << type;
  }
}

void RunKnownOutputs(om2_test::GeneratedModelRunner &runner, const float first) {
  std::array<float, 4U> values{first, 2.0F, -3.0F, 4.0F};
  std::array<std::array<float, 4U>, 3U> storage{};
  gert::Tensor input({{2, 2}, {2, 2}}, {FORMAT_NCHW, FORMAT_NCHW, {}}, gert::kOnDeviceHbm, DT_FLOAT, values.data());
  input.SetSize(sizeof(values));
  std::vector<gert::Tensor> output_tensors;
  for (auto &buffer : storage) {
    buffer.fill(-777.0F);
    output_tensors.emplace_back(gert::StorageShape({2, 2}, {2, 2}), gert::StorageFormat(FORMAT_NCHW, FORMAT_NCHW, {}),
                                gert::kOnDeviceHbm, DT_FLOAT, buffer.data());
    output_tensors.back().SetSize(sizeof(buffer));
  }
  gert::Tensor *inputs[]{&input};
  gert::Tensor *outputs[]{&output_tensors[0], &output_tensors[1], &output_tensors[2]};
  GertModelRunConfig run;
  run.input_count = 1U;
  run.input_data = inputs;
  run.output_count = 3U;
  run.output_data = outputs;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  ASSERT_NE(output_tensors[1].GetData<float>(), nullptr);
  ASSERT_NE(output_tensors[2].GetData<float>(), nullptr);
  const std::array<float, 4U> weights{7.0F, -2.0F, 13.0F, 0.5F};
  // RTS does not compute Conv2d/Relu. The other outputs prove input and weight address binding.
  for (size_t i = 0U; i < values.size(); ++i) {
    EXPECT_FLOAT_EQ(output_tensors[1].GetData<float>()[i], values[i]);
    EXPECT_FLOAT_EQ(output_tensors[2].GetData<float>()[i], weights[i]);
  }
  for (const auto &output : output_tensors) {
    EXPECT_EQ(output.GetStorageShape(), gert::Shape({2, 2}));
    EXPECT_EQ(output.GetDataType(), DT_FLOAT);
    EXPECT_EQ(output.GetSize(), sizeof(values));
  }
}

/**
 * 用例描述：在线 known-shape 子图经真实 OM2 生成、加载、两轮执行和卸载。
 * 预置条件：复用 KnownSubgraph ST 图和 GeModelBuilder；只使用编译模型及 RTS 桩。
 * 测试步骤：在线生成模型 SO，检查创建/地址获取/执行/释放代码；加载后改变输入重复运行。
 * 预期结果：输入直通与常量输出逐元素正确；生命周期成功。RTS 桩不验证 Conv2d 数值。
 */
TEST_F(Om2DynamicResourcesST, OnlineKnownSubgraphPreservesInputAndWeightAcrossRuns) {
  // slow_test_limit = 4000
  gert::GertRuntimeStub stub;
  om2_test::ScopedEnvVar online("ENABLE_RUNTIME_OM2", "1");
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildKnownRoot(root));
  gert::GertModelData generated;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, generated), SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckKnownSources(generated));
  std::vector<void *> constants(root->GetSubgraphInstanceNameToModel().size(), nullptr);
  ASSERT_NE(generated.constants, nullptr);
  const auto &metadata = generated.models[0]->constants_config;
  ASSERT_NE(metadata, nullptr);
  ASSERT_EQ(metadata->consts.size(), generated.constants->constants_data.size());
  for (size_t i = 0U; i < metadata->consts.size(); ++i) {
    const auto &constant = generated.constants->constants_data[i];
    ASSERT_NE(constant, nullptr);
    ASSERT_NE(metadata->consts[i], nullptr);
    ASSERT_LT(metadata->consts[i]->index, constants.size());
    constants[metadata->consts[i]->index] = const_cast<uint8_t *>(constant->data.get());
  }
  gert::RtSession session;
  struct Cleanup {
    gert::RtSession &session;
    ~Cleanup() {
      session.DestroyResources();
    }
  } cleanup{session};
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(generated.models[0]->runtime->so_artifact)) << runner.Error();
  GertModelDynamicLoadConfig dynamic;
  dynamic.rt_session = &session;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic;
  load.constants = constants.data();
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(RunKnownOutputs(runner, 1.0F));
  ASSERT_NO_FATAL_FAILURE(RunKnownOutputs(runner, 19.0F));
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

void BadNames(ExecuteGraph &graph) {
  ASSERT_TRUE(AttrUtils::SetListStr(graph, gert::attr::kModelInputNames, {"input", "input"}));
}

void MissingWeight(ExecuteGraph &graph) {
  const auto node = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Const");
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(node->GetOpDescPtr(), gert::attr::kNodeWeightId, "absent"));
}

void BadPorts(ExecuteGraph &graph) {
  const auto affine = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Om2ExternalGeneratorAffine");
  const auto input = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Data");
  ASSERT_NE(affine, nullptr);
  ASSERT_NE(input, nullptr);
  ASSERT_EQ(affine->GetOpDescPtr()->AddInputDesc(input->GetOpDescPtr()->GetOutputDesc(0)), GRAPH_SUCCESS);
  affine->UpdateDataInNum(4U);
  ASSERT_NE(graph.AddEdge(input, 0, affine, 3), nullptr);
}

void ReversedStages(ExecuteGraph &graph) {
  const auto affine = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Om2ExternalGeneratorAffine");
  const auto input = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Data");
  ASSERT_NE(affine, nullptr);
  ASSERT_NE(input, nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(input->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageMain));
  ASSERT_TRUE(AttrUtils::SetStr(affine->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
}

/**
 * 用例描述：错误元数据、资源引用、Kernel 端口及阶段依赖不能发布 OM2 模型。
 * 预置条件：每行重新构造已有合法 affine 执行图，只注入一个错误，不替换校验器。
 * 测试步骤：表驱动修改图，经 BuildGertModelDataFromGraph 公开构建入口尝试生成。
 * 预期结果：每行均命中对应错误诊断，返回失败且输出模型列表为空。
 */
TEST_F(Om2DynamicResourcesST, InvalidGraphsRejectBeforePublishingModel) {
  struct InvalidCase {
    const char *name;
    bool weight;
    void (*mutate)(ExecuteGraph &);
    const char *diagnostic;
  };
  const InvalidCase cases[]{
      {"metadata", false, BadNames, "Model input description name input is used twice"},
      {"weight_reference", true, MissingWeight, "Node references weight absent which is not an embedded resource"},
      {"ports", false, BadPorts, "Failed to collect OM2 kernel affine type Om2ExternalGeneratorAffine"},
      {"stage_dependency", false, ReversedStages, "produced in later stage main"}};
  gert::GertRuntimeStub stub;
  for (const auto &item : cases) {
    SCOPED_TRACE(item.name);
    auto graph = gert::om2_test::AffineGraph(item.weight);
    ASSERT_NE(graph, nullptr);
    ASSERT_NO_FATAL_FAILURE(item.mutate(*graph));
    auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    stub.GetSlogStub().Clear();
    gert::GertModelData generated;
    EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, generated), SUCCESS);
    EXPECT_TRUE(generated.models.empty());
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, item.diagnostic), -1);
  }
}
}  // namespace
}  // namespace ge
