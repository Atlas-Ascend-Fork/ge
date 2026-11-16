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
#include "common/ge_inner_attrs.h"
#include "common/om2/dynamic_codegen/info_collect/execution_plan_collector.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/info_collect/program_info_collector.h"
#include "common/om2/om2_online_prepare_data.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "framework/common/helper/om2_package_helper.h"
#include "../../graph/load/om2_compile_stub.h"
#include "../../graph/load/om2_package_test_utils.h"
#include "common/share_graph.h"
#include "faker/ge_model_builder.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "graph/op_kernel_bin.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "lowering/model_converter.h"
#include "stage/stage_splitter.h"
#include "stub/gert_runtime_stub.h"

namespace gert {
namespace om2 {
namespace {
void SetNodeDescs(const ge::NodePtr &node, const ge::GeTensorDesc &input, const ge::GeTensorDesc &output) {
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(node->GetOpDesc()->UpdateInputDesc(0U, input), ge::GRAPH_SUCCESS);
  ASSERT_EQ(node->GetOpDesc()->UpdateOutputDesc(0U, output), ge::GRAPH_SUCCESS);
}

void MakeIfGraph(ge::ComputeGraphPtr &graph) {
  graph = ShareGraph::IfGraph();
  ASSERT_NE(graph, nullptr);
  const ge::GeTensorDesc condition(ge::GeShape(std::vector<int64_t>{}), ge::FORMAT_ND, ge::DT_BOOL);
  const ge::GeTensorDesc input(ge::GeShape({-1, -1}), ge::FORMAT_ND, ge::DT_FLOAT);
  const ge::GeTensorDesc output(ge::GeShape({-1}), ge::FORMAT_ND, ge::DT_INT64);
  ASSERT_NO_FATAL_FAILURE(SetNodeDescs(graph->FindNode("pred"), condition, condition));
  ASSERT_NO_FATAL_FAILURE(SetNodeDescs(graph->FindNode("input"), input, input));
  ASSERT_NO_FATAL_FAILURE(SetNodeDescs(graph->FindNode("if"), condition, output));
  ASSERT_EQ(graph->FindNode("if")->GetOpDesc()->UpdateInputDesc(1U, input), ge::GRAPH_SUCCESS);
  ASSERT_EQ(graph->FindNode("NetOutput")->GetOpDesc()->UpdateInputDesc(0U, output), ge::GRAPH_SUCCESS);
  for (const auto &name : {"then", "else"}) {
    auto branch = graph->GetSubgraph(name);
    ASSERT_NE(branch, nullptr);
    ASSERT_NO_FATAL_FAILURE(SetNodeDescs(branch->FindNode("data"), input, input));
    ASSERT_NO_FATAL_FAILURE(SetNodeDescs(branch->FindNode("shape"), input, output));
    auto desc = branch->FindNode("shape")->GetOpDesc();
    ge::OpDescUtilsEx::SetType(desc, "Shape");
    ASSERT_TRUE(ge::AttrUtils::SetInt(desc, "dtype", ge::DT_INT64));
    auto netoutput = branch->FindNode("NetOutput")->GetOpDesc();
    ASSERT_EQ(netoutput->UpdateInputDesc(0U, output), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(ge::AttrUtils::SetInt(netoutput->MutableInputDesc(0U), ge::ATTR_NAME_PARENT_NODE_INDEX, 0));
    ASSERT_EQ(branch->TopologicalSorting(), ge::GRAPH_SUCCESS);
  }
  ASSERT_EQ(graph->TopologicalSorting(), ge::GRAPH_SUCCESS);
}

struct LoweredIf {
  ge::GeRootModelPtr model;
  ge::ExecuteGraphPtr graph;
  ge::ExecuteGraph *main = nullptr;
  ChainNormalizer normalizer;
  std::vector<ChainInfo> chains;

  void Build() {
    ge::ComputeGraphPtr compute;
    ASSERT_NO_FATAL_FAILURE(MakeIfGraph(compute));
    model = GeModelBuilder(compute).BuildGeRootModel();
    ASSERT_NE(model, nullptr);
    ModelConverter::Args args;
    args.for_om2 = true;
    graph = ModelConverter().ConvertGeModelToExecuteGraph(model, args);
    ASSERT_NE(graph, nullptr);
    auto *main_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(graph.get(), "Main");
    ASSERT_NE(main_node, nullptr);
    main = ge::FastNodeUtils::GetSubgraphFromNode(main_node, 0U);
    ASSERT_NE(main, nullptr);
    ASSERT_EQ(normalizer.Normalize(*graph, chains), ge::GRAPH_SUCCESS);
  }
};

void CheckBranchRegions(const ProgramBlock &block) {
  ASSERT_LT(block.root_region_index, block.regions.size());
  const ProgramStep *conditional = nullptr;
  for (const auto &step : block.regions[block.root_region_index].steps) {
    if (step.kind == ProgramStepKind::kIf) {
      ASSERT_EQ(conditional, nullptr);
      conditional = &step;
    }
  }
  ASSERT_NE(conditional, nullptr);
  ASSERT_LT(conditional->condition_region_index, block.regions.size());
  const auto &info = conditional->if_info;
  EXPECT_EQ(info.selector_value_type, Om2ValueType::kUInt32T);
  EXPECT_FALSE(info.merge_info.join_node_name.empty());
  ASSERT_EQ(info.branches.size(), 2U);
  EXPECT_NE(info.branches[0].region_index, info.branches[1].region_index);
  for (size_t i = 0; i < info.branches.size(); ++i) {
    const auto &branch = info.branches[i];
    EXPECT_EQ(branch.selector_value, i);
    ASSERT_LT(branch.region_index, block.regions.size());
    EXPECT_FALSE(branch.entry_node_name.empty());
    EXPECT_FALSE(branch.exit_node_name.empty());
    EXPECT_FALSE(branch.completion_node_names.empty());
    EXPECT_FALSE(block.regions[branch.region_index].steps.empty());
  }
}

TEST(ExecutionPlanCoverageUT, CollectsLoweredIfRegionsForBothTopologicalPolicies) {
  GertRuntimeStub stub;
  LoweredIf lowered;
  ASSERT_NO_FATAL_FAILURE(lowered.Build());
  for (const auto policy : {ProgramPlanType::kStructuredTopological, ProgramPlanType::kStructuredTopologicalPriority}) {
    ProgramBlock block;
    ASSERT_EQ(ExecutionPlanCollector().Collect(*lowered.main, lowered.normalizer, policy, block), ge::GRAPH_SUCCESS);
    EXPECT_EQ(block.plan_type, policy);
    ASSERT_NO_FATAL_FAILURE(CheckBranchRegions(block));
    for (const auto &dependency : block.kernel_dependencies) {
      EXPECT_LT(dependency.predecessor, block.kernels.size());
      EXPECT_LT(dependency.successor, block.kernels.size());
      EXPECT_NE(dependency.predecessor, dependency.successor);
    }
  }
  ProgramInfo collected;
  ASSERT_EQ(ProgramInfoCollector().Collect(*lowered.graph, collected), ge::GRAPH_SUCCESS);
  EXPECT_FALSE(collected.chains.empty());
  EXPECT_FALSE(collected.resources.model_desc.empty());
}

TEST(ExecutionPlanCoverageUT, RejectsBrokenControlSkeletonAndUnsupportedPolicies) {
  struct InvalidControl {
    const char *type;
    const char *replacement;
    const char *diagnostic;
  };
  const InvalidControl cases[]{{"SwitchNotify", "NoOp", "switch_notify"},
                               {"WaitAnyone", "NoOp", "wait_anyone"},
                               {"BranchPivot", "BranchPivot", "invalid branch"}};
  for (const auto &item : cases) {
    SCOPED_TRACE(item.type);
    GertRuntimeStub stub;
    LoweredIf lowered;
    ASSERT_NO_FATAL_FAILURE(lowered.Build());
    auto *if_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(lowered.main, "If");
    ASSERT_NE(if_node, nullptr);
    auto *condition_graph = ge::FastNodeUtils::GetSubgraphFromNode(if_node, 0U);
    ASSERT_NE(condition_graph, nullptr);
    auto *node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(condition_graph, item.type);
    ASSERT_NE(node, nullptr);
    node->GetOpDescBarePtr()->SetType(item.replacement);
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), ge::kRelativeBranch, 0));
    ProgramBlock block;
    stub.GetSlogStub().Clear();
    EXPECT_NE(ExecutionPlanCollector().Collect(*lowered.main, lowered.normalizer,
                                               ProgramPlanType::kStructuredTopological, block),
              ge::GRAPH_SUCCESS);
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, item.diagnostic), -1);
  }
  GertRuntimeStub stub;
  LoweredIf lowered;
  ASSERT_NO_FATAL_FAILURE(lowered.Build());
  for (const auto policy : {ProgramPlanType::kSequential, static_cast<ProgramPlanType>(99)}) {
    ProgramBlock block;
    EXPECT_NE(ExecutionPlanCollector().Collect(*lowered.main, lowered.normalizer, policy, block), ge::GRAPH_SUCCESS);
  }
}

using ExecutionPlanPackageUT = ge::om2_test::DynamicPackageTest;

TEST_F(ExecutionPlanPackageUT, PackagesLoweredIfAndRetainsOnlinePreparationGraphs) {
  GertRuntimeStub stub;
  ge::ComputeGraphPtr graph;
  ASSERT_NO_FATAL_FAILURE(MakeIfGraph(graph));
  graph->SetGraphID(37U);
  auto model = GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(model, nullptr);
  ge::om2_test::Om2CompileStub compile;
  GertModelData data;
  ASSERT_EQ(ge::Om2PackageHelper().BuildDynamicGertModelData(model, data), ge::SUCCESS);
  ASSERT_NE(data.online_prepare_data, nullptr);
  EXPECT_EQ(data.online_prepare_data->graph_id, 37U);
  EXPECT_TRUE(data.online_prepare_data->prepare_ge_variables);
  ASSERT_FALSE(data.online_prepare_data->graphs.empty());
  EXPECT_EQ(data.online_prepare_data->graphs.front(), graph);
  ASSERT_EQ(data.models.size(), 1U);
  ASSERT_NE(data.models[0], nullptr);
  EXPECT_EQ(data.models[0]->execution_type, GertModelExecutionType::kDynamic);
}

class MaterializedRestoreBuilder final : public KernelCodeBuilder {
 public:
  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {0U, 1U};
  }
  std::vector<uint32_t> GetCustomRestoreConstInputIndices() const override {
    return {kKernelLastInputIndex};
  }
  KernelImplType GetImplType() const override {
    return KernelImplType::kCodegenImpl;
  }
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if ((context.input_chains[0U].GetValue<uint64_t>() != 17U) ||
        (context.input_chains[1U].GetValue<uint64_t>() != 29U)) {
      return ge::GRAPH_FAILED;
    }
    info.execution_node_name = context.node.GetNamePtr();
    info.kernel_type = context.node.GetTypePtr();
    info.inputs = {{context.input_chain_indices[0U], "ordinary", Om2ValueType::kUInt64T},
                   {context.input_chain_indices[1U], "custom", Om2ValueType::kUInt64T}};
    return ge::GRAPH_SUCCESS;
  }
  ge::graphStatus RestoreConstInput(const KernelCollectContext &context, uint32_t index,
                                    CustomConstRestoreDescriptor &descriptor) const override {
    if (index != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    int64_t invalid = 0;
    (void)ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "invalid_descriptor", invalid);
    const auto value = std::to_string(context.input_chains[index].GetValue<uint64_t>());
    descriptor.initializer = invalid == 1 ? "" : "restored_scalar";
    if (invalid != 2) {
      descriptor.payload.assign(value.begin(), value.end());
      descriptor.payload.push_back(0U);
    }
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2MaterializedRestoreUT", MaterializedRestoreBuilder);

struct MaterializedRestoreGraph {
  ge::ExecuteGraph graph{"restore_graph"};
  ge::FastNode *consumer = nullptr;
  ge::ExecuteGraph *main = nullptr;
  ChainNormalizer normalizer;
  std::vector<ChainInfo> chains;

  void Build(int64_t invalid_descriptor) {
    auto desc = std::make_shared<ge::OpDesc>("restore_consumer", "Om2MaterializedRestoreUT");
    ASSERT_EQ(desc->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    ASSERT_EQ(desc->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kStage, attr::kStageMain));
    ASSERT_TRUE(ge::AttrUtils::SetInt(desc, "invalid_descriptor", invalid_descriptor));
    consumer = graph.AddNode(desc);
    ASSERT_NE(consumer, nullptr);
    ASSERT_NO_FATAL_FAILURE(AddConstant(0U, 17U));
    ASSERT_NO_FATAL_FAILURE(AddConstant(1U, 29U));
    ge::StagePlan plan;
    ASSERT_EQ(ge::StageAnalyzer(graph).Analyze(plan), ge::GRAPH_SUCCESS);
    ASSERT_EQ(ge::StageSplitter(graph).Split(plan, {}), ge::GRAPH_SUCCESS);
    ASSERT_EQ(graph.TopologicalSorting(), ge::GRAPH_SUCCESS);
    ASSERT_EQ(normalizer.Normalize(graph, chains), ge::GRAPH_SUCCESS);
    auto *main_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Main");
    ASSERT_NE(main_node, nullptr);
    main = ge::FastNodeUtils::GetSubgraphFromNode(main_node, 0U);
    ASSERT_NE(main, nullptr);
  }

  void AddConstant(uint32_t input, uint64_t value) {
    auto desc = std::make_shared<ge::OpDesc>("scalar_" + std::to_string(input), "Const");
    ASSERT_EQ(desc->AddOutputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
        desc, kConstValue, ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&value), sizeof(value))));
    auto *constant = graph.AddNode(desc);
    ASSERT_NE(constant, nullptr);
    ASSERT_NE(graph.AddEdge(constant, 0, consumer, input), nullptr);
  }
};

TEST(ExecutionPlanCoverageUT, MaterializesBothRestoreKindsAndKeepsCustomConstantAtRuntime) {
  GertRuntimeStub stub;
  MaterializedRestoreGraph graph;
  ASSERT_NO_FATAL_FAILURE(graph.Build(0));
  for (const auto policy : {ProgramPlanType::kSequential, ProgramPlanType::kStructuredTopological}) {
    ProgramBlock block;
    ASSERT_EQ(ExecutionPlanCollector().Collect(*graph.main, graph.normalizer, policy, block), ge::GRAPH_SUCCESS);
    ASSERT_EQ(block.kernels.size(), 1U);
    const auto &kernel = block.kernels[0U];
    EXPECT_EQ(kernel.execution_node_name, "restore_consumer");
    EXPECT_EQ(kernel.impl_type, KernelImplType::kCodegenImpl);
    ASSERT_NE(kernel.code_builder, nullptr);
    ASSERT_EQ(kernel.inputs.size(), 2U);
    EXPECT_EQ(kernel.compile_time_const_chain_indices, (std::vector<uint32_t>{kernel.inputs[0U].chain_index}));
    ASSERT_EQ(kernel.custom_const_restore_descriptors.size(), 1U);
    const auto &descriptor = kernel.custom_const_restore_descriptors[0U];
    EXPECT_EQ(descriptor.chain_index, kernel.inputs[1U].chain_index);
    EXPECT_EQ(descriptor.producer_name, "scalar_1");
    EXPECT_EQ(descriptor.initializer, "restored_scalar");
    EXPECT_EQ(descriptor.payload, (std::vector<uint8_t>{'2', '9', 0U}));
    ASSERT_EQ(block.regions.size(), 1U);
    ASSERT_EQ(block.regions[0U].steps.size(), 1U);
    EXPECT_EQ(block.regions[0U].steps[0U].kernel_index, 0U);
  }
}

TEST(ExecutionPlanCoverageUT, RejectsCustomDescriptorsBeforePublishingKernelOrStep) {
  const std::vector<std::pair<int64_t, const char *>> cases{{1, "has no initializer"}, {2, "has no payload"}};
  for (const auto &entry : cases) {
    SCOPED_TRACE(entry.second);
    GertRuntimeStub stub;
    MaterializedRestoreGraph graph;
    ASSERT_NO_FATAL_FAILURE(graph.Build(entry.first));
    stub.GetSlogStub().Clear();
    ProgramBlock block;
    EXPECT_NE(
        ExecutionPlanCollector().Collect(*graph.main, graph.normalizer, ProgramPlanType::kStructuredTopological, block),
        ge::GRAPH_SUCCESS);
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, entry.second), -1);
    EXPECT_TRUE(block.kernels.empty());
    ASSERT_EQ(block.regions.size(), 1U);
    EXPECT_TRUE(block.regions[0U].steps.empty());
  }
}
class ResourceBindingBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    int64_t type = 0;
    if (!ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "resource_type", type)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    info.execution_node_name = context.node.GetNamePtr();
    info.kernel_type = context.node.GetTypePtr();
    for (size_t i = 0U; i < context.input_chain_indices.size(); ++i) {
      info.inputs.push_back(
          {context.input_chain_indices[i], "resource_" + std::to_string(i), static_cast<Om2ValueType>(type)});
    }
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ResourceBindingUT", ResourceBindingBuilder);

void SetBindingGraphResources(MaterializedRestoreGraph &fixture, Om2ValueType type, const ge::OpKernelBinPtr *binary) {
  fixture.consumer->GetOpDescBarePtr()->SetType("Om2ResourceBindingUT");
  ASSERT_TRUE(ge::AttrUtils::SetInt(fixture.consumer->GetOpDescBarePtr(), "resource_type", static_cast<int64_t>(type)));
  const uint8_t resource[] = {1U, 2U};
  for (const auto *name : {kBuffer, kComputeNodeInfo, kKernelExtendInfo, kModelDesc}) {
    ASSERT_TRUE(
        ge::AttrUtils::SetZeroCopyBytes(&fixture.graph, name, ge::Buffer::CopyFrom(resource, sizeof(resource))));
  }
  if (type != Om2ValueType::kGeKernelBinPtr) {
    return;
  }
  for (const auto *node : fixture.graph.GetAllNodes()) {
    if (std::string(node->GetTypePtr()) == "Const") {
      ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
          node->GetOpDescBarePtr(), kConstValue,
          ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&binary), sizeof(binary))));
    }
  }
}

TEST(ExecutionPlanCoverageUT, ProgramCollectionBindsRegistryAndBinaryConstantsAtLoad) {
  struct Binding {
    Om2ValueType type;
    ChainBindingKind kind;
  };
  const Binding cases[]{{Om2ValueType::kGertOpImplSpaceRegistryV2Ptr, ChainBindingKind::kSpaceRegistry},
                        {Om2ValueType::kGertCustomOpRegistryPtr, ChainBindingKind::kCustomOpRegistry},
                        {Om2ValueType::kGeKernelBinPtr, ChainBindingKind::kGeKernelBinary}};
  const auto binary = std::make_shared<ge::OpKernelBin>("binary", std::vector<char>{'a', 'b', 'c'});
  for (const auto &entry : cases) {
    GertRuntimeStub stub;
    MaterializedRestoreGraph fixture;
    ASSERT_NO_FATAL_FAILURE(fixture.Build(0));
    ASSERT_NO_FATAL_FAILURE(SetBindingGraphResources(fixture, entry.type, &binary));
    ProgramInfo info;
    ASSERT_EQ(ProgramInfoCollector().Collect(fixture.graph, info), ge::GRAPH_SUCCESS);
    ASSERT_EQ(info.main.kernels.size(), 1U);
    ASSERT_EQ(info.main.kernels[0U].inputs.size(), 2U);
    for (const auto &port : info.main.kernels[0U].inputs) {
      ASSERT_LT(port.chain_index, info.chains.size());
      const auto &chain = info.chains[port.chain_index];
      EXPECT_EQ(chain.source_kind, ChainSourceKind::kConstant);
      EXPECT_EQ(chain.binding_phase, ChainBindingPhase::kLoad);
      EXPECT_EQ(chain.binding_kind, entry.kind);
      if (entry.kind == ChainBindingKind::kGeKernelBinary) {
        EXPECT_EQ(info.binary_resource_keys.at(chain.index),
                  GetKernelBinaryKey(binary->GetBinData(), binary->GetBinDataSize()));
      }
    }
    EXPECT_TRUE(info.const_chains.empty());
    EXPECT_EQ(info.resources.buffer, (std::vector<uint8_t>{1U, 2U}));
  }
}

}  // namespace
}  // namespace om2
}  // namespace gert
