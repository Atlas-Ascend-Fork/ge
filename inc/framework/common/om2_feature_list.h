/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software: you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef INC_FRAMEWORK_COMMON_OM2_FEATURE_LIST_H_
#define INC_FRAMEWORK_COMMON_OM2_FEATURE_LIST_H_

#include <array>
#include <cstddef>
#include <cstdint>

namespace gert {

/* OM2中商用版本开始不兼容变更的特性清单（Feature List） */
// 特性新增、发生不兼容变更时，需要对4处声明同时修改提交四步必须同一次提交完成（否则编译期断言或看护用例会失败）：
//   1) 在下方清单追加条目，版本值取该能力上线时的 GERT_EXECUTOR_VERSION，并同步抬升宏的 minor。
//   2) om2_package_contants.h 的 FORALL_OM2_CONSTANTS 追加特性名常量 OM2_FEATURE_XXX；
//   3) 在 Om2FeatureIndex 中新增特性索引，并补充 kOm2FeatureList；
//   4) 编译侧补探测函数并追加到 kOm2DetectByIndex（下标与编号一致）；
enum class Om2FeatureIndex : uint8_t {
  kFeatureIndexMax = 0U  // 计数哨兵，必须保持在最后；新增特性时在其之前追加编号
};

struct Om2FeatureInfo {
  Om2FeatureIndex index;  // 特性编号索引
  const char *name;       // 特性名，取自 om2_package_contants.h 的 OM2_FEATURE_* 常量
  const char *version;    // 该特性要求的执行器版本，在特性新增、发生不兼容变更时根据执行器版本号刷新
};

// 示例 {Om2FeatureIndex::kHcclTask, OM2_FEATURE_HCCL_COMMUNICATION, "1.0"},
inline constexpr std::array<Om2FeatureInfo, 0> kOm2FeatureList{};

// 版本号解析：解析规则为 major * 10000 + minor（1.0=10000、1.10=10010、1.1000=11000）
constexpr uint32_t ParseVersion(const char *version) {
  uint32_t major = 0U;
  uint32_t minor = 0U;
  size_t idx = 0U;
  while ((version[idx] >= '0') && (version[idx] <= '9')) {
    major = (major * 10U) + static_cast<uint32_t>(version[idx] - '0');
    ++idx;
  }
  if (version[idx] == '.') {
    ++idx;
    while ((version[idx] >= '0') && (version[idx] <= '9')) {
      minor = (minor * 10U) + static_cast<uint32_t>(version[idx] - '0');
      ++idx;
    }
  }
  return (major * 10000U) + minor;
}

constexpr uint32_t GetMajorVersion(uint32_t version) {
  return version / 10000U;
}
}  // namespace gert

#endif  // INC_FRAMEWORK_COMMON_OM2_FEATURE_LIST_H_
