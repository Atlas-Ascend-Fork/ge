/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_STAGE_STAGE_TYPES_H_
#define METADEF_GRAPH_STAGE_STAGE_TYPES_H_

#include <cstdint>
#include <string>

namespace ge {
// 执行阶段。数值顺序即生命周期顺序，释放阶段不得早于分配阶段。
enum class StageKind : uint32_t { kInit = 0, kMain = 1, kDeInit = 2 };

inline const char *StageKindToString(StageKind stage) {
  switch (stage) {
    case StageKind::kInit:
      return "init";
    case StageKind::kMain:
      return "main";
    case StageKind::kDeInit:
      return "deinit";
    default:
      return "unknown";
  }
}

inline bool ParseStageKind(const std::string &text, StageKind &stage) {
  if (text == "init") {
    stage = StageKind::kInit;
    return true;
  }
  if (text == "main") {
    stage = StageKind::kMain;
    return true;
  }
  if (text == "deinit") {
    stage = StageKind::kDeInit;
    return true;
  }
  return false;
}

// 阶段来源：显式属性、内置映射、默认 Main、前驱闭包提升、释放配对。
enum class StageSource : uint32_t { kDefault = 0, kAttribute = 1, kBuiltin = 2, kClosure = 3, kRelease = 4 };

}  // namespace ge

#endif  // METADEF_GRAPH_STAGE_STAGE_TYPES_H_
