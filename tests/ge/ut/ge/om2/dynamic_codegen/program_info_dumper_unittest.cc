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
#include "common/om2/dynamic_codegen/info_collect/program_info_dumper.h"

namespace gert {
namespace om2 {
namespace {
ProgramStep DumpIfStep() {
  ProgramStep step;
  step.kind = ProgramStepKind::kIf;
  step.if_info.execution_node_name = "choose\"branch";
  step.if_info.condition_region_index = 1U;
  step.if_info.selector_chain_index = 0U;
  step.if_info.selector_value_type = Om2ValueType::kUInt32T;
  IfProgramInfo::Branch branch;
  branch.selector_value = 0U;
  branch.region_index = 2U;
  branch.entry_node_name = "entry";
  branch.exit_node_name = "exit";
  branch.guarded_entry_node_names = {"guard0", "guard1"};
  branch.completion_node_names = {"done0", "done1"};
  step.if_info.branches = {branch};
  branch.selector_value = 1U;
  branch.region_index = 3U;
  step.if_info.branches.push_back(branch);
  step.if_info.merge_info = {"join", {"exit0", "exit1"}};
  step.if_info.parent_predecessor_node_names = {"before0", "before1"};
  step.if_info.parent_continuation_node_names = {"after0", "after1"};
  return step;
}

TEST(ProgramInfoDumperCoverage, EmptyProgramIncludesAllSectionsAndResourceSizes) {
  const auto text = ProgramInfoDumper::Dump(ProgramInfo{});
  EXPECT_EQ(text.find("{\"chains\":[],\"const_chains\":[],\"custom_const_chains\":[],\"init\":"), 0U);
  EXPECT_NE(text.find("\"kernels\":[],\"regions\":[],\"kernel_dependencies\":[]"), std::string::npos);
  EXPECT_NE(text.find("\"main\":"), std::string::npos);
  EXPECT_NE(text.find("\"deinit\":"), std::string::npos);
  EXPECT_NE(text.find("\"resources\":{\"buffer_size\":0,\"compute_node_info_size\":0,"
                      "\"kernel_extend_info_size\":0,\"model_desc_size\":0}}"),
            std::string::npos);
}

TEST(ProgramInfoDumperCoverage, EscapesNamesAndPreservesPortAndChainMetadata) {
  ProgramInfo program;
  program.chains = {
      {0U, 9U, ChainSourceKind::kConstant, ChainBindingPhase::kNone, ChainBindingKind::kNone, -1},
      {1U, 10U, ChainSourceKind::kRuntimeBinding, ChainBindingPhase::kLoad, ChainBindingKind::kRtSession, 2}};
  Om2ConstValue value;
  value.type = Om2ValueType::kInt32T;
  program.const_chains = {{0U, "constant\\name", value}, {1U, "second", value}};
  KernelProgramInfo kernel;
  kernel.execution_node_name = "node\"name";
  kernel.kernel_type = "type\\name";
  kernel.inputs = {{0U, "input\"name", Om2ValueType::kInt32T}, {1U, "second", Om2ValueType::kInt32T}};
  kernel.outputs = {{1U, "out", Om2ValueType::kInt32T}};
  kernel.compute_node_index = 7U;
  program.main.kernels.push_back(kernel);
  kernel.require_compute_node_info = true;
  program.main.kernels.push_back(kernel);
  program.main.kernel_dependencies = {{0U, 1U}, {1U, 2U}};
  const auto text = ProgramInfoDumper::Dump(program);
  EXPECT_NE(text.find("\"name\":\"node\\\"name\",\"type\":\"type\\\\name\""), std::string::npos);
  EXPECT_NE(text.find("\"producer_name\":\"constant\\\\name\""), std::string::npos);
  EXPECT_NE(text.find("\"inputs\":[{\"chain\":0,\"name\":\"input\\\"name\",\"value_type\":0},"
                      "{\"chain\":1,\"name\":\"second\",\"value_type\":0}]"),
            std::string::npos);
  EXPECT_NE(text.find("\"compute_node_index\":7"), std::string::npos);
  EXPECT_NE(text.find("\"require_compute_node_info\":false"), std::string::npos);
  EXPECT_NE(text.find("\"require_compute_node_info\":true"), std::string::npos);
  EXPECT_NE(text.find("\"kernel_dependencies\":[{\"predecessor\":0,\"successor\":1},"
                      "{\"predecessor\":1,\"successor\":2}]"),
            std::string::npos);
}

TEST(ProgramInfoDumperCoverage, DumpsBothConditionSourcesAndCompleteBranchContract) {
  ProgramInfo program;
  auto step = DumpIfStep();
  ProgramStep kernel;
  kernel.kernel_index = 7U;
  program.main.regions = {{0U, "graph\\name", {kernel, step}}, {1U, "condition", {}}};
  step.condition_region_index = 9U;
  program.main.regions[1].steps.push_back(step);
  ProgramStep loop;
  loop.kind = ProgramStepKind::kWhile;
  loop.while_info = {4U, 5U, 6U};
  program.main.regions[1].steps.push_back(loop);
  const auto text = ProgramInfoDumper::Dump(program);
  EXPECT_NE(text.find("\"kind\":\"kernel\",\"kernel_index\":7"), std::string::npos);
  EXPECT_NE(text.find("\"name\":\"choose\\\"branch\",\"condition_region\":1"), std::string::npos);
  EXPECT_NE(text.find("\"name\":\"choose\\\"branch\",\"condition_region\":9"), std::string::npos);
  EXPECT_NE(text.find("\"selector\":1,\"region\":3,\"entry\":\"entry\",\"exit\":\"exit\""), std::string::npos);
  EXPECT_NE(text.find("\"guarded_entries\":[\"guard0\",\"guard1\"]"), std::string::npos);
  EXPECT_NE(text.find("\"completion_nodes\":[\"done0\",\"done1\"]"), std::string::npos);
  EXPECT_NE(text.find("\"merge\":{\"join\":\"join\",\"branch_exits\":[\"exit0\",\"exit1\"]}"), std::string::npos);
  EXPECT_NE(text.find("\"parent_predecessors\":[\"before0\",\"before1\"]"), std::string::npos);
  EXPECT_NE(text.find("\"parent_continuations\":[\"after0\",\"after1\"]"), std::string::npos);
  EXPECT_NE(text.find("\"kind\":\"while\",\"condition_region\":4,\"body_region\":5,\"condition_chain\":6"),
            std::string::npos);
}

TEST(ProgramInfoDumperCoverage, ReportsCustomPayloadMetadataWithoutDumpingPayload) {
  ProgramInfo program;
  program.custom_const_chains = {{0U, "string", "restore\"string", true, {'s', 'e', 'c', 'r', 'e', 't', 0U}},
                                 {1U, "bytes", "restore\\bytes", false, {1U, 2U}}};
  program.resources.buffer.resize(3U);
  program.resources.compute_node_infos.resize(4U);
  program.resources.kernel_extend_infos.resize(5U);
  program.resources.model_desc.resize(6U);
  const auto text = ProgramInfoDumper::Dump(program);
  EXPECT_NE(text.find("\"initializer\":\"restore\\\"string\",\"payload_is_string\":true,\"payload_size\":7"),
            std::string::npos);
  EXPECT_NE(text.find("\"initializer\":\"restore\\\\bytes\",\"payload_is_string\":false,\"payload_size\":2"),
            std::string::npos);
  EXPECT_EQ(text.find("secret"), std::string::npos);
  EXPECT_NE(text.find("\"buffer_size\":3,\"compute_node_info_size\":4,\"kernel_extend_info_size\":5,"
                      "\"model_desc_size\":6"),
            std::string::npos);
}
}  // namespace
}  // namespace om2
}  // namespace gert
