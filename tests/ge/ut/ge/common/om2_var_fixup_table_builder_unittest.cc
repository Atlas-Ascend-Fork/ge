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

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/om2/codegen/om2_codegen_types.h"
#include "common/om2/codegen/var_fixup_table_builder.h"
#include "framework/common/framework_types_internal.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/node.h"
#include "graph/utils/attr_utils.h"

namespace ge {
namespace {
constexpr uint64_t kGraphId = 33U;
// 假设备地址：SetVarAddr 只把它存进 var_addr_mgr_map_，不进 FreeVarMemory 遍历的集合，
// 故 VarManagerPool::Destroy 不会去释放它，测试内可安全使用。
constexpr uint64_t kFakeDevAddr = 0xC000000000ULL;

class VarFixupTableBuilderTest : public testing::Test {
 protected:
  void SetUp() override {
    VarManagerPool::Instance().Destroy();
  }
  void TearDown() override {
    VarManagerPool::Instance().Destroy();
  }
};

std::shared_ptr<VarManager> InitVarManager(const uint64_t session_id) {
  auto var_manager = VarManager::Instance(session_id);
  EXPECT_NE(var_manager, nullptr);
  if (var_manager != nullptr) {
    EXPECT_EQ(var_manager->Init(0U, session_id, 0U, 0U), SUCCESS);
  }
  return var_manager;
}

// builder 从 TensorUtils::GetTensorSizeInBytes 取 size，该接口按 shape/format/dtype 现算，
// 不读私有 size 字段，故此处无需 SetSize。
GeTensorDesc MakeDesc(const GeShape &shape, const Format format, const DataType dtype) {
  return GeTensorDesc(shape, format, dtype);
}

NodePtr AddNode(const ComputeGraphPtr &graph, const std::string &name, const char *const op_type,
                const GeTensorDesc &desc) {
  auto op_desc = std::make_shared<OpDesc>(name, op_type);
  (void)op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset({1024});
  return graph->AddNode(op_desc);
}

// 与 builder 内 BuildVarKey 同公式。运行期容器侧 VarResource::VarKey 也是
// name + format + "_" + dtype（非 batch 场景 GetBatchVarKeyName 是恒等），
// 三者必须一致，否则 SO 拿固化键去查询必然落空。
std::string ExpectedKey(const std::string &name, const Format format, const DataType dtype) {
  return name + std::to_string(static_cast<int32_t>(format)) + "_" + std::to_string(static_cast<int32_t>(dtype));
}

VarSlotMeta MakeSlot(const uint32_t slot, const std::string &var_name) {
  VarSlotMeta meta;
  meta.slot = slot;
  meta.var_name = var_name;
  return meta;
}

// 按 var_key 查条目。工作列表用 LIFO 展开，条目顺序不是 var_metas 顺序，故只能按键查。
const VarFixupEntry *FindByKey(const VarFixupTable &table, const std::string &var_key) {
  for (const auto &entry : table.entries) {
    if (entry.var_key == var_key) {
      return &entry;
    }
  }
  return nullptr;
}

// 空 var_metas 直接返回：表只有 graph_id，条目与数据块都为空。
// 无变量模型走这条路，产物侧 kVarNum 为 0。
TEST_F(VarFixupTableBuilderTest, NoVarMetasYieldsEmptyTable) {
  auto var_manager = InitVarManager(300U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");

  VarFixupTable table;
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, {}, kGraphId, table), SUCCESS);
  EXPECT_EQ(table.graph_id, kGraphId);
  EXPECT_TRUE(table.entries.empty());
  EXPECT_TRUE(table.weight_data.empty());
}

// VARIABLE 算子的基本字段：键公式、尺寸、数据类型、逻辑地址、内存类型、槽位下标。
TEST_F(VarFixupTableBuilderTest, VariableNodeFixatesKeySizeAndAddr) {
  auto var_manager = InitVarManager(301U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_a", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_a", desc, reinterpret_cast<const uint8_t *>(kFakeDevAddr), RT_MEMORY_HBM,
                                    node->GetOpDesc()),
            SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(7U, "var_a")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);

  const auto &entry = table.entries[0];
  EXPECT_EQ(entry.var_key, ExpectedKey("var_a", FORMAT_ND, DT_FLOAT));
  EXPECT_EQ(entry.size, 16U);  // 4 个 DT_FLOAT
  EXPECT_EQ(entry.data_type, static_cast<uint32_t>(DT_FLOAT));
  EXPECT_EQ(entry.logic_addr, kFakeDevAddr);
  EXPECT_EQ(entry.memory_type, static_cast<uint64_t>(RT_MEMORY_HBM));
  EXPECT_EQ(entry.slot, 7U);
  EXPECT_EQ(entry.extern_dev_addr, 0U);
  EXPECT_TRUE(entry.trans_road.empty());
  EXPECT_TRUE(entry.trans_src_var_key.empty());
  EXPECT_TRUE(entry.copy_src_var_key.empty());
  EXPECT_EQ(entry.init_data_size, 0U);
  EXPECT_TRUE(table.weight_data.empty());
}

// CONSTPLACEHOLDER：编译期就拿到设备地址，固化进 extern_dev_addr，
// 产物内 GetVarDevAddr 见到非 0 就直接用、不与容器交互。
TEST_F(VarFixupTableBuilderTest, ConstPlaceHolderFixatesExternDevAddr) {
  auto var_manager = InitVarManager(302U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("ph_a", CONSTPLACEHOLDER);
  (void)op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset({3072});
  // GetConstPlaceHolderAddr 要求的五个属性，取值方式与 test_om2.cc 的 AddConstPlaceHolderNodeToGraph 一致
  (void)AttrUtils::SetListInt(op_desc, "storage_shape", desc.GetShape().GetDims());
  (void)AttrUtils::SetDataType(op_desc, "dtype", desc.GetDataType());
  (void)AttrUtils::SetInt(op_desc, "size", 16L);
  (void)AttrUtils::SetInt(op_desc, "placement", static_cast<int64_t>(kPlacementDevice));
  (void)AttrUtils::SetInt(op_desc, "addr", static_cast<int64_t>(kFakeDevAddr));
  const auto node = graph->AddNode(op_desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("ph_a", desc, nullptr, RT_MEMORY_HBM, op_desc), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "ph_a")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);
  EXPECT_EQ(table.entries[0].extern_dev_addr, kFakeDevAddr);
}

// CONSTANTOP：初始数据取自 ATTR_NAME_WEIGHTS，追加进数据块并回填偏移。
// 算子类型必须用权威常量 CONSTANTOP（值 "Constant"）比较——曾被写成手写字面量
// "Constant" || "ConstantOp"，而 "ConstantOp" 全仓无任何注册点，是死条件。
TEST_F(VarFixupTableBuilderTest, ConstantOpFixatesInitDataIntoWeightBlock) {
  auto var_manager = InitVarManager(303U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("const_a", CONSTANTOP);
  (void)op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset({2048});
  static const uint8_t kBytes[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
  auto weight = std::make_shared<GeTensor>();
  weight->SetData(kBytes, sizeof(kBytes));
  weight->MutableTensorDesc() = desc;
  ASSERT_TRUE(AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight));
  const auto node = graph->AddNode(op_desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("const_a", desc, nullptr, RT_MEMORY_HBM, op_desc), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "const_a")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);
  EXPECT_EQ(table.entries[0].init_data_offset, 0U);
  EXPECT_EQ(table.entries[0].init_data_size, sizeof(kBytes));
  ASSERT_EQ(table.weight_data.size(), sizeof(kBytes));
  EXPECT_EQ(memcmp(table.weight_data.data(), kBytes, sizeof(kBytes)), 0);
}

// 转换链：dims 被 ClampDims 补齐到定长 kVarDimsMax，但 src_dim_num / dst_dim_num 必须是真实维数。
// 曾把 src_dims.size() 当维数用，恒为 8，导致 transpose 报
// "src shape[8] and perm arg[4] are different" 而转换必然失败。
TEST_F(VarFixupTableBuilderTest, TransRoadFixatesRealDimNumNotPaddedLength) {
  auto var_manager = InitVarManager(304U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({1, 2, 3, 4}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_t", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_t", desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);

  VarTransRoad road;
  TransNodeInfo info;
  info.node_type = TRANSDATA;
  info.input = MakeDesc(GeShape({1, 2, 3, 4}), FORMAT_NCHW, DT_FLOAT);
  info.output = MakeDesc(GeShape({1, 2, 3, 4}), FORMAT_ND, DT_FLOAT);
  road.push_back(info);
  ASSERT_EQ(var_manager->SetTransRoad("var_t", road), SUCCESS);
  // 图标识必须让判据通过（changed == kGraphId 且 != allocated），否则转换链在构表期即被折叠掉，
  // 本例要验的维数固化就无从发生。三条判据本身的覆盖见下面的 FoldedOutOfTransRoad 系列。
  ASSERT_EQ(var_manager->SetChangedGraphId("var_t", 33U), SUCCESS);
  ASSERT_EQ(var_manager->SetAllocatedGraphId("var_t", 7U), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_t")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);

  const auto &entry = table.entries[0];
  ASSERT_EQ(entry.trans_road.size(), 1U);
  const auto &fixup = entry.trans_road[0];
  EXPECT_EQ(fixup.kind, VarTransKind::kFormat);
  EXPECT_EQ(fixup.src_format, static_cast<uint32_t>(FORMAT_NCHW));
  EXPECT_EQ(fixup.dst_format, static_cast<uint32_t>(FORMAT_ND));
  EXPECT_EQ(fixup.src_data_type, static_cast<uint32_t>(DT_FLOAT));
  EXPECT_EQ(fixup.dst_data_type, static_cast<uint32_t>(DT_FLOAT));
  // 补齐到定长，但维数是真实的 4
  EXPECT_EQ(fixup.src_dims.size(), kVarDimsMax);
  EXPECT_EQ(fixup.dst_dims.size(), kVarDimsMax);
  EXPECT_EQ(fixup.src_dim_num, 4U);
  EXPECT_EQ(fixup.dst_dim_num, 4U);
  EXPECT_EQ(fixup.src_dims[3], 4);
  EXPECT_EQ(fixup.src_dims[4], 0);  // 补齐位
}

// src 与 dst 的维数必须各自独立固化：格式转换会改变维数（NCHW 4 维 → NC1HWC0 5 维）。
// 曾共用一个 dim_num（取自 input），导致产物里 dst_shape 被截断成 4 维、丢掉 C0，
// 转换单元按 GetItemNumByShape(dst_shape) 算出的目标缓冲少 16 倍。
TEST_F(VarFixupTableBuilderTest, TransRoadFixatesSrcAndDstDimNumSeparately) {
  auto var_manager = InitVarManager(314U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({1, 2, 3, 4}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_d", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_d", desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);

  VarTransRoad road;
  TransNodeInfo info;
  info.node_type = TRANSDATA;
  // NCHW 是 4 维；NC1HWC0 是 5 维存储形状，C1 = ceil(3/16) = 1、末维 C0 = 16
  info.input = MakeDesc(GeShape({2, 3, 4, 5}), FORMAT_NCHW, DT_FLOAT);
  info.output = MakeDesc(GeShape({2, 1, 4, 5, 16}), FORMAT_NC1HWC0, DT_FLOAT);
  road.push_back(info);
  ASSERT_EQ(var_manager->SetTransRoad("var_d", road), SUCCESS);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_d", 33U), SUCCESS);  // 令判据通过，链才不被折叠掉

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_d")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);
  ASSERT_EQ(table.entries[0].trans_road.size(), 1U);

  const auto &fixup = table.entries[0].trans_road[0];
  EXPECT_EQ(fixup.src_dim_num, 4U);
  EXPECT_EQ(fixup.dst_dim_num, 5U);
  // 两个数组都补齐到定长，真实维数只由各自的 dim_num 承载
  EXPECT_EQ(fixup.src_dims.size(), kVarDimsMax);
  EXPECT_EQ(fixup.dst_dims.size(), kVarDimsMax);
  EXPECT_EQ(fixup.dst_dims[4], 16);  // C0 必须被固化，截断会让目标缓冲少算 16 倍
  EXPECT_EQ(fixup.dst_dims[5], 0);   // 补齐位
  EXPECT_EQ(fixup.src_dims[4], 0);
}

// 转换源键取自转换链首节点的 input 描述，不是变量当前描述：
// 二者 format 不同则键不同，SO 正是凭这个键向容器只查询源地址。
TEST_F(VarFixupTableBuilderTest, TransSrcKeyFollowsRoadFrontInput) {
  auto var_manager = InitVarManager(305U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_s", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_s", desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);

  VarTransRoad road;
  TransNodeInfo info;
  info.node_type = TRANSDATA;
  info.input = MakeDesc(GeShape({4}), FORMAT_NCHW, DT_FLOAT16);
  info.output = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  road.push_back(info);
  ASSERT_EQ(var_manager->SetTransRoad("var_s", road), SUCCESS);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_s", 33U), SUCCESS);  // 令判据通过，链才不被折叠掉

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_s")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);

  const auto &entry = table.entries[0];
  EXPECT_EQ(entry.var_key, ExpectedKey("var_s", FORMAT_ND, DT_FLOAT));
  EXPECT_EQ(entry.trans_src_var_key, ExpectedKey("var_s", FORMAT_NCHW, DT_FLOAT16));
  EXPECT_NE(entry.trans_src_var_key, entry.var_key);
}

// 未注册的算子类型映射为 kUnsupported，编译期即失败——不能固化进产物留给运行期。
// 失败码断言用 NE SUCCESS 而非 EQ FAILED：这些路径经 GE_ASSERT_SUCCESS 传播，而 GE_ASSERT
// 返回通用的 ::ErrorResult()，不保留内层的 FAILED；只有 BuildVarFixupTable 自己直接 return
// FAILED 的路径（如槽位重复）才拿得到 FAILED。
// 必须先让图标识判据通过：判据先于 ConvertTransRoad 求值，被排除的条目根本不进转换，
// 其非法节点类型也就不会让本模型编译失败（那正是要消除的误报）。
TEST_F(VarFixupTableBuilderTest, UnsupportedTransNodeTypeFails) {
  auto var_manager = InitVarManager(306U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_u", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_u", desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);

  VarTransRoad road;
  TransNodeInfo info;
  info.node_type = "SomeUnregisteredOp";
  info.input = desc;
  info.output = desc;
  road.push_back(info);
  ASSERT_EQ(var_manager->SetTransRoad("var_u", road), SUCCESS);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_u", 33U), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_u")};
  EXPECT_NE(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
}

// 变量间拷贝：目标条目固化 data_type 与 copy_src_data_type 两个类型，源变量被工作列表展开进
// 同一张表且不占槽位。缺 data_type 时产物内 CopyVarData 无法判定是否需要 CAST，异 dtype 拷贝
// 会退化成裸字节搬运（静默数值错误 + 越界读设备内存）。
TEST_F(VarFixupTableBuilderTest, CopySourceExpandsWithNoSlotAndBothDataTypes) {
  auto var_manager = InitVarManager(307U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");

  // 拷贝源：DT_FLOAT16
  const auto src_desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT16);
  const auto src_node = AddNode(graph, "var_src", VARIABLE, src_desc);
  ASSERT_NE(src_node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_src", src_desc, nullptr, RT_MEMORY_HBM, src_node->GetOpDesc()), SUCCESS);

  // 拷贝目标：DT_FLOAT，经 _copy_from_var_node 指向源
  const auto dst_desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  auto dst_op = std::make_shared<OpDesc>("var_dst", VARIABLE);
  (void)dst_op->AddOutputDesc(dst_desc);
  dst_op->SetOutputOffset({1024});
  ASSERT_TRUE(AttrUtils::SetStr(dst_op, "_copy_from_var_node", "var_src"));
  const auto dst_node = graph->AddNode(dst_op);
  ASSERT_NE(dst_node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_dst", dst_desc, nullptr, RT_MEMORY_HBM, dst_op), SUCCESS);

  VarFixupTable table;
  // 只有目标在 var_metas 里；源必须由工作列表自行展开进来
  const auto var_metas = {MakeSlot(3U, "var_dst")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 2U);

  const auto *dst = FindByKey(table, ExpectedKey("var_dst", FORMAT_ND, DT_FLOAT));
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(dst->slot, 3U);
  EXPECT_EQ(dst->data_type, static_cast<uint32_t>(DT_FLOAT));
  EXPECT_EQ(dst->copy_src_data_type, static_cast<uint32_t>(DT_FLOAT16));
  EXPECT_NE(dst->copy_src_data_type, dst->data_type);
  EXPECT_EQ(dst->copy_src_var_key, ExpectedKey("var_src", FORMAT_ND, DT_FLOAT16));

  // 源不被算子直接引用 ⇒ 无槽位，FillVarAddrTable 跳过它，但 CopyVarData 要靠它取地址
  const auto *src = FindByKey(table, ExpectedKey("var_src", FORMAT_ND, DT_FLOAT16));
  ASSERT_NE(src, nullptr);
  EXPECT_EQ(src->slot, kVarNoSlot);
  EXPECT_EQ(src->data_type, static_cast<uint32_t>(DT_FLOAT16));
}

// 槽位下标与生成代码的 OpArgDesc.index 同一编号空间，重复即编译失败。
TEST_F(VarFixupTableBuilderTest, DuplicatedSlotFails) {
  auto var_manager = InitVarManager(308U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  const auto node_a = AddNode(graph, "var_a", VARIABLE, desc);
  const auto node_b = AddNode(graph, "var_b", VARIABLE, desc);
  ASSERT_NE(node_a, nullptr);
  ASSERT_NE(node_b, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_a", desc, nullptr, RT_MEMORY_HBM, node_a->GetOpDesc()), SUCCESS);
  ASSERT_EQ(var_manager->SetVarAddr("var_b", desc, nullptr, RT_MEMORY_HBM, node_b->GetOpDesc()), SUCCESS);

  VarFixupTable table;
  const std::vector<VarSlotMeta> var_metas = {MakeSlot(5U, "var_a"), MakeSlot(5U, "var_b")};
  EXPECT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), FAILED);
}

// 初始数据大于变量自身尺寸说明图与权重不自洽，固化下去会让产物内 GetVarDevAddr 越界读数据块。
TEST_F(VarFixupTableBuilderTest, InitDataLargerThanVarSizeFails) {
  auto var_manager = InitVarManager(309U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  // 1 个 DT_FLOAT = 4 字节
  const auto desc = MakeDesc(GeShape({1}), FORMAT_ND, DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>("const_big", CONSTANTOP);
  (void)op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset({2048});
  static const uint8_t kTooBig[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
  auto weight = std::make_shared<GeTensor>();
  weight->SetData(kTooBig, sizeof(kTooBig));
  weight->MutableTensorDesc() = desc;
  ASSERT_TRUE(AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight));
  const auto node = graph->AddNode(op_desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("const_big", desc, nullptr, RT_MEMORY_HBM, op_desc), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "const_big")};
  EXPECT_NE(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
}

// 多个变量各带初始数据时偏移按 entries 顺序累加，数据块长度等于末条偏移加其长度。
// 偏移分配与数据块写入必须在同一步完成，否则固化值与落盘内容错位。
TEST_F(VarFixupTableBuilderTest, WeightDataOffsetsAccumulate) {
  auto var_manager = InitVarManager(310U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);  // 16 字节
  static const uint8_t kBytesA[4] = {0xA1U, 0xA2U, 0xA3U, 0xA4U};
  static const uint8_t kBytesB[8] = {0xB1U, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U, 0xB7U, 0xB8U};

  const auto add_const = [&](const std::string &name, const uint8_t *bytes, const size_t len) {
    auto op_desc = std::make_shared<OpDesc>(name, CONSTANTOP);
    (void)op_desc->AddOutputDesc(desc);
    op_desc->SetOutputOffset({2048});
    auto weight = std::make_shared<GeTensor>();
    weight->SetData(bytes, len);
    weight->MutableTensorDesc() = desc;
    ASSERT_TRUE(AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight));
    const auto node = graph->AddNode(op_desc);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(var_manager->SetVarAddr(name, desc, nullptr, RT_MEMORY_HBM, op_desc), SUCCESS);
  };
  add_const("const_a", kBytesA, sizeof(kBytesA));
  add_const("const_b", kBytesB, sizeof(kBytesB));

  VarFixupTable table;
  const std::vector<VarSlotMeta> var_metas = {MakeSlot(0U, "const_a"), MakeSlot(1U, "const_b")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 2U);
  EXPECT_EQ(table.weight_data.size(), sizeof(kBytesA) + sizeof(kBytesB));

  // 两条都必须落在数据块内且互不重叠
  size_t covered = 0U;
  for (const auto &entry : table.entries) {
    ASSERT_GT(entry.init_data_size, 0U);
    EXPECT_LT(entry.init_data_offset + entry.init_data_size, table.weight_data.size() + 1U);
    covered += entry.init_data_size;
  }
  EXPECT_EQ(covered, table.weight_data.size());

  // 数据块内容按偏移可正确取回
  for (const auto &entry : table.entries) {
    const bool is_a = (entry.init_data_size == sizeof(kBytesA));
    const auto *expected = is_a ? kBytesA : kBytesB;
    const auto expected_len = is_a ? sizeof(kBytesA) : sizeof(kBytesB);
    EXPECT_EQ(memcmp(table.weight_data.data() + entry.init_data_offset, expected, expected_len), 0);
  }
}

// 变量尺寸为 0 无法固化：产物内 GetVarDevAddr 会按 size 分配宿主缓冲并向容器申请设备内存。
TEST_F(VarFixupTableBuilderTest, ZeroSizeVarFails) {
  auto var_manager = InitVarManager(311U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  const auto desc = MakeDesc(GeShape({0}), FORMAT_ND, DT_FLOAT);
  const auto node = AddNode(graph, "var_zero", VARIABLE, desc);
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(var_manager->SetVarAddr("var_zero", desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_zero")};
  EXPECT_NE(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
}

// 未在 VarManager 注册过的变量取不到当前描述，必须失败而非固化出空条目。
TEST_F(VarFixupTableBuilderTest, UnregisteredVarFails) {
  auto var_manager = InitVarManager(312U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "never_registered")};
  EXPECT_NE(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
}
// 建一个已注册的 VARIABLE 并挂上单节点转换链。node_type 决定映射出的 VarTransKind
// （TRANSDATA → kFormat、RESHAPE → kNoOp），供下面三个折叠用例复用。
// 链首节点 input 用 NCHW、变量当前描述用 ND，故转换源键与 var_key 必然不同。
NodePtr AddVarWithRoad(const std::shared_ptr<VarManager> &var_manager, const ComputeGraphPtr &graph,
                       const std::string &name, const char *const node_type) {
  const auto desc = MakeDesc(GeShape({4}), FORMAT_ND, DT_FLOAT);
  auto node = AddNode(graph, name, VARIABLE, desc);
  EXPECT_NE(node, nullptr);
  if (node == nullptr) {
    return nullptr;
  }
  EXPECT_EQ(var_manager->SetVarAddr(name, desc, nullptr, RT_MEMORY_HBM, node->GetOpDesc()), SUCCESS);
  TransNodeInfo info;
  info.node_type = node_type;
  info.input = MakeDesc(GeShape({4}), FORMAT_NCHW, DT_FLOAT);
  info.output = desc;
  VarTransRoad road;
  road.push_back(info);
  EXPECT_EQ(var_manager->SetTransRoad(name, road), SUCCESS);
  return node;
}

// 图标识判据前半：触发转换的图不是本图 ⇒ 该条目不写入转换链。同表内本图触发的条目照常保留，
// 证明折叠是逐条目的、不是全表开关。转换源键必须与链成对留空：只清一个会让产物出现
// trans_road_num == 0 却带非空源键的自相矛盾条目，SO 会去只查询一个不该查的源。
TEST_F(VarFixupTableBuilderTest, GraphIdMismatchFoldsOutOfTransRoad) {
  auto var_manager = InitVarManager(315U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  ASSERT_NE(AddVarWithRoad(var_manager, graph, "var_other", TRANSDATA), nullptr);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_other", 88U), SUCCESS);  // != kGraphId
  ASSERT_NE(AddVarWithRoad(var_manager, graph, "var_this", TRANSDATA), nullptr);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_this", 33U), SUCCESS);
  ASSERT_EQ(var_manager->SetAllocatedGraphId("var_this", 7U), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_other"), MakeSlot(1U, "var_this")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 2U);

  const auto *folded = FindByKey(table, ExpectedKey("var_other", FORMAT_ND, DT_FLOAT));
  ASSERT_NE(folded, nullptr);
  EXPECT_TRUE(folded->trans_road.empty());
  EXPECT_TRUE(folded->trans_src_var_key.empty());
  // 只折叠转换链，条目其余字段照常固化
  EXPECT_NE(folded->size, 0U);

  const auto *kept = FindByKey(table, ExpectedKey("var_this", FORMAT_ND, DT_FLOAT));
  ASSERT_NE(kept, nullptr);
  ASSERT_EQ(kept->trans_road.size(), 1U);
  EXPECT_EQ(kept->trans_src_var_key, ExpectedKey("var_this", FORMAT_NCHW, DT_FLOAT));
}

// 图标识判据后半：触发转换的图就是首次分配该变量的图 ⇒ 内存生在目标格式，无需转换。
// 这一条无法从 var_key 推出——var_key 与转换源键在"需要转换"的情形下逐字相同，区别只在
// 内存里当前躺着的是源格式还是目标格式数据，而这由跨图分配来历决定。若照写链，SO 会把
// 已是目标格式的数据再喂进一次 源→目标 转换，静默损坏数据。
TEST_F(VarFixupTableBuilderTest, ChangedGraphIsAllocatingGraphFoldsOutOfTransRoad) {
  auto var_manager = InitVarManager(316U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  ASSERT_NE(AddVarWithRoad(var_manager, graph, "var_same", TRANSDATA), nullptr);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_same", 33U), SUCCESS);
  ASSERT_EQ(var_manager->SetAllocatedGraphId("var_same", 33U), SUCCESS);  // == changed

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_same")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);
  EXPECT_TRUE(table.entries[0].trans_road.empty());
  EXPECT_TRUE(table.entries[0].trans_src_var_key.empty());
}

// 判据"链上全是 NoOp"：只改描述、不搬数据的链不写入。V1 的 TransVarData 在 D2H 之前做同一
// 扫描，全为 IsNoNeedTrans 即 return SUCCESS、从不触碰转换源；若照写，SO 会去只查询一个容器里
// 根本不存在的转换源（dev_addr == nullptr）并把它误判为加载失败。
// 图标识两条都放行，以隔离出本判据。
TEST_F(VarFixupTableBuilderTest, AllNoOpRoadFoldsOutOfTransRoad) {
  auto var_manager = InitVarManager(317U);
  ASSERT_NE(var_manager, nullptr);
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  ASSERT_NE(AddVarWithRoad(var_manager, graph, "var_noop", RESHAPE), nullptr);
  ASSERT_EQ(var_manager->SetChangedGraphId("var_noop", 33U), SUCCESS);
  ASSERT_EQ(var_manager->SetAllocatedGraphId("var_noop", 7U), SUCCESS);

  VarFixupTable table;
  const auto var_metas = {MakeSlot(0U, "var_noop")};
  ASSERT_EQ(BuildVarFixupTable(*var_manager, graph, var_metas, kGraphId, table), SUCCESS);
  ASSERT_EQ(table.entries.size(), 1U);
  EXPECT_TRUE(table.entries[0].trans_road.empty());
  EXPECT_TRUE(table.entries[0].trans_src_var_key.empty());
}
}  // namespace
}  // namespace ge
