/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <numeric>
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"
#include "framework/common/taskdown_common.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
using V = Om2ValueType;
using P = OutputCreatePolicy;
struct PortContract {
  std::string name;
  V type = V::kUnknown;
};
struct CollectContract {
  const char *type;
  P policy;
  bool compute;
  std::vector<PortContract> inputs;
  std::vector<PortContract> outputs;
};

std::vector<uint32_t> ChainIndices(size_t size, uint32_t first) {
  std::vector<uint32_t> indices(size);
  std::iota(indices.begin(), indices.end(), first);
  return indices;
}

void ExpectPorts(const std::vector<KernelPortInfo> &actual, const std::vector<PortContract> &expected, uint32_t first) {
  ASSERT_EQ(actual.size(), expected.size());
  for (size_t i = 0U; i < expected.size(); ++i) {
    SCOPED_TRACE(i);
    EXPECT_EQ(actual[i].chain_index, first + i);
    EXPECT_EQ(actual[i].semantic_name, expected[i].name);
    EXPECT_EQ(actual[i].value_type, expected[i].type);
  }
}

class Om2KernelCollectCoverageUT : public testing::Test {
 protected:
  ge::FastNode *Node(const char *type) {
    auto *node = graph_.AddNode(std::make_shared<ge::OpDesc>(type, type));
    if (node != nullptr) {
      ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), kComputeNodeIndex, 7);
      ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), "priority", -3);
    }
    return node;
  }

  void CheckContract(const CollectContract &entry) {
    SCOPED_TRACE(entry.type);
    auto *node = Node(entry.type);
    ASSERT_NE(node, nullptr);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(entry.type);
    ASSERT_NE(builder, nullptr);
    const auto inputs = ChainIndices(entry.inputs.size(), 11U);
    const auto outputs = ChainIndices(entry.outputs.size(), 101U);
    KernelCollectContext context(*node, inputs, outputs);
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    EXPECT_EQ(info.execution_node_name, entry.type);
    EXPECT_EQ(info.kernel_type, entry.type);
    EXPECT_EQ(info.output_policy, entry.policy);
    EXPECT_EQ(info.require_compute_node_info, entry.compute);
    EXPECT_EQ(info.compute_node_index, 7U);
    EXPECT_EQ(info.priority, -3);
    ExpectPorts(info.inputs, entry.inputs, 11U);
    ExpectPorts(info.outputs, entry.outputs, 101U);
  }

  void RejectCounts(const char *type, size_t input_count, size_t output_count) {
    SCOPED_TRACE(type);
    auto *node = Node(type);
    ASSERT_NE(node, nullptr);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(type);
    ASSERT_NE(builder, nullptr);
    const auto inputs = ChainIndices(input_count, 11U);
    const auto outputs = ChainIndices(output_count, 101U);
    KernelCollectContext context(*node, inputs, outputs);
    KernelProgramInfo info;
    EXPECT_NE(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  }

  ge::ExecuteGraph graph_{"collector_coverage"};
};

// These are positional runtime contracts: scalar sizes and enums must not become opaque pointers.
const std::vector<CollectContract> kFixedContracts = {
    {"BuildHostCCArgs",
     P::kOutputsCreator,
     false,
     {{"io_num", V::kSizeT},
      {"node_name", V::kCStringPtr},
      {"need_device_ext", V::kBool},
      {"arg_data", V::kBytePtr},
      {"arg_size", V::kSizeT},
      {"ext_size", V::kSizeT}},
     {{"args_handler"}}},
    {"CreateStepId", P::kOutputsCreator, false, {}, {{"step_id"}, {"iteration"}}},
    {"EnsureCreateTfSession", P::kGraphBound, false, {{"session_id", V::kUInt64T}}, {}},
    {"IncreaseStepId",
     P::kGraphBound,
     false,
     {{"step_id", V::kVoidPtr}, {"iteration", V::kInt64T}, {"stream", V::kRtStreamTPtr}},
     {}},
    {"BuildCCArgs",
     P::kOutputsCreator,
     false,
     {{"io_num", V::kSizeT},
      {"node_name", V::kCStringPtr},
      {"need_device_ext", V::kBool},
      {"arg_data", V::kBytePtr},
      {"arg_size", V::kSizeT},
      {"ext_size", V::kSizeT},
      {"is_block_op", V::kBool},
      {"rt_event", V::kVoidPtr},
      {"async_timeout", V::kUInt32T},
      {"kernel_name", V::kCStringPtr},
      {"kernel_name_size", V::kSizeT},
      {"so_name", V::kCStringPtr},
      {"so_name_size", V::kSizeT}},
     {{"args_handler"}}},
    {"BuildTfArgs",
     P::kOutputsCreator,
     false,
     {{"io_num", V::kSizeT},
      {"node_name", V::kCStringPtr},
      {"need_device_ext", V::kBool},
      {"arg_data", V::kBytePtr},
      {"arg_size", V::kSizeT},
      {"ext_size", V::kSizeT},
      {"is_block_op", V::kBool},
      {"rt_event", V::kVoidPtr},
      {"async_timeout", V::kUInt32T},
      {"task_data", V::kBytePtr},
      {"task_size", V::kSizeT},
      {"session_id", V::kUInt64T},
      {"step_id", V::kVoidPtr}},
     {{"args_handler"}}},
    {"BuildExtInfoHandle",
     P::kOutputsCreator,
     false,
     {{"ext_data", V::kBytePtr},
      {"ext_len", V::kSizeT},
      {"node_name", V::kCStringPtr},
      {"input_num", V::kSizeT},
      {"output_num", V::kSizeT},
      {"unknown_shape_type", V::kInt32T},
      {"host_addr", V::kVoidPtr},
      {"device_addr", V::kVoidPtr},
      {"session_id", V::kUInt64T},
      {"is_block_op", V::kBool},
      {"event_id", V::kUInt32T}},
     {{"ext_info_handler"}}},
    {"CalcDeviceCopySizes",
     P::kGraphBound,
     false,
     {{"src_address", V::kGertTensorDataPtr},
      {"allocator", V::kGertAllocatorPtr},
      {"data_type", V::kGeDataType},
      {"storage_shape", V::kGertStorageShapePtr},
      {"stream", V::kRtStreamTPtr},
      {"original_tensor_size", V::kSizeT}},
     {{"alloc_size", V::kSizeT}, {"copy_size", V::kSizeT}}},
    {"LaunchH2DCopy",
     P::kGraphBound,
     false,
     {{"dst_address", V::kGertTensorDataPtr},
      {"stream", V::kRtStreamTPtr},
      {"src_address", V::kGertTensorDataPtr},
      {"tensor_size", V::kSizeT}},
     {}},
    {"ShareH2DCopyResult",
     P::kOutputsCreator,
     false,
     {{"dst_address", V::kGertTensorDataPtr},
      {"stream", V::kRtStreamTPtr},
      {"src_address", V::kGertTensorDataPtr},
      {"tensor_size", V::kSizeT}},
     {{"tensor_data"}}},
    {"CopyD2H",
     P::kOutputsCreator,
     false,
     {{"src_address", V::kGertTensorDataPtr}, {"src_length", V::kSizeT}, {"allocator", V::kGertAllocatorPtr}},
     {{"host_tensor_data", V::kGertTensorDataPtr}}},
    {"CopyTensorDataH2H",
     P::kOutputsCreator,
     false,
     {{"src_address", V::kGertTensorDataPtr},
      {"src_shape", V::kGertStorageShapePtr},
      {"src_data_type", V::kGeDataType},
      {"allocator", V::kGertAllocatorPtr}},
     {{"host_tensor_data"}}},
    {"EnsureTensorAtOutMemory",
     P::kGraphBound,
     true,
     {{"shape", V::kGertStorageShapePtr},
      {"tensor_data", V::kGertTensorDataPtr},
      {"tensor_attr", V::kGertBuildTensorAttrPtr},
      {"stream", V::kRtStreamTPtr},
      {"output_data", V::kGertTensorPtr}},
     {}},
    {"SinkWeightData",
     P::kOutputsCreator,
     false,
     {{"weight_data", V::kGertTensorDataPtr},
      {"device_base_addr", V::kGertTensorDataPtr},
      {"stream_id", V::kInt64T},
      {"stream", V::kRtStreamTPtr}},
     {{"tensor_data"}}},
    {"CreateHostL2Allocator",
     P::kOutputsCreator,
     false,
     {{"l1_allocator", V::kGertAllocatorPtr}},
     {{"host_l2_allocator"}}},
    {"CreateL1Allocator", P::kGraphBound, false, {{"placement", V::kGertTensorPlacement}}, {{"l1_allocator"}}},
    {"SelectL1Allocator",
     P::kGraphBound,
     false,
     {{"placement", V::kGertTensorPlacement},
      {"external_allocator", V::kGertAllocatorsPtr},
      {"created_allocator", V::kGeAllocatorPtr},
      {"stream", V::kRtStreamTPtr}},
     {{"l1_allocator"}}},
    {"SelectL2Allocator",
     P::kGraphBound,
     false,
     {{"stream_id", V::kInt64T},
      {"stream", V::kRtStreamTPtr},
      {"l1_allocator", V::kGeAllocatorPtr},
      {"l2_allocators", V::kGertAllocatorVectorPtr}},
     {{"l2_allocator"}}},
    {"CreateL2Allocators",
     P::kOutputsCreator,
     false,
     {{"stream_num", V::kInt64T}, {"placement", V::kGertTensorPlacement}},
     {{"l2_allocators"}, {"l2_mem_pools"}}},
    {"CreateInitL2Allocator",
     P::kOutputsCreator,
     false,
     {{"l1_allocator", V::kGeAllocatorPtr},
      {"l2_allocators", V::kGertAllocatorVectorPtr},
      {"stream", V::kRtStreamTPtr}},
     {{"init_l2_allocator"}, {"l2_allocators_for_init"}}},
    {"AllocBatchHbm",
     P::kOutputsCreator,
     true,
     {{"allocator", V::kGertAllocatorPtr}, {"sizes", V::kTypedContinuousVectorSizeTPtr}},
     {{"tensor_data"}}},
    {"FreeBatchHbm", P::kGraphBound, true, {{"tensor_data", V::kGertTensorDataVectorPtr}}, {}},
    {"FreeMemory", P::kGraphBound, true, {{"tensor_data", V::kGertTensorDataPtr}}, {}},
    {"FreeTensorMemory", P::kGraphBound, false, {{"tensor", V::kGertTensorPtr}}, {}},
    {"FreeBatchHbmHoldAddr", P::kGraphBound, true, {{"tensor_data", V::kGertTensorDataVectorPtr}}, {}},
    {"FreeMemoryHoldAddr", P::kGraphBound, true, {{"tensor_data", V::kGertTensorDataPtr}}, {}},
    {"EmptyTensorData", P::kOutputsCreator, false, {}, {{"tensor_data"}}},
    {"ReusableStreamAllocator", P::kOutputsCreator, false, {}, {{"reusable_stream_allocator"}}},
    {"CreateMemAssigner", P::kGraphBound, false, {{"base_mem", V::kGertTensorDataPtr}}, {{"mem_assigner"}}},
    {"AssignWeightMemory",
     P::kOutputsCreator,
     true,
     {{"offset_tensor", V::kGertTensorDataObjectPtr}, {"assigner", V::kVoidPtr}, {"stream_id", V::kInt64T}},
     {{"tensor_data"}}},
    {"GetOrCreateWeightMem",
     P::kOutputsCreator,
     false,
     {{"required_size", V::kSizeT}, {"appointed_weight", V::kOuterWeightMemPtr}, {"allocator", V::kGertAllocatorPtr}},
     {{"tensor_data"}}},
    {"BuildDualStageAclnnOpFwkData",
     P::kOutputsCreator,
     false,
     {{"execute_prepare_func", V::kKernelFuncPtr},
      {"execute_launch_func", V::kKernelFuncPtr},
      {"platform_info", V::kFePlatFormInfosPtr},
      {"core_num_infos", V::kGertCoreNumInfosPtr},
      {"deterministic_config", V::kAclnnDeterministicConfigPtr},
      {"original_deterministic_config", V::kAclnnOriginalDeterministicConfigPtr}},
     {{"aclnn_fwk_data"}}},
    {"BuildSingleStageAclnnOpFwkData",
     P::kOutputsCreator,
     false,
     {{"core_num_infos", V::kGertCoreNumInfosPtr},
      {"deterministic_config", V::kAclnnDeterministicConfigPtr},
      {"original_deterministic_config", V::kAclnnOriginalDeterministicConfigPtr}},
     {{"aclnn_fwk_data"}}},
    {"FindOpExe2PhaseFunc",
     P::kGraphBound,
     false,
     {{"node_type", V::kCStringPtr}, {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"execute_prepare_func"}, {"execute_launch_func"}}},
    {"FindOpExeFunc",
     P::kGraphBound,
     false,
     {{"node_type", V::kCStringPtr}, {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"execute_func"}}},
    {"FindInferShapeFunc",
     P::kGraphBound,
     true,
     {{"node_type", V::kCStringPtr}, {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"infer_shape_func"}}},
    {"FindInferShapeRangeFunc",
     P::kGraphBound,
     true,
     {{"node_type", V::kCStringPtr}, {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"infer_shape_range_func"}}},
    {"FindCompatibleInferShapeFunc", P::kOutputsCreator, true, {{"node_type", V::kCStringPtr}}, {{"infer_shape_func"}}},
    {"FindCompatibleInferShapeRangeFunc",
     P::kOutputsCreator,
     true,
     {{"node_type", V::kCStringPtr}},
     {{"infer_shape_range_func"}}},
    {"CreateOpFromBuffer",
     P::kOutputsCreator,
     true,
     {{"op_buffer", V::kBytePtr}, {"op_buffer_size", V::kSizeT}},
     {{"op", V::kVoidPtr}}},
    {"TilingAppendWorkspace",
     P::kOutputsCreator,
     false,
     {{"tiling_workspace", V::kTypedContinuousVectorSizeTPtr}, {"known_workspace", V::kTypedContinuousVectorInt64Ptr}},
     {{"workspace_sizes"}}},
    {"PrepareTilingFwkData",
     P::kOutputsCreator,
     false,
     {{"tiling_func", V::kKernelFuncPtr}, {"rt_arg", V::kGertRtKernelLaunchArgsExPtr}},
     {{"tiling_fwk_data"}}},
    {"PrepareCacheableTilingFwkData",
     P::kOutputsCreator,
     true,
     {{"tiling_func", V::kKernelFuncPtr},
      {"rt_arg", V::kGertRtKernelLaunchArgsExPtr},
      {"data_dependency", V::kSizeT},
      {"build_tiling_cache_key_func_name", V::kCStringPtr}},
     {{"tiling_fwk_data"}}},
    {"CreateAttachedStreamProvider", P::kOutputsCreator, false, {}, {{"attached_stream_provider", V::kVoidPtr}}},
    {"FindCustomOp",
     P::kGraphBound,
     false,
     {{"node_type", V::kCStringPtr}, {"custom_op_registry", V::kVoidPtr}},
     {{"custom_op"}}},
    {"FreeCustomOpWorkspaces",
     P::kGraphBound,
     false,
     {{"workspaces", V::kVoidPtr}, {"allocator", V::kGertAllocatorPtr}},
     {}},
    {"FreeArgsGuarder", P::kGraphBound, false, {{"args_handler", V::kVoidPtr}}, {}},
    {"AllocMemory",
     P::kOutputsCreator,
     true,
     {{"allocator", V::kGertAllocatorPtr}, {"size", V::kSizeT}},
     {{"tensor_data"}}},
};

TEST_F(Om2KernelCollectCoverageUT, FixedRuntimeContractsPreserveNamesTypesAndRejectExtraPorts) {
  for (const auto &entry : kFixedContracts) {
    CheckContract(entry);
    RejectCounts(entry.type, entry.inputs.size() + 1U, entry.outputs.size());
    RejectCounts(entry.type, entry.inputs.size(), entry.outputs.size() + 1U);
  }
}

TEST_F(Om2KernelCollectCoverageUT, ComputeMetadataIsRequiredOnlyByItsConsumers) {
  for (const auto &entry : kFixedContracts) {
    SCOPED_TRACE(entry.type);
    ge::ExecuteGraph graph("metadata");
    auto *node = graph.AddNode(std::make_shared<ge::OpDesc>(entry.type, entry.type));
    ASSERT_NE(node, nullptr);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(entry.type);
    ASSERT_NE(builder, nullptr);
    const auto inputs = ChainIndices(entry.inputs.size(), 11U);
    const auto outputs = ChainIndices(entry.outputs.size(), 101U);
    KernelCollectContext context(*node, inputs, outputs);
    KernelProgramInfo info;
    if (entry.compute) {
      EXPECT_NE(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    } else {
      ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
      EXPECT_EQ(info.compute_node_index, kInvalidProgramIndex);
      ExpectPorts(info.outputs, entry.outputs, 101U);
    }
  }
}

const std::vector<PortContract> kTilingTail = {{"compile_info", V::kVoidPtr},
                                               {"platform_info", V::kFePlatFormInfosPtr},
                                               {"tiling_fwk_data", V::kGertTilingFwkDataPtr},
                                               {"deterministic", V::kInt32T},
                                               {"deterministic_level", V::kInt32T},
                                               {"pcie_through", V::kBool}};
const std::vector<PortContract> kCacheableTilingTail = {{"compile_info", V::kVoidPtr},
                                                        {"platform_info", V::kFePlatFormInfosPtr},
                                                        {"tiling_fwk_data", V::kGertCacheableTilingFwkDataPtr},
                                                        {"deterministic", V::kInt32T},
                                                        {"deterministic_level", V::kInt32T},
                                                        {"pcie_through", V::kBool}};
const std::vector<PortContract> kCacheableOutputs = {{"tiling_key"},     {"block_dim"},         {"atomic_flag"},
                                                     {"tiling_data"},    {"workspace_sizes"},   {"tiling_condition"},
                                                     {"schedule_mode"},  {"local_memory_size"}, {"aicpu_block_dim"},
                                                     {"simt_block_dim"}, {"simt_grid_dim"},     {"rt_arg"}};

std::vector<PortContract> JoinPorts(std::vector<PortContract> prefix, const std::vector<PortContract> &tail) {
  prefix.insert(prefix.end(), tail.begin(), tail.end());
  return prefix;
}

std::vector<PortContract> IndexedPorts(const char *prefix, size_t count, V type = V::kUnknown) {
  std::vector<PortContract> result;
  for (size_t i = 0U; i < count; ++i) {
    result.push_back({std::string(prefix) + std::to_string(i), type});
  }
  return result;
}

TEST_F(Om2KernelCollectCoverageUT, TilingVariantsKeepFrameworkTailAndCompatiblePrefix) {
  struct Variant {
    const char *type;
    std::vector<PortContract> tail;
  };
  const std::vector<Variant> variants = {
      {"Tiling", kTilingTail},
      {"FallibleTiling", kTilingTail},
      {"CacheableFallibleTiling", kCacheableTilingTail},
      {"TilingLegacy",
       {{"compile_info", V::kVoidPtr}, {"platform_info", V::kFePlatFormInfosPtr}, {"tiling_func", V::kKernelFuncPtr}}}};
  for (const auto &variant : variants) {
    for (const size_t count : {0U, 2U}) {
      CheckContract({variant.type, P::kOutputsCreator, true,
                     JoinPorts(IndexedPorts("io_shape_", count, V::kVoidPtr), variant.tail),
                     IndexedPorts("tiling_output_", count)});
    }
    RejectCounts(variant.type, variant.tail.size() - 1U, 1U);
  }
  for (const auto *type : {"CompatibleTiling", "FallibleCompatibleTiling", "CompatibleTilingLegacy"}) {
    const V last = std::string(type) == "CompatibleTilingLegacy" ? V::kKernelFuncPtr : V::kGertTilingFwkDataPtr;
    const char *name = last == V::kKernelFuncPtr ? "tiling_func" : "tiling_fwk_data";
    CheckContract(
        {type, P::kOutputsCreator, true,
         JoinPorts({{"op", V::kVoidPtr}, {"compile_info", V::kVoidPtr}, {"tiling_version", V::kSizeT}, {name, last}},
                   IndexedPorts("io_shape_", 2U, V::kVoidPtr)),
         IndexedPorts("tiling_output_", 2U)});
    RejectCounts(type, 3U, 2U);
  }
}

TEST_F(Om2KernelCollectCoverageUT, CacheableSymbolTilingUsesTensorPrefixAndExactOutputOrder) {
  CheckContract({"CacheableTiling", P::kOutputsCreator, true,
                 JoinPorts(IndexedPorts("io_shape_", 2U, V::kVoidPtr), kCacheableTilingTail), kCacheableOutputs});
  RejectCounts("CacheableTiling", 5U, 12U);
  RejectCounts("CacheableTiling", 8U, 11U);
  auto *node = Node("CacheableTiling");
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetBool(node->GetOpDescBarePtr(), kSymbolTiling, true));
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("CacheableTiling");
  ASSERT_NE(builder, nullptr);
  auto inputs = ChainIndices(9U, 11U);
  const auto outputs = ChainIndices(12U, 101U);
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ExpectPorts(info.inputs,
              {{"input_data_num", V::kSizeT},
               {"input_tensor_0", V::kGertTensorPtr},
               {"input_tensor_1", V::kGertTensorPtr},
               {"tiling_parse", V::kVoidPtr},
               {"get_tiling_cache_key", V::kVoidPtr},
               {"cacheable_tiling_fwk_data", V::kGertCacheableTilingFwkDataPtr},
               {"deterministic", V::kInt32T},
               {"deterministic_level", V::kInt32T},
               {"pcie_through", V::kBool}},
              11U);
  ExpectPorts(info.outputs, kCacheableOutputs, 101U);
  inputs.resize(6U);
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(context, invalid), ge::GRAPH_SUCCESS);
}

const std::vector<CollectContract> kTilingLookupContracts = {
    {"FindTilingFunc",
     P::kGraphBound,
     true,
     {{"node_type", V::kCStringPtr}, {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"tiling_func"}}},
    {"FindCompatibleTilingFunc",
     P::kGraphBound,
     true,
     {{"node_type", V::kCStringPtr}},
     {{"tiling_version", V::kSizeT}, {"tiling_parse_func", V::kKernelFuncPtr}, {"tiling_func", V::kKernelFuncPtr}}},
    {"CompatibleTilingParse",
     P::kGraphBound,
     true,
     {{"op", V::kVoidPtr},
      {"compile_info_json", V::kCStringPtr},
      {"compile_info_key", V::kCStringPtr},
      {"tiling_version", V::kSizeT},
      {"tiling_parse_func", V::kKernelFuncPtr}},
     {{"compile_info"}}},
    {"TilingParse",
     P::kOutputsCreator,
     true,
     {{"compile_info_json", V::kCStringPtr},
      {"platform_info", V::kFePlatFormInfosPtr},
      {"node_type", V::kCStringPtr},
      {"space_registry", V::kGertOpImplSpaceRegistryV2Ptr}},
     {{"compile_info"}}},
    {"TilingAppendDfxInfo",
     P::kOutputsCreator,
     true,
     {{"tiling_dfx_input_0", V::kVoidPtr},
      {"launch_arg", V::kGertRtKernelLaunchArgsExPtr},
      {"workspace_sizes", V::kTypedContinuousVectorInt64Ptr},
      {"ori_op_param_size", V::kSizeT},
      {"is_mem_check_enable", V::kSizeT},
      {"is_args_exception_enable", V::kSizeT},
      {"args_size_list", V::kTypedContinuousVectorInt64Ptr},
      {"args_idx_to_io_idx", V::kTypedContinuousVectorArgsIndexToIoIndexPtr}},
     {{"workspace_sizes_with_dfx"}}}};
TEST_F(Om2KernelCollectCoverageUT, TilingLookupAndDfxContractsKeepScalarAndVectorDistinctions) {
  for (const auto &entry : kTilingLookupContracts) {
    CheckContract(entry);
    RejectCounts(entry.type, entry.inputs.size(), entry.outputs.size() + 1U);
  }
  RejectCounts("TilingAppendDfxInfo", 6U, 1U);
  CheckContract({"CheckPcieThrough",
                 P::kGraphBound,
                 false,
                 IndexedPorts("input_data_", 2U, V::kGertTensorDataPtr),
                 {{"pcie_through", V::kBool}}});
}

std::vector<PortContract> CopyInputs(size_t count, bool device) {
  std::vector<PortContract> ports = {{"stream", V::kRtStreamTPtr}, {"allocator", V::kGertAllocatorPtr}};
  for (size_t i = 0U; i < count; ++i) {
    const auto suffix = std::to_string(i);
    ports.push_back({"tensor_data_" + suffix, V::kGertTensorDataPtr});
    ports.push_back({"tensor_size_" + suffix, V::kSizeT});
    if (device) {
      ports.push_back({"storage_shape_" + suffix, V::kGertStorageShapePtr});
      ports.push_back({"data_type_" + suffix, V::kGeDataType});
    }
  }
  return ports;
}

TEST_F(Om2KernelCollectCoverageUT, BatchedCopyContractsValidatePerTensorGroupsAndOutputCount) {
  struct Variant {
    const char *type;
    bool device;
    const char *output;
    V output_type;
  };
  const std::vector<Variant> variants = {{"MakeSureTensorAtDevice", true, "device_tensor_data_", V::kUnknown},
                                         {"CopyH2D", true, "device_tensor_data_", V::kGertTensorDataPtr},
                                         {"MakeSureTensorAtHost", false, "host_tensor_data_", V::kUnknown},
                                         {"MakeSureTensorAtHostWithoutSync", false, "host_tensor_data_", V::kUnknown}};
  for (const auto &variant : variants) {
    for (size_t count : {0U, 1U, 2U}) {
      CheckContract({variant.type, P::kOutputsCreator, true, CopyInputs(count, variant.device),
                     IndexedPorts(variant.output, count, variant.output_type)});
    }
    RejectCounts(variant.type, 1U, 0U);
    RejectCounts(variant.type, 3U, 1U);
    RejectCounts(variant.type, variant.device ? 6U : 4U, 2U);
  }
  for (size_t count : {0U, 1U}) {
    CheckContract({"CopyD2D",
                   P::kOutputsCreator,
                   false,
                   {{"src_address", V::kGertTensorDataPtr},
                    {"dst_address", V::kGertTensorDataPtr},
                    {"tensor_size", V::kSizeT},
                    {"stream", V::kRtStreamTPtr}},
                   count == 0U ? std::vector<PortContract>{} : std::vector<PortContract>{{"device_tensor_data"}}});
  }
  RejectCounts("CopyD2D", 4U, 2U);
}

TEST_F(Om2KernelCollectCoverageUT, MemoryAndKnownSubgraphContractsKeepRepeatedPortGroups) {
  for (size_t count : {1U, 2U}) {
    CheckContract({"AllocMemHbm", P::kOutputsCreator, false,
                   JoinPorts({{"allocator", V::kGertAllocatorPtr}}, IndexedPorts("size_", count, V::kSizeT)),
                   IndexedPorts("tensor_data_", count)});
    CheckContract({"DavinciModelGetRunAddress", P::kOutputsCreator, false,
                   JoinPorts({{"davinci_model", V::kGeDavinciModelPtr}, {"stream_id", V::kInt64T}},
                             IndexedPorts("memory_base_type_offset_", count, V::kMemoryBaseTypeOffsetPtr)),
                   IndexedPorts("device_tensor_data_", count)});
    CheckContract({"DavinciModelFinalizer",
                   P::kGraphBound,
                   false,
                   IndexedPorts("davinci_model_", count, V::kGeDavinciModelPtr),
                   {}});
  }
  for (const auto *type : {"FreeMemHbm", "FreeMemHbmHoldAddr"}) {
    CheckContract({type, P::kGraphBound, true, IndexedPorts("tensor_data_", 2U, V::kGertTensorDataPtr), {}});
    CheckContract({type, P::kGraphBound, true, {}, {}});
  }
  CheckContract({"DavinciModelExecute",
                 P::kGraphBound,
                 false,
                 {{"davinci_model", V::kGeDavinciModelPtr},
                  {"stream", V::kRtStreamTPtr},
                  {"input_num", V::kSizeT},
                  {"output_num", V::kSizeT},
                  {"tensor_data_0", V::kGertTensorDataPtr},
                  {"tensor_data_1", V::kGertTensorDataPtr}},
                 {{"result"}}});
  CheckContract({"DavinciModelUpdateWorkspaces",
                 P::kGraphBound,
                 false,
                 {{"davinci_model", V::kGeDavinciModelPtr},
                  {"workspace_num", V::kSizeT},
                  {"memory_type_0", V::kUInt64T},
                  {"tensor_data_0", V::kGertTensorDataPtr},
                  {"memory_type_1", V::kUInt64T},
                  {"tensor_data_1", V::kGertTensorDataPtr}},
                 {{"result"}}});
  RejectCounts("AllocMemHbm", 1U, 0U);
  RejectCounts("AllocMemHbm", 3U, 1U);
  RejectCounts("DavinciModelExecute", 3U, 1U);
  RejectCounts("DavinciModelUpdateWorkspaces", 1U, 1U);
  RejectCounts("DavinciModelUpdateWorkspaces", 3U, 1U);
  RejectCounts("DavinciModelGetRunAddress", 1U, 0U);
  RejectCounts("DavinciModelGetRunAddress", 3U, 2U);
  RejectCounts("DavinciModelFinalizer", 0U, 0U);
}

TEST_F(Om2KernelCollectCoverageUT, ShapeRangeOutputsAreGroupedAsRangesThenMinimumsThenMaximums) {
  const auto range_outputs =
      JoinPorts(JoinPorts(IndexedPorts("output_shape_range_", 2U), IndexedPorts("output_min_shape_", 2U)),
                IndexedPorts("output_max_shape_", 2U));
  const std::vector<CollectContract> contracts = {
      {"InferShapeRange",
       P::kOutputsCreator,
       true,
       {{"input_shape_range_0", V::kVoidPtr},
        {"input_shape_range_1", V::kVoidPtr},
        {"infer_shape_range_func", V::kKernelFuncPtr}},
       range_outputs},
      {"CompatibleInferShapeRange",
       P::kOutputsCreator,
       true,
       {{"op", V::kVoidPtr},
        {"infer_func", V::kVoidPtr},
        {"input_shape_range_0", V::kVoidPtr},
        {"input_shape_range_1", V::kVoidPtr}},
       range_outputs},
      {"CompatibleInferShape",
       P::kOutputsCreator,
       true,
       {{"op", V::kVoidPtr},
        {"infer_func", V::kVoidPtr},
        {"input_shape_0", V::kGertStorageShapePtr},
        {"input_shape_1", V::kGertStorageShapePtr}},
       IndexedPorts("output_shape_", 2U)},
      {"CreateTensorRangesAndShapeRanges", P::kOutputsCreator, true,
       IndexedPorts("input_tensor_", 2U, V::kGertTensorPtr), IndexedPorts("tensor_range_", 4U)},
      {"SetOutputShape",
       P::kGraphBound,
       true,
       {{"shape_buffer", V::kGertTensorDataPtr}},
       IndexedPorts("output_shape_", 2U)}};
  for (const auto &entry : contracts) {
    CheckContract(entry);
  }
  RejectCounts("InferShapeRange", 0U, 3U);
  RejectCounts("InferShapeRange", 3U, 5U);
  RejectCounts("CompatibleInferShapeRange", 1U, 3U);
  RejectCounts("CompatibleInferShapeRange", 4U, 5U);
  RejectCounts("CompatibleInferShape", 1U, 2U);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("CreateOpFromBuffer");
  ASSERT_NE(builder, nullptr);
  EXPECT_EQ(builder->GetRestoreConstInputIndices(), (std::vector<uint32_t>{0U, 1U}));
}

TEST_F(Om2KernelCollectCoverageUT, ExecuteOpContractsSeparateTensorsFromFrameworkArguments) {
  const std::vector<CollectContract> contracts = {{"ExecuteOpPrepare",
                                                   P::kGraphBound,
                                                   true,
                                                   {{"tensor_0", V::kGertTensorPtr},
                                                    {"tensor_1", V::kGertTensorPtr},
                                                    {"execute_option", V::kGertOpExecuteOptionsPtr},
                                                    {"aclnn_fwk_data", V::kVoidPtr},
                                                    {"stream", V::kRtStreamTPtr}},
                                                   {{"op_param"}, {"workspace_sizes"}}},
                                                  {"ExecuteOpFunc",
                                                   P::kOutputsCreator,
                                                   true,
                                                   {{"tensor_0", V::kGertTensorPtr},
                                                    {"tensor_1", V::kGertTensorPtr},
                                                    {"allocator", V::kGertAllocatorPtr},
                                                    {"stream", V::kRtStreamTPtr},
                                                    {"execute_option", V::kGertOpExecuteOptionsPtr},
                                                    {"execute_func", V::kKernelFuncPtr},
                                                    {"aclnn_fwk_data", V::kVoidPtr}},
                                                   {{"block_memory"}}}};
  for (const auto &entry : contracts) {
    CheckContract(entry);
    RejectCounts(entry.type, entry.inputs.size() - 3U, entry.outputs.size());
    RejectCounts(entry.type, entry.inputs.size(), entry.outputs.size() + 1U);
  }
  const std::vector<PortContract> tail = {
      {"allocator", V::kGertAllocatorPtr}, {"stream", V::kRtStreamTPtr},
      {"custom_op", V::kVoidPtr},          {"attached_stream_provider", V::kVoidPtr},
      {"root_graph_id", V::kUInt32T},      {"node_id", V::kInt64T},
      {"vector_core_num", V::kInt64T}};
  const std::vector<PortContract> outputs = {{"output_tensor_0"}, {"workspace"}, {"args_handler"}, {"context_info"}};
  CheckContract({"ExecuteCustomOp", P::kOutputsCreator, true,
                 JoinPorts(IndexedPorts("input_tensor_", 2U, V::kGertTensorPtr), tail), outputs});
  CheckContract({"ExecuteCustomOpWithInferShape", P::kOutputsCreator, true,
                 JoinPorts(JoinPorts(IndexedPorts("input_tensor_", 2U, V::kGertTensorPtr), tail),
                           {{"template_tensor_0", V::kGertTensorPtr}}),
                 outputs});
  for (const auto *type : {"ExecuteCustomOp", "ExecuteCustomOpWithInferShape"}) {
    RejectCounts(type, 10U, 2U);
    RejectCounts(type, 6U, 4U);
  }
}

TEST_F(Om2KernelCollectCoverageUT, AicpuBinaryCodeEscapesJsonPathAndCachesLoadedHandle) {
  for (const auto *type : {"BuildCCArgsBinHandle", "BuildTfArgsBinHandle"}) {
    SCOPED_TRACE(type);
    auto *node = Node(type);
    ASSERT_NE(node, nullptr);
    const bool cc = std::string(type) == "BuildCCArgsBinHandle";
    const auto inputs = ChainIndices(cc ? 4U : 2U, 11U);
    const auto outputs = ChainIndices(cc ? 2U : 1U, 101U);
    std::vector<Chain> values(inputs.size()), output_values;
    values[0U].GetValue<ge::OpDesc *>() = node->GetOpDescBarePtr();
    char node_type[] = "Add";
    values[1U].GetValue<char *>() = node_type;
    const std::string path = "dir\\file\"\n\r\t.json";
    ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), "ops_json_path", path));
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(type);
    ASSERT_NE(builder, nullptr);
    std::string code;
    EXPECT_NE(builder->GenerateKernelCode("LoadBinary", code), ge::GRAPH_SUCCESS);
    KernelCollectContext context(*node, inputs, outputs, values, output_values);
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    ASSERT_EQ(builder->GenerateKernelCode("LoadBinary", code), ge::GRAPH_SUCCESS);
    EXPECT_EQ(builder->GetImplType(), KernelImplType::kCodegenImpl);
    EXPECT_EQ(builder->GetRestoreConstInputIndices(), ChainIndices(inputs.size(), 0U));
    EXPECT_NE(code.find("aclError LoadBinary()"), std::string::npos);
    EXPECT_NE(code.find("chains_[101U]"), std::string::npos);
    EXPECT_NE(code.find("dir\\\\file\\\"\\n\\r\\t.json"), std::string::npos);
    EXPECT_NE(code.find("aicpu_bin_handles_.find(path)"), std::string::npos);
    EXPECT_NE(code.find("aclrtBinaryLoadFromFile(json_path, &options, &handle)"), std::string::npos);
    EXPECT_NE(code.find("output.Set(iter->second, nullptr)"), std::string::npos);
    EXPECT_EQ(info.output_policy, P::kGraphBound);
  }
}

TEST_F(Om2KernelCollectCoverageUT, ReleaseCustomBinaryUsesItsCollectedLeaseChain) {
  auto *node = Node("ReleaseCustAicpuBinHandle");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("ReleaseCustAicpuBinHandle");
  ASSERT_NE(builder, nullptr);
  std::string code;
  EXPECT_NE(builder->GenerateKernelCode("ReleaseBinary", code), ge::GRAPH_SUCCESS);
  const std::vector<uint32_t> inputs{39U}, outputs;
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  EXPECT_EQ(builder->GetImplType(), KernelImplType::kCodegenImpl);
  ExpectPorts(info.inputs, {{"bin_resource", V::kVoidPtr}}, 39U);
  ASSERT_EQ(builder->GenerateKernelCode("ReleaseBinary", code), ge::GRAPH_SUCCESS);
  EXPECT_EQ(code, "  aclError ReleaseBinary() {\n    return ReleaseCustAicpuBin(chains_[39U]);\n  }\n");
}

TEST_F(Om2KernelCollectCoverageUT, CopyFlowCreatorsUseCollectedCountAndOutputChains) {
  struct Stage {
    const char *type;
    size_t input_count;
    size_t output_count;
    const char *code;
    P policy;
  };
  const std::vector<Stage> stages = {
      {"CalcCopyFlowAllocSizes", 10U, 1U, "InitCopyFlowSizesOutput(chains_[101U], 2U)", P::kCodegenImpl},
      {"PrepareCopyFlowResult", 12U, 2U, "InitCopyFlowTensorOutputs({&chains_[101U], &chains_[102U]})",
       P::kCodegenImpl},
      {"LaunchCopyFlowH2D", 11U, 0U, "", P::kGraphBound},
      {"AllocCopyFlowHbm", 2U, 1U, "InitCopyFlowAllocOutput(chains_[101U], 2U)", P::kCodegenImpl}};
  for (const auto &stage : stages) {
    SCOPED_TRACE(stage.type);
    auto *node = Node(stage.type);
    ASSERT_NE(node, nullptr);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(stage.type);
    ASSERT_NE(builder, nullptr);
    auto inputs = ChainIndices(stage.input_count, 11U);
    const auto outputs = ChainIndices(stage.output_count, 101U);
    KernelCollectContext context(*node, inputs, outputs);
    KernelProgramInfo missing, info;
    EXPECT_NE(builder->Collect(context, missing), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), "copy_flow_count", 2));
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    EXPECT_EQ(info.output_policy, stage.policy);
    ASSERT_EQ(info.inputs.size(), stage.input_count);
    EXPECT_EQ(info.inputs.back().value_type,
              stage.input_count == 2U ? V::kTypedContinuousVectorSizeTPtr : V::kGeDataType);
    std::string code;
    if (stage.output_count == 0U) {
      EXPECT_NE(builder->GenerateOutputsCreatorCode(code), ge::GRAPH_SUCCESS);
    } else {
      ASSERT_EQ(builder->GenerateOutputsCreatorCode(code), ge::GRAPH_SUCCESS);
      EXPECT_EQ(code, stage.code);
    }
    inputs.pop_back();
    KernelProgramInfo malformed;
    EXPECT_NE(builder->Collect(context, malformed), ge::GRAPH_SUCCESS);
  }
}

TEST_F(Om2KernelCollectCoverageUT, CopyFlowLaunchUsesFourInputsPerOutputAfterFrameworkPrefix) {
  CheckContract({"CopyFlowLaunch",
                 P::kOutputsCreator,
                 true,
                 {{"inputs_num", V::kSizeT},
                  {"inputs_index", V::kGertContinuousVectorVectorInt32Ptr},
                  {"rt_arg", V::kGertRtKernelLaunchArgsExPtr},
                  {"stream", V::kRtStreamTPtr},
                  {"allocator", V::kGertAllocatorPtr},
                  {"address_0", V::kGertTensorDataPtr},
                  {"length_0", V::kSizeT},
                  {"shape_0", V::kGertStorageShapePtr},
                  {"data_type_0", V::kGeDataType},
                  {"address_1", V::kGertTensorDataPtr},
                  {"length_1", V::kSizeT},
                  {"shape_1", V::kGertStorageShapePtr},
                  {"data_type_1", V::kGeDataType}},
                 {{"address_0"}, {"address_1"}}});
  RejectCounts("CopyFlowLaunch", 12U, 2U);
  RejectCounts("CopyFlowLaunch", 13U, 1U);
}

TEST_F(Om2KernelCollectCoverageUT, GatherShapesCreatorUsesIntegerWidthAndHostPlacement) {
  for (const auto dtype : {ge::DT_INT32, ge::DT_INT64}) {
    auto *node = Node("GatherShapesKernel");
    ASSERT_NE(node, nullptr);
    node->GetOpDescBarePtr()->SetStreamId(9);
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), "dtype", dtype));
    auto builder = KernelCodeBuilderFactory::GetInstance().Create("GatherShapesKernel");
    ASSERT_NE(builder, nullptr);
    const std::vector<uint32_t> inputs{11U, 12U}, outputs{101U};
    KernelCollectContext context(*node, inputs, outputs);
    std::string code;
    EXPECT_NE(builder->GenerateOutputsCreatorCode(code), ge::GRAPH_SUCCESS);
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    ExpectPorts(info.inputs, IndexedPorts("shape_", 2U, V::kGertStorageShapePtr), 11U);
    ExpectPorts(info.outputs, {{"tensor_data", V::kGertTensorDataPtr}}, 101U);
    ASSERT_EQ(builder->GenerateOutputsCreatorCode(code), ge::GRAPH_SUCCESS);
    const size_t bytes = Shape::kMaxDimNum * (dtype == ge::DT_INT32 ? sizeof(int32_t) : sizeof(int64_t));
    EXPECT_EQ(code, "InitInlineGertTensorDataOutput(chains_[101U], " + std::to_string(bytes) +
                        "U, static_cast<gert::TensorPlacement>(" + std::to_string(static_cast<int32_t>(kOnHost)) +
                        "), 9)");
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), "dtype", ge::DT_FLOAT));
    KernelProgramInfo invalid;
    EXPECT_NE(builder->Collect(context, invalid), ge::GRAPH_SUCCESS);
  }
  RejectCounts("GatherShapesKernel", 2U, 0U);
}

TEST_F(Om2KernelCollectCoverageUT, NotifyCodeValidatesCountAndExtractsEachSelectedNotify) {
  auto *node = Node("CreateNotifies");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("CreateNotifies");
  ASSERT_NE(builder, nullptr);
  std::string code;
  EXPECT_NE(builder->GenerateKernelCode("BindNotifies", code), ge::GRAPH_SUCCESS);
  const std::vector<uint32_t> inputs{11U, 12U}, outputs{101U, 102U};
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ExpectPorts(info.inputs, {{"notifies", V::kTypedContinuousVectorRtNotifyTPtr}, {"notify_num", V::kInt64T}}, 11U);
  ExpectPorts(info.outputs, IndexedPorts("notify_", 2U, V::kRtNotifyTPtr), 101U);
  EXPECT_EQ(builder->GetImplType(), KernelImplType::kCodegenImpl);
  ASSERT_EQ(builder->GenerateKernelCode("BindNotifies", code), ge::GRAPH_SUCCESS);
  EXPECT_NE(code.find("aclError BindNotifies()"), std::string::npos);
  EXPECT_NE(code.find("chains_[11U].GetValue<gert::TypedContinuousVector<aclrtNotify> *>()"), std::string::npos);
  EXPECT_NE(code.find("chains_[12U].GetPointer<int64_t>()"), std::string::npos);
  EXPECT_NE(code.find("static_cast<size_t>(*notify_num) != 2U"), std::string::npos);
  EXPECT_NE(code.find("chains_[101U].Set(static_cast<void *>(notifies->GetData()[0]), nullptr)"), std::string::npos);
  EXPECT_NE(code.find("chains_[102U].Set(static_cast<void *>(notifies->GetData()[1]), nullptr)"), std::string::npos);
}

const std::vector<PortContract> kLegacyLaunchPrefix = {{"stream", V::kRtStreamTPtr},
                                                       {"binary_handle", V::kVoidPtr},
                                                       {"block_dim", V::kUInt64T},
                                                       {"workspace_data", V::kGertTensorDataVectorPtr},
                                                       {"shape_buffer_data", V::kGertTensorDataPtr},
                                                       {"task_config", V::kRtTaskCfgInfoTPtr},
                                                       {"io_num", V::kSizeT},
                                                       {"schedule_mode", V::kUInt32T},
                                                       {"dfx_arg", V::kGertDfxExeArgPtr},
                                                       {"rt_arg", V::kGertRtKernelLaunchArgsExPtr},
                                                       {"local_memory_size", V::kUInt32T}};

TEST_F(Om2KernelCollectCoverageUT, LegacyLaunchVariantsPreserveHandleFlagAtomicAndMixLayouts) {
  const auto handle = JoinPorts(kLegacyLaunchPrefix, {{"tiling_key", V::kSizeT}, {"node_info", V::kCStringPtr}});
  const auto mix_tail = std::vector<PortContract>{{"mix_args", V::kGertMixCoreArgsPtr},
                                                  {"sub_stream", V::kRtStreamTPtr},
                                                  {"notify_0", V::kVoidPtr},
                                                  {"notify_1", V::kVoidPtr}};
  const auto atomic_tail = std::vector<PortContract>{{"workspace_indexes", V::kTypedContinuousVectorInt64Ptr},
                                                     {"clean_output_address_0", V::kGertTensorDataPtr},
                                                     {"clean_output_address_1", V::kGertTensorDataPtr},
                                                     {"clean_workspace_address", V::kGertTensorDataVectorPtr}};
  const auto io = IndexedPorts("io_address_or_shape_", 2U, V::kVoidPtr);
  const std::vector<CollectContract> contracts = {
      {"LaunchKernelWithFlag", P::kGraphBound, true, JoinPorts(kLegacyLaunchPrefix, io), {}},
      {"LaunchKernelWithHandle", P::kGraphBound, true, JoinPorts(handle, io), {}},
      {"LaunchMixKernelWithFlag", P::kGraphBound, true, JoinPorts(JoinPorts(kLegacyLaunchPrefix, io), mix_tail), {}},
      {"LaunchMixKernelWithHandle", P::kGraphBound, true, JoinPorts(JoinPorts(handle, io), mix_tail), {}},
      {"AtomicLaunchKernelWithFlag", P::kGraphBound, true, JoinPorts(kLegacyLaunchPrefix, atomic_tail), {}},
      {"AtomicLaunchKernelWithHandle",
       P::kGraphBound,
       true,
       JoinPorts(JoinPorts(kLegacyLaunchPrefix, {{"tiling_key", V::kSizeT}}), atomic_tail),
       {}}};
  for (const auto &entry : contracts) {
    CheckContract(entry);
    RejectCounts(entry.type, 10U, 0U);
    RejectCounts(entry.type, entry.inputs.size(), 1U);
  }
  CheckContract({"ReportRollbackError", P::kGraphBound, false, IndexedPorts("input_", 2U, V::kVoidPtr), {}});
}

TEST_F(Om2KernelCollectCoverageUT, AtomicV2LaunchRestoresIoCountAndSeparatesWorkspaceAddresses) {
  auto *node = Node("AtomicLaunchKernelV2");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("AtomicLaunchKernelV2");
  ASSERT_NE(builder, nullptr);
  constexpr size_t prefix = static_cast<size_t>(kernel::WithAtomic::kIoAddrs);
  constexpr size_t count_index = static_cast<size_t>(kernel::InputCommon::kIoNum);
  const auto inputs = ChainIndices(prefix + 3U, 11U);
  const std::vector<uint32_t> outputs;
  std::vector<Chain> values(inputs.size()), output_values;
  values[count_index].GetValue<size_t>() = 2U;
  KernelCollectContext context(*node, inputs, outputs, values, output_values);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  EXPECT_EQ(builder->GetRestoreConstInputIndices(), (std::vector<uint32_t>{static_cast<uint32_t>(count_index)}));
  ASSERT_EQ(info.inputs.size(), inputs.size());
  EXPECT_EQ(info.inputs[prefix - 1U].semantic_name, "workspace_indexes");
  EXPECT_EQ(info.inputs[prefix - 1U].value_type, V::kTypedContinuousVectorInt64Ptr);
  EXPECT_EQ(info.inputs[prefix].semantic_name, "clean_output_address_0");
  EXPECT_EQ(info.inputs[prefix + 1U].semantic_name, "clean_output_address_1");
  EXPECT_EQ(info.inputs[prefix + 1U].value_type, V::kGertTensorDataPtr);
  EXPECT_EQ(info.inputs.back().semantic_name, "clean_workspace_address");
  EXPECT_EQ(info.inputs.back().value_type, V::kGertTensorDataVectorPtr);
  values[count_index].GetValue<size_t>() = 3U;
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(context, invalid), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelCollectCoverageUT, AicpuRuntimePortsPreserveShapeDataGroupingAndTfHandle) {
  const std::vector<CollectContract> contracts = {
      {"UpdateAicpuIoAddr",
       P::kGraphBound,
       false,
       {{"args_handler", V::kAicpuArgsHandlerPtr},
        {"tensor_data_0", V::kGertTensorDataPtr},
        {"tensor_data_1", V::kGertTensorDataPtr}},
       {{"updated_0"}, {"updated_1"}}},
      {"AicpuLaunchTfKernel",
       P::kGraphBound,
       false,
       {{"args_handler", V::kAicpuArgsHandlerPtr},
        {"stream", V::kRtStreamTPtr},
        {"bin_handle", V::kVoidPtr},
        {"node_type", V::kCStringPtr}},
       {{"launch_0"}}},
      {"AllocHostCpuOutputMemory",
       P::kOutputsCreator,
       true,
       {{"size", V::kSizeT},
        {"allocator", V::kGertAllocatorPtr},
        {"input_context_0", V::kVoidPtr},
        {"input_context_1", V::kVoidPtr}},
       {{"tensor_data"}}},
      {"AicpuHostExecFunc",
       P::kGraphBound,
       true,
       {{"input_shape_0", V::kGertStorageShapePtr},
        {"input_shape_1", V::kGertStorageShapePtr},
        {"input_data_0", V::kGertTensorDataPtr},
        {"input_data_1", V::kGertTensorDataPtr},
        {"host_exec_func", V::kAicpuHostProcFuncPtr}},
       {{"output_shape_0"}, {"output_shape_1"}, {"output_data_0"}, {"output_data_1"}}}};
  for (const auto &entry : contracts) {
    CheckContract(entry);
  }
  RejectCounts("UpdateAicpuIoAddr", 0U, 1U);
  RejectCounts("AicpuLaunchTfKernel", 3U, 1U);
  RejectCounts("AllocHostCpuOutputMemory", 1U, 1U);
  RejectCounts("AicpuHostExecFunc", 0U, 0U);
  RejectCounts("AicpuHostExecFunc", 2U, 2U);
  RejectCounts("AicpuHostExecFunc", 3U, 3U);
}

TEST_F(Om2KernelCollectCoverageUT, WeightOffsetRestoreSerializesOffsetSizeAndPlacementWithoutDereferencing) {
  auto *node = Node("AssignWeightMemory");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("AssignWeightMemory");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{11U, 12U, 13U}, outputs{101U};
  std::vector<Chain> values(inputs.size()), output_values;
  TensorData tensor;
  ASSERT_EQ(tensor.SetAddr(reinterpret_cast<void *>(uintptr_t{4096U}), nullptr), ge::GRAPH_SUCCESS);
  tensor.SetSize(256U);
  tensor.SetPlacement(kOnDeviceHbm);
  values[0U].GetValue<TensorData *>() = &tensor;
  KernelCollectContext context(*node, inputs, outputs, values, output_values);
  CustomConstRestoreDescriptor descriptor;
  EXPECT_EQ(builder->GetCustomRestoreConstInputIndices(), (std::vector<uint32_t>{0U}));
  EXPECT_NE(builder->RestoreConstInput(context, 1U, descriptor), ge::GRAPH_SUCCESS);
  ASSERT_EQ(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
  EXPECT_EQ(descriptor.initializer, "tensor_data_offset");
  EXPECT_TRUE(descriptor.payload_is_string);
  std::string expected = "4096,256," + std::to_string(static_cast<int32_t>(kOnDeviceHbm));
  expected.push_back('\0');
  EXPECT_EQ(descriptor.payload, (std::vector<uint8_t>(expected.begin(), expected.end())));
  values[0U].GetValue<TensorData *>() = nullptr;
  EXPECT_NE(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelCollectCoverageUT, KnownModelRestoreReadsConstBytesAndDistinguishesNullPointer) {
  auto consumer = std::make_shared<ge::OpDesc>("restore_model", "DavinciModelCreate");
  ASSERT_EQ(consumer->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
  auto *node = graph_.AddNode(consumer);
  ASSERT_NE(node, nullptr);
  auto producer = std::make_shared<ge::OpDesc>("model_address", "Const");
  ASSERT_EQ(producer->AddOutputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
  auto *constant = graph_.AddNode(producer);
  ASSERT_NE(constant, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("DavinciModelCreate");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{11U}, outputs;
  KernelCollectContext context(*node, inputs, outputs);
  CustomConstRestoreDescriptor descriptor;
  EXPECT_EQ(builder->GetCustomRestoreConstInputIndices(), (std::vector<uint32_t>{0U}));
  EXPECT_NE(builder->RestoreConstInput(context, 1U, descriptor), ge::GRAPH_SUCCESS);
  EXPECT_NE(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
  ASSERT_NE(graph_.AddEdge(constant, 0, node, 0), nullptr);
  EXPECT_NE(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
  for (const uintptr_t address : {uintptr_t{0U}, uintptr_t{8192U}}) {
    ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
        producer, kConstValue, ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&address), sizeof(address))));
    ASSERT_EQ(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
    EXPECT_EQ(descriptor.initializer, address == 0U ? "null_pointer" : "process_local_pointer");
    const std::vector<uint8_t> expected =
        address == 0U ? std::vector<uint8_t>{0U} : std::vector<uint8_t>{'8', '1', '9', '2', 0U};
    EXPECT_EQ(descriptor.payload, expected);
  }
  const uint8_t invalid_size = 1U;
  ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(producer, kConstValue, ge::Buffer::CopyFrom(&invalid_size, 1U)));
  EXPECT_NE(builder->RestoreConstInput(context, 0U, descriptor), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelCollectCoverageUT, CustomAicpuBinaryCodeUsesContentKeyAndDedicatedLeaseSlot) {
  auto *node = Node("BuildCCArgsBinHandle");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("BuildCCArgsBinHandle");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{11U, 12U, 13U, 14U}, outputs{101U, 102U};
  std::vector<Chain> values(4U), output_values;
  values[0U].GetValue<ge::OpDesc *>() = node->GetOpDescBarePtr();
  char node_type[] = "CustomAdd";
  char so_name[] = "libcustom.so";
  values[1U].GetValue<char *>() = node_type;
  values[2U].GetValue<uint32_t>() = static_cast<uint32_t>(ge::ccKernelType::CUST_AI_CPU);
  values[3U].GetValue<char *>() = so_name;
  KernelCollectContext context(*node, inputs, outputs, values, output_values);
  KernelProgramInfo missing;
  EXPECT_NE(builder->Collect(context, missing), ge::GRAPH_SUCCESS);
  auto binary = std::make_shared<ge::OpKernelBin>(so_name, std::vector<char>{'E', 'L', 'F', '\0', 'X'});
  node->GetOpDescBarePtr()->SetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL, binary);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  std::string code;
  ASSERT_EQ(builder->GenerateKernelCode("LoadCustom", code), ge::GRAPH_SUCCESS);
  const auto key = GetKernelBinaryKey(binary->GetBinData(), binary->GetBinDataSize());
  EXPECT_EQ(code, "  aclError LoadCustom() {\n    return LoadCustAicpuBin(101U, 102U, \"" + key + "\");\n  }\n");
  ExpectPorts(info.outputs, {{"bin_handle"}, {"bin_resource"}}, 101U);
}

const std::vector<CollectContract> kFrameworkContracts = {
    {"SinkNodeBinWithHandle",
     P::kGraphBound,
     true,
     {{"binary", V::kGertKernelBinDataPtr}, {"kernel_name", V::kCStringPtr}},
     {{"binary_handle"}}},
    {"SinkNodeBinWithoutHandle",
     P::kGraphBound,
     true,
     {{"binary", V::kGertKernelBinDataPtr},
      {"stub_name", V::kCStringPtr},
      {"metadata", V::kCStringPtr},
      {"kernel_name", V::kCStringPtr}},
     {{"stub_func"}}},
    {"AllocLaunchArg",
     P::kOutputsCreator,
     true,
     {{"node_desc", V::kRtKernelLaunchArgsExComputeNodeDescPtr}, {"args_info_desc", V::kGertArgsInfosDescPtr}},
     {{"holder"}, {"rt_arg"}, {"rt_arg_args_base"}, {"tiling_data_base"}}},
    {"GetExtInfoAddrFromArgs",
     P::kGraphBound,
     false,
     {{"args_handler", V::kAicpuArgsHandlerPtr}},
     {{"host_ext_info_addr"}, {"device_ext_info_addr"}}},
    {"SplitVariable",
     P::kOutputsCreator,
     true,
     {{"rt_session", V::kRtSessionPtr}, {"var_id", V::kCStringPtr}, {"allocator", V::kGertAllocatorPtr}},
     {{"shape"}, {"tensor_data"}}},
    {"AllocModelOutTensor",
     P::kOutputsCreator,
     true,
     {{"allocator", V::kGertAllocatorPtr}, {"alloc_size", V::kSizeT}, {"output_data", V::kGertTensorPtr}},
     {{"tensor_data"}}},
    {"GetAutofuseFuncs",
     P::kGraphBound,
     false,
     {{"so_path", V::kCStringPtr}},
     {{"autofuse_so"},
      {"tiling_func"},
      {"infer_shape_func"},
      {"get_tiling_cache_key_func"},
      {"tiling_parse_func"},
      {"dfx_input_symbol_info_func"}}},
    {"SymbolTilingParse",
     P::kGraphBound,
     false,
     {{"platform_info", V::kFePlatFormInfosPtr}, {"tiling_parse_func", V::kKernelFuncPtr}},
     {{"compile_info"}}},
    {"BuildTensor",
     P::kOutputsCreator,
     false,
     {{"shape", V::kGertStorageShapePtr},
      {"tensor_data", V::kGertTensorDataPtr},
      {"tensor_attr", V::kGertBuildTensorAttrPtr}},
     {{"tensor"}}},
    {"BuildRefTensor",
     P::kOutputsCreator,
     false,
     {{"shape", V::kGertStorageShapePtr},
      {"tensor_data", V::kGertTensorDataPtr},
      {"tensor_attr", V::kGertBuildTensorAttrPtr}},
     {{"tensor"}}},
    {"SplitDataTensor",
     P::kOutputsCreator,
     true,
     {{"tensor", V::kGertTensorPtr}, {"allocator", V::kGertAllocatorPtr}},
     {{"shape"}, {"tensor_data"}}},
    {"SplitConstTensor",
     P::kOutputsCreator,
     true,
     {{"tensor", V::kGertTensorPtr}, {"stream_id", V::kInt64T}},
     {{"shape"}, {"tensor_data"}}},
    {"GetSpaceRegistry",
     P::kGraphBound,
     false,
     {{"model_desc", V::kGertModelDescPtr}, {"opp_impl_version", V::kSizeT}},
     {{"space_registry"}}},
    {"GetSpaceRegistries", P::kGraphBound, false, {{"model_desc", V::kGertModelDescPtr}}, {{"space_registries"}}},
    {"GetFileConstantWeightDir",
     P::kGraphBound,
     false,
     {{"model_desc", V::kGertModelDescPtr}},
     {{"file_constant_weight_dir"}}},
    {"NoOp", P::kGraphBound, false, {}, {}},
    {"GenIndexForIf", P::kGraphBound, false, {{"cond", V::kGertTensorPtr}}, {{"branch_index", V::kUInt32T}}},
    {"GenIndexForCase",
     P::kGraphBound,
     false,
     {{"cond", V::kGertTensorPtr}, {"branch_num", V::kUInt32T}},
     {{"branch_index", V::kUInt32T}}},
    {"GenCondForWhile",
     P::kGraphBound,
     false,
     {{"data", V::kGertTensorDataPtr}, {"shape", V::kGertStorageShapePtr}, {"data_type", V::kGeDataType}},
     {{"cond", V::kBool}}},
    {"CalcTensorSizeFromShape",
     P::kGraphBound,
     true,
     {{"data_type", V::kGeDataType}, {"shape", V::kGertShapePtr}},
     {{"tensor_size"}}},
    {"CalcTensorSizeFromStorage",
     P::kGraphBound,
     true,
     {{"data_type", V::kGeDataType}, {"storage_shape", V::kGertStorageShapePtr}},
     {{"tensor_size"}}},
    {"CalcUnalignedTensorSizeFromStorage",
     P::kGraphBound,
     true,
     {{"data_type", V::kGeDataType}, {"storage_shape", V::kGertStorageShapePtr}},
     {{"tensor_size"}}},
    {"AppendCoreTypeToPlatform",
     P::kOutputsCreator,
     false,
     {{"platform_holder", V::kFePlatFormInfosPtr},
      {"core_type", V::kGertCoreTypeIndexPtr},
      {"op_ai_core_num", V::kInt32T},
      {"op_vector_core_num", V::kInt32T},
      {"core_num_infos_holder", V::kGertCoreNumInfosPtr}},
     {{"platform_holder"}, {"core_num_infos_holder"}}}};

TEST_F(Om2KernelCollectCoverageUT, FrameworkContractsKeepControlScalarsAndTensorMetadataTyped) {
  for (const auto &entry : kFrameworkContracts) {
    CheckContract(entry);
    RejectCounts(entry.type, entry.inputs.size() + 1U, entry.outputs.size());
    RejectCounts(entry.type, entry.inputs.size(), entry.outputs.size() + 1U);
  }
  for (const auto *type : {"BuildTensor", "BuildRefTensor"}) {
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(type);
    ASSERT_NE(builder, nullptr);
    EXPECT_EQ(builder->GetRestoreConstInputIndices(), (std::vector<uint32_t>{2U}));
  }
  auto ref_builder = KernelCodeBuilderFactory::GetInstance().Create("BuildRefTensor");
  ASSERT_NE(ref_builder, nullptr);
  auto *ref_node = Node("BuildRefTensor");
  ASSERT_NE(ref_node, nullptr);
  const std::vector<uint32_t> ref_inputs{31U, 37U, 41U}, ref_outputs{43U};
  KernelCollectContext ref_context(*ref_node, ref_inputs, ref_outputs);
  KernelProgramInfo ref_info;
  ASSERT_EQ(ref_builder->Collect(ref_context, ref_info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(ref_info.inputs.size(), 3U);
  EXPECT_EQ(ref_info.inputs[0].value_type, V::kGertStorageShapePtr);
  EXPECT_EQ(ref_info.inputs[0].chain_index, 31U);
  EXPECT_EQ(ref_info.inputs[1].value_type, V::kGertTensorDataPtr);
  EXPECT_EQ(ref_info.inputs[1].chain_index, 37U);
}

TEST_F(Om2KernelCollectCoverageUT, BuildRefTensorValidatesConstPortPayloadsWithoutCni) {
  const ge::GeTensorDesc desc;
  auto ref_desc = std::make_shared<ge::OpDesc>("ref", "BuildRefTensor");
  for (size_t i = 0U; i < 3U; ++i) {
    ASSERT_EQ(ref_desc->AddInputDesc(desc), ge::GRAPH_SUCCESS);
  }
  ASSERT_EQ(ref_desc->AddOutputDesc(desc), ge::GRAPH_SUCCESS);
  auto *ref = graph_.AddNode(ref_desc);
  ASSERT_NE(ref, nullptr);

  const auto add_const = [&](const char *name) {
    auto constant = std::make_shared<ge::OpDesc>(name, "Const");
    EXPECT_EQ(constant->AddOutputDesc(desc), ge::GRAPH_SUCCESS);
    return graph_.AddNode(constant);
  };
  auto *shape = add_const("shape");
  auto *data = add_const("data");
  ASSERT_NE(shape, nullptr);
  ASSERT_NE(data, nullptr);
  ASSERT_NE(graph_.AddEdge(shape, 0, ref, 0), nullptr);
  ASSERT_NE(graph_.AddEdge(data, 0, ref, 1), nullptr);
  const auto set_bytes = [](ge::FastNode &node, size_t size) {
    const std::vector<uint8_t> payload(size, 0U);
    return ge::AttrUtils::SetZeroCopyBytes(node.GetOpDescBarePtr(), kConstValue,
                                           ge::Buffer::CopyFrom(payload.data(), payload.size()));
  };
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("BuildRefTensor");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{31U, 37U, 41U}, outputs{43U};
  const KernelCollectContext context(*ref, inputs, outputs);
  ASSERT_TRUE(set_bytes(*data, sizeof(GertTensorData)));
  KernelProgramInfo missing;
  EXPECT_NE(builder->Collect(context, missing), ge::GRAPH_SUCCESS);
  for (const size_t shape_size : {sizeof(StorageShape), sizeof(Tensor)}) {
    SCOPED_TRACE(shape_size);
    ASSERT_TRUE(set_bytes(*shape, shape_size));
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    EXPECT_FALSE(info.require_compute_node_info);
  }
  ASSERT_TRUE(set_bytes(*data, 1U));
  KernelProgramInfo malformed;
  EXPECT_NE(builder->Collect(context, malformed), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelCollectCoverageUT, IdentityAndShapeBuildersPreserveParallelPortGroups) {
  struct Variant {
    const char *type;
    const char *input;
    const char *output;
    V value_type;
    P policy;
    bool compute;
  };
  const std::vector<Variant> variants = {
      {"IdentityAddr", "address_", "address_", V::kGertTensorDataPtr, P::kOutputsCreator, false},
      {"IdentityShape", "shape_", "shape_", V::kGertStorageShapePtr, P::kOutputsCreator, true},
      {"PointFromInputs", "value_", "value_", V::kVoidPtr, P::kGraphBound, false},
      {"BuildShapeTensorData", "shape_", "tensor_data_", V::kGertStorageShapePtr, P::kOutputsCreator, true}};
  for (const auto &variant : variants) {
    for (size_t count : {0U, 2U}) {
      CheckContract({variant.type, variant.policy, variant.compute,
                     IndexedPorts(variant.input, count, variant.value_type), IndexedPorts(variant.output, count)});
    }
    RejectCounts(variant.type, 2U, 1U);
  }
  CheckContract({"IdentityShapeAndAddr", P::kOutputsCreator, true,
                 JoinPorts(IndexedPorts("shape_", 2U, V::kGertStorageShapePtr),
                           IndexedPorts("address_", 2U, V::kGertTensorDataPtr)),
                 JoinPorts(IndexedPorts("shape_", 2U), IndexedPorts("address_", 2U))});
  RejectCounts("IdentityShapeAndAddr", 4U, 2U);
}

TEST_F(Om2KernelCollectCoverageUT, AutofusePlatformAndAicpuExtensionAllowVariableTensorCounts) {
  for (size_t count : {0U, 2U}) {
    CheckContract(
        {"GetSymbolTilingCacheKey",
         P::kOutputsCreator,
         false,
         JoinPorts(JoinPorts({{"input_data_num", V::kSizeT}}, IndexedPorts("input_tensor_", count, V::kGertTensorPtr)),
                   {{"get_symbol_tiling_cache_key_func", V::kKernelFuncPtr}, {"all_symbol_num", V::kSizeT}}),
         {{"used_symbol_source"}}});
    CheckContract({"UpdateExtShape", P::kGraphBound, false,
                   JoinPorts({{"ext_info_handler", V::kGertAicpuExtInfoHandlerPtr},
                              {"input_num", V::kSizeT},
                              {"output_num", V::kSizeT},
                              {"stream", V::kRtStreamTPtr}},
                             IndexedPorts("shape_", count, V::kGertStorageShapePtr)),
                   IndexedPorts("update_", count)});
  }
  const std::vector<PortContract> outputs{{"platform_holder"}, {"core_num_infos_holder"}};
  CheckContract({"GetPlatformInfo", P::kOutputsCreator, false, {}, outputs});
  CheckContract({"GetPlatformInfo",
                 P::kOutputsCreator,
                 false,
                 {{"aicore_num", V::kInt32T}, {"vectorcore_num", V::kInt32T}},
                 outputs});
  RejectCounts("GetSymbolTilingCacheKey", 2U, 1U);
  RejectCounts("UpdateExtShape", 3U, 1U);
  RejectCounts("GetPlatformInfo", 1U, 2U);
}

TEST_F(Om2KernelCollectCoverageUT, SessionBindingAndShapeSelectorKeepLoadAndRunContractsDistinct) {
  auto *node = Node("GetSessionId");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("GetSessionId");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{11U}, outputs{101U};
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  EXPECT_EQ(builder->GetImplType(), KernelImplType::kEmptyImpl);
  EXPECT_TRUE(info.inputs.empty());
  EXPECT_TRUE(info.outputs.empty());
  ASSERT_EQ(info.chain_updates.size(), 2U);
  const auto &request = info.chain_updates[0U];
  EXPECT_EQ(request.chain_index, 11U);
  EXPECT_EQ(request.source_kind, ChainSourceKind::kInputBindingRequest);
  EXPECT_EQ(request.binding_kind, ChainBindingKind::kRtSession);
  EXPECT_EQ(request.binding_phase, ChainBindingPhase::kLoad);
  EXPECT_EQ(request.input_port, 0);
  const auto &update = info.chain_updates[1U];
  EXPECT_EQ(update.chain_index, 101U);
  EXPECT_EQ(update.source_kind, ChainSourceKind::kSessionId);
  EXPECT_EQ(update.binding_phase, ChainBindingPhase::kLoad);
  EXPECT_EQ(update.binding_kind, ChainBindingKind::kNone);
  EXPECT_EQ(update.source_index, kInvalidChainSourceIndex);
  RejectCounts("GetSessionId", 0U, 1U);
  RejectCounts("GetSessionId", 1U, 0U);
  for (size_t count : {0U, 2U}) {
    CheckContract({"CheckOutputShapesEmpty",
                   P::kGraphBound,
                   true,
                   IndexedPorts("output_shape_", count, V::kGertStorageShapePtr),
                   {{"selector", V::kUInt32T}}});
  }
  RejectCounts("CheckOutputShapesEmpty", 2U, 0U);
}

}  // namespace
}  // namespace om2
}  // namespace gert
