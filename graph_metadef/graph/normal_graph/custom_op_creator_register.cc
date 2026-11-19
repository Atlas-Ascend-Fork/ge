/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "graph/custom_op.h"

#include <memory>

#include "graph/custom_op_factory.h"
#include "graph/custom_op_load_context.h"
#include "graph/custom_op_pull_registry.h"

namespace ge {
namespace {
bool NeedRegisterLocalCreator(const OpRegistrationPriority priority, const OpEngine engine) {
  return (priority == OpRegistrationPriority::kTop) && (engine == OpEngine::kCustom);
}
}  // namespace

// .a 侧 1参 BaseOpCreator 链接锚：强引用触发 ld 提取本 .o 成员（含 pull ABI）
#ifdef CUSTOM_OP_REGISTRY_STATIC
CustomOpCreatorRegister::CustomOpCreatorRegister(const AscendString &operator_type, const BaseOpCreator &op_creator) {
  if (!op_creator || IsOfflineCustomOpSoLoading()) {
    return;
  }
  CustomOpFactory::RegisterCustomOpCreator(operator_type, op_creator);
}
#endif

CustomOpCreatorRegister::CustomOpCreatorRegister(const AscendString &operator_type, const CustomOpCreateFunc op_creator)
    : CustomOpCreatorRegister(operator_type, OpBackend::kDevice, op_creator) {}

CustomOpCreatorRegister::CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend,
                                                 const CustomOpCreateFunc op_creator)
    : CustomOpCreatorRegister(operator_type, backend, OpRegistrationPriority::kTop, OpEngine::kCustom, op_creator) {}

CustomOpCreatorRegister::CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend,
                                                 OpRegistrationPriority priority, OpEngine engine,
                                                 const CustomOpCreateFunc op_creator) {
  if (NeedRegisterLocalCreator(priority, engine)) {
    RegisterCustomOpLocalCreator(operator_type.GetString(), backend, op_creator);
  }
  if ((op_creator == nullptr) || IsOfflineCustomOpSoLoading()) {
    return;
  }
  CustomOpFactory::RegisterCustomOpCreator(
      operator_type, backend, priority, engine,
      [op_creator]() -> std::unique_ptr<BaseCustomOp> { return std::unique_ptr<BaseCustomOp>(op_creator()); });
}
}  // namespace ge
