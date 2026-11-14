/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "stage/stage_graph_utils.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include "common/checker.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace ge {
namespace {
constexpr size_t kMaxTreeDepth = 1024U;

// 输出引用协议：输出 desc 上记录来源节点名与来源输出序号。
// 与 exe_graph/lowering/value_holder.cc 的 ValueHolder::RefFrom 及 runtime 的
// EquivalentDataEdges::ConstructRefOutputEquivalent 保持一致。
constexpr char_t const *kRefFromNode = "RefFromNode";
constexpr char_t const *kRefFromIndex = "RefFromIndex";

void CollectTree(ExecuteGraph &graph, std::vector<ExecuteGraph *> &result, size_t depth) {
  if (depth > kMaxTreeDepth) {
    GELOGE(GRAPH_FAILED, "[GertGraph][Subgraph] Subgraph nesting exceeds %zu levels at graph %s", kMaxTreeDepth,
           graph.GetName().c_str());
    return;
  }
  for (auto *sub : graph.GetAllSubgraphs()) {
    if ((sub == nullptr) || (std::find(result.begin(), result.end(), sub) != result.end())) {
      continue;
    }
    result.emplace_back(sub);
    CollectTree(*sub, result, depth + 1U);
  }
}

ExecuteGraph *FindInTree(ExecuteGraph &current, const std::string &name, size_t depth) {
  if (depth > kMaxTreeDepth) {
    return nullptr;
  }
  for (auto *sub : current.GetAllSubgraphs()) {
    if ((sub != nullptr) && (sub->GetName() == name)) {
      return sub;
    }
  }
  for (auto *sub : current.GetAllSubgraphs()) {
    if (sub == nullptr) {
      continue;
    }
    if (auto *found = FindInTree(*sub, name, depth + 1U); found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

std::string InstanceNameOf(const FastNode &node, size_t index) {
  const auto desc = node.GetOpDescPtr();
  if (desc == nullptr) {
    return std::string();
  }
  const auto &names = desc->GetSubgraphInstanceNames();
  return (index < names.size()) ? names[index] : std::string();
}

}  // namespace

std::vector<ExecuteGraph *> CollectSubgraphTree(ExecuteGraph &root) {
  std::vector<ExecuteGraph *> result;
  CollectTree(root, result, 0U);
  return result;
}

ExecuteGraph *FindUserSubgraph(const FastNode &node, size_t index) {
  const std::string name = InstanceNameOf(node, index);
  if (name.empty()) {
    return nullptr;
  }
  GE_ASSERT_NOTNULL(node.GetExtendInfo());
  auto *root = ExecuteGraphUtils::FindRootGraph(node.GetExtendInfo()->GetOwnerGraphBarePtr());
  GE_ASSERT_NOTNULL(root);
  // 根表按名查不到内层子图（副本里内层子图登记在中间图自己的表），必须再按树遍历兜底。
  auto *found = root->GetSubGraph(name);
  return (found != nullptr) ? found : FindInTree(*root, name, 0U);
}

graphStatus ValidateRefEndpoints(const ExecuteGraph &root) {
  GE_ASSERT_TRUE(root.GetParentNodeBarePtr() == nullptr, "Ref endpoint validation requires a root graph");
  // 运行期按整棵树的名称表解析 RefFrom（EquivalentDataEdges::ConstructRefOutputEquivalent
  // 用 GetNodeMapFromAllNodes），因此校验也必须跨图建表，否则会误拒合法的跨阶段引用；
  // 反过来，跨图重名会让运行期静默取到其中一个，必须在这里拒绝。
  std::vector<FastNode *> all_nodes = const_cast<ExecuteGraph &>(root).GetAllNodes();
  std::unordered_map<std::string, FastNode *> nodes_by_name;
  for (auto *node : all_nodes) {
    GE_ASSERT_NOTNULL(node);
    const auto desc = node->GetOpDescPtr();
    if (desc == nullptr) {
      continue;
    }
    const auto inserted = nodes_by_name.emplace(desc->GetName(), node);
    GE_ASSERT_TRUE(inserted.second || (inserted.first->second == node), "Node name %s is used twice in the graph tree",
                   desc->GetName().c_str());
  }
  for (auto *node : all_nodes) {
    const auto desc = node->GetOpDescPtr();
    for (size_t i = 0U; i < desc->GetOutputsSize(); ++i) {
      const auto out_desc = desc->GetOutputDescPtr(static_cast<uint32_t>(i));
      GE_ASSERT_NOTNULL(out_desc);
      std::string src_name;
      if (!AttrUtils::GetStr(out_desc, kRefFromNode, src_name)) {
        continue;
      }
      // 打了引用名就必须有配套索引，缺一项即无法确定端点。
      int64_t src_index = 0;
      GE_ASSERT_TRUE(AttrUtils::GetInt(out_desc, kRefFromIndex, src_index),
                     "Output %zu of node %s declares %s %s without %s", i, desc->GetName().c_str(), kRefFromNode,
                     src_name.c_str(), kRefFromIndex);
      const auto src_it = nodes_by_name.find(src_name);
      GE_ASSERT_TRUE(src_it != nodes_by_name.end(), "Output %zu of node %s refers to node %s which does not exist", i,
                     desc->GetName().c_str(), src_name.c_str());
      const auto src_desc = src_it->second->GetOpDescPtr();
      GE_ASSERT_NOTNULL(src_desc);
      GE_ASSERT_TRUE((src_index >= 0) && (static_cast<size_t>(src_index) < src_desc->GetOutputsSize()),
                     "Output %zu of node %s refers to output %" PRId64 " of node %s which only has %zu outputs", i,
                     desc->GetName().c_str(), src_index, src_name.c_str(), src_desc->GetOutputsSize());
      // 自引用会形成零长度依赖，等价端点无法收敛。
      GE_ASSERT_TRUE((src_it->second != node) || (src_index != static_cast<int64_t>(i)),
                     "Output %zu of node %s refers to itself", i, desc->GetName().c_str());
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ResetDerivedSymbols(const ExecuteGraph &root) {
  GE_ASSERT_TRUE(root.GetParentNodeBarePtr() == nullptr, "Symbol reset requires a root graph");
  std::vector<ExecuteGraph *> tree{const_cast<ExecuteGraph *>(&root)};
  for (auto *sub : CollectSubgraphTree(const_cast<ExecuteGraph &>(root))) {
    tree.emplace_back(sub);
  }
  for (auto *graph : tree) {
    GE_ASSERT_NOTNULL(graph);
    for (auto *node : graph->GetDirectNode()) {
      GE_ASSERT_NOTNULL(node);
      auto *extend_info = node->GetExtendInfo();
      GE_ASSERT_NOTNULL(extend_info);
      // 先按当前端口数对齐数组长度，再逐项写 kInvalidSymbol：
      // Update*Symbols 只是 resize，长度没变时旧符号会原样保留。
      extend_info->UpdateInputSymbols(node->GetDataInNum());
      extend_info->UpdateOutputSymbols(node->GetDataOutNum());
      for (size_t i = 0U; i < node->GetDataInNum(); ++i) {
        GE_ASSERT_GRAPH_SUCCESS(extend_info->SetInputSymbol(i, kInvalidSymbol));
      }
      for (size_t i = 0U; i < node->GetDataOutNum(); ++i) {
        GE_ASSERT_GRAPH_SUCCESS(extend_info->SetOutputSymbol(i, kInvalidSymbol));
      }
    }
  }
  return GRAPH_SUCCESS;
}
}  // namespace ge
