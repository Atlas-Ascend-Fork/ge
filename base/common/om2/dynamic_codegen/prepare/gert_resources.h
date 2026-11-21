/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_

#include "framework/common/ge_visibility.h"
#include <cstdint>
#include <string>
#include <vector>

#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
}  // namespace ge

namespace gert {
// 装配后的设备二进制。key 由内容算出，生成代码按同一个 key 引用，因此名字与内容必须一致。
struct GertBinaryImage {
  std::string name;
  std::string key;
  std::vector<uint8_t> content;
};

// 按图属性中的 ID 顺序解析设备二进制，并按文件名去重。
VISIBILITY_EXPORT ge::graphStatus ParseGertBinaries(const ge::ExecuteGraph &graph,
                                                    std::vector<GertBinaryImage> &images);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_
