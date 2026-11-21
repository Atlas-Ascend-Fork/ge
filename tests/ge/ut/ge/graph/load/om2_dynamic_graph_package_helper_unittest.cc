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
#include <cstring>
#include <limits>
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "framework/common/gert_model_data_utils.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "stub/gert_runtime_stub.h"
#include "om2_package_test_utils.h"
#include "om2_compile_stub.h"

#include "framework/common/helper/om2_package_helper.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph_metadef/graph/stage/stage_splitter.h"

namespace ge {
namespace {
void SetEmptyModelIo(ExecuteGraph &graph) {
  ASSERT_TRUE(AttrUtils::SetListTensor(graph, gert::attr::kModelInputs, std::vector<GeTensor>{}));
  ASSERT_TRUE(AttrUtils::SetListTensor(graph, gert::attr::kModelOutputs, std::vector<GeTensor>{}));
}
}  // namespace

TEST(Om2GraphPackageHelperUT, GraphFacadeRejectsInvalidAndComputeGraphs) {
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  const auto *original = data.manifest.get();
  Graph invalid("invalid_execute", C_GRAPH_EXECUTE);
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(invalid, data), SUCCESS);
  Graph compute("compute");
  ASSERT_EQ(compute.SetValid(), GRAPH_SUCCESS);
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(compute, data), SUCCESS);
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

TEST(Om2GraphPackageHelperUT, RejectsOutputCountMismatch) {
  gert::GertRuntimeStub stub;
  auto graph = std::make_shared<ExecuteGraph>("registered_outputs");
  SetEmptyModelIo(*graph);
  auto desc = std::make_shared<OpDesc>("producer", "PlainOp");
  ASSERT_EQ(desc->AddOutputDesc("y", GeTensorDesc(GeShape({1}), FORMAT_ND, DT_FLOAT)), GRAPH_SUCCESS);
  auto *node = graph->AddNode(desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(ExecuteGraphUtils::SetGraphOutputs(graph.get(), {{node, 0}}), GRAPH_SUCCESS);
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  const auto node_count = graph->GetDirectNodesSize();
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  const auto *original = data.manifest.get();
  // 登记了一个输出，但模型描述声明零输出，应在阶段拆分前失败。
  stub.GetSlogStub().Clear();
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, "Model output descriptions do not match outputs"), -1);
  EXPECT_EQ(graph->GetDirectNodesSize(), node_count);
  EXPECT_TRUE(graph->GetAllSubgraphs().empty());
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

TEST(Om2GraphPackageHelperUT, MissingModelMetadataLeavesOutputUntouched) {
  gert::GertRuntimeStub stub;
  for (const auto *missing : {gert::attr::kModelInputs, gert::attr::kModelOutputs}) {
    SCOPED_TRACE(missing);
    auto graph = std::make_shared<ExecuteGraph>("missing_metadata");
    ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
    ASSERT_EQ(graph->DelAttr(missing), GRAPH_SUCCESS);
    const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    gert::GertModelData data;
    data.manifest = std::make_unique<gert::GertModelDataManifest>();
    const auto *original = data.manifest.get();
    stub.GetSlogStub().Clear();
    EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, missing), -1);
    EXPECT_EQ(data.manifest.get(), original);
    EXPECT_TRUE(graph->GetDirectNode().empty());
  }
}

TEST(Om2GraphPackageHelperUT, InvalidStageDoesNotMutateSourceGraph) {
  auto graph = std::make_shared<ExecuteGraph>("invalid_stage");
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  SetEmptyModelIo(*graph);
  auto *node = graph->AddNode(std::make_shared<OpDesc>("op", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(node->GetOpDescPtr(), gert::attr::kStage, "invalid"));
  gert::GertModelData data;
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 1U);
  EXPECT_TRUE(graph->GetAllSubgraphs().empty());
  EXPECT_EQ(node->GetExtendInfo()->GetOwnerGraphBarePtr(), graph.get());
  EXPECT_TRUE(data.models.empty());
}

TEST(Om2GraphPackageHelperUT, InvalidSourceJsonKeepsGraphAvailableForCorrection) {
  auto graph = std::make_shared<ExecuteGraph>("in_place_graph");
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  SetEmptyModelIo(*graph);
  auto *node = graph->AddNode(std::make_shared<OpDesc>("op", "NoOp"));
  ASSERT_NE(node, nullptr);
  // 在图已拆分之后校验节点 JSON。
  ASSERT_TRUE(AttrUtils::SetStr(node->GetOpDescBarePtr(), gert::attr::kSourceOp, "{}"));
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  const auto *original = data.manifest.get();
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 1U);
  EXPECT_TRUE(graph->GetAllSubgraphs().empty());
  EXPECT_EQ(node->GetExtendInfo()->GetOwnerGraphBarePtr(), graph.get());
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

namespace {
// This fixture observes assembly only. Its test Kernel declares Tensor ports without
// performing a calculation; numerical execution is covered in the route generator ST.
class AssemblyPortsCodeBuilder : public gert::om2::KernelCodeBuilder {
 public:
  gert::om2::KernelImplType GetImplType() const override {
    return gert::om2::KernelImplType::kCodegenImpl;
  }
  graphStatus Collect(const gert::om2::KernelCollectContext &context, gert::om2::KernelProgramInfo &info) override {
    using namespace gert::om2;
    if (InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    std::vector<KernelInputPortInfo> ports;
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      ports.push_back({"tensor_" + std::to_string(i), Om2ValueType::kGertTensorPtr});
    }
    if (SetInputPortInfos(context, ports, info) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    return SetOutputPortInfos(context, {}, info);
  }
  graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol + "() { return kOk; }\n";
    return GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2TestAssemblyPorts", AssemblyPortsCodeBuilder);

void AddModelOutput(const std::shared_ptr<ExecuteGraph> &graph) {
  // Use the actual RT2 output-buffer protocol: the registered OutputData shell
  // is consumed by a side-effect kernel, which StageSplitter links to NetOutput.
  GeTensorDesc output_desc(GeShape({2}), FORMAT_ND, DT_UINT8);
  output_desc.SetName("output");
  ASSERT_TRUE(AttrUtils::SetListTensor(*graph, gert::attr::kModelOutputs, {GeTensor(output_desc)}));
  auto shell = std::make_shared<OpDesc>("output_shell", "OutputData");
  ASSERT_EQ(shell->AddOutputDesc(output_desc), GRAPH_SUCCESS);
  auto consumer = std::make_shared<OpDesc>("output_writer", "Om2TestAssemblyPorts");
  ASSERT_EQ(consumer->AddInputDesc(output_desc), GRAPH_SUCCESS);
  auto *output_node = graph->AddNode(shell);
  auto *writer = graph->AddNode(consumer);
  ASSERT_NE(output_node, nullptr);
  ASSERT_NE(writer, nullptr);
  ASSERT_NE(graph->AddEdge(output_node, 0, writer, 0), nullptr);
  ASSERT_EQ(ExecuteGraphUtils::SetGraphOutputs(graph.get(), {{output_node, 0}}), GRAPH_SUCCESS);
}

void ExpectResourceFailure(const std::shared_ptr<ExecuteGraph> &graph, const char *expected_log) {
  ASSERT_NO_FATAL_FAILURE(AddModelOutput(graph));
  gert::GertRuntimeStub runtime_stub;
  gert::GertModelData data;
  gert::InitGertModelData(data);
  gert::InitGertModelDataUnit(data);
  data.manifest->model_num = 19U;
  const auto *manifest = data.manifest.get();
  const auto *unit = data.models[0].get();
  const auto *constants = data.constants.get();
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  runtime_stub.GetSlogStub().Clear();
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_NE(runtime_stub.GetSlogStub().FindLog(DLOG_ERROR, expected_log), -1);
  // Resource binding happens after the public graph has been split in place.
  EXPECT_EQ(graph->GetAllSubgraphs().size(), 3U);
  EXPECT_EQ(data.manifest.get(), manifest);
  EXPECT_EQ(data.manifest->model_num, 19U);
  ASSERT_EQ(data.models.size(), 1U);
  EXPECT_EQ(data.models[0].get(), unit);
  EXPECT_EQ(data.constants.get(), constants);
  EXPECT_TRUE(data.models[0]->runtime->source_artifacts.empty());
  EXPECT_EQ(data.models[0]->runtime->so_artifact.data_size, 0U);
}

using Om2DynamicGraphAssemblyUT = om2_test::DynamicPackageTest;
}  // namespace

TEST(Om2GraphPackageHelperUT, CustomKernelWithoutLibraryPathDoesNotPublishOutput) {
  auto graph = std::make_shared<ExecuteGraph>("incomplete_custom_kernel");
  ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
  const auto desc = std::make_shared<OpDesc>("custom", "CustomKernelWithoutSoPath");
  ASSERT_TRUE(AttrUtils::SetStr(desc, gert::attr::kCustomKernelName, desc->GetType()));
  ASSERT_TRUE(AttrUtils::SetStr(desc, gert::attr::kCustomRunFuncSymbol, "RunCustomKernel"));
  ASSERT_NE(graph->AddNode(desc), nullptr);
  ExpectResourceFailure(graph, "Custom kernel CustomKernelWithoutSoPath is missing its shared library path");
}

TEST(Om2GraphPackageHelperUT, StageInputContinuityIsRelaxedOnlyForDeferredBindings) {
  struct Feed {
    const char *type;
    StageKind stage;
    StageInputClass input_class;
  };
  const Feed feeds[]{
      {"ConstData", StageKind::kInit, StageInputClass::kLoad},
      {"Data", StageKind::kMain, StageInputClass::kExecute},
  };
  for (const auto &feed : feeds) {
    SCOPED_TRACE(feed.type);
    for (const bool defer : {false, true}) {
      SCOPED_TRACE(defer);
      for (const int64_t second_index : {1, 2}) {
        ExecuteGraph graph("stage_feed_indices");
        const GeTensorDesc port(GeShape({1}), FORMAT_ND, DT_UINT8);
        for (const int64_t index : {int64_t{0}, second_index}) {
          auto desc = std::make_shared<OpDesc>("input_" + std::to_string(index), feed.type);
          ASSERT_EQ(desc->AddOutputDesc(port), GRAPH_SUCCESS);
          ASSERT_TRUE(AttrUtils::SetInt(desc, gert::attr::kIndex, index));
          ASSERT_TRUE(AttrUtils::SetStr(desc, gert::attr::kStage, StageKindToString(feed.stage)));
          ASSERT_NE(graph.AddNode(desc), nullptr);
        }
        StagePlan plan;
        ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
        StageSplitter splitter(graph, defer);
        const auto status = splitter.Split(plan, {});
        if (defer || second_index == 1) {
          ASSERT_EQ(status, GRAPH_SUCCESS);
          const auto &bindings = splitter.GetInputBindings(feed.stage, feed.input_class);
          ASSERT_EQ(bindings.size(), 2U);
          EXPECT_EQ(bindings[0].index, 0);
          EXPECT_EQ(bindings[1].index, second_index);
        } else {
          EXPECT_NE(status, GRAPH_SUCCESS);
          EXPECT_TRUE(graph.GetAllSubgraphs().empty());
        }
      }
    }
  }
}

// E-01: the public route performs real collection and source generation.
// Verify caller IO order, declaration-order weight numbering and exact binary payloads.
void AddAssemblyPortDescs(OpDesc &ports, OpDesc &output_shell, const GeTensorDesc &first_desc,
                          const GeTensorDesc &second_desc) {
  for (const auto &desc : {first_desc, second_desc}) {
    ASSERT_EQ(ports.AddInputDesc(desc), GRAPH_SUCCESS);
    ASSERT_EQ(output_shell.AddOutputDesc(desc), GRAPH_SUCCESS);
  }
  for (const auto &desc : {first_desc, second_desc}) {
    ASSERT_EQ(ports.AddInputDesc(desc), GRAPH_SUCCESS);
  }
}

void CheckAssembledModel(const gert::GertModelData &data, const std::vector<uint8_t> &binary_bytes) {
  ASSERT_EQ(data.models.size(), 1U);
  const auto &model = *data.models[0];
  EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kDynamic);
  EXPECT_EQ(model.debug, nullptr);
  ASSERT_NE(model.model_meta, nullptr);
  ASSERT_EQ(model.model_meta->input_desc.size(), 2U);
  ASSERT_EQ(model.model_meta->output_desc.size(), 2U);
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->input_desc[0].name), "float_input");
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->input_desc[1].name), "int_input");
  EXPECT_EQ(model.model_meta->input_desc[0].data_type, DT_FLOAT);
  EXPECT_EQ(model.model_meta->input_desc[1].shape, std::vector<int64_t>({3}));
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->output_desc[0].name), "second_result");
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->output_desc[1].name), "first_result");
  EXPECT_EQ(model.model_meta->output_desc[0].data_type, DT_INT32);
  EXPECT_EQ(model.model_meta->output_desc[1].shape, std::vector<int64_t>({2}));
  ASSERT_NE(data.constants, nullptr);
  EXPECT_TRUE(data.constants->constants_data.empty());
  EXPECT_EQ(data.so_bins, nullptr);
  ASSERT_EQ(data.kernels->binaries.size(), 1U);
  ASSERT_EQ(data.kernels->binaries[0]->data_size, binary_bytes.size());
  EXPECT_EQ(std::memcmp(data.kernels->binaries[0]->data.get(), binary_bytes.data(), binary_bytes.size()), 0);
  EXPECT_GT(model.runtime->so_artifact.data_size, 0U);
  EXPECT_FALSE(model.runtime->source_artifacts.empty());
}

TEST_F(Om2DynamicGraphAssemblyUT, AssemblesOrderedIoAndGraphBinaryThroughPublicEntry) {
  om2_test::Om2CompileStub compiler;
  auto graph = std::make_shared<ExecuteGraph>("external_assembly");
  const GeTensorDesc first_desc(GeShape({2}), FORMAT_ND, DT_FLOAT);
  const GeTensorDesc second_desc(GeShape({3}), FORMAT_ND, DT_INT32);
  const auto named_tensor = [](GeTensorDesc desc, const char *name) {
    desc.SetName(name);
    return GeTensor(desc);
  };
  ASSERT_TRUE(
      AttrUtils::SetListTensor(*graph, gert::attr::kModelInputs,
                               {named_tensor(first_desc, "float_input"), named_tensor(second_desc, "int_input")}));
  ASSERT_TRUE(
      AttrUtils::SetListTensor(*graph, gert::attr::kModelOutputs,
                               {named_tensor(second_desc, "second_result"), named_tensor(first_desc, "first_result")}));
  auto first = std::make_shared<OpDesc>("first_input", "Data");
  auto second = std::make_shared<OpDesc>("second_input", "Data");
  ASSERT_EQ(first->AddOutputDesc(first_desc), GRAPH_SUCCESS);
  ASSERT_EQ(second->AddOutputDesc(second_desc), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetInt(first, "index", 0));
  ASSERT_TRUE(AttrUtils::SetInt(second, "index", 1));
  auto *first_node = graph->AddNode(first);
  auto *second_node = graph->AddNode(second);
  ASSERT_NE(first_node, nullptr);
  ASSERT_NE(second_node, nullptr);
  auto ports = std::make_shared<OpDesc>("ports", "Om2TestAssemblyPorts");
  auto output_shell = std::make_shared<OpDesc>("output_shell", "OutputData");
  ASSERT_NO_FATAL_FAILURE(AddAssemblyPortDescs(*ports, *output_shell, first_desc, second_desc));
  auto *output_node = graph->AddNode(output_shell);
  ASSERT_NE(output_node, nullptr);
  auto *ports_node = graph->AddNode(ports);
  ASSERT_NE(ports_node, nullptr);
  ASSERT_NE(graph->AddEdge(first_node, 0, ports_node, 0), nullptr);
  ASSERT_NE(graph->AddEdge(second_node, 0, ports_node, 1), nullptr);
  for (int32_t i = 0; i < 2; ++i) {
    ASSERT_NE(graph->AddEdge(output_node, i, ports_node, 2 + i), nullptr);
  }
  ASSERT_EQ(ExecuteGraphUtils::SetGraphOutputs(graph.get(), {{output_node, 1}, {output_node, 0}}), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kBinaryIds, {"binary"}));
  ASSERT_TRUE(AttrUtils::SetStr(*graph, std::string(gert::attr::kBinaryPrefix) + "binary.name", "probe.o"));
  const std::vector<uint8_t> binary_bytes{0x7fU, 0U, 0x22U, 0x5cU};
  const GeTensor binary(GeTensorDesc(GeShape({4}), FORMAT_ND, DT_UINT8), binary_bytes);
  ASSERT_TRUE(AttrUtils::SetTensor(*graph, std::string(gert::attr::kBinaryPrefix) + "binary.content", binary));
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  gert::GertModelData data;
  ASSERT_EQ(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckAssembledModel(data, binary_bytes));
}
TEST(Om2GraphPackageHelperUT, IgnoresExternalComputeNodeIndexBeforeScheduling) {
  gert::GertRuntimeStub stub;
  om2_test::Om2CompileStub compiler;
  auto graph = std::make_shared<ExecuteGraph>("external_index");
  ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
  auto *node = graph->AddNode(std::make_shared<OpDesc>("noop", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDescBarePtr(), gert::kComputeNodeIndex, -1));
  ASSERT_NO_FATAL_FAILURE(AddModelOutput(graph));
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  gert::GertModelData data;
  ASSERT_EQ(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_FALSE(node->GetOpDescBarePtr()->HasAttr(gert::kComputeNodeIndex));
}

}  // namespace ge
