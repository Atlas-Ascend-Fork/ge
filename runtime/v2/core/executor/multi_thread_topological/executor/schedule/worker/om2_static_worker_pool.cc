/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "common/ge_visibility.h"
#include "core/builder/executor_builder.h"
#include "core/executor/multi_thread_topological/executor/schedule/task/exec_task.h"
#include "core/executor/multi_thread_topological/executor/schedule/task/task_package.h"
#include "core/executor/multi_thread_topological/executor/schedule/worker/task_worker_factory.h"
#include "exe_graph/runtime/kernel_context.h"

namespace gert {
namespace {
using Om2StaticWorkerFunc = uint32_t (*)(void *, size_t);

struct WorkerInvocation {
  Om2StaticWorkerFunc func = nullptr;
  void *opaque = nullptr;
  size_t worker_id = 0U;
  std::atomic<bool> *abort = nullptr;
};

KernelStatus RunWorkerInvocation(KernelRunContext *context) {
  if ((context == nullptr) || (context->input_size != 1U) || (context->values[0] == nullptr)) {
    return ge::GRAPH_FAILED;
  }
  const auto *const chain = reinterpret_cast<const Chain *>(context->values[0]);
  const auto *const invocation = chain->GetValue<WorkerInvocation *>();
  if ((invocation == nullptr) || (invocation->func == nullptr) ||
      ((invocation->abort != nullptr) && invocation->abort->load(std::memory_order_acquire))) {
    return ge::GRAPH_FAILED;
  }
  return invocation->func(invocation->opaque, invocation->worker_id);
}

// 由 OM2 pbody 显式创建和销毁，普通 RT2 执行不会创建此线程池。
class Om2StaticWorkerPool {
 public:
  explicit Om2StaticWorkerPool(const std::vector<ExecTaskType> &worker_types)
      : invocations_(new (std::nothrow) WorkerInvocation[worker_types.size()]),
        invocation_chains_(new (std::nothrow) Chain[worker_types.size()]()) {
    if ((invocations_ == nullptr) || (invocation_chains_ == nullptr)) {
      return;
    }
    workers_.reserve(worker_types.size());
    tasks_.reserve(worker_types.size());
    nodes_.reserve(worker_types.size());
    for (size_t i = 0U; i < worker_types.size(); ++i) {
      TaskWorkerConfig config;
      config.name = "om2_static_" + std::to_string(i);
      config.bind_task_type = worker_types[i];
      config.thread_mode = TaskThreadMode::URGENT;
      config.thread_count = 1U;
      config.pending_queue_size_log2 = 3U;
      config.completed_queue_size_log2 = 3U;
      auto worker = std::unique_ptr<TaskWorker>(TaskWorkerFactory::GetInstance().Create(config));
      if ((worker == nullptr) || !worker->Start()) {
        if (worker != nullptr) {
          worker->WakeupThreads();
          worker->Stop();
        }
        return;
      }

      invocations_[i].worker_id = i;
      invocations_[i].abort = &abort_;
      invocation_chains_[i].Set(&invocations_[i], nullptr);
      auto *input = reinterpret_cast<AsyncAnyValue *>(&invocation_chains_[i]);
      auto *node = CreateNode(i, 1U, &input, 0U, nullptr);
      if (node == nullptr) {
        worker->WakeupThreads();
        worker->Stop();
        return;
      }
      node->func = &RunWorkerInvocation;
      auto task = std::make_unique<ExecTask>(i, worker_types[i], node);
      if (task == nullptr) {
        std::free(node);
        worker->WakeupThreads();
        worker->Stop();
        return;
      }
      task->SetIndegree(0);
      nodes_.push_back(node);
      tasks_.emplace_back(std::move(task));
      workers_.emplace_back(std::move(worker));
    }
    valid_ = workers_.size() == worker_types.size();
  }

  ~Om2StaticWorkerPool() {
    for (auto &worker : workers_) {
      worker->WakeupThreads();
      worker->Stop();
    }
    tasks_.clear();
    for (auto *const node : nodes_) {
      std::free(node);
    }
  }

  bool IsValid() const {
    return valid_;
  }

  uint32_t Run(void *const stream, const Om2StaticWorkerFunc func, void *const opaque) {
    if (!valid_ || (stream == nullptr) || (func == nullptr)) {
      return ge::GRAPH_FAILED;
    }
    const std::lock_guard<std::mutex> lock(run_mutex_);
    abort_.store(false, std::memory_order_relaxed);
    size_t submitted = 0U;
    for (size_t i = 0U; i < workers_.size(); ++i) {
      invocations_[i].func = func;
      invocations_[i].opaque = opaque;
      workers_[i]->SetExecuteStream(stream);
      if (!workers_[i]->Submit(*tasks_[i])) {
        break;
      }
      ++submitted;
    }
    if (submitted != workers_.size()) {
      abort_.store(true, std::memory_order_release);
    }
    for (size_t i = 0U; i < submitted; ++i) {
      workers_[i]->WakeupThreads();
    }

    uint32_t result = submitted == workers_.size() ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
    size_t completed = 0U;
    while (completed < submitted) {
      bool fetched = false;
      for (size_t i = 0U; i < submitted; ++i) {
        TaskPackage package;
        if (workers_[i]->Fetch(package) == 0U) {
          continue;
        }
        fetched = true;
        while (auto *const task = package.pop_front()) {
          if (task->GetKernelRet() != ge::GRAPH_SUCCESS) {
            result = task->GetKernelRet();
          }
          ++completed;
        }
      }
      if (!fetched) {
        std::this_thread::yield();
      }
    }
    for (auto &worker : workers_) {
      worker->SleepThreads();
    }
    return result;
  }

 private:
  bool valid_ = false;
  std::unique_ptr<WorkerInvocation[]> invocations_;
  std::unique_ptr<Chain[]> invocation_chains_;
  std::vector<std::unique_ptr<TaskWorker>> workers_;
  std::vector<std::unique_ptr<ExecTask>> tasks_;
  std::vector<Node *> nodes_;
  std::atomic<bool> abort_{false};
  std::mutex run_mutex_;
};
}  // namespace
}  // namespace gert

extern "C" VISIBILITY_EXPORT void *GeOm2CreateStaticWorkerPool(const size_t worker_count,
                                                               const uint32_t *const worker_types) {
  if ((worker_count < 2U) || (worker_types == nullptr)) {
    return nullptr;
  }
  std::vector<gert::ExecTaskType> types;
  types.reserve(worker_count);
  for (size_t i = 0U; i < worker_count; ++i) {
    if (worker_types[i] >= static_cast<uint32_t>(gert::ExecTaskType::MAX)) {
      return nullptr;
    }
    types.push_back(static_cast<gert::ExecTaskType>(worker_types[i]));
  }
  auto pool = std::make_unique<gert::Om2StaticWorkerPool>(types);
  if ((pool == nullptr) || !pool->IsValid()) {
    return nullptr;
  }
  return pool.release();
}

extern "C" VISIBILITY_EXPORT uint32_t GeOm2RunStaticWorkerPool(void *const handle, void *const stream,
                                                               uint32_t (*const func)(void *, size_t),
                                                               void *const opaque) {
  if (handle == nullptr) {
    return ge::GRAPH_FAILED;
  }
  return static_cast<gert::Om2StaticWorkerPool *>(handle)->Run(stream, func, opaque);
}

extern "C" VISIBILITY_EXPORT void GeOm2DestroyStaticWorkerPool(void *const handle) {
  delete static_cast<gert::Om2StaticWorkerPool *>(handle);
}
