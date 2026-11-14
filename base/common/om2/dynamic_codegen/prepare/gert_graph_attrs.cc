/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_graph_attrs.h"

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
// 一条 IO 描述由「无数据 Tensor 列表」承载：TensorDesc 即描述本身，载荷不参与解析。
ge::graphStatus ReadIoDescs(const ge::ExecuteGraph &graph, const char *descs_attr, const char *names_attr,
                            const char *side, std::vector<ModelIoEntry> &entries) {
  GE_ASSERT_TRUE(graph.HasAttr(descs_attr), "Model %s description attribute %s is required", side, descs_attr);
  std::vector<ge::ConstGeTensorPtr> tensors;
  GE_ASSERT_TRUE(ge::AttrUtils::GetListTensor(graph, descs_attr, tensors),
                 "Model %s description attribute %s is not a Tensor list", side, descs_attr);
  std::vector<std::string> names;
  GE_ASSERT_TRUE(graph.HasAttr(names_attr), "Model %s name attribute %s is required", side, names_attr);
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(graph, names_attr, names), "Model %s name attribute %s is not a string list",
                 side, names_attr);
  // 名称与描述一一对应：长度不等就无法确定某个槽位的对外名称。
  GE_ASSERT_TRUE(names.size() == tensors.size(), "Model %s description has %zu entries but %zu names", side,
                 tensors.size(), names.size());

  std::set<std::string> seen;
  std::vector<ModelIoEntry> parsed;
  parsed.reserve(tensors.size());
  for (size_t i = 0U; i < tensors.size(); ++i) {
    if (tensors[i] == nullptr) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Model %s description entry %zu is null", side, i);
      return ge::GRAPH_PARAM_INVALID;
    }
    ModelIoEntry entry;
    entry.desc = tensors[i]->GetTensorDesc();
    entry.name = names[i];
    if (entry.name.empty()) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Model %s description entry %zu has an empty name", side, i);
      return ge::GRAPH_PARAM_INVALID;
    }
    // 名称是运行期绑定和 dump 的唯一标识，重名会导致绑定到错误槽位。
    if (!seen.insert(entry.name).second) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Model %s description name %s is used twice", side, entry.name.c_str());
      return ge::GRAPH_PARAM_INVALID;
    }
    parsed.emplace_back(std::move(entry));
  }
  entries.swap(parsed);
  return ge::GRAPH_SUCCESS;
}

// 资源槽数：缺省 1/0/0；声明了就按声明校验，负数与 overflow 一律拒绝。
ge::graphStatus ReadResourceCount(const ge::ExecuteGraph &graph, const char *attr, int64_t default_value,
                                  const char *label, int64_t &value) {
  value = default_value;
  if (!graph.HasAttr(attr)) {
    return ge::GRAPH_SUCCESS;
  }
  int64_t parsed = 0;
  GE_ASSERT_TRUE(ge::AttrUtils::GetInt(graph, attr, parsed), "Model resource attribute %s is not an int64", attr);
  GE_ASSERT_TRUE(parsed >= 0, "Model %s count %" PRId64 " is negative", label, parsed);
  value = parsed;
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus ParseGertModelDesc(const ge::ExecuteGraph &graph, GertModelDescInfo &info) {
  GertModelDescInfo parsed;
  GE_ASSERT_SUCCESS(ReadIoDescs(graph, gert::attr::kModelInputs, gert::attr::kModelInputNames, "input", parsed.inputs),
                    "Failed to parse model input descriptions");
  GE_ASSERT_SUCCESS(
      ReadIoDescs(graph, gert::attr::kModelOutputs, gert::attr::kModelOutputNames, "output", parsed.outputs),
      "Failed to parse model output descriptions");
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, gert::attr::kModelStreamCount, ModelDescResources::kDefaultStreamNum,
                                      "stream", parsed.resources.reusable_stream_num),
                    "Failed to parse model stream count");
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, gert::attr::kModelEventCount, ModelDescResources::kDefaultEventNum,
                                      "event", parsed.resources.reusable_event_num),
                    "Failed to parse model event count");
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, gert::attr::kModelNotifyCount, ModelDescResources::kDefaultNotifyNum,
                                      "notify", parsed.resources.reusable_notify_num),
                    "Failed to parse model notify count");
  // stream 包含调用方主流，0 流意味着 Init 无处下发。
  GE_ASSERT_TRUE(parsed.resources.reusable_stream_num > 0,
                 "Model stream count must reserve at least the caller stream");
  info = std::move(parsed);
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
