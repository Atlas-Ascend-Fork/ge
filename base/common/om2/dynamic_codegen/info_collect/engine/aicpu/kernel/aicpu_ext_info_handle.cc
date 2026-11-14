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
class UpdateExtShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 4U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs = {{"ext_info_handler", Om2ValueType::kGertAicpuExtInfoHandlerPtr},
                                               {"input_num", Om2ValueType::kSizeT},
                                               {"output_num", Om2ValueType::kSizeT},
                                               {"stream", Om2ValueType::kRtStreamTPtr}};
    for (size_t i = kFixedInputCount; i < context.input_chain_indices.size(); ++i) {
      inputs.push_back({"shape_" + std::to_string(i - kFixedInputCount), Om2ValueType::kGertStorageShapePtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("update_", context.output_chain_indices.size()), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("UpdateExtShape", UpdateExtShapeKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
