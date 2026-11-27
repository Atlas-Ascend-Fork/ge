/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/ops_deliverable_config_manager.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "common/fe_log.h"
#include "common/fe_type_utils.h"
#include "common/platform_utils.h"

namespace fe {
namespace {
constexpr char kConfigRelativePath[] = "/conf/ops_deliverables.json";
// name -> 期望的交付件类型
const std::map<std::string, std::string> kExpectedDeliverables = {
    {"ops-info", "json"},                 // 算子信息库
    {"opkernel", "obj_file"},             // 算子二进制（kernel 目录，配置到分包级）
    {"opkernel-config", "json"},          // 算子二进制索引（kernel/config 目录，配置到分包级）
    {"opsrc", "src_file"},                // 算子交付件（算子实现）
    {"op-impl-mode", "file_path"},        // 精度模式配置（impl_mode 目录，含 preset 与 allow_hf32 ini）
    {"tiling-device", "shared_library"},  // tiling 下沉 so 目录（op_tiling_device/lib）
};
constexpr char kOpsPathNamePrefixFlag[] = "ops_";
constexpr char kFusionOpsDirName[] = "fusion_ops";
constexpr char kTilingDeviceSoNamePrefix[] = "libtiling_device_";
constexpr char kSharedLibrarySuffix[] = ".so";
constexpr char kVarOppPath[] = "ASCEND_OPP_PATH";
constexpr char kVarSocVersion[] = "socversion";
constexpr char kVarOs[] = "os";
constexpr char kVarArch[] = "arch";

bool HasString(const nlohmann::json &object, const char *key) {
  const auto iter = object.find(key);
  return (iter != object.end()) && iter->is_string() && !iter->get<std::string>().empty();
}

bool HasUnsafePathPart(const std::string &path) {
  return path.empty() || path.find('\0') != std::string::npos || path.find("..") != std::string::npos;
}

bool IsSafeVariable(const std::string &value) {
  if (value.empty() || value.find("..") != std::string::npos) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](unsigned char ch) { return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.'; });
}

// 归一化用于前缀比较：tolower（配置路径段与算子 prefix 属性同为下划线形式，如 ops_legacy）
std::string NormalizeForPrefixMatch(const std::string &str) {
  std::string normalized = str;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return normalized;
}

// 按 '/' 切分为非空路径段
std::vector<std::string> SplitPathSegments(const std::string &path) {
  std::vector<std::string> segments;
  std::string segment;
  for (const char ch : path) {
    if (ch == '/') {
      if (!segment.empty()) {
        segments.emplace_back(segment);
        segment.clear();
      }
      continue;
    }
    segment.push_back(ch);
  }
  if (!segment.empty()) {
    segments.emplace_back(segment);
  }
  return segments;
}
}  // namespace

OpsDeliverableConfigManager &OpsDeliverableConfigManager::Instance() {
  static OpsDeliverableConfigManager instance;
  return instance;
}

std::string OpsDeliverableConfigManager::GetConfigPath() const {
  const char *root = std::getenv(kVarOppPath);
  if (root == nullptr || *root == '\0') {
    return {};
  }
  std::string path(root);
  while (path.size() > 1U && path.back() == '/') {
    path.pop_back();
  }
  return path + kConfigRelativePath;
}

std::map<std::string, std::string> OpsDeliverableConfigManager::MakeVariables() const {
  std::map<std::string, std::string> variables;
  const char *root = std::getenv(kVarOppPath);
  variables[kVarOppPath] = (root == nullptr) ? "" : root;
  FE_LOGD("Deliverable path variable [%s] = [%s].", kVarOppPath, variables[kVarOppPath].c_str());
  // socversion 通过 configuration 链路（PlatformUtils）查询
  const std::string soc_version = NormalizeForPrefixMatch(PlatformUtils::Instance().GetShortSocVersion());
  if (IsSafeVariable(soc_version)) {
    variables[kVarSocVersion] = soc_version;
    FE_LOGD("Deliverable path variable [%s] = [%s].", kVarSocVersion, soc_version.c_str());
  } else {
    FE_LOGD("Deliverable path variable [%s] is unavailable, soc version [%s] is empty or unsafe.", kVarSocVersion,
            soc_version.c_str());
  }
  struct utsname system_info{};
  if (uname(&system_info) == 0) {
    std::string os_name = system_info.sysname;
    std::transform(os_name.begin(), os_name.end(), os_name.begin(), [](unsigned char c) { return std::tolower(c); });
    variables[kVarOs] = os_name;
    variables[kVarArch] = system_info.machine;
    FE_LOGD("Deliverable path variable [%s] = [%s], [%s] = [%s].", kVarOs, os_name.c_str(), kVarArch,
            system_info.machine);
  } else {
    FE_LOGD("Deliverable path variables [%s]/[%s] are unavailable, uname failed.", kVarOs, kVarArch);
  }
  return variables;
}

bool OpsDeliverableConfigManager::ExpandVariables(const std::string &input,
                                                  const std::map<std::string, std::string> &variables,
                                                  std::string &output) const {
  output.clear();
  for (size_t i = 0U; i < input.size();) {
    if (input[i] != '{') {
      if (input[i] == '}') {
        FE_LOGW("Unmatched closing brace in deliverable path: %s.", input.c_str());
        return false;
      }
      output += input[i++];
      continue;
    }
    const auto end = input.find('}', i);
    if (end == std::string::npos) {
      FE_LOGW("Unmatched opening brace in deliverable path: %s.", input.c_str());
      return false;
    }
    const auto variable = input.substr(i + 1U, end - i - 1U);
    if (variable != kVarOppPath && variable != kVarSocVersion && variable != kVarOs && variable != kVarArch) {
      FE_LOGW("Unknown path variable {%s} in deliverable path: %s.", variable.c_str(), input.c_str());
      return false;
    }
    const auto found = variables.find(variable);
    if (found == variables.end() || found->second.empty()) {
      FE_LOGW("Unavailable path variable {%s} in deliverable path: %s.", variable.c_str(), input.c_str());
      return false;
    }
    output += found->second;
    i = end + 1U;
  }
  if (HasUnsafePathPart(output)) {
    FE_LOGW("Unsafe resolved deliverable path: %s.", input.c_str());
    return false;
  }
  return true;
}

bool OpsDeliverableConfigManager::ResolveDeliverablePaths(const nlohmann::json &deliverables, const std::string &name,
                                                          const std::string &expect_type,
                                                          const std::map<std::string, std::string> &variables,
                                                          std::vector<std::string> &paths) const {
  paths.clear();
  for (const auto &item : deliverables) {
    if (!item.is_object() || !HasString(item, "name") || item.at("name").get<std::string>() != name) {
      continue;
    }
    if (!HasString(item, "type") || item.at("type").get<std::string>() != expect_type) {
      FE_LOGW("Deliverable [%s] type mismatch, expect [%s].", name.c_str(), expect_type.c_str());
      return false;
    }
    if (!item.contains("paths") || !item.at("paths").is_array()) {
      FE_LOGW("Deliverable [%s] has no valid paths field.", name.c_str());
      return false;
    }
    for (const auto &value : item.at("paths")) {
      if (!value.is_string()) {
        continue;
      }
      const std::string path_template = value.get<std::string>();
      if (HasUnsafePathPart(path_template)) {
        FE_LOGW("Deliverable [%s] path template is unsafe: %s.", name.c_str(), path_template.c_str());
        continue;
      }
      std::string expanded;
      if (!ExpandVariables(path_template, variables, expanded)) {
        continue;
      }
      const std::string real_path = GetRealPath(expanded);
      if (real_path.empty()) {
        FE_LOGW("Resolved path of deliverable [%s] does not exist: %s.", name.c_str(), expanded.c_str());
        continue;
      }
      paths.emplace_back(real_path);
    }
    if (paths.empty()) {
      FE_LOGW("Deliverable [%s] has no resolvable path.", name.c_str());
      return false;
    }
    return true;
  }
  // 条目未配置：正常回退，不打日志
  return false;
}

bool OpsDeliverableConfigManager::LoadConfig() {
  const std::string config_path = GetConfigPath();
  struct stat info{};
  // 配置文件不存在：静默回退原逻辑
  if (config_path.empty() || (stat(config_path.c_str(), &info) != 0)) {
    FE_LOGD("Ops deliverables config [%s] does not exist, keep legacy path discovery.", config_path.c_str());
    return false;
  }
  if (!S_ISREG(info.st_mode)) {
    FE_LOGW("Ops deliverables config [%s] is not a regular file, keep legacy path discovery.", config_path.c_str());
    return false;
  }
  std::ifstream input(config_path);
  if (!input.is_open()) {
    FE_LOGW("Cannot open ops deliverables config [%s], keep legacy path discovery.", config_path.c_str());
    return false;
  }
  nlohmann::json root;
  try {
    root = nlohmann::json::parse(input);
  } catch (const nlohmann::json::exception &error) {
    // json 格式问题：报 warning 日志，不退出，回退原逻辑
    FE_LOGW("Parse ops deliverables config [%s] failed: %s. Keep legacy path discovery.", config_path.c_str(),
            error.what());
    return false;
  }
  if (!root.is_object() || !root.contains("deliverables") || !root.at("deliverables").is_array()) {
    FE_LOGW("Ops deliverables config [%s] has no valid deliverables field, keep legacy path discovery.",
            config_path.c_str());
    return false;
  }

  const auto &deliverables = root.at("deliverables");
  const auto variables = MakeVariables();
  for (const auto &expected : kExpectedDeliverables) {
    std::vector<std::string> paths;
    if (ResolveDeliverablePaths(deliverables, expected.first, expected.second, variables, paths)) {
      deliverable_paths_map_.emplace(expected.first, std::move(paths));
      FE_LOGI("Ops deliverable [%s] loaded %zu path(s) from config [%s].", expected.first.c_str(),
              deliverable_paths_map_.at(expected.first).size(), config_path.c_str());
    } else {
      // 期望的条目解析不出来（未配置/类型不匹配/路径均不可解析）：报 warning，回退原逻辑
      FE_LOGW("Ops deliverable [%s](type [%s]) cannot be resolved from config [%s].", expected.first.c_str(),
              expected.second.c_str(), config_path.c_str());
    }
  }
  return !deliverable_paths_map_.empty();
}

bool OpsDeliverableConfigManager::Initialize() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (inited_) {
    return loaded_;
  }
  loaded_ = LoadConfig();
  inited_ = true;
  return loaded_;
}

std::vector<std::string> OpsDeliverableConfigManager::GetPathsByKey(const std::string &key) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  const auto iter = deliverable_paths_map_.find(key);
  if (iter == deliverable_paths_map_.end()) {
    return {};
  }
  return iter->second;
}

std::vector<std::string> OpsDeliverableConfigManager::GetOpsInfoPaths() const {
  return GetPathsByKey("ops-info");
}

std::vector<std::string> OpsDeliverableConfigManager::GetOpKernelPaths() const {
  return GetPathsByKey("opkernel");
}

std::vector<std::string> OpsDeliverableConfigManager::GetOpKernelConfigPaths() const {
  return GetPathsByKey("opkernel-config");
}

std::vector<std::string> OpsDeliverableConfigManager::GetOpImplModePaths() const {
  return GetPathsByKey("op-impl-mode");
}

std::vector<std::string> OpsDeliverableConfigManager::GetTilingDevicePaths() const {
  return GetPathsByKey("tiling-device");
}

bool OpsDeliverableConfigManager::GetOpSrcRoot(std::string &root) const {
  const auto paths = GetPathsByKey("opsrc");
  if (paths.empty()) {
    return false;
  }
  root = paths.front();
  // 配置约定不带尾斜杠；防御性剥离，避免调用方拼出 "//impl"
  while (!root.empty() && root.back() == '/') {
    root.pop_back();
  }
  if (root.empty()) {
    return false;
  }
  FE_LOGD("Opsrc deliverable root is [%s].", root.c_str());
  return true;
}

std::string OpsDeliverableConfigManager::ExtractOpsPathNamePrefix(const std::string &path) {
  const auto segments = SplitPathSegments(path);
  if (segments.empty()) {
    return "";
  }
  const std::string &lastSegment = segments.back();
  if (lastSegment.rfind(kOpsPathNamePrefixFlag, 0) == 0) {
    return lastSegment;
  }
  return "";
}

bool OpsDeliverableConfigManager::IsFusionOpsDir(const std::string &path) {
  const auto segments = SplitPathSegments(path);
  return !segments.empty() && segments.back() == kFusionOpsDirName;
}

bool OpsDeliverableConfigManager::IsPathMatchSoc(const std::string &path, const std::string &soc_version) {
  const std::string socLower = NormalizeForPrefixMatch(soc_version);
  if (socLower.empty()) {
    return false;
  }
  for (const auto &segment : SplitPathSegments(path)) {
    if (NormalizeForPrefixMatch(segment) == socLower) {
      return true;
    }
  }
  return false;
}

bool OpsDeliverableConfigManager::SelectOpKernelPaths(const std::string &soc_version, bool is_kernel,
                                                      std::vector<std::string> &paths) const {
  paths.clear();
  const auto candidates = is_kernel ? GetOpKernelPaths() : GetOpKernelConfigPaths();
  for (const auto &candidate : candidates) {
    if (!IsPathMatchSoc(candidate, soc_version)) {
      FE_LOGD("Opkernel config path [%s] does not match soc [%s].", candidate.c_str(), soc_version.c_str());
      continue;
    }
    paths.emplace_back(candidate);
  }
  FE_LOGI("Select %zu opkernel%s config path(s) by soc [%s].", paths.size(), is_kernel ? "" : "-config",
          soc_version.c_str());
  return !paths.empty();
}

bool OpsDeliverableConfigManager::SelectTilingDevicePath(const std::string &prefix_segment, std::string &path) const {
  // 配置项为 so 全路径，按后缀匹配：libtiling_device_<segment>.so（如 ops_transformer → transformer）
  if (prefix_segment.empty()) {
    // 主包算子走 libopmaster 链路，不使用 tiling_device so
    return false;
  }
  const std::string expect_so_name = kTilingDeviceSoNamePrefix + prefix_segment + kSharedLibrarySuffix;
  for (const auto &candidate : GetTilingDevicePaths()) {
    if (candidate.size() < expect_so_name.size()) {
      continue;
    }
    const size_t pos = candidate.size() - expect_so_name.size();
    if (candidate.compare(pos, expect_so_name.size(), expect_so_name) != 0) {
      continue;
    }
    const std::string real_path = GetRealPath(candidate);
    if (!real_path.empty()) {
      path = real_path;
      FE_LOGD("Select tiling device so [%s] by prefix [%s] from deliverable config.", path.c_str(),
              prefix_segment.c_str());
      return true;
    }
  }
  FE_LOGD("No tiling device so [%s] found in deliverable config paths, prefix [%s].", expect_so_name.c_str(),
          prefix_segment.c_str());
  return false;
}
}  // namespace fe
