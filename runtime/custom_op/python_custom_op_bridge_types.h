/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_TYPES_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "graph/custom_op/capability.h"
#include "graph/error_codes.h"
#include "graph/types.h"

namespace gert {
class AnnotatedArgsContext;
class EagerOpExecutionContext;
class HostCpuOpExecutionContext;
class OpCompileContext;
class InferShapeContext;
class StorageShape;
}  // namespace gert

namespace ge {
namespace custom_op {
struct PythonCustomOpStringView {
  const char *data;
  size_t size;
};

// 跨 SO 边界的字符串视图统一拷贝入口（loader / proto / pybind bridge 共用）：
// size 非零却无 data 视为非法；\0 只可能来自对端写坏的内存，一并拒绝。
// allow_empty 仅放宽空串（如仅诊断用途的 source_name），其余语义（非空 + 无 \0）不变。
inline bool CopyPythonCustomOpStringView(const PythonCustomOpStringView &view, const bool allow_empty,
                                         std::string &value) {
  if ((view.size != 0U) && (view.data == nullptr)) {
    return false;
  }
  value.assign(view.data == nullptr ? "" : view.data, view.size);
  return (allow_empty || (!value.empty())) && (value.find('\0') == std::string::npos);
}

enum PythonCustomOpProtoInputKind : uint32_t {
  kPythonInputRequired = 0U,
  kPythonInputOptional = 1U,
  kPythonInputDynamic = 2U,
};

enum PythonCustomOpProtoOutputKind : uint32_t {
  kPythonOutputRequired = 0U,
  kPythonOutputDynamic = 1U,
};

enum PythonCustomOpProtoAttrKind : uint32_t {
  kPythonAttrInt = 0U,
  kPythonAttrFloat = 1U,
  kPythonAttrBool = 2U,
  kPythonAttrString = 3U,
  kPythonAttrDataType = 4U,
  kPythonAttrTensor = 5U,
  kPythonAttrListInt = 6U,
  kPythonAttrListFloat = 7U,
  kPythonAttrListBool = 8U,
  kPythonAttrListString = 9U,
  kPythonAttrListDataType = 10U,
  kPythonAttrListListInt = 11U,
};

struct PythonCustomOpInt64ArrayView {
  const int64_t *data;
  size_t count;
};

struct PythonCustomOpAttrDefaultView {
  uint8_t has_value;
  int64_t int_value;
  double float_value;
  uint8_t bool_value;
  PythonCustomOpStringView string_value;
  int32_t data_type_value;
  const int64_t *list_int_values;
  const double *list_float_values;
  const uint8_t *list_bool_values;
  const PythonCustomOpStringView *list_string_values;
  const int32_t *list_data_type_values;
  const PythonCustomOpInt64ArrayView *list_list_int_values;
  size_t count;
};

struct PythonCustomOpProtoInputView {
  PythonCustomOpStringView name;
  uint32_t kind;
};

struct PythonCustomOpProtoAttrView {
  PythonCustomOpStringView name;
  uint32_t kind;
  uint8_t is_required;
  PythonCustomOpAttrDefaultView default_value;
};

struct PythonCustomOpProtoOutputView {
  PythonCustomOpStringView name;
  uint32_t kind;
};

struct PythonCustomOpInferMetaOutputView {
  gert::StorageShape *shape;
  ge::DataType data_type;
};

struct PythonCustomOpInferMetaResultView {
  PythonCustomOpInferMetaOutputView *outputs;
  size_t output_count;
};

using PythonCustomOpInferMetaFn = graphStatus (*)(const PythonCustomOpStringView *op_type, gert::InferShapeContext *ctx,
                                                  PythonCustomOpInferMetaResultView *result);

struct PythonCustomOpProtoDescriptorView {
  PythonCustomOpStringView descriptor_key;
  PythonCustomOpStringView op_type;
  const PythonCustomOpProtoInputView *inputs;
  size_t input_count;
  const PythonCustomOpProtoAttrView *attrs;
  size_t attr_count;
  const PythonCustomOpProtoOutputView *outputs;
  size_t output_count;
  PythonCustomOpInferMetaFn infer_meta;
};

struct PythonCustomOpAdapterDescriptorView {
  PythonCustomOpStringView op_type;
  PythonCustomOpStringView impl_descriptor_key;
  CustomOpCapabilityMask capabilities;
};

// 恢复侧携带的 canonical IR 定义（保存侧从编译进程 OperatorFactory 冻结）。
// kind 与 PythonCustomOpProtoInputKind / PythonCustomOpProtoOutputKind 位定义一致；
// attr 的 type 为 IR 类型名字符串（int/float/bool/string/dataType/listInt/...）。
// 仅用于 D4 抑制原型注册后 schema-bound 接口的参数绑定，不参与注册与 shape 推导。
struct PythonCustomOpIrInputDefView {
  PythonCustomOpStringView name;
  uint32_t kind;
};

struct PythonCustomOpIrAttrDefView {
  PythonCustomOpStringView name;
  PythonCustomOpStringView type;
};

struct PythonCustomOpIrOutputDefView {
  PythonCustomOpStringView name;
  uint32_t kind;
};

using PythonCustomOpImplHolderCreateFn = void *(*)(const PythonCustomOpAdapterDescriptorView *desc);
using PythonCustomOpImplHolderDestroyFn = void (*)(void *holder);
using PythonCustomOpImplExecuteFn = graphStatus (*)(const void *holder, gert::EagerOpExecutionContext *ctx);
using PythonCustomOpImplHostExecuteFn = graphStatus (*)(const void *holder, gert::HostCpuOpExecutionContext *ctx);
using PythonCustomOpImplDeclareLaunchArgsFn = graphStatus (*)(const void *holder, gert::AnnotatedArgsContext *ctx);
using PythonCustomOpImplCompileFn = graphStatus (*)(const void *holder, gert::OpCompileContext *ctx);
using PythonCustomOpImplSerializeFn = graphStatus (*)(const void *holder, std::vector<uint8_t> *output);
using PythonCustomOpImplDeserializeFn = graphStatus (*)(const void *holder, const uint8_t *data, size_t size);
// 接口扩展约定：新增字段一律尾部追加，不得插入既有字段之间（参考 graph_metadef.md §3）。
struct PythonCustomOpAdapterCallbacks {
  PythonCustomOpImplHolderCreateFn create_impl_holder{nullptr};
  PythonCustomOpImplHolderDestroyFn destroy_impl_holder{nullptr};
  PythonCustomOpImplExecuteFn execute{nullptr};
  PythonCustomOpImplHostExecuteFn host_cpu_execute{nullptr};
  PythonCustomOpImplDeclareLaunchArgsFn declare_launch_args{nullptr};
  PythonCustomOpImplCompileFn compile_impl{nullptr};
  PythonCustomOpInferMetaFn infer_meta{nullptr};
  PythonCustomOpImplSerializeFn serialize{nullptr};
  PythonCustomOpImplDeserializeFn deserialize{nullptr};

  bool IsValid(CustomOpCapabilityMask capabilities) const {
    const auto supported_capabilities = static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute) |
                                        static_cast<CustomOpCapabilityMask>(CustomOpCapability::kHostCpuExecute) |
                                        static_cast<CustomOpCapabilityMask>(CustomOpCapability::kCompilable) |
                                        static_cast<CustomOpCapabilityMask>(CustomOpCapability::kAnnotatedArgs) |
                                        static_cast<CustomOpCapabilityMask>(CustomOpCapability::kPortable);
    if ((capabilities == 0U) || ((capabilities & (~supported_capabilities)) != 0U)) {
      return false;
    }
    if ((create_impl_holder == nullptr) || (destroy_impl_holder == nullptr)) {
      return false;
    }
    if (HasCustomOpCapability(capabilities, CustomOpCapability::kEagerExecute) && (execute == nullptr)) {
      return false;
    }
    if (HasCustomOpCapability(capabilities, CustomOpCapability::kHostCpuExecute) && (host_cpu_execute == nullptr)) {
      return false;
    }
    if (HasCustomOpCapability(capabilities, CustomOpCapability::kAnnotatedArgs) && (declare_launch_args == nullptr)) {
      return false;
    }
    if (HasCustomOpCapability(capabilities, CustomOpCapability::kCompilable) && (compile_impl == nullptr)) {
      return false;
    }
    if (HasCustomOpCapability(capabilities, CustomOpCapability::kPortable) &&
        ((serialize == nullptr) || (deserialize == nullptr))) {
      return false;
    }
    return true;
  }
};
}  // namespace custom_op
}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_TYPES_H_
