/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_OM2_COMPUTE_NODE_INFO_VALIDATION_H_
#define GE_OM2_COMPUTE_NODE_INFO_VALIDATION_H_

#include <cstddef>
#include "securec.h"

#include "base/runtime/runtime_attrs_def.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "graph/utils/math_util.h"

namespace gert {
inline bool ValidateRuntimeAttrs(const uint8_t *attrs, size_t attrs_size) {
  if (attrs == nullptr || attrs_size < sizeof(RuntimeAttrsDef)) {
    return false;
  }
  size_t count = 0U;
  if (memcpy_s(&count, sizeof(count), attrs + offsetof(RuntimeAttrsDef, attr_num), sizeof(count)) != EOK) {
    return false;
  }
  size_t offset = 0U;
  if (ge::MulOverflow(count, sizeof(size_t), offset) || ge::AddOverflow(sizeof(RuntimeAttrsDef), offset, offset) ||
      offset > attrs_size) {
    return false;
  }
  for (size_t index = 0U; index < count; ++index) {
    size_t current = 0U;
    if (memcpy_s(&current, sizeof(current), attrs + offsetof(RuntimeAttrsDef, offset) + index * sizeof(size_t),
                 sizeof(current)) != EOK) {
      return false;
    }
    if (current < offset || current >= attrs_size) {
      return false;
    }
    offset = current;
  }
  return true;
}

// Prefix of the existing serialized ComputeNodeInfo layout. The runtime API
// exposes no accessor for the stored attribute size, so read that wire field
// directly without constructing an output-instance pointer from untrusted data.
struct SerializedComputeNodeInfoHeader {
  const char *type;
  const char *name;
  size_t ir_input_count;
  size_t input_count;
  size_t output_count;
  size_t ir_output_count;
  size_t attrs_size;
};
static_assert(sizeof(SerializedComputeNodeInfoHeader) + 24U + sizeof(uint64_t) == sizeof(ComputeNodeInfo),
              "ComputeNodeInfo wire header size changed");

inline bool ValidateComputeNodeInfo(const ComputeNodeInfo &info, size_t record_size, size_t &attrs_size) {
  size_t base_size = 0U;
  if (ComputeNodeInfo::CalcSize(info.GetIrInputsNum(), info.GetIrOutputsNum(), info.GetInputsNum(),
                                info.GetOutputsNum(), base_size) != ge::GRAPH_SUCCESS ||
      record_size < base_size) {
    return false;
  }
  attrs_size = record_size - base_size;
  const auto *begin = reinterpret_cast<const uint8_t *>(&info);
  size_t stored_attrs_size = 0U;
  if (memcpy_s(&stored_attrs_size, sizeof(stored_attrs_size),
               begin + offsetof(SerializedComputeNodeInfoHeader, attrs_size), sizeof(stored_attrs_size)) != EOK) {
    return false;
  }
  if (stored_attrs_size != attrs_size) {
    return false;
  }
  const auto *attrs = reinterpret_cast<const uint8_t *>(info.GetAttrs());
  const auto attr_offset = static_cast<size_t>(attrs - begin);
  return attr_offset <= record_size && attrs_size <= record_size - attr_offset &&
         ValidateRuntimeAttrs(attrs, attrs_size);
}
}  // namespace gert
#endif  // GE_OM2_COMPUTE_NODE_INFO_VALIDATION_H_
