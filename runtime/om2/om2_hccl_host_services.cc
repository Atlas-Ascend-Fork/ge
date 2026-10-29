/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_hccl_host_services.h"
#include <atomic>
#include <cinttypes>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "common/checker.h"
#include "common/opskernel/ge_task_info.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "framework/common/framework_types_internal.h"
#include "framework/runtime/om2_model_executor.h"
#include "graph_metadef/common/plugin/plugin_manager.h"
#include "mmpa/mmpa_api.h"

namespace gert {
namespace {
using HcclPluginInitializeFunc = ge::Status (*)(const std::map<std::string, std::string> &options);
using HcclPluginGetStoresFunc =
    void (*)(std::map<std::string, std::shared_ptr<ge::OpsKernelInfoStore>> &op_kernel_infos);

const char *const kHcclExecutorLibName = "libhcom_executor.so";
const char *const kHcclKernelInfoStoreName = "ops_kernel_info_hccl";

// 进程级插件缓存
std::mutex g_hccl_store_mu;
void *g_hccl_so_handle = nullptr;                                                 // libhcom_executor.so
std::map<std::string, std::shared_ptr<ge::OpsKernelInfoStore>> g_hccl_executors;  // 持有 store 生命周期
ge::OpsKernelInfoStore *g_hccl_store = nullptr;                                   // "ops_kernel_info_hccl"
std::atomic<uint32_t> g_hccl_next_task_id{1U};                                    // 全局自增 task id

ge::Status HcomExecutorPluginPath(std::string &lib_path) {
  lib_path = ge::GetModelPath() + kHcclExecutorLibName;
  return ge::SUCCESS;
}

ge::OpsKernelInfoStore *GetHcclStore() {
  const std::lock_guard<std::mutex> lk(g_hccl_store_mu);
  if (g_hccl_store != nullptr) {
    return g_hccl_store;
  }
  std::string lib_path;
  GE_ASSERT_SUCCESS(HcomExecutorPluginPath(lib_path));
  g_hccl_so_handle = mmDlopen(lib_path.c_str(), static_cast<int32_t>(MMPA_RTLD_NOW));
  GE_ASSERT_NOTNULL(g_hccl_so_handle, "[OM2] mmDlopen %s failed, please check the hccl plugin.", lib_path.c_str());

  auto initialize_func = reinterpret_cast<HcclPluginInitializeFunc>(mmDlsym(g_hccl_so_handle, "Initialize"));
  auto get_stores_func = reinterpret_cast<HcclPluginGetStoresFunc>(mmDlsym(g_hccl_so_handle, "GetOpsKernelInfoStores"));
  GE_ASSERT_NOTNULL(get_stores_func, "[OM2] mmDlsym GetOpsKernelInfoStores from %s failed.", lib_path.c_str());
  if (initialize_func != nullptr) {
    const std::map<std::string, std::string> options;
    GE_ASSERT_SUCCESS(initialize_func(options), "[OM2] hccl plugin Initialize failed.");
  }
  get_stores_func(g_hccl_executors);
  const auto it = g_hccl_executors.find(kHcclKernelInfoStoreName);
  GE_ASSERT_TRUE(it != g_hccl_executors.end() && (it->second != nullptr), "[OM2] executor %s not found in %s.",
                 kHcclKernelInfoStoreName, lib_path.c_str());
  g_hccl_store = it->second.get();
  GELOGI("[OM2] hccl plugin loaded, lib=%s, store=%s", lib_path.c_str(), kHcclKernelInfoStoreName);
  return g_hccl_store;
}

bool IsHcclReduceOp(const std::string &hccl_type) {
  return (hccl_type == ge::HCOMALLREDUCE) || (hccl_type == ge::HCOMREDUCESCATTER) ||
         (hccl_type == ge::HCOMREDUCESCATTERV) || (hccl_type == ge::HVDCALLBACKALLREDUCE) ||
         (hccl_type == ge::HCOMREDUCE);
}

bool IsHcclUpdateOutputAddr(const std::string &hccl_type) {
  return (hccl_type == ge::HCOMALLGATHER) || (hccl_type == ge::HCOMRECEIVE) ||
         (hccl_type == ge::HVDCALLBACKALLGATHER) || (hccl_type == ge::HCOMALLTOALLV) ||
         (hccl_type == ge::HCOMALLTOALLVC) || (hccl_type == ge::HCOMALLTOALL) || (hccl_type == ge::HCOMALLGATHERV) ||
         (hccl_type == ge::HCOMGATHER);
}

void AssembleGETaskInfo(ge::GETaskInfo &ge_task, const HcclTaskParams &params, ge::OpsKernelInfoStore *store) {
  ge_task.id = g_hccl_next_task_id.fetch_add(1U);
  ge_task.type = static_cast<uint16_t>(ge::ModelTaskType::MODEL_TASK_HCCL);
  ge_task.stream = params.main_stream;
  ge_task.privateDef = const_cast<void *>(params.private_def);
  ge_task.privateDefLen = params.private_def_len;
  ge_task.opsKernelStorePtr = store;
  ge_task.needRefresh = (params.need_refresh != 0U);
  for (uint32_t i = 0U; i < params.attached_stream_num; ++i) {
    ge_task.rt_attached_streams.push_back(params.attached_streams[i]);
  }

  const bool need_refresh = ge_task.needRefresh && (params.args_base != nullptr);
  auto *args_base = static_cast<uint8_t *>(params.args_base);
  const std::string hccl_type = (params.hccl_type != nullptr) ? params.hccl_type : "";
  const bool is_reduce = IsHcclReduceOp(hccl_type);
  const bool is_update_output = IsHcclUpdateOutputAddr(hccl_type);
  const bool set_output = is_reduce || is_update_output;
  const bool has_zero_copy = (params.input_zero_copy_flags != nullptr) || (params.output_zero_copy_flags != nullptr);

  const auto slot_input = [&](uint32_t i) {
    return need_refresh || ((params.input_zero_copy_flags != nullptr) && (params.input_zero_copy_flags[i] != 0));
  };
  const auto slot_output = [&](uint32_t i) {
    return need_refresh || ((params.output_zero_copy_flags != nullptr) && (params.output_zero_copy_flags[i] != 0));
  };

  const auto assemble_common = [&](ge::GETaskKernelHcclInfo &info, uint32_t idx) {
    info.hccl_type = hccl_type;
    info.rootId = static_cast<int64_t>(params.root_id);
    if (is_reduce) {
      info.opType = params.hccl_reduce_op;
    }
    info.workSpaceMemSize = params.workspace_size;
    if (params.workspace_size != 0U) {
      info.workSpaceAddr =
          need_refresh ? static_cast<void *>(args_base + (params.input_num + params.output_num) * sizeof(uint64_t))
                       : params.workspace_addr;
    }
    if ((idx < params.input_num) && (params.input_addrs != nullptr)) {
      info.inputDataAddr =
          slot_input(idx) ? static_cast<void *>(args_base + idx * sizeof(uint64_t)) : params.input_addrs[idx];
    }
    if (set_output && (idx < params.output_num) && (params.output_addrs != nullptr)) {
      info.outputDataAddr = slot_output(idx)
                                ? static_cast<void *>(args_base + (params.input_num + idx) * sizeof(uint64_t))
                                : params.output_addrs[idx];
    }
    if (!need_refresh || has_zero_copy) {
      if (params.input_addrs != nullptr) {
        for (uint32_t i = 0U; i < params.input_num; ++i) {
          info.inputDataAddrs.push_back(slot_input(i) ? static_cast<void *>(args_base + i * sizeof(uint64_t))
                                                      : params.input_addrs[i]);
        }
      }
      if (params.output_addrs != nullptr) {
        for (uint32_t i = 0U; i < params.output_num; ++i) {
          info.outputDataAddrs.push_back(
              slot_output(i) ? static_cast<void *>(args_base + (params.input_num + i) * sizeof(uint64_t))
                             : params.output_addrs[i]);
        }
      }
    }
    if (params.input_zero_copy_flags != nullptr) {
      info.inputZeroCopyFlags.assign(params.input_zero_copy_flags, params.input_zero_copy_flags + params.input_num);
    }
    if (params.output_zero_copy_flags != nullptr) {
      info.outputZeroCopyFlags.assign(params.output_zero_copy_flags, params.output_zero_copy_flags + params.output_num);
    }
    for (uint32_t i = 0U; i < params.follow_stream_num; ++i) {
      info.hcclStreamList.push_back(params.follow_streams[i]);
    }
    if (params.overflow_addr != nullptr) {
      info.global_workspace_addr.emplace_back(params.overflow_addr);
    }
  };

  if (params.hvd_wait != 0U) {
    ge::GETaskKernelHcclInfo info;
    info.hccl_type = hccl_type;
    for (uint32_t i = 0U; i < params.follow_stream_num; ++i) {
      info.hcclStreamList.push_back(params.follow_streams[i]);
    }
    ge_task.kernelHcclInfo.emplace_back(std::move(info));
    return;
  }

  if (params.hvd_metas != nullptr) {
    for (uint32_t i = 0U; i < params.input_num; ++i) {
      ge::GETaskKernelHcclInfo info;
      const auto &meta = params.hvd_metas[i];
      info.hccl_type = hccl_type;
      info.input_name = (meta.input_name != nullptr) ? meta.input_name : "";
      if (meta.dims != nullptr) {
        info.dims.assign(meta.dims, meta.dims + meta.dims_num);
      }
      info.dataType = meta.hccl_data_type;
      info.count = static_cast<int64_t>(meta.count);
      assemble_common(info, i);
      ge_task.kernelHcclInfo.emplace_back(std::move(info));
    }
    return;
  }

  ge::GETaskKernelHcclInfo hccl_info;
  hccl_info.count = static_cast<int64_t>(params.count);
  hccl_info.dataType = params.hccl_data_type;
  assemble_common(hccl_info, 0U);
  ge_task.kernelHcclInfo.emplace_back(std::move(hccl_info));
}
}  // namespace

int32_t HcclHostLoadTask(void *instance_handle, const HcclTaskParams *params) {
  if ((instance_handle == nullptr) || (params == nullptr) || (params->hccl_type == nullptr)) {
    return static_cast<int32_t>(ge::PARAM_INVALID);
  }
  GELOGI("[OM2][Hccl][Load] enter, type=%s, main_stream=%p, in=%u, out=%u, ws=%p/%" PRIu64
         ", args_base=%p, need_refresh=%u, hvd_metas=%p, hvd_wait=%u, collective=%u, follow=%u, attached=%u, "
         "overflow=%p",
         params->hccl_type, params->main_stream, params->input_num, params->output_num, params->workspace_addr,
         params->workspace_size, params->args_base, static_cast<uint32_t>(params->need_refresh),
         static_cast<const void *>(params->hvd_metas), static_cast<uint32_t>(params->hvd_wait),
         static_cast<uint32_t>(params->is_collective), params->follow_stream_num, params->attached_stream_num,
         params->overflow_addr);
  auto *executor = static_cast<gert::Om2ModelExecutor *>(instance_handle);
  auto *store = GetHcclStore();
  // GetHcclStore 所有失败路径（mmDlopen/dlsym/Initialize/store 未注册）经 ErrorResult 返回 nullptr，
  // 不判空会在下方虚调用处空指针 coredump；判空后转为可诊断的错误返回
  if (store == nullptr) {
    GELOGE(ge::FAILED, "[OM2] hccl store unavailable, skip load task, hccl_type=%s.", params->hccl_type);
    return static_cast<int32_t>(ge::INTERNAL_ERROR);
  }
  GELOGI("[OM2][Hccl][Load] store ready, store=%p, hccl_type=%s.", static_cast<const void *>(store), params->hccl_type);
  auto ge_task = std::make_unique<ge::GETaskInfo>();
  AssembleGETaskInfo(*ge_task, *params, store);
  GELOGI("[OM2][Hccl][Load] task assembled, hccl_type=%s, in=%u, out=%u, follow=%u, need_refresh=%u.",
         params->hccl_type, params->input_num, params->output_num, params->follow_stream_num,
         static_cast<uint32_t>(params->need_refresh));

  GE_ASSERT_SUCCESS(store->PrepareTaskAsync(*ge_task), "[OM2] PrepareTaskAsync failed, hccl_type=%s.",
                    params->hccl_type);
  GELOGI("[OM2][Hccl][Load] PrepareTaskAsync done, hccl_type=%s, task_id=%u.", params->hccl_type, ge_task->id);
  GE_ASSERT_SUCCESS(store->LoadTask(*ge_task), "[OM2] LoadTask failed, hccl_type=%s, task_id=%u.", params->hccl_type,
                    ge_task->id);
  GELOGI("[OM2] hccl task loaded, task_id=%u, type=%s, in=%u, out=%u, need_refresh=%u, hvd=%d, follow=%u", ge_task->id,
         params->hccl_type, params->input_num, params->output_num, static_cast<uint32_t>(params->need_refresh),
         (params->hvd_metas != nullptr) ? 1 : 0, params->follow_stream_num);
  if (params->is_collective != 0U) {
    static const ge::char_t kCommunicationTaskType[] = "{\"taskType\":\"communication\"}";
    const auto rt_ret = aclrtCacheLastTaskExtendInfo(kCommunicationTaskType, strlen(kCommunicationTaskType));
    if (rt_ret != 0) {
      GELOGW("[OM2] aclrtCacheLastTaskExtendInfo failed, hccl_type=%s, ret=%d.", params->hccl_type, rt_ret);
    }
  }
  if (!ge_task->kernelHcclInfo.empty() && !ge_task->kernelHcclInfo[0U].hccl_dump_info.empty()) {
    GELOGI("[OM2] HCCL overflow tasks detected, task_id=%u, dump info num=%zu.", ge_task->id,
           ge_task->kernelHcclInfo[0U].hccl_dump_info.size());
  }
  executor->RecordHcclLoadedTask(ge_task.release());
  return static_cast<int32_t>(ge::SUCCESS);
}

int32_t HcclHostUnloadTask(void *instance_handle) {
  if (instance_handle == nullptr) {
    return static_cast<int32_t>(ge::PARAM_INVALID);
  }
  auto *executor = static_cast<gert::Om2ModelExecutor *>(instance_handle);
  auto *store = GetHcclStore();
  auto loaded_tasks = executor->TakeHcclLoadedTasks();
  GELOGI("[OM2][Hccl][Unload] begin, task_num=%zu.", loaded_tasks.size());
  if (store == nullptr) {
    GELOGW("[OM2][Hccl][Unload] hccl store unavailable, skip UnloadTask, drain %zu tasks.", loaded_tasks.size());
  }
  for (void *loaded_task : loaded_tasks) {
    auto *ge_task = static_cast<ge::GETaskInfo *>(loaded_task);
    if (store != nullptr) {
      const ge::Status ret = store->UnloadTask(*ge_task);
      if (ret != ge::SUCCESS) {
        GELOGW("[OM2] UnloadTask failed, task_id=%u, ret=%u.", ge_task->id, ret);
      }
    }
    // store 不可用时仍需释放任务句柄，避免泄漏
    delete ge_task;
  }
  GELOGI("[OM2] hccl tasks unloaded, num=%zu", loaded_tasks.size());
  return static_cast<int32_t>(ge::SUCCESS);
}
}  // namespace gert
