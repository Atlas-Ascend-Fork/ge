/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_SOURCE_INFO_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_SOURCE_INFO_H_

#include <cstdint>
#include <string>
#include <vector>

#include "graph/any_value.h"
#include "graph/ge_tensor.h"

namespace ge {
class ExecuteGraph;
class FastNode;
}  // namespace ge

namespace gert {
// 公开来源算子描述：一个来源描述可以被多个 kernel 节点共享。
// io 描述按下标与实例端口一一对应，ir_* 系列按 IR 定义顺序保存，属性按消费方要求的顺序保存。
struct GertSourceInfo {
  std::string id;
  std::string name;
  std::string type;
  std::vector<ge::GeTensorDesc> inputs;
  std::vector<ge::GeTensorDesc> outputs;
  std::vector<std::string> ir_input_names;
  std::vector<std::string> ir_output_names;
  // 每个 IR 端口实例化的实际端口数；required 恒为 1，optional 为 0/1，dynamic 为实际个数。
  std::vector<int64_t> ir_input_counts;
  std::vector<int64_t> ir_output_counts;
  std::vector<std::string> attr_names;
  std::vector<ge::AnyValue> attrs;
};

// 解析图中 gert.source.* 描述。未声明任何来源时返回空表；
// 声明了就要字段齐全：缺失、类型错误、列表长度不一致、实例数不自洽一律失败。
ge::graphStatus ParseGertSourceInfo(const ge::ExecuteGraph &graph, std::vector<GertSourceInfo> &infos);

// 节点以 gert.source.id 关联来源；未声明时返回 false（固定 ABI kernel 允许无来源）。
// 声明了空 ID 属于非法图，返回 false 但会打印错误，由调用方在消费点拒绝。
bool GetNodeSourceId(const ge::FastNode &node, std::string &source_id);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_SOURCE_INFO_H_
