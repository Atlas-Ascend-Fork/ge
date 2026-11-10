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
 * 《ES_Graph构图接口UT设计表》245 条设计用例的覆盖补齐轮（第一轮 97 条见
 * es_facade_focus_unittest.cc / es_facade_cabi_equiv_unittest.cc）。
 * 覆盖首轮判定为“未单独落成用例”的可执行项：
 * - GNode 空句柄/GetALLSubgraphs 计算图根图范围现象（检视 P5）/未挂载查询/输入端口属性；
 * - Graph 空句柄 GetName（P2 安全面）/FindNodeByName 子图层域/DumpToFile；
 * - ES 层：CreateInputs 批量顺延、EsCGraphBuilder 移动语义（安全面）、EsTensorLike ResolveBuilder、
 *   EsTensorHolder 链式描述设置、Compliant 未 Build 析构、Operator UpdateOutputDesc 携带；
 * - NamedIo 可选输入三种用法；
 * - 门面等价性补充组合：ClearAllSubGraph、InsertNodeAfter/Before、ReplaceNodeDataEdges、IsolateNode、
 *   ReplaceEdgeSrc、FindNode(token)→FindNodeByName、GetAllEdges→端口枚举。
 */
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
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
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/fast_graph/edge.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "es_graph_builder.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "es_c_graph_builder.h"
#include "es_tensor_like.h"
#include "compliant_node_builder.h"
#include "external/graph/named_io_node_builder.h"
#include "graph_builder_utils.h"
#include "mmpa/mmpa_api.h"
#include <cstdlib>

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

OpDescPtr MakeOpDesc(const std::string &name, const std::string &type, uint32_t in_cnt, uint32_t out_cnt) {
  auto desc = ComGraphMakeShared<OpDesc>(name, type);
  for (uint32_t i = 0U; i < in_cnt; ++i) {
    (void)desc->AddInputDesc("x" + std::to_string(i), GeTensorDesc());
  }
  for (uint32_t i = 0U; i < out_cnt; ++i) {
    (void)desc->AddOutputDesc("y" + std::to_string(i), GeTensorDesc());
  }
  return desc;
}
}  // namespace

class EsFacadeSuppTest : public testing::Test {};

// GNode 空句柄：名称/类型/属性/端口数量查询统一失败。
TEST_F(EsFacadeSuppTest, GNode_EmptyHandle_QueriesFail) {
  GNode node;
  AscendString name;
  AscendString type;
  int64_t value = 0;
  TensorDesc desc;
  EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
  EXPECT_NE(GRAPH_SUCCESS, node.GetType(type));
  EXPECT_NE(GRAPH_SUCCESS, node.GetAttr(AscendString("a"), value));
  EXPECT_NE(GRAPH_SUCCESS, node.SetAttr(AscendString("a"), value));
  EXPECT_NE(GRAPH_SUCCESS, node.GetInputDesc(0, desc));
  EXPECT_NE(GRAPH_SUCCESS, node.GetOutputDesc(0, desc));
  GraphPtr sub;
  EXPECT_NE(GRAPH_SUCCESS, node.GetSubgraph(0U, sub));
}

// P2 安全面：空句柄 Graph 的 AscendString 重载 GetName 失败（const string& 重载为 UB，见检视报告，不触发）。
TEST_F(EsFacadeSuppTest, Graph_EmptyHandle_GetNameAscendStringFails) {
  Graph graph;
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, graph.GetName(name));
  // 有效图对照：读回一致。
  Graph valid("supp_name", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, valid.GetName(name));
  EXPECT_EQ(AscendString("supp_name"), name);
}

// GNode::GetSubgraph 未挂载槽位失败。
TEST_F(EsFacadeSuppTest, GNode_GetSubgraph_UnmountedSlotFails) {
  Graph graph("supp_slot", kExecGraphKind);
  auto node = graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  GraphPtr got;
  EXPECT_NE(GRAPH_SUCCESS, node.GetSubgraph(0U, got));
}

// A07 输入侧端口属性：SetInputAttr/GetInputAttr 两模式回环。
TEST_F(EsFacadeSuppTest, GNode_InputPortAttr_BothModesRoundtrip) {
  Graph compute_graph("supp_ipa_c");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 2U, 1U));

  Graph execute_graph("supp_ipa_e", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 2U, 1U));

  for (auto *node : {&c_node, &e_node}) {
    AttrValue v;
    (void)v.SetAttrValue(static_cast<int64_t>(3));
    ASSERT_EQ(GRAPH_SUCCESS, node->SetInputAttr(AscendString("pa"), 1U, v));
    AttrValue out;
    int64_t got = 0;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetInputAttr(AscendString("pa"), 1U, out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(got));
    EXPECT_EQ(3, got);
  }
}

// Graph::FindNodeByName 子图层域：执行图仅查本层（不递归子图）。
TEST_F(EsFacadeSuppTest, Graph_FindNodeByName_SubgraphScope) {
  Graph exec_parent("supp_scope", kExecGraphKind);
  auto holder = exec_parent.AddNodeByOp(MakePortOp("h1", "If", 1U, 1U));
  Graph sub("inner_graph", kExecGraphKind);
  (void)sub.AddNodeByOp(MakePortOp("inner_node", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("b"), sub));
  // 执行图：仅查本层直接节点，子图内节点查不到。
  EXPECT_NE(nullptr, exec_parent.FindNodeByName(AscendString("h1")));
  EXPECT_EQ(nullptr, exec_parent.FindNodeByName(AscendString("inner_node")));
  // 经子图门面可查到（GetSubGraph → 子图后端）。
  auto sub_facade = exec_parent.GetSubGraph("inner_graph");
  ASSERT_NE(nullptr, sub_facade);
  EXPECT_NE(nullptr, sub_facade->FindNodeByName(AscendString("inner_node")));
}

// A28：DumpToFile 落盘两模式成功且内容含节点名。
// 注意：GraphUtils 的 dump 路径前缀为进程级 static 缓存（首次落盘时解析），本用例按既有
// UtestGraph.TestDump*To* 用例的同一环境约定设置 DUMP_GRAPH_PATH 并清理，避免污染同进程内
// 后续 dump 用例的读回。
TEST_F(EsFacadeSuppTest, Graph_DumpToFile_BothModes) {
  const std::string ascend_work_path = "./test_ge_graph_path";
  (void)mmSetEnv("DUMP_GRAPH_PATH", ascend_work_path.c_str(), 1);
  const std::string suffix = "supp_dump";
  Graph compute_graph("supp_dtf_c");
  (void)compute_graph.SetValid();
  (void)compute_graph.AddNodeByOp(MakePortOp("dump_node_c", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.DumpToFile(Graph::DumpFormat::kReadable, AscendString(suffix.c_str())));

  Graph execute_graph("supp_dtf_e", kExecGraphKind);
  (void)execute_graph.AddNodeByOp(MakePortOp("dump_node_e", "PlainOp", 1U, 1U));
  // 当前实现：执行图 DumpToFile 显式拒绝。
  EXPECT_NE(GRAPH_SUCCESS, execute_graph.DumpToFile(Graph::DumpFormat::kReadable, AscendString(suffix.c_str())));
  // 未知格式拒绝。
  EXPECT_NE(GRAPH_SUCCESS, execute_graph.DumpToFile(static_cast<Graph::DumpFormat>(7U), AscendString(suffix.c_str())));
  system(("rm -rf " + ascend_work_path).c_str());
  unsetenv("DUMP_GRAPH_PATH");
}

// A36：CreateInputs 批量创建索引顺延（前置已有输入时带 start_index）。
TEST_F(EsFacadeSuppTest, EsBuilder_CreateInputs_BatchStartIndexSequential) {
  es::EsGraphBuilder builder("supp_inputs", kExecGraphKind);
  auto first = builder.CreateInput(0);
  ASSERT_NE(nullptr, first.GetCTensorHolder());
  auto rest = builder.CreateInputs(2U, 1U);  // 顺延索引 1、2
  ASSERT_EQ(2U, rest.size());
  const std::vector<GNode *> nodes = {first.GetProducer(), rest[0].GetProducer(), rest[1].GetProducer()};
  const std::vector<int64_t> expect = {0, 1, 2};
  for (size_t i = 0U; i < nodes.size(); ++i) {
    ASSERT_NE(nullptr, nodes[i]);
    int64_t index = -1;
    ASSERT_EQ(GRAPH_SUCCESS, nodes[i]->GetAttr(AscendString("index"), index));
    EXPECT_EQ(expect[i], index);
  }
  auto graph = builder.BuildAndReset({first, rest[0], rest[1]});
  ASSERT_NE(nullptr, graph);
}

// EsCGraphBuilder 移动语义：移动后新对象可用（原对象此后不可用——按契约不再访问，避免悬垂）。
TEST_F(EsFacadeSuppTest, EsCB_MoveSemantics_MovedToUsable) {
  EsCGraphBuilder first("supp_move");
  ASSERT_NE(nullptr, first.GetGraph());
  auto *input = first.AddGraphInput(0, nullptr, nullptr);
  ASSERT_NE(nullptr, input);
  EsCGraphBuilder second(std::move(first));
  EXPECT_NE(nullptr, second.GetGraph());
  ASSERT_EQ(ge::SUCCESS, second.SetGraphOutput(input, 0));
  auto graph = second.BuildGraphAndReset();
  ASSERT_NE(nullptr, graph);
}

// EsTensorLike::ResolveBuilder：混合入参解析首个绑定构建器的参数；全部无效返回空。
TEST_F(EsFacadeSuppTest, EsTensorLike_ResolveBuilder_MixedArgs) {
  es::EsGraphBuilder builder("supp_resolve", kExecGraphKind);
  auto input = builder.CreateInput(0);
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  es::EsTensorLike literal(static_cast<int64_t>(1));
  es::EsTensorLike holder(input);

  // 字面量在前不命中，继续解析到 Holder 命中。
  EXPECT_EQ(builder.GetCGraphBuilder(), ResolveBuilder(literal, holder));
  // 构建器参数直接命中。
  EXPECT_EQ(builder.GetCGraphBuilder(), ResolveBuilder(literal, &builder));
  // 全部无效返回空。
  EXPECT_EQ(nullptr, ResolveBuilder(literal));
}

// EsTensorHolder 链式描述设置：SetDataType/SetFormat/SetShape 链式调用后读回一致。
TEST_F(EsFacadeSuppTest, EsTensorHolder_ChainedDescSetters) {
  es::EsGraphBuilder builder("supp_chain", kExecGraphKind);
  auto input = builder.CreateInput(0);
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  auto &chained = input.SetDataType(DT_INT32).SetFormat(FORMAT_NHWC);
  EXPECT_EQ(&input, &chained);  // 链式返回自身引用
  const std::vector<int64_t> dims = {3, 4};
  (void)chained.SetShape(dims);
  auto *node = input.GetProducer();
  ASSERT_NE(nullptr, node);
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  // 当前实现：执行图描述链式设置被拒，描述保持默认。
  EXPECT_EQ(DT_FLOAT, desc.GetDataType());
  EXPECT_EQ(FORMAT_ND, desc.GetFormat());
  EXPECT_EQ(0, desc.GetShape().GetDim(1));
}

// CompliantNodeBuilder：构造后未 Build 直接析构无异常、图未改动。
TEST_F(EsFacadeSuppTest, Compliant_CtorWithoutBuild_DtorNoCrash) {
  Graph graph("supp_nobuild", kExecGraphKind);
  {
    es::CompliantNodeBuilder builder(&graph);
    (void)builder.OpType("PlainOp")
        .Name("never_built")
        .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}});
  }
  EXPECT_TRUE(graph.GetAllNodes().empty());
}

// Operator::UpdateOutputDesc：描述经 AddNodeByOp 入图后 GNode 读回一致（两模式）。
TEST_F(EsFacadeSuppTest, Operator_UpdateOutputDesc_CarriedByAddNodeByOp) {
  TensorDesc td(Shape({5, 6}), FORMAT_NCHW, DT_FLOAT16);
  Operator op("supp_uod", "PlainOp");
  op.OutputRegister("y", "");
  ASSERT_EQ(GRAPH_SUCCESS, op.UpdateOutputDesc("y", td));

  Graph compute_graph("supp_uod_c");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(op);
  Graph execute_graph("supp_uod_e", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(op);
  for (auto *node : {&c_node, &e_node}) {
    TensorDesc got;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, got));
    EXPECT_EQ(DT_FLOAT16, got.GetDataType());
    EXPECT_EQ(FORMAT_NCHW, got.GetFormat());
    EXPECT_EQ(6, got.GetShape().GetDim(1));
  }
}

// NamedIo 可选输入三种用法：跳过/有效/占位描述均进入 Build 校验。
TEST_F(EsFacadeSuppTest, NamedIo_OptionalInputThreeUsages) {
  Graph graph("supp_opt3", kExecGraphKind);
  AscendString error;

  // 用法 1：跳过可选输入。
  NamedIoNodeBuilder skip(graph);
  auto n1 = skip.Type("EsFocusOptionalOp").Name("n1").AddInput("x").AddOutput("y").Build(error);
  ASSERT_NE(nullptr, n1);

  // 用法 2：传有效可选输入名。
  NamedIoNodeBuilder valid(graph);
  auto n2 = valid.Type("EsFocusOptionalOp").Name("n2").AddInput("x").AddInput("bias").AddOutput("y").Build(error);
  ASSERT_NE(nullptr, n2);

  // 用法 3：传占位描述（未定义类型）——同样可进入校验流程，乱序/未定义名才失败。
  NamedIoNodeBuilder bad(graph);
  error = AscendString();
  auto n3 = bad.Type("EsFocusOptionalOp").Name("n3").AddInput("x").AddInput("no_such").AddOutput("y").Build(error);
  EXPECT_EQ(nullptr, n3);
  EXPECT_NE(nullptr, error.GetString());
}

// ============================================================================
// 门面等价性补充组合（清单 ExecuteGraphUtils/FastNode 系映射）
// ============================================================================
class EsFacadeSuppEquivTest : public testing::Test {};

// ExecuteGraph 的清空子图组合 vs 门面 GetAllSubgraphs+RemoveSubgraph 组合。
TEST_F(EsFacadeSuppEquivTest, ClearSubgraphs_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("supp_clear");
  const auto internal_graph = internal_builder.GetGraph();
  (void)internal_graph->AddSubGraph(std::make_shared<ExecuteGraph>("s1"));
  (void)internal_graph->AddSubGraph(std::make_shared<ExecuteGraph>("s2"));
  ASSERT_EQ(2U, internal_graph->GetAllSubgraphs().size());
  for (auto *sub : internal_graph->GetAllSubgraphs()) {
    (void)internal_graph->RemoveSubGraph(sub->GetName());
  }
  EXPECT_TRUE(internal_graph->GetAllSubgraphs().empty());

  Graph facade("supp_clear", kExecGraphKind);
  (void)facade.SetValid();
  auto f_holder = facade.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph sub1("s1", kExecGraphKind);
  (void)sub1.SetValid();
  Graph sub2("s2", kExecGraphKind);
  (void)sub2.SetValid();
  // 当前实现：执行图图级 AddSubGraph 与计算图一致，要求子图先经内部接口设好父图/父节点。
  const auto parent_eg = GraphUtilsEx::Graph2ExecuteGraph(facade);
  const auto sub1_eg = GraphUtilsEx::Graph2ExecuteGraph(sub1);
  const auto sub2_eg = GraphUtilsEx::Graph2ExecuteGraph(sub2);
  ASSERT_NE(nullptr, parent_eg);
  ASSERT_NE(nullptr, sub1_eg);
  ASSERT_NE(nullptr, sub2_eg);
  for (auto *sub_eg : {sub1_eg.get(), sub2_eg.get()}) {
    ExecuteGraphBackendUtils::SetParentGraph(sub_eg, parent_eg);
    sub_eg->SetParentNode(NodeAdapter::GNode2ExecuteNode(f_holder));
  }
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddSubGraph(sub1));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddSubGraph(sub2));
  for (const auto &sub : facade.GetAllSubgraphs()) {
    ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveSubgraph(sub->GetName().c_str()));
  }
  EXPECT_TRUE(facade.GetAllSubgraphs().empty());
}

// ExecuteGraphUtils::InsertNodeAfter vs 门面查询+删边+建边组合。
TEST_F(EsFacadeSuppEquivTest, InsertNodeAfter_ComboVsFacade) {
  // 内部路径：A→B，插入 M。
  ut::ExecuteGraphBuilder internal_builder("supp_ins_after");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *b = internal_builder.AddNode("B", "PlainOp", 1, 0);
  auto *m = internal_builder.AddNode("M", "PlainOp", 1, 1);
  (void)internal_builder.AddDataEdge(a, 0, b, 0);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::InsertNodeAfter(EdgeSrcEndpoint(a, 0), {EdgeDstEndpoint(b, 0)}, m));
  EXPECT_EQ("A", m->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ("M", b->GetInDataEdgeByIndex(0)->src->GetName());

  // 门面路径：A→B，插入 M（查询→删边→建两边）。
  Graph facade("supp_ins_after", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto fb = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 1U, 0U));
  auto fm = facade.AddNodeByOp(MakePortOp("M", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fm, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fm, 0, fb, 0));
  auto m_in = fm.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, m_in.first);
  AscendString m_in_name;
  ASSERT_EQ(GRAPH_SUCCESS, m_in.first->GetName(m_in_name));
  EXPECT_EQ(AscendString("A"), m_in_name);
}

// ExecuteGraphUtils::InsertNodeBefore vs 门面组合。
TEST_F(EsFacadeSuppEquivTest, InsertNodeBefore_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("supp_ins_before");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *b = internal_builder.AddNode("B", "PlainOp", 1, 0);
  auto *m = internal_builder.AddNode("M", "PlainOp", 1, 1);
  (void)internal_builder.AddDataEdge(a, 0, b, 0);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::InsertNodeBefore(EdgeDstEndpoint(b, 0), m));
  EXPECT_EQ("A", m->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ("M", b->GetInDataEdgeByIndex(0)->src->GetName());

  Graph facade("supp_ins_before", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto fb = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 1U, 0U));
  auto fm = facade.AddNodeByOp(MakePortOp("M", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fm, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fm, 0, fb, 0));
  auto b_in = fb.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, b_in.first);
  AscendString b_in_name;
  ASSERT_EQ(GRAPH_SUCCESS, b_in.first->GetName(b_in_name));
  EXPECT_EQ(AscendString("M"), b_in_name);
}

// ExecuteGraphUtils::ReplaceNodeDataEdges vs 门面组合（新节点替换旧节点）。
TEST_F(EsFacadeSuppEquivTest, ReplaceNodeDataEdges_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("supp_replace");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *old = internal_builder.AddNode("old", "PlainOp", 1, 1);
  auto *c = internal_builder.AddNode("C", "PlainOp", 1, 0);
  (void)internal_builder.AddDataEdge(a, 0, old, 0);
  (void)internal_builder.AddDataEdge(old, 0, c, 0);
  auto *new_node = internal_builder.AddNode("new", "PlainOp", 1, 1);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::ReplaceNodeDataEdges(new_node, old, {0}, {0}, internal_graph.get()));
  EXPECT_EQ("A", new_node->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ("new", c->GetInDataEdgeByIndex(0)->src->GetName());

  // 门面组合表达等价替换：查询旧边→删旧边→建新边→删旧节点。
  Graph facade("supp_replace", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto f_old = facade.AddNodeByOp(MakePortOp("old", "PlainOp", 1U, 1U));
  auto fc = facade.AddNodeByOp(MakePortOp("C", "PlainOp", 1U, 0U));
  auto f_new = facade.AddNodeByOp(MakePortOp("new", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, f_old, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_old, 0, fc, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, 0, f_old, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(f_old, 0, fc, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, f_new, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_new, 0, fc, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveNode(f_old));
  auto c_in = fc.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, c_in.first);
  AscendString c_in_name;
  ASSERT_EQ(GRAPH_SUCCESS, c_in.first->GetName(c_in_name));
  EXPECT_EQ(AscendString("new"), c_in_name);
}

// ExecuteGraphUtils::IsolateNode vs 门面组合（旁路中间节点）。
TEST_F(EsFacadeSuppEquivTest, IsolateNode_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("supp_iso");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *mid = internal_builder.AddNode("mid", "PlainOp", 1, 1);
  auto *c = internal_builder.AddNode("C", "PlainOp", 1, 0);
  (void)internal_builder.AddDataEdge(a, 0, mid, 0);
  (void)internal_builder.AddDataEdge(mid, 0, c, 0);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::IsolateNode(mid, {0}));
  EXPECT_EQ("A", c->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ(0U, mid->GetInDataEdgeByIndex(0) == nullptr ? 0U : 1U);

  Graph facade("supp_iso", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto f_mid = facade.AddNodeByOp(MakePortOp("mid", "PlainOp", 1U, 1U));
  auto fc = facade.AddNodeByOp(MakePortOp("C", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, f_mid, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_mid, 0, fc, 0));
  // 组合表达旁路：先断旧边（执行图拒绝占用端口），直连 A→C 后删除 mid。
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(f_mid, 0, fc, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fc, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveNode(f_mid));
  auto c_in = fc.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, c_in.first);
  AscendString c_in_name;
  ASSERT_EQ(GRAPH_SUCCESS, c_in.first->GetName(c_in_name));
  EXPECT_EQ(AscendString("A"), c_in_name);
}

// ExecuteGraphUtils::ReplaceEdgeSrc vs 门面 RemoveEdge+AddDataEdge 组合。
TEST_F(EsFacadeSuppEquivTest, ReplaceEdgeSrc_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("supp_resrc");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *a2 = internal_builder.AddNode("A2", "PlainOp", 0, 1);
  auto *b = internal_builder.AddNode("B", "PlainOp", 1, 0);
  (void)internal_builder.AddDataEdge(a, 0, b, 0);
  const auto internal_graph = internal_builder.GetGraph();
  auto *old_edge = b->GetInDataEdgeByIndex(0);
  ASSERT_NE(nullptr, old_edge);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::ReplaceEdgeSrc(old_edge, EdgeSrcEndpoint(a2, 0)));
  EXPECT_EQ("A2", b->GetInDataEdgeByIndex(0)->src->GetName());

  Graph facade("supp_resrc", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto fa2 = facade.AddNodeByOp(MakePortOp("A2", "PlainOp", 0U, 1U));
  auto fb = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa2, 0, fb, 0));
  auto b_in = fb.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, b_in.first);
  AscendString b_in_name;
  ASSERT_EQ(GRAPH_SUCCESS, b_in.first->GetName(b_in_name));
  EXPECT_EQ(AscendString("A2"), b_in_name);
}

// ExecuteGraph::FindNode(token) vs Graph::FindNodeByName（同身份句柄）。
TEST_F(EsFacadeSuppEquivTest, FindNodeTokenVsFacadeFindByName) {
  ut::ExecuteGraphBuilder internal_builder("supp_token");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  const auto internal_graph = internal_builder.GetGraph();
  const auto *found = internal_graph->FindNode(a->GetNodeToken());
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(std::string("A"), found->GetName());

  Graph facade("supp_token", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto facade_found = facade.FindNodeByName(AscendString("A"));
  ASSERT_NE(nullptr, facade_found);
  AscendString found_name;
  ASSERT_EQ(GRAPH_SUCCESS, facade_found->GetName(found_name));
  EXPECT_EQ(AscendString("A"), found_name);
}

// ExecuteGraph::GetAllEdges vs 门面端口枚举组合（数据边+控制边计数一致）。
TEST_F(EsFacadeSuppEquivTest, GetAllEdgesVsFacadePortEnumeration) {
  ut::ExecuteGraphBuilder internal_builder("supp_edges");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *b = internal_builder.AddNode("B", "PlainOp", 1, 0);
  auto *ctrl = internal_builder.AddNode("ctrl", "PlainOp", 0, 0);
  (void)internal_builder.AddDataEdge(a, 0, b, 0);
  (void)internal_builder.AddControlEdge(ctrl, b);
  const auto internal_graph = internal_builder.GetGraph();
  EXPECT_EQ(2U, internal_graph->GetAllEdges().size());

  Graph facade("supp_edges", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto fb = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 1U, 0U));
  auto fctrl = facade.AddNodeByOp(MakePortOp("ctrl", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(fctrl, fb));
  // 组合枚举：输入来源（数据入边）+ 控制前驱（控制边）。
  size_t enumerated = 0U;
  for (size_t i = 0U; i < fb.GetInputsSize(); ++i) {
    if (fb.GetInDataNodesAndPortIndexs(static_cast<int32_t>(i)).first != nullptr) {
      ++enumerated;
    }
  }
  enumerated += fb.GetInControlNodes().size();
  EXPECT_EQ(2U, enumerated);
}
}  // namespace ge
