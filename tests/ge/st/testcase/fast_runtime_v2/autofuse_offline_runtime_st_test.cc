/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "acl/acl_rt.h"

#include "base/registry/op_impl_space_registry_v2.h"
#include "common/bg_test.h"
#include "common/helper/model_helper.h"
#include "common/model/ge_root_model.h"
#include "common/share_graph.h"
#include "engine/aicore/autofuse_dispatcher/dispatcher_table.h"
#include "faker/aicore_taskdef_faker.h"
#include "faker/ge_model_builder.h"
#include "faker/model_data_faker.h"
#include "framework/runtime/gert_api.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_context.h"
#include "graph/ge_global_options.h"
#include "graph/ge_local_context.h"
#include "graph/utils/graph_utils.h"
#include "kernel/memory/caching_mem_allocator.h"
#include "lowering/model_converter.h"
#include "stub/gert_runtime_stub.h"

namespace ge {
namespace {
struct AddCompileInfoSt {
  int64_t a;
  int64_t b;
};

graphStatus TilingForAddSt(gert::TilingContext *) {
  return GRAPH_SUCCESS;
}

graphStatus TilingParseForAddSt(gert::KernelContext *) {
  return GRAPH_SUCCESS;
}

void *CreateCompileInfoSt() {
  return new AddCompileInfoSt();
}

void DeleteCompileInfoSt(void *const obj) {
  delete reinterpret_cast<AddCompileInfoSt *>(obj);
}

std::string GetHostArchSt() {
#if defined(__aarch64__) || defined(__arm64__)
  return "aarch64";
#else
  return "x86_64";
#endif
}

struct DualBodyStModels {
  GeRootModelPtr main_root;
  GeRootModelPtr fallback_root;
  std::string main_graph_name;
};

GeRootModelPtr BuildSimpleRootSt(const ComputeGraphPtr &graph) {
  return gert::GeModelBuilder(graph)
      .AddTaskDef("Add", gert::AiCoreTaskDefFaker("AddStubBin").WithHandle())
      .FakeTbeBin({"Add"})
      .BuildGeRootModel();
}

DualBodyStModels BuildSimpleDualBodyStModels() {
  DualBodyStModels result;
  auto main_graph = gert::ShareGraph::BuildSingleNodeGraph();
  (void)main_graph->TopologicalSorting();
  result.main_graph_name = main_graph->GetName();
  result.main_root = BuildSimpleRootSt(main_graph);
  auto fallback_graph = std::make_shared<ComputeGraph>(kFallbackModelKey);
  if ((result.main_root == nullptr) ||
      (GraphUtils::CopyComputeGraph(result.main_root->GetRootGraph(), fallback_graph) != SUCCESS)) {
    result.main_root = nullptr;
    return result;
  }
  result.fallback_root = BuildSimpleRootSt(fallback_graph);
  return result;
}

}  // namespace

class AutofuseOfflineRuntimeSt : public gert::bg::BgTest {
 protected:
  void SetUp() override {
    gert::bg::BgTest::SetUp();
    old_global_options_ = GetThreadLocalContext().GetAllGlobalOptions();
    old_graph_options_ = GetThreadLocalContext().GetAllGraphOptions();
    const char *autofuse_env = std::getenv("AUTOFUSE_FLAGS");
    has_autofuse_env_ = (autofuse_env != nullptr);
    if (has_autofuse_env_) {
      autofuse_env_ = autofuse_env;
    }
    const char *opp_env = std::getenv("ASCEND_OPP_PATH");
    has_opp_env_ = (opp_env != nullptr);
    if (has_opp_env_) {
      opp_env_ = opp_env;
    }
    {
      const std::lock_guard<std::mutex> lock(GetGlobalOptionsMutex());
      old_mutable_global_options_ = GetMutableGlobalOptions();
      GetMutableGlobalOptions().erase(DETERMINISTIC);
      GetMutableGlobalOptions().erase(DETERMINISTIC_LEVEL);
    }
    (void)aclrtSetDevice(0);
    (void)setenv("ASCEND_OPP_PATH", "./", 1);
    (void)setenv("GE_USE_STATIC_MEMORY", "3", 1);
    {
      std::ofstream version_info("version.info", std::ios::binary);
      version_info << "Version=3.20.T100.0.B356\n";
    }
    gert::memory::RtsCachingMemAllocator::GetAllocator(0, 1)->Recycle();
    gert::memory::RtsCachingMemAllocator::device_id_to_allocators_.clear();
    runtime_stub_ = std::make_unique<gert::GertRuntimeStub>();
    runtime_stub_->GetKernelStub().StubTiling();
    auto space_registry = std::make_shared<gert::OpImplSpaceRegistryV2>();
    auto add_impl = space_registry->CreateOrGetOpImpl("Add");
    add_impl->tiling = TilingForAddSt;
    add_impl->tiling_parse = TilingParseForAddSt;
    add_impl->compile_info_creator = CreateCompileInfoSt;
    add_impl->compile_info_deleter = DeleteCompileInfoSt;
    original_space_registry_ = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
    gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(space_registry);
  }

  void TearDown() override {
    gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(original_space_registry_);
    runtime_stub_.reset();
    (void)GetThreadLocalContext().SetGlobalOption(old_global_options_);
    (void)GetThreadLocalContext().SetGraphOption(old_graph_options_);
    {
      const std::lock_guard<std::mutex> lock(GetGlobalOptionsMutex());
      GetMutableGlobalOptions() = old_mutable_global_options_;
    }
    if (has_autofuse_env_) {
      (void)setenv("AUTOFUSE_FLAGS", autofuse_env_.c_str(), 1);
    } else {
      (void)unsetenv("AUTOFUSE_FLAGS");
    }
    if (has_opp_env_) {
      (void)setenv("ASCEND_OPP_PATH", opp_env_.c_str(), 1);
    } else {
      (void)unsetenv("ASCEND_OPP_PATH");
    }
    (void)unsetenv("GE_USE_STATIC_MEMORY");
    (void)remove("version.info");
    while (gert::bg::ValueHolder::PopGraphFrame() != nullptr) {
    }
    gert::bg::BgTest::TearDown();
  }

 private:
  std::map<std::string, std::string> old_global_options_;
  std::map<std::string, std::string> old_graph_options_;
  std::map<std::string, std::string> old_mutable_global_options_;
  std::string autofuse_env_;
  std::string opp_env_;
  bool has_autofuse_env_ = false;
  bool has_opp_env_ = false;
  std::unique_ptr<gert::GertRuntimeStub> runtime_stub_;
  std::shared_ptr<gert::OpImplSpaceRegistryV2> original_space_registry_;
};

TEST_F(AutofuseOfflineRuntimeSt, CheckDualBodyLoadExecutorSetWithFallback) {
  auto models = BuildSimpleDualBodyStModels();
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  ASSERT_EQ(
      save_helper.SaveDualBodyRootModel(models.main_root, models.fallback_root, "dual_body_exec", model_buff, true),
      SUCCESS);
  ModelData model_data;
  model_data.model_data = model_buff.data.get();
  model_data.model_len = static_cast<uint32_t>(model_buff.length);
  (void)GetThreadLocalContext().SetGlobalOption({{"ge.host_env_os", "linux"}, {"ge.host_env_cpu", GetHostArchSt()}});
  gert::LoadExecutorArgs args{nullptr, {}};
  gert::ExecutorSet executor_set;
  EXPECT_EQ(gert::LoadExecutorSetFromModelData(model_data, args, executor_set), GRAPH_SUCCESS);
  EXPECT_NE(executor_set.main, nullptr);
  EXPECT_NE(executor_set.fallback, nullptr);
}

TEST_F(AutofuseOfflineRuntimeSt, CheckDualBodyLoadExecutorSetWithoutFallback) {
  auto main_graph = gert::ShareGraph::BuildSingleNodeGraph();
  ASSERT_NE(main_graph, nullptr);
  (void)main_graph->TopologicalSorting();
  auto main_root = BuildSimpleRootSt(main_graph);
  ASSERT_NE(main_root, nullptr);
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  ASSERT_EQ(save_helper.SaveToOmRootModel(main_root, "single_body_exec", model_buff, true), SUCCESS);
  ModelData model_data;
  model_data.model_data = model_buff.data.get();
  model_data.model_len = static_cast<uint32_t>(model_buff.length);
  (void)GetThreadLocalContext().SetGlobalOption({{"ge.host_env_os", "linux"}, {"ge.host_env_cpu", GetHostArchSt()}});
  gert::LoadExecutorArgs args{nullptr, {}};
  gert::ExecutorSet executor_set;
  EXPECT_EQ(gert::LoadExecutorSetFromModelData(model_data, args, executor_set), GRAPH_SUCCESS);
  EXPECT_NE(executor_set.main, nullptr);
  EXPECT_EQ(executor_set.fallback, nullptr);
}

TEST_F(AutofuseOfflineRuntimeSt, CheckGuardDispatcherEmptyAndInvalidSo) {
  ge::autofuse_dispatcher::DispatcherTable table;
  EXPECT_EQ(table.LoadGuardSo(""), SUCCESS);
  std::vector<gert::Tensor *> inputs;
  char_t reason[16] = {};
  EXPECT_FALSE(table.Match(inputs, reason, sizeof(reason)));
  EXPECT_NE(table.LoadGuardSo("not a valid so"), SUCCESS);
  EXPECT_FALSE(table.Match(inputs, reason, sizeof(reason)));
}

TEST_F(AutofuseOfflineRuntimeSt, CheckDualBodyLoadExecutorSetWithInvalidGuard) {
  auto models = BuildSimpleDualBodyStModels();
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  (void)AttrUtils::SetStr(models.main_root->GetRootGraph(), "_guard_check_so_data", "invalid_guard_so");
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  ASSERT_EQ(save_helper.SaveDualBodyRootModel(models.main_root, models.fallback_root, "dual_body_bad_guard", model_buff,
                                              true),
            SUCCESS);
  ModelData model_data;
  model_data.model_data = model_buff.data.get();
  model_data.model_len = static_cast<uint32_t>(model_buff.length);
  gert::LoadExecutorArgs args{nullptr, {}};
  gert::ExecutorSet executor_set;
  EXPECT_EQ(gert::LoadExecutorSetFromModelData(model_data, args, executor_set), GRAPH_SUCCESS);
  EXPECT_NE(executor_set.main, nullptr);
  EXPECT_NE(executor_set.fallback, nullptr);
  EXPECT_EQ(executor_set.guard, nullptr);
}

TEST_F(AutofuseOfflineRuntimeSt, CheckGuardDispatcherMissingSymbol) {
  const std::vector<std::string> candidates = {"/lib/x86_64-linux-gnu/libm.so.6", "/lib64/libm.so.6",
                                               "/usr/lib/x86_64-linux-gnu/libm.so.6"};
  std::string so_data;
  for (const auto &path : candidates) {
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.is_open()) {
      so_data.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      break;
    }
  }
  if (so_data.empty()) {
    GTEST_SKIP();
  }
  ge::autofuse_dispatcher::DispatcherTable table;
  EXPECT_NE(table.LoadGuardSo(so_data), SUCCESS);
}

class DummyGuardCheckerSt : public gert::GuardChecker {
 public:
  bool Match(const std::vector<gert::Tensor *> &, char_t *, size_t) const override {
    return false;
  }
};

TEST_F(AutofuseOfflineRuntimeSt, CheckGuardCheckerVirtualDestructor) {
  auto guard = std::make_shared<DummyGuardCheckerSt>();
  EXPECT_FALSE(guard->Match({}, nullptr, 0U));
  guard.reset();
}

TEST_F(AutofuseOfflineRuntimeSt, CheckDualBodyLoadExecutorSetWithGuard) {
  auto models = BuildSimpleDualBodyStModels();
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  std::ifstream ifs(GUARD_CHECK_ST_STUB_PATH, std::ios::binary);
  ASSERT_TRUE(ifs.is_open());
  const std::string guard_so((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ASSERT_FALSE(guard_so.empty());
  (void)AttrUtils::SetStr(models.main_root->GetRootGraph(), "_guard_check_so_data", guard_so);
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  ASSERT_EQ(
      save_helper.SaveDualBodyRootModel(models.main_root, models.fallback_root, "dual_body_guard", model_buff, true),
      SUCCESS);
  ModelData model_data;
  model_data.model_data = model_buff.data.get();
  model_data.model_len = static_cast<uint32_t>(model_buff.length);
  gert::LoadExecutorArgs args{nullptr, {}};
  gert::ExecutorSet executor_set;
  EXPECT_EQ(gert::LoadExecutorSetFromModelData(model_data, args, executor_set), GRAPH_SUCCESS);
  ASSERT_NE(executor_set.guard, nullptr);
  char_t reason[16] = {};
  EXPECT_TRUE(executor_set.guard->Match({}, reason, sizeof(reason)));
}

TEST_F(AutofuseOfflineRuntimeSt, CheckConvertFallbackWithoutFallback) {
  auto main_graph = gert::ShareGraph::BuildSingleNodeGraph();
  ASSERT_NE(main_graph, nullptr);
  (void)main_graph->TopologicalSorting();
  auto main_root = BuildSimpleRootSt(main_graph);
  ASSERT_NE(main_root, nullptr);
  std::map<std::string, GeModelPtr> fallback_models;
  GeRootModelPtr fallback_root = nullptr;
  EXPECT_EQ(gert::ModelConverter().ConvertFallbackModelToExecuteGraph(main_root, fallback_models, {}, fallback_root),
            nullptr);
  EXPECT_EQ(fallback_root, nullptr);
}

TEST_F(AutofuseOfflineRuntimeSt, CheckLoadExecutorSetInvalidData) {
  gert::LoadExecutorArgs args{nullptr, {}};
  gert::ExecutorSet executor_set;
  ModelData invalid_data{};
  EXPECT_NE(gert::LoadExecutorSetFromModelData(invalid_data, args, executor_set), GRAPH_SUCCESS);
  EXPECT_EQ(executor_set.main, nullptr);
}

TEST_F(AutofuseOfflineRuntimeSt, CheckLoadSingleExecutorFromDualBody) {
  auto models = BuildSimpleDualBodyStModels();
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  ASSERT_EQ(save_helper.SaveDualBodyRootModel(models.main_root, models.fallback_root, "dual_body_single_exec",
                                              model_buff, true),
            SUCCESS);
  ModelData model_data;
  model_data.model_data = model_buff.data.get();
  model_data.model_len = static_cast<uint32_t>(model_buff.length);
  gert::LoadExecutorArgs args{nullptr, {}};
  ge::graphStatus error_code = GRAPH_FAILED;
  auto executor = gert::LoadExecutorFromModelData(model_data, args, error_code);
  EXPECT_EQ(error_code, GRAPH_SUCCESS);
  EXPECT_NE(executor, nullptr);
}

}  // namespace ge
