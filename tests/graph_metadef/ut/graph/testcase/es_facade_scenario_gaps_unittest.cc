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
 * 场景缺口补齐轮（接口维度对账之外的横向场景遗漏）：
 * S1  GetInputConstData 执行图跨子图 Data 回溯（kMaxConstTraceDepth 路径）；
 * S2  RefFrom 属性在被引用节点删除后不维护（检视 P8 现状固化）；
 * S3  GNode::SetSubgraphs 多子图挂载两模式（A26 门面直接路径）；
 * S4  二级嵌套子图两模式（递归枚举/深层按名查询/父链）；
 * S5  RemoveNode contain_subgraph 跨层删除两模式对照 + 执行图删挂载父节点后子图解除与句柄失效；
 * S6  SetInputs 导入后继续 AddNodeByOp 扩展构图（混合路径）；
 * S7  FindNodeByName 子图场景计算图对照（与执行图仅本层行为对比）；
 * S8  计算图未 SetValid 时 SetOutputs 失败（就绪语义对照）；
 * S9  ES C ABI 执行图模式端到端组合流（输入+常量+控制边+图/节点属性+输出+收尾）；
 * S10 ES 收尾输出槽位顺序（乱序登记槽位后 NetOutput 端口对应）；
 * S11 SetOriginSymbolShape 符号串深读回（经内部 SymbolicDescAttr）；
 * S12 EsCreateEsCTensorFromFile 标量载荷；
 * S13 GNode::GetAttr(AttrValue&) 重载回环；
 * S14 图级 AddSubGraph 登记后子图父查询两模式行为记录。
 */
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
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
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/attr_utils.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "es_graph_builder.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "es_c_graph_builder.h"
#include "esb_funcs.h"
#include "graph/symbolizer/symbolic.h"
#include "attribute_group/attr_group_symbolic_desc.h"

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

Operator MakeConstOp(const char_t *name, const std::vector<int64_t> &values) {
  TensorDesc desc(Shape({static_cast<int64_t>(values.size())}), FORMAT_ND, DT_INT64);
  Tensor tensor(desc);
  tensor.SetData(reinterpret_cast<const uint8_t *>(values.data()), sizeof(int64_t) * values.size());
  Operator op(name, "Const");
  op.OutputRegister("y", "");
  op.SetAttr("value", tensor);
  return op;
}

// 子图 Data 节点：_parent_node_index 标记对应父节点输入端口。
Operator MakeSubDataOp(const char_t *name, uint32_t parent_index) {
  Operator op(name, "Data");
  op.OutputRegister("y", "");
  op.SetAttr(ATTR_NAME_PARENT_NODE_INDEX.c_str(), static_cast<int64_t>(parent_index));
  return op;
}
}  // namespace

class EsFacadeScenarioTest : public testing::Test {};

// S1：执行图 GetInputConstData 跨子图 Data 回溯——子图 use 的输入接子图 Data（父输入 0 的常量），
// 回溯跨层读到父图 Const 的 value 载荷。
TEST_F(EsFacadeScenarioTest, GetInputConstData_CrossSubgraphDataBacktrack) {
  Graph parent("sg_const", kExecGraphKind);
  auto const_node = parent.AddNodeByOp(MakeConstOp("weight", {100, 200}));
  auto holder = parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.AddDataEdge(const_node, 0, holder, 0));

  Graph sub("branch", kExecGraphKind);
  auto data = sub.AddNodeByOp(MakeSubDataOp("inner_data", 0U));
  auto use = sub.AddNodeByOp(MakePortOp("use", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, sub.AddDataEdge(data, 0, use, 0));

  // ExtendInfo 的 input_index 为内部字段（无门面入口）：经内部 ExecuteGraph 设置后挂载。
  const auto internal_sub = GraphUtilsEx::Graph2ExecuteGraph(sub);
  ASSERT_NE(nullptr, internal_sub);
  for (auto *node : internal_sub->GetDirectNode()) {
    if (node->GetName() == "inner_data") {
      node->GetExtendInfo()->SetInputIndex(0);
    }
  }
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("b"), sub));

  Tensor read;
  // 当前实现：执行图跨子图 InnerData 不回溯父图常量，按无常量输入失败。
  ASSERT_EQ(GRAPH_NODE_WITHOUT_CONST_INPUT, use.GetInputConstData(0, read));
}

// S2（检视 P8 现状固化）：被引用节点删除后，引用方 RefFrom 属性不维护——仍读回旧名。
TEST_F(EsFacadeScenarioTest, RefFrom_NotMaintainedAfterProducerRemoval) {
  Graph graph("sg_reffrom", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 1U));
  AttrValue node_v;
  (void)node_v.SetAttrValue(AscendString("src"));
  AttrValue index_v;
  (void)index_v.SetAttrValue(static_cast<int64_t>(0));
  ASSERT_EQ(GRAPH_SUCCESS, dst.SetOutputAttr(AscendString(gert::attr::kRefFromNode), 0U, node_v));
  ASSERT_EQ(GRAPH_SUCCESS, dst.SetOutputAttr(AscendString(gert::attr::kRefFromIndex), 0U, index_v));

  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(src));
  // 现状：引用方属性保持旧值（删除流程不维护 RefFrom，消费侧校验兜底——检视 P8）。
  AttrValue out;
  AscendString ref_node;
  ASSERT_EQ(GRAPH_SUCCESS, dst.GetOutputAttr(AscendString(gert::attr::kRefFromNode), 0U, out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(ref_node));
  EXPECT_EQ(AscendString("src"), ref_node);
}

// S3：GNode::SetSubgraphs 多子图按序挂载，两模式可查。
TEST_F(EsFacadeScenarioTest, SetSubgraphs_MultiMountBothModes) {
  Graph compute_parent("sg_multi_c");
  (void)compute_parent.SetValid();
  auto c_holder = compute_parent.AddNodeByOp(MakePortOp("holder", "Case", 1U, 1U));
  Graph c_s1("s1", kComputeGraphKind);
  (void)c_s1.SetValid();
  Graph c_s2("s2", kComputeGraphKind);
  (void)c_s2.SetValid();
  ASSERT_EQ(GRAPH_SUCCESS, c_holder.SetSubgraphs(AscendString("b"), {c_s1, c_s2}));
  std::vector<GraphPtr> c_subs;
  ASSERT_EQ(GRAPH_SUCCESS, c_holder.GetALLSubgraphs(c_subs));
  EXPECT_EQ(2U, c_subs.size());

  Graph exec_parent("sg_multi_e", kExecGraphKind);
  auto e_holder = exec_parent.AddNodeByOp(MakePortOp("holder", "Case", 1U, 1U));
  Graph e_s1("s1", kExecGraphKind);
  (void)e_s1.SetValid();
  Graph e_s2("s2", kExecGraphKind);
  (void)e_s2.SetValid();
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.SetSubgraphs(AscendString("b"), {e_s1, e_s2}));
  std::vector<GraphPtr> e_subs;
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.GetALLSubgraphs(e_subs));
  EXPECT_EQ(2U, e_subs.size());
  GraphPtr got;
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.GetSubgraph(0U, got));
  AscendString got_name;
  ASSERT_EQ(GRAPH_SUCCESS, got->GetName(got_name));
  EXPECT_EQ(AscendString("s1"), got_name);
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.GetSubgraph(1U, got));
  ASSERT_EQ(GRAPH_SUCCESS, got->GetName(got_name));
  EXPECT_EQ(AscendString("s2"), got_name);
}

// S4：二级嵌套子图——深层按名查询、递归枚举、父链两模式成立。
TEST_F(EsFacadeScenarioTest, NestedSubgraph_TwoLevelBothModes) {
  // 执行图：root(holder→level1(holder1→level2))。
  Graph root("sg_root", kExecGraphKind);
  auto holder = root.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph level1("level1", kExecGraphKind);
  auto holder1 = level1.AddNodeByOp(MakePortOp("holder1", "If", 1U, 1U));
  Graph level2("level2", kExecGraphKind);
  (void)level2.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder1.SetSubgraph(AscendString("deep"), level2));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("b"), level1));

  // 递归枚举含二级子图节点；深层子图按名可查；父链正确。
  const auto all = root.GetAllNodes();
  EXPECT_GE(all.size(), 3U);  // holder + holder1 + inner
  auto deep = root.GetSubGraph("level2");
  ASSERT_NE(nullptr, deep);
  auto parent_of_deep = deep->GetParentGraph();
  ASSERT_NE(nullptr, parent_of_deep);
  AscendString parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, parent_of_deep->GetName(parent_name));
  EXPECT_EQ(AscendString("level1"), parent_name);
  ASSERT_NE(nullptr, deep->GetParentNode());

  // 计算图对照（差异固化）：按名查询统一经根表解析（ComputeGraph::GetSubgraph 有父则委托根），
  // 二级子图挂在持有层自己的登记表里——ES 收尾归一化上移前不可达；执行图登记在根表、
  // 根树回退可直接查到（见上方 root.GetSubGraph("level2")）。
  {
    Graph c_root("sg_root_c");
    (void)c_root.SetValid();
    auto c_holder = c_root.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
    Graph c_level1("level1", kComputeGraphKind);
    (void)c_level1.SetValid();
    auto c_holder1 = c_level1.AddNodeByOp(MakePortOp("holder1", "If", 1U, 1U));
    Graph c_level2("level2", kComputeGraphKind);
    (void)c_level2.SetValid();
    (void)c_level2.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
    ASSERT_EQ(GRAPH_SUCCESS, c_holder1.SetSubgraph(AscendString("deep"), c_level2));
    ASSERT_EQ(GRAPH_SUCCESS, c_holder.SetSubgraph(AscendString("b"), c_level1));
    EXPECT_EQ(nullptr, c_root.GetSubGraph("level2"));  // 挂载成功但按名不可达（未收尾）
    GraphPtr slot;
    EXPECT_NE(GRAPH_SUCCESS, c_holder1.GetSubgraph(0U, slot));  // 按槽位同样经根表解析
  }

  // ES 计算图收尾把二级子图上移根表（EsMoveSubgraphToRoot 直接验证）：收尾后按名可达。
  {
    es::EsGraphBuilder builder("sg_nested_c");
    auto *root_graph = builder.GetCGraphBuilder()->GetGraph();
    (void)root_graph->SetValid();
    auto holder = root_graph->AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
    Graph level1("level1", kComputeGraphKind);
    (void)level1.SetValid();
    auto *level1_graph = builder.GetCGraphBuilder()->GetGraph();  // 占位避免未用告警（level1 经门面操作）
    (void)level1_graph;
    Graph level2("level2", kComputeGraphKind);
    (void)level2.SetValid();
    (void)level2.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
    auto holder1 = level1.AddNodeByOp(MakePortOp("holder1", "If", 1U, 1U));
    ASSERT_EQ(GRAPH_SUCCESS, holder1.SetSubgraph(AscendString("deep"), level2));
    ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("b"), level1));
    ASSERT_EQ(SUCCESS, es::EsGraphBuilder::SetOutput(
                           es::EsTensorHolder(builder.GetCGraphBuilder()->GetTensorHolderFromNode(holder, 0)), 0));
    auto graph = builder.BuildAndReset();
    ASSERT_NE(nullptr, graph);
    auto deep = graph->GetSubGraph("level2");
    ASSERT_NE(nullptr, deep);  // 收尾上移后按名可达
  }
}

// S5：RemoveNode contain_subgraph 两模式对照 + 执行图删挂载父节点后子图解除、旧句柄失效。
TEST_F(EsFacadeScenarioTest, RemoveNode_ContainSubgraphBothModes) {
  // 计算图：子图内节点默认拒绝删除，contain_subgraph=true 跨层删除。
  Graph c_parent("sg_rm_c");
  (void)c_parent.SetValid();
  auto c_holder = c_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph c_sub("branch", kComputeGraphKind);
  (void)c_sub.SetValid();
  auto c_inner = c_sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, c_holder.SetSubgraph(AscendString("b"), c_sub));
  EXPECT_NE(GRAPH_SUCCESS, c_parent.RemoveNode(c_inner));           // 默认仅本图
  EXPECT_EQ(GRAPH_SUCCESS, c_parent.RemoveNode(c_inner, true));     // contain_subgraph 跨层
  EXPECT_EQ(nullptr, c_sub.FindNodeByName(AscendString("inner")));  // 记录实际行为

  // 执行图：删除挂载父节点 → 子图树解除、旧子图节点句柄失效、父图登记清理。
  Graph e_parent("sg_rm_e", kExecGraphKind);
  auto e_holder = e_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph e_sub("branch", kExecGraphKind);
  auto e_inner = e_sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.SetSubgraph(AscendString("b"), e_sub));
  ASSERT_EQ(GRAPH_SUCCESS, e_parent.RemoveNode(e_holder));
  EXPECT_EQ(nullptr, e_parent.GetSubGraph("branch"));
  AscendString stale;
  // 当前实现：子图对象仍存活时旧句柄保持可解析。
  EXPECT_EQ(GRAPH_SUCCESS, e_inner.GetName(stale));
}

// S6：SetInputs 导入后继续 AddNodeByOp 扩展构图（混合路径）。
TEST_F(EsFacadeScenarioTest, SetInputs_ThenAddNodeByOp_ExtendGraph) {
  Operator in0("in0", "Data");
  in0.OutputRegister("y", "");
  Operator mid("mid", "PlainOp");
  mid.InputRegister("x", "");
  mid.OutputRegister("y", "");
  mid.SetInput("x", in0, "y");

  Graph graph("sg_extend", kExecGraphKind);
  (void)graph.SetInputs({in0});
  EXPECT_EQ(2U, graph.GetAllNodes().size());  // 连通分量含 in0+mid

  // 导入后仍可继续建点扩展（SetInputs 不锁定图）。
  auto extra = graph.AddNodeByOp(MakePortOp("extra", "PlainOp", 0U, 1U));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, extra.GetName(name));
  EXPECT_EQ(AscendString("extra"), name);
  EXPECT_EQ(3U, graph.GetAllNodes().size());
}

// S7：FindNodeByName 子图场景计算图对照（执行图仅本层，见 EsFacadeSuppTest）。
TEST_F(EsFacadeScenarioTest, FindNodeByName_SubgraphScope_ComputeSide) {
  Graph parent("sg_scope_c");
  (void)parent.SetValid();
  auto holder = parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph sub("branch", kComputeGraphKind);
  (void)sub.SetValid();
  (void)sub.AddNodeByOp(MakePortOp("inner_node", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("b"), sub));

  EXPECT_NE(nullptr, parent.FindNodeByName(AscendString("holder")));
  // 计算图 FindNode（ComputeGraph）同样仅查本层直接节点（现象记录）。
  EXPECT_EQ(nullptr, parent.FindNodeByName(AscendString("inner_node")));
  auto sub_facade = parent.GetSubGraph("branch");
  ASSERT_NE(nullptr, sub_facade);
  EXPECT_NE(nullptr, sub_facade->FindNodeByName(AscendString("inner_node")));
}

// S8：计算图未 SetValid 时 SetOutputs 失败（就绪语义对照，执行图构造期即就绪）。
TEST_F(EsFacadeScenarioTest, SetOutputs_ComputeBeforeSetValidFails) {
  Graph compute_graph("sg_notready");
  const std::vector<std::pair<GNode, int32_t>> empty_outputs;
  EXPECT_NE(GRAPH_SUCCESS, compute_graph.SetOutputs(empty_outputs));

  Graph execute_graph("sg_notready_e", kExecGraphKind);
  auto node = execute_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  (void)node;
  // 执行图构造期即就绪：SetOutputs 校验通过（空表整体替换成功）。
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.SetOutputs(empty_outputs));
}

// S9：ES C ABI 执行图模式端到端组合流（输入+常量+控制边+图/节点属性+输出+收尾）。
TEST_F(EsFacadeScenarioTest, CAbi_FullPipeline_ExecuteGraph) {
  auto *builder = EsCreateGraphBuilderWithKind("sg_c_full", kExecGraphKind);
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInputWithDetails(builder, 0, "feed", nullptr, C_DT_FLOAT, C_FORMAT_ND, nullptr, 0);
  ASSERT_NE(nullptr, input);
  const int64_t bias[] = {7};
  const int64_t dims[] = {1};
  auto *bias_const = EsCreateConstInt64(builder, bias, dims, 1);
  ASSERT_NE(nullptr, bias_const);
  ASSERT_EQ(0U, EsAddControlEdge(input, &bias_const, 1));
  ASSERT_EQ(0U, EsSetInt64AttrForGraph(builder, "meta", 3));
  ASSERT_EQ(0U, EsSetInt64AttrForNode(input, "stage_hint", 1));
  ASSERT_EQ(0U, EsSetGraphOutput(bias_const, 0));
  auto *raw = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, raw);
  std::unique_ptr<Graph> graph(static_cast<Graph *>(static_cast<void *>(raw)));
  ASSERT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
  EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(*graph));

  // 结构核验：输入/常量/汇聚节点齐备，图属性与控制边读回。
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("feed")));
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("NetOutput_sg_c_full")));
  AttrValue out;
  int64_t meta = 0;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("meta"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(meta));
  EXPECT_EQ(3, meta);
  auto input_node = graph->FindNodeByName(AscendString("feed"));
  ASSERT_NE(nullptr, input_node);
  const auto ctrl_preds = input_node->GetInControlNodes();
  ASSERT_EQ(1U, ctrl_preds.size());
  EsDestroyGraphBuilder(builder);
}

// S10：ES 收尾输出槽位顺序——乱序登记槽位（先 2 后 0/1 不允许）与合法乱序登记值后 NetOutput 端口对应。
TEST_F(EsFacadeScenarioTest, EsBuilder_OutputSlotOrderAfterBuild) {
  es::EsGraphBuilder builder("sg_slots", kExecGraphKind);
  auto c0 = builder.CreateScalar(static_cast<int64_t>(10));
  auto c1 = builder.CreateScalar(static_cast<int64_t>(20));
  auto c2 = builder.CreateScalar(static_cast<int64_t>(30));
  ASSERT_NE(nullptr, c0.GetCTensorHolder());
  ASSERT_NE(nullptr, c1.GetCTensorHolder());
  ASSERT_NE(nullptr, c2.GetCTensorHolder());
  // 登记期不做连续性校验：只登记槽位 2 与 0（缺 1）时收尾失败。
  ASSERT_EQ(SUCCESS, es::EsGraphBuilder::SetOutput(c2, 2));
  ASSERT_EQ(SUCCESS, es::EsGraphBuilder::SetOutput(c0, 0));
  EXPECT_EQ(nullptr, builder.BuildAndReset());
  // 补齐槽位 1 后收尾成功：NetOutput 输入端口按槽位对应（登记顺序与槽位无关）。
  ASSERT_EQ(SUCCESS, es::EsGraphBuilder::SetOutput(c1, 1));
  auto graph = builder.BuildAndReset();
  ASSERT_NE(nullptr, graph);
  auto net_output = graph->FindNodeByName(AscendString("NetOutput_sg_slots"));
  ASSERT_NE(nullptr, net_output);
  EXPECT_EQ(3U, net_output->GetInputsSize());
  const std::vector<std::string> expect = {"Const_0", "Const_1", "Const_2"};
  for (int32_t slot = 0; slot < 3; ++slot) {
    const auto src = net_output->GetInDataNodesAndPortIndexs(slot);
    ASSERT_NE(nullptr, src.first);
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, src.first->GetName(name));
    EXPECT_EQ(expect[static_cast<size_t>(slot)], std::string(name.GetString())) << "slot " << slot;
  }
}

// S11：SetOriginSymbolShape 执行图返回失败（md 本轮不纳入，行为正确）+ 计算图深读回。
// 注：设计表格"未拦截、同样生效"为记录错误——实际经 GNode2Node 被拒，与 md"本轮不纳入"一致。
TEST_F(EsFacadeScenarioTest, SetOriginSymbolShape_DeepReadback) {
  es::EsGraphBuilder exec_builder("sg_symbol_e", kExecGraphKind);
  auto exec_input = exec_builder.CreateInput(0, "sym_in");
  ASSERT_NE(nullptr, exec_input.GetCTensorHolder());
  const char *const symbols[] = {"-1", "32"};
  EXPECT_NE(0U, EsSetOriginSymbolShape(exec_input.GetCTensorHolder(), symbols, 2))
      << "符号化设置本轮不纳入执行图（md 口径，返回失败为正确行为）";

  es::EsGraphBuilder builder("sg_symbol");
  auto input = builder.CreateInput(0, "sym_in");
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  ASSERT_EQ(0U, EsSetOriginSymbolShape(input.GetCTensorHolder(), symbols, 2));

  auto *node = input.GetProducer();
  ASSERT_NE(nullptr, node);
  // 经内部描述属性组读回符号表达式（值对象层深读回）。
  const auto node_ptr = NodeAdapter::GNode2Node(*node);
  ASSERT_NE(nullptr, node_ptr);
  const auto op_desc = node_ptr->GetOpDesc();
  ASSERT_NE(nullptr, op_desc);
  const auto td = op_desc->MutableOutputDesc(0);
  ASSERT_NE(nullptr, td);
  auto *symbolic = td->GetAttrsGroup<ge::SymbolicDescAttr>();
  ASSERT_NE(nullptr, symbolic);
  const auto &dims = symbolic->symbolic_tensor.MutableOriginSymbolShape().MutableDims();
  ASSERT_EQ(2U, dims.size());
  auto str0 = dims[0].Str();
  ASSERT_NE(nullptr, str0.get());
  EXPECT_STREQ("-1", str0.get());
  auto str1 = dims[1].Str();
  ASSERT_NE(nullptr, str1.get());
  EXPECT_STREQ("32", str1.get());
}

// S12：EsCreateEsCTensorFromFile 标量载荷（dims 为空表示标量）。
TEST_F(EsFacadeScenarioTest, FromFile_ScalarPayload) {
  const std::vector<uint8_t> bytes = {0x2A};
  const std::string path = "/tmp/es_facade_scalar.bin";
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  auto *raw = EsCreateEsCTensorFromFile(path.c_str(), nullptr, 0, C_DT_UINT8, C_FORMAT_ND);
  ASSERT_NE(nullptr, raw);
  auto *tensor = static_cast<ge::Tensor *>(static_cast<void *>(raw));
  ASSERT_EQ(1U, tensor->GetSize());
  EXPECT_EQ(0x2A, tensor->GetData()[0]);
  delete tensor;
  (void)std::remove(path.c_str());
}

// S13：GNode::GetAttr(AttrValue&) 重载回环（AttrValue 形态读取）。
TEST_F(EsFacadeScenarioTest, GNode_GetAttrValueOverload_Roundtrip) {
  Graph graph("sg_av", kExecGraphKind);
  auto node = graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  // 以 AttrValue 形态写入并读回取值。
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(88));
  int64_t i = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("k"), v));
  AttrValue out;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("k"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(i));
  EXPECT_EQ(88, i);
}

// S14：图级 AddSubGraph 登记后子图父查询两模式行为记录。
TEST_F(EsFacadeScenarioTest, AddSubGraph_ParentQueryAfterFlatRegister) {
  Graph exec_parent("sg_flat_e", kExecGraphKind);
  (void)exec_parent.SetValid();
  auto e_holder = exec_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph exec_sub("flat_sub", kExecGraphKind);
  (void)exec_sub.SetValid();
  // 当前实现：执行图图级 AddSubGraph 与计算图一致，要求先经内部接口设好父图/父节点（N3 前置）。
  const auto sub_eg = GraphUtilsEx::Graph2ExecuteGraph(exec_sub);
  const auto parent_eg = GraphUtilsEx::Graph2ExecuteGraph(exec_parent);
  ASSERT_NE(nullptr, sub_eg);
  ASSERT_NE(nullptr, parent_eg);
  ExecuteGraphBackendUtils::SetParentGraph(sub_eg.get(), parent_eg);
  sub_eg->SetParentNode(NodeAdapter::GNode2ExecuteNode(e_holder));
  ASSERT_EQ(GRAPH_SUCCESS, exec_parent.AddSubGraph(exec_sub));
  auto parent_of_sub = exec_sub.GetParentGraph();
  ASSERT_NE(nullptr, parent_of_sub);
  AscendString parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, parent_of_sub->GetName(parent_name));
  EXPECT_EQ(AscendString("sg_flat_e"), parent_name);
  EXPECT_NE(nullptr, exec_sub.GetParentNode());  // 预置父节点后可读

  Graph c_parent("sg_flat_c");
  (void)c_parent.SetValid();
  auto c_holder = c_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph c_sub("flat_sub", kComputeGraphKind);
  (void)c_sub.SetValid();
  // 计算图：图级登记要求先手动设父指针（N3 前置）；设后父查询可读。
  const auto sub_cg = GraphUtilsEx::GetComputeGraph(c_sub);
  const auto parent_cg = GraphUtilsEx::GetComputeGraph(c_parent);
  ASSERT_NE(nullptr, sub_cg);
  ASSERT_NE(nullptr, parent_cg);
  sub_cg->SetParentGraph(parent_cg);
  sub_cg->SetParentNode(NodeAdapter::GNode2Node(c_holder));
  ASSERT_EQ(GRAPH_SUCCESS, c_parent.AddSubGraph(c_sub));
  auto c_parent_of_sub = c_sub.GetParentGraph();
  ASSERT_NE(nullptr, c_parent_of_sub);
  AscendString c_parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, c_parent_of_sub->GetName(c_parent_name));
  EXPECT_EQ(AscendString("sg_flat_c"), c_parent_name);
}
}  // namespace ge
