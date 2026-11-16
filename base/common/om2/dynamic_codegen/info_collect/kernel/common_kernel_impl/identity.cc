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
class IdentityAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != context.output_chain_indices.size()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"address_" + std::to_string(i), Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("address_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("IdentityAddr", IdentityAddrKernelCodeBuilder);

// Runtime contract (runtime/v2/kernel/common_kernel_impl/identity.cc): n StorageShape inputs are copied
// to n StorageShape outputs. The outputs creator reads the input descs through compute_node_info
// (branch/loop NetOutput nodes have no output descs of their own), so require_compute_node_info is true.
class IdentityShapeKernelCodeBuilder final : public KernelCodeBuilder {
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
    return SetOutputPortNames(context, MakeIndexedPortNames("shape_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("IdentityShape", IdentityShapeKernelCodeBuilder);

// Runtime contract: the first half of the ports pass StorageShape and the second half pass
// GertTensorData (bg::IdentityShapeAndAddr feeds shapes then addrs, outputs are twice the port pair
// count). The outputs creator reads the input descs for the shape half, so compute_node_info is
// required.
class IdentityShapeAndAddrKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != context.output_chain_indices.size()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    const size_t num_shapes = context.output_chain_indices.size() >> 1U;
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < num_shapes; ++i) {
      inputs.push_back({"shape_" + std::to_string(i), Om2ValueType::kGertStorageShapePtr});
    }
    for (size_t i = num_shapes; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"address_" + std::to_string(i - num_shapes), Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    std::vector<std::string> output_names = MakeIndexedPortNames("shape_", num_shapes);
    for (const auto &name : MakeIndexedPortNames("address_", context.output_chain_indices.size() - num_shapes)) {
      output_names.push_back(name);
    }
    return SetOutputPortNames(context, output_names, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("IdentityShapeAndAddr", IdentityShapeAndAddrKernelCodeBuilder);

// Runtime contract: generic pointer passthrough inserted by AddPointFromForConflicts to resolve
// NetOutput chain conflicts on If/While subgraph outputs; n void inputs are copied to n graph-bound
// output slots without an outputs creator.
class PointFromInputsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != context.output_chain_indices.size()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"value_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("value_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("PointFromInputs", PointFromInputsKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
