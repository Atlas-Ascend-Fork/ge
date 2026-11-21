/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/common/helper/model_save_helper.h"
#include "framework/common/zip_archive_reader.h"
#include "framework/common/zip_archive_writer.h"
#include "common/om2/codegen/om2_codegen.h"
#include "common/om2/codegen/om2_codegen_types.h"
#include "framework/om2/model_data/gert_model_data.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/om2/model_data/om2_package_contants.h"
#define private public
#include "framework/common/helper/om2_package_helper.h"
#include "framework/common/gert_model_data_serialize.h"
#undef private
#include "framework/common/framework_types_internal.h"
#include "framework/common/json_file.h"
#include "common/helper/visual_json_converter.h"
#include "common/model/ge_model.h"
#include "file_utils.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "common/env_path.h"
#include "mmpa/mmpa_api.h"
#include "graph/ge_local_context.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/custom_op_factory.h"
#include "graph/custom_op.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/file_utils.h"
#include "graph/utils/graph_utils.h"
#include <cstdio>
#include <cstring>
#include <sstream>

#include "ge_runtime_stub/include/common/share_graph.h"
#include "ge_runtime_stub/include/faker/ge_model_builder.h"
#include "ge_runtime_stub/include/faker/custom_taskdef_faker.h"
#include "ge_runtime_stub/include/faker/aicore_taskdef_faker.h"
#include "common/tbe_handle_store/tbe_kernel_store.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "lowering/model_converter.h"
#include "stub/gert_runtime_stub.h"

#include "om2_package_test_utils.h"
#include "om2_compile_stub.h"

namespace ge {
using Om2DynamicGePackageHelperUt = om2_test::DynamicPackageTest;
using om2_test::CreateGeRootModelWithAicoreOp;

TEST_F(Om2DynamicGePackageHelperUt, DynamicWeightsUseLoweringIndicesRatherThanMapOrder) {
  auto root_model = std::make_shared<GeRootModel>();
  auto first = std::make_shared<GeModel>();
  auto empty = std::make_shared<GeModel>();
  auto last = std::make_shared<GeModel>();
  const std::vector<uint8_t> first_bytes{1U, 2U};
  const std::vector<uint8_t> last_bytes{3U, 4U, 5U};
  first->SetWeight(Buffer::CopyFrom(first_bytes.data(), first_bytes.size()));
  last->SetWeight(Buffer::CopyFrom(last_bytes.data(), last_bytes.size()));
  ASSERT_TRUE(AttrUtils::SetInt(first, "om2_weight_index", 7));
  ASSERT_TRUE(AttrUtils::SetInt(last, "om2_weight_index", 9));
  root_model->SetSubgraphInstanceNameToModel("a", first);
  root_model->SetSubgraphInstanceNameToModel("b", empty);
  root_model->SetSubgraphInstanceNameToModel("c", last);

  gert::GertModelDataModel unit;
  gert::GertModelDataConstants constants;
  ASSERT_EQ(Om2PackageHelper::BuildDynamicConstantsData(Om2PackageHelper::GetModelsToBuild(nullptr, root_model), unit,
                                                        constants),
            SUCCESS);
  ASSERT_EQ(unit.constants_config->consts.size(), 2U);
  EXPECT_EQ(unit.constants_config->consts[0]->index, 7U);
  EXPECT_EQ(unit.constants_config->consts[1]->index, 9U);
  EXPECT_EQ(unit.constants_config->consts[0]->offset, 0);
  EXPECT_EQ(unit.constants_config->consts[1]->offset, 0);
  ASSERT_EQ(constants.constants_data.size(), 2U);
  EXPECT_STREQ(gert::GertGetStr(constants.constants_data[0]->file_name), "constant_7");
  EXPECT_STREQ(gert::GertGetStr(constants.constants_data[1]->file_name), "constant_9");
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicWeightsRejectMissingOrNegativeLoweringIndex) {
  auto root_model = std::make_shared<GeRootModel>();
  auto model = std::make_shared<GeModel>();
  const uint8_t data = 1U;
  model->SetWeight(Buffer::CopyFrom(&data, sizeof(data)));
  root_model->SetSubgraphInstanceNameToModel("model", model);
  gert::GertModelDataModel unit;
  gert::GertModelDataConstants constants;
  EXPECT_NE(Om2PackageHelper::BuildDynamicConstantsData(Om2PackageHelper::GetModelsToBuild(nullptr, root_model), unit,
                                                        constants),
            SUCCESS);
  ASSERT_TRUE(AttrUtils::SetInt(model, "om2_weight_index", -1));
  EXPECT_NE(Om2PackageHelper::BuildDynamicConstantsData(Om2PackageHelper::GetModelsToBuild(nullptr, root_model), unit,
                                                        constants),
            SUCCESS);
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicGeModelWithoutRootModelUsesRootGraph) {
  auto root = std::make_shared<GeRootModel>();
  auto graph = std::make_shared<ComputeGraph>("dynamic_root");
  graph->SetGraphUnknownFlag(true);
  root->SetRootGraph(graph);
  ASSERT_TRUE(AttrUtils::SetInt(graph, ATTR_MODEL_NOTIFY_NUM, 3));
  auto sub_model = std::make_shared<GeModel>();
  sub_model->SetGraph(std::make_shared<ComputeGraph>("static_sub"));
  ASSERT_TRUE(AttrUtils::SetInt(sub_model, ATTR_MODEL_MEMORY_SIZE, 4096));
  ASSERT_TRUE(AttrUtils::SetListStr(sub_model, ATTR_MODEL_OUT_NODES_NAME, {"result:0"}));
  ASSERT_TRUE(AttrUtils::SetStr(sub_model, ATTR_MODEL_HOST_ENV_CPU, "aarch64"));
  root->SetSubgraphInstanceNameToModel("static_sub", sub_model);

  GeModelPtr ge_model;
  ASSERT_EQ(Om2PackageHelper::GetDynamicGeModel(root, ge_model), SUCCESS);
  ASSERT_NE(ge_model, nullptr);
  EXPECT_EQ(ge_model->GetGraph(), graph);
  EXPECT_EQ(ge_model->GetName(), "dynamic_root");
  EXPECT_TRUE(ge_model->HasAttr(ATTR_MODEL_ATC_CMDLINE));
  int64_t value = -1;
  ASSERT_TRUE(AttrUtils::GetInt(ge_model, ATTR_MODEL_NOTIFY_NUM, value));
  EXPECT_EQ(value, 3);
  ASSERT_TRUE(AttrUtils::GetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, value));
  EXPECT_EQ(value, 0);
  std::vector<std::string> names;
  ASSERT_TRUE(AttrUtils::GetListStr(ge_model, ATTR_MODEL_OUT_NODES_NAME, names));
  EXPECT_EQ(names, std::vector<std::string>({"result:0"}));
  std::string cpu;
  ASSERT_TRUE(AttrUtils::GetStr(ge_model, ATTR_MODEL_HOST_ENV_CPU, cpu));
  EXPECT_EQ(cpu, "aarch64");
  EXPECT_EQ(root->GetSubgraphInstanceNameToModel().size(), 1U);
  EXPECT_EQ(sub_model->GetGraph()->GetName(), "static_sub");
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicGeModelPreservesExistingRootModel) {
  auto root = CreateGeRootModelWithAicoreOp();
  GeModelPtr ge_model;
  ASSERT_EQ(Om2PackageHelper::GetDynamicGeModel(root, ge_model), SUCCESS);
  EXPECT_EQ(ge_model, root->GetSubgraphInstanceNameToModel().at(root->GetRootGraph()->GetName()));
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicGeModelPrefersRootGraphAttributes) {
  auto root = std::make_shared<GeRootModel>();
  auto graph = std::make_shared<ComputeGraph>("root");
  root->SetRootGraph(graph);
  ASSERT_TRUE(AttrUtils::SetListStr(graph, ATTR_MODEL_OUT_NODES_NAME, {"root_output:0"}));
  auto sub_model = std::make_shared<GeModel>();
  ASSERT_TRUE(AttrUtils::SetListStr(sub_model, ATTR_MODEL_OUT_NODES_NAME, {"other_output:0"}));
  root->SetSubgraphInstanceNameToModel("sub", sub_model);
  GeModelPtr ge_model;
  ASSERT_EQ(Om2PackageHelper::GetDynamicGeModel(root, ge_model), SUCCESS);
  std::vector<std::string> names;
  ASSERT_TRUE(AttrUtils::GetListStr(ge_model, ATTR_MODEL_OUT_NODES_NAME, names));
  EXPECT_EQ(names, std::vector<std::string>({"root_output:0"}));
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicGeModelBuildsIoWithoutModelBuilderInputProperties) {
  auto root = std::make_shared<GeRootModel>();
  auto graph = std::make_shared<ComputeGraph>("root");
  root->SetRootGraph(graph);
  GeTensorDesc desc(GeShape({2}), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(desc, 8);
  auto data_op = std::make_shared<OpDesc>("data", DATA);
  ASSERT_EQ(data_op->AddInputDesc(desc), GRAPH_SUCCESS);
  ASSERT_EQ(data_op->AddOutputDesc(desc), GRAPH_SUCCESS);
  auto data = graph->AddNode(data_op);
  auto output_op = std::make_shared<OpDesc>("output", NETOUTPUT);
  ASSERT_EQ(output_op->AddInputDesc(desc), GRAPH_SUCCESS);
  auto output = graph->AddNode(output_op);
  ASSERT_EQ(GraphUtils::AddEdge(data->GetOutDataAnchor(0), output->GetInDataAnchor(0)), GRAPH_SUCCESS);

  GeModelPtr ge_model;
  ASSERT_EQ(Om2PackageHelper::GetDynamicGeModel(root, ge_model), SUCCESS);
  gert::GertModelDataModel static_unit;
  static_unit.model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  EXPECT_NE(Om2PackageHelper::BuildModelMeta(ge_model, static_unit), SUCCESS);
  EXPECT_TRUE(static_unit.model_meta->output_desc.empty());

  gert::GertModelDataModel unit;
  unit.execution_type = gert::GertModelExecutionType::kDynamic;
  unit.model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  ASSERT_EQ(Om2PackageHelper::BuildModelMeta(ge_model, unit), SUCCESS);
  EXPECT_EQ(unit.model_meta->input_desc.size(), 1U);
  ASSERT_EQ(unit.model_meta->output_desc.size(), 1U);
  EXPECT_STREQ(gert::GertGetStr(unit.model_meta->output_desc[0].name), "output_0_data_0");
  EXPECT_TRUE(output_op->GetSrcName().empty());
  EXPECT_TRUE(output_op->GetSrcIndex().empty());
}

TEST_F(Om2DynamicGePackageHelperUt, DynamicManifestCollectsSubmodelFeaturesWithoutRootModel) {
  auto root = std::make_shared<GeRootModel>();
  root->SetRootGraph(std::make_shared<ComputeGraph>("root"));
  auto sub_model = std::make_shared<GeModel>();
  auto tasks = std::make_shared<domi::ModelTaskDef>();
  tasks->add_task()->set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_HCCL));
  sub_model->SetModelTaskDef(tasks);
  root->SetSubgraphInstanceNameToModel("sub", sub_model);
  root->SetSubgraphInstanceNameToModel("another_sub", std::make_shared<GeModel>());
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  gert::InitGertModelDataUnit(data);
  const auto *const original_manifest = data.manifest.get();
  const auto *const original_unit = data.models[0].get();
  ASSERT_EQ(Om2PackageHelper::BuildDynamicManifest(data, Om2PackageHelper::GetModelsToBuild(nullptr, root)), SUCCESS);
  EXPECT_EQ(data.manifest.get(), original_manifest);
  EXPECT_EQ(data.models[0].get(), original_unit);
  EXPECT_EQ(data.manifest->model_num, 1U);
  EXPECT_TRUE(data.manifest->compatibility.used_features.empty());
  EXPECT_STREQ(gert::GertGetStr(data.manifest->compatibility.required_executor_version), "");
}

TEST_F(Om2DynamicGePackageHelperUt, BuildDynamicGertModelDataRejectsMissingInput) {
  Om2PackageHelper helper;
  gert::GertModelData data;
  EXPECT_NE(helper.BuildDynamicGertModelData(nullptr, data), SUCCESS);
  auto root = std::make_shared<GeRootModel>();
  EXPECT_NE(helper.BuildDynamicGertModelData(root, data), SUCCESS);
  root->SetRootGraph(std::make_shared<ComputeGraph>("root"));
  EXPECT_NE(helper.BuildDynamicGertModelData(root, data), SUCCESS);
  root->SetSubgraphInstanceNameToModel("root", std::make_shared<GeModel>());
  EXPECT_NE(helper.BuildDynamicGertModelData(root, data), SUCCESS);
  EXPECT_TRUE(data.models.empty());
  EXPECT_EQ(data.manifest, nullptr);
}

TEST_F(Om2DynamicGePackageHelperUt, BuildDynamicGertModelDataFailurePreservesOutput) {
  gert::GertModelData data;
  data.manifest = std::make_unique<gert::GertModelDataManifest>();
  gert::InitGertModelDataUnit(data);
  data.models[0]->execution_type = gert::GertModelExecutionType::kDynamic;
  const auto *original_model = data.models[0].get();
  auto *original_manifest = data.manifest.get();
  EXPECT_NE(Om2PackageHelper().BuildDynamicGertModelData(nullptr, data), SUCCESS);
  ASSERT_EQ(data.models.size(), 1U);
  EXPECT_EQ(data.models[0].get(), original_model);
  EXPECT_EQ(gert::GetGertModelExecutionType(*data.models[0]), gert::GertModelExecutionType::kDynamic);
  EXPECT_EQ(data.manifest.get(), original_manifest);
}

TEST_F(Om2DynamicGePackageHelperUt, CompileFailurePreservesPublishedDynamicModelData) {
  gert::GertRuntimeStub runtime_stub;
  runtime_stub.StubByNodeTypes({"Rank"});
  auto graph = gert::ShareGraph::BuildRankGraph();
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);

  gert::GertModelData data;
  gert::InitGertModelData(data);
  gert::InitGertModelDataUnit(data);
  data.manifest->model_num = 17U;
  data.models[0]->execution_type = gert::GertModelExecutionType::kDynamic;
  const auto *const model = data.models[0].get();
  const auto *const manifest = data.manifest.get();

  const auto fail_compile = +[](const std::vector<gert::GertModelDataFile> &sources, const Om2CompileInfo &info,
                                gert::GertModelDataFile &so, bool) -> Status {
    EXPECT_FALSE(sources.empty());
    EXPECT_FALSE(info.artifact_name.empty());
    const uint8_t partial[] = {0x7f, 'E', 'L', 'F'};
    so.data = gert::GertMakeFileData(partial, sizeof(partial));
    so.data_size = sizeof(partial);
    return FAILED;
  };
  MOCKER(Om2Utils::CompileGeneratedCppToSo).expects(once()).will(invoke(fail_compile));
  EXPECT_NE(Om2PackageHelper().BuildDynamicGertModelData(root, data), SUCCESS);
  EXPECT_NO_THROW(GlobalMockObject::verify());
  GlobalMockObject::reset();

  ASSERT_EQ(data.models.size(), 1U);
  EXPECT_EQ(data.models[0].get(), model);
  EXPECT_EQ(gert::GetGertModelExecutionType(*data.models[0]), gert::GertModelExecutionType::kDynamic);
  ASSERT_EQ(data.manifest.get(), manifest);
  EXPECT_EQ(data.manifest->model_num, 17U);
}

TEST_F(Om2DynamicGePackageHelperUt, PackagesRegisteredKernelWithoutSpecializedCollector) {
  gert::GertRuntimeStub runtime_stub;
  runtime_stub.StubByNodeTypes({"Rank"});
  om2_test::Om2CompileStub compiler;
  auto graph = gert::ShareGraph::BuildRankGraph();
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);

  gert::GertModelData data;
  gert::InitGertModelData(data);
  gert::InitGertModelDataUnit(data);
  data.manifest->model_num = 17U;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, data), SUCCESS);
  ASSERT_NE(data.manifest, nullptr);
  EXPECT_NE(data.manifest->model_num, 17U);
  ASSERT_EQ(data.models.size(), 1U);
  ASSERT_NE(data.models[0]->runtime, nullptr);
  EXPECT_GT(data.models[0]->runtime->so_artifact.data_size, 0U);
  const auto &sources = data.models[0]->runtime->source_artifacts;
  EXPECT_TRUE(std::any_of(sources.begin(), sources.end(), [](const auto &source) {
    return source.data_size != 0U &&
           std::string(reinterpret_cast<const char *>(source.data.get()), source.data_size).find("FakeExecuteNode") !=
               std::string::npos;
  }));
}

class Om2DynamicPackageHelperUt : public Om2DynamicGePackageHelperUt, public testing::WithParamInterface<bool> {
 protected:
  static void SetUpTestSuite() {
    const auto work = EnvPath().GetOrCreateCaseTmpPath("Om2DynamicPackageHelperUt");
    om2_test::ScopedEnvVar home("ASCEND_HOME_PATH", EnvPath().GetAscendInstallPath().c_str());
    om2_test::ScopedEnvVar work_path("ASCEND_WORK_PATH", work.c_str());
    om2_test::ScopedEnvVar asan("ASAN_OPTIONS", "detect_leaks=0:halt_on_error=0");
    om2_test::ScopedEnvVar lsan("LSAN_OPTIONS", "exitcode=0");
    const auto options = GetThreadLocalContext().GetAllGraphOptions();
    GetThreadLocalContext().SetGraphOption({{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0'"}});
    root_ = CreateGeRootModelWithAicoreOp();
    ASSERT_NE(root_, nullptr);
    const auto model = root_->GetSubgraphInstanceNameToModel().begin()->second;
    const auto parent = std::make_shared<ComputeGraph>("root_g1");
    model->GetGraph()->SetParentGraph(parent);
    om2_test::SyncKernelNameForAllModels(root_);
    compiled_ = std::make_unique<gert::GertModelData>();
    const auto ret = Om2PackageHelper().BuildOm2ModelData(model, *compiled_, root_);
    GetThreadLocalContext().SetGraphOption(options);
    ASSERT_EQ(ret, SUCCESS);
  }

  static void TearDownTestSuite() {
    compiled_.reset();
    root_.reset();
  }

  inline static GeRootModelPtr root_;
  inline static std::unique_ptr<gert::GertModelData> compiled_;
};

TEST_P(Om2DynamicPackageHelperUt, PrepareDynamicModelDataPreservesResourcesForCompilation) {
  om2_test::Om2CompileStub compiler;
  // Keep real resource preparation and source publication; compilation is covered separately.
  const auto compiled = compiled_.get();
  ASSERT_NE(compiled, nullptr);
  ASSERT_EQ(compiled->models.size(), 1U);
  auto root = CreateGeRootModelWithAicoreOp();
  ASSERT_NE(root, nullptr);
  std::vector<gert::GertModelDataFile> artifacts;
  std::string artifact_name;
  for (const auto &source : compiled->models[0]->runtime->source_artifacts) {
    const std::string name = gert::GertGetStr(source.file_name);
    constexpr char suffix[] = "_internal.h";
    if (name.size() >= sizeof(suffix) - 1U &&
        name.compare(name.size() - sizeof(suffix) + 1U, sizeof(suffix) - 1U, suffix) == 0) {
      artifact_name = name.substr(0U, name.size() - sizeof(suffix) + 1U);
    }
    gert::GertModelDataFile copy;
    copy.file_name = gert::GertMakeStr(name);
    copy.data = gert::GertMakeFileData(source.data.get(), source.data_size);
    copy.data_size = source.data_size;
    artifacts.push_back(std::move(copy));
  }
  ASSERT_FALSE(artifact_name.empty());
  root->SetModelName(artifact_name);
  const auto ge_model = root->GetSubgraphInstanceNameToModel().begin()->second;
  ASSERT_TRUE(AttrUtils::SetInt(ge_model, "om2_weight_index", 2));
  if (!GetParam()) {
    auto sub_model = ge_model;
    root->RemoveInstanceSubgraphModel(root->GetRootGraph()->GetName());
    root->SetSubgraphInstanceNameToModel("sub", sub_model);
  }
  const auto extra_root = CreateGeRootModelWithAicoreOp();
  ASSERT_NE(extra_root, nullptr);
  const auto extra_model = extra_root->GetSubgraphInstanceNameToModel().begin()->second;
  ASSERT_TRUE(AttrUtils::SetInt(extra_model, "om2_weight_index", 7));
  extra_model->GetModelTaskDefPtr()->add_task()->set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_HCCL));
  root->SetSubgraphInstanceNameToModel("000_extra", extra_model);
  gert::GertModelData data;
  gert::InitGertModelData(data);
  gert::InitGertModelDataUnit(data);
  auto &prepared_unit = *data.models[0];
  prepared_unit.execution_type = gert::GertModelExecutionType::kDynamic;
  ASSERT_EQ(Om2PackageHelper::PrepareDynamicModelData(root, data), SUCCESS);
  ASSERT_EQ(Om2PackageHelper::CompileDynamicProgramBody(artifact_name, root->GetRootGraph()->GetGraphID(),
                                                        std::move(artifacts), *prepared_unit.runtime),
            SUCCESS);
  ASSERT_EQ(Om2PackageHelper::BuildDynamicManifest(data, Om2PackageHelper::GetModelsToBuild(nullptr, root)), SUCCESS);
  ASSERT_EQ(data.models.size(), 1U);
  const auto &unit = *data.models[0];
  ASSERT_NE(unit.runtime, nullptr);
  EXPECT_GT(unit.runtime->so_artifact.data_size, 0U);
  EXPECT_FALSE(unit.runtime->source_artifacts.empty());
  ASSERT_NE(unit.model_meta, nullptr);
  EXPECT_EQ(unit.model_meta->input_desc.size(), compiled->models[0]->model_meta->input_desc.size());
  EXPECT_EQ(unit.model_meta->output_desc.size(), compiled->models[0]->model_meta->output_desc.size());
  ASSERT_NE(unit.constants_config, nullptr);
  ASSERT_EQ(unit.constants_config->consts.size(), 2U);
  // 000_extra 按名称排在其他模型前，但权重编号必须仍取 lowering 写入值。
  EXPECT_EQ(unit.constants_config->consts[0]->index, 7U);
  EXPECT_EQ(unit.constants_config->consts[1]->index, 2U);
  EXPECT_EQ(unit.constants_config->consts[0]->placement, gert::GertModelDataConstPlacement::kHost);
  ASSERT_NE(data.constants, nullptr);
  ASSERT_EQ(data.constants->constants_data.size(), 2U);
  EXPECT_STREQ(gert::GertGetStr(data.constants->constants_data[0]->file_name), "constant_7");
  EXPECT_STREQ(gert::GertGetStr(data.constants->constants_data[1]->file_name), "constant_2");
  ASSERT_NE(data.kernels, nullptr);
  EXPECT_EQ(data.kernels->binaries.size(), compiled->kernels->binaries.size());
  EXPECT_NE(data.custom_ops, nullptr);
  EXPECT_NE(data.so_bins, nullptr);
  EXPECT_NE(data.variables, nullptr);
  EXPECT_NE(unit.debug, nullptr);
  ASSERT_NE(data.manifest, nullptr);
  EXPECT_EQ(data.manifest->model_num, 1U);
  EXPECT_TRUE(data.manifest->compatibility.used_features.empty());
  EXPECT_STREQ(gert::GertGetStr(data.manifest->compatibility.required_executor_version), "");
  EXPECT_EQ(root->GetSubgraphInstanceNameToModel().count(root->GetRootGraph()->GetName()), GetParam() ? 1U : 0U);
}

INSTANTIATE_TEST_SUITE_P(RootGeModelPresentOrAbsent, Om2DynamicPackageHelperUt, testing::Bool());

TEST_F(Om2DynamicGePackageHelperUt, ModelSelectionAndKernelAggregation) {
  auto first_root = CreateGeRootModelWithAicoreOp();
  auto second_root = CreateGeRootModelWithAicoreOp();
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  const auto selected = first_root->GetSubgraphInstanceNameToModel().begin()->second;
  const auto second = second_root->GetSubgraphInstanceNameToModel().begin()->second;
  first_root->SetSubgraphInstanceNameToModel("second", second);

  const auto single = Om2PackageHelper::GetModelsToBuild(selected, first_root);
  ASSERT_EQ(single, std::vector<GeModelPtr>{selected});
  EXPECT_EQ(Om2PackageHelper::GetModelsToBuild(selected, nullptr), single);
  const auto whole = Om2PackageHelper::GetModelsToBuild(nullptr, first_root);
  ASSERT_EQ(whole.size(), 2U);
  gert::GertModelData data;
  gert::InitGertModelData(data);
  for (const auto &model : whole) {
    ASSERT_EQ(Om2PackageHelper::BuildKernelBinaries(model, data), SUCCESS);
  }
  const auto duplicated_count = data.kernels->binaries.size();
  ASSERT_GT(duplicated_count, 0U);
  ASSERT_EQ(Om2PackageHelper::DeduplicateKernelBinaries(data), SUCCESS);
  EXPECT_EQ(data.kernels->binaries.size() * 2U, duplicated_count);
  for (const auto &kernel : data.kernels->binaries) {
    EXPECT_GT(kernel->data_size, 0U);
  }
}

TEST_F(Om2DynamicGePackageHelperUt, KernelAggregationRejectsConflictingContents) {
  const auto make_model = [](const char content) {
    auto model = std::make_shared<GeModel>();
    auto graph = std::make_shared<ComputeGraph>("kernel_graph");
    auto op = std::make_shared<OpDesc>("add", "Add");
    (void)AttrUtils::SetStr(op, "_kernelname", "shared_kernel");
    (void)graph->AddNode(op);
    model->SetGraph(graph);
    model->GetTBEKernelStore().AddKernel(std::make_shared<OpKernelBin>("shared_kernel", std::vector<char>{content}));
    return model;
  };
  const auto first = make_model('a');
  const auto second = make_model('b');
  ASSERT_TRUE(first->GetTBEKernelStore().Build());
  ASSERT_TRUE(second->GetTBEKernelStore().Build());
  gert::GertModelData data;
  gert::InitGertModelData(data);
  ASSERT_EQ(Om2PackageHelper::BuildKernelBinaries(first, data), SUCCESS);
  ASSERT_EQ(Om2PackageHelper::BuildKernelBinaries(second, data), SUCCESS);
  EXPECT_NE(Om2PackageHelper::DeduplicateKernelBinaries(data), SUCCESS);
  ASSERT_EQ(data.kernels->binaries.size(), 2U);
  EXPECT_EQ(data.kernels->binaries[0]->data.get()[0], 'a');
}

}  // namespace ge
