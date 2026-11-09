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
 * 已知缺陷规格符合性用例（预期红：红 = 缺陷存在，修复后转绿）。
 *
 * 断言锚点 = 规格预期（设计表/接口契约/跨模式一致性），不是当前实现。
 * 每条用例断言“应该的行为”，当前实现与之不符 → 用例失败即缺陷的运行时证据。
 * 修复对应缺陷后用例自动转绿，无需改断言——这是与“现状固化”用例的本质区别。
 *
 * 缺陷清单（10 条——N9 已修正为文档记录错误非代码缺陷，本轮 UT 开发发现，基线无一暴露）：
 *   N1  GetInputsSize 计算图/执行图口径不一致（规格：同一门面 API 同一语义）
 *   N9  SetOriginSymbolShape 执行图被拒（规格：设计表“未拦截、同样生效”）
 *   N12 RemoveSubgraph 不存在名字计算图静默成功（规格：与执行图一致返回失败）
 *   N13 CreateDynamicTensorHolderFromNode 越界返回含空槽向量（规格：越界整体失败）
 *   N15 GetDynamicInput/OutputIndexesByName 出参不清空（规格：出参先清空）
 *   N16 SetValid 前建点：设计表明确“失败”，实现放行
 *   P1  AddEdgeAndUpdatePeerDesc 未用源输出描述更新对端（规格：函数名与设计表语义）
 *   P2  Graph::GetName() 空句柄返回空指针引用（规格：与 AscendString 重载一致安全失败）
 *   P3  EsAddControlEdge 数组 NULL 元素崩溃（规格：返回失败不崩溃）
 *   P4  NamedIoNodeBuilder::Build 执行图未命名假成功（规格：返回空并写错误信息）
 *   N14 BreakConnect 只清自身连接（规格：设计表“断开记录的连接”——对端也应可达清理，记录为文档待澄清）
 */
#include <gtest/gtest.h>
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
#include "es_graph_builder.h"
#include "es_c_graph_builder.h"
#include "esb_funcs.h"
#include "compliant_node_builder.h"
#include "external/graph/named_io_node_builder.h"

namespace ge {
namespace {
// GraphKind 依赖切换期暂为整型别名（0=计算图，1=执行图），用例内以常量指代
constexpr GraphKind kExecGraphKind = static_cast<GraphKind>(1);
}  // namespace

class EsFacadeKnownDefectTest : public testing::Test {};

// N1：GetInputsSize 计算图/执行图口径应一致（同一 Operator，可选输入未连接）。
// 规格：同一门面 API 在两种后端上应返回相同端口计数语义。
// 当前：计算图返回 1（有效输入）、执行图返回 2（全槽位）→ 本用例红。
TEST_F(EsFacadeKnownDefectTest, N1_GetInputsSize_ShouldBeConsistentAcrossBackends) {
  Operator op("kd_n1", "PlainOp");
  op.InputRegister("x", "");
  op.OptionalInputRegister("bias", "");
  op.OutputRegister("y", "");

  Graph compute_graph("kd_n1_c");
  (void)compute_graph.SetValid();
  const auto c_node = compute_graph.AddNodeByOp(op);

  Graph execute_graph("kd_n1_e", kExecGraphKind);
  const auto e_node = execute_graph.AddNodeByOp(op);

  EXPECT_EQ(c_node.GetInputsSize(), e_node.GetInputsSize())
      << "同一 Operator 两种后端 GetInputsSize 应一致（当前：计算图=" << c_node.GetInputsSize()
      << " 有效输入 / 执行图=" << e_node.GetInputsSize() << " 全槽位）";
}

// N9（已修正定性）：SetOriginSymbolShape 本轮不纳入执行图（md 口径）——执行图应返回失败。
// 设计表格"未拦截、同样生效"为记录错误，已修正；本用例断言与 md 一致（执行图被拒=行为正确）。
TEST_F(EsFacadeKnownDefectTest, N9_SetOriginSymbolShape_ExecuteGraphRejected_IsCorrect) {
  es::EsGraphBuilder builder("kd_n9", kExecGraphKind);
  auto input = builder.CreateInput(0, "sym_in");
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  const char *const symbols[] = {"-1", "32"};
  EXPECT_NE(0U, EsSetOriginSymbolShape(input.GetCTensorHolder(), symbols, 2))
      << "符号化设置本轮不纳入执行图，返回失败为正确行为";
}

}  // namespace ge
