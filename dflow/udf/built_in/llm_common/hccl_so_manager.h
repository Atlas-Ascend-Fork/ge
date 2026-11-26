/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef BUILT_IN_LLM_COMMON_HCCL_SO_MANAGER_H
#define BUILT_IN_LLM_COMMON_HCCL_SO_MANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include "hccl/base.h"

namespace FlowFunc {
constexpr const char *kHcclRawListenFuncName = "HcclRawListen";
constexpr const char *kHcclRawConnectFuncName = "HcclRawConnect";
constexpr const char *kHcclRawAcceptFuncName = "HcclRawAccept";
constexpr const char *kHcclRawIsendFuncName = "HcclRawIsend";
constexpr const char *kHcclRawImprobeFuncName = "HcclRawImprobe";
constexpr const char *kHcclRawGetCountFuncName = "HcclRawGetCount";
constexpr const char *kHcclRawImRecvFuncName = "HcclRawImrecv";
constexpr const char *kHcclRawImRecvScatterFuncName = "HcclRawImrecvScatter";
constexpr const char *kHcclRawTestSomeFuncName = "HcclRawTestSome";
constexpr const char *kHcclRawOpenFuncName = "HcclRawOpen";
constexpr const char *kHcclRawCloseFuncName = "HcclRawClose";
constexpr const char *kHcclRawForceCloseFuncName = "HcclRawForceClose";
constexpr const char *kHcclRawBindFuncName = "HcclRawBind";
constexpr const char *kHcclRegisterGlobalMemoryFuncName = "HcclRegisterGlobalMemory";
constexpr const char *kHcclUnregisterGlobalMemoryFuncName = "HcclUnregisterGlobalMemory";

// Raw API
using HcclRawAcceptFunc = HcclResult (*)(HcclConn, HcclAddr *, HcclConn *);
using HcclRawIsendFunc = HcclResult (*)(const void *, int, HcclDataType, HcclConn, HcclRequest *);
using HcclRawImprobeFunc = HcclResult (*)(HcclConn, int *, HcclMessage *, HcclStatus *);
using HcclRawGetCountFunc = HcclResult (*)(const HcclStatus *, HcclDataType, int *);
using HcclRawImrecvFunc = HcclResult (*)(void *, int, HcclDataType, HcclMessage *, HcclRequest *);
using HcclRawImrecvScatterFunc = HcclResult (*)(void *[], int[], int, HcclDataType, HcclMessage *, HcclRequest *);
using HcclRawTestSomeFunc = HcclResult (*)(int, HcclRequest[], int *, int[], HcclStatus[]);
using HcclRawBindFunc = HcclResult (*)(HcclConn conn, HcclAddr *bind_addr);
using HcclRawConnectFunc = HcclResult (*)(HcclConn conn, HcclAddr *connect_addr);
using HcclRawListenFunc = HcclResult (*)(HcclConn conn, int backLog);
using HcclRawOpenFunc = HcclResult (*)(HcclConn *conn);
using HcclRawCloseFunc = HcclResult (*)(HcclConn conn);
using HcclRawForceCloseFunc = HcclResult (*)(HcclConn conn);
using HcclRegisterGlobalMemoryFunc = HcclResult (*)(void *addr, u64 size);
using HcclUnregisterGlobalMemoryFunc = HcclResult (*)(void *addr);

class HcclSoManager {
 public:
  struct SoLoader {
    std::function<void *(const char *file, int mode)> open;
    std::function<void *(void *handle, const char *name)> symbol;
    std::function<const char *()> error;
    std::function<int(void *handle)> close;
  };

  static HcclSoManager *GetInstance();
  ~HcclSoManager();
  int32_t LoadSo();
  int32_t UnloadSo();
  void *GetFunc(const std::string &name) const;
  void SetLoader(SoLoader loader);
  static SoLoader DefaultLoader();
  HcclSoManager(const HcclSoManager &) = delete;
  HcclSoManager(const HcclSoManager &&) = delete;
  HcclSoManager &operator=(const HcclSoManager &) = delete;
  HcclSoManager &operator=(const HcclSoManager &&) = delete;

 private:
  HcclSoManager();
  std::unordered_map<std::string, void *> func_map_;
  void *so_handle_ = nullptr;
  SoLoader loader_;
};
}  // namespace FlowFunc

#endif  // BUILT_IN_LLM_COMMON_HCCL_SO_MANAGER_H
