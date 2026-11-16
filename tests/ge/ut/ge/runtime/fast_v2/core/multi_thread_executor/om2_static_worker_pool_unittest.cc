/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <memory>
#include "core/executor/multi_thread_topological/executor/schedule/task/exec_task_type.h"
#include "stub/gert_runtime_stub.h"

extern "C" void *GeOm2CreateStaticWorkerPool(size_t count, const uint32_t *types);
extern "C" uint32_t GeOm2RunStaticWorkerPool(void *handle, void *stream, uint32_t (*func)(void *, size_t),
                                             void *opaque);
extern "C" void GeOm2DestroyStaticWorkerPool(void *handle);

namespace gert {
namespace {
struct WorkerResults {
  std::array<std::atomic<size_t>, 3U> calls{};
  bool fail = false;
};

uint32_t RecordInvocation(void *opaque, size_t worker) {
  auto &results = *static_cast<WorkerResults *>(opaque);
  if (worker >= results.calls.size()) {
    return ge::GRAPH_PARAM_INVALID;
  }
  results.calls[worker].fetch_add(1U);
  return results.fail && worker == 1U ? ge::GRAPH_PARAM_INVALID : ge::GRAPH_SUCCESS;
}

TEST(Om2StaticWorkerPoolUT, ReusesAllWorkerKindsAndRecoversAfterCallbackFailure) {
  GertRuntimeStub stub;
  const uint32_t types[]{static_cast<uint32_t>(ExecTaskType::NORMAL), static_cast<uint32_t>(ExecTaskType::MEMORY),
                         static_cast<uint32_t>(ExecTaskType::LAUNCH)};
  std::unique_ptr<void, decltype(&GeOm2DestroyStaticWorkerPool)> pool(GeOm2CreateStaticWorkerPool(3U, types),
                                                                      GeOm2DestroyStaticWorkerPool);
  ASSERT_NE(pool, nullptr);
  WorkerResults results;
  int stream = 0;
  size_t runs = 0U;
  for (const bool fail : {false, true, false}) {
    results.fail = fail;
    EXPECT_EQ(GeOm2RunStaticWorkerPool(pool.get(), &stream, RecordInvocation, &results),
              fail ? ge::GRAPH_PARAM_INVALID : ge::GRAPH_SUCCESS);
    ++runs;
    for (const auto &count : results.calls) {
      EXPECT_EQ(count.load(), runs);
    }
  }
  EXPECT_NE(GeOm2RunStaticWorkerPool(pool.get(), nullptr, RecordInvocation, &results), ge::GRAPH_SUCCESS);
  EXPECT_NE(GeOm2RunStaticWorkerPool(pool.get(), &stream, nullptr, &results), ge::GRAPH_SUCCESS);
  for (const auto &count : results.calls) {
    EXPECT_EQ(count.load(), runs);
  }
}

TEST(Om2StaticWorkerPoolUT, RejectsInvalidCreationAndNullHandles) {
  const uint32_t normal[]{0U, 0U};
  const uint32_t invalid[]{0U, static_cast<uint32_t>(ExecTaskType::MAX)};
  struct InvalidPool {
    size_t count;
    const uint32_t *types;
  };
  for (const auto &item :
       {InvalidPool{0U, normal}, InvalidPool{1U, normal}, InvalidPool{2U, nullptr}, InvalidPool{2U, invalid}}) {
    std::unique_ptr<void, decltype(&GeOm2DestroyStaticWorkerPool)> pool(
        GeOm2CreateStaticWorkerPool(item.count, item.types), GeOm2DestroyStaticWorkerPool);
    EXPECT_EQ(pool, nullptr);
  }
  EXPECT_NE(GeOm2RunStaticWorkerPool(nullptr, nullptr, nullptr, nullptr), ge::GRAPH_SUCCESS);
  GeOm2DestroyStaticWorkerPool(nullptr);
}
}  // namespace
}  // namespace gert
