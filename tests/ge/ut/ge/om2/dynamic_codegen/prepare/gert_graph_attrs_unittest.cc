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

#include <string>
#include <vector>

// ge_types.h 声明 gert::Tensor 回调类型却不自带该头，必须先引入运行时 Tensor 定义。
#include "exe_graph/runtime/tensor.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"
#include "runtime/model_desc.h"

namespace gert {
namespace {
ge::GeTensor MakeIoDesc(const ge::GeShape &shape, const ge::Format format, const ge::DataType dtype) {
  ge::GeTensorDesc desc(shape, format, dtype);
  // 模型 IO 描述只带 TensorDesc，载荷必须为空：解析不能依赖数据。
  return ge::GeTensor(desc);
}

std::vector<ge::GeTensor> StandardInputs() {
  return {MakeIoDesc(ge::GeShape({2, 3}), ge::FORMAT_ND, ge::DT_FLOAT),
          MakeIoDesc(ge::GeShape({4}), ge::FORMAT_ND, ge::DT_INT32)};
}

std::vector<ge::GeTensor> StandardOutputs() {
  return {MakeIoDesc(ge::GeShape({2, 3}), ge::FORMAT_ND, ge::DT_FLOAT)};
}

void SetModelAttrs(ge::ExecuteGraph &graph, const std::vector<ge::GeTensor> &inputs,
                   const std::vector<ge::GeTensor> &outputs, const std::vector<std::string> &input_names,
                   const std::vector<std::string> &output_names) {
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, gert::attr::kModelInputs, inputs));
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, gert::attr::kModelOutputs, outputs));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kModelInputNames, input_names));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kModelOutputNames, output_names));
}
}  // namespace

class GertGraphAttrsUT : public testing::Test {};

// 8.1：完整 IO 描述可解析，顺序与 TensorDesc 原样保留，资源槽缺省 1/0/0。
TEST_F(GertGraphAttrsUT, ParsesModelIoAndUsesDefaultResources) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});

  GertModelDescInfo info;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));

  ASSERT_EQ(info.inputs.size(), 2U);
  ASSERT_EQ(info.outputs.size(), 1U);
  EXPECT_EQ(info.inputs[0].name, "x1");
  EXPECT_EQ(info.inputs[1].name, "x2");
  EXPECT_EQ(info.outputs[0].name, "y");
  // 顺序按列表下标，不能被名称排序或 map 重排。
  EXPECT_EQ(info.inputs[0].desc.GetDataType(), ge::DT_FLOAT);
  EXPECT_EQ(info.inputs[1].desc.GetDataType(), ge::DT_INT32);
  EXPECT_EQ(info.inputs[1].desc.GetShape().GetDims(), std::vector<int64_t>({4}));
  EXPECT_EQ(info.resources.reusable_stream_num, 1);
  EXPECT_EQ(info.resources.reusable_event_num, 0);
  EXPECT_EQ(info.resources.reusable_notify_num, 0);
}

// 8.1：显式声明的资源槽数生效，用于多流模型。
TEST_F(GertGraphAttrsUT, HonorsDeclaredResourceCounts) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelStreamCount, 3));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelEventCount, 2));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelNotifyCount, 1));

  GertModelDescInfo info;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
  EXPECT_EQ(info.resources.reusable_stream_num, 3);
  EXPECT_EQ(info.resources.reusable_event_num, 2);
  EXPECT_EQ(info.resources.reusable_notify_num, 1);
}

// 8.1：零 IO 用空列表表达，不能被当作「未提供」。
TEST_F(GertGraphAttrsUT, AcceptsEmptyIoList) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, {}, {}, {}, {});

  GertModelDescInfo info;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
  EXPECT_TRUE(info.inputs.empty());
  EXPECT_TRUE(info.outputs.empty());
}

// 8.1：缺少描述属性必须报错，不能从其它信息推测 IO。
TEST_F(GertGraphAttrsUT, RejectsMissingModelIo) {
  ge::ExecuteGraph graph("model_attrs");
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, gert::attr::kModelInputs, StandardInputs()));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kModelInputNames, {"x1", "x2"}));

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
  EXPECT_TRUE(info.inputs.empty());
}

// 8.1：描述与名称列表长度不一致时拒绝。
TEST_F(GertGraphAttrsUT, RejectsNameCountMismatch) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"only_one"}, {"y"});

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：名称重复会让运行期绑定取到错误槽位，必须拒绝。
TEST_F(GertGraphAttrsUT, RejectsDuplicateName) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x", "x"}, {"y"});

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：空名称同样无法定位槽位。
TEST_F(GertGraphAttrsUT, RejectsEmptyName) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", ""}, {"y"});

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：负数资源槽数不可能满足，直接拒绝。
TEST_F(GertGraphAttrsUT, RejectsNegativeResourceCount) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelEventCount, -1));

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：stream 包含调用方主流，0 流无法下发 Init。
TEST_F(GertGraphAttrsUT, RejectsZeroStream) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelStreamCount, 0));

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：类型错误不能被当成数字，哪怕字符串内容看起来是整数。
TEST_F(GertGraphAttrsUT, RejectsNonIntegerCount) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, gert::attr::kModelEventCount, "2"));

  GertModelDescInfo info;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));
}

// 8.1：解析结果能直接序列化成 ModelDesc，且槽位与资源数目一致。
TEST_F(GertGraphAttrsUT, SerializesParsedInfoIntoModelDesc) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, StandardInputs(), StandardOutputs(), {"x1", "x2"}, {"y"});
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, gert::attr::kModelStreamCount, 2));

  GertModelDescInfo info;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));

  bg::BufferPool pool;
  bg::BufferPool::BufId model_desc_id = 0U;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildModelDescBuffer(info.inputs, info.outputs, info.resources, pool, model_desc_id));
  const auto *model_desc = reinterpret_cast<const ModelDesc *>(pool.GetBufById(model_desc_id));
  ASSERT_NE(model_desc, nullptr);

  ASSERT_EQ(model_desc->GetInputNum(), 2U);
  ASSERT_EQ(model_desc->GetOutputNum(), 1U);
  EXPECT_EQ(model_desc->GetReusableStreamNum(), 2U);
  EXPECT_EQ(model_desc->GetReusableEventNum(), 0U);

  const auto *input1 = model_desc->GetInputDesc(1U);
  ASSERT_NE(input1, nullptr);
  EXPECT_EQ(input1->GetDataType(), ge::DT_INT32);
  Shape expect_shape({4});
  EXPECT_EQ(input1->GetStorageShape(), expect_shape);
  // 名称存的是 buffer pool 内的偏移，消费方按同一 pool 反查。
  EXPECT_STREQ(pool.GetBufById(reinterpret_cast<size_t>(input1->GetName())), "x2");

  const auto *output0 = model_desc->GetOutputDesc(0U);
  ASSERT_NE(output0, nullptr);
  EXPECT_EQ(output0->GetDataType(), ge::DT_FLOAT);
  EXPECT_STREQ(pool.GetBufById(reinterpret_cast<size_t>(output0->GetName())), "y");
}

// 8.1：模型描述生成不依赖具体图结构或计算图 lowering，空图也能生成合法产物。
TEST_F(GertGraphAttrsUT, BuildsModelDescForEmptyIo) {
  ge::ExecuteGraph graph("model_attrs");
  SetModelAttrs(graph, {}, {}, {}, {});

  GertModelDescInfo info;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertModelDesc(graph, info));

  bg::BufferPool pool;
  bg::BufferPool::BufId model_desc_id = 0U;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildModelDescBuffer(info.inputs, info.outputs, info.resources, pool, model_desc_id));
  const auto *model_desc = reinterpret_cast<const ModelDesc *>(pool.GetBufById(model_desc_id));
  ASSERT_NE(model_desc, nullptr);
  EXPECT_EQ(model_desc->GetInputNum(), 0U);
  EXPECT_EQ(model_desc->GetOutputNum(), 0U);
  EXPECT_EQ(model_desc->GetReusableStreamNum(), 1U);
}
TEST_F(GertGraphAttrsUT, SerializesUnknownRankShapeRangesAndExplicitAippDimensions) {
  ModelIoEntry entry;
  entry.name = "dynamic_input";
  entry.desc = ge::GeTensorDesc(ge::GeShape({-2}), ge::FORMAT_ND, ge::DT_FLOAT);
  entry.desc.SetOriginShape(ge::GeShape({-2}));
  ASSERT_EQ(entry.desc.SetShapeRange({{1, 16}, {2, 32}}), ge::GRAPH_SUCCESS);
  ASSERT_EQ(entry.desc.SetOriginShapeRange({{3, 24}}), ge::GRAPH_SUCCESS);
  entry.has_aipp_dims = true;
  entry.aipp_dims = {1, 3, 224, 224};
  bg::BufferPool pool;
  bg::BufferPool::BufId id = 0U;
  ASSERT_EQ(BuildModelDescBuffer({entry}, {}, {}, pool, id), ge::GRAPH_SUCCESS);
  const auto *model = reinterpret_cast<const ModelDesc *>(pool.GetBufById(id));
  ASSERT_NE(model, nullptr);
  const auto *input = model->GetInputDesc(0U);
  ASSERT_NE(input, nullptr);
  EXPECT_EQ(input->GetStorageShape(), Shape({-2}));
  EXPECT_EQ(input->GetOriginShape(), Shape({-2}));
  EXPECT_EQ(input->GetStorageShapeRange().GetMin(), Shape({1, 2}));
  EXPECT_EQ(input->GetStorageShapeRange().GetMax(), Shape({16, 32}));
  EXPECT_EQ(input->GetOriginShapeRange().GetMin(), Shape({3}));
  EXPECT_EQ(input->GetOriginShapeRange().GetMax(), Shape({24}));
  EXPECT_EQ(input->GetAippShape(), Shape({1, 3, 224, 224}));
}
}  // namespace gert
