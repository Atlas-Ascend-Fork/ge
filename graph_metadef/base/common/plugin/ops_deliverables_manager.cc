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

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <glob.h>
#include <limits.h>
#include <memory>
#include <mutex>
#include <set>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "common/plugin/plugin_manager.h"
#include "graph/ge_local_context.h"
#include "base/registry/op_impl_space_registry_v2.h"
#include "nlohmann/json.hpp"
#include "platform/platform_info.h"
#include "platform/platform_infos_def.h"

namespace ge {
namespace {
constexpr char kConfigRelativePath[] = "/conf/ops_deliverables.json";

bool InvalidConfig(const std::string &path, const std::string &detail, std::string &reason) {
  reason = detail;
  GELOGW("Invalid ops deliverables config [%s]: %s. Using legacy OPP discovery.", path.c_str(), detail.c_str());
  return false;
}

bool HasString(const nlohmann::json &object, const char *key) {
  return object.contains(key) && object.at(key).is_string() && !object.at(key).get<std::string>().empty();
}

bool IsValidPathTemplate(const std::string &path);

bool IsValidConfigRoot(const nlohmann::json &root) {
  return root.is_object() && HasString(root, "version") && root.at("version") == "1.0" &&
         root.contains("deliverables") && root.at("deliverables").is_array() &&
         (!root.contains("placeholders") || root.at("placeholders").is_object());
}

bool ParseDeliverable(const nlohmann::json &item, std::set<std::string> &names, OpsDeliverable &entry,
                      std::string &reason) {
  if (!item.is_object() || !HasString(item, "name") || !HasString(item, "type") || !item.contains("paths") ||
      !item.at("paths").is_array()) {
    reason = "invalid deliverable entry";
    return false;
  }
  entry = OpsDeliverable{item.at("name").get<std::string>(), item.at("type").get<std::string>(), {}};
  if (!names.insert(entry.name).second) {
    reason = "duplicate deliverable name: " + entry.name;
    return false;
  }
  for (const auto &value : item.at("paths")) {
    if (!value.is_string() || !IsValidPathTemplate(value.get<std::string>())) {
      reason = "deliverable path must use a safe supported template";
      return false;
    }
    entry.paths.push_back(value.get<std::string>());
  }
  return true;
}

bool ParseDeliverables(const nlohmann::json &root, std::vector<OpsDeliverable> &deliverables, std::string &reason) {
  std::set<std::string> names;
  for (const auto &item : root.at("deliverables")) {
    OpsDeliverable entry;
    if (!ParseDeliverable(item, names, entry, reason)) {
      return false;
    }
    deliverables.push_back(std::move(entry));
  }
  return true;
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

// This loader consumes opgraph/ophost; accept the other manifest entries for their respective consumers.
bool IsValidPathTemplate(const std::string &path) {
  if (HasUnsafePathPart(path) || path.rfind("{ASCEND_OPP_PATH}/", 0U) != 0U) {
    return false;
  }
  for (size_t pos = 0U; pos < path.size();) {
    const auto begin = path.find('{', pos);
    if (begin == std::string::npos) {
      return path.find('}', pos) == std::string::npos;
    }
    const auto end = path.find('}', begin);
    if (end == std::string::npos) {
      return false;
    }
    const auto variable = path.substr(begin + 1U, end - begin - 1U);
    if (variable != "ASCEND_OPP_PATH" && variable != "socversion" && variable != "os" && variable != "arch") {
      return false;
    }
    pos = end + 1U;
  }
  return true;
}

bool ExpandVariables(const std::string &input, const std::map<std::string, std::string> &variables, std::string &output,
                     std::string &reason) {
  output.clear();
  for (size_t i = 0U; i < input.size();) {
    if (input[i] != '{') {
      if (input[i] == '}') {
        reason = "unmatched closing brace in path: " + input;
        return false;
      }
      output += input[i++];
      continue;
    }
    const auto end = input.find('}', i);
    if (end == std::string::npos) {
      reason = "unmatched opening brace in path: " + input;
      return false;
    }
    const auto variable = input.substr(i + 1U, end - i - 1U);
    if (variable != "ASCEND_OPP_PATH" && variable != "socversion" && variable != "os" && variable != "arch") {
      reason = "unknown path variable: " + variable;
      return false;
    }
    const auto found = variables.find(variable);
    if (found == variables.end() || found->second.empty()) {
      reason = "unavailable path variable: " + variable;
      return false;
    }
    output += found->second;
    i = end + 1U;
  }
  if (output.size() >= PATH_MAX || output.empty() || output.front() != '/' || HasUnsafePathPart(output)) {
    reason = "unsafe resolved path";
    return false;
  }
  return true;
}

bool IsInOppRoot(const std::string &path, const std::string &root) {
  return path == root ||
         (path.size() > root.size() && path.compare(0U, root.size(), root) == 0 && path[root.size()] == '/');
}

bool AppendMatches(const std::string &pattern, const std::string &root, std::vector<std::string> &paths,
                   std::set<std::string> &seen, std::string &reason) {
  glob_t matches{};
  const int ret = glob(pattern.c_str(), GLOB_ERR, nullptr, &matches);
  if (ret == GLOB_NOMATCH) {
    globfree(&matches);
    return true;  // 路径不存在
  }
  if (ret != 0) {
    globfree(&matches);
    reason = "cannot expand path: " + pattern;
    return false;
  }
  // glob sorts each pattern; preserve the order of patterns in the manifest.
  for (size_t i = 0U; i < matches.gl_pathc; ++i) {
    char canonical[PATH_MAX]{};
    struct stat info{};
    if (realpath(matches.gl_pathv[i], canonical) == nullptr || stat(canonical, &info) != 0 || !S_ISREG(info.st_mode) ||
        access(canonical, R_OK) != 0 || !IsInOppRoot(canonical, root)) {
      reason = "shared library is not a readable regular file below ASCEND_OPP_PATH";
      globfree(&matches);
      return false;
    }
    if (seen.insert(canonical).second) {
      paths.emplace_back(canonical);
    }
  }
  globfree(&matches);
  return true;
}

bool ResolveNamedSharedLibraries(const std::vector<OpsDeliverable> &deliverables,
                                 const std::map<std::string, std::string> &variables,
                                 const std::vector<std::string> &names, std::vector<std::string> &paths,
                                 std::string &reason, const bool require_all) {
  paths.clear();
  reason.clear();
  char canonical_root[PATH_MAX]{};
  const auto root_iter = variables.find("ASCEND_OPP_PATH");
  if (root_iter == variables.end() || realpath(root_iter->second.c_str(), canonical_root) == nullptr) {
    reason = "ASCEND_OPP_PATH cannot be canonicalized";
    return false;
  }
  std::vector<std::string> resolved;
  std::set<std::string> seen;
  for (const auto &name : names) {
    bool found = false;
    for (const auto &entry : deliverables) {
      if (entry.name != name) {
        continue;
      }
      found = true;
      if (entry.type != "shared_library") {
        reason = "deliverable is not a shared library: " + name;
        return false;
      }
      for (const auto &pattern : entry.paths) {
        std::string expanded;
        if (!ExpandVariables(pattern, variables, expanded, reason) ||
            !AppendMatches(expanded, canonical_root, resolved, seen, reason)) {
          return false;
        }
      }
    }
    if (!found && require_all) {
      reason = "missing shared library deliverable: " + name;
      return false;
    }
  }
  paths.swap(resolved);
  return true;
}

std::string GetPlatformSocVersion() {
  fe::PlatFormInfos platform_infos;
  fe::OptionalInfos optional_infos;
  if (fe::PlatformInfoManager::GeInstance().GetPlatformInfoWithOutSocVersion(platform_infos, optional_infos) != 0U) {
    return {};
  }
  auto soc_version = optional_infos.GetSocVersion();
  if (soc_version.empty()) {
    (void)platform_infos.GetPlatformResWithLock("version", "SoC_version", soc_version);
  }
  return IsSafeVariable(soc_version) ? soc_version : std::string();
}

std::map<std::string, std::string> MakeVariables(const std::map<std::string, std::string> &options) {
  const char *root = std::getenv("ASCEND_OPP_PATH");
  std::map<std::string, std::string> variables{{"ASCEND_OPP_PATH", root == nullptr ? "" : root}};
  // Shared libraries run on the compiler host, not the model's target execution environment.
  struct utsname system_info{};
  if (uname(&system_info) == 0) {
    std::string os = system_info.sysname;
    std::transform(os.begin(), os.end(), os.begin(), [](unsigned char c) { return std::tolower(c); });
    variables["os"] = os;
    variables["arch"] = system_info.machine;
  }
  std::string soc_version;
  const auto found = options.find("ge.socVersion");
  if (found != options.end()) {
    soc_version = found->second;
  } else {
    (void)GetThreadLocalContext().GetOption("ge.socVersion", soc_version);
  }
  if (IsSafeVariable(soc_version)) {
    variables["socversion"] = soc_version;
  }
  if (variables.find("socversion") == variables.end()) {
    const auto soc_version = GetPlatformSocVersion();
    if (!soc_version.empty()) {
      variables["socversion"] = soc_version;
    }
  }
  return variables;
}

void AppendCustomLibraries(const std::map<std::string, std::string> &variables, std::vector<std::string> &paths) {
  std::string proto_paths;
  std::string master_paths;
  (void)PluginManager::GetOpsProtoPath(proto_paths);
  (void)PluginManager::GetOpTilingForwardOrderPath(master_paths);
  const std::string platform = variables.at("os") + "/" + variables.at("arch") + "/";
  std::vector<std::string> roots;
  PluginManager::SplitPath(proto_paths, roots);
  for (const auto &root : roots) {
    if (root.find("/built-in/") == std::string::npos) {
      PluginManager::GetFileListWithSuffix(root + "/lib/" + platform, ".so", paths);
    }
  }
  roots.clear();
  PluginManager::SplitPath(master_paths, roots);
  for (const auto &root : roots) {
    if (root.find("/built-in/") == std::string::npos) {
      struct stat info{};
      const std::string master = root + "/op_master/lib/" + platform;
      const auto directory = stat(master.c_str(), &info) == 0 ? master : root + "/op_tiling/lib/" + platform;
      PluginManager::GetFileListWithSuffix(directory, ".so", paths);
    }
  }
}

graphStatus RegisterLibraries(const std::vector<std::string> &paths) {
  static std::map<std::string, void *> handles;
  if (paths.empty()) {
    return GRAPH_SUCCESS;
  }
  std::vector<AscendString> so_paths;
  for (const auto &path : paths) {
    if (handles.find(path) == handles.end()) {
      void *handle = mmDlopen(path.c_str(), MMPA_RTLD_NOW | MMPA_RTLD_GLOBAL);
      if (handle == nullptr) {
        const auto error = mmDlerror();
        GELOGE(GRAPH_FAILED, "Configured SO load failed [%s]: %s. Skipping this library.", path.c_str(),
               error == nullptr ? "unknown dynamic loader error" : error);
        continue;
      }
      handles.emplace(path, handle);
    }
    so_paths.emplace_back(path.c_str());
  }
  if (so_paths.empty()) {
    return GRAPH_SUCCESS;
  }
  auto &defaults = gert::DefaultOpImplSpaceRegistryV2::GetInstance();
  auto registry = defaults.GetSpaceRegistry();
  if (registry == nullptr) {
    registry = std::make_shared<gert::OpImplSpaceRegistryV2>();
    if (defaults.SetSpaceRegistry(registry) != GRAPH_SUCCESS) {
      return GRAPH_FAILED;
    }
  }
  return registry->AddSoToRegistry(gert::OppSoDesc(so_paths, AscendString("built-in")));
}
}  // namespace

std::string OpsDeliverablesManager::GetConfigPath() {
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

bool OpsDeliverablesManager::Load(std::vector<OpsDeliverable> &deliverables, std::string &reason) {
  deliverables.clear();
  reason.clear();
  const auto path = GetConfigPath();
  struct stat info{};
  if (path.empty() || (stat(path.c_str(), &info) != 0)) {
    reason = "configuration file does not exist or cannot be accessed";
    return false;
  }
  std::ifstream input(path);
  // S_ISREG用于判断是不是一个普通文件
  if (!input.is_open() || !S_ISREG(info.st_mode)) {
    return InvalidConfig(path, "cannot open a regular configuration file", reason);
  }
  try {
    const auto root = nlohmann::json::parse(input);
    if (!IsValidConfigRoot(root)) {
      return InvalidConfig(path, "unsupported version or invalid deliverables field", reason);
    }
    std::vector<OpsDeliverable> parsed;
    std::string detail;
    if (!ParseDeliverables(root, parsed, detail)) {
      return InvalidConfig(path, detail, reason);
    }
    deliverables.swap(parsed);
    return true;
  } catch (const nlohmann::json::exception &error) {
    return InvalidConfig(path, error.what(), reason);
  }
}

bool OpsDeliverablesManager::ResolvePaths(const std::vector<OpsDeliverable> &deliverables,
                                          const std::map<std::string, std::string> &variables,
                                          std::vector<std::string> &paths, std::string &reason) {
  // 替代了旧的LoadAllOppPackage逻辑，只针对opgraph和ophost
  if (!ResolveNamedSharedLibraries(deliverables, variables, {"opgraph", "ophost"}, paths, reason, false)) {
    return false;
  }
  // legacy最后加载
  const std::string legacy_suffix = "_legacy.so";
  (void)std::stable_partition(paths.begin(), paths.end(), [&legacy_suffix](const std::string &path) {
    return path.size() < legacy_suffix.size() ||
           path.compare(path.size() - legacy_suffix.size(), legacy_suffix.size(), legacy_suffix) != 0;
  });
  return true;
}

bool OpsDeliverablesManager::ResolveFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                                         const std::string &framework, std::vector<std::string> &paths,
                                                         std::string &reason) {
  static const std::map<std::string, std::string> kDeliverableNames{
      {"tensorflow", "tf-plugin"}, {"onnx", "onnx-plugin"}, {"caffe", "caffe-plugin"}};
  paths.clear();
  reason.clear();
  const auto name_iter = kDeliverableNames.find(framework);
  if (name_iter == kDeliverableNames.end()) {
    reason = "unsupported framework plugin: " + framework;
    return false;
  }
  std::vector<OpsDeliverable> deliverables;
  if (!Load(deliverables, reason)) {
    return false;
  }
  return ResolveNamedSharedLibraries(deliverables, MakeVariables(options), {name_iter->second}, paths, reason, true);
}

void OpsDeliverablesManager::UpdateFrameworkPluginPaths(const std::map<std::string, std::string> &options,
                                                        const std::string &framework, std::vector<std::string> &paths) {
  std::vector<std::string> built_in_plugins;
  std::string reason;
  if (!ResolveFrameworkPluginPaths(options, framework, built_in_plugins, reason)) {
    GELOGI("Use legacy built-in %s plugin discovery, reason: %s", framework.c_str(), reason.c_str());
    return;
  }
  paths.erase(std::remove_if(paths.begin(), paths.end(),
                             [&framework](const std::string &path) {
                               return path.find("/built-in/framework/" + framework + "/") != std::string::npos;
                             }),
              paths.end());
  paths.insert(paths.end(), built_in_plugins.begin(), built_in_plugins.end());
  GELOGI("Load built-in %s plugins from ops_deliverables.json, count: %zu", framework.c_str(), built_in_plugins.size());
}

graphStatus OpsDeliverablesManager::LoadOppPackages(const std::map<std::string, std::string> &options) {
  static std::mutex mutex;
  const std::lock_guard<std::mutex> lock(mutex);
  const char *opp_path = std::getenv("ASCEND_OPP_PATH");
  GELOGI("[OpsDeliverables] ASCEND_OPP_PATH=%s", opp_path == nullptr ? "<unset>" : opp_path);
  std::vector<OpsDeliverable> deliverables;
  std::string reason;
  if (!Load(deliverables, reason)) {
    GELOGI("[OpsDeliverables] Use legacy opgraph/ophost discovery, reason: %s", reason.c_str());
    return gert::OppPackageUtils::LoadAllOppPackage();
  }
  const auto variables = MakeVariables(options);
  std::vector<std::string> built_in;
  if (!ResolvePaths(deliverables, variables, built_in, reason)) {
    (void)InvalidConfig(GetConfigPath(), reason, reason);
    GELOGI("[OpsDeliverables] Use legacy opgraph/ophost discovery, reason: %s", reason.c_str());
    return gert::OppPackageUtils::LoadAllOppPackage();
  }
  GELOGI("[OpsDeliverables] Load opgraph/ophost from %s, count: %zu", GetConfigPath().c_str(), built_in.size());
  std::vector<std::string> paths;
  AppendCustomLibraries(variables, paths);
  paths.insert(paths.end(), built_in.begin(), built_in.end());
  return RegisterLibraries(paths);
}
}  // namespace ge
