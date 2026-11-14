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

namespace gert {
namespace om2 {
namespace {
class BuildDualStageAclnnOpFwkDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(context,
                          {{"execute_prepare_func", Om2ValueType::kKernelFuncPtr},
                           {"execute_launch_func", Om2ValueType::kKernelFuncPtr},
                           {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                           {"core_num_infos", Om2ValueType::kGertCoreNumInfosPtr},
                           {"deterministic_config", Om2ValueType::kAclnnDeterministicConfigPtr},
                           {"original_deterministic_config", Om2ValueType::kAclnnOriginalDeterministicConfigPtr}},
                          info));
    return SetOutputPortNames(context, {"aclnn_fwk_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildDualStageAclnnOpFwkData", BuildDualStageAclnnOpFwkDataKernelCodeBuilder);

class BuildSingleStageAclnnOpFwkDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(context,
                          {{"core_num_infos", Om2ValueType::kGertCoreNumInfosPtr},
                           {"deterministic_config", Om2ValueType::kAclnnDeterministicConfigPtr},
                           {"original_deterministic_config", Om2ValueType::kAclnnOriginalDeterministicConfigPtr}},
                          info));
    return SetOutputPortNames(context, {"aclnn_fwk_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildSingleStageAclnnOpFwkData", BuildSingleStageAclnnOpFwkDataKernelCodeBuilder);

class FindOpExe2PhaseFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"node_type", Om2ValueType::kCStringPtr}, {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
        info));
    return SetOutputPortNames(context, {"execute_prepare_func", "execute_launch_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindOpExe2PhaseFunc", FindOpExe2PhaseFuncKernelCodeBuilder);

class FindOpExeFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"node_type", Om2ValueType::kCStringPtr}, {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
        info));
    return SetOutputPortNames(context, {"execute_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindOpExeFunc", FindOpExeFuncKernelCodeBuilder);

class ExecuteOpPrepareKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() < 3U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    const size_t tensor_count = context.input_chain_indices.size() - 3U;
    std::vector<KernelInputPortInfo> inputs;
    for (size_t i = 0U; i < tensor_count; ++i) {
      inputs.push_back({"tensor_" + std::to_string(i), Om2ValueType::kGertTensorPtr});
    }
    inputs.insert(inputs.end(), {{"execute_option", Om2ValueType::kGertOpExecuteOptionsPtr},
                                 {"aclnn_fwk_data", Om2ValueType::kVoidPtr},
                                 {"stream", Om2ValueType::kRtStreamTPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"op_param", "workspace_sizes"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ExecuteOpPrepare", ExecuteOpPrepareKernelCodeBuilder);

class ExecuteOpLaunchKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() < 5U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    const size_t tensor_count = context.input_chain_indices.size() - 5U;
    std::vector<KernelInputPortInfo> inputs;
    for (size_t i = 0U; i < tensor_count; ++i) {
      inputs.push_back({"tensor_" + std::to_string(i), Om2ValueType::kGertTensorPtr});
    }
    inputs.insert(inputs.end(), {{"op_param", Om2ValueType::kVoidPtr},
                                 {"workspace", Om2ValueType::kGertTensorDataVectorPtr},
                                 {"workspace_sizes", Om2ValueType::kTypedContinuousVectorSizeTPtr},
                                 {"stream", Om2ValueType::kRtStreamTPtr},
                                 {"aclnn_fwk_data", Om2ValueType::kVoidPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ExecuteOpLaunch", ExecuteOpLaunchKernelCodeBuilder);

class ExecuteOpFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() < 5U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    const size_t tensor_count = context.input_chain_indices.size() - 5U;
    std::vector<KernelInputPortInfo> inputs;
    for (size_t i = 0U; i < tensor_count; ++i) {
      inputs.push_back({"tensor_" + std::to_string(i), Om2ValueType::kGertTensorPtr});
    }
    // OpExecuteInputExtendIndex: kAllocate/kStream/kExecuteOption/kExecuteFunc/kFwkData.
    // kAllocate is not consumed inside this repo; declared as GertAllocator per its
    // workspace-allocation role in fe operators.
    inputs.insert(inputs.end(), {{"allocator", Om2ValueType::kGertAllocatorPtr},
                                 {"stream", Om2ValueType::kRtStreamTPtr},
                                 {"execute_option", Om2ValueType::kGertOpExecuteOptionsPtr},
                                 {"execute_func", Om2ValueType::kKernelFuncPtr},
                                 {"aclnn_fwk_data", Om2ValueType::kVoidPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"block_memory"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ExecuteOpFunc", ExecuteOpFuncKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
