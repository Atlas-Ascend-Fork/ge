/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "runtime/custom_op/custom_op_loader.h"

#include <mutex>

#include "framework/common/debug/ge_log.h"
#include "register/op_lib_register_impl.h"
#include "runtime/custom_op/python_custom_op_bridge_loader.h"
#include "runtime/custom_op/python_custom_op_restore_hook.h"

namespace ge {
namespace custom_op {
namespace {
class CustomOpLoader {
 public:
  static CustomOpLoader &GetInstance() {
    static CustomOpLoader instance;
    return instance;
  }

  Status Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    // 安装 PYTHON_CUSTOM_OPS 恢复 hook（进程级单例，幂等）。LoadCustomOps() 是
    // 天然的"确保 custom_op_runtime 已载入"锚点，覆盖所有会加载 Python OM 的进程初始化
    // 路径（GEInitialize/ATC/GeExecutor/ge_generator）。库未被载入的进程 hook 为 null，
    // base 加载侧点名 Python 显式失败，不静默跳过。
    InstallPythonCustomOpRestoreHook();
    if (active_users_ == 0U) {
      shutdown_done_ = false;
      const auto cpp_load_ret = OpLibRegistry::GetInstance().PreProcessForCustomOp();
      if (cpp_load_ret != GRAPH_SUCCESS) {
        GELOGE(cpp_load_ret, "Load C++ custom ops failed.");
        return cpp_load_ret;
      }
      bool need_load_python_custom_ops = false;
      const auto check_ret = CheckNeedLoadPythonCustomOps(need_load_python_custom_ops);
      if (check_ret != SUCCESS) {
        GELOGE(check_ret, "Check Python custom ops failed.");
        return check_ret;
      }
      if ((!python_custom_ops_loaded_) && need_load_python_custom_ops) {
        const auto ret = LoadPythonCustomOps();
        if (ret != SUCCESS) {
          GELOGE(ret, "Load Python custom ops failed.");
          RollbackPythonCustomOpsLoad();
          return ret;
        }
        python_custom_ops_loaded_ = true;
      }
    }
    active_users_++;
    GELOGI("LoadCustomOps active_users_=%zu", active_users_);
    return SUCCESS;
  }

  Status LoadPythonCustomOpsIfNeeded() {
    std::lock_guard<std::mutex> lock(mutex_);
    bool need_load_python_custom_ops = false;
    const auto check_ret = CheckNeedLoadPythonCustomOps(need_load_python_custom_ops);
    if (check_ret != SUCCESS) {
      GELOGE(check_ret, "Check Python custom ops failed.");
      return check_ret;
    }
    if (!need_load_python_custom_ops) {
      return SUCCESS;
    }
    const auto ret = LoadPythonCustomOps();
    if (ret != SUCCESS) {
      GELOGE(ret, "Load Python custom ops failed.");
      RollbackPythonCustomOpsLoad();
      return ret;
    }
    python_custom_ops_loaded_ = true;
    return SUCCESS;
  }

  Status Unload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_users_ == 0U) {
      GELOGW("UnloadCustomOps called with no active users, possible reference leak.");
      return SUCCESS;
    }
    active_users_--;
    GELOGI("UnloadCustomOps active_users_=%zu", active_users_);
    if (active_users_ == 0U) {
      if (python_custom_ops_loaded_) {
        GELOGI("[PythonCustomOp] Last user, unloading python custom ops.");
        UnloadPythonCustomOps();
        python_custom_ops_loaded_ = false;
      }
      if (!shutdown_done_) {
        shutdown_done_ = true;
        ShutdownPythonCustomOpsForProcess();
        GELOGI("ShutdownPythonCustomOpsForProcess done.");
      }
    }
    return SUCCESS;
  }

 private:
  void RollbackPythonCustomOpsLoad() {
    UnloadPythonCustomOps();
    python_custom_ops_loaded_ = false;
  }

  std::mutex mutex_;
  size_t active_users_{0U};
  bool python_custom_ops_loaded_{false};
  bool shutdown_done_{false};
};
}  // namespace

Status LoadCustomOps() {
  return CustomOpLoader::GetInstance().Load();
}

Status LoadPythonCustomOpsIfNeeded() {
  return CustomOpLoader::GetInstance().LoadPythonCustomOpsIfNeeded();
}

Status UnloadCustomOps() {
  return CustomOpLoader::GetInstance().Unload();
}
}  // namespace custom_op
}  // namespace ge
