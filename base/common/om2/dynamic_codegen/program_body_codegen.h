/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_PROGRAM_BODY_CODEGEN_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_PROGRAM_BODY_CODEGEN_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "common/om2/dynamic_codegen/info_collect/program_info.h"
#include "framework/om2/model_data/gert_model_data.h"

namespace gert {
namespace om2 {
struct ProgramBodyCodegenOption {
  std::string model_name;
  std::string interface_file_name;
  std::string source_file_name;
  std::string makefile_file_name;
  bool emit_makefile = true;
  size_t thread_num = 1U;
};

struct ProgramBodyCodegenResult {
  std::vector<gert::GertModelDataFile> artifacts;
};

class ProgramBodyCodegen {
 public:
  ge::Status Generate(const ProgramInfo &program_info, const ProgramBodyCodegenOption &option,
                      ProgramBodyCodegenResult &result) const;
};
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_PROGRAM_BODY_CODEGEN_H_
