/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_RUNTIME_OM2_RT_VAR_MANAGER_H_
#define AIR_RUNTIME_OM2_RT_VAR_MANAGER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "framework/om2/model_api/om2_model_api.h"
#include "framework/om2/model_data/gert_model_data.h"

constexpr uint64_t kMemoryVarLogicBase =
    824633720832U;  // 768GB, keep consistent with kVarMemoryLogicBase in graph_var_manager.h

namespace gert {

// 不含 mutex，保持可拷贝/可移动：并发由 Om2RTVarManager 的单把全局锁保证
struct RTVarRuntimeState {
  std::map<uint32_t, void *> dev_addrs;
  std::map<uint32_t, bool> is_loaded;
  uint64_t size = 0U;  // 解析地址时记下的变量字节数，供提交派生数据时做越界校验与只查询回填
};

class Om2RTVarManager {
 public:
  Om2RTVarManager() = default;
  ~Om2RTVarManager();

  ge::Status GetOrCreateVarAddr(const std::string &key, const uint32_t device_id, const size_t size, void *&addr);
  bool TryGetVarAddr(const std::string &key, const uint32_t device_id, void *&addr) const;
  void Finalize() noexcept;

  // 模型 SO 侧变量准备的两个回调实现，语义见 om2_model_api.h 的 GetVarAddrFunc / CommitVarDataFunc。
  // 变量设备内存一律由本容器分配、写入与释放，调用方拿到的地址是只读引用。
  ge::Status GetVarAddr(GertVarDesc &desc);
  ge::Status CommitVarData(GertVarCommit &commit);
  // 外置变量内存区由执行器直接交给容器，不经模型 SO
  void SetExternalVar(void *const external_var_addr, const uint64_t external_var_size);

 private:
  ge::Status AllocDevAddr(const uint64_t size, const uint64_t memory_type, void *&dev_addr) const;
  ge::Status ResolveDevAddr(const GertVarDesc &desc, void *&dev_addr) const;

  RTVarRuntimeState &GetOrCreateRuntimeState(const std::string &var_key);

  uint64_t logic_var_base_{kMemoryVarLogicBase};
  std::unordered_map<std::string, RTVarRuntimeState> var_runtime_states_;
  void *external_var_addr_{nullptr};
  uint64_t external_var_size_{0};
  struct LegacyVarAddrInfo {
    void *addr = nullptr;
    size_t size = 0U;
  };

  // 单把全局锁，覆盖映射表操作、地址解析与设备内存写入。
  // 用 std::mutex 而非 recursive_mutex：持锁方法不得互调，同线程重复加锁是 UB。
  // mutable 必须保留——TryGetVarAddr 是 const 方法却要加锁。
  mutable std::mutex mutex_;
  std::map<uint32_t, std::map<std::string, LegacyVarAddrInfo>> legacy_device_to_vars_;
};

using Om2RTVarManagerPtr = std::shared_ptr<Om2RTVarManager>;

class __attribute__((visibility("default"))) Om2RTVarManagerPool {
 public:
  static Om2RTVarManagerPool &Instance();
  ~Om2RTVarManagerPool();
  Om2RTVarManagerPtr GetManager(const uint64_t session_id);
  void RemoveManager(const uint64_t session_id);
  void Destroy() noexcept;

 private:
  Om2RTVarManagerPool() = default;
  std::mutex mutex_;
  std::map<uint64_t, Om2RTVarManagerPtr> session_id_to_manager_;
};

}  // namespace gert

#endif  // AIR_RUNTIME_OM2_RT_VAR_MANAGER_H_
