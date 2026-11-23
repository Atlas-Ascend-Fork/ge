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
class AllocMemHbmKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() < 2U ||
        context.output_chain_indices.size() != context.input_chain_indices.size() - 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    std::vector<KernelInputPortInfo> inputs = {{"allocator", Om2ValueType::kGertAllocatorPtr}};
    for (size_t i = 1U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"size_" + std::to_string(i - 1U), Om2ValueType::kSizeT});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tensor_data_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AllocMemHbm", AllocMemHbmKernelCodeBuilder);

class CreateHostL2AllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"l1_allocator", Om2ValueType::kGertAllocatorPtr}}, info));
    return SetOutputPortNames(context, {"host_l2_allocator"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateHostL2Allocator", CreateHostL2AllocatorKernelCodeBuilder);

class CreateL1AllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"placement", Om2ValueType::kGertTensorPlacement}}, info));
    return SetOutputPortNames(context, {"l1_allocator"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateL1Allocator", CreateL1AllocatorKernelCodeBuilder);

class SelectL1AllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"placement", Om2ValueType::kGertTensorPlacement},
                                               {"external_allocator", Om2ValueType::kGertAllocatorsPtr},
                                               {"created_allocator", Om2ValueType::kGeAllocatorPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr}},
                                              info));
    RequestInputBinding(context, 1U, ChainBindingKind::kExternalAllocator, ChainBindingPhase::kLoad, info);
    return SetOutputPortNames(context, {"l1_allocator"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SelectL1Allocator", SelectL1AllocatorKernelCodeBuilder);

class SelectL2AllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"stream_id", Om2ValueType::kInt64T},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"l1_allocator", Om2ValueType::kGeAllocatorPtr},
                                               {"l2_allocators", Om2ValueType::kGertAllocatorVectorPtr}},
                                              info));
    return SetOutputPortNames(context, {"l2_allocator"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SelectL2Allocator", SelectL2AllocatorKernelCodeBuilder);

class CreateL2AllocatorsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"stream_num", Om2ValueType::kInt64T}, {"placement", Om2ValueType::kGertTensorPlacement}}, info));
    return SetOutputPortNames(context, {"l2_allocators", "l2_mem_pools"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateL2Allocators", CreateL2AllocatorsKernelCodeBuilder);

class CreateInitL2AllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"l1_allocator", Om2ValueType::kGeAllocatorPtr},
                                               {"l2_allocators", Om2ValueType::kGertAllocatorVectorPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr}},
                                              info));
    return SetOutputPortNames(context, {"init_l2_allocator", "l2_allocators_for_init"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateInitL2Allocator", CreateInitL2AllocatorKernelCodeBuilder);

class AllocBatchHbmKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"allocator", Om2ValueType::kGertAllocatorPtr}, {"sizes", Om2ValueType::kTypedContinuousVectorSizeTPtr}},
        info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AllocBatchHbm", AllocBatchHbmKernelCodeBuilder);

class FreeBatchHbmKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(context, {{"tensor_data", Om2ValueType::kGertTensorDataVectorPtr}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeBatchHbm", FreeBatchHbmKernelCodeBuilder);

class FreeMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"tensor_data", Om2ValueType::kGertTensorDataPtr}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeMemory", FreeMemoryKernelCodeBuilder);

class FreeTensorMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"tensor", Om2ValueType::kGertTensorPtr}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeTensorMemory", FreeTensorMemoryKernelCodeBuilder);

// FreeMemHbm / FreeMemHbmHoldAddr read a variable number of GertTensorData * inputs
// (run_func loops over GetInputNum, see FreeHbmMem/FreeHbmMemHoldAddr in
// runtime/v2/kernel/memory/memory_kernel.cc).
ge::graphStatus CollectVariableGertTensorDataInputs(const KernelCollectContext &context, KernelProgramInfo &info) {
  GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
  std::vector<KernelInputPortInfo> inputs;
  inputs.reserve(context.input_chain_indices.size());
  const auto names = MakeIndexedPortNames("tensor_data_", context.input_chain_indices.size());
  for (const auto &name : names) {
    inputs.push_back({name, Om2ValueType::kGertTensorDataPtr});
  }
  GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
  return SetOutputPortNames(context, {}, info);
}

class FreeMemHbmKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectVariableGertTensorDataInputs(context, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeMemHbm", FreeMemHbmKernelCodeBuilder);

class FreeMemHbmHoldAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectVariableGertTensorDataInputs(context, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeMemHbmHoldAddr", FreeMemHbmHoldAddrKernelCodeBuilder);

class FreeBatchHbmHoldAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(context, {{"tensor_data", Om2ValueType::kGertTensorDataVectorPtr}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeBatchHbmHoldAddr", FreeBatchHbmHoldAddrKernelCodeBuilder);

class FreeMemoryHoldAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"tensor_data", Om2ValueType::kGertTensorDataPtr}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FreeMemoryHoldAddr", FreeMemoryHoldAddrKernelCodeBuilder);

class EmptyTensorDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortNames(context, {}, info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("EmptyTensorData", EmptyTensorDataKernelCodeBuilder);

class ReusableStreamAllocatorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortNames(context, {}, info));
    return SetOutputPortNames(context, {"reusable_stream_allocator"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ReusableStreamAllocator", ReusableStreamAllocatorKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
