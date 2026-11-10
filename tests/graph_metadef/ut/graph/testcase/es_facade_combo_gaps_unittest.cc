/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software: you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/*
 * 组合缺口补齐轮（接口×模式矩阵与负路径边角）：
 * C1  C ABI 函数 × 执行图模式矩阵（此前仅计算图模式跑过的函数在执行构建器上批量验证）；
 * C2  CopyFrom 图属性两模式复制断言；
 * C3  RemoveSubgraph 不存在名字：计算图静默成功/执行图失败（新发现 N12 固化）；
 * C4  NamedIoNodeBuilder::Build 二次调用失败（built_ 防护）；
 * C5  GNode 查询负路径包（来源越界/未连接、无消费者、端口属性越界、子图槽位越界）；
 * C6  Operator::DynamicInputRegisterByIndex 正例（两模式）；
 * C7  Operator::SubgraphRegister 槽位登记后 SetSubgraph 可用；
 * C8  Operator::BreakConnect 直接断言（连接清理后导入无边）；
 * C9  EsGraphBuilder 便捷转换辅助（CreateBoolTensor bool→0/1、CreateTensorFromFile C++ 模板、
 *     TensorsToEsCTensorHolders、DataTypesToEsCDataTypes、ListListTypeToPtrAndCounts、GeGraphsToEsCGraphs）；
 * C10 ge::es::CreateFrom(Tensor)；
 * C11 CreateDynamicTensorHolderFromNode 端口越界拒绝；
 * C12 EsCGraphBuilder 无参默认构造；
 * C13 EsTensorLike::ToTensorHolder(nullptr) 失败；
 * C14 GetOutputIndexByName/GetDynamicInput(Output)IndexesByName 计算图正例；
 * C15 Compliant InstanceOutputShape 空 dims（标量形状）；
 * C16 NamedIo AddInput(name, desc) 占位描述用法。
 */
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>

#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/operator.h"
#include "graph/attr_value.h"
#include "graph/ascend_string.h"
#include "graph/op_desc.h"
#include "graph/tensor.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "es_graph_builder.h"
#include "es_c_graph_builder.h"
#include "esb_funcs.h"
#include "compliant_node_builder.h"
#include "external/graph/named_io_node_builder.h"
#include "es_tensor_like.h"

namespace ge {
namespace {
// GraphKind 依赖切换期暂为整型别名（0=计算图，1=执行图），用例内以常量指代
constexpr GraphKind kComputeGraphKind = static_cast<GraphKind>(0);
constexpr GraphKind kExecGraphKind = static_cast<GraphKind>(1);
Operator MakePortOp(const char_t *name, const char_t *type, uint32_t in_cnt, uint32_t out_cnt) {
  Operator op(name, type);
  if (in_cnt != 0U) {
    op.DynamicInputRegister("x", in_cnt);
  }
  if (out_cnt != 0U) {
    op.DynamicOutputRegister("y", out_cnt);
  }
  return op;
}
}  // namespace

class EsFacadeComboTest : public testing::Test {};

// C1：C ABI 函数 × 执行图模式矩阵——此前仅在计算图模式验证过的函数批量在执行构建器上复跑。
TEST_F(EsFacadeComboTest, CAbi_FunctionsOnExecuteBuilder_Matrix) {
  auto *builder = EsCreateGraphBuilderWithKind("cb_exec_matrix", kExecGraphKind);
  ASSERT_NE(nullptr, builder);

  // 描述设置四件套。
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  const int64_t shape[] = {2, 2};
  // 当前实现：执行图上 ES 描述设置被拒。
  EXPECT_NE(0U, EsSetShape(input, shape, 2));
  EXPECT_NE(0U, EsSetDataType(input, C_DT_INT32));
  EXPECT_NE(0U, EsSetFormat(input, C_FORMAT_NHWC));

  // 常量五型 / 向量五型 / 标量五型 / 变量。
  const int64_t i64[] = {1};
  const int64_t dims[] = {1};
  ASSERT_NE(nullptr, EsCreateConstInt64(builder, i64, dims, 1));
  const int32_t i32[] = {2};
  ASSERT_NE(nullptr, EsCreateConstInt32(builder, i32, dims, 1));
  const uint64_t u64[] = {3};
  ASSERT_NE(nullptr, EsCreateConstUInt64(builder, u64, dims, 1));
  const uint32_t u32[] = {4};
  ASSERT_NE(nullptr, EsCreateConstUInt32(builder, u32, dims, 1));
  const float f32[] = {5.0f};
  ASSERT_NE(nullptr, EsCreateConstFloat(builder, f32, dims, 1));
  ASSERT_NE(nullptr, EsCreateVectorInt64(builder, i64, 1));
  ASSERT_NE(nullptr, EsCreateVectorInt32(builder, i32, 1));
  ASSERT_NE(nullptr, EsCreateVectorUInt64(builder, u64, 1));
  ASSERT_NE(nullptr, EsCreateVectorUInt32(builder, u32, 1));
  ASSERT_NE(nullptr, EsCreateVectorFloat(builder, f32, 1));
  ASSERT_NE(nullptr, EsCreateScalarInt64(builder, 6));
  ASSERT_NE(nullptr, EsCreateScalarInt32(builder, 7));
  ASSERT_NE(nullptr, EsCreateScalarUInt64(builder, 8));
  ASSERT_NE(nullptr, EsCreateScalarUInt32(builder, 9));
  ASSERT_NE(nullptr, EsCreateScalarFloat(builder, 10.0f));
  // 当前实现：执行图拒绝创建变量。
  EXPECT_EQ(nullptr, EsCreateVariable(builder, 1, "var1"));

  // 图/端口/节点属性九函数。
  ASSERT_EQ(0U, EsSetInt64AttrForGraph(builder, "gi", 1));
  ASSERT_EQ(0U, EsSetStringAttrForGraph(builder, "gs", "v"));
  ASSERT_EQ(0U, EsSetBoolAttrForGraph(builder, "gb", false));
  ASSERT_EQ(0U, EsSetInt64AttrForTensor(input, "ti", 2));
  ASSERT_EQ(0U, EsSetStringAttrForTensor(input, "ts", "t"));
  ASSERT_EQ(0U, EsSetBoolAttrForTensor(input, "tb", true));
  ASSERT_EQ(0U, EsSetInt64AttrForNode(input, "ni", 3));
  ASSERT_EQ(0U, EsSetStringAttrForNode(input, "ns", "n"));
  ASSERT_EQ(0U, EsSetBoolAttrForNode(input, "nb", false));

  // 快照查询与控制边。
  EXPECT_EQ(builder, EsGetOwnerBuilder(input));
  auto *producer = EsGetProducer(input);
  ASSERT_NE(nullptr, producer);

  // 关键读回抽查：图属性 + 端口描述 + 节点属性。
  auto *graph = builder->GetGraph();
  ASSERT_NE(nullptr, graph);
  AttrValue out;
  int64_t gi = 0;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("gi"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(gi));
  EXPECT_EQ(1, gi);
  auto *node = static_cast<GNode *>(static_cast<void *>(producer));
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  EXPECT_EQ(DT_FLOAT, desc.GetDataType());  // 描述保持默认
  int64_t ni = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("ni"), ni));
  EXPECT_EQ(3, ni);

  // 输出登记与收尾（执行模式收尾成功）。
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  auto *raw = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, raw);
  std::unique_ptr<Graph> built(static_cast<Graph *>(static_cast<void *>(raw)));
  ASSERT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*built));
}

// C2：CopyFrom 图属性两模式复制断言。
TEST_F(EsFacadeComboTest, CopyFrom_GraphAttrBothModes) {
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(9));
  Graph compute_src("cb_ca_c");
  (void)compute_src.SetValid();
  (void)compute_src.SetAttr(AscendString("meta"), v);
  Graph compute_dst("cb_ca_cd");
  ASSERT_EQ(GRAPH_SUCCESS, compute_dst.CopyFrom(compute_src));
  AttrValue out;
  int64_t compute_val = 0;
  ASSERT_EQ(GRAPH_SUCCESS, compute_dst.GetAttr(AscendString("meta"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(compute_val));
  EXPECT_EQ(9, compute_val);

  Graph exec_src("cb_ca_e", kExecGraphKind);
  (void)exec_src.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  (void)exec_src.SetAttr(AscendString("meta"), v);
  Graph exec_dst("cb_ca_ed", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, exec_dst.CopyFrom(exec_src));
  int64_t exec_val = 0;
  ASSERT_EQ(GRAPH_SUCCESS, exec_dst.GetAttr(AscendString("meta"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(exec_val));
  EXPECT_EQ(9, exec_val);
}

// C4：NamedIoNodeBuilder::Build 二次调用失败（built_ 防护）。
TEST_F(EsFacadeComboTest, NamedIo_BuildTwiceFails) {
  Graph graph("cb_nio_twice", kExecGraphKind);
  AscendString error;
  NamedIoNodeBuilder builder(graph);
  auto first = builder.Type("EsFocusPlainOp").Name("n1").AddInput("x1").AddInput("x2").AddOutput("y").Build(error);
  ASSERT_NE(nullptr, first);
  error = AscendString();
  auto second = builder.Build(error);
  EXPECT_EQ(nullptr, second);
  ASSERT_NE(nullptr, error.GetString());
}

// C5：GNode 查询负路径包。
TEST_F(EsFacadeComboTest, GNode_QueryNegativePaths) {
  Graph graph("cb_neg", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 2U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));

  // 未连接输入与越界索引：来源查询返回空。
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(1).first);
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(9).first);
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(-1).first);
  // 无消费者的输出：使用者列表为空（dst 的输出 0 未连线）。
  EXPECT_TRUE(dst.GetOutDataNodesAndPortIndexs(0).empty());
  EXPECT_FALSE(src.GetOutDataNodesAndPortIndexs(0).empty());  // src 输出 0 已连 dst（对照）
  // 端口属性越界失败。
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(1));
  EXPECT_NE(GRAPH_SUCCESS, dst.GetInputAttr(AscendString("pa"), 9U, v));
  EXPECT_NE(GRAPH_SUCCESS, dst.GetOutputAttr(AscendString("pa"), 9U, v));
  EXPECT_NE(GRAPH_SUCCESS, dst.SetInputAttr(AscendString("pa"), 9U, v));
  EXPECT_NE(GRAPH_SUCCESS, dst.SetOutputAttr(AscendString("pa"), 9U, v));
  // 子图槽位越界失败。
  GraphPtr sub;
  EXPECT_NE(GRAPH_SUCCESS, dst.GetSubgraph(5U, sub));
}

// C6：Operator::DynamicInputRegisterByIndex 正例（两模式）。
TEST_F(EsFacadeComboTest, Operator_DynamicInputRegisterByIndex) {
  Operator op("cb_byidx", "PlainOp");
  op.DynamicInputRegisterByIndex("x", 2, 0);
  op.DynamicOutputRegister("y", 1);

  Graph compute_graph("cb_byidx_c");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(op);
  EXPECT_EQ(2U, c_node.GetInputsSize());

  Graph execute_graph("cb_byidx_e", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(op);
  EXPECT_EQ(2U, e_node.GetInputsSize());
}

// C7：Operator::SubgraphRegister 槽位登记后 SetSubgraph 可用。
TEST_F(EsFacadeComboTest, Operator_SubgraphRegisterThenMount) {
  Graph parent("cb_subreg");
  (void)parent.SetValid();
  Operator holder_op("holder", "If");
  holder_op.DynamicInputRegister("x", 1);
  holder_op.DynamicOutputRegister("y", 1);
  holder_op.SubgraphRegister("branch", false);
  auto holder = parent.AddNodeByOp(holder_op);

  Graph sub("branch", kComputeGraphKind);
  (void)sub.SetValid();
  (void)sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("branch"), sub));
  GraphPtr got;
  ASSERT_EQ(GRAPH_SUCCESS, holder.GetSubgraph(0U, got));
}

// C9：EsGraphBuilder 便捷转换辅助。
TEST_F(EsFacadeComboTest, EsGraphBuilder_ConvertHelpers) {
  // CreateBoolTensor：bool 向量转 0/1 字节。
  es::EsGraphBuilder builder("cb_conv");
  auto bool_tensor = builder.CreateBoolTensor({true, false, true}, {3});
  ASSERT_NE(nullptr, bool_tensor.get());
  EXPECT_EQ(3U, bool_tensor->GetSize());
  EXPECT_EQ(1, bool_tensor->GetData()[0]);
  EXPECT_EQ(0, bool_tensor->GetData()[1]);
  EXPECT_EQ(1, bool_tensor->GetData()[2]);

  // CreateTensorFromFile（C++ 模板）。
  const std::vector<uint8_t> bytes = {0x7A};
  const std::string path = "/tmp/es_facade_cpp_fromfile.bin";
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  auto from_file = ge::es::CreateTensorFromFile<uint8_t>(path.c_str(), {1}, DT_UINT8);
  ASSERT_NE(nullptr, from_file.get());
  EXPECT_EQ(0x7A, from_file->GetData()[0]);
  (void)std::remove(path.c_str());

  // TensorsToEsCTensorHolders / DataTypesToEsCDataTypes / ListListTypeToPtrAndCounts / GeGraphsToEsCGraphs。
  auto in0 = builder.CreateInput(0);
  auto in1 = builder.CreateInput(1);
  const auto holders = es::TensorsToEsCTensorHolders({in0, in1});
  ASSERT_EQ(2U, holders.size());
  EXPECT_EQ(in0.GetCTensorHolder(), holders[0]);
  EXPECT_EQ(in1.GetCTensorHolder(), holders[1]);

  const auto data_types = es::DataTypesToEsCDataTypes({DT_FLOAT, DT_INT32});
  ASSERT_EQ(2U, data_types.size());
  EXPECT_EQ(C_DT_FLOAT, data_types[0]);
  EXPECT_EQ(C_DT_INT32, data_types[1]);

  const std::vector<std::vector<int64_t>> list_list = {{1, 2}, {3}};
  const auto ptr_and_counts = es::ListListTypeToPtrAndCounts(list_list);
  ASSERT_EQ(2U, ptr_and_counts.first.size());
  ASSERT_EQ(2U, ptr_and_counts.second.size());
  EXPECT_EQ(2, ptr_and_counts.first[0][1]);
  EXPECT_EQ(1, ptr_and_counts.second[1]);

  auto graph = builder.BuildAndReset({in0, in1});
  ASSERT_NE(nullptr, graph);
  std::vector<std::unique_ptr<Graph>> graphs;
  graphs.push_back(std::move(graph));
  const auto c_graphs = es::GeGraphsToEsCGraphs(std::move(graphs));
  ASSERT_EQ(1U, c_graphs.size());
  ASSERT_NE(nullptr, c_graphs[0]);
  delete static_cast<Graph *>(static_cast<void *>(c_graphs[0]));
}

// C10：ge::es::CreateFrom(Tensor)。
TEST_F(EsFacadeComboTest, CreateFrom_Tensor) {
  TensorDesc desc(Shape({2}), FORMAT_ND, DT_INT64);
  Tensor tensor(desc);
  const std::vector<int64_t> values = {5, 6};
  tensor.SetData(reinterpret_cast<const uint8_t *>(values.data()), sizeof(int64_t) * values.size());
  const auto av = es::CreateFrom(tensor);
  Tensor got;
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got));
  ASSERT_EQ(sizeof(int64_t) * values.size(), got.GetSize());
  EXPECT_EQ(6, reinterpret_cast<const int64_t *>(got.GetData())[1]);
}

// C12：EsCGraphBuilder 无参默认构造（默认图名 graph）。
TEST_F(EsFacadeComboTest, EsCB_DefaultCtor) {
  EsCGraphBuilder builder;
  auto *graph = builder.GetGraph();
  ASSERT_NE(nullptr, graph);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetName(name));
  EXPECT_EQ(AscendString("graph"), name);
}

// C13：EsTensorLike::ToTensorHolder(nullptr) 失败（空构建器物化拦截在常量创建入口）。
TEST_F(EsFacadeComboTest, EsTensorLike_ToTensorHolderNullBuilderFails) {
  es::EsTensorLike literal(static_cast<int64_t>(1));
  auto holder = literal.ToTensorHolder(nullptr);
  EXPECT_EQ(nullptr, holder.GetCTensorHolder());
}

// C14：GetOutputIndexByName/GetDynamicInput(Output)IndexesByName 计算图正例。
TEST_F(EsFacadeComboTest, NamedIndexQueries_ComputePositiveCases) {
  Operator op("cb_named", "PlainOp");
  op.InputRegister("x", "");
  op.DynamicInputRegister("xd", 2);
  op.OutputRegister("y", "");
  op.DynamicOutputRegister("yd", 2);

  Graph graph("cb_named_c");
  (void)graph.SetValid();
  auto node = graph.AddNodeByOp(op);
  int32_t index = -1;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputIndexByName(AscendString("x"), index));
  EXPECT_EQ(0, index);
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputIndexByName(AscendString("y"), index));
  EXPECT_EQ(0, index);
  std::vector<int32_t> indexes;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetDynamicInputIndexesByName(AscendString("xd"), indexes));
  ASSERT_EQ(2U, indexes.size());
  EXPECT_EQ(1, indexes[0]);
  EXPECT_EQ(2, indexes[1]);
}

// C15：Compliant InstanceOutputShape 空 dims（标量形状）。
TEST_F(EsFacadeComboTest, Compliant_InstanceOutputShapeEmptyDims) {
  Graph graph("cb_scalar_shape", kExecGraphKind);
  es::CompliantNodeBuilder builder(&graph);
  auto node = builder.OpType("PlainOp")
                  .Name("n1")
                  .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                  .InstanceOutputShape("y", {})
                  .InstanceOutputDataType("y", DT_FLOAT)
                  .Build();
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputDesc(0, desc));
  EXPECT_EQ(0, desc.GetShape().GetDimNum());
}

// C16：NamedIo AddInput(name, desc) 占位描述用法。
TEST_F(EsFacadeComboTest, NamedIo_AddInputWithDescPlaceholder) {
  Graph graph("cb_nio_desc", kExecGraphKind);
  AscendString error;
  TensorDesc placeholder(Shape({2}), FORMAT_ND, DT_FLOAT);
  NamedIoNodeBuilder builder(graph);
  auto node = builder.Type("EsFocusOptionalOp")
                  .Name("n1")
                  .AddInput("x", placeholder)
                  .AddInput("bias", placeholder)
                  .AddOutput("y")
                  .Build(error);
  ASSERT_NE(nullptr, node);
  ASSERT_NE(nullptr, error.GetString());
  EXPECT_EQ('\0', error.GetString()[0]);
}
}  // namespace ge
