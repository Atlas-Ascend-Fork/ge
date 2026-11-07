/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/plugin/ge_make_unique_util.h"
#include "framework/common/framework_types_internal.h"
#include "common/util/mem_utils.h"
#include "common/checker.h"
#include "graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"
#include "graph/optimize/symbolic/symbol_compute_context.h"
#include "graph/optimize/symbolic/symbolic_kernel_factory.h"
#include <limits>

namespace ge {
namespace {
constexpr size_t kParamIndex = 0UL;
constexpr size_t kIndiceIndex = 1UL;
constexpr size_t kAxisIndex = 2UL;
constexpr size_t kBatchDimAttrIndex = 0UL;
constexpr size_t kOutputIndex = 0UL;

graphStatus GetInputDimsAndValue(gert::InferSymbolComputeContext *context, const size_t index,
                                 std::vector<int64_t> &input_dims, std::vector<ge::Expression> &values) {
  GE_ASSERT_NOTNULL(context);
  if (!context->GetConstInputDims(index, input_dims)) {
    GELOGW("SymbolicKernel compute unsupported, reason: Get input[%zu] dim failed, node %s[%s].", index,
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  auto input_tensor = context->GetInputSymbolTensor(index);
  GE_UNSUPPORTED_IF_NULL(input_tensor);
  auto value_symbols = input_tensor->GetSymbolicValue();
  if (value_symbols == nullptr) {
    GELOGW("SymbolicKernel compute unsupported, reason: Get input[%zu] symbolic value failed, node %s[%s].", index,
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  values = *value_symbols;
  return GRAPH_SUCCESS;
}

graphStatus GetIndicesValue(gert::InferSymbolComputeContext *context,
                            const std::vector<ge::Expression> &indice_symbol_values,
                            std::vector<int64_t> &indice_values) {
  indice_values.clear();
  for (size_t i = 0UL; i < indice_symbol_values.size(); i++) {
    int64_t indice_value = 0L;
    if (!indice_symbol_values[i].GetConstValue(indice_value)) {
      GELOGW("SymbolicKernel compute unsupported, reason: get dim sym const value failed, node %s[%s].",
             context->GetNodeName(), context->GetNodeType());
      return UNSUPPORTED;
    }
    indice_values.emplace_back(indice_value);
  }
  return GRAPH_SUCCESS;
}

graphStatus GetAxis(gert::InferSymbolComputeContext *context, int64_t &axis) {
  auto axis_tensor = context->GetInputSymbolTensor(kAxisIndex);
  GE_UNSUPPORTED_IF_NULL(axis_tensor);
  auto axis_symbols = axis_tensor->GetSymbolicValue();
  if (axis_symbols == nullptr) {
    GELOGW("SymbolicKernel compute unsupported, reason: Get axis symbolic value failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  GE_ASSERT_TRUE(axis_symbols->size() == 1UL,
                 "SymbolicKernel compute failed, reason: axis num should be 1, but get:%zu, node %s[%s].",
                 axis_symbols->size(), context->GetNodeName(), context->GetNodeType());
  if (!axis_symbols->front().GetConstValue(axis)) {
    GELOGW("SymbolicKernel compute unsupported, reason: get dim sym const value failed, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  return GRAPH_SUCCESS;
}

graphStatus NormalizeInput(gert::InferSymbolComputeContext *context, const std::vector<int64_t> &indice_dims,
                           std::vector<int64_t> &param_dims, int64_t &axis, int64_t &batch_dims) {
  if (param_dims.empty()) {
    GELOGI("GatherV2 in_shape is scalar, set it's shape to (1,)");
    param_dims.emplace_back(1);
  }
  GE_ASSERT_TRUE(
      (axis >= (-1) * static_cast<int64_t>(param_dims.size())) && (axis < static_cast<int64_t>(param_dims.size())),
      "SymbolicKernel compute failed, reason: axis should in the range [-%zu, %zu), but got %ld, node %s[%s].",
      param_dims.size(), param_dims.size(), axis, context->GetNodeName(), context->GetNodeType());
  axis = axis < 0 ? axis + static_cast<int64_t>(param_dims.size()) : axis;
  GE_ASSERT_TRUE(
      (batch_dims >= (-1) * static_cast<int64_t>(indice_dims.size())) &&
          (batch_dims <= static_cast<int64_t>(indice_dims.size())),
      "SymbolicKernel compute failed, reason: batch_dims should in the range [-%zu, %zu), but got %ld, node %s[%s].",
      indice_dims.size(), indice_dims.size(), batch_dims, context->GetNodeName(), context->GetNodeType());
  batch_dims = batch_dims < 0 ? batch_dims + static_cast<int64_t>(indice_dims.size()) : batch_dims;
  GE_ASSERT_TRUE(
      batch_dims <= axis,
      "SymbolicKernel compute failed, reason: batch_dims:%ld must be less than or equal to axis:%ld, node %s[%s].",
      batch_dims, axis, context->GetNodeName(), context->GetNodeType());
  return GRAPH_SUCCESS;
}
// 计算dims半开区间[begin, end)的乘积，含0维(空tensor，合法图)或溢出时返回false由调用方降级
bool MulDimsSafely(const std::vector<int64_t> &dims, const int64_t begin, const int64_t end, int64_t &product) {
  product = 1L;
  for (int64_t i = begin; i < end; i++) {
    const int64_t dim = dims[static_cast<size_t>(i)];
    if (dim == 0L || product > INT64_MAX / dim) {
      return false;
    }
    product *= dim;
  }
  return true;
}

// 负索引按Gather语义从轴末尾计数（[-dim, dim)）归一化；INT64_MIN与dim相加会溢出，
// 与越界索引同为非法输入，断言硬报错经调用方原样透传打挂推导
graphStatus NormalizeGatherIndex(const int64_t raw_index, const int64_t axis_dim,
                                 const gert::InferSymbolComputeContext *context, int64_t &normalized) {
  GE_ASSERT_TRUE(raw_index != std::numeric_limits<int64_t>::min(),
                 "SymbolicKernel compute failed, reason: indices index:%lld overflows, node %s[%s].", raw_index,
                 context->GetNodeName(), context->GetNodeType());
  normalized = raw_index < 0L ? raw_index + axis_dim : raw_index;
  GE_ASSERT_TRUE(normalized >= 0L && normalized < axis_dim,
                 "SymbolicKernel compute failed, reason: indices index:%lld should be in range [-%lld, %lld), "
                 "node %s[%s].",
                 raw_index, axis_dim, axis_dim, context->GetNodeName(), context->GetNodeType());
  return GRAPH_SUCCESS;
}

graphStatus CalOutputSymbolValue(gert::InferSymbolComputeContext *context, const std::vector<int64_t> &param_dims,
                                 const std::vector<ge::Expression> &param_values,
                                 const std::vector<int64_t> &indice_values, const int64_t axis,
                                 const int64_t batch_dims, std::vector<ge::Expression> &output_values) {
  // 空tensor(维为0，合法图)与溢出防护：除数来自输入dim，无防护会除零SIGFPE或迭代器越界。
  // 整除性按 outer/block/axis 逐个判断，避免先构造 outer*block*axis 乘积(可溢出为0导致除零SIGFPE)
  int64_t outer_loop_num = 0L;
  int64_t block_size = 0L;
  const int64_t axis_dim = param_dims[static_cast<size_t>(axis)];
  if (!MulDimsSafely(param_dims, 0L, batch_dims, outer_loop_num) ||
      !MulDimsSafely(param_dims, axis + 1L, static_cast<int64_t>(param_dims.size()), block_size) || axis_dim <= 0L) {
    return UNSUPPORTED;
  }
  const int64_t param_value_num = static_cast<int64_t>(param_values.size());
  if (param_value_num % outer_loop_num != 0L || (param_value_num / outer_loop_num) % block_size != 0L ||
      (param_value_num / outer_loop_num / block_size) % axis_dim != 0L ||
      static_cast<int64_t>(indice_values.size()) % outer_loop_num != 0L) {
    return UNSUPPORTED;
  }
  const int64_t indice_block_size = static_cast<int64_t>(indice_values.size()) / outer_loop_num;
  const int64_t block_num = static_cast<int64_t>(param_values.size()) / outer_loop_num / block_size / axis_dim;
  const int64_t outer_block_size = static_cast<int64_t>(param_values.size()) / outer_loop_num;
  GELOGD("param total size: %zu, indices total size:%zu, axis dim num: %lld, node %s[%s].", param_values.size(),
         indice_values.size(), axis_dim, context->GetNodeName(), context->GetNodeType());
  for (int64_t i = 0L; i < outer_loop_num; i++) {
    for (int64_t j = 0L; j < block_num; j++) {
      for (int64_t k = 0L; k < indice_block_size; k++) {
        int64_t gather_index = 0L;
        const auto index_ret = NormalizeGatherIndex(indice_values[static_cast<size_t>(k + indice_block_size * i)],
                                                    axis_dim, context, gather_index);
        if (index_ret != GRAPH_SUCCESS) {
          return index_ret;
        }
        const auto start_iter =
            param_values.begin() + (i * outer_block_size + (j * axis_dim + gather_index) * block_size);
        const auto end_iter =
            param_values.begin() + (i * outer_block_size + (j * axis_dim + gather_index + 1L) * block_size);
        output_values.insert(output_values.end(), start_iter, end_iter);
      }
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus CalOutputSymbolShape(gert::InferSymbolComputeContext *context, const std::vector<int64_t> &param_dims,
                                 const std::vector<int64_t> &indice_dims, const int64_t axis, const int64_t batch_dims,
                                 gert::SymbolShape &output_shape) {
  output_shape.Clear();
  for (size_t i = 0UL; i < static_cast<size_t>(axis); i++) {
    output_shape.AppendDim(Symbol(param_dims[i]));
  }

  for (size_t i = 0UL; i < static_cast<size_t>(batch_dims); i++) {
    GE_ASSERT_TRUE(param_dims[i] == indice_dims[i],
                   "SymbolicKernel compute failed, reason: param[%zu] dim size: %lld should equal to indices[%zu] dim "
                   "size: %lld, node %s[%s].",
                   i, param_dims[i], i, indice_dims[i], context->GetNodeName(), context->GetNodeType());
  }
  for (size_t i = static_cast<size_t>(batch_dims); i < indice_dims.size(); i++) {
    output_shape.AppendDim(Symbol(indice_dims[i]));
  }
  for (size_t i = static_cast<size_t>(axis + 1UL); i < param_dims.size(); i++) {
    output_shape.AppendDim(Symbol(param_dims[i]));
  }
  return GRAPH_SUCCESS;
}
graphStatus GatherCompute(gert::InferSymbolComputeContext *context, int64_t axis, int64_t batch_dims) {
  std::vector<int64_t> param_dims;
  std::vector<ge::Expression> param_values;
  auto ret = GetInputDimsAndValue(context, kParamIndex, param_dims, param_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 获取indice的shape
  std::vector<int64_t> indice_dims;
  std::vector<ge::Expression> indice_symbol_values;
  ret = GetInputDimsAndValue(context, kIndiceIndex, indice_dims, indice_symbol_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 获取indice_value
  std::vector<int64_t> indice_values;
  ret = GetIndicesValue(context, indice_symbol_values, indice_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  GE_ASSERT_SUCCESS(NormalizeInput(context, indice_dims, param_dims, axis, batch_dims));
  auto symbolic_tensor = context->GetOutputSymbolTensor(kOutputIndex);
  GE_ASSERT_NOTNULL(symbolic_tensor);
  GE_ASSERT_SUCCESS(CalOutputSymbolShape(context, param_dims, indice_dims, axis, batch_dims,
                                         symbolic_tensor->MutableOriginSymbolShape()));
  auto output_symbol_values = symbolic_tensor->MutableSymbolicValue();
  GE_ASSERT_NOTNULL(output_symbol_values);
  // 原样透传：空tensor/溢出的UNSUPPORTED由外层降级到传统推导，indices越界等非法输入的
  // ErrorResult(断言)传出后由驱动打挂推导，不可统一转UNSUPPORTED吞掉报错
  const auto value_ret =
      CalOutputSymbolValue(context, param_dims, param_values, indice_values, axis, batch_dims, *output_symbol_values);
  if (value_ret != SUCCESS) {
    return value_ret;
  }
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*symbolic_tensor).c_str());
  return SUCCESS;
}
}  // namespace

static graphStatus GatherV2SymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("GatherV2 Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  int64_t axis = 0L;
  auto ret = GetAxis(context, axis);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  auto batch_dims_attr = attrs->GetAttrPointer<int64_t>(kBatchDimAttrIndex);
  GE_ASSERT_NOTNULL(batch_dims_attr);
  return GatherCompute(context, axis, *batch_dims_attr);
}
REGISTER_SYMBOLIC_KERNEL(GatherV2, GatherV2SymbolicKernelCompute);

namespace {
constexpr size_t kGatherBatchDimAttrIndex = 1UL;
}  // namespace

static graphStatus GatherSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("Gather Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  int64_t axis = 0L;
  int64_t batch_dims = 0L;
  auto attrs = context->GetAttrs();
  if (attrs != nullptr && attrs->GetAttrNum() > kGatherBatchDimAttrIndex) {
    auto batch_dims_attr = attrs->GetAttrPointer<int64_t>(kGatherBatchDimAttrIndex);
    if (batch_dims_attr != nullptr) {
      batch_dims = *batch_dims_attr;
    }
  }
  return GatherCompute(context, axis, batch_dims);
}
REGISTER_SYMBOLIC_KERNEL(Gather, GatherSymbolicKernelCompute);

namespace {
constexpr size_t kElementsXIndex = 0UL;
constexpr size_t kElementsIndexIdx = 1UL;
constexpr size_t kElementsDimAttrIdx = 0UL;
constexpr size_t kElementsOutputIdx = 0UL;
}  // namespace

// 按输出扁平位置分解 index 形状的 N-D 坐标（行主序），dim 维替换为归一化索引值后
// 映射 x 扁平偏移，逐元素选取 x 的值表达式
static graphStatus CalGatherElementsOutValue(gert::InferSymbolComputeContext *context,
                                             const std::vector<int64_t> &x_dims, const std::vector<int64_t> &index_dims,
                                             const std::vector<ge::Expression> &x_values,
                                             const std::vector<int64_t> &index_values, int64_t dim,
                                             std::vector<ge::Expression> &out_values) {
  const auto rank = static_cast<int64_t>(x_dims.size());
  out_values.reserve(index_values.size());
  std::vector<int64_t> coords(x_dims.size(), 0L);
  for (size_t p = 0UL; p < index_values.size(); p++) {
    int64_t remain = static_cast<int64_t>(p);
    for (int64_t d = rank - 1; d >= 0; d--) {
      coords[static_cast<size_t>(d)] = remain % index_dims[static_cast<size_t>(d)];
      remain /= index_dims[static_cast<size_t>(d)];
    }
    int64_t gather_index = 0L;
    const auto index_ret =
        NormalizeGatherIndex(index_values[p], x_dims[static_cast<size_t>(dim)], context, gather_index);
    if (index_ret != GRAPH_SUCCESS) {
      return index_ret;
    }
    coords[static_cast<size_t>(dim)] = gather_index;
    int64_t offset = 0L;
    for (int64_t d = 0L; d < rank; d++) {
      offset = offset * x_dims[static_cast<size_t>(d)] + coords[static_cast<size_t>(d)];
    }
    out_values.emplace_back(x_values[static_cast<size_t>(offset)]);
  }
  return GRAPH_SUCCESS;
}

static graphStatus GatherElementsSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("GatherElements Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  std::vector<int64_t> x_dims;
  std::vector<ge::Expression> x_values;
  graphStatus ret = GetInputDimsAndValue(context, kElementsXIndex, x_dims, x_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  std::vector<int64_t> index_dims;
  std::vector<ge::Expression> index_symbol_values;
  ret = GetInputDimsAndValue(context, kElementsIndexIdx, index_dims, index_symbol_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 索引值必须为常量，逐元素按 Gather 语义从 x 值表达式中选取
  std::vector<int64_t> index_values;
  ret = GetIndicesValue(context, index_symbol_values, index_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 0 维输入与秩不等属不支持形态，降级回退传统推导；秩相等与 index.size(d)<=input.size(d)
  // 是 torch gather 的文档化约束，违反即非法图，断言硬报错打挂推导
  if (x_dims.empty() || x_dims.size() != index_dims.size()) {
    GELOGW(
        "SymbolicKernel compute unsupported, reason: x rank %zu should be positive and equal to index rank %zu, "
        "node %s[%s].",
        x_dims.size(), index_dims.size(), context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }
  for (size_t d = 0UL; d < index_dims.size(); d++) {
    GE_ASSERT_TRUE(index_dims[d] <= x_dims[d],
                   "SymbolicKernel compute failed, reason: index dim[%zu]: %lld should be no greater than x dim: "
                   "%lld, node %s[%s].",
                   d, index_dims[d], x_dims[d], context->GetNodeName(), context->GetNodeType());
  }
  auto attrs = context->GetAttrs();
  GE_ASSERT_NOTNULL(attrs);
  const auto dim_ptr = attrs->GetAttrPointer<int64_t>(kElementsDimAttrIdx);
  GE_ASSERT_NOTNULL(dim_ptr);
  int64_t dim = *dim_ptr;
  dim = dim < 0 ? dim + static_cast<int64_t>(x_dims.size()) : dim;
  GE_ASSERT_TRUE(dim >= 0 && dim < static_cast<int64_t>(x_dims.size()),
                 "SymbolicKernel compute failed, reason: dim %lld should be in [-%zu, %zu), node %s[%s].", *dim_ptr,
                 x_dims.size(), x_dims.size(), context->GetNodeName(), context->GetNodeType());
  // 空 tensor(维为0，合法图)与维乘积溢出防护：任一命中即降级
  int64_t x_total = 0L;
  int64_t index_total = 0L;
  const auto rank = static_cast<int64_t>(x_dims.size());
  if (!MulDimsSafely(x_dims, 0L, rank, x_total) || !MulDimsSafely(index_dims, 0L, rank, index_total) ||
      static_cast<int64_t>(x_values.size()) != x_total || static_cast<int64_t>(index_values.size()) != index_total) {
    return UNSUPPORTED;
  }
  auto symbolic_tensor = context->GetOutputSymbolTensor(kElementsOutputIdx);
  GE_ASSERT_NOTNULL(symbolic_tensor);
  symbolic_tensor->MutableOriginSymbolShape().Clear();
  for (const auto d : index_dims) {
    symbolic_tensor->MutableOriginSymbolShape().AppendDim(Symbol(d));
  }
  auto output_symbol_values = symbolic_tensor->MutableSymbolicValue();
  GE_ASSERT_NOTNULL(output_symbol_values);
  const graphStatus value_ret =
      CalGatherElementsOutValue(context, x_dims, index_dims, x_values, index_values, dim, *output_symbol_values);
  if (value_ret != SUCCESS) {
    return value_ret;
  }
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*symbolic_tensor).c_str());
  return SUCCESS;
}
REGISTER_SYMBOLIC_KERNEL(GatherElements, GatherElementsSymbolicKernelCompute);

/**
 * GatherNd算子的符号化计算
 * 【算子功能】按 indices 末 idx_rank 维给出的 N-D 坐标，从 x 中逐前缀位置取整段尾部维数据。
 * 【推导逻辑】输出 Shape = indices 前缀维 + x[idx_rank:]；索引值须为常量，逐前缀位置取
 *          idx_rank 个坐标（负索引归一化、越界断言硬报错），映射 x 扁平偏移后整段拷贝
 *          x 尾部维的值表达式。
 * 【算子约束】x/indices 须有静态 Shape 与 SymbolicValue；idx_rank 不超过 x 的秩；
 *          空 tensor/维乘积溢出/值数不匹配降级。
 * 【举例】x=[2,2] value=[10,11,12,13]、indices shape=[2,2] value=[1,1,0,1] 时，
 *          输出 Shape 为[2]，输出 value 为[13,11]。
 */
static graphStatus GatherNdSymbolicKernelCompute(gert::InferSymbolComputeContext *context) {
  GE_ASSERT_NOTNULL(context);
  GELOGD("GatherNd Symbolic Kernel in, node %s[%s].", context->GetNodeName(), context->GetNodeType());
  std::vector<int64_t> x_dims;
  std::vector<ge::Expression> x_values;
  graphStatus ret = GetInputDimsAndValue(context, kElementsXIndex, x_dims, x_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  std::vector<int64_t> indices_dims;
  std::vector<ge::Expression> indices_symbol_values;
  ret = GetInputDimsAndValue(context, kElementsIndexIdx, indices_dims, indices_symbol_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 索引值必须为常量
  std::vector<int64_t> indices_values;
  ret = GetIndicesValue(context, indices_symbol_values, indices_values);
  if (ret != GRAPH_SUCCESS) {
    return ret;
  }
  // 空 tensor(维为0，合法图)降级；idx_rank 为坐标长度，不得超过 x 的秩
  if (x_dims.empty() || indices_dims.empty() || x_values.empty() || indices_values.empty()) {
    GELOGW("SymbolicKernel compute unsupported, reason: empty x/indices tensor, node %s[%s].", context->GetNodeName(),
           context->GetNodeType());
    return UNSUPPORTED;
  }
  const auto x_rank = static_cast<int64_t>(x_dims.size());
  const int64_t idx_rank = indices_dims.back();
  if (idx_rank <= 0L || idx_rank > x_rank) {
    GELOGW("SymbolicKernel compute unsupported, reason: indices last dim %lld should be in (0, %lld], node %s[%s].",
           idx_rank, x_rank, context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  int64_t prefix_total = 0L;
  int64_t tail_size = 0L;
  int64_t x_total = 0L;
  if (!MulDimsSafely(indices_dims, 0L, static_cast<int64_t>(indices_dims.size()) - 1L, prefix_total) ||
      !MulDimsSafely(x_dims, idx_rank, x_rank, tail_size) || !MulDimsSafely(x_dims, 0L, x_rank, x_total) ||
      static_cast<int64_t>(x_values.size()) != x_total ||
      static_cast<int64_t>(indices_values.size()) != prefix_total * idx_rank) {
    GELOGW("SymbolicKernel compute unsupported, reason: dims product overflow or value size mismatch, node %s[%s].",
           context->GetNodeName(), context->GetNodeType());
    return UNSUPPORTED;
  }

  auto symbolic_tensor = context->GetOutputSymbolTensor(kElementsOutputIdx);
  GE_ASSERT_NOTNULL(symbolic_tensor);
  symbolic_tensor->MutableOriginSymbolShape().Clear();
  for (size_t i = 0UL; i + 1UL < indices_dims.size(); i++) {
    symbolic_tensor->MutableOriginSymbolShape().AppendDim(Symbol(indices_dims[i]));
  }
  for (int64_t i = idx_rank; i < x_rank; i++) {
    symbolic_tensor->MutableOriginSymbolShape().AppendDim(Symbol(x_dims[static_cast<size_t>(i)]));
  }
  auto output_symbol_values = symbolic_tensor->MutableSymbolicValue();
  GE_ASSERT_NOTNULL(output_symbol_values);
  output_symbol_values->clear();
  output_symbol_values->reserve(static_cast<size_t>(prefix_total * tail_size));
  for (int64_t p = 0L; p < prefix_total; p++) {
    // 逐前缀位置解析 idx_rank 个坐标并映射 x 扁平偏移（前缀偏移 * 尾部长度）
    int64_t offset = 0L;
    for (int64_t i = 0L; i < idx_rank; i++) {
      int64_t coord = 0L;
      const auto coord_ret = NormalizeGatherIndex(indices_values[static_cast<size_t>(p * idx_rank + i)],
                                                  x_dims[static_cast<size_t>(i)], context, coord);
      if (coord_ret != GRAPH_SUCCESS) {
        return coord_ret;
      }
      offset = offset * x_dims[static_cast<size_t>(i)] + coord;
    }
    const int64_t base = offset * tail_size;
    for (int64_t t = 0L; t < tail_size; t++) {
      output_symbol_values->emplace_back(x_values[static_cast<size_t>(base + t)]);
    }
  }
  GELOGD("%s[%s] kernel success, %s", context->GetNodeName(), context->GetNodeType(),
         SymbolicInferUtil::DumpSymbolTensor(*symbolic_tensor).c_str());
  return SUCCESS;
}
REGISTER_SYMBOLIC_KERNEL(GatherNd, GatherNdSymbolicKernelCompute);
}  // namespace ge
