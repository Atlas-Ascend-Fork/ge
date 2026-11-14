/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_

#include <functional>
#include <memory>
#include <vector>

#include "exe_graph/lowering/buffer_pool.h"
#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
class FastNode;
}  // namespace ge

namespace gert {
// 由调用方填充单个计算节点的 ComputeNodeInfo 内容。
// buffer_pool 由序列化骨架持有并共享：节点名/类型必须以池 id 写入，
// 因此内容必须与字符串池在同一趟构建，不能预先序列化。
using ComputeNodeInfoWriter = std::function<ge::graphStatus(size_t index, bg::BufferPool &buffer_pool,
                                                            std::unique_ptr<uint8_t[]> &data, size_t &size)>;

struct ExecuteGraphSerializationInput {
  // 执行节点全集：KernelExtendInfo 为每个节点各生成一份。
  std::vector<ge::FastNode *> all_nodes;
  // 需要生成 ComputeNodeInfo 的计算节点数量。下标即缓冲区序号。
  size_t compute_node_num{0U};
  // 逐节点填充 ComputeNodeInfo。index 为缓冲区下标。
  ComputeNodeInfoWriter write_compute_node_info;
  // 执行节点 -> ComputeNodeInfo 下标。只对需要绑定 CNI 的节点给出。
  struct ComputeNodeBinding {
    ge::FastNode *node{nullptr};
    size_t compute_node_index{0U};
  };
  std::vector<ComputeNodeBinding> compute_node_bindings;
  // 已序列化的模型 IO 描述（BuildModelDescBuffer 的产出）。
  std::vector<uint8_t> model_desc;
};

// 把元信息写到执行图的属性上：kBuffer / kComputeNodeInfo / kKernelExtendInfo / kModelDesc。
// 字符串池由本函数构建并被 ComputeNodeInfo 与 KernelExtendInfo 共享，布局与 GE lowering 路径一致。
// 覆盖同名属性，因此同一张图重复序列化是幂等的（不包含 DFX 信息）。
ge::graphStatus SerializeExecuteGraphMetadata(ge::ExecuteGraph &graph, const ExecuteGraphSerializationInput &input);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_
