/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_COMMON_OM2_MODEL_API_UTILS_H_
#define GE_COMMON_OM2_MODEL_API_UTILS_H_

#include "framework/om2/model_api/om2_model_api.h"

namespace gert {
namespace om2 {
inline bool IsValidSharedLibrary(const GertModelSharedLibrary &value) {
  if (value.struct_size < offsetof(GertModelSharedLibrary, data_size) + sizeof(value.data_size)) {
    return false;
  }
  return value.so_name != nullptr && value.so_name[0] != '\0' &&
         value.so_bin_type <= GERT_MODEL_SHARED_LIBRARY_CUSTOM_OP && value.data != nullptr && value.data_size > 0U;
}

inline bool IsValidSharedLibraries(const GertModelSharedLibraries &value) {
  if (value.struct_size < offsetof(GertModelSharedLibraries, libraries) + sizeof(value.libraries)) {
    return false;
  }
  if (value.library_num == 0U) {
    return true;
  }
  if (value.libraries == nullptr) {
    return false;
  }
  for (uint64_t i = 0U; i < value.library_num; ++i) {
    if (value.libraries[i] == nullptr || !IsValidSharedLibrary(*value.libraries[i])) {
      return false;
    }
  }
  return true;
}

inline const GertModelDynamicLoadConfig *GetDynamicConfig(const GertModelLoadConfig &value) {
  if (value.struct_size < offsetof(GertModelLoadConfig, dynamic_config) + sizeof(value.dynamic_config)) {
    return nullptr;
  }
  return value.dynamic_config;
}

inline const GertModelSharedLibraries *GetSharedLibraries(const GertModelLoadConfig &value) {
  const auto *dynamic = GetDynamicConfig(value);
  if (dynamic == nullptr || dynamic->struct_size < offsetof(GertModelDynamicLoadConfig, shared_libraries) +
                                                       sizeof(dynamic->shared_libraries)) {
    return nullptr;
  }
  return dynamic->shared_libraries;
}

inline void *GetRtSession(const GertModelLoadConfig &value) {
  const auto *dynamic = GetDynamicConfig(value);
  if (dynamic == nullptr ||
      dynamic->struct_size < offsetof(GertModelDynamicLoadConfig, rt_session) + sizeof(dynamic->rt_session)) {
    return nullptr;
  }
  return dynamic->rt_session;
}

inline void *GetExternalAllocator(const GertModelLoadConfig &value) {
  if (value.struct_size < offsetof(GertModelLoadConfig, external_allocator) + sizeof(value.external_allocator)) {
    return nullptr;
  }
  return value.external_allocator;
}

inline void *GetExternalAllocator(const GertModelRunConfig &value) {
  if (value.struct_size < offsetof(GertModelRunConfig, external_allocator) + sizeof(value.external_allocator)) {
    return nullptr;
  }
  return value.external_allocator;
}
}  // namespace om2
}  // namespace gert

#endif  // GE_COMMON_OM2_MODEL_API_UTILS_H_
