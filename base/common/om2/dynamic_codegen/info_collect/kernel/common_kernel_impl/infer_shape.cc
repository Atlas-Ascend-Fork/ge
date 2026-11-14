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
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
class FindInferShapeFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"node_type", Om2ValueType::kCStringPtr}, {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
        info));
    return SetOutputPortNames(context, {"infer_shape_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindInferShapeFunc", FindInferShapeFuncKernelCodeBuilder);

class InferShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    bool symbol_infer_shape = false;
    (void)ge::AttrUtils::GetBool(context.node.GetOpDescBarePtr(), kSymbolInferShape, symbol_infer_shape);
    if (symbol_infer_shape) {
      constexpr size_t kSymbolInferShapeFixedInputCount = 4U;
      if (context.input_chain_indices.size() < kSymbolInferShapeFixedInputCount) {
        return ge::GRAPH_PARAM_INVALID;
      }
      const size_t input_tensor_count = context.input_chain_indices.size() - kSymbolInferShapeFixedInputCount;
      inputs.push_back({"input_data_num", Om2ValueType::kSizeT});
      for (const auto &name : MakeIndexedPortNames("input_tensor_", input_tensor_count)) {
        inputs.push_back({name, Om2ValueType::kGertTensorPtr});
      }
      inputs.insert(inputs.end(), {{"all_symbol_num", Om2ValueType::kSizeT},
                                   {"dfx_input_symbol_info_func", Om2ValueType::kKernelFuncPtr},
                                   {"infer_shape_func", Om2ValueType::kKernelFuncPtr}});
    } else {
      for (const auto &name : MakeIndexedPortNames("input_shape_", context.input_chain_indices.size() - 1U)) {
        inputs.push_back({name, Om2ValueType::kGertStorageShapePtr});
      }
      inputs.push_back({"infer_shape_func", Om2ValueType::kKernelFuncPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("output_shape_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("InferShape", InferShapeKernelCodeBuilder);

// Runtime contract (infer_shape_range.cc): 3n outputs for a node with n outputs --
// [0, n) shape ranges, [n, 2n) min shapes, [2n, 3n) max shapes.
std::vector<std::string> MakeShapeRangeOutputPortNames(const size_t output_count) {
  std::vector<std::string> names;
  names.reserve(output_count);
  const size_t node_output_num = output_count / 3U;
  for (const auto &name : MakeIndexedPortNames("output_shape_range_", node_output_num)) {
    names.push_back(name);
  }
  for (const auto &name : MakeIndexedPortNames("output_min_shape_", node_output_num)) {
    names.push_back(name);
  }
  for (const auto &name : MakeIndexedPortNames("output_max_shape_", node_output_num)) {
    names.push_back(name);
  }
  return names;
}

class FindInferShapeRangeFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"node_type", Om2ValueType::kCStringPtr}, {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
        info));
    return SetOutputPortNames(context, {"infer_shape_range_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindInferShapeRangeFunc", FindInferShapeRangeFuncKernelCodeBuilder);

class InferShapeRangeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    // The op infer-shape-range func reads each leading input as a shape range through the context; the
    // generated code only forwards the chain pointers, so an erased pointer contract is enough here.
    for (const auto &name : MakeIndexedPortNames("input_shape_range_", context.input_chain_indices.size() - 1U)) {
      inputs.push_back({name, Om2ValueType::kVoidPtr});
    }
    inputs.push_back({"infer_shape_range_func", Om2ValueType::kKernelFuncPtr});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeShapeRangeOutputPortNames(context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("InferShapeRange", InferShapeRangeKernelCodeBuilder);

class FindCompatibleInferShapeFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"node_type", Om2ValueType::kCStringPtr}}, info));
    return SetOutputPortNames(context, {"infer_shape_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindCompatibleInferShapeFunc", FindCompatibleInferShapeFuncKernelCodeBuilder);

class FindCompatibleInferShapeRangeFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"node_type", Om2ValueType::kCStringPtr}}, info));
    return SetOutputPortNames(context, {"infer_shape_range_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindCompatibleInferShapeRangeFunc", FindCompatibleInferShapeRangeFuncKernelCodeBuilder);

class CreateTensorRangesAndShapeRangesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    for (const auto &name : MakeIndexedPortNames("input_tensor_", context.input_chain_indices.size())) {
      inputs.push_back({name, Om2ValueType::kGertTensorPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tensor_range_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateTensorRangesAndShapeRanges", CreateTensorRangesAndShapeRangesKernelCodeBuilder);

class CreateOpFromBufferKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != 2U || context.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    // Lowering (bg_compatible_utils.h BuildOpDescBufferConst) feeds exactly two consts:
    // the serialized op-desc buffer and its byte size.
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"op_buffer", Om2ValueType::kBytePtr}, {"op_buffer_size", Om2ValueType::kSizeT}}, info));
    return SetOutputPortInfos(context, {{"op", Om2ValueType::kVoidPtr}}, info);
  }
  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {0U, 1U};
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateOpFromBuffer", CreateOpFromBufferKernelCodeBuilder);

class CompatibleInferShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;  // op + infer_func
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"op", Om2ValueType::kVoidPtr}, {"infer_func", Om2ValueType::kVoidPtr}};
    for (const auto &name :
         MakeIndexedPortNames("input_shape_", context.input_chain_indices.size() - kFixedInputCount)) {
      inputs.push_back({name, Om2ValueType::kGertStorageShapePtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("output_shape_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CompatibleInferShape", CompatibleInferShapeKernelCodeBuilder);

class CompatibleInferShapeRangeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 2U;  // op + infer_func
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"op", Om2ValueType::kVoidPtr}, {"infer_func", Om2ValueType::kVoidPtr}};
    for (const auto &name :
         MakeIndexedPortNames("input_shape_range_", context.input_chain_indices.size() - kFixedInputCount)) {
      inputs.push_back({name, Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeShapeRangeOutputPortNames(context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CompatibleInferShapeRange", CompatibleInferShapeRangeKernelCodeBuilder);

// Runtime contract (runtime/v2/engine/aicore/kernel/set_output_shape_kernel.cc): a third-class op
// (DEPEND_SHAPE_RANGE, e.g. NonZero) writes its output shapes into a device shape buffer allocated by
// AllocMemHbm; this kernel copies the buffer to host and fills the node's StorageShape outputs. It reads
// compute_node_info for the output count and node identity, and writes into graph-bound output slots
// without registering an outputs creator.
class SetOutputShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"shape_buffer", Om2ValueType::kGertTensorDataPtr}}, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("output_shape_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SetOutputShape", SetOutputShapeKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
