/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COLLECT_CONTEXT_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COLLECT_CONTEXT_H_

#include <cstdint>
#include <vector>

#include "exe_graph/runtime/kernel_context.h"
#include "graph/fast_graph/fast_node.h"

namespace gert {
namespace om2 {
struct KernelCollectContext {
  KernelCollectContext(const ge::FastNode &node_in, const std::vector<uint32_t> &input_indices,
                       const std::vector<uint32_t> &output_indices)
      : node(node_in),
        input_chain_indices(input_indices),
        output_chain_indices(output_indices),
        input_chains(EmptyChains()),
        output_chains(EmptyChains()) {}
  KernelCollectContext(const ge::FastNode &node_in, const std::vector<uint32_t> &input_indices,
                       const std::vector<uint32_t> &output_indices, const std::vector<gert::Chain> &inputs,
                       const std::vector<gert::Chain> &outputs)
      : node(node_in),
        input_chain_indices(input_indices),
        output_chain_indices(output_indices),
        input_chains(inputs),
        output_chains(outputs) {}

  const ge::FastNode &node;
  const std::vector<uint32_t> &input_chain_indices;
  const std::vector<uint32_t> &output_chain_indices;
  const std::vector<gert::Chain> &input_chains;
  const std::vector<gert::Chain> &output_chains;

 private:
  static const std::vector<gert::Chain> &EmptyChains() {
    static const std::vector<gert::Chain> empty;
    return empty;
  }
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COLLECT_CONTEXT_H_
