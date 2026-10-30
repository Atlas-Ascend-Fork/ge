/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "engines/custom_engine/custom_ops_kernel_info_store.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "common/ge_common/ge_types.h"
#include "common/checker.h"
#include "graph/custom_op_factory.h"
#include "graph/ascend_string.h"

namespace ge {
namespace custom {
namespace {
// 自定义算子注册引擎（OpEngine）-> 目标DNNEngine名称；未列出的仍落到kEngineNameCustom。
const std::map<OpEngine, std::string> kCustomOpRegisteredEngineToDnnEngine = {
    {OpEngine::kHccl, kEngineNameDnnHccl},
};
}  // namespace

Status CustomOpsKernelInfoStore::UpdateOpInfoMap() {
  OpInfo default_op_info = {.engine = kEngineNameCustom,
                            .opKernelLib = kCustomOpKernelLibName,
                            .computeCost = 0,
                            .flagPartial = false,
                            .flagAsync = false,
                            .isAtomic = false,
                            .opFileName = "",
                            .opFuncName = ""};
  std::vector<AscendString> all_ops_types;
  GE_ASSERT_SUCCESS(CustomOpFactory::GetAllRegisteredOps(all_ops_types));
  {
    std::lock_guard<std::mutex> lock(mu_);
    op_info_map_.clear();
    for (auto &op_type : all_ops_types) {
      OpInfo op_info = default_op_info;
      OpEngine engine = OpEngine::kCustom;
      // 认领口径与"配对实例化链"严格一致（且与既有语义对齐）：
      // - kCustom引擎：仅认领kTop注册（认领口径以(kTop, kCustom)为准，与守卫语义一致；
      //   仅kBottom注册的kCustom算子不认领，避免与守卫/打包口径不一致）；
      // - 映射表内引擎（如kHccl）：任意优先级均认领（kBottom兜底注册需生效），运行时经引擎解析取实例；
      // - 仅kHostCPU注册：按 (kTop, kCustom, kHostCPU) 精确认领，与host CPU自定义算子实例化一致；
      // - 其余引擎/优先级组合（框架/插件以非自定义引擎注册）：不认领，避免被错误路由到自定义kernel lib。
      const bool has_device_regist = CustomOpFactory::GetOpEngine(op_type, OpBackend::kDevice, engine);
      const auto mapped_engine = kCustomOpRegisteredEngineToDnnEngine.find(engine);
      const bool device_claimable =
          has_device_regist && (((engine == OpEngine::kCustom) &&
                                 (CustomOpFactory::IsExistOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop,
                                                             OpEngine::kCustom))) ||
                                (mapped_engine != kCustomOpRegisteredEngineToDnnEngine.cend()));
      const bool host_claimable =
          CustomOpFactory::IsExistOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kCustom);
      if ((!device_claimable) && (!host_claimable)) {
        continue;
      }
      if (has_device_regist && (mapped_engine != kCustomOpRegisteredEngineToDnnEngine.cend())) {
        // 按注册引擎映射到目标DNNEngine（如HCCL自定义算子落到DNN_HCCL，与内置集合通信算子一致，
        // 以复用独立流等流分配特性）；kernel lib仍为custom store，由自定义算子编译/执行链路处理。
        op_info.engine = mapped_engine->second;
        GELOGI("Route custom op %s (registered engine %u) to DNN engine %s.", op_type.GetString(),
               static_cast<uint32_t>(engine), mapped_engine->second.c_str());
      }
      op_info_map_[std::string(op_type.GetString())] = op_info;
    }
  }
  return SUCCESS;
}

Status CustomOpsKernelInfoStore::Initialize(const std::map<std::string, std::string> &options) {
  (void)options;
  GE_ASSERT_SUCCESS(UpdateOpInfoMap());
  GELOGI("CustomOpsKernelInfoStore initialized, op info num: %zu", op_info_map_.size());
  return SUCCESS;
}

Status CustomOpsKernelInfoStore::Finalize() {
  std::lock_guard<std::mutex> lock(mu_);
  op_info_map_.clear();
  return SUCCESS;
}

Status CustomOpsKernelInfoStore::Refresh() {
  GE_ASSERT_SUCCESS(UpdateOpInfoMap());
  GELOGI("CustomOpsKernelInfoStore refreshed, op info num: %zu", op_info_map_.size());
  return SUCCESS;
}

void CustomOpsKernelInfoStore::GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const {
  std::lock_guard<std::mutex> lock(mu_);
  infos = op_info_map_;
}

bool CustomOpsKernelInfoStore::CheckSupported(const OpDescPtr &op_desc, std::string &reason) const {
  (void)reason;
  GE_ASSERT_NOTNULL(op_desc);
  // 认领语义：kDevice backend任一优先级、任一引擎有注册即支持（kHccl形态的kBottom注册同样命中）
  return CustomOpFactory::IsExistOp(AscendString(op_desc->GetTypePtr()), OpBackend::kDevice);
}
}  // namespace custom
}  // namespace ge
