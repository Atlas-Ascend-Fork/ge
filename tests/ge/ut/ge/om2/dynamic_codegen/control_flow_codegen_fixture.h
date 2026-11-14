/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_CONTROL_FLOW_CODEGEN_FIXTURE_H_
#define GE_TESTS_OM2_CONTROL_FLOW_CODEGEN_FIXTURE_H_

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

namespace gert {
namespace om2 {
namespace control_flow_fixture {
// These kernels only supply selectors and observe selected branches. The model
// runtime, switch statements, compilation and exported lifecycle are generated.
class ControlProbe final : public KernelCodeBuilder {
 public:
  explicit ControlProbe(std::string body) : body_(std::move(body)) {}
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol +
           "() {\n"
           "    auto *events = chains_[0].GetValue<uint64_t *>();\n"
           "    if (events == nullptr) { return kFailed; }\n" +
           body_ + "    return kOk;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }

 private:
  std::string body_;
};

inline std::vector<uint8_t> PoolBytes(const bg::BufferPool &pool) {
  size_t size = 0U;
  const auto data = pool.Serialize(size);
  if (data == nullptr) {
    ADD_FAILURE() << "BufferPool serialization failed";
    return {};
  }
  return {data.get(), data.get() + size};
}

inline ProgramStep KernelStep(const uint32_t index) {
  ProgramStep step;
  step.kernel_index = index;
  return step;
}

inline ProgramStep IfStep(const uint32_t selector_chain, const uint32_t condition_region, const uint32_t first_region,
                          const uint32_t second_region) {
  ProgramStep step;
  step.kind = ProgramStepKind::kIf;
  step.condition_region_index = condition_region;
  step.if_info.selector_chain_index = selector_chain;
  step.if_info.selector_value_type = Om2ValueType::kUInt32T;
  IfProgramInfo::Branch first;
  first.selector_value = 0U;
  first.region_index = first_region;
  IfProgramInfo::Branch second;
  second.selector_value = 1U;
  second.region_index = second_region;
  step.if_info.branches = {first, second};
  return step;
}

void AddProbe(ProgramBlock &block, const char *name, const std::string &body) {
  KernelProgramInfo kernel;
  kernel.execution_node_name = name;
  kernel.kernel_type = "Om2ControlFlowProbe";
  kernel.impl_type = KernelImplType::kCodegenImpl;
  kernel.output_policy = OutputCreatePolicy::kTemporary;
  kernel.code_builder = std::make_shared<ControlProbe>(body);
  block.kernels.push_back(std::move(kernel));
}

inline ProgramInfo BranchProgram(const bool nested) {
  ProgramInfo program;
  bg::BufferPool empty;
  program.resources.buffer = PoolBytes(empty);
  program.resources.compute_node_infos = PoolBytes(empty);
  program.resources.kernel_extend_infos = PoolBytes(empty);
  bg::BufferPool model;
  bg::BufferPool::BufId id = 0U;
  EXPECT_EQ(BuildModelDescBuffer({}, {}, {}, model, id), ge::GRAPH_SUCCESS);
  program.resources.model_desc = PoolBytes(model);
  program.chains.resize(4U);
  for (uint32_t i = 0U; i < program.chains.size(); ++i) {
    program.chains[i].index = i;
    program.chains[i].symbol = i + 1U;
    program.chains[i].source_kind = ChainSourceKind::kKernelOutput;
  }
  program.chains[0].source_kind = ChainSourceKind::kRuntimeBinding;
  program.chains[0].binding_phase = ChainBindingPhase::kLoad;
  program.chains[0].binding_kind = ChainBindingKind::kRtSession;
  AddProbe(program.init, "initialize_seed",
           "    ++events[5];\n    chains_[3].SetWithDefaultDeleter(new uint64_t(10U));\n");
  program.init.regions.resize(1U);
  program.init.root_region_index = 0U;
  program.init.regions[0].steps = {KernelStep(0U)};
  AddProbe(program.deinit, "release_seed", "    ++events[6];\n    chains_[3].Set(nullptr, nullptr);\n");
  program.deinit.regions.resize(1U);
  program.deinit.root_region_index = 0U;
  program.deinit.regions[0].steps = {KernelStep(0U)};

  AddProbe(program.main, "select_outer",
           "    chains_[1].Set(nullptr, nullptr);\n"
           "    *chains_[1].GetPointer<uint32_t>() = static_cast<uint32_t>(events[0]);\n");
  AddProbe(program.main, "select_inner",
           "    chains_[2].Set(nullptr, nullptr);\n"
           "    *chains_[2].GetPointer<uint32_t>() = static_cast<uint32_t>(events[1]);\n");
  for (uint32_t branch = 0U; branch < 3U; ++branch) {
    const auto ordinal = std::to_string(branch);
    const auto counter = std::to_string(2U + branch);
    const auto name = "branch_" + ordinal;
    AddProbe(program.main, name.c_str(),
             "    const auto *seed = chains_[3].GetValue<uint64_t *>();\n"
             "    if (seed == nullptr) { return kFailed; }\n"
             "    ++events[" +
                 counter +
                 "];\n"
                 "    events[7] = *seed + " +
                 ordinal + "U;\n");
  }
  program.main.plan_type = ProgramPlanType::kStructuredTopological;
  program.main.root_region_index = 0U;
  program.main.regions.resize(nested ? 7U : 4U);
  for (uint32_t i = 0U; i < program.main.regions.size(); ++i) {
    program.main.regions[i].index = i;
    program.main.regions[i].graph_name = "control_region_" + std::to_string(i);
  }
  program.main.regions[0].steps = {IfStep(1U, 1U, 2U, 3U)};
  program.main.regions[1].steps = {KernelStep(0U)};
  program.main.regions[2].steps = {KernelStep(2U)};
  if (nested) {
    program.main.regions[3].steps = {IfStep(2U, 4U, 5U, 6U)};
    program.main.regions[4].steps = {KernelStep(1U)};
    program.main.regions[5].steps = {KernelStep(3U)};
    program.main.regions[6].steps = {KernelStep(4U)};
  } else {
    program.main.regions[3].steps = {KernelStep(3U)};
  }
  return program;
}

}  // namespace control_flow_fixture
}  // namespace om2
}  // namespace gert

#endif  // GE_TESTS_OM2_CONTROL_FLOW_CODEGEN_FIXTURE_H_
