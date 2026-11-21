/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_resources.h"

#include <map>
#include <set>
#include <utility>

#include "common/checker.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
// 条目 ID 规则与来源描述一致；拼接后不会产生层级歧义。
bool IsValidEntryId(const std::string &id) {
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const bool ok = ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
                    (c == '_') || (c == '-');
    if (!ok) {
      return false;
    }
  }
  return true;
}

std::string FieldKey(const char *prefix, const std::string &id, const char *field) {
  return std::string(prefix) + id + "." + field;
}

// 一维 uint8 Tensor 是公共侧的字节载荷承载：长度即 Tensor 数据区大小。
bool ReadBytes(const ge::ExecuteGraph &graph, const std::string &key, std::vector<uint8_t> &bytes) {
  ge::ConstGeTensorPtr tensor;
  if (!ge::AttrUtils::GetTensor(graph, key, tensor) || (tensor == nullptr)) {
    return false;
  }
  const size_t size = tensor->GetData().size();
  const auto *data = tensor->GetData().GetData();
  if ((size != 0U) && (data == nullptr)) {
    return false;
  }
  bytes.assign(data, data + size);
  return true;
}

ge::graphStatus ReadIdList(const ge::ExecuteGraph &graph, const char *attr, std::vector<std::string> &ids) {
  if (!graph.HasAttr(attr)) {
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(graph, attr, ids), "Resource id list %s is not a string list", attr);
  // 同一个 ID 指向同一份图属性，重复时保留首次声明的位置。
  std::set<std::string> seen;
  std::vector<std::string> unique_ids;
  unique_ids.reserve(ids.size());
  for (const auto &id : ids) {
    GE_ASSERT_TRUE(IsValidEntryId(id), "Resource id [%s] is not a valid entry id", id.c_str());
    if (seen.insert(id).second) {
      unique_ids.emplace_back(id);
    }
  }
  ids.swap(unique_ids);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseOneBinary(const ge::ExecuteGraph &graph, const std::string &id, GertBinaryImage &binary) {
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_BINARY_PREFIX, id, "name"), binary.name),
                 "Binary %s has no name", id.c_str());
  GE_ASSERT_TRUE(!binary.name.empty(), "Binary %s name is empty", id.c_str());
  const std::string content_key = FieldKey(GERT_ATTR_BINARY_PREFIX, id, "content");
  GE_ASSERT_TRUE(graph.HasAttr(content_key), "Binary %s has no content", id.c_str());
  GE_ASSERT_TRUE(ReadBytes(graph, content_key, binary.content), "Binary %s content is not a uint8 tensor", id.c_str());
  GE_ASSERT_TRUE(!binary.content.empty(), "Binary %s has empty content", id.c_str());
  return ge::GRAPH_SUCCESS;
}

}  // namespace

ge::graphStatus ParseGertBinaries(const ge::ExecuteGraph &graph, std::vector<GertBinaryImage> &images) {
  std::vector<std::string> ids;
  GE_ASSERT_SUCCESS(ReadIdList(graph, attr::kBinaryIds, ids));
  std::vector<GertBinaryImage> parsed;
  std::map<std::string, size_t> index_by_name;
  for (const auto &id : ids) {
    GertBinaryImage binary;
    GE_ASSERT_SUCCESS(ParseOneBinary(graph, id, binary), "Failed to parse binary %s", id.c_str());
    const auto [iter, inserted] = index_by_name.emplace(binary.name, parsed.size());
    if (inserted) {
      binary.key = om2::GetKernelBinaryKey(binary.content.data(), binary.content.size());
      parsed.emplace_back(std::move(binary));
    } else {
      GE_ASSERT_TRUE(parsed[iter->second].content == binary.content, "Binary %s has conflicting content",
                     binary.name.c_str());
    }
  }
  images = std::move(parsed);
  return ge::GRAPH_SUCCESS;
}

}  // namespace gert
