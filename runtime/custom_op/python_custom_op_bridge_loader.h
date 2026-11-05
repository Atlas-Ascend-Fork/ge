/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_LOADER_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_LOADER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "ge/ge_api_types.h"
#include "graph/custom_op/capability.h"
#include "runtime/custom_op/python_custom_op_bridge_c_api.h"
#include "runtime/custom_op/python_custom_op_bridge_types.h"

namespace ge {
namespace custom_op {
// 恢复侧模型 scope 内执行 embedded source 后捕获的单个 impl descriptor 的深拷贝。
// capabilities 由 bridge 侧从 interfaces 权威映射；class_qualname 用于与 manifest 的
// (op_type, class_qualname, capabilities) 分级比对。callbacks 与编译期
// register_op_impl 对称携带，供 hook 注册运行时 registry 并创建 adapter；函数指针稳定
// 指向已加载的 bridge SO，hook 可直接持有到模型卸载。
struct PythonCustomOpScopedDescriptor {
  std::string op_type;
  std::string impl_descriptor_key;
  std::string class_name;
  std::string class_qualname;
  CustomOpCapabilityMask capabilities{0U};
  PythonCustomOpAdapterCallbacks callbacks;
};

Status CheckNeedLoadPythonCustomOps(bool &need_load);
Status LoadPythonCustomOps();
void UnloadPythonCustomOps();
void ShutdownPythonCustomOpsForProcess();

// PYTHON_CUSTOM_OPS 分区恢复用的模型 scope 会话。与 LoadPythonCustomOps 的差异：
// 只按需拉起 Python runtime 与 bridge SO，不收集编译期 artifact、不把文件系统插件
// 注册进进程级 CustomOpFactory（加载态进程级全局注册表零写入）。
// scope id 由调用方（恢复 hook）权威分配：64-bit 单调递增、永不回收。
// 一次会话完成 open + 逐 embedded source 初始化 + scoped descriptor 收集；
// sources 及其内部指针为同步借用，仅在本调用栈内有效。sink 回调经 user_data 直达
// descriptors，不经单例暂存。
Status RestorePythonCustomOpModelScope(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources,
                                       size_t source_count, std::vector<PythonCustomOpScopedDescriptor> &descriptors);
Status CollectPythonCustomOpModelScopeProtoOpTypes(uint64_t scope_id, std::vector<std::string> &op_types);
// best-effort：bridge 未加载或关闭失败只告警，用于失败回滚与模型卸载兜底（幂等）。
void ClosePythonCustomOpModelScope(uint64_t scope_id);
}  // namespace custom_op
}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_BRIDGE_LOADER_H_
