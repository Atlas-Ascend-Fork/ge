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
class FindTilingFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"node_type", Om2ValueType::kCStringPtr}, {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
        info));
    return SetOutputPortNames(context, {"tiling_func"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindTilingFunc", FindTilingFuncKernelCodeBuilder);

// Runtime contract (tiling_compatible.cc FindCompatibleTilingFunc, no outputs creator): one string
// input, three graph-bound outputs in FindCompatibleTilingFuncOutputIndex order.
class FindCompatibleTilingFuncKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"node_type", Om2ValueType::kCStringPtr}}, info));
    return SetOutputPortInfos(context,
                              {{"tiling_version", Om2ValueType::kSizeT},
                               {"tiling_parse_func", Om2ValueType::kKernelFuncPtr},
                               {"tiling_func", Om2ValueType::kKernelFuncPtr}},
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindCompatibleTilingFunc", FindCompatibleTilingFuncKernelCodeBuilder);

// Runtime contract (tiling_compatible.cc CompatibleTilingParse, no outputs creator): fixed input order
// per CompatibleTilingParseInputIndex {op, compile_info_json, compile_info_key, tiling_version,
// tiling_parse_func}, one graph-bound compile_info output.
class CompatibleTilingParseKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"op", Om2ValueType::kVoidPtr},
                                               {"compile_info_json", Om2ValueType::kCStringPtr},
                                               {"compile_info_key", Om2ValueType::kCStringPtr},
                                               {"tiling_version", Om2ValueType::kSizeT},
                                               {"tiling_parse_func", Om2ValueType::kKernelFuncPtr}},
                                              info));
    return SetOutputPortNames(context, {"compile_info"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CompatibleTilingParse", CompatibleTilingParseKernelCodeBuilder);

class TilingParseKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"compile_info_json", Om2ValueType::kCStringPtr},
                                               {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                               {"node_type", Om2ValueType::kCStringPtr},
                                               {"space_registry", Om2ValueType::kGertOpImplSpaceRegistryV2Ptr}},
                                              info));
    return SetOutputPortNames(context, {"compile_info"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("TilingParse", TilingParseKernelCodeBuilder);

class PrepareCacheableTilingFwkDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"tiling_func", Om2ValueType::kKernelFuncPtr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"data_dependency", Om2ValueType::kSizeT},
                                               {"build_tiling_cache_key_func_name", Om2ValueType::kCStringPtr}},
                                              info));
    return SetOutputPortNames(context, {"tiling_fwk_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("PrepareCacheableTilingFwkData", PrepareCacheableTilingFwkDataKernelCodeBuilder);

class CacheableTilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    bool symbol_tiling = false;
    (void)ge::AttrUtils::GetBool(context.node.GetOpDescBarePtr(), kSymbolTiling, symbol_tiling);
    if (symbol_tiling) {
      return CollectSymbolTiling(context, info);
    }
    constexpr size_t kFixedInputCount = 6U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    std::vector<KernelInputPortInfo> input_infos;
    input_infos.reserve(context.input_chain_indices.size());
    const auto input_names = MakeIndexedPortNames("io_shape_", context.input_chain_indices.size() - kFixedInputCount);
    for (const auto &input_name : input_names) {
      // CacheableTiling reads each leading IO as either StorageShape * or Tensor * according to
      // CacheableTilingFwkData::data_dependency. Its registered run_func therefore exposes an
      // intentionally erased pointer contract at these positions.
      input_infos.push_back({input_name, Om2ValueType::kVoidPtr});
    }
    input_infos.insert(input_infos.end(), {{"compile_info", Om2ValueType::kVoidPtr},
                                           {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                           {"tiling_fwk_data", Om2ValueType::kGertCacheableTilingFwkDataPtr},
                                           {"deterministic", Om2ValueType::kInt32T},
                                           {"deterministic_level", Om2ValueType::kInt32T},
                                           {"pcie_through", Om2ValueType::kBool}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, input_infos, info));
    return SetOutputPortNames(
        context,
        {"tiling_key", "block_dim", "atomic_flag", "tiling_data", "workspace_sizes", "tiling_condition",
         "schedule_mode", "local_memory_size", "aicpu_block_dim", "simt_block_dim", "simt_grid_dim", "rt_arg"},
        info);
  }

 private:
  // Symbol-tiling ABI (lowering: bg_tiling.cc NeedSymbolTiling branch):
  // [input_data_num, input_tensor_0..N, tiling_parse, get_tiling_cache_key, cacheable_tiling_fwk_data,
  //  deterministic, deterministic_level, pcie_through]. The leading input_data_num is a size_t Const shared with
  // GetSymbolTilingCacheKey and symbol InferShape nodes; keep the typed contract aligned with them.
  ge::graphStatus CollectSymbolTiling(const KernelCollectContext &context, KernelProgramInfo &info) const {
    constexpr size_t kSymbolFixedInputCount = 6U;
    constexpr size_t kInputDataNumCount = 1U;
    if (context.input_chain_indices.size() < (kSymbolFixedInputCount + kInputDataNumCount)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const auto input_tensor_count = context.input_chain_indices.size() - kSymbolFixedInputCount - kInputDataNumCount;
    std::vector<KernelInputPortInfo> input_infos;
    input_infos.reserve(context.input_chain_indices.size());
    input_infos.push_back({"input_data_num", Om2ValueType::kSizeT});
    for (const auto &name : MakeIndexedPortNames("input_tensor_", input_tensor_count)) {
      input_infos.push_back({name, Om2ValueType::kGertTensorPtr});
    }
    input_infos.insert(input_infos.end(), {{"tiling_parse", Om2ValueType::kVoidPtr},
                                           {"get_tiling_cache_key", Om2ValueType::kVoidPtr},
                                           {"cacheable_tiling_fwk_data", Om2ValueType::kGertCacheableTilingFwkDataPtr},
                                           {"deterministic", Om2ValueType::kInt32T},
                                           {"deterministic_level", Om2ValueType::kInt32T},
                                           {"pcie_through", Om2ValueType::kBool}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, input_infos, info));
    return SetOutputPortNames(
        context,
        {"tiling_key", "block_dim", "atomic_flag", "tiling_data", "workspace_sizes", "tiling_condition",
         "schedule_mode", "local_memory_size", "aicpu_block_dim", "simt_block_dim", "simt_grid_dim", "rt_arg"},
        info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CacheableTiling", CacheableTilingKernelCodeBuilder);

class CheckPcieThroughKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    std::vector<KernelInputPortInfo> inputs;
    for (const auto &name : MakeIndexedPortNames("input_data_", context.input_chain_indices.size())) {
      inputs.push_back({name, Om2ValueType::kGertTensorDataPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortInfos(context, {{"pcie_through", Om2ValueType::kBool}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CheckPcieThrough", CheckPcieThroughKernelCodeBuilder);

class CacheableFallibleTilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 6U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"compile_info", Om2ValueType::kVoidPtr},
                                 {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                 {"tiling_fwk_data", Om2ValueType::kGertCacheableTilingFwkDataPtr},
                                 {"deterministic", Om2ValueType::kInt32T},
                                 {"deterministic_level", Om2ValueType::kInt32T},
                                 {"pcie_through", Om2ValueType::kBool}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CacheableFallibleTiling", CacheableFallibleTilingKernelCodeBuilder);

class TilingAppendDfxInfoKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    // The lowering appends these seven framework arguments after the input/output shape
    // triplet. Keep the positional contract here instead of treating every argument as
    // an opaque pointer: several of the tail values are scalar constants consumed by
    // the run_func through GetInputValue<size_t>().
    constexpr size_t kFrameworkInputCount = 7U;
    if (context.input_chain_indices.size() < kFrameworkInputCount) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "TilingAppendDfxInfo has too few inputs: %zu",
             context.input_chain_indices.size());
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t shape_input_count = context.input_chain_indices.size() - kFrameworkInputCount;
    const auto shape_input_names = MakeIndexedPortNames("tiling_dfx_input_", shape_input_count);
    for (const auto &name : shape_input_names) {
      inputs.push_back({name, Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"launch_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                 {"workspace_sizes", Om2ValueType::kTypedContinuousVectorInt64Ptr},
                                 {"ori_op_param_size", Om2ValueType::kSizeT},
                                 {"is_mem_check_enable", Om2ValueType::kSizeT},
                                 {"is_args_exception_enable", Om2ValueType::kSizeT},
                                 {"args_size_list", Om2ValueType::kTypedContinuousVectorInt64Ptr},
                                 {"args_idx_to_io_idx", Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, {"workspace_sizes_with_dfx"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("TilingAppendDfxInfo", TilingAppendDfxInfoKernelCodeBuilder);

class TilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 6U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"compile_info", Om2ValueType::kVoidPtr},
                                 {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                 {"tiling_fwk_data", Om2ValueType::kGertTilingFwkDataPtr},
                                 {"deterministic", Om2ValueType::kInt32T},
                                 {"deterministic_level", Om2ValueType::kInt32T},
                                 {"pcie_through", Om2ValueType::kBool}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("Tiling", TilingKernelCodeBuilder);

class FallibleTilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 6U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"compile_info", Om2ValueType::kVoidPtr},
                                 {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                 {"tiling_fwk_data", Om2ValueType::kGertTilingFwkDataPtr},
                                 {"deterministic", Om2ValueType::kInt32T},
                                 {"deterministic_level", Om2ValueType::kInt32T},
                                 {"pcie_through", Om2ValueType::kBool}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FallibleTiling", FallibleTilingKernelCodeBuilder);

class CompatibleTilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 4U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"op", Om2ValueType::kVoidPtr},
                                               {"compile_info", Om2ValueType::kVoidPtr},
                                               {"tiling_version", Om2ValueType::kSizeT},
                                               {"tiling_fwk_data", Om2ValueType::kGertTilingFwkDataPtr}};
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CompatibleTiling", CompatibleTilingKernelCodeBuilder);

class FallibleCompatibleTilingKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 4U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"op", Om2ValueType::kVoidPtr},
                                               {"compile_info", Om2ValueType::kVoidPtr},
                                               {"tiling_version", Om2ValueType::kSizeT},
                                               {"tiling_fwk_data", Om2ValueType::kGertTilingFwkDataPtr}};
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FallibleCompatibleTiling", FallibleCompatibleTilingKernelCodeBuilder);

class TilingLegacyKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 3U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs;
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    inputs.insert(inputs.end(), {{"compile_info", Om2ValueType::kVoidPtr},
                                 {"platform_info", Om2ValueType::kFePlatFormInfosPtr},
                                 {"tiling_func", Om2ValueType::kKernelFuncPtr}});
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("TilingLegacy", TilingLegacyKernelCodeBuilder);

class CompatibleTilingLegacyKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 4U;
    if (context.input_chain_indices.size() < kFixedInputCount) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"op", Om2ValueType::kVoidPtr},
                                               {"compile_info", Om2ValueType::kVoidPtr},
                                               {"tiling_version", Om2ValueType::kSizeT},
                                               {"tiling_func", Om2ValueType::kKernelFuncPtr}};
    const size_t shape_count = context.input_chain_indices.size() - kFixedInputCount;
    for (size_t i = 0U; i < shape_count; ++i) {
      inputs.push_back({"io_shape_" + std::to_string(i), Om2ValueType::kVoidPtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("tiling_output_", context.output_chain_indices.size()),
                              info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CompatibleTilingLegacy", CompatibleTilingLegacyKernelCodeBuilder);

class TilingAppendWorkspaceKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"tiling_workspace", Om2ValueType::kTypedContinuousVectorSizeTPtr},
                                               {"known_workspace", Om2ValueType::kTypedContinuousVectorInt64Ptr}},
                                              info));
    return SetOutputPortNames(context, {"workspace_sizes"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("TilingAppendWorkspace", TilingAppendWorkspaceKernelCodeBuilder);

class PrepareTilingFwkDataKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"tiling_func", Om2ValueType::kKernelFuncPtr}, {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr}}, info));
    return SetOutputPortNames(context, {"tiling_fwk_data"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("PrepareTilingFwkData", PrepareTilingFwkDataKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
