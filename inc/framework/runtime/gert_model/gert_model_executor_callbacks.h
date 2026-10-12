/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_GERT_MODEL_GERT_MODEL_EXECUTOR_CALLBACKS_H_
#define GE_FRAMEWORK_RUNTIME_GERT_MODEL_GERT_MODEL_EXECUTOR_CALLBACKS_H_

#include <stdint.h>

#include "acl/acl_rt.h"
#include "framework/om2/model_api/om2_model_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t GertModelLaunchTask(void *instance_handle, GertModelTaskLaunchInfo *launch_info);

/**
 * @brief 在 OM2 算子任务 launch 前执行 DFX 预处理。
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t ReportDfxTaskPreprocess(uint32_t model_id, void *instance_handle, const GertModelTaskDesc *task_info,
                                const void *extended_attrs, size_t extended_attrs_size);

/**
 * @brief 在 OM2 算子任务 launch 后保存 DFX 任务信息。
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t ReportDfxTaskPostprocess(uint32_t model_id, void *instance_handle, const GertModelTaskDesc *task_info,
                                 const void *extended_attrs, size_t extended_attrs_size);

/**
 * @brief 查询指定算子是否需要 Data Dump。
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t IsDataDumpEnabled(uint32_t model_id, void *instance_handle, const char *op_name, uint8_t *is_data_dump);

/**
 * @brief 上报模型运行时句柄等基础信息, 由模型 SO 加载期回调。
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t ReportModelBaseInfo(void *instance_handle, const GertModelBaseInfo *info);

/**
 * @brief 对BinHandle缓存集合加锁, 需要加解锁成对出现, 需要调用者保证锁释放
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t LockBinHandleStore();

/**
 * @brief 对BinHandle缓存集合解锁, 需要加解锁成对出现, 需要调用者保证锁释放
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t UnlockBinHandleStore();

/**
 * @brief 从缓存集合查询BinHandle, 查询不到BinHandle返回空
 * @param bin_id 输入
 * @param bin_handle 输出
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t QueryBinHandleFromStore(const char *bin_id, aclrtBinHandle *bin_handle);

/**
 * @brief 释放缓存集合中的BinHandle引用, 当引用计数减到0时从集合中移除
 * @param bin_id 输入
 * @param need_unload 输出, 1表示需要卸载, 0表示不需要卸载
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t ReleaseBinHandleFromStore(const char *bin_id, uint8_t *need_unload);

/**
 * @brief 保存BinHandle到缓存集合, 查询到则引用计数自动加1, 查询不到则保存且引用计数为1
 * @param bin_id 输入
 * @param bin_handle 输入
 * @return 返回 0 表示成功, 返回其他值表示失败
 */
int32_t SaveBinHandleToStore(const char *bin_id, const aclrtBinHandle bin_handle);

#ifdef __cplusplus
}
#endif

#endif  // GE_FRAMEWORK_RUNTIME_GERT_MODEL_GERT_MODEL_EXECUTOR_CALLBACKS_H_
