/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "program_info_dumper.h"

#include <sstream>

namespace gert {
namespace om2 {
namespace {
std::string Escape(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const auto c : value) {
    if ((c == '\\') || (c == '"')) {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
}

void DumpPorts(const std::vector<KernelPortInfo> &ports, std::ostringstream &stream) {
  stream << '[';
  for (size_t i = 0U; i < ports.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    stream << "{\"chain\":" << ports[i].chain_index << ",\"name\":\"" << Escape(ports[i].semantic_name)
           << "\",\"value_type\":" << static_cast<uint32_t>(ports[i].value_type) << "}";
  }
  stream << ']';
}

void DumpKernels(const ProgramBlock &block, std::ostringstream &stream) {
  stream << '[';
  for (size_t i = 0U; i < block.kernels.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    const auto &kernel = block.kernels[i];
    stream << "{\"name\":\"" << Escape(kernel.execution_node_name) << "\",\"type\":\"" << Escape(kernel.kernel_type)
           << "\",\"inputs\":";
    DumpPorts(kernel.inputs, stream);
    stream << ",\"outputs\":";
    DumpPorts(kernel.outputs, stream);
    stream << ",\"compute_node_index\":" << kernel.compute_node_index
           << ",\"output_policy\":" << static_cast<uint32_t>(kernel.output_policy)
           << ",\"impl_type\":" << static_cast<uint32_t>(kernel.impl_type)
           << ",\"require_compute_node_info\":" << (kernel.require_compute_node_info ? "true" : "false") << '}';
  }
  stream << ']';
}

void DumpRegions(const ProgramBlock &block, std::ostringstream &stream) {
  stream << '[';
  for (size_t region_index = 0U; region_index < block.regions.size(); ++region_index) {
    if (region_index > 0U) {
      stream << ',';
    }
    const auto &region = block.regions[region_index];
    stream << "{\"index\":" << region.index << ",\"graph\":\"" << Escape(region.graph_name) << "\",\"steps\":[";
    for (size_t step_index = 0U; step_index < region.steps.size(); ++step_index) {
      if (step_index > 0U) {
        stream << ',';
      }
      const auto &step = region.steps[step_index];
      if (step.kind == ProgramStepKind::kKernel) {
        stream << "{\"kind\":\"kernel\",\"kernel_index\":" << step.kernel_index << '}';
      } else if (step.kind == ProgramStepKind::kIf) {
        const auto condition_region_index = (step.condition_region_index != kInvalidProgramIndex)
                                                ? step.condition_region_index
                                                : step.if_info.condition_region_index;
        stream << "{\"kind\":\"if\",\"name\":\"" << Escape(step.if_info.execution_node_name)
               << "\",\"condition_region\":" << condition_region_index
               << ",\"selector_chain\":" << step.if_info.selector_chain_index
               << ",\"selector_value_type\":" << static_cast<uint32_t>(step.if_info.selector_value_type)
               << ",\"branches\":[";
        for (size_t branch_index = 0U; branch_index < step.if_info.branches.size(); ++branch_index) {
          if (branch_index > 0U) {
            stream << ',';
          }
          const auto &branch = step.if_info.branches[branch_index];
          stream << "{\"selector\":" << branch.selector_value << ",\"region\":" << branch.region_index
                 << ",\"entry\":\"" << Escape(branch.entry_node_name) << "\",\"exit\":\""
                 << Escape(branch.exit_node_name) << "\",\"guarded_entries\":[";
          for (size_t entry_index = 0U; entry_index < branch.guarded_entry_node_names.size(); ++entry_index) {
            if (entry_index > 0U) {
              stream << ',';
            }
            stream << '\"' << Escape(branch.guarded_entry_node_names[entry_index]) << '\"';
          }
          stream << "],\"completion_nodes\":[";
          for (size_t completion_index = 0U; completion_index < branch.completion_node_names.size();
               ++completion_index) {
            if (completion_index > 0U) {
              stream << ',';
            }
            stream << '\"' << Escape(branch.completion_node_names[completion_index]) << '\"';
          }
          stream << "]}";
        }
        stream << "],\"merge\":{\"join\":\"" << Escape(step.if_info.merge_info.join_node_name)
               << "\",\"branch_exits\":[";
        for (size_t exit_index = 0U; exit_index < step.if_info.merge_info.branch_exit_node_names.size(); ++exit_index) {
          if (exit_index > 0U) {
            stream << ',';
          }
          stream << '\"' << Escape(step.if_info.merge_info.branch_exit_node_names[exit_index]) << '\"';
        }
        stream << "]},\"parent_predecessors\":[";
        for (size_t name_index = 0U; name_index < step.if_info.parent_predecessor_node_names.size(); ++name_index) {
          if (name_index > 0U) {
            stream << ',';
          }
          stream << '\"' << Escape(step.if_info.parent_predecessor_node_names[name_index]) << '\"';
        }
        stream << "],\"parent_continuations\":[";
        for (size_t name_index = 0U; name_index < step.if_info.parent_continuation_node_names.size(); ++name_index) {
          if (name_index > 0U) {
            stream << ',';
          }
          stream << '\"' << Escape(step.if_info.parent_continuation_node_names[name_index]) << '\"';
        }
        stream << "]}";
      } else {
        stream << "{\"kind\":\"while\",\"condition_region\":" << step.while_info.condition_region_index
               << ",\"body_region\":" << step.while_info.body_region_index
               << ",\"condition_chain\":" << step.while_info.condition_chain_index << '}';
      }
    }
    stream << "]}";
  }
  stream << ']';
}

void DumpBlock(const ProgramBlock &block, std::ostringstream &stream) {
  stream << "{\"plan_type\":" << static_cast<uint32_t>(block.plan_type)
         << ",\"root_region\":" << block.root_region_index << ",\"kernels\":";
  DumpKernels(block, stream);
  stream << ",\"regions\":";
  DumpRegions(block, stream);
  stream << ",\"kernel_dependencies\":[";
  for (size_t i = 0U; i < block.kernel_dependencies.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    stream << "{\"predecessor\":" << block.kernel_dependencies[i].predecessor
           << ",\"successor\":" << block.kernel_dependencies[i].successor << '}';
  }
  stream << "]}";
}
}  // namespace

std::string ProgramInfoDumper::Dump(const ProgramInfo &program_info) {
  std::ostringstream stream;
  stream << "{\"chains\":[";
  for (size_t i = 0U; i < program_info.chains.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    const auto &chain = program_info.chains[i];
    stream << "{\"index\":" << chain.index << ",\"symbol\":" << chain.symbol
           << ",\"source_kind\":" << static_cast<uint32_t>(chain.source_kind)
           << ",\"binding_phase\":" << static_cast<uint32_t>(chain.binding_phase)
           << ",\"binding_kind\":" << static_cast<uint32_t>(chain.binding_kind)
           << ",\"source_index\":" << chain.source_index << '}';
  }
  stream << "],\"const_chains\":[";
  for (size_t i = 0U; i < program_info.const_chains.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    const auto &chain = program_info.const_chains[i];
    stream << "{\"chain_index\":" << chain.chain_index << ",\"producer_name\":\"" << Escape(chain.producer_name)
           << "\",\"value_type\":" << static_cast<uint32_t>(chain.value.type) << '}';
  }
  stream << "],\"custom_const_chains\":[";
  for (size_t i = 0U; i < program_info.custom_const_chains.size(); ++i) {
    if (i > 0U) {
      stream << ',';
    }
    const auto &chain = program_info.custom_const_chains[i];
    stream << "{\"chain_index\":" << chain.chain_index << ",\"producer_name\":\"" << Escape(chain.producer_name)
           << "\",\"initializer\":\"" << Escape(chain.initializer)
           << "\",\"payload_is_string\":" << (chain.payload_is_string ? "true" : "false")
           << ",\"payload_size\":" << chain.payload.size() << '}';
  }
  stream << "],\"init\":";
  DumpBlock(program_info.init, stream);
  stream << ",\"main\":";
  DumpBlock(program_info.main, stream);
  stream << ",\"deinit\":";
  DumpBlock(program_info.deinit, stream);
  stream << ",\"resources\":{\"buffer_size\":" << program_info.resources.buffer.size()
         << ",\"compute_node_info_size\":" << program_info.resources.compute_node_infos.size()
         << ",\"kernel_extend_info_size\":" << program_info.resources.kernel_extend_infos.size()
         << ",\"model_desc_size\":" << program_info.resources.model_desc.size() << "}}";
  return stream.str();
}
}  // namespace om2
}  // namespace gert
