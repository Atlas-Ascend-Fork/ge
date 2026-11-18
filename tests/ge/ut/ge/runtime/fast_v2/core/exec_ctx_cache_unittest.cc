/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include "core/executor/exec_ctx_cache.h"
#include "stub/gert_runtime_stub.h"

namespace gert {
namespace {
class ExecCtxCacheStub : public AclRuntimeStubImpl {
 public:
  aclError aclrtGetCurrentContext(aclrtContext *context) override {
    query_times_.fetch_add(1U);
    if (fail_) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    *context = ctx_;
    return ACL_SUCCESS;
  }

  void SetCtx(aclrtContext ctx) {
    ctx_ = ctx;
  }
  aclrtContext GetCtx() const {
    return ctx_;
  }
  void SetFail(bool fail) {
    fail_ = fail;
  }
  uint32_t QueryTimes() const {
    return query_times_.load();
  }

 private:
  aclrtContext ctx_{reinterpret_cast<aclrtContext>(0x1000)};
  bool fail_{false};
  std::atomic<uint32_t> query_times_{0U};
};

// 缓存状态为 thread_local，每个用例在独立线程中执行，避免用例之间互相影响。
class ExecCtxCacheUT : public testing::Test {
 protected:
  void SetUp() override {
    auto acl = std::make_unique<ExecCtxCacheStub>();
    acl_ = acl.get();
    runtime_stub_ = std::make_unique<GertRuntimeStub>(std::make_unique<RuntimeStubImpl>(), true, std::move(acl));
  }
  void TearDown() override {
    runtime_stub_.reset();
  }

  template <typename Fn>
  void RunInNewThread(Fn &&fn) {
    // AclRuntimeStub 的 fake_instance_ 是 thread_local，GertRuntimeStub 仅在主线程 Install 了本用例的
    // ExecCtxCacheStub；新线程里 GetExecCtx 会回退到默认 stub。这里在新线程内重新 Install，保证
    // aclrtGetCurrentContext 等调用仍路由到本用例的 stub。
    std::thread worker([this, fn = std::forward<Fn>(fn)]() mutable {
      ge::AclRuntimeStub::Install(acl_);
      fn();
      ge::AclRuntimeStub::UnInstall(acl_);
    });
    worker.join();
  }

  ExecCtxCacheStub *acl_{nullptr};
  std::unique_ptr<GertRuntimeStub> runtime_stub_;
};
}  // namespace

// 绑定空 ctx 被拒绝，GetExecCtx 仍走实时查询。
TEST_F(ExecCtxCacheUT, BindExecCtxRejectsNullCtx) {
  RunInNewThread([this]() {
    BindExecCtx(nullptr);
    EXPECT_EQ(GetExecCtx(), acl_->GetCtx());
    EXPECT_EQ(acl_->QueryTimes(), 1U);
  });
}

// worker 绑定后直接读缓存，即使实时查询失败也不影响。
TEST_F(ExecCtxCacheUT, BindExecCtxUsesCachedCtxWithoutQuery) {
  RunInNewThread([this]() {
    auto ctx = reinterpret_cast<aclrtContext>(0x2000);
    BindExecCtx(ctx);
    EXPECT_EQ(GetExecCtx(), ctx);
    EXPECT_EQ(acl_->QueryTimes(), 0U);
    acl_->SetFail(true);
    EXPECT_EQ(GetExecCtx(), ctx);
    EXPECT_EQ(acl_->QueryTimes(), 0U);
    acl_->SetFail(false);
    BindExecCtx(reinterpret_cast<aclrtContext>(0x2001));
    EXPECT_EQ(GetExecCtx(), reinterpret_cast<aclrtContext>(0x2001));
  });
}

// 作用域内只查询一次并缓存，退出后清除缓存恢复实时查询。
TEST_F(ExecCtxCacheUT, ScopeGuardCachesCtxAndClearsOnExit) {
  RunInNewThread([this]() {
    auto ctx_a = reinterpret_cast<aclrtContext>(0x3000);
    auto ctx_b = reinterpret_cast<aclrtContext>(0x3001);
    acl_->SetCtx(ctx_a);
    {
      ExecCtxScopeGuard guard;
      EXPECT_EQ(acl_->QueryTimes(), 1U);
      acl_->SetCtx(ctx_b);
      EXPECT_EQ(GetExecCtx(), ctx_a);
      EXPECT_EQ(acl_->QueryTimes(), 1U);
    }
    EXPECT_EQ(GetExecCtx(), ctx_b);
    EXPECT_EQ(acl_->QueryTimes(), 2U);
  });
}

// 嵌套作用域只在最外层查询一次，内层直接复用缓存。
TEST_F(ExecCtxCacheUT, NestedScopeGuardQueriesOnlyOnce) {
  RunInNewThread([this]() {
    auto ctx_a = reinterpret_cast<aclrtContext>(0x4000);
    auto ctx_b = reinterpret_cast<aclrtContext>(0x4001);
    acl_->SetCtx(ctx_a);
    {
      ExecCtxScopeGuard outer;
      acl_->SetCtx(ctx_b);
      {
        ExecCtxScopeGuard inner;
        EXPECT_EQ(acl_->QueryTimes(), 1U);
        EXPECT_EQ(GetExecCtx(), ctx_a);
      }
      EXPECT_EQ(GetExecCtx(), ctx_a);
      EXPECT_EQ(acl_->QueryTimes(), 1U);
    }
    EXPECT_EQ(GetExecCtx(), ctx_b);
    EXPECT_EQ(acl_->QueryTimes(), 2U);
  });
}

// 构造作用域时查询失败，缓存标记为无效，执行期间走实时查询并返回空。
TEST_F(ExecCtxCacheUT, ScopeGuardWithQueryFailureFallsBackToLiveQuery) {
  RunInNewThread([this]() {
    acl_->SetFail(true);
    {
      ExecCtxScopeGuard guard;
      EXPECT_EQ(acl_->QueryTimes(), 1U);
      EXPECT_EQ(GetExecCtx(), nullptr);
      EXPECT_EQ(acl_->QueryTimes(), 2U);
    }
    EXPECT_EQ(GetExecCtx(), nullptr);
    EXPECT_EQ(acl_->QueryTimes(), 3U);
    acl_->SetFail(false);
  });
}

// worker 绑定后作用域不再查询，退出作用域也不清除绑定。
TEST_F(ExecCtxCacheUT, ScopeGuardKeepsWorkerBoundCtxAfterExit) {
  RunInNewThread([this]() {
    auto ctx_a = reinterpret_cast<aclrtContext>(0x5000);
    auto ctx_b = reinterpret_cast<aclrtContext>(0x5001);
    BindExecCtx(ctx_a);
    {
      ExecCtxScopeGuard guard;
      EXPECT_EQ(acl_->QueryTimes(), 0U);
      EXPECT_EQ(GetExecCtx(), ctx_a);
    }
    EXPECT_EQ(GetExecCtx(), ctx_a);
    EXPECT_EQ(acl_->QueryTimes(), 0U);
    BindExecCtx(ctx_b);
    EXPECT_EQ(GetExecCtx(), ctx_b);
  });
}
}  // namespace gert
