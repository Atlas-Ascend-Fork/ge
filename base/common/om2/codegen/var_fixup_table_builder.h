/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_CODEGEN_VAR_FIXUP_TABLE_BUILDER_H_
#define AIR_CXX_BASE_COMMON_OM2_CODEGEN_VAR_FIXUP_TABLE_BUILDER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "common/om2/codegen/om2_codegen_types.h"
#include "framework/om2/model_data/gert_model_data.h"
#include "graph/ge_error_codes.h"

namespace ge {
class VarManager;
class ComputeGraph;
using ComputeGraphPtr = std::shared_ptr<ComputeGraph>;

// 构建变量固化表与权重数据块。必须在代码生成之前调用，因为 Codegen 要固化
// 元数据与数据块内偏移。工作列表沿 copy_info.src_var_name 展开（不沿 trans_road 展开），
// 故 entries 是 var_metas 的超集：转换源变量也要有偏移可用。
Status BuildVarFixupTable(VarManager &var_manager, const ComputeGraphPtr &compute_graph,
                          const std::vector<VarSlotMeta> &var_metas, const uint64_t graph_id, VarFixupTable &table);
}  // namespace ge

#endif  // AIR_CXX_BASE_COMMON_OM2_CODEGEN_VAR_FIXUP_TABLE_BUILDER_H_
