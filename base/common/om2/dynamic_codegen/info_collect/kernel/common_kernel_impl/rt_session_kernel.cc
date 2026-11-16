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
class GetSessionIdKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_TRUE((context.input_chain_indices.size() == 1U) && (context.output_chain_indices.size() == 1U));
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    info.chain_updates.push_back({context.output_chain_indices[0U], ChainSourceKind::kSessionId,
                                  ChainBindingPhase::kLoad, ChainBindingKind::kNone, kInvalidChainSourceIndex});
    return ge::GRAPH_SUCCESS;
  }

  KernelImplType GetImplType() const override {
    return KernelImplType::kEmptyImpl;
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetSessionId", GetSessionIdKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
