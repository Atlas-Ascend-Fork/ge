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
#include "securec.h"

#include <array>
#include <cstdint>
#include <memory>
#include <cstring>
#include <fstream>
#include <sys/mman.h>
#include <unistd.h>
#include <string>
#include <set>

#include "common/helper/om2/om2_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/program_body_codegen.h"
#include "base/registry/op_impl_space_registry_v2.h"
#include "exe_graph/lowering/buffer_pool.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"
#include "depends/ascendcl/src/ascendcl_stub.h"
#include "../../graph/load/om2_package_test_utils.h"
#include "../../../../common/om2_generated_model_runner.h"

#include "program_body_codegen_fixture.h"
#include "generated_so_fixture.h"

namespace gert {
namespace om2 {
namespace {
using program_body_fixture::LifecycleProgram;
using program_body_fixture::PartialInitFailureProbe;
using program_body_fixture::RegistryBindingProbe;
using program_body_fixture::RegistryBindingProgram;

// Observe only the borrowed handle; leave all other RTS behavior to the existing stub.
class BorrowedStreamObserver final : public ge::AclRuntimeStub {
 public:
  explicit BorrowedStreamObserver(const aclrtStream borrowed)
      : borrowed_(borrowed), previous_(ge::AclRuntimeStub::GetInstance()) {
    ge::AclRuntimeStub::Install(this);
  }
  ~BorrowedStreamObserver() override {
    ge::AclRuntimeStub::Install(previous_);
  }
  aclError aclrtDestroyStream(aclrtStream stream) override {
    if (stream == borrowed_) {
      ++borrowed_destroy_count;
      return ACL_SUCCESS;
    }
    return ge::AclRuntimeStub::aclrtDestroyStream(stream);
  }
  aclError aclrtDestroyStreamForce(aclrtStream stream) override {
    if (stream == borrowed_) {
      ++borrowed_destroy_count;
      return ACL_SUCCESS;
    }
    return ge::AclRuntimeStub::aclrtDestroyStreamForce(stream);
  }
  size_t borrowed_destroy_count = 0U;

 private:
  aclrtStream borrowed_;
  ge::AclRuntimeStub *previous_;
};

using ProgramBodyCodegenUT = ge::om2_test::DynamicPackageTest;

// P-01/02/03: compile every real generated artifact, resolve the C ABI eagerly,
// and exercise one loaded model across four runs and one unload.
TEST_F(ProgramBodyCodegenUT, GeneratedSoPreservesLifecycleAndRebindsEachRunAllocator) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  ProgramBodyCodegenOption option;
  option.model_name = "lifecycle_probe";
  option.interface_file_name = "lifecycle_probe.h";
  option.source_file_name = "lifecycle_probe.cpp";
  option.makefile_file_name = "Makefile";
  ProgramBodyCodegenResult generated;
  ASSERT_EQ(ProgramBodyCodegen().Generate(LifecycleProgram(), option, generated), ge::SUCCESS);
  const std::set<std::string> expected_files{
      "lifecycle_probe.h",     "lifecycle_probe.cpp",       "generated_program_assets.h", "om2_model_api.h",
      "om2_model_api_utils.h", "om2_runtime_value_types.h", "register/kernel_registry.h", "Makefile"};
  std::set<std::string> actual_files;
  for (const auto &artifact : generated.artifacts) {
    ASSERT_NE(artifact.file_name, nullptr);
    EXPECT_TRUE(actual_files.emplace(artifact.file_name.get()).second);
  }
  EXPECT_EQ(actual_files, expected_files);
  gert::GertModelDataFile so;
  ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, so), ge::SUCCESS);
  ASSERT_GT(so.data_size, 0U);
  const auto artifact_directory = ge::EnvPath().GetOrCreateCaseTmpPath("ProgramBodyCodegenUT");
  std::ofstream saved_so(artifact_directory + "/lifecycle_probe.so", std::ios::binary);
  saved_so.write(reinterpret_cast<const char *>(so.data.get()), static_cast<std::streamsize>(so.data_size));
  ASSERT_TRUE(saved_so.good());
  saved_so.close();
  std::array<uintptr_t, 10U> events{};
  int load_allocator = 0;
  int allocator_a = 0;
  int allocator_b = 0;
  // The event storage and borrowed allocator identities outlive Unload and the runner.
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(so)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = events.data();
  load.external_allocator = &load_allocator;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  EXPECT_EQ(events[0], 1U);
  EXPECT_EQ(events[3], reinterpret_cast<uintptr_t>(&load_allocator));
  GertModelRunConfig run;
  run.external_allocator = &allocator_a;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  run.external_allocator = nullptr;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  run.external_allocator = &allocator_b;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  // Put the actual old prefix immediately before an inaccessible page. Reading the
  // absent allocator field would fault, even without AddressSanitizer.
  const long page_size = sysconf(_SC_PAGESIZE);
  ASSERT_GT(page_size, 0);
  void *mapping =
      mmap(nullptr, static_cast<size_t>(page_size) * 2U, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(mapping, MAP_FAILED);
  struct MappingGuard {
    void *address;
    size_t size;
    ~MappingGuard() {
      (void)munmap(address, size);
    }
  } mapping_guard{mapping, static_cast<size_t>(page_size) * 2U};
  ASSERT_EQ(mprotect(static_cast<char *>(mapping) + page_size, static_cast<size_t>(page_size), PROT_NONE), 0);
  constexpr size_t old_size = offsetof(GertModelRunConfig, external_allocator);
  run.struct_size = old_size;
  auto *old_config = reinterpret_cast<GertModelRunConfig *>(static_cast<char *>(mapping) + page_size - old_size);
  ASSERT_EQ(memcpy_s(old_config, old_size, &run, old_size), EOK);
  ASSERT_EQ(runner.Run(*old_config), ACL_SUCCESS);
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
  EXPECT_EQ(events[0], 1U);
  EXPECT_EQ(events[1], 4U);
  EXPECT_EQ(events[2], 1U);
  EXPECT_EQ(events[4], reinterpret_cast<uintptr_t>(&allocator_a));
  EXPECT_EQ(events[5], 0U);
  EXPECT_EQ(events[6], reinterpret_cast<uintptr_t>(&allocator_b));
  EXPECT_EQ(events[7], 0U);
  EXPECT_EQ(events[8], 0U);  // Run cleanup must not restore the Load binding.
  EXPECT_EQ(events[9], 4U);  // Init resource survives every Run and is consumed by DeInit.
}
TEST_F(ProgramBodyCodegenUT, GeneratedLoadFailureReleasesPartialInitResourceExactlyOnce) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  auto program = LifecycleProgram();
  program.init.kernels[0].code_builder = std::make_shared<PartialInitFailureProbe>();
  ProgramBodyCodegenOption option;
  option.model_name = "partial_init_failure";
  option.interface_file_name = "partial_init_failure.h";
  option.source_file_name = "partial_init_failure.cpp";
  option.makefile_file_name = "Makefile";
  ProgramBodyCodegenResult generated;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, option, generated), ge::SUCCESS);
  gert::GertModelDataFile so;
  ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, so), ge::SUCCESS);
  std::array<uintptr_t, 10U> events{};
  int borrowed_allocator = 0;
  int borrowed_stream_storage = 0;
  aclrtStream borrowed_stream = reinterpret_cast<aclrtStream>(&borrowed_stream_storage);
  BorrowedStreamObserver stream_observer(borrowed_stream);
  {
    ge::om2_test::GeneratedModelRunner runner;
    ASSERT_TRUE(runner.Open(so)) << runner.Error();
    GertModelDynamicLoadConfig dynamic_config;
    GertModelLoadConfig load;
    load.dynamic_config = &dynamic_config;
    dynamic_config.rt_session = events.data();
    load.external_allocator = &borrowed_allocator;
    // External Load stream binding is currently optional/ignored by pbody;
    // failure cleanup must still leave this caller-owned handle untouched.
    load.external_streams = &borrowed_stream;
    load.external_stream_num = 1U;
    EXPECT_NE(runner.Load(load), ACL_SUCCESS);
    EXPECT_EQ(events[0], 1U);
    EXPECT_EQ(events[1], 0U);
    EXPECT_EQ(events[2], 1U);
    GertModelRunConfig run;
    EXPECT_NE(runner.Run(run), ACL_SUCCESS);
  }
  EXPECT_EQ(events[2], 1U);  // Failed Load and destruction must not release the chain twice.
  EXPECT_EQ(stream_observer.borrowed_destroy_count, 0U);
}

// F-01 rejection boundaries: unsupported or recursive control flow must not
// produce a publishable set of artifacts, including when the caller had old data.
TEST_F(ProgramBodyCodegenUT, RejectsWhileAndRecursiveRegionsWithoutPublishingArtifacts) {
  ProgramBodyCodegenOption option;
  option.model_name = "invalid_control_flow";
  option.interface_file_name = "invalid_control_flow.h";
  option.source_file_name = "invalid_control_flow.cpp";
  option.makefile_file_name = "Makefile";
  for (const bool recursive : {false, true}) {
    auto program = LifecycleProgram();
    program.main.plan_type = ProgramPlanType::kStructuredTopological;
    auto &step = program.main.regions[0].steps[0];
    if (recursive) {
      step.kind = ProgramStepKind::kIf;
      step.if_info.selector_chain_index = 1U;
      step.if_info.selector_value_type = Om2ValueType::kUInt32T;
      step.condition_region_index = 0U;
    } else {
      step.kind = ProgramStepKind::kWhile;
    }
    ProgramBodyCodegenResult generated;
    generated.artifacts.emplace_back();
    EXPECT_NE(ProgramBodyCodegen().Generate(program, option, generated), ge::SUCCESS);
    EXPECT_TRUE(generated.artifacts.empty());
  }
}

// Regression: a model that binds no space_registry chain directly still needs the default
// registry, because GetSpaceRegistry reads it back through model_desc. Without the indirect
// rule the generated body installed a null registry array and FindInferShapeFunc failed at
// Load with "input or output is nullptr".
TEST_F(ProgramBodyCodegenUT, ModelDescOnlyBindingResolvesDefaultSpaceRegistry) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  // The process registry is normally installed by the lowering path. Pin it here so the case
  // checks the generation rule rather than whatever this binary happens to have.
  auto previous = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
  auto pinned = std::make_shared<gert::OpImplSpaceRegistryV2>();
  ASSERT_EQ(gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(pinned), ge::GRAPH_SUCCESS);
  struct RegistryRestore {
    std::shared_ptr<gert::OpImplSpaceRegistryV2> previous;
    ~RegistryRestore() {
      if (previous != nullptr) {
        (void)gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(previous);
      } else {
        gert::DefaultOpImplSpaceRegistryV2::GetInstance().ClearSpaceRegistry();
      }
    }
  } registry_restore{previous};
  ProgramBodyCodegenOption option;
  option.model_name = "registry_indirect";
  option.interface_file_name = "registry_indirect.h";
  option.source_file_name = "registry_indirect.cpp";
  option.makefile_file_name = "Makefile";
  ProgramBodyCodegenResult generated;
  ASSERT_EQ(ProgramBodyCodegen().Generate(RegistryBindingProgram(false), option, generated), ge::SUCCESS);
  gert::GertModelDataFile so;
  ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, so), ge::SUCCESS);
  std::array<uintptr_t, 5U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(so)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = events.data();
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  GertModelRunConfig run;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  // The probe records what Load bound, but only reads it from the Main kernel.
  EXPECT_NE(events[1], 0U);  // model_desc itself
  EXPECT_NE(events[2], 0U);  // registry array reachable through model_desc
  EXPECT_NE(events[3], 0U);  // default registry entry resolved without a direct binding
  EXPECT_EQ(events[4], 1U);
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

// The widened rule must not become a new loading requirement: a model that carries model_desc
// only, and whose kernels never read the registry, loads successfully even when the process
// default registry is absent.
TEST_F(ProgramBodyCodegenUT, ModelDescOnlyBindingDoesNotRequireDefaultSpaceRegistry) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  // A program that binds model_desc but never dereferences the registry through it.
  auto program = RegistryBindingProgram(false);
  program.main.kernels[0].code_builder = std::make_shared<RegistryBindingProbe>(false);
  ProgramBodyCodegenOption option;
  option.model_name = "registry_not_required";
  option.interface_file_name = "registry_not_required.h";
  option.source_file_name = "registry_not_required.cpp";
  option.makefile_file_name = "Makefile";
  ProgramBodyCodegenResult generated;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, option, generated), ge::SUCCESS);
  gert::GertModelDataFile so;
  ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, so), ge::SUCCESS);
  const auto saved = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
  gert::DefaultOpImplSpaceRegistryV2::GetInstance().ClearSpaceRegistry();
  std::array<uintptr_t, 5U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(so)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = events.data();
  const auto load_ret = runner.Load(load);
  if (load_ret == ACL_SUCCESS) {
    GertModelRunConfig run;
    EXPECT_EQ(runner.Run(run), ACL_SUCCESS);
    EXPECT_EQ(events[4], 1U);
    (void)runner.Unload();
  }
  EXPECT_EQ(load_ret, ACL_SUCCESS) << "model_desc-only models must not require a process registry";
  // Restore the process registry for any later case in this binary.
  (void)gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(saved);
}

}  // namespace
}  // namespace om2
}  // namespace gert
