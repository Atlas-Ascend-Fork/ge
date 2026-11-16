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
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
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

namespace ge {
namespace {
void SetEmptyModelIo(ExecuteGraph &graph) {
  ASSERT_TRUE(AttrUtils::SetListTensor(graph, gert::attr::kModelInputs, std::vector<GeTensor>{}));
  ASSERT_TRUE(AttrUtils::SetListTensor(graph, gert::attr::kModelOutputs, std::vector<GeTensor>{}));
  ASSERT_TRUE(AttrUtils::SetListStr(graph, gert::attr::kModelInputNames, {}));
  ASSERT_TRUE(AttrUtils::SetListStr(graph, gert::attr::kModelOutputNames, {}));
}
}  // namespace

TEST(Om2GraphPackageHelperUT, GraphFacadeRejectsInvalidAndComputeGraphs) {
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  const auto *original = data.manifest.get();
  Graph invalid("invalid_execute", 1);
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(invalid, data), SUCCESS);
  Graph compute("compute");
  ASSERT_EQ(compute.SetValid(), GRAPH_SUCCESS);
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(compute, data), SUCCESS);
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

TEST(Om2GraphPackageHelperUT, RejectsOutputCountMismatch) {
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
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), node_count);
  EXPECT_TRUE(graph->GetAllSubgraphs().empty());
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

TEST(Om2GraphPackageHelperUT, MissingModelMetadataLeavesOutputUntouched) {
  auto graph = std::make_shared<ExecuteGraph>("missing_metadata");
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  auto *original = data.manifest.get();
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(graph->GetDirectNode().empty());
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

TEST(Om2GraphPackageHelperUT, MetadataFailureAfterSplitKeepsInPlaceChanges) {
  auto graph = std::make_shared<ExecuteGraph>("in_place_graph");
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  SetEmptyModelIo(*graph);
  ASSERT_NE(graph->AddNode(std::make_shared<OpDesc>("op", "NoOp")), nullptr);
  // 在图已拆分之后，因来源描述不完整而失败，不进入源码编译。
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kSourceIds, {"missing_source"}));
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  const auto *original = data.manifest.get();
  EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(graph->GetAllSubgraphs().size(), 3U);
  EXPECT_EQ(data.manifest.get(), original);
  EXPECT_TRUE(data.models.empty());
}

namespace {
std::string WeightKey(const std::string &id, const char *field) {
  return std::string(gert::attr::kWeightPrefix) + id + "." + field;
}

void SetHostWeight(ExecuteGraph &graph, const std::string &id, const std::vector<uint8_t> &bytes) {
  ASSERT_TRUE(AttrUtils::SetStr(graph, WeightKey(id, "mode"), "embedded"));
  ASSERT_TRUE(AttrUtils::SetStr(graph, WeightKey(id, "placement"), "host"));
  ASSERT_TRUE(AttrUtils::SetInt(graph, WeightKey(id, "size"), bytes.size()));
  const GeTensor content(GeTensorDesc(GeShape({static_cast<int64_t>(bytes.size())}), FORMAT_ND, DT_UINT8), bytes);
  ASSERT_TRUE(AttrUtils::SetTensor(graph, WeightKey(id, "content"), content));
}

FastNode *AddWeightConst(ExecuteGraph &graph, const std::string &id, const int64_t offset, const int64_t length) {
  auto desc = std::make_shared<OpDesc>("weight_tensor", "Const");
  const GeTensorDesc tensor_desc(GeShape({length}), FORMAT_ND, DT_UINT8);
  if (desc->AddOutputDesc(tensor_desc) != GRAPH_SUCCESS) {
    return nullptr;
  }
  // ExecuteGraph Const stores a raw runtime Tensor, unlike ComputeGraph Const's GeTensor attr.
  const gert::Tensor tensor({{length}, {length}}, {FORMAT_ND, FORMAT_ND, {}}, gert::kOnHost, DT_UINT8, nullptr);
  if (!AttrUtils::SetZeroCopyBytes(desc, "value",
                                   Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&tensor), sizeof(tensor))) ||
      !AttrUtils::SetStr(desc, gert::attr::kNodeWeightId, id) ||
      !AttrUtils::SetInt(desc, "gert.weight.offset", offset) ||
      !AttrUtils::SetInt(desc, "gert.weight.length", length)) {
    return nullptr;
  }
  return graph.AddNode(desc);
}

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
  const GeTensorDesc output_desc(GeShape({2}), FORMAT_ND, DT_UINT8);
  ASSERT_TRUE(AttrUtils::SetListTensor(*graph, gert::attr::kModelOutputs, {GeTensor(output_desc)}));
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kModelOutputNames, {"output"}));
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

// E-02: independent resource errors must reach the actual post-split reference validator.
TEST(Om2GraphPackageHelperUT, UnknownSharedLibraryReferenceDoesNotPublishOutput) {
  auto graph = std::make_shared<ExecuteGraph>("unknown_so");
  SetEmptyModelIo(*graph);
  auto *node = graph->AddNode(std::make_shared<OpDesc>("consumer", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(AttrUtils::SetListStr(node->GetOpDescPtr(), gert::attr::kSoIds, {"missing_so"}));
  ExpectResourceFailure(graph, "Unknown SO resource missing_so");
}

TEST(Om2GraphPackageHelperUT, UnknownKernelBinaryReferenceDoesNotPublishOutput) {
  auto graph = std::make_shared<ExecuteGraph>("unknown_binary");
  SetEmptyModelIo(*graph);
  auto *node = graph->AddNode(std::make_shared<OpDesc>("consumer", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(node->GetOpDescPtr(), gert::attr::kNodeBinaryId, "missing_binary"));
  ExpectResourceFailure(graph, "Unknown binary resource missing_binary");
}

namespace {
void KeepWeightReferenceInPreparedGraph(ExecuteGraph &graph, FastNode *weight) {
  ASSERT_NE(weight, nullptr);
  auto consumer = std::make_shared<OpDesc>("weight_consumer", "Om2TestAssemblyPorts");
  // This side-effect consumer belongs to Main even when its only input is a
  // constant; otherwise CEM lifts the entire fixture before reference binding.
  ASSERT_TRUE(AttrUtils::SetStr(consumer, gert::attr::kStage, gert::attr::kStageMain));
  ASSERT_EQ(consumer->AddInputDesc(weight->GetOpDescBarePtr()->GetOutputDesc(0U)), GRAPH_SUCCESS);
  auto *node = graph.AddNode(consumer);
  ASSERT_NE(node, nullptr);
  ASSERT_NE(graph.AddEdge(weight, 0, node, 0), nullptr);
}
}  // namespace

TEST(Om2GraphPackageHelperUT, UnknownWeightReferenceDoesNotPublishOutput) {
  auto graph = std::make_shared<ExecuteGraph>("unknown_weight");
  SetEmptyModelIo(*graph);
  KeepWeightReferenceInPreparedGraph(*graph, AddWeightConst(*graph, "missing_weight", 0, 2));
  ExpectResourceFailure(graph, "Node references weight missing_weight which is not an embedded resource");
}

TEST(Om2GraphPackageHelperUT, OutOfRangeWeightSliceDoesNotPublishOutput) {
  auto graph = std::make_shared<ExecuteGraph>("weight_slice_overflow");
  SetEmptyModelIo(*graph);
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kWeightIds, {"weight"}));
  SetHostWeight(*graph, "weight", {1U, 2U, 3U, 4U});
  KeepWeightReferenceInPreparedGraph(*graph, AddWeightConst(*graph, "weight", 3, 2));
  ExpectResourceFailure(graph, "Weight weight slice [3, 5) exceeds size 4");
}

// E-01: the public route performs real collection and source generation.
// Verify caller IO order, declaration-order weight numbering and exact binary payloads.
TEST_F(Om2DynamicGraphAssemblyUT, AssemblesOrderedIoResourcesAndWeightSlicesThroughPublicEntry) {
  om2_test::Om2CompileStub compiler;
  auto graph = std::make_shared<ExecuteGraph>("external_assembly");
  const GeTensorDesc first_desc(GeShape({2}), FORMAT_ND, DT_FLOAT);
  const GeTensorDesc second_desc(GeShape({3}), FORMAT_ND, DT_INT32);
  const GeTensorDesc weight_desc(GeShape({2}), FORMAT_ND, DT_UINT8);
  ASSERT_TRUE(
      AttrUtils::SetListTensor(*graph, gert::attr::kModelInputs, {GeTensor(first_desc), GeTensor(second_desc)}));
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kModelInputNames, {"float_input", "int_input"}));
  ASSERT_TRUE(AttrUtils::SetListTensor(*graph, gert::attr::kModelOutputs,
                                       {GeTensor(second_desc), GeTensor(first_desc), GeTensor(weight_desc)}));
  ASSERT_TRUE(
      AttrUtils::SetListStr(*graph, gert::attr::kModelOutputNames, {"second_result", "first_result", "weight_result"}));
  auto first = std::make_shared<OpDesc>("first_input", "Data");
  auto second = std::make_shared<OpDesc>("second_input", "Data");
  ASSERT_EQ(first->AddOutputDesc(first_desc), GRAPH_SUCCESS);
  ASSERT_EQ(second->AddOutputDesc(second_desc), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetInt(first, "index", 0));
  ASSERT_TRUE(AttrUtils::SetInt(second, "index", 1));
  auto *first_node = graph->AddNode(first);
  auto *second_node = graph->AddNode(second);
  auto *weight_node = AddWeightConst(*graph, "a_weight", 2, 2);
  ASSERT_NE(first_node, nullptr);
  ASSERT_NE(second_node, nullptr);
  ASSERT_NE(weight_node, nullptr);
  auto ports = std::make_shared<OpDesc>("ports", "Om2TestAssemblyPorts");
  auto output_shell = std::make_shared<OpDesc>("output_shell", "OutputData");
  for (const auto &desc : {first_desc, second_desc, weight_desc}) {
    ASSERT_EQ(ports->AddInputDesc(desc), GRAPH_SUCCESS);
    ASSERT_EQ(output_shell->AddOutputDesc(desc), GRAPH_SUCCESS);
  }
  for (const auto &desc : {first_desc, second_desc, weight_desc}) {
    ASSERT_EQ(ports->AddInputDesc(desc), GRAPH_SUCCESS);
  }
  auto *output_node = graph->AddNode(output_shell);
  ASSERT_NE(output_node, nullptr);
  auto *ports_node = graph->AddNode(ports);
  ASSERT_NE(ports_node, nullptr);
  ASSERT_NE(graph->AddEdge(first_node, 0, ports_node, 0), nullptr);
  ASSERT_NE(graph->AddEdge(second_node, 0, ports_node, 1), nullptr);
  ASSERT_NE(graph->AddEdge(weight_node, 0, ports_node, 2), nullptr);
  for (int32_t i = 0; i < 3; ++i) {
    ASSERT_NE(graph->AddEdge(output_node, i, ports_node, 3 + i), nullptr);
  }
  ASSERT_EQ(ExecuteGraphUtils::SetGraphOutputs(graph.get(), {{output_node, 1}, {output_node, 0}, {output_node, 2}}),
            GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kWeightIds, {"z_weight", "a_weight"}));
  const std::vector<uint8_t> first_bytes{0x41U, 0U, 0x5cU};
  const std::vector<uint8_t> second_bytes{0x22U, 0U, 0x77U, 0x88U, 0xffU};
  SetHostWeight(*graph, "z_weight", first_bytes);
  SetHostWeight(*graph, "a_weight", second_bytes);
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kBinaryIds, {"binary"}));
  ASSERT_TRUE(AttrUtils::SetStr(*graph, std::string(gert::attr::kBinaryPrefix) + "binary.name", "probe.o"));
  const std::vector<uint8_t> binary_bytes{0x7fU, 0U, 0x22U, 0x5cU};
  const GeTensor binary(GeTensorDesc(GeShape({4}), FORMAT_ND, DT_UINT8), binary_bytes);
  ASSERT_TRUE(AttrUtils::SetTensor(*graph, std::string(gert::attr::kBinaryPrefix) + "binary.content", binary));
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  gert::GertModelData data;
  ASSERT_EQ(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  ASSERT_EQ(data.models.size(), 1U);
  const auto &model = *data.models[0];
  EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kDynamic);
  EXPECT_EQ(model.debug, nullptr);
  ASSERT_NE(model.model_meta, nullptr);
  ASSERT_EQ(model.model_meta->input_desc.size(), 2U);
  ASSERT_EQ(model.model_meta->output_desc.size(), 3U);
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->input_desc[0].name), "float_input");
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->input_desc[1].name), "int_input");
  EXPECT_EQ(model.model_meta->input_desc[0].data_type, DT_FLOAT);
  EXPECT_EQ(model.model_meta->input_desc[1].shape, std::vector<int64_t>({3}));
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->output_desc[0].name), "second_result");
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->output_desc[1].name), "first_result");
  EXPECT_STREQ(gert::GertGetStr(model.model_meta->output_desc[2].name), "weight_result");
  EXPECT_EQ(model.model_meta->output_desc[0].data_type, DT_INT32);
  EXPECT_EQ(model.model_meta->output_desc[1].shape, std::vector<int64_t>({2}));
  ASSERT_NE(data.constants, nullptr);
  ASSERT_EQ(data.constants->constants_data.size(), 2U);
  for (size_t i = 0U; i < 2U; ++i) {
    const auto &expected = i == 0U ? first_bytes : second_bytes;
    const auto &file = *data.constants->constants_data[i];
    ASSERT_EQ(file.data_size, expected.size());
    EXPECT_EQ(std::memcmp(file.data.get(), expected.data(), expected.size()), 0);
    ASSERT_LT(i, model.constants_config->consts.size());
    EXPECT_EQ(model.constants_config->consts[i]->index, i);
    EXPECT_EQ(gert::GetGertModelConstPlacement(*model.constants_config->consts[i]),
              gert::GertModelDataConstPlacement::kHost);
  }
  EXPECT_STREQ(gert::GertGetStr(model.constants_config->consts[0]->op_name), "z_weight");
  EXPECT_STREQ(gert::GertGetStr(model.constants_config->consts[1]->op_name), "a_weight");
  int64_t weight_index = -1;
  const auto bound_desc = weight_node->GetOpDescBarePtr()->GetOutputDesc(0U);
  ASSERT_TRUE(AttrUtils::GetInt(bound_desc, "om2_weight_index", weight_index));
  EXPECT_EQ(weight_index, 1);
  int64_t offset = -1;
  ASSERT_EQ(TensorUtils::GetDataOffset(bound_desc, offset), GRAPH_SUCCESS);
  EXPECT_EQ(offset, 2);
  std::vector<int64_t> slice;
  ASSERT_TRUE(AttrUtils::GetListInt(bound_desc, ATTR_NAME_GRAPH_FLATTEN_OFFSET, slice));
  EXPECT_EQ(slice, std::vector<int64_t>({2, 2}));
  ASSERT_EQ(data.kernels->binaries.size(), 1U);
  ASSERT_EQ(data.kernels->binaries[0]->data_size, binary_bytes.size());
  EXPECT_EQ(std::memcmp(data.kernels->binaries[0]->data.get(), binary_bytes.data(), binary_bytes.size()), 0);
  EXPECT_GT(model.runtime->so_artifact.data_size, 0U);
  EXPECT_FALSE(model.runtime->source_artifacts.empty());
}
namespace {
void SetAssemblyLibrary(ExecuteGraph &graph, const std::string &id, const char *name, const char *purpose) {
  const auto prefix = std::string(gert::attr::kSoPrefix) + id + ".";
  ASSERT_TRUE(AttrUtils::SetStr(graph, prefix + "name", name));
  ASSERT_TRUE(AttrUtils::SetStr(graph, prefix + "purpose", purpose));
  const GeTensor bytes(GeTensorDesc(GeShape({3}), FORMAT_ND, DT_UINT8), std::vector<uint8_t>{1U, 0U, 2U});
  ASSERT_TRUE(AttrUtils::SetTensor(graph, prefix + "content", bytes));
}
}  // namespace

TEST_F(Om2DynamicGraphAssemblyUT, PackagesDeclaredLibrariesAndExplicitTopoMappingWithoutLoadingThem) {
  gert::GertRuntimeStub stub;
  om2_test::Om2CompileStub compiler;
  auto graph = std::make_shared<ExecuteGraph>("library_assembly");
  ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
  ASSERT_NE(graph->AddNode(std::make_shared<OpDesc>("noop", "NoOp")), nullptr);
  ASSERT_NO_FATAL_FAILURE(AddModelOutput(graph));
  ASSERT_TRUE(AttrUtils::SetListInt(*graph, gert::attr::kComputeNodeTopoIds, {3, 7}));
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kSoIds, {"registry", "custom", "alias"}));
  ASSERT_NO_FATAL_FAILURE(SetAssemblyLibrary(*graph, "registry", "libregistry.so", "space_registry"));
  ASSERT_NO_FATAL_FAILURE(SetAssemblyLibrary(*graph, "custom", "libcustom.so", "custom"));
  ASSERT_NO_FATAL_FAILURE(SetAssemblyLibrary(*graph, "alias", "libregistry.so", "space_registry"));
  ASSERT_TRUE(AttrUtils::SetListStr(*graph, std::string(gert::attr::kSoPrefix) + "custom.dependencies", {"registry"}));
  const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  gert::GertModelData data;
  ASSERT_EQ(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
  ASSERT_NE(data.so_bins, nullptr);
  ASSERT_EQ(data.so_bins->libraries.size(), 1U);
  EXPECT_STREQ(gert::GertGetStr(data.so_bins->libraries[0]->so_name), "libregistry.so");
  ASSERT_NE(data.custom_ops, nullptr);
  ASSERT_EQ(data.custom_ops->libraries.size(), 1U);
  EXPECT_STREQ(gert::GertGetStr(data.custom_ops->libraries[0]->file_name), "libcustom.so");
  EXPECT_EQ(data.custom_ops->libraries[0]->data_size, 3U);
}

TEST(Om2GraphPackageHelperUT, RejectsInvalidExplicitTopoMappingsBeforeCompilation) {
  gert::GertRuntimeStub stub;
  const std::vector<std::vector<int64_t>> cases{{}, {-1}, {std::numeric_limits<int64_t>::max()}, {3}};
  for (const auto &ids : cases) {
    auto graph = std::make_shared<ExecuteGraph>("invalid_topo");
    ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
    auto *node = graph->AddNode(std::make_shared<OpDesc>("noop", "NoOp"));
    ASSERT_NE(node, nullptr);
    ASSERT_TRUE(AttrUtils::SetListInt(*graph, gert::attr::kComputeNodeTopoIds, ids));
    ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDescBarePtr(), gert::kComputeNodeIndex, 2));
    const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    gert::GertModelData data;
    stub.GetSlogStub().Clear();
    EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
    const char *diagnostic = ids.empty()   ? "must not be empty"
                             : ids[0] == 3 ? "exceeds explicit topo mapping size"
                                           : "Invalid logical compute-node topo ID";
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, diagnostic), -1);
    EXPECT_TRUE(data.models.empty());
  }
}

TEST(Om2GraphPackageHelperUT, RejectsUnsupportedLibraryPurposeAndForwardDependencyBeforeCompilation) {
  gert::GertRuntimeStub stub;
  for (const bool forward_dependency : {false, true}) {
    auto graph = std::make_shared<ExecuteGraph>("invalid_library");
    ASSERT_NO_FATAL_FAILURE(SetEmptyModelIo(*graph));
    ASSERT_NE(graph->AddNode(std::make_shared<OpDesc>("noop", "NoOp")), nullptr);
    ASSERT_NO_FATAL_FAILURE(AddModelOutput(graph));
    ASSERT_TRUE(AttrUtils::SetListStr(*graph, gert::attr::kSoIds, {"first", "second"}));
    ASSERT_NO_FATAL_FAILURE(
        SetAssemblyLibrary(*graph, "first", "libfirst.so", forward_dependency ? "space_registry" : "tiling"));
    ASSERT_NO_FATAL_FAILURE(SetAssemblyLibrary(*graph, "second", "libsecond.so", "space_registry"));
    if (forward_dependency) {
      ASSERT_TRUE(AttrUtils::SetListStr(*graph, std::string(gert::attr::kSoPrefix) + "first.dependencies", {"second"}));
    }
    const auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    gert::GertModelData data;
    stub.GetSlogStub().Clear();
    EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, data), SUCCESS);
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR,
                                         forward_dependency ? "must precede it" : "Unsupported shared library purpose"),
              -1);
    EXPECT_TRUE(data.models.empty());
  }
}
}  // namespace ge
