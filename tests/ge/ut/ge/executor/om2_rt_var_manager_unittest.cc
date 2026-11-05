/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "runtime/om2/om2_rt_var_manager.h"
#include "framework/common/gert_model_data_utils.h"
#include "rt_external_mem.h"
#include "common/ge_common/error_codes_define.h"
#include "framework/common/framework_types_internal.h"
#include "depends/ascendcl/src/ascendcl_stub.h"

namespace gert {
namespace {

class FreeRecordingAclRuntimeStub : public ge::AclRuntimeStub {
 public:
  aclError aclrtFree(void *devPtr) override {
    freed_ptrs.push_back(devPtr);
    return ge::AclRuntimeStub::aclrtFree(devPtr);
  }

  std::vector<void *> freed_ptrs;
};

class AclRuntimeStubGuard {
 public:
  explicit AclRuntimeStubGuard(ge::AclRuntimeStub *stub) : stub_(stub) {
    ge::AclRuntimeStub::Install(stub_);
  }
  ~AclRuntimeStubGuard() {
    ge::AclRuntimeStub::UnInstall(stub_);
  }

 private:
  ge::AclRuntimeStub *stub_;
};

// 记录 H2D 写入，用于断言"每变量每设备的设备内存恰好被写一次"。
// 计数器自身加锁，因为并发用例会从多个线程进入本桩。
class H2DRecordingAclRuntimeStub : public ge::AclRuntimeStub {
 public:
  aclError aclrtMemcpy(void *dst, size_t dest_max, const void *src, size_t count, aclrtMemcpyKind kind) override {
    if (kind == ACL_MEMCPY_HOST_TO_DEVICE) {
      const std::lock_guard<std::mutex> lock(mutex_);
      ++h2d_count_;
      h2d_bytes_per_dst_[dst] += count;
      if ((dst != nullptr) && (src != nullptr) && (count <= dest_max)) {
        const auto *begin = static_cast<const uint8_t *>(src);
        last_written_.assign(begin, begin + count);
      }
    }
    return ge::AclRuntimeStub::aclrtMemcpy(dst, dest_max, src, count, kind);
  }

  uint32_t H2DCount() {
    const std::lock_guard<std::mutex> lock(mutex_);
    return h2d_count_;
  }
  uint64_t BytesWrittenTo(void *dst) {
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto it = h2d_bytes_per_dst_.find(dst);
    return (it == h2d_bytes_per_dst_.end()) ? 0U : it->second;
  }
  std::vector<uint8_t> LastWritten() {
    const std::lock_guard<std::mutex> lock(mutex_);
    return last_written_;
  }

 private:
  mutable std::mutex mutex_;
  uint32_t h2d_count_ = 0U;
  std::map<void *, uint64_t> h2d_bytes_per_dst_;
  std::vector<uint8_t> last_written_;
};

constexpr uint32_t kVarDeviceId = 0U;
constexpr uint64_t kVarSize = 16U;
const char *const kVarKey = "v0_0";

static GertVarDesc MakeVarDesc(const char *var_key, const uint64_t size, const uint32_t mode, const void *host_data,
                               const uint64_t host_data_size, const uint64_t logic_addr = 0U) {
  GertVarDesc desc;
  desc.var_key = var_key;
  desc.size = size;
  desc.memory_type = RT_MEMORY_HBM;
  desc.logic_addr = logic_addr;
  desc.host_data = host_data;
  desc.host_data_size = host_data_size;
  desc.device_id = kVarDeviceId;
  desc.mode = mode;
  return desc;
}

static GertVarCommit MakeVarCommit(const char *var_key, const void *host_data, const uint64_t host_data_size) {
  GertVarCommit commit;
  commit.var_key = var_key;
  commit.host_data = host_data;
  commit.host_data_size = host_data_size;
  commit.device_id = kVarDeviceId;
  return commit;
}

static std::vector<uint8_t> MakePattern(const uint8_t seed, const size_t len) {
  std::vector<uint8_t> data(len);
  for (size_t i = 0U; i < len; ++i) {
    data[i] = static_cast<uint8_t>(seed + i);
  }
  return data;
}

class Om2RTVarManagerTest : public testing::Test {
 protected:
};

TEST_F(Om2RTVarManagerTest, LegacyGetOrCreateVarAddr) {
  Om2RTVarManager mgr;
  void *addr = nullptr;
  ASSERT_EQ(mgr.GetOrCreateVarAddr("legacy_key", 0, 256, addr), ge::SUCCESS);
  EXPECT_NE(addr, nullptr);

  void *addr2 = nullptr;
  ASSERT_EQ(mgr.GetOrCreateVarAddr("legacy_key", 0, 256, addr2), ge::SUCCESS);
  EXPECT_EQ(addr, addr2);
}

TEST_F(Om2RTVarManagerTest, LegacyTryGetVarAddr) {
  Om2RTVarManager mgr;
  void *addr = nullptr;
  EXPECT_FALSE(mgr.TryGetVarAddr("missing", 0, addr));

  void *created = nullptr;
  ASSERT_EQ(mgr.GetOrCreateVarAddr("key1", 0, 128, created), ge::SUCCESS);
  EXPECT_TRUE(mgr.TryGetVarAddr("key1", 0, addr));
  EXPECT_EQ(addr, created);
}

TEST_F(Om2RTVarManagerTest, FinalizeFreesMemory) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  auto mgr = std::make_unique<Om2RTVarManager>();

  // 经存活的 SO 侧回调解析地址：无外置内存区时容器自行分配设备内存
  auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr->GetVarAddr(desc), ge::SUCCESS);
  ASSERT_NE(desc.dev_addr, nullptr);

  // 析构触发 Finalize，容器分配的设备内存在此释放
  mgr.reset();
}

TEST_F(Om2RTVarManagerTest, PoolGetAndRemove) {
  auto &pool = Om2RTVarManagerPool::Instance();
  auto mgr = pool.GetManager(42);
  ASSERT_NE(mgr, nullptr);
  auto mgr2 = pool.GetManager(42);
  EXPECT_EQ(mgr.get(), mgr2.get());
  pool.RemoveManager(42);
}

TEST_F(Om2RTVarManagerTest, GetVarAddrResolveWritesInitDataOnce) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  const auto init_data = MakePattern(0xA0U, kVarSize);

  auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size());
  ASSERT_EQ(mgr.GetVarAddr(desc), ge::SUCCESS);
  ASSERT_NE(desc.dev_addr, nullptr);
  EXPECT_EQ(desc.is_loaded, 1U);
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  EXPECT_EQ(runtime_stub.BytesWrittenTo(desc.dev_addr), kVarSize);
  EXPECT_EQ(runtime_stub.LastWritten(), init_data);

  // 重复解析同一变量：地址不变、不重复写入
  auto again = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size());
  ASSERT_EQ(mgr.GetVarAddr(again), ge::SUCCESS);
  EXPECT_EQ(again.dev_addr, desc.dev_addr);
  EXPECT_EQ(again.is_loaded, 1U);
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, GetVarAddrResolveWithoutDataLeavesUnloaded) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;

  // 转换/拷贝目标走这条路：只要地址，不要写入
  auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(desc), ge::SUCCESS);
  ASSERT_NE(desc.dev_addr, nullptr);
  EXPECT_EQ(desc.is_loaded, 0U);
  EXPECT_EQ(runtime_stub.H2DCount(), 0U);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, GetVarAddrQueryOnlyReturnsAddrAndBackfillsSize) {
  Om2RTVarManager mgr;
  auto resolve = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(resolve), ge::SUCCESS);

  // 只查询：SO 侧把 size 传 0，容器必须回填真实字节数（SO 随后要按它做 D2H）
  auto query = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(query), ge::SUCCESS);
  EXPECT_EQ(query.dev_addr, resolve.dev_addr);
  EXPECT_EQ(query.size, kVarSize);
  EXPECT_EQ(query.is_loaded, 0U);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, GetVarAddrQueryOnlyMissReturnsNullAddrWithoutError) {
  Om2RTVarManager mgr;
  // 未命中用出参 nullptr 表达，返回码仍是成功（与 QueryBinHandleFromStore 同一约定）
  auto query = MakeVarDesc("absent_key", 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
  EXPECT_EQ(mgr.GetVarAddr(query), ge::SUCCESS);
  EXPECT_EQ(query.dev_addr, nullptr);
  EXPECT_EQ(query.is_loaded, 0U);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, GetVarAddrRejectsInvalidArgs) {
  Om2RTVarManager mgr;
  const auto data = MakePattern(0x10U, kVarSize);

  auto bad_key = MakeVarDesc(nullptr, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  EXPECT_NE(mgr.GetVarAddr(bad_key), ge::SUCCESS);
  auto empty_key = MakeVarDesc("", kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  EXPECT_NE(mgr.GetVarAddr(empty_key), ge::SUCCESS);

  // RESOLVE 必须给出字节数才能分配
  auto zero_size = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  EXPECT_NE(mgr.GetVarAddr(zero_size), ge::SUCCESS);

  // host_data 非空时 host_data_size 必须等于 size，否则尺寸错配会导致越界写
  auto mismatched = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, data.data(), data.size() - 1U);
  EXPECT_NE(mgr.GetVarAddr(mismatched), ge::SUCCESS);

  // 只查询不得携带数据
  auto query_with_data = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, data.data(), data.size());
  EXPECT_NE(mgr.GetVarAddr(query_with_data), ge::SUCCESS);

  // mode 取值必须在枚举内
  auto bad_mode = MakeVarDesc(kVarKey, kVarSize, 7U, nullptr, 0U);
  EXPECT_NE(mgr.GetVarAddr(bad_mode), ge::SUCCESS);

  // struct_size 不足是跨版本误用的第一道拦截
  auto bad_struct = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  bad_struct.struct_size = sizeof(GertVarDesc) - 1U;
  EXPECT_NE(mgr.GetVarAddr(bad_struct), ge::SUCCESS);

  // 全部非法入参都不得建条目、不得写设备
  auto probe = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(probe), ge::SUCCESS);
  EXPECT_EQ(probe.dev_addr, nullptr);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, GetVarAddrUsesExternalArenaFromSetExternalVar) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  constexpr uint64_t kArenaOffset = 128U;
  std::vector<uint8_t> arena(4096U, 0U);
  mgr.SetExternalVar(arena.data(), arena.size());
  const auto init_data = MakePattern(0x20U, kVarSize);

  auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size(),
                          kMemoryVarLogicBase + kArenaOffset);
  ASSERT_EQ(mgr.GetVarAddr(desc), ge::SUCCESS);
  EXPECT_EQ(desc.dev_addr, static_cast<void *>(arena.data() + kArenaOffset));
  EXPECT_EQ(desc.is_loaded, 1U);
  EXPECT_EQ(std::vector<uint8_t>(arena.begin() + kArenaOffset, arena.begin() + kArenaOffset + kVarSize), init_data);

  // 区内越界必须被拦下
  auto overflow =
      MakeVarDesc("overflow_key", 8192U, GERT_VAR_ADDR_RESOLVE, nullptr, 0U, kMemoryVarLogicBase + kArenaOffset);
  EXPECT_NE(mgr.GetVarAddr(overflow), ge::SUCCESS);
  // 逻辑地址低于基址必须被拦下
  auto below_base = MakeVarDesc("below_key", kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U, kMemoryVarLogicBase - 1U);
  EXPECT_NE(mgr.GetVarAddr(below_base), ge::SUCCESS);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, CommitVarDataWritesThenIdempotentDiscard) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  auto resolve = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(resolve), ge::SUCCESS);
  ASSERT_NE(resolve.dev_addr, nullptr);

  const auto derived = MakePattern(0x50U, kVarSize);
  auto commit = MakeVarCommit(kVarKey, derived.data(), derived.size());
  ASSERT_EQ(mgr.CommitVarData(commit), ge::SUCCESS);
  EXPECT_EQ(commit.dev_addr, resolve.dev_addr);
  EXPECT_EQ(commit.is_loaded, 1U);
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  EXPECT_EQ(runtime_stub.LastWritten(), derived);

  // 已加载 ⇒ 幂等丢弃本次数据，返回既有地址，不视为错误
  const auto other = MakePattern(0xC0U, kVarSize);
  auto second = MakeVarCommit(kVarKey, other.data(), other.size());
  ASSERT_EQ(mgr.CommitVarData(second), ge::SUCCESS);
  EXPECT_EQ(second.dev_addr, resolve.dev_addr);
  EXPECT_EQ(second.is_loaded, 1U);
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  EXPECT_EQ(runtime_stub.LastWritten(), derived);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, CommitVarDataRejectsUnresolvedKeyAndOversize) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  const auto data = MakePattern(0x30U, kVarSize);

  // 不分配：目标地址未解析即报错（编程错误路径）
  auto unresolved = MakeVarCommit("absent_key", data.data(), data.size());
  EXPECT_NE(mgr.CommitVarData(unresolved), ge::SUCCESS);

  auto resolve = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(resolve), ge::SUCCESS);

  // 超过容器记下的变量字节数 ⇒ 报错且不写入（防越界写）
  const auto big = MakePattern(0x40U, kVarSize * 2U);
  auto oversize = MakeVarCommit(kVarKey, big.data(), big.size());
  EXPECT_NE(mgr.CommitVarData(oversize), ge::SUCCESS);
  EXPECT_EQ(runtime_stub.H2DCount(), 0U);

  // host_data 为空或长度为 0：本回调没有"只查询"语义
  auto no_data = MakeVarCommit(kVarKey, nullptr, 0U);
  EXPECT_NE(mgr.CommitVarData(no_data), ge::SUCCESS);
  auto zero_len = MakeVarCommit(kVarKey, data.data(), 0U);
  EXPECT_NE(mgr.CommitVarData(zero_len), ge::SUCCESS);

  auto bad_key = MakeVarCommit(nullptr, data.data(), data.size());
  EXPECT_NE(mgr.CommitVarData(bad_key), ge::SUCCESS);

  auto bad_struct = MakeVarCommit(kVarKey, data.data(), data.size());
  bad_struct.struct_size = sizeof(GertVarCommit) - 1U;
  EXPECT_NE(mgr.CommitVarData(bad_struct), ge::SUCCESS);

  EXPECT_EQ(runtime_stub.H2DCount(), 0U);
  mgr.Finalize();
}

TEST_F(Om2RTVarManagerTest, IsLoadedNeverGoesBackward) {
  Om2RTVarManager mgr;
  const auto data = MakePattern(0x60U, kVarSize);
  // 遍历"不带数据 / 只查询 / 带数据 / 提交派生数据 / 重复调用"的组合，断言状态只增不减
  auto no_data = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(no_data), ge::SUCCESS);
  EXPECT_EQ(no_data.is_loaded, 0U);

  auto query = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(query), ge::SUCCESS);
  EXPECT_EQ(query.is_loaded, 0U);

  auto with_data = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, data.data(), data.size());
  ASSERT_EQ(mgr.GetVarAddr(with_data), ge::SUCCESS);
  EXPECT_EQ(with_data.is_loaded, 1U);

  // 已加载后，任何后续调用都不得把状态改回 0
  auto commit = MakeVarCommit(kVarKey, data.data(), data.size());
  ASSERT_EQ(mgr.CommitVarData(commit), ge::SUCCESS);
  EXPECT_EQ(commit.is_loaded, 1U);

  auto repeat_no_data = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(repeat_no_data), ge::SUCCESS);
  EXPECT_EQ(repeat_no_data.is_loaded, 1U);

  auto repeat_query = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(repeat_query), ge::SUCCESS);
  EXPECT_EQ(repeat_query.is_loaded, 1U);
  mgr.Finalize();
}

// ==================== 并发（单把全局锁）====================

constexpr uint32_t kConcurrentThreadNum = 16U;

// 同一变量被多线程首次解析：只有一个线程执行分配与写入，其余读到同一地址
TEST_F(Om2RTVarManagerTest, ConcurrentGetVarAddrSameKeyWritesOnce) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  const auto init_data = MakePattern(0x70U, kVarSize);

  std::vector<void *> addrs(kConcurrentThreadNum, nullptr);
  std::vector<uint32_t> loaded(kConcurrentThreadNum, 0U);
  std::vector<ge::Status> rets(kConcurrentThreadNum, ge::FAILED);
  std::vector<std::thread> workers;
  workers.reserve(kConcurrentThreadNum);
  for (uint32_t i = 0U; i < kConcurrentThreadNum; ++i) {
    workers.emplace_back([&mgr, &init_data, &addrs, &loaded, &rets, &runtime_stub, i]() {
      // fake_instance_ 是 thread_local，桩必须在每个工作线程内各自安装
      AclRuntimeStubGuard worker_guard(&runtime_stub);
      auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size());
      rets[i] = mgr.GetVarAddr(desc);
      addrs[i] = desc.dev_addr;
      loaded[i] = desc.is_loaded;
    });
  }
  for (auto &worker : workers) {
    worker.join();
  }

  for (uint32_t i = 0U; i < kConcurrentThreadNum; ++i) {
    ASSERT_EQ(rets[i], ge::SUCCESS) << "thread " << i;
    EXPECT_EQ(addrs[i], addrs[0]) << "thread " << i;
    EXPECT_EQ(loaded[i], 1U) << "thread " << i;
  }
  ASSERT_NE(addrs[0], nullptr);
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  EXPECT_EQ(runtime_stub.BytesWrittenTo(addrs[0]), kVarSize);
  mgr.Finalize();
}

// 两个模型并发提交同一目标：is_loaded 守卫使恰好一次写入生效，后到者被幂等丢弃且不报错
TEST_F(Om2RTVarManagerTest, ConcurrentCommitVarDataSameKeyWritesOnce) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  auto resolve = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, nullptr, 0U);
  ASSERT_EQ(mgr.GetVarAddr(resolve), ge::SUCCESS);
  ASSERT_NE(resolve.dev_addr, nullptr);

  std::vector<ge::Status> rets(kConcurrentThreadNum, ge::FAILED);
  std::vector<uint32_t> loaded(kConcurrentThreadNum, 0U);
  std::vector<std::thread> workers;
  workers.reserve(kConcurrentThreadNum);
  for (uint32_t i = 0U; i < kConcurrentThreadNum; ++i) {
    workers.emplace_back([&mgr, &rets, &loaded, &runtime_stub, i]() {
      AclRuntimeStubGuard worker_guard(&runtime_stub);
      // 每个线程准备各自的数据，用于验证最终落盘的是其中某一份完整的、而非交织的
      const auto derived = MakePattern(static_cast<uint8_t>(0x80U + i), kVarSize);
      auto commit = MakeVarCommit(kVarKey, derived.data(), derived.size());
      rets[i] = mgr.CommitVarData(commit);
      loaded[i] = commit.is_loaded;
    });
  }
  for (auto &worker : workers) {
    worker.join();
  }

  for (uint32_t i = 0U; i < kConcurrentThreadNum; ++i) {
    EXPECT_EQ(rets[i], ge::SUCCESS) << "thread " << i;
    EXPECT_EQ(loaded[i], 1U) << "thread " << i;
  }
  EXPECT_EQ(runtime_stub.H2DCount(), 1U);
  EXPECT_EQ(runtime_stub.BytesWrittenTo(resolve.dev_addr), kVarSize);
  mgr.Finalize();
}

// 只查询与地址解析并发无撕裂读：只允许两种结果——未命中，或命中且内容完整
TEST_F(Om2RTVarManagerTest, ConcurrentQueryOnlyVsResolveNoTornRead) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  const auto init_data = MakePattern(0x90U, kVarSize);
  constexpr uint32_t kRoundNum = 200U;
  std::atomic<bool> torn{false};
  std::atomic<bool> stop{false};

  std::thread resolver([&mgr, &init_data, &runtime_stub]() {
    AclRuntimeStubGuard worker_guard(&runtime_stub);
    auto desc = MakeVarDesc(kVarKey, kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size());
    (void)mgr.GetVarAddr(desc);
  });
  std::thread querier([&mgr, &init_data, &torn, &stop]() {
    for (uint32_t round = 0U; (round < kRoundNum) && !stop.load(); ++round) {
      auto query = MakeVarDesc(kVarKey, 0U, GERT_VAR_ADDR_QUERY_ONLY, nullptr, 0U);
      if (mgr.GetVarAddr(query) != ge::SUCCESS) {
        continue;
      }
      if (query.dev_addr == nullptr) {
        continue;  // 未命中，合法
      }
      // 命中则内容必须完整：分配 + H2D + 置位在容器同一把锁内原子完成
      const auto *begin = static_cast<const uint8_t *>(query.dev_addr);
      if (!std::equal(init_data.begin(), init_data.end(), begin)) {
        torn.store(true);
        stop.store(true);
      }
    }
  });
  resolver.join();
  querier.join();
  EXPECT_FALSE(torn.load());
  mgr.Finalize();
}

// 单锁串行化基线：只建立数据、不断言并行度。
// 该数据是将来判断"是否需要引入每变量锁"的唯一依据——单锁下不同变量的 H2D 本就串行。
TEST_F(Om2RTVarManagerTest, SingleLockSerializationBaseline) {
  H2DRecordingAclRuntimeStub runtime_stub;
  AclRuntimeStubGuard runtime_stub_guard(&runtime_stub);
  Om2RTVarManager mgr;
  constexpr uint32_t kVarNum = 64U;
  const auto init_data = MakePattern(0xB0U, kVarSize);
  std::vector<std::string> keys;
  keys.reserve(kVarNum);
  for (uint32_t i = 0U; i < kVarNum; ++i) {
    keys.push_back("baseline_var_" + std::to_string(i));
  }

  const auto begin = std::chrono::steady_clock::now();
  std::vector<std::thread> workers;
  workers.reserve(kConcurrentThreadNum);
  for (uint32_t t = 0U; t < kConcurrentThreadNum; ++t) {
    workers.emplace_back([&mgr, &keys, &init_data, &runtime_stub, t]() {
      AclRuntimeStubGuard worker_guard(&runtime_stub);
      for (size_t i = t; i < keys.size(); i += kConcurrentThreadNum) {
        auto desc = MakeVarDesc(keys[i].c_str(), kVarSize, GERT_VAR_ADDR_RESOLVE, init_data.data(), init_data.size());
        (void)mgr.GetVarAddr(desc);
      }
    });
  }
  for (auto &worker : workers) {
    worker.join();
  }
  const auto elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

  EXPECT_EQ(runtime_stub.H2DCount(), kVarNum);
  // 基线数据，供后续"是否引入每变量锁"的决策使用；不设阈值断言，避免机器负载造成抖动
  printf("[OM2][Var][Baseline] %u vars / %u threads, total=%" PRId64 " us, h2d_count=%u\n", kVarNum,
         kConcurrentThreadNum, elapsed_us, runtime_stub.H2DCount());
  mgr.Finalize();
}

}  // namespace
}  // namespace gert
