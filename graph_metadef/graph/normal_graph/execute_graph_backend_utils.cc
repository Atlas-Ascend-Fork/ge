/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <algorithm>
#include <deque>
#include <set>
#include "normal_graph/execute_graph_backend_utils.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "normal_graph/operator_impl.h"
#include "normal_graph/op_desc_impl.h"
#include "normal_graph/operator_graph_builder.h"
#include "normal_graph/execute_node_backend.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "common/checker.h"
#include "exe_graph/lowering/builtin_node_types.h"

namespace ge {
namespace {
constexpr const char *kParentLifetime = "_ge_execute_parent_lifetime";

std::shared_ptr<std::weak_ptr<ExecuteGraph>> GetParentLifetime(ExecuteGraph *graph) {
  auto lifetime = graph->TryGetExtAttr<std::shared_ptr<std::weak_ptr<ExecuteGraph>>>(kParentLifetime, nullptr);
  if (lifetime == nullptr) {
    lifetime = ComGraphMakeShared<std::weak_ptr<ExecuteGraph>>();
    auto *parent = graph->GetParentGraphBarePtr();
    if (parent != nullptr) {
      *lifetime = parent->shared_from_this();
    }
    (void)graph->SetExtAttr(kParentLifetime, lifetime);
  }
  return lifetime;
}

bool HasLocalSubgraph(ExecuteGraph *owner, ExecuteGraph *graph) {
  const auto entries = owner->GetAllSubgraphs();
  return std::find(entries.begin(), entries.end(), graph) != entries.end();
}
}  // namespace

graphStatus ExecuteGraphBackendUtils::CopyGraph(const ExecuteGraph &src, ExecuteGraph &dst) {
  GE_ASSERT_GRAPH_SUCCESS(ExecuteGraphUtils::CopyGraph(src, dst));
  std::vector<ExecuteGraph *> graphs;
  CollectSubgraphTree(&dst, graphs);
  for (auto *graph : graphs) {
    auto *parent = graph->GetParentGraphBarePtr();
    SetParentGraph(graph, parent != nullptr ? parent->shared_from_this() : nullptr);
  }
  return GRAPH_SUCCESS;
}

graphStatus ExecuteGraphBackendUtils::AddDataEdge(FastNode *src_node, int32_t src_port_index, FastNode *dst_node,
                                                  int32_t dst_port_index) {
  // 执行图按图持有边表，边必须登记到两端共同所属的图；负端口不能在此被解释为控制边。
  auto *owner_graph = src_node->GetExtendInfo()->GetOwnerGraphBarePtr();
  GE_ASSERT_TRUE(owner_graph == dst_node->GetExtendInfo()->GetOwnerGraphBarePtr(),
                 "Data edge endpoints must belong to the same execute graph");
  GE_ASSERT_TRUE((src_port_index >= 0) && (dst_port_index >= 0), "AddDataEdge requires data ports, got %d -> %d",
                 src_port_index, dst_port_index);
  return owner_graph->AddEdge(src_node, src_port_index, dst_node, dst_port_index) != nullptr ? GRAPH_SUCCESS
                                                                                             : GRAPH_FAILED;
}

graphStatus ExecuteGraphBackendUtils::AddControlEdge(FastNode *src_node, FastNode *dst_node) {
  auto *owner_graph = src_node->GetExtendInfo()->GetOwnerGraphBarePtr();
  GE_ASSERT_TRUE(owner_graph == dst_node->GetExtendInfo()->GetOwnerGraphBarePtr(),
                 "Control edge endpoints must belong to the same execute graph");
  return owner_graph->AddEdge(src_node, -1, dst_node, -1) != nullptr ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus ExecuteGraphBackendUtils::RemoveEdge(FastNode *src_node, int32_t src_port_index, FastNode *dst_node,
                                                 int32_t dst_port_index) {
  auto *owner_graph = src_node->GetExtendInfo()->GetOwnerGraphBarePtr();
  GE_ASSERT_TRUE(owner_graph == dst_node->GetExtendInfo()->GetOwnerGraphBarePtr(),
                 "Edge endpoints must belong to the same execute graph");
  // 执行图只支持纯数据边或纯控制边，不支持数据输出到控制输入的混合边。
  const bool control_edge = (src_port_index == -1) && (dst_port_index == -1);
  GE_ASSERT_TRUE(control_edge || ((src_port_index >= 0) && (dst_port_index >= 0) &&
                                  (static_cast<size_t>(src_port_index) < src_node->GetDataOutNum()) &&
                                  (static_cast<size_t>(dst_port_index) < dst_node->GetDataInNum())),
                 "Invalid data/control ports %d -> %d", src_port_index, dst_port_index);
  const auto &edges =
      control_edge ? src_node->GetAllOutControlEdgesRef() : src_node->GetOutEdgesRefByIndex(src_port_index);
  for (const auto *edge : edges) {
    // 断边后邻接槽位可能为空；重复控制边每次只删除一条。
    if ((edge != nullptr) && (edge->dst == dst_node) && (edge->dst_input == dst_port_index)) {
      return owner_graph->RemoveEdge(edge);
    }
  }
  return GRAPH_FAILED;
}

bool ExecuteGraphBackendUtils::IsNodeInGraphRecursively(const ExecuteGraph *graph, const FastNode *node) {
  auto *current = node->GetExtendInfo()->GetOwnerGraphBarePtr();
  while (current != nullptr) {
    if (current == graph) {
      return true;
    }
    current = current->GetParentGraphBarePtr();
  }
  return false;
}

graphStatus ExecuteGraphBackendUtils::RemoveNodeWithoutRelink(ExecuteGraph *graph, FastNode *node) {
  // 删除父节点前解除直接子图的裸父指针；子图内仍存活的节点和下级父子关系保持有效。
  auto *root_graph = ExecuteGraphUtils::FindRootGraph(graph);
  for (const auto &name : node->GetOpDescBarePtr()->GetSubgraphInstanceNames()) {
    auto *subgraph = root_graph->GetSubGraph(name);
    if (subgraph != nullptr) {
      subgraph->SetParentNode(nullptr);
      SetParentGraph(subgraph, nullptr);
      for (auto *sub_node : subgraph->GetDirectNode()) {
        if (gert::IsTypeInnerNetOutput(sub_node->GetTypePtr())) {
          sub_node->GetOpDescBarePtr()->SetType(gert::kNetOutput);
        }
      }
    }
  }
  return ExecuteGraphUtils::RemoveNodeWithoutRelink(graph, node);
}

// ExecuteGraph 的父图/父节点使用裸指针，而外部可以在父图析构后继续持有子图门面。
// 因此沿父链用额外记录的 weak_ptr 检查父图是否仍存活、是否与裸指针一致，
// 在访问失效父指针前解除父子关系，并将脱离后的图输出恢复为独立图的 NetOutput。
void ExecuteGraphBackendUtils::NormalizeParentChain(ExecuteGraph *graph) {
  auto current = graph != nullptr ? graph->shared_from_this() : nullptr;
  while (current != nullptr) {
    const auto lifetime = GetParentLifetime(current.get());
    auto *raw_parent = current->GetParentGraphBarePtr();
    if (raw_parent == nullptr) {
      lifetime->reset();
      return;
    }
    const auto parent = lifetime->lock();
    if ((parent == nullptr) || (parent.get() != raw_parent)) {
      current->SetParentGraph(nullptr);
      current->SetParentNode(nullptr);
      for (auto *node : current->GetDirectNode()) {
        if (gert::IsTypeInnerNetOutput(node->GetTypePtr())) {
          node->GetOpDescBarePtr()->SetType(gert::kNetOutput);
        }
      }
      lifetime->reset();
      return;
    }
    current = parent;
  }
}

std::shared_ptr<ExecuteGraph> ExecuteGraphBackendUtils::GetParentGraph(ExecuteGraph *graph) {
  NormalizeParentChain(graph);
  return graph != nullptr ? GetParentLifetime(graph)->lock() : nullptr;
}

void ExecuteGraphBackendUtils::SetParentGraph(ExecuteGraph *graph, const std::shared_ptr<ExecuteGraph> &parent) {
  const auto lifetime = GetParentLifetime(graph);
  graph->SetParentGraph(parent.get());
  *lifetime = parent;
}

void ExecuteGraphBackendUtils::CollectSubgraphTree(ExecuteGraph *root, std::vector<ExecuteGraph *> &tree) {
  if (root == nullptr) {
    return;
  }
  auto *registry_root = ExecuteGraphUtils::FindRootGraph(root);
  std::deque<ExecuteGraph *> pending{root};
  while (!pending.empty()) {
    auto *current = pending.front();
    pending.pop_front();
    if (std::find(tree.begin(), tree.end(), current) != tree.end()) {
      continue;
    }
    tree.emplace_back(current);
    const auto append_children = [&pending, current](ExecuteGraph *registry) {
      for (auto *child : registry->GetAllSubgraphs()) {
        if ((child != nullptr) && (child->GetParentGraphBarePtr() == current)) {
          pending.emplace_back(child);
        }
      }
    };
    append_children(registry_root);
    if (current != registry_root) {
      append_children(current);
    }
  }
}

bool ExecuteGraphBackendUtils::RegisterSubgraphTree(ExecuteGraph *root_graph, ExecuteGraph *sub_graph) {
  std::vector<ExecuteGraph *> tree;
  CollectSubgraphTree(sub_graph, tree);
  // 必须在摘除任何旧登记前取得真实所有权，别名 shared_ptr 不拥有后代对象。
  std::vector<std::shared_ptr<ExecuteGraph>> holders;
  for (auto *current : tree) {
    holders.emplace_back(current->shared_from_this());
  }
  auto *old_root = ExecuteGraphUtils::FindRootGraph(sub_graph);
  for (const auto &current : holders) {
    if ((current.get() == root_graph) || HasLocalSubgraph(root_graph, current.get())) {
      continue;
    }
    // ExecuteGraph::AddSubGraph 会覆盖同名所有权记录，登记时拒绝不同对象重名，避免留下悬空条目。
    if (root_graph->GetSubGraph(current->GetName()) != nullptr) {
      GELOGE(GRAPH_FAILED, "[Add][SubGraph] Duplicate subgraph name %s", current->GetName().c_str());
      return false;
    }
    if (root_graph->AddSubGraph(current) == nullptr) {
      return false;
    }
  }
  // 新根拥有整棵子树后，再移除旧根和中间图的登记，保持唯一的根登记表。
  std::set<ExecuteGraph *> owners(tree.begin(), tree.end());
  owners.insert(old_root);
  for (auto *owner : owners) {
    if (owner == root_graph) {
      continue;
    }
    for (const auto &current : holders) {
      if (HasLocalSubgraph(owner, current.get())) {
        if (owner->RemoveSubGraph(current->GetName()) != GRAPH_SUCCESS) {
          return false;
        }
      }
    }
  }
  return true;
}

struct ExecuteGraphBuildOps {
  using GraphPtr = std::shared_ptr<ExecuteGraph>;
  using NodeType = FastNode *;

  static NodeType CreateNode(const GraphPtr &graph, const OpDescPtr &desc) {
    GE_ASSERT_NOTNULL(desc);
    GE_ASSERT_TRUE(!desc->GetName().empty(), "Missing operator name");
    const auto copied = OpDescUtils::CopyOpDesc(desc);
    GE_ASSERT_NOTNULL(copied);
    // Serialization-based copying rebuilds slots in name order; preserve the Operator's port contract.
    copied->impl_->subgraph_names_to_index_ = desc->GetSubgraphNameIndexes();
    copied->impl_->subgraph_instance_names_ = desc->GetSubgraphInstanceNames();
    return graph->AddNode(copied);
  }

  static graphStatus CheckDataOutput(NodeType, int32_t) {
    return GRAPH_SUCCESS;
  }

  static graphStatus AddDataEdge(const GraphPtr &graph, NodeType src, int32_t src_index, NodeType dst,
                                 int32_t dst_index) {
    GE_ASSERT_NOTNULL(graph->AddEdge(src, src_index, dst, dst_index));
    return GRAPH_SUCCESS;
  }

  static graphStatus AddControlEdge(const GraphPtr &graph, NodeType src, NodeType dst) {
    GE_ASSERT_NOTNULL(graph->AddEdge(src, -1, dst, -1));
    return GRAPH_SUCCESS;
  }

  static graphStatus MountSubgraph(const GraphPtr &owner, NodeType node, const std::string &name, uint32_t,
                                   const Graph &graph) {
    const auto subgraph = GraphUtilsEx::Graph2ExecuteGraph(graph);
    GE_ASSERT_NOTNULL(subgraph, "Execution node only accepts execution subgraphs");
    return ExecuteNodeBackend::MountSubgraph(owner, node, AscendString(name.c_str()), subgraph);
  }
};

class ExecuteGraphBuilderImpl {
 public:
  explicit ExecuteGraphBuilderImpl(const std::string &name)
      : graph_(ComGraphMakeShared<ExecuteGraph>(name)), builder_(graph_) {}

  std::shared_ptr<ExecuteGraph> BuildGraph(const std::vector<Operator> &inputs) {
    std::vector<OperatorImplPtr> vec_inputs;
    for (const auto &input : inputs) {
      const auto src_op_impl = input.operator_impl_;
      GE_CHK_BOOL_EXEC(src_op_impl != nullptr, REPORT_INNER_ERR_MSG("E18888", "src_op_impl is nullptr, check invalid.");
                       return nullptr, "[Check][Param] Operator Impl is null.");
      GE_CHK_BOOL_EXEC(src_op_impl->GetOpDescImpl() != nullptr,
                       REPORT_INNER_ERR_MSG("E18888", "impl's opdesc is nullptr, check invalid.");
                       return nullptr, "[Check][Param] Operator impl's opdesc is null.");
      const auto *type = src_op_impl->GetOpDescImpl()->GetTypePtr();
      GE_ASSERT_TRUE(gert::IsTypeGraphInput(type) || gert::IsTypeConst(type),
                     "Execution graph source type must be Data, AippData, ConstData or Const");
      vec_inputs.emplace_back(src_op_impl);
    }
    GE_CHK_BOOL_EXEC(!vec_inputs.empty(), REPORT_INNER_ERR_MSG("E18888", "Input operators are empty.");
                     return nullptr, "[Check][Param] Input operators are empty.");
    auto ret = builder_.WalkAllOperators(vec_inputs);
    GE_CHK_BOOL_EXEC(ret == GRAPH_SUCCESS, return nullptr, "[Call][WalkAllOperators] failed, ret:%d.", ret);

    ret = builder_.AddEdges();
    GE_CHK_BOOL_EXEC(ret == GRAPH_SUCCESS, return nullptr, "[Add][Edge] failed, ret:%d.", ret);

    std::vector<FastNode *> input_nodes;
    for (const auto &input : vec_inputs) {
      input_nodes.emplace_back(builder_.GetAllNodesInfo().at(input));
    }
    GE_ASSERT_SUCCESS(ExecuteGraphUtils::SetGraphInputs(graph_.get(), input_nodes));
    return graph_;
  }

 private:
  std::shared_ptr<ExecuteGraph> graph_;
  OperatorGraphBuilder<ExecuteGraphBuildOps> builder_;
};

static inline bool HasSameNameNode(const std::shared_ptr<ExecuteGraph> &execute_graph) {
  auto graphs = execute_graph->GetAllSubgraphs();
  graphs.emplace_back(execute_graph.get());
  for (const auto *graph : graphs) {
    std::set<std::string> node_names;
    for (const auto *node : graph->GetDirectNode()) {
      if (!node_names.insert(node->GetName()).second) {
        REPORT_INNER_ERR_MSG("E18888", "[Check][Param] graph %s has same name node %s", graph->GetName().c_str(),
                             node->GetName().c_str());
        GELOGE(GRAPH_FAILED, "[Check][Param] graph %s has same name node %s", graph->GetName().c_str(),
               node->GetName().c_str());
        return true;
      }
    }
  }
  return false;
}

std::shared_ptr<ExecuteGraph> GraphUtilsEx::CreateExecuteGraphFromOperator(const std::string &name,
                                                                           const std::vector<Operator> &inputs) {
  auto graph_builder_impl = ExecuteGraphBuilderImpl(name);
  auto execute_graph = graph_builder_impl.BuildGraph(inputs);
  GE_CHK_BOOL_EXEC(execute_graph != nullptr, REPORT_INNER_ERR_MSG("E18888", "BuildGraph failed, as return nullptr.");
                   return execute_graph, "[Build][Graph] Execute graph is nullptr");
  if (HasSameNameNode(execute_graph)) {
    GELOGW("[CreateGraph][Check] Nodes with same name exist in one execute graph is not allowed, graph_name: %s",
           name.c_str());
    execute_graph = nullptr;
  }

  return execute_graph;
}
}  // namespace ge
