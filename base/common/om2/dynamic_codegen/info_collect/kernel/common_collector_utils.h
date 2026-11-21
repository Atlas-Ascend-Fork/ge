/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COMMON_COLLECTOR_UTILS_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COMMON_COLLECTOR_UTILS_H_

#include <string>
#include <vector>

#include "common/om2/dynamic_codegen/info_collect/kernel_collect_context.h"
#include "common/om2/dynamic_codegen/info_collect/program_info.h"
#include "graph/error_codes.h"

namespace gert {
namespace om2 {
struct KernelInputPortInfo {
  std::string semantic_name;
  Om2ValueType value_type = Om2ValueType::kUnknown;
};

// Requests a lifecycle binding if this input chain originates at an external Data.
void RequestInputBinding(const KernelCollectContext &context, uint32_t port, ChainBindingKind kind,
                         ChainBindingPhase phase, KernelProgramInfo &info);

ge::graphStatus InitKernelProgramInfo(const KernelCollectContext &context, OutputCreatePolicy output_policy,
                                      bool require_compute_node_info, KernelProgramInfo &kernel_info);
ge::graphStatus SetInputPortNames(const KernelCollectContext &context, const std::vector<std::string> &names,
                                  KernelProgramInfo &kernel_info);
ge::graphStatus SetInputPortInfos(const KernelCollectContext &context,
                                  const std::vector<KernelInputPortInfo> &port_infos, KernelProgramInfo &kernel_info);
ge::graphStatus SetOutputPortNames(const KernelCollectContext &context, const std::vector<std::string> &names,
                                   KernelProgramInfo &kernel_info);
ge::graphStatus SetOutputPortInfos(const KernelCollectContext &context,
                                   const std::vector<KernelInputPortInfo> &port_infos, KernelProgramInfo &kernel_info);
std::vector<std::string> MakeIndexedPortNames(const char *prefix, size_t count);
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_COMMON_COLLECTOR_UTILS_H_
