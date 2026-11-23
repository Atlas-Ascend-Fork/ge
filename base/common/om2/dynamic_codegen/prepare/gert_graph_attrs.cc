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
#include "framework/runtime/event_allocator.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
// 一条 IO 描述由「无数据 Tensor 列表」承载：TensorDesc 即描述本身，载荷不参与解析。
ge::graphStatus ReadIoDescs(const ge::ExecuteGraph &graph, const char *descs_attr, const char *side,
                            std::vector<ModelIoEntry> &entries) {
  std::vector<ge::ConstGeTensorPtr> tensors;
  GE_ASSERT_TRUE(ge::AttrUtils::GetListTensor(graph, descs_attr, tensors),
                 "Model %s description attribute %s is not a Tensor list", side, descs_attr);
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
    entry.name = entry.desc.GetName();
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

ge::graphStatus ReadResourceCount(const ge::ExecuteGraph &graph, const char *attr, int64_t &value) {
  if (graph.HasAttr(attr)) {
    GE_ASSERT_TRUE(ge::AttrUtils::GetInt(graph, attr, value), "Model resource attribute %s is not an int64", attr);
  }
  GE_ASSERT_TRUE(value >= 0, "Model resource attribute %s count %" PRId64 " is negative", attr, value);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadGertModelIo(const ge::ExecuteGraph &graph, GertModelDescInfo &info) {
  GE_ASSERT_SUCCESS(ReadIoDescs(graph, attr::kModelInputs, "input", info.inputs),
                    "Failed to parse model input descriptions");
  GE_ASSERT_SUCCESS(ReadIoDescs(graph, attr::kModelOutputs, "output", info.outputs),
                    "Failed to parse model output descriptions");
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadGertModelResourceCounts(const ge::ExecuteGraph &graph, ModelDescResources &resources) {
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, attr::kModelStreamCount, resources.reusable_stream_num));
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, attr::kModelEventCount, resources.reusable_event_num));
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, attr::kModelNotifyCount, resources.reusable_notify_num));
  GE_ASSERT_SUCCESS(ReadResourceCount(graph, attr::kModelAttachedStreamCount, resources.attached_stream_num));
  GE_ASSERT_TRUE(resources.reusable_stream_num > 0 || !graph.HasAttr(attr::kModelStreamCount),
                 "Model stream count must reserve at least the caller stream");
  GE_ASSERT_TRUE(static_cast<uint64_t>(resources.reusable_event_num) <= EventAllocator::kMaxEventNum,
                 "Invalid %s: count exceeds %zu", attr::kModelEventCount,
                 static_cast<size_t>(EventAllocator::kMaxEventNum));
  return ge::GRAPH_SUCCESS;
}

}  // namespace

ge::graphStatus ParseGertModelDesc(const ge::ExecuteGraph &graph, GertModelDescInfo &info) {
  GertModelDescInfo parsed;
  GE_ASSERT_SUCCESS(ReadGertModelIo(graph, parsed));
  GE_ASSERT_SUCCESS(ReadGertModelResourceCounts(graph, parsed.resources));
  info = std::move(parsed);
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
