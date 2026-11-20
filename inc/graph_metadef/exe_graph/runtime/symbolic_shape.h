/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_EXE_GRAPH_SYMBOL_SHAPE_H_
#define METADEF_CXX_INC_EXE_GRAPH_SYMBOL_SHAPE_H_

#include <array>
#include <vector>
#include <iostream>
#include <cstring>
#include <type_traits>
#include <limits>
#include "utils/extern_math_util.h"
#include "graph/symbolizer/symbolic.h"

namespace gert {
/*
 * 注意：此类是一个符号化的shape，它的成员变量是一个Expression数组，只在编译态使用，暂不考虑POD形式组织该类
 * */
class SymbolShape {
 public:
  /**
   * 默认构造一个符号化symbol shape，默认构造的shape实例中，dims长度为空
   */
  SymbolShape() = default;

  /**
   * 通过dims值构造符号化shape，例如：SymbolShape({&s0, &s1, &s2, &s3})创建一个Shape实例，有4个维度，
   * 每个维度的值分别是s0, s1, s2, s3
   * @param dims shape的所有dim值
   */
  SymbolShape(const std::initializer_list<ge::Expression> &args) : dims_(args) {}

  /**
   * 拷贝构造函数,移动构造函数
   * @param other
   */
  SymbolShape(const SymbolShape &other) = default;
  SymbolShape &operator=(const SymbolShape &other) = default;
  SymbolShape(SymbolShape &&other) = default;
  SymbolShape &operator=(SymbolShape &&other) = default;

  /**
   * 判断与另外一个shape对象是否相等：dim num相等，且逐维比较时两侧dim均有效的位置symbol值相等；
   * 任一侧dim为非法表达式（IsValid为false）时该维视为相等
   * @param rht 另一个Shape对象
   * @return true/false
   */
  bool operator==(const SymbolShape &rht) const {
    if (this->dims_.size() != rht.dims_.size()) {
      return false;
    }
    for (size_t i = 0; i < this->dims_.size(); i++) {
      if ((this->dims_[i].IsValid()) && (rht.dims_[i].IsValid()) && (this->dims_[i] != rht.dims_[i])) {
        return false;
      }
    }
    return true;
  }

  /**
   * 判断与另一个Shape对象是否不等
   * @param rht 另一个SymbolShape对象
   * @return true/false
   */
  bool operator!=(const SymbolShape &rht) const {
    return !(*this == rht);
  }

  /**
   * 获取shape size表达式，如果是scalar场景，返回Symbol(1),如果symbol_shape中某个表达式非法，那么返回Symbol(0)
   * @return shape-size，是一个Expression表达式
   */
  const ge::Expression &GetSymbolShapeSize() const {
    if (symsize_cache_is_valid_) {  // 性能优化，避免重复计算
      return symbol_shape_size_;
    }
    symbol_shape_size_ = ge::Symbol(1);
    for (const auto &dim : dims_) {
      if (dim.IsValid()) {
        symbol_shape_size_ = ge::sym::Mul(symbol_shape_size_, dim);
      } else {
        static auto kZero = ge::Symbol(0);
        return kZero;
      }
    }
    symsize_cache_is_valid_ = true;
    return symbol_shape_size_;
  }

  /**
   * 清空symbol shape的所有维度
   * @return none
   */
  void Clear() {
    symsize_cache_is_valid_ = false;
    dims_.clear();
  }

  /**
   * 获取dim num
   * @return
   */
  size_t GetDimNum() const {
    return dims_.size();
  }

  /**
   * 向后扩展一个dim值
   * @param 扩展的dim值
   * @return this引用
   */
  SymbolShape &AppendDim(const ge::Expression &dim_value) {
    symsize_cache_is_valid_ = false;
    dims_.emplace_back(dim_value);
    return *this;
  }

  /**
   * 设置dim num，若设置的dim num大于当前值，扩展出的维度为默认构造的表达式，
   * 需配合SetDim逐维赋值后使用
   * @param dim_num dim的个数
   * @return this引用
   */
  SymbolShape &SetDimNum(size_t dim_num) {
    symsize_cache_is_valid_ = false;
    dims_.resize(dim_num);
    return *this;
  }

  /**
   * 设置第idx位置的dim值
   * @param idx dim的index，调用者需要保证index合法
   * @param dim_value dim值
   * @return this引用
   */
  SymbolShape &SetDim(size_t idx, const ge::Expression &dim_value) {
    symsize_cache_is_valid_ = false;
    dims_[idx] = dim_value;
    return *this;
  }

  /**
   * 获取只读的第idx位置的dim值
   * @param idx dim的index，调用方需保证index合法，越界访问属于未定义行为
   * @return dim值
   */
  const ge::Expression &GetDim(size_t idx) const {
    return dims_[idx];
  }

 private:
  std::vector<ge::Expression> dims_;
  mutable bool symsize_cache_is_valid_{false};  // 性能优化，避免重复计算Symbol shape size
  mutable ge::Expression symbol_shape_size_;
};
}  // namespace gert

#endif  // METADEF_CXX_INC_EXE_GRAPH_SYMBOL_SHAPE_H_
