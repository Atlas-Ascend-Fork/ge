/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <dlfcn.h>
#include <mutex>
#include "common/debug/ge_log.h"
#include "hccl_util.h"

namespace ge {

HcclDllHcomMgr &HcclDllHcomMgr::GetInstance() {
  static HcclDllHcomMgr mgr;
  return mgr;
}

HcclDllHcomMgr::HcclDllHcomMgr() {
  GELOGI("hccl_HcomGetCcuTaskInfo_func load start.");
  dll_handle = dlopen("libhccl_v2.so", RTLD_LAZY);
  if (dll_handle == nullptr) {
    GELOGI("hccl_HcomGetCcuTaskInfo_func: libhccl_v2.so not available");
    return;
  }
  hccl_HcomGetCcuTaskInfo_func = (HcclResult (*)(const std::string &group, void *tilingData, void *ccuTaskGroup))dlsym(
      dll_handle, "HcomGetCcuTaskInfo");
}

HcclResult HcclDllHcomMgr::HcomGetCcuTaskInfoFunc(const std::string &group, void *tilingData, void *ccuTaskGroup) {
  GELOGI("[HcclDllHcomMgr][HcomGetCcuTaskInfoFunc] group[%s] tilingData[%p] ccuTaskGroup[%p]", group.c_str(),
         tilingData, ccuTaskGroup);
  if (hccl_HcomGetCcuTaskInfo_func == nullptr) {
    GELOGI("hccl_HcomGetCcuTaskInfo_func not found");
    return HCCL_E_PTR;
  }
  return hccl_HcomGetCcuTaskInfo_func(group, tilingData, ccuTaskGroup);
}

HcclResult HcclDllHcomMgr::HcomGetCommHandleByGroupFunc(const std::string &group, void *&comm) {
  GELOGI("[GE_MC2_CCU] query comm by group, group[%s].", group.c_str());
  static std::mutex load_mutex;
  std::unique_lock<std::mutex> lock(load_mutex);
  if (hcom_get_comm_handle_func_ == nullptr) {
    if (hcomm_dll_handle_ == nullptr) {
      hcomm_dll_handle_ = dlopen("libhcomm.so", RTLD_NOW | RTLD_LOCAL);
      if (hcomm_dll_handle_ == nullptr) {
        GELOGE(FAILED, "Failed to load libhcomm.so: %s.", dlerror());
        return HCCL_E_INTERNAL;
      }
    }
    hcom_get_comm_handle_func_ =
        reinterpret_cast<HcclResult (*)(const char *, void **)>(dlsym(hcomm_dll_handle_, "HcomGetCommHandleByGroup"));
    if (hcom_get_comm_handle_func_ == nullptr) {
      GELOGE(FAILED, "Failed to load HcomGetCommHandleByGroup: %s.", dlerror());
      return HCCL_E_INTERNAL;
    }
    GELOGI("[GE_MC2_CCU] resolved HcomGetCommHandleByGroup from libhcomm.so.");
  }
  const auto get_comm_func = hcom_get_comm_handle_func_;
  lock.unlock();
  return get_comm_func(group.c_str(), &comm);
}

HcclResult HcclDllHcomMgr::CcuKernelLaunchFunc(void *comm, void *context) {
  GELOGI("[GE_MC2_CCU] mc2_client launch wrapper enter, comm[%p], context[%p].", comm, context);
  if (comm == nullptr || context == nullptr) {
    GELOGE(FAILED, "mc2_client CCU comm or context is null.");
    return HCCL_E_PTR;
  }
  static std::mutex load_mutex;
  std::unique_lock<std::mutex> lock(load_mutex);
  if (mc2_ccu_kernel_launch_func_ == nullptr) {
    if (mc2_dll_handle_ == nullptr) {
      GELOGI("[GE_MC2_CCU] loading libmc2_client.so for CcuKernelLaunch.");
      mc2_dll_handle_ = dlopen("libmc2_client.so", RTLD_NOW | RTLD_LOCAL);
      if (mc2_dll_handle_ == nullptr) {
        GELOGE(FAILED, "Failed to load libmc2_client.so: %s.", dlerror());
        return HCCL_E_INTERNAL;
      }
    }
    // Match opbase's stable return-value ABI without depending on the CCU internal headers.
    mc2_ccu_kernel_launch_func_ =
        reinterpret_cast<uint32_t (*)(void *, void *)>(dlsym(mc2_dll_handle_, "CcuKernelLaunch"));
    if (mc2_ccu_kernel_launch_func_ == nullptr) {
      GELOGE(FAILED, "Failed to load mc2_client CcuKernelLaunch: %s.", dlerror());
      return HCCL_E_INTERNAL;
    }
    GELOGI("[GE_MC2_CCU] resolved CcuKernelLaunch from libmc2_client.so, handle[%p].", mc2_dll_handle_);
  }
  const auto launch_func = mc2_ccu_kernel_launch_func_;
  lock.unlock();
  GELOGI("[GE_MC2_CCU] calling mc2_client CcuKernelLaunch, comm[%p], context[%p].", comm, context);
  const uint32_t ret = launch_func(comm, context);
  GELOGI("[GE_MC2_CCU] mc2_client CcuKernelLaunch returned, ret[%u], comm[%p], context[%p].", ret, comm, context);
  if (ret != 0U) {
    GELOGE(FAILED, "mc2_client CcuKernelLaunch failed, ret[%u], comm[%p], context[%p].", ret, comm, context);
    return HCCL_E_INTERNAL;
  }
  return HCCL_SUCCESS;
}

}  // namespace ge
