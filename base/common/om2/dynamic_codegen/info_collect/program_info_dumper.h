/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_DUMPER_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_DUMPER_H_

#include <string>

#include "program_info.h"

namespace gert {
namespace om2 {
class ProgramInfoDumper {
 public:
  static std::string Dump(const ProgramInfo &program_info);
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_PROGRAM_INFO_DUMPER_H_
