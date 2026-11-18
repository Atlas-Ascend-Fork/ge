/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "graph_metadef/common/plugin/ops_deliverables_parser.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <limits.h>
#include <set>
#include <utility>

#include "common/ge_common/debug/ge_log.h"
#include "nlohmann/json.hpp"

namespace ge {
namespace {
constexpr char kConfigRelativePath[] = "/conf/ops_deliverables.json";

bool InvalidConfig(const std::string &path, const std::string &detail, std::string &reason) {
  reason = detail;
  GELOGW("Invalid ops deliverables config [%s]: %s.", path.c_str(), detail.c_str());
  return false;
}

bool HasString(const nlohmann::json &object, const char *key) {
  return object.contains(key) && object.at(key).is_string() && !object.at(key).get<std::string>().empty();
}

bool HasUnsafePathPart(const std::string &path) {
  return path.empty() || path.find('\0') != std::string::npos || path.find("..") != std::string::npos;
}

// Braces must alternate and close. The variable names themselves are not restricted here: the caller-supplied
// variables map decides at GetPath time which candidates resolve.
bool IsValidPathTemplate(const std::string &path) {
  if (HasUnsafePathPart(path)) {
    return false;
  }
  for (size_t pos = 0U; pos < path.size();) {
    const auto begin = path.find('{', pos);
    if (begin == std::string::npos) {
      return path.find('}', pos) == std::string::npos;
    }
    const auto end = path.find('}', begin);
    if (end == std::string::npos || end == begin + 1U) {
      return false;  // unclosed '{' or empty "{}" placeholder
    }
    pos = end + 1U;
  }
  return true;
}

bool ParseDeliverable(const nlohmann::json &item, std::map<std::string, std::vector<std::string>> &templates,
                      std::string &reason) {
  if (!item.is_object() || !HasString(item, "name") || !HasString(item, "type") || !item.contains("paths") ||
      !item.at("paths").is_array()) {
    reason = "invalid deliverable entry";
    return false;
  }
  const auto name = item.at("name").get<std::string>();
  if (templates.find(name) != templates.end()) {
    reason = "duplicate deliverable name: " + name;
    return false;
  }
  std::vector<std::string> paths;
  for (const auto &value : item.at("paths")) {
    if (!value.is_string() || !IsValidPathTemplate(value.get<std::string>())) {
      reason = "deliverable path must use a safe supported template";
      return false;
    }
    paths.push_back(value.get<std::string>());
  }
  templates.emplace(name, std::move(paths));
  return true;
}

// Replaces `{placeholder}` tokens with caller-supplied values. A token missing (or empty) in `variables`
// invalidates the whole candidate, as does any unsafe or non-absolute resolved path.
bool ExpandVariables(const std::string &input, const std::map<std::string, std::string> &variables,
                     std::string &output) {
  output.clear();
  for (size_t i = 0U; i < input.size();) {
    if (input[i] != '{') {
      if (input[i] == '}') {
        return false;  // rejected at Load time, kept here for defense
      }
      output += input[i++];
      continue;
    }
    const auto end = input.find('}', i);
    if (end == std::string::npos) {
      return false;
    }
    const auto variable = input.substr(i + 1U, end - i - 1U);
    const auto found = variables.find(variable);
    if (found == variables.end() || found->second.empty()) {
      return false;
    }
    output += found->second;
    i = end + 1U;
  }
  return !output.empty() && output.size() < PATH_MAX && output.front() == '/' && !HasUnsafePathPart(output);
}

std::string GetConfigPath() {
  const char *root = std::getenv("ASCEND_OPP_PATH");
  if (root == nullptr || *root == '\0') {
    return {};
  }
  std::string path(root);
  while (path.size() > 1U && path.back() == '/') {
    path.pop_back();
  }
  return path + kConfigRelativePath;
}
}  // namespace

bool OpsDeliverablesParser::Load(std::string &reason) {
  loaded_ = false;
  path_templates_.clear();
  reason.clear();
  const auto config_path = GetConfigPath();
  if (config_path.empty()) {
    return InvalidConfig(config_path, "configuration path is empty because ASCEND_OPP_PATH is not set", reason);
  }
  struct stat info{};
  if (stat(config_path.c_str(), &info) != 0) {
    return InvalidConfig(config_path, "configuration file does not exist or cannot be accessed", reason);
  }
  std::ifstream input(config_path);
  if (!input.is_open() || !S_ISREG(info.st_mode)) {
    return InvalidConfig(config_path, "cannot open a regular configuration file", reason);
  }
  try {
    const auto root = nlohmann::json::parse(input);
    if (!root.is_object() || !HasString(root, "version") || root.at("version") != "1.0" ||
        !root.contains("deliverables") || !root.at("deliverables").is_array()) {
      return InvalidConfig(config_path, "unsupported version or invalid deliverables field", reason);
    }
    std::string detail;
    for (const auto &item : root.at("deliverables")) {
      if (!ParseDeliverable(item, path_templates_, detail)) {
        path_templates_.clear();
        return InvalidConfig(config_path, detail, reason);
      }
    }
  } catch (const nlohmann::json::exception &error) {
    return InvalidConfig(config_path, error.what(), reason);
  }
  loaded_ = true;
  return true;
}

bool OpsDeliverablesParser::GetPaths(const std::string &key, const std::map<std::string, std::string> &variables,
                                     std::vector<std::string> &paths) const {
  paths.clear();
  if (!loaded_) {
    GELOGW("Ops deliverables config is not loaded, cannot resolve key [%s].", key.c_str());
    return false;
  }
  const auto entry = path_templates_.find(key);
  if (entry == path_templates_.end()) {
    GELOGW("Ops deliverable [%s] is not defined in the config.", key.c_str());
    return false;
  }
  std::set<std::string> seen;
  for (const auto &candidate : entry->second) {
    std::string resolved;
    if (!ExpandVariables(candidate, variables, resolved)) {
      GELOGI("Skip deliverable [%s] path [%s]: placeholder value is missing or path is unsafe.", key.c_str(),
             candidate.c_str());
      continue;
    }
    char canonical[PATH_MAX]{};
    struct stat info{};
    if (realpath(resolved.c_str(), canonical) == nullptr || stat(canonical, &info) != 0 || !S_ISREG(info.st_mode) ||
        access(canonical, R_OK) != 0) {
      GELOGI("Deliverable [%s] path [%s] is not an existing readable file.", key.c_str(), resolved.c_str());
      continue;
    }
    if (seen.insert(canonical).second) {
      paths.emplace_back(canonical);
    }
  }
  return !paths.empty();
}

bool OpsDeliverablesParser::GetPath(const std::string &key, const std::map<std::string, std::string> &variables,
                                    std::string &path) const {
  std::vector<std::string> paths;
  if (!GetPaths(key, variables, paths)) {
    return false;
  }
  path = paths.front();
  return true;
}
}  // namespace ge
