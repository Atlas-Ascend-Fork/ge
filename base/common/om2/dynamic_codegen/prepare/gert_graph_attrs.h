/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_GRAPH_ATTRS_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_GRAPH_ATTRS_H_

#include "framework/common/ge_visibility.h"
#include <cstdint>
#include <string>
#include <vector>

#include "graph/ge_tensor.h"
#include "common/om2/dynamic_codegen/prepare/model_desc_builder.h"

namespace ge {
class ExecuteGraph;
}  // namespace ge

namespace gert {
// 内部图属性（见 graph/fast_graph/gert_graph_attributes.h）解析后的表示。
// 这里只做「属性 -> 值对象」的解析与校验，不涉及 codegen、资源池或打包。
struct GertModelDescInfo {
  std::vector<ModelIoEntry> inputs;
  std::vector<ModelIoEntry> outputs;
  ModelDescResources resources;
};

// 从执行图的公开属性解析模型 IO 描述和资源槽数。
// 校验：IO 列表与名称列表等长、名称非空且唯一、Tensor 列表可解出 TensorDesc、计数非负且不溢出。
// 失败时不产生半成品：输出参数只在全部校验通过后写入。
VISIBILITY_EXPORT ge::graphStatus ParseGertModelDesc(const ge::ExecuteGraph &graph, GertModelDescInfo &info);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_GRAPH_ATTRS_H_
