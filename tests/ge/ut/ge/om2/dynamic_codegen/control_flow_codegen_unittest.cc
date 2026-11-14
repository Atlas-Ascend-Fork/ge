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
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/helper/om2/om2_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/program_body_codegen.h"
#include "exe_graph/lowering/buffer_pool.h"
#include "graph/fast_graph/execute_graph.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"
#include "stub/gert_runtime_stub.h"
#include "../../graph/load/om2_package_test_utils.h"
#include "../../../../common/om2_generated_model_runner.h"

#include "control_flow_codegen_fixture.h"
#include "generated_so_fixture.h"

namespace gert {
namespace om2 {
namespace {
using control_flow_fixture::BranchProgram;

using ControlFlowCodegenUT = ge::om2_test::DynamicPackageTest;

void GenerateAndRunBranches(const bool nested) {
  ProgramBodyCodegenOption option;
  option.model_name = nested ? "nested_branch_probe" : "branch_probe";
  option.interface_file_name = option.model_name + ".h";
  option.source_file_name = option.model_name + ".cpp";
  option.makefile_file_name = "Makefile";
  ProgramBodyCodegenResult generated;
  ASSERT_EQ(ProgramBodyCodegen().Generate(BranchProgram(nested), option, generated), ge::SUCCESS);
  GertModelDataFile so;
  ASSERT_EQ(LoadGeneratedSoFixture(generated.artifacts, option.model_name, so), ge::SUCCESS);
  std::array<uint64_t, 8U> events{};
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(so)) << runner.Error();
  GertModelDynamicLoadConfig dynamic_config;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic_config;
  dynamic_config.rt_session = events.data();
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  EXPECT_EQ(events[5], 1U);
  std::array<uint64_t, 3U> expected_counts{};
  GertModelRunConfig run;
  const std::vector<std::array<uint64_t, 2U>> selections =
      nested ? std::vector<std::array<uint64_t, 2U>>{{0U, 1U}, {1U, 0U}, {1U, 1U}, {0U, 0U}}
             : std::vector<std::array<uint64_t, 2U>>{{0U, 0U}, {1U, 0U}, {0U, 1U}, {1U, 1U}};
  for (const auto &selection : selections) {
    events[0] = selection[0];
    events[1] = selection[1];
    const auto selected = nested && selection[0] == 1U ? 1U + selection[1] : selection[0];
    ++expected_counts[selected];
    ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
    for (size_t branch = 0U; branch < expected_counts.size(); ++branch) {
      EXPECT_EQ(events[2U + branch], expected_counts[branch]);
    }
    EXPECT_EQ(events[7], 10U + selected);
    EXPECT_EQ(events[5], 1U);
    EXPECT_EQ(events[6], 0U);
  }
  events[0] = 2U;  // The generated switch rejects undeclared selectors without choosing a branch.
  EXPECT_NE(runner.Run(run), ACL_SUCCESS);
  for (size_t branch = 0U; branch < expected_counts.size(); ++branch) {
    EXPECT_EQ(events[2U + branch], expected_counts[branch]);
  }
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
  EXPECT_EQ(events[5], 1U);
  EXPECT_EQ(events[6], 1U);
}

// F-01: both selectors and branch effects are observed through real generated code.
TEST_F(ControlFlowCodegenUT, GeneratedBranchExecutesOnlySelectedRegionAcrossRuns) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GenerateAndRunBranches(false);
}

TEST_F(ControlFlowCodegenUT, GeneratedNestedBranchesPreserveInitValueAcrossRuns) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GenerateAndRunBranches(true);
}

TEST(ControlFlowCollectorBoundaryUT, RejectsOfflineKnownSubgraphProcessLocalModel) {
  ge::om2_test::ScopedEnvVar mode("ENABLE_RUNTIME_OM2", "0");
  GertRuntimeStub runtime_stub;
  ge::ExecuteGraph graph("offline_known_subgraph");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("known_create", "DavinciModelCreate"));
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("DavinciModelCreate");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> no_ports;
  const KernelCollectContext context(*node, no_ports, no_ports);
  KernelProgramInfo info;
  runtime_stub.GetSlogStub().Clear();
  EXPECT_NE(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  EXPECT_NE(runtime_stub.GetSlogStub().FindLog(DLOG_ERROR,
                                               "OM2 DavinciModelCreate known_create is only supported in online mode"),
            -1);
}
}  // namespace
}  // namespace om2
}  // namespace gert
