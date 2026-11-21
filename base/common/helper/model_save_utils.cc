/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/helper/model_save_utils.h"

#include "common/checker.h"
#include "framework/omg/omg_inner_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/file_utils.h"
#include "graph_metadef/common/plugin/plugin_manager.h"

namespace ge {
namespace {
const std::string kOppVersion = "Version=";

const string kInner = "built-in";
const string kOpsProtoPath = "/op_proto/lib/";
const string kOpsGraphPath = "/op_graph/lib/";
const string kOpMasterPath = "/op_impl/ai_core/tbe/op_tiling/lib/";
const string kOpHostPath = "/op_impl/ai_core/tbe/op_host/lib/";
const std::string kSoSuffix = ".so";
const std::string kRt2SoSuffix = "rt2.0.so";
const std::string kRtSoSuffix = "rt.so";
const std::string kCompilerVersion = "compiler_version=";
const std::string kVersionInfo = "/version.info";

std::string GetOppPkgPath(const std::string &opp_path, const string &whole_pkg_path, const string &sub_pkg_path,
                          const string &os_cpu_type, bool &is_sub_pkg) {
  is_sub_pkg = false;
  const auto idx = opp_path.find(kInner);
  if (idx != std::string::npos) {
    return PluginManager::GetOppPkgPath(opp_path.substr(0, idx) + kInner, whole_pkg_path, sub_pkg_path, os_cpu_type,
                                        is_sub_pkg);
  }
  return opp_path;
}

Status LoadOppSoFromPath(const string &path, bool is_split, bool is_sub_pkg, OpSoStore &so_store) {
  const bool is_built_in_path = (path.find(kInner) != std::string::npos);
  std::vector<std::string> op_so_list;
  if ((!is_sub_pkg) && is_built_in_path) {
    const std::string so_buff = is_split ? kRtSoSuffix : kRt2SoSuffix;
    ge::PluginManager::GetFileListWithSuffix(path, so_buff, op_so_list);
  } else {
    ge::PluginManager::GetFileListWithSuffix(path, kSoSuffix, op_so_list);
  }

  if (is_built_in_path && op_so_list.empty()) {
    GELOGE(FAILED, "Cannot find any op so in path:%s", path.c_str());
    return FAILED;
  }

  std::unordered_set<std::string> op_so_set(op_so_list.begin(), op_so_list.end());
  GE_ASSERT_SUCCESS(LoadOppSoBinaries(op_so_set, SoBinType::kSpaceRegistry, so_store));
  return SUCCESS;
}
}  // namespace

Status InitFirstGeModel(const GeRootModelPtr &ge_root_model, const std::map<std::string, GeModelPtr> &name_to_ge_model,
                        GeModelPtr &first_ge_model) {
  const auto &first_model_it = name_to_ge_model.find(ge_root_model->GetRootGraph()->GetName());
  if (first_model_it == name_to_ge_model.end()) {
    first_ge_model = MakeShared<GeModel>();
    GE_CHECK_NOTNULL(first_ge_model);
    first_ge_model->SetGraph(ge_root_model->GetRootGraph());
  } else {
    first_ge_model = first_model_it->second;
  }
  GE_CHK_BOOL_EXEC(
      ge::AttrUtils::SetStr(*(first_ge_model.get()), ATTR_MODEL_ATC_CMDLINE, domi::GetContext().atc_cmdline),
      GELOGE(FAILED, "SetStr for atc_cmdline failed.");
      return FAILED);
  std::string cur_version;
  const auto get_ret = GetOppVersion(cur_version);
  if ((get_ret != SUCCESS) || (!ge::AttrUtils::SetStr(*(first_ge_model.get()), ATTR_MODEL_OPP_VERSION, cur_version))) {
    GELOGW("Ge model set opp version unsuccessful!");
  }
  return SUCCESS;
}

Status GetOppVersion(std::string &version) {
  std::string opp_path;
  (void)PluginManager::GetOppPath(opp_path);
  const std::string version_path = opp_path + "/version.info";
  if (!PluginManager::GetVersionFromPathWithName(version_path, version, kOppVersion)) {
    GELOGW("Get opp version information from %s unsuccessful!", version_path.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status LoadOppSoBinaries(const std::unordered_set<std::string> &op_so_set, SoBinType so_bin_type, OpSoStore &so_store) {
  for (const auto &op_so : op_so_set) {
    uint32_t bin_len = 0U;
    auto op_so_bin = GetBinDataFromFile(op_so, bin_len);
    GE_ASSERT_NOTNULL(op_so_bin, "open so fail, path=%s", op_so.c_str());
    const auto &pos = op_so.find_last_of("/");
    GE_ASSERT_TRUE(pos != std::string::npos);
    const auto &so_name = op_so.substr(pos + 1UL);
    const auto &vendor_name = op_so.substr(0, pos);
    const auto proto_bin = ge::MakeShared<OpSoBin>(so_name, vendor_name, std::move(op_so_bin), bin_len, so_bin_type);
    GE_ASSERT_NOTNULL(proto_bin);
    so_store.AddKernel(proto_bin);
    GELOGD("Add op so:%s[%u] success.", op_so.c_str(), so_bin_type);
  }
  return SUCCESS;
}

Status CollectSpaceRegistrySoBins(const std::string &cpu_info, const std::string &os_info, OpSoStore &so_store,
                                  std::unordered_set<std::string> &compiler_versions) {
  const auto is_split = PluginManager::IsSplitOpp();

  std::vector<std::string> vendors;
  PluginManager::GetPackageSoPath(vendors);
  const std::string os_cpu_type = os_info + "/" + cpu_info;
  bool is_sub_pkg = false;
  for (size_t i = 0U; i < vendors.size(); i++) {
    if (vendors[i].empty()) {
      continue;
    }
    GELOGD("Begin to scan op proto and master so from path:%s", vendors[i].c_str());
    const auto last_kernel_num = so_store.GetKernelNum();
    const auto op_proto_path =
        GetOppPkgPath(vendors[i] + kOpsProtoPath + os_cpu_type, kOpsProtoPath, kOpsGraphPath, os_cpu_type, is_sub_pkg);
    GE_ASSERT_SUCCESS(LoadOppSoFromPath(op_proto_path, is_split, is_sub_pkg, so_store),
                      "Load and store op proto so failed, path:%s", op_proto_path.c_str());

    const auto op_master_path =
        GetOppPkgPath(vendors[i] + kOpMasterPath + os_cpu_type, kOpMasterPath, kOpHostPath, os_cpu_type, is_sub_pkg);
    GE_ASSERT_SUCCESS(LoadOppSoFromPath(op_master_path, is_split, is_sub_pkg, so_store),
                      "Load and store op master so failed, path:%s", op_master_path.c_str());

    // 保存打包的自定义算子包版本号
    if ((vendors[i].find(kInner) == std::string::npos) && (so_store.GetKernelNum() > last_kernel_num)) {
      std::string compiler_version;
      if (PluginManager::GetVersionFromPathWithName(vendors[i] + kVersionInfo, compiler_version, kCompilerVersion)) {
        (void)compiler_versions.emplace(compiler_version);
      }
    }
  }

  // 加载升级包
  if (is_split) {
    std::vector<std::string> path_vec;
    std::string ops_proto_path;
    if (PluginManager::GetUpgradedOpsProtoPath(ops_proto_path) == ge::SUCCESS) {
      PluginManager::SplitPath(ops_proto_path, path_vec);
      for (const auto &path : path_vec) {
        const auto root_path =
            GetOppPkgPath(path + "/lib/" + os_cpu_type, kOpsProtoPath, kOpsGraphPath, os_cpu_type, is_sub_pkg);
        (void)LoadOppSoFromPath(root_path, true, is_sub_pkg, so_store);
      }
    }
    path_vec.clear();
    std::string op_tiling_path;
    if (PluginManager::GetUpgradedOpMasterPath(op_tiling_path) == ge::SUCCESS) {
      PluginManager::SplitPath(op_tiling_path, path_vec);
      for (const auto &path : path_vec) {
        const auto root_path =
            GetOppPkgPath(path + "/op_tiling/lib/" + os_cpu_type, kOpMasterPath, kOpHostPath, os_cpu_type, is_sub_pkg);
        (void)LoadOppSoFromPath(root_path, true, is_sub_pkg, so_store);
      }
    }
  }
  return SUCCESS;
}
}  // namespace ge
