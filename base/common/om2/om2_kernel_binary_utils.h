/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_OM2_KERNEL_BINARY_UTILS_H_
#define AIR_CXX_BASE_COMMON_OM2_OM2_KERNEL_BINARY_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace gert {
namespace om2 {

inline std::string GetKernelBinaryKey(const void *data, const size_t size) {
  constexpr uint64_t kOffsetBasis = 1469598103934665603ULL;
  constexpr uint64_t kPrime = 1099511628211ULL;
  uint64_t hash = kOffsetBasis;
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0U; i < size; ++i) {
    hash ^= bytes[i];
    hash *= kPrime;
  }
  return std::to_string(hash);
}

}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_OM2_KERNEL_BINARY_UTILS_H_
