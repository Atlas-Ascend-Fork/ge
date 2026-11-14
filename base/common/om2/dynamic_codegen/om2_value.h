/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_OM2_VALUE_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_OM2_VALUE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gert {
namespace om2 {
enum class Om2ValueType : uint8_t {
  kInt32T,
  kInt64T,
  kUInt32T,
  kUInt64T,
  kSizeT,
  kGeDataType,
  kGertTensorPlacement,
  kGertCoreTypeIndexPtr,
  kGeOppImplVersion,
  kRtTaskCfgInfoTPtr,
  kGertDfxExeArgPtr,
  kGertBuildTensorAttrPtr,
  kRtKernelLaunchArgsExComputeNodeDescPtr,
  kGertArgsInfosDescPtr,
  kGertContinuousVectorVectorInt32Ptr,
  kCStringPtr,
  kVoidPtr,
  kGertTensorPtr,
  kGertTensorDataPtr,
  kGertTensorDataObjectPtr,
  kGertShapePtr,
  kGertStorageShapePtr,
  kGertAllocatorPtr,
  kGeAllocatorPtr,
  kGertAllocatorsPtr,
  kRtStreamTPtr,
  kTypedContinuousVectorRtStreamTPtr,
  kRtNotifyTPtr,
  kTypedContinuousVectorRtNotifyTPtr,
  kTypedContinuousVectorInt64Ptr,
  kTypedContinuousVectorSizeTPtr,
  kGertAllocatorVectorPtr,
  kGertTensorDataVectorPtr,
  kTypedContinuousVectorArgsIndexToIoIndexPtr,
  kGertModelDescPtr,
  kOuterWeightMemPtr,
  kGertOpImplSpaceRegistryV2Ptr,
  kGertCustomOpRegistryPtr,
  kGertKernelBinDataPtr,
  kKernelFuncPtr,
  kGertRtKernelLaunchArgsExPtr,
  kFePlatFormInfosPtr,
  kGertTilingFwkDataPtr,
  kGertCacheableTilingFwkDataPtr,
  kGeOpDescPtr,
  kAicpuArgsHandlerPtr,
  kGertAicpuExtInfoHandlerPtr,
  kGertCoreNumInfosPtr,
  kGertOpExecuteOptionsPtr,
  kGertMixCoreArgsPtr,
  kAicpuHostProcFuncPtr,
  kGeDavinciModelPtr,
  kGertOpImplSpaceRegistryV2ArrayPtr,
  kMemoryBaseTypeOffsetPtr,
  kContinuousVectorFileConstantNameAndMemPtr,
  kUnknown,
  kBool,
  kBytePtr,
  kRtSessionPtr,
  kGeKernelBinPtr,
  kAclrtLaunchKernelAttrPtr,
  kAclrtLaunchKernelCfgPtr,
  kAclnnDeterministicConfigPtr,
  kAclnnOriginalDeterministicConfigPtr
};

struct Om2MemoryBaseTypeOffsetValue {
  uint32_t base_type = 0U;
  int64_t offset = 0;
  uint64_t size = 0U;
};

struct Om2FileConstantNameAndMemValue {
  std::string name;
  uint64_t mem = 0U;
  uint64_t size = 0U;
};

struct Om2DfxExeArgValue {
  bool need_assert = false;
  bool need_print = false;
  int64_t buffer_size = 0;
};

struct Om2BuildTensorAttrValue {
  int32_t placement = 0;
  int32_t data_type = 0;
  int32_t storage_format = 0;
  int32_t origin_format = 0;
};

struct Om2ComputeNodeDescValue {
  size_t input_num = 0U;
  size_t output_num = 0U;
  size_t workspace_cap = 0U;
  size_t max_tiling_data = 0U;
  bool need_shape_buffer = false;
  bool need_overflow = false;
  std::vector<uint8_t> compiled_args;
};

struct Om2DynDescValue {
  bool is_group_first = false;
  uint32_t io_group_id = 0U;
  uint32_t io_index = 0U;
  uint32_t dyn_num = 0U;
  uint32_t group_size = 0U;
  uint32_t group_offset = 0U;
  uint32_t ptr_offset = 0U;
};

struct Om2ArgsInfoValue {
  int32_t arg_type = 0;
  int32_t arg_format = 0;
  int32_t start_index = 0;
  uint32_t arg_size = 0U;
  bool folded_first = false;
  bool be_folded = false;
  Om2DynDescValue dyn_desc;
  size_t data_size = 0U;
};

struct Om2ArgsInfosDescValue {
  size_t input_args_info_num = 0U;
  size_t output_args_info_num = 0U;
  size_t input_valid_num = 0U;
  size_t output_valid_num = 0U;
  std::vector<Om2ArgsInfoValue> args_infos;
};

struct Om2ArgsIndexToIoIndexValue {
  int32_t args_role = 0;
  size_t args_index = 0U;
  size_t io_index = 0U;
};

struct Om2MixCoreArgsValue {
  uint32_t mix_type = 0U;
  uint32_t all_core_num = 0U;
  uint32_t vec_core_num = 0U;
  uint32_t ai_core_num = 0U;
};

struct Om2LaunchKernelAttrValue {
  int32_t id = 0;
  uint32_t value = 0U;
};

struct Om2AclnnDeterministicConfigValue {
  bool has_op_deterministic = false;
  int32_t op_deterministic = 0;
  bool has_op_deterministic_level = false;
  int32_t op_deterministic_level = 0;
};

struct Om2AclnnOriginalDeterministicConfigValue {
  int32_t deterministic = 0;
  int32_t deterministic_level = 0;
};

struct Om2TensorConstValue {
  std::vector<int64_t> origin_shape;
  std::vector<int64_t> storage_shape;
  int32_t origin_format = 0;
  int32_t storage_format = 0;
  int32_t placement = 0;
  int32_t data_type = 0;
  uint64_t data_offset = 0U;
  uint64_t data_size = 0U;
  uint32_t weight_index = 0U;
  bool has_weight_index = false;
};

struct Om2ConstValue {
  Om2ValueType type = Om2ValueType::kUnknown;
  bool is_null_pointer = false;
  int64_t signed_value = 0;
  uint64_t unsigned_value = 0U;
  std::string string_value;
  std::vector<uint8_t> raw_bytes;
  Om2DfxExeArgValue dfx_exe_arg;
  Om2BuildTensorAttrValue build_tensor_attr;
  Om2ComputeNodeDescValue compute_node_desc;
  Om2ArgsInfosDescValue args_infos_desc;
  Om2MixCoreArgsValue mix_core_args;
  Om2TensorConstValue tensor;
  std::vector<std::vector<int32_t>> continuous_vector_vector_int32;
  std::vector<int64_t> continuous_vector_int64;
  std::vector<size_t> continuous_vector_size_t;
  std::vector<Om2ArgsIndexToIoIndexValue> continuous_vector_args_index_to_io_index;
  std::vector<uint8_t> op_execute_options;
  std::vector<Om2MemoryBaseTypeOffsetValue> memory_base_type_offsets;
  std::vector<Om2FileConstantNameAndMemValue> file_constant_name_and_mems;
  std::vector<Om2LaunchKernelAttrValue> launch_kernel_attrs;
  Om2AclnnDeterministicConfigValue aclnn_deterministic_config;
  Om2AclnnOriginalDeterministicConfigValue aclnn_original_deterministic_config;
};

}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_OM2_VALUE_H_
