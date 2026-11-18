/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_CORE_EXECUTOR_EXEC_CTX_CACHE_H_
#define AIR_CXX_RUNTIME_V2_CORE_EXECUTOR_EXEC_CTX_CACHE_H_
#include "acl/acl_rt.h"

namespace gert {
// worker 线程入口在 aclrtSetCurrentContext 成功后调用一次，绑定后该线程的 ctx 视为不变。
// ctx 为空时拒绝绑定并告警；传 nullptr 不会清除既有绑定。
void BindExecCtx(const aclrtContext ctx);

// 图执行作用域：在图执行入口构造时绑定当前 ctx，退出时解绑。禁止拷贝/移动（防双重析构）。
class ExecCtxScopeGuard {
 public:
  ExecCtxScopeGuard();
  ~ExecCtxScopeGuard();
  ExecCtxScopeGuard(const ExecCtxScopeGuard &) = delete;
  ExecCtxScopeGuard &operator=(const ExecCtxScopeGuard &) = delete;
  ExecCtxScopeGuard(ExecCtxScopeGuard &&) = delete;
  ExecCtxScopeGuard &operator=(ExecCtxScopeGuard &&) = delete;
};

// 返回当前线程的 ctx：已绑定时读缓存，否则实时查询。查询失败返回 nullptr，调用方必须判空。
aclrtContext GetExecCtx();
}  // namespace gert
#endif  // AIR_CXX_RUNTIME_V2_CORE_EXECUTOR_EXEC_CTX_CACHE_H_
