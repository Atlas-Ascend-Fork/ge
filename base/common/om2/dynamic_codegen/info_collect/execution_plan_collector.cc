/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "execution_plan_collector.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "common/ge_inner_attrs.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "core/builder/node_types.h"
#include "execution_order_collector.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/scope_guard.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/op_kernel_bin.h"
#include "kernel_code_builder.h"
#include "register/kernel_registry.h"

namespace gert {
namespace om2 {
namespace {
struct KernelBinDataView {
  uint32_t magic;
  uint32_t version;
  const void *data;
  uint64_t length;
  uint64_t placeholder;
};

ge::graphStatus MaterializeConstOutput(const ge::FastNode &producer, gert::Chain &output_chain) {
  const auto *const kernel_funcs = KernelRegistry::GetInstance().FindKernelFuncs(producer.GetTypePtr());
  GE_ASSERT_NOTNULL(kernel_funcs, "No kernel functions registered for OM2 Const producer %s", producer.GetNamePtr());
  GE_ASSERT_NOTNULL(kernel_funcs->outputs_creator, "No outputs creator registered for OM2 Const producer %s",
                    producer.GetNamePtr());

  KernelContext context{};
  auto *const run_context = context.GetContext();
  run_context->input_size = 0U;
  run_context->output_size = 1U;
  run_context->output_start = run_context->values;
  run_context->values[0] = reinterpret_cast<AsyncAnyValue *>(&output_chain);
  return kernel_funcs->outputs_creator(&producer, &context);
}

std::string GetBinaryResourceKey(const gert::Chain &chain, const Om2ValueType value_type) {
  if (value_type == Om2ValueType::kGeKernelBinPtr) {
    const auto *const binary = chain.GetValue<const ge::OpKernelBinPtr *>();
    if ((binary == nullptr) || (*binary == nullptr) || ((*binary)->GetBinDataSize() == 0U)) {
      return {};
    }
    return ::gert::om2::GetKernelBinaryKey((*binary)->GetBinData(), (*binary)->GetBinDataSize());
  }
  const auto *const bin_data = chain.GetValue<const KernelBinDataView *>();
  if (bin_data == nullptr || bin_data->length == 0U) {
    return {};
  }
  return ::gert::om2::GetKernelBinaryKey(&bin_data->placeholder, static_cast<size_t>(bin_data->length));
}

bool IsExecutableKernel(const ge::FastNode &node) {
  const auto *const type = node.GetTypePtr();
  return !(IsFeedType(type) || IsConstFeedType(type) || IsOutputType(type) || IsTypeOutputData(type) ||
           IsInnerDataType(type) || IsInnerOutput(type) || IsIfOrCaseType(type) || IsWhileType(type) ||
           IsSubgraphCall(type) || IsConstType(type));
}

bool IsControlSkeletonNode(const char *const type) {
  return IsSwitchNotifyNode(type) || IsBranchPivot(type) || IsBranchDone(type) || IsWaitAnyone(type) ||
         IsWatcherPlaceholderNode(type) || IsCondSwitchNotifyNode(type) || (strcmp(type, "Enter") == 0) ||
         (strcmp(type, "Exit") == 0);
}

bool IsConditionalNode(const char *const type) {
  return IsIfOrCaseType(type);
}

void CollectChainDependencies(ProgramBlock &program_block) {
  std::unordered_map<uint32_t, std::vector<uint32_t>> producers_by_chain;
  std::set<std::pair<uint32_t, uint32_t>> dependencies;
  for (uint32_t kernel_index = 0U; kernel_index < program_block.kernels.size(); ++kernel_index) {
    if (program_block.kernels[kernel_index].impl_type == KernelImplType::kEmptyImpl) {
      // EmptyImpl kernels (e.g. GetSessionId) publish Load-bound values; consumers do not
      // depend on their execution order. Keeping their edges would force the multithread
      // plan to fail back to serial because they never appear in any root step.
      continue;
    }
    for (const auto &output : program_block.kernels[kernel_index].outputs) {
      producers_by_chain[output.chain_index].push_back(kernel_index);
    }
  }
  for (uint32_t kernel_index = 0U; kernel_index < program_block.kernels.size(); ++kernel_index) {
    if (program_block.kernels[kernel_index].impl_type == KernelImplType::kEmptyImpl) {
      continue;
    }
    for (const auto &input : program_block.kernels[kernel_index].inputs) {
      const auto producer = producers_by_chain.find(input.chain_index);
      if (producer == producers_by_chain.cend()) {
        continue;
      }
      for (const uint32_t predecessor : producer->second) {
        if (predecessor != kernel_index) {
          dependencies.emplace(predecessor, kernel_index);
        }
      }
    }
  }
  for (const auto &dependency : dependencies) {
    program_block.kernel_dependencies.push_back({dependency.first, dependency.second});
  }
}

// RT2 does not schedule graph plumbing nodes as additional control
// dependencies.  They are represented by the executable nodes on either side
// of the boundary (or by the structured region itself).
bool IsAdditionalDependencyNode(const ge::FastNode &node) {
  const auto *const type = node.GetTypePtr();
  return !(IsFeedType(type) || IsConstFeedType(type) || IsOutputType(type) || IsTypeOutputData(type) ||
           IsInnerDataType(type) || IsInnerOutput(type) || IsConstType(type));
}

ExecutionOrderPolicy GetExecutionOrderPolicy(const ProgramPlanType plan_type) {
  return plan_type == ProgramPlanType::kStructuredTopological ? ExecutionOrderPolicy::kFifo
                                                              : ExecutionOrderPolicy::kPriority;
}

ge::graphStatus GetBranchIndex(const ge::FastNode &node, const size_t branch_count, uint32_t &branch_index) {
  int64_t value = -1;
  GE_ASSERT_TRUE(ge::AttrUtils::GetInt(node.GetOpDescBarePtr(), ge::kRelativeBranch, value),
                 "OM2 canonical If control node %s has no branch attribute", node.GetNamePtr());
  GE_ASSERT_TRUE((value > 0) && (static_cast<size_t>(value) <= branch_count),
                 "OM2 canonical If control node %s has invalid branch %ld for %zu branches", node.GetNamePtr(), value,
                 branch_count);
  branch_index = static_cast<uint32_t>(value);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetSwitchOutputIndex(const ge::FastNode &pivot, const ge::FastNode &switch_notify,
                                     uint32_t &output_index) {
  const ge::FastEdge *matched = nullptr;
  for (const auto *const edge : pivot.GetAllInDataEdgesRef()) {
    if ((edge == nullptr) || (edge->dst_input != 0)) {
      continue;
    }
    GE_ASSERT_TRUE(matched == nullptr, "OM2 branch pivot %s has multiple input-0 edges", pivot.GetNamePtr());
    matched = edge;
  }
  GE_ASSERT_NOTNULL(matched, "OM2 branch pivot %s has no input-0 edge", pivot.GetNamePtr());
  GE_ASSERT_TRUE((matched->src == &switch_notify) && (matched->src_output > 0),
                 "OM2 branch pivot %s is not driven by a SwitchNotify branch output", pivot.GetNamePtr());
  output_index = static_cast<uint32_t>(matched->src_output);
  return ge::GRAPH_SUCCESS;
}

template <typename Predicate>
ge::FastNode *FindUniqueNode(ge::ExecuteGraph &graph, Predicate predicate) {
  ge::FastNode *matched = nullptr;
  for (auto *const node : graph.GetDirectNode()) {
    if ((node != nullptr) && predicate(*node)) {
      GE_ASSERT_TRUE(matched == nullptr, "OM2 canonical control graph %s contains duplicate control nodes",
                     graph.GetName().c_str());
      matched = node;
    }
  }
  return matched;
}

bool IsZeroIndegreeBranchNode(const ge::FastNode &node) {
  return IsInnerDataType(node.GetTypePtr()) || IsConstType(node.GetTypePtr());
}

bool IsGuardedByPivot(const ge::FastNode &node) {
  if (IsZeroIndegreeBranchNode(node)) {
    return false;
  }
  const auto has_nonzero_source = [](const ge::FastEdge *const edge) {
    return (edge != nullptr) && !IsZeroIndegreeBranchNode(*edge->src);
  };
  for (const auto *const edge : node.GetAllInDataEdgesRef()) {
    if (has_nonzero_source(edge)) {
      return false;
    }
  }
  for (const auto *const edge : node.GetAllInControlEdgesRef()) {
    if (has_nonzero_source(edge)) {
      return false;
    }
  }
  return true;
}

bool HasControlEdge(const ge::FastNode &src, const ge::FastNode &dst) {
  for (const auto *const edge : dst.GetAllInControlEdgesRef()) {
    if ((edge != nullptr) && (edge->src == &src)) {
      return true;
    }
  }
  return false;
}

void AddVirtualDependency(std::vector<VirtualDependency> &dependencies, ge::FastNode *const src,
                          ge::FastNode *const dst) {
  if ((src == nullptr) || (dst == nullptr) || (src == dst)) {
    return;
  }
  // Keep every effective RT2 AddAdditionalInfo event.  The insertion order is
  // observable because virtual edges are consumed by the FIFO ready queue.
  dependencies.push_back({src, dst});
}

}  // namespace

struct ExecutionPlanCollector::CanonicalIfAnalysis {
  ge::ExecuteGraph *condition_graph = nullptr;
  ge::FastNode *switch_notify = nullptr;
  ge::FastNode *wait_anyone = nullptr;
  std::vector<uint32_t> selector_to_subgraph;
  std::vector<std::string> branch_entry_node_names;
  std::vector<std::string> branch_exit_node_names;
  std::vector<std::vector<std::string>> guarded_entry_node_names;
  std::vector<std::vector<std::string>> completion_node_names;
  std::string join_node_name;
  std::vector<std::string> parent_predecessor_node_names;
  std::vector<std::string> parent_continuation_node_names;
};

ge::graphStatus ExecutionPlanCollector::Collect(ge::ExecuteGraph &execute_graph,
                                                const ChainNormalizer &chain_normalizer, ProgramPlanType plan_type,
                                                ProgramBlock &program_block) const {
  program_block = {};
  program_block.plan_type = plan_type;
  program_block.root_region_index = 0U;
  program_block.regions.push_back({0U, execute_graph.GetName(), {}});
  switch (plan_type) {
    case ProgramPlanType::kSequential:
      return CollectSequential(execute_graph, chain_normalizer, program_block);
    case ProgramPlanType::kStructuredTopological:
    case ProgramPlanType::kStructuredTopologicalPriority: {
      const auto ret = CollectRegion(execute_graph, chain_normalizer, 0U, program_block);
      if (ret == ge::GRAPH_SUCCESS) {
        CollectChainDependencies(program_block);
      }
      return ret;
    }
    default:
      break;
  }
  GELOGE(ge::GRAPH_FAILED, "Unknown OM2 plan type for graph %s", execute_graph.GetName().c_str());
  return ge::GRAPH_FAILED;
}

ge::graphStatus ExecutionPlanCollector::CollectSequential(ge::ExecuteGraph &execute_graph,
                                                          const ChainNormalizer &chain_normalizer,
                                                          ProgramBlock &program_block) const {
  std::vector<ge::FastNode *> ordered_nodes;
  std::unordered_map<const ge::FastNode *, uint32_t> nodes_to_kernel_indices;
  std::unordered_map<uint32_t, uint32_t> output_chains_to_kernel_indices;
  std::set<std::pair<uint32_t, uint32_t>> dependencies;
  const std::vector<VirtualDependency> virtual_dependencies;
  // kSequential comes from RT2 kSequentialPriority, whose online builder orders nodes by
  // ReadInNodesByPriority. Align with it so critical-path kernels run first in both modes;
  // nodes without a "priority" attr keep the default (max) and stay in FIFO relative order.
  GE_ASSERT_GRAPH_SUCCESS(ExecutionOrderCollector().Collect(execute_graph, virtual_dependencies,
                                                            ExecutionOrderPolicy::kPriority, ordered_nodes));
  for (const auto *const node : ordered_nodes) {
    const auto *const type = node->GetTypePtr();
    if (IsIfOrCaseType(type) || IsWhileType(type) || IsSubgraphCall(type) || IsControlSkeletonNode(type)) {
      GELOGE(ge::GRAPH_FAILED, "OM2 sequential graph %s contains control node %s type %s",
             execute_graph.GetName().c_str(), node->GetNamePtr(), type);
      return ge::GRAPH_FAILED;
    }
    if (IsExecutableKernel(*node)) {
      uint32_t kernel_index = kInvalidProgramIndex;
      GE_ASSERT_GRAPH_SUCCESS(AppendKernel(*node, chain_normalizer, 0U, program_block, &kernel_index));
      if (kernel_index != kInvalidProgramIndex) {
        nodes_to_kernel_indices[node] = kernel_index;
        const bool is_empty_impl = program_block.kernels[kernel_index].impl_type == KernelImplType::kEmptyImpl;
        if (!is_empty_impl) {
          for (const auto &input : program_block.kernels[kernel_index].inputs) {
            const auto producer = output_chains_to_kernel_indices.find(input.chain_index);
            if ((producer != output_chains_to_kernel_indices.cend()) && (producer->second != kernel_index)) {
              dependencies.emplace(producer->second, kernel_index);
            }
          }
          for (const auto *const edge : node->GetAllInControlEdgesRef()) {
            if (edge == nullptr) {
              continue;
            }
            const auto producer = nodes_to_kernel_indices.find(edge->src);
            if ((producer != nodes_to_kernel_indices.cend()) && (producer->second != kernel_index)) {
              dependencies.emplace(producer->second, kernel_index);
            }
          }
          for (const auto &output : program_block.kernels[kernel_index].outputs) {
            output_chains_to_kernel_indices[output.chain_index] = kernel_index;
          }
        }
      }
    }
  }
  for (const auto &dependency : dependencies) {
    program_block.kernel_dependencies.push_back({dependency.first, dependency.second});
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::CollectKernel(const ge::FastNode &node, const ChainNormalizer &chain_normalizer,
                                                      ProgramBlock &program_block, uint32_t &kernel_index) const {
  // om2动态当前限制：Compare CustomOp partitioning/lowering with experiments and define collector support
  // for direct ExecuteCustomOp/ExecuteHostCustomOp paths. Missing collectors here do not imply
  // that CustomOps executed through DavinciModel static subgraphs are unsupported.
  auto builder = KernelCodeBuilderFactory::GetInstance().Create(node.GetTypePtr());
  if (builder == nullptr) {
    GELOGE(ge::GRAPH_FAILED, "No OM2 kernel code builder registered for kernel %s, execution node %s",
           node.GetTypePtr(), node.GetNamePtr());
    return ge::GRAPH_FAILED;
  }
  std::vector<uint32_t> input_chain_indices;
  std::vector<uint32_t> output_chain_indices;
  GE_ASSERT_GRAPH_SUCCESS(chain_normalizer.GetInputChainIndices(node, input_chain_indices));
  GE_ASSERT_GRAPH_SUCCESS(chain_normalizer.GetOutputChainIndices(node, output_chain_indices));
  std::vector<gert::Chain> input_chains(node.GetDataInNum());
  std::vector<gert::Chain> output_chains(node.GetDataOutNum());
  const ge::ScopeGuard chain_guard([&input_chains, &output_chains]() {
    for (auto &chain : input_chains) {
      chain.Set(nullptr, nullptr);
    }
    for (auto &chain : output_chains) {
      chain.Set(nullptr, nullptr);
    }
  });
  const auto materialize_input = [&node, &input_chains, &chain_normalizer,
                                  &input_chain_indices](const uint32_t input_index) -> ge::graphStatus {
    GE_ASSERT_TRUE(input_index < node.GetDataInNum());
    const ge::FastEdge *producer_edge = nullptr;
    for (const auto *const edge : node.GetAllInDataEdgesRef()) {
      if ((edge != nullptr) && (edge->dst_input == static_cast<int32_t>(input_index))) {
        GE_ASSERT_TRUE(producer_edge == nullptr, "OM2 kernel %s input %u has multiple producers", node.GetNamePtr(),
                       input_index);
        producer_edge = edge;
      }
    }
    GE_ASSERT_NOTNULL(producer_edge, "OM2 kernel %s input %u has no producer", node.GetNamePtr(), input_index);
    const ge::FastNode *producer = producer_edge->src;
    int32_t producer_output = producer_edge->src_output;
    if (!IsConstType(producer->GetTypePtr())) {
      producer = chain_normalizer.FindConstProducer(input_chain_indices[input_index]);
      producer_output = 0;
    }
    GE_ASSERT_NOTNULL(producer, "OM2 kernel %s input %u has no Const producer", node.GetNamePtr(), input_index);
    GE_ASSERT_TRUE(IsConstType(producer->GetTypePtr()), "OM2 kernel %s input %u must be driven by Const",
                   node.GetNamePtr(), input_index);
    GE_ASSERT_TRUE((producer_output == 0) && (producer->GetDataOutNum() == 1U),
                   "OM2 Const producer %s has invalid output index %d", producer->GetNamePtr(), producer_output);
    return MaterializeConstOutput(*producer, input_chains[input_index]);
  };
  const auto ordinary_restore_input_indices = builder->GetRestoreConstInputIndices();
  const auto custom_restore_input_indices = builder->GetCustomRestoreConstInputIndices();
  std::vector<uint32_t> restore_input_indices = ordinary_restore_input_indices;
  for (const auto custom_input_index : custom_restore_input_indices) {
    GE_ASSERT_TRUE((custom_input_index != kKernelLastInputIndex) || (node.GetDataInNum() > 0U),
                   "OM2 kernel %s has no input for last-index custom restorer", node.GetNamePtr());
    const auto resolved_index =
        custom_input_index == kKernelLastInputIndex ? node.GetDataInNum() - 1U : custom_input_index;
    if (std::find(restore_input_indices.cbegin(), restore_input_indices.cend(), resolved_index) ==
        restore_input_indices.cend()) {
      restore_input_indices.push_back(resolved_index);
    }
  }
  std::sort(restore_input_indices.begin(), restore_input_indices.end());
  for (const auto input_index : restore_input_indices) {
    GE_ASSERT_TRUE(input_index < node.GetDataInNum(), "OM2 kernel %s const input index %u is out of range",
                   node.GetNamePtr(), input_index);
    GE_ASSERT_GRAPH_SUCCESS(materialize_input(input_index), "Failed to materialize OM2 kernel %s const input %u",
                            node.GetNamePtr(), input_index);
  }
  KernelProgramInfo kernel_info;
  const KernelCollectContext context{node, input_chain_indices, output_chain_indices, input_chains, output_chains};
  GE_ASSERT_GRAPH_SUCCESS(builder->Collect(context, kernel_info), "Failed to collect OM2 kernel %s type %s",
                          node.GetNamePtr(), node.GetTypePtr());
  for (size_t input_index = 0U; input_index < kernel_info.inputs.size(); ++input_index) {
    auto &port = kernel_info.inputs[input_index];
    if ((port.value_type != Om2ValueType::kGertKernelBinDataPtr) &&
        (port.value_type != Om2ValueType::kGeKernelBinPtr)) {
      continue;
    }
    const auto chain_iter = std::find(input_chain_indices.cbegin(), input_chain_indices.cend(), port.chain_index);
    GE_ASSERT_TRUE(chain_iter != input_chain_indices.cend(), "OM2 kernel %s binary input has unknown chain %u",
                   node.GetNamePtr(), port.chain_index);
    const auto resolved_input_index = static_cast<uint32_t>(std::distance(input_chain_indices.cbegin(), chain_iter));
    GE_ASSERT_GRAPH_SUCCESS(materialize_input(resolved_input_index), "Failed to materialize OM2 kernel binary input %u",
                            resolved_input_index);
    const auto resource_key = GetBinaryResourceKey(input_chains[resolved_input_index], port.value_type);
    GE_ASSERT_TRUE(!resource_key.empty(), "OM2 kernel %s binary input %zu has no binary payload", node.GetNamePtr(),
                   resolved_input_index);
    kernel_info.binary_resource_keys[port.chain_index] = resource_key;
  }
  for (const auto custom_input_index : custom_restore_input_indices) {
    GE_ASSERT_TRUE((custom_input_index != kKernelLastInputIndex) || !input_chain_indices.empty(),
                   "OM2 kernel %s has no input for last-index custom restorer", node.GetNamePtr());
    const auto resolved_index =
        custom_input_index == kKernelLastInputIndex ? input_chain_indices.size() - 1U : custom_input_index;
    GE_ASSERT_TRUE(resolved_index < input_chain_indices.size(),
                   "OM2 kernel %s custom const input index %u is out of range", node.GetNamePtr(), resolved_index);
    CustomConstRestoreDescriptor descriptor;
    GE_ASSERT_GRAPH_SUCCESS(builder->RestoreConstInput(context, resolved_index, descriptor),
                            "Failed to restore custom OM2 kernel %s const input %u", node.GetNamePtr(), resolved_index);
    descriptor.chain_index = input_chain_indices[resolved_index];
    const ge::FastEdge *producer_edge = nullptr;
    for (const auto *const edge : node.GetAllInDataEdgesRef()) {
      if ((edge != nullptr) && (edge->dst_input == static_cast<int32_t>(resolved_index))) {
        producer_edge = edge;
        break;
      }
    }
    GE_ASSERT_NOTNULL(producer_edge);
    const auto *const const_producer = chain_normalizer.FindConstProducer(input_chain_indices[resolved_index]);
    descriptor.producer_name =
        const_producer == nullptr ? producer_edge->src->GetNamePtr() : const_producer->GetNamePtr();
    GE_ASSERT_TRUE(!descriptor.initializer.empty(), "OM2 kernel %s custom const input %u has no initializer",
                   node.GetNamePtr(), resolved_index);
    GE_ASSERT_TRUE(!descriptor.payload.empty(), "OM2 kernel %s custom const input %u has no payload", node.GetNamePtr(),
                   resolved_index);
    kernel_info.custom_const_restore_descriptors.emplace_back(std::move(descriptor));
  }
  kernel_info.impl_type = builder->GetImplType();
  kernel_info.code_builder = builder;
  if (kernel_info.impl_type == KernelImplType::kCodegenImpl) {
    kernel_info.compile_time_const_chain_indices.reserve(ordinary_restore_input_indices.size());
    for (const auto input_index : ordinary_restore_input_indices) {
      GE_ASSERT_TRUE(input_index < input_chain_indices.size());
      const bool has_custom_restorer =
          std::any_of(custom_restore_input_indices.cbegin(), custom_restore_input_indices.cend(),
                      [input_index, input_count = input_chain_indices.size()](const auto custom_index) {
                        return (custom_index == input_index) ||
                               ((custom_index == kKernelLastInputIndex) && (input_count - 1U == input_index));
                      });
      if (has_custom_restorer) {
        continue;
      }
      kernel_info.compile_time_const_chain_indices.push_back(input_chain_indices[input_index]);
    }
  }
  kernel_index = static_cast<uint32_t>(program_block.kernels.size());
  program_block.kernels.emplace_back(std::move(kernel_info));
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::CollectRegion(ge::ExecuteGraph &execute_graph,
                                                      const ChainNormalizer &chain_normalizer, uint32_t region_index,
                                                      ProgramBlock &program_block) const {
  std::unordered_map<ge::FastNode *, CanonicalIfAnalysis> if_analyses;
  std::vector<VirtualDependency> virtual_dependencies;
  for (auto *const node : execute_graph.GetDirectNode()) {
    if (!IsConditionalNode(node->GetTypePtr())) {
      continue;
    }
    CanonicalIfAnalysis analysis;
    GE_ASSERT_GRAPH_SUCCESS(AnalyzeIf(*node, analysis));
    // Contract the RT2 SwitchNotify/BranchPivot/BranchDone/WaitAnyone frame into the parent-region If step.
    for (auto *const predecessor : node->GetAllInNodes()) {
      if (IsAdditionalDependencyNode(*predecessor)) {
        AddVirtualDependency(virtual_dependencies, predecessor, node);
      }
    }
    for (auto *const continuation : node->GetAllOutNodes()) {
      if (IsAdditionalDependencyNode(*continuation)) {
        AddVirtualDependency(virtual_dependencies, node, continuation);
      }
    }
    if_analyses.emplace(node, std::move(analysis));
  }

  std::vector<ge::FastNode *> ordered_nodes;
  GE_ASSERT_GRAPH_SUCCESS(ExecutionOrderCollector().Collect(
      execute_graph, virtual_dependencies, GetExecutionOrderPolicy(program_block.plan_type), ordered_nodes));
  std::unordered_map<const ge::FastNode *, uint32_t> nodes_to_kernel_indices;
  for (auto *const node : ordered_nodes) {
    const auto *const type = node->GetTypePtr();
    if (IsConditionalNode(type)) {
      GE_ASSERT_GRAPH_SUCCESS(AppendIf(*node, chain_normalizer, if_analyses.at(node), region_index, program_block));
      continue;
    }
    if (IsWhileType(type)) {
      return AppendWhile(*node, execute_graph);
    }
    if (IsSubgraphCall(type)) {
      return AppendSubgraphCall(*node, execute_graph);
    }
    if (IsControlSkeletonNode(type)) {
      continue;
    }
    if (IsExecutableKernel(*node)) {
      uint32_t kernel_index = kInvalidProgramIndex;
      GE_ASSERT_GRAPH_SUCCESS(AppendKernel(*node, chain_normalizer, region_index, program_block, &kernel_index));
      if (kernel_index != kInvalidProgramIndex) {
        // Region-local control edges must reach kernel_dependencies: data edges are covered by
        // CollectChainDependencies, but control edges (order-only constraints from lowering, e.g.
        // guarded frees) would otherwise be lost once kernels run concurrently on lanes.
        if (program_block.kernels[kernel_index].impl_type != KernelImplType::kEmptyImpl) {
          for (const auto *const edge : node->GetAllInControlEdgesRef()) {
            if (edge == nullptr) {
              continue;
            }
            const auto producer = nodes_to_kernel_indices.find(edge->src);
            if ((producer != nodes_to_kernel_indices.cend()) && (producer->second != kernel_index)) {
              program_block.kernel_dependencies.push_back({producer->second, kernel_index});
            }
          }
        }
        nodes_to_kernel_indices[node] = kernel_index;
      }
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::AppendKernel(const ge::FastNode &node, const ChainNormalizer &chain_normalizer,
                                                     uint32_t region_index, ProgramBlock &program_block,
                                                     uint32_t *appended_kernel_index) const {
  if (appended_kernel_index != nullptr) {
    *appended_kernel_index = kInvalidProgramIndex;
  }
  uint32_t kernel_index = kInvalidProgramIndex;
  GE_ASSERT_GRAPH_SUCCESS(CollectKernel(node, chain_normalizer, program_block, kernel_index));
  if (program_block.kernels[kernel_index].impl_type == KernelImplType::kEmptyImpl) {
    return ge::GRAPH_SUCCESS;
  }
  ProgramStep step;
  step.kind = ProgramStepKind::kKernel;
  step.kernel_index = kernel_index;
  program_block.regions[region_index].steps.emplace_back(std::move(step));
  if (appended_kernel_index != nullptr) {
    *appended_kernel_index = kernel_index;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::AnalyzeIf(ge::FastNode &node, CanonicalIfAnalysis &analysis) const {
  const auto *const op_desc = node.GetOpDescBarePtr();
  GE_ASSERT_NOTNULL(op_desc);
  GE_ASSERT_TRUE(IsConditionalNode(node.GetTypePtr()));
  GE_ASSERT_TRUE(op_desc->GetSubgraphInstanceNames().size() > 1U,
                 "OM2 canonical If/Case node %s has no branch subgraphs", node.GetNamePtr());
  const auto branch_count = op_desc->GetSubgraphInstanceNames().size() - 1U;
  auto *const cond_graph = ge::FastNodeUtils::GetSubgraphFromNode(&node, 0U);
  GE_ASSERT_NOTNULL(cond_graph);
  auto *const switch_notify = FindUniqueNode(
      *cond_graph, [](const ge::FastNode &candidate) { return IsSwitchNotifyNode(candidate.GetTypePtr()); });
  auto *const wait_anyone =
      FindUniqueNode(*cond_graph, [](const ge::FastNode &candidate) { return IsWaitAnyone(candidate.GetTypePtr()); });
  GE_ASSERT_NOTNULL(switch_notify);
  GE_ASSERT_NOTNULL(wait_anyone);

  std::map<uint32_t, ge::FastNode *> pivots;
  std::map<uint32_t, ge::FastNode *> dones;
  analysis = {};
  analysis.condition_graph = cond_graph;
  analysis.switch_notify = switch_notify;
  analysis.wait_anyone = wait_anyone;
  analysis.selector_to_subgraph.assign(branch_count, kInvalidProgramIndex);
  analysis.branch_entry_node_names.resize(branch_count);
  analysis.branch_exit_node_names.resize(branch_count);
  analysis.guarded_entry_node_names.resize(branch_count);
  analysis.completion_node_names.resize(branch_count);
  analysis.join_node_name = wait_anyone->GetNamePtr();
  for (auto *const predecessor : node.GetAllInNodes()) {
    analysis.parent_predecessor_node_names.emplace_back(predecessor->GetNamePtr());
  }
  for (auto *const continuation : node.GetAllOutNodes()) {
    analysis.parent_continuation_node_names.emplace_back(continuation->GetNamePtr());
  }
  for (auto *const control_node : cond_graph->GetDirectNode()) {
    if (IsBranchPivot(control_node->GetTypePtr())) {
      uint32_t branch_index = 0U;
      GE_ASSERT_GRAPH_SUCCESS(GetBranchIndex(*control_node, branch_count, branch_index));
      GE_ASSERT_TRUE(pivots.emplace(branch_index, control_node).second);
      uint32_t switch_output = 0U;
      GE_ASSERT_GRAPH_SUCCESS(GetSwitchOutputIndex(*control_node, *switch_notify, switch_output));
      GE_ASSERT_TRUE((switch_output > 0U) && (switch_output <= branch_count));
      const auto selector = switch_output - 1U;
      GE_ASSERT_TRUE(analysis.selector_to_subgraph[selector] == kInvalidProgramIndex,
                     "OM2 If/Case node %s has multiple branches for selector %u", node.GetNamePtr(), selector);
      analysis.selector_to_subgraph[selector] = branch_index;
      analysis.branch_entry_node_names[selector] = control_node->GetNamePtr();
    } else if (IsBranchDone(control_node->GetTypePtr())) {
      uint32_t branch_index = 0U;
      GE_ASSERT_GRAPH_SUCCESS(GetBranchIndex(*control_node, branch_count, branch_index));
      GE_ASSERT_TRUE(dones.emplace(branch_index, control_node).second);
    }
  }
  GE_ASSERT_TRUE((pivots.size() == branch_count) && (dones.size() == branch_count));
  for (uint32_t branch_index = 1U; branch_index <= branch_count; ++branch_index) {
    auto *const pivot = pivots.at(branch_index);
    auto *const done = dones.at(branch_index);
    GE_ASSERT_TRUE(HasControlEdge(*pivot, *done) && HasControlEdge(*done, *wait_anyone),
                   "OM2 If/Case node %s branch %u has an incomplete control skeleton", node.GetNamePtr(), branch_index);
    auto *const branch_graph = ge::FastNodeUtils::GetSubgraphFromNode(&node, branch_index);
    GE_ASSERT_NOTNULL(branch_graph);
    auto *const inner_output = FindUniqueNode(
        *branch_graph, [](const ge::FastNode &candidate) { return IsInnerOutput(candidate.GetTypePtr()); });
    GE_ASSERT_NOTNULL(inner_output, "OM2 If/Case node %s branch %u has no InnerNetOutput", node.GetNamePtr(),
                      branch_index);
    auto &guarded_entries = analysis.guarded_entry_node_names[branch_index - 1U];
    auto &completion_nodes = analysis.completion_node_names[branch_index - 1U];
    const auto append_unique_name = [](std::vector<std::string> &names, const char *const name) {
      if (std::find(names.cbegin(), names.cend(), name) == names.cend()) {
        names.emplace_back(name);
      }
    };
    for (auto *const branch_node : branch_graph->GetDirectNode()) {
      if (IsGuardedByPivot(*branch_node)) {
        append_unique_name(guarded_entries, branch_node->GetNamePtr());
      }
      if ((branch_node->GetAllOutNodes().empty()) && !IsInnerOutput(branch_node->GetTypePtr())) {
        append_unique_name(completion_nodes, branch_node->GetNamePtr());
      }
    }
    for (auto *const branch_node : inner_output->GetAllInNodes()) {
      append_unique_name(completion_nodes, branch_node->GetNamePtr());
    }
    for (size_t selector = 0U; selector < analysis.selector_to_subgraph.size(); ++selector) {
      if (analysis.selector_to_subgraph[selector] == branch_index) {
        analysis.branch_exit_node_names[selector] = done->GetNamePtr();
      }
    }
  }
  for (size_t selector = 0U; selector < analysis.selector_to_subgraph.size(); ++selector) {
    GE_ASSERT_TRUE(analysis.selector_to_subgraph[selector] != kInvalidProgramIndex,
                   "OM2 If/Case node %s has no branch for selector %zu", node.GetNamePtr(), selector);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::AppendIf(ge::FastNode &node, const ChainNormalizer &chain_normalizer,
                                                 const CanonicalIfAnalysis &analysis, uint32_t region_index,
                                                 ProgramBlock &program_block) const {
  std::vector<uint32_t> input_chain_indices;
  GE_ASSERT_GRAPH_SUCCESS(chain_normalizer.GetInputChainIndices(node, input_chain_indices));
  GE_ASSERT_TRUE(!input_chain_indices.empty(), "OM2 If node %s has no selector input", node.GetNamePtr());

  ProgramStep step;
  step.kind = ProgramStepKind::kIf;
  step.if_info.execution_node_name = node.GetNamePtr();
  step.condition_region_index = static_cast<uint32_t>(program_block.regions.size());
  step.if_info.condition_region_index = step.condition_region_index;
  std::vector<uint32_t> condition_input_chain_indices;
  GE_ASSERT_GRAPH_SUCCESS(
      chain_normalizer.GetInputChainIndices(*analysis.switch_notify, condition_input_chain_indices));
  step.if_info.selector_chain_index =
      condition_input_chain_indices.empty() ? input_chain_indices[0U] : condition_input_chain_indices[0U];
  step.if_info.selector_value_type = Om2ValueType::kUInt32T;
  step.if_info.merge_info.join_node_name = analysis.join_node_name;
  step.if_info.merge_info.branch_exit_node_names = analysis.branch_exit_node_names;
  step.if_info.parent_predecessor_node_names = analysis.parent_predecessor_node_names;
  step.if_info.parent_continuation_node_names = analysis.parent_continuation_node_names;
  program_block.regions.push_back({step.if_info.condition_region_index, analysis.condition_graph->GetName(), {}});
  GE_ASSERT_GRAPH_SUCCESS(
      CollectRegion(*analysis.condition_graph, chain_normalizer, step.if_info.condition_region_index, program_block));
  for (size_t selector = 0U; selector < analysis.selector_to_subgraph.size(); ++selector) {
    const auto branch_index = analysis.selector_to_subgraph[selector];
    auto *const branch_graph = ge::FastNodeUtils::GetSubgraphFromNode(&node, branch_index);
    GE_ASSERT_NOTNULL(branch_graph, "OM2 If node %s has no subgraph for selector %zu", node.GetNamePtr(), selector);
    const auto branch_region_index = static_cast<uint32_t>(program_block.regions.size());
    program_block.regions.push_back({branch_region_index, branch_graph->GetName(), {}});
    GE_ASSERT_GRAPH_SUCCESS(CollectRegion(*branch_graph, chain_normalizer, branch_region_index, program_block));
    IfProgramInfo::Branch branch{static_cast<uint32_t>(selector),
                                 branch_region_index,
                                 analysis.branch_entry_node_names[selector],
                                 analysis.branch_exit_node_names[selector],
                                 {},
                                 {}};
    GE_ASSERT_TRUE((branch_index > 0U) && (branch_index <= analysis.guarded_entry_node_names.size()));
    branch.guarded_entry_node_names = analysis.guarded_entry_node_names[branch_index - 1U];
    branch.completion_node_names = analysis.completion_node_names[branch_index - 1U];
    step.if_info.branches.emplace_back(std::move(branch));
  }
  program_block.regions[region_index].steps.emplace_back(std::move(step));
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ExecutionPlanCollector::AppendWhile(const ge::FastNode &node,
                                                    const ge::ExecuteGraph &execute_graph) const {
  GELOGE(ge::GRAPH_FAILED,
         "OM2 region %s contains unsupported While node %s: ProgramBody condition semantics "
         "are not available",
         execute_graph.GetName().c_str(), node.GetNamePtr());
  return ge::GRAPH_FAILED;
}

ge::graphStatus ExecutionPlanCollector::AppendSubgraphCall(const ge::FastNode &node,
                                                           const ge::ExecuteGraph &execute_graph) const {
  GELOGE(ge::GRAPH_FAILED, "OM2 region %s contains unsupported SubgraphCall node %s", execute_graph.GetName().c_str(),
         node.GetNamePtr());
  return ge::GRAPH_FAILED;
}
}  // namespace om2
}  // namespace gert
