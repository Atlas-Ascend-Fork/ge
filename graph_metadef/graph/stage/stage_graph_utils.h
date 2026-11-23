/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_STAGE_STAGE_GRAPH_UTILS_H_
#define METADEF_GRAPH_STAGE_STAGE_GRAPH_UTILS_H_

#include <cstddef>
#include <vector>
#include "graph/fast_graph/execute_graph.h"

namespace ge {
// 递归收集根图下的全部子图（含嵌套层级），不含根图自身。
// 用树遍历而不是按名查找：CompleteCopy 会把内层子图登记在中间图自己的表里，
// 而 ExecuteGraph::GetSubGraph 在子图上会委托根图查找，根表里没有内层子图。
std::vector<ExecuteGraph *> CollectSubgraphTree(const ExecuteGraph &root);

// 按父节点的子图实例序号解析用户子图。先查根图登记，再逐层遍历兜底。
// 实例名缺失、越界或登记被解除时返回 nullptr，由调用方决定报错还是跳过。
ExecuteGraph *FindUserSubgraph(const FastNode &node, size_t index);

// 校验整棵执行图的引用端点：每个输出 desc 上的 RefFromNode/RefFromIndex 必须解析到
// 本图或整棵树中唯一的节点，且索引在该节点的输出范围内。同名节点只在本图内可解析，
// 跨图重名视为歧义。RefFrom 以节点名为载体，节点搬移不改名，因此校验结果不随拆图改变，
// 可用于改图前的预校验。不修改任何属性。
graphStatus ValidateRefEndpoints(const ExecuteGraph &root);

// 使整棵执行图的 symbol 数组失效（全部置为 kInvalidSymbol）。
// symbol 由运行期按端点等价类重新推导，拆图/改图后旧的端点划分已不成立，
// 沿用旧值会把过期符号当成已算出的结果。
graphStatus ResetDerivedSymbols(ExecuteGraph &root);
}  // namespace ge

#endif  // METADEF_GRAPH_STAGE_STAGE_GRAPH_UTILS_H_
