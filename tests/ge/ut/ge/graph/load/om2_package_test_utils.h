/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_PACKAGE_TEST_UTILS_H_
#define GE_TESTS_OM2_PACKAGE_TEST_UTILS_H_

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "common/env_path.h"
#include "common/model/ge_root_model.h"
#include "graph/ge_local_context.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "ge_runtime_stub/include/common/share_graph.h"
#include "ge_runtime_stub/include/faker/ge_model_builder.h"
#include "ge_runtime_stub/include/faker/aicore_taskdef_faker.h"

namespace ge {
namespace om2_test {
inline void SyncKernelNameFromOpDesc(const GeModelPtr &ge_model) {
  auto model_task_def = ge_model->GetModelTaskDefPtr();
  if (model_task_def == nullptr) {
    return;
  }
  const auto &graph = ge_model->GetGraph();
  if (graph == nullptr) {
    return;
  }
  for (int i = 0; i < model_task_def->task_size(); ++i) {
    auto *task_def = model_task_def->mutable_task(i);
    for (const auto &node : graph->GetDirectNode()) {
      auto op_desc = node->GetOpDesc();
      if (op_desc == nullptr) {
        continue;
      }
      std::string kernel_name;
      if (ge::AttrUtils::GetStr(op_desc, "_kernelname", kernel_name)) {
        task_def->mutable_kernel()->set_kernel_name(kernel_name);
      }
    }
  }
}

inline void SyncKernelNameForAllModels(const GeRootModelPtr &ge_root_model) {
  if (ge_root_model == nullptr) {
    return;
  }
  for (const auto &kv : ge_root_model->GetSubgraphInstanceNameToModel()) {
    SyncKernelNameFromOpDesc(kv.second);
  }
}

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

inline GeRootModelPtr CreateGeRootModelWithAicoreOp() {
  auto graph = gert::ShareGraph::AicoreStaticGraph();
  graph->TopologicalSorting();
  gert::GeModelBuilder builder(graph);
  auto ge_root_model =
      builder
          .AddTaskDef("Add",
                      gert::AiCoreTaskDefFaker("add_stub").ArgsFormat("{i_instance0*}{i_instance1*}{o_instance0*}"))
          .FakeTbeBin({"Add"})
          .BuildGeRootModel();
  auto &compute_graph = ge_root_model->GetRootGraph();

  compute_graph->SetGraphUnknownFlag(false);
  for (const auto &node : compute_graph->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    if (op_desc == nullptr) {
      return nullptr;
    }
    if ((op_desc->GetType() == DATA)) {
      op_desc->SetOutputOffset({1024});
    } else if (op_desc->GetType() == NETOUTPUT) {
      op_desc->SetInputOffset({3072});
    } else {
      op_desc->SetInputOffset(std::vector<int64_t>(op_desc->GetInputsSize(), 1024));
      op_desc->SetOutputOffset(std::vector<int64_t>(op_desc->GetOutputsSize(), 1024));
      if (op_desc->GetType() == "Add") {
        op_desc->SetIsInputConst({true, true});
        auto input_desc0 = op_desc->MutableInputDesc(0);
        auto input_desc1 = op_desc->MutableInputDesc(1);
        if ((input_desc0 == nullptr) || (input_desc1 == nullptr)) {
          return nullptr;
        }
        TensorUtils::SetDataOffset(*input_desc0, 0);
        TensorUtils::SetDataOffset(*input_desc1, 200704);
      }
    }
  }

  const auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  std::vector<uint8_t> weights_value(401408, 1U);
  const size_t weight_size = weights_value.size();
  ge_model->SetWeight(Buffer::CopyFrom(weights_value.data(), weight_size));

  (void)AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 2048);
  (void)AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size);
  (void)AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  return ge_root_model;
}

class DynamicPackageTest : public testing::Test {
 protected:
  void SetUp() override {
    const auto name = testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
    const auto work = EnvPath().GetOrCreateCaseTmpPath(name);
    home_ = std::make_unique<ScopedEnvVar>("ASCEND_HOME_PATH", EnvPath().GetAscendInstallPath().c_str());
    work_ = std::make_unique<ScopedEnvVar>("ASCEND_WORK_PATH", work.c_str());
    asan_ = std::make_unique<ScopedEnvVar>("ASAN_OPTIONS", "detect_leaks=0:halt_on_error=0");
    lsan_ = std::make_unique<ScopedEnvVar>("LSAN_OPTIONS", "exitcode=0");
    old_options_ = GetThreadLocalContext().GetAllGraphOptions();
    GetThreadLocalContext().SetGraphOption({{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0'"}});
  }

  void TearDown() override {
    GetThreadLocalContext().SetGraphOption(old_options_);
  }

 private:
  std::unique_ptr<ScopedEnvVar> home_;
  std::unique_ptr<ScopedEnvVar> work_;
  std::unique_ptr<ScopedEnvVar> asan_;
  std::unique_ptr<ScopedEnvVar> lsan_;
  std::map<std::string, std::string> old_options_;
};
}  // namespace om2_test
}  // namespace ge
#endif  // GE_TESTS_OM2_PACKAGE_TEST_UTILS_H_
