/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <array>
#include <map>
#include <string>
#include <vector>
#include <securec.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#define private public
#include "device_capability.h"
#undef private
#include "hccl_stub.h"
#include "hcom_graph_mc2.h"
#include "hcom_acl_adapter.h"
#include "hcom/hcom_topo_info.h"
#include "graph/debug/ge_attr_define.h"
#include "register/op_tiling_info.h"
#include "acl/acl_rt.h"

namespace hccl {
using Mc2AllocFunc = HcclResult (*)(HcclComm, void *, void *, void **);
Mc2AllocFunc GetA5Mc2AllocFunc();
}  // namespace hccl

#ifndef OPEN_BUILD_PROJECT
namespace {
using namespace hccl;
constexpr char kFirstGroup[] = "st_mc2_first";
constexpr char kSecondGroup[] = "st_mc2_second";
struct Mc2ResourceCapture {
  uint64_t comm = 0U;
  uint64_t stream = 0U;
  uint64_t op_stream = 0U;
  std::array<uint64_t, 2> contexts{};
  std::array<uint64_t, 2> notifies{};
  std::vector<std::string> groups;
  std::map<std::string, void *> ordered_streams;
  std::map<std::string, void *> published_notifies;
  std::vector<void *> destroyed_notifies;
  size_t allocations = 0U;
  size_t legacy_allocations = 0U;
  size_t notify_count = 0U;
  uint32_t version = 101U;
  HcclResult allocation_result = HCCL_SUCCESS;
  bool null_context = false;
} capture;

HcclResult GetComm(const char *group, HcclComm *comm) {
  capture.groups.emplace_back(group);
  *comm = &capture.comm;
  return HCCL_SUCCESS;
}

HcclResult Allocate(HcclComm comm, void *stream, void *tiling, void **context) {
  EXPECT_EQ(comm, &capture.comm);
  EXPECT_EQ(stream, &capture.stream);
  uint32_t version = 0U;
  EXPECT_EQ(memcpy_s(&version, sizeof(version), tiling, sizeof(version)), EOK);
  EXPECT_EQ(version, capture.version);
  *context = capture.null_context ? nullptr : &capture.contexts[capture.allocations];
  ++capture.allocations;
  return capture.allocation_result;
}

HcclResult AllocateLegacy(HcclComm comm, void *stream, void *tiling, void **context) {
  ++capture.legacy_allocations;
  return Allocate(comm, stream, tiling, context);
}

aclError CreateNotify(aclrtNotify *notify, uint32_t flags) {
  EXPECT_EQ(flags, static_cast<uint32_t>(ACL_NOTIFY_DEFAULT));
  *notify = &capture.notifies[capture.notify_count++];
  return ACL_SUCCESS;
}

aclError DestroyNotify(aclrtNotify notify) {
  capture.destroyed_notifies.push_back(notify);
  return ACL_SUCCESS;
}

ge::graphStatus SetOrderedStream(ge::HcomTopoInfo *, int32_t device, const char *group, void *stream) {
  EXPECT_EQ(device, 0);
  capture.ordered_streams[group] = stream;
  return ge::GRAPH_SUCCESS;
}

void UnsetOrderedStream(ge::HcomTopoInfo *, int32_t device, const char *group) {
  EXPECT_EQ(device, 0);
  EXPECT_EQ(capture.ordered_streams.erase(group), 1U);
}

bool GetTopo(ge::HcomTopoInfo *, const char *, ge::HcomTopoInfo::TopoInfo &topo) {
  topo = {};
  topo.rank_size = 2;
  return true;
}

ge::graphStatus SetTopo(ge::HcomTopoInfo *, const char *group, const ge::HcomTopoInfo::TopoInfo &topo) {
  EXPECT_EQ(topo.rank_size, 2);
  capture.published_notifies[group] = topo.notify_handle;
  return ge::GRAPH_SUCCESS;
}

class HcomMc2ResourceST : public testing::Test {
 protected:
  void SetUp() override {
    saved_capability_ = DeviceCapability::Instance().supportsMc2Resource_;
    saved_v2_kernel_ = DeviceCapability::Instance().supportsV2Kernel_;
    DeviceCapability capability;
    capability.InitFromSocVersion("Ascend950");
    DeviceCapability::Instance().supportsMc2Resource_ = capability.SupportsMc2Resource();
    DeviceCapability::Instance().supportsV2Kernel_ = false;
    capture = {};
    op_ = std::make_shared<ge::OpDesc>();
    saved_type_ = op_->GetType();
    op_->SetType("AllGatherCustomV3");
    const std::vector<std::string> groups{kFirstGroup, "", kSecondGroup};
    MOCKER(HcomGetGroupsByOpDesc).stubs().with(mockcpp::any(), outBound(groups)).will(returnValue(true));
    op_->SetExtAttr("_rt_resource_list", std::make_shared<std::vector<void *>>(1U, &capture.stream));
    MOCKER(HcomGetCommHandleByGroup).stubs().will(invoke(GetComm));
    MOCKER(GetA5Mc2AllocFunc).stubs().will(returnValue(static_cast<Mc2AllocFunc>(&Allocate)));
    MOCKER(HcclAllocComResourceByTiling).stubs().will(invoke(AllocateLegacy));
    MOCKER_CPP(&ge::HcomTopoInfo::SetGroupOrderedStream).stubs().will(invoke(SetOrderedStream));
    MOCKER_CPP(&ge::HcomTopoInfo::UnsetGroupOrderedStream).stubs().will(invoke(UnsetOrderedStream));
    MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).stubs().will(invoke(GetTopo));
    MOCKER_CPP(&ge::HcomTopoInfo::SetGroupTopoInfo).stubs().will(invoke(SetTopo));
    MOCKER(aclrtDestroyNotify).stubs().will(invoke(DestroyNotify));
  }

  void TearDown() override {
    EXPECT_NO_THROW(ReleaseA5AicpuGraphSyncResource(kFirstGroup));
    EXPECT_NO_THROW(ReleaseA5AicpuGraphSyncResource(kSecondGroup));
    EXPECT_TRUE(capture.ordered_streams.empty());
    EXPECT_EQ(capture.destroyed_notifies.size(), capture.notify_count);
    op_->SetType(saved_type_);
    DeviceCapability::Instance().supportsMc2Resource_ = saved_capability_;
    DeviceCapability::Instance().supportsV2Kernel_ = saved_v2_kernel_;
    EXPECT_NO_THROW(GlobalMockObject::verify());
    GlobalMockObject::reset();
  }

  void SetTiling(uint32_t version, const std::vector<uint8_t> &engines) {
    capture.version = version;
    std::array<uint32_t, 16> header{};
    header[0] = version;
    header[1] = engines.size();
    std::string data(sizeof(header) + engines.size() * 16U, '\0');
    for (size_t i = 0U; i < engines.size(); ++i) {
      header[i + 2U] = sizeof(header) + i * 16U;
      data[header[i + 2U] + 13U] = static_cast<char>(engines[i]);
    }
    ASSERT_EQ(memcpy_s(&data[0], data.size(), header.data(), sizeof(header)), EOK);
    auto run_info = std::make_shared<optiling::utils::OpRunInfo>(0, false, 0);
    run_info->AddTilingData(data.data(), data.size());
    ASSERT_TRUE(op_->SetExtAttr(ge::ATTR_NAME_OP_RUN_INFO, run_info));
  }

  void ExpectNoAicpuResources() {
    MOCKER(hrtStreamGetMode).expects(never());
    MOCKER(HcomMc2AiCpuStreamAllocAndGet).expects(never());
    MOCKER(hrtGetDeviceRefresh).expects(never());
    MOCKER(aclrtCreateNotify).expects(never());
  }

  ge::OpDescPtr op_;
  bool saved_capability_ = false;
  bool saved_v2_kernel_ = false;
  std::string saved_type_;
  rtStream_t op_stream_ = reinterpret_cast<rtStream_t>(&capture.op_stream);
  s32 device_ = 0;
};

/**
 * 用例描述：101 CCU 多 group 资源按属性顺序发布，并保留空 group 的 context 占位。
 * 预置条件：设备支持 MC2；使用有序 group 列表和外部通信分配器桩；真实解析 tiling 和 attached stream。
 * 测试步骤：通过 HcomCreateComResource 创建资源并检查 context 列表。
 * 预期结果：两个非空 group 各分配一次，空 group 对应 nullptr；不申请 AICPU stream/notify。
 */
TEST_F(HcomMc2ResourceST, CcuPublishesContextsInGroupOrder) {
  ASSERT_NO_FATAL_FAILURE(SetTiling(101U, {6U}));
  ExpectNoAicpuResources();
  std::vector<void *> contexts;
  ASSERT_EQ(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
  EXPECT_EQ(contexts, (std::vector<void *>{&capture.contexts[0], nullptr, &capture.contexts[1]}));
  EXPECT_EQ(capture.groups, (std::vector<std::string>{kFirstGroup, kSecondGroup}));
  EXPECT_EQ(capture.allocations, 2U);
  EXPECT_EQ(capture.legacy_allocations, 0U);
  EXPECT_TRUE(capture.published_notifies.empty());
}

/**
 * 用例描述：100 AICPU 多 group 创建各自 notify，并在释放时清理对应 ordered stream。
 * 预置条件：使用外部通信及 ACL 资源桩，记录拓扑发布参数。
 * 测试步骤：通过 HcomCreateComResource 创建两个 group 的资源，检查发布结果并释放。
 * 预期结果：context 与 group 顺序一致；每组各一个 notify；重复释放不重复销毁。
 */
TEST_F(HcomMc2ResourceST, AicpuPublishesAndReleasesPerGroupResources) {
  ASSERT_NO_FATAL_FAILURE(SetTiling(100U, {2U}));
  MOCKER(hrtStreamGetMode).stubs().will(returnValue(HCCL_SUCCESS));
  MOCKER(HcomMc2AiCpuStreamAllocAndGet)
      .stubs()
      .with(mockcpp::any(), mockcpp::any(), outBoundP(&op_stream_, sizeof(op_stream_)))
      .will(returnValue(HCCL_SUCCESS));
  MOCKER(hrtGetDeviceRefresh).stubs().with(outBoundP(&device_, sizeof(device_))).will(returnValue(HCCL_SUCCESS));
  MOCKER(aclrtCreateNotify).stubs().will(invoke(CreateNotify));
  std::vector<void *> contexts;
  ASSERT_EQ(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
  EXPECT_EQ(contexts, (std::vector<void *>{&capture.contexts[0], nullptr, &capture.contexts[1]}));
  EXPECT_EQ(capture.notify_count, 2U);
  EXPECT_EQ(capture.legacy_allocations, 0U);
  EXPECT_EQ(capture.ordered_streams[kFirstGroup], &capture.op_stream);
  EXPECT_EQ(capture.ordered_streams[kSecondGroup], &capture.op_stream);
  EXPECT_EQ(capture.published_notifies[kFirstGroup], &capture.notifies[0]);
  EXPECT_EQ(capture.published_notifies[kSecondGroup], &capture.notifies[1]);
  ReleaseA5AicpuGraphSyncResource(kFirstGroup);
  ReleaseA5AicpuGraphSyncResource(kFirstGroup);
  EXPECT_EQ(capture.destroyed_notifies, (std::vector<void *>{&capture.notifies[0]}));
  EXPECT_EQ(capture.ordered_streams.count(kSecondGroup), 1U);
}

/**
 * 用例描述：CCU 分配失败或返回空 context 时停止发布后续 group。
 * 预置条件：设备支持 MC2，分别模拟分配器失败和成功但空 context。
 * 测试步骤：从资源入口发起两次失败场景的创建。
 * 预期结果：返回失败，不发布 context，不创建 AICPU 资源，不访问第二个 group。
 */
TEST_F(HcomMc2ResourceST, AllocationFailureDoesNotPublishContexts) {
  ASSERT_NO_FATAL_FAILURE(SetTiling(101U, {6U}));
  ExpectNoAicpuResources();
  for (bool null_context : {false, true}) {
    capture.allocations = 0U;
    capture.groups.clear();
    capture.null_context = null_context;
    capture.allocation_result = null_context ? HCCL_SUCCESS : HCCL_E_INTERNAL;
    std::vector<void *> contexts;
    EXPECT_NE(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
    EXPECT_TRUE(contexts.empty());
    EXPECT_EQ(capture.groups, (std::vector<std::string>{kFirstGroup}));
    EXPECT_EQ(capture.allocations, 1U);
    EXPECT_EQ(capture.legacy_allocations, 0U);
  }
}

/**
 * 用例描述：缺少 attached stream 的 MC2 算子在通信分配前失败。
 * 预置条件：配置有效的 101 tiling，但 attached stream 列表为空。
 * 测试步骤：调用资源创建入口。
 * 预期结果：返回失败，context 为空，通信查询和分配均未发生。
 */
TEST_F(HcomMc2ResourceST, MissingAttachedStreamStopsBeforeAllocation) {
  ASSERT_NO_FATAL_FAILURE(SetTiling(101U, {6U}));
  op_->SetExtAttr("_rt_resource_list", std::make_shared<std::vector<void *>>());
  ExpectNoAicpuResources();
  std::vector<void *> contexts;
  EXPECT_NE(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(contexts.empty());
  EXPECT_TRUE(capture.groups.empty());
  EXPECT_EQ(capture.allocations, 0U);
}

/**
 * 用例描述：不支持的 COMM_ENGINE 或设备能力保持 hcomm 资源流程。
 * 预置条件：配置有效的 tiling 和通信桩，分别关闭设备 MC2 能力或配置非 MC2 engine。
 * 测试步骤：从资源入口创建两个 group 的资源。
 * 预期结果：context 顺序正确，调用旧分配器，不创建新流程的 ordered stream/notify。
 */
TEST_F(HcomMc2ResourceST, UnsupportedDeviceOrEngineUsesLegacyAllocator) {
  ExpectNoAicpuResources();
  MOCKER(HcomGetAicpuOpStreamNotify).stubs().will(returnValue(HCCL_SUCCESS));
  for (bool unsupported_device : {false, true}) {
    DeviceCapability::Instance().supportsMc2Resource_ = !unsupported_device;
    ASSERT_NO_FATAL_FAILURE(SetTiling(101U, {static_cast<uint8_t>(unsupported_device ? 6U : 1U)}));
    capture.allocations = 0U;
    capture.legacy_allocations = 0U;
    capture.groups.clear();
    std::vector<void *> contexts;
    ASSERT_EQ(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
    EXPECT_EQ(contexts, (std::vector<void *>{&capture.contexts[0], nullptr, &capture.contexts[1]}));
    EXPECT_EQ(capture.groups, (std::vector<std::string>{kFirstGroup, kSecondGroup}));
    EXPECT_EQ(capture.legacy_allocations, 2U);
    EXPECT_TRUE(capture.ordered_streams.empty());
    EXPECT_EQ(capture.notify_count, 0U);
  }
}

/**
 * 用例描述：100 tiling 含 CCU engine 时保持 hcomm 资源流程，覆盖单个和混合 engine。
 * 预置条件：设备支持 MC2，配置有效的 tiling 和旧通信分配器桩。
 * 测试步骤：分别使用 engine 列表 {6}、{2, 6}、{6, 2} 创建多 group 资源。
 * 预期结果：仅调用旧分配器，context 与 group 顺序一致，不创建新流程的 AICPU stream/notify。
 */
TEST_F(HcomMc2ResourceST, Version100WithCcuEngineUsesLegacyAllocator) {
  ExpectNoAicpuResources();
  MOCKER(HcomGetAicpuOpStreamNotify).stubs().will(returnValue(HCCL_SUCCESS));
  const std::vector<std::vector<uint8_t>> engine_lists{{6U}, {2U, 6U}, {6U, 2U}};
  for (const auto &engines : engine_lists) {
    ASSERT_NO_FATAL_FAILURE(SetTiling(100U, engines));
    capture.allocations = 0U;
    capture.legacy_allocations = 0U;
    capture.groups.clear();
    std::vector<void *> contexts;
    ASSERT_EQ(HcomCreateComResource(op_, contexts), ge::GRAPH_SUCCESS);
    EXPECT_EQ(contexts, (std::vector<void *>{&capture.contexts[0], nullptr, &capture.contexts[1]}));
    EXPECT_EQ(capture.groups, (std::vector<std::string>{kFirstGroup, kSecondGroup}));
    EXPECT_EQ(capture.allocations, 2U);
    EXPECT_EQ(capture.legacy_allocations, 2U);
    EXPECT_TRUE(capture.ordered_streams.empty());
    EXPECT_EQ(capture.notify_count, 0U);
  }
}
}  // namespace
#endif
