/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_BASE_COMMON_OM2_GERT_SOURCE_OP_JSON_H_
#define AIR_BASE_COMMON_OM2_GERT_SOURCE_OP_JSON_H_

#include <memory>
#include "exe_graph/lowering/buffer_pool.h"
#include "graph/error_codes.h"

namespace ge {
class FastNode;
}  // namespace ge

namespace gert {
ge::graphStatus BuildComputeNodeInfoFromSourceOp(const ge::FastNode &node, bg::BufferPool &pool,
                                                 std::unique_ptr<uint8_t[]> &data, size_t &size);
}  // namespace gert

#endif  // AIR_BASE_COMMON_OM2_GERT_SOURCE_OP_JSON_H_
