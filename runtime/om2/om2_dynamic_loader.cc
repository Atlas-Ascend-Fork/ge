/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_dynamic_loader.h"

#include <cstdlib>
#include <dlfcn.h>
#include <limits.h>
#include <string>
#include "framework/common/debug/ge_log.h"

namespace gert {
namespace {
__attribute__((noinline)) void *DynamicLoaderSelfPtr() {
  return reinterpret_cast<void *>(&DynamicLoaderSelfPtr);
}

std::string ResolveDynamicPath() {
  Dl_info info{};
  if ((dladdr(DynamicLoaderSelfPtr(), &info) == 0) || (info.dli_fname == nullptr)) {
    return {};
  }
  char absolute_path[PATH_MAX] = {0};
  const std::string self = realpath(info.dli_fname, absolute_path) == nullptr ? info.dli_fname : absolute_path;
  const auto slash = self.find_last_of('/');
  return slash == std::string::npos ? std::string() : self.substr(0U, slash + 1U) + "libom2_dynamic.so";
}
}  // namespace

Om2DynamicApi &Om2DynamicApi::Instance() {
  static Om2DynamicApi api;
  return api;
}

void Om2DynamicApi::Initialize() {
  const auto path = ResolveDynamicPath();
  if (path.empty()) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] Cannot locate executor directory.");
    return;
  }
  void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle == nullptr) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] dlopen %s failed: %s", path.c_str(), dlerror());
    return;
  }
  const auto create = reinterpret_cast<decltype(create_)>(dlsym(handle, "Om2DynamicCreate"));
  const auto constants = reinterpret_cast<decltype(prepare_constants_)>(dlsym(handle, "Om2DynamicPrepareConstants"));
  const auto load = reinterpret_cast<decltype(prepare_load_)>(dlsym(handle, "Om2DynamicPrepareLoad"));
  const auto run = reinterpret_cast<decltype(prepare_run_)>(dlsym(handle, "Om2DynamicPrepareRun"));
  const auto destroy = reinterpret_cast<decltype(destroy_)>(dlsym(handle, "Om2DynamicDestroy"));
  if ((create == nullptr) || (constants == nullptr) || (load == nullptr) || (run == nullptr) || (destroy == nullptr)) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] Missing required API in %s.", path.c_str());
    (void)dlclose(handle);
    return;
  }
  create_ = create;
  prepare_constants_ = constants;
  prepare_load_ = load;
  prepare_run_ = run;
  destroy_ = destroy;
  so_handle_ = handle;
}

ge::Status Om2DynamicApi::Create(const Om2DynamicCreateInput &input, Om2DynamicCreateOutput &output) {
  try {
    std::call_once(once_, [this]() { Initialize(); });
    return create_ == nullptr ? ge::FAILED : static_cast<ge::Status>(create_(&input, &output));
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] Loader initialization raised an exception.");
    return ge::FAILED;
  }
}

ge::Status Om2DynamicApi::PrepareConstants(void *context, void **constants, uint64_t constant_count) {
  return prepare_constants_ == nullptr
             ? ge::FAILED
             : static_cast<ge::Status>(prepare_constants_(context, constants, constant_count));
}

ge::Status Om2DynamicApi::PrepareLoad(void *context, GertModelLoadConfig &config) {
  return prepare_load_ == nullptr ? ge::FAILED : static_cast<ge::Status>(prepare_load_(context, &config));
}

ge::Status Om2DynamicApi::PrepareRun(void *context, void *external_allocator, GertModelRunConfig &config) {
  return prepare_run_ == nullptr ? ge::FAILED
                                 : static_cast<ge::Status>(prepare_run_(context, external_allocator, &config));
}

void Om2DynamicApi::Destroy(void *context) {
  if ((context != nullptr) && (destroy_ != nullptr)) {
    destroy_(context);
  }
}
}  // namespace gert
