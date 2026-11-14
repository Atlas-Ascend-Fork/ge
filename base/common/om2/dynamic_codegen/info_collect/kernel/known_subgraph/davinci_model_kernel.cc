/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"

#include <string>

#include "securec.h"

#include "common/checker.h"
#include "common/helper/om2/om2_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "core/builder/node_types.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
// DavinciModelCreate input count per DavinciModelCreateInput enum (kModelAddr .. kCustomOpRegistry).
constexpr size_t kCreateInputCount = 17U;

// The restore context only carries the consuming kernel node; walk the data edge backwards to
// the Const producer, mirroring ExecutionPlanCollector's own producer lookup.
ge::graphStatus MakeProcessLocalPointerDescriptor(const ge::FastNode &node, const uint32_t input_index,
                                                  CustomConstRestoreDescriptor &descriptor) {
  const ge::FastEdge *producer_edge = nullptr;
  for (const auto *const edge : node.GetAllInDataEdgesRef()) {
    if ((edge != nullptr) && (edge->dst_input == static_cast<int32_t>(input_index))) {
      producer_edge = edge;
      break;
    }
  }
  GE_ASSERT_NOTNULL(producer_edge, "OM2 DavinciModel const input %u of %s has no producer edge", input_index,
                    node.GetNamePtr());
  const auto *const producer = producer_edge->src;
  GE_ASSERT_NOTNULL(producer);
  GE_ASSERT_TRUE(IsConstType(producer->GetTypePtr()), "OM2 DavinciModel const input %u of %s has non-Const producer %s",
                 input_index, node.GetNamePtr(), producer->GetNamePtr());
  ge::Buffer buffer;
  if (!ge::AttrUtils::GetZeroCopyBytes(producer->GetOpDescBarePtr(), kConstValue, buffer)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 DavinciModel const node %s has no value bytes", producer->GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  if (buffer.size() != sizeof(void *)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 DavinciModel const node %s has invalid pointer size %zu",
           producer->GetNamePtr(), buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  uintptr_t address = 0U;
  GE_ASSERT_EOK(memcpy_s(&address, sizeof(address), buffer.data(), sizeof(address)));
  if (address == 0U) {
    descriptor.initializer = "null_pointer";
    descriptor.payload = {0U};
    return ge::GRAPH_SUCCESS;
  }
  descriptor.initializer = "process_local_pointer";
  const auto payload = std::to_string(static_cast<uint64_t>(address));
  descriptor.payload.assign(payload.cbegin(), payload.cend());
  descriptor.payload.push_back(0U);
  return ge::GRAPH_SUCCESS;
}

class DavinciModelCreateKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    // The GeModel* input is serialized as a process-local pointer; the pointed-to object is
    // owned by the in-process GeRootModel, so offline packaged OM2 artifacts cannot carry it.
    GE_ASSERT_TRUE(ge::IsOm2OnlineMode(), "OM2 DavinciModelCreate %s is only supported in online mode",
                   context.node.GetNamePtr());
    GE_ASSERT_TRUE(context.input_chain_indices.size() == kCreateInputCount,
                   "OM2 DavinciModelCreate %s has invalid input count %zu", context.node.GetNamePtr(),
                   context.input_chain_indices.size());
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(context,
                          {{"model_addr", Om2ValueType::kGeDavinciModelPtr},
                           {"session_id", Om2ValueType::kUInt64T},
                           {"assign_mem", Om2ValueType::kGertTensorDataPtr},
                           {"step_id", Om2ValueType::kVoidPtr},
                           {"root_graph_id", Om2ValueType::kUInt32T},
                           {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2ArrayPtr},
                           {"file_constant_weight_dir", Om2ValueType::kCStringPtr},
                           {"rt_stream_reuse", Om2ValueType::kGertAllocatorPtr},
                           {"fixed_mem_addr", Om2ValueType::kUInt64T},
                           {"fixed_mem_size", Om2ValueType::kSizeT},
                           {"fixed_mem_tensor_from_init", Om2ValueType::kGertTensorDataPtr},
                           {"p2p_fixed_mem_addr", Om2ValueType::kUInt64T},
                           {"p2p_fixed_mem_size", Om2ValueType::kSizeT},
                           {"p2p_fixed_mem_tensor_from_init", Om2ValueType::kGertTensorDataPtr},
                           {"frozen_input_indicies", Om2ValueType::kCStringPtr},
                           {"file_constant_user_mem", Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr},
                           {"custom_op_registry", Om2ValueType::kGertCustomOpRegistryPtr}},
                          info));
    return SetOutputPortNames(context, {"davinci_model"}, info);
  }

  std::vector<uint32_t> GetCustomRestoreConstInputIndices() const override {
    return {0U};
  }

  ge::graphStatus RestoreConstInput(const KernelCollectContext &context, const uint32_t input_index,
                                    CustomConstRestoreDescriptor &descriptor) const override {
    if (input_index != 0U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    return MakeProcessLocalPointerDescriptor(context.node, input_index, descriptor);
  }
};
REGISTER_KERNEL_CODE_BUILDER("DavinciModelCreate", DavinciModelCreateKernelCodeBuilder);

class DavinciModelExecuteKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 4U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    const size_t tensor_count = context.input_chain_indices.size() - kFixedInputCount;
    std::vector<KernelInputPortInfo> inputs = {{"davinci_model", Om2ValueType::kGeDavinciModelPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"input_num", Om2ValueType::kSizeT},
                                               {"output_num", Om2ValueType::kSizeT}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < tensor_count; ++i) {
      inputs.push_back({"tensor_data_" + std::to_string(i), Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"result"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("DavinciModelExecute", DavinciModelExecuteKernelCodeBuilder);

class DavinciModelUpdateWorkspacesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    constexpr size_t kInputsPerWorkspace = 2U;
    if ((context.input_chain_indices.size() < kFixedInputCount) ||
        (((context.input_chain_indices.size() - kFixedInputCount) % kInputsPerWorkspace) != 0U)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t workspace_count = (context.input_chain_indices.size() - kFixedInputCount) / kInputsPerWorkspace;
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs = {{"davinci_model", Om2ValueType::kGeDavinciModelPtr},
                                               {"workspace_num", Om2ValueType::kSizeT}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < workspace_count; ++i) {
      const auto suffix = std::to_string(i);
      inputs.push_back({"memory_type_" + suffix, Om2ValueType::kUInt64T});
      inputs.push_back({"tensor_data_" + suffix, Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"result"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("DavinciModelUpdateWorkspaces", DavinciModelUpdateWorkspacesKernelCodeBuilder);

class DavinciModelGetRunAddressKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t output_count = context.input_chain_indices.size() - kFixedInputCount;
    if (context.output_chain_indices.size() != output_count) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    std::vector<KernelInputPortInfo> inputs = {{"davinci_model", Om2ValueType::kGeDavinciModelPtr},
                                               {"stream_id", Om2ValueType::kInt64T}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < output_count; ++i) {
      inputs.push_back({"memory_base_type_offset_" + std::to_string(i), Om2ValueType::kMemoryBaseTypeOffsetPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("device_tensor_data_", output_count), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("DavinciModelGetRunAddress", DavinciModelGetRunAddressKernelCodeBuilder);

class DavinciModelFinalizerKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_TRUE(!context.input_chain_indices.empty(), "OM2 DavinciModelFinalizer %s has no input",
                   context.node.GetNamePtr());
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"davinci_model_" + std::to_string(i), Om2ValueType::kGeDavinciModelPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("DavinciModelFinalizer", DavinciModelFinalizerKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
