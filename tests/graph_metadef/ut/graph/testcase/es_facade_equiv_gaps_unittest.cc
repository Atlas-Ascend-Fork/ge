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
 * 门面等价性缺口补齐轮（对照 executegraph_api_facade_inventory.xlsx 中有门面映射的行）。
 * 覆盖前两轮（EsFacadeEquivTest / EsFacadeSuppEquivTest）未落成的映射组合：
 * - ExecuteGraph::RemoveEdge（数据/控制）内部 vs 门面直接对比；
 * - GetDirectNode/GetDirectNodesSize 直接对比；
 * - FastNode 边计数族（GetAllIn/OutEdgeSize、GetIn/OutEdgesSizeByIndex、GetAllIn/OutDataEdgesSize、
 *   GetInEdgeSize、OutNodesIsEmpty）vs 门面端口枚举组合；
 * - FastNode::IsDirectlyControlledByNode vs GNode::GetInControlNodes；
 * - ExecuteGraphUtils::MoveIn/MoveOut/CopyIn/CopyOutCtrlEdges（控制边转接/复制组合）；
 * - ExecuteGraphUtils::ReplaceNodeEdges（数据转接+控制复制完整语义）；
 * - ExecuteGraphUtils::RemoveSubgraphRecursively / RemoveNodeWithoutRelink 直接等价；
 * - AttrUtils(ExecuteGraph) 剩余类型（Float/ListInt/ListFloat/ListBool/ListStr/Tensor）internal vs 门面；
 * - AttrUtils(OpDesc) 节点属性 internal vs 门面 GNode::SetAttr/GetAttr；
 * - FastNodeUtils::GetParentInput（子图 Data 父输入回溯组合）；
 * - GeShape vs Shape 值对象 API。
 */
#include <gtest/gtest.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/operator.h"
#include "graph/attr_value.h"
#include "graph/ascend_string.h"
#include "graph/op_desc.h"
#include "graph/tensor.h"
#include "graph/buffer.h"
#include "graph/ge_tensor.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/fast_graph/edge.h"
#include "graph_builder_utils.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {
namespace {
// GraphKind 依赖切换期暂为整型别名（0=计算图，1=执行图），用例内以常量指代
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

// 子图 Data 节点：_parent_node_index 标记对应父节点输入端口。
OpDescPtr MakeDataOpDescWithParentIndex(const std::string &name, uint32_t parent_index) {
  auto desc = ComGraphMakeShared<OpDesc>(name, "Data");
  (void)desc->AddOutputDesc("y", GeTensorDesc());
  (void)AttrUtils::SetInt(desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index);
  return desc;
}
}  // namespace

class EsFacadeEquiv2Test : public testing::Test {};

// ExecuteGraph::RemoveEdge（数据/控制）vs Graph::RemoveEdge（端口表达）直接对比。
TEST_F(EsFacadeEquiv2Test, RemoveEdge_InternalVsFacade_DataAndControl) {
  // 内部路径。
  ut::ExecuteGraphBuilder internal_builder("eq2_rm");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *b = internal_builder.AddNode("B", "PlainOp", 1, 0);
  (void)internal_builder.AddDataEdge(a, 0, b, 0);
  (void)internal_builder.AddControlEdge(a, b);
  const auto internal_graph = internal_builder.GetGraph();
  const auto *data_edge = b->GetInDataEdgeByIndex(0);
  ASSERT_NE(nullptr, data_edge);
  ASSERT_EQ(GRAPH_SUCCESS, internal_graph->RemoveEdge(data_edge));
  const auto *ctrl_edge = a->GetAllOutControlEdgesRef().at(0U);
  ASSERT_NE(nullptr, ctrl_edge);
  ASSERT_EQ(GRAPH_SUCCESS, internal_graph->RemoveEdge(ctrl_edge));
  EXPECT_EQ(nullptr, b->GetInDataEdgeByIndex(0));
  EXPECT_TRUE(b->GetInControlNodes().empty());

  // 门面路径：同样结构，按端口表达删边。
  Graph facade("eq2_rm", kExecGraphKind);
  auto fa = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto fb = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(fa, fb));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, 0, fb, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(fa, -1, fb, -1));
  EXPECT_EQ(nullptr, fb.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_TRUE(fb.GetInControlNodes().empty());
}

// ExecuteGraph::GetDirectNode/GetDirectNodesSize vs Graph::GetDirectNode。
TEST_F(EsFacadeEquiv2Test, GetDirectNode_InternalVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_direct");
  (void)internal_builder.AddNode("n1", "PlainOp", 0, 1);
  (void)internal_builder.AddNode("n2", "PlainOp", 0, 1);
  (void)internal_builder.AddNode("n3", "PlainOp", 0, 1);
  const auto internal_graph = internal_builder.GetGraph();
  std::set<std::string> internal_names;
  for (auto *node : internal_graph->GetDirectNode()) {
    internal_names.insert(node->GetName());
  }
  EXPECT_EQ(3U, internal_graph->GetDirectNodesSize());

  Graph facade("eq2_direct", kExecGraphKind);
  (void)facade.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  (void)facade.AddNodeByOp(MakePortOp("n2", "PlainOp", 0U, 1U));
  (void)facade.AddNodeByOp(MakePortOp("n3", "PlainOp", 0U, 1U));
  std::set<std::string> facade_names;
  for (const auto &node : facade.GetDirectNode()) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
    facade_names.insert(std::string(name.GetString()));
  }
  EXPECT_EQ(internal_names, facade_names);
}

// FastNode 边计数族 vs 门面端口枚举组合。
TEST_F(EsFacadeEquiv2Test, EdgeCountFamily_InternalVsFacade) {
  // 内部路径：b 有 2 数据入边 + 1 控制入边；a 有 1 数据出边 + 1 控制出边。
  ut::ExecuteGraphBuilder internal_builder("eq2_count");
  auto *a1 = internal_builder.AddNode("A1", "PlainOp", 0, 1);
  auto *a2 = internal_builder.AddNode("A2", "PlainOp", 0, 1);
  auto *ctrl = internal_builder.AddNode("ctrl", "PlainOp", 0, 0);
  auto *b = internal_builder.AddNode("B", "PlainOp", 2, 1);
  auto *sink = internal_builder.AddNode("sink", "PlainOp", 1, 0);
  (void)internal_builder.AddDataEdge(a1, 0, b, 0);
  (void)internal_builder.AddDataEdge(a2, 0, b, 1);
  (void)internal_builder.AddControlEdge(ctrl, b);
  (void)internal_builder.AddDataEdge(b, 0, sink, 0);
  const auto internal_graph = internal_builder.GetGraph();

  // 门面路径：同结构。
  Graph facade("eq2_count", kExecGraphKind);
  auto f_a1 = facade.AddNodeByOp(MakePortOp("A1", "PlainOp", 0U, 1U));
  auto f_a2 = facade.AddNodeByOp(MakePortOp("A2", "PlainOp", 0U, 1U));
  auto f_ctrl = facade.AddNodeByOp(MakePortOp("ctrl", "PlainOp", 0U, 0U));
  auto f_b = facade.AddNodeByOp(MakePortOp("B", "PlainOp", 2U, 1U));
  auto f_sink = facade.AddNodeByOp(MakePortOp("sink", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_a1, 0, f_b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_a2, 0, f_b, 1));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_ctrl, f_b));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_b, 0, f_sink, 0));

  // 入边计数族：内部 GetAllInEdgeSize（数据+控制）vs 门面枚举（非空来源 + 控制前驱）。
  size_t facade_in = 0U;
  for (size_t i = 0U; i < f_b.GetInputsSize(); ++i) {
    if (f_b.GetInDataNodesAndPortIndexs(static_cast<int32_t>(i)).first != nullptr) {
      ++facade_in;
    }
  }
  facade_in += f_b.GetInControlNodes().size();
  EXPECT_EQ(b->GetAllInEdgeSize(), facade_in);
  EXPECT_EQ(b->GetAllInDataEdgesSize(), 2U);
  // 指定槽位计数：内部 GetInEdgesSizeByIndex vs 门面该槽位来源非空。
  EXPECT_EQ(b->GetInEdgesSizeByIndex(0), 1U);

  // 出边计数族：内部 GetAllOutEdgesSize vs 门面（数据使用者 + 控制后继）。
  size_t facade_out = 0U;
  for (size_t i = 0U; i < f_b.GetOutputsSize(); ++i) {
    facade_out += f_b.GetOutDataNodesAndPortIndexs(static_cast<int32_t>(i)).size();
  }
  facade_out += f_b.GetOutControlNodes().size();
  EXPECT_EQ(b->GetAllOutEdgesSize(), facade_out);
  EXPECT_EQ(b->GetAllOutDataEdgesSize(), 1U);
  EXPECT_EQ(b->GetOutEdgesSizeByIndex(0), 1U);
  // 循环回边过滤计数：无回边场景下与全量一致。
  EXPECT_EQ(b->GetInEdgeSize(), b->GetAllInEdgeSize());

  // OutNodesIsEmpty：sink 无出边，两路径一致。
  EXPECT_TRUE(sink->OutNodesIsEmpty());
  size_t sink_out = 0U;
  for (size_t i = 0U; i < f_sink.GetOutputsSize(); ++i) {
    sink_out += f_sink.GetOutDataNodesAndPortIndexs(static_cast<int32_t>(i)).size();
  }
  sink_out += f_sink.GetOutControlNodes().size();
  EXPECT_EQ(0U, sink_out);
  // 全图边数一致：门面按“各节点入边枚举”求和（4 条：3 数据 + 1 控制）。
  size_t facade_total_edges = 0U;
  for (const auto &node : facade.GetAllNodes()) {
    for (size_t i = 0U; i < node.GetInputsSize(); ++i) {
      if (node.GetInDataNodesAndPortIndexs(static_cast<int32_t>(i)).first != nullptr) {
        ++facade_total_edges;
      }
    }
    facade_total_edges += node.GetInControlNodes().size();
  }
  EXPECT_EQ(internal_graph->GetAllEdges().size(), facade_total_edges);
  EXPECT_EQ(4U, facade_total_edges);
}

// FastNode::IsDirectlyControlledByNode vs GNode::GetInControlNodes。
TEST_F(EsFacadeEquiv2Test, IsDirectlyControlledByNode_InternalVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_isctl");
  auto *src = internal_builder.AddNode("src", "PlainOp", 0, 0);
  auto *other = internal_builder.AddNode("other", "PlainOp", 0, 0);
  auto *dst = internal_builder.AddNode("dst", "PlainOp", 0, 0);
  (void)internal_builder.AddControlEdge(src, dst);
  EXPECT_TRUE(dst->IsDirectlyControlledByNode(src));
  EXPECT_FALSE(dst->IsDirectlyControlledByNode(other));

  Graph facade("eq2_isctl", kExecGraphKind);
  auto f_src = facade.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 0U));
  (void)facade.AddNodeByOp(MakePortOp("other", "PlainOp", 0U, 0U));
  auto f_dst = facade.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_src, f_dst));
  bool found = false;
  for (const auto &node : f_dst.GetInControlNodes()) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetName(name));
    if (std::string(name.GetString()) == "src") {
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

// ExecuteGraphUtils::MoveInCtrlEdges（控制前驱转接）vs 门面组合。
TEST_F(EsFacadeEquiv2Test, MoveInCtrlEdges_ComboVsFacade) {
  // 内部路径：old 的控制前驱全部转接到 new。
  ut::ExecuteGraphBuilder internal_builder("eq2_movein");
  auto *c1 = internal_builder.AddNode("c1", "PlainOp", 0, 0);
  auto *c2 = internal_builder.AddNode("c2", "PlainOp", 0, 0);
  auto *old_node = internal_builder.AddNode("old", "PlainOp", 0, 0);
  auto *new_node = internal_builder.AddNode("new", "PlainOp", 0, 0);
  (void)internal_builder.AddControlEdge(c1, old_node);
  (void)internal_builder.AddControlEdge(c2, old_node);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::MoveInCtrlEdges(old_node, new_node));
  EXPECT_TRUE(old_node->GetInControlNodes().empty());
  EXPECT_EQ(2U, new_node->GetInControlNodes().size());

  // 门面组合：查询→删控制边→建到新节点。
  Graph facade("eq2_movein", kExecGraphKind);
  auto f_c1 = facade.AddNodeByOp(MakePortOp("c1", "PlainOp", 0U, 0U));
  auto f_c2 = facade.AddNodeByOp(MakePortOp("c2", "PlainOp", 0U, 0U));
  auto f_old = facade.AddNodeByOp(MakePortOp("old", "PlainOp", 0U, 0U));
  auto f_new = facade.AddNodeByOp(MakePortOp("new", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_c1, f_old));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_c2, f_old));
  const auto preds = f_old.GetInControlNodes();
  for (const auto &pred : preds) {
    ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(*pred, -1, f_old, -1));
    ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(*pred, f_new));
  }
  EXPECT_TRUE(f_old.GetInControlNodes().empty());
  EXPECT_EQ(2U, f_new.GetInControlNodes().size());
}

// ExecuteGraphUtils::MoveOutCtrlEdges（控制后继转接）vs 门面组合。
TEST_F(EsFacadeEquiv2Test, MoveOutCtrlEdges_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_moveout");
  auto *old_node = internal_builder.AddNode("old", "PlainOp", 0, 0);
  auto *d1 = internal_builder.AddNode("d1", "PlainOp", 0, 0);
  auto *d2 = internal_builder.AddNode("d2", "PlainOp", 0, 0);
  auto *new_node = internal_builder.AddNode("new", "PlainOp", 0, 0);
  (void)internal_builder.AddControlEdge(old_node, d1);
  (void)internal_builder.AddControlEdge(old_node, d2);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::MoveOutCtrlEdges(old_node, new_node));
  EXPECT_TRUE(old_node->GetOutControlNodes().empty());
  EXPECT_EQ(2U, new_node->GetOutControlNodes().size());

  Graph facade("eq2_moveout", kExecGraphKind);
  auto f_old = facade.AddNodeByOp(MakePortOp("old", "PlainOp", 0U, 0U));
  auto f_d1 = facade.AddNodeByOp(MakePortOp("d1", "PlainOp", 0U, 0U));
  auto f_d2 = facade.AddNodeByOp(MakePortOp("d2", "PlainOp", 0U, 0U));
  auto f_new = facade.AddNodeByOp(MakePortOp("new", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_old, f_d1));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_old, f_d2));
  const auto succs = f_old.GetOutControlNodes();
  for (const auto &succ : succs) {
    ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(f_old, -1, *succ, -1));
    ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_new, *succ));
  }
  EXPECT_TRUE(f_old.GetOutControlNodes().empty());
  EXPECT_EQ(2U, f_new.GetOutControlNodes().size());
}

// ExecuteGraphUtils::CopyInCtrlEdges / CopyOutCtrlEdges（复制语义：原边保留）vs 门面组合。
TEST_F(EsFacadeEquiv2Test, CopyCtrlEdges_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_copyctrl");
  auto *c1 = internal_builder.AddNode("c1", "PlainOp", 0, 0);
  auto *old_node = internal_builder.AddNode("old", "PlainOp", 0, 0);
  auto *new_node = internal_builder.AddNode("new", "PlainOp", 0, 0);
  (void)internal_builder.AddControlEdge(c1, old_node);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::CopyInCtrlEdges(old_node, new_node));
  EXPECT_EQ(1U, old_node->GetInControlNodes().size());  // 复制：原边保留
  EXPECT_EQ(1U, new_node->GetInControlNodes().size());

  auto *old_out = internal_builder.AddNode("old_out", "PlainOp", 0, 0);
  auto *new_out = internal_builder.AddNode("new_out", "PlainOp", 0, 0);
  auto *d1 = internal_builder.AddNode("d1", "PlainOp", 0, 0);
  (void)internal_builder.AddControlEdge(old_out, d1);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::CopyOutCtrlEdges(old_out, new_out));
  EXPECT_EQ(1U, old_out->GetOutControlNodes().size());
  EXPECT_EQ(1U, new_out->GetOutControlNodes().size());

  // 门面组合：复制 = 仅新增同向边，不删原边。
  Graph facade("eq2_copyctrl", kExecGraphKind);
  auto f_c1 = facade.AddNodeByOp(MakePortOp("c1", "PlainOp", 0U, 0U));
  auto f_old = facade.AddNodeByOp(MakePortOp("old", "PlainOp", 0U, 0U));
  auto f_new = facade.AddNodeByOp(MakePortOp("new", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_c1, f_old));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_c1, f_new));  // 组合复制表达
  EXPECT_EQ(1U, f_old.GetInControlNodes().size());
  EXPECT_EQ(1U, f_new.GetInControlNodes().size());
}

// ExecuteGraphUtils::ReplaceNodeEdges（数据转接 + 控制复制）完整语义 vs 门面组合。
TEST_F(EsFacadeEquiv2Test, ReplaceNodeEdges_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_repedges");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *ctrl = internal_builder.AddNode("ctrl", "PlainOp", 0, 0);
  auto *old_node = internal_builder.AddNode("old", "PlainOp", 1, 1);
  auto *c = internal_builder.AddNode("C", "PlainOp", 1, 0);
  auto *new_node = internal_builder.AddNode("new", "PlainOp", 1, 1);
  (void)internal_builder.AddDataEdge(a, 0, old_node, 0);
  (void)internal_builder.AddDataEdge(old_node, 0, c, 0);
  (void)internal_builder.AddControlEdge(ctrl, old_node);
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::ReplaceNodeEdges(new_node, old_node, {0}, {0}));
  // 数据边转接到 new；控制边复制（old 保留）。
  EXPECT_EQ("A", new_node->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ("new", c->GetInDataEdgeByIndex(0)->src->GetName());
  EXPECT_EQ(1U, old_node->GetInControlNodes().size());
  EXPECT_EQ(1U, new_node->GetInControlNodes().size());

  // 门面组合：查边→断 old 数据边→建 new 数据边→复制控制边。
  Graph facade("eq2_repedges", kExecGraphKind);
  auto f_a = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto f_ctrl = facade.AddNodeByOp(MakePortOp("ctrl", "PlainOp", 0U, 0U));
  auto f_old = facade.AddNodeByOp(MakePortOp("old", "PlainOp", 1U, 1U));
  auto f_c = facade.AddNodeByOp(MakePortOp("C", "PlainOp", 1U, 0U));
  auto f_new = facade.AddNodeByOp(MakePortOp("new", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_a, 0, f_old, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_old, 0, f_c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_ctrl, f_old));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(f_a, 0, f_old, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveEdge(f_old, 0, f_c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_a, 0, f_new, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_new, 0, f_c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddControlEdge(f_ctrl, f_new));  // 控制复制：old 保留
  auto new_src = f_new.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, new_src.first);
  AscendString new_src_name;
  ASSERT_EQ(GRAPH_SUCCESS, new_src.first->GetName(new_src_name));
  EXPECT_EQ(AscendString("A"), new_src_name);
  EXPECT_EQ(1U, f_old.GetInControlNodes().size());
  EXPECT_EQ(1U, f_new.GetInControlNodes().size());
}

// ExecuteGraphUtils::RemoveSubgraphRecursively vs 门面组合（枚举节点挂载子图+逐个解除）。
TEST_F(EsFacadeEquiv2Test, RemoveSubgraphRecursively_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_rmsub");
  auto *holder = internal_builder.AddNode("holder", "If", 1, 1);
  auto sub1 = std::make_shared<ExecuteGraph>("s1");
  auto sub2 = std::make_shared<ExecuteGraph>("s2");
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, FastNodeUtils::AppendSubgraphToNode(holder, "b1", sub1));
  ASSERT_EQ(GRAPH_SUCCESS, FastNodeUtils::AppendSubgraphToNode(holder, "b2", sub2));
  ASSERT_EQ(2U, internal_graph->GetAllSubgraphs().size());
  ASSERT_EQ(GRAPH_SUCCESS, ExecuteGraphUtils::RemoveSubgraphRecursively(internal_graph.get(), holder));
  EXPECT_EQ(0U, internal_graph->GetAllSubgraphs().size());

  // 门面组合：节点挂载子图枚举 + 图级逐个解除。
  Graph facade("eq2_rmsub", kExecGraphKind);
  auto f_holder = facade.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph f_s1("s1", kExecGraphKind);
  (void)f_s1.SetValid();
  Graph f_s2("s2", kExecGraphKind);
  (void)f_s2.SetValid();
  ASSERT_EQ(GRAPH_SUCCESS, f_holder.SetSubgraph(AscendString("b1"), f_s1));
  ASSERT_EQ(GRAPH_SUCCESS, f_holder.SetSubgraph(AscendString("b2"), f_s2));
  ASSERT_EQ(2U, facade.GetAllSubgraphs().size());
  for (const auto &sub : facade.GetAllSubgraphs()) {
    ASSERT_EQ(GRAPH_SUCCESS, facade.RemoveSubgraph(sub->GetName().c_str()));
  }
  EXPECT_TRUE(facade.GetAllSubgraphs().empty());
}

// AttrUtils(ExecuteGraph) 剩余类型（Float/ListInt/ListFloat/ListBool/ListStr/Tensor）internal vs 门面。
TEST_F(EsFacadeEquiv2Test, AttrUtilsGraphAttr_FullTypes_InternalVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_gattr");
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_TRUE(AttrUtils::SetFloat(internal_graph, "f", 1.5f));
  ASSERT_TRUE(AttrUtils::SetListInt(internal_graph, "li", std::vector<int64_t>({1, 2})));
  ASSERT_TRUE(AttrUtils::SetListFloat(internal_graph, "lf", std::vector<float32_t>({0.5f, 1.5f})));
  ASSERT_TRUE(AttrUtils::SetListBool(internal_graph, "lb", std::vector<bool>({true, false})));
  ASSERT_TRUE(AttrUtils::SetListStr(internal_graph, "ls", std::vector<std::string>({"a", "b"})));
  GeTensorPtr tensor = ComGraphMakeShared<GeTensor>(GeTensorDesc(GeShape({2}), FORMAT_ND, DT_INT64));
  ASSERT_NE(nullptr, tensor);
  ASSERT_TRUE(AttrUtils::SetTensor(internal_graph, "t", tensor));

  // 门面：同值经 AttrValue + Graph::SetAttr 表达。
  Graph facade("eq2_gattr", kExecGraphKind);
  (void)facade.SetValid();
  AttrValue f;
  (void)f.SetAttrValue(1.5f);
  AttrValue li;
  (void)li.SetAttrValue(std::vector<int64_t>({1, 2}));
  AttrValue lf;
  (void)lf.SetAttrValue(std::vector<float32_t>({0.5f, 1.5f}));
  AttrValue lb;
  (void)lb.SetAttrValue(std::vector<bool>({true, false}));
  AttrValue ls;
  (void)ls.SetAttrValue(std::vector<AscendString>({AscendString("a"), AscendString("b")}));
  TensorDesc td(Shape({2}), FORMAT_ND, DT_INT64);
  Tensor ft(td);
  const std::vector<int64_t> data = {3, 4};
  ft.SetData(reinterpret_cast<const uint8_t *>(data.data()), sizeof(int64_t) * data.size());
  AttrValue t;
  (void)t.SetAttrValue(ft);
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("f"), f));
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("li"), li));
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("lf"), lf));
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("lb"), lb));
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("ls"), ls));
  ASSERT_EQ(GRAPH_SUCCESS, facade.SetAttr(AscendString("t"), t));

  // 内部读回 vs 门面读回逐项一致。
  float32_t i_f = 0.0f;
  std::vector<int64_t> i_li;
  std::vector<float32_t> i_lf;
  std::vector<bool> i_lb;
  std::vector<std::string> i_ls;
  ConstGeTensorPtr i_t;
  ASSERT_TRUE(AttrUtils::GetFloat(internal_graph, "f", i_f));
  ASSERT_TRUE(AttrUtils::GetListInt(internal_graph, "li", i_li));
  ASSERT_TRUE(AttrUtils::GetListFloat(internal_graph, "lf", i_lf));
  ASSERT_TRUE(AttrUtils::GetListBool(internal_graph, "lb", i_lb));
  ASSERT_TRUE(AttrUtils::GetListStr(internal_graph, "ls", i_ls));
  ASSERT_TRUE(AttrUtils::GetTensor(internal_graph, "t", i_t));

  AttrValue out;
  float f_f = 0.0f;
  std::vector<int64_t> f_li;
  std::vector<float32_t> f_lf;
  std::vector<bool> f_lb;
  std::vector<AscendString> f_ls;
  Tensor f_t;
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("f"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_f));
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("li"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_li));
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("lf"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_lf));
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("lb"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_lb));
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("ls"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_ls));
  ASSERT_EQ(GRAPH_SUCCESS, facade.GetAttr(AscendString("t"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(f_t));

  EXPECT_FLOAT_EQ(i_f, f_f);
  EXPECT_EQ(i_li, f_li);
  ASSERT_EQ(i_lf.size(), f_lf.size());
  EXPECT_FLOAT_EQ(i_lf[1], f_lf[1]);
  EXPECT_EQ(i_lb, f_lb);
  ASSERT_EQ(i_ls.size(), f_ls.size());
  EXPECT_EQ(i_ls[1], std::string(f_ls[1].GetString()));
  EXPECT_EQ(i_t->GetTensorDesc().GetShape().GetDims(), f_t.GetTensorDesc().GetShape().GetDims());
  EXPECT_EQ(i_t->GetTensorDesc().GetDataType(), f_t.GetTensorDesc().GetDataType());
}

// AttrUtils(OpDesc 节点属性) internal vs 门面 GNode::SetAttr/GetAttr。
TEST_F(EsFacadeEquiv2Test, AttrUtilsNodeAttr_OpDesc_InternalVsFacade) {
  // 内部路径：AttrUtils 直接写 OpDesc。
  ut::ExecuteGraphBuilder internal_builder("eq2_nattr");
  auto *internal_node = internal_builder.AddNode("n1", "PlainOp", 0, 1);
  ASSERT_TRUE(AttrUtils::SetInt(internal_node->GetOpDescPtr(), "k", 5));
  ASSERT_TRUE(AttrUtils::SetStr(internal_node->GetOpDescPtr(), "s", std::string("txt")));
  int64_t i_k = 0;
  std::string i_s;
  ASSERT_TRUE(AttrUtils::GetInt(internal_node->GetOpDescPtr(), "k", i_k));
  ASSERT_TRUE(AttrUtils::GetStr(internal_node->GetOpDescPtr(), "s", i_s));

  // 门面路径：同图同结构经 GNode::SetAttr/GetAttr。
  Graph facade("eq2_nattr", kExecGraphKind);
  auto node = facade.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(5));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("k"), v));
  AscendString s("txt");
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("s"), s));
  int64_t f_k = 0;
  AscendString f_s;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("k"), f_k));
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("s"), f_s));

  EXPECT_EQ(i_k, f_k);
  EXPECT_EQ(i_s, std::string(f_s.GetString()));
}

// FastNodeUtils::GetParentInput（子图 Data 父输入回溯）vs 门面组合（父节点输入来源查询）。
TEST_F(EsFacadeEquiv2Test, GetParentInput_ComboVsFacade) {
  ut::ExecuteGraphBuilder internal_builder("eq2_pinput");
  auto *a = internal_builder.AddNode("A", "PlainOp", 0, 1);
  auto *holder = internal_builder.AddNode("holder", "If", 1, 1);
  (void)internal_builder.AddDataEdge(a, 0, holder, 0);
  auto sub = std::make_shared<ExecuteGraph>("sub");
  auto *data_node = sub->AddNode(MakeDataOpDescWithParentIndex("inner_data", 0U));
  ASSERT_NE(nullptr, data_node);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, FastNodeUtils::AppendSubgraphToNode(holder, "b", sub));
  // 子图 Data 节点按 _parent_node_index 回溯父节点输入来源。
  auto *parent_src = FastNodeUtils::GetParentInput(data_node);
  ASSERT_NE(nullptr, parent_src);
  EXPECT_EQ(std::string("A"), parent_src->GetName());

  // 门面组合：holder 输入 0 的来源查询（同一信息经 GNode 表达）。
  Graph facade("eq2_pinput", kExecGraphKind);
  auto f_a = facade.AddNodeByOp(MakePortOp("A", "PlainOp", 0U, 1U));
  auto f_holder = facade.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, facade.AddDataEdge(f_a, 0, f_holder, 0));
  auto src = f_holder.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, src.first);
  AscendString src_name;
  ASSERT_EQ(GRAPH_SUCCESS, src.first->GetName(src_name));
  EXPECT_EQ(AscendString("A"), src_name);
  (void)data_node;
}

// GeShape vs Shape 值对象 API（端口描述 sheet 的值对象映射）。
TEST_F(EsFacadeEquiv2Test, GeShapeVsShapeValueApi) {
  const GeShape ge_shape({2, 3});
  const Shape shape(std::vector<int64_t>({2, 3}));
  EXPECT_EQ(ge_shape.GetDimNum(), shape.GetDimNum());
  EXPECT_EQ(ge_shape.GetDim(0), shape.GetDim(0));
  EXPECT_EQ(ge_shape.GetDim(1), shape.GetDim(1));
  EXPECT_EQ(ge_shape.GetDims(), shape.GetDims());
  // SetDim 修改语义一致。
  GeShape mutable_ge(ge_shape);
  Shape mutable_shape(shape);
  (void)mutable_ge.SetDim(1, 4);
  (void)mutable_shape.SetDim(1, 4);
  EXPECT_EQ(mutable_ge.GetDim(1), mutable_shape.GetDim(1));
}
}  // namespace ge
