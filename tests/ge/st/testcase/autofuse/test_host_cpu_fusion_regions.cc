/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the license.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "graph/op_so_bin.h"
#include "graph/partition/optimizer/host_cpu_fusion_pass.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph_metadef/common/ge_common/ge_types.h"

namespace ge {
namespace {
constexpr size_t kNodesPerChain = 5U;

NodePtr AddNode(const ComputeGraphPtr &graph, const std::string &name, const std::string &type, size_t inputs,
                size_t outputs) {
  const GeTensorDesc desc(GeShape({1}), FORMAT_ND, DT_INT64);
  auto op = std::make_shared<OpDesc>(name, type);
  for (size_t i = 0U; i < inputs; ++i) {
    EXPECT_EQ(op->AddInputDesc("input_" + std::to_string(i), desc), GRAPH_SUCCESS);
  }
  for (size_t i = 0U; i < outputs; ++i) {
    EXPECT_EQ(op->AddOutputDesc("output_" + std::to_string(i), desc), GRAPH_SUCCESS);
  }
  return graph->AddNode(op);
}

NodePtr AddHostNode(const ComputeGraphPtr &graph, const std::string &name, const NodePtr &input) {
  const auto node = AddNode(graph, name, "HostST", 1U, 1U);
  node->GetOpDesc()->SetOpEngineName("DNN_VM_HOST_CPU");
  node->GetOpDesc()->SetOpKernelLibName("DNN_VM_HOST_CPU_OP_STORE");
  EXPECT_TRUE(AttrUtils::SetBool(node->GetOpDesc(), "SmallShapeHostcpu", true));
  EXPECT_EQ(GraphUtils::AddEdge(input->GetOutDataAnchor(0), node->GetInDataAnchor(0)), GRAPH_SUCCESS);
  return node;
}

std::array<std::vector<NodePtr>, 2U> AddIndependentChains(const ComputeGraphPtr &graph, const NodePtr &input) {
  std::array<std::vector<NodePtr>, 2U> chains;
  for (size_t chain = 0U; chain < chains.size(); ++chain) {
    auto previous = input;
    for (size_t index = 0U; index < kNodesPerChain; ++index) {
      previous = AddHostNode(graph, "host_" + std::to_string(chain) + "_" + std::to_string(index), previous);
      chains[chain].emplace_back(previous);
    }
  }
  return chains;
}

bool SupportsTestKernels(const std::string &) {
  return true;
}

class RegionSoCompiler final : public HostCpuFusionCompiler {
 public:
  explicit RegionSoCompiler(const std::vector<uint8_t> &so_data) : so_data_(so_data) {}

  Status Compile(const std::string &source, std::vector<uint8_t> &so_data) const override {
    if (std::this_thread::get_id() != owner_) {
      ++worker_calls;
    }
    if (fail_second_chain && (source.find("host_1_0") != std::string::npos)) {
      return FAILED;
    }
    so_data = so_data_;
    return SUCCESS;
  }

  mutable std::atomic<size_t> worker_calls{0U};
  bool fail_second_chain = false;

 private:
  const std::thread::id owner_ = std::this_thread::get_id();
  const std::vector<uint8_t> &so_data_;
};
}  // namespace

/**
 * 用例描述：验证融合分支只保留一份共有祖先。
 * 预置条件：构造一个有六个 HostCPU 节点的分量，两条输出路径的祖先集合不同。
 * 测试步骤：生成 HostCPU 融合计划，检查候选节点、输入锚点和输出锚点。
 * 预期结果：分支合为一个区域，共有祖先出现一次，两个输出仍连到原消费端。
 */
TEST(HostCpuFusionRegionsST, MergesBranchesWithSharedAncestors) {
  const auto graph = std::make_shared<ComputeGraph>("st_host_shared_ancestors");
  const auto data = AddNode(graph, "data", "Data", 0U, 1U);
  const auto root = AddHostNode(graph, "root", data);
  const auto shared = AddHostNode(graph, "shared", root);
  const auto left = AddHostNode(graph, "left", shared);
  const auto right = AddHostNode(graph, "right", shared);
  const auto left_tail = AddHostNode(graph, "left_tail", left);
  const auto last = AddHostNode(graph, "last", left_tail);
  const auto output = AddNode(graph, "output", "NetOutput", 2U, 0U);
  ASSERT_EQ(GraphUtils::AddEdge(last->GetOutDataAnchor(0), output->GetInDataAnchor(0)), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(right->GetOutDataAnchor(0), output->GetInDataAnchor(1)), GRAPH_SUCCESS);

  HostCpuFusionPass pass(nullptr, SupportsTestKernels);
  std::vector<std::vector<HostCpuFusionRegion>> components;
  ASSERT_EQ(pass.BuildFusionRegions(graph, components), SUCCESS);
  ASSERT_EQ(components.size(), 1U);
  ASSERT_EQ(components.front().size(), 1U);
  const auto &merged = components.front().front();
  EXPECT_EQ(merged.nodes.size(), 6U);
  EXPECT_EQ(std::count(merged.nodes.begin(), merged.nodes.end(), shared), 1);
  EXPECT_EQ(merged.external_inputs, (std::vector<OutDataAnchorPtr>{data->GetOutDataAnchor(0)}));
  ASSERT_EQ(merged.external_outputs.size(), 2U);
  for (const auto &boundary : merged.external_outputs) {
    ASSERT_EQ(boundary.consumers.size(), 1U);
    const auto index = boundary.consumers.front()->GetIdx();
    ASSERT_GE(index, 0);
    ASSERT_LT(index, 2);
    EXPECT_EQ(boundary.consumers.front(), output->GetInDataAnchor(index));
    EXPECT_EQ(boundary.source, (index == 0 ? last : right)->GetOutDataAnchor(0));
  }
}

/**
 * 用例描述：验证共用输入的独立分量可以合并，以及经过非候选节点的回入路径会阻止合并。
 * 预置条件：构造共用一个图输入的两条五节点 HostCPU 链。
 * 测试步骤：比较增加外部回入路径前后的融合计划。
 * 预期结果：独立链合为一个区域且仅保留一个输入。引入回入路径后，两条链各保留一个区域。
 */
TEST(HostCpuFusionRegionsST, KeepsComponentsSeparateWithExternalReentry) {
  const auto graph = std::make_shared<ComputeGraph>("st_host_independent_chains");
  const auto data = AddNode(graph, "data", "Data", 0U, 1U);
  const auto chains = AddIndependentChains(graph, data);
  const auto output = AddNode(graph, "output", "NetOutput", 2U, 0U);
  ASSERT_EQ(GraphUtils::AddEdge(chains[0].back()->GetOutDataAnchor(0), output->GetInDataAnchor(0)), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(chains[1].back()->GetOutDataAnchor(0), output->GetInDataAnchor(1)), GRAPH_SUCCESS);

  HostCpuFusionPass pass(nullptr, SupportsTestKernels);
  std::vector<std::vector<HostCpuFusionRegion>> components;
  ASSERT_EQ(pass.BuildFusionRegions(graph, components), SUCCESS);
  ASSERT_EQ(components.size(), 1U);
  ASSERT_EQ(components.front().size(), 1U);
  const auto &merged = components.front().front();
  auto expected_nodes = chains[0];
  expected_nodes.insert(expected_nodes.end(), chains[1].begin(), chains[1].end());
  EXPECT_EQ(merged.nodes, expected_nodes);
  EXPECT_EQ(merged.external_inputs, (std::vector<OutDataAnchorPtr>{data->GetOutDataAnchor(0)}));
  EXPECT_EQ(merged.external_outputs.size(), 2U);

  const auto bridge = AddNode(graph, "bridge", "Identity", 1U, 1U);
  const auto second_input = chains[1].front()->GetInDataAnchor(0);
  ASSERT_EQ(GraphUtils::RemoveEdge(data->GetOutDataAnchor(0), second_input), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(chains[0].back()->GetOutDataAnchor(0), bridge->GetInDataAnchor(0)), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(bridge->GetOutDataAnchor(0), second_input), GRAPH_SUCCESS);
  ASSERT_EQ(pass.BuildFusionRegions(graph, components), SUCCESS);
  ASSERT_EQ(components.size(), 2U);
  ASSERT_EQ(components[0].size(), 1U);
  ASSERT_EQ(components[1].size(), 1U);
  EXPECT_EQ(components[0][0].nodes, chains[0]);
  EXPECT_EQ(components[1][0].nodes, chains[1]);
  EXPECT_EQ(components[1][0].external_inputs, (std::vector<OutDataAnchorPtr>{bridge->GetOutDataAnchor(0)}));
}

/**
 * 用例描述：验证多个 HostCPU 区域由工作线程准备，全部准备成功后一起替换成融合算子。
 * 预置条件：构造两条 HostCPU 链，其中一条的下游设备算子禁止跨链合并。编译器返回预编译的测试 SO。
 * 测试步骤：先令其中一条链编译失败，再令两条链都编译成功并重跑融合。
 * 预期结果：失败时保留原图。成功时两个区域各替换为一个融合算子，消费边与 SO 按区域保留。
 */
TEST(HostCpuFusionRegionsST, CommitsBothRegionsAfterParallelPreparation) {
  const auto graph = std::make_shared<ComputeGraph>("st_host_parallel_preparation");
  const auto data = AddNode(graph, "data", "Data", 0U, 1U);
  const auto chains = AddIndependentChains(graph, data);
  const auto device = AddNode(graph, "device", "DeviceOp", 1U, 1U);
  device->GetOpDesc()->SetOpEngineName(kEngineNameAiCore);
  const auto output = AddNode(graph, "output", "NetOutput", 2U, 0U);
  ASSERT_EQ(GraphUtils::AddEdge(chains[0].back()->GetOutDataAnchor(0), device->GetInDataAnchor(0)), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(device->GetOutDataAnchor(0), output->GetInDataAnchor(0)), GRAPH_SUCCESS);
  ASSERT_EQ(GraphUtils::AddEdge(chains[1].back()->GetOutDataAnchor(0), output->GetInDataAnchor(1)), GRAPH_SUCCESS);

  std::vector<uint8_t> so_data;
  const auto status = HostCpuFusionCompiler().Compile(
      "extern \"C\" __attribute__((visibility(\"default\"))) unsigned int "
      "GetRegisteredCustomOpCreatorAbiVersion() { return 2U; }\n"
      "extern \"C\" __attribute__((visibility(\"default\"))) unsigned long "
      "GetRegisteredCustomOpCreatorNum() { return 0U; }\n"
      "extern \"C\" __attribute__((visibility(\"default\"))) int "
      "GetRegisteredCustomOpCreators(void *, unsigned long, unsigned long) { return 0; }\n",
      so_data);
  if (status == UNSUPPORTED) {
    GTEST_SKIP() << "HostCPU fusion JIT is unavailable in this Toolkit.";
  }
  ASSERT_EQ(status, SUCCESS);
  auto compiler = std::make_shared<RegionSoCompiler>(so_data);
  HostCpuFusionPass pass(compiler, SupportsTestKernels);
  NodeEngineMap atomic_map;
  NodeEngineMap composite_map;

  compiler->fail_second_chain = true;
  EXPECT_EQ(pass.Run(graph, atomic_map, composite_map), NOT_CHANGED);
  EXPECT_EQ(compiler->worker_calls.load(), 2U);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U * kNodesPerChain + 3U);
  EXPECT_TRUE(atomic_map.empty());
  EXPECT_TRUE(composite_map.empty());
  EXPECT_EQ((graph->GetExtAttr<std::map<std::string, OpSoBinPtr>>("bin_file_buffer")), nullptr);

  compiler->fail_second_chain = false;
  ASSERT_EQ(pass.Run(graph, atomic_map, composite_map), SUCCESS);
  EXPECT_EQ(compiler->worker_calls.load(), 4U);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5U);
  EXPECT_EQ(atomic_map.size(), 2U);
  EXPECT_EQ(composite_map.size(), 2U);
  const auto device_input = device->GetInDataAnchor(0)->GetPeerOutAnchor();
  const auto second_output = output->GetInDataAnchor(1)->GetPeerOutAnchor();
  ASSERT_NE(device_input, nullptr);
  ASSERT_NE(second_output, nullptr);
  const auto first_fused = device_input->GetOwnerNode();
  const auto second_fused = second_output->GetOwnerNode();
  EXPECT_NE(first_fused, second_fused);
  for (const auto &node : {first_fused, second_fused}) {
    EXPECT_EQ(node->GetInDataAnchor(0)->GetPeerOutAnchor(), data->GetOutDataAnchor(0));
    EXPECT_EQ(atomic_map.count(node), 1U);
    EXPECT_EQ(composite_map.count(node), 1U);
  }
  const auto so_buffer = graph->GetExtAttr<std::map<std::string, OpSoBinPtr>>("bin_file_buffer");
  ASSERT_NE(so_buffer, nullptr);
  EXPECT_EQ(so_buffer->size(), 2U);
}
}  // namespace ge
