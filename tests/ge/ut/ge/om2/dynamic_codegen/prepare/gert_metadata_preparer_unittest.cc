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
#include <cstring>
#include <limits>

#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/lowering/bg_kernel_context_extend.h"
#include "exe_graph/lowering/bg_ir_attrs.h"
#include "exe_graph/runtime/continuous_buffer.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/compute_graph.h"
#include "graph/utils/graph_utils.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/compute_node_info_validation.h"
#include "graph_metadef/base/attr/attrs_to_buffer.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/gert_metadata_preparer.h"
#include "common/om2/dynamic_codegen/prepare/gert_source_op_json.h"

namespace gert {
namespace {
constexpr const char *kEmptySource = R"({
  "version":1,"name":"same_source","type":"MatMul","inputs":[],"outputs":[],
  "ir_inputs":[],"ir_outputs":[],"attrs":[]
})";
}  // namespace

TEST(GertMetadataPreparerUT, NoJsonDiscardsExternalIndicesAndPools) {
  ge::ExecuteGraph graph("metadata");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("node", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kSourceOp, kEmptySource));
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
  ASSERT_EQ(node->GetOpDescBarePtr()->DelAttr(attr::kSourceOp), ge::GRAPH_SUCCESS);
  for (size_t i = 0U; i < 2U; ++i) {
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), kComputeNodeIndex, i));
    ASSERT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
    ge::Buffer bytes;
    ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(graph, kComputeNodeInfo, bytes));
    ASSERT_GE(bytes.size(), sizeof(ContinuousBuffer));
    EXPECT_EQ(reinterpret_cast<const ContinuousBuffer *>(bytes.data())->GetNum(), 0U);
    EXPECT_FALSE(node->GetOpDescBarePtr()->HasAttr(kComputeNodeIndex));
    EXPECT_TRUE(node->GetOpDescBarePtr()->HasAttr(kKernelExtendIndex));
  }
}

void CheckJsonMetadata(ge::ExecuteGraph &graph, const ge::OpDescPtr &desc, ge::FastNode *legacy) {
  std::vector<uint8_t> previous_compute;
  for (size_t i = 0U; i < 2U; ++i) {
    ASSERT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
    ge::Buffer compute;
    ge::Buffer strings;
    ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(graph, kComputeNodeInfo, compute));
    ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(graph, kBuffer, strings));
    const auto *pool = reinterpret_cast<const ContinuousBuffer *>(compute.data());
    const auto *text = reinterpret_cast<const ContinuousBuffer *>(strings.data());
    int64_t index = -1;
    ASSERT_TRUE(ge::AttrUtils::GetInt(desc, kComputeNodeIndex, index));
    const auto *info = pool->Get<ComputeNodeInfo>(static_cast<size_t>(index));
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(text->Get<char>(reinterpret_cast<size_t>(info->GetNodeName())), "source");
    EXPECT_EQ(info->GetInputsNum(), 2U);
    EXPECT_EQ(info->GetOutputsNum(), 2U);
    EXPECT_EQ(info->GetIrOutputsNum(), 0U);
    EXPECT_EQ(info->GetInputInstanceInfo(0)->GetInstanceStart(), 1U);
    EXPECT_EQ(info->GetInputInstanceInfo(2)->GetInstanceNum(), 0U);
    EXPECT_EQ(info->GetOutputTdInfo(0)->GetDataType(), ge::DT_FLOAT);
    EXPECT_EQ(info->GetOutputTdInfo(1)->GetDataType(), ge::DT_INT64);
    EXPECT_FALSE(info->GetOutputTdInfo(1)->IsExist());
    EXPECT_EQ(*info->GetAttrs()->GetInt(0), -2);
    EXPECT_FALSE(legacy->GetOpDescBarePtr()->HasAttr(kComputeNodeIndex));
    EXPECT_EQ(pool->GetNum(), 1U);
    const std::vector<uint8_t> current(compute.data(), compute.data() + compute.size());
    if (i != 0U) {
      EXPECT_EQ(current, previous_compute);
    }
    previous_compute = current;
  }
}

void CheckJsonAttrs(const ComputeNodeInfo *info, bool is_first) {
  const auto *runtime = info->GetAttrs();
  ASSERT_NE(runtime->GetInt(0), nullptr);
  EXPECT_EQ(*runtime->GetInt(0), is_first ? -7 : 19);
  ASSERT_NE(runtime->GetListInt(1), nullptr);
  EXPECT_EQ(runtime->GetListInt(1)->GetSize(), 0U);
  ASSERT_NE(runtime->GetFloat(2), nullptr);
  uint32_t bits = 0U;
  std::copy_n(reinterpret_cast<const uint8_t *>(runtime->GetFloat(2)), sizeof(bits),
              reinterpret_cast<uint8_t *>(&bits));
  EXPECT_EQ(bits, 0x7FC00001U);
  const auto *bytes = runtime->GetTensor(3);
  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->GetDataType(), ge::DT_UINT8);
  EXPECT_EQ(bytes->GetStorageShape().GetDim(0), 3);
  EXPECT_EQ(std::memcmp(bytes->GetData<uint8_t>(), "\x01\x02\x03", 3U), 0);
  const auto *strings = runtime->GetTensor(4);
  ASSERT_NE(strings, nullptr);
  EXPECT_EQ(strings->GetDataType(), ge::DT_STRING);
  EXPECT_EQ(strings->GetStorageShape().GetDim(0), 2);
  // Two StringHead entries store offsets and lengths before the NUL-terminated bytes.
  int64_t headers[4]{};
  const auto *payload = strings->GetData<uint8_t>();
  std::copy_n(payload, sizeof(headers), reinterpret_cast<uint8_t *>(headers));
  EXPECT_EQ(headers[0], sizeof(headers));
  EXPECT_EQ(headers[1], 0);
  EXPECT_EQ(headers[2], sizeof(headers) + 1U);
  EXPECT_EQ(headers[3], 3);
  EXPECT_EQ(payload[sizeof(headers) + 1U], 'a');
  EXPECT_EQ(payload[sizeof(headers) + 2U], 'b');
  EXPECT_EQ(payload[sizeof(headers) + 3U], 'c');
  EXPECT_EQ(payload[sizeof(headers) + 4U], 0U);
}

void CompareJsonInfo(const ComputeNodeInfo *lowered, const ComputeNodeInfo *imported, size_t json_size,
                     size_t lowering_size) {
  EXPECT_EQ(json_size, lowering_size);
  EXPECT_EQ(lowered->GetNodeName(), imported->GetNodeName());
  EXPECT_EQ(lowered->GetNodeType(), imported->GetNodeType());
  EXPECT_EQ(lowered->GetInputsNum(), imported->GetInputsNum());
  EXPECT_EQ(lowered->GetOutputsNum(), imported->GetOutputsNum());
  ASSERT_EQ(lowered->GetIrInputsNum(), imported->GetIrInputsNum());
  ASSERT_EQ(lowered->GetIrOutputsNum(), imported->GetIrOutputsNum());
  EXPECT_EQ(lowered->GetInputInstanceInfo(0)->GetInstanceStart(),
            imported->GetInputInstanceInfo(0)->GetInstanceStart());
  EXPECT_EQ(lowered->GetInputInstanceInfo(0)->GetInstanceNum(), imported->GetInputInstanceInfo(0)->GetInstanceNum());
  EXPECT_EQ(lowered->GetOutputInstanceInfo(0)->GetInstanceStart(),
            imported->GetOutputInstanceInfo(0)->GetInstanceStart());
  EXPECT_EQ(lowered->GetOutputInstanceInfo(0)->GetInstanceNum(), imported->GetOutputInstanceInfo(0)->GetInstanceNum());
  EXPECT_EQ(lowered->GetInputTdInfo(0)->GetDataType(), imported->GetInputTdInfo(0)->GetDataType());
  EXPECT_EQ(lowered->GetOutputTdInfo(0)->GetStorageFormat(), imported->GetOutputTdInfo(0)->GetStorageFormat());
  EXPECT_EQ(*lowered->GetAttrs()->GetInt(0), *imported->GetAttrs()->GetInt(0));
}

void CheckJsonPort(const ComputeNodeInfo *info) {
  const auto *port = info->GetInputTdInfo(0);
  ASSERT_NE(port, nullptr);
  EXPECT_EQ(port->GetOriginFormat(), ge::FORMAT_NCHW);
  EXPECT_EQ(port->GetStorageFormat(), ge::FORMAT_ND);
  EXPECT_TRUE(port->IsExist());
  const auto expand = port->GetExpandDimsType();
  EXPECT_EQ(expand.GetFullSize(), 3);
  EXPECT_FALSE(expand.IsExpandIndex(0));
  EXPECT_TRUE(expand.IsExpandIndex(1));
  EXPECT_FALSE(expand.IsExpandIndex(2));
}

TEST(GertMetadataPreparerUT, JsonOverridesSerializedMetadataAndUsesSourcePorts) {
  ge::ExecuteGraph graph("source_json");
  const auto desc = std::make_shared<ge::OpDesc>("tiling", "NoOp");
  auto *node = graph.AddNode(desc);
  auto *legacy = graph.AddNode(std::make_shared<ge::OpDesc>("legacy", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_NE(legacy, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(legacy->GetOpDescBarePtr(), attr::kSourceOp, R"({
    "version":1,"name":"legacy","type":"Add","inputs":[],"outputs":[],
    "ir_inputs":[],"ir_outputs":[],"attrs":[{"name":"axis","type":"int64","value":31}]
  })"));
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
  ASSERT_EQ(legacy->GetOpDescBarePtr()->DelAttr(attr::kSourceOp), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kSourceOp, ""));
  EXPECT_NE(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kSourceOp, R"({
    "version":1,"name":"source","type":"MatMul",
    "inputs":[{"dtype":"DT_FLOAT16","origin_format":2,"storage_format":2},
              {"dtype":"DT_FLOAT16","origin_format":2,"storage_format":2}],
    "outputs":[{"dtype":"DT_FLOAT","origin_format":2,"storage_format":2},
               {"dtype":"DT_INT64","origin_format":2,"storage_format":2,"exists":false}],
    "ir_inputs":[{"start":1,"count":1},{"start":0,"count":1},{"start":2,"count":0}],
    "ir_outputs":[],"attrs":[{"name":"axis","type":"int64","value":-2}]
  })"));
  ASSERT_NO_FATAL_FAILURE(CheckJsonMetadata(graph, desc, legacy));
}
TEST(GertMetadataPreparerUT, JsonPreservesOrderedRuntimeAttrsForSameSourceName) {
  ge::ExecuteGraph graph("json_attrs");
  const auto first = std::make_shared<ge::OpDesc>("first", "NoOp");
  const auto second = std::make_shared<ge::OpDesc>("second", "NoOp");
  ASSERT_NE(graph.AddNode(first), nullptr);
  ASSERT_NE(graph.AddNode(second), nullptr);
  const std::string attrs = R"({
    "version":1,"name":"same_source","type":"MatMul","inputs":[],"outputs":[],
    "ir_inputs":[],"ir_outputs":[],"attrs":[
      {"name":"axis","type":"int64","value":-7},
      {"name":"empty","type":"list<int64>","value":[]},
      {"name":"special","type":"float32","value":"0x7FC00001"},
      {"name":"bytes","type":"tensor","value":{"dtype":"DT_UINT8","origin_shape":[3],"storage_shape":[3],
          "origin_format":2,"storage_format":2,"data_base64":"AQID"}},
      {"name":"strings","type":"tensor","value":{"dtype":"DT_STRING","origin_shape":[2],"storage_shape":[2],
          "origin_format":2,"storage_format":2,"strings":["","abc"]}}
    ]
  })";
  ASSERT_TRUE(ge::AttrUtils::SetStr(first, attr::kSourceOp, attrs));
  auto different = attrs;
  different.replace(different.find("-7"), 2U, "19");
  ASSERT_TRUE(ge::AttrUtils::SetStr(second, attr::kSourceOp, different));
  ASSERT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
  ge::Buffer storage;
  ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(graph, kComputeNodeInfo, storage));
  const auto *pool = reinterpret_cast<const ContinuousBuffer *>(storage.data());
  ASSERT_EQ(pool->GetNum(), 2U);
  for (const auto &desc : {first, second}) {
    int64_t index = -1;
    ASSERT_TRUE(ge::AttrUtils::GetInt(desc, kComputeNodeIndex, index));
    const auto *info = pool->Get<ComputeNodeInfo>(static_cast<size_t>(index));
    ASSERT_NE(info, nullptr);
    ASSERT_NO_FATAL_FAILURE(CheckJsonAttrs(info, desc == first));
  }
}

TEST(GertMetadataPreparerUT, JsonMatchesGeLoweringForEquivalentSource) {
  auto graph = std::make_shared<ge::ComputeGraph>("source");
  const ge::GeTensorDesc port(ge::GeShape({2}), ge::FORMAT_ND, ge::DT_INT64);
  auto input_desc = std::make_shared<ge::OpDesc>("input", "Data");
  ASSERT_EQ(input_desc->AddOutputDesc("y", port), ge::GRAPH_SUCCESS);
  auto input = graph->AddNode(input_desc);
  ASSERT_NE(input, nullptr);
  auto source = std::make_shared<ge::OpDesc>("matmul", "MatMul");
  ASSERT_EQ(source->AddInputDesc("x", port), ge::GRAPH_SUCCESS);
  ASSERT_EQ(source->AddOutputDesc("y", port), ge::GRAPH_SUCCESS);
  source->AppendIrInput("x", ge::kIrInputRequired);
  source->AppendIrOutput("y", ge::kIrOutputRequired);
  source->AppendIrAttrName("axis");
  ASSERT_TRUE(ge::AttrUtils::SetInt(source, "axis", -7));
  auto original = graph->AddNode(source);
  ASSERT_NE(original, nullptr);
  ASSERT_EQ(ge::GraphUtils::AddEdge(input->GetOutDataAnchor(0), original->GetInDataAnchor(0)), ge::GRAPH_SUCCESS);

  bg::BufferPool pool;
  size_t lowering_size = 0U;
  auto lowering = bg::CreateComputeNodeInfo(original, pool, lowering_size);
  ASSERT_NE(lowering, nullptr);
  ge::ExecuteGraph execute("kernel_graph");
  auto *kernel = execute.AddNode(std::make_shared<ge::OpDesc>("tiling", "NoOp"));
  ASSERT_NE(kernel, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(kernel->GetOpDescBarePtr(), attr::kSourceOp, R"({
    "version":1,"name":"matmul","type":"MatMul",
    "inputs":[{"dtype":"DT_INT64","origin_format":2,"storage_format":2}],
    "outputs":[{"dtype":"DT_INT64","origin_format":2,"storage_format":2}],
    "ir_inputs":[{"start":0,"count":1}],"ir_outputs":[{"start":0,"count":1}],
    "attrs":[{"name":"axis","type":"int64","value":-7}]
  })"));
  std::unique_ptr<uint8_t[]> json;
  size_t json_size = 0U;
  ASSERT_EQ(BuildComputeNodeInfoFromSourceOp(*kernel, pool, json, json_size), ge::GRAPH_SUCCESS);
  const auto *lowered = reinterpret_cast<const ComputeNodeInfo *>(lowering.get());
  const auto *imported = reinterpret_cast<const ComputeNodeInfo *>(json.get());
  ASSERT_NO_FATAL_FAILURE(CompareJsonInfo(lowered, imported, json_size, lowering_size));
}

TEST(GertMetadataPreparerUT, JsonEncodesScalarAndListAttributesLikeNativeValues) {
  ge::ExecuteGraph graph("native_attr_comparison");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kSourceOp, R"({
    "version":1,"name":"op","type":"Example",
    "inputs":[{"dtype":"DT_INT32","origin_format":0,"storage_format":2,"expand_dims":"010"}],
    "outputs":[],"ir_inputs":[],"ir_outputs":[],"attrs":[
      {"name":"enabled","type":"bool","value":true},
      {"name":"label","type":"string","value":{"base64":"R0U="}},
      {"name":"dtype","type":"dtype","value":"DT_UINT8"},
      {"name":"flags","type":"list<bool>","value":[false,true]},
      {"name":"weights","type":"list<float32>","value":[-1.5,"0x40000000"]},
      {"name":"labels","type":"list<string>","value":["","two"]},
      {"name":"types","type":"list<dtype>","value":["DT_FLOAT","DT_INT32"]},
      {"name":"axes","type":"list<list<int64>>","value":[[],[3,-2]]},
      {"name":"ranges","type":"list<list<float32>>","value":[[1.25],[]]}
    ]
  })"));
  bg::BufferPool pool;
  std::unique_ptr<uint8_t[]> record;
  size_t size = 0U;
  ASSERT_EQ(BuildComputeNodeInfoFromSourceOp(*node, pool, record, size), ge::GRAPH_SUCCESS);
  const auto *info = reinterpret_cast<const ComputeNodeInfo *>(record.get());
  ASSERT_NO_FATAL_FAILURE(CheckJsonPort(info));

  using ge::AnyValue;
  const std::vector<AnyValue> expected{AnyValue::CreateFrom<bool>(true),
                                       AnyValue::CreateFrom<std::string>("GE"),
                                       AnyValue::CreateFrom<ge::DataType>(ge::DT_UINT8),
                                       AnyValue::CreateFrom<std::vector<bool>>({false, true}),
                                       AnyValue::CreateFrom<std::vector<float>>({-1.5F, 2.F}),
                                       AnyValue::CreateFrom<std::vector<std::string>>({"", "two"}),
                                       AnyValue::CreateFrom<std::vector<ge::DataType>>({ge::DT_FLOAT, ge::DT_INT32}),
                                       AnyValue::CreateFrom<std::vector<std::vector<int64_t>>>({{}, {3, -2}}),
                                       AnyValue::CreateFrom<std::vector<std::vector<float>>>({{1.25F}, {}})};
  size_t native_size = 0U;
  const auto native = bg::CreateAttrBufferWithAttrs(expected, native_size);
  ASSERT_NE(native, nullptr);
  size_t attr_size = 0U;
  ASSERT_TRUE(ValidateComputeNodeInfo(*info, size, attr_size));
  ASSERT_EQ(attr_size, native_size);
  ASSERT_EQ(info->GetAttrs()->GetAttrNum(), expected.size());
  EXPECT_EQ(std::memcmp(info->GetAttrs(), native.get(), native_size), 0);
}

TEST(GertMetadataPreparerUT, JsonScalarTensorUsesValidatedSizeAndNativeRetainsShapeDerivedSize) {
  const uint8_t byte = 42U;
  const ge::GeTensorDesc desc(ge::GeShape(), ge::FORMAT_ND, ge::DT_UINT8);
  const std::vector<ge::AnyValue> native_attrs{
      ge::AnyValue::CreateFrom<ge::GeTensor>(ge::GeTensor(desc, &byte, sizeof(byte)))};
  size_t native_size = 0U;
  const auto native = bg::CreateAttrBufferWithoutIr(nullptr, native_attrs, native_size);
  ASSERT_NE(native, nullptr);
  const auto *native_tensor = reinterpret_cast<const RuntimeAttrs *>(native.get())->GetTensor(0U);
  ASSERT_NE(native_tensor, nullptr);
  EXPECT_EQ(native_tensor->GetStorageShape().GetDimNum(), 0U);
  EXPECT_EQ(native_tensor->GetSize(), 0U);

  ge::ExecuteGraph graph("scalar_tensor_attr");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kSourceOp, R"({
    "version":1,"name":"op","type":"Example","inputs":[],"outputs":[],"ir_inputs":[],"ir_outputs":[],
    "attrs":[{"name":"scalar","type":"tensor","value":{"dtype":"DT_UINT8",
      "origin_shape":[],"storage_shape":[],"origin_format":2,"storage_format":2,"data_base64":"Kg=="}}]
  })"));
  bg::BufferPool pool;
  std::unique_ptr<uint8_t[]> record;
  size_t size = 0U;
  ASSERT_EQ(BuildComputeNodeInfoFromSourceOp(*node, pool, record, size), ge::GRAPH_SUCCESS);
  const auto *imported = reinterpret_cast<const ComputeNodeInfo *>(record.get())->GetAttrs()->GetTensor(0U);
  ASSERT_NE(imported, nullptr);
  EXPECT_EQ(imported->GetStorageShape().GetDimNum(), 0U);
  ASSERT_EQ(imported->GetSize(), sizeof(byte));
  EXPECT_EQ(*imported->GetData<uint8_t>(), byte);
}

TEST(GertMetadataPreparerUT, ValidatesSerializedRecordLengthAndRuntimeAttrOffsetTable) {
  bg::BufferPool pool;
  ge::ExecuteGraph graph("validation");
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "NoOp"));
  ASSERT_NE(node, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), attr::kSourceOp, R"({
    "version":1,"name":"op","type":"Example","inputs":[],"outputs":[],"ir_inputs":[],"ir_outputs":[],
    "attrs":[{"name":"a","type":"int64","value":1},{"name":"b","type":"int64","value":2}]
  })"));
  size_t size = 0U;
  std::unique_ptr<uint8_t[]> record;
  ASSERT_EQ(BuildComputeNodeInfoFromSourceOp(*node, pool, record, size), ge::GRAPH_SUCCESS);
  auto *info = reinterpret_cast<ComputeNodeInfo *>(record.get());
  size_t attrs_size = 0U;
  ASSERT_TRUE(ValidateComputeNodeInfo(*info, size, attrs_size));
  size_t actual = 0U;
  EXPECT_FALSE(ValidateComputeNodeInfo(*info, sizeof(ComputeNodeInfo) - 1U, actual));
  EXPECT_FALSE(ValidateComputeNodeInfo(*info, size - 1U, actual));
  EXPECT_FALSE(ValidateRuntimeAttrs(nullptr, attrs_size));
  auto *attrs = reinterpret_cast<uint8_t *>(info->MutableAttrs());
  EXPECT_FALSE(ValidateRuntimeAttrs(attrs, sizeof(RuntimeAttrsDef) - 1U));
  const auto original = std::vector<uint8_t>(attrs, attrs + attrs_size);
  const size_t first_offset = offsetof(RuntimeAttrsDef, offset);
  const size_t payload_start = sizeof(RuntimeAttrsDef) + 2U * sizeof(size_t);
  for (const auto &corrupt :
       std::vector<std::pair<size_t, size_t>>{{offsetof(RuntimeAttrsDef, attr_num), std::numeric_limits<size_t>::max()},
                                              {offsetof(RuntimeAttrsDef, attr_num), attrs_size},
                                              {first_offset, payload_start - 1U},
                                              {first_offset, attrs_size},
                                              {first_offset, payload_start + sizeof(int64_t) + 1U}}) {
    std::copy(original.begin(), original.end(), attrs);
    std::copy_n(reinterpret_cast<const uint8_t *>(&corrupt.second), sizeof(size_t), attrs + corrupt.first);
    EXPECT_FALSE(ValidateComputeNodeInfo(*info, size, actual)) << corrupt.first << ", " << corrupt.second;
  }
}

TEST(GertMetadataPreparerUT, RejectsInvalidSourceOpFields) {
  ge::ExecuteGraph graph("invalid_source_op");
  const auto desc = std::make_shared<ge::OpDesc>("tiling_0", "NoOp");
  ASSERT_NE(graph.AddNode(desc), nullptr);
  const auto check = [&](const std::string &field, const std::string &replacement) {
    auto value = std::string(kEmptySource);
    const auto offset = value.find(field);
    ASSERT_NE(offset, std::string::npos);
    value.replace(offset, field.size(), replacement);
    ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kSourceOp, value));
    EXPECT_NE(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS) << value;
    EXPECT_FALSE(graph.HasAttr(kComputeNodeInfo));
  };
  check(R"("version":1)", R"("version":2)");
  check(R"("attrs":[])", R"("unknown":true,"attrs":[])");
  check(R"("ir_inputs":[])", R"("ir_inputs":[{"start":0,"count":1}])");
  check(R"("attrs":[])", R"("attrs":[{"name":"t","type":"list<tensor>","value":[]}])");
  check(R"("attrs":[])", R"("attrs":[{"name":"t","type":"tensor","value":{
      "dtype":"DT_UINT8","origin_shape":[2],"storage_shape":[2],"origin_format":2,"storage_format":2,
      "data_base64":"AQ=="}}])");
  check(R"("name":"same_source")", R"("name":"invalid\u0000name")");
  check(R"("type":"MatMul",)", "");
  check(R"("version":1)", R"("version":1.0)");
  check(R"("outputs":[])", R"("outputs":{})");
  check(R"("ir_outputs":[])", R"("ir_outputs":[{"start":1,"count":0}])");
  check(R"("ir_outputs":[])", R"("ir_outputs":[{"start":-1,"count":0}])");
  for (const auto &port : {R"({"dtype":"INT64","origin_format":2,"storage_format":2})",
                           R"({"dtype":"DT_INT64","origin_format":268435456,"storage_format":2})",
                           R"({"dtype":"DT_FLOAT","origin_format":2,"storage_format":2,"exists":1})",
                           R"({"dtype":"DT_FLOAT","origin_format":2,"storage_format":2,"expand_dims":"12"})"}) {
    check(R"("inputs":[])", std::string(R"("inputs":[)") + port + "]");
  }
  for (const auto &attribute :
       {R"({"name":"","type":"bool","value":true})", R"({"name":"i","type":"int64","value":9223372036854775808})",
        R"({"name":"f","type":"float32","value":1e39})", R"({"name":"f","type":"float32","value":"0x1000000X"})",
        R"({"name":"s","type":"string","value":{"base64":"AA=="}})",
        R"({"name":"s","type":"string","value":{"base64":"!"}})",
        R"({"name":"nested","type":"list<list<float32>>","value":[2.5]})", R"({"name":"t","type":"tensor","value":{}})",
        R"({"name":"t","type":"tensor","value":{"dtype":"DT_UINT8","origin_shape":[-1],"storage_shape":[1],
               "origin_format":2,"storage_format":2,"data_base64":"AQ=="}})",
        R"({"name":"t","type":"tensor","value":{"dtype":"DT_UINT8","origin_shape":[1],
               "storage_shape":[9223372036854775807,2],"origin_format":2,"storage_format":2,"data_base64":""}})",
        R"({"name":"t","type":"tensor","value":{"dtype":"DT_STRING","origin_shape":[1],"storage_shape":[1],
               "origin_format":2,"storage_format":2,"strings":[]}})"}) {
    check(R"("attrs":[])", std::string(R"("attrs":[)") + attribute + "]");
  }
  ASSERT_TRUE(ge::AttrUtils::SetInt(desc, attr::kSourceOp, 1));
  EXPECT_NE(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, attr::kSourceOp, kEmptySource));
  EXPECT_EQ(PrepareExecuteGraphMetadata(graph, {}), ge::GRAPH_SUCCESS);
}
}  // namespace gert
