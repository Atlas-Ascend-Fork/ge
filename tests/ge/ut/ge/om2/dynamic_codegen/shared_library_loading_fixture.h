/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_SHARED_LIBRARY_LOADING_FIXTURE_H_
#define GE_TESTS_OM2_SHARED_LIBRARY_LOADING_FIXTURE_H_

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

namespace gert {
namespace om2 {
namespace shared_library_fixture {
class SharedLibraryProbe final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol +
           "() {\n"
           "    auto *events = chains_[0].GetValue<uintptr_t *>();\n"
           "    auto *registry = chains_[1].GetValue<gert::OpImplSpaceRegistryV2 *>();\n"
           "    if (events == nullptr || registry == nullptr) { return kFailed; }\n"
           "    const auto *implementation = registry->GetOpImpl(\"Om2SharedLibraryProbe\");\n"
           "    if (implementation == nullptr || implementation->infer_shape == nullptr) { return kFailed; }\n"
           "    events[0] = implementation->infer_shape(nullptr);\n"
           "    ++events[1];\n"
           "    return events[0] == 73U ? kOk : kFailed;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }
};

inline std::vector<uint8_t> SerializedPool(const bg::BufferPool &pool) {
  size_t size = 0U;
  const auto data = pool.Serialize(size);
  if (data == nullptr) {
    ADD_FAILURE() << "BufferPool serialization failed";
    return {};
  }
  return {data.get(), data.get() + size};
}

inline ProgramInfo SharedLibraryProgram() {
  ProgramInfo program;
  bg::BufferPool empty;
  program.resources.buffer = SerializedPool(empty);
  program.resources.compute_node_infos = SerializedPool(empty);
  program.resources.kernel_extend_infos = SerializedPool(empty);
  bg::BufferPool model;
  bg::BufferPool::BufId id = 0U;
  EXPECT_EQ(BuildModelDescBuffer({}, {}, {}, model, id), ge::GRAPH_SUCCESS);
  program.resources.model_desc = SerializedPool(model);
  program.chains.resize(2U);
  for (uint32_t i = 0U; i < 2U; ++i) {
    program.chains[i].index = i;
    program.chains[i].symbol = i + 1U;
    program.chains[i].source_kind = ChainSourceKind::kRuntimeBinding;
    program.chains[i].binding_phase = ChainBindingPhase::kLoad;
  }
  program.chains[0].binding_kind = ChainBindingKind::kRtSession;
  program.chains[1].binding_kind = ChainBindingKind::kSpaceRegistry;
  program.main.root_region_index = 0U;
  program.main.regions.resize(1U);
  ProgramStep step;
  step.kernel_index = 0U;
  program.main.regions[0].steps.push_back(step);
  KernelProgramInfo kernel;
  kernel.execution_node_name = "shared_library_probe";
  kernel.kernel_type = "Om2TestSharedLibraryProbe";
  kernel.impl_type = KernelImplType::kCodegenImpl;
  kernel.output_policy = OutputCreatePolicy::kTemporary;
  kernel.code_builder = std::make_shared<SharedLibraryProbe>();
  program.main.kernels.push_back(std::move(kernel));
  return program;
}

}  // namespace shared_library_fixture
}  // namespace om2
}  // namespace gert

#endif  // GE_TESTS_OM2_SHARED_LIBRARY_LOADING_FIXTURE_H_
