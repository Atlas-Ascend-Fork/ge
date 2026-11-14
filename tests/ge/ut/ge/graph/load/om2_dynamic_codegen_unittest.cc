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

#include "om2_package_test_utils.h"

namespace ge {
using Om2DynamicCodegenUt = om2_test::DynamicPackageTest;

TEST_F(Om2DynamicCodegenUt, CompileDynamicProgramBodyRejectsInvalidSource) {
  std::vector<gert::GertModelDataFile> artifacts;
  gert::GertModelDataFile file;
  file.file_name = gert::GertMakeStr("incomplete.cpp");
  const std::string source = "invalid C++";
  file.data = gert::GertMakeFileData(source.data(), source.size());
  file.data_size = source.size();
  artifacts.push_back(std::move(file));
  gert::GertModelDataRuntime runtime;
  EXPECT_NE(Om2PackageHelper::CompileDynamicProgramBody("invalid", 0U, std::move(artifacts), runtime), SUCCESS);
  EXPECT_EQ(runtime.so_artifact.data_size, 0U);
}

TEST_F(Om2DynamicCodegenUt, CompileAndPackageOm2FailureKeepsPreparedResources) {
  auto graph = std::make_shared<ExecuteGraph>("unprepared");
  gert::GertModelData data;
  gert::InitGertModelData(data);
  gert::InitGertModelDataUnit(data);
  data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  data.kernels->binaries.emplace_back(std::make_unique<gert::GertModelDataFile>());
  data.manifest->compatibility.used_features.emplace(gert::GertMakeStr("hccl_communication"), gert::GertMakeStr("1.0"));
  const auto *original_model = data.models[0].get();
  const auto *original_kernel = data.kernels->binaries[0].get();
  const auto *original_libraries = data.so_bins.get();
  // 缺少生命周期子图，信息收集失败；公共流程不能重置准备好的模型数据。
  EXPECT_NE(Om2PackageHelper::CompileAndPackageOm2(graph, "unprepared", 0U, data), SUCCESS);
  ASSERT_EQ(data.models.size(), 1U);
  EXPECT_EQ(data.models[0].get(), original_model);
  ASSERT_EQ(data.kernels->binaries.size(), 1U);
  EXPECT_EQ(data.kernels->binaries[0].get(), original_kernel);
  EXPECT_EQ(data.so_bins.get(), original_libraries);
  EXPECT_EQ(data.manifest->compatibility.used_features.size(), 1U);
}

namespace {
std::vector<gert::GertModelDataFile> CompilationArtifacts(const std::string &source) {
  std::vector<gert::GertModelDataFile> artifacts;
  const auto add = [&artifacts](const std::string &name, const std::string &content) {
    gert::GertModelDataFile file;
    file.file_name = gert::GertMakeStr(name);
    file.data = gert::GertMakeFileData(content.data(), content.size());
    file.data_size = content.size();
    artifacts.push_back(std::move(file));
  };
  add("compile_probe.cpp", source);
  add("Makefile",
      "TARGET := libcompile_probe.so\nSRC_FILES := compile_probe.cpp\n"
      "all: $(TARGET)\n$(TARGET): $(SRC_FILES)\n"
      "\t$(CXX) -std=c++17 -fPIC -shared -o $@ $^\n");
  return artifacts;
}
}  // namespace

TEST_F(Om2DynamicCodegenUt, CompilerCommandFailureDoesNotPublishSharedObject) {
  GetThreadLocalContext().SetGraphOption({{"ge.buildConfig", "false"}});
  auto artifacts = CompilationArtifacts("extern \"C\" int probe() { return 1; }\n");
  gert::GertModelDataRuntime runtime;
  EXPECT_NE(Om2PackageHelper::CompileDynamicProgramBody("tool_failure", 0U, std::move(artifacts), runtime), SUCCESS);
  EXPECT_EQ(runtime.so_artifact.data_size, 0U);
  EXPECT_EQ(runtime.so_artifact.data, nullptr);
  EXPECT_EQ(runtime.source_artifacts.size(), 2U);
}

TEST_F(Om2DynamicCodegenUt, MissingRequiredGeneratedHeaderDoesNotPublishSharedObject) {
  auto artifacts = CompilationArtifacts(
      "#include \"om2_missing_generated_header.h\"\n"
      "extern \"C\" int probe() { return 1; }\n");
  gert::GertModelDataRuntime runtime;
  EXPECT_NE(Om2PackageHelper::CompileDynamicProgramBody("missing_header", 0U, std::move(artifacts), runtime), SUCCESS);
  EXPECT_EQ(runtime.so_artifact.data_size, 0U);
  EXPECT_EQ(runtime.so_artifact.data, nullptr);
  EXPECT_EQ(runtime.source_artifacts.size(), 2U);
}

}  // namespace ge
