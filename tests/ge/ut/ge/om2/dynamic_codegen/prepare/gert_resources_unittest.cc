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
#include "common/om2/dynamic_codegen/prepare/gert_resources.h"

namespace gert {
namespace {
std::string BinaryKey(const std::string &id, const char *field) {
  return std::string(attr::kBinaryPrefix) + id + "." + field;
}

ge::GeTensor MakeBytes(const std::vector<uint8_t> &bytes) {
  ge::GeTensorDesc desc(ge::GeShape({static_cast<int64_t>(bytes.size())}), ge::FORMAT_ND, ge::DT_UINT8);
  return ge::GeTensor(desc, bytes);
}

void SetBinary(ge::ExecuteGraph &graph, const std::string &id, const std::string &name,
               const std::vector<uint8_t> &content) {
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, BinaryKey(id, "name"), name));
  ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, BinaryKey(id, "content"), MakeBytes(content)));
  std::vector<std::string> ids;
  (void)ge::AttrUtils::GetListStr(graph, gert::attr::kBinaryIds, ids);
  ids.push_back(id);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kBinaryIds, ids));
}

}  // namespace

class GertBinariesUT : public testing::Test {};

TEST_F(GertBinariesUT, AcceptsMissingBinaryList) {
  ge::ExecuteGraph graph("empty");
  std::vector<GertBinaryImage> images;
  ASSERT_EQ(ParseGertBinaries(graph, images), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(images.empty());
}

TEST_F(GertBinariesUT, PreservesBinaryOrderAndDeduplicatesFiles) {
  ge::ExecuteGraph graph("binaries");
  ASSERT_NO_FATAL_FAILURE(SetBinary(graph, "second", "z.o", {1U, 0U, 2U}));
  ASSERT_NO_FATAL_FAILURE(SetBinary(graph, "first", "a.o", {3U}));
  ASSERT_NO_FATAL_FAILURE(SetBinary(graph, "alias", "z.o", {1U, 0U, 2U}));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kBinaryIds, {"second", "second", "first", "alias"}));
  std::vector<GertBinaryImage> images;
  ASSERT_EQ(ParseGertBinaries(graph, images), ge::GRAPH_SUCCESS);
  ASSERT_EQ(images.size(), 2U);
  EXPECT_EQ(images[0].name, "z.o");
  EXPECT_EQ(images[0].content, (std::vector<uint8_t>{1U, 0U, 2U}));
  EXPECT_EQ(images[1].name, "a.o");
  EXPECT_FALSE(images[0].key.empty());
  EXPECT_NE(images[0].key, images[1].key);
}

TEST_F(GertBinariesUT, RejectsConflictingBinaryContentsAndPreservesOutput) {
  ge::ExecuteGraph graph("conflicting");
  ASSERT_NO_FATAL_FAILURE(SetBinary(graph, "first", "same.o", {1U}));
  std::vector<GertBinaryImage> images;
  ASSERT_EQ(ParseGertBinaries(graph, images), ge::GRAPH_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(SetBinary(graph, "second", "same.o", {2U}));
  EXPECT_NE(ParseGertBinaries(graph, images), ge::GRAPH_SUCCESS);
  ASSERT_EQ(images.size(), 1U);
  EXPECT_EQ(images[0].content, (std::vector<uint8_t>{1U}));
}

TEST_F(GertBinariesUT, RejectsInvalidIdsAndNonTensorPayloads) {
  for (const auto *id : {"", "bad.id", "valid"}) {
    SCOPED_TRACE(id);
    ge::ExecuteGraph graph("invalid_payload");
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kBinaryIds, {id}));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, BinaryKey(id, "name"), "kernel.o"));
    if (std::string(id) == "valid") {
      ASSERT_TRUE(ge::AttrUtils::SetInt(graph, BinaryKey(id, "content"), 3));
    } else {
      ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, BinaryKey(id, "content"), MakeBytes({1U})));
    }
    std::vector<GertBinaryImage> images;
    EXPECT_NE(ParseGertBinaries(graph, images), ge::GRAPH_SUCCESS);
    EXPECT_TRUE(images.empty());
  }
}

}  // namespace gert
