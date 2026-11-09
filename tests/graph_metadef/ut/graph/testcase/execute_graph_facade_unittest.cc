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

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "common/ge_common/ge_inner_error_codes.h"
#include "compliant_node_builder.h"
#include "error_manager.h"
#include "es_graph_builder.h"
#include "es_c_graph_builder.h"
#include "exe_graph/lowering/builtin_node_types.h"
#include "graph/attr_value.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/debug/ge_op_types.h"
#include "graph/fast_graph/edge.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/ge_attr_value.h"
#include "graph/gnode.h"
#include "graph/graph.h"
#include "graph/graph_buffer.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/operator.h"
#include "graph/operator_factory.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/op_desc_utils.h"
#include "normal_graph/execute_graph_backend.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "normal_graph/execute_node_backend.h"
#include "normal_graph/graph_impl.h"
#include "normal_graph/operator_impl.h"

extern "C" EsCTensorHolder *EsAdd(const EsCTensorHolder *x1, const EsCTensorHolder *x2);
extern "C" void GeApiWrapper_RenameAllNodes(void *graph_ptr, const char *prefix);

namespace ge {
class ExecuteGraphFacadeTest : public testing::Test {};
class ExecuteNodeFacadeTest : public testing::Test {};

namespace {
// 无原型节点：DynamicInput/OutputRegister 声明端口数量，不依赖算子原型注册（任务 3.1 组合方式的前置形态）。
Operator MakeExecuteOp(const char_t *name, const char_t *type, uint32_t in_cnt, uint32_t out_cnt) {
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

namespace {
// 3.2 用：按名称声明固定端口，便于用 Operator::SetInput 表达连接关系。
Operator MakeNamedExecuteOp(const char_t *name, const char_t *type, const std::vector<const char_t *> &out_names,
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

// 3.2 用：带连接关系的三元链 src -> mid -> dst，mid 另有一个共享前驱 side。
struct ExecuteOperatorChain {
  Operator src;
  Operator side;
  Operator mid;
  Operator dst;

  ExecuteOperatorChain()
      : src(MakeNamedExecuteOp("src", "Data", {"out"}, {})),
        side(MakeNamedExecuteOp("side", "Data", {"out"}, {})),
        mid(MakeNamedExecuteOp("mid", "PlainOp", {"out"}, {"in0", "in1"})),
        dst(MakeNamedExecuteOp("dst", "PlainOp", {"out"}, {"in0"})) {
    mid.SetInput("in0", src, "out").SetInput("in1", side, "out");
    dst.SetInput("in0", mid, "out");
  }
};
}  // namespace

namespace {
// 4.1：构造一个执行子图并挂到父节点上。
GraphPtr MountExecuteSubgraph(Graph &parent, const Operator &parent_op, const char_t *ir_name, const char_t *sub_name) {
  auto node = parent.AddNodeByOp(parent_op);
  auto subgraph = std::make_shared<Graph>(sub_name, 1);
  subgraph->AddNodeByOp(MakeExecuteOp("sub_inner", "PlainOp", 0U, 1U));
  if (node.SetSubgraph(AscendString(ir_name), *subgraph) != GRAPH_SUCCESS) {
    return nullptr;
  }
  return subgraph;
}

// 12.11：建一张空的执行子图，内容由调用方按需填充。
GraphPtr MakeExecuteSubgraph(const char_t *name) {
  auto graph = std::make_shared<Graph>(name, 1);
  (void)graph->SetValid();
  return graph;
}

// 12.18.2：按父节点 0 号槽位取子图，并校验其执行图对象与预期一致。
void ExpectExecuteSubgraphAtSlot(const GNode &node, const ExecuteGraph *expected) {
  GraphPtr slot;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetSubgraph(0U, slot));
  ASSERT_NE(nullptr, slot);
  EXPECT_EQ(expected, GraphUtilsEx::Graph2ExecuteGraph(*slot).get());
}
}  // namespace

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphLoadOverloadsRejectComputeSerialization) {
  Graph source("serialized_compute");
  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  source.SetInputs({input}).SetOutputs({input});
  GraphBuffer buffer;
  ASSERT_EQ(GRAPH_SUCCESS, source.SaveToMem(buffer));
  const std::string file_name = "./ut_execute_graph_load_dispatch.txt";
  ASSERT_EQ(GRAPH_SUCCESS, source.SaveToFile(file_name));
  const std::vector<std::function<graphStatus(Graph &)>> loaders = {
      [&file_name](Graph &graph) { return graph.LoadFromFile(file_name); },
      [&file_name](Graph &graph) { return graph.LoadFromFile(file_name.c_str()); },
      [&buffer](Graph &graph) { return graph.LoadFromMem(buffer); },
      [&buffer](Graph &graph) { return graph.LoadFromMem(buffer.GetData(), buffer.GetSize()); },
      [&buffer](Graph &graph) { return graph.LoadFromSerializedModelArray(buffer.GetData(), buffer.GetSize()); }};
  for (const auto &load : loaders) {
    Graph execute("execute", 1);
    ASSERT_EQ(GRAPH_SUCCESS, execute.SetValid());
    const auto old_execute_impl = execute.impl_;
    EXPECT_EQ(GRAPH_FAILED, load(execute));
    EXPECT_EQ(old_execute_impl, execute.impl_);
    EXPECT_TRUE(GraphUtilsEx::IsExecuteGraph(execute));
  }
  EXPECT_EQ(0, std::remove(file_name.c_str()));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphBuilderRejectsDuplicateOperatorNames) {
  auto first = MakeExecuteOp("same", "Data", 0U, 1U);
  auto second = MakeExecuteOp("same", "Data", 0U, 1U);
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateExecuteGraphFromOperator("duplicate_inputs", {first, second}));
}

// 2.1/2.3：执行模式构图不创建 ComputeGraph/Node；计算图默认行为不受影响（IsValid 语义）。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddNodeByOpDoesNotCreateComputeGraph) {
  Graph graph("execute_graph", 1);
  ASSERT_FALSE(graph.IsValid());
  ASSERT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));
  auto node = graph.AddNodeByOp(MakeExecuteOp("node1", "PlainOp", 1U, 1U));
  ASSERT_TRUE(graph.IsValid());
  AscendString node_name;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(node_name));
  ASSERT_EQ(AscendString("node1"), node_name);

  // 执行后端直接持有 ExecuteGraph；计算图接口不得返回任何计算图对象。
  ASSERT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(graph));
  ASSERT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));

  // 计算图构造路径（默认 kind）保持原有行为。
  Graph compute_graph("compute_graph");
  ASSERT_FALSE(compute_graph.IsValid());
  compute_graph.AddNodeByOp(MakeExecuteOp("node1", "PlainOp", 1U, 1U));
  ASSERT_TRUE(compute_graph.IsValid());
  ASSERT_NE(nullptr, GraphUtilsEx::GetComputeGraph(compute_graph));
  ASSERT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(compute_graph));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphQueriesDoNotInitializeGraph) {
  Graph graph("lazy_execute", 1);
  EXPECT_TRUE(GraphUtilsEx::IsExecuteGraph(graph));
  EXPECT_EQ("lazy_execute", graph.GetName());
  EXPECT_TRUE(graph.GetDirectNode().empty());
  EXPECT_TRUE(graph.GetAllNodes().empty());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("missing")));
  EXPECT_FALSE(graph.IsValid());
  EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));

  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  const auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ASSERT_NE(nullptr, native);
  EXPECT_EQ("lazy_execute", native->GetName());
  EXPECT_TRUE(native->GetDirectNode().empty());
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  EXPECT_EQ(native, GraphUtilsEx::Graph2ExecuteGraph(graph));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsRejectsInitializedEmptyGraph) {
  ExecuteGraphBackend backend("initialized");
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetValid());
  const auto native = backend.GetGraph();
  ASSERT_NE(nullptr, native);
  EXPECT_NE(GRAPH_SUCCESS, backend.SetInputs({MakeExecuteOp("input", "Data", 0U, 1U)}));
  EXPECT_EQ(native, backend.GetGraph());
  EXPECT_TRUE(native->GetDirectNode().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsReturnGraphFailedForEmptyInputs) {
  Graph graph("empty_inputs", 1);
  EXPECT_EQ(GRAPH_FAILED, graph.impl_->SetInputs({}));
  EXPECT_FALSE(graph.IsValid());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplSetInputsPreservesFailureRetryAndInitializationOrder) {
  Graph graph("inputs", 1);
  GraphImpl &impl = *graph.impl_;
  EXPECT_EQ(GRAPH_FAILED, impl.SetInputs({}));
  EXPECT_FALSE(graph.IsValid());
  EXPECT_EQ(GRAPH_FAILED, impl.SetInputs({Operator()}));
  EXPECT_FALSE(graph.IsValid());

  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetInputs({input}));
  ASSERT_TRUE(graph.IsValid());
  ASSERT_NE(nullptr, graph.FindNodeByName(AscendString("input")));

  EXPECT_EQ(1U, ExecuteGraphUtils::GetGraphInputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get()).size());

  auto replacement = MakeExecuteOp("replacement", "Data", 0U, 1U);
  EXPECT_NE(GRAPH_SUCCESS, impl.SetInputs({replacement}));
  EXPECT_EQ(1U, graph.GetDirectNode().size());
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("input")));
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("replacement")));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphBuilderRejectsInvalidInputs) {
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateExecuteGraphFromOperator("empty_inputs", {}));

  Operator invalid;
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateExecuteGraphFromOperator("null_impl", {invalid}));

  auto missing_desc = MakeExecuteOp("missing_desc", "PlainOp", 0U, 1U);
  missing_desc.operator_impl_->op_desc_.reset();
  EXPECT_EQ(nullptr, GraphUtilsEx::CreateExecuteGraphFromOperator("null_desc", {missing_desc}));
}

// 2.1：非法 kind/名称在构造期拒绝，不产生可用图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphConstructorRejectsInvalidKindAndName) {
  const auto invalid_kind = static_cast<GraphKind>(2U);
  Graph invalid_kind_graph("invalid_kind", invalid_kind);
  ASSERT_FALSE(invalid_kind_graph.IsValid());

  Graph null_name_graph(nullptr, 1);
  ASSERT_FALSE(null_name_graph.IsValid());
}

// 2.1：计算图专用入口在执行模式下统一报错，不静默降级。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRejectsComputeOnlyEntryPoints) {
  Graph graph("execute_graph", 1);
  Operator op = MakeExecuteOp("node1", "PlainOp", 1U, 1U);
  (void)graph.AddNodeByOp(op);

  std::vector<Operator> ops = {op};
  // SetInputs/SetOutputs/SetTargets 返回 Graph& 且不报状态。
  // SetInputs 已有独立的执行图导入路径（任务 3.2），此处会在已有节点后拒绝重复导入；
  // SetOutputs 自动创建 NetOutput；SetTargets 等计算图专有能力仍被拒绝。
  (void)graph.SetInputs(ops).SetOutputs(ops).SetTargets(ops);
  EXPECT_EQ(2U, graph.GetAllNodes().size());
  EXPECT_EQ(GRAPH_FAILED, graph.AddOp(op));
  Operator found;
  EXPECT_EQ(GRAPH_FAILED, graph.FindOpByName("node1", found));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddNodeByOpAcceptDuplicateAndEmptyNames) {
  Graph graph("names", 1);
  auto first = graph.AddNodeByOp(MakeExecuteOp("same", "PlainOp", 0U, 1U));
  auto second = graph.AddNodeByOp(MakeExecuteOp("same", "PlainOp", 0U, 1U));
  auto empty_op = OpDescUtils::CreateOperatorFromOpDesc(std::make_shared<OpDesc>("", "PlainOp"));
  auto unnamed = graph.AddNodeByOp(empty_op);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, first.GetName(name));
  EXPECT_EQ(AscendString("same"), name);
  ASSERT_EQ(GRAPH_SUCCESS, second.GetName(name));
  EXPECT_EQ(AscendString("same"), name);
  ASSERT_EQ(GRAPH_SUCCESS, unnamed.GetName(name));
  EXPECT_EQ(AscendString(""), name);
  EXPECT_EQ(3U, graph.GetDirectNode().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddNodeByOpSharesOperatorDesc) {
  auto op = MakeExecuteOp("shared", "PlainOp", 0U, 1U);
  const auto desc = OpDescUtils::GetOpDescFromOperator(op);
  Graph first_graph("first", 1);
  Graph second_graph("second", 1);
  auto first = first_graph.AddNodeByOp(op);
  auto second = second_graph.AddNodeByOp(op);
  auto *first_node = NodeAdapter::GNode2ExecuteNode(first);
  auto *second_node = NodeAdapter::GNode2ExecuteNode(second);
  ASSERT_NE(nullptr, first_node);
  ASSERT_NE(nullptr, second_node);
  EXPECT_NE(first_node, second_node);
  EXPECT_EQ(desc.get(), first_node->GetOpDescBarePtr());
  EXPECT_EQ(desc.get(), second_node->GetOpDescBarePtr());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddNodeByOpRejectMissingDescBeforeInitializingGraph) {
  auto missing_desc = MakeExecuteOp("missing_desc", "PlainOp", 0U, 1U);
  missing_desc.operator_impl_->op_desc_.reset();
  Graph graph("invalid_op", 1);
  for (const auto &op : std::vector<Operator>{Operator(), missing_desc}) {
    auto node = graph.AddNodeByOp(op);
    AscendString name;
    EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
    EXPECT_FALSE(graph.IsValid());
  }
}

// 2.3：执行图上的加节点/连边/枚举/查找/删节点闭环。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplNodeEditingPreservesBackendBehavior) {
  Graph graph("edit", 1);
  GraphImpl &impl = *graph.impl_;
  auto source_op = MakeExecuteOp("source", "PlainOp", 0U, 1U);
  auto source = impl.AddNodeByOp(source_op);
  auto target = impl.AddNodeByOp(MakeExecuteOp("target", "PlainOp", 1U, 0U));
  ASSERT_NE(nullptr, impl.FindNodeByName(AscendString("source")));
  EXPECT_EQ(nullptr, impl.FindNodeByName(AscendString("missing")));

  GNode invalid;
  EXPECT_EQ(GRAPH_FAILED, impl.AddDataEdge(invalid, 0, target, 0));
  EXPECT_EQ(GRAPH_FAILED, impl.AddDataEdge(source, 0, invalid, 0));
  EXPECT_EQ(GRAPH_FAILED, impl.AddControlEdge(invalid, target));
  EXPECT_EQ(GRAPH_FAILED, impl.RemoveEdge(source, 0, invalid, 0));
  ASSERT_EQ(GRAPH_SUCCESS, impl.AddDataEdge(source, 0, target, 0));
  EXPECT_EQ(GRAPH_FAILED, impl.AddDataEdge(source, 0, target, 0));
  ASSERT_EQ(GRAPH_SUCCESS, impl.AddControlEdge(source, target));
  EXPECT_EQ(1U, source.GetOutControlNodes().size());
  ASSERT_EQ(GRAPH_SUCCESS, impl.RemoveEdge(source, 0, target, 0));
  EXPECT_EQ(GRAPH_FAILED, impl.RemoveEdge(source, 0, target, 0));
  ASSERT_EQ(GRAPH_SUCCESS, impl.AddDataEdge(source, 0, target, 0));

  ASSERT_EQ(GRAPH_SUCCESS, impl.RemoveNode(source, false));
  EXPECT_EQ(nullptr, impl.FindNodeByName(AscendString("source")));
  EXPECT_EQ(nullptr, target.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_TRUE(target.GetInControlNodes().empty());

  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, source.GetName(name));

  EXPECT_EQ(ge::PARAM_INVALID, impl.RemoveNode(invalid, false));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplEdgesPreserveBackendSpecificCrossGraphRules) {
  Graph first("first", 1);
  Graph second("second", 1);
  auto source = first.AddNodeByOp(MakeExecuteOp("source", "PlainOp", 0U, 1U));
  auto target = second.AddNodeByOp(MakeExecuteOp("target", "PlainOp", 1U, 0U));
  GraphImpl &impl = *first.impl_;

  EXPECT_EQ(GRAPH_FAILED, impl.AddDataEdge(source, 0, target, 0));
  EXPECT_EQ(GRAPH_FAILED, impl.AddControlEdge(source, target));
  EXPECT_EQ(GRAPH_FAILED, impl.RemoveEdge(source, 0, target, 0));

  EXPECT_TRUE(source.GetOutDataNodesAndPortIndexs(0).empty());
  EXPECT_TRUE(source.GetOutControlNodes().empty());
  EXPECT_EQ(nullptr, target.GetInDataNodesAndPortIndexs(0).first);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplAttributesPreserveInitializationAndEmptyNameSemantics) {
  Graph graph("attrs", 1);
  GraphImpl &impl = *graph.impl_;
  AttrValue value;
  ASSERT_EQ(GRAPH_SUCCESS, value.SetAttrValue(static_cast<int64_t>(7)));
  AttrValue read;
  EXPECT_NE(GRAPH_SUCCESS, impl.SetAttr(AscendString("key"), value));
  EXPECT_NE(GRAPH_SUCCESS, impl.GetAttr(AscendString("key"), read));
  EXPECT_FALSE(graph.IsValid());
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  EXPECT_NE(GRAPH_SUCCESS, impl.GetAttr(AscendString("missing"), read));
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetAttr(AscendString(""), value));
  ASSERT_EQ(GRAPH_SUCCESS, impl.GetAttr(AscendString(""), read));
  int64_t number = 0;
  ASSERT_EQ(GRAPH_SUCCESS, read.GetValue(number));
  EXPECT_EQ(7, number);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphBuildAndEditNodesAndEdges) {
  Graph graph("execute_graph", 1);
  auto node1 = graph.AddNodeByOp(MakeExecuteOp("node1", "PlainOp", 2U, 1U));
  auto node2 = graph.AddNodeByOp(MakeExecuteOp("node2", "PlainOp", 1U, 2U));
  auto node3 = graph.AddNodeByOp(MakeExecuteOp("node3", "PlainOp", 1U, 1U));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node1.GetName(name));
  ASSERT_EQ(AscendString("node1"), name);

  // 数据边：合法连边成功，重复占用同输入槽失败，非法端口失败。
  // AddDataEdge 与计算图入口一致，失败统一返回 GRAPH_FAILED。
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(node1, 0, node2, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(node1, 0, node2, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(node1, 1, node2, 1));
  // 控制边：零输出节点可作控制源（A13）。
  auto sink = graph.AddNodeByOp(MakeExecuteOp("sink", "PlainOp", 0U, 0U));
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(sink, node3));
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(sink, node3));

  // 枚举与查找。
  EXPECT_EQ(4U, graph.GetAllNodes().size());
  EXPECT_EQ(4U, graph.GetDirectNode().size());
  AscendString find_name("node2");
  ASSERT_NE(nullptr, graph.FindNodeByName(find_name));
  AscendString missing_name("missing");
  EXPECT_EQ(nullptr, graph.FindNodeByName(missing_name));

  // 断边与删节点。
  EXPECT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(node1, 0, node2, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(node1, 0, node2, 0));
  EXPECT_EQ(GRAPH_SUCCESS, graph.RemoveNode(node3));
  EXPECT_EQ(3U, graph.GetAllNodes().size());
  AscendString removed_name("node3");
  EXPECT_EQ(nullptr, graph.FindNodeByName(removed_name));
}

// 2.2：跨图/跨后端节点不能互相操作（owner 校验）。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRejectsCrossGraphNodes) {
  Graph graph_a("graph_a", 1);
  Graph graph_b("graph_b", 1);
  auto node_a1 = graph_a.AddNodeByOp(MakeExecuteOp("node_a1", "PlainOp", 1U, 1U));
  auto node_a2 = graph_a.AddNodeByOp(MakeExecuteOp("node_a2", "PlainOp", 1U, 1U));
  auto node_b1 = graph_b.AddNodeByOp(MakeExecuteOp("node_b1", "PlainOp", 1U, 1U));

  // 边操作接口归一化失败返回值；RemoveNode 保留当前返回值。
  EXPECT_EQ(GRAPH_FAILED, graph_b.AddDataEdge(node_a1, 0, node_b1, 0));
  EXPECT_EQ(GRAPH_FAILED, graph_b.AddControlEdge(node_a1, node_b1));
  EXPECT_EQ(GRAPH_FAILED, graph_b.RemoveEdge(node_a1, 0, node_b1, 0));
  EXPECT_EQ(GRAPH_FAILED, graph_b.RemoveNode(node_a2));

  // 计算图入口不能消费执行节点；执行图入口不能消费计算节点。
  // AddDataEdge 两侧都在节点转换失败时返回 GRAPH_FAILED。
  Graph compute_graph("compute_graph");
  auto compute_node = compute_graph.AddNodeByOp(MakeExecuteOp("compute_node", "PlainOp", 1U, 1U));
  EXPECT_EQ(GRAPH_FAILED, compute_graph.AddDataEdge(node_a1, 0, compute_node, 0));
  EXPECT_EQ(GRAPH_FAILED, graph_a.AddDataEdge(compute_node, 0, node_a1, 0));
}

// 2.2/2.3：图销毁后其节点句柄全部失效，且不影响同进程其他图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNodeInvalidAfterGraphDestroyed) {
  GNode dangling_node;
  GNodePtr dangling_node_ptr;
  {
    Graph graph("execute_graph", 1);
    auto node1 = graph.AddNodeByOp(MakeExecuteOp("node1", "PlainOp", 1U, 1U));
    auto node2 = graph.AddNodeByOp(MakeExecuteOp("node2", "PlainOp", 1U, 1U));
    EXPECT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(node1, 0, node2, 0));
    dangling_node = node1;
    dangling_node_ptr = graph.FindNodeByName(AscendString("node1"));
  }
  // 图已析构：名称/邻接查询失败，不再访问任何后端对象。
  // 失效类错误的最终状态码待公共头评审（design 3.2.8），这里只断言“不再成功”。
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, dangling_node.GetName(name));
  auto producer = dangling_node.GetInDataNodesAndPortIndexs(0);
  EXPECT_EQ(nullptr, producer.first);
  EXPECT_EQ(nullptr, dangling_node_ptr->GetInDataNodesAndPortIndexs(0).first);
}

// 3.2：SetInputs 导入 Operator 连接关系——数据/控制边、输入顺序、共享前驱。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsImportsOperatorLinks) {
  ExecuteOperatorChain chain;
  Graph graph("execute_graph", 1);
  ASSERT_FALSE(graph.IsValid());
  // 控制关系也随导入保留：dst 在 src 之后执行。
  chain.dst.AddControlInput(chain.src);
  (void)graph.SetInputs({chain.src, chain.side});
  ASSERT_TRUE(graph.IsValid());

  // 导入后不调用 CreateGraphFromOperator：不得产生计算图。
  EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(graph));
  ASSERT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));

  // 连通分量内的全部节点都被导入。
  EXPECT_EQ(4U, graph.GetAllNodes().size());
  auto mid = graph.FindNodeByName(AscendString("mid"));
  auto dst = graph.FindNodeByName(AscendString("dst"));
  ASSERT_NE(nullptr, mid);
  ASSERT_NE(nullptr, dst);

  // 数据边按 Operator 连接关系重建，共享前驱分别接到两个输入槽。
  const auto mid_in0 = mid->GetInDataNodesAndPortIndexs(0);
  const auto mid_in1 = mid->GetInDataNodesAndPortIndexs(1);
  ASSERT_NE(nullptr, mid_in0.first);
  ASSERT_NE(nullptr, mid_in1.first);
  AscendString in0_name;
  AscendString in1_name;
  ASSERT_EQ(GRAPH_SUCCESS, mid_in0.first->GetName(in0_name));
  ASSERT_EQ(GRAPH_SUCCESS, mid_in1.first->GetName(in1_name));
  EXPECT_EQ(AscendString("src"), in0_name);
  EXPECT_EQ(AscendString("side"), in1_name);

  const auto dst_in0 = dst->GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, dst_in0.first);
  AscendString dst_producer;
  ASSERT_EQ(GRAPH_SUCCESS, dst_in0.first->GetName(dst_producer));
  EXPECT_EQ(AscendString("mid"), dst_producer);

  // 控制边同样被导入。
  const auto dst_ctrl = dst->GetInControlNodes();
  ASSERT_EQ(1U, dst_ctrl.size());
  AscendString ctrl_name;
  ASSERT_EQ(GRAPH_SUCCESS, dst_ctrl[0]->GetName(ctrl_name));
  EXPECT_EQ(AscendString("src"), ctrl_name);
}

// 3.2：重复输入沿用计算图行为，同一 Operator 只创建一个节点。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsDeduplicatesNodeCreation) {
  ExecuteOperatorChain chain;
  Graph graph("execute_graph", 1);
  (void)graph.SetInputs({chain.src, chain.src});
  ASSERT_TRUE(graph.IsValid());
  EXPECT_EQ(4U, graph.GetAllNodes().size());
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("src")));
  const auto inputs = ExecuteGraphUtils::GetGraphInputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(2U, inputs.size());
  EXPECT_EQ(inputs[0], inputs[1]);
  EXPECT_EQ("src", inputs[0]->GetName());
}

// 3.2：构建失败的候选图不对外发布，修正输入后可以重新构建。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsDiscardsFailedGraph) {
  Operator producer = MakeNamedExecuteOp("dup", "Data", {"out"}, {});
  Operator consumer = MakeNamedExecuteOp("dup", "PlainOp", {"out"}, {"in0"});
  consumer.SetInput("in0", producer, "out");

  Graph graph("execute_graph", 1);
  (void)graph.SetInputs({producer});
  EXPECT_FALSE(graph.IsValid());
  EXPECT_TRUE(graph.GetAllNodes().empty());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("dup")));

  // 用不同名称重新导入应当成功，且只有本次的节点。
  Operator ok_producer = MakeNamedExecuteOp("ok_src", "Data", {"out"}, {});
  Operator ok_consumer = MakeNamedExecuteOp("ok_dst", "PlainOp", {"out"}, {"in0"});
  ok_consumer.SetInput("in0", ok_producer, "out");
  (void)graph.SetInputs({ok_producer});
  EXPECT_EQ(2U, graph.GetAllNodes().size());
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("ok_dst")));
}

// 3.2：与显式 AddNodeByOp 路径不隐式合并——已建节点的图上再导入被拒绝。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsRejectedAfterAddNodeByOp) {
  ExecuteOperatorChain chain;
  Graph graph("execute_graph", 1);
  (void)graph.AddNodeByOp(MakeExecuteOp("existing", "PlainOp", 0U, 1U));
  (void)graph.SetInputs({chain.src});
  EXPECT_EQ(1U, graph.GetAllNodes().size());
}

// 3.3：多输出节点的使用者查询按输出槽分别返回，不能串。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMultiOutputConsumerQueryIsolated) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 2U));
  auto c0 = graph.AddNodeByOp(MakeExecuteOp("c0", "PlainOp", 1U, 0U));
  auto c1 = graph.AddNodeByOp(MakeExecuteOp("c1", "PlainOp", 1U, 0U));
  auto shared = graph.AddNodeByOp(MakeExecuteOp("shared", "PlainOp", 2U, 0U));

  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, c0, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 1, c1, 0));
  // 同一输出槽可以连多个消费者，两个槽各自独立。
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, shared, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 1, shared, 1));

  const auto out0 = src.GetOutDataNodesAndPortIndexs(0);
  const auto out1 = src.GetOutDataNodesAndPortIndexs(1);
  ASSERT_EQ(2U, out0.size());
  ASSERT_EQ(2U, out1.size());
  for (const auto &item : out0) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, item.first->GetName(name));
    EXPECT_TRUE((name == AscendString("c0")) || (name == AscendString("shared")));
  }
  for (const auto &item : out1) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, item.first->GetName(name));
    EXPECT_TRUE((name == AscendString("c1")) || (name == AscendString("shared")));
  }

  // 反向查询返回的是来源节点的输出端口，两个消费者分别对应 src 的第 0/1 个输出。
  EXPECT_EQ(0, c0.GetInDataNodesAndPortIndexs(0).second);
  EXPECT_EQ(1, c1.GetInDataNodesAndPortIndexs(0).second);

  // 断开 out0 上的边，out1 不受影响。
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(src, 0, shared, 0));
  EXPECT_EQ(1U, src.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_EQ(2U, src.GetOutDataNodesAndPortIndexs(1).size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddDataEdgeUsesEndpointOwnerGraph) {
  Graph owner("owner", 1);
  auto src = owner.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 1U));
  auto dst = owner.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));
  Graph caller("caller", 1);
  ASSERT_EQ(GRAPH_SUCCESS, caller.SetValid());
  const auto native_owner = GraphUtilsEx::Graph2ExecuteGraph(owner);
  const auto native_caller = GraphUtilsEx::Graph2ExecuteGraph(caller);

  ASSERT_EQ(GRAPH_SUCCESS, caller.AddDataEdge(src, 0, dst, 0));
  EXPECT_EQ(1U, native_owner->GetAllEdges().size());
  EXPECT_TRUE(native_caller->GetAllEdges().empty());
  ASSERT_EQ(GRAPH_SUCCESS, owner.RemoveEdge(src, 0, dst, 0));
  EXPECT_TRUE(native_owner->GetAllEdges().empty());
  EXPECT_TRUE(src.GetOutDataNodesAndPortIndexs(0).empty());
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddDataEdgeAllowSelfLoop) {
  Graph graph("self_loop", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(node, 0, node, 0));
  EXPECT_EQ(1U, node.GetOutDataNodesAndPortIndexs(0).size());
  const auto producer = node.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, producer.first);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, producer.first->GetName(name));
  EXPECT_EQ(AscendString("node"), name);
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(node, 0, node, 0));
  EXPECT_TRUE(node.GetOutDataNodesAndPortIndexs(0).empty());
}

// 3.3：非法端口三种形态都被拒绝，且拒绝后邻接表未被破坏。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRejectsInvalidPorts) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));

  // 输出索引越界。
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, 1, dst, 0));
  // 输入索引越界。
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, 0, dst, 1));
  // 非 -1 的负索引。
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, -2, dst, 0));
  // 一端控制、一端数据混用（-1 与 0 组合）必须拒绝。
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, -1, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, 0, dst, -1));

  // 上述失败都没有留下任何边。
  EXPECT_EQ(0U, src.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_TRUE(src.GetOutControlNodes().empty());
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_TRUE(GraphUtilsEx::Graph2ExecuteGraph(graph)->GetAllEdges().empty());

  // 合法连边仍然成功，说明拒绝没有破坏邻接结构。
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));
  EXPECT_EQ(1U, src.GetOutDataNodesAndPortIndexs(0).size());
}

// 3.3：重复占用同一输入槽被拒绝，且原有边保持不变（失败无部分修改）。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRejectsOccupiedInputSlot) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto b = graph.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));

  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(b, 0, dst, 0));

  // 原有边仍在：dst 的来源仍是 a，b 没有多出出边。
  auto producer = dst.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, producer.first);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, producer.first->GetName(name));
  EXPECT_EQ(AscendString("a"), name);
  EXPECT_EQ(0U, b.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_EQ(1U, GraphUtilsEx::Graph2ExecuteGraph(graph)->GetAllEdges().size());
}

// 3.3：控制边两端都是 -1；允许重复添加，每次断边删除一条。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphControlEdgeConvention) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 0U));
  auto b = graph.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 0U, 0U));

  // 零输出节点也能作为控制源（A13）。
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(a, b));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(a, b));

  const auto in_ctrl = b.GetInControlNodes();
  ASSERT_EQ(2U, in_ctrl.size());
  const auto out_ctrl = a.GetOutControlNodes();
  ASSERT_EQ(2U, out_ctrl.size());

  // 断控制边同样用两端 -1。
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(a, -1, b, -1));
  EXPECT_EQ(1U, b.GetInControlNodes().size());
  EXPECT_EQ(1U, a.GetOutControlNodes().size());
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(a, -1, b, -1));
  EXPECT_TRUE(b.GetInControlNodes().empty());
  EXPECT_TRUE(a.GetOutControlNodes().empty());
  // 已经断开的控制边再断一次失败；RemoveEdge 找不到匹配边时返回 GRAPH_FAILED。
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(a, -1, b, -1));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddControlEdgeUsesEndpointOwnerGraph) {
  Graph owner("owner", 1);
  auto src = owner.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 0U));
  auto dst = owner.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 0U, 0U));
  Graph caller("caller", 1);
  ASSERT_EQ(GRAPH_SUCCESS, caller.SetValid());
  const auto native_owner = GraphUtilsEx::Graph2ExecuteGraph(owner);
  const auto native_caller = GraphUtilsEx::Graph2ExecuteGraph(caller);
  GNode invalid;
  EXPECT_EQ(GRAPH_FAILED, caller.AddControlEdge(invalid, dst));
  EXPECT_EQ(GRAPH_FAILED, caller.AddControlEdge(src, invalid));
  Graph compute("compute");
  auto compute_node = compute.AddNodeByOp(MakeExecuteOp("compute_node", "PlainOp", 0U, 0U));
  EXPECT_EQ(GRAPH_FAILED, caller.AddControlEdge(compute_node, dst));
  EXPECT_EQ(GRAPH_FAILED, caller.AddControlEdge(src, compute_node));
  EXPECT_TRUE(native_owner->GetAllEdges().empty());

  ASSERT_EQ(GRAPH_SUCCESS, caller.AddControlEdge(src, dst));
  EXPECT_EQ(1U, native_owner->GetAllEdges().size());
  EXPECT_TRUE(native_caller->GetAllEdges().empty());
  ASSERT_EQ(GRAPH_SUCCESS, owner.RemoveEdge(src, -1, dst, -1));
  EXPECT_TRUE(native_owner->GetAllEdges().empty());
  EXPECT_TRUE(src.GetOutControlNodes().empty());
  EXPECT_TRUE(dst.GetInControlNodes().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddControlEdgeAllowSelfLoopAndDuplicates) {
  Graph graph("self_loop", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(node, node));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(node, node));
  EXPECT_EQ(2U, node.GetInControlNodes().size());
  EXPECT_EQ(2U, node.GetOutControlNodes().size());
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(node, -1, node, -1));
  EXPECT_EQ(1U, node.GetInControlNodes().size());
  EXPECT_EQ(1U, node.GetOutControlNodes().size());
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(node, -1, node, -1));
  EXPECT_TRUE(node.GetInControlNodes().empty());
  EXPECT_TRUE(node.GetOutControlNodes().empty());
}

// 3.4：删除节点后入边/出边/邻居邻接表都清理干净，旧句柄失效，存活节点不受影响。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveNodeCleansNeighbors) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto b = graph.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 1U, 1U));
  auto c = graph.AddNodeByOp(MakeExecuteOp("c", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(b, 0, c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(a, b));

  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(b));

  // 邻居邻接表不再残留指向已删节点的边。
  EXPECT_EQ(0U, a.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_TRUE(a.GetOutControlNodes().empty());
  EXPECT_EQ(nullptr, c.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ(2U, graph.GetAllNodes().size());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("b")));

  // 被删节点的旧句柄失效，存活节点仍可用。
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, b.GetName(name));
  EXPECT_EQ(GRAPH_SUCCESS, a.GetName(name));
  EXPECT_EQ(AscendString("a"), name);
}

// 12.3：删除挂有子图的节点时，子图树、根登记和旧句柄一并清理；邻居不受影响。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveNodeWithSubgraph) {
  Graph graph("root", 1);
  auto keep = graph.AddNodeByOp(MakeExecuteOp("keep", "PlainOp", 1U, 1U));
  auto owner = graph.AddNodeByOp(MakeExecuteOp("owner", "IfOp", 1U, 1U));
  auto tail = graph.AddNodeByOp(MakeExecuteOp("tail", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(keep, 0, owner, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(owner, 0, tail, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(keep, owner));

  // owner -> then_branch -> level2，两层子图。
  auto branch = std::make_shared<Graph>("then_branch", 1);
  auto branch_inner = branch->AddNodeByOp(MakeExecuteOp("branch_inner", "IfOp", 1U, 1U));
  auto level2 = std::make_shared<Graph>("level2", 1);
  level2->AddNodeByOp(MakeExecuteOp("level2_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, branch_inner.SetSubgraph(AscendString("then"), *level2));
  ASSERT_EQ(GRAPH_SUCCESS, owner.SetSubgraph(AscendString("then"), *branch));
  ASSERT_NE(nullptr, graph.GetSubGraph("then_branch"));
  ASSERT_NE(nullptr, graph.GetSubGraph("level2"));

  // 按节点入口逐层取子图句柄：与根后端缓存共享同一份节点归属。
  auto owner_node = *graph.FindNodeByName(AscendString("owner"));
  GraphPtr branch_handle;
  ASSERT_EQ(GRAPH_SUCCESS, owner_node.GetSubgraph(0U, branch_handle));
  ASSERT_NE(nullptr, branch_handle);
  auto branch_inner_handle = *branch_handle->FindNodeByName(AscendString("branch_inner"));
  GraphPtr level2_handle;
  ASSERT_EQ(GRAPH_SUCCESS, branch_inner_handle.GetSubgraph(0U, level2_handle));
  ASSERT_NE(nullptr, level2_handle);
  auto level2_inner = *level2_handle->FindNodeByName(AscendString("level2_inner"));

  // 删除节点：解除直接子图的父子关系，根登记不再包含两层子图。
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(owner));
  EXPECT_EQ(nullptr, graph.GetSubGraph("then_branch"));
  EXPECT_EQ(nullptr, graph.GetSubGraph("level2"));
  EXPECT_TRUE(graph.GetAllSubgraphs().empty());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("owner")));
  // 子图节点仍存活，只有被删除节点的句柄失效；下级父子关系也保留。
  AscendString level2_name;
  EXPECT_NE(GRAPH_SUCCESS, owner.GetName(level2_name));
  EXPECT_EQ(GRAPH_SUCCESS, level2_inner.GetName(level2_name));
  EXPECT_EQ(AscendString("level2_inner"), level2_name);
  EXPECT_EQ(GRAPH_SUCCESS, branch_inner_handle.GetName(level2_name));
  EXPECT_EQ(AscendString("branch_inner"), level2_name);
  EXPECT_EQ(nullptr, branch_handle->GetParentGraph());
  ASSERT_NE(nullptr, level2_handle->GetParentGraph());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch_handle),
            GraphUtilsEx::Graph2ExecuteGraph(*level2_handle->GetParentGraph()));
  EXPECT_EQ(nullptr, tail.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_TRUE(keep.GetOutControlNodes().empty());
  AscendString keep_name;
  ASSERT_EQ(GRAPH_SUCCESS, keep.GetName(keep_name));
  EXPECT_EQ(AscendString("keep"), keep_name);
}

// 12.3：跨图节点（既非本图也非其后代）在两种模式下都被拒绝，无部分修改。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveNodeRejectsForeignNodes) {
  Graph graph("root", 1);
  Graph other("other", 1);
  (void)graph.AddNodeByOp(MakeExecuteOp("mine", "PlainOp", 0U, 1U));
  auto foreign = other.AddNodeByOp(MakeExecuteOp("foreign", "PlainOp", 0U, 1U));
  GNode foreign_node = foreign;
  EXPECT_NE(GRAPH_SUCCESS, graph.RemoveNode(foreign_node));
  EXPECT_NE(GRAPH_SUCCESS, graph.RemoveNode(foreign_node, true));
  EXPECT_NE(nullptr, other.FindNodeByName(AscendString("foreign")));
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("mine")));

  // 计算图节点混入执行图入口同样被拒绝。
  Graph compute("compute");
  compute.AddNodeByOp(MakeExecuteOp("compute_node", "PlainOp", 0U, 1U));
  auto compute_node = *compute.FindNodeByName(AscendString("compute_node"));
  EXPECT_NE(GRAPH_SUCCESS, graph.RemoveNode(compute_node, true));
  EXPECT_NE(nullptr, compute.FindNodeByName(AscendString("compute_node")));
}

// 3.4：删除已登记为图输出的节点后，输出登记同步清理。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveOutputNodeClearsRegistration) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto b = graph.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{b, 0}}));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, b, 0));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);

  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(b));
  // 沿用底层删除语义：输出槽置空，保留 NetOutput 的端口描述，不重新构图。
  const auto outputs = ExecuteGraphUtils::GetGraphOutputs(native.get());
  ASSERT_EQ(1U, outputs.size());
  EXPECT_EQ(nullptr, outputs[0].first);
  EXPECT_EQ(-1, outputs[0].second);
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  EXPECT_EQ(1U, sink->GetDataInNum());
  EXPECT_EQ(1U, sink->GetOpDescBarePtr()->GetAllInputsSize());
  EXPECT_TRUE(sink->GetAllInDataEdges().empty());
  // 已删节点不能再参与输出登记。
  EXPECT_EQ(ge::PARAM_INVALID, graph.SetOutputs({{b, 0}}));
  auto sink_handle = graph.FindNodeByName(AscendString(sink->GetNamePtr()));
  ASSERT_NE(nullptr, sink_handle);
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(*sink_handle));
  EXPECT_EQ(nullptr, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  EXPECT_EQ(1U, native->GetDirectNodesSize());
}

// 3.4：新建替换节点、转接数据边和控制边、删除旧节点后图保持一致。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphReplaceNodeKeepsGraphConsistent) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto old_b = graph.AddNodeByOp(MakeExecuteOp("old_b", "PlainOp", 1U, 1U));
  auto c = graph.AddNodeByOp(MakeExecuteOp("c", "PlainOp", 1U, 1U));
  auto ctrl = graph.AddNodeByOp(MakeExecuteOp("ctrl", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, old_b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(old_b, 0, c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(ctrl, old_b));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{c, 0}}));

  // 替换：先摘除旧节点（释放入/出边与控制边），再接 a -> new_b -> c、ctrl -> new_b。
  auto new_b = graph.AddNodeByOp(MakeExecuteOp("new_b", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(old_b));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, new_b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(new_b, 0, c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(ctrl, new_b));

  // 结构一致：节点数、邻接、输出登记。
  EXPECT_EQ(5U, graph.GetAllNodes().size());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("old_b")));
  const auto producers = c.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, producers.first);
  AscendString producer_name;
  ASSERT_EQ(GRAPH_SUCCESS, producers.first->GetName(producer_name));
  EXPECT_EQ(AscendString("new_b"), producer_name);
  EXPECT_EQ(1U, new_b.GetInControlNodes().size());
  EXPECT_EQ(1U, a.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_EQ(1U, ctrl.GetOutControlNodes().size());

  // 被替换的旧句柄不能继续参与操作。
  EXPECT_NE(GRAPH_SUCCESS, old_b.GetName(producer_name));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(old_b, 0, c, 0));
  EXPECT_EQ(ge::PARAM_INVALID, graph.RemoveNode(old_b));
}

// 3.4：按"新建替换节点 -> 转接数据边 -> 删除旧节点"的顺序改图，节点数、邻接及输出登记保持一致。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphReplaceNodeByTransferringEdges) {
  Graph graph("execute_graph", 1);
  auto a = graph.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto b = graph.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 1U, 1U));
  auto c = graph.AddNodeByOp(MakeExecuteOp("c", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(b, 0, c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{b, 0}}));

  // 旧子链仍在时新建替换节点，先断开旧边再接到新节点，避免与已占用槽位冲突。
  auto new_b = graph.AddNodeByOp(MakeExecuteOp("new_b", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(a, 0, b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(a, 0, new_b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveEdge(b, 0, c, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(new_b, 0, c, 0));
  // 输出登记从旧节点转到新节点，再删除已经孤立的旧节点。
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{new_b, 0}}));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(b));

  // 节点数、邻接、输出登记一致。
  EXPECT_EQ(4U, graph.GetAllNodes().size());
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("b")));
  EXPECT_EQ(1U, a.GetOutDataNodesAndPortIndexs(0).size());
  const auto producer = c.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, producer.first);
  AscendString producer_name;
  ASSERT_EQ(GRAPH_SUCCESS, producer.first->GetName(producer_name));
  EXPECT_EQ(AscendString("new_b"), producer_name);
  EXPECT_EQ(2U, new_b.GetOutDataNodesAndPortIndexs(0).size());
  // 登记已转给新节点，旧节点不能再被登记或参与连边。
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{new_b, 0}}));
  EXPECT_EQ(ge::PARAM_INVALID, graph.SetOutputs({{b, 0}}));
  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(b, 0, c, 0));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveEdgeUsesEndpointOwnerGraph) {
  Graph owner("owner", 1);
  auto src = owner.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 1U));
  auto dst = owner.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, owner.AddDataEdge(src, 0, dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, owner.AddControlEdge(src, dst));
  ASSERT_EQ(GRAPH_SUCCESS, owner.AddControlEdge(src, dst));
  const auto native = GraphUtilsEx::Graph2ExecuteGraph(owner);
  Graph caller("caller", 1);
  GNode invalid;
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(invalid, 0, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(src, 0, invalid, 0));
  Graph compute("compute");
  auto compute_node = compute.AddNodeByOp(MakeExecuteOp("compute_node", "PlainOp", 1U, 1U));
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(compute_node, 0, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(src, 0, compute_node, 0));
  EXPECT_EQ(3U, native->GetAllEdges().size());

  ASSERT_EQ(GRAPH_SUCCESS, caller.RemoveEdge(src, -1, dst, -1));
  EXPECT_EQ(1U, src.GetOutControlNodes().size());
  EXPECT_EQ(1U, dst.GetInControlNodes().size());
  ASSERT_EQ(GRAPH_SUCCESS, caller.RemoveEdge(src, -1, dst, -1));
  EXPECT_TRUE(src.GetOutControlNodes().empty());
  EXPECT_TRUE(dst.GetInControlNodes().empty());
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(src, -1, dst, -1));
  EXPECT_EQ(1U, native->GetAllEdges().size());
  ASSERT_EQ(GRAPH_SUCCESS, caller.RemoveEdge(src, 0, dst, 0));
  EXPECT_TRUE(native->GetAllEdges().empty());
  EXPECT_TRUE(src.GetOutDataNodesAndPortIndexs(0).empty());
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ(GRAPH_FAILED, caller.RemoveEdge(src, 0, dst, 0));
  EXPECT_FALSE(caller.IsValid());
}

// 3.3：断边入口同样校验端口，三类非法端口被拒绝且原边不受影响。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveEdgeRejectsInvalidPorts) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));

  // 输出越界、非 -1 负值、一端 -1 一端非负，均在触达边表前被拒绝。
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, 1, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, 0, dst, 1));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, -2, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, 0, dst, -2));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, -1, dst, 0));
  EXPECT_EQ(GRAPH_FAILED, graph.RemoveEdge(src, 0, dst, -1));

  // 原边仍在：两侧邻接均未变化。
  EXPECT_EQ(1U, src.GetOutDataNodesAndPortIndexs(0).size());
  ASSERT_NE(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_TRUE(src.GetOutControlNodes().empty());
}

// 3.3：AddDataEdge 不接受 -1/-1；该组合属于控制边约定，必须拒绝而不是静默建控制边。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddDataEdgeRejectsControlPorts) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 1U));
  auto dst = graph.AddNodeByOp(MakeExecuteOp("dst", "PlainOp", 1U, 0U));

  EXPECT_EQ(GRAPH_FAILED, graph.AddDataEdge(src, -1, dst, -1));
  // 拒绝后既没有控制边也没有数据边，与 AddControlEdge 的结果必须可区分。
  EXPECT_TRUE(src.GetOutControlNodes().empty());
  EXPECT_TRUE(dst.GetInControlNodes().empty());
  EXPECT_EQ(0U, src.GetOutDataNodesAndPortIndexs(0).size());
  EXPECT_EQ(nullptr, dst.GetInDataNodesAndPortIndexs(0).first);

  EXPECT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(src, dst));
  EXPECT_EQ(1U, src.GetOutControlNodes().size());
}

// 3.5：图级 AttrValue 属性读写（A22），执行图后端使用自身图属性表，不依赖计算图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGraphAttrsRoundTrip) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 1U, 1U));

  AttrValue int_value;
  ASSERT_EQ(GRAPH_SUCCESS, int_value.SetAttrValue(static_cast<int64_t>(9)));
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString("gert.stage"), int_value));
  AttrValue str_value;
  ASSERT_EQ(GRAPH_SUCCESS, str_value.SetAttrValue(AscendString("init")));
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString("name"), str_value));
  AttrValue list_value;
  ASSERT_EQ(GRAPH_SUCCESS, list_value.SetAttrValue(std::vector<AscendString>({AscendString("a"), AscendString("b")})));
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString("names"), list_value));

  AttrValue read_int;
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString("gert.stage"), read_int));
  int64_t stage = 0;
  ASSERT_EQ(GRAPH_SUCCESS, read_int.GetValue(stage));
  EXPECT_EQ(9, stage);

  AttrValue read_str;
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString("name"), read_str));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, read_str.GetValue(name));
  EXPECT_EQ(AscendString("init"), name);

  // 缺失属性读取失败；空字符串可作为属性名，与计算图一致。
  AttrValue missing;
  EXPECT_NE(GRAPH_SUCCESS, graph.GetAttr(AscendString("missing"), missing));
  EXPECT_NE(GRAPH_SUCCESS, graph.GetAttr(AscendString(""), missing));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetAttr(AscendString(""), int_value));
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString(""), missing));
  int64_t empty_name_value = 0;
  ASSERT_EQ(GRAPH_SUCCESS, missing.GetValue(empty_name_value));
  EXPECT_EQ(9, empty_name_value);
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString("gert.stage"), read_int));
  ASSERT_EQ(GRAPH_SUCCESS, read_int.GetValue(stage));
  EXPECT_EQ(9, stage);

  // 图属性与节点属性互不影响。
  AttrValue node_value;
  ASSERT_EQ(GRAPH_SUCCESS, node_value.SetAttrValue(static_cast<int64_t>(1)));
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("name"), node_value));
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetAttr(AscendString("name"), read_str));
  ASSERT_EQ(GRAPH_SUCCESS, read_str.GetValue(name));
  EXPECT_EQ(AscendString("init"), name);
}

// 重名在实际登记时失败，已写入的实例槽位保留，原根登记不被覆盖。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountDuplicateNamePreservesOriginalRegistration) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto first = MakeExecuteSubgraph("dup");
  first->AddNodeByOp(MakeExecuteOp("first_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *first));
  EXPECT_NE(nullptr, root.GetSubGraph("dup"));

  // 写入第二个槽位后，登记阶段拒绝不同对象重名。
  auto dup = MakeExecuteSubgraph("dup");
  dup->AddNodeByOp(MakeExecuteOp("dup_inner", "PlainOp", 0U, 1U));
  EXPECT_NE(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("else"), *dup));
  // 根登记仍指向第一次挂载的子图，没有被子图名相同的第二次挂载覆盖。
  auto registered = root.GetSubGraph("dup");
  ASSERT_NE(nullptr, registered);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*first).get(), GraphUtilsEx::Graph2ExecuteGraph(*registered).get());
  GraphPtr registered_slot;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(1U, registered_slot));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*first), GraphUtilsEx::Graph2ExecuteGraph(*registered_slot));
  EXPECT_EQ(nullptr, dup->GetParentGraph());
  EXPECT_EQ(nullptr, dup->GetParentNode());
  // 已挂载的第一项不受影响。
  GraphPtr keep;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(0U, keep));
  ASSERT_NE(nullptr, keep);

  // 复用已登记的槽位，可以挂载另一张名称合法的子图。
  auto retry = MakeExecuteSubgraph("retry");
  retry->AddNodeByOp(MakeExecuteOp("retry_inner", "PlainOp", 0U, 1U));
  EXPECT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("else"), *retry));
  GraphPtr retried;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(1U, retried));
  ASSERT_NE(nullptr, retried);
  auto parent_graph = retried->GetParentGraph();
  ASSERT_NE(nullptr, parent_graph);
}

// 批量挂载第二项登记失败时，第一项已完成的修改保留。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountSubgraphsPreservesCompletedItemsOnFailure) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("case", "CaseOp", 1U, 1U));
  auto taken = MakeExecuteSubgraph("taken");
  taken->AddNodeByOp(MakeExecuteOp("taken_inner", "PlainOp", 0U, 1U));
  auto taken_parent = root.AddNodeByOp(MakeExecuteOp("taken_parent", "If", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, taken_parent.SetSubgraph(AscendString("body"), *taken));

  // 第二项与已有子图重名：登记失败，第一项保持挂载。
  auto branch0 = MakeExecuteSubgraph("branch0");
  branch0->AddNodeByOp(MakeExecuteOp("branch0_inner", "PlainOp", 0U, 1U));
  auto branch1 = MakeExecuteSubgraph("taken");
  branch1->AddNodeByOp(MakeExecuteOp("branch1_inner", "PlainOp", 0U, 1U));
  std::vector<Graph> batch{*branch0, *branch1};
  EXPECT_NE(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("dynamic"), batch));
  EXPECT_NE(nullptr, branch0->GetParentGraph());
  EXPECT_NE(nullptr, branch0->GetParentNode());
  EXPECT_EQ(nullptr, branch1->GetParentGraph());
  EXPECT_NE(nullptr, root.GetSubGraph("branch0"));

  // 重试复用原槽位，换成合法名称后可以完成剩余挂载。
  auto ok1 = MakeExecuteSubgraph("ok1");
  ok1->AddNodeByOp(MakeExecuteOp("ok1_inner", "PlainOp", 0U, 1U));
  std::vector<Graph> retry{*branch0, *ok1};
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("dynamic"), retry));
  GraphPtr dynamic0;
  GraphPtr dynamic1;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(0U, dynamic0));
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(1U, dynamic1));
  ASSERT_NE(nullptr, dynamic0);
  ASSERT_NE(nullptr, dynamic1);
}

// 12.11：子图不能挂到自己的后代节点上，父子指针不成环。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountRejectsAncestorCycle) {
  Graph root("root", 1);
  auto subgraph = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, subgraph);
  auto inner = subgraph->AddNodeByOp(MakeExecuteOp("inner", "IfOp", 1U, 1U));

  // 把根图挂到它自己的后代节点上：形成祖先环，必须被拒绝。
  EXPECT_NE(GRAPH_SUCCESS, inner.SetSubgraph(AscendString("then"), root));
  EXPECT_EQ(nullptr, root.GetParentGraph());
  EXPECT_EQ(nullptr, root.GetParentNode());
  // 把中间子图挂到它自己的后代节点上同样被拒绝。
  EXPECT_NE(GRAPH_SUCCESS, inner.SetSubgraph(AscendString("then"), *subgraph));
  auto level1_handle = root.GetSubGraph("level1");
  ASSERT_NE(nullptr, level1_handle);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*subgraph).get(), GraphUtilsEx::Graph2ExecuteGraph(*level1_handle).get());
  auto parent_handle = level1_handle->GetParentGraph();
  ASSERT_NE(nullptr, parent_handle);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(root).get(), GraphUtilsEx::Graph2ExecuteGraph(*parent_handle).get());
}

// 12.11：先在子图里建好嵌套层，再整体挂到根上；根登记表包含两层。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountPrebuiltTreeRegistersEveryLevel) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto level1 = MakeExecuteSubgraph("level1");
  auto level1_if = level1->AddNodeByOp(MakeExecuteOp("level1_if", "IfOp", 1U, 1U));
  auto level2 = MakeExecuteSubgraph("level2");
  level2->AddNodeByOp(MakeExecuteOp("level2_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, level1_if.SetSubgraph(AscendString("then"), *level2));
  ASSERT_NE(nullptr, level1->GetSubGraph("level2"));

  // 整棵树挂上去之后，根按名称能查到每一层。
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *level1));
  auto level1_handle = root.GetSubGraph("level1");
  auto level2_from_root = root.GetSubGraph("level2");
  ASSERT_NE(nullptr, level1_handle);
  ASSERT_NE(nullptr, level2_from_root);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*level1).get(), GraphUtilsEx::Graph2ExecuteGraph(*level1_handle).get());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*level2).get(), GraphUtilsEx::Graph2ExecuteGraph(*level2_from_root).get());
  // 父子指针逐层成立：level1 的父图是根，level2 的父图是 level1。
  auto root_exec = GraphUtilsEx::Graph2ExecuteGraph(root);
  auto level1_exec = GraphUtilsEx::Graph2ExecuteGraph(*level1_handle);
  auto level2_exec = GraphUtilsEx::Graph2ExecuteGraph(*level2_from_root);
  auto source_level1_exec = GraphUtilsEx::Graph2ExecuteGraph(*level1);
  ASSERT_NE(nullptr, root_exec);
  ASSERT_NE(nullptr, level1_exec);
  ASSERT_NE(nullptr, level2_exec);
  ASSERT_NE(nullptr, source_level1_exec);
  EXPECT_EQ(root_exec.get(), level1_exec->GetParentGraphBarePtr());
  EXPECT_EQ(source_level1_exec.get(), level2_exec->GetParentGraphBarePtr());
  auto level1_parent = level1_handle->GetParentNode();
  ASSERT_NE(nullptr, level1_parent);
  AscendString level1_parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, level1_parent->GetName(level1_parent_name));
  EXPECT_EQ(AscendString("if"), level1_parent_name);
  auto level2_handle = level1_handle->GetSubGraph("level2");
  ASSERT_NE(nullptr, level2_handle);
  auto level2_parent = level2_handle->GetParentNode();
  ASSERT_NE(nullptr, level2_parent);
  AscendString level2_parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, level2_parent->GetName(level2_parent_name));
  EXPECT_EQ(AscendString("level1_if"), level2_parent_name);
}

// 12.3：contain_subgraph=false 只删本图节点；true 允许删后代图节点并在实际 owner 删除。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveNodeSubgraphOwnershipScope) {
  Graph root("root", 1);
  auto subgraph = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, subgraph);
  auto nested = MountExecuteSubgraph(*subgraph, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, nested);
  auto deep_node = nested->FindNodeByName(AscendString("sub_inner"));
  ASSERT_NE(nullptr, deep_node);
  GNode deep = *deep_node;

  // 默认模式拒绝后代图节点，图结构不变。
  EXPECT_NE(GRAPH_SUCCESS, root.RemoveNode(deep));
  EXPECT_NE(nullptr, nested->FindNodeByName(AscendString("sub_inner")));

  // contain_subgraph 允许删除，删除发生在实际持有该节点的子图后端。
  EXPECT_EQ(GRAPH_SUCCESS, root.RemoveNode(deep, true));
  EXPECT_EQ(nullptr, nested->FindNodeByName(AscendString("sub_inner")));
  // 父图与同级子图保持有效。
  EXPECT_NE(nullptr, root.FindNodeByName(AscendString("l1")));
  EXPECT_NE(nullptr, subgraph->FindNodeByName(AscendString("l2")));
  // 旧句柄失效。
  AscendString deep_name;
  EXPECT_NE(GRAPH_SUCCESS, deep.GetName(deep_name));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteNodeGetAllSubgraphsUsesRootRegistryAndAppendsResults) {
  Graph root("root", 1);
  auto observer = root.AddNodeByOp(MakeExecuteOp("observer", "PlainOp", 0U, 0U));
  std::vector<GraphPtr> graphs;
  EXPECT_EQ(GRAPH_FAILED, observer.GetALLSubgraphs(graphs));
  auto child = MountExecuteSubgraph(root, MakeExecuteOp("parent", "If", 0U, 1U), "then", "child");
  ASSERT_NE(nullptr, child);
  auto nested = MountExecuteSubgraph(*child, MakeExecuteOp("inner", "If", 0U, 1U), "body", "nested");
  ASSERT_NE(nullptr, nested);
  graphs.emplace_back(child);
  ASSERT_EQ(GRAPH_SUCCESS, observer.GetALLSubgraphs(graphs));
  ASSERT_EQ(3U, graphs.size());
  EXPECT_EQ(child, graphs.front());
  std::set<std::string> names;
  for (size_t i = 1U; i < graphs.size(); ++i) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, graphs[i]->GetName(name));
    names.emplace(name.GetString());
  }
  EXPECT_EQ((std::set<std::string>{"child", "nested"}), names);
  auto leaf = nested->AddNodeByOp(MakeExecuteOp("leaf", "PlainOp", 0U, 0U));
  graphs.clear();
  ASSERT_EQ(GRAPH_SUCCESS, leaf.GetALLSubgraphs(graphs));
  EXPECT_EQ(2U, graphs.size());
}

// 4.1：子图挂载、查询、父子关系与解除（A08/A23/A24/A25）。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphUserSubgraphLifecycle) {
  Graph graph("root", 1);
  auto parent = graph.AddNodeByOp(MakeExecuteOp("parent", "IfOp", 1U, 1U));
  auto subgraph = MountExecuteSubgraph(graph, MakeExecuteOp("parent2", "IfOp", 1U, 1U), "then", "then_branch");
  ASSERT_NE(nullptr, subgraph);

  // 通过节点查询：槽位 0 是刚挂载的子图，越界槽位失败。
  auto node = *graph.FindNodeByName(AscendString("parent2"));
  GraphPtr from_node;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetSubgraph(0U, from_node));
  ASSERT_NE(nullptr, from_node);
  AscendString sub_name;
  ASSERT_EQ(GRAPH_SUCCESS, from_node->GetName(sub_name));
  EXPECT_EQ(AscendString("then_branch"), sub_name);

  GraphPtr invalid_slot;
  EXPECT_NE(GRAPH_SUCCESS, node.GetSubgraph(1U, invalid_slot));

  // 通过图查询：根登记与按名称查找都能取到同一子图。
  auto all = graph.GetAllSubgraphs();
  ASSERT_EQ(1U, all.size());
  ASSERT_NE(nullptr, graph.GetSubGraph("then_branch"));
  EXPECT_EQ(nullptr, graph.GetSubGraph("missing"));

  // 父子关系：子图能查回父图与父节点，根图没有父图。
  auto parent_graph = from_node->GetParentGraph();
  ASSERT_NE(nullptr, parent_graph);
  auto parent_node = from_node->GetParentNode();
  ASSERT_NE(nullptr, parent_node);
  AscendString parent_node_name;
  ASSERT_EQ(GRAPH_SUCCESS, parent_node->GetName(parent_node_name));
  EXPECT_EQ(AscendString("parent2"), parent_node_name);
  EXPECT_EQ(nullptr, graph.GetParentGraph());
  EXPECT_EQ(nullptr, graph.GetParentNode());

  // 只移除根登记；父关系与父节点槽位保留，但按槽位查询暂时找不到该子图。
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveSubgraph("then_branch"));
  EXPECT_TRUE(graph.GetAllSubgraphs().empty());
  EXPECT_EQ(nullptr, graph.GetSubGraph("then_branch"));
  GraphPtr after_remove;
  EXPECT_NE(GRAPH_SUCCESS, node.GetSubgraph(0U, after_remove));
  EXPECT_NE(nullptr, from_node->GetParentGraph());
  EXPECT_NE(nullptr, from_node->GetParentNode());
  EXPECT_EQ(GRAPH_SUCCESS, graph.RemoveSubgraph("then_branch"));
}

// 删除登记保留父关系、槽位和节点句柄，重新登记后查询恢复。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveSubgraphPreservesParentSlotsAndNodeHandles) {
  Graph root("root", 1);
  auto node = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto branch = MakeExecuteSubgraph("branch");
  auto inner = branch->AddNodeByOp(MakeExecuteOp("branch_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetSubgraph(AscendString("then"), *branch));
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("branch"));
  EXPECT_EQ(nullptr, root.GetSubGraph("branch"));
  EXPECT_TRUE(root.GetAllSubgraphs().empty());
  EXPECT_NE(nullptr, branch->GetParentNode());
  EXPECT_NE(nullptr, branch->GetParentGraph());
  EXPECT_EQ("branch", ExecuteNodeBackend::GetDesc(node)->GetSubgraphInstanceName(0U));
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, inner.GetName(name));
  EXPECT_EQ(AscendString("branch_inner"), name);
  ASSERT_EQ(GRAPH_SUCCESS, node.SetSubgraph(AscendString("then"), *branch));
  GraphPtr restored;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetSubgraph(0U, restored));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch), GraphUtilsEx::Graph2ExecuteGraph(*restored));
}

// 删除只作用于当前登记表中的指定名称，不委托根图或递归删除。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveSubgraphOnlyChangesLocalRegistry) {
  Graph root("root", 1);
  auto level1 = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, level1);
  auto level2 = MountExecuteSubgraph(*level1, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, level2);
  // Lookup delegates to the root, whereas removal only touches the current graph's registry.
  ASSERT_EQ(GRAPH_SUCCESS, level1->RemoveSubgraph("level2"));
  EXPECT_NE(nullptr, root.GetSubGraph("level2"));
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("level1"));
  EXPECT_EQ(nullptr, root.GetSubGraph("level1"));
  EXPECT_NE(nullptr, root.GetSubGraph("level2"));
  EXPECT_EQ(1U, root.GetAllSubgraphs().size());
  EXPECT_NE(nullptr, level1->GetParentGraph());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*level1).get(),
            GraphUtilsEx::Graph2ExecuteGraph(*level2)->GetParentGraphBarePtr());
  EXPECT_TRUE(level1->GetAllSubgraphs().empty());
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("level2"));
  EXPECT_TRUE(root.GetAllSubgraphs().empty());
  EXPECT_NE(nullptr, level2->GetParentNode());
  EXPECT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("level2"));
  EXPECT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("missing"));
}

// 登记删除和恢复影响新的查询，已有图门面仍可使用。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveSubgraphUpdatesLookupsAndAllowsReregistration) {
  Graph root("root", 1);
  auto node = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto branch = MakeExecuteSubgraph("branch");
  branch->AddNodeByOp(MakeExecuteOp("branch_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetSubgraph(AscendString("then"), *branch));
  auto by_name = root.GetSubGraph("branch");
  GraphPtr by_slot;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetSubgraph(0U, by_slot));
  auto by_list = root.GetAllSubgraphs();
  ASSERT_NE(nullptr, by_name);
  ASSERT_EQ(1U, by_list.size());
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("branch"));
  EXPECT_EQ(nullptr, root.GetSubGraph("branch"));
  GraphPtr removed;
  EXPECT_NE(GRAPH_SUCCESS, node.GetSubgraph(0U, removed));
  EXPECT_TRUE(root.GetAllSubgraphs().empty());
  EXPECT_EQ(1U, by_name->GetDirectNode().size());
  EXPECT_EQ(1U, by_slot->GetDirectNode().size());
  EXPECT_EQ(1U, by_list[0]->GetDirectNode().size());
  ASSERT_EQ(GRAPH_SUCCESS, root.AddSubGraph(*branch));
  GraphPtr restored;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetSubgraph(0U, restored));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch), GraphUtilsEx::Graph2ExecuteGraph(*restored));
  EXPECT_EQ(1U, root.GetAllSubgraphs().size());
}

// 删除登记不解除父关系，后续挂载可以迁移登记并更新父关系。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemovedSubgraphKeepsContentsAndParentRelations) {
  Graph root("root", 1);
  auto branch = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "branch");
  ASSERT_NE(nullptr, branch);
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("branch"));
  branch->AddNodeByOp(MakeExecuteOp("branch_added", "PlainOp", 0U, 1U));
  EXPECT_NE(nullptr, branch->FindNodeByName(AscendString("branch_added")));
  EXPECT_EQ(2U, branch->GetDirectNode().size());
  EXPECT_EQ(nullptr, branch->GetSubGraph("branch"));
  EXPECT_TRUE(root.GetAllSubgraphs().empty());
  EXPECT_NE(nullptr, branch->GetParentGraph());
  Graph other("other", 1);
  auto other_node = other.AddNodeByOp(MakeExecuteOp("other_if", "IfOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, other_node.SetSubgraph(AscendString("then"), *branch));
  EXPECT_NE(nullptr, other.GetSubGraph("branch"));
  EXPECT_EQ(nullptr, root.GetSubGraph("branch"));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(other), GraphUtilsEx::Graph2ExecuteGraph(*branch->GetParentGraph()));
}

// 12.10：GetAllNodes 递归到用户子图，GetDirectNode 仍只返回本层。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGetAllNodesCoversSubgraphs) {
  Graph root("root", 1);
  (void)root.AddNodeByOp(MakeExecuteOp("root_a", "PlainOp", 0U, 1U));
  auto level1 = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, level1);
  auto level2 = MountExecuteSubgraph(*level1, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, level2);

  // 本层：root_a + l1；一层子图：sub_inner + l2；两层子图：sub_inner。
  auto all = root.GetAllNodes();
  EXPECT_EQ(5U, all.size());
  EXPECT_EQ(2U, root.GetDirectNode().size());
  EXPECT_EQ(2U, level1->GetDirectNode().size());
  EXPECT_EQ(1U, level2->GetDirectNode().size());

  // 名称集合覆盖三层，且没有重复。
  std::vector<std::string> names;
  for (const auto &node : all) {
    AscendString node_name;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetName(node_name));
    names.emplace_back(node_name.GetString());
  }
  std::sort(names.begin(), names.end());
  EXPECT_EQ(std::vector<std::string>({"l1", "l2", "root_a", "sub_inner", "sub_inner"}), names);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGetAllNodesFollowsNodeSubgraphSlotsInDepthFirstOrder) {
  Graph root("root", 1);
  root.AddNodeByOp(MakeExecuteOp("before", "PlainOp", 0U, 1U));
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 0U, 1U));
  root.AddNodeByOp(MakeExecuteOp("after", "PlainOp", 0U, 1U));
  auto first = MakeExecuteSubgraph("first");
  first->AddNodeByOp(MakeExecuteOp("first_before", "PlainOp", 0U, 1U));
  auto inner = first->AddNodeByOp(MakeExecuteOp("inner", "If", 0U, 1U));
  first->AddNodeByOp(MakeExecuteOp("first_after", "PlainOp", 0U, 1U));
  auto nested = MakeExecuteSubgraph("nested");
  nested->AddNodeByOp(MakeExecuteOp("nested_node", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, inner.SetSubgraph(AscendString("body"), *nested));
  auto second = MakeExecuteSubgraph("second");
  second->AddNodeByOp(MakeExecuteOp("second_node", "PlainOp", 0U, 1U));
  // Register names in reverse lexical order so that slot order, rather than name sorting, is verified.
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("z"), *first));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("a"), *second));
  auto unreferenced = MakeExecuteSubgraph("unreferenced");
  unreferenced->AddNodeByOp(MakeExecuteOp("unreferenced_node", "PlainOp", 0U, 1U));
  auto native_unreferenced = GraphUtilsEx::Graph2ExecuteGraph(*unreferenced);
  ExecuteGraphBackendUtils::SetParentGraph(native_unreferenced.get(), GraphUtilsEx::Graph2ExecuteGraph(root));
  native_unreferenced->SetParentNode(NodeAdapter::GNode2ExecuteNode(parent));
  ASSERT_EQ(GRAPH_SUCCESS, root.AddSubGraph(*unreferenced));
  const auto names_of = [](const std::vector<GNode> &nodes) {
    std::vector<std::string> names;
    for (const auto &node : nodes) {
      AscendString name;
      EXPECT_EQ(GRAPH_SUCCESS, node.GetName(name));
      if (name.GetString() != nullptr) {
        names.emplace_back(name.GetString());
      }
    }
    return names;
  };
  EXPECT_EQ((std::vector<std::string>{"before", "parent", "first_before", "inner", "nested_node", "first_after",
                                      "second_node", "after"}),
            names_of(root.GetAllNodes()));
  EXPECT_EQ((std::vector<std::string>{"first_before", "inner", "nested_node", "first_after"}),
            names_of(first->GetAllNodes()));
  EXPECT_EQ((std::vector<std::string>{"before", "parent", "after"}), names_of(root.GetDirectNode()));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGetAllNodesOnUninitializedGraphReturnsEmpty) {
  ExecuteGraphBackend backend("uninitialized");
  EXPECT_TRUE(backend.GetAllNodes().empty());
  EXPECT_FALSE(backend.IsValid());
}

// 12.10：不同层级的同名节点各自保留，枚举出的句柄按 owner 区分且可直接改边。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGetAllNodesKeepsSameNamePerLevel) {
  Graph root("root", 1);
  (void)root.AddNodeByOp(MakeExecuteOp("same", "PlainOp", 0U, 1U));
  auto root_dst = root.AddNodeByOp(MakeExecuteOp("root_dst", "PlainOp", 1U, 0U));
  auto subgraph = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, subgraph);
  (void)subgraph->AddNodeByOp(MakeExecuteOp("same", "PlainOp", 0U, 1U));
  auto sub_dst = subgraph->AddNodeByOp(MakeExecuteOp("sub_dst", "PlainOp", 1U, 0U));

  auto all = root.GetAllNodes();
  // 根 3 个（same/root_dst/l1），子图 3 个（sub_inner/same/sub_dst）。
  EXPECT_EQ(6U, all.size());
  // 同名节点不合并：两个 "same" 各留一条。
  std::vector<GNode> same_nodes;
  for (auto &node : all) {
    AscendString node_name;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetName(node_name));
    if (std::string(node_name.GetString()) == "same") {
      same_nodes.emplace_back(node);
    }
  }
  ASSERT_EQ(2U, same_nodes.size());
  // 身份按 owner 区分：每个句柄只能在它所属的那张图上改边，在另一张图上被拒绝。
  size_t root_ok = 0U;
  size_t sub_ok = 0U;
  for (auto &same : same_nodes) {
    if (root.AddDataEdge(same, 0, root_dst, 0) == GRAPH_SUCCESS) {
      ++root_ok;
    }
    if (subgraph->AddDataEdge(same, 0, sub_dst, 0) == GRAPH_SUCCESS) {
      ++sub_ok;
    }
  }
  EXPECT_EQ(1U, root_ok);
  EXPECT_EQ(1U, sub_ok);
  // 两层各自的边都已建立，互不串。
  auto root_dst_in = root_dst.GetInDataNodesAndPortIndexs(0);
  auto sub_dst_in = sub_dst.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, root_dst_in.first);
  ASSERT_NE(nullptr, sub_dst_in.first);
  EXPECT_EQ(3U, root.GetDirectNode().size());
  EXPECT_EQ(3U, subgraph->GetDirectNode().size());
}

// 4.1：嵌套子图逐层保持父子关系；跨类别子图被拒绝。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNestedSubgraphAndCrossKindRejection) {
  Graph root("root", 1);
  auto subgraph = MountExecuteSubgraph(root, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, subgraph);

  // 嵌套：在 level1 里再挂 level2。
  auto nested = MountExecuteSubgraph(*subgraph, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, nested);
  auto level2_graph = root.GetSubGraph("level1")->GetSubGraph("level2");
  ASSERT_NE(nullptr, level2_graph);
  AscendString level2_name;
  ASSERT_EQ(GRAPH_SUCCESS, level2_graph->GetName(level2_name));
  EXPECT_EQ(AscendString("level2"), level2_name);
  ASSERT_NE(nullptr, level2_graph->GetParentNode());

  // 跨类别：计算图不能挂到执行节点，执行图也不能被当作计算图子图。
  Graph compute_child("compute_child");
  compute_child.AddNodeByOp(MakeExecuteOp("c", "PlainOp", 0U, 0U));
  auto l1 = *root.FindNodeByName(AscendString("l1"));
  EXPECT_NE(GRAPH_SUCCESS, l1.SetSubgraph(AscendString("then"), compute_child));

  Graph compute_root("compute_root");
  compute_root.AddNodeByOp(MakeExecuteOp("p", "PlainOp", 0U, 0U));
  EXPECT_NE(GRAPH_SUCCESS, compute_root.AddSubGraph(*subgraph));
  EXPECT_NE(GRAPH_SUCCESS, root.AddSubGraph(compute_child));
}

// 4.2：CopyFrom 深复制结构，副本与源图相互独立（A27）。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromIsIndependent) {
  Graph src("src", 1);
  auto a = src.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  auto b = src.AddNodeByOp(MakeExecuteOp("b", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, src.AddDataEdge(a, 0, b, 0));
  ASSERT_EQ(GRAPH_SUCCESS, src.AddControlEdge(a, b));
  ASSERT_EQ(GRAPH_SUCCESS, src.SetOutputs({{b, 0}}));
  AttrValue src_attr;
  ASSERT_EQ(GRAPH_SUCCESS, src_attr.SetAttrValue(AscendString("init")));
  ASSERT_EQ(GRAPH_SUCCESS, a.SetAttr(AscendString("stage"), src_attr));

  Graph dst("dst", 1);
  ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src));

  // 结构一致：节点数、边与输出登记都被复制。
  EXPECT_EQ(3U, dst.GetAllNodes().size());
  auto dst_a = dst.FindNodeByName(AscendString("a"));
  auto dst_b = dst.FindNodeByName(AscendString("b"));
  ASSERT_NE(nullptr, dst_a);
  ASSERT_NE(nullptr, dst_b);
  ASSERT_EQ(1U, dst_a->GetOutDataNodesAndPortIndexs(0).size());
  ASSERT_EQ(1U, dst_a->GetOutControlNodes().size());
  ASSERT_NE(nullptr, dst_b->GetInDataNodesAndPortIndexs(0).first);

  // 常量/属性有效且复制独立：改副本不影响源图。
  AttrValue copied_attr;
  ASSERT_EQ(GRAPH_SUCCESS, dst_a->GetAttr(AscendString("stage"), copied_attr));
  AscendString copied_stage;
  ASSERT_EQ(GRAPH_SUCCESS, copied_attr.GetValue(copied_stage));
  EXPECT_EQ(AscendString("init"), copied_stage);
  AttrValue changed;
  ASSERT_EQ(GRAPH_SUCCESS, changed.SetAttrValue(AscendString("main")));
  ASSERT_EQ(GRAPH_SUCCESS, dst_a->SetAttr(AscendString("stage"), changed));
  AttrValue src_read;
  ASSERT_EQ(GRAPH_SUCCESS, a.GetAttr(AscendString("stage"), src_read));
  AscendString src_stage;
  ASSERT_EQ(GRAPH_SUCCESS, src_read.GetValue(src_stage));
  EXPECT_EQ(AscendString("init"), src_stage);

  // 改副本结构不影响源图。
  ASSERT_EQ(GRAPH_SUCCESS, dst.RemoveNode(*dst_b));
  EXPECT_EQ(3U, src.GetAllNodes().size());
  EXPECT_NE(nullptr, src.FindNodeByName(AscendString("b")));
}

// 12.13：三级图树的 CopyFrom 逐层重建父图/父节点，副本完全脱离源图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromRelinksEveryLevel) {
  Graph src("src", 1);
  auto level1 = MountExecuteSubgraph(src, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, level1);
  auto level2 = MountExecuteSubgraph(*level1, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, level2);
  auto level3 = MountExecuteSubgraph(*level2, MakeExecuteOp("l3", "IfOp", 1U, 1U), "then", "level3");
  ASSERT_NE(nullptr, level3);
  // 根 l1；level1 有 sub_inner + l2；level2 有 sub_inner + l3；level3 有 sub_inner。
  ASSERT_EQ(6U, src.GetAllNodes().size());

  Graph dst("dst", 1);
  ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src));

  // 三层子图都在副本里，且副本的父子指针逐层指向副本自身，不再指向源图。
  auto copied_l1 = dst.GetSubGraph("level1");
  auto copied_l2 = dst.GetSubGraph("level2");
  auto copied_l3 = dst.GetSubGraph("level3");
  ASSERT_NE(nullptr, copied_l1);
  ASSERT_NE(nullptr, copied_l2);
  ASSERT_NE(nullptr, copied_l3);
  auto dst_exec = GraphUtilsEx::Graph2ExecuteGraph(dst);
  auto src_exec = GraphUtilsEx::Graph2ExecuteGraph(src);
  auto l1_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l1);
  auto l2_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l2);
  auto l3_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l3);
  ASSERT_NE(nullptr, dst_exec);
  ASSERT_NE(nullptr, src_exec);
  ASSERT_NE(nullptr, l1_exec);
  ASSERT_NE(nullptr, l2_exec);
  ASSERT_NE(nullptr, l3_exec);
  // l1 的父图是副本根，l2 的父图是副本 l1，l3 的父图是副本 l2。
  EXPECT_EQ(dst_exec.get(), l1_exec->GetParentGraphBarePtr());
  EXPECT_EQ(l1_exec.get(), l2_exec->GetParentGraphBarePtr());
  EXPECT_EQ(l2_exec.get(), l3_exec->GetParentGraphBarePtr());
  // 父节点逐层落在对应副本图上，名称与源图一致。
  auto l1_parent = copied_l1->GetParentNode();
  auto l2_parent = copied_l2->GetParentNode();
  auto l3_parent = copied_l3->GetParentNode();
  ASSERT_NE(nullptr, l1_parent);
  ASSERT_NE(nullptr, l2_parent);
  ASSERT_NE(nullptr, l3_parent);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, l1_parent->GetName(name));
  EXPECT_EQ(AscendString("l1"), name);
  ASSERT_EQ(GRAPH_SUCCESS, l2_parent->GetName(name));
  EXPECT_EQ(AscendString("l2"), name);
  ASSERT_EQ(GRAPH_SUCCESS, l3_parent->GetName(name));
  EXPECT_EQ(AscendString("l3"), name);
  // 源图父子关系不变。
  auto *src_l1 = src_exec->GetSubGraph("level1");
  ASSERT_NE(nullptr, src_l1);
  EXPECT_EQ(src_exec.get(), src_l1->GetParentGraphBarePtr());
}

// 12.13：不同层级出现同名节点时，副本仍按层级重建父节点，不会串到别的层。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromKeepsSameNamePerLevel) {
  Graph src("src", 1);
  auto level1 = MountExecuteSubgraph(src, MakeExecuteOp("cond", "IfOp", 1U, 1U), "then", "level1");
  ASSERT_NE(nullptr, level1);
  // level1 里同名的 "cond" 节点，mount 出 level2。
  auto level2 = MountExecuteSubgraph(*level1, MakeExecuteOp("cond", "IfOp", 1U, 1U), "then", "level2");
  ASSERT_NE(nullptr, level2);

  Graph dst("dst", 1);
  ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src));
  auto copied_l1 = dst.GetSubGraph("level1");
  auto copied_l2 = dst.GetSubGraph("level2");
  ASSERT_NE(nullptr, copied_l1);
  ASSERT_NE(nullptr, copied_l2);
  auto l1_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l1);
  auto l2_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l2);
  ASSERT_NE(nullptr, l1_exec);
  ASSERT_NE(nullptr, l2_exec);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(dst).get(), l1_exec->GetParentGraphBarePtr());
  // level2 的父图必须是 level1 的副本，而不是根副本。
  EXPECT_EQ(l1_exec.get(), l2_exec->GetParentGraphBarePtr());
  // level2 的父节点必须是 level1 副本里的 "cond"。
  auto l2_parent = copied_l2->GetParentNode();
  ASSERT_NE(nullptr, l2_parent);
  AscendString parent_name;
  ASSERT_EQ(GRAPH_SUCCESS, l2_parent->GetName(parent_name));
  EXPECT_EQ(AscendString("cond"), parent_name);
  auto l1_cond = copied_l1->FindNodeByName(AscendString("cond"));
  ASSERT_NE(nullptr, l1_cond);
  AscendString l1_cond_name;
  ASSERT_EQ(GRAPH_SUCCESS, l1_cond->GetName(l1_cond_name));
  EXPECT_EQ(AscendString("cond"), l1_cond_name);
}

// 12.13：源图销毁后副本仍可查询和构图，副本修改不影响源图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromSurvivesSourceDestroyed) {
  Graph dst("dst", 1);
  {
    Graph src("src", 1);
    auto level1 = MountExecuteSubgraph(src, MakeExecuteOp("l1", "IfOp", 1U, 1U), "then", "level1");
    ASSERT_NE(nullptr, level1);
    auto level2 = MountExecuteSubgraph(*level1, MakeExecuteOp("l2", "IfOp", 1U, 1U), "then", "level2");
    ASSERT_NE(nullptr, level2);
    ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src));
  }
  // 源图已销毁：副本的三层结构、父子关系与节点内容都还能用。
  EXPECT_EQ(nullptr, dst.GetSubGraph("missing"));
  auto copied_l1 = dst.GetSubGraph("level1");
  auto copied_l2 = dst.GetSubGraph("level2");
  ASSERT_NE(nullptr, copied_l1);
  ASSERT_NE(nullptr, copied_l2);
  ASSERT_NE(nullptr, copied_l2->FindNodeByName(AscendString("sub_inner")));
  auto l2_exec = GraphUtilsEx::Graph2ExecuteGraph(*copied_l2);
  ASSERT_NE(nullptr, l2_exec);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*copied_l1).get(), l2_exec->GetParentGraphBarePtr());
  // 副本可继续构图。
  (void)copied_l2->AddNodeByOp(MakeExecuteOp("copied_added", "PlainOp", 0U, 1U));
  EXPECT_NE(nullptr, copied_l2->FindNodeByName(AscendString("copied_added")));
}

// 4.2：拒绝跨类别、无效源图及非根源图，失败时不初始化目标。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromRejectsInvalidCases) {
  Graph src("src", 1);
  src.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));

  Graph compute_src("compute_src");
  Graph dst("dst", 1);
  EXPECT_NE(GRAPH_SUCCESS, dst.CopyFrom(compute_src));

  Graph compute_dst("compute_dst");
  EXPECT_NE(GRAPH_SUCCESS, compute_dst.CopyFrom(src));

  Graph invalid_src;
  EXPECT_EQ(GRAPH_FAILED, dst.CopyFrom(invalid_src));
  EXPECT_FALSE(dst.IsValid());
  auto branch = MountExecuteSubgraph(src, MakeExecuteOp("parent", "If", 0U, 1U), "then", "branch");
  ASSERT_NE(nullptr, branch);
  EXPECT_EQ(GRAPH_FAILED, dst.CopyFrom(*branch));
  EXPECT_FALSE(dst.IsValid());
  Graph empty_impl_dst;
  EXPECT_EQ(GRAPH_FAILED, empty_impl_dst.CopyFrom(src));
  EXPECT_FALSE(empty_impl_dst.IsValid());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFromSupportSelfCopyAndOverwrite) {
  Graph src("src", 1);
  src.AddNodeByOp(MakeExecuteOp("a", "PlainOp", 0U, 1U));
  Graph old_src = src;
  ASSERT_EQ(GRAPH_SUCCESS, src.CopyFrom(src));
  EXPECT_NE(src.impl_, old_src.impl_);
  EXPECT_NE(nullptr, src.FindNodeByName(AscendString("a")));
  EXPECT_NE(nullptr, old_src.FindNodeByName(AscendString("a")));

  Graph nonempty("nonempty", 1);
  auto old_node = nonempty.AddNodeByOp(MakeExecuteOp("x", "PlainOp", 0U, 1U));
  Graph alias = nonempty;
  ASSERT_EQ(GRAPH_SUCCESS, nonempty.CopyFrom(src));
  EXPECT_EQ(1U, nonempty.GetAllNodes().size());
  EXPECT_NE(nullptr, nonempty.FindNodeByName(AscendString("a")));
  EXPECT_EQ(nullptr, nonempty.FindNodeByName(AscendString("x")));
  EXPECT_NE(nullptr, alias.FindNodeByName(AscendString("x")));
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, old_node.GetName(name));
  EXPECT_EQ(AscendString("x"), name);
  EXPECT_EQ("nonempty", nonempty.GetName());

  EXPECT_EQ("nonempty", GraphUtilsEx::Graph2ExecuteGraph(nonempty)->GetName());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyFailurePreservesExistingDestination) {
  Graph src("source", 1);
  auto producer = src.AddNodeByOp(MakeExecuteOp("producer", "PlainOp", 0U, 1U));
  auto consumer = src.AddNodeByOp(MakeExecuteOp("consumer", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, src.AddDataEdge(producer, 0, consumer, 0));
  auto native_src = GraphUtilsEx::Graph2ExecuteGraph(src);
  auto *edge = native_src->GetAllEdges()[0];
  Graph dst("destination", 1);
  auto keep = dst.AddNodeByOp(MakeExecuteOp("keep", "PlainOp", 0U, 1U));
  const auto original_impl = dst.impl_;
  // 源图含有越界端口，复制边时失败；此前已复制的节点不能提交到目标。
  edge->src_output = 1;
  const auto ret = dst.CopyFrom(src);
  edge->src_output = 0;
  EXPECT_EQ(GRAPH_FAILED, ret);
  EXPECT_EQ(original_impl, dst.impl_);
  EXPECT_EQ(nullptr, dst.FindNodeByName(AscendString("producer")));
  EXPECT_NE(nullptr, dst.FindNodeByName(AscendString("keep")));
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, keep.GetName(name));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyPreservesEmptyAndRepeatedOutputSlots) {
  Graph src("source", 1);
  auto gone = src.AddNodeByOp(MakeExecuteOp("gone", "PlainOp", 0U, 1U));
  auto keep = src.AddNodeByOp(MakeExecuteOp("keep", "PlainOp", 0U, 2U));
  ASSERT_EQ(GRAPH_SUCCESS, src.SetOutputs({{gone, 0}, {keep, 1}, {keep, 0}, {gone, 0}}));
  ASSERT_EQ(GRAPH_SUCCESS, src.RemoveNode(gone));
  Graph copied("", 1);
  ASSERT_EQ(GRAPH_SUCCESS, copied.CopyFrom(src));
  EXPECT_EQ("source", copied.GetName());
  auto native = GraphUtilsEx::Graph2ExecuteGraph(copied);
  const auto outputs = ExecuteGraphUtils::GetGraphOutputs(native.get());
  ASSERT_EQ(4U, outputs.size());
  EXPECT_EQ(nullptr, outputs[0].first);
  EXPECT_EQ(-1, outputs[0].second);
  EXPECT_EQ(nullptr, outputs[3].first);
  EXPECT_EQ(-1, outputs[3].second);
  ASSERT_NE(nullptr, outputs[1].first);
  EXPECT_EQ(outputs[1].first, outputs[2].first);
  EXPECT_EQ(1, outputs[1].second);
  EXPECT_EQ(0, outputs[2].second);
  EXPECT_EQ((std::vector<int32_t>{1, 2}), outputs[1].first->GetExtendInfo()->GetOutputIndex());
  EXPECT_EQ(2U, native->GetAllEdges().size());
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);
  EXPECT_EQ(4U, sink->GetDataInNum());
  EXPECT_EQ(nullptr, sink->GetInDataEdgeByIndex(0));
  EXPECT_EQ(nullptr, sink->GetInDataEdgeByIndex(3));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyPreservesInputSlotsAndGraphMetadata) {
  Graph src("source", 1);
  auto first = src.AddNodeByOp(MakeExecuteOp("first", "Data", 0U, 1U));
  auto second = src.AddNodeByOp(MakeExecuteOp("second", "Data", 0U, 1U));
  const auto native_src = GraphUtilsEx::Graph2ExecuteGraph(src);
  native_src->SetGraphId(42U);
  ASSERT_EQ(GRAPH_SUCCESS,
            ExecuteGraphUtils::SetGraphInputs(native_src.get(), {NodeAdapter::GNode2ExecuteNode(second), nullptr,
                                                                 NodeAdapter::GNode2ExecuteNode(first)}));
  AttrValue value;
  ASSERT_EQ(GRAPH_SUCCESS, value.SetAttrValue(AscendString("value")));
  ASSERT_EQ(GRAPH_SUCCESS, src.SetAttr(AscendString("key"), value));
  Graph copied("copy", 1);
  ASSERT_EQ(GRAPH_SUCCESS, copied.CopyFrom(src));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(copied);
  EXPECT_EQ(42U, native->GetGraphId());
  const auto inputs = ExecuteGraphUtils::GetGraphInputs(native.get());
  ASSERT_EQ(3U, inputs.size());
  ASSERT_NE(nullptr, inputs[0]);
  ASSERT_NE(nullptr, inputs[2]);
  EXPECT_EQ("second", inputs[0]->GetName());
  EXPECT_EQ(nullptr, inputs[1]);
  EXPECT_EQ("first", inputs[2]->GetName());
  AttrValue copied_value;
  ASSERT_EQ(GRAPH_SUCCESS, copied.GetAttr(AscendString("key"), copied_value));
  AscendString text;
  ASSERT_EQ(GRAPH_SUCCESS, copied_value.GetValue(text));
  EXPECT_EQ(AscendString("value"), text);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyPreservesNodeSymbolsAndHostFlag) {
  Graph src("source", 1);
  auto node = src.AddNodeByOp(MakeExecuteOp("symbols", "PlainOp", 2U, 2U));
  auto *source_info = NodeAdapter::GNode2ExecuteNode(node)->GetExtendInfo();
  source_info->SetHostNode(true);
  for (size_t i = 0U; i < 2U; ++i) {
    ASSERT_EQ(GRAPH_SUCCESS, source_info->SetInputSymbol(i, 10U + i));
    ASSERT_EQ(GRAPH_SUCCESS, source_info->SetOutputSymbol(i, 20U + i));
  }
  Graph copied("copy", 1);
  ASSERT_EQ(GRAPH_SUCCESS, copied.CopyFrom(src));
  auto copied_node = copied.FindNodeByName(AscendString("symbols"));
  ASSERT_NE(nullptr, copied_node);
  auto *copied_info = NodeAdapter::GNode2ExecuteNode(*copied_node)->GetExtendInfo();
  EXPECT_TRUE(copied_info->GetHostNode());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(copied).get(), copied_info->GetOwnerGraphBarePtr());
  for (size_t i = 0U; i < 2U; ++i) {
    EXPECT_EQ(10U + i, copied_info->GetInputSymbol(i));
    EXPECT_EQ(20U + i, copied_info->GetOutputSymbol(i));
  }
  ASSERT_EQ(GRAPH_SUCCESS, copied_info->SetOutputSymbol(0U, 100U));
  EXPECT_EQ(20U, source_info->GetOutputSymbol(0U));
}

// 执行图诊断接口留待独立方案实现。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphDumpToStream) {
  Graph graph("execute_graph", 1);
  graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 1U));
  std::stringstream readable;
  EXPECT_NE(GRAPH_SUCCESS, graph.Dump(Graph::DumpFormat::kReadable, readable));
  EXPECT_TRUE(readable.str().empty());
  EXPECT_EQ(1U, graph.GetDirectNode().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphDumpToFile) {
  Graph graph("execute_graph", 1);
  EXPECT_NE(GRAPH_SUCCESS, graph.DumpToFile(Graph::DumpFormat::kReadable, AscendString("")));
}

// 5.1：ES 构建器透传后端类别——执行图构建不调用 NormalizeEsToGe，也不产生 ComputeGraph。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsBuilderKindPassthrough) {
  {
    ge::es::EsGraphBuilder compute_builder("es_compute");
    auto input = compute_builder.CreateInput(0);
    ASSERT_NE(nullptr, input.GetCTensorHolder());
    auto graph = compute_builder.BuildAndReset({input});
    ASSERT_NE(nullptr, graph);
    EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(*graph));
    EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
  }
  {
    ge::es::EsGraphBuilder execute_builder("es_execute", 1);
    auto input = execute_builder.CreateInput(0);
    ASSERT_NE(nullptr, input.GetCTensorHolder());
    auto graph = execute_builder.BuildAndReset({input});
    ASSERT_NE(nullptr, graph);
    // 执行图类别：没有计算图，ES 的 Data 描述回填与子图搬迁被跳过。
    EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(*graph));
    EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
    EXPECT_EQ(2U, graph->GetAllNodes().size());
  }
}

// 12.0：ES 层不解释阶段属性。输入沿用一张图的全局连续编号，同序号不能重复创建。
// 阶段属性只作为普通节点属性保存，取值、分组和校验统一在 GERT 拆图入口处理。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsInputsUseGlobalIndexes) {
  {
    ge::es::EsGraphBuilder builder("es_execute_global_index", 1);
    auto load_input = builder.CreateInput(0, "load_input");
    ASSERT_NE(nullptr, load_input.GetCTensorHolder());
    // 阶段属性由 ES 原样保存，不参与输入编号判定。
    ASSERT_EQ(SUCCESS, load_input.SetAttrForNode(gert::attr::kStage, gert::attr::kStageInit));
    // 全局编号下同一个序号不能声明两次，与计算模式一致。
    auto duplicate = builder.CreateInput(0, "exec_input");
    EXPECT_EQ(nullptr, duplicate.GetCTensorHolder());
    auto exec_input = builder.CreateInput(1, "exec_input");
    ASSERT_NE(nullptr, exec_input.GetCTensorHolder());
    auto graph = builder.BuildAndReset({exec_input});
    ASSERT_NE(nullptr, graph);
    EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
    // 两个输入节点 + NetOutput。
    EXPECT_EQ(3U, graph->GetAllNodes().size());
  }
  {
    // 输入序号不从 0 开始被拒绝。
    ge::es::EsGraphBuilder builder("es_execute_bad_start", 1);
    auto load_input = builder.CreateInput(1, "load_input");
    ASSERT_NE(nullptr, load_input.GetCTensorHolder());
    EXPECT_EQ(nullptr, builder.BuildAndReset({load_input}));
  }
  {
    // 输入序号不连续被拒绝。
    ge::es::EsGraphBuilder builder("es_execute_gap", 1);
    auto input0 = builder.CreateInput(0, "input0");
    auto input2 = builder.CreateInput(2, "input2");
    ASSERT_NE(nullptr, input0.GetCTensorHolder());
    ASSERT_NE(nullptr, input2.GetCTensorHolder());
    EXPECT_EQ(nullptr, builder.BuildAndReset({input0}));
  }
}

// 12.0：自动追加的输入沿用全局序号，序号在整张图内递增。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsAppendInputUsesGlobalIndex) {
  ge::es::EsGraphBuilder builder("es_execute_append", 1);
  auto load_input = builder.CreateInput(0, "load_input");
  ASSERT_NE(nullptr, load_input.GetCTensorHolder());
  ASSERT_EQ(SUCCESS, load_input.SetAttrForNode(gert::attr::kStage, gert::attr::kStageInit));
  auto exec_input = builder.CreateInput(1, "exec_input");
  ASSERT_NE(nullptr, exec_input.GetCTensorHolder());
  auto appended = builder.GetCGraphBuilder()->AppendGraphInput("appended");
  ASSERT_NE(nullptr, appended);
  std::vector<ge::es::EsTensorHolder> outputs{load_input, exec_input, ge::es::EsTensorHolder(appended)};
  auto graph = builder.BuildAndReset(outputs);
  ASSERT_NE(nullptr, graph);
  // 三个输入节点 + NetOutput；自动追加的输入取下一个全局序号 2。
  EXPECT_EQ(4U, graph->GetAllNodes().size());
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("appended")));
}

// 5.3：类型化常量读回保持 dtype/长度；消费节点可经公共 GNode 读取常量输入。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphTypedConstReadableFromConsumer) {
  Graph graph("const_consumer", 1);
  auto const_node = graph.AddNodeByOp(MakeExecuteOp("const", "Const", 0U, 1U));
  auto consumer = graph.AddNodeByOp(MakeExecuteOp("consumer", "PlainOp", 1U, 1U));
  ASSERT_NE(nullptr, graph.FindNodeByName(AscendString("const")));
  const std::vector<int32_t> values{7, 8, 9};
  TensorDesc td(Shape({3}), FORMAT_ND, DT_INT32);
  Tensor input(td);
  ASSERT_EQ(GRAPH_SUCCESS,
            input.SetData(reinterpret_cast<const uint8_t *>(values.data()), values.size() * sizeof(int32_t)));
  ASSERT_EQ(GRAPH_SUCCESS, const_node.SetAttr(AscendString("value"), input));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(const_node, 0, consumer, 0));

  Tensor read_back;
  ASSERT_EQ(GRAPH_SUCCESS, consumer.GetInputConstData(0, read_back));
  EXPECT_EQ(DT_INT32, read_back.GetDataType());
  ASSERT_EQ(values.size() * sizeof(int32_t), read_back.GetSize());
  EXPECT_EQ(0, memcmp(values.data(), read_back.GetData(), read_back.GetSize()));

  // 非常量输入返回“无常量输入”，不返回伪造数据。
  auto plain = graph.AddNodeByOp(MakeExecuteOp("plain", "PlainOp", 0U, 1U));
  auto consumer2 = graph.AddNodeByOp(MakeExecuteOp("consumer2", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(plain, 0, consumer2, 0));
  Tensor empty;
  EXPECT_EQ(GRAPH_NODE_WITHOUT_CONST_INPUT, consumer2.GetInputConstData(0, empty));
  EXPECT_EQ(PARAM_INVALID, consumer.GetInputConstData(-1, empty));
  EXPECT_EQ(PARAM_INVALID, consumer.GetInputConstData(1, empty));
  auto unconnected = graph.AddNodeByOp(MakeExecuteOp("unconnected", "PlainOp", 1U, 1U));
  EXPECT_EQ(PARAM_INVALID, unconnected.GetInputConstData(0, empty));

  // 常量只从节点属性读取，不回退到输出端口属性。
  auto port_const = graph.AddNodeByOp(MakeExecuteOp("port_const", "Const", 0U, 1U));
  AttrValue port_value;
  ASSERT_EQ(GRAPH_SUCCESS, port_value.SetAttrValue(input));
  ASSERT_EQ(GRAPH_SUCCESS, port_const.SetOutputAttr(AscendString("value"), 0U, port_value));
  auto port_consumer = graph.AddNodeByOp(MakeExecuteOp("port_consumer", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(port_const, 0, port_consumer, 0));
  EXPECT_EQ(GRAPH_FAILED, port_consumer.GetInputConstData(0, empty));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphConstInputFollowsInnerDataIndexWithoutDepthLimit) {
  auto root = MakeExecuteSubgraph("root");
  auto source = root->AddNodeByOp(MakeExecuteOp("constant", "Const", 0U, 1U));
  const int32_t value = 42;
  Tensor tensor(TensorDesc(Shape({1}), FORMAT_ND, DT_INT32));
  ASSERT_EQ(GRAPH_SUCCESS, tensor.SetData(reinterpret_cast<const uint8_t *>(&value), sizeof(value)));
  ASSERT_EQ(GRAPH_SUCCESS, source.SetAttr(AscendString("value"), tensor));

  std::vector<GraphPtr> children;
  auto current = root;
  for (size_t i = 0U; i < 101U; ++i) {
    auto parent = current->AddNodeByOp(MakeExecuteOp("parent", "If", 2U, 1U));
    ASSERT_EQ(GRAPH_SUCCESS, current->AddDataEdge(source, 0, parent, 1));
    const auto name = std::string("level") + std::to_string(i);
    auto child = MakeExecuteSubgraph(name.c_str());
    source = child->AddNodeByOp(MakeExecuteOp("input", "InnerData", 0U, 1U));
    // InnerData 没有图输入登记；index=1 指向父节点输入 1，而不是输入 0。
    int32_t index = 1;
    ASSERT_EQ(GRAPH_SUCCESS, source.SetAttr(AscendString("index"), index));
    ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("body"), *child));
    children.emplace_back(child);
    current = child;
  }

  auto consumer = current->AddNodeByOp(MakeExecuteOp("consumer", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, current->AddDataEdge(source, 0, consumer, 0));
  Tensor read_back;
  ASSERT_EQ(GRAPH_SUCCESS, consumer.GetInputConstData(0, read_back));
  EXPECT_EQ(DT_INT32, read_back.GetDataType());
  ASSERT_EQ(sizeof(value), read_back.GetSize());
  EXPECT_EQ(0, memcmp(&value, read_back.GetData(), sizeof(value)));

  // 子图仍存活，根图销毁后停止追溯，不能解引用已销毁的父节点。
  root.reset();
  EXPECT_EQ(GRAPH_NODE_WITHOUT_CONST_INPUT, consumer.GetInputConstData(0, read_back));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphConstInputRejectsInvalidInnerDataMappingsWithoutChangingTensor) {
  Graph root("root", 1);
  auto constant = root.AddNodeByOp(MakeExecuteOp("constant", "Const", 0U, 1U));
  const int32_t value = 42;
  Tensor tensor(TensorDesc(Shape({1}), FORMAT_ND, DT_INT32));
  ASSERT_EQ(GRAPH_SUCCESS, tensor.SetData(reinterpret_cast<const uint8_t *>(&value), sizeof(value)));
  ASSERT_EQ(GRAPH_SUCCESS, constant.SetAttr(AscendString("value"), tensor));
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 2U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, root.AddDataEdge(constant, 0, parent, 1));
  auto child = MakeExecuteSubgraph("child");
  auto input = child->AddNodeByOp(MakeExecuteOp("input", "InnerData", 0U, 1U));
  auto consumer = child->AddNodeByOp(MakeExecuteOp("consumer", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, child->AddDataEdge(input, 0, consumer, 0));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("body"), *child));

  const int32_t sentinel = 7;
  Tensor read_back(TensorDesc(Shape({1}), FORMAT_ND, DT_INT32));
  ASSERT_EQ(GRAPH_SUCCESS, read_back.SetData(reinterpret_cast<const uint8_t *>(&sentinel), sizeof(sentinel)));
  const auto expect_mapping_failure = [&]() {
    EXPECT_EQ(GRAPH_NODE_WITHOUT_CONST_INPUT, consumer.GetInputConstData(0, read_back));
    EXPECT_EQ(DT_INT32, read_back.GetDataType());
    ASSERT_EQ(sizeof(sentinel), read_back.GetSize());
    ASSERT_NE(nullptr, read_back.GetData());
    EXPECT_EQ(0, memcmp(&sentinel, read_back.GetData(), sizeof(sentinel)));
  };
  expect_mapping_failure();           // 缺少 index。
  for (int32_t index : {-1, 0, 2}) {  // 负值、父输入未连接、父输入越界。
    ASSERT_EQ(GRAPH_SUCCESS, input.SetAttr(AscendString("index"), index));
    expect_mapping_failure();
  }
  int32_t index = 1;
  ASSERT_EQ(GRAPH_SUCCESS, input.SetAttr(AscendString("index"), index));
  ASSERT_EQ(GRAPH_SUCCESS, consumer.GetInputConstData(0, read_back));
  ASSERT_EQ(sizeof(value), read_back.GetSize());
  EXPECT_EQ(0, memcmp(&value, read_back.GetData(), sizeof(value)));
}

// 5.3：复制后常量载荷仍然有效，且副本与源图互不影响。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphConstSurvivesCopy) {
  Graph src("const_src", 1);
  auto const_node = src.AddNodeByOp(MakeExecuteOp("const", "Const", 0U, 1U));
  auto consumer = src.AddNodeByOp(MakeExecuteOp("consumer", "PlainOp", 1U, 1U));
  const std::vector<float> values{1.5F, 2.5F};
  TensorDesc td(Shape({2}), FORMAT_ND, DT_FLOAT);
  Tensor input(td);
  ASSERT_EQ(GRAPH_SUCCESS,
            input.SetData(reinterpret_cast<const uint8_t *>(values.data()), values.size() * sizeof(float)));
  ASSERT_EQ(GRAPH_SUCCESS, const_node.SetAttr(AscendString("value"), input));
  ASSERT_EQ(GRAPH_SUCCESS, src.AddDataEdge(const_node, 0, consumer, 0));

  Graph dst("const_dst", 1);
  ASSERT_EQ(GRAPH_SUCCESS, dst.CopyFrom(src));
  auto dst_consumer = dst.FindNodeByName(AscendString("consumer"));
  ASSERT_NE(nullptr, dst_consumer);
  Tensor read_back;
  ASSERT_EQ(GRAPH_SUCCESS, dst_consumer->GetInputConstData(0, read_back));
  ASSERT_EQ(values.size() * sizeof(float), read_back.GetSize());
  EXPECT_EQ(0, memcmp(values.data(), read_back.GetData(), read_back.GetSize()));

  // 改副本常量不改变源图读值。
  auto dst_const = dst.FindNodeByName(AscendString("const"));
  ASSERT_NE(nullptr, dst_const);
  const std::vector<float> changed{9.0F, 9.0F};
  Tensor changed_tensor(td);
  ASSERT_EQ(GRAPH_SUCCESS,
            changed_tensor.SetData(reinterpret_cast<const uint8_t *>(changed.data()), changed.size() * sizeof(float)));
  ASSERT_EQ(GRAPH_SUCCESS, dst_const->SetAttr(AscendString("value"), changed_tensor));
  Tensor src_read;
  ASSERT_EQ(GRAPH_SUCCESS, consumer.GetInputConstData(0, src_read));
  EXPECT_EQ(0, memcmp(values.data(), src_read.GetData(), src_read.GetSize()));
}

// 5.4：收尾按声明组装逻辑 NetOutput，图移交后 Builder 不可再用。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsBuildHandsOverOnce) {
  ge::es::EsGraphBuilder builder("es_execute_handover", 1);
  auto input = builder.CreateInput(0);
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  auto graph = builder.BuildAndReset({input});
  ASSERT_NE(nullptr, graph);
  // 移交后图内包含输入、逻辑 NetOutput，且输出登记指向产生的节点。
  auto net_output = graph->FindNodeByName(AscendString("NetOutput_es_execute_handover"));
  ASSERT_NE(nullptr, net_output);
  AscendString net_output_type;
  ASSERT_EQ(GRAPH_SUCCESS, net_output->GetType(net_output_type));
  EXPECT_EQ(AscendString("NetOutput"), net_output_type);
  ASSERT_EQ(1U, net_output->GetInputsSize());
  // 重复移交明确失败，且不移交第二张图。
  EXPECT_EQ(nullptr, builder.BuildAndReset({input}));
}

// 5.4/12.1：同一输出值可以占据多个槽位，输出顺序与槽位一一对应。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsAllowsRepeatedOutputValue) {
  {
    // [a, a]：同一个值同时作为 0、1 号输出。
    ge::es::EsGraphBuilder builder("es_execute_repeat_aa", 1);
    auto input = builder.CreateInput(0);
    ASSERT_NE(nullptr, input.GetCTensorHolder());
    auto graph = builder.BuildAndReset({input, input});
    ASSERT_NE(nullptr, graph);
    const auto outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(*graph).get());
    ASSERT_EQ(2U, outputs.size());
    for (const auto &output : outputs) {
      ASSERT_NE(nullptr, output.first);
      EXPECT_EQ(AscendString("input_0"), AscendString(output.first->GetName().c_str()));
      EXPECT_EQ(0, output.second);
    }
    // 逻辑 NetOutput 保留两个输入槽。
    auto net_output = graph->FindNodeByName(AscendString("NetOutput_es_execute_repeat_aa"));
    ASSERT_NE(nullptr, net_output);
    EXPECT_EQ(2U, net_output->GetInputsSize());
  }
  {
    // [a, b, a]：重复值与非重复值交错，顺序保持。
    ge::es::EsGraphBuilder builder("es_execute_repeat_aba", 1);
    auto input_a = builder.CreateInput(0, "a");
    auto input_b = builder.CreateInput(1, "b");
    ASSERT_NE(nullptr, input_a.GetCTensorHolder());
    ASSERT_NE(nullptr, input_b.GetCTensorHolder());
    auto graph = builder.BuildAndReset({input_a, input_b, input_a});
    ASSERT_NE(nullptr, graph);
    const auto outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(*graph).get());
    ASSERT_EQ(3U, outputs.size());
    EXPECT_EQ(AscendString("a"), AscendString(outputs[0].first->GetName().c_str()));
    EXPECT_EQ(AscendString("b"), AscendString(outputs[1].first->GetName().c_str()));
    EXPECT_EQ(AscendString("a"), AscendString(outputs[2].first->GetName().c_str()));
  }
  {
    // 多端口输出：同一个节点的不同端口视为不同值，不触发去重。
    Graph graph("es_execute_multi_port", 1);
    auto node = graph.AddNodeByOp(MakeExecuteOp("multi", "PlainOp", 1U, 2U));
    ASSERT_NE(nullptr, graph.FindNodeByName(AscendString("multi")));
    std::vector<std::pair<GNode, int32_t>> outputs{{node, 0}, {node, 1}, {node, 0}};
    ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs(outputs));
    const auto registered = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
    ASSERT_EQ(3U, registered.size());
    EXPECT_EQ(0, registered[0].second);
    EXPECT_EQ(1, registered[1].second);
    EXPECT_EQ(0, registered[2].second);
  }
}

// 5.4：输出登记沿用 Holder 所属构建器，重复槽位和已删除 producer 被拒绝。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsRejectsInvalidOutputs) {
  {
    // 输出登记跟随 Holder 所属构建器：用 A 的 Holder 经 B 登记，实际记到 A。
    ge::es::EsGraphBuilder builder("es_execute_owner_a", 1);
    ge::es::EsGraphBuilder other("es_execute_owner_b", 1);
    auto input = builder.CreateInput(0);
    ASSERT_NE(nullptr, input.GetCTensorHolder());
    ASSERT_EQ(SUCCESS, other.SetOutput(input, 0));
    // A 有 0 号输出可收尾；B 没有输出登记，收尾得到没有输出的图。
    auto graph_a = builder.BuildAndReset();
    ASSERT_NE(nullptr, graph_a);
    EXPECT_NE(nullptr, graph_a->FindNodeByName(AscendString("NetOutput_es_execute_owner_a")));
    auto graph_b = other.BuildAndReset();
    ASSERT_NE(nullptr, graph_b);
    EXPECT_EQ(1U, graph_b->GetAllNodes().size());
  }
  {
    // 槽位冲突：两个值抢同一槽位时后一次登记被拒绝，不影响后续收尾。
    ge::es::EsGraphBuilder builder("es_execute_slot_conflict", 1);
    auto input = builder.CreateInput(0);
    ASSERT_NE(nullptr, input.GetCTensorHolder());
    ASSERT_EQ(SUCCESS, builder.SetOutput(input, 0));
    EXPECT_NE(SUCCESS, builder.SetOutput(input, 0));
    auto graph = builder.BuildAndReset();
    ASSERT_NE(nullptr, graph);
    EXPECT_EQ(1U, ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(*graph).get()).size());
  }
  {
    // 已删除 producer：后端连边失败，收尾返回空，Builder 仍持有工作图。
    ge::es::EsGraphBuilder builder("es_execute_deleted_producer", 1);
    auto input0 = builder.CreateInput(0);
    ASSERT_NE(nullptr, input0.GetCTensorHolder());
    ASSERT_EQ(SUCCESS, builder.SetOutput(input0, 0));
    auto removed_node = input0.GetCTensorHolder()->GetProducer();
    ASSERT_EQ(GRAPH_SUCCESS, builder.GetCGraphBuilder()->GetGraph()->RemoveNode(removed_node));
    EXPECT_EQ(nullptr, builder.BuildAndReset());
    EXPECT_NE(nullptr, builder.GetCGraphBuilder()->GetGraph());
  }
}

// 5.4：输入输出序号校验在改图前执行，补齐序号后可继续收尾。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsIndexValidationFailureKeepsGraph) {
  ge::es::EsGraphBuilder builder("es_execute_retry", 1);
  auto input0 = builder.CreateInput(0);
  auto input1 = builder.CreateInput(1);
  ASSERT_NE(nullptr, input0.GetCTensorHolder());
  ASSERT_NE(nullptr, input1.GetCTensorHolder());
  // 输出序号不从 0 开始：收尾失败。
  ASSERT_EQ(SUCCESS, builder.SetOutput(input1, 1));
  EXPECT_EQ(nullptr, builder.BuildAndReset());
  // 原图未被销毁：节点仍可查询，Builder 仍可继续编辑。
  EXPECT_NE(nullptr, builder.GetCGraphBuilder()->GetGraph()->FindNodeByName(AscendString("input_0")));
  EXPECT_NE(nullptr, builder.GetCGraphBuilder()->GetGraph()->FindNodeByName(AscendString("input_1")));
  // 补齐 0 号输出后重新收尾成功。
  ASSERT_EQ(SUCCESS, builder.SetOutput(input0, 0));
  auto graph = builder.BuildAndReset();
  ASSERT_NE(nullptr, graph);
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("NetOutput_es_execute_retry")));
}

// 5.1：非法类别在 C 入口被拒绝，不产生构建器。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsBuilderRejectsInvalidKind) {
  auto *invalid = EsCreateGraphBuilderWithKind("bad", 9);
  EXPECT_EQ(nullptr, invalid);
  // 原 C 入口保持默认计算图语义。
  auto *legacy = EsCreateGraphBuilder("legacy");
  ASSERT_NE(nullptr, legacy);
  EXPECT_NE(nullptr, legacy->GetGraph());
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(*legacy->GetGraph()));
  EsDestroyGraphBuilder(legacy);
}

// 12.18.2：三级子树先独立建好再整体挂到根上，逐层登记与父子关系都成立。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountPrebuiltThreeLevelTree) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto level1 = MakeExecuteSubgraph("level1");
  auto level1_if = level1->AddNodeByOp(MakeExecuteOp("level1_if", "IfOp", 1U, 1U));
  auto level2 = MakeExecuteSubgraph("level2");
  auto level2_if = level2->AddNodeByOp(MakeExecuteOp("level2_if", "IfOp", 1U, 1U));
  auto level3 = MakeExecuteSubgraph("level3");
  level3->AddNodeByOp(MakeExecuteOp("deep_op", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, level2_if.SetSubgraph(AscendString("then"), *level3));
  ASSERT_EQ(GRAPH_SUCCESS, level1_if.SetSubgraph(AscendString("then"), *level2));

  // 三级树整体挂载：不能被"后代直接父图必须是待挂子图"的旧约束拒绝。
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *level1));

  // 逐层父子关系成立：每层的父图是上一层的实际对象。
  auto *root_exec = GraphUtilsEx::Graph2ExecuteGraph(root).get();
  auto *level1_exec = GraphUtilsEx::Graph2ExecuteGraph(*level1).get();
  auto *level2_exec = GraphUtilsEx::Graph2ExecuteGraph(*level2).get();
  auto *level3_exec = GraphUtilsEx::Graph2ExecuteGraph(*level3).get();
  ASSERT_NE(nullptr, root_exec);
  ASSERT_NE(nullptr, level1_exec);
  ASSERT_NE(nullptr, level2_exec);
  ASSERT_NE(nullptr, level3_exec);
  EXPECT_EQ(root_exec, level1_exec->GetParentGraphBarePtr());
  EXPECT_EQ(level1_exec, level2_exec->GetParentGraphBarePtr());
  EXPECT_EQ(level2_exec, level3_exec->GetParentGraphBarePtr());

  // 按父节点槽位查询各层，与调用方持有的对象一致。
  ExpectExecuteSubgraphAtSlot(parent, level1_exec);
  ExpectExecuteSubgraphAtSlot(level1_if, level2_exec);
  ExpectExecuteSubgraphAtSlot(level2_if, level3_exec);

  // 从根按名称逐层查得到；深层节点在递归枚举里只出现一次，owner 不分叉。
  auto level1_by_name = root.GetSubGraph("level1");
  auto level2_by_name = root.GetSubGraph("level2");
  auto level3_by_name = root.GetSubGraph("level3");
  ASSERT_NE(nullptr, level1_by_name);
  ASSERT_NE(nullptr, level2_by_name);
  ASSERT_NE(nullptr, level3_by_name);
  EXPECT_EQ(level3_exec, GraphUtilsEx::Graph2ExecuteGraph(*level3_by_name).get());
  size_t deep_count = 0U;
  for (auto &node : root.GetAllNodes()) {
    AscendString node_name;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetName(node_name));
    if (std::string(node_name.GetString()) == "deep_op") {
      ++deep_count;
    }
  }
  EXPECT_EQ(1U, deep_count);
}

// 深层名称冲突在逐项登记时发现，保留前面已经完成的根登记。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountDeepNameConflictPreservesCompletedRegistrations) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("if", "IfOp", 1U, 1U));
  auto taken = MakeExecuteSubgraph("taken");
  taken->AddNodeByOp(MakeExecuteOp("taken_inner", "PlainOp", 0U, 1U));
  auto taken_parent = root.AddNodeByOp(MakeExecuteOp("taken_parent", "If", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, taken_parent.SetSubgraph(AscendString("body"), *taken));

  auto level1 = MakeExecuteSubgraph("conflict_l1");
  auto level1_if = level1->AddNodeByOp(MakeExecuteOp("conflict_if", "IfOp", 1U, 1U));
  auto level2 = MakeExecuteSubgraph("taken");
  level2->AddNodeByOp(MakeExecuteOp("level2_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, level1_if.SetSubgraph(AscendString("then"), *level2));

  // 顶层先登记，随后孙层重名失败；父关系在整棵子树登记成功后更新。
  EXPECT_NE(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *level1));
  EXPECT_EQ(nullptr, level1->GetParentGraph());
  EXPECT_EQ(nullptr, level1->GetParentNode());
  EXPECT_NE(nullptr, root.GetSubGraph("conflict_l1"));
  GraphPtr registered_slot;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(0U, registered_slot));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*level1), GraphUtilsEx::Graph2ExecuteGraph(*registered_slot));
  auto kept = root.GetSubGraph("taken");
  ASSERT_NE(nullptr, kept);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*taken).get(), GraphUtilsEx::Graph2ExecuteGraph(*kept).get());

  // 改掉冲突层名称后，复用已完成的登记并继续挂载。
  auto renamed = MakeExecuteSubgraph("level2_ok");
  renamed->AddNodeByOp(MakeExecuteOp("level2_ok_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, level1->RemoveSubgraph("taken"));
  ExecuteNodeBackend::GetDesc(level1_if)->RemoveSubgraphInstanceName("taken");
  EXPECT_EQ(GRAPH_SUCCESS, level1_if.SetSubgraph(AscendString("then"), *renamed));
  EXPECT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *level1));
  EXPECT_NE(nullptr, root.GetSubGraph("level2_ok"));
}

// 批次不预检去重；重复对象可以占多个槽位，不同对象重名由实际登记拒绝。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphMountSubgraphsReusesObjectsAndSlots) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("case", "CaseOp", 1U, 1U));
  auto branch = MakeExecuteSubgraph("same_branch");
  branch->AddNodeByOp(MakeExecuteOp("same_branch_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("branches"), {*branch, *branch}));
  GraphPtr first;
  GraphPtr second;
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(0U, first));
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(1U, second));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch), GraphUtilsEx::Graph2ExecuteGraph(*first));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch), GraphUtilsEx::Graph2ExecuteGraph(*second));
  EXPECT_TRUE(ExecuteNodeBackend::GetDesc(parent)->GetSubgraphIrNames().empty());

  auto twin = MakeExecuteSubgraph("same_branch");
  twin->AddNodeByOp(MakeExecuteOp("twin_inner", "PlainOp", 0U, 1U));
  EXPECT_NE(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("branches"), {*branch, *twin}));
  EXPECT_EQ(nullptr, twin->GetParentGraph());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*branch),
            GraphUtilsEx::Graph2ExecuteGraph(*root.GetSubGraph("same_branch")));

  auto replacement = MakeExecuteSubgraph("replacement");
  replacement->AddNodeByOp(MakeExecuteOp("replacement_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("branches1"), *replacement));
  ASSERT_EQ(GRAPH_SUCCESS, parent.GetSubgraph(1U, second));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*replacement), GraphUtilsEx::Graph2ExecuteGraph(*second));
  EXPECT_TRUE(ExecuteNodeBackend::GetDesc(parent)->GetSubgraphIrNames().empty());
  EXPECT_EQ(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("empty"), {}));
  EXPECT_EQ(2U, ExecuteNodeBackend::GetDesc(parent)->GetSubgraphNameIndexes().size());
}

// 删除外层登记不影响后代登记、父子关系和节点句柄。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemoveOuterRegistrationPreservesDescendantsAndHandles) {
  Graph root("root", 1);
  auto outer = root.AddNodeByOp(MakeExecuteOp("outer", "If", 0U, 1U));
  auto a = MakeExecuteSubgraph("a");
  auto a_if = a->AddNodeByOp(MakeExecuteOp("a_if", "If", 0U, 1U));
  auto b = MakeExecuteSubgraph("b");
  auto b_if = b->AddNodeByOp(MakeExecuteOp("b_if", "If", 0U, 1U));
  auto c = MakeExecuteSubgraph("c");
  auto leaf = c->AddNodeByOp(MakeExecuteOp("leaf", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, b_if.SetSubgraph(AscendString("then"), *c));
  ASSERT_EQ(GRAPH_SUCCESS, a_if.SetSubgraph(AscendString("then"), *b));
  ASSERT_EQ(GRAPH_SUCCESS, outer.SetSubgraph(AscendString("then"), *a));
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("a"));
  EXPECT_EQ(nullptr, root.GetSubGraph("a"));
  EXPECT_NE(nullptr, root.GetSubGraph("b"));
  EXPECT_NE(nullptr, root.GetSubGraph("c"));
  EXPECT_EQ(2U, root.GetAllSubgraphs().size());
  EXPECT_TRUE(a->GetAllSubgraphs().empty());
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, a_if.GetName(name));
  EXPECT_EQ(GRAPH_SUCCESS, b_if.GetName(name));
  EXPECT_EQ(GRAPH_SUCCESS, leaf.GetName(name));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(root).get(),
            GraphUtilsEx::Graph2ExecuteGraph(*a)->GetParentGraphBarePtr());
  GraphPtr nested;
  ASSERT_EQ(GRAPH_SUCCESS, a_if.GetSubgraph(0U, nested));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*b), GraphUtilsEx::Graph2ExecuteGraph(*nested));
  ASSERT_EQ(GRAPH_SUCCESS, root.AddSubGraph(*a));
  ASSERT_EQ(GRAPH_SUCCESS, outer.GetSubgraph(0U, nested));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*a), GraphUtilsEx::Graph2ExecuteGraph(*nested));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRenameAllNodesRejectsExecuteGraph) {
  Graph graph("rename_execute", 1);
  (void)graph.AddNodeByOp(MakeExecuteOp("keep_name", "PlainOp", 0U, 1U));
  GeApiWrapper_RenameAllNodes(&graph, "prefix");
  // 执行图不参与重命名，节点名保持原样。
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("keep_name")));
  EXPECT_EQ(nullptr, graph.FindNodeByName(AscendString("prefix_keep_name")));
}

// 12.17：图类别取值与 metadef c_types.h 中 C_GraphKind 枚举一致：0 为计算图，1 为执行图。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphKindMatchesCEquivalent) {
  // 两条公开构造路径按同一数值建出同一类别的图。
  Graph compute_graph("kind_compute", 0);
  EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(compute_graph));
  Graph execute_graph("kind_execute", 1);
  EXPECT_TRUE(GraphUtilsEx::IsExecuteGraph(execute_graph));
  EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(execute_graph));
}

// 12.17：提取和包装底层执行图，不对工具接口暴露 Backend。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphWrapperPreservesUnderlyingGraph) {
  Graph execute_graph("wrapped_execute", 1);
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.SetValid());
  const auto native_graph = GraphUtilsEx::Graph2ExecuteGraph(execute_graph);
  ASSERT_NE(nullptr, native_graph);
  auto wrapped = GraphUtilsEx::ExecuteGraph2Graph(native_graph);
  ASSERT_NE(nullptr, wrapped);
  EXPECT_TRUE(wrapped->IsValid());
  EXPECT_EQ(native_graph, GraphUtilsEx::Graph2ExecuteGraph(*wrapped));
  EXPECT_EQ(nullptr, GraphUtilsEx::ExecuteGraph2Graph(nullptr));

  Graph compute_graph("wrapped_compute");
  compute_graph.AddNodeByOp(MakeExecuteOp("plain", "PlainOp", 0U, 1U));
  EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(compute_graph));
}

// 12.17：门面包装同一底层图，重复查询子图得到的节点归属不变。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphFacadeWrapperKeepsGraphIdentity) {
  Graph root("wrap_root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("wrap_if", "IfOp", 1U, 1U));
  auto sub = MakeExecuteSubgraph("wrap_sub");
  auto inner = sub->AddNodeByOp(MakeExecuteOp("wrap_inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), *sub));

  // 经根图按名称取到的子图门面与原对象包装同一底层图。
  auto wrapped = root.GetSubGraph("wrap_sub");
  ASSERT_NE(nullptr, wrapped);
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*sub).get(), GraphUtilsEx::Graph2ExecuteGraph(*wrapped).get());

  // 不同包装按底层图共享节点引用，因此节点句柄可继续用于改图。
  auto wrapped_inner = wrapped->FindNodeByName(AscendString("wrap_inner"));
  ASSERT_NE(nullptr, wrapped_inner);
  AscendString inner_name;
  ASSERT_EQ(GRAPH_SUCCESS, wrapped_inner->GetName(inner_name));
  EXPECT_EQ(AscendString("wrap_inner"), inner_name);
  EXPECT_EQ(GRAPH_SUCCESS, wrapped->RemoveNode(*wrapped_inner, false));
  // 删除的是同一份后端数据：原对象也查不到该节点，旧句柄随之失效。
  EXPECT_EQ(nullptr, sub->FindNodeByName(AscendString("wrap_inner")));
  EXPECT_NE(GRAPH_SUCCESS, inner.GetName(inner_name));
}

// 预构子树整体挂载后，底层查询、枚举和解除必须使用同一份根登记。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphPrebuiltTreeUsesRootRegistry) {
  Graph root("root", 1);
  auto outer = root.AddNodeByOp(MakeExecuteOp("outer", "IfOp", 0U, 1U));
  auto a = MakeExecuteSubgraph("a");
  auto a_if = a->AddNodeByOp(MakeExecuteOp("a_if", "IfOp", 0U, 1U));
  auto b = MakeExecuteSubgraph("b");
  auto b_if = b->AddNodeByOp(MakeExecuteOp("b_if", "IfOp", 0U, 1U));
  auto c = MakeExecuteSubgraph("c");
  ASSERT_EQ(GRAPH_SUCCESS, b_if.SetSubgraph(AscendString("then"), *c));
  ASSERT_EQ(GRAPH_SUCCESS, a_if.SetSubgraph(AscendString("then"), *b));
  ASSERT_EQ(GRAPH_SUCCESS, outer.SetSubgraph(AscendString("then"), *a));

  auto native_root = GraphUtilsEx::Graph2ExecuteGraph(root);
  auto native_a = GraphUtilsEx::Graph2ExecuteGraph(*a);
  auto native_b = GraphUtilsEx::Graph2ExecuteGraph(*b);
  auto native_c = GraphUtilsEx::Graph2ExecuteGraph(*c);
  EXPECT_EQ(3U, native_root->GetAllSubgraphs().size());
  EXPECT_TRUE(native_a->GetAllSubgraphs().empty());
  EXPECT_TRUE(native_b->GetAllSubgraphs().empty());
  EXPECT_EQ(native_c.get(), native_root->GetSubGraph("c"));
  EXPECT_TRUE(a->GetAllSubgraphs().empty());
  EXPECT_TRUE(b->GetAllSubgraphs().empty());
  const auto root_subgraphs = root.GetAllSubgraphs();
  const auto native_subgraphs = native_root->GetAllSubgraphs();
  ASSERT_EQ(native_subgraphs.size(), root_subgraphs.size());
  for (size_t i = 0U; i < root_subgraphs.size(); ++i) {
    ASSERT_NE(nullptr, root_subgraphs[i]);
    EXPECT_EQ(native_subgraphs[i], GraphUtilsEx::Graph2ExecuteGraph(*root_subgraphs[i]).get());
  }
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("b"));
  EXPECT_EQ(nullptr, native_root->GetSubGraph("b"));
  EXPECT_EQ(native_c.get(), native_root->GetSubGraph("c"));
  EXPECT_EQ(native_c.get(), native_b->GetSubGraph("c"));
  EXPECT_EQ(native_b.get(), native_c->GetParentGraphBarePtr());
  EXPECT_TRUE(b->GetAllSubgraphs().empty());
  EXPECT_EQ(native_a.get(), native_b->GetParentGraphBarePtr());
  EXPECT_EQ("b", ExecuteNodeBackend::GetDesc(a_if)->GetSubgraphInstanceName(0U));
  EXPECT_TRUE(c->GetAllSubgraphs().empty());
  ASSERT_EQ(2U, root.GetAllSubgraphs().size());
  GraphPtr old_slot;
  EXPECT_NE(GRAPH_SUCCESS, a_if.GetSubgraph(0U, old_slot));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGetSubGraphUsesRootRegistryFromAnyLevel) {
  Graph root("root", 1);
  auto left = MountExecuteSubgraph(root, MakeExecuteOp("left_parent", "If", 0U, 1U), "then", "left");
  auto right = MountExecuteSubgraph(root, MakeExecuteOp("right_parent", "If", 0U, 1U), "then", "right");
  ASSERT_NE(nullptr, left);
  ASSERT_NE(nullptr, right);
  auto nested = MountExecuteSubgraph(*left, MakeExecuteOp("inner_parent", "If", 0U, 1U), "body", "nested");
  ASSERT_NE(nullptr, nested);
  for (const auto *scope : std::vector<const Graph *>{&root, left.get(), right.get(), nested.get()}) {
    for (const auto &expected : std::vector<GraphPtr>{left, right, nested}) {
      const auto found = scope->GetSubGraph(expected->GetName().c_str());
      ASSERT_NE(nullptr, found);
      EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*expected), GraphUtilsEx::Graph2ExecuteGraph(*found));
    }
    EXPECT_EQ(nullptr, scope->GetSubGraph("missing"));
  }
  Graph unrelated("unrelated", 1);
  ASSERT_EQ(GRAPH_SUCCESS, unrelated.SetValid());
  EXPECT_EQ(nullptr, unrelated.GetSubGraph("left"));
}

// 删除登记不迁移后代所有权；根图释放后，未被外部持有的后代正常释放。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphRemovedRegistrationDoesNotTransferDescendantOwnership) {
  GraphPtr removed;
  std::weak_ptr<ExecuteGraph> weak_a;
  std::weak_ptr<ExecuteGraph> weak_b;
  {
    Graph root("root", 1);
    auto a = MountExecuteSubgraph(root, MakeExecuteOp("outer", "If", 0U, 1U), "then", "a");
    ASSERT_NE(nullptr, a);
    auto b = MountExecuteSubgraph(*a, MakeExecuteOp("inner", "If", 0U, 1U), "then", "b");
    ASSERT_NE(nullptr, b);
    weak_a = GraphUtilsEx::Graph2ExecuteGraph(*a);
    weak_b = GraphUtilsEx::Graph2ExecuteGraph(*b);
    removed = root.GetSubGraph("a");
    ASSERT_NE(nullptr, removed);
    ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("a"));
    EXPECT_NE(nullptr, root.GetSubGraph("b"));
    EXPECT_TRUE(removed->GetAllSubgraphs().empty());
  }
  EXPECT_FALSE(weak_a.expired());
  EXPECT_TRUE(weak_b.expired());
  EXPECT_EQ(nullptr, removed->GetParentGraph());
  EXPECT_EQ(nullptr, removed->GetSubGraph("b"));
  removed.reset();
  EXPECT_TRUE(weak_a.expired());
}

// 不同待挂子树的后代重名时，保留已完成挂载和登记，修复名称后可以重试。
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphBatchPreservesProgressOnNamesAcrossSubtrees) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "IfOp", 0U, 1U));
  auto a = MakeExecuteSubgraph("a");
  auto b = MakeExecuteSubgraph("b");
  auto x1 = MountExecuteSubgraph(*a, MakeExecuteOp("a_if", "IfOp", 0U, 1U), "then", "x");
  auto x2 = MountExecuteSubgraph(*b, MakeExecuteOp("b_if", "IfOp", 0U, 1U), "then", "x");
  ASSERT_NE(nullptr, x1);
  ASSERT_NE(nullptr, x2);
  EXPECT_NE(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("branches"), {*a, *b}));
  const auto desc = ExecuteNodeBackend::GetDesc(parent);
  ASSERT_NE(nullptr, desc);
  EXPECT_EQ(2U, desc->GetSubgraphNameIndexes().size());
  EXPECT_TRUE(desc->GetSubgraphIrNames().empty());
  EXPECT_EQ(3U, GraphUtilsEx::Graph2ExecuteGraph(root)->GetAllSubgraphs().size());
  EXPECT_NE(nullptr, a->GetParentGraph());
  EXPECT_EQ(nullptr, b->GetParentGraph());
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*x1).get(), GraphUtilsEx::Graph2ExecuteGraph(root)->GetSubGraph("x"));
  EXPECT_EQ(GraphUtilsEx::Graph2ExecuteGraph(*x2).get(), GraphUtilsEx::Graph2ExecuteGraph(*b)->GetSubGraph("x"));
  ASSERT_EQ(GRAPH_SUCCESS, b->RemoveSubgraph("x"));
  ExecuteNodeBackend::GetDesc(*b->FindNodeByName(AscendString("b_if")))->RemoveSubgraphInstanceName("x");
  auto unique = MountExecuteSubgraph(*b, MakeExecuteOp("b_other", "IfOp", 0U, 1U), "then", "unique");
  ASSERT_NE(nullptr, unique);
  EXPECT_EQ(GRAPH_SUCCESS, parent.SetSubgraphs(AscendString("branches"), {*a, *b}));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplAddSubGraphPreservesRegistrationAndFailureSemantics) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 0U, 1U));
  Graph child("child", 1);
  child.AddNodeByOp(MakeExecuteOp("inner", "PlainOp", 0U, 1U));

  auto native_child = GraphUtilsEx::Graph2ExecuteGraph(child);
  ExecuteGraphBackendUtils::SetParentGraph(native_child.get(), GraphUtilsEx::Graph2ExecuteGraph(root));
  native_child->SetParentNode(NodeAdapter::GNode2ExecuteNode(parent));

  GraphImpl &impl = *root.impl_;
  ASSERT_EQ(GRAPH_SUCCESS, impl.AddSubGraph(child));
  ASSERT_EQ(1U, root.GetAllSubgraphs().size());
  ASSERT_NE(nullptr, root.GetSubGraph("child"));
  EXPECT_EQ(GRAPH_FAILED, impl.AddSubGraph(child));
  EXPECT_EQ(1U, root.GetAllSubgraphs().size());

  Graph wrong_kind("wrong_kind", 0);
  ASSERT_EQ(GRAPH_SUCCESS, wrong_kind.SetValid());
  EXPECT_NE(GRAPH_SUCCESS, impl.AddSubGraph(wrong_kind));
  EXPECT_NE(GRAPH_SUCCESS, wrong_kind.impl_->AddSubGraph(root));
  Graph invalid;
  EXPECT_NE(GRAPH_SUCCESS, impl.AddSubGraph(invalid));
  Graph missing_parent("missing_parent", 1);
  ASSERT_EQ(GRAPH_SUCCESS, missing_parent.SetValid());
  EXPECT_EQ(GRAPH_FAILED, impl.AddSubGraph(missing_parent));
  EXPECT_EQ(1U, root.GetAllSubgraphs().size());
  EXPECT_EQ(nullptr, root.GetSubGraph("missing_parent"));
  ASSERT_NE(nullptr, child.GetParentGraph());
  ASSERT_NE(nullptr, child.GetParentNode());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphAddSubGraphRegistersOnlyPreparedSubgraph) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 0U, 1U));
  auto a = MakeExecuteSubgraph("a");
  auto b = MountExecuteSubgraph(*a, MakeExecuteOp("a_if", "IfOp", 0U, 1U), "then", "b");
  ASSERT_NE(nullptr, b);
  auto c = MountExecuteSubgraph(*b, MakeExecuteOp("b_if", "IfOp", 0U, 1U), "then", "c");
  ASSERT_NE(nullptr, c);
  auto native_root = GraphUtilsEx::Graph2ExecuteGraph(root);
  auto native_a = GraphUtilsEx::Graph2ExecuteGraph(*a);
  EXPECT_EQ(GRAPH_FAILED, root.AddSubGraph(*a));
  EXPECT_TRUE(native_root->GetAllSubgraphs().empty());
  EXPECT_EQ(nullptr, a->GetParentGraph());
  ExecuteGraphBackendUtils::SetParentGraph(native_a.get(), native_root);
  EXPECT_EQ(GRAPH_FAILED, root.AddSubGraph(*a));
  Graph wrong_owner("wrong_owner", 1);
  auto wrong_parent = wrong_owner.AddNodeByOp(MakeExecuteOp("wrong_parent", "If", 0U, 1U));
  native_a->SetParentNode(NodeAdapter::GNode2ExecuteNode(wrong_parent));
  EXPECT_EQ(GRAPH_FAILED, root.AddSubGraph(*a));
  EXPECT_TRUE(native_root->GetAllSubgraphs().empty());
  auto *native_parent = NodeAdapter::GNode2ExecuteNode(parent);
  native_a->SetParentNode(native_parent);
  ASSERT_EQ(GRAPH_SUCCESS, root.AddSubGraph(*a));
  const auto registered = root.GetAllSubgraphs();
  ASSERT_EQ(1U, registered.size());
  EXPECT_EQ(native_a, GraphUtilsEx::Graph2ExecuteGraph(*registered[0]));
  // Registration does not migrate descendants or populate the parent's subgraph slots.
  EXPECT_EQ(2U, native_a->GetAllSubgraphs().size());
  EXPECT_EQ(native_root.get(), native_a->GetParentGraphBarePtr());
  EXPECT_EQ(native_parent, native_a->GetParentNodeBarePtr());
  EXPECT_TRUE(native_parent->GetOpDescBarePtr()->GetSubgraphInstanceNames().empty());
  EXPECT_EQ(GRAPH_FAILED, root.AddSubGraph(*a));
  EXPECT_EQ(1U, root.GetAllSubgraphs().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphBackendNameAndOperatorOutputs) {
  Graph graph("backend_name", 1);
  EXPECT_EQ("backend_name", graph.GetName());
  auto op = MakeNamedExecuteOp("producer", "PlainOp", {"left", "right"}, {});
  graph.AddNodeByOp(op);
  std::vector<std::pair<Operator, std::vector<size_t>>> outputs{{op, {1, 0, 1}}};
  graph.SetOutputs(outputs);
  const auto registered = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(3U, registered.size());
  EXPECT_EQ(1, registered[0].second);
  EXPECT_EQ(0, registered[1].second);
  EXPECT_EQ(1, registered[2].second);
  EXPECT_EQ(3U, registered[0].first->GetExtendInfo()->GetOutputIndex().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteNodeConversionUsesOwnerGraphAndSharesReference) {
  EXPECT_EQ(nullptr, NodeAdapter::ExecuteNode2GNode(nullptr));
  auto native_graph = std::make_shared<ExecuteGraph>("owner");
  auto *node = native_graph->AddNode(std::make_shared<OpDesc>("node", "PlainOp"));
  ASSERT_NE(nullptr, node);
  auto first = NodeAdapter::ExecuteNode2GNode(node);
  auto second = NodeAdapter::ExecuteNode2GNode(node);
  ASSERT_NE(nullptr, first);
  ASSERT_NE(nullptr, second);
  const auto ref = NodeAdapter::GNode2ExecuteNode(*first);
  ASSERT_NE(nullptr, ref);
  EXPECT_EQ(native_graph.get(), ref->GetExtendInfo()->GetOwnerGraphBarePtr());
  EXPECT_EQ(ref, NodeAdapter::GNode2ExecuteNode(*second));

  auto graph = GraphUtilsEx::ExecuteGraph2Graph(native_graph);
  ASSERT_NE(nullptr, graph);
  auto queried = graph->FindNodeByName(AscendString("node"));
  ASSERT_NE(nullptr, queried);
  EXPECT_EQ(ref, NodeAdapter::GNode2ExecuteNode(*queried));
  ASSERT_EQ(GRAPH_SUCCESS, graph->RemoveNode(*first));
  EXPECT_EQ(nullptr, NodeAdapter::GNode2ExecuteNode(*second));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNodeSurvivesTemporaryFacade) {
  Graph graph("root", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 1U));
  auto facade = GraphUtilsEx::ExecuteGraph2Graph(GraphUtilsEx::Graph2ExecuteGraph(graph));
  ASSERT_NE(nullptr, facade);
  auto queried = facade->FindNodeByName(AscendString("node"));
  ASSERT_NE(nullptr, queried);
  facade.reset();
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, queried->GetName(name));
  EXPECT_EQ(GRAPH_SUCCESS, graph.RemoveNode(*queried));
  EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphOutputOverloadsUseUnderlyingGraph) {
  Graph graph("outputs", 1);
  auto op = MakeNamedExecuteOp("producer", "PlainOp", {"left", "right"}, {});
  auto node = graph.AddNodeByOp(op);
  auto backend = std::make_shared<ExecuteGraphBackend>(GraphUtilsEx::Graph2ExecuteGraph(graph));
  ASSERT_EQ(GRAPH_SUCCESS, backend->SetOutputs(std::vector<Operator>{op}));
  EXPECT_EQ(2U, ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get()).size());
  graph.SetOutputs(std::vector<std::pair<Operator, std::string>>{{op, "right"}});
  auto outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(1U, outputs.size());
  EXPECT_EQ(1, outputs[0].second);
  graph.SetOutputs(std::vector<std::pair<Operator, AscendString>>{{op, AscendString("left")}});
  outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(1U, outputs.size());
  EXPECT_EQ(0, outputs[0].second);
  // Named ports fail atomically; index overloads skip missing nodes/ports like ComputeGraph.
  EXPECT_NE(GRAPH_SUCCESS, backend->SetOutputs(std::vector<std::pair<Operator, std::string>>{{op, "missing"}}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get()));
  ASSERT_EQ(GRAPH_SUCCESS,
            backend->SetOutputs(std::vector<std::pair<Operator, std::vector<size_t>>>{{op, {1, 99, 1}}}));
  outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(2U, outputs.size());
  EXPECT_EQ(outputs[0], outputs[1]);
  EXPECT_EQ(2U, outputs[0].first->GetExtendInfo()->GetOutputIndex().size());
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(node));
  outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get());
  ASSERT_EQ(2U, outputs.size());
  for (const auto &output : outputs) {
    EXPECT_EQ(nullptr, output.first);
    EXPECT_EQ(-1, output.second);
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphImplSetOutputsSharesSelectionRules) {
  using IndexedOutputs = std::vector<std::pair<Operator, std::vector<size_t>>>;
  using NamedOutputs = std::vector<std::pair<Operator, std::string>>;
  using AscendNamedOutputs = std::vector<std::pair<Operator, AscendString>>;
  using NodeOutputs = std::vector<std::pair<GNode, int32_t>>;
  Graph graph("outputs", 1);
  auto op = MakeNamedExecuteOp("producer", "PlainOp", {"left", "middle", "right"}, {});
  auto node = graph.AddNodeByOp(op);
  auto alias = MakeExecuteOp("producer", "OtherType", 0U, 0U);
  auto missing = MakeExecuteOp("missing", "PlainOp", 0U, 1U);
  GraphImpl &impl = *graph.impl_;
  const auto ports = [&graph]() {
    std::vector<int32_t> result;

    for (const auto &output : ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get())) {
      result.emplace_back(output.second);
    }

    return result;
  };
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetOutputs(std::vector<Operator>{alias}));
  EXPECT_EQ((std::vector<int32_t>{0, 1, 2}), ports());
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetOutputs(IndexedOutputs{{alias, {2, 99, 2}}, {missing, {0}}}));
  EXPECT_EQ((std::vector<int32_t>{2, 2}), ports());
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetOutputs(NamedOutputs{{alias, "middle"}, {alias, ""}}));
  EXPECT_EQ((std::vector<int32_t>{1, 0, 1, 2}), ports());
  ASSERT_EQ(GRAPH_SUCCESS,
            impl.SetOutputs(AscendNamedOutputs{{alias, AscendString("right")}, {alias, AscendString("")}}));
  EXPECT_EQ((std::vector<int32_t>{2, 0, 1, 2}), ports());
  EXPECT_EQ(GRAPH_SUCCESS, impl.SetOutputs(std::vector<Operator>{}));
  EXPECT_EQ(GRAPH_SUCCESS, impl.SetOutputs(IndexedOutputs{}));
  EXPECT_EQ(GRAPH_SUCCESS, impl.SetOutputs(NamedOutputs{}));
  EXPECT_EQ(GRAPH_SUCCESS, impl.SetOutputs(AscendNamedOutputs{}));
  EXPECT_EQ((std::vector<int32_t>{2, 0, 1, 2}), ports());
  EXPECT_EQ(GRAPH_FAILED, impl.SetOutputs(NamedOutputs{{alias, "left"}, {missing, "left"}}));
  EXPECT_EQ((std::vector<int32_t>{2, 0, 1, 2}), ports());
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetOutputs(NodeOutputs{{node, 0}, {node, 2}}));
  EXPECT_EQ((std::vector<int32_t>{0, 2}), ports());
  ASSERT_EQ(GRAPH_SUCCESS, impl.SetOutputs(NodeOutputs{}));
  EXPECT_TRUE(ports().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphOperatorOutputsRejectInvalidGraphBeforeCheckingEmptyList) {
  ExecuteGraphBackend backend("invalid");
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(std::vector<Operator>{}));
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(std::vector<Operator>{MakeExecuteOp("output", "PlainOp", 0U, 1U)}));
  EXPECT_FALSE(backend.IsValid());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEmptyOperatorOutputsKeepExistingOutputs) {
  Graph graph("outputs", 1);
  auto op = MakeExecuteOp("producer", "PlainOp", 0U, 2U);
  graph.AddNodeByOp(op);
  auto native_graph = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ExecuteGraphBackend backend(native_graph);
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetOutputs(std::vector<Operator>{op}));
  const auto outputs = ExecuteGraphUtils::GetGraphOutputs(native_graph.get());
  ASSERT_EQ(2U, outputs.size());
  const auto output_indexes = outputs[0].first->GetExtendInfo()->GetOutputIndex();
  EXPECT_EQ(GRAPH_SUCCESS, backend.SetOutputs(std::vector<Operator>{}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native_graph.get()));
  EXPECT_EQ(output_indexes, outputs[0].first->GetExtendInfo()->GetOutputIndex());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphIndexedOutputsRejectInvalidGraphBeforeCheckingEmptyList) {
  using IndexedOutputs = std::vector<std::pair<Operator, std::vector<size_t>>>;
  ExecuteGraphBackend backend("invalid");
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(IndexedOutputs{}));
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(IndexedOutputs{{MakeExecuteOp("output", "PlainOp", 0U, 1U), {0U}}}));
  EXPECT_FALSE(backend.IsValid());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphIndexedOutputsResolveNamesAndSkipInvalidEntries) {
  using IndexedOutputs = std::vector<std::pair<Operator, std::vector<size_t>>>;
  Graph graph("outputs", 1);
  auto producer = MakeExecuteOp("producer", "PlainOp", 0U, 3U);
  auto other = MakeExecuteOp("other", "PlainOp", 0U, 1U);
  graph.AddNodeByOp(producer);
  graph.AddNodeByOp(other);
  auto native_graph = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ExecuteGraphBackend backend(native_graph);
  // Resolve by name and expand the graph node's ports, rather than the supplied Operator's description.
  auto alias = MakeExecuteOp("producer", "OtherType", 0U, 1U);
  auto missing = MakeExecuteOp("missing", "PlainOp", 0U, 1U);
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetOutputs(IndexedOutputs{{alias, {}}}));
  auto outputs = ExecuteGraphUtils::GetGraphOutputs(native_graph.get());
  ASSERT_EQ(3U, outputs.size());
  for (size_t i = 0U; i < outputs.size(); ++i) {
    EXPECT_EQ("producer", outputs[i].first->GetName());
    EXPECT_EQ(static_cast<int32_t>(i), outputs[i].second);
  }
  ASSERT_EQ(GRAPH_SUCCESS,
            backend.SetOutputs(IndexedOutputs{{missing, {0U}}, {alias, {2U, 99U, 0U, 2U}}, {other, {}}}));
  outputs = ExecuteGraphUtils::GetGraphOutputs(native_graph.get());
  ASSERT_EQ(4U, outputs.size());
  EXPECT_EQ(2, outputs[0].second);
  EXPECT_EQ(0, outputs[1].second);
  EXPECT_EQ(outputs[0], outputs[2]);
  EXPECT_EQ("other", outputs[3].first->GetName());
  EXPECT_EQ(0, outputs[3].second);
  EXPECT_EQ(GRAPH_SUCCESS, backend.SetOutputs(IndexedOutputs{}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native_graph.get()));
  EXPECT_EQ(GRAPH_SUCCESS, backend.SetOutputs(IndexedOutputs{{missing, {0U}}, {alias, {99U}}}));
  EXPECT_TRUE(ExecuteGraphUtils::GetGraphOutputs(native_graph.get()).empty());
  EXPECT_TRUE(outputs[0].first->GetExtendInfo()->GetOutputIndex().empty());
  EXPECT_TRUE(outputs[3].first->GetExtendInfo()->GetOutputIndex().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNativeOutputsValidateBeforeReplacingEndpoints) {
  Graph graph("outputs", 1);
  auto producer = graph.AddNodeByOp(MakeExecuteOp("producer", "PlainOp", 0U, 2U));
  auto native_graph = GraphUtilsEx::Graph2ExecuteGraph(graph);
  auto *node = NodeAdapter::GNode2ExecuteNode(producer);
  ASSERT_NE(nullptr, node);
  const std::vector<std::pair<FastNode *, int32_t>> outputs{{node, 1}, {node, 0}, {node, 1}};
  ASSERT_EQ(GRAPH_SUCCESS, native_graph->SetGraphOutNodesInfo(outputs));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native_graph.get()));
  EXPECT_EQ(3U, node->GetExtendInfo()->GetOutputIndex().size());
  EXPECT_NE(GRAPH_SUCCESS, native_graph->SetGraphOutNodesInfo({{node, 2}}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native_graph.get()));
  EXPECT_EQ(3U, node->GetExtendInfo()->GetOutputIndex().size());
  Graph other("other", 1);
  auto foreign = other.AddNodeByOp(MakeExecuteOp("foreign", "PlainOp", 0U, 1U));
  EXPECT_NE(GRAPH_SUCCESS, native_graph->SetGraphOutNodesInfo({{NodeAdapter::GNode2ExecuteNode(foreign), 0}}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native_graph.get()));
  EXPECT_EQ(GRAPH_SUCCESS, native_graph->SetGraphOutNodesInfo({}));
  EXPECT_TRUE(ExecuteGraphUtils::GetGraphOutputs(native_graph.get()).empty());
  EXPECT_TRUE(node->GetExtendInfo()->GetOutputIndex().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphOutputsCreateAndUpdateNetOutputDataEdgesAndDescriptors) {
  Graph graph("outputs", 1);
  auto op = MakeExecuteOp("producer", "PlainOp", 0U, 2U);
  auto desc = OpDescUtils::GetOpDescFromOperator(op);
  ASSERT_EQ(GRAPH_SUCCESS, desc->UpdateOutputDesc(1U, GeTensorDesc(GeShape({2, 3}), FORMAT_ND, DT_INT32)));
  auto producer = graph.AddNodeByOp(op);
  auto replacement = graph.AddNodeByOp(MakeExecuteOp("replacement", "PlainOp", 0U, 1U));
  auto control = graph.AddNodeByOp(MakeExecuteOp("control", "PlainOp", 0U, 0U));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  auto *src = NodeAdapter::GNode2ExecuteNode(producer);
  auto *other = NodeAdapter::GNode2ExecuteNode(replacement);
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{producer, 1}, {producer, 0}, {producer, 1}}));
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);
  EXPECT_EQ(4U, native->GetDirectNodesSize());
  EXPECT_EQ(3U, sink->GetDataInNum());
  EXPECT_EQ(3U, sink->GetOpDescBarePtr()->GetAllInputsSize());
  const std::vector<int32_t> ports{1, 0, 1};
  for (size_t i = 0U; i < ports.size(); ++i) {
    const auto *edge = sink->GetInDataEdgeByIndex(static_cast<int32_t>(i));
    ASSERT_NE(nullptr, edge);
    EXPECT_EQ(src, edge->src);
    EXPECT_EQ(ports[i], edge->src_output);
    EXPECT_EQ(static_cast<int32_t>(i), edge->dst_input);
  }
  EXPECT_EQ((std::vector<int64_t>{2, 3}), sink->GetOpDescBarePtr()->GetInputDesc(0).GetShape().GetDims());
  EXPECT_EQ(DT_INT32, sink->GetOpDescBarePtr()->GetInputDesc(0).GetDataType());
  ASSERT_NE(nullptr, native->AddEdge(NodeAdapter::GNode2ExecuteNode(control), -1, sink, -1));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{replacement, 0}}));
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  EXPECT_EQ(4U, native->GetDirectNodesSize());
  EXPECT_EQ(1U, sink->GetDataInNum());
  EXPECT_EQ(1U, sink->GetOpDescBarePtr()->GetAllInputsSize());
  ASSERT_NE(nullptr, sink->GetInDataEdgeByIndex(0));
  EXPECT_EQ(other, sink->GetInDataEdgeByIndex(0)->src);
  EXPECT_TRUE(src->GetAllOutDataEdges().empty());
  EXPECT_EQ(1U, sink->GetAllInControlEdges().size());
  ASSERT_EQ(GRAPH_SUCCESS, native->SetGraphOutNodesInfo({}));
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  EXPECT_EQ(0U, sink->GetDataInNum());
  EXPECT_EQ(0U, sink->GetOpDescBarePtr()->GetAllInputsSize());
  EXPECT_TRUE(sink->GetAllInDataEdges().empty());
  EXPECT_EQ(1U, sink->GetAllInControlEdges().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphOutputsReusePreviouslyConstructedNetOutput) {
  Graph graph("es_style", 1);
  auto producer = graph.AddNodeByOp(MakeExecuteOp("producer", "PlainOp", 0U, 2U));
  auto net_output = graph.AddNodeByOp(MakeExecuteOp("NetOutput_es_style", "NetOutput", 2U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(producer, 0, net_output, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(producer, 1, net_output, 1));
  auto *original = NodeAdapter::GNode2ExecuteNode(net_output);
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{producer, 1}}));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  EXPECT_EQ(2U, native->GetDirectNodesSize());
  EXPECT_EQ(original, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  EXPECT_EQ(1U, original->GetDataInNum());
  ASSERT_NE(nullptr, original->GetInDataEdgeByIndex(0));
  EXPECT_EQ(1, original->GetInDataEdgeByIndex(0)->src_output);
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSubgraphOutputsFollowParentRelationsRatherThanRegistration) {
  Graph root("root", 1);
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 0U, 1U));
  Graph branch("branch", 1);
  auto input = branch.AddNodeByOp(MakeExecuteOp("input", "Data", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, branch.SetOutputs({{input, 0}}));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(branch);
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("then"), branch));
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "InnerNetOutput"));
  ASSERT_EQ(GRAPH_SUCCESS, branch.SetOutputs({{input, 0}}));
  EXPECT_EQ(2U, native->GetDirectNodesSize());
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "InnerNetOutput"));
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveSubgraph("branch"));
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "InnerNetOutput"));
  EXPECT_EQ(2U, native->GetDirectNodesSize());
  EXPECT_NE(nullptr, branch.GetParentGraph());
  ASSERT_EQ(GRAPH_SUCCESS, root.AddSubGraph(branch));
  ASSERT_EQ(GRAPH_SUCCESS, root.RemoveNode(parent));
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
  {
    Graph temporary_root("temporary_root", 1);
    auto temporary_parent = temporary_root.AddNodeByOp(MakeExecuteOp("temporary_parent", "If", 0U, 1U));
    ASSERT_EQ(GRAPH_SUCCESS, temporary_parent.SetSubgraph(AscendString("then"), branch));
  }
  EXPECT_EQ(nullptr, branch.GetParentGraph());
  EXPECT_EQ(sink, ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput"));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNamedOutputsRejectInvalidGraphBeforeCheckingEmptyList) {
  using NamedOutputs = std::vector<std::pair<Operator, std::string>>;
  ExecuteGraphBackend backend("invalid");
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(NamedOutputs{}));
  EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(NamedOutputs{{MakeExecuteOp("output", "PlainOp", 0U, 1U), ""}}));
  EXPECT_FALSE(backend.IsValid());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNamedOutputsResolvePortsAndKeepOutputsOnLookupFailure) {
  using NamedOutputs = std::vector<std::pair<Operator, std::string>>;
  Graph graph("outputs", 1);
  auto producer = MakeNamedExecuteOp("producer", "PlainOp", {"left", "right"}, {});
  graph.AddNodeByOp(producer);
  auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ExecuteGraphBackend backend(native);
  auto alias = MakeExecuteOp("producer", "OtherType", 0U, 0U);
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetOutputs(NamedOutputs{{alias, "right"}, {alias, ""}, {alias, "right"}}));
  const auto outputs = ExecuteGraphUtils::GetGraphOutputs(native.get());
  ASSERT_EQ(4U, outputs.size());
  const std::vector<int32_t> ports{1, 0, 1, 1};
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);
  ASSERT_EQ(ports.size(), sink->GetDataInNum());
  for (size_t i = 0U; i < ports.size(); ++i) {
    EXPECT_EQ(ports[i], outputs[i].second);
    const auto *edge = sink->GetInDataEdgeByIndex(static_cast<int32_t>(i));
    ASSERT_NE(nullptr, edge);
    EXPECT_EQ(outputs[i].first, edge->src);
    EXPECT_EQ(ports[i], edge->src_output);
  }
  EXPECT_EQ(GRAPH_SUCCESS, backend.SetOutputs(NamedOutputs{}));
  EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native.get()));
  auto missing = MakeExecuteOp("missing", "PlainOp", 0U, 1U);
  for (const auto &invalid : NamedOutputs{{missing, "left"}, {alias, "missing_port"}}) {
    EXPECT_EQ(GRAPH_FAILED, backend.SetOutputs(NamedOutputs{{alias, "left"}, invalid}));
    EXPECT_EQ(outputs, ExecuteGraphUtils::GetGraphOutputs(native.get()));
    EXPECT_EQ(ports.size(), sink->GetDataInNum());
    for (size_t i = 0U; i < ports.size(); ++i) {
      const auto *edge = sink->GetInDataEdgeByIndex(static_cast<int32_t>(i));
      ASSERT_NE(nullptr, edge);
      EXPECT_EQ(ports[i], edge->src_output);
    }
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphGNodeOutputsRejectInvalidEndpointsBeforeUpdatingNetOutput) {
  using NodeOutputs = std::vector<std::pair<GNode, int32_t>>;
  Graph graph("outputs", 1);
  auto producer = graph.AddNodeByOp(MakeExecuteOp("producer", "PlainOp", 0U, 2U));
  auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ExecuteGraphBackend backend(native);
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetOutputs(NodeOutputs{{producer, 1}}));
  const auto original = ExecuteGraphUtils::GetGraphOutputs(native.get());
  auto *sink = ExecuteGraphUtils::FindFirstNodeMatchType(native.get(), "NetOutput");
  ASSERT_NE(nullptr, sink);
  Graph foreign_graph("foreign", 1);
  auto foreign = foreign_graph.AddNodeByOp(MakeExecuteOp("foreign", "PlainOp", 0U, 1U));
  Graph compute_graph("compute");
  auto compute = compute_graph.AddNodeByOp(MakeExecuteOp("compute", "PlainOp", 0U, 1U));
  auto deleted = graph.AddNodeByOp(MakeExecuteOp("deleted", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(deleted));
  for (const auto &invalid :
       NodeOutputs{{producer, -1}, {producer, 2}, {foreign, 0}, {compute, 0}, {deleted, 0}, {GNode(), 0}}) {
    EXPECT_NE(GRAPH_SUCCESS, backend.SetOutputs(NodeOutputs{{producer, 0}, invalid}));
    EXPECT_EQ(original, ExecuteGraphUtils::GetGraphOutputs(native.get()));
    EXPECT_EQ(1U, sink->GetDataInNum());
    const auto *edge = sink->GetInDataEdgeByIndex(0);
    ASSERT_NE(nullptr, edge);
    EXPECT_EQ(original[0].first, edge->src);
    EXPECT_EQ(1, edge->src_output);
  }
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetOutputs(NodeOutputs{}));
  EXPECT_TRUE(ExecuteGraphUtils::GetGraphOutputs(native.get()).empty());
  EXPECT_EQ(0U, sink->GetDataInNum());
  EXPECT_TRUE(sink->GetAllInDataEdges().empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsRejectsInitializedSubgraphWithoutChangingState) {
  Graph root("root", 1);
  Graph branch("branch", 1);
  ASSERT_EQ(GRAPH_SUCCESS, root.SetValid());
  ASSERT_EQ(GRAPH_SUCCESS, branch.SetValid());
  auto parent = root.AddNodeByOp(MakeExecuteOp("parent", "If", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, parent.SetSubgraph(AscendString("body"), branch));
  auto original = GraphUtilsEx::Graph2ExecuteGraph(branch);
  auto facade = root.GetSubGraph("branch");
  AttrValue value;
  ASSERT_EQ(GRAPH_SUCCESS, value.SetAttrValue(static_cast<int64_t>(7)));
  ASSERT_EQ(GRAPH_SUCCESS, branch.SetAttr(AscendString("stage"), value));
  branch.SetInputs({MakeExecuteOp("input", "Data", 0U, 1U)});
  EXPECT_EQ(original, GraphUtilsEx::Graph2ExecuteGraph(branch));
  EXPECT_TRUE(facade->GetDirectNode().empty());
  AttrValue actual;
  EXPECT_EQ(GRAPH_SUCCESS, branch.GetAttr(AscendString("stage"), actual));
  int64_t stage = 0;
  ASSERT_EQ(GRAPH_SUCCESS, actual.GetValue(stage));
  EXPECT_EQ(7, stage);
  EXPECT_NE(nullptr, branch.GetParentGraph());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphValidityAndNameUseBackendState) {
  Graph graph("created_name", 1);
  const auto &name = graph.GetName();
  ASSERT_FALSE(graph.IsValid());
  EXPECT_EQ("created_name", name);
  AscendString facade_name;
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetName(facade_name));
  EXPECT_EQ(AscendString("created_name"), facade_name);
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  GraphUtilsEx::Graph2ExecuteGraph(graph)->SetName("renamed");
  EXPECT_EQ("created_name", graph.GetName());
  ASSERT_EQ(GRAPH_SUCCESS, graph.GetName(facade_name));
  EXPECT_EQ(AscendString("created_name"), facade_name);
  EXPECT_EQ(&name, &graph.GetName());
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetValid());

  ExecuteGraphBackend backend("renamed");
  EXPECT_FALSE(backend.IsValid());
  EXPECT_EQ("renamed", backend.GetName());
  ASSERT_EQ(GRAPH_SUCCESS, backend.SetValid());
  EXPECT_TRUE(backend.IsValid());
  EXPECT_EQ("renamed", backend.GetGraph()->GetName());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyPreservesRepeatedOutputSlots) {
  Graph graph("source", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("producer", "PlainOp", 0U, 2U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs(std::vector<std::pair<GNode, int32_t>>{{node, 1}, {node, 0}, {node, 1}}));
  Graph copied("copy", 1);
  EXPECT_FALSE(copied.IsValid());
  ASSERT_EQ(GRAPH_SUCCESS, copied.CopyFrom(graph));
  EXPECT_TRUE(copied.IsValid());
  auto outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(copied).get());
  ASSERT_EQ(3U, outputs.size());
  EXPECT_EQ(1, outputs[0].second);
  EXPECT_EQ(0, outputs[1].second);
  EXPECT_EQ(outputs[0], outputs[2]);
  EXPECT_NE(outputs[0].first,
            ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(graph).get())[0].first);
  EXPECT_EQ(3U, outputs[0].first->GetExtendInfo()->GetOutputIndex().size());
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(node));
  EXPECT_EQ(3U, ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(copied).get()).size());
  auto copy_node = copied.FindNodeByName(AscendString("producer"));
  ASSERT_NE(nullptr, copy_node);
  ASSERT_EQ(GRAPH_SUCCESS, copied.RemoveNode(*copy_node));
  outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(copied).get());
  ASSERT_EQ(3U, outputs.size());
  for (const auto &output : outputs) {
    EXPECT_EQ(nullptr, output.first);
    EXPECT_EQ(-1, output.second);
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphNodeLifetimeFollowsUnderlyingGraph) {
  GNode node;
  std::shared_ptr<ExecuteGraph> underlying;
  {
    Graph graph("graph", 1);
    node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 1U));
    underlying = GraphUtilsEx::Graph2ExecuteGraph(graph);
  }
  AscendString name;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetName(name));
  EXPECT_EQ(AscendString("node"), name);
  underlying.reset();
  EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphCopyReplacesImportedDestinationBeforeAnyNodeQuery) {
  Graph source("source", 1);
  source.AddNodeByOp(MakeExecuteOp("source_node", "PlainOp", 0U, 1U));
  Graph destination("destination", 1);
  destination.SetInputs({MakeExecuteOp("imported", "Data", 0U, 1U)});
  Graph alias = destination;
  ASSERT_EQ(GRAPH_SUCCESS, destination.CopyFrom(source));
  EXPECT_EQ(nullptr, destination.FindNodeByName(AscendString("imported")));
  EXPECT_NE(nullptr, destination.FindNodeByName(AscendString("source_node")));
  EXPECT_NE(nullptr, alias.FindNodeByName(AscendString("imported")));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphChildDoesNotDereferenceDestroyedParent) {
  Graph child("child", 1);
  ASSERT_EQ(GRAPH_SUCCESS, child.SetValid());
  GNode node;
  {
    Graph root("root", 1);
    node = root.AddNodeByOp(MakeExecuteOp("parent", "IfOp", 0U, 1U));
    ASSERT_EQ(GRAPH_SUCCESS, node.SetSubgraph(AscendString("then"), child));
  }
  EXPECT_EQ(nullptr, child.GetParentGraph());
  EXPECT_EQ(nullptr, child.GetParentNode());
  EXPECT_TRUE(child.GetAllSubgraphs().empty());
  EXPECT_TRUE(child.GetAllNodes().empty());
  Graph next("next", 1);
  ASSERT_EQ(GRAPH_SUCCESS, next.SetValid());
  auto next_parent = next.AddNodeByOp(MakeExecuteOp("next_parent", "If", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, next_parent.SetSubgraph(AscendString("body"), child));
  EXPECT_NE(nullptr, child.GetParentGraph());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsPreservesInputOrderAfterFailedBuild) {
  Graph graph("ordered", 1);
  graph.SetInputs({MakeExecuteOp("plain", "ConstData", 0U, 1U), MakeExecuteOp("data", "Data", 0U, 1U)});
  auto plain = NodeAdapter::GNode2ExecuteNode(*graph.FindNodeByName(AscendString("plain")));
  auto data = NodeAdapter::GNode2ExecuteNode(*graph.FindNodeByName(AscendString("data")));
  ASSERT_NE(nullptr, plain);
  ASSERT_NE(nullptr, data);
  EXPECT_EQ(0, plain->GetExtendInfo()->GetInputIndex());
  EXPECT_EQ(1, data->GetExtendInfo()->GetInputIndex());
  Graph retry("retry", 1);
  retry.SetInputs({MakeExecuteOp("dup", "Data", 0U, 1U), MakeExecuteOp("dup", "Data", 0U, 1U)});
  EXPECT_TRUE(retry.GetDirectNode().empty());
  retry.SetInputs({MakeExecuteOp("only", "Data", 0U, 1U)});
  auto only = retry.FindNodeByName(AscendString("only"));
  ASSERT_NE(nullptr, only);
  EXPECT_EQ(0, NodeAdapter::GNode2ExecuteNode(*only)->GetExtendInfo()->GetInputIndex());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsBuildsNestedOperatorSubgraphs) {
  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  auto parent = MakeExecuteOp("parent", "If", 1U, 1U);
  parent.SetInput(0U, input, 0U);
  auto source_desc = OpDescUtils::GetOpDescFromOperator(parent);
  source_desc->RegisterSubgraphIrName("then", kStatic);
  parent.SubgraphCountRegister("then", 1U);
  size_t calls = 0U;
  parent.SetSubgraphBuilder("then", 0U, [&calls]() {
    ++calls;
    auto branch_input = MakeExecuteOp("branch_input", "Data", 0U, 1U);
    auto inner = MakeExecuteOp("inner", "If", 1U, 1U);
    inner.SetInput(0U, branch_input, 0U);
    auto desc = OpDescUtils::GetOpDescFromOperator(inner);
    desc->RegisterSubgraphIrName("body", kStatic);
    inner.SubgraphCountRegister("body", 1U);
    inner.SetSubgraphBuilder("body", 0U, []() {
      Graph leaf("leaf", 1);
      leaf.SetInputs({MakeExecuteOp("leaf_input", "Data", 0U, 1U)});
      return leaf;
    });
    Graph branch("branch", 1);
    branch.SetInputs({branch_input});
    return branch;
  });
  Graph graph("root", 1);
  graph.SetInputs({input});
  auto root = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ASSERT_NE(nullptr, root);
  ASSERT_EQ(1U, calls);
  ASSERT_EQ(2U, root->GetAllSubgraphs().size());
  auto *branch = root->GetSubGraph("branch");
  auto *leaf = root->GetSubGraph("leaf");
  ASSERT_NE(nullptr, branch);
  ASSERT_NE(nullptr, leaf);
  ASSERT_NE(nullptr, branch->GetParentNodeBarePtr());
  ASSERT_NE(nullptr, leaf->GetParentNodeBarePtr());
  EXPECT_EQ(root.get(), branch->GetParentGraphBarePtr());
  EXPECT_EQ("parent", branch->GetParentNodeBarePtr()->GetName());
  EXPECT_EQ(branch, leaf->GetParentGraphBarePtr());
  EXPECT_EQ("inner", leaf->GetParentNodeBarePtr()->GetName());
  EXPECT_TRUE(branch->GetAllSubgraphs().empty());
  EXPECT_EQ("branch", branch->GetParentNodeBarePtr()->GetOpDescBarePtr()->GetSubgraphInstanceName(0));
  EXPECT_TRUE(source_desc->GetSubgraphInstanceName(0).empty());
  Graph other("other", 1);
  other.SetInputs({input});
  ASSERT_TRUE(other.IsValid());
  EXPECT_EQ(2U, calls);
  EXPECT_NE(branch, GraphUtilsEx::Graph2ExecuteGraph(other)->GetSubGraph("branch"));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsPreservesDynamicSubgraphSlotIndexes) {
  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  auto parent = MakeExecuteOp("case", "Case", 1U, 1U);
  parent.SetInput(0U, input, 0U);
  auto desc = OpDescUtils::GetOpDescFromOperator(parent);
  desc->RegisterSubgraphIrName("branches", kDynamic);
  parent.SubgraphCountRegister("branches", 12U);
  for (uint32_t i = 0U; i < 12U; ++i) {
    parent.SetSubgraphBuilder("branches", i, [i]() {
      const auto name = "branch_" + std::to_string(i);
      Graph branch(name.c_str(), 1);
      branch.SetInputs({MakeExecuteOp("input", "Data", 0U, 1U)});
      return branch;
    });
  }
  Graph graph("root", 1);
  graph.SetInputs({input});
  auto root = GraphUtilsEx::Graph2ExecuteGraph(graph);
  ASSERT_NE(nullptr, root);
  ASSERT_EQ(12U, root->GetAllSubgraphs().size());
  auto parent_node = graph.FindNodeByName(AscendString("case"));
  ASSERT_NE(nullptr, parent_node);
  auto *native_parent = NodeAdapter::GNode2ExecuteNode(*parent_node);
  ASSERT_NE(nullptr, native_parent);
  const auto *built_desc = native_parent->GetOpDescBarePtr();
  for (uint32_t i = 0U; i < 12U; ++i) {
    EXPECT_EQ("branch_" + std::to_string(i), built_desc->GetSubgraphInstanceName(i));
    EXPECT_TRUE(desc->GetSubgraphInstanceName(i).empty());
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsRejectsComputeSubgraphBuilder) {
  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  auto parent = MakeExecuteOp("parent", "If", 1U, 1U);
  parent.SetInput(0U, input, 0U);
  auto desc = OpDescUtils::GetOpDescFromOperator(parent);
  desc->RegisterSubgraphIrName("then", kStatic);
  parent.SubgraphCountRegister("then", 1U);
  parent.SetSubgraphBuilder("then", 0U, []() {
    Graph branch("compute_branch");
    (void)branch.SetValid();
    return branch;
  });
  Graph graph("root", 1);
  graph.SetInputs({input});
  EXPECT_FALSE(graph.IsValid());
  EXPECT_TRUE(desc->GetSubgraphInstanceName(0).empty());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsChecksSourceTypes) {
  for (const auto *type : {"Data", "AippData", "ConstData", "Const", "RefData", "AnyData", "PlainOp", "If"}) {
    SCOPED_TRACE(type);
    const bool allowed = std::string(type) == "Data" || std::string(type) == "AippData" ||
                         std::string(type) == "ConstData" || std::string(type) == "Const";
    Graph graph("root", 1);
    graph.SetInputs({MakeExecuteOp("source", type, 0U, 1U)});
    EXPECT_EQ(allowed, graph.IsValid());
    EXPECT_EQ(allowed ? 1U : 0U, graph.GetDirectNode().size());
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphSetInputsRejectsMixedSourceTypesWithoutPublishingGraph) {
  auto input = MakeExecuteOp("input", "Data", 0U, 1U);
  auto invalid = MakeExecuteOp("invalid", "PlainOp", 0U, 1U);
  Graph graph("root", 1);
  graph.SetInputs({input, invalid});
  EXPECT_FALSE(graph.IsValid());
  EXPECT_TRUE(graph.GetDirectNode().empty());
  graph.SetInputs({input});
  ASSERT_TRUE(graph.IsValid());
  EXPECT_EQ(1U, graph.GetDirectNode().size());
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("input")));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphEsCreateInputChecksFeedTypesBeforeReservingIndex) {
  for (const auto *type : {"Data", "AippData", "ConstData", "Const", "RefData", "AnyData", "PlainOp", "If"}) {
    SCOPED_TRACE(type);
    const bool allowed =
        std::string(type) == "Data" || std::string(type) == "AippData" || std::string(type) == "ConstData";
    ge::es::EsGraphBuilder builder("es_inputs", 1);
    auto input = builder.CreateInput(0, "input", type);
    EXPECT_EQ(allowed, input.GetCTensorHolder() != nullptr);
    if (!allowed) {
      auto retry = builder.CreateInput(0, "retry", "Data");
      EXPECT_NE(nullptr, retry.GetCTensorHolder());
    }
  }
}

TEST_F(ExecuteGraphFacadeTest, EsExecuteVariableReportsExternalErrorWithoutCreatingNode) {
  ge::es::EsGraphBuilder builder("es_execute_variable", 1);
  auto *graph = builder.GetCGraphBuilder()->GetGraph();
  (void)ErrorManager::GetInstance().GetErrorMessage();
  EXPECT_EQ(nullptr, EsCreateVariable(builder.GetCGraphBuilder(), 0, "variable"));
  auto error = ErrorManager::GetInstance().GetErrorMessage();
  EXPECT_NE(std::string::npos, error.find("E10062"));
  EXPECT_NE(std::string::npos, error.find("EsCreateVariable"));
  EXPECT_TRUE(graph->GetDirectNode().empty());
  EXPECT_EQ(nullptr, builder.CreateVariable(0, "variable_cpp").GetCTensorHolder());
  (void)ErrorManager::GetInstance().GetErrorMessage();

  ge::es::EsGraphBuilder compute("es_compute_variable");
  EXPECT_NE(nullptr, compute.CreateVariable(0, "variable").GetCTensorHolder());
}

TEST_F(ExecuteGraphFacadeTest, EsExecuteOriginSymbolShapeReportsExternalError) {
  ge::es::EsGraphBuilder builder("es_execute_symbolic", 1);
  auto input = builder.CreateInput(0);
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  const char *dims[] = {"s0"};
  (void)ErrorManager::GetInstance().GetErrorMessage();
  EXPECT_EQ(UNSUPPORTED, EsSetOriginSymbolShape(input.GetCTensorHolder(), dims, 1));
  auto error = ErrorManager::GetInstance().GetErrorMessage();
  EXPECT_NE(std::string::npos, error.find("E10062"));
  EXPECT_NE(std::string::npos, error.find("EsSetOriginSymbolShape"));
  EXPECT_EQ(std::string::npos, error.find("E18888"));
  input.SetOriginSymbolShape({"s0"});
  error = ErrorManager::GetInstance().GetErrorMessage();
  EXPECT_NE(std::string::npos, error.find("E10062"));
  EXPECT_EQ(1U, builder.GetCGraphBuilder()->GetGraph()->GetDirectNode().size());
}

TEST_F(ExecuteGraphFacadeTest, EsGeneratedArithmeticBuildsExecuteNodes) {
  ge::es::EsGraphBuilder builder("es_execute_arithmetic", 1);
  auto left = builder.CreateInput(0);
  auto right = builder.CreateInput(1);
  ASSERT_NE(nullptr, left.GetCTensorHolder());
  ASSERT_NE(nullptr, right.GetCTensorHolder());
  const std::vector<std::pair<const char *, std::function<ge::es::EsTensorHolder()>>> operations = {
      {"Add", [&]() { return left + right; }},
      {"Sub", [&]() { return left - right; }},
      {"Mul", [&]() { return left * right; }},
      {"Div", [&]() { return left / right; }}};
  auto *graph = builder.GetCGraphBuilder()->GetGraph();
  size_t expected_nodes = 2U;
  for (const auto &operation : operations) {
    (void)ErrorManager::GetInstance().GetErrorMessage();
    auto output = operation.second();
    ASSERT_NE(nullptr, output.GetCTensorHolder()) << operation.first;
    const auto &node = output.GetCTensorHolder()->GetProducer();
    EXPECT_NE(nullptr, NodeAdapter::GNode2ExecuteNode(node));
    AscendString type;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetType(type));
    EXPECT_EQ(AscendString(operation.first), type);
    EXPECT_EQ(2U, node.GetInputsSize());
    EXPECT_EQ(1U, node.GetOutputsSize());
    EXPECT_NE(nullptr, node.GetInDataNodesAndPortIndexs(0).first);
    EXPECT_NE(nullptr, node.GetInDataNodesAndPortIndexs(1).first);
    EXPECT_EQ(++expected_nodes, graph->GetDirectNode().size());
    EXPECT_TRUE(ErrorManager::GetInstance().GetErrorMessage().empty());
  }
  EXPECT_NE(nullptr, EsAdd(left.GetCTensorHolder(), right.GetCTensorHolder()));
  EXPECT_EQ(++expected_nodes, graph->GetDirectNode().size());
  EXPECT_NE(nullptr, builder.CreateScalar(int64_t{7}).GetCTensorHolder());
  EXPECT_NE(nullptr, builder.CreateVector(std::vector<int64_t>{1, 2}).GetCTensorHolder());
  ge::es::EsGraphBuilder compute("es_compute_arithmetic");
  auto x = compute.CreateInput(0);
  auto y = compute.CreateInput(1);
  EXPECT_NE(nullptr, (x + y).GetCTensorHolder());
}

TEST_F(ExecuteGraphFacadeTest, EsExecuteTensorDescSettersRejectWithoutMutation) {
  ge::es::EsGraphBuilder builder("execute_desc_setters", 1);
  auto input = builder.CreateInput(0, "input", DT_FLOAT, FORMAT_ND, {2, 3});
  auto *holder = input.GetCTensorHolder();
  ASSERT_NE(nullptr, holder);
  TensorDesc before;
  ASSERT_EQ(GRAPH_SUCCESS, holder->GetProducer().GetOutputDesc(0, before));
  const int64_t dims[] = {8};
  const std::vector<std::pair<const char *, std::function<Status()>>> setters = {
      {"EsSetDataType", [&]() { return EsSetDataType(holder, C_DT_INT32); }},
      {"EsSetFormat", [&]() { return EsSetFormat(holder, C_FORMAT_NCHW); }},
      {"EsSetShape", [&]() { return EsSetShape(holder, dims, 1); }},
      {"EsCTensorHolder::SetOriginFormat", [&]() { return holder->SetOriginFormat(FORMAT_NCHW); }},
      {"EsCTensorHolder::SetStorageFormat", [&]() { return holder->SetStorageFormat(FORMAT_NHWC); }},
      {"EsCTensorHolder::SetOriginShape", [&]() { return holder->SetOriginShape(Shape({4})); }},
      {"EsCTensorHolder::SetStorageShape", [&]() { return holder->SetStorageShape(Shape({6})); }}};
  for (const auto &setter : setters) {
    SCOPED_TRACE(setter.first);
    (void)ErrorManager::GetInstance().GetErrorMessage();
    EXPECT_EQ(UNSUPPORTED, setter.second());
    const auto error = ErrorManager::GetInstance().GetErrorMessage();
    EXPECT_NE(std::string::npos, error.find("E10062"));
    EXPECT_NE(std::string::npos, error.find(setter.first));
    TensorDesc after;
    ASSERT_EQ(GRAPH_SUCCESS, holder->GetProducer().GetOutputDesc(0, after));
    EXPECT_EQ(before.GetDataType(), after.GetDataType());
    EXPECT_EQ(before.GetOriginDataType(), after.GetOriginDataType());
    EXPECT_EQ(before.GetFormat(), after.GetFormat());
    EXPECT_EQ(before.GetOriginFormat(), after.GetOriginFormat());
    EXPECT_EQ(before.GetShape().GetDims(), after.GetShape().GetDims());
    EXPECT_EQ(before.GetOriginShape().GetDims(), after.GetOriginShape().GetDims());
  }
}

TEST_F(ExecuteGraphFacadeTest, EsExecuteCppTensorDescSettersRejectWithoutMutation) {
  ge::es::EsGraphBuilder builder("execute_cpp_desc_setters", 1);
  auto input = builder.CreateInput(0, "input", DT_FLOAT, FORMAT_ND, {2, 3});
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  const std::vector<std::pair<const char *, std::function<void()>>> setters = {
      {"EsSetDataType", [&]() { input.SetDataType(DT_INT32); }},
      {"EsSetFormat", [&]() { input.SetFormat(FORMAT_NCHW); }},
      {"EsSetShape", [&]() { input.SetShape({8}); }}};
  for (const auto &setter : setters) {
    SCOPED_TRACE(setter.first);
    (void)ErrorManager::GetInstance().GetErrorMessage();
    setter.second();
    const auto error = ErrorManager::GetInstance().GetErrorMessage();
    EXPECT_NE(std::string::npos, error.find("E10062"));
    EXPECT_NE(std::string::npos, error.find(setter.first));
    TensorDesc desc;
    ASSERT_EQ(GRAPH_SUCCESS, input.GetCTensorHolder()->GetProducer().GetOutputDesc(0, desc));
    EXPECT_EQ(DT_FLOAT, desc.GetDataType());
    EXPECT_EQ(FORMAT_ND, desc.GetFormat());
    EXPECT_EQ(std::vector<int64_t>({2, 3}), desc.GetShape().GetDims());
  }
}

TEST_F(ExecuteGraphFacadeTest, EsExecuteConstructionKeepsTensorDescInitialization) {
  ge::es::EsGraphBuilder builder("execute_desc_initialization", 1);
  auto input = builder.CreateInput(0);
  auto detailed = builder.CreateInput(1, "detailed", DT_INT32, FORMAT_NCHW, {2, 3});
  auto constant = builder.CreateConst(std::vector<int64_t>{1, 2}, {2});
  auto scalar = builder.CreateScalar(int64_t{3});
  auto vector = builder.CreateVector(std::vector<int64_t>{4, 5});
  for (const auto &holder : {input, detailed, constant, scalar, vector}) {
    ASSERT_NE(nullptr, holder.GetCTensorHolder());
  }
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, detailed.GetCTensorHolder()->GetProducer().GetOutputDesc(0, desc));
  EXPECT_EQ(DT_INT32, desc.GetDataType());
  EXPECT_EQ(FORMAT_NCHW, desc.GetFormat());
  EXPECT_EQ(std::vector<int64_t>({2, 3}), desc.GetShape().GetDims());
  ASSERT_EQ(GRAPH_SUCCESS, constant.GetCTensorHolder()->GetProducer().GetOutputDesc(0, desc));
  EXPECT_EQ(DT_INT64, desc.GetDataType());
  EXPECT_EQ(std::vector<int64_t>({2}), desc.GetShape().GetDims());
  EXPECT_EQ(SUCCESS, detailed.SetAttr("port_attr", int64_t{7}));
  EXPECT_EQ(SUCCESS, detailed.AddControlEdge({input}));
  auto graph = builder.BuildAndReset({input, detailed, constant, scalar, vector});
  ASSERT_NE(nullptr, graph);
  EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
  EXPECT_EQ(6U, graph->GetDirectNode().size());
}

TEST_F(ExecuteGraphFacadeTest, ExecuteNodeInvalidAfterNativeRemovalAndAddressReuse) {
  Graph graph("native_reuse", 1);
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  const auto native = GraphUtilsEx::Graph2ExecuteGraph(graph);
  auto desc = std::make_shared<OpDesc>("node", "PlainOp");
  auto *original = native->AddNode(desc);
  auto first = NodeAdapter::ExecuteNode2GNode(original);
  auto second = NodeAdapter::ExecuteNode2GNode(original);
  ASSERT_NE(nullptr, first);
  ASSERT_NE(nullptr, second);
  auto sentinel = MakeExecuteSubgraph("sentinel");
  const auto expect_queries_preserve_outputs = [&sentinel](const GNode &stale) {
    GraphPtr subgraph = sentinel;
    EXPECT_EQ(GRAPH_FAILED, stale.GetSubgraph(0U, subgraph));
    EXPECT_EQ(sentinel, subgraph);
    std::vector<GraphPtr> subgraphs{sentinel};
    EXPECT_EQ(GRAPH_FAILED, stale.GetALLSubgraphs(subgraphs));
    ASSERT_EQ(1U, subgraphs.size());
    EXPECT_EQ(sentinel, subgraphs.front());
    const int32_t value = 7;
    Tensor data(TensorDesc(Shape({1}), FORMAT_ND, DT_INT32));
    ASSERT_EQ(GRAPH_SUCCESS, data.SetData(reinterpret_cast<const uint8_t *>(&value), sizeof(value)));
    EXPECT_EQ(GRAPH_FAILED, stale.GetInputConstData(0, data));
    EXPECT_EQ(DT_INT32, data.GetDataType());
    ASSERT_EQ(sizeof(value), data.GetSize());
    ASSERT_NE(nullptr, data.GetData());
    EXPECT_EQ(0, memcmp(&value, data.GetData(), sizeof(value)));
  };
  ASSERT_EQ(GRAPH_SUCCESS, native->RemoveJustNode(original));
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, first->GetName(name));
  expect_queries_preserve_outputs(*first);
  auto *replacement = native->AddNode(desc);
  ASSERT_EQ(original, replacement);
  EXPECT_NE(GRAPH_SUCCESS, first->GetName(name));
  EXPECT_NE(GRAPH_SUCCESS, second->GetName(name));
  expect_queries_preserve_outputs(*first);
  expect_queries_preserve_outputs(*second);
  auto current = NodeAdapter::ExecuteNode2GNode(replacement);
  ASSERT_NE(nullptr, current);
  EXPECT_EQ(GRAPH_SUCCESS, current->GetName(name));
}

TEST_F(ExecuteGraphFacadeTest, ExecuteNodeInvalidAfterNativeMoveAndReturn) {
  Graph source("source", 1);
  Graph destination("destination", 1);
  ASSERT_EQ(GRAPH_SUCCESS, destination.SetValid());
  auto node = source.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 1U));
  auto native_source = GraphUtilsEx::Graph2ExecuteGraph(source);
  auto native_destination = GraphUtilsEx::Graph2ExecuteGraph(destination);
  auto *raw = native_source->GetDirectNode().front();
  ASSERT_EQ(raw, native_destination->AddNode(raw));
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
  ASSERT_EQ(raw, native_source->AddNodeFront(raw));
  EXPECT_NE(GRAPH_SUCCESS, node.GetName(name));
  auto current = source.FindNodeByName(AscendString("node"));
  ASSERT_NE(nullptr, current);
  EXPECT_EQ(GRAPH_SUCCESS, current->GetName(name));
}

// 2.2：执行节点的名称/类型/端口计数/属性读写经 ExecuteNodeBackend 正确返回（A01/A06）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeQueriesAndAttrs) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "SrcOp", 1U, 1U));
  auto dst = graph.AddNodeByOp(MakeExecuteOp("dst", "DstOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(src, 0, dst, 0));

  AscendString name;
  AscendString type;
  ASSERT_EQ(GRAPH_SUCCESS, src.GetName(name));
  ASSERT_EQ(GRAPH_SUCCESS, src.GetType(type));
  EXPECT_EQ(AscendString("src"), name);
  EXPECT_EQ(AscendString("SrcOp"), type);
  EXPECT_EQ(1U, src.GetInputsSize());
  EXPECT_EQ(1U, src.GetOutputsSize());

  // 邻接查询（A02/A03）。
  const auto producer = dst.GetInDataNodesAndPortIndexs(0);
  ASSERT_NE(nullptr, producer.first);
  AscendString producer_name;
  ASSERT_EQ(GRAPH_SUCCESS, producer.first->GetName(producer_name));
  EXPECT_EQ(AscendString("src"), producer_name);
  EXPECT_EQ(0, producer.second);
  const auto consumers = src.GetOutDataNodesAndPortIndexs(0);
  ASSERT_EQ(1U, consumers.size());

  // 具名属性读写（A06）。
  AttrValue value;
  int64_t attr_int = 42;
  value.SetAttrValue(attr_int);
  EXPECT_EQ(GRAPH_SUCCESS, src.SetAttr(AscendString("answer"), value));
  AttrValue read_back;
  int64_t read_int = 0;
  EXPECT_EQ(GRAPH_SUCCESS, src.GetAttr(AscendString("answer"), read_back));
  ASSERT_EQ(GRAPH_SUCCESS, read_back.GetValue(read_int));
  EXPECT_EQ(42, read_int);
  EXPECT_FALSE(src.HasAttr(AscendString("missing")));
}

TEST_F(ExecuteNodeFacadeTest, ExecuteNodeNamedAndDynamicPortIndexes) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 2U, 2U));
  int32_t index = -1;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputIndexByName("x1", index));
  EXPECT_EQ(1, index);
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputIndexByName("y0", index));
  EXPECT_EQ(0, index);
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputIndexByName("missing", index));
  EXPECT_EQ(-1, index);
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputIndexByName("missing", index));
  EXPECT_EQ(-1, index);

  std::vector<int32_t> indexes;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetDynamicInputIndexesByName("x", indexes));
  EXPECT_EQ(std::vector<int32_t>({0, 1}), indexes);
  indexes.clear();
  ASSERT_EQ(GRAPH_SUCCESS, node.GetDynamicOutputIndexesByName("y", indexes));
  EXPECT_EQ(std::vector<int32_t>({0, 1}), indexes);
  indexes.clear();
  EXPECT_NE(GRAPH_SUCCESS, node.GetDynamicInputIndexesByName("missing", indexes));
  EXPECT_NE(GRAPH_SUCCESS, node.GetDynamicOutputIndexesByName("missing", indexes));
}

TEST_F(ExecuteNodeFacadeTest, ExecuteNodeBatchAttrsReplaceExistingEntriesAndFilterInternalAttrs) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 0U));
  AttrValue value;
  ASSERT_EQ(GRAPH_SUCCESS, value.SetAttrValue(int64_t{42}));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttrs({{"answer", value}, {"_internal", value}}));
  std::map<AscendString, AttrValue> attrs = {{"existing", value}};
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAllAttrs(attrs));
  EXPECT_EQ(1U, attrs.size());
  EXPECT_EQ(0U, attrs.count("existing"));
  ASSERT_EQ(1U, attrs.count("answer"));
  EXPECT_EQ(0U, attrs.count("_internal"));
  int64_t answer = 0;
  ASSERT_EQ(GRAPH_SUCCESS, attrs.at("answer").GetAttrValue(answer));
  EXPECT_EQ(42, answer);

  EXPECT_EQ(GRAPH_PARAM_INVALID, node.SetAttrs({{"", value}, {"new_attr", value}}));
  EXPECT_FALSE(node.HasAttr("new_attr"));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(node));
  graph.AddNodeByOp(MakeExecuteOp("replacement", "PlainOp", 0U, 0U));
  EXPECT_NE(GRAPH_SUCCESS, node.SetAttrs({{"answer", value}}));
  EXPECT_NE(GRAPH_SUCCESS, node.GetAllAttrs(attrs));
}

// 2.2：节点删除后其所有句柄失效（A16 后置条件）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeInvalidAfterRemoveNode) {
  Graph graph("execute_graph", 1);
  auto node1 = graph.AddNodeByOp(MakeExecuteOp("node1", "PlainOp", 1U, 1U));
  auto node2 = graph.AddNodeByOp(MakeExecuteOp("node2", "PlainOp", 1U, 1U));
  auto removed = node2;
  ASSERT_EQ(GRAPH_SUCCESS, graph.AddDataEdge(node1, 0, node2, 0));
  ASSERT_EQ(GRAPH_SUCCESS, graph.RemoveNode(node2));

  // 被删节点及其副本失效；删除前摘边保证邻居邻接表无悬挂边。
  // 失效类错误的最终状态码待公共头评审（design 3.2.8），这里只断言“不再成功”。
  AscendString name;
  EXPECT_NE(GRAPH_SUCCESS, removed.GetName(name));
  EXPECT_NE(GRAPH_SUCCESS, node2.GetName(name));
  EXPECT_EQ(nullptr, node2.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ(nullptr, node1.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ(0U, node1.GetOutDataNodesAndPortIndexs(0).size());

  // 存活节点仍可用。
  EXPECT_EQ(GRAPH_SUCCESS, node1.GetName(name));
  EXPECT_EQ(AscendString("node1"), name);
}

// 2.2：执行节点不能被换装成计算节点使用（NodeAdapter 防线）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeRejectsComputeNodeAdapter) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 1U, 1U));
  EXPECT_EQ(nullptr, NodeAdapter::GNode2Node(node));
}

// 3.1：CompliantNodeBuilder 组合到 AddNodeByOp——无原型节点、多端口、创建时属性。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeCompliantBuilderWithoutPrototype) {
  Graph graph("execute_graph", 1);
  AttrValue default_alpha;
  ASSERT_EQ(GRAPH_SUCCESS, default_alpha.SetAttrValue(static_cast<int64_t>(7)));
  es::CompliantNodeBuilder builder(&graph);
  builder.OpType("PlainOp")
      .Name("built")
      .IrDefInputsV2({
          {"x", es::CompliantNodeBuilder::kEsIrInputDynamic, ""},
      })
      .IrDefOutputsV2({
          {"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""},
          {"z", es::CompliantNodeBuilder::kEsIrOutputRequired, ""},
      })
      .InstanceDynamicInputNum("x", 2)
      .InstanceOutputDataType("y", DT_FLOAT)
      .InstanceOutputShape("y", {4, 8})
      .IrDefAttrsV2({
          {"alpha", es::CompliantNodeBuilder::kEsAttrOptional, "Int", default_alpha},
      });
  // Kernel type 未注册计算算子原型：构建不依赖原型，也不触发推导。
  ASSERT_FALSE(OperatorFactory::IsExistOp("PlainOp"));
  auto node = builder.Build();

  AscendString name;
  AscendString type;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
  ASSERT_EQ(GRAPH_SUCCESS, node.GetType(type));
  EXPECT_EQ(AscendString("built"), name);
  EXPECT_EQ(AscendString("PlainOp"), type);
  // 输入按动态实例数展开为 2 个端口；输出为两个固定端口。
  EXPECT_EQ(2U, node.GetInputsSize());
  EXPECT_EQ(2U, node.GetOutputsSize());
  // 创建时属性随描述一起进入执行节点。
  EXPECT_TRUE(node.HasAttr(AscendString("alpha")));

  // 创建时 TensorDesc 随描述导入（可选描述路径）。
  TensorDesc out_desc;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputDesc(0, out_desc));
  EXPECT_EQ(DT_FLOAT, out_desc.GetDataType());
  EXPECT_EQ(std::vector<int64_t>({4, 8}), out_desc.GetShape().GetDims());

  // 执行图类别：节点没有被换算成计算图节点。
  EXPECT_EQ(nullptr, NodeAdapter::GNode2Node(node));
  EXPECT_EQ(1U, graph.GetAllNodes().size());
}

// 3.1：零端口节点可创建，且可作为控制边目标（清理类 Kernel）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeCompliantBuilderZeroPorts) {
  Graph graph("execute_graph", 1);
  es::CompliantNodeBuilder builder(&graph);
  builder.OpType("CleanupOp").Name("cleanup");
  auto cleanup = builder.Build();

  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, cleanup.GetName(name));
  EXPECT_EQ(AscendString("cleanup"), name);
  EXPECT_EQ(0U, cleanup.GetInputsSize());
  EXPECT_EQ(0U, cleanup.GetOutputsSize());

  auto worker = graph.AddNodeByOp(MakeExecuteOp("worker", "PlainOp", 0U, 0U));
  EXPECT_EQ(GRAPH_SUCCESS, graph.AddControlEdge(cleanup, worker));
}

// CompliantNodeBuilder 沿用 AddNodeByOp 的重复名称行为，两个节点保持独立。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeCompliantBuilderAllowsDuplicateNames) {
  Graph graph("execute_graph", 1);
  es::CompliantNodeBuilder first(&graph);
  first.OpType("PlainOp").Name("dup");
  ASSERT_NE(nullptr, NodeAdapter::GNode2ExecuteNode(first.Build()));

  es::CompliantNodeBuilder second(&graph);
  second.OpType("PlainOp").Name("dup");
  EXPECT_NE(nullptr, NodeAdapter::GNode2ExecuteNode(second.Build()));
  EXPECT_EQ(2U, graph.GetAllNodes().size());
}

// 3.5：基线属性类型（标量/字符串/Tensor/列表/字节）经公共门面写入后读回一致（A06）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodeAttributeTypesRoundTrip) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 1U, 1U));

  AttrValue int_value;
  ASSERT_EQ(GRAPH_SUCCESS, int_value.SetAttrValue(static_cast<int64_t>(11)));
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("i64"), int_value));
  AttrValue str_value;
  ASSERT_EQ(GRAPH_SUCCESS, str_value.SetAttrValue(AscendString("hello")));
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("str"), str_value));
  AttrValue list_value;
  ASSERT_EQ(GRAPH_SUCCESS, list_value.SetAttrValue(std::vector<int64_t>({1, 2, 3})));
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("list"), list_value));

  TensorDesc attr_tensor_desc(Shape({2}), FORMAT_ND, DT_FLOAT);
  Tensor attr_tensor(attr_tensor_desc, std::vector<uint8_t>(8U, 0U));
  AttrValue tensor_value;
  ASSERT_EQ(GRAPH_SUCCESS, tensor_value.SetAttrValue(attr_tensor));
  EXPECT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("tensor"), tensor_value));

  // 类型化读取入口（非 Tensor 执行值）各自独立工作。
  int64_t read_i64 = 0;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("i64"), read_i64));
  EXPECT_EQ(11, read_i64);
  AscendString read_str;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("str"), read_str));
  EXPECT_EQ(AscendString("hello"), read_str);
  std::vector<int64_t> read_list;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("list"), read_list));
  EXPECT_EQ(std::vector<int64_t>({1, 2, 3}), read_list);
  Tensor read_tensor;
  EXPECT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("tensor"), read_tensor));
  EXPECT_EQ(8U, read_tensor.GetSize());

  // 缺失属性：读取失败、HasAttr 为假。
  EXPECT_FALSE(node.HasAttr(AscendString("missing")));
  int64_t missing_value = 0;
  EXPECT_NE(GRAPH_SUCCESS, node.GetAttr(AscendString("missing"), missing_value));

  // 类型不匹配：按字符串读取整数属性失败，原值不被改写。
  AscendString wrong_type;
  EXPECT_NE(GRAPH_SUCCESS, node.GetAttr(AscendString("i64"), wrong_type));
  EXPECT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("i64"), read_i64));
  EXPECT_EQ(11, read_i64);
}

TEST_F(ExecuteNodeFacadeTest, ExecuteNodeStringAttrsRejectEmptyEntriesAndAppendReads) {
  Graph graph("strings", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 0U));
  AscendString value("text");
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("scalar"), value));
  AscendString scalar;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("scalar"), scalar));
  EXPECT_EQ(value, scalar);

  std::vector<AscendString> values{AscendString("first"), AscendString(), AscendString("last")};
  EXPECT_EQ(GRAPH_PARAM_INVALID, node.SetAttr(AscendString("list"), values));
  EXPECT_FALSE(node.HasAttr(AscendString("list")));
  values.erase(values.begin() + 1);
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("list"), values));
  std::vector<AscendString> read{AscendString("existing")};
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("list"), read));
  EXPECT_EQ((std::vector<AscendString>{AscendString("existing"), AscendString("first"), AscendString("last")}), read);
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("list"), read));
  const auto before_failure = read;
  EXPECT_EQ(GRAPH_FAILED, node.GetAttr(AscendString("missing"), read));
  EXPECT_EQ(before_failure, read);
  EXPECT_EQ((std::vector<AscendString>{AscendString("existing"), AscendString("first"), AscendString("last"),
                                       AscendString("first"), AscendString("last")}),
            read);
}

// 3.5：端口描述的可选读写与端口范围校验（A07）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodePortDescReadWrite) {
  Graph graph("execute_graph", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 1U, 1U));

  TensorDesc input_desc;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputDesc(0, input_desc));
  EXPECT_EQ(1U, node.GetInputsSize());

  TensorDesc updated(Shape({3, 4}), FORMAT_NCHW, DT_INT32);
  ASSERT_EQ(GRAPH_SUCCESS, node.UpdateInputDesc(0, updated));
  TensorDesc read_back;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetInputDesc(0, read_back));
  EXPECT_EQ(DT_INT32, read_back.GetDataType());
  EXPECT_EQ(std::vector<int64_t>({3, 4}), read_back.GetShape().GetDims());

  TensorDesc output_desc(Shape({8}), FORMAT_ND, DT_FLOAT);
  ASSERT_EQ(GRAPH_SUCCESS, node.UpdateOutputDesc(0, output_desc));
  TensorDesc output_read_back;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputDesc(0, output_read_back));
  EXPECT_EQ(std::vector<int64_t>({8}), output_read_back.GetShape().GetDims());

  // 越界端口：读失败，写失败，且不改动已有描述。
  TensorDesc out_of_range;
  EXPECT_NE(GRAPH_SUCCESS, node.GetInputDesc(1, out_of_range));
  EXPECT_NE(GRAPH_SUCCESS, node.GetOutputDesc(1, out_of_range));
  EXPECT_NE(GRAPH_SUCCESS, node.UpdateInputDesc(1, updated));
  EXPECT_NE(GRAPH_SUCCESS, node.UpdateOutputDesc(1, output_desc));
  EXPECT_EQ(GRAPH_SUCCESS, node.GetInputDesc(0, read_back));
  EXPECT_EQ(std::vector<int64_t>({3, 4}), read_back.GetShape().GetDims());
}

// 3.5：端口属性读写与 RefFrom 输出引用协议（A07/A09）。
TEST_F(ExecuteNodeFacadeTest, ExecuteNodePortAttrsAndRefFrom) {
  Graph graph("execute_graph", 1);
  auto src = graph.AddNodeByOp(MakeExecuteOp("src", "PlainOp", 0U, 2U));
  auto ref = graph.AddNodeByOp(MakeExecuteOp("ref", "PlainOp", 1U, 1U));

  AttrValue in_attr;
  ASSERT_EQ(GRAPH_SUCCESS, in_attr.SetAttrValue(static_cast<int64_t>(5)));
  EXPECT_EQ(GRAPH_SUCCESS, ref.SetInputAttr(AscendString("rank"), 0, in_attr));
  AttrValue read_in_attr;
  ASSERT_EQ(GRAPH_SUCCESS, ref.GetInputAttr(AscendString("rank"), 0, read_in_attr));
  int64_t rank = 0;
  ASSERT_EQ(GRAPH_SUCCESS, read_in_attr.GetValue(rank));
  EXPECT_EQ(5, rank);

  // RefFrom 端点协议：输出属性成对写入来源节点名与非负输出索引。
  AttrValue ref_node;
  ASSERT_EQ(GRAPH_SUCCESS, ref_node.SetAttrValue(AscendString("src")));
  ASSERT_EQ(GRAPH_SUCCESS, ref.SetOutputAttr(AscendString("RefFromNode"), 0, ref_node));
  AttrValue ref_index;
  ASSERT_EQ(GRAPH_SUCCESS, ref_index.SetAttrValue(static_cast<int64_t>(1)));
  ASSERT_EQ(GRAPH_SUCCESS, ref.SetOutputAttr(AscendString("RefFromIndex"), 0, ref_index));

  AttrValue read_ref_node;
  ASSERT_EQ(GRAPH_SUCCESS, ref.GetOutputAttr(AscendString("RefFromNode"), 0, read_ref_node));
  AscendString ref_node_name;
  ASSERT_EQ(GRAPH_SUCCESS, read_ref_node.GetValue(ref_node_name));
  EXPECT_EQ(AscendString("src"), ref_node_name);
  AttrValue read_ref_index;
  ASSERT_EQ(GRAPH_SUCCESS, ref.GetOutputAttr(AscendString("RefFromIndex"), 0, read_ref_index));
  int64_t ref_index_value = -1;
  ASSERT_EQ(GRAPH_SUCCESS, read_ref_index.GetValue(ref_index_value));
  EXPECT_EQ(1, ref_index_value);

  // 端口属性按端口隔离，越界端口与缺失属性都失败。
  AttrValue other_port_attr;
  EXPECT_NE(GRAPH_SUCCESS, ref.GetInputAttr(AscendString("rank"), 1, other_port_attr));
  EXPECT_NE(GRAPH_SUCCESS, ref.GetOutputAttr(AscendString("missing"), 0, other_port_attr));
  EXPECT_NE(GRAPH_SUCCESS, ref.SetOutputAttr(AscendString("RefFromNode"), 1, ref_node));
}
TEST_F(ExecuteGraphFacadeTest, ExecuteGraphUnsupportedOperatorInterfacesPreserveState) {
  for (const bool initialize : {false, true}) {
    Graph graph("unsupported_ops", 1);
    auto op = MakeExecuteOp("node", "PlainOp", 0U, 1U);
    if (initialize) {
      graph.AddNodeByOp(op);
    }
    const auto original = GraphUtilsEx::Graph2ExecuteGraph(graph);
    Operator found;
    EXPECT_EQ(GRAPH_FAILED, graph.FindOpByName(std::string("node"), found));
    EXPECT_EQ(GRAPH_FAILED, graph.FindOpByName("node", found));
    std::vector<Operator> ops{op};
    EXPECT_EQ(GRAPH_FAILED, graph.FindOpByType(std::string("PlainOp"), ops));
    EXPECT_EQ(GRAPH_FAILED, graph.FindOpByType("PlainOp", ops));
    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ("node", ops.front().GetName());
    std::vector<std::string> names{"existing"};
    EXPECT_EQ(GRAPH_FAILED, graph.GetAllOpName(names));
    EXPECT_EQ((std::vector<std::string>{"existing"}), names);
    std::vector<AscendString> ascend_names{AscendString("existing")};
    EXPECT_EQ(GRAPH_FAILED, graph.GetAllOpName(ascend_names));
    EXPECT_EQ((std::vector<AscendString>{AscendString("existing")}), ascend_names);
    EXPECT_EQ(GRAPH_FAILED, graph.impl_->SetTargets({op}));
    EXPECT_EQ(&graph, &graph.SetTargets({op}));
    for (const bool need_iteration : {true, false}) {
      (void)ErrorManager::GetInstance().GetErrorMessage();
      graph.SetNeedIteration(need_iteration);
      const auto error = ErrorManager::GetInstance().GetErrorMessage();
      EXPECT_NE(std::string::npos, error.find("Graph::SetNeedIteration"));
    }
    EXPECT_EQ(initialize, graph.IsValid());
    EXPECT_EQ(original, GraphUtilsEx::Graph2ExecuteGraph(graph));
    EXPECT_EQ(initialize ? 1U : 0U, graph.GetDirectNode().size());
  }
}

TEST_F(ExecuteGraphFacadeTest, ExecuteGraphUnsupportedSaveOverloadsPreserveFileBufferAndGraph) {
  const std::string file_name = "./ut_execute_graph_unsupported_save.txt";
  {
    std::ofstream stream(file_name);
    ASSERT_TRUE(stream.is_open());
    stream << "sentinel";
  }
  for (const bool initialize : {false, true}) {
    Graph graph("unsupported_save", 1);
    if (initialize) {
      graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 1U));
    }
    const auto original = GraphUtilsEx::Graph2ExecuteGraph(graph);
    EXPECT_EQ(GRAPH_FAILED, graph.SaveToFile(file_name));
    EXPECT_EQ(GRAPH_FAILED, graph.SaveToFile(file_name.c_str()));
    GraphBuffer buffer;
    const auto *data = buffer.GetData();
    const auto size = buffer.GetSize();
    EXPECT_EQ(GRAPH_FAILED, graph.SaveToMem(buffer));
    EXPECT_EQ(data, buffer.GetData());
    EXPECT_EQ(size, buffer.GetSize());
    std::ifstream stream(file_name);
    std::string content;
    stream >> content;
    EXPECT_EQ("sentinel", content);
    EXPECT_EQ(initialize, graph.IsValid());
    EXPECT_EQ(original, GraphUtilsEx::Graph2ExecuteGraph(graph));
  }
  EXPECT_EQ(0, std::remove(file_name.c_str()));
}

TEST_F(ExecuteNodeFacadeTest, ExecuteNodeAllAttributeOverloadsRoundTrip) {
  Graph graph("all_attrs", 1);
  auto node = graph.AddNodeByOp(MakeExecuteOp("node", "PlainOp", 0U, 0U));
  const auto check = [&node](const char *name, auto value) {
    decltype(value) read{};
    ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString(name), value));
    ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString(name), read));
    EXPECT_EQ(value, read);
  };
  check("i64", int64_t{-5});
  check("i32", int32_t{7});
  check("u32", uint32_t{9});
  check("bool", true);
  check("str", AscendString("text"));
  check("list_i64", std::vector<int64_t>{1, -2});
  check("list_i32", std::vector<int32_t>{3, -4});
  check("list_u32", std::vector<uint32_t>{5, 6});
  check("list_float", std::vector<float32_t>{1.25F, -2.5F});
  check("list_str", std::vector<AscendString>{AscendString("first"), AscendString("last")});
  check("list_bool", std::vector<bool>{true, false});
  check("bytes", OpBytes{7, 8, 9});
  check("list_list_i64", std::vector<std::vector<int64_t>>{{1, 2}, {}, {3}});
  check("list_dtype", std::vector<DataType>{DT_FLOAT, DT_INT32});
  check("dtype", DT_UINT8);

  float32_t value = 1.25F;
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("float"), value));
  float32_t read_float = 0.0F;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("float"), read_float));
  EXPECT_FLOAT_EQ(value, read_float);
  const auto check_tensor = [](const Tensor &expected, const Tensor &actual) {
    EXPECT_EQ(expected.GetDataType(), actual.GetDataType());
    EXPECT_EQ(expected.GetTensorDesc().GetFormat(), actual.GetTensorDesc().GetFormat());
    EXPECT_EQ(expected.GetTensorDesc().GetShape().GetDims(), actual.GetTensorDesc().GetShape().GetDims());
    ASSERT_EQ(expected.GetSize(), actual.GetSize());
    ASSERT_NE(nullptr, actual.GetData());
    EXPECT_EQ(0, memcmp(expected.GetData(), actual.GetData(), expected.GetSize()));
  };
  Tensor tensor(TensorDesc(Shape({2}), FORMAT_ND, DT_UINT8), std::vector<uint8_t>{4, 9});
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("tensor"), tensor));
  Tensor read_tensor;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("tensor"), read_tensor));
  check_tensor(tensor, read_tensor);
  std::vector<Tensor> tensors{tensor, tensor};
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("list_tensor"), tensors));
  std::vector<Tensor> read_tensors;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("list_tensor"), read_tensors));
  ASSERT_EQ(tensors.size(), read_tensors.size());
  for (size_t i = 0U; i < tensors.size(); ++i) {
    check_tensor(tensors[i], read_tensors[i]);
  }
  AttrValue attr;
  ASSERT_EQ(GRAPH_SUCCESS, attr.SetAttrValue(int64_t{23}));
  ASSERT_EQ(GRAPH_SUCCESS, node.SetAttr(AscendString("attr_value"), attr));
  AttrValue read_attr;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetAttr(AscendString("attr_value"), read_attr));
  int64_t read_value = 0;
  ASSERT_EQ(GRAPH_SUCCESS, read_attr.GetValue(read_value));
  EXPECT_EQ(23, read_value);
}
}  // namespace ge
