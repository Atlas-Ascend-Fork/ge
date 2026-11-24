/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_H_

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/om2/dynamic_codegen/om2_value.h"

namespace gert {
namespace om2 {
constexpr uint32_t kInvalidProgramIndex = std::numeric_limits<uint32_t>::max();

enum class OutputCreatePolicy : uint8_t { kGraphBound, kOutputsCreator, kRuntimeAllocated, kTemporary, kCodegenImpl };

enum class KernelImplType : uint8_t { kEmptyImpl, kRunFuncImpl, kCodegenImpl };

class KernelCodeBuilder;

enum class ChainSourceKind : uint8_t {
  kUninitialized,
  kConstant,
  kModelInput,
  kModelOutput,
  kRuntimeBinding,
  kKernelOutput,
  kSessionId,
  kUnresolvedFeed,
  kInputBindingRequest
};

// A chain can be initialized either from an embedded constant, from the model invocation, or
// from the model lifecycle.  This is deliberately independent of the C++ value type.
enum class ChainBindingPhase : uint8_t { kNone, kLoad, kRun };

// The lifecycle source is still distinct from the C++ ABI type supplied by a kernel
// collector.  This category describes which ProgramBody binding slot owns the value.
// Keeping it on the normalized chain prevents codegen from inferring ownership from
// collector semantic names.
enum class ChainBindingKind : uint8_t {
  kNone,
  kStream,
  kExternalAllocator,
  kRuntimeEvents,
  kNotifies,
  kRtSession,
  kOuterWeightMem,
  kModelDesc,
  kSpaceRegistry,
  kCustomOpRegistry,
  kBinary,
  kGeKernelBinary
};

constexpr int64_t kInvalidChainSourceIndex = std::numeric_limits<int64_t>::min();

struct KernelPortInfo {
  uint32_t chain_index = kInvalidProgramIndex;
  std::string semantic_name;
  Om2ValueType value_type = Om2ValueType::kUnknown;
};

struct ChainUpdate {
  uint32_t chain_index = kInvalidProgramIndex;
  ChainSourceKind source_kind = ChainSourceKind::kUninitialized;
  ChainBindingPhase binding_phase = ChainBindingPhase::kNone;
  ChainBindingKind binding_kind = ChainBindingKind::kNone;
  union {
    int64_t source_index = kInvalidChainSourceIndex;
    // Active for kInputBindingRequest, which is resolved during collection.
    int64_t input_port;
  };
};

struct CustomConstRestoreDescriptor {
  uint32_t chain_index = kInvalidProgramIndex;
  std::string producer_name;
  // Name of the generated initializer.  The code generator maps this stable
  // identifier to a ProgramBody initializer function.
  std::string initializer;
  bool payload_is_string = true;
  // Serialized, process-independent payload consumed by the initializer.
  std::vector<uint8_t> payload;
};

struct KernelProgramInfo {
  std::string execution_node_name;
  std::string kernel_type;
  std::vector<KernelPortInfo> inputs;
  std::vector<KernelPortInfo> outputs;
  // Const inputs restored for a codegen collector are consumed during collection only. They
  // must not be emitted as runtime Const chains in the generated ProgramBody.
  std::vector<uint32_t> compile_time_const_chain_indices;
  uint32_t compute_node_index = kInvalidProgramIndex;
  OutputCreatePolicy output_policy = OutputCreatePolicy::kGraphBound;
  bool require_compute_node_info = false;
  KernelImplType impl_type = KernelImplType::kRunFuncImpl;
  // Priority written by RT2 NodePriorityCalculator (node attr "priority", smaller runs first).
  // Consumed by the multithread program plan for NORMAL-lane affinity.
  int64_t priority = std::numeric_limits<int64_t>::max();
  std::vector<ChainUpdate> chain_updates;
  std::shared_ptr<KernelCodeBuilder> code_builder;
  // Const inputs whose process-local value is reconstructed by a custom
  // ProgramBody initializer instead of Om2ConstValue parsing.
  std::vector<CustomConstRestoreDescriptor> custom_const_restore_descriptors;
  std::unordered_map<uint32_t, std::string> binary_resource_keys;
};

struct ChainInfo {
  uint32_t index = kInvalidProgramIndex;
  uint64_t symbol = 0U;
  ChainSourceKind source_kind = ChainSourceKind::kUninitialized;
  ChainBindingPhase binding_phase = ChainBindingPhase::kNone;
  ChainBindingKind binding_kind = ChainBindingKind::kNone;
  int64_t source_index = kInvalidChainSourceIndex;
};

struct ConstChainInfo {
  uint32_t chain_index = kInvalidProgramIndex;
  std::string producer_name;
  Om2ConstValue value;
};

enum class ProgramStepKind : uint8_t { kKernel, kIf, kWhile };

enum class ProgramPlanType : uint8_t { kSequential, kStructuredTopological, kStructuredTopologicalPriority };

struct IfMergeInfo {
  std::string join_node_name;
  std::vector<std::string> branch_exit_node_names;
};

struct IfProgramInfo {
  std::string execution_node_name;
  uint32_t condition_region_index = kInvalidProgramIndex;
  uint32_t selector_chain_index = kInvalidProgramIndex;
  // The canonical If control contract reads its selector as uint32_t.  Keep this
  // separate from the chain source/lifecycle metadata: the selector may come
  // from a kernel output, a model input, or a constant.
  Om2ValueType selector_value_type = Om2ValueType::kUnknown;
  struct Branch {
    uint32_t selector_value = kInvalidProgramIndex;
    uint32_t region_index = kInvalidProgramIndex;
    std::string entry_node_name;
    std::string exit_node_name;
    std::vector<std::string> guarded_entry_node_names;
    std::vector<std::string> completion_node_names;
  };
  std::vector<Branch> branches;
  IfMergeInfo merge_info;
  std::vector<std::string> parent_predecessor_node_names;
  std::vector<std::string> parent_continuation_node_names;
};

struct WhileProgramInfo {
  uint32_t condition_region_index = kInvalidProgramIndex;
  uint32_t body_region_index = kInvalidProgramIndex;
  uint32_t condition_chain_index = kInvalidProgramIndex;
};

struct ProgramStep {
  ProgramStepKind kind = ProgramStepKind::kKernel;
  uint32_t kernel_index = kInvalidProgramIndex;
  uint32_t condition_region_index = kInvalidProgramIndex;
  IfProgramInfo if_info;
  WhileProgramInfo while_info;
};

struct ProgramRegion {
  uint32_t index = kInvalidProgramIndex;
  std::string graph_name;
  std::vector<ProgramStep> steps;
};

struct KernelDependency {
  uint32_t predecessor = kInvalidProgramIndex;
  uint32_t successor = kInvalidProgramIndex;
};

struct ProgramBlock {
  ProgramPlanType plan_type = ProgramPlanType::kSequential;
  std::vector<KernelProgramInfo> kernels;
  std::vector<ProgramRegion> regions;
  std::vector<KernelDependency> kernel_dependencies;
  uint32_t root_region_index = kInvalidProgramIndex;
};

struct ModelResourceInfo {
  std::vector<uint8_t> buffer;
  std::vector<uint8_t> compute_node_infos;
  std::vector<uint8_t> kernel_extend_infos;
  std::vector<uint8_t> model_desc;
};

struct ProgramInfo {
  ProgramBlock init;
  ProgramBlock main;
  ProgramBlock deinit;
  std::vector<ChainInfo> chains;
  std::vector<ConstChainInfo> const_chains;
  std::vector<CustomConstRestoreDescriptor> custom_const_chains;
  std::unordered_map<uint32_t, std::string> binary_resource_keys;
  ModelResourceInfo resources;
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_H_
