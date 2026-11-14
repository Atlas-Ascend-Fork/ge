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
class CalcDeviceCopySizesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr},
                                               {"data_type", Om2ValueType::kGeDataType},
                                               {"storage_shape", Om2ValueType::kGertStorageShapePtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"original_tensor_size", Om2ValueType::kSizeT}},
                                              info));
    return SetOutputPortInfos(context, {{"alloc_size", Om2ValueType::kSizeT}, {"copy_size", Om2ValueType::kSizeT}},
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CalcDeviceCopySizes", CalcDeviceCopySizesKernelCodeBuilder);

class LaunchH2DCopyKernelCodeBuilder : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"dst_address", Om2ValueType::kGertTensorDataPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"tensor_size", Om2ValueType::kSizeT}},
                                              info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("LaunchH2DCopy", LaunchH2DCopyKernelCodeBuilder);

class ShareH2DCopyResultKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"dst_address", Om2ValueType::kGertTensorDataPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"tensor_size", Om2ValueType::kSizeT}},
                                              info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ShareH2DCopyResult", ShareH2DCopyResultKernelCodeBuilder);

class MakeSureTensorAtDeviceKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    constexpr size_t kInputsPerOutput = 4U;
    if ((context.input_chain_indices.size() < kFixedInputCount) ||
        (((context.input_chain_indices.size() - kFixedInputCount) % kInputsPerOutput) != 0U)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t output_count = (context.input_chain_indices.size() - kFixedInputCount) / kInputsPerOutput;
    if (context.output_chain_indices.size() != output_count) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < output_count; ++i) {
      const auto suffix = std::to_string(i);
      inputs.push_back({"tensor_data_" + suffix, Om2ValueType::kGertTensorDataPtr});
      inputs.push_back({"tensor_size_" + suffix, Om2ValueType::kSizeT});
      inputs.push_back({"storage_shape_" + suffix, Om2ValueType::kGertStorageShapePtr});
      inputs.push_back({"data_type_" + suffix, Om2ValueType::kGeDataType});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("device_tensor_data_", output_count), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("MakeSureTensorAtDevice", MakeSureTensorAtDeviceKernelCodeBuilder);

class CopyH2DKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    constexpr size_t kInputsPerOutput = 4U;
    if ((context.input_chain_indices.size() < kFixedInputCount) ||
        (((context.input_chain_indices.size() - kFixedInputCount) % kInputsPerOutput) != 0U)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t output_count = (context.input_chain_indices.size() - kFixedInputCount) / kInputsPerOutput;
    if (context.output_chain_indices.size() != output_count) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < output_count; ++i) {
      const auto suffix = std::to_string(i);
      inputs.push_back({"tensor_data_" + suffix, Om2ValueType::kGertTensorDataPtr});
      inputs.push_back({"tensor_size_" + suffix, Om2ValueType::kSizeT});
      inputs.push_back({"storage_shape_" + suffix, Om2ValueType::kGertStorageShapePtr});
      inputs.push_back({"data_type_" + suffix, Om2ValueType::kGeDataType});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortInfos(
        context, MakeTypedPortInfos("device_tensor_data_", output_count, Om2ValueType::kGertTensorDataPtr), info);
  }

 private:
  static std::vector<KernelInputPortInfo> MakeTypedPortInfos(const char *const prefix, const size_t count,
                                                             const Om2ValueType value_type) {
    std::vector<KernelInputPortInfo> infos;
    infos.reserve(count);
    for (size_t i = 0U; i < count; ++i) {
      infos.push_back({std::string(prefix) + std::to_string(i), value_type});
    }
    return infos;
  }
};
REGISTER_KERNEL_CODE_BUILDER("CopyH2D", CopyH2DKernelCodeBuilder);

class CopyD2HKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"src_length", Om2ValueType::kSizeT},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}},
                                              info));
    return SetOutputPortInfos(context, {{"host_tensor_data", Om2ValueType::kGertTensorDataPtr}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CopyD2H", CopyD2HKernelCodeBuilder);

class CopyTensorDataH2HKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"src_shape", Om2ValueType::kGertStorageShapePtr},
                                               {"src_data_type", Om2ValueType::kGeDataType},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}},
                                              info));
    return SetOutputPortNames(context, {"host_tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CopyTensorDataH2H", CopyTensorDataH2HKernelCodeBuilder);

class CopyD2DKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"src_address", Om2ValueType::kGertTensorDataPtr},
                                               {"dst_address", Om2ValueType::kGertTensorDataPtr},
                                               {"tensor_size", Om2ValueType::kSizeT},
                                               {"stream", Om2ValueType::kRtStreamTPtr}},
                                              info));
    // Two CopyD2D forms exist in the lowering. The data form (CreateSingleDataOutput) carries one
    // device_tensor_data output built by the runtime outputs creator. The identity converter emits a
    // void order-holder form (CreateVoid, e.g. ScatterNdUpdate_Identity2): zero data outputs, the copy
    // runs for its side effect only and the destination address flows through the separate AllocMemHbm
    // chain. The runtime creator no-ops on zero outputs, so just declare what the node actually has.
    if (context.output_chain_indices.empty()) {
      return SetOutputPortNames(context, {}, info);
    }
    return SetOutputPortNames(context, {"device_tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CopyD2D", CopyD2DKernelCodeBuilder);

class MakeSureTensorAtHostKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;
    constexpr size_t kInputsPerOutput = 2U;
    if ((context.input_chain_indices.size() < kFixedInputCount) ||
        (((context.input_chain_indices.size() - kFixedInputCount) % kInputsPerOutput) != 0U)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const size_t output_count = (context.input_chain_indices.size() - kFixedInputCount) / kInputsPerOutput;
    if (context.output_chain_indices.size() != output_count) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"stream", Om2ValueType::kRtStreamTPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}};
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < output_count; ++i) {
      const auto suffix = std::to_string(i);
      inputs.push_back({"tensor_data_" + suffix, Om2ValueType::kGertTensorDataPtr});
      inputs.push_back({"tensor_size_" + suffix, Om2ValueType::kSizeT});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("host_tensor_data_", output_count), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("MakeSureTensorAtHost", MakeSureTensorAtHostKernelCodeBuilder);
REGISTER_KERNEL_CODE_BUILDER("MakeSureTensorAtHostWithoutSync", MakeSureTensorAtHostKernelCodeBuilder);

class EnsureTensorAtOutMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"shape", Om2ValueType::kGertStorageShapePtr},
                                               {"tensor_data", Om2ValueType::kGertTensorDataPtr},
                                               {"tensor_attr", Om2ValueType::kGertBuildTensorAttrPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"output_data", Om2ValueType::kGertTensorPtr}},
                                              info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("EnsureTensorAtOutMemory", EnsureTensorAtOutMemoryKernelCodeBuilder);

class SinkWeightDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"weight_data", Om2ValueType::kGertTensorDataPtr},
                                               {"device_base_addr", Om2ValueType::kGertTensorDataPtr},
                                               {"stream_id", Om2ValueType::kInt64T},
                                               {"stream", Om2ValueType::kRtStreamTPtr}},
                                              info));
    return SetOutputPortNames(context, {"tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SinkWeightData", SinkWeightDataKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
