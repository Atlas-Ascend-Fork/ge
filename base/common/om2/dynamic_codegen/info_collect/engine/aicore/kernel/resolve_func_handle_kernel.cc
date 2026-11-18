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
// Runtime contract (runtime/v2/engine/aicore/kernel/resolve_func_handle_kernel.cc):
// [bin_handle(void*), tiling_key(uint64_t), kernel_name(char*), with_handle_flag(uint32_t)] -> func_handle(void*).
class ResolveKernelFuncHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"bin_handle", Om2ValueType::kVoidPtr},
                                               {"tiling_key", Om2ValueType::kUInt64T},
                                               {"kernel_name", Om2ValueType::kCStringPtr},
                                               {"with_handle_flag", Om2ValueType::kUInt32T}},
                                              info));
    return SetOutputPortNames(context, {"func_handle"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("ResolveKernelFuncHandle", ResolveKernelFuncHandleKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
