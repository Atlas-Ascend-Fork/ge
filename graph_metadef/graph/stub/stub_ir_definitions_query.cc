/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <cstdint>
#include <utility>
#include <vector>

// GE_API_TYPES_DEF 跳过 ge_common_api_types.h 中 ir_option 的静态字符串表，
// 否则本桩 TU 会膨胀到 31 KB，请勿删除。
namespace ge {
using Status = uint32_t;
}
#define GE_API_TYPES_DEF

#include "graph/utils/ir_definitions_query.h"

// 返回 FAILED 让调用方跳过 IR 快照，返回 SUCCESS 会让它拿空 vector 打包出无效快照。
// 不用 ge::FAILED 常量是因为 include ge_api_error_codes.h 会带出 weak 的 RegisterErrorNo。
extern "C" ge::Status GetRegisteredIrDefFromGraph(const char *op_type,
                                                  std::vector<std::pair<ge::AscendString, ge::AscendString>> &inputs,
                                                  std::vector<std::pair<ge::AscendString, ge::AscendString>> &outputs,
                                                  std::vector<std::pair<ge::AscendString, ge::AscendString>> &attrs) {
  return static_cast<ge::Status>(-1);
}
