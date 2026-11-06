/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_DESCRIPTORS_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_DESCRIPTORS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "graph/error_codes.h"
#include "pybind11/pytypes.h"
#include "runtime/custom_op/python_custom_op_bridge_c_api.h"
#include "runtime/custom_op/python_custom_op_bridge_types.h"

namespace ge {
namespace custom_op {
struct ProtoInputStorage {
  std::string name;
  uint32_t kind{0U};
};

struct ProtoOutputStorage {
  std::string name;
  uint32_t kind{0U};
};

struct ProtoAttrStorage {
  std::string name;
  std::string type;
  uint32_t kind{0U};
  bool is_required{true};
  int64_t int_value{0};
  double float_value{0.0};
  uint8_t bool_value{0U};
  std::string string_value;
  int32_t data_type_value{0};
  std::vector<int64_t> list_int_values;
  std::vector<double> list_float_values;
  std::vector<uint8_t> list_bool_values;
  std::vector<std::string> list_string_values;
  std::vector<PythonCustomOpStringView> list_string_views;
  std::vector<int32_t> list_data_type_values;
  std::vector<std::vector<int64_t>> list_list_int_values;
  std::vector<PythonCustomOpInt64ArrayView> list_list_int_views;

  Status Parse(const pybind11::dict &item);
  Status ParseList(const pybind11::handle &value);
  PythonCustomOpProtoAttrView BuildView();
};

struct ProtoDescriptorStorage {
  std::string descriptor_key;
  std::string op_type;
  std::vector<ProtoInputStorage> inputs;
  std::vector<ProtoAttrStorage> attrs;
  std::vector<ProtoOutputStorage> outputs;
  std::vector<PythonCustomOpProtoInputView> input_views;
  std::vector<PythonCustomOpProtoAttrView> attr_views;
  std::vector<PythonCustomOpProtoOutputView> output_views;

  Status Parse(const pybind11::dict &dict);
  PythonCustomOpProtoDescriptorView BuildView();
};

struct AdapterDescriptorStorage {
  std::string op_type;
  std::string impl_descriptor_key;
  CustomOpCapabilityMask capabilities{0U};

  Status Parse(const pybind11::dict &dict);
  PythonCustomOpAdapterDescriptorView BuildView() const;
};

// 恢复侧模型 scope 内执行 embedded source 后捕获的单个 impl descriptor 的深拷贝载体。
// capabilities 由 interfaces 经 ParseInterfaces 权威映射；class_qualname 供 manifest 分级比对。
// view 中的指针指向本结构持有的存储，sink 回调必须在本结构析构前完成消费。
struct ScopedDescriptorStorage {
  std::string op_type;
  std::string impl_descriptor_key;
  std::string class_name;
  std::string class_qualname;
  CustomOpCapabilityMask capabilities{0U};

  Status Parse(const pybind11::dict &dict);
  PythonCustomOpScopedDescriptorView BuildView() const;
};

// 编译期冻结的 Python 部署 artifact 在 bridge 侧的深拷贝载体。
// view 中的指针指向本结构持有的存储，因此 sink 回调必须在本结构析构前完成消费。
// ir_* 为保存侧（编译进程）经 OperatorFactory 冻结的 canonical IR；Python 侧不产出该数据，
// 因此 Parse 不读它——loader 调 FreezeArtifactIrSnapshots 注入后由 loader 侧直接持有。
struct DeploymentDescriptorStorage {
  std::string op_type;
  std::string class_name;
  uint32_t capabilities{0U};
  std::vector<PythonCustomOpIrInputDefView> ir_input_views;
  std::vector<PythonCustomOpIrAttrDefView> ir_attr_views;
  std::vector<PythonCustomOpIrOutputDefView> ir_output_views;

  Status Parse(const pybind11::dict &dict);
  PythonCustomOpDeploymentDescriptorView BuildView() const;
};

struct DeploymentArtifactStorage {
  std::string artifact_id;
  std::string source_name;
  std::vector<uint8_t> source_sha256;
  std::vector<uint8_t> source;
  std::vector<DeploymentDescriptorStorage> descriptors;
  std::vector<PythonCustomOpDeploymentDescriptorView> descriptor_views;
  uint32_t producer_python_major{0U};
  uint32_t producer_python_minor{0U};

  Status Parse(const pybind11::dict &dict);
  PythonCustomOpDeploymentArtifactView BuildView();
};
}  // namespace custom_op
}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_DESCRIPTORS_H_
