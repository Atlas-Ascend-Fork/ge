/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software: you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_COMMON_OM2_ONLINE_PREPARE_DATA_H_
#define GE_COMMON_OM2_ONLINE_PREPARE_DATA_H_
#include <cstdint>
#include <memory>
#include <vector>
namespace ge {
class Node;
class ComputeGraph;
}  // namespace ge
namespace gert {
struct Om2OnlinePrepareData {
  uint32_t graph_id = 0U;
  bool prepare_ge_variables = false;
  // Keep owner graphs alive while the borrowed node descriptions are consumed.
  std::vector<std::shared_ptr<ge::ComputeGraph>> graphs;
  std::vector<std::shared_ptr<ge::Node>> variables;
  std::vector<std::shared_ptr<ge::Node>> shared_constants;
  std::vector<std::shared_ptr<ge::Node>> file_constants;
};
}  // namespace gert
#endif
