/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_GRAPH_CUSTOM_OP_ARTIFACT_PROVIDER_H_
#define METADEF_CXX_INC_GRAPH_CUSTOM_OP_ARTIFACT_PROVIDER_H_

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "graph/custom_op/capability.h"

namespace ge {
// Python 自定义算子 IR 输入/输出定义：name + kind（与 bridge 侧
// PythonCustomOpProtoInputKind / PythonCustomOpProtoOutputKind 位定义一致）。
struct PythonIrInputSnapshot {
  std::string name;
  uint32_t kind{0U};

  bool operator==(const PythonIrInputSnapshot &other) const {
    return (name == other.name) && (kind == other.kind);
  }
};

struct PythonIrOutputSnapshot {
  std::string name;
  uint32_t kind{0U};

  bool operator==(const PythonIrOutputSnapshot &other) const {
    return (name == other.name) && (kind == other.kind);
  }
};

// IR 属性定义：type 为 GetRegisteredIrDefFromGraph 返回的原样类型字符串，
// 加载侧经 bridge 的 runtime attr 规格解释（PythonCustomOpProtoAttrKind）。
struct PythonIrAttrSnapshot {
  std::string name;
  std::string type;

  bool operator==(const PythonIrAttrSnapshot &other) const {
    return (name == other.name) && (type == other.type);
  }
};

// Python 自定义算子部署 artifact 中单个实现类的描述快照。
// 只保存跨进程稳定的字段：op_type、类限定名、capability mask 与 canonical IR 快照。
// 不得保存 descriptor_key 与 module_name —— 二者由 Python 侧字符串 hash 派生，跨进程不稳定。
// IR 快照由保存侧（编译进程）从 OperatorFactory 冻结：加载态 D4 抑制原型注册，
// schema-bound execute 的参数绑定依赖它而非进程级 OperatorFactory。
struct PythonOpDescriptorSnapshot {
  std::string op_type;
  std::string class_name;
  CustomOpCapabilityMask capabilities{0U};
  std::vector<PythonIrInputSnapshot> ir_inputs;
  std::vector<PythonIrAttrSnapshot> ir_attrs;
  std::vector<PythonIrOutputSnapshot> ir_outputs;
};

// 编译态冻结的 immutable Python 部署 artifact：一个定义文件对应一个 artifact，
// 同一文件内注册的全部实现类共享它。不持有模型、registry 或 holder 引用。
struct PythonCustomOpDeploymentArtifact {
  static constexpr size_t kSha256Len = 32U;
  static constexpr size_t kArtifactIdLen = 64U;

  // 冻结该 artifact 的 bridge/Python 环境。写入 package 头部供加载侧做兼容性校验，
  // 由 runtime 侧填充，使 base 层无需依赖 runtime/custom_op 的 ABI 常量。
  uint32_t bridge_abi_version{0U};
  uint32_t producer_python_major{0U};
  uint32_t producer_python_minor{0U};

  std::string artifact_id;  // source_sha256 的 64 字符小写十六进制形式
  std::string source_name;  // 仅用于诊断的文件名，不作为加载路径使用
  std::array<uint8_t, kSha256Len> source_sha256{};
  std::shared_ptr<const std::vector<uint8_t>> source;
  std::vector<PythonOpDescriptorSnapshot> descriptors;
};

// GE 内部只读接口：查询自定义算子实例关联的 Python 部署 artifact。
//
// 刻意不继承 BaseCustomOp：PortableOp 已虚继承 BaseCustomOp，若本接口也虚继承会形成菱形，
// 使 ModelHelper 侧从 PortableOp* 到本接口的 dynamic_cast 语义复杂化。作为独立兄弟基类时，
// 该转换是同一 most-derived object 内的标准兄弟基类转换。
//
// 本接口不是 public capability：不进入 CustomOpCapability 枚举、不进入用户接口、不写入 OM state。
// 普通 C++ PortableOp 不实现它，查询结果为空，保存行为保持不变。
class CustomOpArtifactProvider {
 public:
  virtual ~CustomOpArtifactProvider() = default;
  // 返回全部与该 op_type 相关的部署 artifact（按 artifact_id 升序，与打包契约一致）：
  //   - 实现 artifact（承载 @register_op_impl 的定义文件）；
  //   - 分文件原型时，另有承载 @register_op 声明的原型 artifact。
  // 后者必须一并交出：原型声明只存在于源码里，不进 OM 则加载后无 infer_func，
  // shape 推导能力静默缺失。同文件原型只有实现 artifact 一项（声明在同一个文件里）。
  virtual std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> GetDeploymentArtifacts() const = 0;
};
}  // namespace ge

#endif  // METADEF_CXX_INC_GRAPH_CUSTOM_OP_ARTIFACT_PROVIDER_H_
