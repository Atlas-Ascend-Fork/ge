/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef FRAMEWORK_RUNTIME_OM2_DYNAMIC_LOADER_H_
#define FRAMEWORK_RUNTIME_OM2_DYNAMIC_LOADER_H_

#include <mutex>
#include "dynamic/om2_dynamic_api.h"
#include "framework/common/ge_types.h"

namespace gert {
class Om2DynamicApi {
 public:
  static Om2DynamicApi &Instance();
  ge::Status Create(const Om2DynamicCreateInput &input, Om2DynamicCreateOutput &output);
  ge::Status PrepareConstants(void *context, void **constants, uint64_t constant_count);
  ge::Status PrepareLoad(void *context, GertModelLoadConfig &config);
  ge::Status PrepareRun(void *context, void *external_allocator, GertModelRunConfig &config);
  void Destroy(void *context);

 private:
  void Initialize();
  std::once_flag once_;
  // Successful loads remain mapped for process lifetime, including allocator teardown.
  void *so_handle_ = nullptr;
  decltype(&Om2DynamicCreate) create_ = nullptr;
  decltype(&Om2DynamicPrepareConstants) prepare_constants_ = nullptr;
  decltype(&Om2DynamicPrepareLoad) prepare_load_ = nullptr;
  decltype(&Om2DynamicPrepareRun) prepare_run_ = nullptr;
  decltype(&Om2DynamicDestroy) destroy_ = nullptr;
};
}  // namespace gert

#endif  // FRAMEWORK_RUNTIME_OM2_DYNAMIC_LOADER_H_
