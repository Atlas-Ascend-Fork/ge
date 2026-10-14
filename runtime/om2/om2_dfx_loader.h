/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef FRAMEWORK_RUNTIME_OM2_OM2_DFX_LOADER_H_
#define FRAMEWORK_RUNTIME_OM2_OM2_DFX_LOADER_H_

#include <cstdint>
#include "framework/common/ge_types.h"
#include "framework/runtime/dump/model_dump_c_api.h"

namespace gert {
// 存放 libom2_dump.so 中解析的符号
struct Om2DumpApi {
  // ---- dump 全局初始化 API ----
  int32_t (*dump_global_init)(void) = nullptr;
  int32_t (*parse_dump_config)(void) = nullptr;

  // ---- 句柄式 ModelDumpManager 生命周期/方法 API ----
  void *(*create_dump_manager)(uint32_t model_id) = nullptr;
  void (*destroy_dump_manager)(void *handle) = nullptr;
  void (*set_clear_dfx_cache_flag)(void *handle, uint8_t clear_cache) = nullptr;
  int32_t (*dispatch_dump_info)(void *handle) = nullptr;
  int32_t (*update_step_id)(void *handle, uint64_t step_id, void *stream) = nullptr;
  int32_t (*get_op_desc_info)(void *handle, uint32_t task_id, uint32_t stream_id, int32_t device_id,
                              ge::OpDescInfo *op_info) = nullptr;
  int32_t (*preprocess_om2_task_info)(void *handle, const GertModelTaskDesc *task_info) = nullptr;
  int32_t (*postprocess_om2_task_info)(void *handle, const GertModelTaskDesc *task_info) = nullptr;
  int32_t (*is_data_dump_enabled)(void *handle, const char *op_name, uint8_t *is_data_dump) = nullptr;
  int32_t (*set_model_dump_info)(void *handle, const ModelDumpInfo *info) = nullptr;

  void Reset() {
    dump_global_init = nullptr;
    parse_dump_config = nullptr;
    create_dump_manager = nullptr;
    destroy_dump_manager = nullptr;
    set_clear_dfx_cache_flag = nullptr;
    dispatch_dump_info = nullptr;
    update_step_id = nullptr;
    get_op_desc_info = nullptr;
    preprocess_om2_task_info = nullptr;
    postprocess_om2_task_info = nullptr;
    is_data_dump_enabled = nullptr;
    set_model_dump_info = nullptr;
  }

  ~Om2DumpApi() {
    Reset();
  }
};

// Om2DfxApi 是 libom2_executor.so 侧的 DFX 全局入口，负责加载 libom2_dump.so 并解析符号，对 dump 相关的函数进行包装
class Om2DfxApi {
 public:
  // VISIBILITY_EXPORT：供 libacl_mdl_impl_om2.so 等跨 .so 调用方访问（om2_executor 默认 -fvisibility=hidden）。
  // 获取 Om2DfxApi 单例，首次调用时调用 Initialize 完成 libom2_dump.so 的加载和符号解析
  VISIBILITY_EXPORT static Om2DfxApi &Instance();

  // GlobalInit 在 libacl_mdl_impl_om2.so 加载时被调用
  VISIBILITY_EXPORT int32_t GlobalInit(void);
  int32_t ParseDumpConfig(void);
  int32_t CreateDumpManager(uint32_t model_id, void **handle_ptr);
  void DestroyDumpManager(void *handle);
  void SetClearDfxCacheFlag(void *handle, uint8_t clear_cache);
  int32_t DispatchDumpInfo(void *handle);
  int32_t UpdateStepId(void *handle, uint64_t step_id, void *stream);
  // 返回 1 表示查询到算子信息，返回 0 表示未查询到或入参为 nullptr。
  int32_t GetOpDescInfo(void *handle, uint32_t task_id, uint32_t stream_id, int32_t device_id, ge::OpDescInfo *op_info);
  int32_t PreprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info);
  int32_t PostprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info);
  int32_t IsDataDumpEnabled(void *handle, const char *op_name, uint8_t *is_data_dump);
  int32_t SetModelDumpInfo(void *handle, const ModelDumpInfo *info);

 private:
  // 加载 libom2_dump.so 并解析符号
  void Initialize();
  ~Om2DfxApi();

  void *so_handle_ = nullptr;
  bool dump_so_exist = true;
  Om2DumpApi dump_api_;
};

}  // namespace gert

#endif  // FRAMEWORK_RUNTIME_OM2_OM2_DFX_LOADER_H_
