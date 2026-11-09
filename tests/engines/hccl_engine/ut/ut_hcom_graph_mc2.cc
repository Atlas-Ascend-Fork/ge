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
#include <securec.h>
#include <limits>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#define private public
#include "device_capability.h"
#undef private
#include "hcom_graph_mc2.h"
#include "hcom_acl_adapter.h"
#include "hcom/hcom_topo_info.h"
#include "acl/acl_rt.h"

namespace hccl {
bool NeedUseMc2Resource(u32 tilingVersion, const std::string &tilingData);
using Mc2AllocFunc = HcclResult (*)(HcclComm, void *, void *, void **);
Mc2AllocFunc GetA5Mc2AllocFunc();
#ifndef OPEN_BUILD_PROJECT
void *HcomGetMC2Context(rtStream_t stream, const void *tilingData, const char *groupName, u32 tilingVersion);
#endif
}  // namespace hccl

namespace {
using namespace hccl;
constexpr char kGroup[] = "ut_mc2_context_group";
uint64_t comm_storage = 0U;
uint64_t stream_storage = 0U;
uint64_t context_storage = 0U;
uint64_t op_stream_storage = 0U;
uint64_t notify_storage = 0U;
size_t alloc_calls = 0U;
HcclResult alloc_result = HCCL_SUCCESS;

std::string MakeMc2Tiling(uint32_t version, const std::vector<uint8_t> &engines) {
  // Init tiling occupies 64 bytes; commEngine is byte 13 in each 16-byte CC prefix.
  std::array<uint32_t, 16> init{};
  init[0] = version;
  init[1] = static_cast<uint32_t>(engines.size());
  std::string tiling(sizeof(init) + engines.size() * 16U, '\0');
  for (size_t i = 0U; i < engines.size(); ++i) {
    init[i + 2U] = static_cast<uint32_t>(sizeof(init) + i * 16U);
    tiling[init[i + 2U] + 13U] = static_cast<char>(engines[i]);
  }
  EXPECT_EQ(memcpy_s(&tiling[0], tiling.size(), init.data(), sizeof(init)), EOK);
  return tiling;
}

HcclResult AllocateMc2Context(HcclComm comm, void *stream, void *tiling, void **context) {
  ++alloc_calls;
  EXPECT_EQ(comm, &comm_storage);
  EXPECT_EQ(stream, &stream_storage);
  EXPECT_NE(tiling, nullptr);
  *context = &context_storage;
  return alloc_result;
}

ge::graphStatus PublishOrderedStream(ge::HcomTopoInfo *, int32_t device_id, const char *group, void *stream) {
  EXPECT_EQ(device_id, 0);
  EXPECT_STREQ(group, kGroup);
  EXPECT_EQ(stream, &op_stream_storage);
  return ge::GRAPH_SUCCESS;
}

bool GetAicpuTopo(ge::HcomTopoInfo *, const char *group, ge::HcomTopoInfo::TopoInfo &topo) {
  EXPECT_STREQ(group, kGroup);
  topo = {};
  topo.rank_size = 2;
  return true;
}

ge::graphStatus PublishAicpuTopo(ge::HcomTopoInfo *, const char *group, const ge::HcomTopoInfo::TopoInfo &topo) {
  EXPECT_STREQ(group, kGroup);
  EXPECT_EQ(topo.rank_size, 2);
  EXPECT_EQ(topo.notify_handle, &notify_storage);
  return ge::GRAPH_SUCCESS;
}

class HcomMc2ContextTest : public testing::Test {
 protected:
  void SetUp() override {
    saved_capability_ = DeviceCapability::Instance().supportsMc2Resource_;
    DeviceCapability::Instance().supportsMc2Resource_ = true;
    alloc_calls = 0U;
    alloc_result = HCCL_SUCCESS;
  }

  void TearDown() override {
    EXPECT_NO_THROW(ReleaseA5AicpuGraphSyncResource(kGroup));
    ge::HcomTopoInfo::Instance().UnsetGroupTopoInfo(kGroup);
    DeviceCapability::Instance().supportsMc2Resource_ = saved_capability_;
    EXPECT_NO_THROW(GlobalMockObject::verify());
    GlobalMockObject::reset();
  }

  void MockAllocator(Mc2AllocFunc allocator = &AllocateMc2Context) {
    MOCKER(HcomGetCommHandleByGroup)
        .stubs()
        .with(mockcpp::any(), outBoundP(&mock_comm_, sizeof(mock_comm_)))
        .will(returnValue(HCCL_SUCCESS));
    MOCKER(GetA5Mc2AllocFunc).stubs().will(returnValue(allocator));
  }

  void MockAicpuResources() {
    MOCKER(hrtStreamGetMode)
        .expects(once())
        .with(mockcpp::any(), outBoundP(&mock_stream_mode_, sizeof(mock_stream_mode_)))
        .will(returnValue(HCCL_SUCCESS));
    MOCKER(HcomMc2AiCpuStreamAllocAndGet)
        .expects(once())
        .with(mockcpp::any(), mockcpp::any(), outBoundP(&mock_op_stream_, sizeof(mock_op_stream_)))
        .will(returnValue(HCCL_SUCCESS));
    MOCKER(hrtGetDeviceRefresh)
        .stubs()
        .with(outBoundP(&mock_device_, sizeof(mock_device_)))
        .will(returnValue(HCCL_SUCCESS));
    MOCKER(aclrtCreateNotify)
        .expects(once())
        .with(outBoundP(&mock_notify_, sizeof(mock_notify_)), mockcpp::any())
        .will(returnValue(ACL_SUCCESS));
    MOCKER(aclrtDestroyNotify).expects(once()).with(eq(mock_notify_)).will(returnValue(ACL_SUCCESS));
    // The shared HcomTopoInfo stubs do not retain published streams or topology.
    MOCKER_CPP(&ge::HcomTopoInfo::SetGroupOrderedStream).expects(once()).will(invoke(PublishOrderedStream));
    MOCKER_CPP(&ge::HcomTopoInfo::UnsetGroupOrderedStream).expects(once()).with(eq(mock_device_), mockcpp::any());
  }

  bool saved_capability_ = false;
  HcclComm mock_comm_ = &comm_storage;
  uint64_t mock_stream_mode_ = 0U;
  rtStream_t mock_op_stream_ = reinterpret_cast<rtStream_t>(&op_stream_storage);
  s32 mock_device_ = 0;
  void *mock_notify_ = &notify_storage;
};

TEST_F(HcomMc2ContextTest, Versions100And101AcceptSupportedEngines) {
  EXPECT_TRUE(NeedUseMc2Resource(100U, MakeMc2Tiling(100U, {2U})));
  EXPECT_TRUE(NeedUseMc2Resource(100U, MakeMc2Tiling(100U, {2U, 2U})));
  EXPECT_TRUE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, {6U})));
  EXPECT_TRUE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, {2U, 6U})));
  EXPECT_TRUE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, std::vector<uint8_t>(8U, 6U))));
  EXPECT_FALSE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, {6U, 1U})));
  EXPECT_FALSE(NeedUseMc2Resource(3U, MakeMc2Tiling(3U, {6U})));
}

TEST_F(HcomMc2ContextTest, Version100WithCcuEngineKeepsLegacyResources) {
  EXPECT_FALSE(NeedUseMc2Resource(100U, MakeMc2Tiling(100U, {6U})));
  EXPECT_FALSE(NeedUseMc2Resource(100U, MakeMc2Tiling(100U, {2U, 6U})));
  EXPECT_FALSE(NeedUseMc2Resource(100U, MakeMc2Tiling(100U, {6U, 2U})));
}

TEST_F(HcomMc2ContextTest, UnsupportedDeviceKeepsLegacyResources) {
  DeviceCapability::Instance().supportsMc2Resource_ = false;
  EXPECT_FALSE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, {6U})));
  EXPECT_FALSE(DeviceCapability::Instance().SupportsMc2Resource());
}

TEST_F(HcomMc2ContextTest, SocMappingsSelectMc2ResourceCapability) {
  DeviceCapability capability;
  capability.InitFromSocVersion("Ascend950");
  EXPECT_TRUE(capability.SupportsMc2Resource());
  capability.InitFromSocVersion("Ascend910_96_custom");
  EXPECT_TRUE(capability.SupportsMc2Resource());
  capability.InitFromSocVersion("Ascend910B1");
  EXPECT_FALSE(capability.SupportsMc2Resource());
}

TEST_F(HcomMc2ContextTest, InvalidTilingDoesNotSelectMc2Resources) {
  EXPECT_FALSE(NeedUseMc2Resource(101U, std::string(63U, '\0')));
  EXPECT_FALSE(NeedUseMc2Resource(101U, MakeMc2Tiling(101U, {})));
  EXPECT_FALSE(NeedUseMc2Resource(101U, MakeMc2Tiling(99U, {6U})));
  auto tiling = MakeMc2Tiling(101U, {6U});
  uint32_t invalid_count = 9U;
  ASSERT_EQ(
      memcpy_s(&tiling[sizeof(uint32_t)], tiling.size() - sizeof(uint32_t), &invalid_count, sizeof(invalid_count)),
      EOK);
  EXPECT_FALSE(NeedUseMc2Resource(101U, tiling));
  tiling = MakeMc2Tiling(101U, {6U});
  const uint32_t invalid_offset = std::numeric_limits<uint32_t>::max();
  ASSERT_EQ(memcpy_s(&tiling[2U * sizeof(uint32_t)], tiling.size() - 2U * sizeof(uint32_t), &invalid_offset,
                     sizeof(invalid_offset)),
            EOK);
  EXPECT_FALSE(NeedUseMc2Resource(101U, tiling));
  tiling = MakeMc2Tiling(101U, {6U});
  tiling.pop_back();
  EXPECT_FALSE(NeedUseMc2Resource(101U, tiling));
}

#ifndef OPEN_BUILD_PROJECT
TEST_F(HcomMc2ContextTest, CcuContextDoesNotAllocateAicpuStreamOrNotify) {
  MockAllocator();
  MOCKER(hrtStreamGetMode).expects(never());
  MOCKER(HcomMc2AiCpuStreamAllocAndGet).expects(never());
  MOCKER(hrtGetDeviceRefresh).expects(never());
  MOCKER(aclrtCreateNotify).expects(never());
  const auto tiling = MakeMc2Tiling(101U, {6U, 6U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U),
            &context_storage);
  EXPECT_EQ(alloc_calls, 1U);
  EXPECT_FALSE(ge::HcomTopoInfo::Instance().TopoInfoHasBeenSet(kGroup));
  void *ordered_stream = nullptr;
  EXPECT_NE(ge::HcomTopoInfo::Instance().GetGroupOrderedStream(0, kGroup, ordered_stream), ge::GRAPH_SUCCESS);
}

TEST_F(HcomMc2ContextTest, AicpuContextPublishesOrderedStreamAndNotify) {
  MockAllocator();
  MockAicpuResources();
  MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).expects(once()).will(invoke(GetAicpuTopo));
  MOCKER_CPP(&ge::HcomTopoInfo::SetGroupTopoInfo).expects(once()).will(invoke(PublishAicpuTopo));
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U),
            &context_storage);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, Version101AicpuContextPublishesOrderedStreamAndNotify) {
  MockAllocator();
  MockAicpuResources();
  MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).expects(once()).will(invoke(GetAicpuTopo));
  MOCKER_CPP(&ge::HcomTopoInfo::SetGroupTopoInfo).expects(once()).will(invoke(PublishAicpuTopo));
  const auto tiling = MakeMc2Tiling(101U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U),
            &context_storage);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, MixedEnginesPublishAicpuOrderedStreamAndNotify) {
  MockAllocator();
  MockAicpuResources();
  MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).expects(once()).will(invoke(GetAicpuTopo));
  MOCKER_CPP(&ge::HcomTopoInfo::SetGroupTopoInfo).expects(once()).will(invoke(PublishAicpuTopo));
  const auto tiling = MakeMc2Tiling(101U, {6U, 2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U),
            &context_storage);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, MissingAicpuTopoReleasesNewNotifyAndOrderedStream) {
  MockAllocator();
  MockAicpuResources();
  MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).expects(once()).will(returnValue(false));
  MOCKER_CPP(&ge::HcomTopoInfo::SetGroupTopoInfo).expects(never());
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U), nullptr);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, AllocationFailureDoesNotPrepareAicpuResources) {
  MockAllocator();
  alloc_result = HCCL_E_INTERNAL;
  MOCKER(hrtStreamGetMode).expects(never());
  MOCKER(aclrtCreateNotify).expects(never());
  const auto tiling = MakeMc2Tiling(101U, {6U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U), nullptr);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, CommLookupFailureDoesNotAllocateContext) {
  MOCKER(HcomGetCommHandleByGroup).stubs().will(returnValue(HCCL_E_NOT_FOUND));
  MOCKER(GetA5Mc2AllocFunc).expects(never());
  const auto tiling = MakeMc2Tiling(101U, {6U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U), nullptr);
  EXPECT_EQ(alloc_calls, 0U);
}

TEST_F(HcomMc2ContextTest, NullCommDoesNotLoadAllocator) {
  mock_comm_ = nullptr;
  MOCKER(HcomGetCommHandleByGroup)
      .expects(once())
      .with(mockcpp::any(), outBoundP(&mock_comm_, sizeof(mock_comm_)))
      .will(returnValue(HCCL_SUCCESS));
  MOCKER(GetA5Mc2AllocFunc).expects(never());
  const auto tiling = MakeMc2Tiling(101U, {6U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 101U), nullptr);
  EXPECT_EQ(alloc_calls, 0U);
}

TEST_F(HcomMc2ContextTest, MissingAllocatorDoesNotPrepareAicpuResources) {
  MockAllocator(nullptr);
  MOCKER(hrtStreamGetMode).expects(never());
  MOCKER(aclrtCreateNotify).expects(never());
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U), nullptr);
  EXPECT_EQ(alloc_calls, 0U);
}

TEST_F(HcomMc2ContextTest, StreamModeFailureDoesNotAllocateAicpuStream) {
  MockAllocator();
  MOCKER(hrtStreamGetMode).expects(once()).will(returnValue(HCCL_E_INTERNAL));
  MOCKER(HcomMc2AiCpuStreamAllocAndGet).expects(never());
  MOCKER(aclrtCreateNotify).expects(never());
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U), nullptr);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, AicpuStreamFailureDoesNotAllocateNotify) {
  MockAllocator();
  MOCKER(hrtStreamGetMode).expects(once()).will(returnValue(HCCL_SUCCESS));
  MOCKER(HcomMc2AiCpuStreamAllocAndGet).expects(once()).will(returnValue(HCCL_E_INTERNAL));
  MOCKER(hrtGetDeviceRefresh).expects(never());
  MOCKER(aclrtCreateNotify).expects(never());
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U), nullptr);
  EXPECT_EQ(alloc_calls, 1U);
}

TEST_F(HcomMc2ContextTest, OrderedStreamPublishFailureReleasesNewNotify) {
  MockAllocator();
  MOCKER(hrtStreamGetMode).expects(once()).will(returnValue(HCCL_SUCCESS));
  MOCKER(HcomMc2AiCpuStreamAllocAndGet)
      .expects(once())
      .with(mockcpp::any(), mockcpp::any(), outBoundP(&mock_op_stream_, sizeof(mock_op_stream_)))
      .will(returnValue(HCCL_SUCCESS));
  MOCKER(hrtGetDeviceRefresh)
      .stubs()
      .with(outBoundP(&mock_device_, sizeof(mock_device_)))
      .will(returnValue(HCCL_SUCCESS));
  MOCKER(aclrtCreateNotify)
      .expects(once())
      .with(outBoundP(&mock_notify_, sizeof(mock_notify_)), mockcpp::any())
      .will(returnValue(ACL_SUCCESS));
  MOCKER_CPP(&ge::HcomTopoInfo::SetGroupOrderedStream).expects(once()).will(returnValue(ge::GRAPH_FAILED));
  MOCKER_CPP(&ge::HcomTopoInfo::TryGetGroupTopoInfo).expects(never());
  MOCKER_CPP(&ge::HcomTopoInfo::UnsetGroupOrderedStream).expects(once()).with(eq(mock_device_), mockcpp::any());
  MOCKER(aclrtDestroyNotify).expects(once()).with(eq(mock_notify_)).will(returnValue(ACL_SUCCESS));
  const auto tiling = MakeMc2Tiling(100U, {2U});
  EXPECT_EQ(HcomGetMC2Context(reinterpret_cast<rtStream_t>(&stream_storage), tiling.data(), kGroup, 100U), nullptr);
  EXPECT_EQ(alloc_calls, 1U);
}
#endif
}  // namespace
