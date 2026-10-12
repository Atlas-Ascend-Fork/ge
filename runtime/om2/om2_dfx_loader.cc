/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_dfx_loader.h"

#include <atomic>
#include <cstddef>
#include <cstring>
#include <dlfcn.h>
#include <limits.h>
#include <mutex>
#include <string>
#include <unistd.h>

#include "framework/common/debug/ge_log.h"
#include "framework/runtime/dump/profiling_callback_manager.h"
#include "common/checker.h"

namespace gert {
namespace {

constexpr const char *kDumpLibName = "libom2_dump.so";

// 取 libom2_executor.so 自身路径，推导出同目录下的 libom2_dump.so 路径。
__attribute__((noinline)) void *Om2DfxLoaderSelfPtr() {
  return reinterpret_cast<void *>(&Om2DfxLoaderSelfPtr);
}

std::string ResolveDumpLibPath() {
  Dl_info info;
  if (dladdr(Om2DfxLoaderSelfPtr(), &info) == 0 || info.dli_fname == nullptr) {
    return kDumpLibName;
  }
  std::string self_path(info.dli_fname);
  char abs[PATH_MAX] = {0};
  if (realpath(self_path.c_str(), abs) != nullptr) {
    self_path = abs;
  }
  const size_t slash = self_path.find_last_of('/');
  if (slash == std::string::npos) {
    return kDumpLibName;
  }
  return self_path.substr(0U, slash + 1U) + kDumpLibName;
}

template <typename T>
T Sym(void *lib, const char *name) {
  return reinterpret_cast<T>(dlsym(lib, name));
}

// Check if the parameter is null. If yes, return PARAM_INVALID and record the error
#define GE_CHECK_NOTNULL(val, ...)                                                            \
  do {                                                                                        \
    if ((val) == nullptr) {                                                                   \
      REPORT_INNER_ERR_MSG("E19999", "Param:" #val " is nullptr, check invalid" __VA_ARGS__); \
      GELOGE(ge::FAILED, "[Check][Param:" #val "]null is invalid" __VA_ARGS__);               \
      return ge::PARAM_INVALID;                                                               \
    }                                                                                         \
  } while (false)

#define RESOLVE_SYMBOL(VAR, LIB, SYM)                                    \
  do {                                                                   \
    VAR = Sym<decltype(VAR)>((LIB), (SYM));                              \
    if ((VAR) == nullptr) {                                              \
      GELOGE(ge::FAILED, "[OM2][Dump]Resolve symbol %s failed.", (SYM)); \
      return false;                                                      \
    }                                                                    \
  } while (false)

// 解析 libom2_dump.so 中全部 C API 符号并做完整性校验，全部命中则填入 tmp 并返回 true
bool ResolveDumpApi(void *lib, Om2DumpApi &dump_api) {
  RESOLVE_SYMBOL(dump_api.dump_global_init, lib, "Om2DumpGlobalInit");
  RESOLVE_SYMBOL(dump_api.parse_dump_config, lib, "Om2DumpManagerParseConfig");
  RESOLVE_SYMBOL(dump_api.create_dump_manager, lib, "Om2DumpManagerCreate");
  RESOLVE_SYMBOL(dump_api.destroy_dump_manager, lib, "Om2DumpManagerDestroy");
  RESOLVE_SYMBOL(dump_api.set_clear_dfx_cache_flag, lib, "Om2DumpManagerSetClearDfxCacheFlag");
  RESOLVE_SYMBOL(dump_api.dispatch_dump_info, lib, "Om2DumpManagerDispatchDumpInfo");
  RESOLVE_SYMBOL(dump_api.update_step_id, lib, "Om2DumpManagerUpdateStepId");
  RESOLVE_SYMBOL(dump_api.get_op_desc_info, lib, "Om2DumpManagerGetOpDescInfo");
  RESOLVE_SYMBOL(dump_api.preprocess_om2_task_info, lib, "Om2DumpManagerPreprocessOm2TaskInfo");
  RESOLVE_SYMBOL(dump_api.postprocess_om2_task_info, lib, "Om2DumpManagerPostprocessOm2TaskInfo");
  RESOLVE_SYMBOL(dump_api.is_data_dump_enabled, lib, "Om2DumpManagerIsDataDumpEnabled");
  RESOLVE_SYMBOL(dump_api.set_model_dump_info, lib, "Om2DumpManagerSetModelDumpInfo");

  return true;
}
}  // namespace

#define CHECK_DUMP_API(SO_EXIST, FUNC_PTR, SUCCESS_VAL, FAIL_VAL) \
  do {                                                            \
    if (!(SO_EXIST)) {                                            \
      return (SUCCESS_VAL);                                       \
    };                                                            \
    if ((FUNC_PTR) == nullptr) {                                  \
      return (FAIL_VAL);                                          \
    }                                                             \
  } while (false)

#define CHECK_DUMP_API_VOID(SO_EXIST, FUNC_PTR) \
  do {                                          \
    if (!(SO_EXIST)) {                          \
      return;                                   \
    };                                          \
    if ((FUNC_PTR) == nullptr) {                \
      return;                                   \
    }                                           \
  } while (false)

int32_t Om2DfxApi::GlobalInit(void) {
  // 1. 执行 dump 的初始化
  CHECK_DUMP_API(dump_so_exist, dump_api_.dump_global_init, ge::SUCCESS, ge::FAILED);
  GE_ASSERT_SUCCESS(dump_api_.dump_global_init());

  // 2. 初始化 Profiling 的 Callback
  GE_ASSERT_SUCCESS(ge::dump::ProfilingCallbackManager::GlobalInit());

  return ge::SUCCESS;
}

int32_t Om2DfxApi::ParseDumpConfig(void) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.parse_dump_config, ge::SUCCESS, ge::FAILED);
  return dump_api_.parse_dump_config();
}

int32_t Om2DfxApi::CreateDumpManager(uint32_t model_id, void **handle_ptr) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.dump_global_init, ge::SUCCESS, ge::FAILED);
  GE_ASSERT_NOTNULL(handle_ptr);
  auto dump_manager = dump_api_.create_dump_manager(model_id);
  GE_ASSERT_NOTNULL(dump_manager);
  *handle_ptr = dump_manager;
  return ge::SUCCESS;
}

void Om2DfxApi::DestroyDumpManager(void *handle) {
  CHECK_DUMP_API_VOID(dump_so_exist, dump_api_.destroy_dump_manager);
  dump_api_.destroy_dump_manager(handle);
}

void Om2DfxApi::SetClearDfxCacheFlag(void *handle, uint8_t clear_cache) {
  CHECK_DUMP_API_VOID(dump_so_exist, dump_api_.set_clear_dfx_cache_flag);
  dump_api_.set_clear_dfx_cache_flag(handle, clear_cache);
}

int32_t Om2DfxApi::DispatchDumpInfo(void *handle) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.dispatch_dump_info, ge::SUCCESS, ge::FAILED);
  GE_ASSERT_NOTNULL(handle);
  return dump_api_.dispatch_dump_info(handle);
}

int32_t Om2DfxApi::UpdateStepId(void *handle, uint64_t step_id, void *stream) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.update_step_id, ge::SUCCESS, ge::FAILED);
  return dump_api_.update_step_id(handle, step_id, stream);
}

int32_t Om2DfxApi::GetOpDescInfo(void *handle, uint32_t task_id, uint32_t stream_id, int32_t device_id,
                                 ge::OpDescInfo *op_info) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.get_op_desc_info, 1, 0);
  GE_ASSERT_NOTNULL(handle);
  return dump_api_.get_op_desc_info(handle, task_id, stream_id, device_id, op_info);
}

int32_t Om2DfxApi::PreprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.preprocess_om2_task_info, ge::SUCCESS, ge::FAILED);
  return dump_api_.preprocess_om2_task_info(handle, task_info);
}

int32_t Om2DfxApi::PostprocessOm2TaskInfo(void *handle, const GertModelTaskDesc *task_info) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.postprocess_om2_task_info, ge::SUCCESS, ge::FAILED);
  return dump_api_.postprocess_om2_task_info(handle, task_info);
}

int32_t Om2DfxApi::IsDataDumpEnabled(void *handle, const char *op_name, uint8_t *is_data_dump) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.is_data_dump_enabled, ge::SUCCESS, ge::FAILED);
  return dump_api_.is_data_dump_enabled(handle, op_name, is_data_dump);
}

int32_t Om2DfxApi::SetModelDumpInfo(void *handle, const ModelDumpInfo *info) {
  CHECK_DUMP_API(dump_so_exist, dump_api_.set_model_dump_info, ge::SUCCESS, ge::FAILED);
  return dump_api_.set_model_dump_info(handle, info);
}

Om2DfxApi &Om2DfxApi::Instance() {
  static Om2DfxApi api;
  // 首次调用时调用 Initialize 完成 libom2_dump.so 的加载和符号解析
  static std::once_flag once;
  std::call_once(once, []() { api.Initialize(); });
  return api;
}

void Om2DfxApi::Initialize() {
  const std::string dump_path = ResolveDumpLibPath();
  // 判断 libom2_dump.so 文件是否存在，若文件不存在，则dump不使能，业务正常执行
  if (access(dump_path.c_str(), F_OK) != 0) {
    GELOGW("[OM2][Dump] %s not found, dump disabled.", dump_path.c_str());
    GEEVENT("[OM2][Dump] %s not found, dump disabled.", dump_path.c_str());
    dump_so_exist = false;
    return;
  }

  dump_so_exist = true;
  so_handle_ = dlopen(dump_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (so_handle_ == nullptr) {
    const char *err = dlerror();
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][Dump] dlopen %s failed. err=%s", dump_path.c_str(),
           (err != nullptr) ? err : "null");
    return;
  }

  if (!ResolveDumpApi(so_handle_, dump_api_)) {
    const char *err = dlerror();
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[OM2][Dump] %s loaded but some symbols missing. err=%s", dump_path.c_str(),
           (err != nullptr) ? err : "null");
    dlclose(so_handle_);
    so_handle_ = nullptr;
    dump_api_.Reset();
    return;
  }

  GELOGI("[OM2][Dump] dlopen %s success, dump enabled.", dump_path.c_str());
}

Om2DfxApi::~Om2DfxApi() {
  if (so_handle_ != nullptr) {
    (void)dlclose(so_handle_);
    so_handle_ = nullptr;
  }
}

}  // namespace gert
