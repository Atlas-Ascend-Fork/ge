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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "common/helper/om2/om2_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/program_body_codegen.h"
#include "exe_graph/lowering/buffer_pool.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"
#include "stub/gert_runtime_stub.h"
#include "../../graph/load/om2_package_test_utils.h"
#include "../../../../common/om2_generated_model_runner.h"

#include "shared_library_loading_fixture.h"
#include "generated_so_fixture.h"

namespace gert {
namespace om2 {
namespace {
using shared_library_fixture::SharedLibraryProgram;

std::vector<uint8_t> ReadBytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

class SharedLibraryLoadingUT : public ge::om2_test::DynamicPackageTest {
 protected:
  void ReadDependencyLibraries(const bool missing_dependency) {
    // Positive and negative cases have different DT_NEEDED names. A provider loaded
    // by another case (including --gtest_repeat/shuffle) cannot mask a missing dependency.
    const std::string variant = missing_dependency ? "missing" : "present";
    dependency_name_ = "libom2_dependency_" + variant + ".so";
    const auto directory = std::filesystem::path(OM2_GENERATED_FIXTURE_DIR) / "dependencies" / variant;
    provider_bytes_ = ReadBytes(directory / dependency_name_);
    consumer_bytes_ = ReadBytes(directory / consumer_name_);
    ASSERT_FALSE(provider_bytes_.empty());
    ASSERT_FALSE(consumer_bytes_.empty());
  }

  void GenerateModel() {
    ProgramBodyCodegenOption option;
    option.model_name = "shared_library_probe";
    option.interface_file_name = "shared_library_probe.h";
    option.source_file_name = "shared_library_probe.cpp";
    option.makefile_file_name = "Makefile";
    ProgramBodyCodegenResult generated;
    ASSERT_EQ(ProgramBodyCodegen().Generate(SharedLibraryProgram(), option, generated), ge::SUCCESS);
    ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, model_), ge::SUCCESS);
    ASSERT_GT(model_.data_size, 0U);
  }

  GertModelSharedLibrary Library(const std::string &name, const std::vector<uint8_t> &bytes) const {
    GertModelSharedLibrary library;
    library.so_name = name.c_str();
    library.so_bin_type = GERT_MODEL_SHARED_LIBRARY_SPACE_REGISTRY;
    library.data = bytes.data();
    library.data_size = bytes.size();
    return library;
  }

  gert::GertRuntimeStub runtime_stub_;
  std::string dependency_name_;
  std::string consumer_name_ = "libom2_consumer.so";
  std::vector<uint8_t> provider_bytes_;
  std::vector<uint8_t> consumer_bytes_;
  gert::GertModelDataFile model_;
};

// R-01: descriptor declaration order is opposite to ELF dependency order.
// Build-time fixtures retain real ELF dependencies and real generated model execution.
TEST_F(SharedLibraryLoadingUT, MaterializesDependenciesBeforeLoadingAndUsesRegisteredImplementation) {
  ASSERT_NO_FATAL_FAILURE(ReadDependencyLibraries(false));
  ASSERT_NO_FATAL_FAILURE(GenerateModel());
  const auto consumer = Library(consumer_name_, consumer_bytes_);
  const auto provider = Library(dependency_name_, provider_bytes_);
  const GertModelSharedLibrary *libraries[] = {&consumer, &provider};
  GertModelSharedLibraries shared;
  shared.library_num = 2U;
  shared.libraries = libraries;
  std::array<uintptr_t, 2U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(model_)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.shared_libraries = &shared;
  dynamic_config.rt_session = events.data();
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  GertModelRunConfig run;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  EXPECT_EQ(events[0], 73U);
  EXPECT_EQ(events[1], 2U);
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

// R-01 boundary: the public registry keeps its fault tolerance for operator SOs, so a
// consumer whose ELF dependency is absent does not fail the registry addition itself.
// What must hold is that the missing implementation is never invoked: loading reports the
// failure, the run fails, and the probe counter stays at zero.
TEST_F(SharedLibraryLoadingUT, MissingElfDependencyDoesNotInvokeMissingImplementation) {
  ASSERT_NO_FATAL_FAILURE(ReadDependencyLibraries(true));
  ASSERT_NO_FATAL_FAILURE(GenerateModel());
  const auto consumer = Library(consumer_name_, consumer_bytes_);
  const GertModelSharedLibrary *libraries[] = {&consumer};
  GertModelSharedLibraries shared;
  shared.library_num = 1U;
  shared.libraries = libraries;
  std::array<uintptr_t, 2U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(model_)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.shared_libraries = &shared;
  dynamic_config.rt_session = events.data();
  const auto load_ret = runner.Load(load);
  if (load_ret == ACL_SUCCESS) {
    GertModelRunConfig run;
    EXPECT_NE(runner.Run(run), ACL_SUCCESS);
  }
  EXPECT_EQ(events[1], 0U);
  const std::string dlopen_failure = consumer_name_ + "` via dlopen";
  EXPECT_NE(runtime_stub_.GetSlogStub().FindLog(DLOG_ERROR, dlopen_failure.c_str()), -1);
}

TEST_F(SharedLibraryLoadingUT, ConflictingDeclaredLibraryNamesRejectLoad) {
  // Duplicate names are rejected before bytes are interpreted as ELF.
  consumer_bytes_ = {1U};
  provider_bytes_ = {2U};
  ASSERT_NO_FATAL_FAILURE(GenerateModel());
  const auto consumer = Library(consumer_name_, consumer_bytes_);
  const auto different_bytes_same_name = Library(consumer_name_, provider_bytes_);
  const GertModelSharedLibrary *libraries[] = {&consumer, &different_bytes_same_name};
  GertModelSharedLibraries shared;
  shared.library_num = 2U;
  shared.libraries = libraries;
  std::array<uintptr_t, 2U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(model_)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.shared_libraries = &shared;
  dynamic_config.rt_session = events.data();
  EXPECT_NE(runner.Load(load), ACL_SUCCESS);
  EXPECT_EQ(events[1], 0U);
}
}  // namespace
}  // namespace om2
}  // namespace gert
