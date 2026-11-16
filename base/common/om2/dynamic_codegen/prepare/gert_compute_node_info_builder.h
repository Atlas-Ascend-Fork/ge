/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_COMPUTE_NODE_INFO_BUILDER_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_COMPUTE_NODE_INFO_BUILDER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "exe_graph/lowering/buffer_pool.h"
#include "graph/any_value.h"
#include "graph/ge_tensor.h"
#include "common/om2/dynamic_codegen/prepare/gert_source_info.h"

namespace ge {
class FastNode;
}  // namespace ge

namespace gert {
// 一份已经解析好的计算节点描述：来源算子描述 + 本节点实际端口 + 有序属性。
// 与 Node/ComputeGraph 无关；改图后重建只需要重新填充这个结构。
struct GertComputeNodeDef {
  const GertSourceInfo *source{nullptr};
  std::string name;
  std::string type;
  // 本实例实际的输入/输出端口数量；有来源时必须与来源的实例数一致。
  size_t input_num{0U};
  size_t output_num{0U};
  // 按来源 attr_names 顺序排列的属性值。无来源时为空。
  std::vector<ge::AnyValue> attrs;
  // 每个实例端口的描述，顺序与端口一一对应。
  std::vector<ge::GeTensorDesc> input_descs;
  std::vector<ge::GeTensorDesc> output_descs;
};

// 从来源描述与节点实际端口生成节点描述。
// 有来源：IR 定义、实例数与属性顺序全部取自来源，端口数不一致即失败。
// 无来源（固定 ABI kernel）：IR 端口按实际端口一一对应，不携带任何 IR/运行时属性。
ge::graphStatus BuildComputeNodeDef(const ge::FastNode &node, const GertSourceInfo *source, GertComputeNodeDef &def);

// 直接从值描述构造 ComputeNodeInfo，不创建临时 Node/ComputeGraph。
// 名称字符串写入 buffer_pool，返回的缓冲区由调用方持有并登记序号。
ge::graphStatus BuildComputeNodeInfoBuffer(const GertComputeNodeDef &def, bg::BufferPool &buffer_pool,
                                           std::unique_ptr<uint8_t[]> &compute_node_info, size_t &total_size);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_COMPUTE_NODE_INFO_BUILDER_H_
