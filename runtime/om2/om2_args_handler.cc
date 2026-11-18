/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_args_handler.h"

#include <new>

#include "ge_common/debug/ge_log.h"
#include "securec.h"

namespace ge {
const gert::KernelArgs *Om2ArgsHandler::MallocReadOnlyDevArgs(void *host_args, size_t args_size) {
  if ((host_args == nullptr) || (args_size == 0U) || (allocate_args_buffer_ == nullptr)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[OM2][ArgsHandler] invalid param, host_args=%p, args_size=%zu.", host_args,
           args_size);
    return nullptr;
  }
  void *host_addr = nullptr;
  void *device_addr = nullptr;
  const auto ret = allocate_args_buffer_(args_table_, static_cast<uint64_t>(args_size), &host_addr, &device_addr);
  if (ret != ACL_SUCCESS) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][ArgsHandler] allocate args buffer failed, ret=%d, size=%zu.", ret,
           args_size);
    return nullptr;
  }
  if ((host_addr == nullptr) || (device_addr == nullptr)) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][ArgsHandler] allocate returned null, host=%p, device=%p.", host_addr,
           device_addr);
    return nullptr;
  }
  // 把算子私有参数拷入池的 host 子区间。device 侧内容不在此上传：模型 SO 每轮按 reserved 游标
  // 统一 H2D，避免逐算子下发。
  if (memcpy_s(host_addr, args_size, host_args, args_size) != EOK) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][ArgsHandler] copy host args failed, size=%zu.", args_size);
    return nullptr;
  }

  gert::KernelArgs host_entry;
  host_entry.args_data = host_addr;
  host_entry.args_size = args_size;
  host_entry.placement = kPlacementHost;
  gert::KernelArgs device_entry;
  device_entry.args_data = device_addr;
  device_entry.args_size = args_size;
  device_entry.placement = kPlacementDevice;

  // 两端按注册序一一对应；deque push_back 不失效已有元素指针，故可把元素地址交给算子长期持有。
  (void)host_kernel_args_.emplace_back(host_entry);
  return &device_kernel_args_.emplace_back(device_entry);
}

const std::deque<gert::KernelArgs> &Om2ArgsHandler::GetKernelArgs(gert::Placement placement) const {
  return (placement == kPlacementDevice) ? device_kernel_args_ : host_kernel_args_;
}
}  // namespace ge

int32_t GertOm2ArgsHandlerCreate(void *args_table, GertOm2ArgsAllocFunc allocate_args_buffer, void **handler) {
  if (handler == nullptr) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[OM2][ArgsHandler] create failed, handler out param is null.");
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  *handler = nullptr;
  if ((args_table == nullptr) || (allocate_args_buffer == nullptr)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[OM2][ArgsHandler] create failed, args_table=%p, allocate_cb=%p.", args_table,
           allocate_args_buffer);
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  auto *impl = new (std::nothrow) ge::Om2ArgsHandler(args_table, allocate_args_buffer);
  if (impl == nullptr) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][ArgsHandler] create failed, out of memory.");
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  // 句柄以基类地址形式暴露：SO 侧只经 gert::ArgsHandler 接口访问，不依赖派生类布局。
  *handler = static_cast<void *>(static_cast<gert::ArgsHandler *>(impl));
  return ACL_SUCCESS;
}

void GertOm2ArgsHandlerDestroy(void *handler) {
  if (handler == nullptr) {
    return;
  }
  // 经基类指针删除：gert::ArgsHandler 有虚析构，可正确销毁派生类。
  delete static_cast<gert::ArgsHandler *>(handler);
}
