/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_metadata_preparer.h"

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "graph/buffer.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/execute_graph_serialization.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"

namespace gert {
ge::graphStatus NormalizeExecuteGraphConstants(ge::ExecuteGraph &graph) {
  for (auto *node : graph.GetAllNodes()) {
    if (node->GetType() != "Const") {
      continue;
    }
    const auto desc = node->GetOpDescPtr();
    ge::Buffer buffer;
    if (ge::AttrUtils::GetZeroCopyBytes(desc, kConstValue, buffer)) {
      continue;
    }
    ge::ConstGeTensorPtr tensor;
    GE_ASSERT_TRUE(ge::AttrUtils::GetTensor(desc, kConstValue, tensor) && tensor != nullptr,
                   "Execution graph Const %s requires a Tensor or Buffer value", node->GetNamePtr());
    const auto &data = tensor->GetData();
    GE_ASSERT_TRUE(tensor->GetTensorDesc().GetPlacement() == ge::kPlacementHost &&
                       data.GetSize() <= static_cast<size_t>(std::numeric_limits<ptrdiff_t>::max()) &&
                       (data.GetSize() == 0U || data.GetData() != nullptr),
                   "Execution graph Const %s has invalid Tensor data", node->GetNamePtr());
    buffer = ge::Buffer::CopyFrom(data.GetData(), data.GetSize());
    GE_ASSERT_TRUE(
        buffer.GetSize() == data.GetSize() && ge::AttrUtils::SetZeroCopyBytes(desc, kConstValue, std::move(buffer)),
        "Failed to store execution graph Const %s bytes", node->GetNamePtr());
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus PrepareExecuteGraphMetadata(ge::ExecuteGraph &graph, const GertModelDescInfo &info) {
  bg::BufferPool pool;
  bg::BufferPool::BufId desc_id = 0U;
  GE_ASSERT_SUCCESS(BuildModelDescBuffer(info.inputs, info.outputs, info.resources, pool, desc_id),
                    "[GertGraph][Resource] Failed to build model desc");
  size_t size = 0U;
  const auto buffer = pool.Serialize(size);
  GE_ASSERT_NOTNULL(buffer);
  return SerializeExecuteGraphMetadata(graph, std::vector<uint8_t>(buffer.get(), buffer.get() + size));
}
}  // namespace gert
