/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <algorithm>
#include <list>
#include "framework/common/framework_types_internal.h"
#include "common/util.h"
#include "common/plugin/ge_make_unique_util.h"
#include "graph/optimize/symbolic/symbol_compute_context.h"
#include "graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"
#include "graph/optimize/symbolic/symbolic_kernel_factory.h"

namespace ge {
namespace {
constexpr size_t kXInputIndex = 0UL;
constexpr size_t kMultiplesInputIndex = 1UL;
constexpr size_t kOutputIndex = 0UL;
constexpr size_t kMaxSupportDim = 8UL;
constexpr int64_t kMaxOutputSize = 1024L;

graphStatus GetMultiplesDims(const gert::InferSymbolComputeContext *context, std::vector<int64_t> &multiples_dims) {
  std::vector<int64_t> input_dims;
  context->GetConstInputDims(kMultiplesInputIndex, input_dims);
  if (input_dims.size() != 1) {
    GELOGW("SymbolicKernel compute unsupported, reason: %zu input dims size (%zu) not equal 1, node %s[%s].",
           kMultiplesInputIndex, input_dims.size(), context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  auto input_tensor = context->GetInputSymbolTensor(kMultiplesInputIndex);
  GE_UNSUPPORTED_IF_NULL(input_tensor);
  auto multiples_symbols = input_tensor->GetSymbolicValue();
  if (multiples_symbols == nullptr) {
    GELOGW("SymbolicKernel compute unsupported, reason: get %zu input symbolic value failed, node %s[%s].",
           kMultiplesInputIndex, context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  if (multiples_symbols->size() > kMaxSupportDim) {
    GELOGW("SymbolicKernel compute unsupported, reason: multiples_symbols size(%zu) over limit(%zu), node %s[%s].",
           multiples_symbols->size(), kMaxSupportDim, context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  multiples_dims.clear();
  for (size_t i = 0UL; i < multiples_symbols->size(); i++) {
    int64_t dim = 0L;
    if (!(*multiples_symbols)[i].GetConstValue(dim)) {
      GELOGW("SymbolicKernel compute unsupported, reason: get %zu input const value failed, node %s[%s].", i,
             context->GetNodeName(), context->GetNodeType());
      return UNSUPPORTED;
    }
    multiples_dims.emplace_back(dim);
  }
  GELOGD("%s[%s] kernel compute, GetMultiplesDims %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::VectorToStr(multiples_dims).c_str());
  return SUCCESS;
}

void AlignDimNum(std::vector<int64_t> &x_dims, std::vector<int64_t> &y_dims) {
  std::reverse(x_dims.begin(), x_dims.end());
  std::reverse(y_dims.begin(), y_dims.end());
  // 获取维度较小的dims
  std::vector<int64_t> &dims_with_less_size = x_dims.size() < y_dims.size() ? x_dims : y_dims;
  // 给维度较小的dims补维
  dims_with_less_size.insert(dims_with_less_size.end(),
                             abs(static_cast<int64_t>(x_dims.size()) - static_cast<int64_t>(y_dims.size())), 1);
  std::reverse(x_dims.begin(), x_dims.end());
  std::reverse(y_dims.begin(), y_dims.end());
}

Status TileInputXSymbols(const std::vector<int64_t> &x_dims, const std::vector<int64_t> &multiples_dims,
                         const std::vector<Expression> &x_symbols, std::vector<Expression> &output_symbols) {
  // scalar x(TF合法图内容，秩-0参与Tile)或空multiples不支持值传播，降级到传统推导而非打挂
  if (x_dims.empty() || multiples_dims.empty()) {
    GELOGW("SymbolicKernel compute unsupported, reason: x dims or multiples dims is empty.");
    return UNSUPPORTED;
  }
  // multiples=0(TF合法repeats=0，输出为空tensor)不支持值传播，降级；负值是非法shape语义直接assert报错；
  // 两者(size_t)(m-1)均会回绕导致insert循环近乎无限挂死，必须拦截
  for (const auto multiple : multiples_dims) {
    if (multiple == 0L) {
      GELOGW("SymbolicKernel compute unsupported, reason: multiples value is 0.");
      return UNSUPPORTED;
    }
    GE_ASSERT_TRUE(multiple > 0L, "SymbolicKernel compute failed, reason: multiples value %lld is negative.", multiple);
  }
  int64_t dim_multis = 1L;
  std::list<Expression> outputs_symbols_list(x_symbols.begin(), x_symbols.end());
  for (int64_t dim_index = static_cast<int64_t>(x_dims.size()) - 1; dim_index >= 0; dim_index--) {
    int64_t block_size = dim_multis * x_dims[static_cast<size_t>(dim_index)];
    auto insert_pos = outputs_symbols_list.begin();
    GE_ASSERT_TRUE(block_size > 0);
    size_t block_num = outputs_symbols_list.size() / static_cast<size_t>(block_size);
    size_t cur_index = 0UL;
    while (cur_index < block_num) {
      for (size_t tile_index = 0UL;
           tile_index < static_cast<size_t>(multiples_dims[static_cast<size_t>(dim_index)] - 1); tile_index++) {
        outputs_symbols_list.insert(insert_pos, insert_pos, std::next(insert_pos, block_size));
      }
      insert_pos = std::next(insert_pos, block_size);
      cur_index++;
    }
    dim_multis = block_size * multiples_dims[static_cast<size_t>(dim_index)];
  }
  output_symbols.assign(outputs_symbols_list.begin(), outputs_symbols_list.end());
  return SUCCESS;
}

Status CalcOutputShapeSymbols(const std::vector<int64_t> &x_dims, const std::vector<int64_t> &multiples_dims,
                              std::vector<Expression> &output_shape_symbols) {
  GE_ASSERT_TRUE(x_dims.size() == multiples_dims.size());
  for (size_t i = 0UL; i < x_dims.size(); i++) {
    output_shape_symbols.emplace_back(Symbol(x_dims[i] * multiples_dims[i]));
  }
  return SUCCESS;
}

bool CheckOutputSize(const std::vector<int64_t> &x_dims, const std::vector<int64_t> &multiples_dims) {
  int64_t output_size = 1L;
  for (size_t i = 0U; i < x_dims.size(); i++) {
    if (ge::MulOverflow(output_size, x_dims[i] * multiples_dims[i], output_size)) {
      return false;
    }
  }
  return output_size <= kMaxOutputSize;
}
}  // namespace

static graphStatus TileSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("Tile Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  // 获取multiples
  std::vector<int64_t> multiples_dims;
  auto ret = GetMultiplesDims(context, multiples_dims);
  GE_ASSERT_TRUE(ret == UNSUPPORTED || ret == SUCCESS);
  if (ret == UNSUPPORTED) {
    GELOGW("SymbolicKernel compute unsupported, reason: get const input failed, node %s[%s].", context->GetNodeName(),
           context->GetNodeType());
    return UNSUPPORTED;
  }
  // 获取x的SymShape,shape必须是静态shape
  std::vector<int64_t> x_dims;
  if (!context->GetConstInputDims(kXInputIndex, x_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: get const input dim failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  // 对齐multiples_symbols和x_dims的维度
  AlignDimNum(x_dims, multiples_dims);
  if (!CheckOutputSize(x_dims, multiples_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: check output size failed, node %s[%s].", context->GetNodeName(),
           context->GetNodeType());
    return UNSUPPORTED;
  }
  GELOGI("Input x shape dims: %s, multiples dims: %s.", SymbolicInferUtil::VectorToStr(x_dims).c_str(),
         SymbolicInferUtil::VectorToStr(multiples_dims).c_str());
  // 获取value的值
  auto x_symbols = context->GetInputSymbolTensor(kXInputIndex)->GetSymbolicValue();
  if (x_symbols == nullptr) {
    GELOGW("SymbolicKernel compute unsupported, reason: get input symbolic value failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  // 扩展x_symbols（原样透传：scalar/空/multiples=0的UNSUPPORTED由外层降级，
  // 负multiples等非法输入的ErrorResult由驱动打挂推导）
  auto output_symbols = context->GetOutputSymbolTensor(kOutputIndex);
  GE_ASSERT_NOTNULL(output_symbols);
  auto output_symbols_value = output_symbols->MutableSymbolicValue();
  GE_ASSERT_NOTNULL(output_symbols_value);
  const auto tile_ret = TileInputXSymbols(x_dims, multiples_dims, *x_symbols, *output_symbols_value);
  if (tile_ret != SUCCESS) {
    return tile_ret;
  }
  // 计算输出shape
  std::vector<Expression> output_shape_symbols;
  GE_ASSERT_SUCCESS(CalcOutputShapeSymbols(x_dims, multiples_dims, output_shape_symbols));
  output_symbols->MutableOriginSymbolShape().MutableDims() = output_shape_symbols;

  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*output_symbols).c_str());
  return SUCCESS;
}
REGISTER_SYMBOLIC_KERNEL(Tile, TileSymbolicKernelCompute);

namespace {
constexpr size_t kShapeInputIndex = 1UL;

// 目标shape取自输入1的SymbolicValue：仅接受常量维；-1表示沿用输入对应维度（与shape推导语义一致）。
// 非常量目标维度无法在值路径展开，降级交由 shape 推导回调处理。
graphStatus GetBroadcastTargetDims(const gert::InferSymbolComputeContext *context, const std::vector<int64_t> &x_dims,
                                   std::vector<int64_t> &target_dims) {
  auto shape_tensor = context->GetInputSymbolTensor(kShapeInputIndex);
  GE_UNSUPPORTED_IF_NULL(shape_tensor);
  auto shape_symbols = shape_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(shape_symbols);
  if (shape_symbols->empty() || shape_symbols->size() > kMaxSupportDim) {
    GELOGW("SymbolicKernel compute unsupported, reason: target shape rank(%zu) invalid, node %s[%s].",
           shape_symbols->size(), context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  target_dims.clear();
  const int64_t diff = static_cast<int64_t>(shape_symbols->size() - x_dims.size());
  for (size_t i = 0UL; i < shape_symbols->size(); i++) {
    int64_t dim = 0L;
    if (!(*shape_symbols)[i].GetConstValue(dim)) {
      GELOGW("SymbolicKernel compute unsupported, reason: target dim(%zu) is not constant, node %s[%s].", i,
             context->GetNodeName(), context->GetNodeType());
      return UNSUPPORTED;
    }
    if (dim == -1) {
      const int64_t src_idx = static_cast<int64_t>(i) - diff;
      if (src_idx < 0) {
        GELOGW("SymbolicKernel compute unsupported, reason: -1 dim(%zu) has no input dim to reuse, node %s[%s].", i,
               context->GetNodeName(), context->GetNodeType());
        return UNSUPPORTED;
      }
      dim = x_dims[static_cast<size_t>(src_idx)];
    }
    if (dim <= 0L) {
      // 0维输出为空tensor，值传播无意义，降级
      GELOGW("SymbolicKernel compute unsupported, reason: target dim(%zu) value %lld is not positive, node %s[%s].", i,
             dim, context->GetNodeName(), context->GetNodeType());
      return UNSUPPORTED;
    }
    target_dims.emplace_back(dim);
  }
  return SUCCESS;
}

// x维度右对齐后校验可广播：每个对齐维为1或等于目标维
bool CheckBroadcastable(const std::vector<int64_t> &x_dims, const std::vector<int64_t> &target_dims) {
  const size_t diff = target_dims.size() - x_dims.size();
  for (size_t i = 0UL; i < target_dims.size(); i++) {
    if (i < diff) {
      continue;
    }
    const int64_t x_dim = x_dims[i - diff];
    if (x_dim != 1L && x_dim != target_dims[i]) {
      return false;
    }
  }
  return true;
}

// 依据stride将x的SymbolicValue映射到输出线性位置（广播维取0步长）
graphStatus BroadcastInputXSymbols(const std::vector<Expression> &src_data, const std::vector<int64_t> &x_dims,
                                   const std::vector<int64_t> &target_dims, std::vector<Expression> &dst_data) {
  std::vector<int64_t> aligned_x = x_dims;
  while (aligned_x.size() < target_dims.size()) {
    aligned_x.insert(aligned_x.begin(), 1L);
  }
  auto compute_strides = [](const std::vector<int64_t> &shape) {
    std::vector<int64_t> strides(shape.size(), 1L);
    for (int64_t i = static_cast<int64_t>(shape.size()) - 2; i >= 0; i--) {
      strides[static_cast<size_t>(i)] = strides[static_cast<size_t>(i + 1)] * shape[static_cast<size_t>(i + 1)];
    }
    return strides;
  };
  const std::vector<int64_t> src_strides = compute_strides(aligned_x);
  const std::vector<int64_t> dst_strides = compute_strides(target_dims);

  int64_t total_elements = 1L;
  for (auto dim : target_dims) {
    total_elements *= dim;
  }
  dst_data.clear();
  dst_data.reserve(static_cast<size_t>(total_elements));
  for (int64_t pos = 0L; pos < total_elements; pos++) {
    int64_t src_linear = 0L;
    for (size_t i = 0UL; i < target_dims.size(); i++) {
      const int64_t idx = (pos / dst_strides[i]) % target_dims[i];
      src_linear += (idx % aligned_x[i]) * src_strides[i];
    }
    dst_data.push_back(src_data[static_cast<size_t>(src_linear)]);
  }
  return SUCCESS;
}
}  // namespace

/**
 * BroadcastTo算子的符号化计算
 * 【算子功能】将输入张量按目标shape广播（对齐维为1的维度扩展到目标大小）。
 * 【推导逻辑】目标shape取自输入1 SymbolicValue的常量维（-1沿用输入对应维），x维度右对齐后校验
 *          广播兼容性生成输出Shape；再依据stride将x的SymbolicValue映射到输出线性位置。
 *          输出元素数超过上限或目标维非常量时降级，交由shape推导回调处理。
 * 【算子约束】x与目标shape必须静态可确定且满足广播规则，x必须存在SymbolicValue。
 * 【举例】x Shape=[1,2]、value=[10,20]，目标shape=[2,2]时，
 *          输出Shape为[2,2]，输出value为[10,20,10,20]。
 */
static graphStatus BroadcastToSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("BroadcastTo Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());

  // 获取x的静态Shape
  std::vector<int64_t> x_dims;
  if (!context->GetConstInputDims(kXInputIndex, x_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: get x const input dim failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  if (x_dims.size() > kMaxSupportDim) {
    GELOGW("SymbolicKernel compute unsupported, reason: x rank(%zu) over limit(%zu), node %s[%s].", x_dims.size(),
           kMaxSupportDim, context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  // 获取目标shape并校验广播兼容性与输出规模
  std::vector<int64_t> target_dims;
  if (GetBroadcastTargetDims(context, x_dims, target_dims) != SUCCESS) {
    return UNSUPPORTED;
  }
  if (target_dims.size() < x_dims.size() || !CheckBroadcastable(x_dims, target_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: x cannot broadcast to target shape, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  int64_t output_size = 1L;
  for (auto dim : target_dims) {
    if (ge::MulOverflow(output_size, dim, output_size)) {
      return UNSUPPORTED;
    }
  }
  if (output_size > kMaxOutputSize) {
    GELOGW("SymbolicKernel compute unsupported, reason: output size(%lld) over limit(%lld), node %s[%s].", output_size,
           kMaxOutputSize, context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  // 获取x的SymbolicValue并广播
  auto x_tensor = context->GetInputSymbolTensor(kXInputIndex);
  GE_UNSUPPORTED_IF_NULL(x_tensor);
  auto x_symbols = x_tensor->GetSymbolicValue();
  GE_UNSUPPORTED_IF_NULL(x_symbols);
  std::vector<Expression> output_values;
  GE_ASSERT_SUCCESS(BroadcastInputXSymbols(*x_symbols, x_dims, target_dims, output_values));

  // 写入输出Shape和SymbolicValue
  auto output_tensor = context->GetOutputSymbolTensor(kOutputIndex);
  GE_ASSERT_NOTNULL(output_tensor);
  std::vector<Expression> output_shape_symbols;
  for (auto dim : target_dims) {
    output_shape_symbols.emplace_back(Symbol(dim));
  }
  output_tensor->MutableOriginSymbolShape().MutableDims() = output_shape_symbols;
  auto output_values_unique = MakeUnique<std::vector<Expression>>(std::move(output_values));
  if (output_values_unique != nullptr) {
    output_tensor->SetSymbolicValue(std::move(output_values_unique));
  }
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*output_tensor).c_str());
  return SUCCESS;
}
REGISTER_SYMBOLIC_KERNEL(BroadcastTo, BroadcastToSymbolicKernelCompute);
}  // namespace ge
