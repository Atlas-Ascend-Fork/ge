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
class CheckOutputShapesEmptyKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    std::vector<KernelInputPortInfo> inputs;
    for (const auto &name : MakeIndexedPortNames("output_shape_", context.input_chain_indices.size())) {
      inputs.push_back({name, Om2ValueType::kGertStorageShapePtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortInfos(context, {{"selector", Om2ValueType::kUInt32T}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CheckOutputShapesEmpty", CheckOutputShapesEmptyKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
