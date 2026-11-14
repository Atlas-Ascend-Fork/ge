/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_ONLINE_TEST_UTILS_H_
#define GE_TESTS_OM2_ONLINE_TEST_UTILS_H_

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
namespace om2_test {
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

inline void EnableOm2OnlineMode() {
  ASSERT_EQ(setenv("ENABLE_RUNTIME_OM2", "1", 1), 0);
}

inline const std::string fake_om2_so_src = R"(
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
  void *var_context;
  void *work_ptr;
  uint64_t *session_id;
  uint64_t model_id;
  void *instance_handle;
  const struct GertModelLoadCallbacks *callbacks;
  int64_t priority;
  uint64_t reuse_zero_copy;
  aclmdlRI external_rt_model = nullptr;
  aclrtStream *external_streams = nullptr;
  uint64_t external_stream_num = 0;
  aclrtEvent *external_events = nullptr;
  uint64_t external_event_num = 0;
  aclrtLabel *external_labels = nullptr;
  uint64_t external_label_num = 0;
  aclrtNotify *external_notifies = nullptr;
  uint64_t external_notify_num = 0;
  void *external_allocator = nullptr;
  const struct GertModelDynamicLoadConfig *dynamic_config = nullptr;
};

struct GertModelRunConfig {
  uint64_t struct_size;
  uint64_t input_count;
  void **input_data;
  uint64_t output_count;
  void **output_data;
  uint64_t stream_sync_timeout_ms;
  const struct GertModelRunCallbacks *run_callbacks = nullptr;
  void *external_allocator = nullptr;
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

inline std::string MakeFakeOm2SoSource() {
  return fake_om2_so_src;
}

inline std::vector<uint8_t> ReadFileBytes(const std::string &path) {
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

inline gert::GertModelData MakeOm2ModelDataWithFakeSo(const std::string &so_path) {
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
}  // namespace om2_test

using om2_test::EnableOm2OnlineMode;
using om2_test::EnvValueGuard;
using om2_test::MakeFakeOm2SoSource;
using om2_test::MakeOm2ModelDataWithFakeSo;

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

  inline static std::string test_work_dir_;
  inline static std::string fake_so_path_;
};

}  // namespace ge
#endif  // GE_TESTS_OM2_ONLINE_TEST_UTILS_H_
