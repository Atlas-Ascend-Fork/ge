/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_RESTORE_HOOK_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_RESTORE_HOOK_H_

namespace ge {
namespace custom_op {
// 安装 PYTHON_CUSTOM_OPS 分区恢复 hook（进程级单例，幂等）。
// 注册时机随 CustomOpLoader::Load()（即 LoadCustomOps() 入口）：这些入口覆盖所有会
// 加载 Python OM 的进程初始化路径；库未被载入的进程中 GetPythonCustomOpRestoreHook()
// 为 null，base 加载侧点名 Python 显式失败，不静默跳过。
// hook 为进程生命周期单例，注册后不清除（与 SetPythonCustomOpRestoreHook 契约一致）。
void InstallPythonCustomOpRestoreHook();
}  // namespace custom_op
}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_RESTORE_HOOK_H_
