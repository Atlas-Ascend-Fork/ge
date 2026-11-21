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

#include <cstdint>
#include <string>

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "exe_graph/runtime/runtime_tensor.h"

namespace gert {
namespace om2 {
namespace {
class CreateMemAssignerKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"base_mem", Om2ValueType::kGertTensorDataPtr}}, info));
    return SetOutputPortNames(context, {"mem_assigner"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateMemAssigner", CreateMemAssignerKernelCodeBuilder);

class AssignWeightMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"offset_tensor", Om2ValueType::kGertTensorDataObjectPtr},
                                               {"assigner", Om2ValueType::kVoidPtr},
                                               {"stream_id", Om2ValueType::kInt64T}},
                                              info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }

  std::vector<uint32_t> GetCustomRestoreConstInputIndices() const override {
    return {0U};
  }

  ge::graphStatus RestoreConstInput(const KernelCollectContext &context, const uint32_t input_index,
                                    CustomConstRestoreDescriptor &descriptor) const override {
    if (input_index != 0U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    auto *const tensor = context.input_chains[input_index].GetValue<gert::TensorData *>();
    GE_ASSERT_NOTNULL(tensor);
    const auto placement = tensor->GetPlacement();
    const auto offset = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(tensor->GetAddr()));
    const auto size = static_cast<uint64_t>(tensor->GetSize());
    const std::string payload =
        std::to_string(offset) + "," + std::to_string(size) + "," + std::to_string(static_cast<int32_t>(placement));
    descriptor.initializer = "tensor_data_offset";
    descriptor.payload_is_string = true;
    descriptor.payload.assign(payload.cbegin(), payload.cend());
    descriptor.payload.push_back(0U);
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("AssignWeightMemory", AssignWeightMemoryKernelCodeBuilder);

class GetOrCreateWeightMemKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"required_size", Om2ValueType::kSizeT},
                                               {"appointed_weight", Om2ValueType::kOuterWeightMemPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}},
                                              info));
    RequestInputBinding(context, 1U, ChainBindingKind::kOuterWeightMem, ChainBindingPhase::kLoad, info);
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetOrCreateWeightMem", GetOrCreateWeightMemKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
