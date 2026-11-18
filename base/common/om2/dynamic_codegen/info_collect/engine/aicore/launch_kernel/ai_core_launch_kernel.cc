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
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"

namespace gert {
namespace om2 {
namespace {
class ReportRollbackErrorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs;
    for (const auto &name : MakeIndexedPortNames("input_", context.input_chain_indices.size())) {
      inputs.push_back({name, Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ReportRollbackError", ReportRollbackErrorKernelCodeBuilder);

std::vector<KernelInputPortInfo> MakeLaunchV2Inputs() {
  return {{"stream", Om2ValueType::kRtStreamTPtr},
          {"block_dim", Om2ValueType::kUInt32T},
          {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
          {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
          {"launch_attrs", Om2ValueType::kAclrtLaunchKernelAttrPtr},
          {"launch_config", Om2ValueType::kAclrtLaunchKernelCfgPtr},
          {"io_num", Om2ValueType::kSizeT},
          {"schedule_mode", Om2ValueType::kUInt32T},
          {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
          {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
          {"local_memory_size", Om2ValueType::kUInt32T},
          {"func_handle", Om2ValueType::kVoidPtr}};
}

class LaunchKernelV2KernelCodeBuilder : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectLaunch(context, false, info);
  }

  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {static_cast<uint32_t>(kernel::InputCommon::kIoNum)};
  }

 protected:
  ge::graphStatus CollectLaunch(const KernelCollectContext &context, const bool is_mix, KernelProgramInfo &info) const {
    constexpr size_t kPrefixCount = static_cast<size_t>(kernel::InputCommon::kNum);
    constexpr size_t kMixTailCount = 4U;
    GE_ASSERT_TRUE(context.input_chain_indices.size() >= kPrefixCount);
    GE_ASSERT_TRUE(context.input_chains.size() > static_cast<size_t>(kernel::InputCommon::kIoNum));
    const auto io_num = context.input_chains[static_cast<size_t>(kernel::InputCommon::kIoNum)].GetValue<size_t>();
    const auto remaining = context.input_chain_indices.size() - kPrefixCount;
    GE_ASSERT_TRUE(io_num <= remaining / 2U);
    const auto tail_count = remaining - io_num * 2U;
    GE_ASSERT_TRUE(is_mix ? (tail_count == kMixTailCount) : (tail_count <= kMixTailCount));
    auto inputs = MakeLaunchV2Inputs();
    GE_ASSERT_TRUE(inputs.size() == kPrefixCount);
    for (const auto &name : MakeIndexedPortNames("io_address_", io_num)) {
      inputs.push_back({name, Om2ValueType::kGertTensorDataPtr});
    }
    for (const auto &name : MakeIndexedPortNames("io_shape_", io_num)) {
      inputs.push_back({name, Om2ValueType::kGertStorageShapePtr});
    }
    const std::vector<KernelInputPortInfo> tail = {{"mix_args", Om2ValueType::kGertMixCoreArgsPtr},
                                                   {"sub_stream", Om2ValueType::kRtStreamTPtr},
                                                   {"notify_0", Om2ValueType::kRtNotifyTPtr},
                                                   {"notify_1", Om2ValueType::kRtNotifyTPtr}};
    // BuildLaunchCommonTail can leave a partial tail when it falls back to ordinary launch.
    inputs.insert(inputs.end(), tail.cbegin(), tail.cbegin() + tail_count);
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchKernelV2", LaunchKernelV2KernelCodeBuilder);

class LaunchMixKernelV2KernelCodeBuilder final : public LaunchKernelV2KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectLaunch(context, true, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchMixKernelV2", LaunchMixKernelV2KernelCodeBuilder);

class AtomicLaunchKernelV2KernelCodeBuilder final : public LaunchKernelV2KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kPrefixCount = static_cast<size_t>(kernel::WithAtomic::kIoAddrs);
    GE_ASSERT_TRUE(context.input_chain_indices.size() > kPrefixCount);
    GE_ASSERT_TRUE(context.input_chains.size() > static_cast<size_t>(kernel::InputCommon::kIoNum));
    const auto io_num = context.input_chains[static_cast<size_t>(kernel::InputCommon::kIoNum)].GetValue<size_t>();
    GE_ASSERT_TRUE(io_num == context.input_chain_indices.size() - kPrefixCount - 1U);
    auto inputs = MakeLaunchV2Inputs();
    inputs.push_back({"workspace_indexes", Om2ValueType::kTypedContinuousVectorInt64Ptr});
    GE_ASSERT_TRUE(inputs.size() == kPrefixCount);
    for (const auto &name : MakeIndexedPortNames("clean_output_address_", io_num)) {
      inputs.push_back({name, Om2ValueType::kGertTensorDataPtr});
    }
    inputs.push_back({"clean_workspace_address", Om2ValueType::kGertTensorDataVectorPtr});
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AtomicLaunchKernelV2", AtomicLaunchKernelV2KernelCodeBuilder);

class LaunchKernelWithHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 13U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T},
                                               {"tiling_key", Om2ValueType::kSizeT},
                                               {"node_info", Om2ValueType::kCStringPtr}};
    auto instance_inputs =
        MakeIndexedPortNames("io_address_or_shape_", context.input_chain_indices.size() - kFixedInputCount);
    for (const auto &instance_input : instance_inputs) {
      inputs.push_back({instance_input, Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchKernelWithHandle", LaunchKernelWithHandleKernelCodeBuilder);

class LaunchKernelWithFlagKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 11U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T}};
    for (size_t i = kFixedInputCount; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"io_address_or_shape_" + std::to_string(i - kFixedInputCount), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchKernelWithFlag", LaunchKernelWithFlagKernelCodeBuilder);

class AtomicLaunchKernelWithFlagKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 13U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T},
                                               {"workspace_indexes", Om2ValueType::kTypedContinuousVectorInt64Ptr}};
    const size_t clean_output_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < clean_output_count; ++i) {
      inputs.push_back({"clean_output_address_" + std::to_string(i), Om2ValueType::kGertTensorDataPtr});
    }
    inputs.push_back({"clean_workspace_address", Om2ValueType::kGertTensorDataVectorPtr});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AtomicLaunchKernelWithFlag", AtomicLaunchKernelWithFlagKernelCodeBuilder);

class AtomicLaunchKernelWithHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 14U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T},
                                               {"tiling_key", Om2ValueType::kSizeT},
                                               {"workspace_indexes", Om2ValueType::kTypedContinuousVectorInt64Ptr}};
    const size_t clean_output_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < clean_output_count; ++i) {
      inputs.push_back({"clean_output_address_" + std::to_string(i), Om2ValueType::kGertTensorDataPtr});
    }
    inputs.push_back({"clean_workspace_address", Om2ValueType::kGertTensorDataVectorPtr});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AtomicLaunchKernelWithHandle", AtomicLaunchKernelWithHandleKernelCodeBuilder);

class MixLaunchKernelWithHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kPrefixInputCount = 13U;
    constexpr size_t kTailInputCount = 4U;
    if (context.input_chain_indices.size() < kPrefixInputCount + kTailInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T},
                                               {"tiling_key", Om2ValueType::kSizeT},
                                               {"node_info", Om2ValueType::kCStringPtr}};
    const size_t io_value_count = context.input_chain_indices.size() - kPrefixInputCount - kTailInputCount;
    for (size_t i = 0U; i < io_value_count; ++i) {
      inputs.push_back({"io_address_or_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"mix_args", Om2ValueType::kGertMixCoreArgsPtr},
                                 {"sub_stream", Om2ValueType::kRtStreamTPtr},
                                 {"notify_0", Om2ValueType::kVoidPtr},
                                 {"notify_1", Om2ValueType::kVoidPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchMixKernelWithHandle", MixLaunchKernelWithHandleKernelCodeBuilder);

class MixLaunchKernelWithFlagKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kPrefixInputCount = 11U;
    constexpr size_t kTailInputCount = 4U;
    if (context.input_chain_indices.size() < kPrefixInputCount + kTailInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"binary_handle", Om2ValueType::kVoidPtr},
                                               {"block_dim", Om2ValueType::kUInt64T},
                                               {"workspace_data", Om2ValueType::kGertTensorDataVectorPtr},
                                               {"shape_buffer_data", Om2ValueType::kGertTensorDataPtr},
                                               {"task_config", Om2ValueType::kRtTaskCfgInfoTPtr},
                                               {"io_num", Om2ValueType::kSizeT},
                                               {"schedule_mode", Om2ValueType::kUInt32T},
                                               {"dfx_arg", Om2ValueType::kGertDfxExeArgPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"local_memory_size", Om2ValueType::kUInt32T}};
    const size_t io_value_count = context.input_chain_indices.size() - kPrefixInputCount - kTailInputCount;
    for (size_t i = 0U; i < io_value_count; ++i) {
      inputs.push_back({"io_address_or_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"mix_args", Om2ValueType::kGertMixCoreArgsPtr},
                                 {"sub_stream", Om2ValueType::kRtStreamTPtr},
                                 {"notify_0", Om2ValueType::kVoidPtr},
                                 {"notify_1", Om2ValueType::kVoidPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchMixKernelWithFlag", MixLaunchKernelWithFlagKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
