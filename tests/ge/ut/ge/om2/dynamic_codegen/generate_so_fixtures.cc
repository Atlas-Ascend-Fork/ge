/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "framework/common/gert_model_data_utils.h"
#include "program_body_codegen_fixture.h"
#include "control_flow_codegen_fixture.h"
#include "shared_library_loading_fixture.h"
#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "common/om2/codegen/task_code_builder/task_code_builder_util.h"
#include "common/om2/codegen/task_code_builder/aicpu/kernel_ex_task_code_builder.h"

namespace {
void WriteFile(const std::filesystem::path &path, const void *data, const size_t size) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  output.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
  ASSERT_TRUE(output.good()) << path;
}

void CompileModel(const std::filesystem::path &root, const std::string &name, const gert::om2::ProgramInfo &program) {
  gert::om2::ProgramBodyCodegenOption option;
  option.model_name = name;
  option.interface_file_name = name + ".h";
  option.source_file_name = name + ".cpp";
  option.makefile_file_name = "Makefile";
  gert::om2::ProgramBodyCodegenResult generated;
  ASSERT_EQ(gert::om2::ProgramBodyCodegen().Generate(program, option, generated), ge::SUCCESS);
  gert::GertModelDataFile so;
  ASSERT_EQ(ge::Om2Utils::CompileGeneratedCppToSo(generated.artifacts, {name, name, 0U}, so, false), ge::SUCCESS);
  for (const auto &artifact : generated.artifacts) {
    ASSERT_NO_FATAL_FAILURE(
        WriteFile(root / name / gert::GertGetStr(artifact.file_name), artifact.data.get(), artifact.data_size));
  }
  ASSERT_NO_FATAL_FAILURE(WriteFile(root / name / (name + ".so"), so.data.get(), so.data_size));
}

void GenerateLiterals(const std::filesystem::path &root) {
  const std::vector<uint8_t> bytes{0x44U, 0x00U, 0x22U, 0x5cU, 0xffU};
  const std::array<std::pair<std::string, std::string>, 2U> literals{
      {{"hccl", ge::TaskCodeBuilderUtil::EscapeToOctalString(bytes)},
       {"aicpu", ge::KernelExTaskCodeBuilder::SerializeBytesToOctalString(bytes)}}};
  std::string source = "#include <cstddef>\n";
  for (const auto &literal : literals) {
    ge::AstContext context;
    ge::AstBuildContext ast{context};
    const auto declaration = ast.VarDecl("const char", "data[]", ge::Arg::StringLiteral(literal.second));
    ge::CppEmitter emitter;
    std::string code;
    ASSERT_EQ(declaration->Accept(emitter, code), ge::SUCCESS);
    ASSERT_NO_FATAL_FAILURE(WriteFile(root / (literal.first + ".txt"), code.data(), code.size()));
    source += "namespace " + literal.first + " {\n" + code + "}\n";
    source += "extern \"C\" const char *" + literal.first + "_data() { return " + literal.first + "::data; }\n";
    source += "extern \"C\" size_t " + literal.first + "_size() { return sizeof(" + literal.first + "::data) - 1U; }\n";
  }
  ASSERT_NO_FATAL_FAILURE(WriteFile(root / "literals.cpp", source.data(), source.size()));
}

void GenerateFixtures(const std::filesystem::path &root) {
  using namespace gert::om2;
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "lifecycle_probe", program_body_fixture::LifecycleProgram()));
  auto failed_init = program_body_fixture::LifecycleProgram();
  failed_init.init.kernels[0].code_builder = std::make_shared<program_body_fixture::PartialInitFailureProbe>();
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "partial_init_failure", failed_init));
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "registry_indirect", program_body_fixture::RegistryBindingProgram(false)));
  auto no_registry = program_body_fixture::RegistryBindingProgram(false);
  no_registry.main.kernels[0].code_builder = std::make_shared<program_body_fixture::RegistryBindingProbe>(false);
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "registry_not_required", no_registry));
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "branch_probe", control_flow_fixture::BranchProgram(false)));
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "nested_branch_probe", control_flow_fixture::BranchProgram(true)));
  ASSERT_NO_FATAL_FAILURE(CompileModel(root, "shared_library_probe", shared_library_fixture::SharedLibraryProgram()));
  ASSERT_NO_FATAL_FAILURE(GenerateLiterals(root));
}
}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    return 1;
  }
  const ge::om2_test::ScopedEnvVar home("ASCEND_HOME_PATH", ge::EnvPath().GetAscendInstallPath().c_str());
  const auto work = ge::EnvPath().GetOrCreateCaseTmpPath("Om2GeneratedFixtures");
  const ge::om2_test::ScopedEnvVar workspace("ASCEND_WORK_PATH", work.c_str());
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GenerateFixtures(argv[1]);
  return testing::UnitTest::GetInstance()->ad_hoc_test_result().Failed() ? 1 : 0;
}
