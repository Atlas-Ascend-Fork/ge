/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_PROGRAM_BODY_CODEGEN_FIXTURE_H_
#define GE_TESTS_OM2_PROGRAM_BODY_CODEGEN_FIXTURE_H_

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

namespace gert {
namespace om2 {
namespace program_body_fixture {
// A test-only codegen Kernel observes lifecycle bindings in the generated model itself.
// It does not replace ProgramBodyCodegen, its runtime, or any exported model function.
class LifecycleProbe : public KernelCodeBuilder {
 public:
  explicit LifecycleProbe(const size_t phase) : phase_(phase) {}
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol +
           "() {\n"
           "    auto *events = chains_[0].GetValue<uintptr_t *>();\n"
           "    if (events == nullptr) { return kFailed; }\n";
    if (phase_ == 0U) {
      code +=
          "    ++events[0];\n"
          "    events[3] = reinterpret_cast<uintptr_t>(chains_[1].GetValue<void *>());\n"
          "    chains_[2].Set(new uint64_t(0U), nullptr);\n";
    } else if (phase_ == 1U) {
      code +=
          "    auto *resource = chains_[2].GetValue<uint64_t *>();\n"
          "    if (resource == nullptr || events[1] >= 4U) { return kFailed; }\n"
          "    events[4U + events[1]] = reinterpret_cast<uintptr_t>(chains_[1].GetValue<void *>());\n"
          "    ++events[1];\n"
          "    ++(*resource);\n";
    } else {
      code +=
          "    ++events[2];\n"
          "    events[8] = reinterpret_cast<uintptr_t>(chains_[1].GetValue<void *>());\n"
          "    auto *resource = chains_[2].GetValue<uint64_t *>();\n"
          "    if (resource == nullptr) { return kFailed; }\n"
          "    events[9] = *resource;\n"
          "    delete resource;\n"
          "    chains_[2].Set(nullptr, nullptr);\n";
    }
    code += "    return kOk;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }

 private:
  size_t phase_;
};

// Fail only after a chain has acquired an owned resource, so Load's production
// failure guard and the model destructor must coordinate their cleanup.
class PartialInitFailureProbe final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol +
           "() {\n"
           "    auto *events = chains_[0].GetValue<uintptr_t *>();\n"
           "    if (events == nullptr) { return kFailed; }\n"
           "    ++events[0];\n"
           "    auto *resource = new uintptr_t[1]{reinterpret_cast<uintptr_t>(events)};\n"
           "    chains_[2].Set(resource, [](void *pointer) {\n"
           "      auto *owned = static_cast<uintptr_t *>(pointer);\n"
           "      ++reinterpret_cast<uintptr_t *>(owned[0])[2];\n"
           "      delete[] owned;\n"
           "    });\n"
           "    return kFailed;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }
};

// Reports what Load bound to the model_desc and space_registry chains. A chain that was never
// bound still reads as nullptr, which is what the indirect-registry regression must detect.
class RegistryBindingProbe final : public KernelCodeBuilder {
 public:
  explicit RegistryBindingProbe(const bool read_model_desc) : read_model_desc_(read_model_desc) {}
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol +
           "() {\n"
           "    auto *events = chains_[0].GetValue<uintptr_t *>();\n"
           "    if (events == nullptr) { return kFailed; }\n";
    if (read_model_desc_) {
      // Reading model_desc->GetSpaceRegistries() is the indirect use: it must hand back the
      // registry array the body installed, with a non-null default entry.
      code +=
          "    auto *model_desc = chains_[1].GetValue<gert::ModelDesc *>();\n"
          "    if (model_desc == nullptr) { return kFailed; }\n"
          "    auto *registries = model_desc->GetSpaceRegistries();\n"
          "    if (registries == nullptr) { return kFailed; }\n"
          "    auto *registry = registries->at(static_cast<size_t>(gert::OppImplVersionTag::kOpp)).get();\n"
          "    events[1] = reinterpret_cast<uintptr_t>(model_desc);\n"
          "    events[2] = reinterpret_cast<uintptr_t>(registries);\n"
          "    events[3] = reinterpret_cast<uintptr_t>(registry);\n"
          "    if (registry == nullptr) { return kFailed; }\n";
    } else {
      code += "    events[1] = reinterpret_cast<uintptr_t>(chains_[1].GetValue<void *>());\n";
    }
    code +=
        "    ++events[4];\n"
        "    return kOk;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }

 private:
  bool read_model_desc_;
};

inline std::vector<uint8_t> SerializePool(const bg::BufferPool &pool) {
  size_t size = 0U;
  const auto bytes = pool.Serialize(size);
  if (bytes == nullptr) {
    ADD_FAILURE() << "BufferPool serialization failed";
    return {};
  }
  return {bytes.get(), bytes.get() + size};
}

inline ProgramInfo RegistryBindingProgram(const bool bind_registry_directly) {
  ProgramInfo program;
  bg::BufferPool empty;
  program.resources.buffer = SerializePool(empty);
  program.resources.compute_node_infos = SerializePool(empty);
  program.resources.kernel_extend_infos = SerializePool(empty);
  bg::BufferPool model;
  bg::BufferPool::BufId model_id = 0U;
  EXPECT_EQ(BuildModelDescBuffer({}, {}, {}, model, model_id), ge::GRAPH_SUCCESS);
  program.resources.model_desc = SerializePool(model);
  program.chains.resize(bind_registry_directly ? 3U : 2U);
  for (uint32_t i = 0U; i < program.chains.size(); ++i) {
    program.chains[i].index = i;
    program.chains[i].symbol = i + 1U;
  }
  program.chains[0].source_kind = ChainSourceKind::kRuntimeBinding;
  program.chains[0].binding_kind = ChainBindingKind::kRtSession;
  program.chains[0].binding_phase = ChainBindingPhase::kLoad;
  program.chains[1].source_kind = ChainSourceKind::kRuntimeBinding;
  program.chains[1].binding_kind = ChainBindingKind::kModelDesc;
  program.chains[1].binding_phase = ChainBindingPhase::kLoad;
  program.chains[1].source_index = 0;
  if (bind_registry_directly) {
    program.chains[2].source_kind = ChainSourceKind::kRuntimeBinding;
    program.chains[2].binding_kind = ChainBindingKind::kSpaceRegistry;
    program.chains[2].binding_phase = ChainBindingPhase::kLoad;
    program.chains[2].source_index = 0;
  }
  KernelProgramInfo kernel;
  kernel.execution_node_name = "registry_binding_probe";
  kernel.kernel_type = "Om2TestRegistryBindingProbe";
  kernel.impl_type = KernelImplType::kCodegenImpl;
  kernel.output_policy = OutputCreatePolicy::kTemporary;
  kernel.code_builder = std::make_shared<RegistryBindingProbe>(true);
  program.main.kernels.push_back(std::move(kernel));
  program.main.regions.resize(1U);
  program.main.root_region_index = 0U;
  ProgramStep step;
  step.kernel_index = 0U;
  program.main.regions[0].steps.push_back(step);
  return program;
}

inline ProgramInfo LifecycleProgram() {
  ProgramInfo program;
  bg::BufferPool empty;
  program.resources.buffer = SerializePool(empty);
  program.resources.compute_node_infos = SerializePool(empty);
  program.resources.kernel_extend_infos = SerializePool(empty);
  bg::BufferPool model;
  bg::BufferPool::BufId id = 0U;
  EXPECT_EQ(BuildModelDescBuffer({}, {}, {}, model, id), ge::GRAPH_SUCCESS);
  program.resources.model_desc = SerializePool(model);
  program.chains.resize(3U);
  for (uint32_t i = 0U; i < program.chains.size(); ++i) {
    program.chains[i].index = i;
    program.chains[i].symbol = i + 1U;
  }
  program.chains[0].source_kind = ChainSourceKind::kRuntimeBinding;
  program.chains[0].binding_kind = ChainBindingKind::kRtSession;
  program.chains[0].binding_phase = ChainBindingPhase::kLoad;
  program.chains[1].source_kind = ChainSourceKind::kRuntimeBinding;
  program.chains[1].binding_kind = ChainBindingKind::kExternalAllocator;
  // The collector assigns this lifecycle slot to Load; each Run then rebinds it.
  program.chains[1].binding_phase = ChainBindingPhase::kLoad;
  program.chains[2].source_kind = ChainSourceKind::kKernelOutput;
  ProgramBlock *blocks[] = {&program.init, &program.main, &program.deinit};
  for (size_t phase = 0U; phase < 3U; ++phase) {
    KernelProgramInfo kernel;
    kernel.execution_node_name = "lifecycle_\"probe\\phase\n" + std::to_string(phase);
    kernel.kernel_type = "Om2TestLifecycleProbe";
    kernel.impl_type = KernelImplType::kCodegenImpl;
    kernel.output_policy = OutputCreatePolicy::kTemporary;
    kernel.code_builder = std::make_shared<LifecycleProbe>(phase);
    blocks[phase]->kernels.push_back(std::move(kernel));
    blocks[phase]->regions.resize(1U);
    blocks[phase]->root_region_index = 0U;
    ProgramStep step;
    step.kernel_index = 0U;
    blocks[phase]->regions[0].steps.push_back(step);
  }
  return program;
}

}  // namespace program_body_fixture
}  // namespace om2
}  // namespace gert

#endif  // GE_TESTS_OM2_PROGRAM_BODY_CODEGEN_FIXTURE_H_
