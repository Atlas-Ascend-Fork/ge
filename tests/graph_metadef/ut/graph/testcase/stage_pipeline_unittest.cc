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

#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "stage/stage_graph_utils.h"
#include "stage/stage_splitter.h"

namespace ge {
namespace {
FastNode *AddStageTestNode(ExecuteGraph &graph, const char *name, const char *type, uint32_t inputs = 0U,
                           uint32_t outputs = 1U) {
  auto desc = std::make_shared<OpDesc>(name, type);
  for (uint32_t i = 0U; i < inputs; ++i) {
    if (desc->AddInputDesc(GeTensorDesc()) != GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  for (uint32_t i = 0U; i < outputs; ++i) {
    if (desc->AddOutputDesc(GeTensorDesc()) != GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  return graph.AddNode(desc);
}
}  // namespace

TEST(StagePipelineTest, SelectsBuiltinAndExplicitStages) {
  ExecuteGraph graph("stages");
  auto *init = AddStageTestNode(graph, "init", "FindKernelFunc");
  auto *main = AddStageTestNode(graph, "main", "PlainKernel");
  auto *deinit = AddStageTestNode(graph, "deinit", "DavinciModelFinalizer", 0U, 0U);
  auto *explicit_init = AddStageTestNode(graph, "explicit", "PlainKernel");
  ASSERT_TRUE(AttrUtils::SetStr(explicit_init->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  EXPECT_EQ(plan.GetStage(*init), StageKind::kInit);
  EXPECT_EQ(plan.GetStage(*main), StageKind::kMain);
  EXPECT_EQ(plan.GetStage(*deinit), StageKind::kDeInit);
  EXPECT_EQ(plan.GetStage(*explicit_init), StageKind::kInit);
}

TEST(StagePipelineTest, RejectsInvalidStageAndExecutionDependencyOfInit) {
  ExecuteGraph graph("invalid_stage");
  auto *input = AddStageTestNode(graph, "input", "PlainKernel");
  auto *load = AddStageTestNode(graph, "load", "PlainKernel", 1U);
  ASSERT_NE(graph.AddEdge(input, 0, load, 0), nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(input->GetOpDescPtr(), gert::attr::kStage, "invalid"));
  StagePlan plan;
  EXPECT_NE(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetStr(input->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageMain));
  ASSERT_TRUE(AttrUtils::SetStr(load->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  EXPECT_NE(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
}

TEST(StagePipelineTest, BridgesInitValueAndPreservesOutputOrder) {
  ExecuteGraph graph("bridges");
  auto *init = AddStageTestNode(graph, "init", "PlainKernel");
  auto *main = AddStageTestNode(graph, "main", "PlainKernel", 1U);
  ASSERT_TRUE(AttrUtils::SetStr(init->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  ASSERT_NE(graph.AddEdge(init, 0, main, 0), nullptr);
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  StageSplitter splitter(graph);
  ASSERT_EQ(splitter.Split(plan, {{main, 0}, {init, 0}}), GRAPH_SUCCESS);
  EXPECT_EQ(graph.GetDirectNodesSize(), 3U);
  EXPECT_GE(splitter.GetExportPort(StageKind::kInit, *init, 0), 0);
  const auto &outputs = splitter.GetOutputBindings();
  ASSERT_EQ(outputs.size(), 2U);
  EXPECT_EQ(outputs[0].node, main);
  EXPECT_STREQ(outputs[1].node->GetTypePtr(), "InnerData");
  EXPECT_EQ(outputs[0].user_index, 0);
  EXPECT_EQ(outputs[1].user_index, 1);
  EXPECT_NE(splitter.Split(plan, {{main, 0}}), GRAPH_SUCCESS);
}

TEST(StagePipelineTest, KeepsLoadAndExecuteInputIndexesIndependent) {
  ExecuteGraph graph("feeds");
  auto *load = AddStageTestNode(graph, "load", "ConstData");
  auto *input = AddStageTestNode(graph, "input", "Data");
  ASSERT_TRUE(AttrUtils::SetStr(load->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  ASSERT_TRUE(AttrUtils::SetInt(load->GetOpDescPtr(), "index", 0));
  ASSERT_TRUE(AttrUtils::SetInt(input->GetOpDescPtr(), "index", 0));
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  StageSplitter splitter(graph);
  ASSERT_EQ(splitter.Split(plan, {{input, 0}}), GRAPH_SUCCESS);
  EXPECT_EQ(splitter.GetInputBindings(StageKind::kInit, StageInputClass::kLoad).size(), 1U);
  EXPECT_EQ(splitter.GetInputBindings(StageKind::kMain, StageInputClass::kExecute).size(), 1U);
}

TEST(StagePipelineTest, RejectsInvalidOutputBeforeMutation) {
  ExecuteGraph graph("invalid_output");
  auto *node = AddStageTestNode(graph, "node", "PlainKernel");
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  StageSplitter splitter(graph);
  EXPECT_NE(splitter.Split(plan, {{node, 1}}), GRAPH_SUCCESS);
  EXPECT_EQ(graph.GetDirectNodesSize(), 1U);
  EXPECT_EQ(node->GetExtendInfo()->GetOwnerGraphBarePtr(), &graph);
}

TEST(StagePipelineTest, ClosesInitDataPredecessorsTransitively) {
  ExecuteGraph graph("init_closure");
  auto *constant = AddStageTestNode(graph, "constant", "Const");
  auto *prepare = AddStageTestNode(graph, "prepare", "PlainKernel", 1U);
  auto *init = AddStageTestNode(graph, "init", "FindKernelFunc", 1U);
  auto *main = AddStageTestNode(graph, "main", "PlainKernel");
  ASSERT_NE(graph.AddEdge(constant, 0, prepare, 0), nullptr);
  ASSERT_NE(graph.AddEdge(prepare, 0, init, 0), nullptr);

  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  EXPECT_EQ(plan.GetStage(*constant), StageKind::kInit);
  EXPECT_EQ(plan.GetStage(*prepare), StageKind::kInit);
  EXPECT_EQ(plan.GetStage(*main), StageKind::kMain);
  ASSERT_NE(plan.FindInfo(*constant), nullptr);
  ASSERT_NE(plan.FindInfo(*prepare), nullptr);
  EXPECT_EQ(plan.FindInfo(*constant)->source, StageSource::kClosure);
  EXPECT_EQ(plan.FindInfo(*prepare)->source, StageSource::kClosure);

  StageSplitter splitter(graph);
  ASSERT_EQ(splitter.Split(plan, {{main, 0}}), GRAPH_SUCCESS);
  auto *init_graph = splitter.GetStageGraph(StageKind::kInit);
  ASSERT_NE(init_graph, nullptr);
  EXPECT_EQ(constant->GetExtendInfo()->GetOwnerGraphBarePtr(), init_graph);
  EXPECT_EQ(prepare->GetExtendInfo()->GetOwnerGraphBarePtr(), init_graph);
  EXPECT_EQ(init->GetExtendInfo()->GetOwnerGraphBarePtr(), init_graph);
  EXPECT_EQ(main->GetExtendInfo()->GetOwnerGraphBarePtr(), splitter.GetStageGraph(StageKind::kMain));
  ASSERT_NE(prepare->GetInDataEdgeByIndex(0), nullptr);
  EXPECT_EQ(prepare->GetInDataEdgeByIndex(0)->src, constant);
  ASSERT_NE(init->GetInDataEdgeByIndex(0), nullptr);
  EXPECT_EQ(init->GetInDataEdgeByIndex(0)->src, prepare);
  EXPECT_EQ(splitter.GetExportPort(StageKind::kInit, *prepare, 0), -1);
}

TEST(StagePipelineTest, PairsSeparateProducerOutputsWithReleaseStage) {
  ExecuteGraph graph("release_pairing");
  auto *resource = AddStageTestNode(graph, "resource", "PlainKernel", 0U, 2U);
  auto *release_first = AddStageTestNode(graph, "release_first", "FreeMemory", 1U, 0U);
  auto *release_second = AddStageTestNode(graph, "release_second", "FreeMemHbm", 1U, 0U);
  ASSERT_TRUE(AttrUtils::SetStr(resource->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  ASSERT_NE(graph.AddEdge(resource, 0, release_first, 0), nullptr);
  ASSERT_NE(graph.AddEdge(resource, 1, release_second, 0), nullptr);

  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  EXPECT_EQ(plan.GetStage(*release_first), StageKind::kInit);
  EXPECT_EQ(plan.GetStage(*release_second), StageKind::kInit);
  ASSERT_NE(plan.FindInfo(*release_first), nullptr);
  ASSERT_NE(plan.FindInfo(*release_second), nullptr);
  EXPECT_EQ(plan.FindInfo(*release_first)->source, StageSource::kRelease);
  EXPECT_EQ(plan.FindInfo(*release_second)->source, StageSource::kRelease);
  EXPECT_EQ(plan.FindInfo(*release_first)->reason, "resource");
  EXPECT_EQ(plan.FindInfo(*release_second)->reason, "resource");
  StageSplitter splitter(graph);
  ASSERT_EQ(splitter.Split(plan, {}), GRAPH_SUCCESS);
  EXPECT_EQ(release_first->GetExtendInfo()->GetOwnerGraphBarePtr(), splitter.GetStageGraph(StageKind::kInit));
  EXPECT_EQ(release_second->GetExtendInfo()->GetOwnerGraphBarePtr(), splitter.GetStageGraph(StageKind::kInit));
  ASSERT_NE(release_first->GetInDataEdgeByIndex(0), nullptr);
  ASSERT_NE(release_second->GetInDataEdgeByIndex(0), nullptr);
  EXPECT_EQ(release_first->GetInDataEdgeByIndex(0)->src, resource);
  EXPECT_EQ(release_second->GetInDataEdgeByIndex(0)->src, resource);
  EXPECT_EQ(release_first->GetInDataEdgeByIndex(0)->src_output, 0);
  EXPECT_EQ(release_second->GetInDataEdgeByIndex(0)->src_output, 1);
}

TEST(StagePipelineTest, RejectsDuplicateReleaseOfSameProducerOutput) {
  ExecuteGraph graph("duplicate_release");
  auto *resource = AddStageTestNode(graph, "resource", "PlainKernel");
  auto *release_first = AddStageTestNode(graph, "release_first", "FreeMemory", 1U, 0U);
  auto *release_second = AddStageTestNode(graph, "release_second", "FreeMemHbm", 1U, 0U);
  auto *first_edge = graph.AddEdge(resource, 0, release_first, 0);
  auto *second_edge = graph.AddEdge(resource, 0, release_second, 0);
  ASSERT_NE(first_edge, nullptr);
  ASSERT_NE(second_edge, nullptr);
  StagePlan plan;
  EXPECT_NE(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  EXPECT_TRUE(plan.GetAllStages().empty());
  EXPECT_EQ(graph.GetDirectNodesSize(), 3U);
  EXPECT_EQ(release_first->GetInDataEdgeByIndex(0), first_edge);
  EXPECT_EQ(release_second->GetInDataEdgeByIndex(0), second_edge);
  EXPECT_EQ(resource->GetExtendInfo()->GetOwnerGraphBarePtr(), &graph);
}

TEST(StagePipelineTest, RejectsReleaseWithoutPairedProducer) {
  ExecuteGraph graph("unpaired_release");
  auto *release = AddStageTestNode(graph, "release", "FreeMemory", 1U, 0U);
  StagePlan plan;
  EXPECT_NE(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  EXPECT_TRUE(plan.GetAllStages().empty());
  EXPECT_EQ(graph.GetDirectNodesSize(), 1U);
  EXPECT_EQ(release->GetInDataEdgeByIndex(0), nullptr);
  EXPECT_EQ(release->GetExtendInfo()->GetOwnerGraphBarePtr(), &graph);
}

TEST(StagePipelineTest, BridgesMainValueToDeInitAndKeepsUserOutput) {
  ExecuteGraph graph("main_to_deinit");
  auto *main = AddStageTestNode(graph, "main", "PlainKernel");
  auto *deinit = AddStageTestNode(graph, "deinit", "PlainKernel", 1U, 0U);
  ASSERT_TRUE(AttrUtils::SetStr(deinit->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageDeInit));
  ASSERT_NE(graph.AddEdge(main, 0, deinit, 0), nullptr);
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  StageSplitter splitter(graph);
  ASSERT_EQ(splitter.Split(plan, {{main, 0}}), GRAPH_SUCCESS);
  auto *main_graph = splitter.GetStageGraph(StageKind::kMain);
  auto *deinit_graph = splitter.GetStageGraph(StageKind::kDeInit);
  ASSERT_NE(main_graph, nullptr);
  ASSERT_NE(deinit_graph, nullptr);
  EXPECT_EQ(main->GetExtendInfo()->GetOwnerGraphBarePtr(), main_graph);
  EXPECT_EQ(deinit->GetExtendInfo()->GetOwnerGraphBarePtr(), deinit_graph);
  const auto export_port = splitter.GetExportPort(StageKind::kMain, *main, 0);
  ASSERT_GE(export_port, 0);

  auto *net_output = splitter.GetStageNetOutput(StageKind::kMain);
  ASSERT_NE(net_output, nullptr);
  ASSERT_NE(net_output->GetInDataEdgeByIndex(0), nullptr);
  EXPECT_EQ(net_output->GetInDataEdgeByIndex(0)->src, main);
  FastNode *inner_output = nullptr;
  for (auto *node : main_graph->GetDirectNode()) {
    if (node->GetOpDescPtr()->GetType() == "InnerNetOutput") {
      inner_output = node;
    }
  }
  ASSERT_NE(inner_output, nullptr);
  ASSERT_NE(inner_output->GetInDataEdgeByIndex(export_port), nullptr);
  EXPECT_EQ(inner_output->GetInDataEdgeByIndex(export_port)->src, main);
  EXPECT_EQ(inner_output->GetInDataEdgeByIndex(export_port)->src_output, 0);
  ASSERT_NE(deinit->GetInDataEdgeByIndex(0), nullptr);
  auto *inner_data = deinit->GetInDataEdgeByIndex(0)->src;
  ASSERT_NE(inner_data, nullptr);
  EXPECT_STREQ(inner_data->GetTypePtr(), "InnerData");
  EXPECT_EQ(inner_data->GetExtendInfo()->GetOwnerGraphBarePtr(), deinit_graph);
  int64_t import_port = -1;
  ASSERT_TRUE(AttrUtils::GetInt(inner_data->GetOpDescPtr(), "index", import_port));
  ASSERT_GE(import_port, 0);
  auto *deinit_container = deinit_graph->GetParentNodeBarePtr();
  ASSERT_NE(deinit_container, nullptr);
  auto *bridge = deinit_container->GetInDataEdgeByIndex(static_cast<int32_t>(import_port));
  ASSERT_NE(bridge, nullptr);
  EXPECT_EQ(bridge->src, main_graph->GetParentNodeBarePtr());
  EXPECT_EQ(bridge->src_output, export_port);
  EXPECT_EQ(bridge->dst, deinit_container);
}

TEST(StagePipelineTest, RejectsReverseControlDependencyBeforeMutation) {
  ExecuteGraph graph("reverse_control");
  auto *main = AddStageTestNode(graph, "main", "PlainKernel");
  auto *init = AddStageTestNode(graph, "init", "PlainKernel");
  ASSERT_TRUE(AttrUtils::SetStr(init->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
  auto *control_edge = graph.AddEdge(main, -1, init, -1);
  ASSERT_NE(control_edge, nullptr);
  StagePlan plan;
  ASSERT_EQ(StageAnalyzer(graph).Analyze(plan), GRAPH_SUCCESS);
  StageSplitter splitter(graph);
  EXPECT_NE(splitter.Split(plan, {{main, 0}}), GRAPH_SUCCESS);
  EXPECT_EQ(graph.GetDirectNodesSize(), 2U);
  EXPECT_EQ(main->GetExtendInfo()->GetOwnerGraphBarePtr(), &graph);
  EXPECT_EQ(init->GetExtendInfo()->GetOwnerGraphBarePtr(), &graph);
  ASSERT_EQ(init->GetAllInControlEdgesRef().size(), 1U);
  EXPECT_EQ(init->GetAllInControlEdgesRef()[0], control_edge);
  EXPECT_EQ(control_edge->src, main);
  EXPECT_EQ(control_edge->dst, init);
  EXPECT_EQ(splitter.GetStageGraph(StageKind::kInit), nullptr);
}

}  // namespace ge
