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
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/tensor.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"

namespace gert {
namespace om2 {
namespace {
class BuildTensorKernelCodeBuilder : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    // BuildTensor and BuildRefTensor use their input ports for shape, storage and format.
    // Their shared output creator allocates an empty Tensor without reading source metadata.
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"shape", Om2ValueType::kGertStorageShapePtr},
                                               {"tensor_data", Om2ValueType::kGertTensorDataPtr},
                                               {"tensor_attr", Om2ValueType::kGertBuildTensorAttrPtr}},
                                              info));
    return SetOutputPortNames(context, {"tensor"}, info);
  }

  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {2U};
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildTensor", BuildTensorKernelCodeBuilder);

class BuildRefTensorKernelCodeBuilder final : public BuildTensorKernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(BuildTensorKernelCodeBuilder::Collect(context, info));
    constexpr int32_t kShapePort = 0;
    constexpr int32_t kTensorDataPort = 1;
    for (const int32_t port : {kShapePort, kTensorDataPort}) {
      const auto *edge = context.node.GetInDataEdgeByIndex(port);
      if (edge == nullptr || edge->src->GetType() != "Const") {
        continue;
      }
      ge::Buffer value;
      GE_ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(edge->src->GetOpDescPtr(), kConstValue, value));
      const bool valid_size = port == kShapePort
                                  ? value.GetSize() == sizeof(StorageShape) || value.GetSize() == sizeof(Tensor)
                                  : value.GetSize() == sizeof(GertTensorData);
      GE_ASSERT_TRUE(valid_size, "Invalid Const %s size %zu at BuildRefTensor %s input %d", edge->src->GetNamePtr(),
                     value.GetSize(), context.node.GetNamePtr(), port);
    }
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildRefTensor", BuildRefTensorKernelCodeBuilder);

class SplitDataTensorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"tensor", Om2ValueType::kGertTensorPtr}, {"allocator", Om2ValueType::kGertAllocatorPtr}}, info));
    return SetOutputPortNames(context, {"shape", "tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SplitDataTensor", SplitDataTensorKernelCodeBuilder);

class SplitConstTensorKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"tensor", Om2ValueType::kGertTensorPtr}, {"stream_id", Om2ValueType::kInt64T}}, info));
    return SetOutputPortNames(context, {"shape", "tensor_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SplitConstTensor", SplitConstTensorKernelCodeBuilder);

class BuildShapeTensorDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != context.output_chain_indices.size()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"shape_" + std::to_string(i), Om2ValueType::kGertStorageShapePtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tensor_data_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildShapeTensorData", BuildShapeTensorDataKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
