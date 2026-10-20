/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef GE_COMMON_PLUGIN_OPS_DELIVERABLES_MANAGER_H_
#define GE_COMMON_PLUGIN_OPS_DELIVERABLES_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include "graph/types.h"
#include "graph/error_codes.h"

namespace ge {
struct OpsDeliverable {
  std::string name;
  std::string type;
  std::vector<std::string> paths;
};

class OpsDeliverablesManager {
 public:
  // Returns false when the optional file is absent or invalid. The caller must use legacy discovery then.
  static bool Load(std::vector<OpsDeliverable> &deliverables, std::string &reason);
  static bool ResolvePaths(const std::vector<OpsDeliverable> &deliverables,
                           const std::map<std::string, std::string> &variables, std::vector<std::string> &paths,
                           std::string &reason);
  // Returns false when the manifest is unavailable, invalid, or has no entry for the framework. Callers must then
  // use legacy built-in plugin discovery.
  static bool ResolveFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                          const std::string &framework, std::vector<std::string> &paths,
                                          std::string &reason);
  // Replaces legacy built-in plugin paths on success; leaves paths unchanged when the manifest is unavailable.
  static void UpdateFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                         const std::string &framework, std::vector<std::string> &paths);
  // Invalid/absent configuration uses legacy discovery before any configured library is opened.
  // A shared library that fails to load is logged and skipped; other registration failures remain fatal.
  static graphStatus LoadOppPackages(const std::map<std::string, std::string> &options = {});
  static std::string GetConfigPath();
};
}  // namespace ge
#endif
