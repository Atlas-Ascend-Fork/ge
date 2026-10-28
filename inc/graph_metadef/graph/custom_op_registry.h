/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_CORE_H
#define CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_CORE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "graph/ascend_string.h"
#include "graph/custom_op/args_refresh.h"
#include "graph/custom_op/capability.h"
#include "graph/error_codes.h"
#include "graph/custom_op/op_proto_ledger.h"

namespace ge {
class BaseCustomOp;
enum class OpBackend : uint32_t;
enum class OpRegistrationPriority : uint32_t;
enum class OpEngine : uint32_t;
using BaseOpCreator = std::function<std::unique_ptr<BaseCustomOp>()>;

class CustomOpSoHandle;
using CustomOpSoHandlePtr = std::shared_ptr<CustomOpSoHandle>;

class CustomOpRegistry {
 public:
  ~CustomOpRegistry();

  graphStatus RegisterCreator(const AscendString &op_type, OpBackend backend, const BaseOpCreator &creator);
  graphStatus RegisterCreator(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority,
                              OpEngine engine, const BaseOpCreator &creator);
  void AddSoHandles(const std::vector<CustomOpSoHandlePtr> &so_handles);

  BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend);
  // 按优先级从高到低（kTop -> kBottom）在指定engine/backend下解析注册项并取实例，kBottom兜底注册可命中
  BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend, OpEngine engine);
  BaseCustomOp *CreateOrGetCustomOp(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority,
                                    OpEngine engine);
  // 优先级从高到低逐级回退：高优先级存在provider（含冲突报错）即终止，无provider才看低优先级
  BaseCustomOp *GetCustomOpCommonCapability(const AscendString &op_type, CustomOpCapability capability);
  BaseCustomOp *GetCustomOpCommonCapability(const AscendString &op_type, CustomOpCapability capability,
                                            OpRegistrationPriority priority);
  BaseCustomOp *GetAnyCustomOpInstance(const AscendString &op_type);
  void RemoveCustomOps(const std::vector<AscendString> &op_types);
  ArgsRefreshStrategy GetArgsRefreshStrategy(const AscendString &op_type);
  bool IsAddressRefreshable(const AscendString &op_type);
  // 查询op在指定backend下注册的引擎（同一优先级内命中任一引擎即返回）
  bool GetOpEngine(const AscendString &op_type, OpBackend backend, OpEngine &engine) const;
  // 注册项查询约定：带优先级形参的按该优先级过滤；不带优先级形参即覆盖全部优先级。
  // - 守卫口径（带优先级形参，按该优先级过滤）：指定优先级槽位已被占用即返回true，
  //   用于防重注册/防冲突判断。kBottom兜底注册不占用kTop槽位（存在kTop注册时让位），故守卫查询不可见
  bool HasCreator(const AscendString &op_type, OpRegistrationPriority priority) const;
  bool HasCreator(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority) const;
  // - 精确到引擎（任意backend）：按 (priority, engine) 过滤；用于等价表达默认槽位查询（kTop + kCustom）
  bool HasCreator(const AscendString &op_type, OpRegistrationPriority priority, OpEngine engine) const;
  // - 精确口径（按 (backend, priority, engine) 精确过滤）
  bool HasCreator(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority,
                  OpEngine engine) const;
  // - 认领口径（不带优先级形参即覆盖全部优先级）：任意优先级（kTop/kBottom）、任意引擎、
  //   任意backend存在注册即返回true。供OM打包收集、内核编译任务收集、序列化收集等
  //   "是否由自定义算子链路承载"的判断使用
  bool HasCreator(const AscendString &op_type) const;
  bool HasCreator(const AscendString &op_type, OpBackend backend) const;
  // 实例口径（任意优先级）：已有实例即返回true（反序列化完成的判据）；
  // 与HasCreator（注册项口径）不同：一个查已创建实例，一个查注册项
  bool HasCustomOp(const AscendString &op_type) const;
  // 自定义算子承载/收集口径（OM打包、so-in-om、序列化收集与反序列化校验、自定义算子编译任务收集共用）：
  // 按引擎维度匹配，与 custom store 认领口径一致：kCustom 引擎仅 kTop；映射引擎（kHccl→DNN_HCCL）任意优先级；
  // 框架内置(kBottom, OpEngine::kHostCpu, kHostCPU)常量折叠通道不在此口径内，不收集
  // （其实现由CANN安装目录的常量折叠库在编译期装载，不属于OM自包含的用户so打包范畴）。
  bool IsCustomOpToBeCollected(const AscendString &op_type) const;
  // 认领清单语义：收集所有优先级（kTop/kBottom）下注册的算子；守卫判断请用带优先级形参的HasCreator
  graphStatus GetAllRegisteredOps(std::vector<AscendString> &all_registered_ops) const;
  graphStatus LoadCustomOpsPartition(const uint8_t *data, size_t len);
  void AppendProtoClaims(std::vector<OpProtoClaimRecord> &&claims);

 private:
  struct Impl;
  BaseCustomOp *CreateOrGetCustomOpLocked(const AscendString &op_type, OpBackend backend,
                                          OpRegistrationPriority priority, OpEngine engine);
  BaseCustomOp *CacheCustomOpLocked(const AscendString &op_type, OpBackend backend, OpRegistrationPriority priority,
                                    OpEngine engine, std::unique_ptr<BaseCustomOp> base_custom_op);

  mutable std::mutex mu_;
  std::vector<CustomOpSoHandlePtr> so_handles_;
  using BackendCreatorMap = std::map<OpBackend, BaseOpCreator>;
  using EngineCreatorMap = std::map<OpEngine, BackendCreatorMap>;
  using PriorityCreatorMap = std::map<OpRegistrationPriority, EngineCreatorMap>;
  using BackendInstanceMap = std::map<OpBackend, std::shared_ptr<BaseCustomOp>>;
  using EngineInstanceMap = std::map<OpEngine, BackendInstanceMap>;
  using PriorityInstanceMap = std::map<OpRegistrationPriority, EngineInstanceMap>;
  // 扫描单一优先级内的公共能力provider：命中返回实例；冲突返回空且has_provider为真；无provider两者皆空
  BaseCustomOp *ScanCommonCapabilityLocked(const AscendString &op_type, CustomOpCapability capability,
                                           OpRegistrationPriority priority, const EngineCreatorMap &engine_creators,
                                           bool &has_provider);
  // 调用方需已持有mu_；返回nullptr表示capability非法或算子未注册（已记日志）
  const PriorityCreatorMap *FindCreatorsForCapabilityLocked(const AscendString &op_type,
                                                            CustomOpCapability capability) const;
  std::map<AscendString, PriorityCreatorMap> creators_;
  std::map<AscendString, PriorityInstanceMap> custom_ops_;
  std::shared_ptr<Impl> impl_;
};

using CustomOpRegistryPtr = std::shared_ptr<CustomOpRegistry>;

}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_CUSTOM_OP_REGISTRY_CORE_H
