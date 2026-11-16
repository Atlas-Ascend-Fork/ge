/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "program_info_collector.h"

#include <cstdint>
#include <unordered_set>

#include "common/checker.h"
#include "const_value_collector.h"
#include "core/builder/executor_selection_policy.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "execution_plan_collector.h"
#include "framework/common/debug/ge_log.h"
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

ge::graphStatus CopyBytesAttr(const ge::ExecuteGraph &execute_graph, const char *const attr_name,
                              std::vector<uint8_t> &bytes) {
  ge::Buffer buffer;
  GE_ASSERT_TRUE(ge::AttrUtils::GetZeroCopyBytes(&execute_graph, attr_name, buffer),
                 "Failed to get OM2 model resource %s", attr_name);
  bytes.assign(buffer.data(), buffer.data() + buffer.size());
  return ge::GRAPH_SUCCESS;
}

bool IsSupportedBinding(const ChainBindingKind binding_kind) {
  switch (binding_kind) {
    case ChainBindingKind::kStream:
    case ChainBindingKind::kExternalAllocator:
    case ChainBindingKind::kModelDesc:
    case ChainBindingKind::kSpaceRegistry:
    case ChainBindingKind::kCustomOpRegistry:
    case ChainBindingKind::kBinary:
    case ChainBindingKind::kGeKernelBinary:
    case ChainBindingKind::kOuterWeightMem:
    case ChainBindingKind::kNotifies:
    case ChainBindingKind::kRtSession:
      return true;
    case ChainBindingKind::kNone:
    case ChainBindingKind::kRuntimeEvents:
      return false;
  }
  return false;
}

void SetResourceBinding(ChainInfo &chain, const ChainBindingKind binding_kind, const ChainBindingPhase binding_phase) {
  chain.binding_kind = binding_kind;
  chain.binding_phase = binding_phase;
}

void CollectKernelBindings(const ProgramBlock &block, ProgramInfo &program_info,
                           std::unordered_set<uint32_t> &used_runtime_chains) {
  for (const auto &kernel : block.kernels) {
    for (const auto &port : kernel.inputs) {
      auto &chain = program_info.chains[port.chain_index];
      if (chain.source_kind == ChainSourceKind::kRuntimeBinding) {
        used_runtime_chains.insert(chain.index);
      }
      if (chain.source_kind != ChainSourceKind::kConstant) {
        continue;
      }
      // These two values are process-local objects.  A Const producer only carries
      // their serialized identity; the actual object must be supplied at Load.
      if (port.value_type == Om2ValueType::kGertOpImplSpaceRegistryV2Ptr) {
        SetResourceBinding(chain, ChainBindingKind::kSpaceRegistry, ChainBindingPhase::kLoad);
      } else if (port.value_type == Om2ValueType::kGertCustomOpRegistryPtr) {
        SetResourceBinding(chain, ChainBindingKind::kCustomOpRegistry, ChainBindingPhase::kLoad);
      } else if ((port.value_type == Om2ValueType::kGertKernelBinDataPtr) ||
                 (port.value_type == Om2ValueType::kGeKernelBinPtr)) {
        SetResourceBinding(chain,
                           port.value_type == Om2ValueType::kGeKernelBinPtr ? ChainBindingKind::kGeKernelBinary
                                                                            : ChainBindingKind::kBinary,
                           ChainBindingPhase::kLoad);
        const auto key_iter = kernel.binary_resource_keys.find(port.chain_index);
        if (key_iter != kernel.binary_resource_keys.cend()) {
          const auto iter = program_info.binary_resource_keys.find(chain.index);
          if (iter == program_info.binary_resource_keys.cend()) {
            program_info.binary_resource_keys.emplace(chain.index, key_iter->second);
          } else {
            if (iter->second != key_iter->second) {
              GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 binary chain %u has conflicting resource keys", chain.index);
              return;
            }
          }
        }
      }
    }
  }
}

void CollectStepBindings(const ProgramBlock &block, const ProgramInfo &program_info,
                         std::unordered_set<uint32_t> &used_runtime_chains) {
  for (const auto &region : block.regions) {
    for (const auto &step : region.steps) {
      if (step.kind != ProgramStepKind::kIf) {
        continue;
      }
      const auto &if_info = step.if_info;
      const auto &selector_chain = program_info.chains[if_info.selector_chain_index];
      if (selector_chain.source_kind == ChainSourceKind::kRuntimeBinding) {
        used_runtime_chains.insert(selector_chain.index);
      }
    }
  }
}

ge::graphStatus ApplyChainUpdate(const ChainUpdate &update, std::vector<ChainInfo> &chains) {
  GE_ASSERT_TRUE((update.chain_index < chains.size()) && (chains[update.chain_index].index == update.chain_index),
                 "OM2 chain update index %u is out of range", update.chain_index);
  auto &chain = chains[update.chain_index];
  if ((chain.source_kind != ChainSourceKind::kUninitialized) && (chain.source_kind != ChainSourceKind::kKernelOutput)) {
    GE_ASSERT_TRUE((chain.source_kind == update.source_kind) && (chain.source_index == update.source_index) &&
                       (chain.binding_phase == update.binding_phase) && (chain.binding_kind == update.binding_kind),
                   "OM2 chain %u has conflicting collector updates", update.chain_index);
    return ge::GRAPH_SUCCESS;
  }
  chain.source_kind = update.source_kind;
  chain.source_index = update.source_index;
  chain.binding_phase = update.binding_phase;
  chain.binding_kind = update.binding_kind;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ApplyKernelChainUpdates(const ProgramBlock &block, std::vector<ChainInfo> &chains) {
  for (const auto &kernel : block.kernels) {
    for (const auto &update : kernel.chain_updates) {
      GE_ASSERT_GRAPH_SUCCESS(ApplyChainUpdate(update, chains));
    }
  }
  return ge::GRAPH_SUCCESS;
}

ProgramPlanType GetMainPlanType(const ge::ExecuteGraph &main_graph) {
  switch (SelectDefaultExecutorType(main_graph)) {
    case ExecutorType::kSequentialPriority:
      return ProgramPlanType::kSequential;
    case ExecutorType::kTopological:
      return ProgramPlanType::kStructuredTopological;
    case ExecutorType::kTopologicalPriority:
    case ExecutorType::kTopologicalMultiThread:
      return ProgramPlanType::kStructuredTopologicalPriority;
    case ExecutorType::kEnd:
      break;
  }
  return ProgramPlanType::kStructuredTopological;
}

}  // namespace

ge::graphStatus ProgramInfoCollector::Collect(ge::ExecuteGraph &execute_graph, ProgramInfo &program_info) const {
  program_info = {};
  ChainNormalizer chain_normalizer;
  GE_ASSERT_GRAPH_SUCCESS(chain_normalizer.Normalize(execute_graph, program_info.chains));
  GE_ASSERT_GRAPH_SUCCESS(CollectModelResources(execute_graph, program_info.resources));

  auto *const init_graph = GetLifecycleGraph(execute_graph, "Init");
  auto *const main_graph = GetLifecycleGraph(execute_graph, "Main");
  auto *const deinit_graph = GetLifecycleGraph(execute_graph, "DeInit");
  const ExecutionPlanCollector execution_plan_collector;
  const auto main_plan_type = GetMainPlanType(*main_graph);
  GE_ASSERT_GRAPH_SUCCESS(
      execution_plan_collector.Collect(*init_graph, chain_normalizer, ProgramPlanType::kSequential, program_info.init));
  GE_ASSERT_GRAPH_SUCCESS(
      execution_plan_collector.Collect(*main_graph, chain_normalizer, main_plan_type, program_info.main));
  GE_ASSERT_GRAPH_SUCCESS(execution_plan_collector.Collect(*deinit_graph, chain_normalizer,
                                                           ProgramPlanType::kSequential, program_info.deinit));
  GE_ASSERT_GRAPH_SUCCESS(ApplyKernelChainUpdates(program_info.init, program_info.chains));
  GE_ASSERT_GRAPH_SUCCESS(ApplyKernelChainUpdates(program_info.main, program_info.chains));
  GE_ASSERT_GRAPH_SUCCESS(ApplyKernelChainUpdates(program_info.deinit, program_info.chains));
  GE_ASSERT_GRAPH_SUCCESS(CollectChainBindings(program_info));
  GE_ASSERT_GRAPH_SUCCESS(ConstValueCollector().Collect(execute_graph, chain_normalizer, program_info));

  GELOGI("OM2 ProgramInfo collected: chains=%zu, constants=%zu, init=%zu, main=%zu, deinit=%zu",
         program_info.chains.size(), program_info.const_chains.size(), program_info.init.kernels.size(),
         program_info.main.kernels.size(), program_info.deinit.kernels.size());
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ProgramInfoCollector::CollectChainBindings(ProgramInfo &program_info) const {
  std::unordered_set<uint32_t> used_runtime_chains;
  CollectStepBindings(program_info.main, program_info, used_runtime_chains);
  CollectKernelBindings(program_info.init, program_info, used_runtime_chains);
  CollectKernelBindings(program_info.main, program_info, used_runtime_chains);
  CollectKernelBindings(program_info.deinit, program_info, used_runtime_chains);

  for (const auto chain_index : used_runtime_chains) {
    const auto &chain = program_info.chains[chain_index];
    if (!IsSupportedBinding(chain.binding_kind)) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 runtime binding %u is not represented by the ProgramBody ABI (chain %u)",
             static_cast<uint32_t>(chain.binding_kind), chain.index);
      return ge::GRAPH_PARAM_INVALID;
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ProgramInfoCollector::CollectModelResources(const ge::ExecuteGraph &execute_graph,
                                                            ModelResourceInfo &resources) const {
  GE_ASSERT_GRAPH_SUCCESS(CopyBytesAttr(execute_graph, kBuffer, resources.buffer));
  GE_ASSERT_GRAPH_SUCCESS(CopyBytesAttr(execute_graph, kComputeNodeInfo, resources.compute_node_infos));
  GE_ASSERT_GRAPH_SUCCESS(CopyBytesAttr(execute_graph, kKernelExtendInfo, resources.kernel_extend_infos));
  return CopyBytesAttr(execute_graph, kModelDesc, resources.model_desc);
}
}  // namespace om2
}  // namespace gert
