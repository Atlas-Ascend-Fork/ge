/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef FRAMEWORK_RUNTIME_OM2_DYNAMIC_API_H_
#define FRAMEWORK_RUNTIME_OM2_DYNAMIC_API_H_

#include <cstdint>

namespace gert {
struct GertModelData;
}
struct GertModelLoadConfig;
struct GertModelRunConfig;

// Internal executor/plugin contract. model_data is borrowed through pbody Load;
// rt_session is borrowed until Destroy. The plugin owns the returned context.
struct Om2DynamicCreateInput {
  uint64_t struct_size = sizeof(Om2DynamicCreateInput);
  const gert::GertModelData *model_data = nullptr;
  void *rt_session = nullptr;
  void *external_allocator = nullptr;
  uint64_t session_id = 0U;
  uint32_t device_id = 0U;
  uint32_t runtime_context_valid = 0U;
};
struct Om2DynamicCreateOutput {
  uint64_t struct_size = sizeof(Om2DynamicCreateOutput);
  void *context = nullptr;
  void *external_var_addr = nullptr;
  uint64_t external_var_size = 0;
};

extern "C" {
__attribute__((visibility("default"))) int32_t Om2DynamicCreate(const Om2DynamicCreateInput *input,
                                                                Om2DynamicCreateOutput *output);
// Only Host constant slots are written. Their storage remains owned by context.
__attribute__((visibility("default"))) int32_t Om2DynamicPrepareConstants(void *context, void **constants,
                                                                          uint64_t constant_count);
// Shared-library bytes and strings are borrowed from model_data for the Load call.
__attribute__((visibility("default"))) int32_t Om2DynamicPrepareLoad(void *context, GertModelLoadConfig *config);
__attribute__((visibility("default"))) int32_t Om2DynamicPrepareRun(void *context, void *external_allocator,
                                                                    GertModelRunConfig *config);
// Called after pbody unload; nullptr is accepted.
__attribute__((visibility("default"))) void Om2DynamicDestroy(void *context);
}

#endif  // FRAMEWORK_RUNTIME_OM2_DYNAMIC_API_H_
