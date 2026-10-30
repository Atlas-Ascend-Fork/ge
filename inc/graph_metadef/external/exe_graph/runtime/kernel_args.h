/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_INC_EXE_GRAPH_RUNTIME_KERNEL_ARGS_H_
#define METADEF_INC_EXE_GRAPH_RUNTIME_KERNEL_ARGS_H_

#include <cstddef>
#include <cstdint>
#include "graph/types.h"

namespace gert {

using Placement = ge::Placement;

/// Kernel launch arguments with memory placement info
struct KernelArgs {
  void *args_data;                 ///< Pointer to argument data
  size_t args_size;                ///< Size of argument data in bytes
  uint64_t reserved[4];            ///< Reserved for future extension
  Placement placement;             ///< Memory placement (host or device)
  uint8_t placement_reserved_[4];  ///< Reserved field, 4-byte aligned for Placement

  KernelArgs() : args_data(nullptr), args_size(0U), reserved{}, placement(ge::kPlacementHost), placement_reserved_{} {}
};

/**
 * GE与自定义算子之间的上下文交互参数。
 * 统一按数值取值语义暴露（布尔参数取值为0/1）；数值类参数按需扩展，
 * 新增参数只需增加枚举值，不改变接口签名。
 *
 * 不同上下文支持的参数子集不同，未设置的参数查询返回false（不写出value）。
 */
enum class ContextParam : uint32_t {
  kStaticModel = 0,            // 是否运行于静态图（含动态shape的静态子图），取值0/1
  kAddressRefreshEnabled = 1,  // 地址是否需要刷新，取值0/1
  kGraphId = 2,                // 根图graph id（恒为根图，不受子图影响）
  kNodeId = 3,                 // 算子节点在图内的id（图内唯一）
  kVectorCoreNum = 4,          // 算子节点可用的vector core数量
  kAiCoreNum = 5,              // 算子节点可用的AI core数量
  kAiCpuNum = 6,               // 算子节点可用的AICPU核数量
  kNum,                        // 枚举数量哨兵，必须保持在最后；后续新增枚举值只能插在本项之前
};

}  // namespace gert

#endif  // METADEF_INC_EXE_GRAPH_RUNTIME_KERNEL_ARGS_H_
