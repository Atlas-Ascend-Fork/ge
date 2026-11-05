/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_rt_var_manager.h"

#include <cinttypes>

#include "acl/acl_rt.h"
#include "common/checker.h"
#include "common/ge_inner_error_codes.h"
#include "common/ge_common/debug/ge_log.h"
#include "graph_metadef/common/ge_common/util.h"
#include "om2_malloc_helper.h"
#include "rt_external_mem.h"
#include "framework/common/gert_model_data_utils.h"

namespace gert {

Om2RTVarManager::~Om2RTVarManager() {
  Finalize();
}

ge::Status Om2RTVarManager::AllocDevAddr(const uint64_t size, const uint64_t memory_type, void *&dev_addr) const {
  void *new_addr = nullptr;
  const auto malloc_ret = Om2Malloc(&new_addr, size, static_cast<rtMemType_t>(memory_type), 0);
  if (malloc_ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "[OM2][Var][Alloc] aclrtMalloc failed, size=%" PRIu64 ", memory_type=%" PRIu64 ", ret=%u.", size,
           memory_type, malloc_ret);
    return ge::FAILED;
  }
  dev_addr = new_addr;
  return ge::SUCCESS;
}

RTVarRuntimeState &Om2RTVarManager::GetOrCreateRuntimeState(const std::string &var_key) {
  return var_runtime_states_[var_key];
}

ge::Status Om2RTVarManager::ResolveDevAddr(const GertVarDesc &desc, void *&dev_addr) const {
  if (external_var_addr_ != nullptr) {
    if (desc.logic_addr < logic_var_base_) {
      GELOGE(ge::FAILED, "[OM2][Var][Alloc] logic_addr %" PRIu64 " < base %" PRIu64 " for var_key=%s.", desc.logic_addr,
             logic_var_base_, desc.var_key);
      return ge::FAILED;
    }
    const uint64_t offset = desc.logic_addr - logic_var_base_;
    if (offset > external_var_size_ || desc.size > (external_var_size_ - offset)) {
      GELOGE(ge::FAILED,
             "[OM2][Var][Alloc] external arena overflow, var_key=%s, offset=%" PRIu64 ", size=%" PRIu64
             ", arena=%" PRIu64 ".",
             desc.var_key, offset, desc.size, external_var_size_);
      return ge::FAILED;
    }
    dev_addr = static_cast<uint8_t *>(external_var_addr_) + offset;
    return ge::SUCCESS;
  }
  return AllocDevAddr(desc.size, desc.memory_type, dev_addr);
}

ge::Status Om2RTVarManager::GetVarAddr(GertVarDesc &desc) {
  if (desc.struct_size < sizeof(GertVarDesc)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] GertVarDesc struct_size %" PRIu64 " < %zu.", desc.struct_size,
           sizeof(GertVarDesc));
    return ge::PARAM_INVALID;
  }
  if (desc.var_key == nullptr || desc.var_key[0] == '\0') {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] var_key is null or empty.");
    return ge::PARAM_INVALID;
  }
  if ((desc.mode != static_cast<uint32_t>(GERT_VAR_ADDR_RESOLVE)) &&
      (desc.mode != static_cast<uint32_t>(GERT_VAR_ADDR_QUERY_ONLY))) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] invalid mode=%u, var_key=%s.", desc.mode, desc.var_key);
    return ge::PARAM_INVALID;
  }
  const bool query_only = (desc.mode == static_cast<uint32_t>(GERT_VAR_ADDR_QUERY_ONLY));
  if (query_only && (desc.host_data != nullptr)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] query-only mode must not carry host_data, var_key=%s.", desc.var_key);
    return ge::PARAM_INVALID;
  }
  if (!query_only && (desc.size == 0U)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] resolve mode requires size > 0, var_key=%s.", desc.var_key);
    return ge::PARAM_INVALID;
  }
  if ((desc.host_data != nullptr) && (desc.host_data_size != desc.size)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] host_data_size %" PRIu64 " != size %" PRIu64 ", var_key=%s.",
           desc.host_data_size, desc.size, desc.var_key);
    return ge::PARAM_INVALID;
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  auto &state = GetOrCreateRuntimeState(std::string(desc.var_key));
  const auto addr_it = state.dev_addrs.find(desc.device_id);

  if (query_only) {
    if (addr_it == state.dev_addrs.end()) {
      // 未命中用出参 nullptr 表达，返回码仍是成功（与 QueryBinHandleFromStore 同一约定）
      desc.dev_addr = nullptr;
      desc.is_loaded = 0U;
      GELOGW("[OM2][Var] query miss, var_key=%s, device=%u.", desc.var_key, desc.device_id);
      return ge::SUCCESS;
    }
    desc.dev_addr = addr_it->second;
    // 回填字节数：调用方随后要对该地址做设备到主机拷贝，而它没有源条目的元数据
    desc.size = state.size;
    const auto loaded_it = state.is_loaded.find(desc.device_id);
    desc.is_loaded = ((loaded_it != state.is_loaded.end()) && loaded_it->second) ? 1U : 0U;
    return ge::SUCCESS;
  }

  void *dev_addr = nullptr;
  if (addr_it != state.dev_addrs.end()) {
    // 共享变量一致性：同一 var_key 被多个子模型解析时字节数必须一致。
    if (state.size != desc.size) {
      GELOGE(ge::PARAM_INVALID, "[OM2][Var] size conflict for var_key=%s, existing=%" PRIu64 ", incoming=%" PRIu64 ".",
             desc.var_key, state.size, desc.size);
      return ge::PARAM_INVALID;
    }
    dev_addr = addr_it->second;
  } else {
    GE_RETURN_IF_ERROR(ResolveDevAddr(desc, dev_addr));
    state.dev_addrs[desc.device_id] = dev_addr;
    state.size = desc.size;
  }

  if ((desc.host_data != nullptr) && !state.is_loaded[desc.device_id]) {
    const auto ret = aclrtMemcpy(dev_addr, desc.size, desc.host_data, desc.host_data_size, ACL_MEMCPY_HOST_TO_DEVICE);
    if (ret != ACL_SUCCESS) {
      // 已解析的地址保留在容器内、is_loaded 保持假，由 Finalize 释放
      GELOGE(ge::FAILED, "[OM2][Var][H2D] aclrtMemcpy failed, var_key=%s, ret=%u.", desc.var_key, ret);
      return ge::FAILED;
    }
    state.is_loaded[desc.device_id] = true;
  }

  desc.dev_addr = dev_addr;
  desc.is_loaded = state.is_loaded[desc.device_id] ? 1U : 0U;
  return ge::SUCCESS;
}

ge::Status Om2RTVarManager::CommitVarData(GertVarCommit &commit) {
  if (commit.struct_size < sizeof(GertVarCommit)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] GertVarCommit struct_size %" PRIu64 " < %zu.", commit.struct_size,
           sizeof(GertVarCommit));
    return ge::PARAM_INVALID;
  }
  if (commit.var_key == nullptr || commit.var_key[0] == '\0') {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] var_key is null or empty.");
    return ge::PARAM_INVALID;
  }
  if ((commit.host_data == nullptr) || (commit.host_data_size == 0U)) {
    GELOGE(ge::PARAM_INVALID, "[OM2][Var] commit requires non-empty host_data, var_key=%s.", commit.var_key);
    return ge::PARAM_INVALID;
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  const auto state_it = var_runtime_states_.find(std::string(commit.var_key));
  if (state_it == var_runtime_states_.end()) {
    // 编程错误：本回调不分配内存，该变量必须已由 GetVarAddr 解析出地址
    GELOGE(ge::FAILED, "[OM2][Var][Commit] no state for var_key=%s, GetVarAddr must be called first.", commit.var_key);
    return ge::FAILED;
  }
  auto &state = state_it->second;
  const auto addr_it = state.dev_addrs.find(commit.device_id);
  if (addr_it == state.dev_addrs.end() || addr_it->second == nullptr) {
    GELOGE(ge::FAILED, "[OM2][Var][Commit] no addr for var_key=%s, device=%u.", commit.var_key, commit.device_id);
    return ge::FAILED;
  }

  const auto loaded_it = state.is_loaded.find(commit.device_id);
  if ((loaded_it != state.is_loaded.end()) && loaded_it->second) {
    // 幂等：已加载则丢弃本次数据，返回既有地址，不视为错误
    commit.dev_addr = addr_it->second;
    commit.is_loaded = 1U;
    return ge::SUCCESS;
  }
  if (commit.host_data_size > state.size) {
    GELOGE(ge::FAILED, "[OM2][Var][Commit] host_data_size %" PRIu64 " > recorded size %" PRIu64 ", var_key=%s.",
           commit.host_data_size, state.size, commit.var_key);
    return ge::FAILED;
  }

  const auto ret =
      aclrtMemcpy(addr_it->second, state.size, commit.host_data, commit.host_data_size, ACL_MEMCPY_HOST_TO_DEVICE);
  if (ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "[OM2][Var][Commit] aclrtMemcpy failed, var_key=%s, ret=%u.", commit.var_key, ret);
    return ge::FAILED;
  }
  state.is_loaded[commit.device_id] = true;
  commit.dev_addr = addr_it->second;
  commit.is_loaded = 1U;
  return ge::SUCCESS;
}

void Om2RTVarManager::SetExternalVar(void *const external_var_addr, const uint64_t external_var_size) {
  const std::lock_guard<std::mutex> lock(mutex_);
  external_var_addr_ = external_var_addr;
  external_var_size_ = external_var_size;
}

void Om2RTVarManager::Finalize() noexcept {
  const std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[var_key, state] : var_runtime_states_) {
    // var_runtime_states_ 只由模型 SO 经 GetVarAddr 填充，本身即待释放地址的权威集合，
    // 无需再拿别的表作护栏。
    (void)var_key;
    for (auto &[_, addr] : state.dev_addrs) {
      (void)_;
      const auto external_begin = reinterpret_cast<uintptr_t>(external_var_addr_);
      const auto value = reinterpret_cast<uintptr_t>(addr);
      const bool is_external =
          external_var_addr_ != nullptr && value >= external_begin && (value - external_begin) < external_var_size_;
      // 无需比对 extern_dev_addr：编译期已知的外部设备地址（CONSTPLACEHOLDER）由模型 SO 的
      // GetVarDevAddr 直接使用、不调 get_var_addr 回调，故这类地址根本不会进入
      // var_runtime_states_，本循环见不到它们。
      if (addr != nullptr && !is_external) {
        (void)aclrtFree(addr);
      }
    }
  }
  var_runtime_states_.clear();
  for (auto &[_, vars] : legacy_device_to_vars_) {
    (void)_;
    for (auto &[__, info] : vars) {
      (void)__;
      if (info.addr != nullptr) {
        (void)aclrtFree(info.addr);
      }
    }
  }
  legacy_device_to_vars_.clear();
}

ge::Status Om2RTVarManager::GetOrCreateVarAddr(const std::string &key, const uint32_t device_id, const size_t size,
                                               void *&addr) {
  addr = nullptr;
  if (size == 0U) {
    return ge::SUCCESS;
  }
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    auto dev_it = legacy_device_to_vars_.find(device_id);
    if (dev_it != legacy_device_to_vars_.end()) {
      auto var_it = dev_it->second.find(key);
      if (var_it != dev_it->second.end()) {
        if (var_it->second.size != size) {
          GELOGE(ge::FAILED, "[OM2][Var][Legacy] size mismatch for key=%s, cached=%zu, requested=%zu.", key.c_str(),
                 var_it->second.size, size);
          return ge::FAILED;
        }
        addr = var_it->second.addr;
        return ge::SUCCESS;
      }
    }
  }
  void *new_addr = nullptr;
  const auto malloc_ret = Om2Malloc(&new_addr, size, RT_MEMORY_HBM, 0);
  if (malloc_ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "[OM2][Var][Alloc] legacy malloc failed, key=%s, size=%zu.", key.c_str(), size);
    return ge::FAILED;
  }
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    auto &dev_vars = legacy_device_to_vars_[device_id];
    auto var_it = dev_vars.find(key);
    if (var_it != dev_vars.end()) {
      if (var_it->second.size != size) {
        (void)aclrtFree(new_addr);
        GELOGE(ge::FAILED, "[OM2][Var][Legacy] size mismatch for key=%s, cached=%zu, requested=%zu.", key.c_str(),
               var_it->second.size, size);
        return ge::FAILED;
      }
      addr = var_it->second.addr;
      (void)aclrtFree(new_addr);
      return ge::SUCCESS;
    }
    dev_vars[key] = LegacyVarAddrInfo{new_addr, size};
    addr = new_addr;
  }
  return ge::SUCCESS;
}

bool Om2RTVarManager::TryGetVarAddr(const std::string &key, const uint32_t device_id, void *&addr) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  addr = nullptr;
  auto dev_it = legacy_device_to_vars_.find(device_id);
  if (dev_it == legacy_device_to_vars_.end()) {
    return false;
  }
  auto var_it = dev_it->second.find(key);
  if (var_it == dev_it->second.end()) {
    return false;
  }
  addr = var_it->second.addr;
  return addr != nullptr;
}

Om2RTVarManagerPool &Om2RTVarManagerPool::Instance() {
  static Om2RTVarManagerPool pool;
  return pool;
}

Om2RTVarManagerPool::~Om2RTVarManagerPool() {
  Destroy();
}

Om2RTVarManagerPtr Om2RTVarManagerPool::GetManager(const uint64_t session_id) {
  const std::lock_guard<std::mutex> lock(mutex_);
  auto &manager = session_id_to_manager_[session_id];
  if (manager == nullptr) {
    manager = std::make_shared<Om2RTVarManager>();
    if (manager == nullptr) {
      GELOGE(ge::INTERNAL_ERROR, "[OM2][New][RTVarManager] failed, session_id=%" PRIu64 ".", session_id);
      return nullptr;
    }
  }
  return manager;
}

void Om2RTVarManagerPool::RemoveManager(const uint64_t session_id) {
  Om2RTVarManagerPtr manager = nullptr;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    auto it = session_id_to_manager_.find(session_id);
    if (it != session_id_to_manager_.end()) {
      manager = it->second;
      (void)session_id_to_manager_.erase(it);
    }
  }
  if (manager != nullptr) {
    manager->Finalize();
  }
}

void Om2RTVarManagerPool::Destroy() noexcept {
  const std::lock_guard<std::mutex> lock(mutex_);
  for (const auto &[_, mgr] : session_id_to_manager_) {
    (void)_;
    if (mgr != nullptr) {
      mgr->Finalize();
    }
  }
  session_id_to_manager_.clear();
}

}  // namespace gert
