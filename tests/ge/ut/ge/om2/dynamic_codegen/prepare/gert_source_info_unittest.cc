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
#include <memory>
#include <string>
#include <vector>
#include "exe_graph/runtime/tensor.h"
#include "common/om2/dynamic_codegen/prepare/gert_source_info.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
std::string SourceField(const std::string &field) {
  return "gert.source.Op_1-a." + field;
}

void SetSource(ge::ExecuteGraph &graph) {
  const ge::GeTensorDesc desc(ge::GeShape({2, 3}), ge::FORMAT_ND, ge::DT_FLOAT);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kSourceIds, {"Op_1-a"}));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SourceField("name"), "source_op"));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SourceField("type"), "Add"));
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, SourceField("inputs"), {ge::GeTensor(desc)}));
  ASSERT_TRUE(ge::AttrUtils::SetListTensor(graph, SourceField("outputs"), {ge::GeTensor(desc)}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("ir_input_names"), {"x"}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("ir_output_names"), {"y"}));
  ASSERT_TRUE(ge::AttrUtils::SetListInt(graph, SourceField("ir_input_counts"), {1}));
  ASSERT_TRUE(ge::AttrUtils::SetListInt(graph, SourceField("ir_output_counts"), {1}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("attr_names"), {"axis"}));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, SourceField("attr.axis"), -2));
}

TEST(GertSourceInfoUT, PreservesDescriptionsAndRequiredOptionalDynamicInstances) {
  ge::ExecuteGraph graph("sources");
  ASSERT_NO_FATAL_FAILURE(SetSource(graph));
  for (const auto &kind : {"", "required", "optional", "dynamic"}) {
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("ir_input_kinds"), {kind}));
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("ir_output_kinds"), {kind}));
    std::vector<GertSourceInfo> infos;
    ASSERT_EQ(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
    ASSERT_EQ(infos.size(), 1U);
    EXPECT_EQ(infos[0].id, "Op_1-a");
    EXPECT_EQ(infos[0].name, "source_op");
    EXPECT_EQ(infos[0].type, "Add");
    ASSERT_EQ(infos[0].inputs.size(), 1U);
    ASSERT_EQ(infos[0].outputs.size(), 1U);
    EXPECT_EQ(infos[0].inputs[0].GetShape().GetDims(), (std::vector<int64_t>{2, 3}));
    EXPECT_EQ(infos[0].outputs[0].GetDataType(), ge::DT_FLOAT);
    EXPECT_EQ(infos[0].ir_input_names, (std::vector<std::string>{"x"}));
    EXPECT_EQ(infos[0].ir_output_counts, (std::vector<int64_t>{1}));
    EXPECT_EQ(infos[0].attr_names, (std::vector<std::string>{"axis"}));
    ASSERT_EQ(infos[0].attrs.size(), 1U);
    int64_t axis = 0;
    ASSERT_EQ(infos[0].attrs[0].GetValue(axis), ge::GRAPH_SUCCESS);
    EXPECT_EQ(axis, -2);
  }
}

TEST(GertSourceInfoUT, RejectsMissingAndWrongTypedFieldsWithoutReplacingOutput) {
  const std::vector<std::string> fields{"name",
                                        "type",
                                        "inputs",
                                        "outputs",
                                        "ir_input_names",
                                        "ir_output_names",
                                        "ir_input_counts",
                                        "ir_output_counts",
                                        "attr_names",
                                        "attr.axis"};
  for (const auto &field : fields) {
    for (const bool missing : {false, true}) {
      if (field == "attr.axis" && !missing) {
        continue;  // Source attributes intentionally allow arbitrary AnyValue types.
      }
      SCOPED_TRACE(field + (missing ? " missing" : " wrong type"));
      ge::ExecuteGraph graph("bad_field");
      ASSERT_NO_FATAL_FAILURE(SetSource(graph));
      ASSERT_EQ(graph.DelAttr(SourceField(field)), ge::GRAPH_SUCCESS);
      if (!missing) {
        ASSERT_TRUE(ge::AttrUtils::SetInt(graph, SourceField(field), 1));
      }
      std::vector<GertSourceInfo> infos(1U);
      infos[0].id = "unchanged";
      EXPECT_NE(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
      ASSERT_EQ(infos.size(), 1U);
      EXPECT_EQ(infos[0].id, "unchanged");
    }
  }
}

TEST(GertSourceInfoUT, RejectsInvalidIrCountsAndAttributeDeclarations) {
  struct InvalidIr {
    const char *kind;
    std::vector<int64_t> counts;
  };
  const InvalidIr cases[]{{"required", {0}}, {"optional", {2}}, {"dynamic", {-1}},   {"dynamic", {2}},
                          {"dynamic", {0}},  {"unknown", {1}},  {"required", {1, 1}}};
  for (const auto &item : cases) {
    for (const auto &side : {std::string("input"), std::string("output")}) {
      SCOPED_TRACE(side + " " + item.kind);
      ge::ExecuteGraph graph("bad_ir");
      ASSERT_NO_FATAL_FAILURE(SetSource(graph));
      ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("ir_" + side + "_kinds"), {item.kind}));
      ASSERT_TRUE(ge::AttrUtils::SetListInt(graph, SourceField("ir_" + side + "_counts"), item.counts));
      std::vector<GertSourceInfo> infos;
      EXPECT_NE(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
    }
  }
  for (const auto &names :
       {std::vector<std::string>{""}, std::vector<std::string>{"axis", "axis"}, std::vector<std::string>{"absent"}}) {
    ge::ExecuteGraph graph("bad_attributes");
    ASSERT_NO_FATAL_FAILURE(SetSource(graph));
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SourceField("attr_names"), names));
    std::vector<GertSourceInfo> infos;
    EXPECT_NE(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
  }
}

TEST(GertSourceInfoUT, HandlesEmptyTablesAndRejectsInvalidSourceIds) {
  ge::ExecuteGraph graph("source_ids");
  std::vector<GertSourceInfo> infos(1U);
  EXPECT_EQ(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(infos.empty());
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, attr::kSourceIds, 1));
  EXPECT_NE(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(SetSource(graph));
  ASSERT_EQ(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);  // Default required kinds.
  for (const auto &ids : {std::vector<std::string>{""}, std::vector<std::string>{"bad.id"},
                          std::vector<std::string>{"Op_1-a", "Op_1-a"}}) {
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kSourceIds, ids));
    EXPECT_NE(ParseGertSourceInfo(graph, infos), ge::GRAPH_SUCCESS);
  }
  auto *node = graph.AddNode(std::make_shared<ge::OpDesc>("kernel", "IdentityAddr"));
  ASSERT_NE(node, nullptr);
  std::string id;
  EXPECT_FALSE(GetNodeSourceId(*node, id));
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), "gert.source.id", ""));
  EXPECT_FALSE(GetNodeSourceId(*node, id));
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), "gert.source.id", "Op_1-a"));
  EXPECT_TRUE(GetNodeSourceId(*node, id));
  EXPECT_EQ(id, "Op_1-a");
}
}  // namespace
}  // namespace gert
