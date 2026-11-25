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

#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "runtime/om2/om2_args_handler.h"

namespace ge {
namespace {
constexpr size_t kTestCapacity = 256UL;
constexpr size_t kTestAlign = 8UL;

class FakeArgsAllocator {
 public:
  explicit FakeArgsAllocator(const size_t capacity = kTestCapacity)
      : host_buf_(capacity, 0U), device_buf_(capacity, 0U) {}

  static int32_t Allocate(void *args_table, uint64_t size, void **host_addr, void **device_addr) {
    auto *self = static_cast<FakeArgsAllocator *>(args_table);
    return self->DoAllocate(static_cast<size_t>(size), host_addr, device_addr);
  }

  size_t last_request_size() const {
    return last_request_size_;
  }

 private:
  int32_t DoAllocate(const size_t size, void **host_addr, void **device_addr) {
    last_request_size_ = size;
    if ((host_addr == nullptr) || (device_addr == nullptr) || (size > (host_buf_.size() - offset_))) {
      return ACL_ERROR_FAILURE;
    }
    *host_addr = host_buf_.data() + offset_;
    *device_addr = device_buf_.data() + offset_;
    offset_ += size;
    return ACL_SUCCESS;
  }

  std::vector<uint8_t> host_buf_;
  std::vector<uint8_t> device_buf_;
  size_t last_request_size_{0UL};
  size_t offset_{0UL};
};

TEST(Om2ArgsHandlerTest, MallocReadOnlyDevArgs_ShouldRequestAlignedSize_WhenArgsSizeUnaligned) {
  FakeArgsAllocator allocator;
  Om2ArgsHandler handler(&allocator, &FakeArgsAllocator::Allocate);
  uint8_t host_args[12] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

  const gert::KernelArgs *first = handler.MallocReadOnlyDevArgs(host_args, sizeof(host_args));
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(allocator.last_request_size(), 16UL);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(first->args_data) % kTestAlign, 0UL);
  EXPECT_EQ(first->args_size, sizeof(host_args));
  EXPECT_EQ(first->placement, kPlacementDevice);

  const gert::KernelArgs *second = handler.MallocReadOnlyDevArgs(host_args, sizeof(host_args));
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(allocator.last_request_size(), 16UL);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(second->args_data) % kTestAlign, 0UL);
  EXPECT_EQ(second->args_size, sizeof(host_args));
  EXPECT_EQ(reinterpret_cast<uintptr_t>(second->args_data) - reinterpret_cast<uintptr_t>(first->args_data), 16UL);

  const auto &host_args_deque = handler.GetKernelArgs(kPlacementHost);
  const auto &device_args_deque = handler.GetKernelArgs(kPlacementDevice);
  ASSERT_EQ(host_args_deque.size(), 2UL);
  ASSERT_EQ(device_args_deque.size(), 2UL);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(host_args_deque[0].args_data) % kTestAlign, 0UL);
  EXPECT_EQ(device_args_deque[0].args_data, first->args_data);
  EXPECT_EQ(std::memcmp(host_args_deque[0].args_data, host_args, sizeof(host_args)), 0);
}

TEST(Om2ArgsHandlerTest, MallocReadOnlyDevArgs_ShouldKeepRequestSize_WhenArgsSizeAligned) {
  FakeArgsAllocator allocator;
  Om2ArgsHandler handler(&allocator, &FakeArgsAllocator::Allocate);
  uint64_t host_args[3] = {0x11U, 0x22U, 0x33U};

  const gert::KernelArgs *result = handler.MallocReadOnlyDevArgs(host_args, sizeof(host_args));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(allocator.last_request_size(), sizeof(host_args));
  EXPECT_EQ(reinterpret_cast<uintptr_t>(result->args_data) % kTestAlign, 0UL);
  EXPECT_EQ(result->args_size, sizeof(host_args));
}

TEST(Om2ArgsHandlerTest, MallocReadOnlyDevArgs_ShouldReturnNullptr_WhenParamInvalid) {
  FakeArgsAllocator allocator;
  Om2ArgsHandler handler(&allocator, &FakeArgsAllocator::Allocate);
  uint64_t host_args = 0U;

  EXPECT_EQ(handler.MallocReadOnlyDevArgs(nullptr, sizeof(host_args)), nullptr);
  EXPECT_EQ(handler.MallocReadOnlyDevArgs(&host_args, 0U), nullptr);
  EXPECT_EQ(handler.MallocReadOnlyDevArgs(&host_args, std::numeric_limits<size_t>::max()), nullptr);
}

TEST(Om2ArgsHandlerTest, MallocReadOnlyDevArgs_ShouldReturnNullptr_WhenArgsPoolExhausted) {
  FakeArgsAllocator allocator(16UL);
  Om2ArgsHandler handler(&allocator, &FakeArgsAllocator::Allocate);
  uint64_t host_args[2] = {1U, 2U};

  EXPECT_NE(handler.MallocReadOnlyDevArgs(host_args, sizeof(host_args)), nullptr);
  EXPECT_EQ(handler.MallocReadOnlyDevArgs(host_args, sizeof(host_args)), nullptr);
}
}  // namespace
}  // namespace ge
