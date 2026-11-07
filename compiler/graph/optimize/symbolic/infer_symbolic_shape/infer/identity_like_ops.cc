/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/checker.h"
#include "common/framework_types_internal.h"
#include "graph/compute_graph.h"
#include "exe_graph/runtime/infer_symbol_shape_context.h"
#include "graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"

namespace ge {
namespace {
graphStatus InferShape4Assign(gert::InferSymbolShapeContext *context) {
  auto value_shape = context->GetInputSymbolShape(1);
  GE_UNSUPPORTED_IF_NULL(value_shape);

  auto ref_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(ref_shape);

  *ref_shape = *value_shape;
  return ge::GRAPH_SUCCESS;
}

graphStatus InferShape4L2Loss(gert::InferSymbolShapeContext *context) {
  auto out_shape = context->GetOutputSymbolShape(0);
  GE_UNSUPPORTED_IF_NULL(out_shape);
  out_shape->Clear();
  return ge::GRAPH_SUCCESS;
}

graphStatus InferSymbolicShapeFunc(gert::InferSymbolShapeContext *context) {
  GE_ASSERT_NOTNULL(context);
  const auto in_tensor = context->GetInputSymbolTensor(0);
  GE_UNSUPPORTED_IF_NULL(in_tensor);
  const auto in_tensor_val = in_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(in_tensor_val);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  out_shape->MutableDims() = *in_tensor_val;
  return GRAPH_SUCCESS;
}
/**
 * RandomStandardNormal 的符号 Shape 推导。
 * 【算子功能】从标准正态分布采样随机值，输出张量 y。
 * 【算子约束】shape 为 int32/int64 的 1-D 常量张量（data dependency），其值即输出各维度；dtype、seed、seed2 为属性。
 * 【推导逻辑】读取 shape 输入的 SymbolicValue，逐元素写入输出 Shape。
 * 【举例】shape=[2,3,4] 时，输出 y=[2,3,4]。
 */
graphStatus InferShape4RandomStandardNormal(gert::InferSymbolShapeContext *context) {
  const auto shape_tensor = context->GetInputSymbolTensor(0);
  GE_UNSUPPORTED_IF_NULL(shape_tensor);
  const auto shape_value = shape_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(shape_value);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  out_shape->MutableDims().clear();
  for (const auto &dim : *shape_value) {
    out_shape->MutableDims().emplace_back(dim);
  }
  return GRAPH_SUCCESS;
}

/**
 * RandomUniformIntV2/RandomStandardNormalV2/TruncatedNormalV2 的符号 Shape 推导。
 * 【算子功能】随机采样类算子的 V2 变体：fusion pass 换型后新增 offset 输出（shape=[1]，记录随机数流偏移）。
 * 【算子约束】shape 为 int32/int64 的 1-D 张量（data dependency），其值即输出各维度。
 * 【推导逻辑】输出 y 的 Shape 取 shape 输入的 SymbolicValue；输出 offset 恒为 [1]。
 * 【举例】shape=[2,3] 时，y=[2,3]，offset=[1]。
 */
graphStatus InferShape4RandomV2(gert::InferSymbolShapeContext *context) {
  const auto shape_tensor = context->GetInputSymbolTensor(0);
  GE_UNSUPPORTED_IF_NULL(shape_tensor);
  const auto shape_value = shape_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(shape_value);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  out_shape->MutableDims() = *shape_value;
  const auto offset_shape = context->GetOutputSymbolShape(1);
  GE_ASSERT_NOTNULL(offset_shape);
  offset_shape->MutableDims().clear();
  offset_shape->MutableDims().emplace_back(Symbol(1));
  return GRAPH_SUCCESS;
}

IMPL_OP_INFER_SYMBOL_SHAPE_INNER(Assign).InferSymbolShape(InferShape4Assign);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(L2Loss).InferSymbolShape(InferShape4L2Loss);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(RandomUniform).InferSymbolShape(InferSymbolicShapeFunc);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(RandomUniformV2).InferSymbolShape(InferSymbolicShapeFunc);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(RandomStandardNormal).InferSymbolShape(InferShape4RandomStandardNormal);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(TruncatedNormal).InferSymbolShape(InferSymbolicShapeFunc);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(RandomUniformIntV2).InferSymbolShape(InferShape4RandomV2);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(RandomStandardNormalV2).InferSymbolShape(InferShape4RandomV2);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(TruncatedNormalV2).InferSymbolShape(InferShape4RandomV2);

}  // namespace
}  // namespace ge
