/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "dispatcher_table.h"

#include <cerrno>
#include <sys/syscall.h>
#include <unistd.h>
#include <string>
#include "mmpa/mmpa_api.h"
#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "common/compile_profiling/ge_call_wrapper.h"

namespace ge {
namespace autofuse_dispatcher {
namespace {
constexpr char const *kGuardCheckSoName = "libguard_check.so";
}  // namespace

DispatcherTable::~DispatcherTable() {
  CloseGuardSo();
}

void DispatcherTable::CloseGuardSo() {
  if (guard_so_handle_ != nullptr) {
    (void)mmDlclose(guard_so_handle_);
    guard_so_handle_ = nullptr;
  }
  func_ = nullptr;
  if (file_handle_ != -1) {
    (void)close(file_handle_);
    file_handle_ = -1;
  }
}

Status DispatcherTable::LoadGuardSo(const std::string &guard_so_data) {
  CloseGuardSo();  // 保证可重复调用：先清理上一次加载的句柄
  if (guard_so_data.empty()) {
    return SUCCESS;  // 未启用
  }
  file_handle_ = static_cast<int32_t>(syscall(__NR_memfd_create, kGuardCheckSoName, 0));
  if (file_handle_ < 0) {
    GELOGE(FAILED, "memfd_create for guard_check.so failed, errno: %d.", errno);
    return FAILED;
  }
  const auto write_count = mmWrite(file_handle_, const_cast<char_t *>(guard_so_data.c_str()), guard_so_data.size());
  if ((write_count == EN_INVALID_PARAM) || (write_count == EN_ERROR)) {
    GELOGE(FAILED, "write guard so failed.");
    CloseGuardSo();
    return FAILED;
  }
  (void)lseek(static_cast<int32_t>(file_handle_), 0, SEEK_SET);
  const std::string so_path = "/proc/self/fd/" + std::to_string(file_handle_);
  guard_so_handle_ = mmDlopen(so_path.c_str(), static_cast<int32_t>(MMPA_RTLD_NOW));
  if (guard_so_handle_ == nullptr) {
    GELOGE(FAILED, "dlopen guard check so failed, so_path=%s.", so_path.c_str());
    CloseGuardSo();
    return FAILED;
  }
  func_ = reinterpret_cast<GuardCheckFuncPtr>(mmDlsym(guard_so_handle_, "GuardCheckFunc"));
  if (func_ == nullptr) {
    GELOGE(FAILED, "dlsym GuardCheckFunc failed, so_path=%s.", so_path.c_str());
    CloseGuardSo();
    return FAILED;
  }
  GELOGI("load guard check so success, size=%zu, so_path=%s.", guard_so_data.size(), so_path.c_str());
  return SUCCESS;
}

bool DispatcherTable::Match(const std::vector<gert::Tensor *> &inputs, char_t *reason, size_t reason_size) const {
  if (func_ == nullptr) {
    return false;
  }
  GE_TIMESTAMP_START(GuardMatch);
  const bool hit = func_(const_cast<gert::Tensor **>(inputs.data()), inputs.size(), reason, reason_size);
  GE_TIMESTAMP_END(GuardMatch, "GuardMatch");
  if (hit) {
    GELOGI("guard match hit, take optimized body, num_inputs=%zu.", inputs.size());
  } else {
    GELOGI("guard match miss, take fallback body, reason: %s", reason);
  }
  return hit;
}

}  // namespace autofuse_dispatcher
}  // namespace ge
