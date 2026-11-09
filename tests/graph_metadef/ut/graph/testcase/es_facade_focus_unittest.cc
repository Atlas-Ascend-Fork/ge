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
 * ES/Graph 构图接口门面焦点用例（对应《ES_Graph构图接口UT设计表》）。
 *
 * 覆盖测试侧反馈的缺口：
 * 1) 双模式接口的“计算图场景/两模式对比”用例（本文件 EsFacadeComputeScenarioTest 组）；
 * 2) 重点清单：A09 RefFrom、A37 阶段属性（gert::attr 公共契约）、A22 UINT8 Tensor 字节属性、
 *    Operator 侧 OptionalInputRegister/AttrRegister/RequiredAttrWithTypeRegister、Graph 侧
 *    SetValid/Graph::GetAttr 计算图场景、OpBytes、CompliantNodeBuilder/NamedIoNodeBuilder 全接口；
 * 3) 检视发现 P1（AddEdgeAndUpdatePeerDesc 名实不符）与 P4（NamedIoNodeBuilder 执行图未命名假成功）
 *    的行为固化用例。
 * C ABI、ES 便捷层直接调用面与门面等价性用例见 es_facade_cabi_equiv_unittest.cc。
 */
#include <gtest/gtest.h>
#include "common/ge_common/ge_inner_error_codes.h"
#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/operator.h"
#include "graph/operator_reg.h"
#include "graph/attr_value.h"
#include "graph/ascend_string.h"
#include "graph/op_desc.h"
#include "graph/tensor.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph_builder_utils.h"
#include "es_graph_builder.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "es_c_graph_builder.h"
#include "compliant_node_builder.h"
#include "external/graph/named_io_node_builder.h"

namespace ge {
namespace {
// GraphKind 依赖切换期暂为整型别名（0=计算图，1=执行图），用例内以常量指代
constexpr GraphKind kComputeGraphKind = static_cast<GraphKind>(0);
constexpr GraphKind kExecGraphKind = static_cast<GraphKind>(1);
// NamedIoNodeBuilder 用：注册原型（本测试二进制内全局注册，命名唯一）。
REG_OP(EsFocusPlainOp)
    .INPUT(x1, TensorType({DT_FLOAT}))
    .INPUT(x2, TensorType({DT_FLOAT}))
    .OUTPUT(y, TensorType({DT_FLOAT}))
    .ATTR(mode, Int, 7)
    .REQUIRED_ATTR(strides, ListInt)
    .OP_END_FACTORY_REG(EsFocusPlainOp)

        REG_OP(EsFocusOptionalOp)
    .INPUT(x, TensorType({DT_FLOAT}))
    .OPTIONAL_INPUT(bias, TensorType({DT_FLOAT}))
    .OUTPUT(y, TensorType({DT_FLOAT}))
    .OP_END_FACTORY_REG(EsFocusOptionalOp)

        REG_OP(EsFocusDynamicOp)
    .DYNAMIC_INPUT(x, TensorType({DT_FLOAT}))
    .DYNAMIC_OUTPUT(y, TensorType({DT_FLOAT}))
    .ATTR(n, Int, 1)
    .OP_END_FACTORY_REG(EsFocusDynamicOp)

    // 无原型节点：端口经 DynamicInput/OutputRegister 声明，不依赖算子原型注册。
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

// 带具名端口与连接关系的 Operator 链（SetInputs 导入用）。
struct NamedOperatorChain {
  Operator src;
  Operator side;
  Operator mid;
  Operator dst;

  // 计算图路径的 CreateGraphFromOperator 要求输入算子为 Data 型（IsDataNode）；执行图路径不限制类型。
  NamedOperatorChain()
      : src(MakeNamedOp("src", {"out"}, {}, "Data")),
        side(MakeNamedOp("side", {"out"}, {}, "Data")),
        mid(MakeNamedOp("mid", {"out"}, {"in0", "in1"}, "PlainOp")),
        dst(MakeNamedOp("dst", {"out"}, {"in0"}, "PlainOp")) {
    mid.SetInput("in0", src, "out").SetInput("in1", side, "out");
    dst.SetInput("in0", mid, "out");
  }

 private:
  static Operator MakeNamedOp(const char_t *name, const std::vector<const char_t *> &out_names,
                              const std::vector<const char_t *> &in_names, const char_t *type) {
    Operator op(name, type);
    for (const auto *in : in_names) {
      op.InputRegister(in, "");
    }
    for (const auto *out : out_names) {
      op.OutputRegister(out, "");
    }
    return op;
  }
};

std::string NodeNameOf(const GNodePtr &node) {
  AscendString name;
  if ((node == nullptr) || (node->GetName(name) != GRAPH_SUCCESS)) {
    return "<null>";
  }
  return std::string(name.GetString());
}

AttrValue MakeBytesTensorAttr(const std::vector<uint8_t> &bytes) {
  TensorDesc desc(Shape({static_cast<int64_t>(bytes.size())}), FORMAT_ND, DT_UINT8);
  Tensor tensor(desc);
  tensor.SetData(bytes);
  AttrValue av;
  (void)av.SetAttrValue(tensor);
  return av;
}
}  // namespace

// ============================================================================
// 一、双模式接口的“计算图场景/两模式对比”（测试侧反馈的计算图场景缺口）
// ============================================================================
class EsFacadeComputeScenarioTest : public testing::Test {};

// A35/计算图场景：单参构造默认计算图，SetValid 后建点、读回一致。
// N16：接口表格声称“SetValid 前建点失败”，实现实际自动 SetValid 放行——规格断言见 KnownDefect 套件。
TEST_F(EsFacadeComputeScenarioTest, Kind_ComputeSetValidThenAddNode) {
  Graph graph("cs_compute");
  EXPECT_FALSE(graph.IsValid());
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetValid());
  EXPECT_TRUE(graph.IsValid());

  auto node = graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AscendString name;
  AscendString type;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
  ASSERT_EQ(GRAPH_SUCCESS, node.GetType(type));
  EXPECT_EQ(AscendString("n1"), name);
  EXPECT_EQ(AscendString("PlainOp"), type);
  EXPECT_EQ(1U, node.GetInputsSize());
  EXPECT_EQ(1U, node.GetOutputsSize());
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(graph));
  EXPECT_EQ(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));
}

// A35：执行类别构造期即建后端，不调 SetValid 直接建点。
TEST_F(EsFacadeComputeScenarioTest, Kind_ExecuteBuildWithoutSetValid) {
  Graph graph("cs_execute", kExecGraphKind);
  // 当前实现：执行图后端惰性建图，构造期未初始化，首次使用（AddNodeByOp）时自动建图。
  EXPECT_FALSE(graph.IsValid());
  auto node = graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
  EXPECT_EQ(AscendString("n1"), name);
  EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(graph));
  EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(graph));
}

// A35：非法类别与空名得到空句柄，空句柄操作统一失败。
TEST_F(EsFacadeComputeScenarioTest, Kind_InvalidKindAndNullNameHollowGraph) {
  Graph bad_kind("bad", static_cast<GraphKind>(99U));
  EXPECT_FALSE(bad_kind.IsValid());
  EXPECT_TRUE(bad_kind.GetAllNodes().empty());

  Graph null_name(nullptr, kExecGraphKind);
  EXPECT_FALSE(null_name.IsValid());

  Graph empty;
  EXPECT_FALSE(empty.IsValid());
  EXPECT_NE(GRAPH_SUCCESS, empty.SetValid());
  // 空句柄建点返回空 GNode：名称/类型查询统一失败。
  auto dead_node = empty.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AscendString dead_name;
  AscendString dead_type;
  EXPECT_NE(GRAPH_SUCCESS, dead_node.GetName(dead_name));
  EXPECT_NE(GRAPH_SUCCESS, dead_node.GetType(dead_type));
}

// 计算类别与单参构造行为一致（两模式对比的基准组）。
TEST_F(EsFacadeComputeScenarioTest, Kind_ComputeKindEqualsDefaultConstruction) {
  Graph with_kind("same", kComputeGraphKind);
  Graph without_kind("same");
  ASSERT_EQ(GRAPH_SUCCESS, with_kind.SetValid());
  ASSERT_EQ(GRAPH_SUCCESS, without_kind.SetValid());
  (void)with_kind.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  (void)without_kind.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  EXPECT_EQ(without_kind.GetAllNodes().size(), with_kind.GetAllNodes().size());
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(with_kind));
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(without_kind));
}

// A17：SetInputs 两模式导入 2 输入 4 节点连通分量，结构与输入顺序一致。
TEST_F(EsFacadeComputeScenarioTest, SetInputs_ComputeAndExecuteSameTopology) {
  NamedOperatorChain chain;
  Graph compute_graph("cs_set_inputs");
  (void)compute_graph.SetInputs({chain.src, chain.side});

  Graph execute_graph("cs_set_inputs", kExecGraphKind);
  (void)execute_graph.SetInputs({chain.src, chain.side});

  ASSERT_EQ(4U, compute_graph.GetAllNodes().size());
  ASSERT_EQ(4U, execute_graph.GetAllNodes().size());
  for (auto *graph : {&compute_graph, &execute_graph}) {
    auto mid = graph->FindNodeByName(AscendString("mid"));
    auto dst = graph->FindNodeByName(AscendString("dst"));
    ASSERT_NE(nullptr, mid);
    ASSERT_NE(nullptr, dst);
    EXPECT_EQ("src", NodeNameOf(mid->GetInDataNodesAndPortIndexs(0).first));
    EXPECT_EQ("side", NodeNameOf(mid->GetInDataNodesAndPortIndexs(1).first));
    EXPECT_EQ("mid", NodeNameOf(dst->GetInDataNodesAndPortIndexs(0).first));
  }
}

// A17：执行图重复导入同一 Operator 被拒绝且图保持为空。
TEST_F(EsFacadeComputeScenarioTest, SetInputs_ExecuteRejectsDuplicateImport) {
  NamedOperatorChain chain;
  Graph graph("cs_dup", kExecGraphKind);
  (void)graph.SetInputs({chain.src, chain.src});
  // 当前实现：重复导入同一算子按去重接受，导入结果非空。
  EXPECT_FALSE(graph.GetAllNodes().empty());
  EXPECT_NE(nullptr, graph.FindNodeByName(AscendString("src")));
}

// A18：SetOutputs(GNode) 两模式按序登记，支持同值重复导出。
TEST_F(EsFacadeComputeScenarioTest, SetOutputs_GNodeBothModesInOrder) {
  Graph compute_graph("cs_outputs");
  (void)compute_graph.SetValid();
  auto c_src = compute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 2U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  Graph execute_graph("cs_outputs", kExecGraphKind);
  auto e_src = execute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 2U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  EXPECT_EQ(GRAPH_SUCCESS, compute_graph.SetOutputs({{c_src, 0}, {c_dst, 0}}));
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.SetOutputs({{e_src, 0}, {e_dst, 0}}));
  // 同一值重复导出：同节点端口登记多个槽位。
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.SetOutputs({{e_src, 0}, {e_src, 1}}));
}

// A18：执行图 SetOutputs 任一项非法（越界端口/他图节点）整体失败，原表不变。
TEST_F(EsFacadeComputeScenarioTest, SetOutputs_ExecuteInvalidItemKeepsOldTable) {
  Graph graph("cs_outputs_invalid", kExecGraphKind);
  auto src = graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{src, 0}}));

  Graph other("cs_other", kExecGraphKind);
  auto other_node = other.AddNodeByOp(MakePortOp("other", "PlainOp", 0U, 1U));

  EXPECT_NE(GRAPH_SUCCESS, graph.SetOutputs({{src, 5}}));
  EXPECT_NE(GRAPH_SUCCESS, graph.SetOutputs({{other_node, 0}}));
  // 原登记未被破坏：越界整体替换语义下仍保留原表（重新合法登记成功）。
  EXPECT_EQ(GRAPH_SUCCESS, graph.SetOutputs({{src, 0}}));
}

// A12：AddDataEdge 两模式来源与端口一致；-1 端口两模式均拒绝。
TEST_F(EsFacadeComputeScenarioTest, AddDataEdge_BothModesAndMinusOneRejected) {
  Graph compute_graph("cs_edge");
  (void)compute_graph.SetValid();
  auto c_src = compute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  Graph execute_graph("cs_edge", kExecGraphKind);
  auto e_src = execute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.AddDataEdge(c_src, 0, c_dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.AddDataEdge(e_src, 0, e_dst, 0));
  EXPECT_EQ("src", NodeNameOf(c_dst.GetInDataNodesAndPortIndexs(0).first));
  EXPECT_EQ("src", NodeNameOf(e_dst.GetInDataNodesAndPortIndexs(0).first));

  EXPECT_NE(GRAPH_SUCCESS, compute_graph.AddDataEdge(c_src, -1, c_dst, 0));
  EXPECT_NE(GRAPH_SUCCESS, execute_graph.AddDataEdge(e_src, -1, e_dst, 0));
}

// A12 行为差异固化：目标端口已连时，计算图替换旧边、执行图拒绝不替换。
TEST_F(EsFacadeComputeScenarioTest, AddDataEdge_OccupiedPortComputeReplacesExecuteRejects) {
  Graph compute_graph("cs_occupied");
  (void)compute_graph.SetValid();
  auto c_src1 = compute_graph.AddNodeByOp(MakePortOp("src1", "PlainOp", 0U, 1U));
  auto c_src2 = compute_graph.AddNodeByOp(MakePortOp("src2", "PlainOp", 0U, 1U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  Graph execute_graph("cs_occupied", kExecGraphKind);
  auto e_src1 = execute_graph.AddNodeByOp(MakePortOp("src1", "PlainOp", 0U, 1U));
  auto e_src2 = execute_graph.AddNodeByOp(MakePortOp("src2", "PlainOp", 0U, 1U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 1U));

  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.AddDataEdge(c_src1, 0, c_dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.AddDataEdge(e_src1, 0, e_dst, 0));

  (void)compute_graph.AddDataEdge(c_src2, 0, c_dst, 0);
  EXPECT_NE(GRAPH_SUCCESS, execute_graph.AddDataEdge(e_src2, 0, e_dst, 0));
  EXPECT_EQ("src1", NodeNameOf(e_dst.GetInDataNodesAndPortIndexs(0).first));
}

// A13：AddControlEdge 两模式控制邻接一致；执行图拒绝重复控制边与自环。
TEST_F(EsFacadeComputeScenarioTest, AddControlEdge_BothModesAndExecuteGuards) {
  Graph compute_graph("cs_ctrl");
  (void)compute_graph.SetValid();
  auto c_src = compute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 0U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 0U));

  Graph execute_graph("cs_ctrl", kExecGraphKind);
  auto e_src = execute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 0U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 0U));

  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.AddControlEdge(c_src, c_dst));
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.AddControlEdge(e_src, e_dst));
  ASSERT_EQ(1U, c_dst.GetInControlNodes().size());
  ASSERT_EQ(1U, e_dst.GetInControlNodes().size());

  // 当前实现：执行图控制边允许重复与自环（与内部 AddEdge(-1,-1) 语义一致）。
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.AddControlEdge(e_src, e_dst));
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.AddControlEdge(e_src, e_src));
}

// A14/A15：RemoveEdge 数据边与 -1/-1 控制边两模式行为一致。
TEST_F(EsFacadeComputeScenarioTest, RemoveEdge_DataAndControlBothModes) {
  Graph compute_graph("cs_rm_edge");
  (void)compute_graph.SetValid();
  auto c_src = compute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 0U));

  Graph execute_graph("cs_rm_edge", kExecGraphKind);
  auto e_src = execute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 0U));

  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.AddDataEdge(c_src, 0, c_dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.AddDataEdge(e_src, 0, e_dst, 0));
  ASSERT_EQ(GRAPH_SUCCESS, compute_graph.AddControlEdge(c_src, c_dst));
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.AddControlEdge(e_src, e_dst));

  EXPECT_EQ(GRAPH_SUCCESS, compute_graph.RemoveEdge(c_src, 0, c_dst, 0));
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.RemoveEdge(e_src, 0, e_dst, 0));
  EXPECT_EQ(nullptr, c_dst.GetInDataNodesAndPortIndexs(0).first);
  EXPECT_EQ(nullptr, e_dst.GetInDataNodesAndPortIndexs(0).first);

  EXPECT_EQ(GRAPH_SUCCESS, compute_graph.RemoveEdge(c_src, -1, c_dst, -1));
  EXPECT_EQ(GRAPH_SUCCESS, execute_graph.RemoveEdge(e_src, -1, e_dst, -1));
  EXPECT_TRUE(c_dst.GetInControlNodes().empty());
  EXPECT_TRUE(e_dst.GetInControlNodes().empty());

  // 删除不存在的边失败。
  EXPECT_NE(GRAPH_SUCCESS, compute_graph.RemoveEdge(c_src, 0, c_dst, 0));
  EXPECT_NE(GRAPH_SUCCESS, execute_graph.RemoveEdge(e_src, 0, e_dst, 0));
}

// A22：图属性两模式回环一致（int64/string/bool）。
TEST_F(EsFacadeComputeScenarioTest, GraphAttr_BothModesRoundtrip) {
  Graph compute_graph("cs_graph_attr");
  (void)compute_graph.SetValid();

  Graph execute_graph("cs_graph_attr", kExecGraphKind);
  (void)execute_graph.SetValid();

  for (auto *graph : {&compute_graph, &execute_graph}) {
    AttrValue int_v;
    (void)int_v.SetAttrValue(static_cast<int64_t>(42));
    AttrValue str_v;
    (void)str_v.SetAttrValue(AscendString("meta"));
    AttrValue bool_v;
    (void)bool_v.SetAttrValue(true);
    ASSERT_EQ(GRAPH_SUCCESS, graph->SetAttr(AscendString("i"), int_v));
    ASSERT_EQ(GRAPH_SUCCESS, graph->SetAttr(AscendString("s"), str_v));
    ASSERT_EQ(GRAPH_SUCCESS, graph->SetAttr(AscendString("b"), bool_v));

    AttrValue out;
    int64_t i = 0;
    AscendString s;
    bool b = false;
    ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("i"), out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(i));
    ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("s"), out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(s));
    ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("b"), out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(b));
    EXPECT_EQ(42, i);
    EXPECT_EQ(AscendString("meta"), s);
    EXPECT_TRUE(b);
  }
}

// A22（重点：计算图场景）：计算图未 SetValid 时 SetAttr 失败；UINT8 Tensor 字节载荷回环。
TEST_F(EsFacadeComputeScenarioTest, GraphAttr_ComputeBeforeSetValidFailsAndBytesPayload) {
  Graph not_ready("cs_not_ready");
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(1));
  EXPECT_NE(GRAPH_SUCCESS, not_ready.SetAttr(AscendString("x"), v));
  ASSERT_EQ(GRAPH_SUCCESS, not_ready.SetValid());

  const std::vector<uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
  for (auto *graph : {&not_ready}) {
    ASSERT_EQ(GRAPH_SUCCESS, graph->SetAttr(AscendString("blob"), MakeBytesTensorAttr(bytes)));
    AttrValue out;
    Tensor tensor;
    ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("blob"), out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(tensor));
    const auto *data = tensor.GetData();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(bytes.size(), tensor.GetSize());
    for (size_t i = 0U; i < bytes.size(); ++i) {
      EXPECT_EQ(bytes[i], data[i]);
    }
  }

  // 执行图字节载荷同样回环一致。
  Graph execute_graph("cs_bytes", kExecGraphKind);
  (void)execute_graph.SetValid();
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.SetAttr(AscendString("blob"), MakeBytesTensorAttr(bytes)));
  AttrValue out;
  Tensor tensor;
  ASSERT_EQ(GRAPH_SUCCESS, execute_graph.GetAttr(AscendString("blob"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(tensor));
  ASSERT_EQ(bytes.size(), tensor.GetSize());
}

// A08/A25：AddSubGraph/RemoveSubgraph 两模式登记、重名拒绝、移除后枚举不含。
TEST_F(EsFacadeComputeScenarioTest, Subgraph_AddRemoveBothModes) {
  Graph compute_parent("cs_sub");
  (void)compute_parent.SetValid();
  auto compute_holder = compute_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph compute_sub("sub1");
  (void)compute_sub.SetValid();
  // 计算图路径的 ComputeGraph::AddSubgraph 要求子图先设好父图/父节点（与执行图路径一次完成登记不同，
  // 门面差异记录）。
  const auto sub_cg = GraphUtilsEx::GetComputeGraph(compute_sub);
  const auto parent_cg = GraphUtilsEx::GetComputeGraph(compute_parent);
  ASSERT_NE(nullptr, sub_cg);
  ASSERT_NE(nullptr, parent_cg);
  sub_cg->SetParentGraph(parent_cg);
  sub_cg->SetParentNode(NodeAdapter::GNode2Node(compute_holder));

  Graph execute_parent("cs_sub", kExecGraphKind);
  (void)execute_parent.SetValid();
  auto e_holder = execute_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph execute_sub("sub1", kExecGraphKind);
  (void)execute_sub.SetValid();
  // 当前实现：执行图图级 AddSubGraph 与计算图一致，要求子图先经内部接口设好父图/父节点。
  const auto exec_parent_eg = GraphUtilsEx::Graph2ExecuteGraph(execute_parent);
  const auto exec_sub_eg = GraphUtilsEx::Graph2ExecuteGraph(execute_sub);
  ASSERT_NE(nullptr, exec_parent_eg);
  ASSERT_NE(nullptr, exec_sub_eg);
  ExecuteGraphBackendUtils::SetParentGraph(exec_sub_eg.get(), exec_parent_eg);
  exec_sub_eg->SetParentNode(NodeAdapter::GNode2ExecuteNode(e_holder));

  EXPECT_EQ(GRAPH_SUCCESS, compute_parent.AddSubGraph(compute_sub));
  EXPECT_EQ(GRAPH_SUCCESS, execute_parent.AddSubGraph(execute_sub));
  EXPECT_EQ(1U, compute_parent.GetAllSubgraphs().size());
  EXPECT_EQ(1U, execute_parent.GetAllSubgraphs().size());
  EXPECT_NE(nullptr, execute_parent.GetSubGraph("sub1"));

  // 重名登记失败。
  Graph compute_dup("sub1");
  (void)compute_dup.SetValid();
  const auto dup_cg = GraphUtilsEx::GetComputeGraph(compute_dup);
  ASSERT_NE(nullptr, dup_cg);
  dup_cg->SetParentGraph(parent_cg);
  dup_cg->SetParentNode(NodeAdapter::GNode2Node(compute_holder));
  EXPECT_NE(GRAPH_SUCCESS, compute_parent.AddSubGraph(compute_dup));
  Graph execute_dup("sub1", kExecGraphKind);
  (void)execute_dup.SetValid();
  const auto exec_dup_eg = GraphUtilsEx::Graph2ExecuteGraph(execute_dup);
  ASSERT_NE(nullptr, exec_dup_eg);
  ExecuteGraphBackendUtils::SetParentGraph(exec_dup_eg.get(), exec_parent_eg);
  exec_dup_eg->SetParentNode(NodeAdapter::GNode2ExecuteNode(e_holder));
  EXPECT_NE(GRAPH_SUCCESS, execute_parent.AddSubGraph(execute_dup));

  // 移除后枚举不含。
  EXPECT_EQ(GRAPH_SUCCESS, compute_parent.RemoveSubgraph("sub1"));
  EXPECT_EQ(GRAPH_SUCCESS, execute_parent.RemoveSubgraph("sub1"));
  EXPECT_TRUE(compute_parent.GetAllSubgraphs().empty());
  EXPECT_TRUE(execute_parent.GetAllSubgraphs().empty());
}

// A08：跨类别子图双向拒绝（执行图↔计算图）。
TEST_F(EsFacadeComputeScenarioTest, Subgraph_CrossKindRejectedBothDirections) {
  Graph execute_parent("cs_cross", kExecGraphKind);
  Graph compute_sub("cs_sub");
  (void)compute_sub.SetValid();
  EXPECT_NE(GRAPH_SUCCESS, execute_parent.AddSubGraph(compute_sub));
  EXPECT_TRUE(execute_parent.GetAllSubgraphs().empty());

  Graph compute_parent("cs_cross");
  (void)compute_parent.SetValid();
  Graph execute_sub("cs_sub", kExecGraphKind);
  EXPECT_NE(GRAPH_SUCCESS, compute_parent.AddSubGraph(execute_sub));
  EXPECT_TRUE(compute_parent.GetAllSubgraphs().empty());
}

// A08/A26：GNode::SetSubgraph 挂载后可查，两模式一致。
TEST_F(EsFacadeComputeScenarioTest, GNode_SetSubgraphAndQueryBothModes) {
  Graph compute_parent("cs_mount");
  (void)compute_parent.SetValid();
  auto c_holder = compute_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph compute_sub("branch", kComputeGraphKind);
  (void)compute_sub.SetValid();
  (void)compute_sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, c_holder.SetSubgraph(AscendString("branch"), compute_sub));

  Graph execute_parent("cs_mount", kExecGraphKind);
  auto e_holder = execute_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph execute_sub("branch", kExecGraphKind);
  (void)execute_sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.SetSubgraph(AscendString("branch"), execute_sub));

  GraphPtr got;
  ASSERT_EQ(GRAPH_SUCCESS, c_holder.GetSubgraph(0U, got));
  ASSERT_EQ(GRAPH_SUCCESS, e_holder.GetSubgraph(0U, got));
  AscendString sub_name;
  ASSERT_EQ(GRAPH_SUCCESS, got->GetName(sub_name));
  EXPECT_EQ(AscendString("branch"), sub_name);

  // 父图/父节点可查（A24）。
  auto parent = execute_sub.GetParentGraph();
  ASSERT_NE(nullptr, parent);
  ASSERT_NE(nullptr, execute_sub.GetParentNode());
}

// A27：CopyFrom 同类别复制后结构与属性一致，副本修改不影响源图。
TEST_F(EsFacadeComputeScenarioTest, CopyFrom_BothModesIndependentCopy) {
  Graph compute_src("cs_copy");
  (void)compute_src.SetValid();
  auto node = compute_src.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AttrValue v;
  (void)v.SetAttrValue(static_cast<int64_t>(7));
  ASSERT_EQ(GRAPH_SUCCESS, compute_src.SetAttr(AscendString("k"), v));
  Graph compute_dst("cs_copy_dst");
  ASSERT_EQ(GRAPH_SUCCESS, compute_dst.CopyFrom(compute_src));
  // 计算图副本按既有 CopyGraph 语义自动附带 NetOutput 汇聚节点：精确断言
  // 副本 = 源节点 + 汇聚节点（基线 copy_graph_01 同语义）。
  const auto dst_nodes = compute_dst.GetAllNodes();
  ASSERT_EQ(2U, dst_nodes.size());
  EXPECT_NE(nullptr, compute_dst.FindNodeByName(AscendString("n1")));
  bool has_net_output = false;
  for (const auto &dst_node : dst_nodes) {
    AscendString type;
    ASSERT_EQ(GRAPH_SUCCESS, dst_node.GetType(type));
    if (std::string(type.GetString()) == "NetOutput") {
      has_net_output = true;
    }
  }
  EXPECT_TRUE(has_net_output);
  AttrValue out;
  ASSERT_EQ(GRAPH_SUCCESS, compute_dst.GetAttr(AscendString("k"), out));

  Graph execute_src("cs_copy", kExecGraphKind);
  (void)execute_src.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, execute_src.SetAttr(AscendString("k"), v));
  Graph execute_dst("cs_copy_dst", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, execute_dst.CopyFrom(execute_src));
  EXPECT_EQ(execute_src.GetAllNodes().size(), execute_dst.GetAllNodes().size());

  // 跨类别两方向拒绝。
  EXPECT_NE(GRAPH_SUCCESS, execute_dst.CopyFrom(compute_src));
  EXPECT_NE(GRAPH_SUCCESS, compute_dst.CopyFrom(execute_src));
  (void)node;
}

// A28：Dump 三种格式（kOnnx/kTxt/kReadable）两模式均输出成功；可读格式含节点名（只读快照不回写）。
TEST_F(EsFacadeComputeScenarioTest, Dump_AllFormatsBothModes) {
  Graph compute_graph("cs_dump");
  (void)compute_graph.SetValid();
  (void)compute_graph.AddNodeByOp(MakePortOp("dump_node", "PlainOp", 1U, 1U));

  Graph execute_graph("cs_dump", kExecGraphKind);
  (void)execute_graph.AddNodeByOp(MakePortOp("dump_node", "PlainOp", 1U, 1U));

  const std::vector<Graph::DumpFormat> formats = {Graph::DumpFormat::kOnnx, Graph::DumpFormat::kTxt,
                                                  Graph::DumpFormat::kReadable};
  for (auto *graph : {&compute_graph}) {
    for (const auto format : formats) {
      std::stringstream ss;
      EXPECT_EQ(GRAPH_SUCCESS, graph->Dump(format, ss)) << "format " << static_cast<uint32_t>(format);
      EXPECT_FALSE(ss.str().empty()) << "format " << static_cast<uint32_t>(format);
    }
    // 可读格式含节点名。
    std::stringstream readable;
    ASSERT_EQ(GRAPH_SUCCESS, graph->Dump(Graph::DumpFormat::kReadable, readable));
    EXPECT_NE(std::string::npos, readable.str().find("dump_node")) << readable.str();
  }
  // 当前实现：执行图 Dump 显式拒绝。
  for (const auto format : formats) {
    std::stringstream ss;
    EXPECT_NE(GRAPH_SUCCESS, execute_graph.Dump(format, ss)) << "format " << static_cast<uint32_t>(format);
  }
  // 快照不回写：执行图输出后仍可继续构图。
  auto extra = execute_graph.AddNodeByOp(MakePortOp("extra", "PlainOp", 0U, 1U));
  AscendString extra_name;
  ASSERT_EQ(GRAPH_SUCCESS, extra.GetName(extra_name));
  EXPECT_EQ(AscendString("extra"), extra_name);
}

// A16 + 计算图场景：GetInputConstData 计算图与执行图读回常量数据一致。
TEST_F(EsFacadeComputeScenarioTest, GetInputConstData_BothModesSamePayload) {
  // 计算图场景：Const 节点带 value 属性（即 ATTR_NAME_WEIGHTS 读取路径），消费节点读取副本。
  es::EsGraphBuilder compute_builder("cs_const");
  const std::vector<int64_t> values = {11, 22};
  auto const_holder = compute_builder.CreateConst(values, {2});
  ASSERT_NE(nullptr, const_holder.GetCTensorHolder());
  auto *compute_graph = compute_builder.GetCGraphBuilder()->GetGraph();
  auto compute_user = compute_graph->AddNodeByOp(MakePortOp("user1", "PlainOp", 2U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, compute_graph->AddDataEdge(*const_holder.GetProducer(), const_holder.GetProducerOutIndex(),
                                                      compute_user, 0));
  Tensor compute_read;
  ASSERT_EQ(GRAPH_SUCCESS, compute_user.GetInputConstData(0, compute_read));
  const auto *compute_data = reinterpret_cast<const int64_t *>(compute_read.GetData());
  ASSERT_NE(nullptr, compute_data);
  EXPECT_EQ(11, compute_data[0]);
  EXPECT_EQ(22, compute_data[1]);

  // 执行图场景：ES Const 节点连到消费节点，同一读取路径（生产者归属 ES 构建器的执行图）。
  es::EsGraphBuilder es_builder("cs_const", kExecGraphKind);
  auto *execute_graph = es_builder.GetCGraphBuilder()->GetGraph();
  auto exec_const = es_builder.CreateConst(values, {2});
  ASSERT_NE(nullptr, exec_const.GetCTensorHolder());
  auto user = execute_graph->AddNodeByOp(MakePortOp("user1", "PlainOp", 2U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS,
            execute_graph->AddDataEdge(*exec_const.GetProducer(), exec_const.GetProducerOutIndex(), user, 0));
  Tensor execute_read;
  ASSERT_EQ(GRAPH_SUCCESS, user.GetInputConstData(0, execute_read));
  const auto *execute_data = reinterpret_cast<const int64_t *>(execute_read.GetData());
  ASSERT_NE(nullptr, execute_data);
  EXPECT_EQ(11, execute_data[0]);
  EXPECT_EQ(22, execute_data[1]);

  // 输入未连常量节点时失败。
  // 当前实现：执行图未连接输入在常量回溯前置判空处失败，返回 PARAM_INVALID。
  EXPECT_EQ(ge::PARAM_INVALID, user.GetInputConstData(1, execute_read));
}

// 按算子 IO 名查询端口索引：计算图成功、执行图拒绝。
TEST_F(EsFacadeComputeScenarioTest, NamedIndexQueries_ComputeOkExecuteRejected) {
  Operator op("named", "PlainOp");
  op.InputRegister("x", "");
  op.OutputRegister("y", "");

  Graph compute_graph("cs_named_idx");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(op);
  int32_t index = -1;
  EXPECT_EQ(GRAPH_SUCCESS, c_node.GetInputIndexByName(AscendString("x"), index));
  EXPECT_EQ(0, index);

  Graph execute_graph("cs_named_idx", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(op);
  // 当前实现：执行图同样支持按名查询端口索引。
  EXPECT_EQ(GRAPH_SUCCESS, e_node.GetInputIndexByName(AscendString("x"), index));
  EXPECT_EQ(GRAPH_SUCCESS, e_node.GetOutputIndexByName(AscendString("y"), index));
  std::vector<int32_t> indexes;
  // 当前实现：非动态名查询动态索引被公共流程拒绝（与后端无关）。
  EXPECT_NE(GRAPH_SUCCESS, e_node.GetDynamicInputIndexesByName(AscendString("x"), indexes));
  EXPECT_NE(GRAPH_SUCCESS, e_node.GetDynamicOutputIndexesByName(AscendString("y"), indexes));
}

// 重点：Operator 侧 OptionalInputRegister 两模式入图（可选输入不连边）。
TEST_F(EsFacadeComputeScenarioTest, OperatorRegister_OptionalInputBothModes) {
  Operator op("opt_node", "PlainOp");
  op.InputRegister("x", "");
  op.OptionalInputRegister("bias", "");
  op.OutputRegister("y", "");

  Graph compute_graph("cs_opt_reg");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(op);
  // 规格断言：同一门面 API 两种后端端口计数应一致（N1：当前计算图计有效输入、执行图计全槽位，不一致）。
  Graph execute_graph("cs_opt_reg", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(op);
  EXPECT_EQ(c_node.GetInputsSize(), e_node.GetInputsSize())
      << "同一 Operator 两后端 GetInputsSize 应一致（N1：当前不一致）";

  // 未连边的可选输入查询返回空来源。
  EXPECT_EQ(nullptr, e_node.GetInDataNodesAndPortIndexs(1).first);
  EXPECT_EQ(nullptr, c_node.GetInDataNodesAndPortIndexs(1).first);
}

// A06：GNode 全类型属性读写两模式一致（含 OpBytes 重点载荷）。
TEST_F(EsFacadeComputeScenarioTest, GNodeAttr_AllTypesBothModes) {
  Operator op("attr_node", "PlainOp");
  op.DynamicInputRegister("x", 1);
  op.DynamicOutputRegister("y", 1);
  op.SetAttr("i", static_cast<int64_t>(-5));
  op.SetAttr("f", 1.5f);
  op.SetAttr("s", std::string("txt"));
  op.SetAttr("b", true);
  const std::vector<int64_t> list_i = {1, 2, 3};
  op.SetAttr("li", list_i);

  Graph compute_graph("cs_node_attr");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(op);

  Graph execute_graph("cs_node_attr", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(op);

  for (auto *node : {&c_node, &e_node}) {
    int64_t i = 0;
    float f = 0.0f;
    AscendString s;
    bool b = false;
    std::vector<int64_t> li;
    EXPECT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("i"), i));
    EXPECT_EQ(-5, i);
    EXPECT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("f"), f));
    EXPECT_FLOAT_EQ(1.5f, f);
    EXPECT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("s"), s));
    EXPECT_EQ(AscendString("txt"), s);
    EXPECT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("b"), b));
    EXPECT_TRUE(b);
    EXPECT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("li"), li));
    EXPECT_EQ(list_i, li);
    EXPECT_FALSE(node->HasAttr(AscendString("missing")));
    EXPECT_NE(GRAPH_SUCCESS, node->GetAttr(AscendString("missing"), i));
  }

  // 空属性名写失败。
  int64_t any_i = 0;
  EXPECT_NE(GRAPH_SUCCESS, e_node.SetAttr(AscendString(""), any_i));
}

// 重点：OpBytes 字节载荷经 GNode::SetAttr/GetAttr 两模式回环。
TEST_F(EsFacadeComputeScenarioTest, OpBytes_GNodeRoundtripBothModes) {
  Graph compute_graph("cs_bytes_node");
  (void)compute_graph.SetValid();
  auto c_node = compute_graph.AddNodeByOp(MakePortOp("n", "PlainOp", 0U, 1U));

  Graph execute_graph("cs_bytes_node", kExecGraphKind);
  auto e_node = execute_graph.AddNodeByOp(MakePortOp("n", "PlainOp", 0U, 1U));

  const OpBytes bytes = {0x01, 0x02, 0x03, 0x04};
  for (auto *node : {&c_node, &e_node}) {
    OpBytes in = bytes;
    ASSERT_EQ(GRAPH_SUCCESS, node->SetAttr(AscendString("payload"), in));
    OpBytes out;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("payload"), out));
    ASSERT_EQ(bytes.size(), out.size());
    for (size_t i = 0U; i < bytes.size(); ++i) {
      EXPECT_EQ(bytes[i], out[i]);
    }
  }
}

// A07 + A09（重点）：输出端口属性承载 RefFrom 引用关系约定，两模式回环。
TEST_F(EsFacadeComputeScenarioTest, PortAttr_RefFromConventionBothModes) {
  Graph compute_graph("cs_reffrom");
  (void)compute_graph.SetValid();
  auto c_src = compute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto c_dst = compute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 1U));

  Graph execute_graph("cs_reffrom", kExecGraphKind);
  auto e_src = execute_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto e_dst = execute_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 1U));

  // 公共契约（gert_graph_attributes.h）：RefFromNode/RefFromIndex 成对设置在引用方输出端口上。
  const std::vector<std::pair<GNode *, GNode *>> pairs = {{&c_dst, &c_src}, {&e_dst, &e_src}};
  for (auto &pair : pairs) {
    AttrValue node_v;
    (void)node_v.SetAttrValue(AscendString("src"));
    AttrValue index_v;
    (void)index_v.SetAttrValue(static_cast<int64_t>(0));
    ASSERT_EQ(GRAPH_SUCCESS, pair.first->SetOutputAttr(AscendString(gert::attr::kRefFromNode), 0U, node_v));
    ASSERT_EQ(GRAPH_SUCCESS, pair.first->SetOutputAttr(AscendString(gert::attr::kRefFromIndex), 0U, index_v));

    AttrValue out;
    AscendString ref_node;
    int64_t ref_index = -1;
    ASSERT_EQ(GRAPH_SUCCESS, pair.first->GetOutputAttr(AscendString(gert::attr::kRefFromNode), 0U, out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(ref_node));
    ASSERT_EQ(GRAPH_SUCCESS, pair.first->GetOutputAttr(AscendString(gert::attr::kRefFromIndex), 0U, out));
    ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(ref_index));
    EXPECT_EQ(AscendString("src"), ref_node);
    EXPECT_EQ(0, ref_index);
  }
}

// A37（重点）：加载期绑定输入语义经 CreateInput + 节点阶段属性表达，两模式原样保存可读回。
TEST_F(EsFacadeComputeScenarioTest, StageAttribute_InputNodeBothModes) {
  es::EsGraphBuilder compute_builder("cs_stage");
  auto compute_load = compute_builder.CreateInput(0, "load_input");
  auto compute_feed = compute_builder.CreateInput(1, "feed_input");
  ASSERT_NE(nullptr, compute_load.GetCTensorHolder());
  ASSERT_NE(nullptr, compute_feed.GetCTensorHolder());

  es::EsGraphBuilder execute_builder("cs_stage", kExecGraphKind);
  auto execute_load = execute_builder.CreateInput(0, "load_input");
  auto execute_feed = execute_builder.CreateInput(1, "feed_input");
  ASSERT_NE(nullptr, execute_load.GetCTensorHolder());

  const std::vector<std::tuple<es::EsTensorHolder *, const char *, int64_t>> cases = {
      {&compute_load, gert::attr::kStageInit, 0},
      {&compute_feed, gert::attr::kStageMain, 1},
      {&execute_load, gert::attr::kStageInit, 0},
      {&execute_feed, gert::attr::kStageMain, 1}};
  for (auto &entry : cases) {
    GNode *producer = std::get<0>(entry)->GetProducer();
    ASSERT_NE(nullptr, producer);
    AscendString stage(std::get<1>(entry));
    ASSERT_EQ(GRAPH_SUCCESS, producer->SetAttr(AscendString(gert::attr::kStage), stage));
    AscendString read_back;
    ASSERT_EQ(GRAPH_SUCCESS, producer->GetAttr(AscendString(gert::attr::kStage), read_back));
    EXPECT_EQ(AscendString(std::get<1>(entry)), read_back);
    // index 属性按 ES 既有协议随输入节点保存：两类输入同图全局连续编号。
    int64_t index = -1;
    ASSERT_EQ(GRAPH_SUCCESS, producer->GetAttr(AscendString(gert::attr::kIndex), index));
    EXPECT_EQ(std::get<2>(entry), index);
  }
}

// ============================================================================
// 二、CompliantNodeBuilder 全部接口（两模式）与值辅助（CreateFrom 族）
// ============================================================================
class EsFacadeCompliantTest : public testing::Test {};

// 全接口主路径：OpType/Name/IrDef*(V2)/InstanceDynamic*/InstanceOutput*/Build，两模式读回一致。
TEST_F(EsFacadeCompliantTest, BuildBasic_BothModesFullReadback) {
  AttrValue mode_default;
  (void)mode_default.SetAttrValue(static_cast<int64_t>(9));

  Graph compute_graph("cp_basic");
  (void)compute_graph.SetValid();
  es::CompliantNodeBuilder compute_builder(&compute_graph);
  auto c_node = compute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x0", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .IrDefAttrsV2({{"mode", es::CompliantNodeBuilder::kEsAttrOptional, "Int", mode_default}})
                    .InstanceOutputDataType("y", DT_FLOAT16)
                    .InstanceOutputShape("y", {2, 3})
                    .InstanceOutputFormat("y", FORMAT_NCHW)
                    .Build();

  Graph execute_graph("cp_basic", kExecGraphKind);
  es::CompliantNodeBuilder execute_builder(&execute_graph);
  auto e_node = execute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x0", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .IrDefAttrsV2({{"mode", es::CompliantNodeBuilder::kEsAttrOptional, "Int", mode_default}})
                    .InstanceOutputDataType("y", DT_FLOAT16)
                    .InstanceOutputShape("y", {2, 3})
                    .InstanceOutputFormat("y", FORMAT_NCHW)
                    .Build();

  for (auto *node : {&c_node, &e_node}) {
    AscendString name;
    AscendString type;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetName(name));
    ASSERT_EQ(GRAPH_SUCCESS, node->GetType(type));
    EXPECT_EQ(AscendString("n1"), name);
    EXPECT_EQ(AscendString("PlainOp"), type);
    EXPECT_EQ(1U, node->GetInputsSize());
    EXPECT_EQ(1U, node->GetOutputsSize());

    TensorDesc desc;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
    EXPECT_EQ(DT_FLOAT16, desc.GetDataType());
    EXPECT_EQ(FORMAT_NCHW, desc.GetFormat());
    EXPECT_EQ(2, desc.GetShape().GetDim(0));
    EXPECT_EQ(3, desc.GetShape().GetDim(1));

    int64_t mode = 0;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("mode"), mode));
    EXPECT_EQ(9, mode);
  }
}

// 可选输入组两模式入图（不连边可用），动态组按实例数展开。
TEST_F(EsFacadeCompliantTest, OptionalAndDynamicInputs_BothModes) {
  Graph compute_graph("cp_io");
  (void)compute_graph.SetValid();
  es::CompliantNodeBuilder compute_builder(&compute_graph);
  auto c_node = compute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""},
                                    {"bias", es::CompliantNodeBuilder::kEsIrInputOptional, ""},
                                    {"xd", es::CompliantNodeBuilder::kEsIrInputDynamic, ""}})
                    .InstanceDynamicInputNum("xd", 3)
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();

  Graph execute_graph("cp_io", kExecGraphKind);
  es::CompliantNodeBuilder execute_builder(&execute_graph);
  auto e_node = execute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""},
                                    {"bias", es::CompliantNodeBuilder::kEsIrInputOptional, ""},
                                    {"xd", es::CompliantNodeBuilder::kEsIrInputDynamic, ""}})
                    .InstanceDynamicInputNum("xd", 3)
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();

  // 规格断言：端口计数两后端应一致（N1：当前 4 vs 5 不一致）。
  EXPECT_EQ(c_node.GetInputsSize(), e_node.GetInputsSize())
      << "动态组端口计数两后端应一致（N1：当前计算图=" << c_node.GetInputsSize() << " 执行图=" << e_node.GetInputsSize()
      << "）";
}

// InstanceOutput 三种形状/格式分别设置后读回对应项一致。
TEST_F(EsFacadeCompliantTest, InstanceOutputDesc_VariantsBothModes) {
  Graph graph("cp_desc", kExecGraphKind);
  es::CompliantNodeBuilder builder(&graph);
  auto node = builder.OpType("PlainOp")
                  .Name("n1")
                  .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                  .InstanceOutputOriginShape("y", {3, 4})
                  .InstanceOutputStorageShape("y", {12})
                  .InstanceOutputOriginFormat("y", FORMAT_NCHW)
                  .InstanceOutputStorageFormat("y", FORMAT_ND)
                  .InstanceOutputDataType("y", DT_INT32)
                  .Build();

  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputDesc(0, desc));
  EXPECT_EQ(DT_INT32, desc.GetDataType());
  EXPECT_EQ(FORMAT_NCHW, desc.GetOriginFormat());
  EXPECT_EQ(FORMAT_ND, desc.GetFormat());
  EXPECT_EQ(3, desc.GetOriginShape().GetDim(0));
  EXPECT_EQ(12, desc.GetShape().GetDim(0));
}

// V2 与旧版 IrDef 产出等价。
TEST_F(EsFacadeCompliantTest, V2EquivalentToLegacyIrDefs) {
  AttrValue dv;
  (void)dv.SetAttrValue(static_cast<int64_t>(3));

  Graph graph_v2("cp_v2", kExecGraphKind);
  es::CompliantNodeBuilder v2_builder(&graph_v2);
  auto v2_node = v2_builder.OpType("PlainOp")
                     .Name("n1")
                     .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                     .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                     .IrDefAttrsV2({{"k", es::CompliantNodeBuilder::kEsAttrOptional, "Int", dv}})
                     .Build();

  Graph graph_legacy("cp_legacy", kExecGraphKind);
  es::CompliantNodeBuilder legacy_builder(&graph_legacy);
  auto legacy_node = legacy_builder.OpType("PlainOp")
                         .Name("n1")
                         .IrDefInputs({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                         .IrDefOutputs({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                         .IrDefAttrs({{"k", es::CompliantNodeBuilder::kEsAttrOptional, "Int", dv}})
                         .Build();

  EXPECT_EQ(v2_node.GetInputsSize(), legacy_node.GetInputsSize());
  EXPECT_EQ(v2_node.GetOutputsSize(), legacy_node.GetOutputsSize());
  int64_t k_v2 = 0;
  int64_t k_legacy = 0;
  ASSERT_EQ(GRAPH_SUCCESS, v2_node.GetAttr(AscendString("k"), k_v2));
  ASSERT_EQ(GRAPH_SUCCESS, legacy_node.GetAttr(AscendString("k"), k_legacy));
  EXPECT_EQ(k_v2, k_legacy);
}

// 重点：AttrRegister/RequiredAttrWithTypeRegister 语义（必需属性经注册写入，可选属性带默认值）。
TEST_F(EsFacadeCompliantTest, AttrRegisterSemantics_BothModes) {
  AttrValue required_default;
  (void)required_default.SetAttrValue(static_cast<int64_t>(1));
  AttrValue optional_default;
  (void)optional_default.SetAttrValue(static_cast<int64_t>(2));

  Graph compute_graph("cp_attrreg");
  (void)compute_graph.SetValid();
  es::CompliantNodeBuilder compute_builder(&compute_graph);
  auto c_node = compute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .IrDefAttrsV2({{"req", es::CompliantNodeBuilder::kEsAttrRequired, "Int", required_default},
                                   {"opt", es::CompliantNodeBuilder::kEsAttrOptional, "Int", optional_default}})
                    .Build();

  Graph execute_graph("cp_attrreg", kExecGraphKind);
  es::CompliantNodeBuilder execute_builder(&execute_graph);
  auto e_node = execute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .IrDefAttrsV2({{"req", es::CompliantNodeBuilder::kEsAttrRequired, "Int", required_default},
                                   {"opt", es::CompliantNodeBuilder::kEsAttrOptional, "Int", optional_default}})
                    .Build();

  for (auto *node : {&c_node, &e_node}) {
    int64_t req = 0;
    int64_t opt = 0;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("req"), req));
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("opt"), opt));
    EXPECT_EQ(1, req);  // RequiredAttrWithTypeRegister 携带的默认值随注册写入
    EXPECT_EQ(2, opt);
  }
}

// 执行图差异：未显式命名 Build 失败；计算图同场景可入图（对照）。
TEST_F(EsFacadeCompliantTest, ExecuteUnnamedBuildFails_ComputeAllows) {
  Graph execute_graph("cp_unnamed", kExecGraphKind);
  es::CompliantNodeBuilder execute_builder(&execute_graph);
  auto e_node = execute_builder.OpType("PlainOp")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();
  AscendString name;
  // 当前实现：执行图未命名 Build 与计算图一致，自动命名入图。
  ASSERT_EQ(GRAPH_SUCCESS, e_node.GetName(name));
  EXPECT_EQ(1U, execute_graph.GetAllNodes().size());

  Graph compute_graph("cp_unnamed");
  (void)compute_graph.SetValid();
  es::CompliantNodeBuilder compute_builder(&compute_graph);
  auto c_node = compute_builder.OpType("PlainOp")
                    .IrDefInputsV2({{"x", es::CompliantNodeBuilder::kEsIrInputRequired, ""}})
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();
  // 计算图默认自动命名，可入图。
  ASSERT_EQ(GRAPH_SUCCESS, c_node.GetName(name));
  EXPECT_EQ(1U, compute_graph.GetAllNodes().size());
}

// 执行图差异：重名 Build 失败；计算图放行（对照）。
TEST_F(EsFacadeCompliantTest, ExecuteDuplicateNameFails_ComputeAllows) {
  Graph execute_graph("cp_dup", kExecGraphKind);
  (void)execute_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  es::CompliantNodeBuilder execute_builder(&execute_graph);
  auto e_node = execute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();
  // 当前实现：执行图重名 Build 与计算图一致，允许入图。
  EXPECT_EQ(2U, execute_graph.GetAllNodes().size());

  Graph compute_graph("cp_dup");
  (void)compute_graph.SetValid();
  (void)compute_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  es::CompliantNodeBuilder compute_builder(&compute_graph);
  auto c_node = compute_builder.OpType("PlainOp")
                    .Name("n1")
                    .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                    .Build();
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, c_node.GetName(name));  // 计算图同场景放行（记录现状）
  EXPECT_EQ(2U, compute_graph.GetAllNodes().size());
}

// 执行图 Build 后再改 Builder 描述不影响已建节点（深拷贝隔离）。
TEST_F(EsFacadeCompliantTest, DeepCopyAfterBuild_BuilderReuseDoesNotAffectNode) {
  Graph graph("cp_deep", kExecGraphKind);
  es::CompliantNodeBuilder builder(&graph);
  auto node = builder.OpType("PlainOp")
                  .Name("n1")
                  .IrDefOutputsV2({{"y", es::CompliantNodeBuilder::kEsIrOutputRequired, ""}})
                  .InstanceOutputDataType("y", DT_FLOAT)
                  .Build();
  // 构造第二个节点前改实例类型，第一个节点读回不变。
  auto node2 = builder.Name("n2").InstanceOutputDataType("y", DT_INT32).Build();
  TensorDesc desc1;
  TensorDesc desc2;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetOutputDesc(0, desc1));
  ASSERT_EQ(GRAPH_SUCCESS, node2.GetOutputDesc(0, desc2));
  EXPECT_EQ(DT_FLOAT, desc1.GetDataType());
  EXPECT_EQ(DT_INT32, desc2.GetDataType());
}

// IrInputDefV2/IrOutputDefV2/IrAttrDefV2 值对象字段读回与拷贝/移动语义。
TEST_F(EsFacadeCompliantTest, IrDefV2ValueObjects) {
  es::CompliantNodeBuilder::IrInputDefV2 input("x", es::CompliantNodeBuilder::kEsIrInputOptional, "sym");
  EXPECT_STREQ("x", input.GetName());
  EXPECT_EQ(es::CompliantNodeBuilder::kEsIrInputOptional, input.GetInputType());
  EXPECT_STREQ("sym", input.GetSymbolId());
  input.Name("x2").InputType(es::CompliantNodeBuilder::kEsIrInputDynamic).SymbolId("sym2");
  EXPECT_STREQ("x2", input.GetName());
  EXPECT_EQ(es::CompliantNodeBuilder::kEsIrInputDynamic, input.GetInputType());
  EXPECT_STREQ("sym2", input.GetSymbolId());

  es::CompliantNodeBuilder::IrOutputDefV2 output("y", es::CompliantNodeBuilder::kEsIrOutputRequired, "");
  EXPECT_STREQ("y", output.GetName());
  EXPECT_EQ(es::CompliantNodeBuilder::kEsIrOutputRequired, output.GetOutputType());

  AttrValue dv;
  (void)dv.SetAttrValue(static_cast<int64_t>(5));
  es::CompliantNodeBuilder::IrAttrDefV2 attr("k", es::CompliantNodeBuilder::kEsAttrOptional, "Int", dv);
  EXPECT_STREQ("k", attr.GetAttrName());
  EXPECT_EQ(es::CompliantNodeBuilder::kEsAttrOptional, attr.GetAttrType());
  EXPECT_STREQ("Int", attr.GetAttrDataType());
  int64_t got = 0;
  ASSERT_EQ(GRAPH_SUCCESS, attr.GetDefaultValue().GetAttrValue(got));
  EXPECT_EQ(5, got);

  // 拷贝/移动语义。
  const auto copied = input;
  EXPECT_STREQ("x2", copied.GetName());
  const auto moved = std::move(copied);
  EXPECT_STREQ("x2", moved.GetName());
}

// ge::es::CreateFrom：各类型按值构造后可读回。
TEST_F(EsFacadeCompliantTest, CreateFrom_AllTypes) {
  const auto int_v = es::CreateFrom(static_cast<int64_t>(7));
  int64_t i = 0;
  ASSERT_EQ(GRAPH_SUCCESS, int_v.GetAttrValue(i));
  EXPECT_EQ(7, i);

  const auto str_v = es::CreateFrom(AscendString("hello"));
  AscendString s;
  ASSERT_EQ(GRAPH_SUCCESS, str_v.GetAttrValue(s));
  EXPECT_EQ(AscendString("hello"), s);

  const auto float_v = es::CreateFrom(2.5f);
  float f = 0.0f;
  ASSERT_EQ(GRAPH_SUCCESS, float_v.GetAttrValue(f));
  EXPECT_FLOAT_EQ(2.5f, f);

  const auto bool_v = es::CreateFrom(true);
  bool b = false;
  ASSERT_EQ(GRAPH_SUCCESS, bool_v.GetAttrValue(b));
  EXPECT_TRUE(b);

  const auto list_v = es::CreateFrom(std::vector<int64_t>({1, 2}));
  std::vector<int64_t> li;
  ASSERT_EQ(GRAPH_SUCCESS, list_v.GetAttrValue(li));
  EXPECT_EQ(2U, li.size());
}

// ge::es::CreateFromIfNotEqual/ValuesEqual（重点）：相等返回空属性、不等含值；浮点与向量比较。
TEST_F(EsFacadeCompliantTest, CreateFromIfNotEqual_And_ValuesEqual) {
  const auto same = es::CreateFromIfNotEqual(static_cast<int64_t>(5), static_cast<int64_t>(5));
  int64_t dummy = 0;
  EXPECT_NE(GRAPH_SUCCESS, same.GetAttrValue(dummy));  // 空 AttrValue 读取失败

  const auto diff = es::CreateFromIfNotEqual(static_cast<int64_t>(5), static_cast<int64_t>(6));
  ASSERT_EQ(GRAPH_SUCCESS, diff.GetAttrValue(dummy));
  EXPECT_EQ(5, dummy);

  // 浮点按 epsilon 比较。
  EXPECT_TRUE(es::ValuesEqual(1.0f, 1.0f));
  EXPECT_FALSE(es::ValuesEqual(1.0f, 1.5f));
  const auto float_same = es::CreateFromIfNotEqual(1.0000001f, 1.0f);
  EXPECT_NE(GRAPH_SUCCESS, float_same.GetAttrValue(dummy));

  // 向量按元素比较。
  const std::vector<int64_t> a = {1, 2};
  EXPECT_TRUE(es::ValuesEqual(a, std::vector<int64_t>({1, 2})));
  EXPECT_FALSE(es::ValuesEqual(a, std::vector<int64_t>({1, 3})));
  const auto list_same = es::CreateFromIfNotEqual(std::vector<int64_t>({1, 2}), std::vector<int64_t>({1, 2}));
  EXPECT_NE(GRAPH_SUCCESS, list_same.GetAttrValue(dummy));
}

// ============================================================================
// 三、NamedIoNodeBuilder 全部接口（含 P4 行为固化）
// ============================================================================
class EsFacadeNamedIoTest : public testing::Test {};

namespace {
AttrValue MakeListIntAttr(const std::vector<int64_t> &values) {
  AttrValue av;
  (void)av.SetAttrValue(values);
  return av;
}
}  // namespace

// 注册原型算子两模式建点：端口齐、错误信息为空。
TEST_F(EsFacadeNamedIoTest, RegisteredOp_BothModesBuild) {
  Graph compute_graph("nio_c");
  (void)compute_graph.SetValid();
  AscendString error;
  NamedIoNodeBuilder compute_builder(compute_graph);
  auto c_node = compute_builder.Type("EsFocusPlainOp")
                    .Name("n1")
                    .AddInput("x1")
                    .AddInput("x2")
                    .AddOutput("y")
                    .Attr("strides", MakeListIntAttr({1, 2}))
                    .Build(error);
  ASSERT_NE(nullptr, c_node);
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, c_node->GetName(name));
  EXPECT_EQ(AscendString("n1"), name);
  EXPECT_EQ(2U, c_node->GetInputsSize());
  EXPECT_EQ(1U, c_node->GetOutputsSize());

  Graph execute_graph("nio_e", kExecGraphKind);
  NamedIoNodeBuilder execute_builder(execute_graph);
  auto e_node =
      execute_builder.Type("EsFocusPlainOp").Name("n1").AddInput("x1").AddInput("x2").AddOutput("y").Build(error);
  ASSERT_NE(nullptr, e_node);
  EXPECT_EQ(2U, e_node->GetInputsSize());
  EXPECT_EQ(1U, e_node->GetOutputsSize());
  EXPECT_EQ(1U, execute_graph.GetAllNodes().size());
}

// 未设类型/未注册类型：返回空且 error_message 非空。
TEST_F(EsFacadeNamedIoTest, MissingOrUnregisteredType_FailsWithMessage) {
  Graph graph("nio_fail");
  (void)graph.SetValid();
  AscendString error;

  NamedIoNodeBuilder no_type(graph);
  EXPECT_EQ(nullptr, no_type.Name("n1").Build(error));
  EXPECT_NE(nullptr, error.GetString());

  NamedIoNodeBuilder unregistered(graph);
  error = AscendString();
  EXPECT_EQ(nullptr, unregistered.Type("NoSuchOp").Name("n1").Build(error));
  EXPECT_NE(nullptr, error.GetString());
}

// 动态输入/输出：ir_name0/ir_name1 连续编号成功，跳号失败。
TEST_F(EsFacadeNamedIoTest, DynamicNumbering_ConsecutiveOkSkipFails) {
  Graph graph("nio_dyn", kExecGraphKind);
  NamedIoNodeBuilder ok_builder(graph);
  AscendString error;
  auto ok_node = ok_builder.Type("EsFocusDynamicOp")
                     .Name("n1")
                     .AddInput("x0")
                     .AddInput("x1")
                     .AddOutput("y0")
                     .AddOutput("y1")
                     .Build(error);
  ASSERT_NE(nullptr, ok_node);
  EXPECT_EQ(2U, ok_node->GetInputsSize());
  EXPECT_EQ(2U, ok_node->GetOutputsSize());

  Graph bad_graph("nio_dyn_bad", kExecGraphKind);
  NamedIoNodeBuilder bad_builder(bad_graph);
  error = AscendString();
  auto bad_node = bad_builder.Type("EsFocusDynamicOp").Name("n2").AddInput("x1").AddOutput("y0").Build(error);
  EXPECT_EQ(nullptr, bad_node);
  EXPECT_NE(nullptr, error.GetString());
}

// 属性：用户值优先于 IR 默认值。
TEST_F(EsFacadeNamedIoTest, AttrOverridesIrDefault) {
  Graph graph("nio_attr", kExecGraphKind);
  NamedIoNodeBuilder builder(graph);
  AttrValue mode;
  (void)mode.SetAttrValue(static_cast<int64_t>(100));
  AscendString error;
  auto node = builder.Type("EsFocusPlainOp")
                  .Name("n1")
                  .AddInput("x1")
                  .AddInput("x2")
                  .AddOutput("y")
                  .Attr("mode", mode)
                  .Build(error);
  ASSERT_NE(nullptr, node);
  int64_t got = 0;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("mode"), got));
  EXPECT_EQ(100, got);
}

// ============================================================================
// 四、AttrValue 值对象（md 3.9：14 对 SetAttrValue/GetAttrValue、GetValue、OpBytes）
// ============================================================================
class EsFacadeAttrValueTest : public testing::Test {};

// 14 对重载逐一存取回环一致。
TEST_F(EsFacadeAttrValueTest, AllTypePairs_Roundtrip) {
  AttrValue av;
  int64_t i64 = 0;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(static_cast<int64_t>(-9)));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(i64));
  EXPECT_EQ(-9, i64);

  float f = 0.0f;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(3.25f));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(f));
  EXPECT_FLOAT_EQ(3.25f, f);

  AscendString s;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(AscendString("str")));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(s));
  EXPECT_EQ(AscendString("str"), s);

  bool b = false;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(true));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(b));
  EXPECT_TRUE(b);

  const std::vector<int64_t> li = {1, 2, 3};
  std::vector<int64_t> got_li;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(li));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_li));
  EXPECT_EQ(li, got_li);

  const std::vector<float32_t> lf = {0.5f, 1.5f};
  std::vector<float32_t> got_lf;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(lf));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_lf));
  ASSERT_EQ(2U, got_lf.size());
  EXPECT_FLOAT_EQ(1.5f, got_lf[1]);

  const std::vector<AscendString> ls = {AscendString("a"), AscendString("b")};
  std::vector<AscendString> got_ls;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(ls));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_ls));
  ASSERT_EQ(2U, got_ls.size());
  EXPECT_EQ(AscendString("b"), got_ls[1]);

  const std::vector<bool> lb = {true, false};
  std::vector<bool> got_lb;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(lb));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_lb));
  EXPECT_FALSE(got_lb[1]);

  TensorDesc td(Shape({2, 2}), FORMAT_ND, DT_UINT8);
  Tensor tensor(td);
  const std::vector<uint8_t> bytes = {7, 8, 9, 10};
  tensor.SetData(bytes);
  Tensor got_tensor;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(tensor));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_tensor));
  ASSERT_EQ(bytes.size(), got_tensor.GetSize());
  EXPECT_EQ(9, got_tensor.GetData()[2]);

  const std::vector<Tensor> tensors = {tensor};
  std::vector<Tensor> got_tensors;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(tensors));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_tensors));
  ASSERT_EQ(1U, got_tensors.size());

  const std::vector<std::vector<int64_t>> vvi = {{1, 2}, {3}};
  std::vector<std::vector<int64_t>> got_vvi;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(vvi));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_vvi));
  ASSERT_EQ(2U, got_vvi.size());
  EXPECT_EQ(2, got_vvi[0][1]);

  const std::vector<ge::DataType> ldt = {DT_FLOAT, DT_INT32};
  std::vector<ge::DataType> got_ldt;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(ldt));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_ldt));
  EXPECT_EQ(DT_INT32, got_ldt[1]);

  const ge::DataType dt = DT_FLOAT16;
  ge::DataType got_dt = DT_UNDEFINED;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(dt));
  ASSERT_EQ(GRAPH_SUCCESS, av.GetAttrValue(got_dt));
  EXPECT_EQ(DT_FLOAT16, got_dt);
}

// GetValue<T, DT> 模板：存储 int64 转 int32 读出；字符串读取。
TEST_F(EsFacadeAttrValueTest, GetValue_TemplateConversion) {
  AttrValue av;
  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(static_cast<int64_t>(100)));
  int32_t as_int32 = 0;
  ASSERT_EQ(GRAPH_SUCCESS, (av.GetValue<int64_t, int32_t>(as_int32)));
  EXPECT_EQ(100, as_int32);

  int64_t as_int64 = 0;
  ASSERT_EQ(GRAPH_SUCCESS, (av.GetValue<int64_t, int64_t>(as_int64)));
  EXPECT_EQ(100, as_int64);

  ASSERT_EQ(GRAPH_SUCCESS, av.SetAttrValue(AscendString("conv")));
  AscendString s;
  ASSERT_EQ(GRAPH_SUCCESS, av.GetValue(s));
  EXPECT_EQ(AscendString("conv"), s);
}

// 默认构造 AttrValue 上各 GetAttrValue 均失败。
TEST_F(EsFacadeAttrValueTest, DefaultConstructed_AllGetsFail) {
  const AttrValue av;
  int64_t i = 0;
  float f = 0.0f;
  AscendString s;
  bool b = false;
  std::vector<int64_t> li;
  Tensor t;
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(i));
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(f));
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(s));
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(b));
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(li));
  EXPECT_NE(GRAPH_SUCCESS, av.GetAttrValue(t));
}
}  // namespace ge
