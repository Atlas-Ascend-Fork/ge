/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_metadata_preparer.h"

#include <cinttypes>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/buffer.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "exe_graph/runtime/continuous_buffer.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/execute_graph_serialization.h"
#include "common/om2/dynamic_codegen/prepare/gert_compute_node_info_builder.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/gert_source_info.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"

namespace gert {
namespace {
// 外部图没有 GE lowering 的 CNI 池，为有效执行节点构造配套描述。
bool NeedsComputeNodeInfo(const ge::FastNode &node) {
  auto *const extend_info = node.GetExtendInfo();
  if (extend_info == nullptr) {
    return false;
  }
  // 外部工作图的 CNI 和字符串池一起重建，不能沿用上一次序列化的下标。
  return true;
}

bool PreserveSerializedMetadata(ge::ExecuteGraph &graph, const std::vector<ge::FastNode *> &nodes) {
  if (!graph.HasAttr(kBuffer) || !graph.HasAttr(kComputeNodeInfo) || !graph.HasAttr(kKernelExtendInfo) ||
      !graph.HasAttr(kModelDesc)) {
    GELOGW("[GertGraph] Serialized metadata attrs are incomplete for %s", graph.GetName().c_str());
    return false;
  }
  ge::Buffer strings;
  ge::Buffer compute_infos;
  ge::Buffer kernel_infos;
  if (!ge::AttrUtils::GetZeroCopyBytes(&graph, kBuffer, strings) ||
      !ge::AttrUtils::GetZeroCopyBytes(&graph, kComputeNodeInfo, compute_infos) ||
      !ge::AttrUtils::GetZeroCopyBytes(&graph, kKernelExtendInfo, kernel_infos) ||
      strings.GetSize() < sizeof(ContinuousBuffer) || compute_infos.GetSize() < sizeof(ContinuousBuffer) ||
      kernel_infos.GetSize() < sizeof(ContinuousBuffer)) {
    GELOGW("[GertGraph] Serialized metadata buffers are invalid for %s: strings=%zu compute=%zu kernel=%zu",
           graph.GetName().c_str(), strings.GetSize(), compute_infos.GetSize(), kernel_infos.GetSize());
    return false;
  }
  const auto *compute_info_pool = reinterpret_cast<const ContinuousBuffer *>(compute_infos.GetData());
  const auto *kernel_info_pool = reinterpret_cast<const ContinuousBuffer *>(kernel_infos.GetData());
  size_t fallback_compute_index = compute_info_pool->GetNum();
  size_t fallback_kernel_index = kernel_info_pool->GetNum();
  size_t missing_bindings = 0U;
  for (const auto *node : nodes) {
    if (node == nullptr || !NeedsComputeNodeInfo(*node)) {
      continue;
    }
    int64_t compute_node_index = -1;
    int64_t kernel_extend_index = -1;
    const auto desc = node->GetOpDescPtr();
    if (desc == nullptr) {
      GELOGW("[GertGraph] Missing OpDesc for node %s", node->GetNamePtr());
      return false;
    }
    const bool has_compute_index = ge::AttrUtils::GetInt(desc, kComputeNodeIndex, compute_node_index);
    const bool has_kernel_index = ge::AttrUtils::GetInt(desc, kKernelExtendIndex, kernel_extend_index);
    if (has_compute_index && compute_node_index >= 0 &&
        static_cast<size_t>(compute_node_index) < compute_info_pool->GetNum()) {
      const auto *info = compute_info_pool->Get<ComputeNodeInfo>(static_cast<size_t>(compute_node_index));
      // Fxrt serializes CNI name/type as string-pool offsets. Offset zero is a
      // valid first string and is represented as nullptr by this ABI, so those
      // optional labels cannot be used to validate the CNI record.
      if (info == nullptr) {
        GELOGW("[GertGraph] Invalid serialized CNI record: node=%s index=%" PRId64 " pool_num=%zu", node->GetNamePtr(),
               compute_node_index, compute_info_pool->GetNum());
        return false;
      }
      fallback_compute_index = static_cast<size_t>(compute_node_index);
    } else {
      ++missing_bindings;
    }
    if (has_kernel_index && kernel_extend_index >= 0 &&
        static_cast<size_t>(kernel_extend_index) < kernel_info_pool->GetNum()) {
      fallback_kernel_index = static_cast<size_t>(kernel_extend_index);
    } else {
      ++missing_bindings;
    }
  }
  if (fallback_compute_index >= compute_info_pool->GetNum() || fallback_kernel_index >= kernel_info_pool->GetNum()) {
    GELOGW("[GertGraph] No fallback serialized metadata for %s: compute=%zu/%zu kernel=%zu/%zu",
           graph.GetName().c_str(), fallback_compute_index, compute_info_pool->GetNum(), fallback_kernel_index,
           kernel_info_pool->GetNum());
    return false;
  }
  for (auto *const node : nodes) {
    if (node == nullptr || !NeedsComputeNodeInfo(*node)) {
      continue;
    }
    const auto desc = node->GetOpDescPtr();
    if (desc == nullptr) {
      return false;
    }
    int64_t index = -1;
    if (!ge::AttrUtils::GetInt(desc, kComputeNodeIndex, index) || index < 0 ||
        static_cast<size_t>(index) >= compute_info_pool->GetNum()) {
      if (!ge::AttrUtils::SetInt(desc, kComputeNodeIndex, static_cast<int64_t>(fallback_compute_index))) {
        return false;
      }
    }
    index = -1;
    if (!ge::AttrUtils::GetInt(desc, kKernelExtendIndex, index) || index < 0 ||
        static_cast<size_t>(index) >= kernel_info_pool->GetNum()) {
      if (!ge::AttrUtils::SetInt(desc, kKernelExtendIndex, static_cast<int64_t>(fallback_kernel_index))) {
        return false;
      }
    }
  }
  GELOGI("[GertGraph] Preserving serialized metadata for %s: nodes=%zu, fallback_bindings=%zu", graph.GetName().c_str(),
         nodes.size(), missing_bindings);
  return true;
}

ge::graphStatus BuildModelDesc(const ge::ExecuteGraph &graph, std::vector<uint8_t> &model_desc) {
  GertModelDescInfo info;
  GE_ASSERT_SUCCESS(ParseGertModelDesc(graph, info), "[GertGraph][Resource] Failed to parse model IO");
  bg::BufferPool pool;
  bg::BufferPool::BufId desc_id = 0U;
  GE_ASSERT_SUCCESS(BuildModelDescBuffer(info.inputs, info.outputs, info.resources, pool, desc_id),
                    "[GertGraph][Resource] Failed to build model desc");
  size_t size = 0U;
  const auto buffer = pool.Serialize(size);
  GE_ASSERT_NOTNULL(buffer);
  model_desc.assign(buffer.get(), buffer.get() + size);
  return ge::SUCCESS;
}

struct PendingComputeNode {
  ge::FastNode *node{nullptr};
  GertComputeNodeDef def;
};

ge::graphStatus CollectComputeNodeDefs(const std::vector<ge::FastNode *> &all_nodes,
                                       const std::map<std::string, const GertSourceInfo *> &source_by_id,
                                       std::vector<PendingComputeNode> &pending) {
  for (auto *const node : all_nodes) {
    GE_ASSERT_NOTNULL(node);
    if (!NeedsComputeNodeInfo(*node)) {
      continue;
    }
    PendingComputeNode entry;
    entry.node = node;
    std::string source_id;
    // 未声明来源是合法情况（固定 ABI kernel），GetNodeSourceId 用返回值表达「是否声明」，
    // 不能当作成功/失败判定。
    if (ge::AttrUtils::HasAttr(node->GetOpDescPtr(), gert::attr::kSourceId)) {
      GE_ASSERT_TRUE(GetNodeSourceId(*node, source_id), "Invalid source id on node %s", node->GetNamePtr());
    }
    const GertSourceInfo *source = nullptr;
    if (!source_id.empty()) {
      const auto iter = source_by_id.find(source_id);
      GE_ASSERT_TRUE(iter != source_by_id.end(), "[GertGraph][Resource] Node %s references unknown source %s",
                     node->GetNamePtr(), source_id.c_str());
      source = iter->second;
    }
    GE_ASSERT_SUCCESS(BuildComputeNodeDef(*node, source, entry.def),
                      "[GertGraph][Resource] Failed to build compute node def for %s", node->GetNamePtr());
    pending.emplace_back(std::move(entry));
  }

  return ge::SUCCESS;
}

ge::graphStatus ParseSourceNodeInfos(const ge::ExecuteGraph &graph, std::vector<GertSourceInfo> &sources,
                                     std::map<std::string, const GertSourceInfo *> &source_by_id) {
  GE_ASSERT_SUCCESS(ParseGertSourceInfo(graph, sources), "[GertGraph][Resource] Failed to parse sources");
  for (const auto &source : sources) {
    (void)source_by_id.emplace(source.id, &source);
  }

  return ge::SUCCESS;
}

ge::graphStatus PrepareComputeNodeInfos(const std::vector<ge::FastNode *> &all_nodes,
                                        const std::map<std::string, const GertSourceInfo *> &source_by_id,
                                        ExecuteGraphSerializationInput &input) {
  auto pending = std::make_shared<std::vector<PendingComputeNode>>();
  GE_ASSERT_SUCCESS(CollectComputeNodeDefs(all_nodes, source_by_id, *pending));

  // ComputeNodeInfo 必须在序列化时与字符串池同趟构建（名称写的是池 id）。
  input.compute_node_num = pending->size();
  input.write_compute_node_info = [pending](size_t index, bg::BufferPool &buffer_pool, std::unique_ptr<uint8_t[]> &data,
                                            size_t &size) {
    if (index >= pending->size()) {
      return ge::GRAPH_FAILED;
    }
    return BuildComputeNodeInfoBuffer((*pending)[index].def, buffer_pool, data, size);
  };
  input.compute_node_bindings.reserve(pending->size());
  for (size_t i = 0U; i < pending->size(); ++i) {
    input.compute_node_bindings.push_back({(*pending)[i].node, i});
  }

  return ge::SUCCESS;
}
}  // namespace

ge::graphStatus PrepareExecuteGraphMetadata(ge::ExecuteGraph &graph) {
  const auto all_nodes = graph.GetAllNodes();
  if (PreserveSerializedMetadata(graph, all_nodes)) {
    return ge::GRAPH_SUCCESS;
  }

  std::vector<GertSourceInfo> sources;
  std::map<std::string, const GertSourceInfo *> source_by_id;
  GE_ASSERT_SUCCESS(ParseSourceNodeInfos(graph, sources, source_by_id));

  ExecuteGraphSerializationInput input;
  input.all_nodes = all_nodes;
  GE_ASSERT_SUCCESS(BuildModelDesc(graph, input.model_desc));
  GE_ASSERT_SUCCESS(PrepareComputeNodeInfos(input.all_nodes, source_by_id, input));
  GE_ASSERT_SUCCESS(SerializeExecuteGraphMetadata(graph, input));

  GELOGI("[GertGraph] Prepared metadata for %s: nodes=%zu, compute_nodes=%zu", graph.GetName().c_str(),
         input.all_nodes.size(), input.compute_node_num);
  return ge::SUCCESS;
}
}  // namespace gert
