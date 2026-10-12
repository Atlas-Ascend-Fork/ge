/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_DUMP_MODEL_DUMP_C_API_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_MODEL_DUMP_C_API_H_

#include <cstddef>
#include <cstdint>
#include "framework/common/ge_visibility.h"
#include "framework/common/ge_types.h"
#include "framework/om2/model_api/om2_model_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
//  以下接口为 libom2_dump.so 的句柄式 C API，供 libom2_executor.so 通过 dlopen
//  动态加载并管理 ModelDumpManager 生命周期使用。libom2_dump.so 不反向依赖
//  libom2_executor.so：executor 自行持有 ModelDumpManager 句柄（不透明 void*），
//  调用下列 API 时直接传入该句柄。
// =============================================================================
struct ModelDumpInfo {
  uint32_t model_id;
  const char *model_name;
  const char *root_graph_name;
  uint32_t device_id;
  void *rt_model_handle;  // rtModel_t
  uint64_t step_id_addr;
  uint64_t loop_cond_addr;
  uint64_t iterations_per_loop_addr;
};

/**
 * @brief 创建 ModelDumpManager 实例（对应 ge::dump::ModelDumpManager 构造，每个模型一个实例）。
 * @param model_id 模型 ID。
 * @return 返回不透明句柄（ModelDumpManager *），创建失败时返回 nullptr。
 */
VISIBILITY_EXPORT void *Om2DumpManagerCreate(uint32_t model_id);

/**
 * @brief 销毁句柄对应的 ModelDumpManager 实例（对应 ge::dump::ModelDumpManager 析构）。
 * @param handle Om2DumpManagerCreate 返回的句柄，传入 nullptr 时不做任何操作。
 */
VISIBILITY_EXPORT void Om2DumpManagerDestroy(void *handle);

/**
 * @brief dump 模块全局初始化（对应 ge::dump::ModelDumpManager::GlobalInit）。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpGlobalInit(void);

/**
 * @brief 解析 dump 配置（对应 ge::dump::ModelDumpManager::ParseDumpConfig），须在模型加载前调用。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerParseConfig(void);

/**
 * @brief 设置模型加载完成后是否清理 DFX 缓存（对应 ModelDumpManager::SetClearDfxCacheFlagAfterLoad）。
 * @param handle Om2DumpManagerCreate 返回的句柄。
 * @param clear_cache 非 0 表示加载后清理 DFX 缓存，0 表示不清理。
 */
VISIBILITY_EXPORT void Om2DumpManagerSetClearDfxCacheFlag(void *handle, uint8_t clear_cache);

/**
 * @brief 下发 dump 信息（对应 ModelDumpManager::DispatchDumpInfo）。
 * @param handle Om2DumpManagerCreate 返回的句柄。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerDispatchDumpInfo(void *handle);

/**
 * @brief 模型执行前刷新当前 step 到设备内存，供 AICPU dump kernel 计算 step 落盘目录
 *        （对应 ModelDumpManager::UpdateStepId）。
 * @param handle Om2DumpManagerCreate 返回的句柄。
 * @param step_id 当前 step（从 0 开始）。
 * @param stream 模型流（aclrtStream），传入 nullptr 时走同步刷新。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerUpdateStepId(void *handle, uint64_t step_id, void *stream);

/**
 * @brief 按任务、流与 device 标识查询算子描述信息，用于异常 dump 定位算子
 *        （对应 ModelDumpManager::GetOpDescInfo）。
 * @param handle Om2DumpManagerCreate 返回的句柄。
 * @param task_id 任务 ID。
 * @param stream_id 流 ID。
 * @param device_id 设备 ID。
 * @param op_info 出参，查询到的算子描述信息（ge::OpDescInfo *）。
 * @return 返回 1 表示查询到算子信息，返回 0 表示未查询到或入参为 nullptr。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerGetOpDescInfo(void *handle, uint32_t task_id, uint32_t stream_id,
                                                      int32_t device_id, void *op_info);

/**
 * @brief 在 OM2 算子任务 launch 前执行 DFX 预处理（对应 ModelDumpManager::PreprocessOm2TaskInfo）。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerPreprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info);

/**
 * @brief 在 OM2 算子任务 launch 后保存 DFX 任务信息（对应 ModelDumpManager::PostprocessOm2TaskInfo）。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerPostprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info);

/**
 * @brief 查询指定算子是否需要 Data Dump（对应 ModelDumpManager::IsDataDumpEnabled）。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerIsDataDumpEnabled(void *handle, const char *op_name, uint8_t *is_data_dump);

/**
 * @brief 写入模型 dump 信息（对应 ModelDumpManager::SetModelDumpInfo）。
 * @return 返回 0 表示成功，返回其他值表示失败。
 */
VISIBILITY_EXPORT int32_t Om2DumpManagerSetModelDumpInfo(void *handle, const ModelDumpInfo *info);
#ifdef __cplusplus
}
#endif

#endif  // GE_FRAMEWORK_RUNTIME_DUMP_MODEL_DUMP_C_API_H_
