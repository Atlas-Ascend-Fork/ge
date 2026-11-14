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

#include "common/checker.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/context_extend.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
// 把一段缓冲区挂到图属性上。与既有序列化路径使用相同的属性名和零拷贝语义。
ge::graphStatus AttachBuffer(ge::ExecuteGraph &graph, const char *const attr_name, bg::BufferPool &pool) {
  size_t size = 0U;
  const auto buffer = pool.Serialize(size);
  GE_ASSERT_NOTNULL(buffer, "Failed to serialize buffer pool for %s", attr_name);
  GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(&graph, attr_name, ge::Buffer::CopyFrom(buffer.get(), size)),
                 "Failed to attach %s to execute graph %s", attr_name, graph.GetName().c_str());
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus SerializeExecuteGraphMetadata(ge::ExecuteGraph &graph, const ExecuteGraphSerializationInput &input) {
  bg::BufferPool buffer_pool;
  bg::BufferPool compute_node_infos;
  bg::BufferPool kernel_extend_infos;

  // KernelExtendInfo 只依赖节点自身，字符串池在这里顺带登记每个节点的名称与类型。
  for (auto *const node : input.all_nodes) {
    GE_ASSERT_NOTNULL(node);
    const auto name_id = buffer_pool.AddStr(node->GetNamePtr());
    const auto type_id = buffer_pool.AddStr(node->GetTypePtr());

    alignas(KernelExtendInfo) uint8_t holder[sizeof(KernelExtendInfo)]{};
    auto *extend_info = reinterpret_cast<KernelExtendInfo *>(holder);
    extend_info->SetKernelName(reinterpret_cast<const char *>(name_id));
    extend_info->SetKernelType(reinterpret_cast<const char *>(type_id));
    (void)kernel_extend_infos.AddBuf(holder, sizeof(holder));
  }

  // ComputeNodeInfo 与字符串池同趟构建：节点名/类型写的是池 id，顺序不能拆开。
  for (size_t i = 0U; i < input.compute_node_num; ++i) {
    GE_ASSERT_TRUE(static_cast<bool>(input.write_compute_node_info),
                   "Compute node writer is required when compute_node_num is non-zero");
    std::unique_ptr<uint8_t[]> data;
    size_t size = 0U;
    GE_ASSERT_SUCCESS(input.write_compute_node_info(i, buffer_pool, data, size),
                      "Failed to build compute node info #%zu", i);
    GE_ASSERT_NOTNULL(data, "Compute node info #%zu is empty", i);
    (void)compute_node_infos.AddBuf(data.get(), size);
  }

  // 执行节点到 CNI 下标的映射必须回写到节点上，codegen 按该属性取值。
  for (const auto &binding : input.compute_node_bindings) {
    GE_ASSERT_NOTNULL(binding.node);
    const auto op_desc = binding.node->GetOpDescBarePtr();
    GE_ASSERT_NOTNULL(op_desc);
    GE_ASSERT_TRUE(ge::AttrUtils::SetInt(op_desc, kComputeNodeIndex, static_cast<int64_t>(binding.compute_node_index)),
                   "Failed to bind compute node index for %s", binding.node->GetNamePtr());
  }

  GE_ASSERT_SUCCESS(AttachBuffer(graph, kBuffer, buffer_pool));
  GE_ASSERT_SUCCESS(AttachBuffer(graph, kComputeNodeInfo, compute_node_infos));
  GE_ASSERT_SUCCESS(AttachBuffer(graph, kKernelExtendInfo, kernel_extend_infos));
  // 模型描述已经是调用方按 BuildModelDescBuffer 固定好的完整序列化结果（自带 BufferPool
  // 头），不能再包一层 pool：codegen 会按「最后一条 buffer 即 RawModelDesc」解析，
  // 二次封装会把池头当成 ModelDesc 结构体读。
  if (!input.model_desc.empty()) {
    GE_ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
                       &graph, kModelDesc, ge::Buffer::CopyFrom(input.model_desc.data(), input.model_desc.size())),
                   "Failed to attach %s to execute graph %s", kModelDesc, graph.GetName().c_str());
  }
  GELOGI("[GertGraph] Serialized execute graph %s: nodes=%zu, compute_infos=%zu, model_desc=%zu bytes",
         graph.GetName().c_str(), input.all_nodes.size(), input.compute_node_num, input.model_desc.size());
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
