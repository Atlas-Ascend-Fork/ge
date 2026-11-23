/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "chain_normalizer.h"

#include <algorithm>
#include <cinttypes>

#include "common/checker.h"
#include "core/builder/equivalent_data_edges.h"
#include "core/builder/node_types.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/runtime/gert_const_types.h"
#include "framework/runtime/model_v2_executor.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"

namespace gert {
namespace om2 {
namespace {
ge::ExecuteGraph *GetLifecycleGraph(ge::ExecuteGraph &root_graph, const char *const graph_type) {
  auto *const graph_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(&root_graph, graph_type);
  return graph_node == nullptr ? nullptr : ge::FastNodeUtils::GetSubgraphFromNode(graph_node, 0U);
}

ChainBindingPhase GetExecuteArgBindingPhase(const int64_t index) {
  switch (static_cast<ExecuteArgIndex>(index)) {
    case ExecuteArgIndex::kStream:
    case ExecuteArgIndex::kExternalAllocator:
      return ChainBindingPhase::kLoad;
    case ExecuteArgIndex::kRtEvents:
    case ExecuteArgIndex::kNotifies:
      return ChainBindingPhase::kRun;
    default:
      return ChainBindingPhase::kNone;
  }
}

ChainBindingKind GetExecuteArgBindingKind(const int64_t index) {
  switch (static_cast<ExecuteArgIndex>(index)) {
    case ExecuteArgIndex::kStream:
      return ChainBindingKind::kStream;
    case ExecuteArgIndex::kExternalAllocator:
      return ChainBindingKind::kExternalAllocator;
    case ExecuteArgIndex::kRtEvents:
      return ChainBindingKind::kRuntimeEvents;
    case ExecuteArgIndex::kNotifies:
      return ChainBindingKind::kNotifies;
    default:
      return ChainBindingKind::kNone;
  }
}

ChainBindingPhase GetConstDataBindingPhase(const int64_t index) {
  switch (static_cast<ConstDataType>(index)) {
    case ConstDataType::kRtSession:
    case ConstDataType::KWeight:
    case ConstDataType::kModelDescription:
      return ChainBindingPhase::kLoad;
    default:
      return ChainBindingPhase::kNone;
  }
}

ChainBindingKind GetConstDataBindingKind(const int64_t index) {
  switch (static_cast<ConstDataType>(index)) {
    case ConstDataType::kRtSession:
      return ChainBindingKind::kRtSession;
    case ConstDataType::KWeight:
      return ChainBindingKind::kOuterWeightMem;
    case ConstDataType::kModelDescription:
      return ChainBindingKind::kModelDesc;
    default:
      return ChainBindingKind::kNone;
  }
}

ge::graphStatus UpdateChainSource(ChainInfo &chain, const ChainSourceKind source_kind, const int64_t source_index,
                                  const ChainBindingPhase binding_phase, const ChainBindingKind binding_kind) {
  if (chain.source_kind != ChainSourceKind::kUninitialized) {
    if (source_kind == ChainSourceKind::kKernelOutput) {
      return ge::GRAPH_SUCCESS;
    }
    if (chain.source_kind != ChainSourceKind::kKernelOutput) {
      GE_ASSERT_TRUE(chain.source_kind == source_kind, "OM2 chain %u has conflicting sources", chain.index);
      if (source_kind == ChainSourceKind::kRuntimeBinding) {
        GE_ASSERT_TRUE(chain.source_index == source_index, "OM2 chain %u has conflicting runtime bindings",
                       chain.index);
        GE_ASSERT_TRUE(chain.binding_kind == binding_kind, "OM2 chain %u has conflicting runtime binding kinds",
                       chain.index);
      } else if (source_kind == ChainSourceKind::kModelInput || source_kind == ChainSourceKind::kModelOutput ||
                 source_kind == ChainSourceKind::kUnresolvedFeed) {
        GE_ASSERT_TRUE(chain.source_index == source_index, "OM2 chain %u has conflicting model IO indices",
                       chain.index);
      }
      return ge::GRAPH_SUCCESS;
    }
  }
  chain.source_kind = source_kind;
  chain.source_index = source_index;
  switch (source_kind) {
    case ChainSourceKind::kModelInput:
    case ChainSourceKind::kModelOutput:
      chain.binding_phase = ChainBindingPhase::kRun;
      break;
    case ChainSourceKind::kRuntimeBinding:
      chain.binding_phase = binding_phase;
      chain.binding_kind = binding_kind;
      GE_ASSERT_TRUE(chain.binding_phase != ChainBindingPhase::kNone,
                     "OM2 chain %u has no lifecycle phase for runtime source index %ld", chain.index, source_index);
      GE_ASSERT_TRUE(chain.binding_kind != ChainBindingKind::kNone,
                     "OM2 chain %u has no binding kind for runtime source index %ld", chain.index, source_index);
      break;
    case ChainSourceKind::kSessionId:
      chain.binding_phase = ChainBindingPhase::kLoad;
      chain.binding_kind = ChainBindingKind::kNone;
      break;
    case ChainSourceKind::kInputBindingRequest:
      return ge::GRAPH_PARAM_INVALID;
    case ChainSourceKind::kUnresolvedFeed:
    case ChainSourceKind::kUninitialized:
    case ChainSourceKind::kConstant:
    case ChainSourceKind::kKernelOutput:
      chain.binding_phase = ChainBindingPhase::kNone;
      chain.binding_kind = ChainBindingKind::kNone;
      break;
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus ChainNormalizer::Normalize(ge::ExecuteGraph &execute_graph, std::vector<ChainInfo> &chains,
                                           bool defer_data_bindings) {
  unresolved_feeds_.clear();
  symbols_to_chain_indices_.clear();
  const_producers_.clear();
  chains.clear();
  EquivalentDataEdges equivalent_data_edges;
  GE_ASSERT_GRAPH_SUCCESS(equivalent_data_edges.UpdateEquivalentEdges(&execute_graph));
  GE_ASSERT_GRAPH_SUCCESS(AllocateChainIndices(execute_graph, chains));
  GE_ASSERT_GRAPH_SUCCESS(RecordChainSources(execute_graph.GetAllNodes(), chains, defer_data_bindings));
  for (const auto *const node : execute_graph.GetAllNodes()) {
    if ((node == nullptr) || !IsConstType(node->GetTypePtr())) {
      continue;
    }
    for (size_t i = 0U; i < node->GetDataOutNum(); ++i) {
      uint32_t chain_index = 0U;
      GE_ASSERT_GRAPH_SUCCESS(GetChainIndex(*node, static_cast<uint32_t>(i), EndpointDirection::kOutput, chain_index));
      const auto iter = const_producers_.find(chain_index);
      GE_ASSERT_TRUE(iter == const_producers_.cend() || iter->second == node,
                     "OM2 chain %u has multiple Const producers", chain_index);
      const_producers_[chain_index] = node;
    }
  }
  return ge::GRAPH_SUCCESS;
}

bool ChainNormalizer::IsUnresolvedFeed(uint32_t chain_index) const {
  return unresolved_feeds_.count(chain_index) != 0U;
}

const ge::FastNode *ChainNormalizer::FindConstProducer(uint32_t chain_index) const {
  const auto iter = const_producers_.find(chain_index);
  return iter == const_producers_.cend() ? nullptr : iter->second;
}

ge::graphStatus ChainNormalizer::AllocateChainIndices(ge::ExecuteGraph &execute_graph, std::vector<ChainInfo> &chains) {
  GE_ASSERT_GRAPH_SUCCESS(AllocateGraphChains(execute_graph.GetDirectNode(), chains));
  constexpr const char *kLifecycleGraphTypes[] = {"Init", "Main", "DeInit"};
  for (const auto *const graph_type : kLifecycleGraphTypes) {
    auto *const graph = GetLifecycleGraph(execute_graph, graph_type);
    GE_ASSERT_NOTNULL(graph, "Failed to find OM2 lifecycle graph %s", graph_type);
    GE_ASSERT_GRAPH_SUCCESS(AllocateGraphChains(graph->GetAllNodes(), chains));
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ChainNormalizer::AllocateGraphChains(const std::vector<ge::FastNode *> &nodes,
                                                     std::vector<ChainInfo> &chains) {
  for (const auto *const node : nodes) {
    GE_ASSERT_NOTNULL(node);
    GE_ASSERT_NOTNULL(node->GetExtendInfo());
    for (size_t direction = 0U; direction < 2U; ++direction) {
      const bool is_output = direction == 0U;
      const size_t endpoint_count = is_output ? node->GetDataOutNum() : node->GetDataInNum();
      for (size_t i = 0U; i < endpoint_count; ++i) {
        const auto symbol =
            is_output ? node->GetExtendInfo()->GetOutputSymbol(i) : node->GetExtendInfo()->GetInputSymbol(i);
        if (!is_output && IsFeedType(node->GetTypePtr()) &&
            node->GetInDataEdgeByIndex(static_cast<int32_t>(i)) == nullptr) {
          // ES graph inputs use the compliant Data IR, whose required `x` input
          // is only a schema placeholder. The model input is represented by
          // the Data output chain, and feed nodes are omitted from execution.
          continue;
        }
        const auto *owner_graph = node->GetExtendInfo()->GetOwnerGraphBarePtr();
        const auto *edge = is_output ? nullptr : node->GetInDataEdgeByIndex(static_cast<int32_t>(i));
        GE_ASSERT_TRUE(symbol != ge::kInvalidSymbol,
                       "OM2 node %s(type=%s, id=%" PRId64
                       ", graph=%s) %s endpoint %zu has no RT2 symbol "
                       "(data_in=%zu, data_out=%zu, input_edge=%s)",
                       node->GetNamePtr(), node->GetTypePtr(), node->GetOpDescBarePtr()->GetId(),
                       owner_graph == nullptr ? "<null>" : owner_graph->GetName().c_str(),
                       is_output ? "output" : "input", i, node->GetDataInNum(), node->GetDataOutNum(),
                       edge == nullptr ? "null" : edge->src->GetNamePtr());
        if (symbols_to_chain_indices_.count(symbol) > 0U) {
          continue;
        }
        GE_ASSERT_TRUE(chains.size() < static_cast<size_t>(kInvalidProgramIndex));
        const auto chain_index = static_cast<uint32_t>(chains.size());
        symbols_to_chain_indices_[symbol] = chain_index;
        chains.push_back({chain_index, symbol, ChainSourceKind::kUninitialized, ChainBindingPhase::kNone,
                          ChainBindingKind::kNone, kInvalidChainSourceIndex});
      }
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ChainNormalizer::RecordChainSources(const std::vector<ge::FastNode *> &nodes,
                                                    std::vector<ChainInfo> &chains, bool defer_data_bindings) {
  // Legacy GE graphs use OutputData itself as the model-output marker. FXRT
  // graphs add an explicit Output/NetOutput node and use OutputData as tensor
  // shells. Select the convention per graph to preserve both routes.
  const bool has_explicit_model_output = std::any_of(nodes.cbegin(), nodes.cend(), [](const ge::FastNode *node) {
    return (node != nullptr) && (IsOutputType(node->GetTypePtr()) || IsTypeNetOutput(node->GetTypePtr()));
  });
  for (const auto *const node : nodes) {
    GE_ASSERT_NOTNULL(node);
    const auto *const op_desc = node->GetOpDescBarePtr();
    GE_ASSERT_NOTNULL(op_desc);
    ChainSourceKind source_kind = ChainSourceKind::kKernelOutput;
    int64_t source_index = kInvalidChainSourceIndex;
    ChainBindingPhase binding_phase = ChainBindingPhase::kNone;
    ChainBindingKind binding_kind = ChainBindingKind::kNone;
    if (IsConstType(node->GetTypePtr())) {
      source_kind = ChainSourceKind::kConstant;
    } else if (IsFeedType(node->GetTypePtr())) {
      GE_ASSERT_TRUE(ge::AttrUtils::GetInt(op_desc, kFeedIndex, source_index), "OM2 Data node %s has no feed index",
                     node->GetNamePtr());
      // Legacy negative indices encode ExecuteArgIndex. External nonnegative
      // feeds keep their original index until their consumers have been collected.
      source_kind = (source_index < 0) ? ChainSourceKind::kRuntimeBinding : ChainSourceKind::kModelInput;
      if (defer_data_bindings && source_kind == ChainSourceKind::kModelInput) {
        source_kind = ChainSourceKind::kUnresolvedFeed;
      }
      if (source_kind == ChainSourceKind::kRuntimeBinding) {
        binding_phase = GetExecuteArgBindingPhase(source_index);
        binding_kind = GetExecuteArgBindingKind(source_index);
      }
    } else if (IsConstFeedType(node->GetTypePtr())) {
      GE_ASSERT_TRUE(ge::AttrUtils::GetInt(op_desc, kFeedIndex, source_index),
                     "OM2 ConstData node %s has no feed index", node->GetNamePtr());
      source_kind = ChainSourceKind::kRuntimeBinding;
      binding_phase = GetConstDataBindingPhase(source_index);
      binding_kind = GetConstDataBindingKind(source_index);
    } else if (IsTypeOutputData(node->GetTypePtr())) {
      // OutputData is also used as a shell for intermediate tensors by FXRT.
      // The model-output ownership and port index are recorded from NetOutput
      // below, so every OutputData output starts as an ordinary kernel output.
      source_kind = has_explicit_model_output ? ChainSourceKind::kKernelOutput : ChainSourceKind::kModelOutput;
    } else if (IsOutputType(node->GetTypePtr()) || IsTypeNetOutput(node->GetTypePtr())) {
      // A NetOutput input position is the authoritative model-output port.
      // Do not infer it from OutputData's local output index: intermediate
      // OutputData shells commonly have one output and would all become index 0.
      for (size_t i = 0U; i < node->GetDataInNum(); ++i) {
        uint32_t chain_index = 0U;
        GE_ASSERT_GRAPH_SUCCESS(GetChainIndex(*node, static_cast<uint32_t>(i), EndpointDirection::kInput, chain_index));
        GE_ASSERT_GRAPH_SUCCESS(UpdateChainSource(chains[chain_index], ChainSourceKind::kModelOutput,
                                                  static_cast<int64_t>(i), ChainBindingPhase::kRun,
                                                  ChainBindingKind::kNone));
      }
      continue;
    }
    for (size_t i = 0U; i < node->GetDataOutNum(); ++i) {
      uint32_t chain_index = 0U;
      GE_ASSERT_GRAPH_SUCCESS(GetChainIndex(*node, static_cast<uint32_t>(i), EndpointDirection::kOutput, chain_index));
      auto &chain = chains[chain_index];
      GE_ASSERT_GRAPH_SUCCESS(UpdateChainSource(chain, source_kind, source_index, binding_phase, binding_kind));
      if (source_kind == ChainSourceKind::kUnresolvedFeed) {
        unresolved_feeds_.insert(chain_index);
      }
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ChainNormalizer::GetInputChainIndices(const ge::FastNode &node, std::vector<uint32_t> &indices) const {
  indices.clear();
  indices.reserve(node.GetDataInNum());
  for (size_t i = 0U; i < node.GetDataInNum(); ++i) {
    uint32_t chain_index = 0U;
    GE_ASSERT_GRAPH_SUCCESS(GetChainIndex(node, static_cast<uint32_t>(i), EndpointDirection::kInput, chain_index));
    indices.push_back(chain_index);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ChainNormalizer::GetOutputChainIndices(const ge::FastNode &node, std::vector<uint32_t> &indices) const {
  indices.clear();
  indices.reserve(node.GetDataOutNum());
  for (size_t i = 0U; i < node.GetDataOutNum(); ++i) {
    uint32_t chain_index = 0U;
    GE_ASSERT_GRAPH_SUCCESS(GetChainIndex(node, static_cast<uint32_t>(i), EndpointDirection::kOutput, chain_index));
    indices.push_back(chain_index);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ChainNormalizer::GetChainIndex(const ge::FastNode &node, uint32_t index, EndpointDirection direction,
                                               uint32_t &chain_index) const {
  GE_ASSERT_NOTNULL(node.GetExtendInfo());
  const bool is_output = direction == EndpointDirection::kOutput;
  const auto endpoint_count = is_output ? node.GetDataOutNum() : node.GetDataInNum();
  GE_ASSERT_TRUE(index < endpoint_count, "OM2 node %s endpoint %u is out of range", node.GetNamePtr(), index);
  const auto symbol =
      is_output ? node.GetExtendInfo()->GetOutputSymbol(index) : node.GetExtendInfo()->GetInputSymbol(index);
  GE_ASSERT_TRUE(symbol != ge::kInvalidSymbol, "OM2 node %s endpoint %u has no RT2 symbol", node.GetNamePtr(), index);
  const auto iter = symbols_to_chain_indices_.find(symbol);
  GE_ASSERT_TRUE(iter != symbols_to_chain_indices_.cend(), "No chain for node %s endpoint %u", node.GetNamePtr(),
                 index);
  chain_index = iter->second;
  return ge::GRAPH_SUCCESS;
}
}  // namespace om2
}  // namespace gert
