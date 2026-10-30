/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include "aicpu/kernel/aicpu_bin_handler.h"
#include "stub/gert_runtime_stub.h"

using namespace std;
using namespace gert;
namespace {
class MultiContextAclStub : public AclRuntimeStubImpl {
 public:
  aclError aclrtGetCurrentContext(aclrtContext *context) override {
    *context = current_context;
    return ACL_SUCCESS;
  }

  aclError aclrtBinaryLoadFromData(const void *data, size_t length, const aclrtBinaryLoadOptions *options,
                                   aclrtBinHandle *bin_handle) override {
    (void)data;
    (void)length;
    (void)options;
    *bin_handle = reinterpret_cast<void *>(0x2000UL);
    load_count++;
    return ACL_SUCCESS;
  }

  aclError aclrtBinaryUnLoad(aclrtBinHandle bin_handle) override {
    (void)bin_handle;
    unload_count++;
    return ACL_SUCCESS;
  }

  aclrtContext current_context = reinterpret_cast<void *>(1UL);
  size_t load_count = 0U;
  size_t unload_count = 0U;
};
}  // namespace
class BinHandlerUt : public testing::Test {};

TEST_F(BinHandlerUt, LoadJsonBinarySuccess) {
  OpJsonBinHandler bin_handler;
  TfJsonBinHandler::Instance();
  AicpuJsonBinHandler::Instance();
  auto ret = bin_handler.LoadBinary("test.json");
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  ret = bin_handler.LoadBinary("test.json");
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
}

TEST_F(BinHandlerUt, LoadJsonBinaryFail) {
  OpJsonBinHandler bin_handler;
  auto ret = bin_handler.LoadBinary("");
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(BinHandlerUt, LoadDataBinarySuccess) {
  OpDataBinHandler bin_handler;

  vector<char> buffer(1, 'a');
  ge::OpKernelBinPtr bin = std::make_shared<ge::OpKernelBin>("op", std::move(buffer));
  auto ret = bin_handler.LoadBinary("test.so", bin);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  ret = bin_handler.LoadBinary("test.so", bin);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
}

TEST_F(BinHandlerUt, LoadDataBinaryFailByNullptr) {
  OpDataBinHandler bin_handler;
  auto ret = bin_handler.LoadBinary("test.so", nullptr);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(BinHandlerUt, LoadDataBinaryFailByEmptySo) {
  OpDataBinHandler bin_handler;

  vector<char> buffer(1, 'a');
  ge::OpKernelBinPtr bin = std::make_shared<ge::OpKernelBin>("op", std::move(buffer));
  auto ret = bin_handler.LoadBinary("", bin);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(BinHandlerUt, LoadAndGetBinHandleSuccess) {
  const std::string so_name = "test.so";
  vector<char> buffer(1, 'a');
  ge::OpKernelBinPtr bin = std::make_shared<ge::OpKernelBin>("op", std::move(buffer));
  rtBinHandle handle = nullptr;
  void *resource_token = nullptr;
  auto ret = CustBinHandlerManager::Instance().LoadAndGetBinHandle(so_name, bin, handle, resource_token);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  rtBinHandle handle2 = nullptr;
  void *resource_token2 = nullptr;
  ret = CustBinHandlerManager::Instance().LoadAndGetBinHandle("test2.so", bin, handle2, resource_token2);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  rtBinHandle reused_handle = nullptr;
  void *reused_resource_token = nullptr;
  ret = CustBinHandlerManager::Instance().LoadAndGetBinHandle("test2.so", bin, reused_handle, reused_resource_token);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
  EXPECT_EQ(handle2, reused_handle);
  EXPECT_EQ(resource_token2, reused_resource_token);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(resource_token), ge::GRAPH_SUCCESS);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(resource_token2), ge::GRAPH_SUCCESS);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(reused_resource_token), ge::GRAPH_SUCCESS);
}

TEST_F(BinHandlerUt, LoadAndGetBinHandleFailed) {
  const std::string so_name = "";
  vector<char> buffer(1, 'a');
  ge::OpKernelBinPtr bin = std::make_shared<ge::OpKernelBin>("op", std::move(buffer));
  rtBinHandle handle = nullptr;
  void *resource_token = nullptr;
  auto ret = CustBinHandlerManager::Instance().LoadAndGetBinHandle(so_name, bin, handle, resource_token);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(BinHandlerUt, LoadAndGetBinHandleIsolatedByContext) {
  auto acl_stub = std::make_unique<MultiContextAclStub>();
  auto *const acl_stub_ptr = acl_stub.get();
  GertRuntimeStub runtime_stub(std::make_unique<RuntimeStubImpl>(), true, std::move(acl_stub));
  std::vector<char> buffer(1U, 'a');
  auto bin = std::make_shared<ge::OpKernelBin>("multi_context_op", std::move(buffer));

  rtBinHandle context0_handle = nullptr;
  void *context0_resource_token = nullptr;
  ASSERT_EQ(CustBinHandlerManager::Instance().LoadAndGetBinHandle("libmulti_context.so", bin, context0_handle,
                                                                  context0_resource_token),
            ge::GRAPH_SUCCESS);
  rtBinHandle reused_handle = nullptr;
  void *reused_resource_token = nullptr;
  ASSERT_EQ(CustBinHandlerManager::Instance().LoadAndGetBinHandle("libmulti_context.so", bin, reused_handle,
                                                                  reused_resource_token),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(context0_handle, reused_handle);
  EXPECT_EQ(acl_stub_ptr->load_count, 1U);

  acl_stub_ptr->current_context = reinterpret_cast<void *>(2UL);
  rtBinHandle context1_handle = nullptr;
  void *context1_resource_token = nullptr;
  ASSERT_EQ(CustBinHandlerManager::Instance().LoadAndGetBinHandle("libmulti_context.so", bin, context1_handle,
                                                                  context1_resource_token),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(context0_handle, context1_handle);
  EXPECT_NE(context0_resource_token, context1_resource_token);
  EXPECT_EQ(acl_stub_ptr->load_count, 2U);

  acl_stub_ptr->current_context = reinterpret_cast<void *>(1UL);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(context0_resource_token), ge::GRAPH_SUCCESS);
  EXPECT_EQ(acl_stub_ptr->unload_count, 0U);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(reused_resource_token), ge::GRAPH_SUCCESS);
  EXPECT_EQ(acl_stub_ptr->unload_count, 1U);
  acl_stub_ptr->current_context = reinterpret_cast<void *>(2UL);
  EXPECT_EQ(CustBinHandlerManager::Instance().ReleaseBinHandle(context1_resource_token), ge::GRAPH_SUCCESS);
  EXPECT_EQ(acl_stub_ptr->unload_count, 2U);
}
