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
#include "../../common/om2_generated_model_runner.h"
#include "../../common/om2_dynamic_host_graph.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "framework/common/helper/om2_package_helper.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "register/kernel_registry.h"
#include "stub/gert_runtime_stub.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

namespace gert {
namespace {
using Om2CustomKernelST = ge::om2_test::DynamicPackageTest;

void SetRegistration(ge::FastNode &node, const char *run, const char *creator) {
  auto *desc = node.GetOpDescBarePtr();
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kCustomKernelName, node.GetTypePtr()));
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kCustomKernelSoPath, OM2_CUSTOM_TEST_PLUGIN));
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kCustomRunFuncSymbol, run));
  if (creator != nullptr) {
    ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kCustomOutputsCreatorSymbol, creator));
  }
}

void BuildBufferGraph(ge::ExecuteGraph &graph, const char *type, const char *run, const char *creator,
                      ge::ModelBufferData &data, ge::ModelBufferData &empty) {
  const ge::GeTensorDesc port(ge::GeShape({1}), ge::FORMAT_ND, ge::DT_INT64);
  auto *read = om2_test::AddNode(graph, "read", type, {port, port}, {port});
  auto *write = om2_test::AddNode(graph, "write", "Om2CustomWriteChecksumST", {port, port}, {});
  auto *output = om2_test::AddNode(graph, "output", "OutputData", {}, {port});
  ASSERT_NE(read, nullptr);
  ASSERT_NE(write, nullptr);
  ASSERT_NE(output, nullptr);
  ASSERT_NO_FATAL_FAILURE(SetRegistration(*read, run, creator));
  ASSERT_NO_FATAL_FAILURE(SetRegistration(*write, "WriteChecksum", nullptr));
  int32_t index = 0;
  for (auto *buffer : {&data, &empty}) {
    auto *constant = om2_test::AddNode(graph, index == 0 ? "bytes" : "empty", "Const", {}, {port});
    ASSERT_NE(constant, nullptr);
    ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
        constant->GetOpDescBarePtr(), kConstValue,
        ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&buffer), sizeof(buffer))));
    ASSERT_NE(graph.AddEdge(constant, 0, read, index++), nullptr);
  }
  ASSERT_NE(graph.AddEdge(read, 0, write, 0), nullptr);
  ASSERT_NE(graph.AddEdge(output, 0, write, 1), nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, attr::kModelInputs, std::vector<ge::GeTensor>{}));
  ge::GeTensor output_tensor(port);
  output_tensor.MutableTensorDesc().SetName("checksum");
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, attr::kModelOutputs, {output_tensor}));
  ASSERT_EQ(ge::ExecuteGraphUtils::SetGraphOutputs(&graph, {{output, 0}}), ge::GRAPH_SUCCESS);
}

void CheckRegisteredCreator() {
  const auto *funcs = KernelRegistry::GetInstance().FindKernelFuncs("Om2CustomReadBufferST");
  ASSERT_NE(funcs, nullptr);
  ASSERT_NE(funcs->outputs_creator, nullptr);
  EXPECT_NE(funcs->outputs_creator(nullptr, nullptr), ge::GRAPH_SUCCESS);
  KernelContext context{};
  EXPECT_NE(funcs->outputs_creator(nullptr, &context), ge::GRAPH_SUCCESS);
  ge::ExecuteGraph creator_graph("creator_node");
  auto *read = om2_test::AddNode(creator_graph, "read", "Om2CustomReadBufferST", {}, {});
  ASSERT_NE(read, nullptr);
  Chain chain{};
  auto *run_context = context.GetContext();
  run_context->input_size = 0U;
  run_context->output_size = 1U;
  run_context->output_start = run_context->values;
  run_context->values[0] = reinterpret_cast<AsyncAnyValue *>(&chain);
  EXPECT_EQ(funcs->outputs_creator(read, &context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(chain.GetValue<int64_t>(), 11);
  read->GetOpDescBarePtr()->SetType("Om2UnknownOutputsCreatorST");
  EXPECT_NE(funcs->outputs_creator(read, &context), ge::GRAPH_SUCCESS);
}

TEST_F(Om2CustomKernelST, RunsDynamicallyRegisteredKernelsWithRestoredModelBuffers) {
  // slow_test_limit = 3000
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GertModelData model;
  {
    ge::ModelBufferData data;
    data.data.reset(new uint8_t[3U]{3U, 0U, 255U}, std::default_delete<uint8_t[]>());
    data.length = 3U;
    ge::ModelBufferData empty{};
    auto graph = std::make_shared<ge::ExecuteGraph>("custom_buffer");
    ASSERT_NO_FATAL_FAILURE(
        BuildBufferGraph(*graph, "Om2CustomReadBufferST", "ReadBuffer", "CreateChecksum", data, empty));
    auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    ASSERT_EQ(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
    ASSERT_NO_FATAL_FAILURE(CheckRegisteredCreator());
  }

  ASSERT_EQ(model.models.size(), 1U);
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(model.models[0]->runtime->so_artifact)) << runner.Error();
  for (int i = 0; i < 2; ++i) {
    ASSERT_EQ(runner.Load(GertModelLoadConfig{}), ACL_SUCCESS);
    int64_t checksum = -1;
    Tensor tensor({{1}, {1}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT64, &checksum);
    tensor.SetSize(sizeof(checksum));
    Tensor *outputs[] = {&tensor};
    GertModelRunConfig config;
    config.output_count = 1U;
    config.output_data = outputs;
    ASSERT_EQ(runner.Run(config), ACL_SUCCESS);
    EXPECT_EQ(checksum, 11 + 3 + 3 * 255);
    ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
  }
}

TEST_F(Om2CustomKernelST, RejectsUnavailableSymbolsBeforePublishingModel) {
  struct Symbols {
    const char *run;
    const char *creator;
    const char *diagnostic;
  };
  for (const Symbols &symbols :
       {Symbols{"MissingRun", nullptr, "Failed to resolve custom RunFunc MissingRun"},
        Symbols{"ReadBuffer", "MissingCreator", "Failed to resolve custom OutputsCreator MissingCreator"}}) {
    SCOPED_TRACE(symbols.diagnostic);
    GertRuntimeStub stub;
    ge::ModelBufferData empty{};
    auto graph = std::make_shared<ge::ExecuteGraph>("invalid_custom");
    ASSERT_NO_FATAL_FAILURE(
        BuildBufferGraph(*graph, "Om2InvalidCustomSymbolsST", symbols.run, symbols.creator, empty, empty));
    auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    GertModelData model;
    EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
    EXPECT_TRUE(model.models.empty());
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, symbols.diagnostic), -1);
  }
}

TEST_F(Om2CustomKernelST, RejectsMalformedRegistrationBeforePublishingModel) {
  struct Case {
    const char *key;
    const char *replacement;
    const char *diagnostic;
  };
  const Case cases[] = {
      {attr::kCustomKernelSoPath, nullptr, "missing its shared library path"},
      {attr::kCustomRunFuncSymbol, nullptr, "missing its RunFunc symbol"},
      {attr::kCustomKernelName, "OtherCustomName", "does not match node type"},
      {attr::kCustomKernelSoPath, "/nonexistent/libom2_test_custom.so", "Failed to load custom kernel library"},
  };
  for (const auto &item : cases) {
    SCOPED_TRACE(item.diagnostic);
    GertRuntimeStub stub;
    ge::ModelBufferData empty{};
    auto graph = std::make_shared<ge::ExecuteGraph>("invalid_custom_metadata");
    constexpr char type[] = "Om2InvalidRegistrationST";
    ASSERT_NO_FATAL_FAILURE(BuildBufferGraph(*graph, type, "ReadBuffer", "CreateChecksum", empty, empty));
    auto *read = ge::ExecuteGraphUtils::FindFirstNodeMatchType(graph.get(), type);
    ASSERT_NE(read, nullptr);
    auto *desc = read->GetOpDescBarePtr();
    if (item.replacement == nullptr) {
      ASSERT_EQ(desc->DelAttr(item.key), ge::GRAPH_SUCCESS);
    } else {
      ASSERT_TRUE(ge::AttrUtils::SetStr(desc, item.key, item.replacement));
    }
    auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    GertModelData model;
    EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
    EXPECT_TRUE(model.models.empty());
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, item.diagnostic), -1);
  }
}

TEST_F(Om2CustomKernelST, RejectsInvalidModelBufferHandlesAndPayloads) {
  for (const bool null_handle : {true, false}) {
    SCOPED_TRACE(null_handle);
    GertRuntimeStub stub;
    ge::ModelBufferData source{};
    source.length = 3U;
    ge::ModelBufferData empty{};
    auto graph = std::make_shared<ge::ExecuteGraph>("invalid_custom_buffer");
    ASSERT_NO_FATAL_FAILURE(
        BuildBufferGraph(*graph, "Om2InvalidModelBufferST", "ReadBuffer", "CreateChecksum", source, empty));
    if (null_handle) {
      auto *node = ge::ExecuteGraphUtils::FindNodeFromAllNodes(graph.get(), "bytes");
      ASSERT_NE(node, nullptr);
      ge::ModelBufferData *missing = nullptr;
      ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
          node->GetOpDescBarePtr(), kConstValue,
          ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&missing), sizeof(missing))));
    }
    auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    GertModelData model;
    EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
    EXPECT_TRUE(model.models.empty());
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR,
                                         null_handle ? "has invalid buffer metadata" : "has a null buffer with size 3"),
              -1);
  }
}

TEST_F(Om2CustomKernelST, MissingRuntimeKernelFailsLoadAfterDefaultCollection) {
  // slow_test_limit = 4000
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GertRuntimeStub stub;
  auto graph = om2_test::AffineGraph(false);
  ASSERT_NE(graph, nullptr);
  constexpr char type[] = "Om2MissingRunFuncST";
  ASSERT_EQ(KernelRegistry::GetInstance().FindKernelFuncs(type), nullptr);
  ASSERT_NE(om2_test::AddNode(*graph, "unknown", type, {}, {}), nullptr);
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertModelData model;
  ASSERT_EQ(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
  ASSERT_EQ(model.models.size(), 1U);
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(model.models[0]->runtime->so_artifact)) << runner.Error();
  EXPECT_NE(runner.Load(GertModelLoadConfig{}), ACL_SUCCESS);
}

TEST_F(Om2CustomKernelST, RejectsDifferentSymbolsForTheSameKernelType) {
  GertRuntimeStub stub;
  ge::ModelBufferData empty{};
  auto graph = std::make_shared<ge::ExecuteGraph>("conflicting_custom");
  constexpr char type[] = "Om2ConflictingCustomST";
  ASSERT_NO_FATAL_FAILURE(BuildBufferGraph(*graph, type, "ReadBuffer", "CreateChecksum", empty, empty));
  auto *write = ge::ExecuteGraphUtils::FindFirstNodeMatchType(graph.get(), "Om2CustomWriteChecksumST");
  ASSERT_NE(write, nullptr);
  write->GetOpDescBarePtr()->SetType(type);
  ASSERT_NO_FATAL_FAILURE(SetRegistration(*write, "WriteChecksum", nullptr));
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertModelData model;
  EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
  EXPECT_TRUE(model.models.empty());
  EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, "Conflicting custom registration metadata"), -1);
}
}  // namespace
}  // namespace gert
