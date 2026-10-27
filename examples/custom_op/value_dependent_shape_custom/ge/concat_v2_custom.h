/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef EXAMPLES_CUSTOM_OP_VALUE_DEPENDENT_SHAPE_CUSTOM_GE_CONCAT_V2_CUSTOM_H_
#define EXAMPLES_CUSTOM_OP_VALUE_DEPENDENT_SHAPE_CUSTOM_GE_CONCAT_V2_CUSTOM_H_

#include "graph/operator_reg.h"

namespace ge {
// 值依赖算子：拼接轴由 concat_dim 输入的值决定，输出 shape 依赖该值。
// proto 结构对齐 ops-math 生产算子 ConcatV2（DYNAMIC_INPUT x + 标量 concat_dim + ATTR N）。
REG_OP(ConcatV2Custom)
    .DYNAMIC_INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32}))
    .INPUT(concat_dim, TensorType({DT_INT32, DT_INT64}))
    .OUTPUT(y, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32}))
    .ATTR(N, Int, 2)
    .OP_END_FACTORY_REG(ConcatV2Custom)
}  // namespace ge

#endif  // EXAMPLES_CUSTOM_OP_VALUE_DEPENDENT_SHAPE_CUSTOM_GE_CONCAT_V2_CUSTOM_H_
