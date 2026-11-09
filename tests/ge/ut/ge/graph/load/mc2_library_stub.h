/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_MC2_LIBRARY_STUB_H_
#define GE_TESTS_MC2_LIBRARY_STUB_H_

#include <dlfcn.h>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "graph/load/model_manager/task_info/hccl/hccl_util.h"

namespace ge {
// Scope loader interception to MC2 interface calls, before loading the rest of a graph.
class Mc2LibraryStub {
 public:
  Mc2LibraryStub() : saved_mgr_(HcclDllHcomMgr::GetInstance()) {
    auto &mgr = HcclDllHcomMgr::GetInstance();
    mgr.hcom_get_comm_handle_func_ = nullptr;
    mgr.mc2_ccu_kernel_launch_func_ = nullptr;
    mgr.hcomm_dll_handle_ = nullptr;
    mgr.mc2_dll_handle_ = nullptr;
    Active() = this;
    MOCKER(dlopen).stubs().will(invoke(Open));
    MOCKER(dlsym).stubs().will(invoke(Symbol));
  }

  ~Mc2LibraryStub() {
    StopIntercepting();
    HcclDllHcomMgr::GetInstance() = saved_mgr_;
    Active() = nullptr;
  }

  void StopIntercepting() {
    if (intercepting_) {
      EXPECT_NO_THROW(GlobalMockObject::verify());
      GlobalMockObject::reset();
      intercepting_ = false;
    }
  }

  std::string missing_library;
  std::string missing_symbol;
  HcclResult query_result = HCCL_SUCCESS;
  uint32_t launch_result = 0U;
  void *comm = nullptr;
  std::vector<std::string> groups;
  std::vector<std::pair<void *, void *>> launches;
  std::function<void()> on_launch;
  size_t open_calls = 0U;
  size_t symbol_calls = 0U;

 private:
  static Mc2LibraryStub *&Active() {
    static Mc2LibraryStub *active = nullptr;
    return active;
  }

  static void *Open(const char *name, int flags) {
    auto &self = *Active();
    if (name != nullptr && (std::string(name) == "libhcomm.so" || std::string(name) == "libmc2_client.so")) {
      ++self.open_calls;
      EXPECT_EQ(flags, RTLD_NOW | RTLD_LOCAL);
      if (self.missing_library == name) {
        return nullptr;
      }
      return std::string(name) == "libhcomm.so" ? &self.hcomm_handle_ : &self.mc2_handle_;
    }
    ADD_FAILURE() << "Unexpected MC2 test library: " << (name == nullptr ? "null" : name);
    return nullptr;
  }

  static void *Symbol(void *handle, const char *name) {
    auto &self = *Active();
    if (handle != &self.hcomm_handle_ && handle != &self.mc2_handle_) {
      ADD_FAILURE() << "Unexpected MC2 test symbol: " << name;
      return nullptr;
    }
    ++self.symbol_calls;
    if (self.missing_symbol == name) {
      return nullptr;
    }
    if (handle == &self.hcomm_handle_ && std::string(name) == "HcomGetCommHandleByGroup") {
      return reinterpret_cast<void *>(&GetComm);
    }
    EXPECT_EQ(handle, &self.mc2_handle_);
    EXPECT_STREQ(name, "CcuKernelLaunch");
    return reinterpret_cast<void *>(&Launch);
  }

  static HcclResult GetComm(const char *group, void **comm) {
    auto &self = *Active();
    self.groups.emplace_back(group);
    *comm = self.comm;
    return self.query_result;
  }

  static uint32_t Launch(void *comm, void *context) {
    auto &self = *Active();
    self.launches.emplace_back(comm, context);
    if (self.on_launch) {
      self.on_launch();
    }
    return self.launch_result;
  }

  HcclDllHcomMgr saved_mgr_;
  bool intercepting_ = true;
  uint64_t hcomm_handle_ = 0U;
  uint64_t mc2_handle_ = 0U;
};
}  // namespace ge

#endif  // GE_TESTS_MC2_LIBRARY_STUB_H_
