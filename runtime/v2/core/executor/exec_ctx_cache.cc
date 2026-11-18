/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "core/executor/exec_ctx_cache.h"
#include "acl/acl_rt.h"
#include "framework/common/debug/log.h"

namespace gert {
namespace detail {
// 缓存槽定义为本文件内的单一非 inline 符号（libgert 内唯一定义），避免 inline 函数内的
// static thread_local 被多个动态库分别包含时发生弱符号分裂。
struct ExecCtxCache {
  aclrtContext ctx{nullptr};
  bool worker_bound{false};
  bool valid{false};
  uint32_t scope_depth{0U};
};

ExecCtxCache &ExecCtxCacheSlot() {
  static thread_local ExecCtxCache cache;
  return cache;
}
}  // namespace detail

// 同一 OS 线程先后服务不同 ctx 时（如引入线程池复用），必须重新绑定，否则缓存会返回旧 ctx。
void BindExecCtx(const aclrtContext ctx) {
  if (ctx == nullptr) {
    GELOGW("BindExecCtx rejected: ctx is nullptr.");
    return;
  }
  auto &cache = detail::ExecCtxCacheSlot();
  cache.ctx = ctx;
  cache.valid = true;
  cache.worker_bound = true;
}

ExecCtxScopeGuard::ExecCtxScopeGuard() {
  auto &cache = detail::ExecCtxCacheSlot();
  if ((cache.scope_depth == 0U) && !cache.worker_bound) {
    aclrtContext ctx = nullptr;
    if (aclrtGetCurrentContext(&ctx) == ACL_SUCCESS) {
      cache.ctx = ctx;
      cache.valid = true;
    } else {
      // 查询失败时不得沿用旧缓存值：标记无效，执行期间 GetExecCtx 走实时查询（与不缓存语义一致）。
      cache.valid = false;
    }
  }
  ++cache.scope_depth;
}

ExecCtxScopeGuard::~ExecCtxScopeGuard() {
  auto &cache = detail::ExecCtxCacheSlot();
  if (cache.scope_depth > 0U) {
    --cache.scope_depth;
    if ((cache.scope_depth == 0U) && !cache.worker_bound) {
      // 作用域完全退出后清除非 worker 绑定的缓存状态，避免残留旧值被绕过 GetExecCtx 的内部代码误读。
      cache.valid = false;
      cache.ctx = nullptr;
    }
  }
}

aclrtContext GetExecCtx() {
  auto &cache = detail::ExecCtxCacheSlot();
  if (cache.valid && (cache.worker_bound || (cache.scope_depth > 0U))) {
    return cache.ctx;
  }
  aclrtContext ctx = nullptr;
  if (aclrtGetCurrentContext(&ctx) != ACL_SUCCESS) {
    return nullptr;
  }
  return ctx;
}
}  // namespace gert
