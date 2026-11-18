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
 * ES/Graph 执行图接口 dump 验证用例（对齐《ES_Graph构图接口dump验证覆盖方案》）。
 * 三层验证模型：
 *   L1 真值——门面 API 直查构图结构；
 *   L2 文本断言——Dump(kReadable) 节点行与构图语义对照（锚定 %name/#users/Node[type]/inputs）；
 *   L3 回环断言——Dump(kTxt) → ModelDef → Model::Load 回读 ComputeGraph 拓扑再断言。
 * dump 为 adapter 只读快照：OutNodeInfo/target 有损区不做 dump 断言（由 L1 承载）；
 * readable 不渲染控制边，控制边语义由 L1/L3 断言承载。
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
#include "graph/gnode.h"
#include "graph/model.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/operator.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "es_graph_builder.h"
#include "common/es_dump_check_utils.h"

using namespace ge;
using namespace ge::ut::esdump;

namespace {
constexpr GraphKind kComputeGraphKind = C_GRAPH_COMPUTE;
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

// 导入用 4 节点链：src/side → mid → dst（SetInputs 导入路径）。
struct OperatorChain {
  Operator src;
  Operator side;
  Operator mid;
  Operator dst;
  OperatorChain()
      : src(MakePortOp("src", "Data", 0U, 1U)),
        side(MakePortOp("side", "Data", 0U, 1U)),
        mid(MakePortOp("mid", "Add", 2U, 1U)),
        dst(MakePortOp("dst", "Relu", 1U, 1U)) {
    mid.SetInput("x0", src, "y0");
    mid.SetInput("x1", side, "y0");
    dst.SetInput("x0", mid, "y0");
  }
};
}  // namespace

class EsFacadeDumpTest : public testing::Test {};

// D1：SetInputs 导入链——readable 节点行/inputs 引用/#users 与构图语义一致。
TEST_F(EsFacadeDumpTest, ImportedChainReadableTopology) {
  OperatorChain chain;
  Graph graph("dump_chain", kExecGraphKind);
  (void)graph.SetInputs({chain.src, chain.side});

  // L1：4 节点、边关系正确。
  ASSERT_EQ(4U, graph.GetAllNodes().size());
  const auto mid = graph.FindNodeByName(AscendString("mid"));
  ASSERT_NE(nullptr, mid);
  ASSERT_NE(nullptr, mid->GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ("src", GNodeNameOf(mid->GetInDataNodesAndPortIndexs(0).first));
  EXPECT_EQ("side", GNodeNameOf(mid->GetInDataNodesAndPortIndexs(1).first));

  // L2：节点行逐项对照构图代码。
  const auto lines = CollectNodeLines(DumpReadable(graph));
  ASSERT_EQ(4U, lines.size());
  EXPECT_NE(std::string::npos, lines.at("src").find("Node[type=Data]"));
  EXPECT_NE(std::string::npos, lines.at("mid").find("Node[type=Add]"));
  EXPECT_NE(std::string::npos, lines.at("dst").find("Node[type=Relu]"));
  // 边语义：mid 行 inputs 引用 src/side，dst 行引用 mid；src/side/mid 各 1 个使用者。
  EXPECT_NE(std::string::npos, lines.at("mid").find("%src"));
  EXPECT_NE(std::string::npos, lines.at("mid").find("%side"));
  EXPECT_NE(std::string::npos, lines.at("dst").find("%mid"));
  EXPECT_NE(std::string::npos, lines.at("src").find("[#users=1]"));
  EXPECT_NE(std::string::npos, lines.at("side").find("[#users=1]"));
  EXPECT_NE(std::string::npos, lines.at("mid").find("[#users=1]"));
}

// D2：SetInputs 导入链——kTxt 回环（ModelDef → Model::Load）拓扑一致。
TEST_F(EsFacadeDumpTest, ImportedChainRoundTripProto) {
  OperatorChain chain;
  Graph graph("dump_chain_rt", kExecGraphKind);
  (void)graph.SetInputs({chain.src, chain.side});

  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  // 快照语义：转换经 SetGraphOutNodesInfo 触发 NetOutput 补建（与计算图登记口径一致，仅作用于快照），
  // 回读节点数 = 原图 4 + 1 个 NetOutput。
  ASSERT_EQ(5U, loaded->GetDirectNodesSize());
  EXPECT_EQ(1U, CountTypeOf(loaded->GetDirectNode(), "NetOutput"));
  const auto mid = loaded->FindNode("mid");
  ASSERT_NE(nullptr, mid);
  EXPECT_EQ(2U, mid->GetInDataNodes().size());
  EXPECT_EQ((std::set<std::string>{"src", "side"}), NamesOf(mid->GetInDataNodes()));
  const auto dst = loaded->FindNode("dst");
  ASSERT_NE(nullptr, dst);
  EXPECT_EQ((std::set<std::string>{"mid"}), NamesOf(dst->GetInDataNodes()));
  EXPECT_EQ((std::set<std::string>{"mid"}), NamesOf(loaded->FindNode("src")->GetOutDataNodes()));
}

// D3：AddControlEdge——readable 含两端节点行（不渲染控制边），控制边语义经 L1/L3 回读一致。
TEST_F(EsFacadeDumpTest, ControlEdgeReadableAndRoundTrip) {
  Graph graph("dump_ctrl", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("ctrl_src", "Data", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakePortOp("ctrl_dst", "Relu", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(src, dst));

  // L1：控制邻接查询。
  ASSERT_EQ(1U, dst.GetInControlNodes().size());
  EXPECT_EQ("ctrl_src", GNodeNameOf(dst.GetInControlNodes()[0]));

  // L2：两端节点行均在（控制边本身不进入 readable，见文件头说明）。
  const auto lines = CollectNodeLines(DumpReadable(graph));
  ASSERT_EQ(2U, lines.size());
  EXPECT_EQ(1U, lines.count("ctrl_src"));
  EXPECT_EQ(1U, lines.count("ctrl_dst"));

  // L3：控制边经 proto 回读保持。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  const auto loaded_dst = loaded->FindNode("ctrl_dst");
  ASSERT_NE(nullptr, loaded_dst);
  EXPECT_EQ((std::set<std::string>{"ctrl_src"}), NamesOf(loaded_dst->GetInControlNodes()));
}

// D4：SetSubgraph 两级嵌套——readable 逐图块呈现，回读子图注册表一致。
TEST_F(EsFacadeDumpTest, SubgraphMountedReadableBlocks) {
  Graph parent("dump_sub", kExecGraphKind);
  auto holder = parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph branch("branch", kExecGraphKind);
  auto branch_holder = branch.AddNodeByOp(MakePortOp("branch_holder", "If", 1U, 1U));
  (void)branch.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  Graph leaf("leaf", kExecGraphKind);
  (void)leaf.AddNodeByOp(MakePortOp("leaf_node", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("branch"), branch));
  ASSERT_EQ(GRAPH_SUCCESS, branch_holder.SetSubgraph(AscendString("leaf"), leaf));

  // L1：父图经注册表按名取子图。
  ASSERT_NE(nullptr, parent.GetSubGraph("branch"));

  // L2：readable 含父/子/孙三级图块与各层节点行。
  const auto readable = DumpReadable(parent);
  EXPECT_NE(std::string::npos, readable.find("graph(\"dump_sub\")")) << readable;
  EXPECT_NE(std::string::npos, readable.find("graph(\"branch\")")) << readable;
  EXPECT_NE(std::string::npos, readable.find("graph(\"leaf\")")) << readable;
  const auto lines = CollectNodeLines(readable);
  EXPECT_EQ(1U, lines.count("holder"));
  EXPECT_EQ(1U, lines.count("branch_holder"));
  EXPECT_EQ(1U, lines.count("inner"));
  EXPECT_EQ(1U, lines.count("leaf_node"));

  // L3：回读子图树完整（两级）。
  const auto loaded = RoundTripFromTxtDump(parent);
  ASSERT_NE(nullptr, loaded);
  ASSERT_EQ(2U, loaded->GetAllSubgraphs().size());
  const auto loaded_branch = loaded->GetSubgraph("branch");
  ASSERT_NE(nullptr, loaded_branch);
  EXPECT_NE(nullptr, loaded_branch->FindNode("inner"));
  const auto loaded_leaf = loaded->GetSubgraph("leaf");
  ASSERT_NE(nullptr, loaded_leaf);
  EXPECT_NE(nullptr, loaded_leaf->FindNode("leaf_node"));
}

// D5：图属性/节点属性——设置后经 kTxt 回环持久化。
TEST_F(EsFacadeDumpTest, GraphAndNodeAttrsSurviveRoundTrip) {
  Graph graph("dump_attr", kExecGraphKind);
  (void)graph.SetValid();
  AttrValue int_v;
  (void)int_v.SetAttrValue(static_cast<int64_t>(42));
  AttrValue str_v;
  (void)str_v.SetAttrValue(AscendString("meta"));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString("g_int"), int_v));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString("g_str"), str_v));
  auto node = graph.AddNodeByOp(MakePortOp("attr_node", "PlainOp", 1U, 1U));
  int64_t node_attr = 7;
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("n_int"), node_attr));

  // L1：读回。
  AttrValue out;
  int64_t got_int = 0;
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString("g_int"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(got_int));
  EXPECT_EQ(42, got_int);
  int64_t node_int = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("n_int"), node_int));
  EXPECT_EQ(7, node_int);

  // L3：回读图属性与节点属性。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  int64_t loaded_g_int = 0;
  std::string loaded_g_str;
  ASSERT_TRUE(AttrUtils::GetInt(loaded, "g_int", loaded_g_int));
  EXPECT_EQ(42, loaded_g_int);
  ASSERT_TRUE(AttrUtils::GetStr(loaded, "g_str", loaded_g_str));
  EXPECT_EQ("meta", loaded_g_str);
  const auto loaded_node = loaded->FindNode("attr_node");
  ASSERT_NE(nullptr, loaded_node);
  int64_t loaded_n_int = 0;
  ASSERT_TRUE(AttrUtils::GetInt(loaded_node->GetOpDesc(), "n_int", loaded_n_int));
  EXPECT_EQ(7, loaded_n_int);
}

// D6：RemoveNode——删中间节点后 dump 反映剩余拓扑。
TEST_F(EsFacadeDumpTest, RemoveNodeThenDumpReflectsTopology) {
  Graph graph("dump_rm", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("rm_src", "Data", 0U, 1U));
  auto mid = graph.AddNodeByOp(MakePortOp("rm_mid", "Add", 2U, 1U));
  auto dst = graph.AddNodeByOp(MakePortOp("rm_dst", "Relu", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, mid, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(mid, 0, dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(mid));

  // L1：剩 2 节点、dst 输入悬空。
  ASSERT_EQ(2U, graph.GetAllNodes().size());
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);

  // L2：readable 不再含 rm_mid 节点行与对它的引用。
  const auto readable = DumpReadable(graph);
  EXPECT_EQ(std::string::npos, readable.find("rm_mid")) << readable;
  const auto lines = CollectNodeLines(readable);
  ASSERT_EQ(2U, lines.size());
  EXPECT_EQ(1U, lines.count("rm_src"));
  EXPECT_EQ(1U, lines.count("rm_dst"));

  // L3：回读 2 节点 + 快照补建的 NetOutput。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  ASSERT_EQ(3U, loaded->GetDirectNodesSize());
  EXPECT_EQ(1U, CountTypeOf(loaded->GetDirectNode(), "NetOutput"));
  EXPECT_EQ(nullptr, loaded->FindNode("rm_mid"));
}

// D7：CopyFrom——副本与源图 readable 节点行一致（结构等价的 dump 证据）。
TEST_F(EsFacadeDumpTest, CopyFromProducesEquivalentDump) {
  OperatorChain chain;
  Graph src_graph("dump_copy", kExecGraphKind);
  (void)src_graph.SetInputs({chain.src, chain.side});
  Graph dst_graph("dump_copy", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, dst_graph.SetValid());
  ASSERT_EQ(GRAPH_SUCCESS, dst_graph.CopyFrom(src_graph));

  // L1：副本结构与源一致。
  ASSERT_EQ(src_graph.GetAllNodes().size(), dst_graph.GetAllNodes().size());

  // L2：两图 dump 节点行逐行一致（独立锚点：副本与源语义等价）。
  const auto src_lines = CollectNodeLines(DumpReadable(src_graph));
  const auto dst_lines = CollectNodeLines(DumpReadable(dst_graph));
  ASSERT_EQ(4U, src_lines.size());
  EXPECT_EQ(src_lines, dst_lines);
}

// D8：ES 构建器全流程（执行类别）——CreateInput/CreateConst/SetOutput/BuildAndReset 产物 dump 验证。
TEST_F(EsFacadeDumpTest, EsBuilderExecutePipelineDump) {
  es::EsGraphBuilder builder("dump_es", kExecGraphKind);
  auto in0 = builder.CreateInput(0);
  auto in1 = builder.CreateInput(1);
  ASSERT_NE(nullptr, in0.GetCTensorHolder());
  ASSERT_NE(nullptr, in1.GetCTensorHolder());
  const std::vector<int64_t> values = {11, 22};
  auto cst = builder.CreateConst(values, {2});
  ASSERT_NE(nullptr, cst.GetCTensorHolder());
  auto graph = builder.BuildAndReset({in0});
  ASSERT_NE(nullptr, graph);

  // L1：产物为执行图（2 输入 + 1 常量 + NetOutput 汇聚）。
  ASSERT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
  EXPECT_GE(graph->GetAllNodes().size(), 4U);

  // L2：readable 含数据输入与常量节点行（按 type 锚定，NetOutput 渲染为图输出段）。
  const auto lines = CollectNodeLines(DumpReadable(*graph));
  const auto count_type = [&lines](const std::string &type) {
    return std::count_if(lines.begin(), lines.end(),
                         [&type](const auto &kv) { return kv.second.find(type) != std::string::npos; });
  };
  EXPECT_EQ(2U, count_type("Node[type=Data]"));
  EXPECT_EQ(1U, count_type("Node[type=Const]"));

  // L3：回读节点数与源一致（含 NetOutput）。
  const auto loaded = RoundTripFromTxtDump(*graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(graph->GetAllNodes().size(), loaded->GetDirectNodesSize());
}

// D9：空图——SetValid 后可 dump（仅图头），构造期未初始化与计算图口径一致拒绝。
TEST_F(EsFacadeDumpTest, EmptyExecuteGraphDump) {
  Graph graph("dump_empty", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());

  // L2：仅图头、无节点行。
  const auto readable = DumpReadable(graph);
  EXPECT_NE(std::string::npos, readable.find("graph(\"dump_empty\")")) << readable;
  EXPECT_TRUE(CollectNodeLines(readable).empty());

  // L3：回读 0 节点。
  const auto loaded = RoundTripFromTxtDump(graph);
  ASSERT_NE(nullptr, loaded);
  EXPECT_EQ(0U, loaded->GetDirectNodesSize());

  // 构造期未初始化：与计算图 SetValid 前口径一致拒绝 dump。
  Graph uninit("dump_uninit", kExecGraphKind);
  std::stringstream ss;
  EXPECT_NE(GRAPH_SUCCESS, uninit.Dump(Graph::DumpFormat::kReadable, ss));
}

// D10：跨模式等价（独立锚点）——同一构图意图双模式产物 readable 节点行一致。
TEST_F(EsFacadeDumpTest, CrossModeDumpEquivalence) {
  OperatorChain compute_chain;
  Graph compute_graph("dump_equiv", kComputeGraphKind);
  // 计算图 SetInputs 需在 SetValid 前导入（导入时建图；已初始化图拒绝导入）。
  (void)compute_graph.SetInputs({compute_chain.src, compute_chain.side});

  OperatorChain execute_chain;
  Graph execute_graph("dump_equiv", kExecGraphKind);
  (void)execute_graph.SetInputs({execute_chain.src, execute_chain.side});

  ASSERT_EQ(4U, compute_graph.GetAllNodes().size());
  ASSERT_EQ(4U, execute_graph.GetAllNodes().size());

  // L2：计算图/执行图 dump 节点行（名称/type/inputs/#users）逐行一致。
  const auto compute_lines = CollectNodeLines(DumpReadable(compute_graph));
  const auto execute_lines = CollectNodeLines(DumpReadable(execute_graph));
  ASSERT_EQ(4U, compute_lines.size());
  EXPECT_EQ(compute_lines, execute_lines);
}
