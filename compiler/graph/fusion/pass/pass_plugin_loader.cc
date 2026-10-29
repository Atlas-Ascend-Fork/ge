/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "pass_plugin_loader.h"

#include <cstdint>
#include <cstdlib>
#include <mutex>

#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "register/custom_pass_helper.h"
#include "python_pass_pybind_bridge.h"

namespace ge {
namespace fusion {
namespace {
constexpr const char *kEnvPythonPassPath = "ASCEND_GE_PY_PASS_PATH";

bool IsUserPythonPassPathSet() {
  const char *env_value = std::getenv(kEnvPythonPassPath);
  return (env_value != nullptr) && (env_value[0] != '\0');
}

class PassPluginLoader {
 public:
  static PassPluginLoader &GetInstance() {
    static PassPluginLoader instance;
    return instance;
  }

  Status Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_users_ == 0U) {
      // A previous final Unload may have shut the bridge down.  Loading a new
      // generation must be allowed to register the built-in descriptors
      // again.
      shutdown_done_ = false;
      if (!cpp_pass_loaded_) {
        const auto ret = CustomPassHelper::Instance().Load();
        if (ret != SUCCESS) {
          GELOGE(ret, "Load C++ custom pass plugins failed.");
          // CustomPassHelper may have opened a prefix of the plugin list
          // before reporting an error.  Roll that prefix back so the next
          // Load() starts from a clean generation.
          const auto cleanup_ret = CustomPassHelper::Instance().Unload();
          if (cleanup_ret != SUCCESS) {
            GELOGW("Cleanup after loading C++ custom pass plugins failed, status[%u].",
                   static_cast<uint32_t>(cleanup_ret));
          }
          cpp_pass_loaded_ = false;
          return ret;
        }
        cpp_pass_loaded_ = true;
      }
      if (IsUserPythonPassPathSet()) {
        const auto ret = LoadPythonPassesLocked();
        if (ret != SUCCESS) {
          GELOGE(ret, "Load Python fusion pass plugins failed.");
          // Python registrations have been rolled back; also undo C++ loading
          // because this initialization attempt did not acquire a user.
          const auto cleanup_ret = CustomPassHelper::Instance().Unload();
          if (cleanup_ret != SUCCESS) {
            GELOGW("Cleanup after loading Python fusion pass plugins failed, status[%u].",
                   static_cast<uint32_t>(cleanup_ret));
          }
          cpp_pass_loaded_ = false;
          return ret;
        }
      }
    }
    active_users_++;
    GELOGD("LoadPassPlugins active_users_=%zu", active_users_);
    return SUCCESS;
  }

  Status EnsurePythonPassesLoaded() {
    std::lock_guard<std::mutex> lock(mutex_);
    GE_ASSERT_TRUE(active_users_ > 0U);
    return LoadPythonPassesLocked();
  }

  Status Unload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_users_ == 0U) {
      GELOGW("UnloadPassPlugins called with no active users, possible reference leak.");
      return SUCCESS;
    }
    active_users_--;
    GELOGD("UnloadPassPlugins active_users_=%zu", active_users_);
    if (active_users_ == 0U) {
      if (python_pass_loaded_) {
        UnloadPythonPasses();
        python_pass_loaded_ = false;
      }
      if (cpp_pass_loaded_) {
        cpp_pass_loaded_ = false;
        (void)CustomPassHelper::Instance().Unload();
      }
      if (!shutdown_done_) {
        shutdown_done_ = true;
        ShutdownPythonPassesForProcess();
        GELOGI("[PythonPass] ShutdownPythonPassesForProcess done.");
      }
    }
    return SUCCESS;
  }

 private:
  Status LoadPythonPassesLocked() {
    if (python_pass_loaded_) {
      return SUCCESS;
    }
    const auto ret = RegisterPythonPassesFromPlugin();
    if (ret != SUCCESS) {
      // Discard partial registrations so a later first-use attempt can retry.
      UnloadPythonPasses();
      return ret;
    }
    python_pass_loaded_ = true;
    return SUCCESS;
  }

  std::mutex mutex_;
  size_t active_users_{0U};
  bool cpp_pass_loaded_{false};
  bool python_pass_loaded_{false};
  bool shutdown_done_{false};
};
}  // namespace

Status LoadPassPlugins() {
  return PassPluginLoader::GetInstance().Load();
}

Status UnloadPassPlugins() {
  return PassPluginLoader::GetInstance().Unload();
}

Status EnsurePythonPassesLoaded() {
  return PassPluginLoader::GetInstance().EnsurePythonPassesLoaded();
}
}  // namespace fusion
}  // namespace ge
