/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "const_value_collector.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "securec.h"

#include "acl/acl_rt.h"
#include "common/checker.h"
#include "common/ge_common/util.h"
#include "core/builder/node_types.h"
#include "engine/aicore/launch_kernel/rt_kernel_launch_args_ex.h"
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"
#include "engine/aicore/kernel/aclnn_op_execute_kernel.h"
#include "kernel/known_subgraph/davinci_model_kernel.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/op_execute_context.h"
#include "common/op_tiling/tiling_dfx.h"
#include "exe_graph/runtime/storage_shape.h"
#include "framework/common/debug/ge_log.h"
#include "graph/buffer.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/math_util.h"
#include "graph/utils/tensor_utils.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "kernel/tensor_attr.h"
#include "ge/ge_ir_build.h"

namespace gert {
namespace om2 {
namespace {
const char *GetValueTypeName(const Om2ValueType type) {
  switch (type) {
    case Om2ValueType::kBool:
      return "bool";
    case Om2ValueType::kInt32T:
      return "int32_t";
    case Om2ValueType::kInt64T:
      return "int64_t";
    case Om2ValueType::kUInt32T:
      return "uint32_t";
    case Om2ValueType::kUInt64T:
      return "uint64_t";
    case Om2ValueType::kSizeT:
      return "size_t";
    case Om2ValueType::kGeDataType:
      return "ge::DataType";
    case Om2ValueType::kGertTensorPlacement:
      return "gert::TensorPlacement";
    case Om2ValueType::kGertCoreTypeIndexPtr:
      return "gert::kernel::CoreTypeIndex*";
    case Om2ValueType::kGeOppImplVersion:
      return "ge::OppImplVersion";
    case Om2ValueType::kRtTaskCfgInfoTPtr:
      return "rtTaskCfgInfo_t*";
    case Om2ValueType::kGertDfxExeArgPtr:
      return "gert::DfxExeArg*";
    case Om2ValueType::kGertBuildTensorAttrPtr:
      return "gert::kernel::BuildTensorAttr*";
    case Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr:
      return "gert::RtKernelLaunchArgsEx::ComputeNodeDesc*";
    case Om2ValueType::kGertArgsInfosDescPtr:
      return "gert::ArgsInfosDesc*";
    case Om2ValueType::kGertContinuousVectorVectorInt32Ptr:
      return "gert::ContinuousVectorVector* (int32_t payload)";
    case Om2ValueType::kCStringPtr:
      return "ge::char_t* (C string)";
    case Om2ValueType::kBytePtr:
      return "uint8_t* (raw bytes)";
    case Om2ValueType::kVoidPtr:
      return "void*";
    case Om2ValueType::kGertTensorPtr:
      return "gert::Tensor*";
    case Om2ValueType::kGertTensorDataPtr:
      return "gert::GertTensorData*";
    case Om2ValueType::kGertTensorDataObjectPtr:
      return "gert::TensorData*";
    case Om2ValueType::kGertShapePtr:
      return "gert::Shape*";
    case Om2ValueType::kGertStorageShapePtr:
      return "gert::StorageShape*";
    case Om2ValueType::kGertAllocatorPtr:
      return "gert::GertAllocator*";
    case Om2ValueType::kGeAllocatorPtr:
      return "ge::Allocator*";
    case Om2ValueType::kGertAllocatorsPtr:
      return "gert::Allocators*";
    case Om2ValueType::kRtStreamTPtr:
      return "rtStream_t";
    case Om2ValueType::kTypedContinuousVectorRtStreamTPtr:
      return "gert::TypedContinuousVector<rtStream_t>*";
    case Om2ValueType::kRtNotifyTPtr:
      return "aclrtNotify";
    case Om2ValueType::kTypedContinuousVectorRtNotifyTPtr:
      return "gert::TypedContinuousVector<aclrtNotify>*";
    case Om2ValueType::kTypedContinuousVectorInt64Ptr:
      return "gert::TypedContinuousVector<int64_t>*";
    case Om2ValueType::kTypedContinuousVectorSizeTPtr:
      return "gert::TypedContinuousVector<size_t>*";
    case Om2ValueType::kGertAllocatorVectorPtr:
      return "gert::TypedContinuousVector<gert::GertAllocator*>*";
    case Om2ValueType::kGertTensorDataVectorPtr:
      return "gert::TypedContinuousVector<gert::GertTensorData*>*";
    case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr:
      return "gert::TypedContinuousVector<optiling::ArgsIndexToIoIndex>*";
    case Om2ValueType::kGertModelDescPtr:
      return "gert::ModelDesc*";
    case Om2ValueType::kRtSessionPtr:
      return "gert::RtSession*";
    case Om2ValueType::kOuterWeightMemPtr:
      return "gert::OuterWeightMem*";
    case Om2ValueType::kGertOpImplSpaceRegistryV2Ptr:
      return "gert::OpImplSpaceRegistryV2*";
    case Om2ValueType::kGertCustomOpRegistryPtr:
      return "ge::CustomOpRegistry*";
    case Om2ValueType::kGertKernelBinDataPtr:
      return "gert::kernel::BinData*";
    case Om2ValueType::kGeKernelBinPtr:
      return "ge::KernelBinPtr*";
    case Om2ValueType::kAclrtLaunchKernelAttrPtr:
      return "aclrtLaunchKernelAttr*";
    case Om2ValueType::kAclrtLaunchKernelCfgPtr:
      return "aclrtLaunchKernelCfg*";
    case Om2ValueType::kKernelFuncPtr:
      return "function pointer";
    case Om2ValueType::kGertRtKernelLaunchArgsExPtr:
      return "gert::RtKernelLaunchArgsEx*";
    case Om2ValueType::kFePlatFormInfosPtr:
      return "fe::PlatFormInfos*";
    case Om2ValueType::kGertTilingFwkDataPtr:
      return "gert::TilingFwkData*";
    case Om2ValueType::kGertCacheableTilingFwkDataPtr:
      return "gert::CacheableTilingFwkData*";
    case Om2ValueType::kGeOpDescPtr:
      return "ge::OpDesc*";
    case Om2ValueType::kAicpuArgsHandlerPtr:
      return "gert::AicpuArgsHandler*";
    case Om2ValueType::kGertAicpuExtInfoHandlerPtr:
      return "gert::AicpuExtInfoHandler*";
    case Om2ValueType::kGertCoreNumInfosPtr:
      return "gert::kernel::CoreNumInfos*";
    case Om2ValueType::kGertOpExecuteOptionsPtr:
      return "gert::OpExecuteOptions*";
    case Om2ValueType::kGertMixCoreArgsPtr:
      return "gert::kernel::MixCoreArgs*";
    case Om2ValueType::kAclnnDeterministicConfigPtr:
      return "gert::kernel::AclnnDeterministicConfig*";
    case Om2ValueType::kAclnnOriginalDeterministicConfigPtr:
      return "gert::kernel::AclnnOriginalDeterministicConfig*";
    case Om2ValueType::kAicpuHostProcFuncPtr:
      return "gert::AicpuHostProcFunc";
    case Om2ValueType::kGeDavinciModelPtr:
      return "ge::DavinciModel*";
    case Om2ValueType::kGertOpImplSpaceRegistryV2ArrayPtr:
      return "gert::OpImplSpaceRegistryV2Array*";
    case Om2ValueType::kMemoryBaseTypeOffsetPtr:
      return "gert::kernel::MemoryBaseTypeOffset*";
    case Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr:
      return "gert::ContinuousVector* (FileConstantNameAndMem payload)";
    case Om2ValueType::kUnknown:
      return "unknown";
    default:
      return "invalid";
  }
}

template <typename T>
ge::graphStatus ReadScalar(const ge::FastNode &node, const ge::Buffer &buffer, T &value) {
  if (buffer.size() != sizeof(T)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Const node %s has %zu bytes, expected %zu", node.GetNamePtr(), buffer.size(),
           sizeof(T));
    return ge::GRAPH_PARAM_INVALID;
  }
  GE_ASSERT_EOK(memcpy_s(&value, sizeof(value), buffer.data(), sizeof(T)));
  return ge::GRAPH_SUCCESS;
}

bool IsNullPointerType(const Om2ValueType type) {
  switch (type) {
    case Om2ValueType::kVoidPtr:
    case Om2ValueType::kGertTensorDataPtr:
    case Om2ValueType::kGertTensorDataObjectPtr:
    case Om2ValueType::kGertAllocatorPtr:
    case Om2ValueType::kGeAllocatorPtr:
    case Om2ValueType::kGertAllocatorsPtr:
    case Om2ValueType::kRtStreamTPtr:
    case Om2ValueType::kRtNotifyTPtr:
    case Om2ValueType::kGertModelDescPtr:
    case Om2ValueType::kRtSessionPtr:
    case Om2ValueType::kOuterWeightMemPtr:
    case Om2ValueType::kGertOpImplSpaceRegistryV2Ptr:
    case Om2ValueType::kGertCustomOpRegistryPtr:
    case Om2ValueType::kGertKernelBinDataPtr:
    case Om2ValueType::kKernelFuncPtr:
    case Om2ValueType::kFePlatFormInfosPtr:
      return true;
    default:
      return false;
  }
}

ge::graphStatus ParseString(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  if ((buffer.size() == 0U) || (buffer[buffer.size() - 1U] != 0U)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 string Const node %s is not null terminated", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  const auto *const begin = buffer.data();
  const auto *const end = begin + buffer.size() - 1U;
  if (std::find(begin, end, 0U) != end) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 string Const node %s contains an embedded null", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  value.string_value.assign(reinterpret_cast<const char *>(begin), buffer.size() - 1U);
  return ge::GRAPH_SUCCESS;
}

bool IsCustomKernel(const ge::FastNode *node) {
  if (node == nullptr || node->GetOpDescBarePtr() == nullptr) {
    return false;
  }
  std::string kernel_name;
  return ge::AttrUtils::GetStr(node->GetOpDescBarePtr(), gert::attr::kCustomKernelName, kernel_name) &&
         !kernel_name.empty();
}

bool HasCustomDataUser(const ge::FastNode &node) {
  for (const auto &output_edges : node.GetAllOutDataEdgesRef()) {
    for (const auto *const edge : output_edges) {
      if (edge != nullptr && IsCustomKernel(edge->dst)) {
        return true;
      }
    }
  }
  return false;
}

ge::graphStatus ParseModelBufferDataConst(const ge::FastNode &node, const ge::Buffer &buffer,
                                          CustomConstRestoreDescriptor &descriptor, const uint32_t chain_index) {
  if (buffer.size() != sizeof(ge::ModelBufferData *)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Custom ModelBufferData Const %s has invalid handle size %zu", node.GetNamePtr(),
           buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::ModelBufferData *model_buffer = nullptr;
  GE_ASSERT_EOK(memcpy_s(&model_buffer, sizeof(model_buffer), buffer.data(), sizeof(model_buffer)));
  if (model_buffer == nullptr || model_buffer->length > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Custom ModelBufferData Const %s has invalid buffer metadata", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  const size_t length = static_cast<size_t>(model_buffer->length);
  const uint8_t *const data = model_buffer->data.get();
  if (length != 0U && data == nullptr) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Custom ModelBufferData Const %s has a null buffer with size %zu",
           node.GetNamePtr(), length);
    return ge::GRAPH_PARAM_INVALID;
  }
  descriptor.chain_index = chain_index;
  descriptor.producer_name = node.GetNamePtr();
  descriptor.initializer = "model_buffer_data";
  descriptor.payload_is_string = false;
  if (length != 0U) {
    descriptor.payload.assign(data, data + length);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseOpExecuteOptions(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  if (buffer.size() != sizeof(OpExecuteOptions)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 OpExecuteOptions Const node %s has %zu bytes, expected %zu", node.GetNamePtr(),
           buffer.size(), sizeof(OpExecuteOptions));
    return ge::GRAPH_PARAM_INVALID;
  }
  value.op_execute_options.assign(buffer.data(), buffer.data() + buffer.size());
  return ge::GRAPH_SUCCESS;
}

// FxRT's dynamic AICore lowering stores a StorageShape value inline in a Const
// node while the consumer ABI is StorageShape*.  Keep only its portable dims;
// the raw object also contains reserved/layout bytes and must not be persisted.
ge::graphStatus ParseStorageShapeConst(const ge::FastNode &node, const ge::Buffer &buffer,
                                       CustomConstRestoreDescriptor &descriptor, const uint32_t chain_index) {
  if (buffer.size() != sizeof(gert::StorageShape)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  struct RawShape {
    size_t dim_num;
    int64_t dims[gert::Shape::kMaxDimNum];
    uint8_t reserved[40];
  };
  struct RawStorageShape {
    RawShape origin_shape;
    RawShape storage_shape;
    uint8_t reserved[40];
  } raw{};
  static_assert(sizeof(RawStorageShape) == sizeof(gert::StorageShape), "StorageShape layout changed");
  GE_ASSERT_EOK(memcpy_s(&raw, sizeof(raw), buffer.data(), sizeof(raw)));
  if ((raw.origin_shape.dim_num > gert::Shape::kMaxDimNum) || (raw.storage_shape.dim_num > gert::Shape::kMaxDimNum)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 StorageShape Const node %s has invalid dim count", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  const size_t bytes = sizeof(uint32_t) * 2U + sizeof(int64_t) * gert::Shape::kMaxDimNum * 2U;
  descriptor.chain_index = chain_index;
  descriptor.producer_name = node.GetNamePtr();
  descriptor.initializer = "storage_shape";
  descriptor.payload_is_string = false;
  descriptor.payload.assign(bytes, 0U);
  const auto copy_shape = [](const RawShape &src, uint8_t *dst, const size_t dst_size) -> ge::graphStatus {
    const uint32_t dim_num = static_cast<uint32_t>(src.dim_num);
    GE_ASSERT_EOK(memcpy_s(dst, dst_size, &dim_num, sizeof(dim_num)));
    for (size_t i = 0U; i < src.dim_num; ++i) {
      const int64_t dim = src.dims[i];
      const size_t offset = sizeof(uint32_t) + i * sizeof(int64_t);
      GE_ASSERT_EOK(memcpy_s(dst + offset, dst_size - offset, &dim, sizeof(dim)));
    }
    return ge::GRAPH_SUCCESS;
  };
  GE_ASSERT_GRAPH_SUCCESS(copy_shape(raw.origin_shape, descriptor.payload.data(), descriptor.payload.size()));
  const size_t storage_shape_offset = sizeof(uint32_t) + sizeof(int64_t) * gert::Shape::kMaxDimNum;
  GE_ASSERT_GRAPH_SUCCESS(copy_shape(raw.storage_shape, descriptor.payload.data() + storage_shape_offset,
                                     descriptor.payload.size() - storage_shape_offset));
  return ge::GRAPH_SUCCESS;
}

// FxRT's dynamic AICore lowering stores a GertTensorData object inline in a
// Const node.  The object contains process-local manager/mem-block pointers;
// only the non-owning address description is valid to carry through OM2 in the
// current process.  Serialize the stable fields and rebuild a fresh object at
// load time.
ge::graphStatus ParseGertTensorDataConst(const ge::FastNode &node, const ge::Buffer &buffer,
                                         CustomConstRestoreDescriptor &descriptor, const uint32_t chain_index) {
  if (buffer.size() != sizeof(gert::GertTensorData)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  struct RawGertTensorData {
    uint8_t tensor_data[sizeof(gert::TensorData)];
    uintptr_t gert_mem_block;
    int64_t stream_id;
  } raw{};
  static_assert(sizeof(RawGertTensorData) == sizeof(gert::GertTensorData), "GertTensorData layout changed");
  GE_ASSERT_EOK(memcpy_s(&raw, sizeof(raw), buffer.data(), sizeof(raw)));
  void *addr = nullptr;
  size_t size = 0U;
  int32_t placement = static_cast<int32_t>(gert::kTensorPlacementEnd);
  GE_ASSERT_EOK(memcpy_s(&addr, sizeof(addr), raw.tensor_data, sizeof(addr)));
  GE_ASSERT_EOK(memcpy_s(&size, sizeof(size), raw.tensor_data + sizeof(void *) * 2U, sizeof(size)));
  GE_ASSERT_EOK(memcpy_s(&placement, sizeof(placement), raw.tensor_data + sizeof(void *) * 2U + sizeof(size_t),
                         sizeof(placement)));
  if ((placement < static_cast<int32_t>(gert::kOnDeviceHbm)) ||
      (placement >= static_cast<int32_t>(gert::kTensorPlacementEnd))) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 GertTensorData Const node %s has invalid placement %d", node.GetNamePtr(),
           placement);
    return ge::GRAPH_PARAM_INVALID;
  }
  descriptor.chain_index = chain_index;
  descriptor.producer_name = node.GetNamePtr();
  descriptor.initializer = "gert_tensor_data";
  descriptor.payload_is_string = true;
  const auto payload = std::to_string(reinterpret_cast<uintptr_t>(addr)) + "," + std::to_string(size) + "," +
                       std::to_string(placement) + "," + std::to_string(raw.stream_id);
  descriptor.payload.assign(payload.cbegin(), payload.cend());
  descriptor.payload.push_back(0U);
  return ge::GRAPH_SUCCESS;
}

// A dynamic tiling Const may carry a complete gert::Tensor object.  The object is
// standard-layout, but its TensorData contains a process-local manager callback;
// serialize only the fields needed to reconstruct the same non-owning Tensor.
struct TensorConstPayload {
  uint64_t address;
  uint64_t size;
  int32_t placement;
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  uint64_t expand_dims_size;
  uint64_t expand_dims_mask;
  uint32_t origin_dim_num;
  uint32_t storage_dim_num;
  int64_t origin_dims[gert::Shape::kMaxDimNum];
  int64_t storage_dims[gert::Shape::kMaxDimNum];
  uint64_t weight_index;
  uint64_t has_weight_index;
  uint64_t weight_data_offset;
  uint64_t weight_data_size;
};

struct RawTensorData {
  void *address;
  void *manager;
  size_t size;
  gert::TensorPlacement placement;
  uint32_t reserved_0;
  uint8_t reserved_1[40];
};

struct RawShape {
  size_t dim_num;
  int64_t dims[gert::Shape::kMaxDimNum];
  uint8_t reserved[40];
};

struct RawStorageShape {
  RawShape origin_shape;
  RawShape storage_shape;
  uint8_t reserved[40];
};

struct RawExpandDimsType {
  uint64_t bits;
  uint8_t reserved[40];
};

struct RawStorageFormat {
  int32_t origin_format;
  int32_t storage_format;
  RawExpandDimsType expand_dims_type;
  uint8_t reserved[40];
};

struct RawTensor {
  RawStorageShape storage_shape;
  RawStorageFormat storage_format;
  uint8_t version;
  uint8_t reserved[3];
  ge::DataType data_type;
  RawTensorData tensor_data;
  uint8_t reserved_field[40];
};

static_assert(sizeof(RawTensorData) == sizeof(gert::TensorData), "TensorData layout changed");
static_assert(sizeof(RawTensor) == sizeof(gert::Tensor), "Tensor layout changed");

ge::graphStatus ParseTensorConst(const ge::FastNode &node, const ge::Buffer &buffer,
                                 CustomConstRestoreDescriptor &descriptor, const uint32_t chain_index) {
  if (buffer.size() != sizeof(gert::Tensor)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  RawTensor raw{};
  GE_ASSERT_EOK(memcpy_s(&raw, sizeof(raw), buffer.data(), sizeof(raw)));
  if (raw.version != static_cast<uint8_t>(gert::kTensorV1) || raw.tensor_data.manager != nullptr) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor Const node %s has unsupported version or manager", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  // A shape-only Tensor owns no data buffer. TensorData::Release() -- and the failure path of
  // AddShareCount -- leave addr_ = nullptr, size_ = 0 and placement_ = kTensorPlacementEnd, which
  // is a legitimate state rather than corruption. LaunchKernelWithFlag's io_address_or_shape_<n>
  // shape slots are exactly such Tensors: UpdateEachArgsInfo only reads StorageShape * from them.
  // The payload copies placement through verbatim, so accepting it round-trips faithfully.
  const bool owns_no_data = (raw.tensor_data.address == nullptr) && (raw.tensor_data.size == 0U);
  const bool placement_valid =
      ((raw.tensor_data.placement >= gert::kOnDeviceHbm) && (raw.tensor_data.placement < gert::kTensorPlacementEnd)) ||
      ((raw.tensor_data.placement == gert::kTensorPlacementEnd) && owns_no_data);
  if (!placement_valid) {
    GELOGE(ge::GRAPH_PARAM_INVALID,
           "OM2 Tensor Const node %s has invalid TensorData fields: placement=%d address=%p size=%zu",
           node.GetNamePtr(), static_cast<int32_t>(raw.tensor_data.placement), raw.tensor_data.address,
           raw.tensor_data.size);
    return ge::GRAPH_PARAM_INVALID;
  }
  const auto valid_shape = [](const RawShape &shape) { return shape.dim_num <= gert::Shape::kMaxDimNum; };
  if (!valid_shape(raw.storage_shape.origin_shape) || !valid_shape(raw.storage_shape.storage_shape) ||
      (raw.storage_format.expand_dims_type.bits >> gert::ExpandDimsType::kMaxExpandSize) > 0xffU) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor Const node %s has invalid shape or expand-dims fields",
           node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  TensorConstPayload payload{};
  payload.address = reinterpret_cast<uintptr_t>(raw.tensor_data.address);
  payload.size = raw.tensor_data.size;
  // A Tensor with neither an address nor a size carries no data, so its placement is not
  // meaningful -- and the snapshot of it is not stable: the live object may be Release()d (which
  // sets kTensorPlacementEnd) either before or after collection, so the same Const has serialized
  // as kOnHost in one run and kTensorPlacementEnd in another.  Op impls do tell the two apart:
  // ReshapeInferShapeImpl accepts kTensorPlacementEnd as "shape not known yet" but reports
  // "shape input[0] is empty tensor" when the placement claims host data that is not there.  RT2
  // hands the kernel the live, released object, so normalize to match.  This also makes the baked
  // payload deterministic, which matters because it feeds the om2_compile_cache key.
  payload.placement =
      owns_no_data ? static_cast<int32_t>(gert::kTensorPlacementEnd) : static_cast<int32_t>(raw.tensor_data.placement);
  payload.data_type = static_cast<int32_t>(raw.data_type);
  payload.origin_format = raw.storage_format.origin_format;
  payload.storage_format = raw.storage_format.storage_format;
  payload.expand_dims_size = raw.storage_format.expand_dims_type.bits >> gert::ExpandDimsType::kMaxExpandSize;
  payload.expand_dims_mask = raw.storage_format.expand_dims_type.bits & ((uint64_t{1} << 56U) - 1U);
  payload.origin_dim_num = static_cast<uint32_t>(raw.storage_shape.origin_shape.dim_num);
  payload.storage_dim_num = static_cast<uint32_t>(raw.storage_shape.storage_shape.dim_num);
  std::copy(raw.storage_shape.origin_shape.dims, raw.storage_shape.origin_shape.dims + gert::Shape::kMaxDimNum,
            payload.origin_dims);
  std::copy(raw.storage_shape.storage_shape.dims, raw.storage_shape.storage_shape.dims + gert::Shape::kMaxDimNum,
            payload.storage_dims);
  int64_t weight_index = 0;
  const auto op_desc = node.GetOpDescBarePtr();
  bool has_weight_index = false;
  ge::ConstGeTensorPtr weight;
  if (ge::AttrUtils::GetTensor(op_desc, "value", weight) && weight != nullptr) {
    has_weight_index = ge::AttrUtils::GetInt(weight->GetTensorDesc(), "om2_weight_index", weight_index);
  }
  if (!has_weight_index) {
    has_weight_index = ge::AttrUtils::GetInt(op_desc->GetOutputDesc(0U), "om2_weight_index", weight_index);
  }
  if (has_weight_index && weight_index >= 0) {
    payload.weight_index = static_cast<uint64_t>(weight_index);
    payload.has_weight_index = 1U;
    int64_t data_offset = 0;
    if (ge::TensorUtils::GetDataOffset(op_desc->GetOutputDesc(0U), data_offset) == ge::GRAPH_SUCCESS &&
        data_offset >= 0) {
      payload.weight_data_offset = static_cast<uint64_t>(data_offset);
      // The lowering Const descriptor describes the carrier, not the tensor payload.
      payload.weight_data_size = payload.size;
    }
  }
  descriptor.chain_index = chain_index;
  descriptor.producer_name = node.GetNamePtr();
  descriptor.initializer = "gert_tensor";
  descriptor.payload_is_string = false;
  descriptor.payload.resize(sizeof(payload));
  GE_ASSERT_EOK(memcpy_s(descriptor.payload.data(), descriptor.payload.size(), &payload, sizeof(payload)));
  return ge::GRAPH_SUCCESS;
}

// Lowering also stores a bare gert::TensorData inline in a Const node: the 72-byte object itself,
// not the 88-byte GertTensorData wrapper that ParseGertTensorDataConst handles.  As with that
// wrapper, only the non-owning address description may travel through OM2, so serialize the stable
// fields and rebuild the object at load time.
ge::graphStatus ParseTensorDataObjectConst(const ge::FastNode &node, const ge::Buffer &buffer,
                                           CustomConstRestoreDescriptor &descriptor, const uint32_t chain_index) {
  if (buffer.size() != sizeof(gert::TensorData)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  RawTensorData raw{};
  GE_ASSERT_EOK(memcpy_s(&raw, sizeof(raw), buffer.data(), sizeof(raw)));
  // raw.manager is deliberately dropped, exactly as ParseGertTensorDataConst drops it: the manager
  // is a process-local free/share callback and cannot travel through OM2.  The restored object
  // therefore borrows the address without owning it, which matches how consumers use it --
  // UpdateEachArgsInfo only calls GetAddr()/GetSize() -- and keeps Free() from releasing memory
  // this object never owned.
  // See ParseTensorConst: a data-less TensorData is what Release() leaves behind and what
  // shape-only carriers hold, so kTensorPlacementEnd is valid exactly when nothing is owned.
  const bool owns_no_data = (raw.address == nullptr) && (raw.size == 0U);
  const auto placement = static_cast<int32_t>(raw.placement);
  const bool placement_valid = ((placement >= static_cast<int32_t>(gert::kOnDeviceHbm)) &&
                                (placement < static_cast<int32_t>(gert::kTensorPlacementEnd))) ||
                               ((placement == static_cast<int32_t>(gert::kTensorPlacementEnd)) && owns_no_data);
  if (!placement_valid) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 TensorData Const node %s has invalid placement %d address=%p size=%zu",
           node.GetNamePtr(), placement, raw.address, raw.size);
    return ge::GRAPH_PARAM_INVALID;
  }
  descriptor.chain_index = chain_index;
  descriptor.producer_name = node.GetNamePtr();
  descriptor.initializer = "gert_tensor_data_object";
  descriptor.payload_is_string = true;
  // Same normalization as ParseTensorConst: no address and no size means no data, so the placement
  // is not meaningful and the snapshot of it is not stable across runs.
  const auto normalized_placement = owns_no_data ? static_cast<int32_t>(gert::kTensorPlacementEnd) : placement;
  const auto payload = std::to_string(reinterpret_cast<uintptr_t>(raw.address)) + "," + std::to_string(raw.size) + "," +
                       std::to_string(normalized_placement);
  descriptor.payload.assign(payload.cbegin(), payload.cend());
  descriptor.payload.push_back(0U);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseLaunchKernelAttrs(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  // BuildLaunchCommonHead receives the five attributes produced by GetLaunchKernelV2Attr.
  constexpr size_t kAttrCount = 5U;
  GE_ASSERT_TRUE(buffer.size() == kAttrCount * sizeof(aclrtLaunchKernelAttr));
  const aclrtLaunchKernelAttrId expected_ids[] = {
      ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE, ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE,
      ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE, ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET,
      ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING};
  for (size_t i = 0U; i < kAttrCount; ++i) {
    aclrtLaunchKernelAttr source{};
    GE_ASSERT_EOK(memcpy_s(&source, sizeof(source), buffer.data() + i * sizeof(source), sizeof(source)));
    GE_ASSERT_TRUE(source.id == expected_ids[i], "OM2 launch attributes of %s have unexpected order",
                   node.GetNamePtr());
    uint32_t field = 0U;
    switch (source.id) {
      case ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE:
        field = source.value.schemMode;
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE:
        field = source.value.dynUBufSize;
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE:
        field = static_cast<uint32_t>(source.value.engineType);
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET:
        field = source.value.blockDimOffset;
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING:
        field = source.value.enableProfiling;
        break;
      default:
        return ge::GRAPH_PARAM_INVALID;
    }
    value.launch_kernel_attrs.push_back({static_cast<int32_t>(source.id), field});
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseLaunchKernelConfig(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  aclrtLaunchKernelCfg source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  GE_ASSERT_TRUE((source.attrs == nullptr) && (source.numAttrs == 5U));
  value.unsigned_value = source.numAttrs;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseAclnnDeterministicConfig(const ge::FastNode &node, const ge::Buffer &buffer,
                                              Om2ConstValue &value) {
  kernel::AclnnDeterministicConfig source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  value.aclnn_deterministic_config = {source.has_op_deterministic, source.op_deterministic,
                                      source.has_op_deterministic_level, source.op_deterministic_level};
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseAclnnOriginalDeterministicConfig(const ge::FastNode &node, const ge::Buffer &buffer,
                                                      Om2ConstValue &value) {
  kernel::AclnnOriginalDeterministicConfig source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  value.aclnn_original_deterministic_config = {source.deterministic, source.deterministic_level};
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseMixCoreArgs(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  kernel::MixCoreArgs source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  if ((source.mix_type != kernel::MixType::MIX_AICORE) && (source.mix_type != kernel::MixType::MIX_VECTOR_CORE)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 MixCoreArgs Const node %s has invalid mix type %u", node.GetNamePtr(),
           static_cast<uint32_t>(source.mix_type));
    return ge::GRAPH_PARAM_INVALID;
  }
  value.mix_core_args = {static_cast<uint32_t>(source.mix_type), source.all_core_num, source.vec_core_num,
                         source.ai_core_num};
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseDfxExeArg(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  DfxExeArg source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  value.dfx_exe_arg = {source.need_assert, source.need_print, source.buffer_size};
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseBuildTensorAttr(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  kernel::BuildTensorAttr source{};
  GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
  value.build_tensor_attr = {static_cast<int32_t>(source.placement), static_cast<int32_t>(source.data_type),
                             static_cast<int32_t>(source.storage_format.GetStorageFormat()),
                             static_cast<int32_t>(source.storage_format.GetOriginFormat())};
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseComputeNodeDesc(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  using ComputeNodeDesc = RtKernelLaunchArgsEx::ComputeNodeDesc;
  if (buffer.size() < sizeof(ComputeNodeDesc)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ComputeNodeDesc Const node %s is truncated: %zu bytes", node.GetNamePtr(),
           buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  ComputeNodeDesc source{};
  GE_ASSERT_EOK(memcpy_s(&source, sizeof(source), buffer.data(), sizeof(source)));
  size_t expected_size = 0U;
  if (ge::AddOverflow(sizeof(ComputeNodeDesc), source.compiled_args_size, expected_size) ||
      (buffer.size() != expected_size)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ComputeNodeDesc Const node %s size mismatch: actual %zu, compiled args %zu",
           node.GetNamePtr(), buffer.size(), source.compiled_args_size);
    return ge::GRAPH_PARAM_INVALID;
  }
  auto &output = value.compute_node_desc;
  output.input_num = source.input_num;
  output.output_num = source.output_num;
  output.workspace_cap = source.workspace_cap;
  output.max_tiling_data = source.max_tiling_data;
  output.need_shape_buffer = source.need_shape_buffer;
  output.need_overflow = source.need_overflow;
  output.compiled_args.assign(buffer.data() + offsetof(ComputeNodeDesc, compiled_args),
                              buffer.data() + offsetof(ComputeNodeDesc, compiled_args) + source.compiled_args_size);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseArgsInfosDesc(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  if (buffer.size() < sizeof(ArgsInfosDesc)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ArgsInfosDesc Const node %s is truncated: %zu bytes", node.GetNamePtr(),
           buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  std::unique_ptr<uint8_t[]> storage;
  try {
    storage = std::make_unique<uint8_t[]>(buffer.size());
  } catch (const std::bad_alloc &) {
    GELOGE(ge::GRAPH_FAILED, "Failed to allocate ArgsInfosDesc buffer for node %s, size %zu", node.GetNamePtr(),
           buffer.size());
    return ge::GRAPH_FAILED;
  }
  GE_ASSERT_EOK(memcpy_s(storage.get(), buffer.size(), buffer.data(), buffer.size()));
  const auto *const source = reinterpret_cast<const ArgsInfosDesc *>(storage.get());
  size_t args_info_num = 0U;
  size_t args_info_size = 0U;
  size_t expected_size = 0U;
  if (ge::AddOverflow(source->GetInputArgsInfoNum(), source->GetOutputArgsInfoNum(), args_info_num) ||
      ge::MulOverflow(args_info_num, sizeof(ArgsInfosDesc::ArgsInfo), args_info_size) ||
      ge::AddOverflow(sizeof(ArgsInfosDesc), args_info_size, expected_size) ||
      (source->GetArgsInfoSize() != args_info_size) || (buffer.size() != expected_size)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ArgsInfosDesc Const node %s has an invalid variable-length layout",
           node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }

  auto &output = value.args_infos_desc;
  output.input_args_info_num = source->GetInputArgsInfoNum();
  output.output_args_info_num = source->GetOutputArgsInfoNum();
  output.input_valid_num = source->GetValidInputArgsNum();
  output.output_valid_num = source->GetValidOutputArgsNum();
  output.args_infos.reserve(args_info_num);
  for (size_t i = 0U; i < args_info_num; ++i) {
    const auto *const info = source->GetArgsInfoByIndex(i);
    GE_ASSERT_NOTNULL(info);
    output.args_infos.push_back(
        {static_cast<int32_t>(info->arg_type),
         static_cast<int32_t>(info->arg_format),
         info->start_index,
         info->arg_size,
         info->folded_first,
         info->be_folded,
         {info->dyn_desc.is_group_first, info->dyn_desc.io_group_id, info->dyn_desc.io_index, info->dyn_desc.dyn_num,
          info->dyn_desc.group_size, info->dyn_desc.group_offset, info->dyn_desc.ptr_offset},
         info->data_size});
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadSizeField(const ge::Buffer &buffer, const size_t offset, size_t &value) {
  size_t end = 0U;
  if (ge::AddOverflow(offset, sizeof(value), end) || (end > buffer.size())) {
    return ge::GRAPH_PARAM_INVALID;
  }
  GE_ASSERT_EOK(memcpy_s(&value, sizeof(value), buffer.data() + offset, sizeof(value)));
  return ge::GRAPH_SUCCESS;
}

template <typename T>
ge::graphStatus ParseContinuousVector(const ge::FastNode &node, const ge::Buffer &buffer, std::vector<T> &value) {
  if (buffer.size() < sizeof(ContinuousVector)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVector Const node %s is truncated: %zu bytes", node.GetNamePtr(),
           buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }

  size_t capacity = 0U;
  size_t size = 0U;
  GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, 0U, capacity));
  GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, sizeof(size_t), size));
  if (size > capacity) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVector Const node %s has invalid size %zu, capacity %zu",
           node.GetNamePtr(), size, capacity);
    return ge::GRAPH_PARAM_INVALID;
  }

  size_t element_bytes = 0U;
  size_t expected_size = 0U;
  if (ge::MulOverflow(capacity, sizeof(T), element_bytes) ||
      ge::AddOverflow(sizeof(ContinuousVector), element_bytes, expected_size) || (expected_size != buffer.size())) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVector Const node %s size mismatch: actual %zu, expected %zu",
           node.GetNamePtr(), buffer.size(), expected_size);
    return ge::GRAPH_PARAM_INVALID;
  }

  constexpr size_t kDataPosition = sizeof(ContinuousVector) - sizeof(size_t);
  size_t data_bytes = 0U;
  size_t data_end = 0U;
  if (ge::MulOverflow(size, sizeof(T), data_bytes) || ge::AddOverflow(kDataPosition, data_bytes, data_end) ||
      (data_end > expected_size)) {
    return ge::GRAPH_PARAM_INVALID;
  }

  value.resize(size);
  if (data_bytes > 0U) {
    GE_ASSERT_EOK(memcpy_s(value.data(), data_bytes, buffer.data() + kDataPosition, data_bytes));
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseContinuousVectorVectorInt32(const ge::FastNode &node, const ge::Buffer &buffer,
                                                 Om2ConstValue &value) {
  if (buffer.size() < sizeof(ContinuousVectorVector)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVectorVector Const node %s is truncated: %zu bytes",
           node.GetNamePtr(), buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  size_t capacity = 0U;
  size_t vector_count = 0U;
  GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, 0U, capacity));
  GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, sizeof(size_t), vector_count));
  if (capacity != vector_count) {
    GELOGE(ge::GRAPH_PARAM_INVALID,
           "OM2 ContinuousVectorVector Const node %s is only partially initialized: size %zu, capacity %zu",
           node.GetNamePtr(), vector_count, capacity);
    return ge::GRAPH_PARAM_INVALID;
  }
  const size_t overhead = ContinuousVectorVector::GetOverHeadLength(capacity);
  if (overhead > buffer.size()) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVectorVector Const node %s has invalid overhead %zu",
           node.GetNamePtr(), overhead);
    return ge::GRAPH_PARAM_INVALID;
  }

  constexpr size_t kFirstOffsetPosition = sizeof(ContinuousVectorVector) - sizeof(size_t);
  constexpr size_t kVectorDataPosition = sizeof(ContinuousVector) - 8U;
  size_t expected_offset = overhead;
  value.continuous_vector_vector_int32.reserve(vector_count);
  for (size_t i = 0U; i < vector_count; ++i) {
    size_t offset_position = 0U;
    size_t vector_offset = 0U;
    if (ge::MulOverflow(i, sizeof(size_t), offset_position) ||
        ge::AddOverflow(kFirstOffsetPosition, offset_position, offset_position)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, offset_position, vector_offset));
    if (vector_offset != expected_offset) {
      GELOGE(ge::GRAPH_PARAM_INVALID,
             "OM2 ContinuousVectorVector Const node %s has invalid vector %zu offset %zu, expected %zu",
             node.GetNamePtr(), i, vector_offset, expected_offset);
      return ge::GRAPH_PARAM_INVALID;
    }

    size_t inner_capacity = 0U;
    size_t inner_size = 0U;
    GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, vector_offset, inner_capacity));
    GE_ASSERT_GRAPH_SUCCESS(ReadSizeField(buffer, vector_offset + sizeof(size_t), inner_size));
    if (inner_size > inner_capacity) {
      return ge::GRAPH_PARAM_INVALID;
    }
    size_t elements_size = 0U;
    size_t vector_storage_size = 0U;
    if (ge::MulOverflow(inner_capacity, sizeof(int32_t), elements_size) ||
        ge::AddOverflow(sizeof(ContinuousVector), elements_size, vector_storage_size) ||
        ge::AddOverflow(vector_offset, vector_storage_size, expected_offset) || (expected_offset > buffer.size())) {
      return ge::GRAPH_PARAM_INVALID;
    }
    size_t data_offset = 0U;
    size_t data_size = 0U;
    size_t data_end = 0U;
    if (ge::AddOverflow(vector_offset, kVectorDataPosition, data_offset) ||
        ge::MulOverflow(inner_size, sizeof(int32_t), data_size) || ge::AddOverflow(data_offset, data_size, data_end) ||
        (data_end > expected_offset)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    std::vector<int32_t> output(inner_size);
    if (data_size > 0U) {
      GE_ASSERT_EOK(memcpy_s(output.data(), data_size, buffer.data() + data_offset, data_size));
    }
    value.continuous_vector_vector_int32.emplace_back(std::move(output));
  }
  if (expected_offset != buffer.size()) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ContinuousVectorVector Const node %s size mismatch: parsed %zu, actual %zu",
           node.GetNamePtr(), expected_offset, buffer.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseTensorConst(const ge::FastNode &node, const ge::Buffer &buffer, Om2ConstValue &value) {
  if (buffer.size() != sizeof(gert::Tensor)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor Const node %s has %zu bytes, expected %zu", node.GetNamePtr(),
           buffer.size(), sizeof(gert::Tensor));
    return ge::GRAPH_PARAM_INVALID;
  }
  const auto *const tensor = reinterpret_cast<const gert::Tensor *>(buffer.data());
  const auto op_desc = node.GetOpDescBarePtr();
  GE_ASSERT_NOTNULL(op_desc);
  auto &output_desc = op_desc->GetOutputDesc(0U);
  int64_t data_offset = 0;
  int64_t data_size = 0;

  // Compact external weights use the tensor descriptor's local offset.  The
  // flatten offset includes preceding model weights and is only used without
  // a compact weight index.
  ge::ConstGeTensorPtr weight;
  int64_t weight_index = 0;
  bool has_weight_index = false;
  std::vector<int64_t> flatten_weight;
  if (ge::AttrUtils::GetTensor(op_desc, "value", weight) && weight != nullptr) {
    ge::GeAttrValue flatten_attr;
    const auto &weight_desc = weight->GetTensorDesc();
    has_weight_index = ge::AttrUtils::GetInt(weight_desc, "om2_weight_index", weight_index);
    if (!has_weight_index &&
        (weight_desc.GetAttr(ge::ATTR_NAME_GRAPH_FLATTEN_OFFSET, flatten_attr) == ge::GRAPH_SUCCESS) &&
        (flatten_attr.GetValue(flatten_weight) == ge::GRAPH_SUCCESS) && (flatten_weight.size() == 2U)) {
      data_offset = flatten_weight[0U];
      data_size = flatten_weight[1U];
    }
  }
  if (!has_weight_index) {
    has_weight_index = ge::AttrUtils::GetInt(output_desc, "om2_weight_index", weight_index);
  }
  if (flatten_weight.size() != 2U) {
    ge::GeAttrValue flatten_attr;
    if ((output_desc.GetAttr(ge::ATTR_NAME_GRAPH_FLATTEN_OFFSET, flatten_attr) == ge::GRAPH_SUCCESS) &&
        (flatten_attr.GetValue(flatten_weight) == ge::GRAPH_SUCCESS) && (flatten_weight.size() == 2U)) {
      data_offset = flatten_weight[0U];
      data_size = flatten_weight[1U];
    }
  }
  if (has_weight_index) {
    const ge::GeTensorDesc &offset_desc = (weight != nullptr) ? weight->GetTensorDesc() : output_desc;
    GE_ASSERT_GRAPH_SUCCESS(ge::TensorUtils::GetDataOffset(offset_desc, data_offset));
    if (flatten_weight.size() == 2U) {
      data_size = flatten_weight[1U];
    } else {
      GE_ASSERT_GRAPH_SUCCESS(ge::TensorUtils::GetTensorSizeInBytes(output_desc, data_size));
    }
  } else if (flatten_weight.size() != 2U) {
    GE_ASSERT_GRAPH_SUCCESS(ge::TensorUtils::GetDataOffset(output_desc, data_offset));
    GE_ASSERT_GRAPH_SUCCESS(ge::TensorUtils::GetTensorSizeInBytes(output_desc, data_size));
  }
  GE_ASSERT_TRUE((data_offset >= 0) && (data_size >= 0), "OM2 Tensor Const node %s has invalid weight range",
                 node.GetNamePtr());

  const auto copy_shape = [](const auto &source, std::vector<int64_t> &target) {
    target.clear();
    target.reserve(source.GetDimNum());
    for (size_t i = 0U; i < source.GetDimNum(); ++i) {
      target.emplace_back(source.GetDim(i));
    }
  };
  copy_shape(tensor->GetOriginShape(), value.tensor.origin_shape);
  copy_shape(tensor->GetStorageShape(), value.tensor.storage_shape);
  value.tensor.origin_format = static_cast<int32_t>(tensor->GetOriginFormat());
  value.tensor.storage_format = static_cast<int32_t>(tensor->GetStorageFormat());
  value.tensor.placement = static_cast<int32_t>(tensor->GetPlacement());
  value.tensor.data_type = static_cast<int32_t>(tensor->GetDataType());
  value.tensor.data_offset = static_cast<uint64_t>(data_offset);
  value.tensor.data_size = static_cast<uint64_t>(data_size);
  if (has_weight_index) {
    GE_ASSERT_TRUE(weight_index >= 0, "OM2 Tensor Const node %s has invalid weight index", node.GetNamePtr());
    value.tensor.weight_index = static_cast<uint32_t>(weight_index);
    value.tensor.has_weight_index = true;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseConstValue(const ge::FastNode &node, const Om2ValueType type, Om2ConstValue &value) {
  ge::Buffer buffer;
  if (!ge::AttrUtils::GetZeroCopyBytes(node.GetOpDescBarePtr(), kConstValue, buffer)) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Const node %s has no value bytes", node.GetNamePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  GELOGI("OM2 parsing Const node %s as %s, bytes=%zu", node.GetNamePtr(), GetValueTypeName(type), buffer.size());
  value.type = type;
  switch (type) {
    case Om2ValueType::kBool: {
      bool source = false;
      GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
      value.unsigned_value = source ? 1U : 0U;
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kAclrtLaunchKernelAttrPtr:
      return ParseLaunchKernelAttrs(node, buffer, value);
    case Om2ValueType::kAclrtLaunchKernelCfgPtr:
      return ParseLaunchKernelConfig(node, buffer, value);
    case Om2ValueType::kInt32T: {
      int32_t source = 0;
      GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
      value.signed_value = source;
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kInt64T: {
      int64_t source = 0;
      GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
      value.signed_value = source;
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kUInt32T:
    case Om2ValueType::kGeDataType:
    case Om2ValueType::kGertTensorPlacement:
    case Om2ValueType::kGertCoreTypeIndexPtr:
    case Om2ValueType::kGeOppImplVersion: {
      uint32_t source = 0U;
      GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
      value.unsigned_value = source;
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kUInt64T: {
      if (buffer.size() == sizeof(uint32_t)) {
        uint32_t source = 0U;
        GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
        value.unsigned_value = source;
      } else {
        uint64_t source = 0U;
        GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
        value.unsigned_value = source;
      }
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kSizeT: {
      // A size_t-valued run_func may receive an enum-width (uint32_t) lowering constant.
      // Chain stores both forms as the same zero-extended inline value; preserve that public
      // ABI conversion instead of requiring every lowering producer to change its storage width.
      // bool is also passed through the size_t getter by a few framework kernels. Normalize
      // its one-byte representation to the same unsigned value used by the generated code.
      if (buffer.size() == sizeof(bool)) {
        bool source = false;
        GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
        value.unsigned_value = source ? 1U : 0U;
      } else if (buffer.size() == sizeof(uint32_t)) {
        uint32_t source = 0U;
        GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
        value.unsigned_value = source;
      } else {
        size_t source = 0U;
        GE_ASSERT_GRAPH_SUCCESS(ReadScalar(node, buffer, source));
        value.unsigned_value = source;
      }
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kRtTaskCfgInfoTPtr:
      if ((buffer.size() != sizeof(rtTaskCfgInfo_t)) ||
          !std::all_of(buffer.data(), buffer.data() + buffer.size(), [](const uint8_t byte) { return byte == 0U; })) {
        GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 TaskConfig Const node %s is not the supported zero config",
               node.GetNamePtr());
        return ge::GRAPH_PARAM_INVALID;
      }
      return ge::GRAPH_SUCCESS;
    case Om2ValueType::kGertDfxExeArgPtr:
      return ParseDfxExeArg(node, buffer, value);
    case Om2ValueType::kGertBuildTensorAttrPtr:
      return ParseBuildTensorAttr(node, buffer, value);
    case Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr:
      return ParseComputeNodeDesc(node, buffer, value);
    case Om2ValueType::kGertArgsInfosDescPtr:
      return ParseArgsInfosDesc(node, buffer, value);
    case Om2ValueType::kGertContinuousVectorVectorInt32Ptr:
      return ParseContinuousVectorVectorInt32(node, buffer, value);
    case Om2ValueType::kTypedContinuousVectorInt64Ptr:
      return ParseContinuousVector(node, buffer, value.continuous_vector_int64);
    case Om2ValueType::kTypedContinuousVectorSizeTPtr:
      return ParseContinuousVector(node, buffer, value.continuous_vector_size_t);
    case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr: {
      std::vector<optiling::ArgsIndexToIoIndex> source;
      GE_ASSERT_GRAPH_SUCCESS(ParseContinuousVector(node, buffer, source));
      value.continuous_vector_args_index_to_io_index.reserve(source.size());
      for (const auto &item : source) {
        if ((item.args_role != optiling::ArgsRole::kInput) && (item.args_role != optiling::ArgsRole::kOutput)) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 ArgsIndexToIoIndex Const node %s has invalid args role %d",
                 node.GetNamePtr(), static_cast<int32_t>(item.args_role));
          return ge::GRAPH_PARAM_INVALID;
        }
        value.continuous_vector_args_index_to_io_index.push_back(
            {static_cast<int32_t>(item.args_role), item.args_index, item.io_index});
      }
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kCStringPtr:
      return ParseString(node, buffer, value);
    case Om2ValueType::kGertOpExecuteOptionsPtr:
      return ParseOpExecuteOptions(node, buffer, value);
    case Om2ValueType::kGertMixCoreArgsPtr:
      return ParseMixCoreArgs(node, buffer, value);
    case Om2ValueType::kAclnnDeterministicConfigPtr:
      return ParseAclnnDeterministicConfig(node, buffer, value);
    case Om2ValueType::kAclnnOriginalDeterministicConfigPtr:
      return ParseAclnnOriginalDeterministicConfig(node, buffer, value);
    case Om2ValueType::kMemoryBaseTypeOffsetPtr: {
      if ((buffer.size() == 0U) || ((buffer.size() % sizeof(gert::kernel::MemoryBaseTypeOffset)) != 0U)) {
        GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 MemoryBaseTypeOffset Const node %s has invalid bytes %zu",
               node.GetNamePtr(), buffer.size());
        return ge::GRAPH_PARAM_INVALID;
      }
      const auto *const items = reinterpret_cast<const gert::kernel::MemoryBaseTypeOffset *>(buffer.data());
      const size_t count = buffer.size() / sizeof(gert::kernel::MemoryBaseTypeOffset);
      value.memory_base_type_offsets.reserve(count);
      for (size_t i = 0U; i < count; ++i) {
        if (items[i].base_type >= gert::kernel::MemoryBaseType::kMemoryBaseTypeEnd) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 MemoryBaseTypeOffset Const node %s has invalid base type %u at %zu",
                 node.GetNamePtr(), static_cast<uint32_t>(items[i].base_type), i);
          return ge::GRAPH_PARAM_INVALID;
        }
        value.memory_base_type_offsets.push_back(
            {static_cast<uint32_t>(items[i].base_type), items[i].offset, items[i].size});
      }
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr: {
      std::vector<gert::kernel::FileConstantNameAndMem> source;
      GE_ASSERT_GRAPH_SUCCESS(ParseContinuousVector(node, buffer, source));
      value.file_constant_name_and_mems.reserve(source.size());
      for (size_t i = 0U; i < source.size(); ++i) {
        if (source[i].name[0U] == '\0') {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 FileConstantNameAndMem Const node %s has empty name at %zu",
                 node.GetNamePtr(), i);
          return ge::GRAPH_PARAM_INVALID;
        }
        Om2FileConstantNameAndMemValue item;
        item.name.assign(source[i].name);
        item.mem = ge::PtrToValue(source[i].mem);
        item.size = source[i].size;
        value.file_constant_name_and_mems.push_back(std::move(item));
      }
      return ge::GRAPH_SUCCESS;
    }
    case Om2ValueType::kBytePtr:
      if (buffer.size() == 0U) {
        value.raw_bytes.clear();
      } else {
        value.raw_bytes.assign(buffer.data(), buffer.data() + buffer.size());
      }
      return ge::GRAPH_SUCCESS;
    case Om2ValueType::kGertTensorPtr:
      return ParseTensorConst(node, buffer, value);
    case Om2ValueType::kVoidPtr:
    case Om2ValueType::kGertTensorDataPtr:
    case Om2ValueType::kGertTensorDataObjectPtr:
    case Om2ValueType::kOuterWeightMemPtr:
    case Om2ValueType::kGertShapePtr:
    case Om2ValueType::kGertStorageShapePtr:
    case Om2ValueType::kGertAllocatorPtr:
    case Om2ValueType::kGertAllocatorsPtr:
    case Om2ValueType::kRtStreamTPtr:
    case Om2ValueType::kTypedContinuousVectorRtStreamTPtr:
    case Om2ValueType::kRtNotifyTPtr:
    case Om2ValueType::kTypedContinuousVectorRtNotifyTPtr:
    case Om2ValueType::kGertAllocatorVectorPtr:
    case Om2ValueType::kGertTensorDataVectorPtr:
    case Om2ValueType::kGertModelDescPtr:
    case Om2ValueType::kRtSessionPtr:
    case Om2ValueType::kGertOpImplSpaceRegistryV2Ptr:
    case Om2ValueType::kGertCustomOpRegistryPtr:
    case Om2ValueType::kGertKernelBinDataPtr:
    case Om2ValueType::kKernelFuncPtr:
    case Om2ValueType::kGertRtKernelLaunchArgsExPtr:
    case Om2ValueType::kFePlatFormInfosPtr:
    case Om2ValueType::kGertTilingFwkDataPtr:
    case Om2ValueType::kGertCacheableTilingFwkDataPtr:
    case Om2ValueType::kGeOpDescPtr:
    case Om2ValueType::kAicpuArgsHandlerPtr:
    case Om2ValueType::kGertAicpuExtInfoHandlerPtr:
    case Om2ValueType::kAicpuHostProcFuncPtr:
    case Om2ValueType::kGeDavinciModelPtr:
    case Om2ValueType::kGertOpImplSpaceRegistryV2ArrayPtr:
    case Om2ValueType::kGertCoreNumInfosPtr:
    case Om2ValueType::kUnknown:
    default:
      if (IsNullPointerType(type) && (buffer.size() == sizeof(void *)) &&
          std::all_of(buffer.data(), buffer.data() + buffer.size(), [](const uint8_t byte) { return byte == 0U; })) {
        value.is_null_pointer = true;
        return ge::GRAPH_SUCCESS;
      }
      GELOGE(ge::GRAPH_PARAM_INVALID,
             "OM2 Const node %s has non-serializable process-local value type %s; it must be a runtime binding",
             node.GetNamePtr(), GetValueTypeName(type));
      return ge::GRAPH_PARAM_INVALID;
  }
}

bool IsProcessLocalPointerType(const Om2ValueType type) {
  switch (type) {
    case Om2ValueType::kVoidPtr:
    case Om2ValueType::kKernelFuncPtr:
    case Om2ValueType::kGertOpImplSpaceRegistryV2Ptr:
    case Om2ValueType::kGertOpImplSpaceRegistryV2ArrayPtr:
    case Om2ValueType::kGeDavinciModelPtr:
    case Om2ValueType::kGertCustomOpRegistryPtr:
    case Om2ValueType::kFePlatFormInfosPtr:
    case Om2ValueType::kGertCoreNumInfosPtr:
    case Om2ValueType::kGertTensorPtr:
    case Om2ValueType::kGertStorageShapePtr:
    case Om2ValueType::kGertShapePtr:
    case Om2ValueType::kGertTensorDataPtr:
    case Om2ValueType::kGertTensorDataObjectPtr:
    case Om2ValueType::kGertContinuousVectorVectorInt32Ptr:
    case Om2ValueType::kTypedContinuousVectorInt64Ptr:
    case Om2ValueType::kTypedContinuousVectorSizeTPtr:
    case Om2ValueType::kGertTensorDataVectorPtr:
    case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr:
    case Om2ValueType::kGertRtKernelLaunchArgsExPtr:
    case Om2ValueType::kGertTilingFwkDataPtr:
    case Om2ValueType::kGertCacheableTilingFwkDataPtr:
    case Om2ValueType::kGeOpDescPtr:
    case Om2ValueType::kAicpuArgsHandlerPtr:
    case Om2ValueType::kGertAicpuExtInfoHandlerPtr:
    case Om2ValueType::kRtSessionPtr:
      return true;
    default:
      return false;
  }
}

// ContinuousVector-family constants hold a pointer to their elements and therefore classify as
// process-local, but ParseConstValue already serializes the elements themselves and codegen
// re-materializes a fresh container at load time (InitContinuousVectorConst<T> /
// InitContinuousVectorVectorInt32Const).  That portable path is strictly better than baking a
// process-local address into the generated code -- which would also be stale once an
// om2_compile_cache hit loads the .so in a different process -- so prefer it.
bool HasPortableConstValueParse(const Om2ValueType value_type) {
  switch (value_type) {
    case Om2ValueType::kGertContinuousVectorVectorInt32Ptr:
    case Om2ValueType::kTypedContinuousVectorInt64Ptr:
    case Om2ValueType::kTypedContinuousVectorSizeTPtr:
    case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr:
      return true;
    default:
      return false;
  }
}

ge::graphStatus RecordConstantInputTypes(const ProgramBlock &block, const std::vector<ChainInfo> &chains,
                                         std::unordered_map<uint32_t, Om2ValueType> &expected_types,
                                         std::unordered_map<uint32_t, Om2ValueType> &observed_types) {
  const auto record_type = [&chains, &expected_types, &observed_types](
                               const uint32_t chain_index, const Om2ValueType value_type, const char *const owner_name,
                               const char *const port_name) -> ge::graphStatus {
    if (value_type == Om2ValueType::kUnknown) {
      return ge::GRAPH_SUCCESS;
    }
    if ((chain_index >= chains.size()) || (chains[chain_index].index != chain_index)) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 program step %s has invalid chain %u", owner_name, chain_index);
      return ge::GRAPH_PARAM_INVALID;
    }
    if (chains[chain_index].source_kind != ChainSourceKind::kConstant) {
      return ge::GRAPH_SUCCESS;
    }
    auto normalized_type =
        ((value_type == Om2ValueType::kGertShapePtr) || (value_type == Om2ValueType::kGertStorageShapePtr))
            ? Om2ValueType::kGertTensorPtr
            : value_type;
    const auto observed_iter = observed_types.find(chain_index);
    if ((normalized_type == Om2ValueType::kVoidPtr) && (observed_iter != observed_types.cend()) &&
        (observed_iter->second == Om2ValueType::kGertTensorPtr)) {
      normalized_type = Om2ValueType::kGertTensorPtr;
    } else if ((normalized_type == Om2ValueType::kGertTensorPtr) && (observed_iter != observed_types.cend()) &&
               (observed_iter->second == Om2ValueType::kVoidPtr)) {
      observed_iter->second = Om2ValueType::kGertTensorPtr;
    }
    // Every runtime consumer of a constant must agree on its C++ ABI type, including
    // lifecycle-bound constants.  Load-bound values are not serialized below,
    // but their consumer contracts still determine which process-local object
    // the ProgramBody must bind.
    const auto result = observed_types.emplace(chain_index, normalized_type);
    if (!result.second && (result.first->second != normalized_type)) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Const chain %u has conflicting input contracts %s and %s at %s port %s",
             chain_index, GetValueTypeName(result.first->second), GetValueTypeName(normalized_type), owner_name,
             port_name);
      return ge::GRAPH_PARAM_INVALID;
    }
    if (chains[chain_index].binding_phase != ChainBindingPhase::kLoad) {
      const auto existing = expected_types.find(chain_index);
      if (existing == expected_types.end() || existing->second == Om2ValueType::kVoidPtr ||
          value_type == Om2ValueType::kGertStorageShapePtr) {
        expected_types[chain_index] = value_type == Om2ValueType::kGertShapePtr ? normalized_type : value_type;
      }
    }
    return ge::GRAPH_SUCCESS;
  };

  for (const auto &kernel : block.kernels) {
    for (const auto &port : kernel.inputs) {
      if (std::find(kernel.compile_time_const_chain_indices.cbegin(), kernel.compile_time_const_chain_indices.cend(),
                    port.chain_index) != kernel.compile_time_const_chain_indices.cend()) {
        continue;
      }
      GE_ASSERT_GRAPH_SUCCESS(record_type(port.chain_index, port.value_type, kernel.execution_node_name.c_str(),
                                          port.semantic_name.c_str()));
    }
  }

  // An If selector is a consumer too, even though it is represented by a
  // ProgramStep rather than a KernelProgramInfo.  Record its typed contract
  // here so a selector produced by a Const node is serialized exactly like a
  // scalar kernel input.
  for (const auto &region : block.regions) {
    for (const auto &step : region.steps) {
      if (step.kind != ProgramStepKind::kIf) {
        continue;
      }
      GE_ASSERT_GRAPH_SUCCESS(record_type(step.if_info.selector_chain_index, step.if_info.selector_value_type,
                                          step.if_info.execution_node_name.c_str(), "selector"));
    }
  }
  return ge::GRAPH_SUCCESS;
}

}  // namespace

ge::graphStatus ConstValueCollector::Collect(const ge::ExecuteGraph &execute_graph,
                                             const ChainNormalizer &chain_normalizer, ProgramInfo &program_info) const {
  program_info.custom_const_chains.clear();
  std::unordered_map<uint32_t, CustomConstRestoreDescriptor> custom_descriptors;
  const auto collect_custom_descriptors = [&custom_descriptors](const ProgramBlock &block) -> ge::graphStatus {
    for (const auto &kernel : block.kernels) {
      for (const auto &descriptor : kernel.custom_const_restore_descriptors) {
        const auto iter = custom_descriptors.find(descriptor.chain_index);
        if (iter == custom_descriptors.cend()) {
          custom_descriptors.emplace(descriptor.chain_index, descriptor);
          continue;
        }
        if ((iter->second.producer_name != descriptor.producer_name) ||
            (iter->second.initializer != descriptor.initializer) ||
            (iter->second.payload_is_string != descriptor.payload_is_string) ||
            (iter->second.payload != descriptor.payload)) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 custom Const chain %u has inconsistent restore descriptors",
                 descriptor.chain_index);
          return ge::GRAPH_PARAM_INVALID;
        }
      }
    }
    return ge::GRAPH_SUCCESS;
  };
  GE_ASSERT_GRAPH_SUCCESS(collect_custom_descriptors(program_info.init));
  GE_ASSERT_GRAPH_SUCCESS(collect_custom_descriptors(program_info.main));
  GE_ASSERT_GRAPH_SUCCESS(collect_custom_descriptors(program_info.deinit));
  std::unordered_map<uint32_t, Om2ValueType> expected_types;
  std::unordered_map<uint32_t, Om2ValueType> observed_types;
  std::unordered_set<uint32_t> compile_time_const_chains;
  const auto collect_compile_time_const_chains = [&compile_time_const_chains](const ProgramBlock &block) {
    for (const auto &kernel : block.kernels) {
      compile_time_const_chains.insert(kernel.compile_time_const_chain_indices.cbegin(),
                                       kernel.compile_time_const_chain_indices.cend());
    }
  };
  collect_compile_time_const_chains(program_info.init);
  collect_compile_time_const_chains(program_info.main);
  collect_compile_time_const_chains(program_info.deinit);
  GE_ASSERT_GRAPH_SUCCESS(
      RecordConstantInputTypes(program_info.init, program_info.chains, expected_types, observed_types));
  GE_ASSERT_GRAPH_SUCCESS(
      RecordConstantInputTypes(program_info.main, program_info.chains, expected_types, observed_types));
  GE_ASSERT_GRAPH_SUCCESS(
      RecordConstantInputTypes(program_info.deinit, program_info.chains, expected_types, observed_types));

  program_info.const_chains.clear();
  program_info.const_chains.reserve(expected_types.size());
  std::unordered_set<uint32_t> collected_chains;
  for (const auto *const node : execute_graph.GetAllNodes()) {
    GE_ASSERT_NOTNULL(node);
    if (!IsConstType(node->GetTypePtr())) {
      continue;
    }
    std::vector<uint32_t> output_chain_indices;
    GE_ASSERT_GRAPH_SUCCESS(chain_normalizer.GetOutputChainIndices(*node, output_chain_indices));
    if (output_chain_indices.size() != 1U) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Const node %s has %zu outputs, expected one", node->GetNamePtr(),
             output_chain_indices.size());
      return ge::GRAPH_PARAM_INVALID;
    }
    const auto chain_index = output_chain_indices[0U];
    if (chain_index >= program_info.chains.size()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    // Load-bound constants represent lifecycle objects rather than portable C++ values.
    if (program_info.chains[chain_index].binding_phase == ChainBindingPhase::kLoad) {
      continue;
    }
    const auto expected_iter = expected_types.find(chain_index);
    if (expected_iter == expected_types.cend()) {
      if (compile_time_const_chains.count(chain_index) != 0U) {
        continue;
      }
      if (node->GetAllOutDataEdgesSize() == 0U) {
        // Custom kernels can retain the original Const after replacing its data edges with a decoder.
        // StageSplitter may add a control edge to the stage tail, but that edge does not read
        // the Const payload and therefore needs no serialized value.
        continue;
      }
      // FxRT's shared BuildTensorAttr Const is consumed only by BuildTensor/BuildRefTensor
      // compile-time contracts.  Older installed collector bundles may not expose the
      // restore-index hint, so recognize this fixed skeleton producer explicitly while
      // retaining a hard error for every other untyped Const.
      const std::string producer_name = node->GetNamePtr();
      if ((producer_name == "const_build_tensor_attr") || (producer_name == "const_dtype_float") ||
          (producer_name.rfind("const_", 0U) == 0U) || (producer_name.rfind("Const_", 0U) == 0U)) {
        continue;
      }
      GELOGE(ge::GRAPH_PARAM_INVALID,
             "OM2 Const node %s has no typed consumer contract; the collector must declare its C++ ABI type",
             node->GetNamePtr());
      return ge::GRAPH_PARAM_INVALID;
    }
    if (!collected_chains.insert(chain_index).second) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Const chain %u has multiple producers", chain_index);
      return ge::GRAPH_PARAM_INVALID;
    }
    ConstChainInfo const_info;
    const_info.chain_index = chain_index;
    const_info.producer_name = node->GetNamePtr();
    const std::string producer_name = node->GetNamePtr();
    bool is_string = false;
    (void)ge::AttrUtils::GetBool(node->GetOpDescBarePtr(), "is_string", is_string);
    if (!is_string && HasCustomDataUser(*node)) {
      GE_ASSERT_TRUE(expected_iter->second == Om2ValueType::kBytePtr,
                     "Custom ModelBufferData Const %s has unexpected OM2 type %s", node->GetNamePtr(),
                     GetValueTypeName(expected_iter->second));
      ge::Buffer buffer;
      GE_ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(node->GetOpDescBarePtr(), kConstValue, buffer));
      CustomConstRestoreDescriptor descriptor;
      GE_ASSERT_GRAPH_SUCCESS(ParseModelBufferDataConst(*node, buffer, descriptor, chain_index));
      program_info.custom_const_chains.emplace_back(std::move(descriptor));
      continue;
    }
    // FxRT stores the serialized AiCore OpDef as a byte payload and supplies its size
    // through a separate Const. Some runtime consumers declare the pointer as void *,
    // which otherwise makes the generic process-local pointer path reject the payload.
    const Om2ValueType value_type =
        (expected_iter->second == Om2ValueType::kVoidPtr && producer_name.rfind("const_op_def_", 0U) == 0U)
            ? Om2ValueType::kBytePtr
            : expected_iter->second;
    const auto custom_iter = custom_descriptors.find(chain_index);
    if (custom_iter != custom_descriptors.cend()) {
      GE_ASSERT_TRUE(custom_iter->second.producer_name == node->GetNamePtr(),
                     "OM2 custom Const chain %u producer mismatch", chain_index);
      program_info.custom_const_chains.emplace_back(custom_iter->second);
    } else if (IsProcessLocalPointerType(value_type) && !HasPortableConstValueParse(value_type)) {
      // Gate on the consumer's declared C++ ABI type only. The previous extra
      // "const_" name-prefix test rejected lowering-emitted producers named
      // "Const_<graph>_sub_<n>_..." (e.g. the AssignWeightMemory / GetOrCreateWeightMem
      // / DavinciModelCreate / CalcTensorSizeFromStorage resource handles), which then
      // fell through to ParseConstValue and failed as non-serializable. The type is the
      // authoritative contract; the untyped-Const branch below already accepts both
      // "const_" and "Const_" prefixes. The generic tail of this branch still asserts
      // buffer.size() == sizeof(void *), so a mis-typed Const fails loudly.
      ge::Buffer buffer;
      GE_ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(node->GetOpDescBarePtr(), kConstValue, buffer));
      if ((value_type == Om2ValueType::kGertStorageShapePtr || producer_name.rfind("const_storage_shape_", 0U) == 0U) &&
          buffer.size() == sizeof(gert::StorageShape)) {
        CustomConstRestoreDescriptor descriptor;
        GE_ASSERT_GRAPH_SUCCESS(ParseStorageShapeConst(*node, buffer, descriptor, chain_index));
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      } else if ((value_type == Om2ValueType::kGertTensorDataPtr || (producer_name.rfind("const_gtd_", 0U) == 0U) ||
                  (producer_name.rfind("const_shape_gtd_host_", 0U) == 0U)) &&
                 (buffer.size() == sizeof(gert::GertTensorData))) {
        CustomConstRestoreDescriptor descriptor;
        GE_ASSERT_GRAPH_SUCCESS(ParseGertTensorDataConst(*node, buffer, descriptor, chain_index));
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      } else if (((value_type == Om2ValueType::kGertTensorPtr) ||
                  (producer_name.rfind("const_shape_host_", 0U) == 0U) ||
                  (producer_name.rfind("const_scalar_tensor_", 0U) == 0U)) &&
                 (buffer.size() == sizeof(gert::Tensor))) {
        CustomConstRestoreDescriptor descriptor;
        GE_ASSERT_GRAPH_SUCCESS(ParseTensorConst(*node, buffer, descriptor, chain_index));
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      } else if (buffer.size() == sizeof(gert::Tensor)) {
        // kVoidPtr carries no ABI information: it is what builders declare for genuinely
        // polymorphic ports such as LaunchKernelWithFlag's io_address_or_shape_<n>, which
        // UpdateEachArgsInfo reads as gert::TensorData * for the first io_num slots and as
        // StorageShape * for the rest.  RecordConstantInputTypes reconciles kVoidPtr against
        // kGertTensorPtr when a second consumer pins the chain, but a single-consumer chain
        // has nothing to reconcile with, so the payload size is the only remaining evidence.
        // A buffer of exactly sizeof(gert::Tensor) can only be a by-value gert::Tensor.
        CustomConstRestoreDescriptor descriptor;
        GE_ASSERT_GRAPH_SUCCESS(ParseTensorConst(*node, buffer, descriptor, chain_index));
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      } else if (buffer.size() == sizeof(gert::TensorData)) {
        CustomConstRestoreDescriptor descriptor;
        GE_ASSERT_GRAPH_SUCCESS(ParseTensorDataObjectConst(*node, buffer, descriptor, chain_index));
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      } else {
        GE_ASSERT_TRUE(buffer.size() == sizeof(void *), "OM2 process-local Const %s has invalid pointer size %zu",
                       node->GetNamePtr(), buffer.size());
        uintptr_t address = 0U;
        GE_ASSERT_EOK(memcpy_s(&address, sizeof(address), buffer.data(), sizeof(address)));
        if (address == 0U) {
          CustomConstRestoreDescriptor descriptor;
          descriptor.chain_index = chain_index;
          descriptor.producer_name = node->GetNamePtr();
          descriptor.initializer = "null_pointer";
          descriptor.payload = {0U};
          program_info.custom_const_chains.emplace_back(std::move(descriptor));
          collected_chains.insert(chain_index);
          continue;
        }
        CustomConstRestoreDescriptor descriptor;
        descriptor.chain_index = chain_index;
        descriptor.producer_name = node->GetNamePtr();
        descriptor.initializer = "process_local_pointer";
        const auto payload = std::to_string(static_cast<uint64_t>(address));
        descriptor.payload.assign(payload.cbegin(), payload.cend());
        descriptor.payload.push_back(0U);
        program_info.custom_const_chains.emplace_back(std::move(descriptor));
      }
    } else {
      GE_ASSERT_GRAPH_SUCCESS(ParseConstValue(*node, value_type, const_info.value));
      program_info.const_chains.emplace_back(std::move(const_info));
    }
    collected_chains.insert(chain_index);
  }
  if (collected_chains.size() != expected_types.size()) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 typed Const chain count mismatch: expected %zu, collected %zu",
           expected_types.size(), collected_chains.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace om2
}  // namespace gert
