/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "stage/stage_splitter.h"

#include <algorithm>
#include <cinttypes>
#include <set>
#include "common/checker.h"
#include "graph/debug/ge_op_types.h"
#include "graph/fast_graph/edge.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "stage/stage_graph_utils.h"

namespace ge {
namespace {
// 与 exe_graph/lowering/builtin_node_types.h 保持一致的内部节点类型名。
constexpr char_t const *kInnerData = "InnerData";
constexpr char_t const *kInnerNetOutput = "InnerNetOutput";
constexpr char_t const *kNetOutput = "NetOutput";
constexpr char_t const *kConstData = "ConstData";
// 图输入节点序号属性，与 ES 及 RT2 的 index 协议一致。
constexpr char_t const *kFeedIndex = "index";
// 固定参数槽使用负序号，与用户槽（非负）分开编号。
constexpr int64_t kMinUserIndex = 0;

const char *StageNodeType(StageKind stage) {
  switch (stage) {
    case StageKind::kInit:
      return kStageInitNodeType;
    case StageKind::kDeInit:
      return kStageDeInitNodeType;
    default:
      return kStageMainNodeType;
  }
}

std::string NodeName(const FastNode &node) {
  const auto desc = node.GetOpDescPtr();
  return (desc == nullptr) ? std::string() : desc->GetName();
}

FastNode *AddPlainNode(ExecuteGraph &graph, const std::string &name, const char *type, uint32_t input_num,
                       uint32_t output_num) {
  auto desc = ComGraphMakeShared<OpDesc>(name, type);
  GE_ASSERT_NOTNULL(desc);
  for (uint32_t i = 0U; i < input_num; ++i) {
    GE_ASSERT_GRAPH_SUCCESS(desc->AddInputDesc(GeTensorDesc()));
  }
  for (uint32_t i = 0U; i < output_num; ++i) {
    GE_ASSERT_GRAPH_SUCCESS(desc->AddOutputDesc(GeTensorDesc()));
  }
  return graph.AddNode(desc);
}

bool IsFeedNodeType(const std::string &type) {
  return (type == DATA) || (type == kConstData) || (type == "RefData") || (type == "AippData") || (type == "AnyData");
}

// 输入分类：加载输入（ConstData）、执行输入（Data 系）、固定参数槽（负序号）。
// 不是输入节点时 is_feed 为 false；是输入节点却缺少序号属性属于非法图，必须报错而不是跳过。
graphStatus ClassifyFeedNode(const FastNode &node, bool &is_feed, StageInputClass &input_class, int64_t &index) {
  is_feed = false;
  const auto desc = node.GetOpDescPtr();
  if ((desc == nullptr) || !IsFeedNodeType(desc->GetType())) {
    return GRAPH_SUCCESS;
  }
  is_feed = true;
  int64_t node_index = 0;
  GE_ASSERT_TRUE(AttrUtils::GetInt(desc, kFeedIndex, node_index), "Feed node %s has no %s attribute",
                 NodeName(node).c_str(), kFeedIndex);
  index = node_index;
  if (node_index < kMinUserIndex) {
    // 固定槽不占用用户编号空间，也不参与连续性校验。
    input_class = StageInputClass::kFixed;
  } else if (desc->GetType() == kConstData) {
    input_class = StageInputClass::kLoad;
  } else {
    input_class = StageInputClass::kExecute;
  }
  return GRAPH_SUCCESS;
}

using ValueKey = std::pair<const FastNode *, int32_t>;

// 在根图内部搬移节点。不能直接用 ExecuteGraphUtils::MoveNodeToGraph：它会递归删除节点
// 挂载的用户子图，而阶段拆分只移动节点本身，子图与父节点的对应关系必须保留。
graphStatus MoveNodeKeepingSubgraphs(FastNode *node, ExecuteGraph *dst_graph) {
  GE_ASSERT_NOTNULL(node);
  GE_ASSERT_NOTNULL(dst_graph);
  GE_ASSERT_NOTNULL(node->GetExtendInfo());
  auto *src_graph = node->GetExtendInfo()->GetOwnerGraphBarePtr();
  GE_ASSERT_NOTNULL(src_graph);
  if (src_graph == dst_graph) {
    return GRAPH_SUCCESS;
  }
  // 输入/输出节点登记随节点一起迁移，避免旧图残留失效引用。
  (void)src_graph->RemoveInputNode(node);
  (void)src_graph->RemoveOutputNode(node);
  if (src_graph->CheckNodeIsInGraph(node)) {
    GE_ASSERT_GRAPH_SUCCESS(src_graph->RemoveJustNode(node), "Failed to detach node %s", NodeName(*node).c_str());
  }
  GE_ASSERT_NOTNULL(dst_graph->AddNode(node));
  GE_ASSERT_GRAPH_SUCCESS(node->GetExtendInfo()->SetOwnerGraph(dst_graph, node));
  // 用户子图必须随父节点一起可达：解析不到时搬移即失败，不能留下悬挂的实例名。
  // 父图取直接父图而不是根图，保证 GetAllSubgraphs 仍能按父图区分直接子图与孙图。
  const auto desc = node->GetOpDescPtr();
  if (desc != nullptr) {
    const auto &instance_names = desc->GetSubgraphInstanceNames();
    for (size_t index = 0U; index < instance_names.size(); ++index) {
      if (instance_names[index].empty()) {
        continue;
      }
      auto *sub_graph = FindUserSubgraph(*node, index);
      GE_ASSERT_NOTNULL(sub_graph, "Node %s lost user subgraph %s during the stage split", node->GetNamePtr(),
                        instance_names[index].c_str());
      sub_graph->SetParentGraph(dst_graph);
      sub_graph->SetParentNode(node);
    }
  }
  return GRAPH_SUCCESS;
}

// 子图边界用的父端口索引属性，与解析器/控制流语义一致。
constexpr char_t const *kParentNodeIndex = "_parent_node_index";

bool IsWhileType(const std::string &type) {
  return (type == "While") || (type == "_While") || (type == "StatelessWhile");
}

bool IsIfOrCaseType(const std::string &type) {
  return (type == "If") || (type == "_If") || (type == "StatelessIf") || (type == "Case") || (type == "StatelessCase");
}

// 收集子图内所有以父端口为边界的节点：Data 系按节点属性，NetOutput 按输入描述属性。
graphStatus CollectBoundaryIndices(const ExecuteGraph &sub_graph, std::vector<int64_t> &input_ports,
                                   std::vector<int64_t> &output_slots) {
  for (const auto *node : sub_graph.GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    const auto desc = node->GetOpDescPtr();
    if (desc == nullptr) {
      continue;
    }
    if (IsFeedNodeType(desc->GetType())) {
      int64_t parent_index = 0;
      // 没有父端口属性的输入是子图内部的局部输入，不属于边界，跳过。
      if (AttrUtils::GetInt(desc, kParentNodeIndex, parent_index)) {
        input_ports.emplace_back(parent_index);
      }
      continue;
    }
    if (desc->GetType() != kNetOutput) {
      continue;
    }
    for (size_t i = 0U; i < desc->GetInputsSize(); ++i) {
      const auto input_desc = desc->GetInputDescPtr(static_cast<uint32_t>(i));
      GE_ASSERT_NOTNULL(input_desc);
      int64_t parent_index = 0;
      if (AttrUtils::GetInt(input_desc, kParentNodeIndex, parent_index)) {
        output_slots.emplace_back(parent_index);
      }
    }
  }
  return GRAPH_SUCCESS;
}

// 用户子图边界校验：逐层解析子图、确认父节点/父图归属，并校验父端口索引与分支输出/循环变量约束。
// 这些约束无法用阶段结构表达，必须在拆图前明确拒绝，不能静默丢边。
graphStatus ValidateUserSubgraphs(const FastNode &node, const ExecuteGraph &work_graph, size_t depth) {
  GE_ASSERT_TRUE(depth <= 64U, "User subgraph nesting of node %s is too deep", node.GetNamePtr());
  const auto desc = node.GetOpDescPtr();
  if (desc == nullptr) {
    return GRAPH_SUCCESS;
  }
  const auto &instance_names = desc->GetSubgraphInstanceNames();
  size_t resolved = 0U;
  for (size_t index = 0U; index < instance_names.size(); ++index) {
    if (instance_names[index].empty()) {
      continue;
    }
    auto *sub_graph = FindUserSubgraph(node, index);
    GE_ASSERT_NOTNULL(sub_graph, "User subgraph %s of node %s cannot be resolved", instance_names[index].c_str(),
                      node.GetNamePtr());
    // 父关系必须指向本次拆分的这张图：指向源图的子图搬移后仍能解析到，却会与副本串用。
    GE_ASSERT_TRUE(sub_graph->GetParentNodeBarePtr() == &node, "User subgraph %s does not belong to node %s",
                   instance_names[index].c_str(), node.GetNamePtr());
    GE_ASSERT_TRUE(sub_graph->GetParentGraphBarePtr() == &work_graph, "User subgraph %s is not owned by graph %s",
                   instance_names[index].c_str(), work_graph.GetName().c_str());

    std::vector<int64_t> input_ports;
    std::vector<int64_t> output_slots;
    GE_ASSERT_SUCCESS(CollectBoundaryIndices(*sub_graph, input_ports, output_slots));
    for (const auto port : input_ports) {
      GE_ASSERT_TRUE((port >= 0) && (static_cast<size_t>(port) < desc->GetAllInputsSize()),
                     "Subgraph %s reads parent input %" PRId64 " which node %s does not have",
                     instance_names[index].c_str(), port, node.GetNamePtr());
    }
    std::set<int64_t> unique_slots;
    for (const auto slot : output_slots) {
      GE_ASSERT_TRUE((slot >= 0) && (static_cast<size_t>(slot) < desc->GetOutputsSize()),
                     "Subgraph %s writes parent output %" PRId64 " which node %s does not have",
                     instance_names[index].c_str(), slot, node.GetNamePtr());
      // 分支输出槽位重复会让同一父输出被两个分支写，语义无法确定。
      GE_ASSERT_TRUE(unique_slots.insert(slot).second, "Subgraph %s writes parent output %" PRId64 " twice",
                     instance_names[index].c_str(), slot);
    }
    // 循环变量：While 子图读入的每个父输入都必须在 NetOutput 上产出下一轮的值，否则循环状态断链。
    if (IsWhileType(desc->GetType())) {
      for (const auto port : input_ports) {
        GE_ASSERT_TRUE(unique_slots.count(port) != 0U,
                       "While subgraph %s does not produce loop variable for parent input %" PRId64,
                       instance_names[index].c_str(), port);
      }
    }
    ++resolved;
    // 逐层下钻：子图内节点可能再挂用户子图，其父图必须指向该子图本身。
    for (const auto *inner : sub_graph->GetDirectNode()) {
      GE_ASSERT_NOTNULL(inner);
      GE_ASSERT_SUCCESS(ValidateUserSubgraphs(*inner, *sub_graph, depth + 1U));
    }
  }
  // If/Case 必须至少有一个分支子图，否则条件节点没有可执行内容。
  if (IsIfOrCaseType(desc->GetType())) {
    GE_ASSERT_TRUE(resolved > 0U, "%s node %s has no branch subgraph", desc->GetType().c_str(), node.GetNamePtr());
  }
  return GRAPH_SUCCESS;
}

// 按生产节点名称和端口排序，保证同一逻辑图每次拆分得到相同的端口分配。
bool ValueLess(const ValueKey &lhs, const ValueKey &rhs) {
  const std::string lhs_name = NodeName(*lhs.first);
  const std::string rhs_name = NodeName(*rhs.first);
  if (lhs_name != rhs_name) {
    return lhs_name < rhs_name;
  }
  return lhs.second < rhs.second;
}
}  // namespace

graphStatus StageSplitter::ValidatePreconditions(const StagePlan &plan,
                                                 const std::vector<std::pair<FastNode *, int32_t>> &user_outputs) {
  GE_ASSERT_NOTNULL(plan.GetGraph());
  GE_ASSERT_TRUE(work_graph_.GetParentNodeBarePtr() == nullptr, "Stage split requires a root work graph");
  GE_ASSERT_TRUE(!plan.GetAllStages().empty(), "Stage plan is empty");
  for (const auto &item : plan.GetAllStages()) {
    GE_ASSERT_NOTNULL(item.first);
    GE_ASSERT_TRUE(work_graph_.CheckNodeIsInGraph(item.first), "Node %s is not in the work graph",
                   NodeName(*item.first).c_str());
  }
  std::set<ValueKey> seen_outputs;
  for (const auto &output : user_outputs) {
    GE_ASSERT_NOTNULL(output.first, "User output node is null");
    GE_ASSERT_TRUE(work_graph_.CheckNodeIsInGraph(output.first), "User output %s is not in the work graph",
                   NodeName(*output.first).c_str());
    GE_ASSERT_TRUE(output.second >= 0 && static_cast<size_t>(output.second) < output.first->GetDataOutNum(),
                   "User output port %d of node %s is invalid", output.second, NodeName(*output.first).c_str());
    GE_ASSERT_TRUE(seen_outputs.insert({output.first, output.second}).second, "Duplicated user output %s:%d",
                   NodeName(*output.first).c_str(), output.second);
    // 图输出必须在 Main 之前产出；DeInit 的结果对用户不可见。
    GE_ASSERT_TRUE(plan.GetStage(*output.first) != StageKind::kDeInit, "User output %s is produced in stage %s",
                   NodeName(*output.first).c_str(), StageKindToString(plan.GetStage(*output.first)));
  }
  for (const auto &item : plan.GetAllStages()) {
    const StageKind dst_stage = item.second.stage;
    for (const auto *edge : item.first->GetAllInControlEdgesRef()) {
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      const StageKind src_stage = plan.GetStage(*edge->src);
      // 控制依赖只能沿生命周期方向；逆序无法由阶段结构表达，必须拒绝而不能静默丢边。
      GE_ASSERT_TRUE(src_stage <= dst_stage, "Control edge %s -> %s is reversed across stages %s -> %s",
                     NodeName(*edge->src).c_str(), NodeName(*item.first).c_str(), StageKindToString(src_stage),
                     StageKindToString(dst_stage));
    }
  }
  // 用户子图边界与分支/循环约束在改图前校验，失败时工作图保持原样。
  for (const auto &item : plan.GetAllStages()) {
    GE_ASSERT_SUCCESS(ValidateUserSubgraphs(*item.first, work_graph_, 0U));
  }
  // 引用端点也必须在改图前校验：RefFrom 按节点名解析，搬移不改名，因此拆图前后同名可解析性
  // 相同；在这里拒绝不会留下拆了一半的图。
  GE_ASSERT_SUCCESS(ValidateRefEndpoints(work_graph_));
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::BuildLayout(const StagePlan &plan,
                                       const std::vector<std::pair<FastNode *, int32_t>> &user_outputs,
                                       Layout &layout) const {
  for (const auto &item : plan.GetAllStages()) {
    const StageKind dst_stage = item.second.stage;
    for (const auto *edge : item.first->GetAllInDataEdgesRef()) {
      if ((edge == nullptr) || (edge->src == nullptr) || (plan.GetStage(*edge->src) == dst_stage)) {
        continue;
      }
      layout.consumers[{edge->src, edge->src_output}].emplace_back(
          CrossConsumer{dst_stage, item.first, edge->dst_input});
    }
  }
  for (size_t slot = 0U; slot < user_outputs.size(); ++slot) {
    const auto &output = user_outputs[slot];
    const StageKind producer_stage = plan.GetStage(*output.first);
    if (producer_stage == StageKind::kMain) {
      continue;
    }
    // 非 Main 阶段产出的用户输出同样跨阶段交给 Main 的 NetOutput，槽位保持不变。
    layout.consumers[{output.first, output.second}].emplace_back(
        CrossConsumer{StageKind::kMain, nullptr, static_cast<int32_t>(slot)});
  }

  for (const auto &consumer : layout.consumers) {
    auto &exports = layout.exports[plan.GetStage(*consumer.first.first)];
    if (std::find(exports.begin(), exports.end(), consumer.first) == exports.end()) {
      exports.emplace_back(consumer.first);
    }
    for (const auto &dst : consumer.second) {
      auto &imports = layout.imports[dst.stage];
      if (std::find(imports.begin(), imports.end(), consumer.first) == imports.end()) {
        imports.emplace_back(consumer.first);
      }
    }
  }
  for (auto &entry : layout.exports) {
    std::sort(entry.second.begin(), entry.second.end(), ValueLess);
  }
  for (auto &entry : layout.imports) {
    std::sort(entry.second.begin(), entry.second.end(), ValueLess);
    for (size_t port = 0U; port < entry.second.size(); ++port) {
      layout.import_ports[{entry.first, entry.second[port]}] = static_cast<int32_t>(port);
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::CreateStageContainers() {
  for (const auto stage : {StageKind::kInit, StageKind::kMain, StageKind::kDeInit}) {
    auto *stage_node = AddPlainNode(work_graph_, StageNodeType(stage), StageNodeType(stage), 0U, 0U);
    GE_ASSERT_NOTNULL(stage_node);
    auto stage_graph = ComGraphMakeShared<ExecuteGraph>(StageNodeType(stage));
    GE_ASSERT_NOTNULL(stage_graph);
    // 容器节点的 OpDesc 先登记子图 IR 名再挂载，0 号实例名才能在挂载时写入。
    GE_ASSERT_GRAPH_SUCCESS(FastNodeUtils::AppendSubgraphToNode(stage_node, StageNodeType(stage), stage_graph));
    stage_nodes_[stage] = stage_node;
    stage_graphs_[stage] = stage_graph;
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::AddStageControlOrder() {
  // 阶段生命周期顺序由容器控制边表达，跨阶段的合法控制依赖因此都能得到满足。
  // 必须在清空原图边之后建立，否则会被当作原边一并删除。
  GE_ASSERT_NOTNULL(work_graph_.AddEdge(stage_nodes_.at(StageKind::kInit), kControlEdgeIndex,
                                        stage_nodes_.at(StageKind::kMain), kControlEdgeIndex));
  GE_ASSERT_NOTNULL(work_graph_.AddEdge(stage_nodes_.at(StageKind::kMain), kControlEdgeIndex,
                                        stage_nodes_.at(StageKind::kDeInit), kControlEdgeIndex));
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::MoveNodesIntoStages(const StagePlan &plan) {
  // 先记录同阶段边再整体断边：MoveNodeToGraph 会按邻居重连，跨图端点不能交给它处理。
  for (const auto &item : plan.GetAllStages()) {
    const auto *node = item.first;
    for (const auto *edge : node->GetAllInDataEdgesRef()) {
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      if (plan.GetStage(*edge->src) == item.second.stage) {
        same_stage_data_edges_.emplace_back(CrossEdge{edge->src, edge->src_output, node, edge->dst_input});
      }
    }
    for (const auto *edge : node->GetAllInControlEdgesRef()) {
      if ((edge == nullptr) || (edge->src == nullptr)) {
        continue;
      }
      if (plan.GetStage(*edge->src) == item.second.stage) {
        same_stage_control_edges_.emplace_back(edge->src, node);
      }
    }
  }

  const auto all_edges = work_graph_.GetAllEdges();
  for (auto *edge : all_edges) {
    if (edge != nullptr) {
      GE_ASSERT_GRAPH_SUCCESS(work_graph_.RemoveEdge(edge));
    }
  }

  for (const auto &item : plan.GetAllStages()) {
    auto *node = const_cast<FastNode *>(item.first);
    GE_ASSERT_TRUE(node_map_.count(node) == 0U, "Node %s was moved twice", NodeName(*node).c_str());
    GE_ASSERT_GRAPH_SUCCESS(MoveNodeKeepingSubgraphs(node, stage_graphs_.at(item.second.stage).get()),
                            "Failed to move node %s", NodeName(*node).c_str());
    node_map_[node] = node;
  }

  for (const auto &edge : same_stage_data_edges_) {
    auto *stage_graph = stage_graphs_.at(plan.GetStage(*edge.src)).get();
    GE_ASSERT_NOTNULL(
        stage_graph->AddEdge(node_map_.at(edge.src), edge.src_port, node_map_.at(edge.dst), edge.dst_port),
        "Failed to restore data edge %s:%d -> %s:%d", NodeName(*edge.src).c_str(), edge.src_port,
        NodeName(*edge.dst).c_str(), edge.dst_port);
  }
  for (const auto &edge : same_stage_control_edges_) {
    auto *stage_graph = stage_graphs_.at(plan.GetStage(*edge.first)).get();
    GE_ASSERT_NOTNULL(
        stage_graph->AddEdge(node_map_.at(edge.first), kControlEdgeIndex, node_map_.at(edge.second), kControlEdgeIndex),
        "Failed to restore control edge %s -> %s", NodeName(*edge.first).c_str(), NodeName(*edge.second).c_str());
  }
  same_stage_data_edges_.clear();
  same_stage_control_edges_.clear();
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::RecordInputBindings(const StagePlan &plan) {
  std::map<StageInputClass, std::set<int64_t>> used;
  for (const auto &item : plan.GetAllStages()) {
    auto *node = const_cast<FastNode *>(item.first);
    StageInputClass input_class = StageInputClass::kExecute;
    int64_t index = 0;
    bool is_feed = false;
    GE_ASSERT_SUCCESS(ClassifyFeedNode(*node, is_feed, input_class, index));
    if (!is_feed) {
      continue;
    }
    GE_ASSERT_TRUE(used[input_class].insert(index).second,
                   "Duplicated input index %" PRId64 " for class %u in stage %s", index,
                   static_cast<uint32_t>(input_class), StageKindToString(item.second.stage));
    // 加载输入只能在加载阶段物化为 ConstData，执行输入只能在执行阶段物化为 Data。
    if (input_class == StageInputClass::kLoad) {
      GE_ASSERT_TRUE(item.second.stage == StageKind::kInit, "Load input %s is declared in stage %s, only init is legal",
                     NodeName(*node).c_str(), StageKindToString(item.second.stage));
    } else if (input_class == StageInputClass::kExecute) {
      GE_ASSERT_TRUE(item.second.stage == StageKind::kMain,
                     "Execute input %s is declared in stage %s, only main is legal", NodeName(*node).c_str(),
                     StageKindToString(item.second.stage));
    }
    input_bindings_[item.second.stage][input_class].emplace_back(StageInputBinding{input_class, index, node});
  }

  std::map<StageInputClass, std::set<int64_t>> user_indexes;
  for (auto &stage_item : input_bindings_) {
    for (auto &class_item : stage_item.second) {
      std::sort(class_item.second.begin(), class_item.second.end(),
                [](const StageInputBinding &lhs, const StageInputBinding &rhs) { return lhs.index < rhs.index; });
      if (class_item.first == StageInputClass::kFixed) {
        continue;
      }
      for (const auto &binding : class_item.second) {
        user_indexes[class_item.first].insert(binding.index);
      }
    }
  }
  // 加载输入、执行输入各自从 0 连续；用户槽与固定槽不共用编号空间。
  for (const auto &item : user_indexes) {
    const char *class_name = (item.first == StageInputClass::kLoad) ? "load" : "execute";
    GE_ASSERT_TRUE(*item.second.begin() == 0, "%s input indexes must start from 0", class_name);
    GE_ASSERT_TRUE(static_cast<size_t>(*item.second.rbegin()) + 1U == item.second.size(),
                   "%s input indexes are not continuous", class_name);
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::CreateStageTailNodes(const std::vector<std::pair<FastNode *, int32_t>> &user_outputs) {
  for (const auto stage : {StageKind::kInit, StageKind::kMain, StageKind::kDeInit}) {
    const auto exports_it = layout_.exports.find(stage);
    const auto import_it = layout_.imports.find(stage);
    const size_t export_num = (exports_it == layout_.exports.end()) ? 0U : exports_it->second.size();
    // 每个阶段都保留 InnerNetOutput 作为跨阶段导出与收尾目标；无导出时输入数为 0。
    auto *inner_net_output =
        AddPlainNode(*stage_graphs_.at(stage), std::string(kInnerNetOutput) + "_" + StageNodeType(stage),
                     kInnerNetOutput, static_cast<uint32_t>(export_num), 0U);
    GE_ASSERT_NOTNULL(inner_net_output);
    stage_inner_net_outputs_[stage] = inner_net_output;
    stage_net_outputs_[stage] = inner_net_output;
    // 容器输出端口数等于导出值个数，端口号即 InnerNetOutput 的输入序号。
    GE_ASSERT_GRAPH_SUCCESS(
        FastNodeUtils::AppendOutputEdgeInfo(stage_nodes_.at(stage), static_cast<uint32_t>(export_num)));
    // 容器输入端口数等于导入值个数，端口号即子图内 InnerData 的 index。
    if (import_it != layout_.imports.end()) {
      GE_ASSERT_GRAPH_SUCCESS(
          FastNodeUtils::AppendInputEdgeInfo(stage_nodes_.at(stage), static_cast<uint32_t>(import_it->second.size())));
    }
  }

  // Main 的 NetOutput 表达用户输出，槽位顺序与用户登记一致。
  auto *net_output =
      AddPlainNode(*stage_graphs_.at(StageKind::kMain), std::string(kNetOutput) + "_" + StageNodeType(StageKind::kMain),
                   kNetOutput, static_cast<uint32_t>(user_outputs.size()), 0U);
  GE_ASSERT_NOTNULL(net_output);
  stage_net_outputs_[StageKind::kMain] = net_output;
  return GRAPH_SUCCESS;
}

FastNode *StageSplitter::CreateInnerData(StageKind stage, int32_t port, const ValueKey &value) {
  auto &stage_graph = stage_graphs_.at(stage);
  auto desc = ComGraphMakeShared<OpDesc>(NodeName(*value.first) + "_inner_data_" + std::to_string(port), kInnerData);
  GE_ASSERT_NOTNULL(desc);
  GE_ASSERT_GRAPH_SUCCESS(desc->AddOutputDesc(GeTensorDesc()));
  // index 对应该消费阶段容器的输入端口，不能套用生产阶段的导出序号。
  GE_ASSERT_TRUE(AttrUtils::SetInt(desc, kFeedIndex, static_cast<int64_t>(port)));
  auto *inner_data = stage_graph->AddNode(desc);
  GE_ASSERT_NOTNULL(inner_data);
  inner_data->GetExtendInfo()->SetInputIndex(port);
  return inner_data;
}

int32_t StageSplitter::ExportPortOf(StageKind stage, const ValueKey &value) const {
  const auto it = layout_.exports.find(stage);
  if (it == layout_.exports.end()) {
    return -1;
  }
  const auto value_it = std::find(it->second.begin(), it->second.end(), value);
  return (value_it == it->second.end()) ? -1 : static_cast<int32_t>(std::distance(it->second.begin(), value_it));
}

graphStatus StageSplitter::BuildCrossStageBridges(const StagePlan &plan) {
  // 消费侧：每个 (阶段, 值) 建一个 InnerData，index 即该阶段容器的输入端口。
  for (const auto &import : layout_.imports) {
    for (size_t port = 0U; port < import.second.size(); ++port) {
      auto *inner_data = CreateInnerData(import.first, static_cast<int32_t>(port), import.second[port]);
      GE_ASSERT_NOTNULL(inner_data);
      inner_datas_[{import.first, import.second[port]}] = inner_data;
    }
  }

  for (const auto &consumer : layout_.consumers) {
    const ValueKey &value = consumer.first;
    const StageKind producer_stage = plan.GetStage(*value.first);
    const int32_t export_port = ExportPortOf(producer_stage, value);
    GE_ASSERT_TRUE(export_port >= 0, "Value %s:%d is not exported from stage %s", NodeName(*value.first).c_str(),
                   value.second, StageKindToString(producer_stage));
    // 生产侧：阶段导出端口即容器输出端口，与 InnerNetOutput 输入序号一致。
    GE_ASSERT_NOTNULL(stage_graphs_.at(producer_stage)
                          ->AddEdge(node_map_.at(value.first), value.second,
                                    stage_inner_net_outputs_.at(producer_stage), export_port),
                      "Failed to export %s:%d from %s", NodeName(*value.first).c_str(), value.second,
                      StageKindToString(producer_stage));

    for (const auto &dst : consumer.second) {
      // 消费侧经 InnerData 接入，端口号由容器输入端口在下一段统一绑定。
      auto *inner_data = inner_datas_.at({dst.stage, value});
      if (dst.dst == nullptr) {
        // Main 的 NetOutput 槽位在阶段输出连接时统一处理。
        continue;
      }
      GE_ASSERT_NOTNULL(stage_graphs_.at(dst.stage)->AddEdge(inner_data, 0, node_map_.at(dst.dst), dst.dst_port),
                        "Failed to reconnect consumer %s", NodeName(*dst.dst).c_str());
    }
  }

  // 容器之间的边只承载跨阶段值：生产容器输出端口 -> 消费容器输入端口。
  for (const auto &import : layout_.imports) {
    for (const auto &value : import.second) {
      const StageKind producer_stage = plan.GetStage(*value.first);
      const int32_t export_port = ExportPortOf(producer_stage, value);
      GE_ASSERT_TRUE(export_port >= 0);
      const int32_t import_port = layout_.import_ports.at({import.first, value});
      GE_ASSERT_NOTNULL(
          work_graph_.AddEdge(stage_nodes_.at(producer_stage), export_port, stage_nodes_.at(import.first), import_port),
          "Failed to bridge %s:%d from %s to %s", NodeName(*value.first).c_str(), value.second,
          StageKindToString(producer_stage), StageKindToString(import.first));
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::ConnectStageOutputs(const StagePlan &plan,
                                               const std::vector<std::pair<FastNode *, int32_t>> &user_outputs) {
  auto *stage_graph = stage_graphs_.at(StageKind::kMain).get();
  auto *net_output = stage_net_outputs_.at(StageKind::kMain);
  GE_ASSERT_NOTNULL(net_output);
  output_bindings_.clear();
  for (size_t slot = 0U; slot < user_outputs.size(); ++slot) {
    const auto &output = user_outputs[slot];
    FastNode *src = nullptr;
    int32_t src_port = 0;
    if (plan.GetStage(*output.first) == StageKind::kMain) {
      src = node_map_.at(output.first);
      src_port = output.second;
    } else {
      // 跨阶段产出的用户输出通过 Main 子图内的 InnerData 落到 NetOutput。
      const auto inner_it = inner_datas_.find({StageKind::kMain, {output.first, output.second}});
      GE_ASSERT_TRUE(inner_it != inner_datas_.end(), "Missing bridge for user output %s:%d",
                     NodeName(*output.first).c_str(), output.second);
      src = inner_it->second;
      src_port = 0;
    }
    GE_ASSERT_NOTNULL(stage_graph->AddEdge(src, src_port, net_output, static_cast<int32_t>(slot)),
                      "Failed to connect user output slot %zu", slot);
    output_bindings_.emplace_back(StageOutputBinding{static_cast<int64_t>(slot), src, src_port});
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::ValidateSplitResult() const {
  // 根图上只能有三个阶段容器，且都挂在根图下；多出来的节点说明拆图把节点漏在了外层。
  for (const auto stage : {StageKind::kInit, StageKind::kMain, StageKind::kDeInit}) {
    const auto it = stage_nodes_.find(stage);
    GE_ASSERT_TRUE(it != stage_nodes_.end(), "Stage %s container is missing", StageKindToString(stage));
    GE_ASSERT_TRUE(work_graph_.CheckNodeIsInGraph(it->second), "Stage %s container is not in the root graph",
                   StageKindToString(stage));
    // 每个容器必须挂一张且仅一张子图，父指针指回容器本身。
    auto *stage_graph = it->second->GetExtendInfo()->GetOwnerGraphBarePtr();
    GE_ASSERT_NOTNULL(stage_graph);
    auto *sub = FindUserSubgraph(*it->second, 0U);
    GE_ASSERT_NOTNULL(sub, "Stage %s container lost its subgraph", StageKindToString(stage));
    GE_ASSERT_TRUE(sub->GetParentNodeBarePtr() == it->second, "Stage %s subgraph has a foreign parent node",
                   StageKindToString(stage));
    GE_ASSERT_TRUE(sub->GetParentGraphBarePtr() == &work_graph_, "Stage %s subgraph has a foreign parent graph",
                   StageKindToString(stage));
  }
  // 拆完后根图不应残留任何非容器节点，否则节点搬移不完整。
  for (const auto *node : work_graph_.GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    const auto desc = node->GetOpDescPtr();
    GE_ASSERT_NOTNULL(desc);
    const std::string &type = desc->GetType();
    GE_ASSERT_TRUE((type == kStageInitNodeType) || (type == kStageMainNodeType) || (type == kStageDeInitNodeType),
                   "Node %s (%s) is left in the root graph after the split", desc->GetName().c_str(), type.c_str());
  }
  // 每个阶段子图必须有自己的收尾节点，否则该阶段的执行目标无处可挂。
  for (const auto stage : {StageKind::kInit, StageKind::kMain, StageKind::kDeInit}) {
    const auto *tail = GetStageNetOutput(stage);
    GE_ASSERT_NOTNULL(tail, "Stage %s tail node is missing", StageKindToString(stage));
    auto *owner = tail->GetExtendInfo()->GetOwnerGraphBarePtr();
    GE_ASSERT_NOTNULL(owner);
    GE_ASSERT_TRUE(owner == GetStageGraph(stage), "Stage %s tail node does not belong to its stage graph",
                   StageKindToString(stage));
  }
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::RefreshDerivedMetadata() {
  // 引用端点：节点搬移不改名，RefFrom 的解析结果应与拆图前一致；重新校验是为了拦住
  // 桥接节点/容器节点引入的重名，避免运行期按名取到错误的生产者。
  GE_ASSERT_SUCCESS(ValidateRefEndpoints(work_graph_), "Ref endpoints are broken after the stage split");
  // symbol 是运行期按端点等价类推导的派生量，拆图后旧的划分不再成立，必须全部作废。
  GE_ASSERT_SUCCESS(ResetDerivedSymbols(work_graph_), "Failed to invalidate derived symbols after the stage split");
  return GRAPH_SUCCESS;
}

graphStatus StageSplitter::LinkExecutionTargets(StageKind stage) {
  auto *tail = stage_net_outputs_.at(stage);
  auto &stage_graph = stage_graphs_.at(stage);
  auto *inner_net_output = stage_inner_net_outputs_.at(stage);
  for (auto *node : stage_graph->GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    if ((node == tail) || (node == inner_net_output) || (node->GetType() == kInnerData)) {
      continue;
    }
    if (!node->GetAllOutDataEdges().empty() || !node->GetAllOutControlEdges().empty()) {
      continue;
    }
    // 无数据输出的准备/清理节点本身不产生结果，必须显式连到收尾节点，否则会被当作死节点丢弃。
    GE_ASSERT_NOTNULL(stage_graph->AddEdge(node, kControlEdgeIndex, tail, kControlEdgeIndex),
                      "Failed to register execution target %s in stage %s", NodeName(*node).c_str(),
                      StageKindToString(stage));
  }
  return GRAPH_SUCCESS;
}

ExecuteGraph *StageSplitter::GetStageGraph(StageKind stage) const {
  const auto it = stage_graphs_.find(stage);
  return (it == stage_graphs_.end()) ? nullptr : it->second.get();
}

const std::vector<StageInputBinding> &StageSplitter::GetInputBindings(StageKind stage,
                                                                      StageInputClass input_class) const {
  static const std::vector<StageInputBinding> kEmpty;
  const auto stage_it = input_bindings_.find(stage);
  if (stage_it == input_bindings_.end()) {
    return kEmpty;
  }
  const auto class_it = stage_it->second.find(input_class);
  return (class_it == stage_it->second.end()) ? kEmpty : class_it->second;
}

FastNode *StageSplitter::GetStageNetOutput(StageKind stage) const {
  const auto it = stage_net_outputs_.find(stage);
  return (it == stage_net_outputs_.end()) ? nullptr : it->second;
}

int32_t StageSplitter::GetExportPort(StageKind stage, const FastNode &producer, int32_t out_index) const {
  return ExportPortOf(stage, ValueKey{&producer, out_index});
}

graphStatus StageSplitter::Split(const StagePlan &plan,
                                 const std::vector<std::pair<FastNode *, int32_t>> &user_outputs) {
  // 同一个拆分器对象不能重复使用：阶段结果表若不清空会与上一次的节点混在一起。
  GE_ASSERT_TRUE(!split_started_, "The stage splitter has already been used, create a new one");
  // 步骤 1：校验拆分前置条件。
  GE_ASSERT_SUCCESS(ValidatePreconditions(plan, user_outputs));
  // 步骤 2：计算跨阶段值的导入、导出布局。
  GE_ASSERT_SUCCESS(BuildLayout(plan, user_outputs, layout_));
  // 步骤 3：登记并校验输入绑定。
  // 输入绑定只读 plan 和节点本身，不依赖拆完的图结构，因此放在改图之前：
  // 索引重复、越界、类别与阶段不匹配都在这里失败，工作图仍保持原样。
  GE_ASSERT_SUCCESS(RecordInputBindings(plan));
  split_started_ = true;
  // 步骤 4：创建阶段容器并搬移原节点。
  GE_ASSERT_SUCCESS(CreateStageContainers());
  GE_ASSERT_SUCCESS(MoveNodesIntoStages(plan));
  // 步骤 5：建立阶段生命周期顺序及收尾节点。
  GE_ASSERT_SUCCESS(AddStageControlOrder());
  GE_ASSERT_SUCCESS(CreateStageTailNodes(user_outputs));
  // 步骤 6：连接跨阶段值及用户输出。
  GE_ASSERT_SUCCESS(BuildCrossStageBridges(plan));
  GE_ASSERT_SUCCESS(ConnectStageOutputs(plan, user_outputs));
  // 步骤 7：登记各阶段执行目标。
  for (const auto stage : {StageKind::kInit, StageKind::kMain, StageKind::kDeInit}) {
    GE_ASSERT_SUCCESS(LinkExecutionTargets(stage));
  }
  // 步骤 8：校验拆分结果并刷新派生信息。
  GE_ASSERT_SUCCESS(ValidateSplitResult());
  GE_ASSERT_SUCCESS(RefreshDerivedMetadata());
  return GRAPH_SUCCESS;
}
}  // namespace ge
