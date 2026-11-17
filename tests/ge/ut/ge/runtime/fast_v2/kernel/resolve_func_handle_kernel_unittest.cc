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
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include "engine/aicore/kernel/resolve_func_handle_kernel.h"
#include "faker/kernel_run_context_facker.h"
#include "register/kernel_registry.h"
#include "stub/gert_runtime_stub.h"

namespace gert {
namespace {
class ResolveAclStub : public AclRuntimeStubImpl {
 public:
  aclError aclrtGetCurrentContext(aclrtContext *context) override {
    if (fail_ctx_) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    *context = ctx_;
    return ACL_SUCCESS;
  }

  aclError aclrtBinaryGetFunction(const aclrtBinHandle, const char *kernel_name,
                                  aclrtFuncHandle *func_handle) override {
    if (fail_by_name_) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    *func_handle = &by_name_handles_[std::string(kernel_name)];
    return ACL_SUCCESS;
  }

  aclError aclrtBinaryGetFunctionByEntry(const aclrtBinHandle, uint64_t func_entry,
                                         aclrtFuncHandle *func_handle) override {
    if (fail_by_entry_) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    *func_handle = &by_entry_handles_[func_entry];
    return ACL_SUCCESS;
  }

  void SetFailCtx(bool fail) {
    fail_ctx_ = fail;
  }
  void SetFailByName(bool fail) {
    fail_by_name_ = fail;
  }
  void SetFailByEntry(bool fail) {
    fail_by_entry_ = fail;
  }

 private:
  aclrtContext ctx_{reinterpret_cast<aclrtContext>(0x1000)};
  bool fail_ctx_{false};
  bool fail_by_name_{false};
  bool fail_by_entry_{false};
  std::map<std::string, uint8_t> by_name_handles_;
  std::map<uint64_t, uint8_t> by_entry_handles_;
};

// bin handle 只作为缓存键使用，不会被解引用；用递增地址保证各用例（及 TLS 缓存）互不干扰。
void *UniqueBinHandle() {
  static std::atomic<uint64_t> counter{0x10000U};
  return reinterpret_cast<void *>(counter.fetch_add(0x10U));
}

ge::graphStatus RunResolve(void *bin_handle, const char *kernel_name, uint64_t tiling_key, uint32_t with_handle_flag,
                           aclrtFuncHandle *func_handle) {
  auto holder = KernelRunContextFaker()
                    .KernelIONum(4U, 1U)
                    .Inputs({bin_handle, reinterpret_cast<void *>(tiling_key), const_cast<char *>(kernel_name),
                             reinterpret_cast<void *>(with_handle_flag)})
                    .Outputs({nullptr})
                    .Build();
  auto funcs = KernelRegistry::GetInstance().FindKernelFuncs("ResolveKernelFuncHandle");
  if ((funcs == nullptr) || (funcs->run_func == nullptr)) {
    return ge::GRAPH_FAILED;
  }
  const auto ret = funcs->run_func(holder.GetContext<KernelContext>());
  // func_handle 为 void* 输出（8 字节，走 faker 的内联存储），kernel 会把结果写到输出 chain 的
  // inline 缓冲而非 Outputs 传入的地址，故需从输出 chain 读回后再写回调用方变量。
  if (func_handle != nullptr) {
    *func_handle = holder.GetContext<KernelContext>()->GetOutput(0)->GetValue<void *>();
  }
  return ret;
}

class ResolveFuncHandleKernelUT : public testing::Test {
 protected:
  void SetUp() override {
    auto acl = std::make_unique<ResolveAclStub>();
    acl_ = acl.get();
    runtime_stub_ = std::make_unique<GertRuntimeStub>(std::make_unique<RuntimeStubImpl>(), true, std::move(acl));
  }
  void TearDown() override {
    runtime_stub_.reset();
  }

  ResolveAclStub *acl_{nullptr};
  std::unique_ptr<GertRuntimeStub> runtime_stub_;
};
}  // namespace

// 首次按内核名解析走 aclrt，第二次命中线程内 L1 缓存。
TEST_F(ResolveFuncHandleKernelUT, ResolveByNameMissThenTlsHit) {
  void *bin = UniqueBinHandle();
  aclrtFuncHandle first = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_kernel_a", 11U, 0U, &first), ge::GRAPH_SUCCESS);
  ASSERT_NE(first, nullptr);

  aclrtFuncHandle second = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_kernel_a", 11U, 0U, &second), ge::GRAPH_SUCCESS);
  EXPECT_EQ(second, first);
}

// 同一 bin 下不同内核名分别解析，返回不同的函数句柄。
TEST_F(ResolveFuncHandleKernelUT, ResolveByNameDifferentNamesSameBin) {
  void *bin = UniqueBinHandle();
  aclrtFuncHandle handle_b = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_kernel_b", 12U, 0U, &handle_b), ge::GRAPH_SUCCESS);
  aclrtFuncHandle handle_c = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_kernel_c", 13U, 0U, &handle_c), ge::GRAPH_SUCCESS);
  ASSERT_NE(handle_b, nullptr);
  ASSERT_NE(handle_c, nullptr);
  EXPECT_NE(handle_b, handle_c);
}

// 超长内核名不进 L1 缓存，第二次通过 L2 全局缓存命中。
TEST_F(ResolveFuncHandleKernelUT, ResolveByNameLongKernelNameSkipsTls) {
  void *bin = UniqueBinHandle();
  const std::string long_name(200U, 'x');
  aclrtFuncHandle first = nullptr;
  ASSERT_EQ(RunResolve(bin, long_name.c_str(), 14U, 0U, &first), ge::GRAPH_SUCCESS);
  ASSERT_NE(first, nullptr);
  aclrtFuncHandle second = nullptr;
  ASSERT_EQ(RunResolve(bin, long_name.c_str(), 14U, 0U, &second), ge::GRAPH_SUCCESS);
  EXPECT_EQ(second, first);
}

// 另一个线程写入 L2 后，本线程 L1 未命中，回退 L2 读锁命中。
TEST_F(ResolveFuncHandleKernelUT, ResolveByNameL2HitInAnotherThread) {
  void *bin = UniqueBinHandle();
  aclrtFuncHandle first = nullptr;
  std::thread worker([&]() {
    ge::AclRuntimeStub::Install(acl_);
    EXPECT_EQ(RunResolve(bin, "ut_resolve_kernel_l2_name", 15U, 0U, &first), ge::GRAPH_SUCCESS);
    ge::AclRuntimeStub::UnInstall(acl_);
  });
  worker.join();
  ASSERT_NE(first, nullptr);

  aclrtFuncHandle second = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_kernel_l2_name", 15U, 0U, &second), ge::GRAPH_SUCCESS);
  EXPECT_EQ(second, first);
}

// with_handle 场景按 tiling key 解析：首次走 aclrt，第二次命中 L1，改 key 后重新解析。
TEST_F(ResolveFuncHandleKernelUT, ResolveByEntryMissThenTlsHit) {
  void *bin = UniqueBinHandle();
  aclrtFuncHandle first = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_entry", 0x1234U, 1U, &first), ge::GRAPH_SUCCESS);
  ASSERT_NE(first, nullptr);

  aclrtFuncHandle second = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_entry", 0x1234U, 1U, &second), ge::GRAPH_SUCCESS);
  EXPECT_EQ(second, first);

  aclrtFuncHandle other_key = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_entry", 0x5678U, 1U, &other_key), ge::GRAPH_SUCCESS);
  ASSERT_NE(other_key, nullptr);
  EXPECT_NE(other_key, first);
}

// 另一个线程写入 L2 后，本线程 L1 未命中，回退 L2 读锁命中（entry 路线）。
TEST_F(ResolveFuncHandleKernelUT, ResolveByEntryL2HitInAnotherThread) {
  void *bin = UniqueBinHandle();
  aclrtFuncHandle first = nullptr;
  std::thread worker([&]() {
    ge::AclRuntimeStub::Install(acl_);
    EXPECT_EQ(RunResolve(bin, "ut_resolve_entry_l2", 0x2233U, 1U, &first), ge::GRAPH_SUCCESS);
    ge::AclRuntimeStub::UnInstall(acl_);
  });
  worker.join();
  ASSERT_NE(first, nullptr);

  aclrtFuncHandle second = nullptr;
  ASSERT_EQ(RunResolve(bin, "ut_resolve_entry_l2", 0x2233U, 1U, &second), ge::GRAPH_SUCCESS);
  EXPECT_EQ(second, first);
}

// 按名解析失败时返回失败。
TEST_F(ResolveFuncHandleKernelUT, ResolveByNameAclFailure) {
  void *bin = UniqueBinHandle();
  acl_->SetFailByName(true);
  aclrtFuncHandle func_handle = nullptr;
  EXPECT_EQ(RunResolve(bin, "ut_resolve_kernel_fail_name", 16U, 0U, &func_handle), ge::GRAPH_FAILED);
  EXPECT_EQ(func_handle, nullptr);
}

// 按 tiling key 解析失败时返回失败。
TEST_F(ResolveFuncHandleKernelUT, ResolveByEntryAclFailure) {
  void *bin = UniqueBinHandle();
  acl_->SetFailByEntry(true);
  aclrtFuncHandle func_handle = nullptr;
  EXPECT_EQ(RunResolve(bin, "ut_resolve_entry_fail", 0x3344U, 1U, &func_handle), ge::GRAPH_FAILED);
  EXPECT_EQ(func_handle, nullptr);
}

// 拿不到当前 ctx 时跳过 L1/L2 缓存，直接解析句柄，不因缺 ctx 而失败。在独立线程中执行，确保无历史缓存。
TEST_F(ResolveFuncHandleKernelUT, ResolveWithoutCtxSkipCache) {
  acl_->SetFailCtx(true);
  std::thread worker([&]() {
    ge::AclRuntimeStub::Install(acl_);
    void *bin = UniqueBinHandle();
    aclrtFuncHandle func_handle = nullptr;
    EXPECT_EQ(RunResolve(bin, "ut_resolve_kernel_no_ctx", 17U, 0U, &func_handle), ge::GRAPH_SUCCESS);
    EXPECT_NE(func_handle, nullptr);
    ge::AclRuntimeStub::UnInstall(acl_);
  });
  worker.join();
  acl_->SetFailCtx(false);
}
}  // namespace gert
