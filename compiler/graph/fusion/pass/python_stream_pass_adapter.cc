/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "python_pass_adapter.h"

#include <map>
#include <mutex>
#include <new>
#include <string>
#include <utility>

#include "framework/common/debug/ge_log.h"
#include "register/custom_pass_helper.h"

namespace ge {
namespace fusion {
namespace {
struct PythonResourcePlanRuntimeEntry {
  PythonPassDescriptor pass_desc;
  PythonResourcePlanPassCallbacks callbacks;
};

class PythonResourcePlanRuntimeRegistryImpl {
 public:
  bool Register(const PythonPassDescriptor &pass_desc, const PythonResourcePlanPassCallbacks &callbacks) {
    if (!pass_desc.IsValid() || !callbacks.IsValid()) {
      GELOGW("Invalid Python resource-plan runtime registration, descriptor[%s].", pass_desc.descriptor_key.c_str());
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.find(pass_desc.descriptor_key) != entries_.cend()) {
      GELOGW("Python resource-plan descriptor[%s] is already registered.", pass_desc.descriptor_key.c_str());
      return false;
    }
    for (const auto &item : entries_) {
      if (item.second.pass_desc.pass_name == pass_desc.pass_name && item.second.pass_desc.stage == pass_desc.stage) {
        GELOGW("Python resource-plan pass[%s] is already registered.", pass_desc.pass_name.c_str());
        return false;
      }
    }
    entries_.emplace(pass_desc.descriptor_key, PythonResourcePlanRuntimeEntry{pass_desc, callbacks});
    return true;
  }

  bool Unregister(const std::string &descriptor_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.erase(descriptor_key) != 0U;
  }

  bool Get(const std::string &descriptor_key, PythonResourcePlanRuntimeEntry &entry) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = entries_.find(descriptor_key);
    if (iter == entries_.cend()) {
      return false;
    }
    entry = iter->second;
    return true;
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
  }

 private:
  mutable std::mutex mutex_;
  std::map<std::string, PythonResourcePlanRuntimeEntry> entries_;
};

PythonResourcePlanRuntimeRegistryImpl &GetPythonResourcePlanRuntimeRegistry() {
  static PythonResourcePlanRuntimeRegistryImpl registry;
  return registry;
}

class StreamHolderGuard {
 public:
  StreamHolderGuard(const PythonResourcePlanRuntimeEntry &entry, void *holder) : entry_(entry), holder_(holder) {}
  ~StreamHolderGuard() {
    if (holder_ != nullptr && entry_.callbacks.destroy != nullptr) {
      entry_.callbacks.destroy(holder_);
    }
  }
  StreamHolderGuard(const StreamHolderGuard &) = delete;
  StreamHolderGuard &operator=(const StreamHolderGuard &) = delete;

 private:
  const PythonResourcePlanRuntimeEntry &entry_;
  void *holder_{nullptr};
};
}  // namespace

Status RunPythonResourcePlanPass(const std::string &descriptor_key, const ConstGraphPtr &graph,
                                 CustomPassContext &pass_context) {
  if (graph == nullptr) {
    pass_context.SetErrorMessage("python resource-plan pass graph is null");
    return FAILED;
  }

  PythonResourcePlanRuntimeEntry entry;
  if (!GetPythonResourcePlanRuntimeRegistry().Get(descriptor_key, entry)) {
    pass_context.SetErrorMessage("python resource-plan pass descriptor is not registered");
    return FAILED;
  }
  if (!entry.callbacks.IsValid()) {
    pass_context.SetErrorMessage("python resource-plan pass callbacks are invalid");
    return FAILED;
  }

  void *holder = entry.callbacks.create(&entry.pass_desc);
  if (holder == nullptr) {
    pass_context.SetErrorMessage("python resource-plan pass holder creation failed");
    return FAILED;
  }
  StreamHolderGuard holder_guard(entry, holder);
  return entry.callbacks.run_resource_plan(holder, graph, pass_context);
}

struct ResourcePlanHandler {
  PythonPassKind kind;
  const char *name;
  bool (*validate)(const PythonPassDescriptor &);
  bool (*register_pass)(const PythonPassDescriptor &, const PythonResourcePlanPassCallbacks &);
  Status (*run)(const PythonPassDescriptor &, const ConstGraphPtr &, CustomPassContext &);
};

bool ValidateStreamResourcePlan(const PythonPassDescriptor &pass_desc) {
  return pass_desc.IsValid() && pass_desc.kind == PythonPassKind::kStream &&
         pass_desc.stage == CustomPassStage::kAfterAssignLogicStream;
}

Status RunStreamResourcePlan(const PythonPassDescriptor &pass_desc, const ConstGraphPtr &graph,
                             CustomPassContext &pass_context) {
  auto *stream_context = dynamic_cast<StreamPassContext *>(&pass_context);
  if (stream_context == nullptr) {
    pass_context.SetErrorMessage("resource-plan stream pass requires StreamPassContext");
    return FAILED;
  }
  return RunPythonResourcePlanPass(pass_desc.descriptor_key, graph, *stream_context);
}

bool RegisterStreamResourcePlan(const PythonPassDescriptor &pass_desc,
                                const PythonResourcePlanPassCallbacks &callbacks) {
  if (!GetPythonResourcePlanRuntimeRegistry().Register(pass_desc, callbacks)) {
    return false;
  }

  PassRegistrationData reg_data(pass_desc.pass_name);
  reg_data.CustomAllocateStreamPassFn([pass_desc](const ConstGraphPtr &graph, StreamPassContext &context) -> Status {
    // Route through the kind-specific handler so the concrete context
    // contract is checked at the adapter boundary.
    return RunStreamResourcePlan(pass_desc, graph, context);
  });
  // Python stream resource-plan passes are intentionally fixed to the post logic-stream stage.
  // Keep the registration metadata in sync with the descriptor so the common
  // pass helper accepts the callback and executes it in the promised phase.
  reg_data.Stage(CustomPassStage::kAfterAssignLogicStream);
  if (!CustomPassHelper::Instance().RegisterPythonResourcePlanPass(reg_data)) {
    (void)GetPythonResourcePlanRuntimeRegistry().Unregister(pass_desc.descriptor_key);
    return false;
  }
  GELOGI("Python resource-plan pass[%s] registered, descriptor key[%s].", pass_desc.pass_name.c_str(),
         pass_desc.descriptor_key.c_str());
  return true;
}

const ResourcePlanHandler *FindResourcePlanHandler(PythonPassKind kind) {
  static const ResourcePlanHandler kHandlers[] = {
      {PythonPassKind::kStream, "stream", &ValidateStreamResourcePlan, &RegisterStreamResourcePlan,
       &RunStreamResourcePlan},
  };
  for (const auto &handler : kHandlers) {
    if (handler.kind == kind) {
      return &handler;
    }
  }
  return nullptr;
}

bool RegisterPythonResourcePlanPass(const PythonPassDescriptor &pass_desc,
                                    const PythonResourcePlanPassCallbacks &callbacks) {
  const auto *handler = FindResourcePlanHandler(pass_desc.kind);
  if ((handler == nullptr) || !handler->validate(pass_desc) || !callbacks.IsValid()) {
    GELOGW("Unknown or invalid resource-plan kind[%u] for descriptor[%s].", static_cast<uint32_t>(pass_desc.kind),
           pass_desc.descriptor_key.c_str());
    return false;
  }
  return handler->register_pass(pass_desc, callbacks);
}

void ClearPythonResourcePlanRuntimeRegistry() {
  GetPythonResourcePlanRuntimeRegistry().Clear();
}
}  // namespace fusion
}  // namespace ge
