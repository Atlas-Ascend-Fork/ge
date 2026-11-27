/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "fe_llt_utils.h"
#include "common/fe_type_utils.h"
#define protected public
#define private public
#include "common/ops_deliverable_config_manager.h"
#include "common/platform_utils.h"
#undef private
#undef protected

using namespace std;
using namespace fe;

namespace {
constexpr char kConfigRelPath[] = "/conf/ops_deliverables.json";

// 交付件配置单例：生产代码 Initialize 幂等，UT 需按用例显式复位后重新加载
class OpsDeliverableConfigUt : public testing::Test {
 protected:
  void SetUp() override {
    const char *opp_path = std::getenv("ASCEND_OPP_PATH");
    if (opp_path != nullptr) {
      saved_opp_path_ = opp_path;
      has_saved_opp_path_ = true;
    }
    char tmpl[] = "/tmp/fe_ops_deliverable_ut_XXXXXX";
    char *created = mkdtemp(tmpl);
    ASSERT_NE(created, nullptr);
    root_ = created;
    ResetManager();
  }

  void TearDown() override {
    ResetManager();
    if (has_saved_opp_path_) {
      (void)setenv("ASCEND_OPP_PATH", saved_opp_path_.c_str(), 1);
    } else {
      (void)unsetenv("ASCEND_OPP_PATH");
    }
    if (!root_.empty()) {
      std::error_code error;
      (void)std::filesystem::remove_all(root_, error);
    }
  }

  static void ResetManager() {
    OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
    manager.inited_ = false;
    manager.loaded_ = false;
    manager.deliverable_paths_map_.clear();
  }

  void SetOppPath(const std::string &path) const {
    (void)setenv("ASCEND_OPP_PATH", path.c_str(), 1);
  }

  std::string MakeDir(const std::string &relative_path) const {
    const std::string path = root_ + relative_path;
    std::error_code error;
    (void)std::filesystem::create_directories(path, error);
    return path;
  }

  std::string WriteFile(const std::string &relative_path, const std::string &content) const {
    const std::string path = root_ + relative_path;
    const size_t last_slash = relative_path.find_last_of('/');
    if (last_slash != std::string::npos) {
      (void)MakeDir(relative_path.substr(0, last_slash));
    }
    std::ofstream output(path);
    EXPECT_TRUE(output.is_open());
    output << content;
    return path;
  }

  void WriteDeliverablesConfig(const nlohmann::json &deliverables) const {
    nlohmann::json root;
    root["version"] = "1.0";
    root["deliverables"] = deliverables;
    (void)WriteFile(std::string(kConfigRelPath), root.dump(4));
  }

  static std::string RealPathOf(const std::string &path) {
    return GetRealPath(path);
  }

  std::string root_;
  std::string saved_opp_path_;
  bool has_saved_opp_path_ = false;
};

nlohmann::json MakeDeliverable(const std::string &name, const std::string &type,
                               const std::vector<std::string> &paths) {
  nlohmann::json item;
  item["name"] = name;
  item["type"] = type;
  item["paths"] = paths;
  return item;
}
}  // namespace

// 无配置文件：初始化失败，所有 getter 返回空，调用方走原逻辑
TEST_F(OpsDeliverableConfigUt, initialize_without_config_keeps_legacy_behavior) {
  SetOppPath(root_);
  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  EXPECT_FALSE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
  EXPECT_TRUE(manager.GetOpKernelPaths().empty());
  EXPECT_TRUE(manager.GetOpKernelConfigPaths().empty());
  EXPECT_TRUE(manager.GetOpImplModePaths().empty());
  EXPECT_TRUE(manager.GetTilingDevicePaths().empty());
  std::string opsrc_root;
  EXPECT_FALSE(manager.GetOpSrcRoot(opsrc_root));
  std::vector<std::string> paths;
  EXPECT_FALSE(manager.SelectOpKernelPaths("ascend950", true, paths));
  EXPECT_TRUE(paths.empty());
  std::string so_path;
  EXPECT_FALSE(manager.SelectTilingDevicePath("nn", so_path));
}

// json 格式错误：打 warning 后回退原逻辑
TEST_F(OpsDeliverableConfigUt, initialize_with_broken_json_falls_back) {
  SetOppPath(root_);
  (void)WriteFile(std::string(kConfigRelPath), "{\"deliverables\": [}");
  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  EXPECT_FALSE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
}

// 顶层缺少 deliverables 字段：回退原逻辑
TEST_F(OpsDeliverableConfigUt, initialize_without_deliverables_field_falls_back) {
  SetOppPath(root_);
  (void)WriteFile(std::string(kConfigRelPath), "{\"version\": \"1.0\"}");
  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  EXPECT_FALSE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
}

// 六个条目全部配置（含占位符）：逐项加载并可被调用方查询
TEST_F(OpsDeliverableConfigUt, load_all_deliverable_entries) {
  SetOppPath(root_);
  const std::string ops_info_nn = WriteFile("/ops_info/aic-ascend950-ops-info-nn.json", "{}");
  const std::string ops_info_legacy = WriteFile("/ops_info/aic-ascend950-ops-info-legacy.json", "{}");
  const std::string kernel_nn = MakeDir("/kernel/ascend950/ops_nn");
  const std::string kernel_legacy = MakeDir("/kernel/ascend950/ops_legacy");
  const std::string config_nn = MakeDir("/kernel_config/ascend950/ops_nn");
  const std::string config_legacy = MakeDir("/kernel_config/ascend950/ops_legacy");
  const std::string opsrc = MakeDir("/opsrc");
  (void)MakeDir("/opsrc/impl");
  const std::string impl_mode = MakeDir("/impl_mode");
  (void)WriteFile("/impl_mode/ops_allow_hf32_nn.ini", "");
  (void)WriteFile("/lib/libtiling_device_nn.so", "");

  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "json",
                       {"{ASCEND_OPP_PATH}/ops_info/aic-ascend950-ops-info-nn.json",
                        "{ASCEND_OPP_PATH}/ops_info/aic-ascend950-ops-info-legacy.json"}),
       MakeDeliverable("opkernel", "obj_file",
                       {"{ASCEND_OPP_PATH}/kernel/ascend950/ops_nn", "{ASCEND_OPP_PATH}/kernel/ascend950/ops_legacy"}),
       MakeDeliverable("opkernel-config", "json",
                       {"{ASCEND_OPP_PATH}/kernel_config/ascend950/ops_nn",
                        "{ASCEND_OPP_PATH}/kernel_config/ascend950/ops_legacy"}),
       MakeDeliverable("opsrc", "src_file", {"{ASCEND_OPP_PATH}/opsrc"}),
       MakeDeliverable("op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode"}),
       MakeDeliverable("tiling-device", "shared_library", {"{ASCEND_OPP_PATH}/lib/libtiling_device_nn.so"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());

  const std::vector<std::string> ops_info_paths = manager.GetOpsInfoPaths();
  ASSERT_EQ(ops_info_paths.size(), 2U);
  EXPECT_EQ(ops_info_paths[0], RealPathOf(ops_info_nn));
  EXPECT_EQ(ops_info_paths[1], RealPathOf(ops_info_legacy));

  const std::vector<std::string> kernel_paths = manager.GetOpKernelPaths();
  ASSERT_EQ(kernel_paths.size(), 2U);
  EXPECT_EQ(kernel_paths[0], RealPathOf(kernel_nn));
  EXPECT_EQ(kernel_paths[1], RealPathOf(kernel_legacy));

  const std::vector<std::string> config_paths = manager.GetOpKernelConfigPaths();
  ASSERT_EQ(config_paths.size(), 2U);
  EXPECT_EQ(config_paths[0], RealPathOf(config_nn));
  EXPECT_EQ(config_paths[1], RealPathOf(config_legacy));

  const std::vector<std::string> impl_mode_paths = manager.GetOpImplModePaths();
  ASSERT_EQ(impl_mode_paths.size(), 1U);
  EXPECT_EQ(impl_mode_paths[0], RealPathOf(impl_mode));

  std::string opsrc_root;
  ASSERT_TRUE(manager.GetOpSrcRoot(opsrc_root));
  EXPECT_EQ(opsrc_root, RealPathOf(opsrc));
}

// 按 soc 段筛选 opkernel / opkernel-config 路径
TEST_F(OpsDeliverableConfigUt, select_op_kernel_paths_by_soc) {
  SetOppPath(root_);
  (void)MakeDir("/kernel/ascend950/ops_nn");
  (void)MakeDir("/kernel/ascend910b/ops_nn");
  (void)MakeDir("/kernel_config/ascend950/ops_nn");
  (void)MakeDir("/kernel_config/ascend910b/ops_nn");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("opkernel", "obj_file",
                       {"{ASCEND_OPP_PATH}/kernel/ascend950/ops_nn", "{ASCEND_OPP_PATH}/kernel/ascend910b/ops_nn"}),
       MakeDeliverable("opkernel-config", "json",
                       {"{ASCEND_OPP_PATH}/kernel_config/ascend950/ops_nn",
                        "{ASCEND_OPP_PATH}/kernel_config/ascend910b/ops_nn"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());

  std::vector<std::string> paths;
  ASSERT_TRUE(manager.SelectOpKernelPaths("ascend950", true, paths));
  ASSERT_EQ(paths.size(), 1U);
  EXPECT_EQ(paths[0], RealPathOf(root_ + "/kernel/ascend950/ops_nn"));

  ASSERT_TRUE(manager.SelectOpKernelPaths("ascend910b", false, paths));
  ASSERT_EQ(paths.size(), 1U);
  EXPECT_EQ(paths[0], RealPathOf(root_ + "/kernel_config/ascend910b/ops_nn"));

  EXPECT_FALSE(manager.SelectOpKernelPaths("ascend310", true, paths));
  EXPECT_TRUE(paths.empty());
  EXPECT_FALSE(manager.SelectOpKernelPaths("", true, paths));
}

// tiling-device 按 so 后缀匹配：前缀段命中返回 real path，未命中返回 false
TEST_F(OpsDeliverableConfigUt, select_tiling_device_path_by_prefix) {
  SetOppPath(root_);
  const std::string nn_so = WriteFile("/lib/libtiling_device_nn.so", "");
  (void)WriteFile("/lib/libtiling_device_legacy.so", "");
  WriteDeliverablesConfig(nlohmann::json::array({MakeDeliverable(
      "tiling-device", "shared_library",
      {"{ASCEND_OPP_PATH}/lib/libtiling_device_nn.so", "{ASCEND_OPP_PATH}/lib/libtiling_device_legacy.so"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());

  std::string so_path;
  ASSERT_TRUE(manager.SelectTilingDevicePath("nn", so_path));
  EXPECT_EQ(so_path, RealPathOf(nn_so));
  EXPECT_FALSE(manager.SelectTilingDevicePath("transformer", so_path));
  EXPECT_FALSE(manager.SelectTilingDevicePath("", so_path));
}

// 条目 type 不匹配：该条目整体丢弃，不影响其它条目
TEST_F(OpsDeliverableConfigUt, entry_with_type_mismatch_is_dropped) {
  SetOppPath(root_);
  const std::string impl_mode = MakeDir("/impl_mode");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "obj_file", {"{ASCEND_OPP_PATH}/ops_info/aic-ascend950-ops-info-nn.json"}),
       MakeDeliverable("op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
  const std::vector<std::string> impl_mode_paths = manager.GetOpImplModePaths();
  ASSERT_EQ(impl_mode_paths.size(), 1U);
  EXPECT_EQ(impl_mode_paths[0], RealPathOf(impl_mode));
}

// 不安全路径（..）与未知占位符：该条目整体丢弃
TEST_F(OpsDeliverableConfigUt, unsafe_or_unknown_template_is_rejected) {
  SetOppPath(root_);
  (void)MakeDir("/impl_mode");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "json", {"{ASCEND_OPP_PATH}/../etc/passwd", "{not_exist_variable}/ops-info.json"}),
       MakeDeliverable("op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
  EXPECT_EQ(manager.GetOpImplModePaths().size(), 1U);
}

// 路径不存在：该条目整体丢弃
TEST_F(OpsDeliverableConfigUt, entry_with_missing_path_is_dropped) {
  SetOppPath(root_);
  (void)MakeDir("/impl_mode");
  WriteDeliverablesConfig(
      nlohmann::json::array({MakeDeliverable("ops-info", "json", {"{ASCEND_OPP_PATH}/ops_info/not_exist.json"}),
                             MakeDeliverable("op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode"})}));

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpsInfoPaths().empty());
}

// 占位符展开：合法模板、未知变量、括号不匹配、变量缺失、.. 拒绝
TEST_F(OpsDeliverableConfigUt, expand_variables_cases) {
  SetOppPath(root_);
  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  const std::map<std::string, std::string> variables = manager.MakeVariables();
  ASSERT_FALSE(variables.at("ASCEND_OPP_PATH").empty());

  std::string output;
  EXPECT_TRUE(manager.ExpandVariables("{ASCEND_OPP_PATH}/kernel/ascend950", variables, output));
  EXPECT_EQ(output, root_ + "/kernel/ascend950");

  EXPECT_FALSE(manager.ExpandVariables("{unknown}/kernel", variables, output));
  EXPECT_FALSE(manager.ExpandVariables("{ASCEND_OPP_PATH/kernel", variables, output));
  EXPECT_FALSE(manager.ExpandVariables("{ASCEND_OPP_PATH}/kernel}", variables, output));
  EXPECT_FALSE(manager.ExpandVariables("{ASCEND_OPP_PATH}/../escape", variables, output));

  std::map<std::string, std::string> variables_without_opp = variables;
  variables_without_opp.erase("ASCEND_OPP_PATH");
  EXPECT_FALSE(manager.ExpandVariables("{ASCEND_OPP_PATH}/kernel", variables_without_opp, output));

  std::string saved_short_soc_version = PlatformUtils::Instance().short_soc_version_;
  PlatformUtils::Instance().short_soc_version_ = "Ascend950";
  const std::map<std::string, std::string> soc_variables = manager.MakeVariables();
  EXPECT_TRUE(manager.ExpandVariables("{socversion}/ops_nn", soc_variables, output));
  EXPECT_EQ(output, "ascend950/ops_nn");
  PlatformUtils::Instance().short_soc_version_ = saved_short_soc_version;
}

// 路径工具函数：分包前缀提取、fusion_ops 目录识别、soc 段匹配
TEST_F(OpsDeliverableConfigUt, path_helper_functions) {
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/a/kernel/ascend950/ops_nn"), "ops_nn");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/a/kernel/ascend950/ops_nn/"), "ops_nn");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/a/kernel/ascend950"), "");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix(""), "");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("ops_nn"), "ops_nn");

  EXPECT_TRUE(OpsDeliverableConfigManager::IsFusionOpsDir("/a/kernel/ascend950/fusion_ops"));
  EXPECT_TRUE(OpsDeliverableConfigManager::IsFusionOpsDir("/a/kernel/ascend950/fusion_ops/"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsFusionOpsDir("/a/kernel/ascend950/fusion_ops_x"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsFusionOpsDir("/a/kernel/ascend950/ops_legacy"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsFusionOpsDir(""));

  EXPECT_TRUE(OpsDeliverableConfigManager::IsPathMatchSoc("/a/ascend950/ops_nn", "Ascend950"));
  EXPECT_TRUE(OpsDeliverableConfigManager::IsPathMatchSoc("/a/ASCEND950/ops_nn", "ascend950"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsPathMatchSoc("/a/ascend950b/ops_nn", "ascend950"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsPathMatchSoc("/a/ascend950/ops_nn", ""));
}
