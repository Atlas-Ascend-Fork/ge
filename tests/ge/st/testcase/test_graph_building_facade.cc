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
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "ge_running_env/op_reg.h"
#include "es_graph_builder.h"
#include "graph/graph.h"
#include "graph/compute_graph.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/operator.h"
#include "graph/op_desc.h"
#include "graph/ascend_string.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils_ex.h"

using namespace ge;
namespace {
// 按名称声明固定端口的通用算子，避免依赖算子注册中心。
Operator MakeFacadeOp(const char_t *name, const char_t *type, const std::vector<const char_t *> &out_names,
                      const std::vector<const char_t *> &in_names) {
  Operator op(name, type);
  for (const auto *in : in_names) {
    op.InputRegister(in, "");
  }
  for (const auto *out : out_names) {
    op.OutputRegister(out, "");
  }
  return op;
}
}  // namespace

class GraphBuildingFacadeTest : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

// 稳定构图成功路径：子图经 builder 构建并挂载，节点顺序与入参一致，子图归根。
TEST_F(GraphBuildingFacadeTest, StableTopoBuildMountsSubgraphsAndKeepsOrder) {
  auto src = MakeFacadeOp("src", "PlainOp", {"out"}, {});
  auto dst = MakeFacadeOp("dst", "PlainOp", {"out"}, {"in"});
  dst.SetInput("in", src, "out");
  auto desc = OpDescUtils::GetOpDescFromOperator(dst);
  ASSERT_NE(nullptr, desc);
  desc->RegisterSubgraphIrName("z", kStatic);
  dst.SubgraphCountRegister("z", 1U);
  dst.SetSubgraphBuilder("z", 0U, []() {
    Graph branch("z");
    (void)branch.SetValid();
    return branch;
  });
  const auto graph = GraphUtilsEx::CreateComputeGraphFromOperatorWithStableTopo("stable_semantics", {dst, src});
  ASSERT_NE(nullptr, graph);
  EXPECT_EQ(2U, graph->GetDirectNode().size());
  EXPECT_EQ("dst", graph->GetDirectNode().at(0)->GetName());
  EXPECT_EQ("src", graph->GetDirectNode().at(1)->GetName());
  const auto subgraph = graph->GetSubgraph("z");
  ASSERT_NE(nullptr, subgraph);
  EXPECT_EQ(graph, subgraph->GetParentGraph());
  EXPECT_NE(nullptr, subgraph->GetParentNode());
}

// 稳定构图失败路径：builder 返回未 SetValid 的子图时，构图整体失败并返回空。
TEST_F(GraphBuildingFacadeTest, StableTopoBuildRejectsInvalidSubgraph) {
  auto parent = MakeFacadeOp("parent", "PlainOp", {"out"}, {});
  auto desc = OpDescUtils::GetOpDescFromOperator(parent);
  ASSERT_NE(nullptr, desc);
  desc->RegisterSubgraphIrName("broken", kStatic);
  parent.SubgraphCountRegister("broken", 1U);
  parent.SetSubgraphBuilder("broken", 0U, []() {
    Graph branch("broken");
    return branch;
  });
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateComputeGraphFromOperatorWithStableTopo("stable_invalid_sub", {parent}));
}

// 常规构图入口：Data 算子作为输入建图成功，构图后图对象有效。
TEST_F(GraphBuildingFacadeTest, SetInputsBuildsGraphFromDataOperator) {
  auto data = op::Data("data_sem");
  Graph graph("inputs_sem");
  graph.SetInputs({data});
  EXPECT_TRUE(graph.IsValid());
  EXPECT_EQ(1U, graph.GetDirectNode().size());
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(graph));
}

/**
 * 用例描述：图连边编辑及节点查询的成功、失败语义。
 * 预置条件：通过公共构图接口创建具有固定端口的非常量生产者和消费者。
 * 测试步骤：建边后查询非常量输入，再校验端口越界、控制边、删边和端口索引查询。
 * 预期结果：非常量输入查询返回 GRAPH_NODE_WITHOUT_CONST_INPUT，合法边操作成功，非法操作失败。
 */
TEST_F(GraphBuildingFacadeTest, GraphEdgeEditingReportsFailuresWithNodeNames) {
  Graph graph("edges_sem");
  auto producer = MakeFacadeOp("producer", "PlainOp", {"out"}, {});
  auto consumer = MakeFacadeOp("consumer", "PlainOp", {"out"}, {"in"});
  GNode src = graph.AddNodeByOp(producer);
  GNode dst = graph.AddNodeByOp(consumer);
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));
  Tensor data;
  EXPECT_EQ(GRAPH_NODE_WITHOUT_CONST_INPUT, dst.GetInputConstData(0, data));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, 99, dst, 0));
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(src, dst));
  EXPECT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(src, 0, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, 0, dst, 0));
  // 节点名称/端口索引查询（含动态端口）走 OpDesc 延迟获取路径。
  int32_t index = -1;
  EXPECT_EQ(GRAPH_SUCCESS, dst.GetInputIndexByName(AscendString("in"), index));
  EXPECT_EQ(0, index);
  EXPECT_EQ(GRAPH_SUCCESS, dst.GetOutputIndexByName(AscendString("out"), index));
  EXPECT_EQ(0, index);
  std::vector<int32_t> dynamic_indexes;
  EXPECT_EQ(GRAPH_FAILED, dst.GetDynamicInputIndexesByName(AscendString("in"), dynamic_indexes));
  EXPECT_EQ(GRAPH_FAILED, dst.GetDynamicOutputIndexesByName(AscendString("out"), dynamic_indexes));
}

// 删点语义：外来节点按含/不含子图两种口径拒绝并上报，本图节点删除成功。
TEST_F(GraphBuildingFacadeTest, GraphRemoveNodeRejectsForeignNode) {
  Graph owner("owner_sem");
  Graph other("other_sem");
  auto op1 = MakeFacadeOp("mine", "PlainOp", {}, {});
  auto op2 = MakeFacadeOp("foreign", "PlainOp", {}, {});
  GNode mine = owner.AddNodeByOp(op1);
  GNode foreign = other.AddNodeByOp(op2);
  EXPECT_EQ(GRAPH_FAILED, owner.RemoveNode(foreign, false));
  EXPECT_EQ(GRAPH_FAILED, owner.RemoveNode(foreign, true));
  EXPECT_EQ(GRAPH_SUCCESS, owner.RemoveNode(mine));
  EXPECT_TRUE(owner.GetDirectNode().empty());
}

// 挂子图语义：无归属父节点的子图挂载失败，上报携带图与子图的本名。
TEST_F(GraphBuildingFacadeTest, GraphAddSubgraphFailureReportsNativeNames) {
  Graph parent("parent_sem");
  (void)parent.AddNodeByOp(MakeFacadeOp("pn", "PlainOp", {}, {}));
  Graph sub("sub_sem");
  ASSERT_EQ(GRAPH_SUCCESS, sub.SetValid());
  EXPECT_EQ(GRAPH_FAILED, parent.AddSubGraph(sub));
}

// 输出设置语义：GNode/索引/按名三类入口成功生效；未知算子按名设置失败且不覆盖既有输出。
TEST_F(GraphBuildingFacadeTest, GraphOutputsPropagateResultToCaller) {
  Graph graph("outputs_sem");
  auto producer = MakeFacadeOp("producer", "PlainOp", {"left", "right"}, {});
  GNode node = graph.AddNodeByOp(producer);
  const auto native = GraphUtilsEx::GetComputeGraph(graph);
  ASSERT_NE(nullptr, native);
  const std::vector<std::pair<GNode, int32_t>> gnode_outputs{{node, 0}};
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetOutputs(gnode_outputs));
  const auto out_info = native->GetGraphOutNodesInfo();
  ASSERT_EQ(1U, out_info.size());
  EXPECT_EQ("producer", out_info.at(0).first->GetName());
  EXPECT_EQ(0, out_info.at(0).second);
  const std::vector<std::pair<Operator, std::vector<size_t>>> index_outputs{{producer, {0, 1}}};
  graph.SetOutputs(index_outputs);
  EXPECT_EQ("producer:0;producer:1", native->GetOutput());
  auto missing = MakeFacadeOp("missing", "PlainOp", {"out"}, {});
  graph.SetOutputs(std::vector<std::pair<Operator, std::string>>{{missing, "out"}});
  EXPECT_EQ("producer:0;producer:1", native->GetOutput());
}

// 拷贝语义：目标未取名时沿用源图名；CopyFrom 保留已取名目标的名称；源图节点在副本中可查。
TEST_F(GraphBuildingFacadeTest, CopyGraphInfersTargetNameAndKeepsBackend) {
  Graph source("source_sem");
  (void)source.AddNodeByOp(MakeFacadeOp("n", "PlainOp", {}, {}));
  Graph copied;
  ASSERT_EQ(GRAPH_SUCCESS, GraphUtilsEx::CopyGraph(source, copied));
  const auto copied_native = GraphUtilsEx::GetComputeGraph(copied);
  ASSERT_NE(nullptr, copied_native);
  EXPECT_EQ("source_sem", copied_native->GetName());
  AscendString copied_name;
  ASSERT_EQ(GRAPH_SUCCESS, copied.FindNodeByName(AscendString("n"))->GetName(copied_name));
  EXPECT_EQ(AscendString("n"), copied_name);
  Graph target("target_sem");
  ASSERT_EQ(GRAPH_SUCCESS, target.CopyFrom(source));
  const auto target_native = GraphUtilsEx::GetComputeGraph(target);
  ASSERT_NE(nullptr, target_native);
  EXPECT_EQ("target_sem", target_native->GetName());
}

// 执行图构图：Data 入口算子经执行图 builder 建图成功，执行子图经 builder 挂载。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphBuilderMountsExecuteSubgraphs) {
  auto data = MakeFacadeOp("edata", "Data", {"out"}, {});
  auto mid = MakeFacadeOp("emid", "PlainOp", {"out"}, {"in"});
  mid.SetInput("in", data, "out");
  auto desc = OpDescUtils::GetOpDescFromOperator(mid);
  ASSERT_NE(nullptr, desc);
  desc->RegisterSubgraphIrName("ebranch", kStatic);
  mid.SubgraphCountRegister("ebranch", 1U);
  mid.SetSubgraphBuilder("ebranch", 0U, []() {
    Graph branch("ebranch", C_GRAPH_EXECUTE);
    (void)branch.SetValid();
    return branch;
  });
  const auto execute_graph = GraphUtilsEx::CreateExecuteGraphFromOperator("execute_sem", {data});
  ASSERT_NE(nullptr, execute_graph);
  EXPECT_EQ(2U, execute_graph->GetDirectNode().size());
  EXPECT_EQ(1U, execute_graph->GetAllSubgraphs().size());
}

// 执行图构图失败路径：子图 builder 返回计算图 kind 的子图时挂载失败、整体返回空。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphBuilderRejectsComputeSubgraph) {
  auto data = MakeFacadeOp("bdata", "Data", {"out"}, {});
  auto mid = MakeFacadeOp("bmid", "PlainOp", {"out"}, {"in"});
  mid.SetInput("in", data, "out");
  auto desc = OpDescUtils::GetOpDescFromOperator(mid);
  ASSERT_NE(nullptr, desc);
  desc->RegisterSubgraphIrName("compute_sub", kStatic);
  mid.SubgraphCountRegister("compute_sub", 1U);
  mid.SetSubgraphBuilder("compute_sub", 0U, []() {
    Graph branch("compute_sub");
    (void)branch.SetValid();
    return branch;
  });
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateExecuteGraphFromOperator("execute_bad_sub", {data}));
}

// 执行图侧节点语义：名称/索引/属性查询走 OpDesc 延迟获取，外来执行图节点删除被拒绝。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphNodeQueriesFallBackToOpDesc) {
  Graph graph("enode_sem", C_GRAPH_EXECUTE);
  GNode node = graph.AddNodeByOp(MakeFacadeOp("enode", "PlainOp", {"out"}, {"in"}));
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetName(name));
  EXPECT_EQ(AscendString("enode"), name);
  int32_t index = -1;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetInputIndexByName(AscendString("in"), index));
  EXPECT_EQ(0, index);
  EXPECT_EQ(GRAPH_SUCCESS, node.GetOutputIndexByName(AscendString("out"), index));
  EXPECT_EQ(0, index);
  int64_t attr_value = 7;
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("sem_attr"), attr_value));

  Graph other("eother_sem", C_GRAPH_EXECUTE);
  GNode foreign = other.AddNodeByOp(MakeFacadeOp("eforeign", "PlainOp", {}, {}));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveNode(foreign, false));
}

// 节点生命周期语义：底层图析构后被外部持有的节点参与边编辑，按 owner 失效拒绝并携带节点名上报。
TEST_F(GraphBuildingFacadeTest, GraphEdgeEditingRejectsOrphanedNodeWithoutOwner) {
  auto producer = MakeFacadeOp("orphan", "PlainOp", {"out"}, {});
  auto consumer = MakeFacadeOp("orphan_dst", "PlainOp", {}, {"in"});
  GNode orphan;
  NodePtr keeper;
  {
    Graph temp("orphan_sem");
    orphan = temp.AddNodeByOp(producer);
    const auto native = GraphUtilsEx::GetComputeGraph(temp);
    ASSERT_NE(nullptr, native);
    const auto nodes = native->GetDirectNode();
    ASSERT_EQ(1U, nodes.size());
    keeper = nodes.at(0);
  }
  Graph other("orphan_other_sem");
  GNode dst_node = other.AddNodeByOp(consumer);
  EXPECT_EQ(GRAPH_FAILED, other.AddDataEdge(orphan, 0, dst_node, 0));
  EXPECT_EQ(GRAPH_FAILED, other.AddDataEdge(dst_node, 0, orphan, 0));
}

// ES 构图入口：单名入参创建的图默认有效，输入创建按类型校验。
TEST_F(GraphBuildingFacadeTest, EsGraphBuilderDefaultKindKeepsValidGraph) {
  ge::es::EsGraphBuilder builder("es_single_name");
  auto input = builder.CreateInput(0, "es_in", "Data");
  EXPECT_NE(nullptr, input.GetCTensorHolder());
  auto invalid = builder.CreateInput(1, "es_bad", "ConstData");
  EXPECT_EQ(nullptr, invalid.GetCTensorHolder());
}

// 执行图输出设置：合法 GNode 生效；无效 GNode 触发输出挂载断言并失败返回。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphOutputsAcceptGNodesAndRejectInvalid) {
  Graph graph("eoutputs_sem", C_GRAPH_EXECUTE);
  auto producer = MakeFacadeOp("eproducer", "PlainOp", {"out"}, {});
  GNode node = graph.AddNodeByOp(producer);
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetOutputs(std::vector<std::pair<GNode, int32_t>>{{node, 0}}));
  GNode invalid;
  EXPECT_NE(GRAPH_SUCCESS, graph.SetOutputs(std::vector<std::pair<GNode, int32_t>>{{invalid, 0}}));
}

// 端口属性诊断：越界索引触发失败上报，节点名/类型经延迟取描述参与诊断。
TEST_F(GraphBuildingFacadeTest, GraphPortAttrQueriesReportNamesForInvalidIndex) {
  AttrValue value;
  Graph compute_graph("port_sem");
  GNode compute_node = compute_graph.AddNodeByOp(MakeFacadeOp("cnode", "PlainOp", {"out"}, {"in"}));
  EXPECT_NE(GRAPH_SUCCESS, compute_node.GetInputAttr(AscendString("sem_attr"), 99U, value));
  EXPECT_NE(GRAPH_SUCCESS, compute_node.SetInputAttr(AscendString("sem_attr"), 99U, value));
  Graph execute_graph("eport_sem", C_GRAPH_EXECUTE);
  GNode execute_node = execute_graph.AddNodeByOp(MakeFacadeOp("enode", "PlainOp", {"out"}, {"in"}));
  EXPECT_NE(GRAPH_SUCCESS, execute_node.GetInputAttr(AscendString("sem_attr"), 99U, value));
  EXPECT_NE(GRAPH_SUCCESS, execute_node.SetInputAttr(AscendString("sem_attr"), 99U, value));
}

// 拷贝约束：未初始化源图不可拷贝；仅根图可拷贝，带父图的子图整图拷贝被拒绝。
TEST_F(GraphBuildingFacadeTest, CopyGraphRejectsInvalidSourceAndSubgraph) {
  Graph invalid_source;
  Graph target;
  EXPECT_NE(GRAPH_SUCCESS, GraphUtilsEx::CopyGraph(invalid_source, target));

  auto src = MakeFacadeOp("cop_src", "PlainOp", {"out"}, {});
  auto dst = MakeFacadeOp("cop_dst", "PlainOp", {"out"}, {"in"});
  dst.SetInput("in", src, "out");
  auto desc = OpDescUtils::GetOpDescFromOperator(dst);
  ASSERT_NE(nullptr, desc);
  desc->RegisterSubgraphIrName("cop_sub", kStatic);
  dst.SubgraphCountRegister("cop_sub", 1U);
  dst.SetSubgraphBuilder("cop_sub", 0U, []() {
    Graph branch("cop_sub");
    (void)branch.SetValid();
    return branch;
  });
  const auto compute_graph = GraphUtilsEx::CreateComputeGraphFromOperatorWithStableTopo("copy_root_sem", {dst, src});
  ASSERT_NE(nullptr, compute_graph);
  Graph root = GraphUtilsEx::CreateGraphFromComputeGraph(compute_graph);
  const auto dst_node = root.FindNodeByName(AscendString("cop_dst"));
  ASSERT_NE(nullptr, dst_node);
  GraphPtr subgraph;
  ASSERT_EQ(GRAPH_SUCCESS, dst_node->GetSubgraph(0U, subgraph));
  ASSERT_NE(nullptr, subgraph);
  Graph copied;
  EXPECT_EQ(GRAPH_FAILED, GraphUtilsEx::CopyGraph(*subgraph, copied));
}

// 执行图 dump 只读快照：SetOutputs 登记输出后 Dump 走"转 ComputeGraph 复用计算图 dump 链路"，
// readable 文本呈现节点行与 return 输出段。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphDumpRendersReadableSnapshot) {
  Graph graph("edump_sem", C_GRAPH_EXECUTE);
  auto producer = graph.AddNodeByOp(MakeFacadeOp("edump_out", "PlainOp", {"out"}, {}));
  auto consumer = graph.AddNodeByOp(MakeFacadeOp("edump_in", "PlainOp", {"out"}, {"in"}));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(producer, 0, consumer, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{producer, 0}}));
  ASSERT_TRUE(graph.IsValid());

  std::stringstream readable;
  EXPECT_EQ(GRAPH_SUCCESS, graph.Dump(Graph::DumpFormat::kReadable, readable));
  EXPECT_NE(std::string::npos, readable.str().find("edump_out")) << readable.str();
  EXPECT_NE(std::string::npos, readable.str().find("return (%edump_out")) << readable.str();
}

// 执行图 DumpToFile：DUMP_GRAPH_PATH 指定目录后 readable 落盘成功；未知格式与计算图口径一致
// （不落盘、返回成功）。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphDumpToFileWritesSnapshot) {
  const std::string ascend_work_path = "./test_ge_graph_path";
  (void)setenv("DUMP_GRAPH_PATH", ascend_work_path.c_str(), 1);
  Graph graph("edump_file_sem", C_GRAPH_EXECUTE);
  (void)graph.AddNodeByOp(MakeFacadeOp("edump_file_node", "PlainOp", {"out"}, {}));
  EXPECT_EQ(GRAPH_SUCCESS, graph.DumpToFile(Graph::DumpFormat::kReadable, AscendString("edump_sem")));
  EXPECT_EQ(GRAPH_SUCCESS, graph.DumpToFile(static_cast<Graph::DumpFormat>(7U), AscendString("edump_bad_sem")));
  (void)system(("rm -rf " + ascend_work_path).c_str());
  (void)unsetenv("DUMP_GRAPH_PATH");
}

// RemoveSubgraph 后节点子图槽位保留（支持重新登记口径）：dump 快照对注册表缺失的实例名按现状
// 跳过，子图块与其中节点行不再呈现。
TEST_F(GraphBuildingFacadeTest, ExecuteGraphDumpSkipsRemovedSubgraphSlot) {
  Graph parent("edump_rm_sem", C_GRAPH_EXECUTE);
  auto holder = parent.AddNodeByOp(MakeFacadeOp("edump_holder", "If", {"out"}, {"in"}));
  Graph sub("edump_rm_branch", C_GRAPH_EXECUTE);
  (void)sub.AddNodeByOp(MakeFacadeOp("edump_rm_inner", "PlainOp", {"out"}, {}));
  ASSERT_EQ(GRAPH_SUCCESS, holder.SetSubgraph(AscendString("edump_rm_branch"), sub));

  std::stringstream mounted;
  ASSERT_EQ(GRAPH_SUCCESS, parent.Dump(Graph::DumpFormat::kReadable, mounted));
  EXPECT_NE(std::string::npos, mounted.str().find("graph(\"edump_rm_branch\")")) << mounted.str();

  ASSERT_EQ(GRAPH_SUCCESS, parent.RemoveSubgraph("edump_rm_branch"));
  std::stringstream removed;
  EXPECT_EQ(GRAPH_SUCCESS, parent.Dump(Graph::DumpFormat::kReadable, removed));
  EXPECT_EQ(std::string::npos, removed.str().find("graph(\"edump_rm_branch\"")) << removed.str();
  EXPECT_EQ(std::string::npos, removed.str().find("edump_rm_inner")) << removed.str();
}
