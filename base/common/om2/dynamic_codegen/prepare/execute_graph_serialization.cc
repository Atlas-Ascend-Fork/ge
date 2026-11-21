/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "execute_graph_serialization.h"
#include "gert_source_op_json.h"

#include "common/checker.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/context_extend.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
ge::graphStatus SerializePool(bg::BufferPool &pool, ge::Buffer &result) {
  size_t size = 0U;
  const auto bytes = pool.Serialize(size);
  GE_ASSERT_NOTNULL(bytes);
  result = ge::Buffer::CopyFrom(bytes.get(), size);
  GE_ASSERT_TRUE(result.GetSize() == size);
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus SerializeExecuteGraphMetadata(ge::ExecuteGraph &graph, const std::vector<uint8_t> &model_desc) {
  bg::BufferPool buffer_pool;
  bg::BufferPool compute_node_infos;
  bg::BufferPool kernel_extend_infos;
  const auto nodes = graph.GetAllNodes();
  std::vector<size_t> kernel_ids;
  kernel_ids.reserve(nodes.size());
  std::vector<std::pair<ge::FastNode *, size_t>> compute_bindings;

  // KernelExtendInfo 只依赖节点自身，字符串池在这里顺带登记每个节点的名称与类型。
  for (auto *const node : nodes) {
    GE_ASSERT_NOTNULL(node);
    const auto name_id = buffer_pool.AddStr(node->GetNamePtr());
    const auto type_id = buffer_pool.AddStr(node->GetTypePtr());

    alignas(KernelExtendInfo) uint8_t holder[sizeof(KernelExtendInfo)]{};
    auto *extend_info = reinterpret_cast<KernelExtendInfo *>(holder);
    extend_info->SetKernelName(reinterpret_cast<const char *>(name_id));
    extend_info->SetKernelType(reinterpret_cast<const char *>(type_id));
    kernel_ids.push_back(kernel_extend_infos.AddBuf(holder, sizeof(holder)));
  }

  for (auto *const node : nodes) {
    if (!node->GetOpDescBarePtr()->HasAttr(attr::kSourceOp)) {
      continue;
    }
    std::unique_ptr<uint8_t[]> data;
    size_t size = 0U;
    GE_ASSERT_SUCCESS(BuildComputeNodeInfoFromSourceOp(*node, buffer_pool, data, size));
    compute_bindings.emplace_back(node, compute_node_infos.AddBuf(data.get(), size));
  }

  ge::Buffer strings;
  ge::Buffer compute;
  ge::Buffer kernels;
  ge::Buffer model;
  GE_ASSERT_SUCCESS(SerializePool(buffer_pool, strings));
  GE_ASSERT_SUCCESS(SerializePool(compute_node_infos, compute));
  GE_ASSERT_SUCCESS(SerializePool(kernel_extend_infos, kernels));
  model = ge::Buffer::CopyFrom(model_desc.data(), model_desc.size());
  GE_ASSERT_TRUE(model.GetSize() == model_desc.size());

  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kBuffer, std::move(strings)));
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kComputeNodeInfo, std::move(compute)));
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kKernelExtendInfo, std::move(kernels)));
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, kModelDesc, std::move(model)));
  for (size_t i = 0U; i < nodes.size(); ++i) {
    auto *const desc = nodes[i]->GetOpDescBarePtr();
    if (desc->HasAttr(kComputeNodeIndex)) {
      GE_ASSERT_SUCCESS(desc->DelAttr(kComputeNodeIndex));
    }
    GE_ASSERT_TRUE(ge::AttrUtils::SetInt(desc, kKernelExtendIndex, kernel_ids[i]));
  }
  for (const auto &binding : compute_bindings) {
    GE_ASSERT_TRUE(ge::AttrUtils::SetInt(binding.first->GetOpDescBarePtr(), kComputeNodeIndex, binding.second),
                   "Failed to bind compute node index for %s", binding.first->GetNamePtr());
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
