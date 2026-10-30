/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "aicpu_bin_handler.h"

#include "common/checker.h"
#include "graph/error_codes.h"
#include "framework/common/debug/log.h"
#include "framework/common/debug/ge_log.h"
#include "rt_external_device.h"
#include "common/ge_rts_decl.h"

using namespace ge;

namespace gert {
namespace {
constexpr size_t kSocVersionLen = 128U;
constexpr int32_t kCpuKernelModeJson = 0;
constexpr int32_t kCpuKernelModeData = 2;

std::string GenerateCustBinKey(const std::string &so_name, const ge::OpKernelBinPtr &kernel_bin) {
  const auto binary_data = reinterpret_cast<const char *>(kernel_bin->GetBinData());
  const size_t hash_id = std::hash<std::string>{}(std::string(binary_data, kernel_bin->GetBinDataSize()));
  return so_name + "_" + std::to_string(hash_id) + "_CustAicpuKernel";
}
}  // namespace

bool OpJsonBinHandler::is_support_ = true;
std::once_flag OpJsonBinHandler::init_flag_;

bool OpJsonBinHandler::IsSupportBinHandle() {
  std::call_once(init_flag_, []() {
    char soc_version[kSocVersionLen] = {0};
    if (rtGetSocVersion(soc_version, kSocVersionLen) != ge::GRAPH_SUCCESS) {
      GELOGE(ge::FAILED, "Get soc version failed.");
      return;
    }

    if (strncmp(soc_version, "Ascend910_96", (sizeof("Ascend910_96") - 1UL)) == 0) {
      is_support_ = false;
    }
    GELOGI("Init bin handle support status, val=%u, socVersion=%s", static_cast<uint32_t>(is_support_), soc_version);
  });
  return is_support_;
}

ge::graphStatus OpJsonBinHandler::LoadBinary(const std::string &json_path) {
  std::unique_lock lock(bin_mutex_);
  if (bin_handle_ != nullptr) {
    GELOGI("Load binary from json success, no need to load again. json_path=%s", json_path.c_str());
    return ge::GRAPH_SUCCESS;
  }

  if (json_path.empty()) {
    GELOGE(ge::FAILED, "Load binary failed because json path is empty.");
    return ge::FAILED;
  }

  aclrtBinaryLoadOption bin_option = {};
  bin_option.type = ACL_RT_BINARY_LOAD_OPT_CPU_KERNEL_MODE;
  bin_option.value.cpuKernelMode = kCpuKernelModeJson;
  aclrtBinaryLoadOptions load_bin_cfg = {&bin_option, 1U};
  GE_ASSERT_SUCCESS(aclrtBinaryLoadFromFile(json_path.c_str(), &load_bin_cfg, &bin_handle_));
  GE_ASSERT_NOTNULL(bin_handle_);

  GELOGI("Load json binary success for %s.", json_path.c_str());

  return ge::GRAPH_SUCCESS;
}

ge::graphStatus OpDataBinHandler::LoadBinary(const std::string &so_name, const ge::OpKernelBinPtr &kernel_bin) {
  std::unique_lock lock(bin_mutex_);
  if (bin_handle_ != nullptr) {
    GELOGI("Load binary from data success, no need to load again. so=%s", so_name.c_str());
    return ge::GRAPH_SUCCESS;
  }

  if (kernel_bin == nullptr) {
    GELOGE(ge::FAILED, "Load custom aicpu binary failed because bin is nullptr. so=%s", so_name.c_str());
    return ge::FAILED;
  }

  if (so_name.empty()) {
    GELOGE(ge::FAILED, "Load custom aicpu binary failed because so name is empty.");
    return ge::FAILED;
  }

  aclrtBinaryLoadOption bin_option = {};
  bin_option.type = ACL_RT_BINARY_LOAD_OPT_CPU_KERNEL_MODE;
  bin_option.value.cpuKernelMode = kCpuKernelModeData;
  const aclrtBinaryLoadOptions load_bin_cfg = {&bin_option, 1U};
  GE_ASSERT_SUCCESS(
      aclrtBinaryLoadFromData(kernel_bin->GetBinData(), kernel_bin->GetBinDataSize(), &load_bin_cfg, &bin_handle_));
  GE_ASSERT_NOTNULL(bin_handle_);

  GELOGI("Load data binary success for %s.", so_name.c_str());

  return ge::GRAPH_SUCCESS;
}

ge::graphStatus OpDataBinHandler::UnloadBinary() {
  std::unique_lock lock(bin_mutex_);
  if (bin_handle_ == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  GE_CHK_ACL_RET(aclrtBinaryUnLoad(bin_handle_));
  bin_handle_ = nullptr;
  return ge::GRAPH_SUCCESS;
}

TfJsonBinHandler &TfJsonBinHandler::Instance() {
  static TfJsonBinHandler inst;
  return inst;
}

AicpuJsonBinHandler &AicpuJsonBinHandler::Instance() {
  static AicpuJsonBinHandler inst;
  return inst;
}

CustBinHandlerManager &CustBinHandlerManager::Instance() {
  static CustBinHandlerManager inst;
  return inst;
}

CustAicpuBinLease::~CustAicpuBinLease() {
  const auto ret = Reset();
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "Failed to release custom aicpu binary lease during destruction.");
  }
}

ge::graphStatus CustAicpuBinLease::Acquire(const std::string &so_name, const ge::OpKernelBinPtr &kernel_bin,
                                           rtBinHandle &handle) {
  GE_ASSERT_SUCCESS(Reset());
  GE_CHK_ACL_RET(aclrtGetCurrentContext(&owner_context_));
  return CustBinHandlerManager::Instance().LoadAndGetBinHandle(so_name, kernel_bin, handle, resource_token_);
}

ge::graphStatus CustAicpuBinLease::Reset() {
  if (resource_token_ == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  aclrtContext current_context = nullptr;
  const auto context_ret = aclrtGetCurrentContext(&current_context);
  if (context_ret == ACL_ERROR_RT_CONTEXT_NULL) {
    // A thread without a context can still release in the owner's context.
    current_context = nullptr;
  } else {
    GE_CHK_ACL_RET(context_ret);
  }
  const bool switch_context = current_context != owner_context_;
  if (switch_context) {
    GE_CHK_ACL_RET(aclrtSetCurrentContext(owner_context_));
  }
  const auto ret = CustBinHandlerManager::Instance().ReleaseBinHandle(resource_token_);
  if (ret == ge::GRAPH_SUCCESS) {
    // Release has committed even if restoring the caller's context fails.
    resource_token_ = nullptr;
    owner_context_ = nullptr;
  }
  // ACL cannot clear a context binding; an initially unbound caller keeps the owner's context.
  if (switch_context && (current_context != nullptr)) {
    const auto restore_ret = aclrtSetCurrentContext(current_context);
    if (restore_ret != ACL_SUCCESS) {
      GELOGE(ge::FAILED, "Restore context after custom aicpu binary release failed, ret=%d.", restore_ret);
      return ret == ge::GRAPH_SUCCESS ? ge::GRAPH_FAILED : ret;
    }
  }
  return ret;
}

ge::graphStatus CustBinHandlerManager::LoadAndGetBinHandle(const std::string &so_name,
                                                           const ge::OpKernelBinPtr &kernel_bin, rtBinHandle &handle,
                                                           void *&resource_token) {
  handle = nullptr;
  resource_token = nullptr;
  if ((so_name.empty()) || (kernel_bin == nullptr)) {
    GELOGE(ge::FAILED, "Load bin failed because so name is empty or bin is nullptr, so_name=%s", so_name.c_str());
    return ge::FAILED;
  }

  aclrtContext current_ctx = nullptr;
  GE_CHK_ACL_RET(aclrtGetCurrentContext(&current_ctx));
  const uintptr_t resource_id = reinterpret_cast<uintptr_t>(current_ctx);
  const std::string bin_key = GenerateCustBinKey(so_name, kernel_bin);

  const std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto &so_bin_maps = bin_manager_.find(resource_id);
  if (so_bin_maps == bin_manager_.end()) {
    OpDataBinHandlerPtr bin_handler = std::make_shared<OpDataBinHandler>();
    GE_ASSERT_NOTNULL(bin_handler);
    GE_ASSERT_SUCCESS(bin_handler->LoadBinary(so_name, kernel_bin));
    bin_manager_.emplace(resource_id,
                         std::unordered_map<std::string, BinResource>{{bin_key, BinResource{bin_handler, 1U}}});
    handle = bin_handler->GetBinHandle();
    resource_token = &bin_manager_.at(resource_id).at(bin_key);
    GELOGI("Add resource and bin handle success, resource_id=%lu, so=%s, size=%lu", resource_id, so_name.c_str(),
           bin_manager_.size());
    return ge::GRAPH_SUCCESS;
  }

  const auto &iter = so_bin_maps->second.find(bin_key);
  if (iter == so_bin_maps->second.end()) {
    OpDataBinHandlerPtr bin_handler = std::make_shared<OpDataBinHandler>();
    GE_ASSERT_NOTNULL(bin_handler);
    GE_ASSERT_SUCCESS(bin_handler->LoadBinary(so_name, kernel_bin));
    so_bin_maps->second.emplace(bin_key, BinResource{bin_handler, 1U});
    handle = bin_handler->GetBinHandle();
    resource_token = &so_bin_maps->second.at(bin_key);
    GELOGI("Add bin handle for so %s success, resource_id=%lu, size=%lu", so_name.c_str(), resource_id,
           so_bin_maps->second.size());
    return ge::GRAPH_SUCCESS;
  }

  GELOGI("Bin handle for so %s already exists, will not repeat load. resource_id=%lu", so_name.c_str(), resource_id);
  iter->second.ref_count++;
  handle = iter->second.bin_handler->GetBinHandle();
  resource_token = &iter->second;

  return ge::GRAPH_SUCCESS;
}

ge::graphStatus CustBinHandlerManager::ReleaseBinHandle(void *const resource_token) {
  if (resource_token == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  aclrtContext current_ctx = nullptr;
  GE_CHK_ACL_RET(aclrtGetCurrentContext(&current_ctx));
  const uintptr_t current_resource_id = reinterpret_cast<uintptr_t>(current_ctx);
  const std::lock_guard<std::recursive_mutex> lk(mutex_);
  for (auto ctx_iter = bin_manager_.begin(); ctx_iter != bin_manager_.end(); ++ctx_iter) {
    for (auto bin_iter = ctx_iter->second.begin(); bin_iter != ctx_iter->second.end(); ++bin_iter) {
      auto &resource = bin_iter->second;
      if (&resource == resource_token) {
        GE_ASSERT_TRUE(current_resource_id == ctx_iter->first,
                       "Current context does not match custom aicpu binary owner, current=%p, owner=%p", current_ctx,
                       reinterpret_cast<void *>(ctx_iter->first));
        GE_ASSERT_TRUE(resource.ref_count > 0U);
        if (resource.ref_count == 1U) {
          GE_ASSERT_SUCCESS(resource.bin_handler->UnloadBinary());
          (void)ctx_iter->second.erase(bin_iter);
          if (ctx_iter->second.empty()) {
            (void)bin_manager_.erase(ctx_iter);
          }
        } else {
          resource.ref_count--;
        }
        return ge::GRAPH_SUCCESS;
      }
    }
  }
  GELOGW("Cannot find cust aicpu resource token %p when releasing, resource_id=%lu.", resource_token,
         current_resource_id);
  return ge::GRAPH_SUCCESS;
}

}  // namespace gert
