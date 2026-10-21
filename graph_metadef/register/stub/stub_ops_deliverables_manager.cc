/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "common/plugin/ops_deliverables_manager.h"

namespace ge {
bool OpsDeliverablesManager::Load(std::vector<OpsDeliverable> &deliverables, std::string &reason) {
  return false;
}

bool OpsDeliverablesManager::ResolvePaths(const std::vector<OpsDeliverable> &deliverables,
                                          const std::map<std::string, std::string> &variables,
                                          std::vector<std::string> &paths, std::string &reason) {
  return false;
}

bool OpsDeliverablesManager::ResolveFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                                         const std::string &framework, std::vector<std::string> &paths,
                                                         std::string &reason) {
  return false;
}

void OpsDeliverablesManager::UpdateFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                                        const std::string &framework, std::vector<std::string> &paths) {
}

graphStatus OpsDeliverablesManager::LoadOppPackages(const std::map<std::string, std::string> &options) {
  return GRAPH_SUCCESS;
}

std::string OpsDeliverablesManager::GetConfigPath() {
  return "";
}
}  // namespace ge
