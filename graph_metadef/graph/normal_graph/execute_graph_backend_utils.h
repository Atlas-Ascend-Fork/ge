/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_UTILS_H_
#define METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_UTILS_H_

#include <memory>
#include <vector>
#include "graph/fast_graph/execute_graph.h"

namespace ge {
class ExecuteGraphBackendUtils {
 public:
  static graphStatus CopyGraph(const ExecuteGraph &src, ExecuteGraph &dst);
  static graphStatus AddDataEdge(FastNode *src_node, int32_t src_port_index, FastNode *dst_node,
                                 int32_t dst_port_index);
  static graphStatus AddControlEdge(FastNode *src_node, FastNode *dst_node);
  static graphStatus RemoveEdge(FastNode *src_node, int32_t src_port_index, FastNode *dst_node, int32_t dst_port_index);
  static bool IsNodeInGraphRecursively(const ExecuteGraph *graph, const FastNode *node);
  static graphStatus RemoveNodeWithoutRelink(ExecuteGraph *graph, FastNode *node);
  static void NormalizeParentChain(ExecuteGraph *graph);
  static std::shared_ptr<ExecuteGraph> GetParentGraph(ExecuteGraph *graph);
  static void SetParentGraph(ExecuteGraph *graph, const std::shared_ptr<ExecuteGraph> &parent);
  static bool RegisterSubgraphTree(ExecuteGraph *root_graph, ExecuteGraph *subgraph);

 private:
  static void CollectSubgraphTree(ExecuteGraph *graph, std::vector<ExecuteGraph *> &tree);
};
}  // namespace ge

#endif  // METADEF_GRAPH_NORMAL_GRAPH_EXECUTE_GRAPH_BACKEND_UTILS_H_
