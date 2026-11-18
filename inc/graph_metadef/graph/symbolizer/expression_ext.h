/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GRAPH_SYMBOLIZER_EXPRESSION_EXT_H_
#define GRAPH_SYMBOLIZER_EXPRESSION_EXT_H_

#include "graph/symbolizer/symbolic.h"

namespace ge {
namespace expr_ext {
/**
 * @brief 将字符串转换成表达式（宽松解析，与拆分旧头原 Expression::Parse 语义一致，
 *        不校验字符串是否为序列化接口产出，容忍解析截断）
 * @param str 表达式字符串
 * @return 解析出的表达式，str为nullptr或解析失败时返回非法表达式
 */
Expression Parse(const char_t *str);
}  // namespace expr_ext
}  // namespace ge

#endif  // GRAPH_SYMBOLIZER_EXPRESSION_EXT_H_
