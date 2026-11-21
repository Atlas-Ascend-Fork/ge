/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_CHAIN_NORMALIZER_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_CHAIN_NORMALIZER_H_

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graph/fast_graph/execute_graph.h"
#include "program_info.h"

namespace gert {
namespace om2 {
class ChainNormalizer {
 public:
  ge::graphStatus Normalize(ge::ExecuteGraph &execute_graph, std::vector<ChainInfo> &chains,
                            bool defer_data_bindings = false);
  bool IsUnresolvedFeed(uint32_t chain_index) const;
  ge::graphStatus GetInputChainIndices(const ge::FastNode &node, std::vector<uint32_t> &indices) const;
  ge::graphStatus GetOutputChainIndices(const ge::FastNode &node, std::vector<uint32_t> &indices) const;
  // CEM may replace a Const edge with an InnerData edge when a lifecycle constant is
  // exported from Init into Main.  Preserve the original Const producer so collectors
  // can materialize the process-local value without depending on graph plumbing.
  const ge::FastNode *FindConstProducer(uint32_t chain_index) const;

 private:
  enum class EndpointDirection : uint64_t { kInput = 0U, kOutput = 1U };

  ge::graphStatus AllocateChainIndices(ge::ExecuteGraph &execute_graph, std::vector<ChainInfo> &chains);
  ge::graphStatus AllocateGraphChains(const std::vector<ge::FastNode *> &nodes, std::vector<ChainInfo> &chains);
  ge::graphStatus RecordChainSources(const std::vector<ge::FastNode *> &nodes, std::vector<ChainInfo> &chains,
                                     bool defer_data_bindings);
  ge::graphStatus GetChainIndex(const ge::FastNode &node, uint32_t index, EndpointDirection direction,
                                uint32_t &chain_index) const;

  std::unordered_set<uint32_t> unresolved_feeds_;
  std::unordered_map<uint64_t, uint32_t> symbols_to_chain_indices_;
  std::unordered_map<uint32_t, const ge::FastNode *> const_producers_;
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_CHAIN_NORMALIZER_H_
