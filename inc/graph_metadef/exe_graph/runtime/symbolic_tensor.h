/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_EXE_GRAPH_SYMBOL_TENSOR_H_
#define METADEF_CXX_INC_EXE_GRAPH_SYMBOL_TENSOR_H_

#include <array>
#include <vector>
#include <iostream>
#include <cstring>
#include <type_traits>
#include <limits>
#include <memory>
#include "utils/extern_math_util.h"
#include "symbolic_shape.h"

namespace gert {
class SymbolTensor {
 public:
  SymbolTensor() = default;
  SymbolTensor(const std::initializer_list<ge::Expression> &origin_symbol_shape)
      : origin_symbol_shape_(origin_symbol_shape) {}

  SymbolTensor(const std::initializer_list<ge::Expression> &origin_symbol_shape,
               const std::initializer_list<ge::Expression> &symbolic_values)
      : origin_symbol_shape_(origin_symbol_shape),
        symbolic_values_(std::unique_ptr<std::vector<ge::Expression>>(
            new (std::nothrow) std::vector<ge::Expression>(symbolic_values))) {}

  // 拷贝构造函数
  SymbolTensor(const SymbolTensor &other)
      : origin_symbol_shape_(other.origin_symbol_shape_),
        symbolic_values_(other.symbolic_values_
                             ? std::unique_ptr<std::vector<ge::Expression>>(
                                   new (std::nothrow) std::vector<ge::Expression>(*other.symbolic_values_))
                             : nullptr) {}

  // 拷贝赋值运算符
  SymbolTensor &operator=(const SymbolTensor &other) {
    if (this != &other) {
      origin_symbol_shape_ = other.origin_symbol_shape_;
      if (other.symbolic_values_) {
        symbolic_values_ = std::unique_ptr<std::vector<ge::Expression>>(
            new (std::nothrow) std::vector<ge::Expression>(*other.symbolic_values_));
      } else {
        symbolic_values_.reset();
      }
    }
    return *this;
  }

  // 移动构造函数
  SymbolTensor(SymbolTensor &&other) noexcept = default;

  // 移动赋值运算符
  SymbolTensor &operator=(SymbolTensor &&other) noexcept = default;

  /**
   * 获取只读的原始格式符号化shape
   * @return 原始格式符号化shape引用
   */
  const SymbolShape &GetOriginSymbolShape() const {
    return origin_symbol_shape_;
  }
  /**
   * 设置原始格式符号化shape
   * @return void
   */
  void SetOriginSymbolShape(const SymbolShape &ori_symbol_shape) {
    origin_symbol_shape_ = ori_symbol_shape;
  }

  /**
   * 判断是否存在符号值：无符号值（未设置）返回false；已设置符号值（含个数为0的显式空值）返回true
   * @return 是否存在符号值
   */
  bool HasValue() const {
    return symbolic_values_ != nullptr;
  }
  /**
   * 获取符号值个数，无符号值时返回0
   * @return 符号值个数
   */
  size_t GetValueCount() const {
    return (symbolic_values_ == nullptr) ? 0U : symbolic_values_->size();
  }
  /**
   * 获取只读的第idx位置的符号值
   * @param idx 符号值的index
   * @return 符号值；无符号值或index越界时返回默认构造的表达式，该表达式并非有效符号值，
   * 调用方应先通过HasValue/GetValueCount判断取值是否可用
   */
  const ge::Expression &GetValue(size_t idx) const {
    if (symbolic_values_ == nullptr || idx >= symbolic_values_->size()) {
      static const ge::Expression kInvalidExpr;
      return kInvalidExpr;
    }
    return (*symbolic_values_)[idx];
  }
  /**
   * 设置符号值个数，若设置的个数大于当前值，扩展出的符号值为默认构造的表达式，
   * 需配合SetValue逐个赋值后使用
   * @param value_count 符号值个数
   * @return this引用
   */
  SymbolTensor &SetValueCount(size_t value_count) {
    if (symbolic_values_ == nullptr) {
      symbolic_values_ = std::unique_ptr<std::vector<ge::Expression>>(new (std::nothrow) std::vector<ge::Expression>());
    }
    if (symbolic_values_ != nullptr) {
      symbolic_values_->resize(value_count);
    }
    return *this;
  }
  /**
   * 设置第idx位置的符号值
   * @param idx 符号值的index，调用者需要保证index合法
   * @param value 符号值
   * @return this引用
   */
  SymbolTensor &SetValue(size_t idx, const ge::Expression &value) {
    if (symbolic_values_ != nullptr) {
      (*symbolic_values_)[idx] = value;
    }
    return *this;
  }

 private:
  SymbolShape origin_symbol_shape_;
  std::unique_ptr<std::vector<ge::Expression>> symbolic_values_;
};
}  // namespace gert

#endif  // METADEF_CXX_INC_EXE_GRAPH_SYMBOL_SHAPE_H_
