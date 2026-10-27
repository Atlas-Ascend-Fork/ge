/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstdint>
#include <iostream>
#include "graph/custom_op.h"
#include "register/op_impl_registry.h"
#include "concat_v2_custom.h"

extern "C" void LaunchConcatV2Custom(const void *x0, const void *x1, void *y, uint64_t outer, uint64_t inner,
                                     uint64_t dim0, uint64_t dim1, uint64_t total_dim, uint64_t elem_size,
                                     void *stream);

namespace {
// IR 输入索引（与生产算子 ConcatV2 一致：x 动态输入占 IR 0，concat_dim 占 IR 1）
constexpr size_t kXIrIndex = 0U;
constexpr size_t kConcatDimIrIndex = 1U;
constexpr size_t kOutputIndex = 0U;
constexpr int64_t kUnknownDim = -1;
constexpr int64_t kUnknownRankDim = -2;  // 单维 [-2] 表示未知秩
constexpr int64_t kDemoInputNum = 2;     // 演示 kernel 固定 2 路输入

// ---- InferShape 辅助函数 ----

ge::graphStatus GetXInstanceNum(const gert::ExtendedKernelContext *context, int64_t &num_inputs) {
  const auto compute_node_info = context->GetComputeNodeInfo();
  if (compute_node_info == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << " null compute node info" << std::endl;
    return ge::GRAPH_FAILED;
  }
  const auto x_info = compute_node_info->GetInputInstanceInfo(kXIrIndex);
  if (x_info == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << " null input instance info" << std::endl;
    return ge::GRAPH_FAILED;
  }
  num_inputs = static_cast<int64_t>(x_info->GetInstanceNum());
  return ge::GRAPH_SUCCESS;
}

// 读取拼接轴值（按 IR 索引取值/取 dtype，写法与生产算子一致，详见 README 注意事项）。
// 值不可得时 axis_available=false；返回失败仅表示 dtype 非法或描述缺失。
ge::graphStatus ReadConcatAxis(gert::InferShapeContext *context, int64_t &axis, bool &axis_available) {
  const gert::CompileTimeTensorDesc *concat_dim_desc = context->GetRequiredInputDesc(kConcatDimIrIndex);
  if (concat_dim_desc == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << " null concat_dim desc" << std::endl;
    return ge::GRAPH_FAILED;
  }
  const ge::DataType concat_dim_dtype = concat_dim_desc->GetDataType();
  const gert::Tensor *concat_dim_tensor = context->GetRequiredInputTensor(kConcatDimIrIndex);
  if (concat_dim_dtype == ge::DT_INT32) {
    const int32_t *data = (concat_dim_tensor == nullptr) ? nullptr : concat_dim_tensor->GetData<int32_t>();
    if (data != nullptr) {
      axis = static_cast<int64_t>(data[0]);
      axis_available = true;
    }
  } else if (concat_dim_dtype == ge::DT_INT64) {
    const int64_t *data = (concat_dim_tensor == nullptr) ? nullptr : concat_dim_tensor->GetData<int64_t>();
    if (data != nullptr) {
      axis = data[0];
      axis_available = true;
    }
  } else {
    std::cerr << __FILE__ << ":" << __LINE__ << " unsupported concat_dim dtype: " << static_cast<int>(concat_dim_dtype)
              << std::endl;
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

// 等价生产算子 opbase 的 IsUnknownRank：单维 [-2] 表示未知秩
bool IsUnknownRankShape(const gert::Shape &shape) {
  return (shape.GetDimNum() == 1U) && (shape.GetDim(0) == kUnknownRankDim);
}

// 轴值不可得：保秩输出全 -1（全未知秩时输出 [-2]），交由运行期重推
ge::graphStatus DegradeToUnknownDims(gert::InferShapeContext *context, const int64_t num_inputs) {
  gert::Shape *output_shape = context->GetOutputShape(kOutputIndex);
  if (output_shape == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << " null output shape" << std::endl;
    return ge::GRAPH_FAILED;
  }
  std::cout << "[InferShape] concat_dim value unavailable, degrade to unknown dims" << std::endl;
  int64_t known_rank = kUnknownDim;
  for (int64_t i = 0; i < num_inputs; ++i) {
    const gert::Shape *shape_i = context->GetDynamicInputShape(kXIrIndex, static_cast<size_t>(i));
    if (shape_i == nullptr) {
      std::cerr << __FILE__ << ":" << __LINE__ << " null dynamic input shape" << std::endl;
      return ge::GRAPH_FAILED;
    }
    const int64_t dim_num = static_cast<int64_t>(shape_i->GetDimNum());
    if (!IsUnknownRankShape(*shape_i) && (dim_num > known_rank)) {
      known_rank = dim_num;
    }
  }
  output_shape->SetDimNum(0U);
  if (known_rank < 0) {
    // 所有输入均为未知秩
    (void)output_shape->AppendDim(kUnknownRankDim);
    return ge::GRAPH_SUCCESS;
  }
  for (int64_t i = 0; i < known_rank; ++i) {
    (void)output_shape->AppendDim(kUnknownDim);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus AccumulateAxisDims(gert::InferShapeContext *context, const int64_t num_inputs, const int64_t axis,
                                   bool &dim_unknown, int64_t &new_dim) {
  for (int64_t i = 1; i < num_inputs; ++i) {
    const gert::Shape *shape_i = context->GetDynamicInputShape(kXIrIndex, static_cast<size_t>(i));
    if (shape_i == nullptr) {
      std::cerr << __FILE__ << ":" << __LINE__ << " null dynamic input shape" << std::endl;
      return ge::GRAPH_FAILED;
    }
    const int64_t dim_value = shape_i->GetDim(axis);
    if (dim_value == kUnknownDim) {
      dim_unknown = true;
    } else {
      new_dim += dim_value;
    }
  }
  return ge::GRAPH_SUCCESS;
}

// 输出 = 首输入 shape，轴维替换为各输入该维之和
ge::graphStatus InferShapeByAxis(gert::InferShapeContext *context, const int64_t num_inputs, int64_t axis) {
  gert::Shape *output_shape = context->GetOutputShape(kOutputIndex);
  const gert::Shape *first_shape = context->GetDynamicInputShape(kXIrIndex, 0U);
  if ((output_shape == nullptr) || (first_shape == nullptr)) {
    std::cerr << __FILE__ << ":" << __LINE__ << " null output shape or first input shape" << std::endl;
    return ge::GRAPH_FAILED;
  }
  *output_shape = *first_shape;

  const int64_t rank = static_cast<int64_t>(output_shape->GetDimNum());
  const int64_t raw_axis = axis;
  if (axis < 0) {
    axis += rank;
  }
  if ((axis < 0) || (axis >= rank)) {
    std::cerr << __FILE__ << ":" << __LINE__ << " invalid concat_dim " << raw_axis << ", rank " << rank << std::endl;
    return ge::GRAPH_FAILED;
  }
  std::cout << "[InferShape] got concat axis from input value: " << raw_axis << " (normalized to " << axis << ")"
            << std::endl;

  bool dim_unknown = output_shape->GetDim(axis) == kUnknownDim;
  int64_t new_dim = 0;
  if (!dim_unknown) {
    new_dim = output_shape->GetDim(axis);
  }
  if (AccumulateAxisDims(context, num_inputs, axis, dim_unknown, new_dim) != ge::GRAPH_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  output_shape->SetDim(axis, dim_unknown ? kUnknownDim : new_dim);
  return ge::GRAPH_SUCCESS;
}

// ---- Execute 辅助函数 ----

// 拼接拷贝布局（kernel 下发参数，均由 shape 推得）
struct ConcatLayout {
  int64_t outer;  // 轴前各维乘积（块数）
  int64_t inner;  // 轴后各维乘积（每行元素数）
  int64_t dim0;
  int64_t dim1;
  int64_t total_dim;
};

ge::graphStatus ValidateConcatShapes(const gert::Shape &shape0, const gert::Shape &shape1,
                                     const gert::Shape &output_shape) {
  const int64_t rank = static_cast<int64_t>(shape0.GetDimNum());
  if ((shape1.GetDimNum() != rank) || (output_shape.GetDimNum() != rank)) {
    std::cerr << __FILE__ << ":" << __LINE__ << " rank mismatch: x0 " << rank << ", x1 " << shape1.GetDimNum() << ", y "
              << output_shape.GetDimNum() << std::endl;
    return ge::GRAPH_FAILED;
  }
  for (int64_t d = 0; d < rank; ++d) {
    if ((shape0.GetDim(d) <= 0) || (shape1.GetDim(d) <= 0) || (output_shape.GetDim(d) <= 0)) {
      std::cerr << __FILE__ << ":" << __LINE__ << " shapes must be fully known and positive, dim " << d << ": "
                << shape0.GetDim(d) << "/" << shape1.GetDim(d) << "/" << output_shape.GetDim(d)
                << ". If output stays unknown, the graph did not run on the dynamic execution path"
                << " (runtime InferShape re-inference is required to pre-fill exact shape)." << std::endl;
      return ge::GRAPH_FAILED;
    }
  }
  return ge::GRAPH_SUCCESS;
}

// 拼接语义下输出与首输入恰在轴维不同（轴维=各输入之和），其余维一致
ge::graphStatus DeriveAxisFromShapes(const gert::Shape &shape0, const gert::Shape &output_shape, int64_t &axis) {
  axis = -1;
  const int64_t rank = static_cast<int64_t>(shape0.GetDimNum());
  for (int64_t d = 0; d < rank; ++d) {
    if (output_shape.GetDim(d) == shape0.GetDim(d)) {
      continue;
    }
    if (axis >= 0) {
      std::cerr << __FILE__ << ":" << __LINE__ << " output shape differs from x0 at more than one dim" << std::endl;
      return ge::GRAPH_FAILED;
    }
    axis = d;
  }
  if (axis < 0) {
    std::cerr << __FILE__ << ":" << __LINE__ << " output shape is identical to x0, not a concat result" << std::endl;
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ComputeConcatLayout(const gert::Shape &shape0, const gert::Shape &shape1,
                                    const gert::Shape &output_shape, const int64_t axis, ConcatLayout &layout) {
  layout = ConcatLayout{1, 1, 0, 0, 0};
  const int64_t rank = static_cast<int64_t>(shape0.GetDimNum());
  for (int64_t d = 0; d < rank; ++d) {
    if ((d != axis) && (shape0.GetDim(d) != shape1.GetDim(d))) {
      std::cerr << __FILE__ << ":" << __LINE__ << " non-axis dim mismatch at " << d << ": " << shape0.GetDim(d)
                << " vs " << shape1.GetDim(d) << std::endl;
      return ge::GRAPH_FAILED;
    }
    if (d < axis) {
      layout.outer *= shape0.GetDim(d);
    }
    if (d > axis) {
      layout.inner *= shape0.GetDim(d);
    }
  }
  layout.dim0 = shape0.GetDim(axis);
  layout.dim1 = shape1.GetDim(axis);
  layout.total_dim = layout.dim0 + layout.dim1;
  if (output_shape.GetDim(axis) != layout.total_dim) {
    std::cerr << __FILE__ << ":" << __LINE__ << " pre-filled output dim " << output_shape.GetDim(axis) << " at axis "
              << axis << " != sum of inputs " << layout.total_dim << std::endl;
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

void LogPreFilledShape(const gert::Shape &output_shape) {
  std::cout << "[Execute] use pre-filled output shape: [";
  for (size_t d = 0U; d < output_shape.GetDimNum(); ++d) {
    if (d != 0U) {
      std::cout << ", ";
    }
    std::cout << output_shape.GetDim(d);
  }
  std::cout << "]" << std::endl;
}

gert::Tensor *MallocExactOutput(gert::EagerOpExecutionContext *ctx, const gert::Tensor *input_x0,
                                const gert::Shape &output_shape) {
  gert::StorageShape exact_output_shape;
  for (size_t d = 0U; d < output_shape.GetDimNum(); ++d) {
    (void)exact_output_shape.MutableOriginShape().AppendDim(output_shape.GetDim(d));
    (void)exact_output_shape.MutableStorageShape().AppendDim(output_shape.GetDim(d));
  }
  return ctx->MallocOutputTensor(kOutputIndex, exact_output_shape, input_x0->GetFormat(), input_x0->GetDataType());
}

ge::graphStatus GetExecuteInputs(gert::EagerOpExecutionContext *ctx, const gert::Tensor *&input_x0,
                                 const gert::Tensor *&input_x1, const gert::Tensor *&output_template) {
  input_x0 = ctx->GetDynamicInputTensor(kXIrIndex, 0U);
  input_x1 = ctx->GetDynamicInputTensor(kXIrIndex, 1U);
  output_template = ctx->GetOutputTensor(kOutputIndex);
  if ((input_x0 == nullptr) || (input_x1 == nullptr) || (output_template == nullptr)) {
    std::cerr << __FILE__ << ":" << __LINE__ << " tensor is null, x0: " << input_x0 << ", x1: " << input_x1
              << ", output template: " << output_template << std::endl;
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus LaunchConcatKernel(gert::EagerOpExecutionContext *ctx, const gert::Tensor *input_x0,
                                   const gert::Tensor *input_x1, gert::Tensor *output_y, const ConcatLayout &layout) {
  const int64_t elem_size = ge::GetSizeInBytes(1, input_x0->GetDataType());
  if (elem_size <= 0) {
    std::cerr << __FILE__ << ":" << __LINE__ << " invalid element size: " << elem_size << std::endl;
    return ge::GRAPH_FAILED;
  }
  LaunchConcatV2Custom(input_x0->GetAddr(), input_x1->GetAddr(), output_y->GetAddr(),
                       static_cast<uint64_t>(layout.outer), static_cast<uint64_t>(layout.inner),
                       static_cast<uint64_t>(layout.dim0), static_cast<uint64_t>(layout.dim1),
                       static_cast<uint64_t>(layout.total_dim), static_cast<uint64_t>(elem_size), ctx->GetStream());
  return ge::GRAPH_SUCCESS;
}
}  // namespace

namespace ge {
// 值依赖算子：InferShape 读 concat_dim 输入值推导输出 shape（对齐生产算子 ops-math ConcatV2）；
// Execute 不读输入值，输出 shape 由框架预填（ExecuteCustomOpWithInferShape）。
class ConcatV2Custom : public EagerExecuteOp, public ShapeInferOp {
 public:
  graphStatus InferShape(gert::InferShapeContext *context) override {
    int64_t num_inputs = 0;
    if (GetXInstanceNum(context, num_inputs) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    if (num_inputs <= 1) {
      std::cerr << __FILE__ << ":" << __LINE__ << " ConcatV2Custom requires at least 2 inputs, got " << num_inputs
                << std::endl;
      return GRAPH_FAILED;
    }
    int64_t axis = 0;
    bool axis_available = false;
    if (ReadConcatAxis(context, axis, axis_available) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    if (!axis_available) {
      return DegradeToUnknownDims(context, num_inputs);
    }
    return InferShapeByAxis(context, num_inputs, axis);
  }

  graphStatus InferDataType(gert::InferDataTypeContext *context) override {
    const ge::DataType dtype = context->GetDynamicInputDataType(kXIrIndex, 0U);
    if (dtype == ge::DT_UNDEFINED) {
      std::cerr << __FILE__ << ":" << __LINE__ << " undefined input dtype" << std::endl;
      return GRAPH_FAILED;
    }
    return context->SetOutputDataType(kOutputIndex, dtype);
  }

  // Execute 只做校验、按预填 shape 申请输出与 kernel 下发，不做算子侧 D2H / stream 同步
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    if (ctx == nullptr) {
      std::cerr << __FILE__ << ":" << __LINE__ << " execute context is null" << std::endl;
      return GRAPH_FAILED;
    }
    int64_t num_inputs = 0;
    if (GetXInstanceNum(ctx, num_inputs) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    if (num_inputs != kDemoInputNum) {
      std::cerr << __FILE__ << ":" << __LINE__ << " demo kernel only supports " << kDemoInputNum << " inputs, got "
                << num_inputs << std::endl;
      return GRAPH_FAILED;
    }
    const gert::Tensor *input_x0 = nullptr;
    const gert::Tensor *input_x1 = nullptr;
    const gert::Tensor *output_template = nullptr;
    if (GetExecuteInputs(ctx, input_x0, input_x1, output_template) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    const gert::Shape &shape0 = input_x0->GetOriginShape();
    const gert::Shape &shape1 = input_x1->GetOriginShape();
    const gert::Shape &output_shape = output_template->GetOriginShape();
    if (ValidateConcatShapes(shape0, shape1, output_shape) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    int64_t axis = 0;
    if (DeriveAxisFromShapes(shape0, output_shape, axis) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    ConcatLayout layout = {};
    if (ComputeConcatLayout(shape0, shape1, output_shape, axis, layout) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
    LogPreFilledShape(output_shape);
    gert::Tensor *output_y = MallocExactOutput(ctx, input_x0, output_shape);
    if (output_y == nullptr) {
      std::cerr << __FILE__ << ":" << __LINE__ << " failed to malloc output tensor" << std::endl;
      return GRAPH_FAILED;
    }
    return LaunchConcatKernel(ctx, input_x0, input_x1, output_y, layout);
  }
};

// 声明 concat_dim（IR 输入 1）为数据依赖输入
IMPL_OP(ConcatV2Custom).InputsDataDependency({kConcatDimIrIndex});

REG_AUTO_MAPPING_OP(ConcatV2Custom);
}  // namespace ge
