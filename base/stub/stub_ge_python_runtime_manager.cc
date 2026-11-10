/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <cstdint>

// GE_API_TYPES_DEF 跳过 ge_common_api_types.h 中 ir_option 的静态字符串表，
// 否则本桩 TU 会膨胀约 8.6 KB 顶穿 tiny 对 stub/libge_common.so 的体积限额，请勿删除。
namespace ge {
using Status = uint32_t;
}
#define GE_API_TYPES_DEF

#include "common/python_runtime/ge_python_runtime_manager.h"

// 桩版 libge_common.so 不承载 Python 运行时，仅为让依赖桩库的形态通过装载期符号解析。
// EnsureReady 返回 FAILED：桩形态无解释器可附着，明确失败优于假装就绪。
namespace ge {
GePythonRuntimeManager &GePythonRuntimeManager::Instance() {
  static GePythonRuntimeManager instance;
  return instance;
}

Status GePythonRuntimeManager::EnsureReady() {
  return FAILED;
}

Status GePythonRuntimeManager::ShutdownProcess() {
  return SUCCESS;
}

void GePythonRuntimeManager::SetInterpreterFinalizer(void (*finalizer)()) {}
}  // namespace ge
