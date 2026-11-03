/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/runtime/dump/model_dump_manager.h"
#include "framework/runtime/dump/dump_config.h"
#include "framework/runtime/dump/dump_callback_manager.h"
#include "framework/runtime/dump/data_dump_impl.h"
#include "framework/runtime/dump/exception_dump_impl.h"
#include "framework/runtime/dump/overflow_dump_impl.h"
#include "framework/runtime/dump/dump_op_impl.h"
#include "common/checker.h"
#include "framework/common/debug/ge_log.h"

namespace ge {
namespace dump {
Status ModelDumpManager::GlobalInit() {
  GELOGD("ModelDumpManager::GlobalInit start");
  DumpConfig::Instance().Reset();
  GE_ASSERT_SUCCESS(DumpCallbackManager::GlobalInit());
  return SUCCESS;
}

Status ModelDumpManager::ParseDumpConfig() {
  GELOGD("ModelDumpManager::ParseDumpConfig start");
  DumpConfig::Instance().Reset();
  return DumpCallbackManager::GetInstance().ParseDumpConfig();
}

ModelDumpManager::ModelDumpManager(uint32_t model_id) : model_id_(model_id) {
  GELOGD("ModelDumpManager constructed, model_id=%u", model_id);

  data_dump_impl_ = std::make_unique<DataDumpImpl>();
  exception_impl_ = std::make_unique<ExceptionDumpImpl>();
  overflow_impl_ = std::make_unique<OverflowDumpImpl>();
  dump_op_impl_ = std::make_unique<DumpOpImpl>();
  dump_op_range_state_ = std::make_unique<DumpOpRangeState>();
}

ModelDumpManager::~ModelDumpManager() {
  Clear();
  GELOGD("ModelDumpManager destructed, model_id=%u", model_id_);
}

Status ModelDumpManager::SetModelDumpInfo(const ModelDumpInfo &model_info) {
  const char *model_name = (model_info.model_name != nullptr) ? model_info.model_name : "";
  GELOGD("SetModelDumpInfo: model_id=%u, model_name=%s", model_info.model_id, model_name);
  model_info_ = model_info;
  exception_impl_->SetDeviceId(model_info.device_id);
  const char *root_graph_name = (model_info.root_graph_name != nullptr) ? model_info.root_graph_name : "";
  // 模型加载开始时重置 opname_range 状态机（对齐 v1 ModelArgsManager::ParseModelTaskDef 加载前 ResetDumpFsmState）
  dump_op_range_state_->Reset(model_name, root_graph_name);

  // 如果 Overflow 开关开启，自动注册
  if (DumpConfig::Instance().IsOverflowDumpEnabled()) {
    Status ret = overflow_impl_->RegisterForModel(model_info.rt_model_handle);
    if (ret != SUCCESS) {
      GELOGE(ret, "Overflow register failed for model_id=%u", model_info.model_id);
      return ret;
    }
    // 将 overflow debug 信息传递给 data_dump_impl，用于构建 Opdebug 算子
    data_dump_impl_->SetOpDebugInfo(overflow_impl_->GetOpDebugTaskId(), overflow_impl_->GetOpDebugStreamId(),
                                    overflow_impl_->GetOpDebugAddr());
    GELOGD("Set overflow debug info: task_id=%u, stream_id=%u, addr=%p", overflow_impl_->GetOpDebugTaskId(),
           overflow_impl_->GetOpDebugStreamId(), overflow_impl_->GetOpDebugAddr());
  }

  return SUCCESS;
}

void ModelDumpManager::SetClearDfxCacheFlagAfterLoad(bool clear_cache) {
  GELOGI("set clear_cache %d", static_cast<int32_t>(clear_cache));
  need_clear_dfx_cache_ = clear_cache;
}

Status ModelDumpManager::IsDataDumpEnabled(const char *op_name, uint8_t *is_data_dump) const {
  if (is_data_dump == nullptr) {
    GELOGW("is_data_dump is null, skip");
    return PARAM_INVALID;
  }

  const char *safe_op_name = (op_name != nullptr) ? op_name : "";
  const char *model_name = (model_info_.model_name != nullptr) ? model_info_.model_name : "";
  const char *root_graph_name = (model_info_.root_graph_name != nullptr) ? model_info_.root_graph_name : "";

  // is_data_dump 是纯 data dump 相关的，只判断：
  // 1. data dump 开关已启用
  // 2. 算子在 dump layer/list 配置中，或命中 opname_range 状态机（对齐 v1 DavinciModel::OpNeedDump）
  // 3. watcher 模式下的 layer 算子（对齐 v1 DavinciModel::OpNeedSetDumpFlagOnWatcherModel：
  //    layer 算子自身不落数据，但要打上 dump 标志，其 task_id 才会触发 <layerOp>_To_<watcherNode> 落盘）
  // data dump 是否生效需要同时满足全局开关和当前模型/op 命中 dump_list，避免 model_name 不匹配时误 dump。
  // 对无 Preprocess 回调的 task（如 DSA），launch 前查询时状态机尚未推进到本 task，
  // 需按当前状态预演该 op 的转移，避免 range 判定早于状态迁移导致漏置 dump 标志。
  const bool in_dump_range = dump_op_range_state_->IsInDumpOpRange(safe_op_name) ||
                             dump_op_range_state_->IsOpInDumpRangeAtCurrentState(safe_op_name);
  const bool need_data_dump =
      DumpConfig::Instance().IsDataDumpEnabled() &&
      (DumpConfig::Instance().IsOpNeedDump(model_name, root_graph_name, safe_op_name) || in_dump_range ||
       DumpConfig::Instance().IsLayerOpOnWatcherMode(model_name, root_graph_name, safe_op_name));

  GELOGD("IsDataDumpEnabled: op_name=%s, need_data_dump=%u", safe_op_name, static_cast<uint32_t>(need_data_dump));
  *is_data_dump = need_data_dump ? 1U : 0U;
  return SUCCESS;
}

// 按算子判定的纯 data dump 条件：全局 data dump 开关 && (算子在 dump_list / 命中 opname_range)。
// 与 overflow 全局开关解耦，避免"data dump + overflow 同时使能"时把仅 overflow 命中的算子误判为 data dump。
bool ModelDumpManager::IsTaskNeedDataDump(const GertModelTaskDesc &task_info) const {
  const char *op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";
  const char *model_name = (model_info_.model_name != nullptr) ? model_info_.model_name : "";
  const char *root_graph_name = (model_info_.root_graph_name != nullptr) ? model_info_.root_graph_name : "";
  return DumpConfig::Instance().IsDataDumpEnabled() &&
         (DumpConfig::Instance().IsOpNeedDump(model_name, root_graph_name, op_name) ||
          dump_op_range_state_->IsInDumpOpRange(op_name));
}

bool ModelDumpManager::NeedDataDump(const GertModelTaskDesc &task_info) const {
  const char *op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";

  GELOGD("NeedDataDump: op_name=%s, task_id=%u, stream_id=%u", op_name, task_info.task_id, task_info.stream_id);

  // 判断该算子是否需要保存到 data dump：
  // 1. 配置了 data dump 且算子在列表中，或命中 opname_range 状态机（对齐 v1 DavinciModel::OpNeedDump）
  // 2. 开启了 overflow dump（所有算子都需要保存用于定位）
  // data dump 是否生效需要同时满足全局开关和当前模型/op 命中 dump_list，避免 model_name 不匹配时误保存任务。
  const bool need_data_dump = IsTaskNeedDataDump(task_info);
  const bool need_overflow_dump = DumpConfig::Instance().IsOverflowDumpEnabled();
  const bool need_save_to_data_dump = need_data_dump || need_overflow_dump;

  GELOGD(
      "PreprocessOm2TaskInfo: op_name=%s, need_data_dump=%u, need_overflow_dump=%u, "
      "need_save_to_data_dump=%u",
      op_name, need_data_dump, need_overflow_dump, need_save_to_data_dump);

  return need_save_to_data_dump;
}

Status ModelDumpManager::PreprocessOm2TaskInfo(const GertModelTaskDesc &task_info) {
  const char *op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";
  GELOGD("PreprocessOm2TaskInfo: op_name=%s, task_id=%u, stream_id=%u", op_name, task_info.task_id,
         task_info.stream_id);

  // 按 task 下发顺序推进 opname_range 状态机（对齐 v1 ModelArgsManager 加载期 SetDumpFsmState）
  dump_op_range_state_->Update(task_info);

  const bool need_save_to_data_dump = NeedDataDump(task_info);
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_input = (dump_mode == GE_DUMP_MODE_INPUT) || (dump_mode == GE_DUMP_MODE_ALL);

  GELOGD("PreprocessOm2TaskInfo: op_name=%s, need_save_to_data_dump=%u, need_dump_input=%u", op_name,
         need_save_to_data_dump, need_dump_input);

  ModelTaskType type = static_cast<ModelTaskType>(task_info.task_type);
  // dump custom/hccl op input（HCCL 通信任务无常规 kernel args，走 DumpOp
  // 通道：hccl_load_task 前插 input dump op）
  if (need_dump_input && need_save_to_data_dump &&
      ((type == ModelTaskType::MODEL_TASK_CUSTOM_KERNEL) || (type == ModelTaskType::MODEL_TASK_HCCL))) {
    // 仅 data dump 落盘自定义算子张量；overflow dump 由 AICPU 依据 Opdebug 信息在溢出时落盘，
    // 不能仅凭溢出开关开启就落盘，否则无溢出时也会产生 dump。此处按算子判定是否命中 data dump，
    // 避免"data dump + overflow 同时使能"时，仅 overflow 命中的算子也被落盘。
    if (!IsTaskNeedDataDump(task_info)) {
      GELOGI("[OM2 Dump] op %s is not in data dump config, skip custom/hccl op input dump.", op_name);
      return SUCCESS;
    }
    auto &dump_op = dump_op_impl_->GetInputDumpOp(task_info.op_name);
    if (dump_op->IsProtoVer()) {
      GE_CHK_STATUS_RET(data_dump_impl_->BuildOpMappingBasicInfo(model_info_, dump_op->GetOpMappingInfo()));
    } else {
      GE_CHK_STATUS_RET(data_dump_impl_->BuildDumpTransportBasicInfo(model_info_, dump_op->GetDumpTransportInfo()));
    }
    const char *cur_model_name = (model_info_.model_name != nullptr) ? model_info_.model_name : "";
    const char *cur_root_graph_name = (model_info_.root_graph_name != nullptr) ? model_info_.root_graph_name : "";
    GE_CHK_STATUS_RET(dump_op->BuildTaskInputs(task_info, cur_model_name, cur_root_graph_name));
    GE_CHK_STATUS_RET(dump_op->ExecutorDumpOp(task_info.op_name, task_info.stream));

    return SUCCESS;
  }

  if (task_info.task_raw_info == nullptr) {
    return SUCCESS;
  }

  Status ret = exception_impl_->ReportL0ExceptionDumpInfo(task_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "Report L0 exception dump info failed, op_name=%s", op_name);
    return ret;
  }
  return SUCCESS;
}

Status ModelDumpManager::PostprocessOm2TaskInfo(const GertModelTaskDesc &task_info) {
  const char *op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";
  GELOGD("PostprocessOm2TaskInfo: op_name=%s, task_id=%u, stream_id=%u", op_name, task_info.task_id,
         task_info.stream_id);

  // 按 task 下发顺序推进 opname_range 状态机（与 Preprocess 按 task_info 指针去重；
  // DSA 等无 Preprocess 回调的 task 由此处推进，其 launch 前的 IsDataDumpEnabled 判定可能早于本 task 的状态迁移）
  dump_op_range_state_->Update(task_info);

  const bool need_save_to_data_dump = NeedDataDump(task_info);
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_output = (dump_mode == GE_DUMP_MODE_OUTPUT) || (dump_mode == GE_DUMP_MODE_ALL);

  GELOGD("PostprocessOm2TaskInfo: op_name=%s, need_save_to_data_dump=%u, need_dump_output=%u", op_name,
         need_save_to_data_dump, need_dump_output);

  // watcher 模式：非 watcher 算子只记录 task/stream 信息，供 watcher 节点生成
  // "<layerOpName>_To_<watcherNode>" 的 dump task（对齐 v1 DavinciModel::OpNoNeedDumpOnWatcherModel
  // + DataDumper::SaveLayerOpInfoOnWatcherMode）
  const char *model_name = (model_info_.model_name != nullptr) ? model_info_.model_name : "";
  const char *root_graph_name = (model_info_.root_graph_name != nullptr) ? model_info_.root_graph_name : "";
  if (DumpConfig::Instance().IsDataDumpEnabled() && op_name[0] != '\0' &&
      !DumpConfig::Instance().IsWatcherNode(model_name, root_graph_name, op_name) &&
      DumpConfig::Instance().IsLayerOpOnWatcherMode(model_name, root_graph_name, op_name)) {
    data_dump_impl_->SaveLayerOpInfo(task_info.task_id, task_info.stream_id, op_name);
  }

  // task_type 类型转换
  ModelTaskType type = static_cast<ModelTaskType>(task_info.task_type);
  // dump custom/hccl op output（hccl_load_task 后插 output dump op）
  if (need_dump_output && need_save_to_data_dump &&
      ((type == ModelTaskType::MODEL_TASK_CUSTOM_KERNEL) || (type == ModelTaskType::MODEL_TASK_HCCL))) {
    // 仅 data dump 落盘自定义算子张量；overflow dump 由 AICPU 依据 Opdebug 信息在溢出时落盘，
    // 不能仅凭溢出开关开启就落盘，否则无溢出时也会产生 dump。此处按算子判定是否命中 data dump，
    // 避免"data dump + overflow 同时使能"时，仅 overflow 命中的算子也被落盘。
    if (!IsTaskNeedDataDump(task_info)) {
      GELOGI("[OM2 Dump] op %s is not in data dump config, skip custom/hccl op output dump.", op_name);
      return SUCCESS;
    }
    auto &dump_op = dump_op_impl_->GetOutputDumpOp(task_info.op_name);
    if (dump_op->IsProtoVer()) {
      GE_CHK_STATUS_RET(data_dump_impl_->BuildOpMappingBasicInfo(model_info_, dump_op->GetOpMappingInfo()));
    } else {
      GE_CHK_STATUS_RET(data_dump_impl_->BuildDumpTransportBasicInfo(model_info_, dump_op->GetDumpTransportInfo()));
    }
    const char *cur_model_name = (model_info_.model_name != nullptr) ? model_info_.model_name : "";
    const char *cur_root_graph_name = (model_info_.root_graph_name != nullptr) ? model_info_.root_graph_name : "";
    GE_CHK_STATUS_RET(dump_op->BuildTaskOutputs(task_info, cur_model_name, cur_root_graph_name));
    GE_CHK_STATUS_RET(dump_op->ExecutorDumpOp(task_info.op_name, task_info.stream));

    return SUCCESS;
  }

  // Data Dump / Overflow Dump：保存 Task 信息到 AICPU（HCCL 的 input/output dump 已走 DumpOp 通道，跳过通用
  // SaveTask 避免重复下发；溢出任务信息另经各 task_info/日志通道保留）
  // Overflow dump 需要保存算子的输入输出信息用于问题定位
  if (need_save_to_data_dump && (type != ModelTaskType::MODEL_TASK_HCCL)) {
    Status ret = data_dump_impl_->SaveTask(task_info, type, task_info.stream, overflow_impl_->IsOpDebugEnabled());
    if (ret != SUCCESS) {
      GELOGE(ret, "Save task dump info failed, op_name=%s", op_name);
      return ret;
    }
  }

  const bool need_save_exception_info = DumpConfig::Instance().IsExceptionDumpEnabled() || !need_clear_dfx_cache_;
  if (need_save_exception_info) {
    Status ret = exception_impl_->SaveOpInfo(task_info);
    if (ret != SUCCESS) {
      GELOGE(ret, "Save task exception info failed, op_name=%s", op_name);
      return ret;
    }
  }

  return SUCCESS;
}

Status ModelDumpManager::DispatchDumpInfo() {
  GELOGD("DispatchDumpInfo: model_id=%u", model_id_);

  // Exception dump 优先级最高，不需要下发 data dump 信息
  if (DumpConfig::Instance().IsExceptionDumpEnabled()) {
    GELOGD("Exception dump enabled, skip data dump dispatch");
    return SUCCESS;
  }

  // Data dump 或 Overflow dump：按 OM2_DUMP_DATA_VERSION 构建 protobuf OpMapping 或 DumpTransport TLV 并下发到 AICPU
  // Overflow dump 虽然走 Adump 路径写入数据，但仍需下发 Opdebug 算子的信息给 AICPU
  if (DumpConfig::Instance().IsDataDumpEnabled() || DumpConfig::Instance().IsOverflowDumpEnabled()) {
    if (data_dump_impl_->IsProtoVer()) {
      return data_dump_impl_->BuildAndLoadOpMappingInfo(model_info_);
    }
    return data_dump_impl_->BuildAndLoadDumpTransportInfo(model_info_);
  }

  return SUCCESS;
}

Status ModelDumpManager::UpdateStepId(uint64_t step_id, aclrtStream stream) {
  if (data_dump_impl_ == nullptr) {
    return SUCCESS;
  }
  return data_dump_impl_->UpdateStepId(step_id, stream);
}

bool ModelDumpManager::GetOpDescInfo(const OpDescInfoId &op_id, OpDescInfo &op_info) const {
  return exception_impl_->GetOpDescInfo(op_id, op_info);
}

void ModelDumpManager::Clear() {
  GELOGD("Clear: model_id=%u", model_id_);

  if (data_dump_impl_ != nullptr) {
    data_dump_impl_->Clear();
  }
  if (exception_impl_ != nullptr) {
    exception_impl_->Clear();
  }
  if (overflow_impl_ != nullptr && model_info_.rt_model_handle != nullptr) {
    overflow_impl_->UnregisterForModel(model_info_.rt_model_handle);
  }
}

}  // namespace dump
}  // namespace ge
