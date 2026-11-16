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
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <memory>
#include <vector>

#include "framework/common/helper/om2_package_helper.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/runtime/rt_session.h"
#include "common/tbe_handle_store/tbe_kernel_store.h"
#include "common/op_so_store/op_so_store.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "graph/utils/tensor_utils.h"
#include "common/share_graph.h"
#include "faker/fake_value.h"
#include "faker/ge_model_builder.h"
#include "stub/gert_runtime_stub.h"
#include "tests/ge/common/om2_generated_model_runner.h"
#include "tests/ge/ut/ge/graph/load/om2_package_test_utils.h"

namespace ge {
namespace {
class Om2DynamicGeGeneratorST : public om2_test::DynamicPackageTest, public testing::WithParamInterface<bool> {
 protected:
  void SetUp() override {
    DynamicPackageTest::SetUp();
    // Match the ABI of the production-generated Makefile and linked GE libraries.
    GetThreadLocalContext().SetGraphOption(
        {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  }
};

void RunGatherShapes(om2_test::GeneratedModelRunner &runner, const std::initializer_list<int64_t> first_shape,
                     const std::initializer_list<int64_t> second_shape) {
  auto first = gert::TensorFaker().Shape(first_shape).Placement(gert::kOnHost).DataType(DT_FLOAT).Build();
  auto second = gert::TensorFaker().Shape(second_shape).Placement(gert::kOnHost).DataType(DT_INT64).Build();
  auto output =
      gert::TensorFaker().Shape({2}).Placement(gert::kOnHost).DataType(DT_INT64).Value<int64_t>({-777, -777}).Build();
  gert::Tensor *inputs[] = {first.GetTensor(), second.GetTensor()};
  gert::Tensor *outputs[] = {output.GetTensor()};
  GertModelRunConfig run;
  run.input_count = 2U;
  run.input_data = inputs;
  run.output_count = 1U;
  run.output_data = outputs;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);

  // Independent CPU reference for the graph's axes {{1, 0}, {0, 2}}.
  const std::vector<int64_t> first_dims(first_shape);
  const std::vector<int64_t> second_dims(second_shape);
  const std::array<int64_t, 2U> expected{second_dims[0U], first_dims[2U]};
  auto *const result = output.GetTensor();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->GetDataType(), DT_INT64);
  ASSERT_EQ(result->GetShape().GetStorageShape().GetDimNum(), 1U);
  EXPECT_EQ(result->GetShape().GetStorageShape().GetDim(0U), 2);
  ASSERT_EQ(result->GetShape().GetOriginShape().GetDimNum(), 1U);
  EXPECT_EQ(result->GetShape().GetOriginShape().GetDim(0U), 2);
  const auto *actual = result->GetData<int64_t>();
  ASSERT_NE(actual, nullptr);
  EXPECT_EQ(actual[0U], expected[0U]);
  EXPECT_EQ(actual[1U], expected[1U]);
}
}  // namespace

// SG-01: supported Host GatherShapes, root GeModel present/absent.
// Build/serialize/deserialize the actual generator output, then load the returned
// SO once and run two different input shapes against an independent CPU reference.
// RTS calls are stubbed; neither lowering, collector, codegen, nor model SO is mocked.
TEST_P(Om2DynamicGeGeneratorST, GeneratesPackagesAndRunsHostModelWithChangingShapes) {
  gert::GertRuntimeStub runtime_stub;
  auto graph = gert::ShareGraph::BuildGatherShapesGraph();
  ASSERT_NE(graph, nullptr);
  // SG-01 deliberately has no weight dependency, so removing the compiled root
  // model cannot discard a weight source. Weight consumption is a separate SG-02 scenario.
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

  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
  if (!GetParam()) {
    root->RemoveInstanceSubgraphModel(graph->GetName());
  }
  gert::GertModelData generated;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, generated), SUCCESS);
  ASSERT_EQ(generated.models.size(), 1U);
  ASSERT_NE(generated.models[0U]->runtime, nullptr);
  ASSERT_GT(generated.models[0U]->runtime->so_artifact.data_size, 0U);
  ASSERT_FALSE(generated.models[0U]->runtime->source_artifacts.empty());

  ModelBufferData archive;
  ASSERT_EQ(gert::SerializeGertModelData(generated, archive, false), SUCCESS);
  ASSERT_NE(archive.data, nullptr);
  ASSERT_GT(archive.length, 0U);
  // Keep the archive and its byte views alive until after model Unload.
  gert::GertModelData restored;
  ASSERT_EQ(gert::DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            SUCCESS);
  ASSERT_EQ(restored.models.size(), 1U);
  ASSERT_NE(restored.models[0U]->runtime, nullptr);
  gert::RtSession session;
  struct SessionCleanup {
    gert::RtSession &session;
    ~SessionCleanup() {
      session.DestroyResources();
    }
  } session_cleanup{session};
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0U]->runtime->so_artifact)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = &session;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  RunGatherShapes(runner, {1, 2, 3, 4}, {5, 6});
  RunGatherShapes(runner, {7, 8, 9, 10}, {11, 12});
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

using Om2DynamicGeWeightGeneratorST = om2_test::DynamicPackageTest;

// SG-02 partial boundary: one empty GeModel reserves index 0; the root model's
// actual shape-vector bytes must be consumed by Reshape at index 1. ModelConverter
// overwrites seeded indices in map order, so this does not claim a non-map-order
// lowering contract or coverage of two nonempty models' weights/Kernel stores.
TEST_F(Om2DynamicGeWeightGeneratorST, EmptyModelReservesIndexBeforeConsumedRootWeight) {
  // slow_test_limit = 3000
  // End-to-end execution includes compiling generated model SOs.
  GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  gert::GertRuntimeStub runtime_stub;
  auto graph = gert::ShareGraph::BuildReshapeGraph();
  ASSERT_NE(graph, nullptr);
  auto reshape_desc = graph->FindNode("reshape1")->GetOpDesc();
  OpDescUtilsEx::SetType(reshape_desc, "Reshape");
  auto &input_names = reshape_desc->MutableAllInputName();
  input_names.clear();
  input_names["x"] = 0U;
  input_names["shape"] = 1U;
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
  const auto model = root->GetSubgraphInstanceNameToModel().at(graph->GetName());
  ASSERT_NE(model, nullptr);

  // The shared fixture starts with Const value {2,-1}. Supply different bytes
  // through the GeModel weight block: expected output {3,N/3} proves that the
  // runtime consumes these bytes rather than a filename or Const's old payload.
  const std::array<int64_t, 2U> reshape_weight{3, -1};
  const auto *weight_bytes = reinterpret_cast<const uint8_t *>(reshape_weight.data());
  model->SetWeight(Buffer::CopyFrom(weight_bytes, sizeof(reshape_weight)));
  ASSERT_TRUE(AttrUtils::SetInt(model, "om2_weight_index", 99));
  GeTensorPtr const_tensor;
  ASSERT_TRUE(AttrUtils::MutableTensor(graph->FindNode("const1")->GetOpDesc(), "value", const_tensor));
  ASSERT_NE(const_tensor, nullptr);
  TensorUtils::SetDataOffset(const_tensor->MutableTensorDesc(), 0);
  TensorUtils::SetWeightSize(const_tensor->MutableTensorDesc(), sizeof(reshape_weight));
  TensorUtils::SetSize(*graph->FindNode("const1")->GetOpDesc()->MutableOutputDesc(0U), sizeof(reshape_weight));

  auto empty = std::make_shared<GeModel>();
  empty->SetName("a_empty");
  empty->SetGraph(std::make_shared<ComputeGraph>("a_empty"));
  root->SetSubgraphInstanceNameToModel("a_empty", empty);
  ASSERT_EQ(root->GetSubgraphInstanceNameToModel().size(), 2U);
  ASSERT_LT(std::string("a_empty"), graph->GetName());

  gert::GertModelData generated;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, generated), SUCCESS);
  int64_t empty_index = -1;
  int64_t root_index = -1;
  ASSERT_TRUE(AttrUtils::GetInt(empty, "om2_weight_index", empty_index));
  ASSERT_TRUE(AttrUtils::GetInt(model, "om2_weight_index", root_index));
  EXPECT_EQ(empty_index, 0);
  EXPECT_EQ(root_index, 1);
  ASSERT_EQ(generated.models.size(), 1U);
  ASSERT_NE(generated.models[0U]->constants_config, nullptr);
  ASSERT_EQ(generated.models[0U]->constants_config->consts.size(), 1U);
  EXPECT_EQ(generated.models[0U]->constants_config->consts[0U]->index, 1U);
  EXPECT_EQ(gert::GetGertModelConstPlacement(*generated.models[0U]->constants_config->consts[0U]),
            gert::GertModelDataConstPlacement::kHost);

  ModelBufferData archive;
  ASSERT_EQ(gert::SerializeGertModelData(generated, archive, false), SUCCESS);
  ASSERT_NE(archive.data, nullptr);
  gert::GertModelData restored;
  ASSERT_EQ(gert::DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            SUCCESS);
  ASSERT_EQ(restored.models.size(), 1U);
  ASSERT_NE(restored.constants, nullptr);
  ASSERT_EQ(restored.constants->constants_data.size(), 1U);
  const auto &weight = restored.constants->constants_data[0U];
  ASSERT_NE(weight, nullptr);
  EXPECT_STREQ(gert::GertGetStr(weight->file_name), "constant_1");
  ASSERT_EQ(weight->data_size, sizeof(reshape_weight));
  EXPECT_EQ(std::memcmp(weight->data.get(), reshape_weight.data(), sizeof(reshape_weight)), 0);
  std::array<void *, 2U> constants{nullptr, const_cast<uint8_t *>(weight->data.get())};
  gert::RtSession session;
  struct SessionCleanup {
    gert::RtSession &session;
    ~SessionCleanup() {
      session.DestroyResources();
    }
  } session_cleanup{session};
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0U]->runtime->so_artifact)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  load.constants = constants.data();
  dynamic_config.rt_session = &session;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  for (const int64_t count : {6, 12}) {
    std::vector<float> values(static_cast<size_t>(count));
    for (size_t i = 0U; i < values.size(); ++i) {
      values[i] = static_cast<float>(i) + 0.25F;
    }
    auto input =
        gert::TensorFaker().Shape({1, 1, 1, count}).Placement(gert::kOnHost).DataType(DT_FLOAT).Value(values).Build();
    auto output = gert::TensorFaker()
                      .Shape({count})
                      .Placement(gert::kOnHost)
                      .DataType(DT_FLOAT)
                      .Value(std::vector<float>(values.size(), -777.0F))
                      .Build();
    gert::Tensor *inputs[] = {input.GetTensor()};
    gert::Tensor *outputs[] = {output.GetTensor()};
    GertModelRunConfig run;
    run.input_count = 1U;
    run.input_data = inputs;
    run.output_count = 1U;
    run.output_data = outputs;
    ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
    const auto *actual = output.GetTensor();
    ASSERT_NE(actual, nullptr);
    EXPECT_EQ(actual->GetDataType(), DT_FLOAT);
    ASSERT_EQ(actual->GetStorageShape().GetDimNum(), 2U);
    EXPECT_EQ(actual->GetStorageShape().GetDim(0U), 3);
    EXPECT_EQ(actual->GetStorageShape().GetDim(1U), count / 3);
    ASSERT_EQ(actual->GetOriginShape().GetDimNum(), 2U);
    EXPECT_EQ(actual->GetOriginShape().GetDim(0U), 3);
    EXPECT_EQ(actual->GetOriginShape().GetDim(1U), count / 3);
    const auto *actual_values = actual->GetData<float>();
    ASSERT_NE(actual_values, nullptr);
    for (size_t i = 0U; i < values.size(); ++i) {
      EXPECT_FLOAT_EQ(actual_values[i], values[i]);
    }
  }
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

void AddArchiveCollisionLibraries(const GeRootModelPtr &root) {
  OpSoStore store;
  // Duplicate payload, same-name different payload, then collision with an assigned suffix.
  const std::array<std::string, 5U> names{"shape.so", "shape.so", "shape.so", "shape_1.so", "so_bins.json"};
  const std::array<char_t, 5U> payloads{'a', 'a', 'b', 'c', 'd'};
  for (size_t i = 0U; i < names.size(); ++i) {
    auto bytes = std::make_unique<char_t[]>(1U);
    bytes[0] = payloads[i];
    store.AddKernel(std::make_shared<OpSoBin>(names[i], "st_vendor", std::move(bytes), 1U, SoBinType::kCustomOp));
  }
  ASSERT_TRUE(store.Build());
  ASSERT_TRUE(root->LoadSoBinData(store.Data(), store.DataSize()));
  ASSERT_EQ(root->GetAllSoBin().size(), names.size());
}

void CheckArchiveCollisionLibraries(const gert::GertModelData &data) {
  ASSERT_NE(data.so_bins, nullptr);
  ASSERT_EQ(data.so_bins->libraries.size(), 4U);
  const std::array<std::string, 4U> original{"shape.so", "shape.so", "shape_1.so", "so_bins.json"};
  const std::array<std::string, 4U> archive{"shape.so", "shape_1.so", "shape_1_1.so", "so_bins.json_1"};
  for (size_t i = 0U; i < original.size(); ++i) {
    const auto &library = data.so_bins->libraries[i];
    ASSERT_NE(library, nullptr);
    ASSERT_NE(library->binary, nullptr);
    EXPECT_EQ(gert::GertGetStr(library->so_name), original[i]);
    EXPECT_EQ(gert::GertGetStr(library->vendor_name), std::string("st_vendor"));
    EXPECT_EQ(library->so_bin_type, static_cast<uint64_t>(SoBinType::kCustomOp));
    EXPECT_EQ(gert::GertGetStr(library->binary->file_name), archive[i]);
    ASSERT_EQ(library->binary->data_size, 1U);
    ASSERT_NE(library->binary->data, nullptr);
    EXPECT_EQ(library->binary->data.get()[0], static_cast<uint8_t>('a' + i));
  }
  ASSERT_EQ(data.models.size(), 1U);
  ASSERT_NE(data.models[0], nullptr);
  ASSERT_NE(data.models[0]->model_meta, nullptr);
  const auto &outputs = data.models[0]->model_meta->output_desc;
  ASSERT_EQ(outputs.size(), 2U);
  for (const auto &output : outputs) {
    EXPECT_EQ(output.data_type, DT_FLOAT);
    EXPECT_EQ(output.format, FORMAT_ND);
    EXPECT_EQ(output.shape, (std::vector<int64_t>{-1, -1}));
    EXPECT_FALSE(std::string(gert::GertGetStr(output.name)).empty());
  }
}

struct WeightedReshapeModels {
  ComputeGraphPtr graph;
  ComputeGraphPtr child;
  GeRootModelPtr root;
  std::array<GeModelPtr, 2U> compiled_models;
  std::array<ComputeGraphPtr, 2U> graphs;
  const std::array<int64_t, 2U> root_shape{3, -1};
  const std::array<int64_t, 2U> child_shape{4, -1};
  const std::array<std::array<int64_t, 2U>, 2U> weights{child_shape, root_shape};
  const std::array<std::string, 2U> kernel_names{"child_shape_kernel", "root_shape_kernel"};
  std::array<std::vector<char>, 2U> kernel_bytes{std::vector<char>{'c', 'h', 'i', 'l', 'd', '\0', 'k'},
                                                 std::vector<char>{'r', 'o', 'o', 't', '\0', 'k'}};
  std::array<void *, 2U> constants{};
  std::vector<const char *> binary_files;
  std::vector<const void *> binary_data;
  std::vector<uint64_t> binary_sizes;

  void InitializeGraphs() {
    graph = gert::ShareGraph::BuildReshapeGraph();
    child = gert::ShareGraph::BuildReshapeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_NE(child, nullptr);
    child->SetName("a_weighted_child");
    ASSERT_LT(child->GetName(), graph->GetName());
    child->SetGraphUnknownFlag(true);
    for (const auto &current : {graph, child}) {
      auto reshape = current->FindNode("reshape1")->GetOpDesc();
      OpDescUtilsEx::SetType(reshape, "Reshape");
      auto &names = reshape->MutableAllInputName();
      names.clear();
      names["x"] = 0U;
      names["shape"] = 1U;
    }
  }

  void ConnectGraphs() {
    // Root Data feeds its own Reshape and a genuine dynamic PartitionedCall.
    // Preserve the root Reshape output as slot 0 and expose the child's as slot 1.
    const auto input_desc = graph->FindNode("data1")->GetOpDesc()->GetOutputDesc(0U);
    const GeTensorDesc output_desc(GeShape({-1, -1}), FORMAT_ND, DT_FLOAT);
    auto call_desc = std::make_shared<OpDesc>("weighted_child_call", PARTITIONEDCALL);
    ASSERT_EQ(call_desc->AddInputDesc("x", input_desc), GRAPH_SUCCESS);
    ASSERT_EQ(call_desc->AddOutputDesc("y", output_desc), GRAPH_SUCCESS);
    call_desc->RegisterSubgraphIrName("f", SubgraphType::kStatic);
    ASSERT_EQ(call_desc->AddSubgraphName("f"), GRAPH_SUCCESS);
    ASSERT_EQ(call_desc->SetSubgraphInstanceName(0U, child->GetName()), GRAPH_SUCCESS);
    auto call = graph->AddNode(call_desc);
    ASSERT_NE(call, nullptr);
    ASSERT_EQ(GraphUtils::AddEdge(graph->FindNode("data1")->GetOutDataAnchor(0), call->GetInDataAnchor(0)),
              GRAPH_SUCCESS);
    auto netoutput = graph->FindNode("NetOutput");
    ASSERT_NE(netoutput, nullptr);
    ASSERT_EQ(netoutput->GetOpDesc()->UpdateInputDesc(0U, output_desc), GRAPH_SUCCESS);
    ASSERT_EQ(netoutput->GetOpDesc()->AddInputDesc("child_result", output_desc), GRAPH_SUCCESS);
    ASSERT_EQ(NodeUtils::AppendInputAnchor(netoutput, 2U), GRAPH_SUCCESS);
    ASSERT_EQ(GraphUtils::AddEdge(call->GetOutDataAnchor(0), netoutput->GetInDataAnchor(1)), GRAPH_SUCCESS);
    netoutput->GetOpDesc()->SetSrcName({"reshape1", "weighted_child_call"});
    netoutput->GetOpDesc()->SetSrcIndex({0, 0});
    child->SetParentNode(call);
    child->SetParentGraph(graph);
    ASSERT_EQ(graph->AddSubgraph(child), GRAPH_SUCCESS);
    ASSERT_TRUE(AttrUtils::SetInt(child->FindNode("data1")->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0));
    ASSERT_TRUE(AttrUtils::SetInt(child->FindNode("NetOutput")->GetOpDesc()->MutableInputDesc(0U),
                                  ATTR_NAME_PARENT_NODE_INDEX, 0));
    ASSERT_EQ(child->TopologicalSorting(), GRAPH_SUCCESS);
    ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  }

  void InitializeModels() {
    root = gert::GeModelBuilder(graph).BuildGeRootModel();
    ASSERT_NE(root, nullptr);
    const auto &models = root->GetSubgraphInstanceNameToModel();
    ASSERT_EQ(models.size(), 2U);
    const auto root_model = models.at(graph->GetName());
    const auto child_model = models.at(child->GetName());
    ASSERT_NE(root_model, nullptr);
    ASSERT_NE(child_model, nullptr);
    ASSERT_NO_FATAL_FAILURE(AddArchiveCollisionLibraries(root));
    compiled_models = {child_model, root_model};
    graphs = {child, graph};
  }

  void SetWeightsAndKernels() {
    for (size_t index = 0U; index < compiled_models.size(); ++index) {
      const auto &model = compiled_models[index];
      const auto &current = graphs[index];
      model->SetWeight(
          Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(weights[index].data()), sizeof(weights[index])));
      ASSERT_TRUE(AttrUtils::SetInt(model, "om2_weight_index", 90U + index));
      GeTensorPtr tensor;
      ASSERT_TRUE(AttrUtils::MutableTensor(current->FindNode("const1")->GetOpDesc(), "value", tensor));
      ASSERT_NE(tensor, nullptr);
      TensorUtils::SetDataOffset(tensor->MutableTensorDesc(), 0);
      TensorUtils::SetWeightSize(tensor->MutableTensorDesc(), sizeof(weights[index]));
      TensorUtils::SetSize(*current->FindNode("const1")->GetOpDesc()->MutableOutputDesc(0U), sizeof(weights[index]));
      ASSERT_TRUE(AttrUtils::SetStr(current->FindNode("reshape1")->GetOpDesc(), "_kernelname", kernel_names[index]));
      model->GetTBEKernelStore().AddKernel(
          std::make_shared<OpKernelBin>(kernel_names[index], std::vector<char>(kernel_bytes[index])));
      ASSERT_TRUE(model->GetTBEKernelStore().Build());
      ASSERT_GT(model->GetWeightSize(), 0U);
    }
  }

  void CheckGenerated(const gert::GertModelData &generated) {
    ASSERT_EQ(generated.models.size(), 1U);
    ASSERT_NE(generated.constants, nullptr);
    ASSERT_EQ(generated.constants->constants_data.size(), 2U);
    ASSERT_NE(generated.models[0U]->constants_config, nullptr);
    ASSERT_EQ(generated.models[0U]->constants_config->consts.size(), 2U);
    std::array<bool, 2U> configured{};
    for (const auto &constant : generated.models[0U]->constants_config->consts) {
      ASSERT_NE(constant, nullptr);
      ASSERT_LT(constant->index, configured.size());
      EXPECT_FALSE(configured[constant->index]);
      configured[constant->index] = true;
      EXPECT_EQ(gert::GetGertModelConstPlacement(*constant), gert::GertModelDataConstPlacement::kHost);
    }
    EXPECT_TRUE(configured[0U]);
    EXPECT_TRUE(configured[1U]);
    for (size_t index = 0U; index < compiled_models.size(); ++index) {
      int64_t actual_index = -1;
      ASSERT_TRUE(AttrUtils::GetInt(compiled_models[index], "om2_weight_index", actual_index));
      EXPECT_EQ(actual_index, static_cast<int64_t>(index));
    }
  }

  void CheckRestoredWeights(const gert::GertModelData &restored) {
    ASSERT_EQ(restored.models.size(), 1U);
    ASSERT_NE(restored.models[0U]->runtime, nullptr);
    ASSERT_NE(restored.constants, nullptr);
    ASSERT_EQ(restored.constants->constants_data.size(), 2U);
    for (const auto &weight : restored.constants->constants_data) {
      ASSERT_NE(weight, nullptr);
      const std::string name = gert::GertGetStr(weight->file_name);
      const size_t index = name == "constant_0" ? 0U : 1U;
      ASSERT_EQ(name, "constant_" + std::to_string(index));
      ASSERT_EQ(weight->data_size, sizeof(weights[index]));
      EXPECT_EQ(std::memcmp(weight->data.get(), weights[index].data(), sizeof(weights[index])), 0);
      constants[index] = const_cast<uint8_t *>(weight->data.get());
    }
    ASSERT_NE(constants[0U], nullptr);
    ASSERT_NE(constants[1U], nullptr);
  }

  void CheckRestoredKernels(const gert::GertModelData &restored) {
    ASSERT_NE(restored.kernels, nullptr);
    ASSERT_EQ(restored.kernels->binaries.size(), 2U);
    std::map<std::string, size_t> binary_indices{{"child_shape_kernel.o", 0U}, {"root_shape_kernel.o", 1U}};
    for (const auto &binary : restored.kernels->binaries) {
      ASSERT_NE(binary, nullptr);
      const auto expected = binary_indices.find(gert::GertGetStr(binary->file_name));
      ASSERT_NE(expected, binary_indices.end());
      const auto &bytes = kernel_bytes[expected->second];
      ASSERT_EQ(binary->data_size, bytes.size());
      EXPECT_EQ(std::memcmp(binary->data.get(), bytes.data(), bytes.size()), 0);
      binary_indices.erase(expected);
      binary_files.push_back(gert::GertGetStr(binary->file_name));
      binary_data.push_back(binary->data.get());
      binary_sizes.push_back(binary->data_size);
    }
    EXPECT_TRUE(binary_indices.empty());
  }

  void CheckOutputs(gert::Tensor *const *outputs, int64_t count, const std::vector<float> &values) {
    for (size_t output_index = 0U; output_index < 2U; ++output_index) {
      const auto *actual = outputs[output_index];
      const int64_t leading_dim = output_index == 0U ? 3 : 4;
      EXPECT_EQ(actual->GetDataType(), DT_FLOAT);
      ASSERT_EQ(actual->GetStorageShape().GetDimNum(), 2U);
      EXPECT_EQ(actual->GetStorageShape().GetDim(0U), leading_dim);
      EXPECT_EQ(actual->GetStorageShape().GetDim(1U), count / leading_dim);
      ASSERT_EQ(actual->GetOriginShape().GetDimNum(), 2U);
      EXPECT_EQ(actual->GetOriginShape().GetDim(0U), leading_dim);
      EXPECT_EQ(actual->GetOriginShape().GetDim(1U), count / leading_dim);
      ASSERT_NE(actual->GetData<float>(), nullptr);
      for (size_t i = 0U; i < values.size(); ++i) {
        EXPECT_FLOAT_EQ(actual->GetData<float>()[i], values[i]);
      }
    }
  }

  void RunShapes(om2_test::GeneratedModelRunner &runner, int64_t count) {
    std::vector<float> values(static_cast<size_t>(count));
    for (size_t i = 0U; i < values.size(); ++i) {
      values[i] = static_cast<float>(i) - 2.5F;
    }
    auto input =
        gert::TensorFaker().Shape({1, 1, 1, count}).Placement(gert::kOnHost).DataType(DT_FLOAT).Value(values).Build();
    auto first = gert::TensorFaker()
                     .Shape({count})
                     .Placement(gert::kOnHost)
                     .DataType(DT_FLOAT)
                     .Value(std::vector<float>(values.size(), -777.0F))
                     .Build();
    auto second = gert::TensorFaker()
                      .Shape({count})
                      .Placement(gert::kOnHost)
                      .DataType(DT_FLOAT)
                      .Value(std::vector<float>(values.size(), -888.0F))
                      .Build();
    gert::Tensor *inputs[] = {input.GetTensor()};
    gert::Tensor *outputs[] = {first.GetTensor(), second.GetTensor()};
    GertModelRunConfig run;
    run.input_count = 1U;
    run.input_data = inputs;
    run.output_count = 2U;
    run.output_data = outputs;
    ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(CheckOutputs(outputs, count, values));
  }

  void RunRestored(const gert::GertModelData &restored) {
    gert::RtSession session;
    struct SessionCleanup {
      gert::RtSession &session;
      ~SessionCleanup() {
        session.DestroyResources();
      }
    } session_cleanup{session};
    om2_test::GeneratedModelRunner runner;
    ASSERT_TRUE(runner.Open(restored.models[0U]->runtime->so_artifact)) << runner.Error();
    GertModelDynamicLoadConfig dynamic_config;
    GertModelLoadConfig load;
    load.dynamic_config = &dynamic_config;
    load.constants = constants.data();
    dynamic_config.rt_session = &session;
    load.bin_num = binary_files.size();
    load.bin_files = binary_files.data();
    load.bin_data = binary_data.data();
    load.bin_size = binary_sizes.data();
    ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
    for (const int64_t count : {12, 24}) {
      ASSERT_NO_FATAL_FAILURE(RunShapes(runner, count));
    }
    ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
  }
};

// SG-02: both models have nonempty weights and distinct Kernel stores. The two
// reshapes run in parallel, so each output independently proves which model's
// shape-vector bytes were consumed. No intermediate shape can be hidden by a
// subsequent reshape. Device binary payloads use the existing RTS stub boundary;
// Host Reshape is the actual computation, not a simulated model entry point.
TEST_F(Om2DynamicGeWeightGeneratorST, TwoNonemptyModelsConsumeOwnWeightsAndAggregateKernels) {
  // slow_test_limit = 3000
  // End-to-end execution includes compiling generated model SOs.
  GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  gert::GertRuntimeStub runtime_stub;
  WeightedReshapeModels models;
  ASSERT_NO_FATAL_FAILURE(models.InitializeGraphs());
  ASSERT_NO_FATAL_FAILURE(models.ConnectGraphs());
  ASSERT_NO_FATAL_FAILURE(models.InitializeModels());
  ASSERT_NO_FATAL_FAILURE(models.SetWeightsAndKernels());
  gert::GertModelData generated;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(models.root, generated), SUCCESS);
  ASSERT_NO_FATAL_FAILURE(models.CheckGenerated(generated));

  ModelBufferData archive;
  ASSERT_EQ(gert::SerializeGertModelData(generated, archive, false), SUCCESS);
  ASSERT_NE(archive.data, nullptr);
  gert::GertModelData restored;
  ASSERT_EQ(gert::DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            SUCCESS);
  ASSERT_NO_FATAL_FAILURE(models.CheckRestoredWeights(restored));
  ASSERT_NO_FATAL_FAILURE(models.CheckRestoredKernels(restored));
  ASSERT_NO_FATAL_FAILURE(CheckArchiveCollisionLibraries(generated));
  ASSERT_NO_FATAL_FAILURE(CheckArchiveCollisionLibraries(restored));
  ASSERT_NO_FATAL_FAILURE(models.RunRestored(restored));
}

// F-01 route ST: the two real Host Shape branches read different parent
// inputs. Different ranks and values make branch selection observable through
// the generated model's output, rather than through names in generated source.
TEST_F(Om2DynamicGeWeightGeneratorST, GeneratedIfSelectsDifferentHostInputsAcrossRuns) {
  // slow_test_limit = 3000
  // End-to-end execution includes compiling generated model SOs.
  GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  gert::GertRuntimeStub runtime_stub;
  auto graph = gert::ShareGraph::IfGraph();
  ASSERT_NE(graph, nullptr);
  const GeTensorDesc condition_desc(GeShape(std::vector<int64_t>{}), FORMAT_ND, DT_BOOL);
  const GeTensorDesc first_desc(GeShape({-1, -1, -1, -1}), FORMAT_ND, DT_FLOAT);
  const GeTensorDesc second_desc(GeShape({-1, -1}), FORMAT_ND, DT_FLOAT);
  const GeTensorDesc result_desc(GeShape({-1}), FORMAT_ND, DT_INT64);
  auto pred = graph->FindNode("pred")->GetOpDesc();
  ASSERT_EQ(pred->UpdateInputDesc(0U, condition_desc), GRAPH_SUCCESS);
  ASSERT_EQ(pred->UpdateOutputDesc(0U, condition_desc), GRAPH_SUCCESS);
  auto first_data = graph->FindNode("input")->GetOpDesc();
  ASSERT_EQ(first_data->UpdateInputDesc(0U, first_desc), GRAPH_SUCCESS);
  ASSERT_EQ(first_data->UpdateOutputDesc(0U, first_desc), GRAPH_SUCCESS);
  auto second_data_desc = std::make_shared<OpDesc>("second_input", DATA);
  OpDescUtilsEx::SetType(second_data_desc, DATA);
  ASSERT_EQ(second_data_desc->AddInputDesc("x", second_desc), GRAPH_SUCCESS);
  ASSERT_EQ(second_data_desc->AddOutputDesc("y", second_desc), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetInt(second_data_desc, "index", 2));
  auto second_data = graph->AddNode(second_data_desc);
  ASSERT_NE(second_data, nullptr);
  ASSERT_NE(graph->AddInputNode(second_data), nullptr);
  graph->SetInputSize(3U);
  auto branch = graph->FindNode("if");
  ASSERT_NE(branch, nullptr);
  auto branch_desc = branch->GetOpDesc();
  ASSERT_EQ(branch_desc->UpdateInputDesc(0U, condition_desc), GRAPH_SUCCESS);
  ASSERT_EQ(branch_desc->UpdateInputDesc(1U, first_desc), GRAPH_SUCCESS);
  ASSERT_EQ(branch_desc->AddInputDesc("input1", second_desc), GRAPH_SUCCESS);
  ASSERT_EQ(NodeUtils::AppendInputAnchor(branch, 3U), GRAPH_SUCCESS);
  ASSERT_EQ(branch_desc->UpdateOutputDesc(0U, result_desc), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(second_data->GetOutDataAnchor(0), branch->GetInDataAnchor(2)), GRAPH_SUCCESS);
  ASSERT_EQ(graph->FindNode("NetOutput")->GetOpDesc()->UpdateInputDesc(0U, result_desc), GRAPH_SUCCESS);
  for (const bool then_branch : {true, false}) {
    auto subgraph = graph->GetSubgraph(then_branch ? "then" : "else");
    ASSERT_NE(subgraph, nullptr);
    subgraph->SetGraphUnknownFlag(true);
    const auto &input_desc = then_branch ? first_desc : second_desc;
    auto data = subgraph->FindNode("data")->GetOpDesc();
    ASSERT_TRUE(AttrUtils::SetInt(data, ATTR_NAME_PARENT_NODE_INDEX, then_branch ? 1 : 2));
    ASSERT_EQ(data->UpdateInputDesc(0U, input_desc), GRAPH_SUCCESS);
    ASSERT_EQ(data->UpdateOutputDesc(0U, input_desc), GRAPH_SUCCESS);
    auto shape = subgraph->FindNode("shape")->GetOpDesc();
    OpDescUtilsEx::SetType(shape, "Shape");
    ASSERT_TRUE(AttrUtils::SetInt(shape, "dtype", DT_INT64));
    ASSERT_EQ(shape->UpdateInputDesc(0U, input_desc), GRAPH_SUCCESS);
    ASSERT_EQ(shape->UpdateOutputDesc(0U, result_desc), GRAPH_SUCCESS);
    auto output = subgraph->FindNode("NetOutput")->GetOpDesc();
    ASSERT_EQ(output->UpdateInputDesc(0U, result_desc), GRAPH_SUCCESS);
    ASSERT_TRUE(AttrUtils::SetInt(output->MutableInputDesc(0U), ATTR_NAME_PARENT_NODE_INDEX, 0));
    ASSERT_EQ(subgraph->TopologicalSorting(), GRAPH_SUCCESS);
  }
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
  gert::GertModelData built;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, built), SUCCESS);
  ModelBufferData archive;
  ASSERT_EQ(gert::SerializeGertModelData(built, archive, false), SUCCESS);
  ASSERT_NE(archive.data, nullptr);
  gert::GertModelData restored;
  ASSERT_EQ(gert::DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            SUCCESS);
  ASSERT_EQ(restored.models.size(), 1U);
  ASSERT_NE(restored.models[0U]->runtime, nullptr);
  gert::RtSession session;
  struct SessionCleanup {
    gert::RtSession &session;
    ~SessionCleanup() {
      session.DestroyResources();
    }
  } session_cleanup{session};
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0U]->runtime->so_artifact)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = &session;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  for (const bool choose_first : {true, false, true, false}) {
    auto condition = gert::TensorFaker()
                         .Shape({})
                         .Placement(gert::kOnHost)
                         .DataType(DT_BOOL)
                         .Value<uint8_t>({static_cast<uint8_t>(choose_first)})
                         .Build();
    auto first = gert::TensorFaker().Shape({2, 3, 4, 5}).Placement(gert::kOnHost).DataType(DT_FLOAT).Build();
    auto second = gert::TensorFaker().Shape({7, 11}).Placement(gert::kOnHost).DataType(DT_FLOAT).Build();
    auto output = gert::TensorFaker()
                      .Shape({4})
                      .Placement(gert::kOnHost)
                      .DataType(DT_INT64)
                      .Value<int64_t>({-777, -777, -777, -777})
                      .Build();
    gert::Tensor *inputs[] = {condition.GetTensor(), first.GetTensor(), second.GetTensor()};
    gert::Tensor *outputs[] = {output.GetTensor()};
    GertModelRunConfig run;
    run.input_count = 3U;
    run.input_data = inputs;
    run.output_count = 1U;
    run.output_data = outputs;
    ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
    const std::vector<int64_t> expected = choose_first ? std::vector<int64_t>{2, 3, 4, 5} : std::vector<int64_t>{7, 11};
    const auto *actual = output.GetTensor();
    EXPECT_EQ(actual->GetDataType(), DT_INT64);
    ASSERT_EQ(actual->GetStorageShape().GetDimNum(), 1U);
    EXPECT_EQ(actual->GetStorageShape().GetDim(0U), static_cast<int64_t>(expected.size()));
    ASSERT_EQ(actual->GetOriginShape().GetDimNum(), 1U);
    EXPECT_EQ(actual->GetOriginShape().GetDim(0U), static_cast<int64_t>(expected.size()));
    ASSERT_NE(actual->GetData<int64_t>(), nullptr);
    for (size_t i = 0U; i < expected.size(); ++i) {
      EXPECT_EQ(actual->GetData<int64_t>()[i], expected[i]);
    }
  }
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

INSTANTIATE_TEST_SUITE_P(RootGeModelPresentOrAbsent, Om2DynamicGeGeneratorST, testing::Bool());
}  // namespace ge
