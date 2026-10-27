/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "aprof_pub.h"
#include "acl_stub.h"
#include "runtime/subscriber/global_profiler.h"
#include <vector>

std::vector<ProfCommandHandle> &GetProfCtrlHandles() {
  static std::vector<ProfCommandHandle> profCtrlHandles;
  return profCtrlHandles;
}

int32_t MsprofRegisterCallback(uint32_t moduleId, ProfCommandHandle handle) {
  if (handle != nullptr) {
    GetProfCtrlHandles().push_back(handle);
  }
  return MSPROF_ERROR_NONE;
}

int32_t ProfAclCfgToSampleCfg(const std::string &aclCfg, std::string &sampleCfg) {
  return 0;
}

int32_t aclStub::MsprofFinalize() {
  return 0;
}

int32_t aclStub::MsprofInit(uint32_t aclDataType, void *data, uint32_t dataLen) {
  return 0;
}

int32_t aclStub::MsprofRegTypeInfo(uint16_t level, uint32_t typeId, const char *typeName) {
  return 0;
}

uint64_t aclStub::MsprofSysCycleTime() {
  return 0;
}

int32_t aclStub::MsprofReportApi(uint32_t agingFlag, const void *api) {
  return 0;
}

int32_t MsprofFinalize() {
  return MockFunctionTest::aclStubInstance().MsprofFinalize();
}

int32_t MsprofInit(uint32_t aclDataType, void *data, uint32_t dataLen) {
  return MockFunctionTest::aclStubInstance().MsprofInit(aclDataType, data, dataLen);
}

int32_t MsprofReportApi(uint32_t agingFlag, const MsprofApi *api) {
  return MockFunctionTest::aclStubInstance().MsprofReportApi(agingFlag, static_cast<const void *>(api));
}

int32_t MsprofRegTypeInfo(uint16_t level, uint32_t typeId, const char *typeName) {
  return MockFunctionTest::aclStubInstance().MsprofRegTypeInfo(level, typeId, typeName);
}

uint64_t MsprofSysCycleTime() {
  return MockFunctionTest::aclStubInstance().MsprofSysCycleTime();
}

int32_t MsprofReportEvent(uint32_t agingFlag, const MsprofEvent *event) {
  return 0;
}

gert::GlobalProfilingWrapper *gert::GlobalProfilingWrapper::GetInstance() {
  static gert::GlobalProfilingWrapper global_prof_wrapper;
  return &global_prof_wrapper;
}
