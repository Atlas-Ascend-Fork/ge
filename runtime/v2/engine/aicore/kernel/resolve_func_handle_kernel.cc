/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "resolve_func_handle_kernel.h"
#include <cstring>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include "acl/acl_rt.h"
#include "common/checker.h"
#include "core/executor/exec_ctx_cache.h"
#include "framework/common/debug/log.h"
#include "graph/error_codes.h"
#include "register/kernel_registry.h"
#include "securec.h"

namespace gert {
namespace kernel {
namespace {
enum class ResolveInput { kBinHandle = 0, kTilingKey, kKernelName, kWithHandleFlag, kNum };
enum class ResolveOutput { kFuncHandle = 0, kNum };

struct EntryCacheKey {
  uint64_t bin_ptr{0U};
  void *ctx{nullptr};
  uint64_t tiling_key{0U};
  bool operator==(const EntryCacheKey &rhs) const {
    return (bin_ptr == rhs.bin_ptr) && (ctx == rhs.ctx) && (tiling_key == rhs.tiling_key);
  }
};

struct EntryCacheKeyHash {
  size_t operator()(const EntryCacheKey &key) const {
    uint64_t hash = key.bin_ptr ^ (reinterpret_cast<uint64_t>(key.ctx) >> 4U) ^ key.tiling_key;
    hash ^= hash >> 33U;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33U;
    return static_cast<size_t>(hash);
  }
};

struct BinCtxKey {
  uint64_t bin_ptr{0U};
  void *ctx{nullptr};
  bool operator==(const BinCtxKey &rhs) const {
    return (bin_ptr == rhs.bin_ptr) && (ctx == rhs.ctx);
  }
};

struct BinCtxKeyHash {
  size_t operator()(const BinCtxKey &key) const {
    uint64_t hash = key.bin_ptr ^ (reinterpret_cast<uint64_t>(key.ctx) >> 4U);
    hash ^= hash >> 33U;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33U;
    return static_cast<size_t>(hash);
  }
};

std::shared_mutex g_func_cache_mutex;
std::unordered_map<EntryCacheKey, aclrtFuncHandle, EntryCacheKeyHash> g_entry_cache;
std::unordered_map<BinCtxKey, std::map<std::string, aclrtFuncHandle, std::less<>>, BinCtxKeyHash> g_name_cache;
// ===== resolve L1 TLS 免锁缓存：命中免全局锁与哈希表；未命中回退全局缓存，正确性以全局缓存兜底 =====
// 按路线分两张表，互不挤占：by_name 表 1024 槽（名字指纹 + strcmp 复核），by_entry 表 256 槽（tiling_key 直接做键）。
// 总内存 1024*160B + 256*32B = 168KB/线程。
// 超出容量时仅命中率下降（未命中回退 L2，成本 ~230ns），不影响正确性。
constexpr size_t kTlsByNameSlotNum = 1024U;
constexpr size_t kTlsByEntrySlotNum = 256U;
constexpr size_t kTlsFuncHandleProbeNum = 8U;
// L1 by_name 表内联保存内核名副本：内核名原始数据随 exe graph/model 生命周期结束而释放，
// 若只存裸指针，模型卸载后同线程命中 TLS 槽会在 strcmp 时读已释放内存（use-after-free）。
// 超长内核名（>= kTlsKernelNameMaxLen）不进 L1，回退 L2 全局缓存（std::string 副本）兜底。
constexpr size_t kTlsKernelNameMaxLen = 128U;

struct TlsByNameEntry {
  uint64_t bin_ptr{0U};
  void *ctx{nullptr};
  uint64_t key_hash{0U};
  char kernel_name[kTlsKernelNameMaxLen]{};
  aclrtFuncHandle func_handle{nullptr};
};

struct TlsByEntryEntry {
  uint64_t bin_ptr{0U};
  void *ctx{nullptr};
  uint64_t tiling_key{0U};
  aclrtFuncHandle func_handle{nullptr};
};

thread_local TlsByNameEntry g_tls_by_name[kTlsByNameSlotNum];
thread_local TlsByEntryEntry g_tls_by_entry[kTlsByEntrySlotNum];

uint64_t HashKernelName(const char *kernel_name) {
  uint64_t hash = 1469598103934665603ULL;  // FNV-1a 64 位
  while ((kernel_name != nullptr) && (*kernel_name != '\0')) {
    hash ^= static_cast<uint8_t>(*kernel_name);
    hash *= 1099511628211ULL;
    ++kernel_name;
  }
  return hash;
}

// 由 (bin, ctx, key) 混散出槽位号：SlotNum 由各表的数组长度推导。
template <size_t SlotNum>
size_t TlsSlotIndex(const uint64_t bin_ptr, const void *ctx, const uint64_t key) {
  uint64_t mix = bin_ptr ^ (reinterpret_cast<uint64_t>(ctx) >> 4U) ^ key;
  mix ^= mix >> 33U;
  mix *= 0xff51afd7ed558ccdULL;
  mix ^= mix >> 33U;
  return static_cast<size_t>(mix) % SlotNum;
}

// 定长开放寻址表的查/写通用骨架，两张表复用同一份探测逻辑：
// EntryT 需含 func_handle 成员作为空槽标志（只写不删，首空槽终止保证命中键必在探测窗口内）。
template <typename EntryT, size_t SlotNum, typename MatchT>
bool TlsProbeGet(const EntryT (&table)[SlotNum], const size_t idx, MatchT &&match, aclrtFuncHandle &func_handle) {
  for (size_t i = 0U; i < kTlsFuncHandleProbeNum; ++i) {
    const auto &entry = table[(idx + i) % SlotNum];
    if (entry.func_handle == nullptr) {
      return false;  // 首空槽终止：本缓存只写不删，命中键必在 home 槽与首空槽之间
    }
    if (match(entry)) {
      func_handle = entry.func_handle;
      return true;
    }
  }
  return false;
}

template <typename EntryT, size_t SlotNum, typename FillT>
void TlsProbePut(EntryT (&table)[SlotNum], const size_t idx, FillT &&fill) {
  for (size_t i = 0U; i < kTlsFuncHandleProbeNum; ++i) {
    auto &entry = table[(idx + i) % SlotNum];
    if (entry.func_handle == nullptr) {
      fill(entry);
      return;
    }
  }
  // 探测耗尽：覆盖 home 槽（直接淘汰；正确性由全局缓存兜底，仅损失一次命中）
  fill(table[idx]);
}

bool TlsByNameGet(const uint64_t bin_ptr, const void *ctx, const char *kernel_name, aclrtFuncHandle &func_handle) {
  const uint64_t key_hash = HashKernelName(kernel_name);
  const size_t idx = TlsSlotIndex<kTlsByNameSlotNum>(bin_ptr, ctx, key_hash);
  return TlsProbeGet(
      g_tls_by_name, idx,
      [bin_ptr, ctx, key_hash, kernel_name](const TlsByNameEntry &entry) {
        if ((entry.bin_ptr != bin_ptr) || (entry.ctx != ctx) || (entry.key_hash != key_hash)) {
          return false;
        }
        return (strcmp(entry.kernel_name, kernel_name) == 0);
      },
      func_handle);
}

void TlsByNamePut(const uint64_t bin_ptr, const void *ctx, const char *kernel_name, const aclrtFuncHandle func_handle) {
  if (func_handle == nullptr) {
    return;
  }
  const size_t name_len = strlen(kernel_name);
  if (name_len >= kTlsKernelNameMaxLen) {
    return;
  }
  const uint64_t key_hash = HashKernelName(kernel_name);
  const size_t idx = TlsSlotIndex<kTlsByNameSlotNum>(bin_ptr, ctx, key_hash);
  TlsProbePut(g_tls_by_name, idx, [bin_ptr, ctx, key_hash, kernel_name, name_len, func_handle](TlsByNameEntry &entry) {
    entry.bin_ptr = bin_ptr;
    entry.ctx = const_cast<void *>(ctx);
    entry.key_hash = key_hash;
    (void)memcpy_s(entry.kernel_name, sizeof(entry.kernel_name), kernel_name, name_len + 1U);
    entry.func_handle = func_handle;
  });
}

bool TlsByEntryGet(const uint64_t bin_ptr, const void *ctx, const uint64_t tiling_key, aclrtFuncHandle &func_handle) {
  const size_t idx = TlsSlotIndex<kTlsByEntrySlotNum>(bin_ptr, ctx, tiling_key);
  return TlsProbeGet(
      g_tls_by_entry, idx,
      [bin_ptr, ctx, tiling_key](const TlsByEntryEntry &entry) {
        return (entry.bin_ptr == bin_ptr) && (entry.ctx == ctx) && (entry.tiling_key == tiling_key);
      },
      func_handle);
}

void TlsByEntryPut(const uint64_t bin_ptr, const void *ctx, const uint64_t tiling_key,
                   const aclrtFuncHandle func_handle) {
  if (func_handle == nullptr) {
    return;
  }
  const size_t idx = TlsSlotIndex<kTlsByEntrySlotNum>(bin_ptr, ctx, tiling_key);
  TlsProbePut(g_tls_by_entry, idx, [bin_ptr, ctx, tiling_key, func_handle](TlsByEntryEntry &entry) {
    entry = {bin_ptr, const_cast<void *>(ctx), tiling_key, func_handle};
  });
}
// ===== resolve L1 TLS 缓存结束 =====

ge::graphStatus GetFuncHandleByEntry(const aclrtBinHandle bin_handle, const uint64_t tiling_key,
                                     aclrtFuncHandle &func_handle) {
  aclError ret = aclrtBinaryGetFunctionByEntry(bin_handle, tiling_key, &func_handle);
  if (ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "Get func handle by tiling key [%lu] failed, ret %d.", tiling_key, ret);
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetFuncHandleByName(const aclrtBinHandle bin_handle, const char *kernel_name,
                                    aclrtFuncHandle &func_handle) {
  aclError ret = aclrtBinaryGetFunction(bin_handle, kernel_name, &func_handle);
  if (ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "Get func handle by kernel name [%s] failed, ret %d.", kernel_name, ret);
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

// 以下 Lookup 接口要求调用方已持有 g_func_cache_mutex，未命中返回 nullptr。
aclrtFuncHandle LookupEntryCache(const uint64_t bin_ptr, void *ctx, const uint64_t tiling_key) {
  auto iter = g_entry_cache.find({bin_ptr, ctx, tiling_key});
  return (iter == g_entry_cache.end()) ? nullptr : iter->second;
}

aclrtFuncHandle LookupNameCache(const uint64_t bin_ptr, void *ctx, const char *kernel_name) {
  auto bin_iter = g_name_cache.find({bin_ptr, ctx});
  if (bin_iter == g_name_cache.end()) {
    return nullptr;
  }
  auto name_iter = bin_iter->second.find(kernel_name);
  return (name_iter == bin_iter->second.end()) ? nullptr : name_iter->second;
}

// L2 缓存按 tiling_key 定位：先读锁查，未命中再升级写锁二次确认后回源 aclrt 并回填。
ge::graphStatus ResolveFuncHandleByEntry(const aclrtBinHandle bin_handle, const uint64_t bin_ptr, void *ctx,
                                         const uint64_t tiling_key, aclrtFuncHandle &func_handle) {
  {
    std::shared_lock<std::shared_mutex> read_lock(g_func_cache_mutex);
    func_handle = LookupEntryCache(bin_ptr, ctx, tiling_key);
  }
  if (func_handle != nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  std::unique_lock<std::shared_mutex> write_lock(g_func_cache_mutex);
  func_handle = LookupEntryCache(bin_ptr, ctx, tiling_key);
  if (func_handle != nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  if (GetFuncHandleByEntry(bin_handle, tiling_key, func_handle) != ge::GRAPH_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  (void)g_entry_cache.emplace(EntryCacheKey{bin_ptr, ctx, tiling_key}, func_handle);
  return ge::GRAPH_SUCCESS;
}

// L2 缓存按内核名定位，加锁策略同 ResolveFuncHandleByEntry。
ge::graphStatus ResolveFuncHandleByName(const aclrtBinHandle bin_handle, const uint64_t bin_ptr, void *ctx,
                                        const char *kernel_name, aclrtFuncHandle &func_handle) {
  {
    std::shared_lock<std::shared_mutex> read_lock(g_func_cache_mutex);
    func_handle = LookupNameCache(bin_ptr, ctx, kernel_name);
  }
  if (func_handle != nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  std::unique_lock<std::shared_mutex> write_lock(g_func_cache_mutex);
  func_handle = LookupNameCache(bin_ptr, ctx, kernel_name);
  if (func_handle != nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  if (GetFuncHandleByName(bin_handle, kernel_name, func_handle) != ge::GRAPH_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  (void)g_name_cache[{bin_ptr, ctx}].emplace(std::string(kernel_name), func_handle);
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus ResolveKernelFuncHandle(KernelContext *context) {
  auto bin_handle = context->GetInputValue<void *>(static_cast<int32_t>(ResolveInput::kBinHandle));
  GE_ASSERT_NOTNULL(bin_handle);
  const uint64_t *tiling_key_ptr = context->GetInputPointer<uint64_t>(static_cast<int32_t>(ResolveInput::kTilingKey));
  GE_ASSERT_NOTNULL(tiling_key_ptr);
  const uint64_t tiling_key = *tiling_key_ptr;
  auto kernel_name = context->GetInputValue<char *>(static_cast<int32_t>(ResolveInput::kKernelName));
  GE_ASSERT_NOTNULL(kernel_name);
  const uint32_t with_handle_flag =
      context->GetInputValue<uint32_t>(static_cast<int32_t>(ResolveInput::kWithHandleFlag));

  // ctx 仅作为 L1/L2 缓存键标签，缺 ctx 时不构成失败面：跳过缓存直接解析句柄（与旧 GetFuncHandle 行为一致）。
  aclrtContext ctx = gert::GetExecCtx();

  const uint64_t bin_ptr = reinterpret_cast<uint64_t>(bin_handle);
  aclrtFuncHandle func_handle = nullptr;
  if (with_handle_flag != 0U) {
    if (ctx == nullptr) {
      if (GetFuncHandleByEntry(bin_handle, tiling_key, func_handle) != ge::GRAPH_SUCCESS) {
        return ge::GRAPH_FAILED;
      }
    } else if (!TlsByEntryGet(bin_ptr, ctx, tiling_key, func_handle)) {
      if (ResolveFuncHandleByEntry(bin_handle, bin_ptr, ctx, tiling_key, func_handle) != ge::GRAPH_SUCCESS) {
        return ge::GRAPH_FAILED;
      }
      TlsByEntryPut(bin_ptr, ctx, tiling_key, func_handle);
    }
  } else {
    if (ctx == nullptr) {
      if (GetFuncHandleByName(bin_handle, kernel_name, func_handle) != ge::GRAPH_SUCCESS) {
        return ge::GRAPH_FAILED;
      }
    } else if (!TlsByNameGet(bin_ptr, ctx, kernel_name, func_handle)) {
      if (ResolveFuncHandleByName(bin_handle, bin_ptr, ctx, kernel_name, func_handle) != ge::GRAPH_SUCCESS) {
        return ge::GRAPH_FAILED;
      }
      TlsByNamePut(bin_ptr, ctx, kernel_name, func_handle);
    }
  }
  GE_ASSERT_NOTNULL(func_handle, "Failed to get func handle.");

  auto output = context->GetOutputPointer<void *>(static_cast<int32_t>(ResolveOutput::kFuncHandle));
  GE_ASSERT_NOTNULL(output);
  *output = func_handle;
  return ge::GRAPH_SUCCESS;
}

REGISTER_KERNEL(ResolveKernelFuncHandle).RunFunc(ResolveKernelFuncHandle);
}  // namespace kernel
}  // namespace gert
