/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_DYNAMIC_HOST_GRAPH_H_
#define GE_TESTS_OM2_DYNAMIC_HOST_GRAPH_H_
#include <memory>
#include <vector>
#include "graph/fast_graph/execute_graph.h"
namespace gert {
namespace om2_test {
ge::FastNode *AddNode(ge::ExecuteGraph &graph, const char *name, const char *type,
                      const std::vector<ge::GeTensorDesc> &inputs, const std::vector<ge::GeTensorDesc> &outputs);
std::shared_ptr<ge::ExecuteGraph> AffineGraph(bool load_bound_parameters, bool fail_run = false);
}  // namespace om2_test
}  // namespace gert
#endif
