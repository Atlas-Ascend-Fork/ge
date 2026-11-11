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

namespace ge {
namespace {

class ScopedEnvVar {
 public:
  ScopedEnvVar(const char *name, const char *value) : name_(name) {
    const char *old_value = getenv(name);
    if (old_value != nullptr) {
      old_value_ = old_value;
      has_old_value_ = true;
    }
    (void)setenv(name, value, 1);
  }

  ~ScopedEnvVar() {
    if (has_old_value_) {
      (void)setenv(name_.c_str(), old_value_.c_str(), 1);
      return;
    }
    (void)unsetenv(name_.c_str());
  }

 private:
  std::string name_;
  std::string old_value_;
  bool has_old_value_ = false;
};

GeRootModelPtr CreateSimpleGeRootModel() {
  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  auto data_op = std::make_shared<OpDesc>("data1", "Data");
  data_op->AddInputDesc(GeTensorDesc(GeShape({2, 16}), FORMAT_ND, DT_FLOAT));
  data_op->AddOutputDesc(GeTensorDesc(GeShape({2, 16}), FORMAT_ND, DT_FLOAT));
  auto data_node = compute_graph->AddNode(data_op);

  auto output_op = std::make_shared<OpDesc>("output1", "NetOutput");
  output_op->AddInputDesc(GeTensorDesc(GeShape({2, 16}), FORMAT_ND, DT_FLOAT));
  auto output_node = compute_graph->AddNode(output_op);

  (void)ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));

  auto ge_root_model = std::make_shared<GeRootModel>();
  (void)ge_root_model->Initialize(compute_graph);
  auto ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);
  return ge_root_model;
}
}  // namespace

class Om2OnlineSessionTest : public testing::Test {
 protected:
  void SetUp() override {
    test_work_dir_ = EnvPath().GetOrCreateCaseTmpPath("Om2OnlineSessionTest");
    setenv("ASCEND_WORK_PATH", test_work_dir_.c_str(), 1);
    const auto ascend_install_path = EnvPath().GetAscendInstallPath();
    setenv("ASCEND_HOME_PATH", ascend_install_path.c_str(), 1);
  }

  void TearDown() override {
    unsetenv("ASCEND_WORK_PATH");
    unsetenv("ASCEND_HOME_PATH");
    unsetenv("ENABLE_RUNTIME_OM2");
    std::error_code ec;
    (void)std::filesystem::remove_all(test_work_dir_, ec);
  }

  std::string test_work_dir_;
};

// Test: OM2 online mode detection via env var
TEST_F(Om2OnlineSessionTest, Om2ModeDetection) {
  unsetenv("ENABLE_RUNTIME_OM2");
  EXPECT_FALSE(IsOm2OnlineMode());

  setenv("ENABLE_RUNTIME_OM2", "1", 1);
  EXPECT_TRUE(IsOm2OnlineMode());

  unsetenv("ENABLE_RUNTIME_OM2");
  EXPECT_FALSE(IsOm2OnlineMode());
}

// Test: GeRootModel OM2 data lifecycle
TEST_F(Om2OnlineSessionTest, GeRootModel_Om2DataLifecycle) {
  auto ge_root_model = CreateSimpleGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);

  // Initially no OM2 data
  EXPECT_EQ(ge_root_model->GetOm2ModelData(), nullptr);

  // Set OM2 data
  auto om2_data = std::make_shared<gert::GertModelData>();
  gert::InitGertModelData(*om2_data);
  om2_data->models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  om2_data->constants->constants_data.emplace_back();
  om2_data->models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  om2_data->models[0]->model_meta->model_name = gert::GertMakeStr("test_model");
  om2_data->models[0]->model_meta->work_size = 2048U;
  ge_root_model->SetOm2ModelData(om2_data);

  EXPECT_NE(ge_root_model->GetOm2ModelData(), nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(ge_root_model->GetOm2ModelData()->models[0]->model_meta->model_name)),
            "test_model");
  EXPECT_EQ(ge_root_model->GetOm2ModelData()->models[0]->model_meta->work_size, 2048U);

  // Clear OM2 data
  ge_root_model->SetOm2ModelData(nullptr);
  EXPECT_EQ(ge_root_model->GetOm2ModelData(), nullptr);
}

// Test: GertModelData structure integrity
TEST_F(Om2OnlineSessionTest, Om2ModelData_StructureIntegrity) {
  gert::GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->runtime = std::make_unique<gert::GertModelDataRuntime>();
  model_data.models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  model_data.models[0]->constants_config = std::make_unique<gert::GertModelDataConstantsConfig>();
  model_data.constants->constants_data[0] = std::make_unique<gert::GertModelDataFile>();

  model_data.models[0]->runtime->so_artifact.file_name = gert::GertMakeStr("libtest.so");
  const std::string so_bytes(
      "\x7f"
      "ELF",
      4);
  model_data.models[0]->runtime->so_artifact.data = gert::GertMakeFileData(so_bytes.data(), so_bytes.size());
  model_data.models[0]->runtime->so_artifact.data_size = 4U;

  // Populate model meta
  model_data.models[0]->model_meta->model_name = gert::GertMakeStr("test_model");
  model_data.models[0]->model_meta->work_size = 4096U;

  gert::GertModelDataFile kernel;
  kernel.file_name = gert::GertMakeStr("test_kernel");
  auto kbuf = std::make_unique<uint8_t[]>(3);
  kbuf[0] = 0x01;
  kbuf[1] = 0x02;
  kbuf[2] = 0x03;
  kernel.data = ge::ReadonlyByteBuffer(kbuf.release(), ge::ConditionalDeleter{true});
  kernel.data_size = 3U;
  model_data.kernels->binaries.push_back(std::make_unique<gert::GertModelDataFile>(std::move(kernel)));

  auto wbuf = std::make_unique<uint8_t[]>(4);
  wbuf[0] = 0xAA;
  wbuf[1] = 0xBB;
  wbuf[2] = 0xCC;
  wbuf[3] = 0xDD;
  model_data.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(wbuf.release(), ge::ConditionalDeleter{true});
  model_data.constants->constants_data[0]->data_size = 4U;
  model_data.models[0]->constants_config->internal_weight_size = 4U;

  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->runtime->so_artifact.file_name)), "libtest.so");
  EXPECT_EQ(model_data.models[0]->runtime->so_artifact.data_size, 4U);
  EXPECT_EQ(model_data.models[0]->model_meta->work_size, 4096U);
  EXPECT_EQ(model_data.kernels->binaries.size(), 1U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.kernels->binaries[0]->file_name)), "test_kernel");
  EXPECT_EQ(model_data.constants->constants_data[0]->data_size, 4U);
  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 4U);
}

// Test: Multiple GeRootModel instances with OM2 data
TEST_F(Om2OnlineSessionTest, MultipleGeRootModels_IndependentOm2Data) {
  auto model1 = CreateSimpleGeRootModel();
  auto model2 = CreateSimpleGeRootModel();
  ASSERT_NE(model1, nullptr);
  ASSERT_NE(model2, nullptr);

  auto data1 = std::make_shared<gert::GertModelData>();
  gert::InitGertModelData(*data1);
  data1->models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  data1->constants->constants_data.emplace_back();
  data1->models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  data1->models[0]->model_meta->model_name = gert::GertMakeStr("model_1");
  model1->SetOm2ModelData(data1);

  auto data2 = std::make_shared<gert::GertModelData>();
  gert::InitGertModelData(*data2);
  data2->models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  data2->constants->constants_data.emplace_back();
  data2->models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  data2->models[0]->model_meta->model_name = gert::GertMakeStr("model_2");
  model2->SetOm2ModelData(data2);

  EXPECT_NE(model1->GetOm2ModelData(), nullptr);
  EXPECT_NE(model2->GetOm2ModelData(), nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(model1->GetOm2ModelData()->models[0]->model_meta->model_name)), "model_1");
  EXPECT_EQ(std::string(gert::GertGetStr(model2->GetOm2ModelData()->models[0]->model_meta->model_name)), "model_2");

  // Clearing one doesn't affect the other
  model1->SetOm2ModelData(nullptr);
  EXPECT_EQ(model1->GetOm2ModelData(), nullptr);
  EXPECT_NE(model2->GetOm2ModelData(), nullptr);
}

// Test: GertModelData shared_ptr semantics (Fork scenario)
TEST_F(Om2OnlineSessionTest, Om2ModelData_SharedPtrFork) {
  auto ge_root_model = CreateSimpleGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);

  auto om2_data = std::make_shared<gert::GertModelData>();
  gert::InitGertModelData(*om2_data);
  om2_data->models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  om2_data->constants->constants_data.emplace_back();
  om2_data->models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  om2_data->models[0]->constants_config = std::make_unique<gert::GertModelDataConstantsConfig>();
  om2_data->constants->constants_data[0] = std::make_unique<gert::GertModelDataFile>();
  om2_data->models[0]->model_meta->model_name = gert::GertMakeStr("shared_model");
  auto wbuf2 = std::make_unique<uint8_t[]>(3);
  wbuf2[0] = 0x01;
  wbuf2[1] = 0x02;
  wbuf2[2] = 0x03;
  om2_data->constants->constants_data[0]->data = ge::ReadonlyByteBuffer(wbuf2.release(), ge::ConditionalDeleter{true});
  om2_data->constants->constants_data[0]->data_size = 3U;
  om2_data->models[0]->constants_config->internal_weight_size = 3U;
  ge_root_model->SetOm2ModelData(om2_data);

  auto forked_data = om2_data;

  EXPECT_NE(ge_root_model->GetOm2ModelData(), nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(forked_data->models[0]->model_meta->model_name)), "shared_model");
  EXPECT_EQ(forked_data->constants->constants_data[0]->data_size, 3U);
  EXPECT_EQ(forked_data->models[0]->constants_config->internal_weight_size, 3U);

  // Both point to the same data
  EXPECT_EQ(ge_root_model->GetOm2ModelData().get(), forked_data.get());
}

namespace {
class EnvValueGuard {
 public:
  explicit EnvValueGuard(const char *name) : name_(name) {
    const char *value = std::getenv(name_.c_str());
    if (value != nullptr) {
      old_value_ = value;
      had_value_ = true;
    }
  }

  ~EnvValueGuard() {
    if (had_value_) {
      (void)setenv(name_.c_str(), old_value_.c_str(), 1);
    } else {
      (void)unsetenv(name_.c_str());
    }
  }

 private:
  std::string name_;
  std::string old_value_;
  bool had_value_ = false;
};

void EnableOm2OnlineMode() {
  ASSERT_EQ(setenv("ENABLE_RUNTIME_OM2", "1", 1), 0);
}

static const std::string fake_om2_so_src = R"(
#include <cstddef>
#include <cstdint>

typedef void* aclmdlRI;
typedef void* aclrtStream;
typedef void* aclrtEvent;
typedef void* aclrtLabel;
typedef void* aclrtNotify;

struct GertModelLoadConfig {
  uint64_t struct_size;
  const char **bin_files;
  const void **bin_data;
  uint64_t *bin_size;
  uint64_t bin_num;
  void **constants;
  void **var_addrs;
  void *work_ptr;
  uint64_t *session_id;
  uint64_t model_id;
  void *instance_handle;
  void *executor_handle;
  const struct GertModelLoadCallbacks *callbacks;
  int64_t priority;
  aclmdlRI external_rt_model = nullptr;
  aclrtStream *external_streams = nullptr;
  uint64_t external_stream_num = 0;
  aclrtEvent *external_events = nullptr;
  uint64_t external_event_num = 0;
  aclrtLabel *external_labels = nullptr;
  uint64_t external_label_num = 0;
  aclrtNotify *external_notifies = nullptr;
  uint64_t external_notify_num = 0;
};

struct GertModelRunConfig {
  uint64_t struct_size;
  uint64_t input_count;
  void **input_data;
  uint64_t output_count;
  void **output_data;
  uint64_t stream_sync_timeout_ms;
  const struct GertModelRunCallbacks *run_callbacks = nullptr;
};

struct GertModelUnloadConfig {
  uint64_t struct_size;
};

struct GertModelLoadOutput {
  uint64_t struct_size;
};

struct GertModelRunOutput {
  uint64_t struct_size;
};

struct GertModelUnloadOutput {
  uint64_t struct_size;
};

extern "C" {
int GertModelLoad(const struct GertModelLoadConfig *config, void **model_handle, struct GertModelLoadOutput *output) {
  if (model_handle) *model_handle = (void *)0x1;
  return 0;
}

int GertModelRunAsync(void *model_handle, void *stream, const struct GertModelRunConfig *config, struct GertModelRunOutput *output) {
  return 0;
}

int GertModelRun(void *model_handle, const struct GertModelRunConfig *config, struct GertModelRunOutput *output) {
  return 0;
}

int GertModelUnload(void *model_handle, const struct GertModelUnloadConfig *config, struct GertModelUnloadOutput *output) { return 0; }
}
)";

std::string MakeFakeOm2SoSource() {
  return fake_om2_so_src;
}

std::vector<uint8_t> ReadFileBytes(const std::string &path) {
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if (!ifs.is_open()) {
    return {};
  }
  const auto size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::vector<uint8_t> data(static_cast<size_t>(size));
  ifs.read(reinterpret_cast<char *>(data.data()), size);
  return data;
}

gert::GertModelData MakeOm2ModelDataWithFakeSo(const std::string &so_path) {
  gert::GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  model_data.models[0]->runtime = std::make_unique<gert::GertModelDataRuntime>();
  model_data.constants->constants_data[0] = std::make_unique<gert::GertModelDataFile>();
  model_data.models[0]->constants_config = std::make_unique<gert::GertModelDataConstantsConfig>();
  model_data.models[0]->debug = std::make_unique<gert::GertModelDataDebug>();
  model_data.manifest = std::make_unique<gert::GertModelDataManifest>();
  model_data.models[0]->model_meta->model_name = gert::GertMakeStr("st_om2_test_model");
  model_data.models[0]->model_meta->work_size = 1024U;

  gert::GertTensorDesc input_desc;
  input_desc.name = gert::GertMakeStr("input");
  input_desc.data_type = ge::DT_FLOAT;
  input_desc.shape = {1, 4};
  input_desc.size = 16U;
  model_data.models[0]->model_meta->input_desc.push_back(std::move(input_desc));

  gert::GertTensorDesc input_desc_v2;
  input_desc_v2.name = gert::GertMakeStr("input");
  input_desc_v2.data_type = ge::DT_FLOAT;
  input_desc_v2.shape = {1, 4};
  input_desc_v2.size = 16U;
  model_data.models[0]->model_meta->input_desc_v2.push_back(std::move(input_desc_v2));

  gert::GertTensorDesc output_desc;
  output_desc.name = gert::GertMakeStr("output");
  output_desc.data_type = ge::DT_FLOAT;
  output_desc.shape = {1, 4};
  output_desc.size = 16U;
  model_data.models[0]->model_meta->output_desc.push_back(std::move(output_desc));

  gert::GertTensorDesc output_desc_v2;
  output_desc_v2.name = gert::GertMakeStr("output");
  output_desc_v2.data_type = ge::DT_FLOAT;
  output_desc_v2.shape = {1, 4};
  output_desc_v2.size = 16U;
  model_data.models[0]->model_meta->output_desc_v2.push_back(std::move(output_desc_v2));

  auto so_bytes = ReadFileBytes(so_path);
  model_data.models[0]->runtime->so_artifact.file_name = gert::GertMakeStr("libst_test_model_om2.so");
  model_data.models[0]->runtime->so_artifact.data = gert::GertMakeFileData(so_bytes.data(), so_bytes.size());
  model_data.models[0]->runtime->so_artifact.data_size = so_bytes.size();

  return model_data;
}
}  // namespace

class Om2OnlineModelExecutorTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    test_work_dir_ = EnvPath().GetOrCreateCaseTmpPath("Om2OnlineModelExecutorTest");
    setenv("ASCEND_WORK_PATH", test_work_dir_.c_str(), 1);

    const std::string src_path = PathUtils::Join({test_work_dir_, "fake_om2.cpp"});
    fake_so_path_ = PathUtils::Join({test_work_dir_, "libfake_om2.so"});
    std::ofstream ofs(src_path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(ofs.is_open());
    ofs << MakeFakeOm2SoSource();
    ofs.close();
    const std::string compile_cmd = "ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=detect_leaks=0 g++ -shared -fPIC -o " +
                                    fake_so_path_ + " " + src_path;
    ASSERT_EQ(std::system(compile_cmd.c_str()), 0);
    ASSERT_EQ(mmAccess2(fake_so_path_.c_str(), M_F_OK), EOK);
  }

  static void TearDownTestSuite() {
    unsetenv("ASCEND_WORK_PATH");
    EnvPath().RemoveRfCaseTmpPath("Om2OnlineModelExecutorTest");
  }

  void TearDown() override {
    unsetenv("ENABLE_RUNTIME_OM2");
  }

  static std::string test_work_dir_;
  static std::string fake_so_path_;
};

std::string Om2OnlineModelExecutorTest::test_work_dir_;
std::string Om2OnlineModelExecutorTest::fake_so_path_;

TEST_F(Om2OnlineModelExecutorTest, LoadAndUnload_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5001;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunGraph_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5002;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);

  std::vector<uint8_t> data(16U, 0U);
  gert::Tensor input;
  input.SetPlacement(gert::kOnDeviceHbm);
  input.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  gert::Tensor output;
  output.SetPlacement(gert::kOnDeviceHbm);
  output.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  std::vector<gert::Tensor> inputs;
  inputs.push_back(std::move(input));
  std::vector<gert::Tensor> outputs;
  outputs.push_back(std::move(output));
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);

  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunGraph_EmptyOutputs_PrepareOm2Outputs) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5003;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);

  std::vector<uint8_t> data(16U, 0U);
  gert::Tensor input;
  input.SetPlacement(gert::kOnDeviceHbm);
  input.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  std::vector<gert::Tensor> inputs;
  inputs.push_back(std::move(input));
  std::vector<gert::Tensor> outputs;
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);
  EXPECT_EQ(outputs.size(), 1U);

  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunGraph_HostInput_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5004;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);

  gert::Tensor host_input;
  host_input.SetPlacement(gert::kOnHost);
  std::vector<uint8_t> data(16U, 0U);
  host_input.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnHost));

  gert::Tensor output;
  output.SetPlacement(gert::kOnDeviceHbm);
  output.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  std::vector<gert::Tensor> inputs;
  inputs.push_back(std::move(host_input));
  std::vector<gert::Tensor> outputs;
  outputs.push_back(std::move(output));
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);

  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunGraph_NotLoaded_ReturnsGraphNotExist) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  GraphId graph_id = 5005;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  std::vector<gert::Tensor> inputs(1);
  std::vector<gert::Tensor> outputs(1);
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), GE_GRAPH_GRAPH_NOT_EXIST);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, UnloadGraph_NotLoaded_ReturnsSuccess) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GraphId graph_id = 5006;
  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, DumpDebugJSONPrint_ReturnsUnsupported) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  AscendString json_result;
  EXPECT_EQ(model_executor.DumpDebugJSONPrint(1U, 1U, 0U, json_result), GE_GRAPH_UNSUPPORTED);
}

TEST_F(Om2OnlineModelExecutorTest, PaRemapped_ReturnsUnsupported) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(1U);
  graph_node->SetGeRootModel(std::make_shared<GeRootModel>());

  std::vector<std::pair<uint64_t, uint64_t>> cross_ranges;
  EXPECT_EQ(model_executor.PaRemapped(graph_node, 0U, 0U, 0U, cross_ranges), GE_GRAPH_UNSUPPORTED);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunGraphWithStream_Om2Mode_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5007;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);
  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);

  GeTensorDesc input_desc(GeShape({1, 4}), FORMAT_ND, DT_FLOAT);
  input_desc.SetPlacement(kPlacementDevice);
  GeTensorDesc output_desc(GeShape({1, 4}), FORMAT_ND, DT_FLOAT);
  output_desc.SetPlacement(kPlacementDevice);
  std::vector<uint8_t> input_data(16U, 0U);
  std::vector<uint8_t> output_data(16U, 0U);
  std::vector<GeTensor> inputs{GeTensor(input_desc, input_data.data(), input_data.size())};
  std::vector<GeTensor> outputs{GeTensor(output_desc, output_data.data(), output_data.size())};

  EXPECT_EQ(model_executor.RunGraphWithStream(graph_node, graph_id, nullptr, inputs, outputs), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, ExecuteGraphWithStream_Om2Mode_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5008;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);
  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);

  std::vector<uint8_t> data(16U, 0U);
  gert::Tensor input;
  input.SetPlacement(gert::kOnDeviceHbm);
  input.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  gert::Tensor output;
  output.SetPlacement(gert::kOnDeviceHbm);
  output.SetData(gert::TensorData(data.data(), nullptr, data.size(), gert::kOnDeviceHbm));

  std::vector<gert::Tensor> inputs;
  inputs.push_back(std::move(input));
  std::vector<gert::Tensor> outputs;
  outputs.push_back(std::move(output));

  EXPECT_EQ(model_executor.ExecuteGraphWithStream(graph_node, graph_id, nullptr, inputs, outputs), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, RunThread_Om2Mode_ReturnsUnsupported) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  model_executor.StartRunThread();

  GraphId graph_id = 5009;
  error_message::ErrorManagerContext error_context;
  GEThreadLocalContext context = GetThreadLocalContext();

  std::promise<Status> status_promise;
  auto status_future = status_promise.get_future();
  const auto callback = [&status_promise](Status status, std::vector<gert::Tensor> &outputs) {
    status_promise.set_value(status);
  };

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->IncreaseLoadCount();
  graph_node->Lock();

  std::vector<gert::Tensor> input_tensors(1);

  auto run_args = std::make_shared<RunArgs>();
  ASSERT_TRUE(run_args != nullptr);
  run_args->graph_node = graph_node;
  run_args->graph_id = graph_id;
  run_args->session_id = 0;
  run_args->error_context = error_context;
  run_args->input_tensor = std::move(input_tensors);
  run_args->context = context;
  run_args->callback = callback;
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  auto status = status_future.wait_for(std::chrono::seconds(5));
  ASSERT_EQ(status, std::future_status::ready);
  EXPECT_EQ(status_future.get(), GE_GRAPH_UNSUPPORTED);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, LoadGraph_ExternalConstAndFeatureMemory) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = std::make_shared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(compute_graph), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(compute_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 5010;
  GraphNodePtr graph_node = std::make_shared<ge::GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);

  std::vector<uint8_t> const_mem(16U, 0U);
  std::vector<uint8_t> feature_mem(1024U, 0U);
  graph_node->SetConstMemoryBase(const_mem.data(), const_mem.size());
  graph_node->SetFeatureMemoryBase(feature_mem.data(), feature_mem.size());

  auto model_data = std::make_shared<gert::GertModelData>(MakeOm2ModelDataWithFakeSo(fake_so_path_));
  ge_root_model->SetOm2ModelData(model_data);

  EXPECT_EQ(model_executor.LoadGraph(ge_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(ge_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(Om2OnlineModelExecutorTest, GetCompiledModel_Om2Mode_Success) {
  EnvValueGuard guard("ENABLE_RUNTIME_OM2");
  EnableOm2OnlineMode();

  GraphManager graph_manager;
  const GraphId graph_id = 5012;
  const auto ge_root_model = CreateSimpleGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);

  auto graph_node = std::make_shared<GraphNode>(graph_id);
  graph_node->SetGeRootModel(ge_root_model);
  graph_node->SetBuildFlag(true);
  graph_manager.AddGraphNode(graph_id, graph_node);

  const auto om2_model_data = std::make_shared<gert::GertModelData>();
  gert::InitGertModelData(*om2_model_data);
  om2_model_data->models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  om2_model_data->constants->constants_data.emplace_back();
  om2_model_data->models[0]->model_meta = std::make_unique<gert::GertModelDataModelMeta>();
  om2_model_data->models[0]->constants_config = std::make_unique<gert::GertModelDataConstantsConfig>();
  om2_model_data->models[0]->debug = std::make_unique<gert::GertModelDataDebug>();
  om2_model_data->models[0]->model_meta->model_name = gert::GertMakeStr("om2_st_model");
  om2_model_data->models[0]->debug->visual_json =
      gert::GertMakeStr(R"({"format":"ge_visual_json","format_version":1})");
  ge_root_model->SetOm2ModelData(om2_model_data);

  ModelBufferData model_buffer;
  EXPECT_EQ(graph_manager.GetCompiledModel(graph_id, model_buffer), SUCCESS);
  ASSERT_NE(model_buffer.data, nullptr);
  EXPECT_GT(model_buffer.length, 0U);
}

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

/**
 * 用例描述：测试创建 Session 时 options 中 ge.exec.disableReuseMemory 为非法值的参数校验
 * 预置条件：
 *   1. GEInitialize 已成功执行
 * 测试步骤：
 *   1. 构造 session options，注入 ge.exec.disableReuseMemory=100（合法值仅为 0/1）
 *   2. 构造 ge::Session 对象
 * 预期结果：
 *   1. Session 创建失败，GetSessionId 保持 0，并上报 disableReuseMemory 非法错误码
 * 注意：本用例置于本文件末尾——Session 初始化失败路径会在全局留下状态残留
 *      （InnerSession::Initialize 在参数校验失败前已创建 UserGraphsManager 等组件），
 *      可能影响同进程后续用例，故必须作为本 target 的最后一个用例执行。
 */
TEST(SessionOptionInvalidTest, disable_reuse_memory_invalid) {
  std::map<std::string, std::string> options;
  options[ge::OPTION_EXEC_DEVICE_ID] = "0";
  EXPECT_EQ(ge::GEInitialize(options), ge::SUCCESS);

  std::map<std::string, std::string> session_options;
  session_options["ge.exec.disableReuseMemory"] = "100";
  ge::Session session(session_options);
  EXPECT_EQ(session.GetSessionId(), 0U);
  EXPECT_EQ(ge::GEFinalize(), ge::SUCCESS);
}

}  // namespace ge
