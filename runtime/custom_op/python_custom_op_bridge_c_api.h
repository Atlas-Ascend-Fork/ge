/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_C_API_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_C_API_H_

#include <cstddef>
#include <cstdint>

#include "ge/ge_api_types.h"
#include "runtime/custom_op/python_custom_op_bridge_types.h"

namespace ge {
namespace custom_op {
struct PythonCustomOpProtoDescriptorView;
struct PythonCustomOpAdapterDescriptorView;
struct PythonCustomOpAdapterCallbacks;

struct PythonCustomOpRegistrar {
  bool (*register_op_proto)(const PythonCustomOpProtoDescriptorView *desc);
  bool (*register_op_impl)(const PythonCustomOpAdapterDescriptorView *desc,
                           const PythonCustomOpAdapterCallbacks *callbacks);
};

struct PythonCustomOpBridgeArtifactConfig {
  const char *artifact_root;
  const char *native_module_path;
};

// 单个 Python 实现类的部署描述视图。只承载跨进程稳定的字段，
// 不含 descriptor_key 与 module_name（二者由 Python 字符串 hash 派生，跨进程不稳定）。
// ir_* 为保存侧冻结的 canonical IR（编译进程 OperatorFactory 可查而加载进程不可），
// 在加载态供 schema-bound execute/compile 的参数绑定查询；旧 OM 分区无该数据时指针为空、计数为 0。
struct PythonCustomOpDeploymentDescriptorView {
  PythonCustomOpStringView op_type;
  PythonCustomOpStringView class_name;
  uint32_t capabilities;
  const PythonCustomOpIrInputDefView *ir_inputs;
  size_t ir_input_count;
  const PythonCustomOpIrAttrDefView *ir_attrs;
  size_t ir_attr_count;
  const PythonCustomOpIrOutputDefView *ir_outputs;
  size_t ir_output_count;
};

// 一个已冻结的定义文件及其完整实现类描述集合。
// 所有指针均为同步借用：sink 回调返回前必须完成校验和深拷贝。
// bridge ABI 版本不在此传递，由 loader 侧按 kPythonCustomOpBridgeAbiVersion 权威填充。
struct PythonCustomOpDeploymentArtifactView {
  PythonCustomOpStringView artifact_id;
  PythonCustomOpStringView source_name;
  const uint8_t *source_sha256;
  size_t source_sha256_size;
  const uint8_t *source;
  size_t source_size;
  const PythonCustomOpDeploymentDescriptorView *descriptors;
  size_t descriptor_count;
  uint32_t producer_python_major;
  uint32_t producer_python_minor;
};

struct PythonCustomOpDeploymentArtifactSink {
  bool (*on_artifact)(void *user_data, const PythonCustomOpDeploymentArtifactView *view);
};

// 恢复侧（PYTHON_CUSTOM_OPS 分区）一段 embedded source 的内存视图。
// ir_defs 随该 artifact 的 manifest 一并携带（保存侧冻结的 canonical IR），
// 在执行 embedded source 前传入 Python scope，供加载态 schema-bound 参数绑定查询。
// 指针为同步借用：被调方返回前必须完成消费，不得跨调用保存。
struct PythonCustomOpEmbeddedSourceView {
  PythonCustomOpStringView artifact_id;
  PythonCustomOpStringView source_name;
  const uint8_t *source;
  size_t source_size;
  const PythonCustomOpDeploymentDescriptorView *ir_defs;
  size_t ir_def_count;
};

// 恢复侧 scope 内执行 embedded source 后产出的单个 impl descriptor。
// capabilities 由 bridge 侧 ParseInterfaces 从 interfaces 权威映射；
// class_qualname 用于与 manifest 的 (op_type, class_qualname, capabilities) 分级比对。
struct PythonCustomOpScopedDescriptorView {
  PythonCustomOpStringView op_type;
  PythonCustomOpStringView impl_descriptor_key;
  PythonCustomOpStringView class_name;
  PythonCustomOpStringView class_qualname;
  CustomOpCapabilityMask capabilities;
};

struct PythonCustomOpScopedDescriptorSink {
  // callbacks 与编译期 register_op_impl 对称传递：bridge 侧 GetCallbacks() 产出的同一套
  // 函数指针（按 descriptor_key/instance_id 区分具体 holder），供恢复 hook 注册运行时
  // registry 与创建 adapter。指针为同步借用，回调返回前必须完成深拷贝。
  bool (*on_descriptor)(void *user_data, const PythonCustomOpScopedDescriptorView *view,
                        const PythonCustomOpAdapterCallbacks *callbacks);
};

// 通用字符串 sink：当前仅用于 scope 内被抑制的原型声明 op_type（只诊断）。
struct PythonCustomOpStringSink {
  bool (*on_string)(void *user_data, const PythonCustomOpStringView *value);
};

struct PythonCustomOpBridgeApi {
  uint32_t abi_version;
  Status (*set_artifact_config)(const PythonCustomOpBridgeArtifactConfig *config);
  Status (*register_custom_ops)(const PythonCustomOpRegistrar *registrar);
  void (*reset_bridge_state)();
  void (*shutdown_bridge)();
  // 本特性新增：拉取编译期已冻结的 Python 部署 artifact。
  // 追加在结构体末尾，因此必须先校验 abi_version 再访问。
  Status (*collect_deployment_artifacts)(const PythonCustomOpDeploymentArtifactSink *sink, void *user_data);
  // 模型 scope 恢复能力（设计文档「加载侧分层与 hook 解耦」节）：同样追加在函数表末尾。
  // ABI 版本保持 2——ABI 2 尚未商发（已商发版本为 ABI 1），现存的 ABI 2 bridge SO 仅为
  // 开发/测试环境内与加载侧头文件同批构建的产物，本特性新增的槽位并入 2 而不递增版本号；
  // 加载侧 IsBridgeApiValid 对全部字段做精确校验（abi_version == 2 且指针非空）。
  // restore_model_scope 一次会话完成 open + 逐 embedded source 初始化 + descriptor 收集：
  // 调用方只有恢复 hook，三步原本就是同一锁内顺序执行，合并后「open 成功但 init 失败」的
  // 中间态由 Python 侧一处 try/finally 收敛（失败即 close）。sources 指针为同步借用。
  // collect_model_scope_proto_op_types（被抑制原型诊断）与 close_model_scope
  // （卸载/回滚路径）不参与合并。
  Status (*restore_model_scope)(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources, size_t source_count,
                                const PythonCustomOpScopedDescriptorSink *sink, void *user_data);
  Status (*collect_model_scope_proto_op_types)(uint64_t scope_id, const PythonCustomOpStringSink *sink,
                                               void *user_data);
  Status (*close_model_scope)(uint64_t scope_id);
};

constexpr uint32_t kPythonCustomOpBridgeAbiVersion = 2U;
constexpr const char *kPythonCustomOpBridgeGetApiSymbol = "GeGetPythonCustomOpBridgeApi";
}  // namespace custom_op
}  // namespace ge

extern "C" __attribute__((visibility("default"))) const ge::custom_op::PythonCustomOpBridgeApi *
GeGetPythonCustomOpBridgeApi();

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_C_API_H_
