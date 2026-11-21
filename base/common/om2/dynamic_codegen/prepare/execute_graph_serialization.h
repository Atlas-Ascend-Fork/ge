/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_

#include <cstdint>
#include <vector>

#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
}  // namespace ge

namespace gert {
// Generate the external graph's metadata. Only explicit source JSON can bind CNI.
ge::graphStatus SerializeExecuteGraphMetadata(ge::ExecuteGraph &graph, const std::vector<uint8_t> &model_desc);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_EXECUTE_GRAPH_SERIALIZATION_H_
