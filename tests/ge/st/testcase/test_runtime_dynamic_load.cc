/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <dlfcn.h>
#include <memory>
#include <gtest/gtest.h>
#include "depends/runtime/src/runtime_stub.h"

namespace ge {
// Python ctypes uses the same dlopen/dlsym path. It must see the linked Runtime stub.
TEST(STEST_runtime_dynamic_load, SocSpecUsesLinkedStub) {
  const std::unique_ptr<void, decltype(&dlclose)> library(dlopen("libruntime.so", RTLD_NOW), &dlclose);
  ASSERT_NE(library, nullptr) << dlerror();
  const auto get_soc_spec = reinterpret_cast<decltype(&rtGetSocSpec)>(dlsym(library.get(), "rtGetSocSpec"));
  ASSERT_EQ(get_soc_spec, &rtGetSocSpec);
  char value[32] = {};
  ASSERT_EQ(get_soc_spec("SoCInfo", "ai_core_cnt", value, sizeof(value)), RT_ERROR_NONE);
  EXPECT_STREQ(value, "24");
  ASSERT_EQ(get_soc_spec("SoCInfo", "vector_core_cnt", value, sizeof(value)), RT_ERROR_NONE);
  EXPECT_STREQ(value, "48");
}
}  // namespace ge
