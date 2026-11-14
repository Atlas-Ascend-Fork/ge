/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_source_info.h"

#include <cinttypes>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
// 来源描述键的拼接方式：gert.source.<id>.<field>。ID 本身不含 '.'，因此不存在歧义。
constexpr const char *kSourcePrefix = "gert.source.";

std::string FieldKey(const std::string &id, const char *field) {
  return std::string(kSourcePrefix) + id + "." + field;
}

// 条目 ID 只允许 ASCII 字母数字、下划线和连字符，且非空。
// 这样拼接出来的键不会被误解析成层级结构，也能和属性名后缀区分开。
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

ge::graphStatus ReadTensorDescList(const ge::ExecuteGraph &graph, const std::string &key, const char *label,
                                   std::vector<ge::GeTensorDesc> &descs) {
  GE_ASSERT_TRUE(graph.HasAttr(key), "Source %s is required", key.c_str());
  std::vector<ge::ConstGeTensorPtr> tensors;
  GE_ASSERT_TRUE(ge::AttrUtils::GetListTensor(graph, key, tensors), "Source %s is not a Tensor list", key.c_str());
  std::vector<ge::GeTensorDesc> parsed;
  parsed.reserve(tensors.size());
  for (size_t i = 0U; i < tensors.size(); ++i) {
    if (tensors[i] == nullptr) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Source %s entry %zu is null", key.c_str(), i);
      return ge::GRAPH_PARAM_INVALID;
    }
    parsed.emplace_back(tensors[i]->GetTensorDesc());
  }
  (void)label;
  descs.swap(parsed);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadStrList(const ge::ExecuteGraph &graph, const std::string &key, std::vector<std::string> &values) {
  GE_ASSERT_TRUE(graph.HasAttr(key), "Source %s is required", key.c_str());
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(graph, key, values), "Source %s is not a string list", key.c_str());
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadIntList(const ge::ExecuteGraph &graph, const std::string &key, std::vector<int64_t> &values) {
  GE_ASSERT_TRUE(graph.HasAttr(key), "Source %s is required", key.c_str());
  GE_ASSERT_TRUE(ge::AttrUtils::GetListInt(graph, key, values), "Source %s is not an int64 list", key.c_str());
  return ge::GRAPH_SUCCESS;
}

// 实例数与 IR 定义必须自洽：required 恒为 1，optional 只能 0/1，dynamic 非负，
// 且各 IR 端口的实例数之和必须等于实例描述个数。仅按端口总数猜测会掩盖描述缺失。
ge::graphStatus ValidateIrCounts(const std::string &id, const char *label, const std::vector<std::string> &kinds,
                                 const std::vector<int64_t> &counts, const size_t instance_num) {
  GE_ASSERT_TRUE(kinds.size() == counts.size(), "Source %s IR %s kinds has %zu entries but %zu counts", id.c_str(),
                 label, kinds.size(), counts.size());
  size_t total = 0U;
  for (size_t i = 0U; i < counts.size(); ++i) {
    const auto &kind = kinds[i];
    // 空类型表示未声明类别，按 required 处理：这是固定 ABI kernel 的常见形态。
    if (kind.empty() || (kind == "required")) {
      GE_ASSERT_TRUE(counts[i] == 1, "Source %s IR %s %zu is required but count is %" PRId64, id.c_str(), label, i,
                     counts[i]);
    } else if (kind == "optional") {
      GE_ASSERT_TRUE((counts[i] == 0) || (counts[i] == 1), "Source %s IR %s %zu is optional but count is %" PRId64,
                     id.c_str(), label, i, counts[i]);
    } else if (kind == "dynamic") {
      GE_ASSERT_TRUE(counts[i] >= 0, "Source %s IR %s %zu is dynamic but count is negative", id.c_str(), label, i);
    } else {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Source %s IR %s %zu has unsupported kind %s", id.c_str(), label, i,
             kind.c_str());
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_TRUE(static_cast<size_t>(counts[i]) <= instance_num, "Source %s IR %s %zu count exceeds instances",
                   id.c_str(), label, i);
    total += static_cast<size_t>(counts[i]);
  }
  GE_ASSERT_TRUE(total == instance_num,
                 "Source %s IR %s instance counts sum to %zu but %zu instance descriptions are given", id.c_str(),
                 label, total, instance_num);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadOneSource(const ge::ExecuteGraph &graph, const std::string &id, GertSourceInfo &info) {
  GE_ASSERT_TRUE(IsValidEntryId(id), "Source id [%s] is not a valid entry id", id.c_str());
  info.id = id;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(id, "name"), info.name), "Source %s has no name", id.c_str());
  GE_ASSERT_TRUE(!info.name.empty(), "Source %s name is empty", id.c_str());
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(id, "type"), info.type), "Source %s has no type", id.c_str());
  GE_ASSERT_TRUE(!info.type.empty(), "Source %s type is empty", id.c_str());

  GE_ASSERT_SUCCESS(ReadTensorDescList(graph, FieldKey(id, "inputs"), "inputs", info.inputs));
  GE_ASSERT_SUCCESS(ReadTensorDescList(graph, FieldKey(id, "outputs"), "outputs", info.outputs));

  GE_ASSERT_SUCCESS(ReadStrList(graph, FieldKey(id, "ir_input_names"), info.ir_input_names));
  GE_ASSERT_SUCCESS(ReadStrList(graph, FieldKey(id, "ir_output_names"), info.ir_output_names));

  // 类别列表可缺省：缺省级别全部按 required 处理，此时实例数与 IR 数必须相等。
  std::vector<std::string> input_kinds;
  if (graph.HasAttr(FieldKey(id, "ir_input_kinds"))) {
    GE_ASSERT_SUCCESS(ReadStrList(graph, FieldKey(id, "ir_input_kinds"), input_kinds));
  } else {
    input_kinds.assign(info.ir_input_names.size(), "required");
  }
  std::vector<std::string> output_kinds;
  if (graph.HasAttr(FieldKey(id, "ir_output_kinds"))) {
    GE_ASSERT_SUCCESS(ReadStrList(graph, FieldKey(id, "ir_output_kinds"), output_kinds));
  } else {
    output_kinds.assign(info.ir_output_names.size(), "required");
  }

  GE_ASSERT_SUCCESS(ReadIntList(graph, FieldKey(id, "ir_input_counts"), info.ir_input_counts));
  GE_ASSERT_SUCCESS(ReadIntList(graph, FieldKey(id, "ir_output_counts"), info.ir_output_counts));
  GE_ASSERT_SUCCESS(ValidateIrCounts(id, "input", input_kinds, info.ir_input_counts, info.inputs.size()));
  GE_ASSERT_SUCCESS(ValidateIrCounts(id, "output", output_kinds, info.ir_output_counts, info.outputs.size()));

  GE_ASSERT_SUCCESS(ReadStrList(graph, FieldKey(id, "attr_names"), info.attr_names));
  std::set<std::string> seen_attrs;
  for (const auto &attr_name : info.attr_names) {
    GE_ASSERT_TRUE(!attr_name.empty(), "Source %s has an empty attribute name", id.c_str());
    GE_ASSERT_TRUE(seen_attrs.insert(attr_name).second, "Source %s attribute %s is declared twice", id.c_str(),
                   attr_name.c_str());
    const std::string key = FieldKey(id, ("attr." + attr_name).c_str());
    GE_ASSERT_TRUE(graph.HasAttr(key), "Source %s declares attribute %s but %s is missing", id.c_str(),
                   attr_name.c_str(), key.c_str());
    ge::AnyValue value;
    GE_ASSERT_SUCCESS(graph.GetAttr(key, value), "Source %s attribute %s cannot be read", id.c_str(),
                      attr_name.c_str());
    info.attrs.emplace_back(std::move(value));
  }
  return ge::GRAPH_SUCCESS;
}

// 节点上以 gert.source.id 关联来源；未声明返回 false。声明了空值属于非法。
ge::graphStatus GetRawNodeSourceId(const ge::FastNode &node, bool &declared, std::string &source_id) {
  declared = false;
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  const auto *value = ge::AttrUtils::GetStr(desc, "gert.source.id");
  if (value == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  declared = true;
  source_id = *value;
  GE_ASSERT_TRUE(!source_id.empty(), "Node %s declares an empty source id", node.GetNamePtr());
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus ParseGertSourceInfo(const ge::ExecuteGraph &graph, std::vector<GertSourceInfo> &infos) {
  std::vector<std::string> ids;
  if (graph.HasAttr(gert::attr::kSourceIds)) {
    GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(graph, gert::attr::kSourceIds, ids),
                   "Source id list is not a string list");
  }
  std::vector<GertSourceInfo> parsed;
  parsed.reserve(ids.size());
  std::set<std::string> seen;
  for (const auto &id : ids) {
    GE_ASSERT_TRUE(seen.insert(id).second, "Source id %s is listed twice", id.c_str());
    GertSourceInfo info;
    GE_ASSERT_SUCCESS(ReadOneSource(graph, id, info), "Failed to parse source %s", id.c_str());
    parsed.emplace_back(std::move(info));
  }
  // 节点引用了未声明的来源时不在这里报错：调用方按实际消费关系给出更精确的诊断。
  infos.swap(parsed);
  return ge::GRAPH_SUCCESS;
}

bool GetNodeSourceId(const ge::FastNode &node, std::string &source_id) {
  bool declared = false;
  return (GetRawNodeSourceId(node, declared, source_id) == ge::GRAPH_SUCCESS) && declared;
}

}  // namespace gert
