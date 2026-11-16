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
class GetPlatformInfoKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    if (context.input_chain_indices.empty()) {
      GE_ASSERT_GRAPH_SUCCESS(SetInputPortNames(context, {}, info));
    } else {
      GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
          context, {{"aicore_num", Om2ValueType::kInt32T}, {"vectorcore_num", Om2ValueType::kInt32T}}, info));
    }
    return SetOutputPortNames(context, {"platform_holder", "core_num_infos_holder"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetPlatformInfo", GetPlatformInfoKernelCodeBuilder);

class AppendCoreTypeToPlatformKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"platform_holder", Om2ValueType::kFePlatFormInfosPtr},
                                               {"core_type", Om2ValueType::kGertCoreTypeIndexPtr},
                                               {"op_ai_core_num", Om2ValueType::kInt32T},
                                               {"op_vector_core_num", Om2ValueType::kInt32T},
                                               {"core_num_infos_holder", Om2ValueType::kGertCoreNumInfosPtr}},
                                              info));
    return SetOutputPortNames(context, {"platform_holder", "core_num_infos_holder"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AppendCoreTypeToPlatform", AppendCoreTypeToPlatformKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
