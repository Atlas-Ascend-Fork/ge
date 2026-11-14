/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_COMMON_OM2_GENERATED_MODEL_RUNNER_H_
#define GE_TESTS_COMMON_OM2_GENERATED_MODEL_RUNNER_H_

#include <dlfcn.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>

#include "framework/om2/model_api/om2_model_api.h"
#include "framework/om2/model_data/gert_model_data.h"

namespace ge {
namespace om2_test {
// 只加载调用方本次生成的 SO 字节；不提供模型或 Kernel 的替代实现。
class GeneratedModelRunner {
 public:
  GeneratedModelRunner() = default;
  GeneratedModelRunner(const GeneratedModelRunner &) = delete;
  GeneratedModelRunner &operator=(const GeneratedModelRunner &) = delete;

  ~GeneratedModelRunner() {
    if (model_ != nullptr) {
      (void)Unload();
    }
    if (library_ != nullptr) {
      (void)dlclose(library_);
    }
    if (!path_.empty()) {
      (void)unlink(path_.c_str());
    }
  }

  bool Open(const gert::GertModelDataFile &artifact) {
    if (library_ != nullptr || !path_.empty() || artifact.data == nullptr || artifact.data_size == 0U) {
      error_ = "missing generated SO or runner already opened";
      return false;
    }
    char name[] = "/tmp/ge_om2_generated_XXXXXX";
    const int fd = mkstemp(name);
    if (fd < 0) {
      error_ = std::strerror(errno);
      return false;
    }
    path_ = name;
    const auto *bytes = reinterpret_cast<const char *>(artifact.data.get());
    size_t written = 0U;
    while (written < artifact.data_size) {
      const ssize_t count = write(fd, bytes + written, artifact.data_size - written);
      if (count < 0 && errno == EINTR) {
        continue;
      }
      if (count <= 0) {
        error_ = std::strerror(errno);
        (void)close(fd);
        return false;
      }
      written += static_cast<size_t>(count);
    }
    if (close(fd) != 0) {
      error_ = std::strerror(errno);
      return false;
    }
    library_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library_ == nullptr) {
      error_ = dlerror();
      return false;
    }
    // Om2ModelExecutor::ResolveSymbols requires these four lifecycle functions.
    // Dynamic models manage their streams internally; stream queries are optional there.
    return Resolve("GertModelLoad", load_) && Resolve("GertModelRun", run_) &&
           Resolve("GertModelRunAsync", run_async_) && Resolve("GertModelUnload", unload_);
  }

  const std::string &Error() const {
    return error_;
  }

  int32_t Load(const GertModelLoadConfig &config) {
    if (load_ == nullptr || model_ != nullptr) {
      return ACL_ERROR_FAILURE;
    }
    GertModelLoadOutput output;
    return load_(&config, &model_, &output);
  }

  int32_t Run(const GertModelRunConfig &config) {
    if (run_ == nullptr || model_ == nullptr) {
      return ACL_ERROR_FAILURE;
    }
    GertModelRunOutput output;
    return run_(model_, &config, &output);
  }

  int32_t RunAsync(aclrtStream stream, const GertModelRunConfig &config) {
    if (run_async_ == nullptr || model_ == nullptr) {
      return ACL_ERROR_FAILURE;
    }
    GertModelRunOutput output;
    return run_async_(model_, stream, &config, &output);
  }

  int32_t Unload() {
    if (unload_ == nullptr || model_ == nullptr) {
      return ACL_ERROR_FAILURE;
    }
    GertModelUnloadConfig config;
    GertModelUnloadOutput output;
    const auto result = unload_(model_, &config, &output);
    model_ = nullptr;
    return result;
  }

 private:
  template <typename T>
  bool Resolve(const char *name, T &function) {
    (void)dlerror();
    function = reinterpret_cast<T>(dlsym(library_, name));
    const char *error = dlerror();
    if (error != nullptr || function == nullptr) {
      error_ = error == nullptr ? name : error;
      return false;
    }
    return true;
  }

  std::string path_;
  std::string error_;
  void *library_ = nullptr;
  GertModelHandle model_ = nullptr;
  decltype(&GertModelLoad) load_ = nullptr;
  decltype(&GertModelRun) run_ = nullptr;
  decltype(&GertModelRunAsync) run_async_ = nullptr;
  decltype(&GertModelUnload) unload_ = nullptr;
};
}  // namespace om2_test
}  // namespace ge

#endif  // GE_TESTS_COMMON_OM2_GENERATED_MODEL_RUNNER_H_
