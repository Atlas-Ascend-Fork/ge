/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/*
 * ES/Graph 执行图接口级 dump 验证用例（对齐《ES_Graph构图接口dump验证覆盖方案》接口矩阵）。
 * 三种 dump 验证模式：
 *   正向——构图接口调用后 dump 呈现其结构效果；
 *   不变性——拒绝类接口调用前后 dump 逐字节一致（拒绝不污染图）；
 *   存活性——生命周期事件后存活对象 dump 结构完整。
 * 每条用例锚定覆盖明细（ES_Graph构图接口用例覆盖明细.xlsx）中的接口条目，L1/L2/L3 分层断言。
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "graph/ascend_string.h"
#include "graph/attr_value.h"
#include "graph/graph.h"
#include "graph/graph_buffer.h"
#include "graph/gnode.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/operator.h"
#include "graph/operator_reg.h"
#include "graph/tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "es_graph_builder.h"
#include "es_tensor_like.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "es_c_graph_builder.h"
#include "compliant_node_builder.h"
#include "external/graph/named_io_node_builder.h"
#include "common/es_dump_check_utils.h"

using namespace ge;
using namespace ge::ut::esdump;

namespace {
constexpr GraphKind kExecGraphKind = C_GRAPH_EXECUTE;

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

AttrValue MakeListIntAttr(const std::vector<int64_t> &values) {
  AttrValue av;
  (void)av.SetAttrValue(values);
  return av;
}
}  // namespace

REG_OP(EsIfaceDumpOp)
    .INPUT(x1, TensorType({DT_FLOAT}))
    .INPUT(x2, TensorType({DT_FLOAT}))
    .OUTPUT(y, TensorType({DT_FLOAT}))
    .ATTR(mode, Int, 7)
    .REQUIRED_ATTR(strides, ListInt)
    .OP_END_FACTORY_REG(EsIfaceDumpOp)

        class EsFacadeDumpIfaceTest : public testing::Test {};

// ============================================================================
// 一、正向：构图接口 → dump 呈现结构效果
// ============================================================================

// I01：Graph::SetOutputs(pair<GNode, index>)——NetOutput 承接，readable 渲染 return 段。
TEST_F(EsFacadeDumpIfaceTest, Graph_SetOutputsGNode_NetOutputInDump) {
  Graph graph("if_set_outputs", kExecGraphKind);
  auto producer = graph.AddNodeByOp(MakePortOp("out_node", "PlainOp", 1U, 1U));
  auto consumer = graph.AddNodeByOp(MakePortOp("use_node", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(producer, 0, consumer, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{producer, 0}}));

  // L1：NetOutput 承接（输出登记后图中含 NetOutput 节点）。
  EXPECT_GE(graph.GetAllNodes().size(), 3U);

  // L2：readable 渲染图输出段（NetOutput 不作节点行）。
  const auto readable = DumpReadable(graph);
  EXPECT_NE(std::string::npos, readable.find("return (%out_node")) << readable;

  // L3：回读 NetOutput 与数据边。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(1U, CountTypeOf(loaded->GetDirectNode(), "NetOutput"));
  const auto loaded_netoutput = loaded->FindNode("NetOutput");
  ASSERT_NE(nullptr, loaded_netoutput);
  EXPECT_EQ((std::set<std::string>{"out_node"}), NamesOf(loaded_netoutput->GetInDataNodes()));
}

// I02：Graph::SetOutputs(vector<Operator>) 重载——自动建 NetOutput 后 dump 一致。
TEST_F(EsFacadeDumpIfaceTest, Graph_SetOutputsOperatorOverload_NetOutputInDump) {
  Graph graph("if_set_outputs_op", kExecGraphKind);
  Operator producer = MakePortOp("op_out_node", "PlainOp", 1U, 1U);
  (void)graph.AddNodeByOp(producer);
  (void)graph.SetOutputs({producer});

  // L1：Operator 输出经 NetOutput 承接入图。
  const auto lines = CollectNodeLines(DumpReadable(graph));
  EXPECT_EQ(1U, lines.count("op_out_node"));

  // L3：回读含 NetOutput 且其输入为该节点。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(1U, CountTypeOf(loaded->GetDirectNode(), "NetOutput"));
  EXPECT_NE(nullptr, loaded->FindNode("op_out_node"));
}

// I03：执行图 AddSubGraph 仅登记"已就绪"（含挂载父子关系）子图——未挂载直接登记拒绝，dump 不变。
// 挂载登记路径（SetSubgraph 内部经 AddSubGraph）由 D4/I08 的 dump 证据承载。
TEST_F(EsFacadeDumpIfaceTest, Graph_AddSubGraph_RegistrationInDump) {
  Graph parent("if_add_sub", kExecGraphKind);
  (void)parent.AddNodeByOp(MakePortOp("plain", "PlainOp", 1U, 1U));
  Graph sub("if_branch", kExecGraphKind);
  (void)sub.AddNodeByOp(MakePortOp("sub_node", "PlainOp", 0U, 1U));

  // 未挂载的子图直接登记被拒（执行图口径：只登记已就绪子图），且 dump 不变。
  ExpectReadableUnchangedAround(parent, [&parent, &sub]() { EXPECT_NE(GRAPH_SUCCESS, parent.AddSubGraph(sub)); });
  EXPECT_EQ(0U, parent.GetAllSubgraphs().size());
}

// I04：Graph::RemoveSubgraph——移除后 dump 不再含子图块，可重新登记。
TEST_F(EsFacadeDumpIfaceTest, Graph_RemoveSubgraph_DumpReflectsUnregister) {
  Graph parent("if_rm_sub", kExecGraphKind);
  auto holder = parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph sub("rm_branch", kExecGraphKind);
  (void)sub.AddNodeByOp(MakePortOp("rm_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("rm_branch"), sub));

  // 挂载态 dump 含子图块。
  EXPECT_NE(std::string::npos, DumpReadable(parent).find("graph(\"rm_branch\")"));

  ASSERT_EQ(GRAPH_SUCCESS, parent.RemoveSubgraph("rm_branch"));
  // L1：移除后枚举不含。
  EXPECT_EQ(0U, parent.GetAllSubgraphs().size());
  // L2：dump 不再含子图块与其中节点行。
  const auto readable = DumpReadable(parent);
  EXPECT_EQ(std::string::npos, readable.find("graph(\"rm_branch\")")) << readable;
  EXPECT_EQ(0U, CollectNodeLines(readable).count("rm_inner"));

  // 重新登记后 dump 恢复呈现。
  ASSERT_EQ(GRAPH_SUCCESS, parent.AddSubGraph(sub));
  EXPECT_NE(std::string::npos, DumpReadable(parent).find("rm_branch"));
}

// I05：Graph::RemoveEdge——删数据边后 readable inputs 引用消失、回读边消失。
TEST_F(EsFacadeDumpIfaceTest, Graph_RemoveEdge_DumpReflectsRemovedEdge) {
  Graph graph("if_rm_edge", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("edge_src", "Data", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakePortOp("edge_dst", "Relu", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));
  EXPECT_NE(std::string::npos, DumpReadable(graph).find("%edge_src"));

  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(src, 0, dst, 0));
  // L1：删后可重连。
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);
  // L2：dst 行不再引用 src。
  const auto lines = CollectNodeLines(DumpReadable(graph));
  EXPECT_EQ(std::string::npos, lines.at("edge_dst").find("%edge_src")) << lines.at("edge_dst");
  // L3：回读无该数据边。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_dst = loaded->FindNode("edge_dst");
  ASSERT_NE(nullptr, loaded_dst);
  EXPECT_TRUE(loaded_dst->GetInDataNodes().empty());
  // 重连后 dump 恢复引用。
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));
  EXPECT_NE(std::string::npos, CollectNodeLines(DumpReadable(graph)).at("edge_dst").find("%edge_src"));
}

// I06：GNode::SetInputAttr/SetOutputAttr——端口属性经 kTxt dump 持久化（属性存于端口描述）。
TEST_F(EsFacadeDumpIfaceTest, GNode_PortAttrs_PersistThroughDump) {
  Graph graph("if_port_attr", kExecGraphKind);
  auto node = graph.AddNodeByOp(MakePortOp("pa_node", "PlainOp", 1U, 1U));
  AttrValue in_v;
  (void)in_v.SetAttrValue(static_cast<int64_t>(99));
  AttrValue out_v;
  (void)out_v.SetAttrValue(AscendString("port_val"));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetInputAttr(AscendString("pa_in"), 0U, in_v));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetOutputAttr(AscendString("pa_out"), 0U, out_v));

  // L1：读回一致。
  AttrValue got;
  int64_t in_read = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputAttr(AscendString("pa_in"), 0U, got));
  ASSERT_EQ(GRAPH_SUCCESS, got.GetAttrValue(in_read));
  EXPECT_EQ(99, in_read);

  // L3：kTxt dump 含端口属性名（端口描述序列化），回读描述一致。
  const auto txt = DumpTxt(graph);
  EXPECT_NE(std::string::npos, txt.find("pa_in")) << txt.substr(0, 512);
  EXPECT_NE(std::string::npos, txt.find("pa_out"));
}

// I07：GNode::UpdateInputDesc/UpdateOutputDesc——端口描述经 dump 回环一致。
TEST_F(EsFacadeDumpIfaceTest, GNode_UpdatePortDesc_RoundTripInDump) {
  Graph graph("if_port_desc", kExecGraphKind);
  auto node = graph.AddNodeByOp(MakePortOp("pd_node", "PlainOp", 1U, 1U));
  const TensorDesc in_desc(Shape({2, 3}), FORMAT_NCHW, DT_FLOAT16);
  const TensorDesc out_desc(Shape({5}), FORMAT_ND, DT_INT32);
  ASSERT_EQ(GRAPH_SUCCESS, node.UpdateInputDesc(0, in_desc));
  ASSERT_EQ(GRAPH_SUCCESS, node.UpdateOutputDesc(0, out_desc));

  // L3：回读端口描述 shape/dtype/format 一致。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_node = loaded->FindNode("pd_node");
  ASSERT_NE(nullptr, loaded_node);
  const auto &loaded_in = loaded_node->GetOpDesc()->GetInputDesc(0);
  EXPECT_EQ((std::vector<int64_t>({2, 3})), loaded_in.GetShape().GetDims());
  EXPECT_EQ(DT_FLOAT16, loaded_in.GetDataType());
  EXPECT_EQ(FORMAT_NCHW, loaded_in.GetFormat());
  const auto &loaded_out = loaded_node->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ((std::vector<int64_t>({5})), loaded_out.GetShape().GetDims());
  EXPECT_EQ(DT_INT32, loaded_out.GetDataType());
}

// I08：GNode::SetSubgraphs——一组子图挂载，dump 多图块呈现。
TEST_F(EsFacadeDumpIfaceTest, GNode_SetSubgraphs_MultiBlocksInDump) {
  Graph parent("if_multi_sub", kExecGraphKind);
  auto holder = parent.AddNodeByOp(MakePortOp("multi_holder", "If", 1U, 1U));
  Graph then_g("then", kExecGraphKind);
  (void)then_g.AddNodeByOp(MakePortOp("then_node", "PlainOp", 0U, 1U));
  Graph else_g("else", kExecGraphKind);
  (void)else_g.AddNodeByOp(MakePortOp("else_node", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraphs(AscendString("b"), {then_g, else_g}));

  // L1：两子图可查。
  EXPECT_EQ(2U, parent.GetAllSubgraphs().size());
  // L2：readable 两子图块。
  const auto readable = DumpReadable(parent);
  EXPECT_NE(std::string::npos, readable.find("graph(\"then\")")) << readable;
  EXPECT_NE(std::string::npos, readable.find("graph(\"else\")")) << readable;
  const auto lines = CollectNodeLines(readable);
  EXPECT_EQ(1U, lines.count("then_node"));
  EXPECT_EQ(1U, lines.count("else_node"));
}

// I09：Operator 端口注册族——入图后端口计数在 dump 回读一致。
TEST_F(EsFacadeDumpIfaceTest, Operator_PortRegistration_VisibleInDump) {
  Graph graph("if_port_reg", kExecGraphKind);
  // 固定输入 + 固定输出（InputRegister/OutputRegister）。
  Operator fixed_op("reg_fixed", "PlainOp");
  fixed_op.InputRegister("a", "");
  fixed_op.OutputRegister("r", "");
  // 可选输入（OptionalInputRegister）。
  Operator optional_op("reg_optional", "PlainOp");
  optional_op.InputRegister("a", "");
  optional_op.OptionalInputRegister("o", "");
  optional_op.OutputRegister("r", "");
  // 动态输入组按位插入（DynamicInputRegisterByIndex）+ 动态输出。
  Operator dynamic_op("reg_dynamic", "PlainOp");
  dynamic_op.DynamicInputRegisterByIndex("d", 2U, 0U);
  dynamic_op.DynamicOutputRegister("r", 2U);

  auto fixed_node = graph.AddNodeByOp(fixed_op);
  auto optional_node = graph.AddNodeByOp(optional_op);
  auto dynamic_node = graph.AddNodeByOp(dynamic_op);
  // L1：端口计数。
  EXPECT_EQ(1U, fixed_node.GetInputsSize());
  EXPECT_EQ(1U, fixed_node.GetOutputsSize());
  EXPECT_EQ(1U, optional_node.GetInputsSize());  // 未连接的可选输入不产生端口
  EXPECT_EQ(2U, dynamic_node.GetInputsSize());
  EXPECT_EQ(2U, dynamic_node.GetOutputsSize());

  // L3：回读端口计数一致。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(1U, loaded->FindNode("reg_fixed")->GetOpDesc()->GetInputsSize());
  EXPECT_EQ(1U, loaded->FindNode("reg_optional")->GetOpDesc()->GetInputsSize());
  EXPECT_EQ(2U, loaded->FindNode("reg_dynamic")->GetOpDesc()->GetInputsSize());
  EXPECT_EQ(2U, loaded->FindNode("reg_dynamic")->GetOpDesc()->GetOutputsSize());
}

// I10：Operator::SetInput/AddControlInput——经 SetInputs 导入，数据边与控制边均入 dump。
TEST_F(EsFacadeDumpIfaceTest, Operator_SetInputAddControlInput_ImportedInDump) {
  Operator src(MakePortOp("imp_src", "Data", 0U, 1U));
  Operator data_dst(MakePortOp("imp_data_dst", "Relu", 1U, 1U));
  Operator ctrl_dst(MakePortOp("imp_ctrl_dst", "PlainOp", 0U, 1U));
  data_dst.SetInput("x0", src, "y0");
  ctrl_dst.AddControlInput(src);

  Graph graph("if_import", kExecGraphKind);
  (void)graph.SetInputs({src});
  ASSERT_EQ(3U, graph.GetAllNodes().size());

  // L1：数据边与控制邻接均导入。
  const auto data_node = graph.FindNodeByName(AscendString("imp_data_dst"));
  ASSERT_NE(nullptr, data_node);
  EXPECT_EQ("imp_src", GNodeNameOf(data_node->GetInDataNodesAndPortIndexs(0).first));
  const auto ctrl_node = graph.FindNodeByName(AscendString("imp_ctrl_dst"));
  ASSERT_NE(nullptr, ctrl_node);
  ASSERT_EQ(1U, ctrl_node->GetInControlNodes().size());

  // L2：数据边引用入 readable。
  EXPECT_NE(std::string::npos, CollectNodeLines(DumpReadable(graph)).at("imp_data_dst").find("%imp_src"));
  // L3：控制边经回读保持。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ((std::set<std::string>{"imp_src"}), NamesOf(loaded->FindNode("imp_ctrl_dst")->GetInControlNodes()));
}

// I11：EsGraphBuilder::CreateInput(具名/带描述) + CreateInputs 批量——产物描述入 dump。
TEST_F(EsFacadeDumpIfaceTest, Es_CreateInputWithDetails_DescInDump) {
  es::EsGraphBuilder builder("if_es_input", kExecGraphKind);
  auto named = builder.CreateInput(0, "named_input", DT_FLOAT16, FORMAT_NCHW, {2, 3});
  ASSERT_NE(nullptr, named.GetCTensorHolder());
  auto batch = builder.CreateInputs(2U, 1U);
  ASSERT_EQ(2U, batch.size());
  auto graph = builder.BuildAndReset({named});
  ASSERT_NE(nullptr, graph);

  // L1：具名输入节点名与描述。
  const auto node = graph->FindNodeByName(AscendString("named_input"));
  ASSERT_NE(nullptr, node);
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  EXPECT_EQ(DT_FLOAT16, desc.GetDataType());

  // L3：回读描述一致、批量输入节点存在。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_named = loaded->FindNode("named_input");
  ASSERT_NE(nullptr, loaded_named);
  EXPECT_EQ((std::vector<int64_t>({2, 3})), loaded_named->GetOpDesc()->GetOutputDesc(0).GetShape().GetDims());
  EXPECT_NE(nullptr, loaded->FindNode("input_1"));
  EXPECT_NE(nullptr, loaded->FindNode("input_2"));
}

// I12：EsGraphBuilder::CreateScalar/CreateVector/CreateConst(类型化)——常量值入 readable。
TEST_F(EsFacadeDumpIfaceTest, Es_CreateScalarVectorConst_ValuesInDump) {
  es::EsGraphBuilder builder("if_es_const", kExecGraphKind);
  auto scalar = builder.CreateScalar(static_cast<int64_t>(9));
  auto vector = builder.CreateVector(std::vector<int64_t>{7, 8});
  auto typed = builder.CreateConst(std::vector<int32_t>{1, 2, 3}, {3});
  ASSERT_NE(nullptr, scalar.GetCTensorHolder());
  ASSERT_NE(nullptr, vector.GetCTensorHolder());
  ASSERT_NE(nullptr, typed.GetCTensorHolder());
  auto graph = builder.BuildAndReset({scalar});
  ASSERT_NE(nullptr, graph);

  // L2：常量值在 readable attrs 段呈现。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("value: [9]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [7 8]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [1 2 3]")) << readable;

  // L3：回读常量节点数（3 常量 + 输入无 + NetOutput）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(3U, CountTypeOf(loaded->GetDirectNode(), "Const"));
}

// I13：EsGraphBuilder::SetOutput 多槽位——回读 NetOutput 数据边按登记顺序。
TEST_F(EsFacadeDumpIfaceTest, Es_SetOutputMultiSlot_NetOutputInputsInDump) {
  es::EsGraphBuilder builder("if_es_out", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  auto in1 = builder.CreateInput(1);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  ASSERT_NE(nullptr, in1.GetCTensorHolder());
  // 槽位 1 先登记、0 后登记：槽位序与登记顺序无关（既有口径），产物按槽位组织。
  ASSERT_EQ(ge::SUCCESS, es::EsGraphBuilder::SetOutput(in1, 1));
  ASSERT_EQ(ge::SUCCESS, es::EsGraphBuilder::SetOutput(in0, 0));
  auto graph = builder.BuildAndReset();
  ASSERT_NE(nullptr, graph);

  // L3：回读 NetOutput 输入按端口序（0→in0、1→in1）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  NodePtr netoutput;
  for (const auto &node : loaded->GetDirectNode()) {
    if (node->GetType() == "NetOutput") {
      netoutput = node;
      break;
    }
  }
  ASSERT_NE(nullptr, netoutput);
  const auto in_names = NamesOf(netoutput->GetInDataNodes());
  ASSERT_EQ(2U, in_names.size());
  EXPECT_NE(in_names.find("input_0"), in_names.end());
  EXPECT_NE(in_names.find("input_1"), in_names.end());
}

// I14：EsGraphBuilder::SetAttr（图属性便捷入口）——经 dump 回环持久化。
TEST_F(EsFacadeDumpIfaceTest, Es_SetGraphAttr_RoundTripInDump) {
  es::EsGraphBuilder builder("if_es_gattr", kExecGraphKind);
  ASSERT_EQ(ge::SUCCESS, builder.SetAttr("es_g_i", static_cast<int64_t>(66)));
  ASSERT_EQ(ge::SUCCESS, builder.SetAttr("es_g_s", "meta"));
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);

  // L1：收尾后图属性可读。
  AttrValue got;
  int64_t int_read = 0;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("es_g_i"), got));
  ASSERT_EQ(GRAPH_SUCCESS, got.GetAttrValue(int_read));
  EXPECT_EQ(66, int_read);

  // L3：回读图属性一致。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  int64_t loaded_int = 0;
  std::string loaded_str;
  ASSERT_TRUE(AttrUtils::GetInt(loaded, "es_g_i", loaded_int));
  EXPECT_EQ(66, loaded_int);
  ASSERT_TRUE(AttrUtils::GetStr(loaded, "es_g_s", loaded_str));
  EXPECT_EQ("meta", loaded_str);
}

// I15：EsTensorHolder::SetAttrForNode/SetAttr(端口)/AddControlEdge——属性与控制边入 dump。
TEST_F(EsFacadeDumpIfaceTest, Holder_AttrsAndControlEdge_InDump) {
  es::EsGraphBuilder builder("if_holder", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  auto in1 = builder.CreateInput(1);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  ASSERT_NE(nullptr, in1.GetCTensorHolder());
  // 节点属性（SetAttrForNode）与端口属性（SetAttr）。
  ASSERT_EQ(ge::SUCCESS, in0.SetAttrForNode("hn", static_cast<int64_t>(5)));
  ASSERT_EQ(ge::SUCCESS, in0.SetAttr("hp", "port_val"));
  // 控制边：in0 生产者 → in1 生产者。
  ASSERT_EQ(ge::SUCCESS, in1.AddControlEdge({in0}));
  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);

  // L1：控制邻接与属性读回。
  const auto node1 = graph->FindNodeByName(AscendString("input_1"));
  ASSERT_NE(nullptr, node1);
  ASSERT_EQ(1U, node1->GetInControlNodes().size());
  const auto node0 = graph->FindNodeByName(AscendString("input_0"));
  ASSERT_NE(nullptr, node0);
  int64_t hn = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node0->GetAttr(AscendString("hn"), hn));
  EXPECT_EQ(5, hn);

  // L3：控制边与节点属性经回读保持。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ((std::set<std::string>{"input_0"}), NamesOf(loaded->FindNode("input_1")->GetInControlNodes()));
  int64_t loaded_hn = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded->FindNode("input_0")->GetOpDesc(), "hn", loaded_hn));
  EXPECT_EQ(5, loaded_hn);
  // 端口属性（SetAttr）经 kTxt 持久化。
  EXPECT_NE(std::string::npos, DumpTxt(*graph).find("hp"));
}

// I16：EsTensorLike::ToTensorHolder——字面量物化为常量节点入 dump。
TEST_F(EsFacadeDumpIfaceTest, TensorLike_MaterializedConst_InDump) {
  es::EsGraphBuilder builder("if_tensorlike", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  const es::EsTensorLike literal(static_cast<int64_t>(3));
  auto materialized = literal.ToTensorHolder(builder.GetCGraphBuilder());
  ASSERT_NE(nullptr, materialized.GetCTensorHolder());
  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);

  // L2：物化常量值入 readable。
  EXPECT_NE(std::string::npos, DumpReadable(*graph).find("value: [3]"));
  // L3：回读含常量节点。
  EXPECT_GE(CountTypeOf(RoundTripFromTxtDump(*graph)->GetDirectNode(), "Const"), 1U);
}

// I17：CompliantNodeBuilder 全套 IR 定义 + Build——端口/属性/描述入 dump 回环。
TEST_F(EsFacadeDumpIfaceTest, Compliant_BuildProduct_InDump) {
  Graph graph("if_compliant", kExecGraphKind);
  AttrValue mode_default;
  (void)mode_default.SetAttrValue(static_cast<int64_t>(9));
  es::CompliantNodeBuilder builder(&graph);
  auto node = builder.OpType("PlainOp")
                  .Name("cp_node")
                  .IrDefInputsV2({{"x0", es::CompliantNodeBuilder::kEsIrInputRequired, ""},
                                  {"dx", es::CompliantNodeBuilder::kEsIrInputDynamic, ""},
                                  {"ox", es::CompliantNodeBuilder::kEsIrInputOptional, ""}})
                  .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""},
                                   {"dz", es::CompliantNodeBuilder::kEsIrOutputDynamic, ""}})
                  .IrDefAttrsV2({{"mode", es::CompliantNodeBuilder::kEsAttrOptional, "Int", mode_default}})
                  .InstanceDynamicInputNum("dx", 3)
                  .InstanceDynamicOutputNum("dz", 2)
                  .InstanceOutputDataType("y", DT_FLOAT16)
                  .InstanceOutputShape("y", {2, 3})
                  .InstanceOutputFormat("y", FORMAT_NCHW)
                  .Build();
  AscendString node_name;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(node_name));

  // L1：动态组实例数与端口计数（未连接可选输入不产生端口：1 必选 + 3 动态 = 4 入；1 + 2 动态 = 3 出）。
  EXPECT_EQ(4U, node.GetInputsSize());
  EXPECT_EQ(3U, node.GetOutputsSize());

  // L3：回读端口/属性/描述一致。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_node = loaded->FindNode("cp_node");
  ASSERT_NE(nullptr, loaded_node);
  EXPECT_EQ(4U, loaded_node->GetOpDesc()->GetInputsSize());
  EXPECT_EQ(3U, loaded_node->GetOpDesc()->GetOutputsSize());
  int64_t loaded_mode = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded_node->GetOpDesc(), "mode", loaded_mode));
  EXPECT_EQ(9, loaded_mode);
  const auto &y_desc = loaded_node->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(DT_FLOAT16, y_desc.GetDataType());
  EXPECT_EQ((std::vector<int64_t>({2, 3})), y_desc.GetShape().GetDims());
  EXPECT_EQ(FORMAT_NCHW, y_desc.GetFormat());
}

// I18：NamedIoNodeBuilder::Build（注册算子）——命名端口产物入 dump。
TEST_F(EsFacadeDumpIfaceTest, NamedIo_BuildProduct_InDump) {
  Graph graph("if_named_io", kExecGraphKind);
  AscendString error;
  NamedIoNodeBuilder builder(graph);
  auto node = builder.Type("EsIfaceDumpOp")
                  .Name("nio_node")
                  .AddInput("x1")
                  .AddInput("x2")
                  .AddOutput("y")
                  .Attr("strides", MakeListIntAttr({1, 2}))
                  .Build(error);
  ASSERT_NE(nullptr, node);

  // L1：命名端口与属性。
  EXPECT_EQ(2U, node->GetInputsSize());
  EXPECT_EQ(1U, node->GetOutputsSize());

  // L3：回读节点类型/名称/端口/属性一致。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_node = loaded->FindNode("nio_node");
  ASSERT_NE(nullptr, loaded_node);
  EXPECT_EQ("EsIfaceDumpOp", loaded_node->GetType());
  EXPECT_EQ(2U, loaded_node->GetOpDesc()->GetInputsSize());
  EXPECT_EQ(1U, loaded_node->GetOpDesc()->GetOutputsSize());
}

// ============================================================================
// 二、不变性：拒绝类接口——调用前后 dump 逐字节一致
// ============================================================================

// I19：Graph 计算图专属入口拒绝（AddOp/SetTargets/SetNeedIteration）——dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Graph_RejectComputeOnly_ReadableUnchanged) {
  Graph graph("if_reject", kExecGraphKind);
  auto node = graph.AddNodeByOp(MakePortOp("base", "PlainOp", 1U, 1U));
  (void)node;
  Operator op("rejected_op", "PlainOp");

  ExpectReadableUnchangedAround(graph, [&graph, &op]() { EXPECT_NE(GRAPH_SUCCESS, graph.AddOp(op)); });
  // SetTargets 返回 Graph& 且不报状态（与 B 套件口径一致），拒绝以 dump 不变承载。
  ExpectReadableUnchangedAround(graph, [&graph, &op]() { (void)graph.SetTargets({op}); });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    graph.SetNeedIteration(true);  // void 入口，拒绝以日志与图状态不变承载
  });
}

// I20：Graph Save/Load 族拒绝（计算图序列化入口）——dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Graph_RejectSaveLoadOverloads_ReadableUnchanged) {
  Graph graph("if_reject_save", kExecGraphKind);
  (void)graph.AddNodeByOp(MakePortOp("base", "PlainOp", 1U, 1U));

  ExpectReadableUnchangedAround(
      graph, [&graph]() { EXPECT_NE(GRAPH_SUCCESS, graph.SaveToFile("./ut_reject_wont_write.bin")); });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    GraphBuffer buffer;
    EXPECT_NE(GRAPH_SUCCESS, graph.SaveToMem(buffer));
  });
  ExpectReadableUnchangedAround(graph,
                                [&graph]() { EXPECT_NE(GRAPH_SUCCESS, graph.LoadFromFile("./ut_reject_no_src.txt")); });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    const GraphBuffer buffer;
    EXPECT_NE(GRAPH_SUCCESS, graph.LoadFromMem(buffer));
  });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    const std::vector<uint8_t> data = {0U};
    EXPECT_NE(GRAPH_SUCCESS, graph.LoadFromSerializedModelArray(data.data(), data.size()));
  });
}

// I21：Graph::SetOutputs 非法项拒绝（越界端口/非本图节点）——原输出表与 dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Graph_SetOutputsInvalidItems_ReadableUnchanged) {
  Graph graph("if_reject_out", kExecGraphKind);
  auto producer = graph.AddNodeByOp(MakePortOp("valid_out", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{producer, 0}}));
  const auto dump_with_output = DumpReadable(graph);

  // 越界端口：整体失败，原输出登记保持。
  ExpectReadableUnchangedAround(graph,
                                [&graph, &producer]() { EXPECT_NE(GRAPH_SUCCESS, graph.SetOutputs({{producer, 5}})); });
  // 非本图节点：整体失败。
  Graph other("if_reject_out_other", kExecGraphKind);
  auto foreign = other.AddNodeByOp(MakePortOp("foreign", "PlainOp", 1U, 1U));
  ExpectReadableUnchangedAround(graph,
                                [&graph, &foreign]() { EXPECT_NE(GRAPH_SUCCESS, graph.SetOutputs({{foreign, 0}})); });
  EXPECT_EQ(dump_with_output, DumpReadable(graph));
}

// I22：执行图构建器 EsCreateVariable 拦截——不建点，dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Es_VariableOnExecute_ReadableUnchanged) {
  es::EsGraphBuilder builder("if_es_var", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  const auto *inner_graph = builder.GetCGraphBuilder()->GetGraph();

  ExpectReadableUnchangedAround(*inner_graph, [&builder]() {
    const auto variable = builder.CreateVariable(0, "var_node");
    EXPECT_EQ(nullptr, variable.GetCTensorHolder());  // 拦截：返回空引用且不建点
  });
}

// I23：构建器重复登记拒绝（重复输入索引/重复输出槽位）——dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Es_DuplicateRegistrations_ReadableUnchanged) {
  es::EsGraphBuilder builder("if_es_dup", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  const auto *inner_graph = builder.GetCGraphBuilder()->GetGraph();

  // 重复输入索引。
  ExpectReadableUnchangedAround(*inner_graph, [&builder]() {
    const auto dup = builder.CreateInput(0);
    EXPECT_EQ(nullptr, dup.GetCTensorHolder());
  });
  // 重复输出槽位。
  ASSERT_EQ(ge::SUCCESS, es::EsGraphBuilder::SetOutput(in0, 0));
  ExpectReadableUnchangedAround(*inner_graph,
                                [&in0]() { EXPECT_NE(ge::SUCCESS, es::EsGraphBuilder::SetOutput(in0, 0)); });
}

// I24：EsTensorHolder 描述设置器在执行图被拦截（kTxt 口径含描述）——dump 不变。
TEST_F(EsFacadeDumpIfaceTest, Holder_DescSettersOnExecute_TxtUnchanged) {
  es::EsGraphBuilder builder("if_es_desc", kExecGraphKind);
  auto in0 = builder.CreateInput(0, "desc_in", DT_FLOAT, FORMAT_ND, {2});
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  const auto *inner_graph = builder.GetCGraphBuilder()->GetGraph();

  // 描述设置器逐一拦截：描述保持默认，kTxt dump（含端口描述）逐字节不变。
  ExpectTxtUnchangedAround(*inner_graph, [&in0]() { (void)in0.SetDataType(DT_FLOAT16); });
  ExpectTxtUnchangedAround(*inner_graph, [&in0]() { (void)in0.SetShape({3, 4}); });
  ExpectTxtUnchangedAround(*inner_graph, [&in0]() {
    const std::vector<const char *> symbol_dims = {"-1"};
    (void)in0.SetOriginSymbolShape(symbol_dims);
  });
}

// ============================================================================
// 三、存活性：生命周期事件后存活对象 dump 结构完整
// ============================================================================

// I25：CopyFrom 副本在源图析构后 dump 结构完整（副本独立语义）。
TEST_F(EsFacadeDumpIfaceTest, Graph_CopyFromSurvivesSourceDestruction_DumpIntact) {
  Graph dst("if_copy_alive", kExecGraphKind);
  (void)dst.SetValid();
  {
    Operator src(MakePortOp("src", "Data", 0U, 1U));
    Operator side(MakePortOp("side", "Data", 0U, 1U));
    Operator mid(MakePortOp("mid", "Add", 2U, 1U));
    Operator dst_op(MakePortOp("dst", "Relu", 1U, 1U));
    mid.SetInput("x0", src, "y0");
    mid.SetInput("x1", side, "y0");
    dst_op.SetInput("x0", mid, "y0");
    Graph src_graph("if_copy_src", kExecGraphKind);
    (void)src_graph.SetInputs({src, side});
    ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src_graph));
  }  // 源图析构。
  // L1：副本节点齐全。
  ASSERT_EQ(4U, dst.GetAllNodes().size());
  // L2：副本 dump 结构完整（4 节点行 + 边引用）。
  const auto lines = CollectNodeLines(DumpReadable(dst));
  ASSERT_EQ(4U, lines.size());
  EXPECT_NE(std::string::npos, lines.at("mid").find("%src"));
  EXPECT_NE(std::string::npos, lines.at("mid").find("%side"));
}

// I26：构建器销毁后移交图 dump 结构完整（BuildAndReset 一次性移交语义）。
TEST_F(EsFacadeDumpIfaceTest, Es_BuilderDestroyed_MigratedGraphDumpIntact) {
  std::unique_ptr<Graph> graph;
  {
    es::EsGraphBuilder builder("if_es_alive", kExecGraphKind);
    auto in0 = builder.CreateInput(0, "alive_in", DT_FLOAT, FORMAT_ND, {2});
    auto in1 = builder.CreateInput(1);
    ASSERT_NE(nullptr, in0.GetCTensorHolder());
    ASSERT_NE(nullptr, in1.GetCTensorHolder());
    const std::vector<int64_t> values = {11, 22};
    auto cst = builder.CreateConst(values, {2});
    ASSERT_NE(nullptr, cst.GetCTensorHolder());
    graph = builder.BuildAndReset({in0});
    ASSERT_NE(nullptr, graph);
  }  // 构建器析构。
  // L1：移交图节点齐全（2 输入 + 1 常量 + NetOutput）。
  EXPECT_GE(graph->GetAllNodes().size(), 4U);
  // L2：dump 结构完整。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("%alive_in")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [11 22]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("return (%alive_in")) << readable;
}

// I27：类型化常量变体（uint64/uint32/float 的 Const/Vector/Scalar）——值入 readable、回读计数一致。
TEST_F(EsFacadeDumpIfaceTest, Es_TypedConstVariants_ValuesInDump) {
  es::EsGraphBuilder builder("if_typed_const", kExecGraphKind);
  auto cu64 = builder.CreateConst(std::vector<uint64_t>{3, 4}, {2});
  auto cu32 = builder.CreateConst(std::vector<uint32_t>{5}, {1});
  auto cf = builder.CreateConst(std::vector<float>{1.5f, 2.5f}, {2});
  auto vu64 = builder.CreateVector(std::vector<uint64_t>{7});
  auto sf = builder.CreateScalar(2.5f);
  ASSERT_NE(nullptr, cu64.GetCTensorHolder());
  ASSERT_NE(nullptr, cu32.GetCTensorHolder());
  ASSERT_NE(nullptr, cf.GetCTensorHolder());
  ASSERT_NE(nullptr, vu64.GetCTensorHolder());
  ASSERT_NE(nullptr, sf.GetCTensorHolder());
  auto graph = builder.BuildAndReset({cu64});
  ASSERT_NE(nullptr, graph);

  // L2：各类型常量值入 readable attrs 段。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("value: [3 4]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [5]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [7]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("2.5")) << readable;

  // L3：回读常量节点计数（5 常量 + NetOutput）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(5U, CountTypeOf(loaded->GetDirectNode(), "Const"));
}

// I28：CreateConst 自由模板（uint8 字节载荷，A31/A32）与 CreateConstV2（ABI 安全）——值入 dump。
TEST_F(EsFacadeDumpIfaceTest, Es_GenericBytesConstAndV2_InDump) {
  es::EsGraphBuilder builder("if_bytes_const", kExecGraphKind);
  const auto bytes = builder.CreateConst(std::vector<uint8_t>{1, 2, 3, 4}, {4}, DT_UINT8, FORMAT_ND);
  const auto v2 = builder.CreateConstV2(std::vector<int64_t>{8, 9}, {2}, DT_INT64, FORMAT_ND);
  ASSERT_NE(nullptr, bytes.GetCTensorHolder());
  ASSERT_NE(nullptr, v2.GetCTensorHolder());
  auto graph = builder.BuildAndReset({bytes});
  ASSERT_NE(nullptr, graph);

  // L2：字节载荷与 V2 常量值入 readable。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("value: [1 2 3 4]")) << readable;
  EXPECT_NE(std::string::npos, readable.find("value: [8 9]")) << readable;

  // L3：回读常量节点数一致。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(2U, CountTypeOf(loaded->GetDirectNode(), "Const"));
}

// I29：ES 输入节点阶段属性（A37：gert::attr::kStage 加载期绑定语义）——经 dump 回环持久化。
TEST_F(EsFacadeDumpIfaceTest, Es_StageAttribute_PersistInDump) {
  es::EsGraphBuilder builder("if_stage", kExecGraphKind);
  auto load = builder.CreateInput(0, "load_input");
  auto feed = builder.CreateInput(1, "feed_input");
  ASSERT_NE(nullptr, load.GetCTensorHolder());
  ASSERT_NE(nullptr, feed.GetCTensorHolder());
  GNode *load_producer = load.GetProducer();
  ASSERT_NE(nullptr, load_producer);
  GNode *feed_producer = feed.GetProducer();
  ASSERT_NE(nullptr, feed_producer);
  AscendString stage_name(gert::attr::kStage);
  AscendString stage_init(gert::attr::kStageInit);
  AscendString stage_main(gert::attr::kStageMain);
  ASSERT_EQ(GRAPH_SUCCESS, load_producer->SetAttr(stage_name, stage_init));
  ASSERT_EQ(GRAPH_SUCCESS, feed_producer->SetAttr(stage_name, stage_main));
  auto graph = builder.BuildAndReset({load});
  ASSERT_NE(nullptr, graph);

  // L1：读回一致。
  AscendString stage_read;
  ASSERT_EQ(GRAPH_SUCCESS, load_producer->GetAttr(AscendString(gert::attr::kStage), stage_read));
  EXPECT_EQ(AscendString(gert::attr::kStageInit), stage_read);

  // L3：回读阶段属性持久化（readable 对 gert 内部属性按过滤口径渲染，阶段语义以回环承载）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  std::string loaded_stage;
  ASSERT_TRUE(AttrUtils::GetStr(loaded->FindNode("load_input")->GetOpDesc(), gert::attr::kStage, loaded_stage));
  EXPECT_EQ(gert::attr::kStageInit, loaded_stage);
  ASSERT_TRUE(AttrUtils::GetStr(loaded->FindNode("feed_input")->GetOpDesc(), gert::attr::kStage, loaded_stage));
  EXPECT_EQ(gert::attr::kStageMain, loaded_stage);
}

// I30：EsCGraphBuilder 底层三入口（AppendGraphInput / GetTensorHolderFromNode /
// CreateDynamicTensorHolderFromNode）——产物与属性入 dump。
TEST_F(EsFacadeDumpIfaceTest, EsCGraph_InputAppendAndHolders_InDump) {
  es::EsGraphBuilder builder("if_cgb", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  auto *cgb = builder.GetCGraphBuilder();
  ASSERT_NE(nullptr, cgb);

  // AppendGraphInput：末尾新建输入节点（全局索引 1）。
  auto *appended = cgb->AppendGraphInput("appended_in");
  ASSERT_NE(nullptr, appended);
  // GetTensorHolderFromNode：对非 Data 类节点输出建引用（Data 类节点被拒为既有口径），经引用设节点属性。
  auto *inner_graph = cgb->GetGraph();
  ASSERT_NE(nullptr, inner_graph);
  auto multi = inner_graph->AddNodeByOp(MakePortOp("multi_out", "PlainOp", 0U, 3U));
  auto *cth = cgb->GetTensorHolderFromNode(multi, 0);
  ASSERT_NE(nullptr, cth);
  es::EsTensorHolder holder(cth);
  ASSERT_EQ(ge::SUCCESS, holder.SetAttrForNode("cgb_hn", static_cast<int64_t>(31)));
  // CreateDynamicTensorHolderFromNode：3 输出节点取第 1~2 号引用。
  auto *holders = cgb->CreateDynamicTensorHolderFromNode(multi, 1, 2);
  ASSERT_NE(nullptr, holders);
  ASSERT_EQ(2U, holders->size());

  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);
  // L2：追加输入节点入 readable。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("%appended_in")) << readable;
  // L3：回读追加输入节点与引用所设属性。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_NE(nullptr, loaded->FindNode("appended_in"));
  EXPECT_NE(nullptr, loaded->FindNode("multi_out"));
  int64_t loaded_hn = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded->FindNode("multi_out")->GetOpDesc(), "cgb_hn", loaded_hn));
  EXPECT_EQ(31, loaded_hn);
}

// I31：es::AddEdgeAndUpdatePeerDesc（Compliant 辅助）——连边入 dump 且对端输入描述按源输出更新。
TEST_F(EsFacadeDumpIfaceTest, Compliant_AddEdgeAndUpdatePeerDesc_InDump) {
  Graph graph("if_peer_desc", kExecGraphKind);
  es::CompliantNodeBuilder src_builder(&graph);
  auto src_node = src_builder.OpType("PlainOp")
                      .Name("peer_src")
                      .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                      .InstanceOutputDataType("y", DT_FLOAT16)
                      .InstanceOutputShape("y", {3, 4})
                      .Build();
  es::CompliantNodeBuilder dst_builder(&graph);
  auto dst_node = dst_builder.OpType("PlainOp")
                      .Name("peer_dst")
                      .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                      .IrDefOutputsV2({{"r", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                      .Build();
  ASSERT_EQ(GRAPH_SUCCESS, es::AddEdgeAndUpdatePeerDesc(graph, src_node, 0, dst_node, 0));

  // L1：连边建立；对端描述未按源输出更新（P1 已知名实不符，覆盖明细 §四"另"行；
  // 现状由用例承载，规格对齐后翻转断言）。
  TensorDesc peer_input;
  ASSERT_EQ(GRAPH_SUCCESS, dst_node.GetInputDesc(0, peer_input));
  EXPECT_EQ(DT_FLOAT, peer_input.GetDataType());

  // L2：readable 中 dst 行引用 src。
  EXPECT_NE(std::string::npos, CollectNodeLines(DumpReadable(graph)).at("peer_dst").find("%peer_src"));
  // L3：回读边存在；对端描述保持默认（P1 现状与 L1 口径一致）。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ((std::set<std::string>{"peer_src"}), NamesOf(loaded->FindNode("peer_dst")->GetInDataNodes()));
  const auto &peer_in = loaded->FindNode("peer_dst")->GetOpDesc()->GetInputDesc(0);
  EXPECT_EQ(DT_FLOAT, peer_in.GetDataType());
}

// I32：Graph::SetOutputs 索引重载（pair<Operator, vector<size_t>>）与具名重载（string/AscendString）
// ——各重载经 NetOutput 承接，dump 一致（I01/I02 已覆盖 GNode 与 Operator 整体重载）。
TEST_F(EsFacadeDumpIfaceTest, Graph_SetOutputsIndexAndNamedOverloads_NetOutputInDump) {
  Graph graph("if_set_outputs_more", kExecGraphKind);
  Operator producer = MakePortOp("more_out", "PlainOp", 1U, 1U);
  (void)graph.AddNodeByOp(producer);

  // 索引重载：按输出实例索引登记。
  (void)graph.SetOutputs({{producer, std::vector<size_t>{0U}}});
  // 具名重载：按输出实例名解析端口（string / AscendString 两种形态）。
  (void)graph.SetOutputs({{producer, std::string("y0")}});
  (void)graph.SetOutputs({{producer, AscendString("y0")}});

  // L1：NetOutput 承接且不重复建点。
  EXPECT_GE(graph.GetAllNodes().size(), 2U);
  // L2：readable 渲染图输出段。
  EXPECT_NE(std::string::npos, DumpReadable(graph).find("return (%more_out"));

  // L3：回读 NetOutput 唯一、输入为该节点。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(1U, CountTypeOf(loaded->GetDirectNode(), "NetOutput"));
  ge::NodePtr netoutput;
  for (const auto &node : loaded->GetDirectNode()) {
    if (node->GetType() == "NetOutput") {
      netoutput = node;
      break;
    }
  }
  ASSERT_NE(nullptr, netoutput);
  EXPECT_EQ((std::set<std::string>{"more_out"}), NamesOf(netoutput->GetInDataNodes()));
}

// I33：Graph::FindOpByName/FindOpByType/GetAllOpName 拒绝（计算图 Operator 登记族）——dump 不变；
// 附默认构造空句柄 Dump 负向。
TEST_F(EsFacadeDumpIfaceTest, Graph_RejectFindOpFamily_ReadableUnchanged) {
  Graph graph("if_reject_findop", kExecGraphKind);
  (void)graph.AddNodeByOp(MakePortOp("base", "PlainOp", 1U, 1U));

  ExpectReadableUnchangedAround(graph, [&graph]() {
    ge::Operator got;
    EXPECT_NE(GRAPH_SUCCESS, graph.FindOpByName("base", got));
  });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    std::vector<ge::Operator> ops;
    EXPECT_NE(GRAPH_SUCCESS, graph.FindOpByType("PlainOp", ops));
  });
  ExpectReadableUnchangedAround(graph, [&graph]() {
    std::vector<std::string> names;
    EXPECT_NE(GRAPH_SUCCESS, graph.GetAllOpName(names));
  });

  // 默认构造空句柄：无后端，Dump 拒绝。
  Graph empty;
  std::stringstream ss;
  EXPECT_NE(GRAPH_SUCCESS, empty.Dump(Graph::DumpFormat::kReadable, ss));
}

// I34：Operator 注册族携带（AttrRegister 默认值/RequiredAttrWithTypeRegister/SubgraphRegister 槽位/
// Operator::SetAttr 实例属性/UpdateOutputDesc(name)）——入图后 dump 证据。
TEST_F(EsFacadeDumpIfaceTest, Operator_RegistrationCarried_InDump) {
  Graph graph("if_op_reg", kExecGraphKind);
  Operator op("reg_carry", "PlainOp");
  op.InputRegister("x", "");
  op.OutputRegister("y", "");
  AttrValue def_v;
  (void)def_v.SetAttrValue(static_cast<int64_t>(55));
  op.AttrRegister("def_i", def_v);
  op.RequiredAttrWithTypeRegister("req_i", "Int");
  op.SubgraphRegister("sg", false);
  int64_t inst_attr = 77;
  (void)op.SetAttr("inst_i", inst_attr);
  const TensorDesc out_desc(Shape({7}), FORMAT_ND, DT_INT64);
  (void)op.UpdateOutputDesc("y", out_desc);
  auto node = graph.AddNodeByOp(op);
  // SubgraphRegister 槽位挂载子图。
  Graph sub("carry_branch", kExecGraphKind);
  (void)sub.AddNodeByOp(MakePortOp("carry_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetSubgraph(AscendString("sg"), sub));

  // L1：属性默认值与实例属性入图可读。
  int64_t def_read = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("def_i"), def_read));
  EXPECT_EQ(55, def_read);
  int64_t inst_read = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("inst_i"), inst_read));
  EXPECT_EQ(77, inst_read);

  // L2：readable 含子图块。
  const auto readable = DumpReadable(graph);
  EXPECT_NE(std::string::npos, readable.find("graph(\"carry_branch\")")) << readable;

  // L3：回读属性/子图/按名更新的输出描述。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_node = loaded->FindNode("reg_carry");
  ASSERT_NE(nullptr, loaded_node);
  int64_t loaded_def = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded_node->GetOpDesc(), "def_i", loaded_def));
  EXPECT_EQ(55, loaded_def);
  int64_t loaded_inst = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded_node->GetOpDesc(), "inst_i", loaded_inst));
  EXPECT_EQ(77, loaded_inst);
  EXPECT_NE(nullptr, loaded->GetSubgraph("carry_branch"));
  EXPECT_EQ((std::vector<int64_t>({7})), loaded_node->GetOpDesc()->GetOutputDesc(0).GetShape().GetDims());
  EXPECT_EQ(DT_INT64, loaded_node->GetOpDesc()->GetOutputDesc(0).GetDataType());
}

// I35：常量→消费节点构图语义闭环（GetInputConstData 读回）+ UINT8 Tensor 字节属性（A22）dump 持久化。
TEST_F(EsFacadeDumpIfaceTest, ConstToConsumerSemantics_InDump) {
  es::EsGraphBuilder builder("if_const_flow", kExecGraphKind);
  const std::vector<int64_t> values = {11, 22};
  auto cst = builder.CreateConst(values, {2});
  ASSERT_NE(nullptr, cst.GetCTensorHolder());
  auto *inner = builder.GetCGraphBuilder()->GetGraph();
  ASSERT_NE(nullptr, inner);
  auto user = inner->AddNodeByOp(MakePortOp("flow_user", "Relu", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, inner->AddDataEdge(*cst.GetProducer(), cst.GetProducerOutIndex(), user, 0));
  // UINT8 Tensor 字节载荷挂常量节点属性（A22）。
  Tensor bytes_tensor(TensorDesc(Shape({4}), FORMAT_ND, DT_UINT8));
  (void)bytes_tensor.SetData(std::vector<uint8_t>{9, 8, 7, 6});
  AttrValue bytes_av;
  (void)bytes_av.SetAttrValue(bytes_tensor);
  GNode *const_producer = cst.GetProducer();
  ASSERT_NE(nullptr, const_producer);
  ASSERT_EQ(GRAPH_SUCCESS, const_producer->SetAttr(AscendString("node_bytes"), bytes_av));
  auto graph = builder.BuildAndReset({cst});
  ASSERT_NE(nullptr, graph);

  // L1：消费节点 GetInputConstData 读回常量数据。
  Tensor read;
  ASSERT_EQ(GRAPH_SUCCESS, user.GetInputConstData(0, read));
  const auto *read_data = reinterpret_cast<const int64_t *>(read.GetData());
  ASSERT_NE(nullptr, read_data);
  EXPECT_EQ(11, read_data[0]);
  EXPECT_EQ(22, read_data[1]);

  // L2：常量值与边关系入 readable。
  const auto readable = DumpReadable(*graph);
  EXPECT_NE(std::string::npos, readable.find("value: [11 22]")) << readable;
  EXPECT_NE(std::string::npos, CollectNodeLines(readable).at("flow_user").find("%Const_0"));

  // L3：回读字节属性持久化（dims/首字节）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  ConstGeTensorPtr loaded_bytes;
  ASSERT_TRUE(AttrUtils::GetTensor(loaded->FindNode("Const_0")->GetOpDesc(), "node_bytes", loaded_bytes));
  ASSERT_NE(nullptr, loaded_bytes);
  EXPECT_EQ((std::vector<int64_t>({4})), loaded_bytes->GetTensorDesc().GetShape().GetDims());
  EXPECT_EQ(4U, loaded_bytes->GetData().GetSize());
  EXPECT_EQ(9, loaded_bytes->GetData()[0]);
}

// I36：描述设置器剩余变体拦截（Holder SetFormat + EsCTensorHolder C 侧描述族）——kTxt 不变。
TEST_F(EsFacadeDumpIfaceTest, Holder_DescSettersRemainingVariants_TxtUnchanged) {
  es::EsGraphBuilder builder("if_desc_rest", kExecGraphKind);
  auto in0 = builder.CreateInput(0, "desc_in2", DT_FLOAT, FORMAT_ND, {2});
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  const auto *inner_graph = builder.GetCGraphBuilder()->GetGraph();
  ASSERT_NE(nullptr, inner_graph);

  // Holder SetFormat（I24 遗漏变体）。
  ExpectTxtUnchangedAround(*inner_graph, [&in0]() { (void)in0.SetFormat(FORMAT_NCHW); });
  // C 侧 EsCTensorHolder 描述族（执行图统一拦截，含 I24 未覆盖的 Origin/Storage 变体）。
  auto *cth = in0.GetCTensorHolder();
  ASSERT_NE(nullptr, cth);
  ExpectTxtUnchangedAround(*inner_graph, [&cth]() {
    (void)cth->SetOriginFormat(FORMAT_NCHW);
    (void)cth->SetStorageFormat(FORMAT_NHWC);
  });
  ExpectTxtUnchangedAround(*inner_graph, [&cth]() {
    (void)cth->SetOriginShape(ge::Shape({3}));
    (void)cth->SetStorageShape(ge::Shape({4}));
  });
}

// I37：EsGraphBuilder::SetAttr bool 变体（图属性三型便捷入口收口）——回环一致。
TEST_F(EsFacadeDumpIfaceTest, Es_SetGraphAttrBool_RoundTripInDump) {
  es::EsGraphBuilder builder("if_bool_attr", kExecGraphKind);
  ASSERT_EQ(ge::SUCCESS, builder.SetAttr("es_g_b", true));
  auto in0 = builder.CreateInput(0);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);

  // L1：读回一致。
  AttrValue got;
  bool bool_read = false;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("es_g_b"), got));
  ASSERT_EQ(GRAPH_SUCCESS, got.GetAttrValue(bool_read));
  EXPECT_TRUE(bool_read);

  // L3：回读一致。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  bool loaded_bool = false;
  ASSERT_TRUE(AttrUtils::GetBool(loaded, "es_g_b", loaded_bool));
  EXPECT_TRUE(loaded_bool);
}

// I38：NamedIoNodeBuilder::AddInput(name, desc)/AddOutput(name, desc) 带描述形态——描述经 dump 回读。
TEST_F(EsFacadeDumpIfaceTest, NamedIo_AddWithDesc_InDump) {
  Graph graph("if_nio_desc", kExecGraphKind);
  AscendString error;
  const TensorDesc in_desc(Shape({5}), FORMAT_ND, DT_INT32);
  const TensorDesc out_desc(Shape({6}), FORMAT_ND, DT_INT64);
  NamedIoNodeBuilder builder(graph);
  auto node = builder.Type("EsIfaceDumpOp")
                  .Name("nio_desc_node")
                  .AddInput("x1", in_desc)
                  .AddInput("x2", in_desc)
                  .AddOutput("y", out_desc)
                  .Attr("strides", MakeListIntAttr({1}))
                  .Build(error);
  ASSERT_NE(nullptr, node);

  // L1：带描述输入读回。
  TensorDesc got_in;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetInputDesc(0, got_in));
  EXPECT_EQ((std::vector<int64_t>({5})), got_in.GetShape().GetDims());

  // L3：回读描述一致。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_node = loaded->FindNode("nio_desc_node");
  ASSERT_NE(nullptr, loaded_node);
  EXPECT_EQ((std::vector<int64_t>({5})), loaded_node->GetOpDesc()->GetInputDesc(0).GetShape().GetDims());
  EXPECT_EQ(DT_INT32, loaded_node->GetOpDesc()->GetInputDesc(0).GetDataType());
  EXPECT_EQ((std::vector<int64_t>({6})), loaded_node->GetOpDesc()->GetOutputDesc(0).GetShape().GetDims());
  EXPECT_EQ(DT_INT64, loaded_node->GetOpDesc()->GetOutputDesc(0).GetDataType());
}
