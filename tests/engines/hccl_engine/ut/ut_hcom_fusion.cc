/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gtest/gtest.h"
#include <mockcpp/mockcpp.hpp>
#include <stdio.h>

#define private public
#define protected public
#include "op_fusion_base_pub.h"
#include "hcom_allgather_fusion.h"
#include "hcom_alltoallvc_fusion.h"
#include "hcom_reducescatter_fusion.h"
#include "hcom_all_reduce_fusion.h"
#include "hcom_broadcast_fusion.h"
#include "hcom_op_utils.h"
#undef private
#undef protected
#include "hccl_stub.h"
#include "llt_hccl_stub_ge.h"

#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/node.h"
#include "graph/op_desc.h"

using namespace std;
using namespace hccl;

class NodeTest : public ge::Node {
 public:
  NodeTest() {
    ;
  };
  ~NodeTest() {
    ;
  };
};

// GraphUtils::RemoveEdge 存在多个重载, 通过显式函数指针类型消歧, 便于 mockcpp hook 对应重载
using RemoveEdgeDataToDataFunc = ge::graphStatus (*)(const ge::OutDataAnchorPtr &, const ge::InDataAnchorPtr &);
using RemoveEdgeDataToCtrlFunc = ge::graphStatus (*)(const ge::OutDataAnchorPtr &, const ge::InControlAnchorPtr &);
using RemoveEdgeCtrlToCtrlFunc = ge::graphStatus (*)(const ge::OutControlAnchorPtr &, const ge::InControlAnchorPtr &);
static RemoveEdgeDataToDataFunc g_removeEdgeDataToData = &ge::GraphUtils::RemoveEdge;
static RemoveEdgeDataToCtrlFunc g_removeEdgeDataToCtrl = &ge::GraphUtils::RemoveEdge;
static RemoveEdgeCtrlToCtrlFunc g_removeEdgeCtrlToCtrl = &ge::GraphUtils::RemoveEdge;

// GetAllInputsTensorMemSize 成功桩, 避免访问未初始化的桩数据
static HcclResult stub_GetAllInputsMemSizeOk(const ge::OpDescPtr &opDescPtr, uint64_t &tensorSize) {
  tensorSize = 2048;
  return HCCL_SUCCESS;
}

class HcomFusionTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    std::cout << "\033[36m--HcomFusionTest SetUP--\033[0m" << std::endl;
  }
  static void TearDownTestCase() {
    std::cout << "\033[36m--HcomFusionTest TearDown--\033[0m" << std::endl;
  }
  virtual void SetUp() {
    std::cout << "A Test SetUP" << std::endl;
  }
  virtual void TearDown() {
    std::cout << "A Test TearDown" << std::endl;
  }
};

/* ---------------- OpFusionBase ---------------- */
TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutDataToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusionBase.GetPeerOutDataToInControl(anchorSet, peerOutDataToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutDataToInControl_When_InControlAnchorNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  // 节点未 Init, in control anchor 为空
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();

  ret = fusionBase.GetPeerOutDataToInControl(anchorSet, peerOutDataToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutDataToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  // 建 data -> inControl 边, 保证 inControlAnchor 的 peerOutDataAnchors 非空
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusionBase.GetPeerOutDataToInControl(anchorSet, peerOutDataToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutControlToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusionBase.GetPeerOutControlToInControl(anchorSet, peerOutControlToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutControlToInControl_When_InControlAnchorNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  // 节点未 Init, in control anchor 为空
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();

  ret = fusionBase.GetPeerOutControlToInControl(anchorSet, peerOutControlToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerOutControlToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  // 建 control -> inControl 边, 保证 inControlAnchor 的 peerOutControlAnchors 非空
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusionBase.GetPeerOutControlToInControl(anchorSet, peerOutControlToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerInControlFromOutControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusionBase.GetPeerInControlFromOutControl(anchorSet, peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerInControlFromOutControl_When_OutControlAnchorNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  // 节点未 Init, out control anchor 为空
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();

  ret = fusionBase.GetPeerInControlFromOutControl(anchorSet, peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_OpFusionBase_GetPeerInControlFromOutControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  // 建 control -> inControl 边, 保证 outControlAnchor 的 peerInControlAnchors 非空
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusionBase.GetPeerInControlFromOutControl(anchorSet, peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerAnchorFromOutData_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<std::pair<u32, ge::InDataAnchorPtr>> peerInDataFromOutDataVec;
  std::vector<std::pair<u32, ge::InControlAnchorPtr>> peerInControlFromOutDataVec;
  ge::NodePtr srcNodePtr = nullptr;
  ge::OpDescPtr dstOpDescPtr = std::make_shared<ge::OpDesc>();
  int index = 0;
  std::vector<std::pair<int, int>> duplication;

  ret = fusionBase.GetPeerAnchorFromOutData(anchorSet, peerInDataFromOutDataVec, peerInControlFromOutDataVec,
                                            srcNodePtr, dstOpDescPtr, index, duplication);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest, ut_OpFusionBase_GetPeerAnchorFromOutData_When_DstOpDescPtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<std::pair<u32, ge::InDataAnchorPtr>> peerInDataFromOutDataVec;
  std::vector<std::pair<u32, ge::InControlAnchorPtr>> peerInControlFromOutDataVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::OpDescPtr dstOpDescPtr = nullptr;
  int index = 0;
  std::vector<std::pair<int, int>> duplication;

  ret = fusionBase.GetPeerAnchorFromOutData(anchorSet, peerInDataFromOutDataVec, peerInControlFromOutDataVec,
                                            srcNodePtr, dstOpDescPtr, index, duplication);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_OpFusionBase_GetPeerAnchorFromOutData_When_RemovePeerInDataEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<std::pair<u32, ge::InDataAnchorPtr>> peerInDataFromOutDataVec;
  std::vector<std::pair<u32, ge::InControlAnchorPtr>> peerInControlFromOutDataVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  ge::OpDescPtr dstOpDescPtr = std::make_shared<ge::OpDesc>();
  int index = 0;
  std::vector<std::pair<int, int>> duplication;
  // 建 data -> inData 边, 保证 outDataAnchor 的 peerInDataAnchors 非空
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToData).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusionBase.GetPeerAnchorFromOutData(anchorSet, peerInDataFromOutDataVec, peerInControlFromOutDataVec,
                                            srcNodePtr, dstOpDescPtr, index, duplication);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_OpFusionBase_GetPeerAnchorFromOutData_When_RemovePeerInControlEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  OpFusionBase fusionBase;
  std::unordered_set<uintptr_t> anchorSet;
  std::vector<std::pair<u32, ge::InDataAnchorPtr>> peerInDataFromOutDataVec;
  std::vector<std::pair<u32, ge::InControlAnchorPtr>> peerInControlFromOutDataVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  ge::OpDescPtr dstOpDescPtr = std::make_shared<ge::OpDesc>();
  int index = 0;
  std::vector<std::pair<int, int>> duplication;
  // 建 data -> inControl 边, 保证 outDataAnchor 的 peerInControlAnchors 非空
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusionBase.GetPeerAnchorFromOutData(anchorSet, peerInDataFromOutDataVec, peerInControlFromOutDataVec,
                                            srcNodePtr, dstOpDescPtr, index, duplication);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

/* ---------------- HcomAllGatherFusion ---------------- */
TEST_F(HcomFusionTest, ut_HcomAllGatherFusion_GetPeerOutDataToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAllGatherFusion_GetPeerOutDataToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_HcomAllGatherFusion_GetPeerOutControlToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAllGatherFusion_GetPeerOutControlToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_HcomAllGatherFusion_GetPeerInControlFromOutControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAllGatherFusion_GetPeerInControlFromOutControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAllGatherFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

/* ---------------- HcomAlltoAllVCFusion ---------------- */
TEST_F(HcomFusionTest, ut_HcomAlltoAllVCFusion_GetPeerOutDataToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAlltoAllVCFusion_GetPeerOutDataToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_HcomAlltoAllVCFusion_GetPeerOutControlToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAlltoAllVCFusion_GetPeerOutControlToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_HcomAlltoAllVCFusion_GetPeerInControlFromOutControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomAlltoAllVCFusion_GetPeerInControlFromOutControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_HcomAlltoAllVCFusion_CreateSplitVNode_When_NumSplitExceedMax_Expect_ReturnNotSupportError) {
  HcclResult ret;
  HcomAlltoAllVCFusion fusion;
  ge::ComputeGraph graph("test_graph");
  SplitVNodeInfo splitvNodeInfo;
  splitvNodeInfo.splitvNodePtr = nullptr;
  splitvNodeInfo.nodeName = "splitv_node";
  // numSplit 超过 SPLITV_NUMSPLIT_MAX(61), 触发不支持分支
  splitvNodeInfo.numSplit = SPLITV_NUMSPLIT_MAX + 1;

  ret = fusion.CreateSplitVNode(splitvNodeInfo, graph);
  EXPECT_EQ(ret, HCCL_E_NOT_SUPPORT);
}

/* ---------------- HcomReduceScatterFusion ---------------- */
TEST_F(HcomFusionTest, ut_HcomReduceScatterFusion_GetPeerOutDataToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_GetPeerOutDataToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::OutDataAnchorPtr> peerOutDataToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutDataAnchor(0)->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeDataToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutDataToInControl(peerOutDataToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_GetPeerOutControlToInControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_GetPeerOutControlToInControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::OutControlAnchorPtr> peerOutControlToInControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerOutControlToInControl(peerOutControlToInControlVec, dstNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_GetPeerInControlFromOutControl_When_SrcNodePtrNull_Expect_ReturnPtrError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = nullptr;

  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_PTR);
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_GetPeerInControlFromOutControl_When_RemoveEdgeFailed_Expect_ReturnInternalError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  std::vector<ge::InControlAnchorPtr> peerInControlFromOutControlVec;
  ge::NodePtr srcNodePtr = std::make_shared<NodeTest>();
  srcNodePtr->Init();
  ge::NodePtr dstNodePtr = std::make_shared<NodeTest>();
  dstNodePtr->Init();
  EXPECT_EQ(srcNodePtr->GetOutControlAnchor()->LinkTo(dstNodePtr->GetInControlAnchor()), ge::GRAPH_SUCCESS);

  MOCKER(g_removeEdgeCtrlToCtrl).stubs().will(returnValue(ge::GRAPH_FAILED));
  ret = fusion.GetPeerInControlFromOutControl(peerInControlFromOutControlVec, srcNodePtr);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest,
       ut_HcomReduceScatterFusion_CreateSplitVNode_When_NumSplitExceedMax_Expect_ReturnNotSupportError) {
  HcclResult ret;
  HcomReduceScatterFusion fusion;
  ge::ComputeGraph graph("test_graph");
  SplitVNodeInfo splitvNodeInfo;
  splitvNodeInfo.splitvNodePtr = nullptr;
  splitvNodeInfo.nodeName = "splitv_node";
  // numSplit 超过 SPLITV_NUMSPLIT_MAX(61), 触发不支持分支
  splitvNodeInfo.numSplit = SPLITV_NUMSPLIT_MAX + 1;

  ret = fusion.CreateSplitVNode(splitvNodeInfo, graph);
  EXPECT_EQ(ret, HCCL_E_NOT_SUPPORT);
}

/* ---------------- HcomAllReduceFusion ---------------- */
TEST_F(HcomFusionTest,
       ut_HcomAllReduceFusion_GetGradSplitStrategy_When_GetAllInputsTensorMemSizeFailed_Expect_ReturnError) {
  HcclResult ret;
  HcomAllReduceFusion fusion;
  FusionSection fusionSection;
  ge::NodePtr nodePtr = std::make_shared<NodeTest>();
  fusionSection.push_back(nodePtr);
  u32 segmentNum = 0;
  std::vector<u32> segmentIndex;

  MOCKER(&HcomOpUtils::GetAllInputsTensorMemSize).stubs().will(returnValue(HCCL_E_PARA));
  ret = fusion.GetGradSplitStrategy("test_model", "hccl_world_group", fusionSection, segmentNum, segmentIndex);
  EXPECT_EQ(ret, HCCL_E_PARA);
  GlobalMockObject::verify();
}

TEST_F(HcomFusionTest, ut_HcomAllReduceFusion_GetGradSplitStrategy_When_GetSplitStrategyFailed_Expect_ReturnError) {
  HcclResult ret;
  HcomAllReduceFusion fusion;
  FusionSection fusionSection;
  ge::NodePtr nodePtr = std::make_shared<NodeTest>();
  fusionSection.push_back(nodePtr);
  u32 segmentNum = 0;
  std::vector<u32> segmentIndex;

  // 先保证 GetAllInputsTensorMemSize 成功, 再触发 HcomGetSplitStrategy 失败分支
  MOCKER(&HcomOpUtils::GetAllInputsTensorMemSize).stubs().will(invoke(stub_GetAllInputsMemSizeOk));
  MOCKER(HcomGetSplitStrategy)
      .stubs()
      .with(mockcpp::any(), mockcpp::any(), mockcpp::any(), mockcpp::any(), mockcpp::any(), mockcpp::any(),
            mockcpp::any())
      .will(returnValue(HCCL_E_INTERNAL));
  ret = fusion.GetGradSplitStrategy("test_model", "hccl_world_group", fusionSection, segmentNum, segmentIndex);
  EXPECT_EQ(ret, HCCL_E_INTERNAL);
  GlobalMockObject::verify();
}

/* ---------------- HcomBroadcastFusion ---------------- */
/*
 * fusion 属性为非法值(5)时 GetFusionOption 走 default 分支返回 HCCL_E_PARA;
 * 不经 Run 直调: stub 的 ComputeGraph::AddNode 丢弃传入的 OpDesc, Run 路径无法构造出带属性的节点
 */
TEST_F(HcomFusionTest, ut_HcomBroadcastFusion_GetFusionOption_When_FusionAttrInvalid_Expect_ReturnParaError) {
  HcclResult ret;
  HcomBroadcastFusion fusion;
  ge::NodePtr nodePtr = std::make_shared<NodeTest>();
  auto opDescPtr = nodePtr->GetOpDesc();
  EXPECT_NE(opDescPtr, nullptr);
  // fusion 属性为非法值(5); fusion_id 需同时设置合法值, 否则会被重置为 NO_FUSION 无法走到 default 分支
  EXPECT_TRUE(ge::AttrUtils::SetInt(opDescPtr, HCOM_ATTR_NAME_FUSION, 5));
  EXPECT_TRUE(ge::AttrUtils::SetInt(opDescPtr, HCOM_ATTR_NAME_FUSION_ID, 0));

  FusionOption fusionOption;
  ret = fusion.GetFusionOption(nodePtr, fusionOption);
  EXPECT_EQ(ret, HCCL_E_PARA);

  // 恢复全局属性桩值, 避免影响后续依赖属性缺失语义的用例
  EXPECT_TRUE(ge::AttrUtils::SetInt(opDescPtr, HCOM_ATTR_NAME_FUSION, HCOM_ATTR_FUSION_NO_FUSION));
}
