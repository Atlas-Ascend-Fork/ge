/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_MODEL_DESC_BUILDER_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_MODEL_DESC_BUILDER_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "exe_graph/lowering/buffer_pool.h"
#include "graph/ge_tensor.h"

namespace gert {
// 一条模型 IO 描述：公开值对象承载的 TensorDesc、对外名称，以及可选的 aipp 补维后维度。
// GE lowering 从 Data/NetOutput 节点采集，公开属性路径从图属性里的 Tensor 列表采集，
// 两条路径最终都交给同一份序列化实现，保证产物的 ModelDesc 布局一致。
struct ModelIoEntry {
  ge::GeTensorDesc desc;
  std::string name;
  std::vector<int64_t> aipp_dims;
  // 为 true 时把 aipp_dims 写入 ModelIoDesc 的 aipp shape；输入恒为 true（缺省即自身维度）。
  bool has_aipp_dims{false};
};

// 模型级资源槽数量。stream 包含调用方主流，因此不得为 0。
struct ModelDescResources {
  static constexpr int64_t kDefaultStreamNum = 1;
  static constexpr int64_t kDefaultEventNum = 0;
  static constexpr int64_t kDefaultNotifyNum = 0;

  int64_t reusable_stream_num{kDefaultStreamNum};
  int64_t reusable_event_num{kDefaultEventNum};
  int64_t reusable_notify_num{kDefaultNotifyNum};
  int64_t attached_stream_num{0};
};

// 把 IO 描述写成 ModelDesc 并放入 buffer pool，返回的 BufId 即为缓冲区序号。
// 名称字符串同时写入 pool，ModelIoDesc 中保存的是 pool 内的偏移，序列化后由使用方换算。
ge::graphStatus BuildModelDescBuffer(const std::vector<ModelIoEntry> &inputs, const std::vector<ModelIoEntry> &outputs,
                                     const ModelDescResources &resources, bg::BufferPool &pool,
                                     bg::BufferPool::BufId &model_desc_id);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_MODEL_DESC_BUILDER_H_
