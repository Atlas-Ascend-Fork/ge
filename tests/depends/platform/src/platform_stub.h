/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef TESTS_DEPENDS_PLATFORM_SRC_PLATFORM_STUB_H
#define TESTS_DEPENDS_PLATFORM_SRC_PLATFORM_STUB_H

#include <string>

// 设置 platform stub 返回的 NpuArch，供 UT 覆盖芯片形态分支
void SetPlatformNpuArch(const std::string &npu_arch);

#endif  // TESTS_DEPENDS_PLATFORM_SRC_PLATFORM_STUB_H
