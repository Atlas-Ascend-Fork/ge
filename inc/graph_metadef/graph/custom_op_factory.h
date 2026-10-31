/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_H
#define CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_H
#include <memory>
#include <type_traits>
#include <vector>

#include "graph/custom_op.h"
#include "graph/custom_op/args_refresh.h"
#include "graph/custom_op/cast.h"
#include "graph/custom_op_registry.h"
#include "graph/ascend_string.h"
#include "graph/error_codes.h"

namespace ge {
class CustomOpFactory {
 public:
  static graphStatus RegisterCustomOpCreator(const AscendString &op_type, const BaseOpCreator &op_creator);
  static graphStatus RegisterCustomOpCreator(const AscendString &op_type, OpBackend backend,
                                             const BaseOpCreator &op_creator);
  static graphStatus RegisterCustomOpCreator(const AscendString &op_type, OpBackend backend,
                                             OpRegistrationPriority priority, OpEngine engine,
                                             const BaseOpCreator &op_creator);

  static BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type);
  static BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend);
  // 按优先级从高到低（kTop -> kBottom）在指定engine/backend下解析注册项并取实例
  static BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend, OpEngine engine);
  static BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend,
                                           OpRegistrationPriority priority, OpEngine engine);

  template <typename T>
  static T *GetCustomOpCommonCapability(const AscendString &op_type) {
    static_assert(std::is_same<T, ShapeInferOp>::value || std::is_same<T, CustomOpInferMetaProvider>::value ||
                      std::is_same<T, PortableOp>::value || std::is_same<T, CompilableOp>::value,
                  "GetCustomOpCommonCapability only supports ShapeInferOp, CustomOpInferMetaProvider, PortableOp "
                  "and CompilableOp");
    auto *const custom_op =
        GetGlobalRegistry().GetCustomOpCommonCapability(op_type, CustomOpCapabilityTrait<T>::kCapability);
    return CustomOpCast<T>(custom_op);
  }

  template <typename T>
  static T *GetCustomOpCommonCapability(const AscendString &op_type, OpRegistrationPriority priority) {
    static_assert(std::is_same<T, ShapeInferOp>::value || std::is_same<T, CustomOpInferMetaProvider>::value ||
                      std::is_same<T, PortableOp>::value || std::is_same<T, CompilableOp>::value,
                  "GetCustomOpCommonCapability only supports ShapeInferOp, CustomOpInferMetaProvider, PortableOp "
                  "and CompilableOp");
    auto *const custom_op =
        GetGlobalRegistry().GetCustomOpCommonCapability(op_type, CustomOpCapabilityTrait<T>::kCapability, priority);
    return CustomOpCast<T>(custom_op);
  }

  static void RemoveCustomOps(const std::vector<AscendString> &op_types);

  static CustomOpRegistryPtr GetGlobalRegistryPtr();

  static graphStatus GetAllRegisteredOps(std::vector<AscendString> &all_registered_ops);

  // 注册项查询约定：带优先级形参的按该优先级过滤；不带优先级形参即覆盖全部优先级。
  // - 守卫口径（带优先级形参，按该优先级过滤）：指定优先级槽位已被占用即返回true；
  //   防重注册/防冲突使用（kBottom兜底注册不占用kTop槽位）
  static bool IsExistOp(const AscendString &op_type, OpRegistrationPriority priority);
  static bool IsExistOp(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority);
  // - 精确到引擎（任意backend）：按 (priority, engine) 过滤
  static bool IsExistOp(const AscendString &op_type, OpRegistrationPriority priority, OpEngine engine);
  // - 精确口径（按 (backend, priority, engine) 精确过滤）
  static bool IsExistOp(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority,
                        OpEngine engine);
  // - 认领口径（不带优先级形参即覆盖全部优先级）：任意优先级（kTop/kBottom）存在注册即返回true；
  //   OM打包收集等使用
  static bool IsExistOp(const AscendString &op_type);
  static bool IsExistOp(const AscendString &op_type, OpBackend backend);

  static graphStatus LoadCustomOpsPartition(const uint8_t *data, size_t len);

  static bool IsAddressRefreshable(const AscendString &op_type);
  static ArgsRefreshStrategy GetArgsRefreshStrategy(const AscendString &op_type);
  // 查询op在指定backend下注册的引擎（同一优先级内命中任一引擎即返回）
  static bool GetOpEngine(const AscendString &op_type, OpBackend backend, OpEngine &engine);

 private:
  static CustomOpRegistry &GetGlobalRegistry();
};

}  // namespace ge
#endif  // CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_H
