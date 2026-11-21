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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/om2/dynamic_codegen/info_collect/program_info_collector.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/runtime/model_v2_executor.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "stage/stage_splitter.h"

namespace gert {
namespace om2 {
namespace {
ge::FastNode *AddNode(ge::ExecuteGraph &graph, const char *name, const char *type, const uint32_t inputs,
                      const uint32_t outputs) {
  auto desc = std::make_shared<ge::OpDesc>(name, type);
  for (uint32_t i = 0U; i < inputs; ++i) {
    if (desc->AddInputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  for (uint32_t i = 0U; i < outputs; ++i) {
    if (desc->AddOutputDesc(ge::GeTensorDesc()) != ge::GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  return graph.AddNode(desc);
}

void SetResources(ge::ExecuteGraph &graph) {
  // Collection copies the four independent blobs; parsing their contents belongs to codegen/runtime tests.
  const uint8_t buffer[] = {1U, 0U, 2U};
  const uint8_t compute[] = {3U, 0U, 4U};
  const uint8_t extend[] = {5U, 0U, 6U};
  const uint8_t model[] = {7U, 0U, 8U};
  ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kBuffer, ge::Buffer::CopyFrom(buffer, sizeof(buffer))));
  ASSERT_TRUE(
      ge::AttrUtils::SetZeroCopyBytes(&graph, kComputeNodeInfo, ge::Buffer::CopyFrom(compute, sizeof(compute))));
  ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kKernelExtendInfo, ge::Buffer::CopyFrom(extend, sizeof(extend))));
  ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kModelDesc, ge::Buffer::CopyFrom(model, sizeof(model))));
}

void SplitGraph(ge::ExecuteGraph &graph, const std::vector<std::pair<ge::FastNode *, int32_t>> &outputs) {
  ge::StagePlan plan;
  ASSERT_EQ(ge::StageAnalyzer(graph).Analyze(plan), ge::GRAPH_SUCCESS);
  ASSERT_EQ(ge::StageSplitter(graph).Split(plan, outputs), ge::GRAPH_SUCCESS);
  ASSERT_EQ(graph.TopologicalSorting(), ge::GRAPH_SUCCESS);
  SetResources(graph);
}

uint32_t FindKernelIndex(const ProgramBlock &block, const std::string &name) {
  for (uint32_t i = 0U; i < block.kernels.size(); ++i) {
    if (block.kernels[i].execution_node_name == name) {
      return i;
    }
  }
  ADD_FAILURE() << "Missing execution node " << name;
  return kInvalidProgramIndex;
}

void ExpectDependency(const ProgramBlock &block, const std::string &before, const std::string &after) {
  const auto predecessor = FindKernelIndex(block, before);
  const auto successor = FindKernelIndex(block, after);
  ASSERT_NE(predecessor, kInvalidProgramIndex);
  ASSERT_NE(successor, kInvalidProgramIndex);
  EXPECT_LT(predecessor, successor);
  EXPECT_TRUE(std::any_of(block.kernel_dependencies.begin(), block.kernel_dependencies.end(),
                          [predecessor, successor](const KernelDependency &edge) {
                            return edge.predecessor == predecessor && edge.successor == successor;
                          }))
      << before << " -> " << after;
}

class ProgramInfoCollectorUT : public testing::Test {
 protected:
  void SetUp() override {
    auto *stream = AddNode(graph_, "stream", "Data", 0U, 1U);
    auto *init_sync = AddNode(graph_, "init_sync", "SyncStream", 1U, 0U);
    auto *main_sync = AddNode(graph_, "main_sync", "SyncStream", 1U, 0U);
    auto *deinit_sync = AddNode(graph_, "deinit_sync", "SyncStream", 1U, 0U);
    input_ = AddNode(graph_, "input", "Data", 0U, 1U);
    auto *other_input = AddNode(graph_, "other_input", "Data", 0U, 1U);
    left_ = AddNode(graph_, "left", "IdentityAddr", 1U, 1U);
    auto *left_copy = AddNode(graph_, "left_copy", "IdentityAddr", 1U, 1U);
    auto *right = AddNode(graph_, "right", "IdentityAddr", 1U, 1U);
    auto *join = AddNode(graph_, "join", "PointFromInputs", 2U, 2U);
    auto *deinit_copy = AddNode(graph_, "deinit_copy", "IdentityAddr", 1U, 1U);
    for (auto *node :
         {stream, init_sync, main_sync, deinit_sync, input_, other_input, left_, left_copy, right, join, deinit_copy}) {
      ASSERT_NE(node, nullptr);
    }
    ASSERT_TRUE(
        ge::AttrUtils::SetInt(stream->GetOpDescBarePtr(), kFeedIndex, static_cast<int64_t>(ExecuteArgIndex::kStream)));
    ASSERT_TRUE(ge::AttrUtils::SetInt(input_->GetOpDescBarePtr(), kFeedIndex, 0));
    ASSERT_TRUE(ge::AttrUtils::SetInt(other_input->GetOpDescBarePtr(), kFeedIndex, 1));
    for (auto *node : {stream, init_sync}) {
      ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kStage, attr::kStageInit));
    }
    for (auto *node : {deinit_sync, deinit_copy}) {
      ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kStage, attr::kStageDeInit));
    }
    for (auto *consumer : {init_sync, main_sync, deinit_sync}) {
      ASSERT_NE(graph_.AddEdge(stream, 0, consumer, 0), nullptr);
    }
    ASSERT_NE(graph_.AddEdge(input_, 0, left_, 0), nullptr);
    ASSERT_NE(graph_.AddEdge(input_, 0, left_copy, 0), nullptr);
    ASSERT_NE(graph_.AddEdge(other_input, 0, right, 0), nullptr);
    ASSERT_NE(graph_.AddEdge(left_, 0, join, 0), nullptr);
    ASSERT_NE(graph_.AddEdge(right, 0, join, 1), nullptr);
    ASSERT_NE(graph_.AddEdge(left_copy, 0, deinit_copy, 0), nullptr);
    ASSERT_NE(graph_.AddEdge(main_sync, -1, left_, -1), nullptr);
    ASSERT_NE(graph_.AddEdge(left_copy, -1, right, -1), nullptr);
    ASSERT_NE(graph_.AddEdge(deinit_sync, -1, deinit_copy, -1), nullptr);
    SplitGraph(graph_, {{join, 0}, {join, 1}});
  }

  ge::ExecuteGraph graph_{"collector"};
  ge::FastNode *input_ = nullptr;
  ge::FastNode *left_ = nullptr;
};

// C-01: collect the actual split lifecycle graphs and preserve both data and order-only constraints.
TEST_F(ProgramInfoCollectorUT, CollectsThreeStagesIoResourcesAndDependencyOrder) {
  ProgramInfo info;
  ASSERT_EQ(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.init.kernels.size(), 1U);
  ASSERT_EQ(info.main.kernels.size(), 5U);
  ASSERT_EQ(info.deinit.kernels.size(), 2U);
  EXPECT_EQ(info.init.kernels[0].execution_node_name, "init_sync");
  EXPECT_EQ(info.init.kernels[0].kernel_type, "SyncStream");
  EXPECT_EQ(info.init.plan_type, ProgramPlanType::kSequential);
  EXPECT_EQ(info.deinit.plan_type, ProgramPlanType::kSequential);
  const auto left_index = FindKernelIndex(info.main, "left");
  const auto right_index = FindKernelIndex(info.main, "right");
  const auto join_index = FindKernelIndex(info.main, "join");
  ASSERT_LT(left_index, info.main.kernels.size());
  ASSERT_LT(right_index, info.main.kernels.size());
  ASSERT_LT(join_index, info.main.kernels.size());
  const auto &left = info.main.kernels[left_index];
  const auto &right = info.main.kernels[right_index];
  const auto &join = info.main.kernels[join_index];
  ASSERT_EQ(left.inputs.size(), 1U);
  ASSERT_EQ(left.outputs.size(), 1U);
  EXPECT_EQ(left.inputs[0].value_type, Om2ValueType::kGertTensorDataPtr);
  EXPECT_EQ(left.output_policy, OutputCreatePolicy::kOutputsCreator);
  EXPECT_EQ(info.chains.at(left.inputs[0].chain_index).source_kind, ChainSourceKind::kModelInput);
  EXPECT_EQ(info.chains.at(left.inputs[0].chain_index).source_index, 0);
  ASSERT_EQ(right.inputs.size(), 1U);
  EXPECT_EQ(info.chains.at(right.inputs[0].chain_index).source_index, 1);
  ASSERT_EQ(join.outputs.size(), 2U);
  for (size_t i = 0U; i < join.outputs.size(); ++i) {
    const auto &chain = info.chains.at(join.outputs[i].chain_index);
    EXPECT_EQ(chain.source_kind, ChainSourceKind::kModelOutput);
    EXPECT_EQ(chain.source_index, static_cast<int64_t>(i));
    EXPECT_EQ(chain.binding_phase, ChainBindingPhase::kRun);
  }
  ExpectDependency(info.main, "main_sync", "left");
  ExpectDependency(info.main, "left_copy", "right");
  ExpectDependency(info.main, "left", "join");
  ExpectDependency(info.main, "right", "join");
  ExpectDependency(info.deinit, "deinit_sync", "deinit_copy");
  EXPECT_EQ(info.resources.buffer, (std::vector<uint8_t>{1U, 0U, 2U}));
  EXPECT_EQ(info.resources.compute_node_infos, (std::vector<uint8_t>{3U, 0U, 4U}));
  EXPECT_EQ(info.resources.kernel_extend_infos, (std::vector<uint8_t>{5U, 0U, 6U}));
  EXPECT_EQ(info.resources.model_desc, (std::vector<uint8_t>{7U, 0U, 8U}));
  EXPECT_TRUE(info.const_chains.empty());
}

// C-02: compare endpoint equivalence, never the allocator's incidental numeric chain IDs.
TEST_F(ProgramInfoCollectorUT, SharesFanoutAndStageBridgesWithoutMergingIndependentValues) {
  ProgramInfo info;
  ASSERT_EQ(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.init.kernels.size(), 1U);
  const auto left_index = FindKernelIndex(info.main, "left");
  const auto copy_index = FindKernelIndex(info.main, "left_copy");
  const auto right_index = FindKernelIndex(info.main, "right");
  const auto sync_index = FindKernelIndex(info.main, "main_sync");
  const auto deinit_sync_index = FindKernelIndex(info.deinit, "deinit_sync");
  const auto deinit_copy_index = FindKernelIndex(info.deinit, "deinit_copy");
  ASSERT_LT(left_index, info.main.kernels.size());
  ASSERT_LT(copy_index, info.main.kernels.size());
  ASSERT_LT(right_index, info.main.kernels.size());
  ASSERT_LT(sync_index, info.main.kernels.size());
  ASSERT_LT(deinit_sync_index, info.deinit.kernels.size());
  ASSERT_LT(deinit_copy_index, info.deinit.kernels.size());
  const auto &left = info.main.kernels[left_index];
  const auto &copy = info.main.kernels[copy_index];
  const auto &right = info.main.kernels[right_index];
  ASSERT_EQ(left.inputs.size(), 1U);
  ASSERT_EQ(copy.inputs.size(), 1U);
  ASSERT_EQ(right.inputs.size(), 1U);
  EXPECT_EQ(left.inputs[0].chain_index, copy.inputs[0].chain_index);
  EXPECT_NE(left.inputs[0].chain_index, right.inputs[0].chain_index);
  ASSERT_EQ(left.outputs.size(), 1U);
  ASSERT_EQ(copy.outputs.size(), 1U);
  EXPECT_NE(left.outputs[0].chain_index, copy.outputs[0].chain_index);
  ASSERT_EQ(info.deinit.kernels[deinit_copy_index].inputs.size(), 1U);
  EXPECT_EQ(copy.outputs[0].chain_index, info.deinit.kernels[deinit_copy_index].inputs[0].chain_index);
  ASSERT_EQ(info.init.kernels[0].inputs.size(), 1U);
  ASSERT_EQ(info.main.kernels[sync_index].inputs.size(), 1U);
  ASSERT_EQ(info.deinit.kernels[deinit_sync_index].inputs.size(), 1U);
  const auto stream_chain = info.init.kernels[0].inputs[0].chain_index;
  EXPECT_EQ(stream_chain, info.main.kernels[sync_index].inputs[0].chain_index);
  EXPECT_EQ(stream_chain, info.deinit.kernels[deinit_sync_index].inputs[0].chain_index);
  EXPECT_EQ(info.chains.at(stream_chain).source_kind, ChainSourceKind::kRuntimeBinding);
  EXPECT_EQ(info.chains.at(stream_chain).binding_kind, ChainBindingKind::kStream);
  EXPECT_EQ(info.chains.at(stream_chain).binding_phase, ChainBindingPhase::kLoad);
}

TEST_F(ProgramInfoCollectorUT, DefaultCollectorKeepsKernelForRuntimeResolution) {
  constexpr char type[] = "UnregisteredOm2CollectorTestKernel";
  left_->GetOpDescBarePtr()->SetType(type);
  ProgramInfo info;
  ASSERT_EQ(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
  const auto index = FindKernelIndex(info.main, "left");
  ASSERT_LT(index, info.main.kernels.size());
  const auto &kernel = info.main.kernels[index];
  EXPECT_EQ(kernel.kernel_type, type);
  EXPECT_EQ(kernel.impl_type, KernelImplType::kRunFuncImpl);
  EXPECT_EQ(kernel.output_policy, OutputCreatePolicy::kGraphBound);
  EXPECT_FALSE(kernel.require_compute_node_info);
  ASSERT_EQ(kernel.inputs.size(), 1U);
  ASSERT_EQ(kernel.outputs.size(), 1U);
  EXPECT_EQ(kernel.inputs[0].value_type, Om2ValueType::kBytePtr);
  EXPECT_EQ(kernel.outputs[0].value_type, Om2ValueType::kBytePtr);
}

TEST_F(ProgramInfoCollectorUT, RejectsMissingGraphResource) {
  graph_.DelAttr(kComputeNodeInfo);
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

TEST_F(ProgramInfoCollectorUT, RejectsDataWithoutFeedIndex) {
  input_->GetOpDescBarePtr()->DelAttr(kFeedIndex);
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

TEST_F(ProgramInfoCollectorUT, RejectsMalformedFeedIndex) {
  input_->GetOpDescBarePtr()->DelAttr(kFeedIndex);
  ASSERT_TRUE(ge::AttrUtils::SetStr(input_->GetOpDescBarePtr(), kFeedIndex, "not_an_index"));
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

TEST_F(ProgramInfoCollectorUT, RejectsMissingRequiredComputeNodeIndex) {
  left_->GetOpDescBarePtr()->SetType("IdentityShape");
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

TEST_F(ProgramInfoCollectorUT, RejectsInvalidComputeNodeIndex) {
  ASSERT_TRUE(ge::AttrUtils::SetInt(left_->GetOpDescBarePtr(), kComputeNodeIndex, -1));
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

TEST_F(ProgramInfoCollectorUT, RejectsKernelPortCountMismatch) {
  left_->GetOpDescBarePtr()->SetType("NoOp");
  ProgramInfo info;
  EXPECT_NE(ProgramInfoCollector().Collect(graph_, info), ge::GRAPH_SUCCESS);
}

// C-03: fixed-ABI kernels do not require a source graph or ComputeNodeIndex.
TEST(ProgramInfoCollectorMetadataUT, AcceptsNoOpWithoutSourceOrComputeNodeMetadata) {
  ge::ExecuteGraph graph("fixed_abi");
  ASSERT_NE(AddNode(graph, "noop", "NoOp", 0U, 0U), nullptr);
  SplitGraph(graph, {});
  ASSERT_FALSE(testing::Test::HasFatalFailure());
  ProgramInfo info;
  ASSERT_EQ(ProgramInfoCollector().Collect(graph, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.main.kernels.size(), 1U);
  EXPECT_EQ(info.main.kernels[0].execution_node_name, "noop");
  EXPECT_EQ(info.main.kernels[0].compute_node_index, kInvalidProgramIndex);
  EXPECT_FALSE(info.main.kernels[0].require_compute_node_info);
  EXPECT_TRUE(info.init.kernels.empty());
  EXPECT_TRUE(info.deinit.kernels.empty());
}
}  // namespace
}  // namespace om2
}  // namespace gert
