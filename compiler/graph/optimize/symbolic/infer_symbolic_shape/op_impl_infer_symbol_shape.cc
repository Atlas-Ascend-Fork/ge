/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "op_impl_infer_symbol_shape.h"
#include "framework/common/debug/ge_log.h"

namespace gert {
OpImplInferSymbolShape &OpImplInferSymbolShape::InferSymbolShape(OpImplKernelRegistry::InferSymbolKernelFunc func) {
  auto &registered = OpImplInferSymbolShapeRegistry::GetInstance().types_to_impl_[op_type];
  GELOGI("Register infer symbol shape for %s.", op_type.GetString());
  if (registered != nullptr) {
    GELOGW("[%s] infer_symbol_shape already registered, will using new function.", op_type.GetString());
  }
  registered = func;
  return *this;
}

OpImplInferSymbolShapeRegistry &OpImplInferSymbolShapeRegistry::GetInstance() {
  static OpImplInferSymbolShapeRegistry instance;
  return instance;
}

OpImplKernelRegistry::InferSymbolKernelFunc OpImplInferSymbolShapeRegistry::GetOpImpl(const ge::char_t *op_type) const {
  const auto iter = types_to_impl_.find(op_type);
  if (iter == types_to_impl_.cend()) {
    GELOGW("[%s] infer_symbol_shape is not registered, get the func failed.", op_type);
    return nullptr;
  }
  return iter->second;
}

}  // namespace gert
