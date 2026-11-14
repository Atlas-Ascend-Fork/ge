/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_STAGE_BUILTIN_STAGE_TABLE_H_
#define METADEF_GRAPH_STAGE_BUILTIN_STAGE_TABLE_H_

#include <string>
#include "stage/stage_types.h"

namespace ge {
// 内置阶段表：只收录阶段语义确定、且能在分析文档与 Kernel 阶段工作簿中找到证据的类型。
// 通用 Alloc/Free 不在此表内：同一 Kernel 既能承担加载分配也能承担逐次分配，必须由调用方声明或由配对分析决定。
class BuiltinStageTable {
 public:
  // 返回 false 表示该类型没有确定的固定阶段，交给属性与默认规则处理。
  static bool Lookup(const std::string &node_type, StageKind &stage);
  static bool IsBuiltinStageType(const std::string &node_type);
  // 该类型是否属于分配类（用于资源配对分析）。
  static bool IsAllocType(const std::string &node_type);
  // 该类型是否属于释放类。
  static bool IsFreeType(const std::string &node_type);
  // 该类型是否为按次执行的任务下发（不得前置到加载）。
  static bool IsExecutionType(const std::string &node_type);
};
}  // namespace ge

#endif  // METADEF_GRAPH_STAGE_BUILTIN_STAGE_TABLE_H_
