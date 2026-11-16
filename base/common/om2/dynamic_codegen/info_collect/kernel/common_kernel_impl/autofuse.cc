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
class GetAutofuseFuncsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"so_path", Om2ValueType::kCStringPtr}}, info));
    return SetOutputPortNames(context,
                              {"autofuse_so", "tiling_func", "infer_shape_func", "get_tiling_cache_key_func",
                               "tiling_parse_func", "dfx_input_symbol_info_func"},
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetAutofuseFuncs", GetAutofuseFuncsKernelCodeBuilder);

class GetSymbolTilingCacheKeyKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 3U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t input_tensor_count = context.input_chain_indices.size() - kFixedInputCount;
    inputs.reserve(context.input_chain_indices.size());
    inputs.push_back({"input_data_num", Om2ValueType::kSizeT});
    for (const auto &name : MakeIndexedPortNames("input_tensor_", input_tensor_count)) {
      inputs.push_back({name, Om2ValueType::kGertTensorPtr});
    }
    inputs.insert(inputs.end(), {{"get_symbol_tiling_cache_key_func", Om2ValueType::kKernelFuncPtr},
                                 {"all_symbol_num", Om2ValueType::kSizeT}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"used_symbol_source"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetSymbolTilingCacheKey", GetSymbolTilingCacheKeyKernelCodeBuilder);

class SymbolTilingParseKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"platform_info", Om2ValueType::kFePlatFormInfosPtr}, {"tiling_parse_func", Om2ValueType::kKernelFuncPtr}},
        info));
    return SetOutputPortNames(context, {"compile_info"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("SymbolTilingParse", SymbolTilingParseKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
