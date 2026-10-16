/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstdlib>
#include "graph/compute_graph.h"
#include "exe_graph/runtime/infer_symbol_shape_context.h"
#include "common/checker.h"
#include "common/framework_types_internal.h"
#include "graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"

namespace ge {
namespace {

struct GatherInfo {
  int64_t axis;
  int64_t index_batch_dims;
};

bool CheckAndUpdateAxis(int64_t &axis, const int64_t shape_dim_num) {
  if (axis >= 0) {
    return shape_dim_num == 0 ? axis == 0 : axis < shape_dim_num;
  }
  const bool check_res = (axis >= -shape_dim_num);
  if (check_res) {
    axis += shape_dim_num;
  }
  return check_res;
}

graphStatus GatherCommonInfer(gert::InferSymbolShapeContext *context, const gert::SymbolShape *in_shape,
                              const gert::SymbolShape *indies_shape, const GatherInfo &gather_info) {
  const auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  const auto batch_dims_ptr = attrs->GetAttrPointer<int64_t>(gather_info.index_batch_dims);
  GE_ASSERT_NOTNULL(batch_dims_ptr);
  int64_t batch_dims = *batch_dims_ptr;
  int64_t axis = gather_info.axis;
  int64_t in_real_dim_cnt = in_shape->GetDimNum();
  // 如果是标量为0，如果是向量为1
  const int64_t rank_indices = indies_shape->GetDimNum();
  GE_ASSERT_TRUE(CheckAndUpdateAxis(batch_dims, rank_indices),
                 "Expected batch_dims in the range [-%ld, %ld), but got %ld", rank_indices, rank_indices, batch_dims);
  auto real_in_shape = gert::SymbolShape();
  for (int64_t i = 0; i < in_real_dim_cnt; i++) {
    real_in_shape.AppendDim(in_shape->GetDim(i));
  }
  if (in_shape->IsScalar()) {
    GELOGD("GatherCommInfer in_shape is scalar, set it's shape to (1,)");
    real_in_shape.AppendDim(Symbol(1));
    in_real_dim_cnt = 1;
  }
  GE_ASSERT_TRUE(in_real_dim_cnt >= 1, "in_real_dim_cnt:%d must be greater than or equal to 1", in_real_dim_cnt);
  GE_ASSERT_TRUE(batch_dims < in_real_dim_cnt, "batch_dims:%d must be less than rank x:%d", batch_dims,
                 in_real_dim_cnt);
  // todo 添加guard in_shape和indies_shape前batch_dims的维度相同
  GE_ASSERT_TRUE(CheckAndUpdateAxis(axis, in_real_dim_cnt), "axis:%d is invalid, in_real_dim_cnt:%d", axis,
                 in_real_dim_cnt);
  GE_ASSERT_TRUE(batch_dims <= axis, "batch_dims:%d must be less than or equal to axis:%d", batch_dims, axis);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  for (int64_t i = 0; i < axis; i++) {
    out_shape->AppendDim(real_in_shape.GetDim(i));
  }
  for (int64_t i = 0L; i < batch_dims; i++) {
    ASSERT_SYMBOL_EQ(real_in_shape.GetDim(i), indies_shape->GetDim(i));
  }
  for (int64_t i = batch_dims; i < rank_indices; i++) {
    out_shape->AppendDim(indies_shape->GetDim(i));
  }
  for (int64_t i = axis + 1; i < in_real_dim_cnt; i++) {
    out_shape->AppendDim(real_in_shape.GetDim(i));
  }
  return GRAPH_SUCCESS;
}

/**
 * GatherV2 算子的符号化shape推导
 * 【算子功能】按照指定的轴(axis)，从输入数据中取给定下标(indices)的数据，每次取整个轴(下标)的数据，并按照下标顺序组织取出来的数据。
 * 【算子约束】
 * 【推导逻辑】
 */
graphStatus InferShape4GatherV2(gert::InferSymbolShapeContext *context) {
  constexpr size_t axes_idx = 2U;
  const auto in_shape = context->GetInputSymbolShape(0);
  GE_UNSUPPORTED_IF_NULL(in_shape);
  // 可能是标量、向量或者张量
  const auto indies_shape = context->GetInputSymbolShape(1);
  GE_UNSUPPORTED_IF_NULL(indies_shape);
  const auto axes_tensor = context->GetInputSymbolTensor(axes_idx);
  GE_UNSUPPORTED_IF_NULL(axes_tensor);
  if (axes_tensor->GetSymbolicValue() == nullptr) {
    GELOGW("Symbol Infer unsupported, get symbolic value is nullptr, node %s[%s]", context->GetNodeName(),
           context->GetNodeType());
    return UNSUPPORTED;
  }
  const auto axes_size = static_cast<int32_t>(axes_tensor->GetSymbolicValue()->size());
  GE_ASSERT_EQ(axes_size, 1);
  int64_t axis = 0;
  const auto axes_desc = context->GetInputDesc(axes_idx);
  GE_ASSERT_NOTNULL(axes_desc);
  const auto status = SymbolicInferUtil::GetConstInt(axes_tensor, axes_desc->GetDataType(), axis);
  if (status != GRAPH_SUCCESS) {
    return status;
  }
  const GatherInfo gather_info = {axis, 0};
  return GatherCommonInfer(context, in_shape, indies_shape, gather_info);
}

graphStatus InferShape4Gather(gert::InferSymbolShapeContext *context) {
  const auto in_shape = context->GetInputSymbolShape(0);
  GE_UNSUPPORTED_IF_NULL(in_shape);
  // 可能是标量、向量或者张量
  const auto indies_shape = context->GetInputSymbolShape(1);
  GE_UNSUPPORTED_IF_NULL(indies_shape);
  constexpr GatherInfo gather_info = {0, 1};

  return GatherCommonInfer(context, in_shape, indies_shape, gather_info);
}

IMPL_OP_INFER_SYMBOL_SHAPE_INNER(GatherV2).InferSymbolShape(InferShape4GatherV2);
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(Gather).InferSymbolShape(InferShape4Gather);

/**
 * GatherNd: 对于维度为r≥1的输入张量self，和维度q≥1的输入张量indices，将数据切片收集到输出张量out中
 * out的shape为[indices_shape[0:q-1], self_shape[c:r]]
 */
graphStatus InferShape4GatherNd(gert::InferSymbolShapeContext *context) {
  GE_ASSERT_NOTNULL(context);
  const auto data_shape = context->GetInputSymbolShape(0);
  GE_UNSUPPORTED_IF_NULL(data_shape);
  const int64_t data_dim = data_shape->GetDimNum();

  const auto indices_shape = context->GetInputSymbolShape(1);
  GE_UNSUPPORTED_IF_NULL(indices_shape);
  const int64_t indices_dim = indices_shape->GetDimNum();
  GE_ASSERT_TRUE(indices_dim >= 1, "indices_dim must be >= 1, but got %lld", indices_dim);

  const auto k_symbol = indices_shape->GetDim(indices_dim - 1);
  int64_t k_value = -1;
  if (!k_symbol.GetConstValue<int64_t>(k_value)) {
    GELOGW("Last indices are not const in node %s.", context->GetNodeName());
    return UNSUPPORTED;
  }
  ASSERT_SYMBOL_GE(k_symbol, Symbol(0));
  ASSERT_SYMBOL_LE(k_symbol, Symbol(data_dim));

  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  for (int64_t i = 0; i < indices_dim - 1; ++i) {
    out_shape->AppendDim(indices_shape->GetDim(i));
  }
  for (int64_t i = k_value; i < data_dim; ++i) {
    out_shape->AppendDim(data_shape->GetDim(i));
  }

  return GRAPH_SUCCESS;
}
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(GatherNd).InferSymbolShape(InferShape4GatherNd);

graphStatus InferShape4GatherEles(gert::InferSymbolShapeContext *context) {
  GE_ASSERT_NOTNULL(context);
  const auto in_shape = context->GetInputSymbolShape(1);
  GE_UNSUPPORTED_IF_NULL(in_shape);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  out_shape->MutableDims() = in_shape->GetDims();
  return SUCCESS;
}
IMPL_OP_INFER_SYMBOL_SHAPE_INNER(GatherElements).InferSymbolShape(InferShape4GatherEles);

// DynamicStitch: 根据indices将多个输入张量x拼接到输出张量的指定位置
// 符号化shape推导逻辑：
// 1. 输出shape第一维为所有indices中的最大值+1
// 2. 输出shape其余维度与输入x的shape除去indices维度后的部分相同
// 3. 验证所有输入x的shape除去indices维度后的部分一致
graphStatus InferShapeForDynamicStitch(gert::InferSymbolShapeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GE_UNSUPPORTED_IF_NULL(context->GetInputSymbolShape(0));
  const auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  const int32_t *attr_n = attrs->GetAttrPointer<int32_t>(0);
  GE_ASSERT_TRUE(attr_n != nullptr && *attr_n >= 1);
  const auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  auto &out_shape_dims = out_shape->MutableDims();
  out_shape_dims.clear();

  for (int32_t i = 0U; i < *attr_n; ++i) {
    const auto indices_shape = context->GetDynamicInputSymbolShape(0, i);
    GE_UNSUPPORTED_IF_NULL(indices_shape);
    const auto indices_dims = indices_shape->GetDims();
    const auto x_shape = context->GetDynamicInputSymbolShape(1, i);
    GE_UNSUPPORTED_IF_NULL(x_shape);
    const auto x_dims = x_shape->GetDims();
    GE_ASSERT_TRUE(x_dims.size() >= indices_dims.size(), "Op %s: x rank %zu is smaller than indices rank %zu.",
                   context->GetNodeName(), x_dims.size(), indices_dims.size());
    for (size_t j = 0U; j < indices_dims.size(); ++j) {
      ASSERT_SYMBOL_EQ(x_dims[j], indices_dims[j]);
    }
    if (i == 0U) {
      out_shape_dims.push_back(kSymbolZero);
      out_shape_dims.insert(out_shape_dims.end(), x_dims.begin() + indices_dims.size(), x_dims.end());
    } else {
      for (size_t j = indices_dims.size(); j < x_dims.size(); ++j) {
        ASSERT_SYMBOL_EQ(x_dims[j], out_shape_dims[j - indices_dims.size() + 1]);
      }
    }

    const auto indices_tensor = context->GetDynamicInputSymbolTensor(0, i);
    GE_UNSUPPORTED_IF_NULL(indices_tensor);
    const auto indices_tensor_val = indices_tensor->GetSymbolicValue();
    GE_UNSUPPORTED_IF_NULL(indices_tensor_val);
    for (const auto &tensor : *indices_tensor_val) {
      out_shape_dims[0] = sym::Max(out_shape_dims[0], tensor);
    }
  }

  out_shape_dims[0] = out_shape_dims[0] + kSymbolOne;
  return ge::GRAPH_SUCCESS;
}

IMPL_OP_INFER_SYMBOL_SHAPE_INNER(DynamicStitch).InferSymbolShape(InferShapeForDynamicStitch);

/**
 * GatherShapes算子的符号化shape推导
 * 【算子功能】收集指定的输入dim, 作为输出
 * 【算子约束】
 *      1. 至少有一个输入
 *      2. 属性axes存放要输出的shape的dim值，格式为[input_index, dim_index]，input_index要在输入个数范围内，
 *         dim_index要在输入dim_num范围内
 *      举例： 输入0=(3)，输入1=(1,2)，属性axes={{0, 0},{1, 1}}，输出=(3,2)
 * 【推导逻辑】
 *      1. 按照算子约束校验输入和axes属性
 *      2. 属性axes的size作为输出shape
 */
graphStatus InferShape4GatherShapes(gert::InferSymbolShapeContext *context) {
  auto input_shape = context->GetInputSymbolShape(0);
  GE_UNSUPPORTED_IF_NULL(input_shape);
  auto input_num = context->GetComputeNodeInputNum();
  GE_ASSERT(input_num != 0U);

  auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  const auto *attr_axes = attrs->GetListListInt(0);
  GE_ASSERT_NOTNULL(attr_axes);
  auto axes_size = attr_axes->GetSize();
  GE_ASSERT(axes_size != 0U);

  for (size_t i = 0U; i < axes_size; ++i) {
    const auto cv = attr_axes->Get(i);
    GE_ASSERT_NOTNULL(cv);
    GE_ASSERT_EQ(cv->GetSize(), 2U);  // [input_index, dim_index]
    const uint64_t *data = reinterpret_cast<const uint64_t *>(cv->GetData());
    GE_ASSERT_NOTNULL(data);
    const auto input_index = *data;
    GE_ASSERT(input_index < context->GetComputeNodeInputNum(),
              "Node %s input_index[%lu] must be less than input num[%zu],"
              " i: %zu",
              context->GetNodeName(), input_index, context->GetComputeNodeInputNum(), i);
    const auto dim_index = *(data + 1U);
    auto in_shape = context->GetInputSymbolShape(input_index);
    GE_UNSUPPORTED_IF_NULL(in_shape);
    GE_ASSERT(dim_index < in_shape->GetDimNum(),
              "Node %s dim_index[%lu] must be less than input shape"
              " dim number[%zu], i: %zu",
              context->GetNodeName(), dim_index, in_shape->GetDimNum(), i);
  }

  auto out_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(out_shape);
  out_shape->Clear();
  out_shape->AppendDim(Symbol(axes_size));
  return GRAPH_SUCCESS;
}

IMPL_OP_INFER_SYMBOL_SHAPE_INNER(GatherShapes).InferSymbolShape(InferShape4GatherShapes);
}  // namespace
}  // namespace ge
