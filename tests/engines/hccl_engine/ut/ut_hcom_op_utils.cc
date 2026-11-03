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
#include <map>
#include <string>
#include "hcom_op_utils.h"
#include "hcom_acl_adapter.h"
#include "llt_hccl_stub_ge.h"

using namespace hccl;

namespace {
// mock桩：SalGetDataTypeSize正常返回，FP32对应4字节
HcclResult SalGetDataTypeSizeStub(HcclDataType dataType, u32 &unitSize) {
  unitSize = (dataType == HCCL_DATA_TYPE_FP32) ? 4U : 1U;
  return HCCL_SUCCESS;
}

// mock桩：SalGetDataTypeSize返回的数据类型长度为0
HcclResult SalGetDataTypeSizeZeroStub(HcclDataType, u32 &unitSize) {
  unitSize = 0U;
  return HCCL_SUCCESS;
}

// mock桩：TensorUtils::GetSize失败
ge::graphStatus TensorUtilsGetSizeFailStub(const ge::GeTensorDesc &, int64_t &size) {
  size = 0;
  return ge::GRAPH_FAILED;
}

// mock桩：TensorUtils::GetSize返回负值，转成u64后触发溢出校验
ge::graphStatus TensorUtilsGetSizeOverflowStub(const ge::GeTensorDesc &, int64_t &size) {
  size = -1;
  return ge::GRAPH_SUCCESS;
}

// mock桩：TensorUtils::CalcTensorMemSize失败
ge::graphStatus TensorUtilsCalcMemSizeFailStub(const ge::GeShape &, ge::Format, ge::DataType, int64_t &memSize) {
  memSize = 0;
  return ge::GRAPH_FAILED;
}

// 不在HCOM_DATA_TYPE_MAP中的非法ge数据类型
const ge::DataType kInvalidGeDataType = static_cast<ge::DataType>(100);

// stub的AttrUtils::GetStr按名称读取全局dummyStringMAP，本类临时篡改其中一项并在析构时还原
class ScopedStrAttr {
 public:
  // value为nullptr时临时移除该属性，用于构造GetStr失败场景
  ScopedStrAttr(const std::string &name, const char *value) : name_(name) {
    const auto iter = ge::dummyStringMAP.find(name);
    if (iter != ge::dummyStringMAP.end()) {
      savedValue_ = iter->second;
      existed_ = true;
    }
    if (value == nullptr) {
      ge::dummyStringMAP.erase(name);
    } else {
      ge::dummyStringMAP[name] = value;
    }
  }
  ~ScopedStrAttr() {
    if (existed_) {
      ge::dummyStringMAP[name_] = savedValue_;
    } else {
      ge::dummyStringMAP.erase(name_);
    }
  }
  ScopedStrAttr(const ScopedStrAttr &) = delete;
  ScopedStrAttr &operator=(const ScopedStrAttr &) = delete;

 private:
  std::string name_;
  std::string savedValue_;
  bool existed_ = false;
};

// stub的AttrUtils::HasAttr("group")读取全局dummyHasAttr_group，本类临时置位并在析构时还原
class ScopedGroupAttr {
 public:
  explicit ScopedGroupAttr(bool hasAttr) : saved_(ge::dummyHasAttr_group) {
    ge::dummyHasAttr_group = hasAttr;
  }
  ~ScopedGroupAttr() {
    ge::dummyHasAttr_group = saved_;
  }
  ScopedGroupAttr(const ScopedGroupAttr &) = delete;
  ScopedGroupAttr &operator=(const ScopedGroupAttr &) = delete;

 private:
  bool saved_;
};
}  // namespace

class HcomOpUtilsTest : public testing::Test {
 protected:
  void TearDown() override {
    GlobalMockObject::verify();
    GlobalMockObject::reset();
  }
};

// 测试场景：数据类型长度为0时，GetAccuracyCountFromOpDesc应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, Ut_GetAccuracyCountFromOpDesc_When_DataTypeSizeZero_Expect_E_PARA) {
  MOCKER(SalGetDataTypeSize).stubs().will(invoke(SalGetDataTypeSizeZeroStub));
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  u64 count = 0;
  EXPECT_EQ(HcomOpUtils::GetAccuracyCountFromOpDesc(op, HCCL_KERNEL_OP_TYPE_ALLREDUCE, HCCL_DATA_TYPE_FP32, count, 1U),
            HCCL_E_PARA);
}

// 测试场景：RECEIVE算子不支持获取count，GetAccuracyCountFromOpDesc应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, Ut_GetAccuracyCountFromOpDesc_When_ReceiveOp_Expect_Success) {
  MOCKER(SalGetDataTypeSize).stubs().will(invoke(SalGetDataTypeSizeStub));
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  u64 count = 0;
  EXPECT_EQ(HcomOpUtils::GetAccuracyCountFromOpDesc(op, HCCL_KERNEL_OP_TYPE_RECEIVE, HCCL_DATA_TYPE_FP32, count, 1U),
            HCCL_SUCCESS);
}

// 测试场景：reduction属性缺失或值不在HCOM_REDUCE_TYPE_MAP中，GetReduction应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetReduction_When_AttrMissingOrInvalid_Expect_ReturnHCCL_E_PARA) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  HcclReduceOp reduction = HCCL_REDUCE_SUM;
  {
    // 属性缺失，AttrUtils::GetStr失败
    const ScopedStrAttr missingAttr("reduction", nullptr);
    EXPECT_EQ(HcomOpUtils::GetReduction(op, reduction), HCCL_E_PARA);
  }
  {
    // 属性值非法，不在HCOM_REDUCE_TYPE_MAP中
    const ScopedStrAttr invalidAttr("reduction", "invalid_reduction");
    EXPECT_EQ(HcomOpUtils::GetReduction(op, reduction), HCCL_E_PARA);
  }
}

// 测试场景：reduction属性合法，GetReduction应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetReduction_When_AttrValid_Expect_ReturnHCCL_SUCCESS) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  const ScopedStrAttr validAttr("reduction", "sum");
  HcclReduceOp reduction = HCCL_REDUCE_SUM;
  EXPECT_EQ(HcomOpUtils::GetReduction(op, reduction), HCCL_SUCCESS);
  EXPECT_EQ(reduction, HCCL_REDUCE_SUM);
}

// 测试场景：group属性存在但GetStr失败或取值为空串，GetGroup应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetGroup_When_GetStrFailOrGroupEmpty_Expect_ReturnHCCL_E_PARA) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  std::string group;
  const ScopedGroupAttr hasGroupAttr(true);
  {
    // 属性存在但GetStr失败
    const ScopedStrAttr missingAttr("group", nullptr);
    EXPECT_EQ(HcomOpUtils::GetGroup(op, group), HCCL_E_PARA);
  }
  {
    // 属性值为空字符串
    const ScopedStrAttr emptyAttr("group", "");
    EXPECT_EQ(HcomOpUtils::GetGroup(op, group), HCCL_E_PARA);
  }
}

// 测试场景：group属性合法，GetGroup应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetGroup_When_GroupValid_Expect_ReturnHCCL_SUCCESS) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  const ScopedGroupAttr hasGroupAttr(true);
  const ScopedStrAttr validAttr("group", "hccl_world_group");
  std::string group;
  EXPECT_EQ(HcomOpUtils::GetGroup(op, group), HCCL_SUCCESS);
  EXPECT_EQ(group, "hccl_world_group");
}

// 测试场景：ge数据类型不在HCOM_DATA_TYPE_MAP中，TransformDataType应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_TransformDataType_When_GeDataTypeUnsupported_Expect_ReturnHCCL_E_PARA) {
  HcclDataType hcclDataType = HCCL_DATA_TYPE_FP32;
  EXPECT_EQ(HcomOpUtils::TransformDataType(kInvalidGeDataType, hcclDataType), HCCL_E_PARA);
}

// 测试场景：ge数据类型在HCOM_DATA_TYPE_MAP中，TransformDataType应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_TransformDataType_When_GeDataTypeSupported_Expect_ReturnHCCL_SUCCESS) {
  HcclDataType hcclDataType = HCCL_DATA_TYPE_INT8;
  EXPECT_EQ(HcomOpUtils::TransformDataType(ge::DT_FLOAT, hcclDataType), HCCL_SUCCESS);
  EXPECT_EQ(hcclDataType, HCCL_DATA_TYPE_FP32);
}

// 测试场景：输入/输出TensorDesc数据类型不在HCOM_DATA_TYPE_MAP中，GetAlltoAllDataType应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetAlltoAllDataType_When_DescDataTypeInvalid_Expect_ReturnHCCL_E_PARA) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  // stub的GetDataType恒返回DT_FLOAT，hook其返回非法类型以构造数据类型不支持场景
  MOCKER_CPP(&ge::GeTensorDesc::GetDataType).stubs().will(returnValue(kInvalidGeDataType));
  HcclDataType sendType = HCCL_DATA_TYPE_FP32;
  HcclDataType recvType = HCCL_DATA_TYPE_FP32;
  // 输入desc存在但数据类型非法，触发sendType校验失败分支
  EXPECT_EQ(HcomOpUtils::GetAlltoAllDataType(op, sendType, recvType), HCCL_E_PARA);

  // 输入desc为空时sendType走DT_INT8默认值，输出desc数据类型非法，触发recvType校验失败分支
  const ge::GeTensorDescPtr savedInputDesc = ge::dummy_inputsDesc;
  ge::dummy_inputsDesc = nullptr;
  EXPECT_EQ(HcomOpUtils::GetAlltoAllDataType(op, sendType, recvType), HCCL_E_PARA);
  EXPECT_EQ(sendType, HCCL_DATA_TYPE_INT8);
  ge::dummy_inputsDesc = savedInputDesc;
}

// 测试场景：输入/输出TensorDesc数据类型合法，GetAlltoAllDataType应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetAlltoAllDataType_When_DescDataTypeValid_Expect_ReturnHCCL_SUCCESS) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  HcclDataType sendType = HCCL_DATA_TYPE_INT8;
  HcclDataType recvType = HCCL_DATA_TYPE_INT8;
  EXPECT_EQ(HcomOpUtils::GetAlltoAllDataType(op, sendType, recvType), HCCL_SUCCESS);
  EXPECT_EQ(sendType, HCCL_DATA_TYPE_FP32);
  EXPECT_EQ(recvType, HCCL_DATA_TYPE_FP32);
}

// 测试场景：group属性存在但GetStr失败或取值为空串，GetGroupFromOpDesc应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetGroupFromOpDesc_When_GetStrFailOrGroupEmpty_Expect_ReturnHCCL_E_PARA) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  std::string group;
  const ScopedGroupAttr hasGroupAttr(true);
  {
    // 属性存在但GetStr失败
    const ScopedStrAttr missingAttr("group", nullptr);
    EXPECT_EQ(HcomOpUtils::GetGroupFromOpDesc(op, group), HCCL_E_PARA);
  }
  {
    // 属性值为空字符串
    const ScopedStrAttr emptyAttr("group", "");
    EXPECT_EQ(HcomOpUtils::GetGroupFromOpDesc(op, group), HCCL_E_PARA);
  }
}

// 测试场景：group属性合法，GetGroupFromOpDesc应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetGroupFromOpDesc_When_GroupValid_Expect_ReturnHCCL_SUCCESS) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  const ScopedGroupAttr hasGroupAttr(true);
  const ScopedStrAttr validAttr("group", "hccl_world_group");
  std::string group;
  EXPECT_EQ(HcomOpUtils::GetGroupFromOpDesc(op, group), HCCL_SUCCESS);
  EXPECT_EQ(group, "hccl_world_group");
}

// 测试场景：CalcTensorMemSize失败，GetTensorSize应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetTensorSize_When_CalcTensorMemSizeFail_Expect_ReturnHCCL_E_PARA) {
  MOCKER(ge::TensorUtils::CalcTensorMemSize).stubs().will(invoke(TensorUtilsCalcMemSizeFailStub));
  const ge::GeTensorDesc tensorDesc;
  int64_t size = 0;
  EXPECT_EQ(HcomOpUtils::GetTensorSize(tensorDesc, size), HCCL_E_PARA);
}

// 测试场景：CalcTensorMemSize成功，GetTensorSize应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_GetTensorSize_When_CalcTensorMemSizeSuccess_Expect_ReturnHCCL_SUCCESS) {
  const ge::GeTensorDesc tensorDesc;
  int64_t size = 0;
  EXPECT_EQ(HcomOpUtils::GetTensorSize(tensorDesc, size), HCCL_SUCCESS);
  EXPECT_EQ(size, 1024);
}

// 测试场景：GetSize失败，CalcCountForAlignedOp应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_CalcCountForAlignedOp_When_GetSizeFail_Expect_ReturnHCCL_E_PARA) {
  MOCKER(ge::TensorUtils::GetSize).stubs().will(invoke(TensorUtilsGetSizeFailStub));
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  u64 count = 0;
  EXPECT_EQ(HcomOpUtils::CalcCountForAlignedOp(op, HCCL_KERNEL_OP_TYPE_ALLREDUCE, 4U, count), HCCL_E_PARA);
}

// 测试场景：输入Tensor大小转成u64后溢出，CalcCountForAlignedOp应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_CalcCountForAlignedOp_When_InputSizeOverflow_Expect_ReturnHCCL_E_PARA) {
  MOCKER(ge::TensorUtils::GetSize).stubs().will(invoke(TensorUtilsGetSizeOverflowStub));
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  u64 count = 0;
  EXPECT_EQ(HcomOpUtils::CalcCountForAlignedOp(op, HCCL_KERNEL_OP_TYPE_ALLREDUCE, 4U, count), HCCL_E_PARA);
}

// 测试场景：输入合法，CalcCountForAlignedOp应按512字节对齐后计算count并返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_HcomOpUtils_CalcCountForAlignedOp_When_InputValid_Expect_ReturnHCCL_SUCCESS) {
  const ge::OpDescPtr op = std::make_shared<ge::OpDesc>();
  u64 count = 0;
  EXPECT_EQ(HcomOpUtils::CalcCountForAlignedOp(op, HCCL_KERNEL_OP_TYPE_ALLREDUCE, 4U, count), HCCL_SUCCESS);
  // stub中GetInputsSize与GetSize均为1024，对齐后totalSize为1024*1024字节，count = 1024*1024/4
  EXPECT_EQ(count, 262144U);
}

// 测试场景：非离线编译场景，GetHcclCommNameFromConfig应返回HCCL_E_NOT_SUPPORT
TEST_F(HcomOpUtilsTest,
       ut_HcomOpUtils_GetHcclCommNameFromConfig_When_NotOfflineCompile_Expect_ReturnHCCL_E_NOT_SUPPORT) {
  std::string commName = "leftover";
  EXPECT_EQ(HcomOpUtils::GetHcclCommNameFromConfig(commName), HCCL_E_NOT_SUPPORT);
  EXPECT_TRUE(commName.empty());
}

// 测试场景：memcpy类型非法，MemcpyKindTranslate应返回HCCL_E_PARA
TEST_F(HcomOpUtilsTest, ut_MemcpyKindTranslate_When_MemcpyKindInvalid_Expect_ReturnHCCL_E_PARA) {
  aclrtMemcpyKind rtKind = ACL_MEMCPY_HOST_TO_HOST;
  EXPECT_EQ(MemcpyKindTranslate(HcclRtMemcpyKind::HCCL_RT_MEMCPY_KIND_RESERVED, &rtKind), HCCL_E_PARA);
}

// 测试场景：memcpy类型合法，MemcpyKindTranslate应返回HCCL_SUCCESS
TEST_F(HcomOpUtilsTest, ut_MemcpyKindTranslate_When_MemcpyKindValid_Expect_ReturnHCCL_SUCCESS) {
  aclrtMemcpyKind rtKind = ACL_MEMCPY_HOST_TO_HOST;
  EXPECT_EQ(MemcpyKindTranslate(HcclRtMemcpyKind::HCCL_RT_MEMCPY_KIND_HOST_TO_DEVICE, &rtKind), HCCL_SUCCESS);
  EXPECT_EQ(rtKind, ACL_MEMCPY_HOST_TO_DEVICE);
  EXPECT_EQ(MemcpyKindTranslate(HcclRtMemcpyKind::HCCL_RT_MEMCPY_ADDR_DEVICE_TO_DEVICE, &rtKind), HCCL_SUCCESS);
  EXPECT_EQ(rtKind, ACL_MEMCPY_INNER_DEVICE_TO_DEVICE);
}
