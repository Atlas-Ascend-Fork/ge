/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef API_PYTHON_GE_GE_PASSES_NATIVE_BINDINGS_BINDING_LOG_H_
#define API_PYTHON_GE_GE_PASSES_NATIVE_BINDINGS_BINDING_LOG_H_

#include <cinttypes>
#include <cstdint>

#ifdef __GNUC__
#include <unistd.h>
#include <sys/syscall.h>
#else
#include "mmpa/mmpa_api.h"
#endif

#include "base/dlog_pub.h"

// 本模块需要脱离 GE 源码树、仅对着 CANN 包单独编译（多 Python 版本矩阵重编、下发
// 用户机器编译），不能引 GE/metadef 内部的 ge_log.h。此处参照 llm_log.h 的 LLMLOGx
// 直接封装 dlog 公共接口，模块号与落盘格式均与 GELOGx 对齐。
#define GE_PYPASS_MODULE_NAME static_cast<int32_t>(GE)

namespace ge {
namespace python_pass_native {
class BindingLog {
 public:
  static uint64_t GetTid() {
#ifdef __GNUC__
    const uint64_t tid = static_cast<uint64_t>(syscall(__NR_gettid));
#else
    const uint64_t tid = static_cast<uint64_t>(GetCurrentThreadId());
#endif
    return tid;
  }
};
}  // namespace python_pass_native
}  // namespace ge

#define GE_PYPASS_LOGE(fmt, ...)                                                                           \
  do {                                                                                                     \
    dlog_error(GE_PYPASS_MODULE_NAME, "%" PRIu64 " %s:" fmt, ge::python_pass_native::BindingLog::GetTid(), \
               &__FUNCTION__[0U], ##__VA_ARGS__);                                                          \
  } while (false)

#define GE_PYPASS_LOGW(fmt, ...)                                                                          \
  do {                                                                                                    \
    dlog_warn(GE_PYPASS_MODULE_NAME, "%" PRIu64 " %s:" fmt, ge::python_pass_native::BindingLog::GetTid(), \
              &__FUNCTION__[0U], ##__VA_ARGS__);                                                          \
  } while (false)

#define GE_PYPASS_LOGI(fmt, ...)                                                                          \
  do {                                                                                                    \
    dlog_info(GE_PYPASS_MODULE_NAME, "%" PRIu64 " %s:" fmt, ge::python_pass_native::BindingLog::GetTid(), \
              &__FUNCTION__[0U], ##__VA_ARGS__);                                                          \
  } while (false)

#define GE_PYPASS_LOGD(fmt, ...)                                                                           \
  do {                                                                                                     \
    dlog_debug(GE_PYPASS_MODULE_NAME, "%" PRIu64 " %s:" fmt, ge::python_pass_native::BindingLog::GetTid(), \
               &__FUNCTION__[0U], ##__VA_ARGS__);                                                          \
  } while (false)

#endif  // API_PYTHON_GE_GE_PASSES_NATIVE_BINDINGS_BINDING_LOG_H_
