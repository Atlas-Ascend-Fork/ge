/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_GRAPH_CUSTOM_OP_MODEL_SCOPE_H_
#define METADEF_CXX_INC_GRAPH_CUSTOM_OP_MODEL_SCOPE_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "graph/custom_op_registry.h"
#include "graph/error_codes.h"

namespace ge {
// 模型级自定义算子作用域的不透明句柄。CustomOpModelScopePtr 别名在 custom_op_registry.h 中
// 以前向声明形式提供，使 CustomOpRegistry 可以在不感知 Python 的前提下持有 scope。
//
// 具体实现由上层（runtime/custom_op 的 PythonCustomOpModelScope）提供；graph_metadef 与 base
// 只通过它持有生命周期。Close() 幂等，必须在解释器仍存活时调用，
// 因此 CustomOpRegistry 的析构顺序必须保证 scope 早于进程级 Python shutdown 释放。
class CustomOpModelScope {
 public:
  virtual ~CustomOpModelScope() = default;
  virtual void Close() = 0;
};

// 由上层注册的 PYTHON_CUSTOM_OPS 恢复实现。
//
// 存在的原因：恢复逻辑需要 PythonCustomOpAdapter 与 bridge loader（位于 runtime/custom_op），
// 而加载编排位于 base/common/helper。custom_op_runtime 已链接 ge_common_base，
// 反向直接链接会形成循环依赖，因此通过本 hook 解耦。
class PythonCustomOpRestoreHook {
 public:
  virtual ~PythonCustomOpRestoreHook() = default;

  // 解析并校验 PYTHON_CUSTOM_OPS 分区，准备 Python runtime/bridge，在内存中初始化全部
  // embedded source，校验完整 descriptor 集合，把全部 Python creator 注册进模型级 registry，
  // 并将 scope 交给 registry 持有。任一步失败必须自行完成回滚，不留半初始化状态。
  virtual graphStatus Restore(const uint8_t *data, size_t len, const CustomOpRegistryPtr &registry) = 0;
};

// hook 为进程级单例，由 runtime/custom_op 在库初始化时注册，进程退出前不清除。
// 未注册而 OM 携带 PYTHON_CUSTOM_OPS 时，加载必须显式失败而不是静默跳过。
void SetPythonCustomOpRestoreHook(PythonCustomOpRestoreHook *hook);
PythonCustomOpRestoreHook *GetPythonCustomOpRestoreHook();
}  // namespace ge

#endif  // METADEF_CXX_INC_GRAPH_CUSTOM_OP_MODEL_SCOPE_H_
