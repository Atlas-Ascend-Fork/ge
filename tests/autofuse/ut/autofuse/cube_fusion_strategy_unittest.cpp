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

#include "all_ops_cpp.h"
#include "ascir_ops.h"
#include "can_fuse/backend/backend_utils.h"
#include "can_fuse/strategy/cube_fusion_strategy.h"
#include "common/autofuse_platform_api.h"
#include "depends/runtime/src/runtime_stub.h"
#include "fusion/autofuse_attrs.h"
#include "graph/utils/graph_utils.h"
#include "utils/auto_fuse_config.h"

namespace ge {
using namespace autofuse;
namespace ascir_op = af::ascir_op;
namespace {
enum class PointwiseGraphType {
  kCompute,
  kDirectExplicitNBroadcast,
  kDirectExplicitBatchBroadcast,
  kDirectInnerNBroadcast,
  kDirectInnerMBroadcast,
  kDirectInnerBatchBroadcast,
  kDirectInnerMNBroadcast,
  kIndirectBatchBroadcast,
  kIndirectInnerBroadcast,
  kViewOnly,
};

struct AscAxes {
  std::vector<int64_t> ids;
  std::vector<Expression> repeats;
  std::vector<Expression> strides;
  int64_t loop_axis;
};

struct CubePair {
  ComputeGraphPtr graph;
  NodePtr cube;
  NodePtr pointwise;
};

AscAxes CreateAscAxes(AscGraph &graph) {
  const auto a_size = Symbol(2);
  const auto b_size = Symbol(3);
  const auto c_size = Symbol(4);
  const auto a = graph.CreateAxis("A", a_size).id;
  const auto b = graph.CreateAxis("B", b_size).id;
  const auto c = graph.CreateAxis("C", c_size).id;
  return {{a, b, c}, {a_size, b_size, c_size}, {b_size * c_size, c_size, Symbol(1)}, b};
}

template <typename Op>
void SetAscOpAttrs(Op &op, const AscAxes &axes, const std::vector<Expression> &repeats,
                   const std::vector<Expression> &strides) {
  op.attr.sched.axis = axes.ids;
  op.attr.sched.loop_axis = axes.loop_axis;
  *op.y.axis = axes.ids;
  *op.y.repeats = repeats;
  *op.y.strides = strides;
}

void SetAscGraphOutput(AscGraph &graph, const std::string &output_name) {
  auto output = graph.FindNode(output_name.c_str());
  ASSERT_NE(output, nullptr);
  auto compute_graph = output->GetOwnerComputeGraph();
  ASSERT_NE(compute_graph, nullptr);
  compute_graph->SetOutputSize(1U);
  compute_graph->SetGraphOutNodesInfo({{output, 0}});
}

std::shared_ptr<AscGraph> CreateComputeAscGraph(const std::string &name) {
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  ascir_op::Data data((name + "_data").c_str(), graph);
  SetAscOpAttrs(data, axes, axes.repeats, axes.strides);
  ascir_op::Load load((name + "_load").c_str());
  load.x = data.y;
  SetAscOpAttrs(load, axes, axes.repeats, axes.strides);
  ascir_op::Abs abs((name + "_abs").c_str());
  abs.x = load.y;
  SetAscOpAttrs(abs, axes, axes.repeats, axes.strides);
  ascir_op::Store store((name + "_store").c_str());
  store.x = abs.y;
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output((name + "_output").c_str());
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, name + "_output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreateDirectExplicitBroadcastAscGraph(const std::string &name, bool batch_axis, bool m_axis,
                                                                bool n_axis) {
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  // 模拟GeFallback后的真实形态：mm输出对应轴为1（Load属性压缩编码repeat=1/stride=0），
  // 且Load后挂显式Broadcast节点
  const auto broadcast_repeats =
      std::vector<Expression>{batch_axis ? Symbol(1) : axes.repeats[0], m_axis ? Symbol(1) : axes.repeats[1],
                              n_axis ? Symbol(1) : axes.repeats[2]};
  const auto broadcast_strides =
      std::vector<Expression>{batch_axis ? Symbol(0) : axes.strides[0], m_axis ? Symbol(0) : axes.strides[1],
                              n_axis ? Symbol(0) : axes.strides[2]};
  ascir_op::Data data((name + "_data").c_str(), graph);
  SetAscOpAttrs(data, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Load load((name + "_load").c_str());
  load.x = data.y;
  SetAscOpAttrs(load, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Broadcast broadcast((name + "_broadcast").c_str());
  broadcast.x = load.y;
  SetAscOpAttrs(broadcast, axes, axes.repeats, axes.strides);
  ascir_op::Abs abs((name + "_abs").c_str());
  abs.x = broadcast.y;
  SetAscOpAttrs(abs, axes, axes.repeats, axes.strides);
  ascir_op::Store store((name + "_store").c_str());
  store.x = abs.y;
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output((name + "_output").c_str());
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, name + "_output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreateSharedBroadcastInputGraph(size_t first_axis, size_t second_axis,
                                                          bool indirect = false) {
  const std::string name = "shared_broadcast";
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  ascir_op::Load direct_load("direct_load");
  if (indirect) {
    ascir_op::Data direct("direct_data", graph);
    SetAscOpAttrs(direct, axes, axes.repeats, axes.strides);
    direct_load.x = direct.y;
    SetAscOpAttrs(direct_load, axes, axes.repeats, axes.strides);
  }
  ascir_op::Data data("shared_data", graph);
  SetAscOpAttrs(data, axes, axes.repeats, axes.strides);
  ascir_op::Load first("first_load");
  ascir_op::Load second("second_load");
  auto connect_load = [&](ascir_op::Load &load, size_t axis) {
    load.x = data.y;
    auto repeats = axes.repeats;
    auto strides = axes.strides;
    // An axis outside the graph rank represents a plain Load.
    if (axis < axes.ids.size()) {
      repeats[axis] = Symbol(1);
      strides[axis] = Symbol(0);
    }
    SetAscOpAttrs(load, axes, repeats, strides);
  };
  connect_load(first, first_axis);
  connect_load(second, second_axis);
  ascir_op::Add add("add");
  add.x1 = first.y;
  add.x2 = second.y;
  SetAscOpAttrs(add, axes, axes.repeats, axes.strides);
  ascir_op::Store store("store");
  if (indirect) {
    ascir_op::Add direct_add("direct_add");
    direct_add.x1 = direct_load.y;
    direct_add.x2 = add.y;
    SetAscOpAttrs(direct_add, axes, axes.repeats, axes.strides);
    store.x = direct_add.y;
  } else {
    store.x = add.y;
  }
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output("output");
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, "output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreateViewOnlyAscGraph(const std::string &name) {
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  ascir_op::Data data((name + "_data").c_str(), graph);
  SetAscOpAttrs(data, axes, axes.repeats, axes.strides);
  ascir_op::Load load((name + "_load").c_str());
  load.x = data.y;
  SetAscOpAttrs(load, axes, axes.repeats, axes.strides);
  ascir_op::Store store((name + "_store").c_str());
  store.x = load.y;
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output((name + "_output").c_str());
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, name + "_output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreateBatchBroadcastAscGraph(const std::string &name, bool batch_axis) {
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  ascir_op::Data direct((name + "_direct_data").c_str(), graph);
  SetAscOpAttrs(direct, axes, axes.repeats, axes.strides);
  ascir_op::Load direct_load((name + "_direct_load").c_str());
  direct_load.x = direct.y;
  SetAscOpAttrs(direct_load, axes, axes.repeats, axes.strides);
  const auto broadcast_repeats = batch_axis ? std::vector<Expression>{Symbol(1), axes.repeats[1], axes.repeats[2]}
                                            : std::vector<Expression>{axes.repeats[0], axes.repeats[1], Symbol(1)};
  const auto broadcast_strides = batch_axis ? std::vector<Expression>{Symbol(0), axes.strides[1], axes.strides[2]}
                                            : std::vector<Expression>{axes.strides[0], axes.strides[1], Symbol(0)};
  ascir_op::Data indirect((name + "_indirect_data").c_str(), graph);
  SetAscOpAttrs(indirect, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Load indirect_load((name + "_indirect_load").c_str());
  indirect_load.x = indirect.y;
  SetAscOpAttrs(indirect_load, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Add add((name + "_add").c_str());
  add.x1 = direct_load.y;
  add.x2 = indirect_load.y;
  SetAscOpAttrs(add, axes, axes.repeats, axes.strides);
  ascir_op::Store store((name + "_store").c_str());
  store.x = add.y;
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output((name + "_output").c_str());
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, name + "_output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreateDirectImplicitBroadcastAscGraph(const std::string &name, bool batch_axis, bool m_axis,
                                                                bool n_axis) {
  AscGraph graph(name.c_str());
  const auto axes = CreateAscAxes(graph);
  // 模拟mm输出对应轴为1（repeat=1/stride=0），vector消费侧按满秩图轴广播
  const auto broadcast_repeats =
      std::vector<Expression>{batch_axis ? Symbol(1) : axes.repeats[0], m_axis ? Symbol(1) : axes.repeats[1],
                              n_axis ? Symbol(1) : axes.repeats[2]};
  const auto broadcast_strides =
      std::vector<Expression>{batch_axis ? Symbol(0) : axes.strides[0], m_axis ? Symbol(0) : axes.strides[1],
                              n_axis ? Symbol(0) : axes.strides[2]};
  ascir_op::Data data((name + "_data").c_str(), graph);
  SetAscOpAttrs(data, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Load load((name + "_load").c_str());
  load.x = data.y;
  SetAscOpAttrs(load, axes, broadcast_repeats, broadcast_strides);
  ascir_op::Abs abs((name + "_abs").c_str());
  abs.x = load.y;
  SetAscOpAttrs(abs, axes, axes.repeats, axes.strides);
  ascir_op::Store store((name + "_store").c_str());
  store.x = abs.y;
  SetAscOpAttrs(store, axes, axes.repeats, axes.strides);
  ascir_op::Output output((name + "_output").c_str());
  output.x = store.y;
  SetAscOpAttrs(output, axes, axes.repeats, axes.strides);
  SetAscGraphOutput(graph, name + "_output");
  return std::make_shared<AscGraph>(graph);
}

std::shared_ptr<AscGraph> CreatePointwiseAscGraph(PointwiseGraphType type, const std::string &name) {
  if (type == PointwiseGraphType::kDirectExplicitNBroadcast) {
    return CreateDirectExplicitBroadcastAscGraph(name, false, false, true);
  }
  if (type == PointwiseGraphType::kDirectExplicitBatchBroadcast) {
    return CreateDirectExplicitBroadcastAscGraph(name, true, false, false);
  }
  if (type == PointwiseGraphType::kDirectInnerNBroadcast) {
    return CreateDirectImplicitBroadcastAscGraph(name, false, false, true);
  }
  if (type == PointwiseGraphType::kDirectInnerMBroadcast) {
    return CreateDirectImplicitBroadcastAscGraph(name, false, true, false);
  }
  if (type == PointwiseGraphType::kDirectInnerBatchBroadcast) {
    return CreateDirectImplicitBroadcastAscGraph(name, true, false, false);
  }
  if (type == PointwiseGraphType::kDirectInnerMNBroadcast) {
    return CreateDirectImplicitBroadcastAscGraph(name, false, true, true);
  }
  if (type == PointwiseGraphType::kIndirectBatchBroadcast) {
    return CreateBatchBroadcastAscGraph(name, true);
  }
  if (type == PointwiseGraphType::kIndirectInnerBroadcast) {
    return CreateBatchBroadcastAscGraph(name, false);
  }
  if (type == PointwiseGraphType::kViewOnly) {
    return CreateViewOnlyAscGraph(name);
  }
  return CreateComputeAscGraph(name);
}

NodePtr AddOuterNode(const ComputeGraphPtr &graph, const std::string &name, const std::string &type,
                     int32_t input_count, int32_t output_count) {
  GeTensorDesc tensor_desc(GeShape({2, 3, 4}), FORMAT_ND, DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (int32_t i = 0; i < input_count; ++i) {
    op_desc->AddInputDesc(tensor_desc);
  }
  for (int32_t i = 0; i < output_count; ++i) {
    op_desc->AddOutputDesc(tensor_desc);
  }
  return graph->AddNode(op_desc);
}

CubePair CreateCubePair(PointwiseGraphType pointwise_type = PointwiseGraphType::kCompute, bool vertical = true) {
  auto graph = std::make_shared<ComputeGraph>("cube_pair");
  const int32_t pointwise_inputs = (pointwise_type == PointwiseGraphType::kIndirectBatchBroadcast ||
                                    pointwise_type == PointwiseGraphType::kIndirectInnerBroadcast)
                                       ? 2
                                       : 1;
  auto cube_input = AddOuterNode(graph, "cube_input", kDataType, 0, 1);
  auto pointwise_input = AddOuterNode(graph, "pointwise_input", kDataType, 0, 1);
  auto cube = AddOuterNode(graph, "cube", kAscBackendType, 1, 1);
  auto pointwise = AddOuterNode(graph, "pointwise", kAscBackendType, pointwise_inputs, 1);
  auto cube_attr = GetOrCreateAutoFuseAttrs(cube->GetOpDesc());
  cube_attr->SetAscGraph(CreateComputeAscGraph("cube_graph"), loop::FuseType::kCube);
  auto pointwise_attr = GetOrCreateAutoFuseAttrs(pointwise->GetOpDesc());
  pointwise_attr->SetAscGraph(CreatePointwiseAscGraph(pointwise_type, "pointwise_graph"), loop::FuseType::kPointwise);
  EXPECT_EQ(GraphUtils::AddEdge(cube_input->GetOutDataAnchor(0), cube->GetInDataAnchor(0)), GRAPH_SUCCESS);
  auto pointwise_source = vertical ? cube : pointwise_input;
  EXPECT_EQ(GraphUtils::AddEdge(pointwise_source->GetOutDataAnchor(0), pointwise->GetInDataAnchor(0)), GRAPH_SUCCESS);
  if (pointwise_inputs == 2) {
    EXPECT_EQ(GraphUtils::AddEdge(pointwise_input->GetOutDataAnchor(0), pointwise->GetInDataAnchor(1)), GRAPH_SUCCESS);
  }
  return {graph, cube, pointwise};
}

void SetFuseType(const NodePtr &node, loop::FuseType fuse_type) {
  auto attr = GetOrCreateAutoFuseAttrs(node->GetOpDesc());
  ASSERT_NE(attr, nullptr);
  attr->SetFuseType(fuse_type);
}

class CubeFusionStrategyUT : public testing::Test {
 protected:
  void SetUp() override {
    const auto lower_matmul = std::getenv("ENABLE_LOWER_MATMUL");
    had_lower_matmul_env_ = lower_matmul != nullptr;
    original_lower_matmul_env_ = had_lower_matmul_env_ ? lower_matmul : "";
    setenv("ENABLE_LOWER_MATMUL", "true", 1);
    original_max_fusion_size_ = AutoFuseConfig::Config().GetFusionStrategySolver().max_fusion_size;
    AutoFuseConfig::MutableConfig().GetMutableFusionStrategySolver().max_fusion_size = 64U;
    RuntimeStub::SetInstance(std::make_shared<RuntimeStubV2Common>());
    ResetAutofusePlatform();
  }

  void TearDown() override {
    AutoFuseConfig::MutableConfig().GetMutableFusionStrategySolver().max_fusion_size = original_max_fusion_size_;
    if (had_lower_matmul_env_) {
      setenv("ENABLE_LOWER_MATMUL", original_lower_matmul_env_.c_str(), 1);
    } else {
      unsetenv("ENABLE_LOWER_MATMUL");
    }
    RuntimeStub::Reset();
    ResetAutofusePlatform();
  }

  uint64_t original_max_fusion_size_ = 0U;
  bool had_lower_matmul_env_ = false;
  std::string original_lower_matmul_env_;
};

TEST_F(CubeFusionStrategyUT, GetMaxFusionNodesSizeAddsCubeNode) {
  auto pair = CreateCubePair();
  EXPECT_EQ(CubeFusionStrategy().GetMaxFusionNodesSize(pair.cube, pair.pointwise), 65U);
}

TEST_F(CubeFusionStrategyUT, GetFusionPairPriorityReturnsLowForVerticalCube) {
  auto pair = CreateCubePair();
  EXPECT_EQ(CubeFusionStrategy().GetFusionPairPriority(pair.cube, pair.pointwise), FusionPriority::LOW);
}

TEST_F(CubeFusionStrategyUT, GetFusionPairPriorityReturnsDefaultForHorizontalCube) {
  auto pair = CreateCubePair(PointwiseGraphType::kCompute, false);
  EXPECT_EQ(CubeFusionStrategy().GetFusionPairPriority(pair.cube, pair.pointwise), FusionPriority::DEFAULT);
}

TEST_F(CubeFusionStrategyUT, CanFuseVerticalCubeAndPointwise) {
  auto pair = CreateCubePair();
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsCubeForwardFusion) {
  auto pair = CreateCubePair();
  SetFuseType(pair.pointwise, loop::FuseType::kCube);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsNonPointwiseFusion) {
  auto pair = CreateCubePair();
  SetFuseType(pair.pointwise, loop::FuseType::kReduction);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsHorizontalPointwiseFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kCompute, false);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, AllowsDirectExplicitNBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectExplicitNBroadcast);
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsDirectExplicitBatchBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectExplicitBatchBroadcast);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsBatchBroadcastOnSharedCubeInputInEitherOrder) {
  for (const bool batch_first : {true, false}) {
    SCOPED_TRACE(batch_first);
    auto pair = CreateCubePair();
    auto attr = GetOrCreateAutoFuseAttrs(pair.pointwise->GetOpDesc());
    ASSERT_NE(attr, nullptr);
    attr->SetAscGraph(CreateSharedBroadcastInputGraph(batch_first ? 0U : 2U, batch_first ? 2U : 0U),
                      loop::FuseType::kPointwise);
    std::vector<ViewOpAttrInfo> views;
    ASSERT_EQ(BackendUtils::CollectInputLoadViewInfos(pair.pointwise, 0, views), SUCCESS);
    ASSERT_EQ(views.size(), 2U);
    EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
  }
}

TEST_F(CubeFusionStrategyUT, CollectsExplicitBroadcastAcrossFusedBackend) {
  const auto outer = std::make_shared<ComputeGraph>("outer");
  const auto subgraph = std::make_shared<ComputeGraph>("fused");
  const auto fused = AddOuterNode(outer, "fused", kFusedAscBackendType, 1, 1);
  const auto data = AddOuterNode(subgraph, "data", kDataType, 0, 1);
  const auto inner = AddOuterNode(subgraph, "inner", kAscBackendType, 1, 1);
  ASSERT_NE(subgraph->AddInputNode(data), nullptr);
  ASSERT_EQ(GraphUtils::AddEdge(data->GetOutDataAnchor(0), inner->GetInDataAnchor(0)), GRAPH_SUCCESS);
  const auto inner_attr = GetOrCreateAutoFuseAttrs(inner->GetOpDesc());
  ASSERT_NE(inner_attr, nullptr);
  inner_attr->SetAscGraph(CreateDirectExplicitBroadcastAscGraph("batch_broadcast", true, false, false),
                          loop::FuseType::kPointwise);
  const auto fused_attr = GetOrCreateAutoFuseAttrs(fused->GetOpDesc());
  ASSERT_NE(fused_attr, nullptr);
  fused_attr->SetFuseComputeGraph(subgraph);
  std::vector<ViewOpAttrInfo> views;
  ASSERT_EQ(BackendUtils::CollectInputLoadViewInfos(fused, 0, views), SUCCESS);
  ASSERT_EQ(views.size(), 1U);
  ASSERT_EQ(views.front().broadcast_info.size(), 1U);
  EXPECT_EQ(views.front().broadcast_info.front(), 0);
}

TEST_F(CubeFusionStrategyUT, AllowsDirectInnerNBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectInnerNBroadcast);
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, AllowsDirectInnerMBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectInnerMBroadcast);
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsDirectInnerBatchBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectInnerBatchBroadcast);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, AllowsDirectInnerMNBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kDirectInnerMNBroadcast);
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsIndirectBatchBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kIndirectBatchBroadcast);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, AllowsIndirectInnerBroadcastFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kIndirectInnerBroadcast);
  EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsBatchBroadcastOnSharedIndirectInputInEitherOrder) {
  for (const bool batch_first : {true, false}) {
    SCOPED_TRACE(batch_first);
    auto pair = CreateCubePair(PointwiseGraphType::kIndirectBatchBroadcast);
    auto attr = GetOrCreateAutoFuseAttrs(pair.pointwise->GetOpDesc());
    ASSERT_NE(attr, nullptr);
    attr->SetAscGraph(CreateSharedBroadcastInputGraph(batch_first ? 0U : 2U, batch_first ? 2U : 0U, true),
                      loop::FuseType::kPointwise);
    EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
  }
}

TEST_F(CubeFusionStrategyUT, AllowsSharedIndirectPlainLoadsAndInnerBroadcasts) {
  // The graph axes are batch, M, N; 3 denotes a plain Load.
  for (const auto &axes : std::vector<std::pair<size_t, size_t>>{{3U, 3U}, {1U, 2U}, {2U, 1U}, {3U, 2U}, {2U, 3U}}) {
    SCOPED_TRACE(::testing::Message() << axes.first << "," << axes.second);
    auto pair = CreateCubePair(PointwiseGraphType::kIndirectInnerBroadcast);
    auto attr = GetOrCreateAutoFuseAttrs(pair.pointwise->GetOpDesc());
    ASSERT_NE(attr, nullptr);
    attr->SetAscGraph(CreateSharedBroadcastInputGraph(axes.first, axes.second, true), loop::FuseType::kPointwise);
    EXPECT_TRUE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
  }
}

TEST_F(CubeFusionStrategyUT, RejectsIndirectInputWhenLoadViewQueryFails) {
  auto pair = CreateCubePair(PointwiseGraphType::kIndirectInnerBroadcast);
  auto asc_graph = CreateSharedBroadcastInputGraph(1U, 2U, true);
  auto data = asc_graph->FindNode("shared_data");
  ASSERT_NE(data, nullptr);
  auto output = data->GetOutDataAnchor(0);
  ASSERT_NE(output, nullptr);
  for (const auto &name : {"first_load", "second_load"}) {
    auto load = asc_graph->FindNode(name);
    ASSERT_NE(load, nullptr);
    ASSERT_EQ(GraphUtils::RemoveEdge(output, load->GetInDataAnchor(0)), GRAPH_SUCCESS);
  }
  auto attr = GetOrCreateAutoFuseAttrs(pair.pointwise->GetOpDesc());
  ASSERT_NE(attr, nullptr);
  attr->SetAscGraph(asc_graph, loop::FuseType::kPointwise);
  std::vector<ViewOpAttrInfo> views;
  ASSERT_NE(BackendUtils::CollectInputLoadViewInfos(pair.pointwise, 1, views), SUCCESS);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}

TEST_F(CubeFusionStrategyUT, RejectsViewOnlyPointwiseFusion) {
  auto pair = CreateCubePair(PointwiseGraphType::kViewOnly);
  EXPECT_FALSE(CubeFusionStrategy().CanFuse(pair.cube, pair.pointwise));
}
}  // namespace
}  // namespace ge
