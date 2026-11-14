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

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
class UpdateAicpuIoAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs = {{"args_handler", Om2ValueType::kAicpuArgsHandlerPtr}};
    for (size_t i = 1U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"tensor_data_" + std::to_string(i - 1U), Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("updated_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("UpdateAicpuIoAddr", UpdateAicpuIoAddrKernelCodeBuilder);

class AicpuLaunchCCKernelKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kInputCount = 8U;
    if (context.input_chain_indices.size() != kInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"args_handler", Om2ValueType::kAicpuArgsHandlerPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"block_dim", Om2ValueType::kUInt32T},
                                               {"kernel_type", Om2ValueType::kUInt32T},
                                               {"ext_info_handler", Om2ValueType::kGertAicpuExtInfoHandlerPtr},
                                               {"bin_handle", Om2ValueType::kVoidPtr},
                                               {"function_name", Om2ValueType::kCStringPtr},
                                               {"node_type", Om2ValueType::kCStringPtr}},
                                              info));
    return SetOutputPortNames(context, MakeIndexedPortNames("launch_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AicpuLaunchCCKernel", AicpuLaunchCCKernelKernelCodeBuilder);

class AicpuLaunchTfKernelKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kInputCount = 4U;
    if (context.input_chain_indices.size() != kInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"args_handler", Om2ValueType::kAicpuArgsHandlerPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"bin_handle", Om2ValueType::kVoidPtr},
                                               {"node_type", Om2ValueType::kCStringPtr}},
                                              info));
    return SetOutputPortNames(context, MakeIndexedPortNames("launch_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AicpuLaunchTfKernel", AicpuLaunchTfKernelKernelCodeBuilder);

class AllocHostCpuOutputMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"size", Om2ValueType::kSizeT},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}};
    for (size_t i = kFixedInputCount; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"input_context_" + std::to_string(i - kFixedInputCount), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AllocHostCpuOutputMemory", AllocHostCpuOutputMemoryKernelCodeBuilder);

class AicpuHostExecFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.empty() || ((context.input_chain_indices.size() - 1U) % 2U) != 0U ||
        (context.output_chain_indices.size() % 2U) != 0U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t input_num = (context.input_chain_indices.size() - 1U) / 2U;
    const size_t output_num = context.output_chain_indices.size() / 2U;
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs;
    for (size_t i = 0U; i < input_num; ++i) {
      inputs.push_back({"input_shape_" + std::to_string(i), Om2ValueType::kGertStorageShapePtr});
    }
    for (size_t i = 0U; i < input_num; ++i) {
      inputs.push_back({"input_data_" + std::to_string(i), Om2ValueType::kGertTensorDataPtr});
    }
    inputs.push_back({"host_exec_func", Om2ValueType::kAicpuHostProcFuncPtr});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    std::vector<std::string> output_names;
    for (size_t i = 0U; i < output_num; ++i) {
      output_names.push_back("output_shape_" + std::to_string(i));
    }
    for (size_t i = 0U; i < output_num; ++i) {
      output_names.push_back("output_data_" + std::to_string(i));
    }
    return SetOutputPortNames(context, output_names, info);
  }

  std::vector<uint32_t> GetCustomRestoreConstInputIndices() const override {
    return {kKernelLastInputIndex};
  }

  ge::graphStatus RestoreConstInput(const KernelCollectContext &context, const uint32_t input_index,
                                    CustomConstRestoreDescriptor &descriptor) const override {
    if (input_index != context.input_chain_indices.size() - 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const auto *original_type =
        ge::AttrUtils::GetStr(context.node.GetOpDescBarePtr(), ge::ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE);
    if ((original_type == nullptr) || original_type->empty()) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 AicpuHostExecFunc %s has no original type", context.node.GetNamePtr());
      return ge::GRAPH_PARAM_INVALID;
    }
    descriptor.initializer = "aicpu_host_proc_func";
    descriptor.payload.assign(original_type->begin(), original_type->end());
    descriptor.payload.push_back(0U);
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("AicpuHostExecFunc", AicpuHostExecFuncKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
