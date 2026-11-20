/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common_collector_utils.h"

#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
ge::graphStatus SetPortNames(const std::vector<uint32_t> &chain_indices, const std::vector<std::string> &names,
                             std::vector<KernelPortInfo> &ports) {
  if (chain_indices.size() != names.size()) {
    return ge::GRAPH_PARAM_INVALID;
  }
  ports.reserve(names.size());
  for (size_t i = 0U; i < names.size(); ++i) {
    // Outputs are type-erased Chain slots at this stage. Input ABI types are declared by the
    // consuming kernel collector through SetInputPortInfos.
    ports.push_back({chain_indices[i], names[i], Om2ValueType::kUnknown});
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus SetPortInfos(const std::vector<uint32_t> &chain_indices,
                             const std::vector<KernelInputPortInfo> &port_infos, std::vector<KernelPortInfo> &ports) {
  if (chain_indices.size() != port_infos.size()) {
    return ge::GRAPH_PARAM_INVALID;
  }
  ports.reserve(port_infos.size());
  for (size_t i = 0U; i < port_infos.size(); ++i) {
    if (port_infos[i].value_type == Om2ValueType::kUnknown) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 kernel port %s has no C++ ABI value type",
             port_infos[i].semantic_name.c_str());
      return ge::GRAPH_PARAM_INVALID;
    }
    ports.push_back({chain_indices[i], port_infos[i].semantic_name, port_infos[i].value_type});
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus InitKernelProgramInfo(const KernelCollectContext &context, OutputCreatePolicy output_policy,
                                      bool require_compute_node_info, KernelProgramInfo &kernel_info) {
  kernel_info.execution_node_name = context.node.GetNamePtr();
  kernel_info.kernel_type = context.node.GetTypePtr();
  kernel_info.output_policy = output_policy;
  kernel_info.require_compute_node_info = require_compute_node_info;
  (void)ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "priority", kernel_info.priority);

  int64_t compute_node_index = 0;
  const bool has_compute_node =
      ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), kComputeNodeIndex, compute_node_index);
  if (require_compute_node_info && !has_compute_node) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 kernel %s type %s has no ComputeNodeIndex", context.node.GetNamePtr(),
           context.node.GetTypePtr());
    return ge::GRAPH_PARAM_INVALID;
  }
  if (has_compute_node) {
    if ((compute_node_index < 0) ||
        (static_cast<uint64_t>(compute_node_index) > static_cast<uint64_t>(kInvalidProgramIndex - 1U))) {
      return ge::GRAPH_PARAM_INVALID;
    }
    kernel_info.compute_node_index = static_cast<uint32_t>(compute_node_index);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus SetInputPortNames(const KernelCollectContext &context, const std::vector<std::string> &names,
                                  KernelProgramInfo &kernel_info) {
  const auto ret = SetPortNames(context.input_chain_indices, names, kernel_info.inputs);
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "OM2 kernel %s type %s input count mismatch: actual %zu, expected %zu", context.node.GetNamePtr(),
           context.node.GetTypePtr(), context.input_chain_indices.size(), names.size());
  }
  return ret;
}

ge::graphStatus SetInputPortInfos(const KernelCollectContext &context,
                                  const std::vector<KernelInputPortInfo> &port_infos, KernelProgramInfo &kernel_info) {
  if (context.input_chain_indices.size() != port_infos.size()) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 kernel %s type %s input count mismatch: actual %zu, expected %zu",
           context.node.GetNamePtr(), context.node.GetTypePtr(), context.input_chain_indices.size(), port_infos.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  kernel_info.inputs.reserve(port_infos.size());
  return SetPortInfos(context.input_chain_indices, port_infos, kernel_info.inputs);
}

ge::graphStatus SetOutputPortNames(const KernelCollectContext &context, const std::vector<std::string> &names,
                                   KernelProgramInfo &kernel_info) {
  const auto ret = SetPortNames(context.output_chain_indices, names, kernel_info.outputs);
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "OM2 kernel %s type %s output count mismatch: actual %zu, expected %zu", context.node.GetNamePtr(),
           context.node.GetTypePtr(), context.output_chain_indices.size(), names.size());
  }
  return ret;
}

ge::graphStatus SetOutputPortInfos(const KernelCollectContext &context,
                                   const std::vector<KernelInputPortInfo> &port_infos, KernelProgramInfo &kernel_info) {
  if (context.output_chain_indices.size() != port_infos.size()) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 kernel %s type %s output count mismatch: actual %zu, expected %zu",
           context.node.GetNamePtr(), context.node.GetTypePtr(), context.output_chain_indices.size(),
           port_infos.size());
    return ge::GRAPH_PARAM_INVALID;
  }
  kernel_info.outputs.reserve(port_infos.size());
  return SetPortInfos(context.output_chain_indices, port_infos, kernel_info.outputs);
}

std::vector<std::string> MakeIndexedPortNames(const char *prefix, size_t count) {
  std::vector<std::string> names;
  names.reserve(count);
  for (size_t i = 0U; i < count; ++i) {
    names.emplace_back(std::string(prefix).append(std::to_string(i)));
  }
  return names;
}
}  // namespace om2
}  // namespace gert
