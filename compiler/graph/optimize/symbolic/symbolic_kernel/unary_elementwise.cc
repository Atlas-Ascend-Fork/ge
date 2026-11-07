/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "common/plugin/ge_make_unique_util.h"
#include "framework/common/framework_types_internal.h"
#include "common/util.h"
#include "common/checker.h"
#include "graph/optimize/symbolic/symbol_compute_context.h"
#include "graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"
#include "graph/optimize/symbolic/symbolic_kernel_factory.h"

namespace ge {
namespace {
constexpr size_t kXInputIndex = 0UL;
constexpr size_t kOutputIndex = 0UL;
constexpr size_t kValueAttrIdx = 0UL;

/**
 * ZerosLike/OnesLike算子的符号化计算
 * 【算子功能】生成与输入 x 同 Shape 的全 0/全 1 张量（不读取 x 的值，仅借用其 Shape）。
 * 【推导逻辑】读取 x 的静态 Shape 原样作为输出 Shape，按元素总数填充常量值表达式。
 * 【算子约束】x 的 Shape 必须静态可确定；空 tensor 降级。
 * 【举例】x Shape=[2,2] 时，ZerosLike 输出 Shape 为[2,2]、value 为[0,0,0,0]；
 *          OnesLike 输出 value 为[1,1,1,1]。
 */
graphStatus FillLikeSymbolicKernelCompute(gert::InferSymbolComputeContext *context, const int64_t fill_value) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("FillLike Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  std::vector<int64_t> x_dims;
  if (!context->GetConstInputDims(kXInputIndex, x_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: get x const input dim failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  if (x_dims.empty()) {
    GELOGW("SymbolicKernel compute unsupported, reason: x dims is empty, node %s[%s].", context->GetNodeName(),
           context->GetNodeType());
    return UNSUPPORTED;
  }
  int64_t output_size = 1L;
  for (const auto dim : x_dims) {
    if (ge::MulOverflow(output_size, dim, output_size)) {
      GELOGW("SymbolicKernel compute unsupported, reason: output size overflow, node %s[%s].", context->GetNodeName(),
             context->GetNodeType());
      return UNSUPPORTED;
    }
  }

  auto output_tensor = context->GetOutputSymbolTensor(kOutputIndex);
  GE_ASSERT_NOTNULL(output_tensor);
  output_tensor->MutableOriginSymbolShape().MutableDims().clear();
  for (const auto dim : x_dims) {
    output_tensor->MutableOriginSymbolShape().AppendDim(Symbol(dim));
  }
  std::vector<Expression> output_values(static_cast<size_t>(output_size), Symbol(fill_value));
  output_tensor->SetSymbolicValue(MakeUnique<std::vector<Expression>>(std::move(output_values)));
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*output_tensor).c_str());
  return SUCCESS;
}

graphStatus ZerosLikeSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  return FillLikeSymbolicKernelCompute(context, 0L);
}

graphStatus OnesLikeSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  return FillLikeSymbolicKernelCompute(context, 1L);
}

/**
 * Muls/Adds算子的符号化计算
 * 【算子功能】对输入 x 逐元素乘/加标量（value 为 attr 0）。
 * 【推导逻辑】输出 Shape 透传 x 的静态 Shape；遍历 x 的每个 SymbolicValue，
 *          构造 value*x / value+x 的值表达式。
 * 【算子约束】x 的 Shape 必须静态可确定且存在 SymbolicValue；value 属性缺失降级。
 * 【举例】x Shape=[2]、value=[2,3] 时，Muls(value=2) 输出 value 为[4,6]；
 *          Adds(value=2) 输出 value 为[4,5]。
 */
graphStatus ScalarSymbolicKernelCompute(gert::InferSymbolComputeContext *context, const bool is_mul) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("Scalar Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  const auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  const auto value_ptr = attrs->GetAttrPointer<float>(kValueAttrIdx);
  GE_UNSUPPORTED_IF_NULL(value_ptr);

  auto input_tensor = context->GetInputSymbolTensor(kXInputIndex);
  GE_UNSUPPORTED_IF_NULL(input_tensor);
  auto input_values = input_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(input_values);
  if (input_values->empty()) {
    GELOGW("SymbolicKernel compute unsupported, reason: input symbolic value is empty, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  auto output_tensor = context->GetOutputSymbolTensor(kOutputIndex);
  GE_ASSERT_NOTNULL(output_tensor);
  output_tensor->MutableOriginSymbolShape().MutableDims() = input_tensor->GetOriginSymbolShape().GetDims();
  std::vector<Expression> output_values;
  output_values.reserve(input_values->size());
  const auto scalar = Symbol(static_cast<double>(*value_ptr));
  for (const auto &value : *input_values) {
    output_values.emplace_back(is_mul ? value * scalar : value + scalar);
  }
  output_tensor->SetSymbolicValue(MakeUnique<std::vector<Expression>>(std::move(output_values)));
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*output_tensor).c_str());
  return SUCCESS;
}

graphStatus MulsSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  return ScalarSymbolicKernelCompute(context, true);
}

graphStatus AddsSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  return ScalarSymbolicKernelCompute(context, false);
}
}  // namespace

REGISTER_SYMBOLIC_KERNEL(ZerosLike, ZerosLikeSymbolicKernelCompute);
REGISTER_SYMBOLIC_KERNEL(OnesLike, OnesLikeSymbolicKernelCompute);
REGISTER_SYMBOLIC_KERNEL(Muls, MulsSymbolicKernelCompute);
REGISTER_SYMBOLIC_KERNEL(Adds, AddsSymbolicKernelCompute);
}  // namespace ge
