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
#include <cstring>

#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/continuous_buffer.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/gert_metadata_preparer.h"
#include "common/om2/dynamic_codegen/prepare/gert_compute_node_info_builder.h"

namespace gert {
namespace {
void SetEmptyIo(ge::ExecuteGraph &graph) {
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, attr::kModelInputs, std::vector<ge::GeTensor>{}));
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, attr::kModelOutputs, std::vector<ge::GeTensor>{}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kModelInputNames, {}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kModelOutputNames, {}));
}
}  // namespace

TEST(GertMetadataPreparerUT, RebuildsComputeInfoOnRepeatedPreparation) {
  ge::ExecuteGraph graph("metadata");
  SetEmptyIo(graph);
  auto *first = graph.AddNode(std::make_shared<ge::OpDesc>("first", "NoOp"));
  auto *second = graph.AddNode(std::make_shared<ge::OpDesc>("second", "NoOp"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  for (size_t i = 0U; i < 2U; ++i) {
    ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
    ge::Buffer bytes;
    ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kComputeNodeInfo, bytes));
    ASSERT_GE(bytes.size(), sizeof(ContinuousBuffer));
    EXPECT_EQ(reinterpret_cast<const ContinuousBuffer *>(bytes.data())->GetNum(), 2U);
    int64_t index = -1;
    ASSERT_TRUE(ge::AttrUtils::GetInt(second->GetOpDescBarePtr(), kComputeNodeIndex, index));
    EXPECT_EQ(index, 1);
  }
}

TEST(GertMetadataPreparerUT, RejectsExplicitEmptySourceId) {
  ge::ExecuteGraph graph("empty_source");
  SetEmptyIo(graph);
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("node", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescPtr(), attr::kSourceId, ""));
  EXPECT_NE(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
}

TEST(GertMetadataPreparerUT, PreservesSerializedMetadataAndBindsAddedHelper) {
  ge::ExecuteGraph graph("serialized_metadata");
  SetEmptyIo(graph);
  auto *first = graph.AddNode(std::make_shared<ge::OpDesc>("first", "NoOp"));
  auto *second = graph.AddNode(std::make_shared<ge::OpDesc>("second", "NoOp"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetInt(first->GetOpDescBarePtr(), kKernelExtendIndex, 0));
  ASSERT_TRUE(ge::AttrUtils::SetInt(second->GetOpDescBarePtr(), kKernelExtendIndex, 1));
  ge::Buffer original_compute_infos;
  ge::Buffer original_strings;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kComputeNodeInfo, original_compute_infos));
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kBuffer, original_strings));
  auto *helper = graph.AddNode(std::make_shared<ge::OpDesc>("helper", "NoOp"));
  ASSERT_NE(helper, nullptr);

  ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  ge::Buffer compute_infos;
  ge::Buffer strings;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kComputeNodeInfo, compute_infos));
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kBuffer, strings));
  ASSERT_EQ(compute_infos.size(), original_compute_infos.size());
  EXPECT_EQ(std::memcmp(compute_infos.data(), original_compute_infos.data(), compute_infos.size()), 0);
  ASSERT_EQ(strings.size(), original_strings.size());
  EXPECT_EQ(std::memcmp(strings.data(), original_strings.data(), strings.size()), 0);
  int64_t index = -1;
  ASSERT_TRUE(ge::AttrUtils::GetInt(helper->GetOpDescBarePtr(), kComputeNodeIndex, index));
  EXPECT_EQ(index, 1);
  ASSERT_TRUE(ge::AttrUtils::GetInt(helper->GetOpDescBarePtr(), kKernelExtendIndex, index));
  EXPECT_EQ(index, 1);
}

TEST(GertMetadataPreparerUT, RebuildsWhenSerializedComputePoolIsIncomplete) {
  ge::ExecuteGraph graph("incomplete_metadata");
  SetEmptyIo(graph);
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("node", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  const uint8_t invalid_pool = 0U;
  ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kComputeNodeInfo,
                                              ge::Buffer::CopyFrom(&invalid_pool, sizeof(invalid_pool))));
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  ge::Buffer compute_infos;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&graph, kComputeNodeInfo, compute_infos));
  ASSERT_GE(compute_infos.size(), sizeof(ContinuousBuffer));
  EXPECT_EQ(reinterpret_cast<const ContinuousBuffer *>(compute_infos.data())->GetNum(), 1U);
}
TEST(GertMetadataPreparerUT, BuildsSourceInstancesAndPreservesTensorDescriptorFlags) {
  ge::ExecuteGraph graph("source_instances");
  auto desc = std::make_shared<ge::OpDesc>("kernel", "NoOp");
  ge::GeTensorDesc tensor(ge::GeShape({2}), ge::FORMAT_ND, ge::DT_INT64);
  ASSERT_TRUE(ge::AttrUtils::SetInt(tensor, ge::ATTR_NAME_RESHAPE_TYPE_MASK, 1));
  ASSERT_TRUE(ge::AttrUtils::SetBool(tensor, ge::ATTR_NAME_IS_NULL_OUTPUT, true));
  ASSERT_EQ(desc->AddInputDesc(tensor), ge::GRAPH_SUCCESS);
  ASSERT_EQ(desc->AddOutputDesc(tensor), ge::GRAPH_SUCCESS);
  auto *node = graph.AddNode(desc);
  ASSERT_NE(node, nullptr);
  GertSourceInfo source;
  source.ir_input_names = {"dynamic", "optional"};
  source.ir_input_counts = {2, 0};
  source.ir_output_names = {"y"};
  source.ir_output_counts = {1};
  source.attr_names = {"axis"};
  source.attrs = {ge::AnyValue::CreateFrom<int64_t>(-2)};
  GertComputeNodeDef def;
  ASSERT_EQ(BuildComputeNodeDef(*node, &source, def), ge::GRAPH_SUCCESS);
  ASSERT_EQ(def.input_descs.size(), 2U);
  bg::BufferPool pool;
  std::unique_ptr<uint8_t[]> bytes;
  size_t size = 0U;
  ASSERT_EQ(BuildComputeNodeInfoBuffer(def, pool, bytes, size), ge::GRAPH_SUCCESS);
  const auto *info = reinterpret_cast<const ComputeNodeInfo *>(bytes.get());
  EXPECT_EQ(info->GetIrInputsNum(), 2U);
  EXPECT_EQ(info->GetInputsNum(), 2U);
  EXPECT_EQ(info->GetInputInstanceInfo(1U)->GetInstanceStart(), 2U);
  EXPECT_EQ(info->GetInputInstanceInfo(1U)->GetInstanceNum(), 0U);
  EXPECT_EQ(info->GetOutputInstanceInfo(0U)->GetInstanceNum(), 1U);
  EXPECT_EQ(info->GetInputTdInfo(0U)->GetExpandDimsType(), ExpandDimsType(1));
  EXPECT_EQ(info->GetOutputTdInfo(0U)->GetDataType(), ge::DT_INT64);
  EXPECT_FALSE(info->GetOutputTdInfo(0U)->IsExist());
  EXPECT_EQ(*info->GetAttrs()->GetInt(0U), -2);
}

TEST(GertMetadataPreparerUT, RejectsInconsistentSourceCountsAndUnsupportedAttributes) {
  ge::ExecuteGraph graph("invalid_source");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "NoOp"));
  ASSERT_NE(node, nullptr);
  GertSourceInfo source;
  source.attr_names = {"missing"};
  GertComputeNodeDef def;
  EXPECT_NE(BuildComputeNodeDef(*node, &source, def), ge::GRAPH_SUCCESS);
  source.attr_names.clear();
  ASSERT_EQ(BuildComputeNodeDef(*node, &source, def), ge::GRAPH_SUCCESS);
  bg::BufferPool pool;
  std::unique_ptr<uint8_t[]> bytes;
  size_t size = 0U;
  for (const bool input : {false, true}) {
    source.ir_input_names = input ? std::vector<std::string>{"x"} : std::vector<std::string>{};
    source.ir_output_names = input ? std::vector<std::string>{} : std::vector<std::string>{"y"};
    EXPECT_NE(BuildComputeNodeInfoBuffer(def, pool, bytes, size), ge::GRAPH_SUCCESS);
    EXPECT_EQ(bytes, nullptr);
  }
  source.ir_input_names.clear();
  source.ir_output_names.clear();
  def.attrs = {ge::AnyValue{}};
  EXPECT_NE(BuildComputeNodeInfoBuffer(def, pool, bytes, size), ge::GRAPH_SUCCESS);
  EXPECT_EQ(bytes, nullptr);
}

TEST(GertMetadataPreparerUT, ResolvesDeclaredSourcesAndRejectsDanglingNodeReferences) {
  ge::ExecuteGraph graph("source_reference");
  ASSERT_NO_FATAL_FAILURE(SetEmptyIo(graph));
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kSourceId, "op"));
  EXPECT_NE(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kSourceIds, {"op"}));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, "gert.source.op.name", "source"));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, "gert.source.op.type", "Identity"));
  for (const auto *side : {"inputs", "outputs"}) {
    ASSERT_TRUE(
        ge::AttrUtils::SetListTensor(graph, std::string("gert.source.op.") + side, std::vector<ge::GeTensor>{}));
  }
  for (const auto *field : {"ir_input_names", "ir_output_names", "attr_names"}) {
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, std::string("gert.source.op.") + field, {}));
  }
  for (const auto *field : {"ir_input_counts", "ir_output_counts"}) {
    ASSERT_TRUE(ge::AttrUtils::SetListInt(graph, std::string("gert.source.op.") + field, {}));
  }
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph), ge::GRAPH_SUCCESS);
  int64_t index = -1;
  ASSERT_TRUE(ge::AttrUtils::GetInt(node->GetOpDescBarePtr(), kComputeNodeIndex, index));
  EXPECT_EQ(index, 0);
  ge::Buffer bytes;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(graph, kComputeNodeInfo, bytes));
  const auto *pool = reinterpret_cast<const ContinuousBuffer *>(bytes.data());
  ASSERT_EQ(pool->GetNum(), 1U);
  EXPECT_EQ(pool->Get<ComputeNodeInfo>(0U)->GetInputsNum(), 0U);
}
}  // namespace gert
