/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "attribute_group/attr_group_symbolic_desc.h"
#include "graph/symbolizer/expression_ext.h"
#include "common/checker.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "proto/ge_ir.pb.h"

namespace ge {

graphStatus SymbolicDescAttr::Serialize(proto::AttrGroupDef &attr_group_def) {
  auto tensor_attr_group = attr_group_def.mutable_tensor_attr_group();
  GE_ASSERT_NOTNULL(tensor_attr_group);
  tensor_attr_group->clear_origin_symbol_shape();
  const auto &origin_symbol_shape = symbolic_tensor.GetOriginSymbolShape();
  for (size_t i = 0UL; i < origin_symbol_shape.GetDimNum(); ++i) {
    tensor_attr_group->add_origin_symbol_shape(origin_symbol_shape.GetDim(i).Str().get());
  }
  tensor_attr_group->clear_symbolic_value();
  for (size_t i = 0UL; i < symbolic_tensor.GetValueCount(); ++i) {
    tensor_attr_group->add_symbolic_value(symbolic_tensor.GetValue(i).Str().get());
  }
  return GRAPH_SUCCESS;
}

graphStatus SymbolicDescAttr::Deserialize(const proto::AttrGroupDef &attr_group_def, AttrHolder *attr_holder) {
  (void)attr_holder;
  const auto &tensor_attr_group = attr_group_def.tensor_attr_group();
  gert::SymbolShape origin_symbol_shape;
  for (const auto &ori_sh : tensor_attr_group.origin_symbol_shape()) {
    origin_symbol_shape.AppendDim(ge::expr_ext::Parse(ori_sh.c_str()));
  }
  this->symbolic_tensor.SetOriginSymbolShape(origin_symbol_shape);
  if (!tensor_attr_group.symbolic_value().empty()) {
    const int value_num = tensor_attr_group.symbolic_value().size();
    this->symbolic_tensor.SetValueCount(static_cast<size_t>(value_num));
    for (int i = 0; i < value_num; ++i) {
      this->symbolic_tensor.SetValue(static_cast<size_t>(i),
                                     ge::expr_ext::Parse(tensor_attr_group.symbolic_value().Get(i).c_str()));
    }
  }
  return GRAPH_SUCCESS;
}

std::unique_ptr<AttrGroupsBase> SymbolicDescAttr::Clone() {
  std::unique_ptr<AttrGroupsBase> attr = ComGraphMakeUnique<SymbolicDescAttr>(*this);
  GE_ASSERT_NOTNULL(attr);
  return attr;
}
}  // namespace ge
