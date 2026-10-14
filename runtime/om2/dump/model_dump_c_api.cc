/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/runtime/dump/model_dump_c_api.h"
#include "framework/runtime/dump/model_dump_manager.h"
#include "framework/common/debug/ge_log.h"

// =============================================================================
//  句柄式 C API 实现：供 libom2_executor.so 通过 dlopen 管理 ModelDumpManager 生命周期。
// =============================================================================

void *Om2DumpManagerCreate(uint32_t model_id) {
  auto *mgr = new (std::nothrow) ge::dump::ModelDumpManager(model_id);
  return static_cast<void *>(mgr);
}

void Om2DumpManagerDestroy(void *handle) {
  if (handle == nullptr) {
    return;
  }
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  delete mgr;
}

int32_t Om2DumpGlobalInit(void) {
  return static_cast<int32_t>(ge::dump::ModelDumpManager::GlobalInit());
}

int32_t Om2DumpManagerParseConfig(void) {
  return static_cast<int32_t>(ge::dump::ModelDumpManager::ParseDumpConfig());
}

void Om2DumpManagerSetClearDfxCacheFlag(void *handle, uint8_t clear_cache) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if (mgr == nullptr) {
    return;
  }
  mgr->SetClearDfxCacheFlagAfterLoad(clear_cache != 0U);
}

int32_t Om2DumpManagerDispatchDumpInfo(void *handle) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if (mgr == nullptr) {
    return static_cast<int32_t>(ge::FAILED);
  }
  return static_cast<int32_t>(mgr->DispatchDumpInfo());
}

int32_t Om2DumpManagerUpdateStepId(void *handle, uint64_t step_id, void *stream) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if (mgr == nullptr) {
    return static_cast<int32_t>(ge::FAILED);
  }
  return static_cast<int32_t>(mgr->UpdateStepId(step_id, stream));
}

int32_t Om2DumpManagerGetOpDescInfo(void *handle, uint32_t task_id, uint32_t stream_id, int32_t device_id,
                                    void *op_info) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  auto *op_desc_info = static_cast<ge::OpDescInfo *>(op_info);
  if ((mgr == nullptr) || (op_desc_info == nullptr)) {
    return 0;
  }
  return mgr->GetOpDescInfo(ge::OpDescInfoId(task_id, stream_id, device_id), *op_desc_info) ? 1 : 0;
}

int32_t Om2DumpManagerPreprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if ((mgr == nullptr) || (task_info == nullptr)) {
    return static_cast<int32_t>(ge::FAILED);
  }
  return static_cast<int32_t>(mgr->PreprocessOm2TaskInfo(*task_info));
}

int32_t Om2DumpManagerPostprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if ((mgr == nullptr) || (task_info == nullptr)) {
    return static_cast<int32_t>(ge::FAILED);
  }
  return static_cast<int32_t>(mgr->PostprocessOm2TaskInfo(*task_info));
}

int32_t Om2DumpManagerIsDataDumpEnabled(void *handle, const char *op_name, uint8_t *is_data_dump) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if ((mgr == nullptr) || (is_data_dump == nullptr)) {
    return static_cast<int32_t>(ge::FAILED);
  }
  return static_cast<int32_t>(mgr->IsDataDumpEnabled(op_name, is_data_dump));
}

int32_t Om2DumpManagerSetModelDumpInfo(void *handle, const ModelDumpInfo *info) {
  auto *mgr = static_cast<ge::dump::ModelDumpManager *>(handle);
  if ((mgr == nullptr) || (info == nullptr)) {
    return static_cast<int32_t>(ge::FAILED);
  }
  ge::dump::ModelDumpInfo model_dump_info;
  model_dump_info.model_id = info->model_id;
  model_dump_info.model_name = info->model_name;
  model_dump_info.root_graph_name = info->root_graph_name;
  model_dump_info.device_id = info->device_id;
  model_dump_info.rt_model_handle = info->rt_model_handle;
  model_dump_info.step_id_addr = info->step_id_addr;
  model_dump_info.loop_cond_addr = info->loop_cond_addr;
  model_dump_info.iterations_per_loop_addr = info->iterations_per_loop_addr;
  return static_cast<int32_t>(mgr->SetModelDumpInfo(model_dump_info));
}
