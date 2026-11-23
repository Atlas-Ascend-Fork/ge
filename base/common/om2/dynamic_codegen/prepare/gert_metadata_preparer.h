/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_METADATA_PREPARER_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_METADATA_PREPARER_H_

#include "framework/common/ge_visibility.h"
#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
}  // namespace ge

namespace gert {
struct GertModelDescInfo;
ge::graphStatus NormalizeExecuteGraphConstants(ge::ExecuteGraph &graph);

// 为没有经过 GE lowering 的执行图生成 codegen 所需的元信息并挂到图属性上。
// GE 路径在 lowering 阶段由 ExeGraphSerializer 完成同样的工作；外部执行图没有那个上下文，
// 元信息改由内部图属性推导：
//   - 模型 IO 描述   <- gert.model.*  (GertModelDescInfo)
//   - ComputeNodeInfo <- 节点的 gert.source_op
//   - KernelExtendInfo / 字符串池 <- 节点自身的名称与类型
// 不创建临时 Node/ComputeGraph，也不修改图的拓扑。重复调用是幂等的。
VISIBILITY_EXPORT ge::graphStatus PrepareExecuteGraphMetadata(ge::ExecuteGraph &graph, const GertModelDescInfo &info);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_METADATA_PREPARER_H_
