/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef BASE_COMMON_HELPER_OM2_OM2_UTILS
#define BASE_COMMON_HELPER_OM2_OM2_UTILS

#include <cstdint>
#include <map>
#include <string>
#include "common/ge_common/ge_types.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "common/om2/codegen/om2_codegen_types.h"
#include "ge_common/ge_common_api_types.h"
#include "graph/ge_local_context.h"

namespace ge {
constexpr int32_t MODEL_FORMAT_OM = 0;
constexpr int32_t MODEL_FORMAT_OM2 = 7;

inline bool IsOm2OnlineMode(const std::map<std::string, std::string> &options, const bool is_execute_graph) {
  if (is_execute_graph) {
    return true;
  }
  const auto iter = options.find(OPTION_MODEL_FORMAT);
  return iter != options.end() && iter->second == std::to_string(MODEL_FORMAT_OM2);
}

inline bool IsOm2OnlineMode(const bool is_execute_graph = false) {
  const auto options = GetThreadLocalContext().GetAllGraphOptions();
  return IsOm2OnlineMode(options, is_execute_graph);
}

struct Om2CompileInfo {
  std::string model_name;
  std::string artifact_name;
  uint32_t graph_id = 0U;
};

class Om2Utils {
 public:
  static Status CompileGeneratedCppToSo(const std::vector<gert::GertModelDataFile> &artifacts,
                                        const Om2CompileInfo &compile_info, gert::GertModelDataFile &so_artifact,
                                        const bool is_release = true);
  static std::string EscapeArtifactName(const std::string &model_name);
  static std::string NormalizeCpuArch(const std::string &cpu);
};
}  // namespace ge

#endif  // BASE_COMMON_HELPER_OM2_OM2_UTILS
