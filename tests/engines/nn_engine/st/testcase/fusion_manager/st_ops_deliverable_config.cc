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
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "fe_llt_utils.h"
#include "common/fe_type_utils.h"
#define protected public
#define private public
#include "common/ops_deliverable_config_manager.h"
#include "common/configuration.h"
#include "common/config_parser/op_impl_mode_config_parser.h"
#include "common/platform_utils.h"
#undef private
#undef protected
#include "common/aicore_util_constants.h"
#include "common/aicore_util_types.h"
#include "ops_store/ops_kernel_error_codes.h"
#include "ops_store/sub_op_info_store.h"

using namespace std;
using namespace fe;

namespace {
constexpr char kConfigRelPath[] = "/conf/ops_deliverables.json";
// 现网内置包中真实的信息库 json，用于验证"配置到具体 json 文件"的加载链路
constexpr char kStubOpsInfoRelPath[] =
    "/tests/engines/nn_engine/depends/CANN_910b_stub/cann/opp/built-in/op_impl/ai_core/tbe/config/ascend910b/"
    "aic-ascend910b-ops-info-nn.json";
constexpr char kStubOpType[] = "AdaLayerNorm";

class StOpsDeliverableConfig : public testing::Test {
 protected:
  void SetUp() override {
    const char *opp_path = std::getenv("ASCEND_OPP_PATH");
    if (opp_path != nullptr) {
      saved_opp_path_ = opp_path;
      has_saved_opp_path_ = true;
    }
    char tmpl[] = "/tmp/fe_ops_deliverable_st_XXXXXX";
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

  std::string MakeDir(const std::string &relative_path) const {
    const std::string path = root_ + relative_path;
    std::error_code error;
    (void)std::filesystem::create_directories(path, error);
    return path;
  }

  void WriteTextFile(const std::string &relative_path, const std::string &content) const {
    const std::string path = root_ + relative_path;
    const size_t last_slash = relative_path.find_last_of('/');
    if (last_slash != std::string::npos) {
      (void)MakeDir(relative_path.substr(0, last_slash));
    }
    std::ofstream output(path);
    EXPECT_TRUE(output.is_open());
    output << content;
  }

  void WriteDeliverablesConfig(const nlohmann::json &deliverables) const {
    nlohmann::json root;
    root["version"] = "1.0";
    root["deliverables"] = deliverables;
    WriteTextFile(std::string(kConfigRelPath), root.dump(4));
  }

  nlohmann::json MakeDeliverable(const std::string &name, const std::string &type,
                                 const std::vector<std::string> &paths) const {
    nlohmann::json item;
    item["name"] = name;
    item["type"] = type;
    item["paths"] = paths;
    return item;
  }

  // 复制现网信息库 json 到临时 OPP 树，供"配置指定的信息库文件"加载
  std::string CopyStubOpsInfoJson(const std::string &relative_path) const {
    const std::string source = GetCodeDir() + kStubOpsInfoRelPath;
    const std::string dest = root_ + relative_path;
    const size_t last_slash = relative_path.find_last_of('/');
    (void)MakeDir(relative_path.substr(0, last_slash));
    std::error_code error;
    (void)std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, error);
    EXPECT_FALSE(static_cast<bool>(error));
    return dest;
  }

  FEOpsStoreInfo MakeStoreInfo(bool is_custom_store) const {
    FEOpsStoreInfo store_info(2, is_custom_store ? "tbe-custom" : "tbe-builtin", EN_IMPL_HW_TBE,
                              root_ + "/legacy_missing/ascend910b", root_ + "/legacy_missing/impl", true, true,
                              is_custom_store);
    return store_info;
  }

  std::string root_;
  std::string saved_opp_path_;
  bool has_saved_opp_path_ = false;
};

// 配置指定的信息库 json 生效：即使 legacy 信息库目录不存在，也能加载算子信息并继承分包前缀
TEST_F(StOpsDeliverableConfig, configured_ops_info_is_loaded_when_legacy_dir_missing) {
  (void)CopyStubOpsInfoJson("/ops_info/aic-ascend910b-ops-info-nn.json");
  (void)MakeDir("/opsrc/impl");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "json", {"{ASCEND_OPP_PATH}/ops_info/aic-ascend910b-ops-info-nn.json"}),
       MakeDeliverable("opsrc", "src_file", {"{ASCEND_OPP_PATH}/opsrc"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  SubOpInfoStore sub_store(MakeStoreInfo(false));
  ASSERT_EQ(sub_store.Initialize(AI_CORE_NAME), SUCCESS);
  const auto &kernels = sub_store.GetAllOpKernels();
  EXPECT_FALSE(kernels.empty());
  OpKernelInfoPtr op_kernel_info = sub_store.GetOpKernelByOpType(kStubOpType);
  ASSERT_NE(op_kernel_info, nullptr);
  // 分包前缀来自配置的信息库文件名（aic-ascend910b-ops-info-nn.json -> ops_nn），供 opkernel 配置配对使用
  EXPECT_EQ(op_kernel_info->GetOpsPathNamePrefix(), "ops_nn");
}

// 无配置时保持原逻辑：legacy 信息库目录不存在则返回目录不存在错误
TEST_F(StOpsDeliverableConfig, without_config_missing_legacy_dir_returns_not_exist) {
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  SubOpInfoStore sub_store(MakeStoreInfo(false));
  EXPECT_EQ(sub_store.Initialize(AI_CORE_NAME), OP_STORE_CFG_FILE_NOT_EXIST);
}

// custom 信息库不受配置影响：即便配置了 ops-info，也仍走自身 legacy 目录
TEST_F(StOpsDeliverableConfig, custom_store_ignores_ops_info_config) {
  (void)CopyStubOpsInfoJson("/ops_info/aic-ascend910b-ops-info-nn.json");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "json", {"{ASCEND_OPP_PATH}/ops_info/aic-ascend910b-ops-info-nn.json"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  SubOpInfoStore custom_store(MakeStoreInfo(true));
  EXPECT_EQ(custom_store.Initialize(AI_CORE_NAME), OP_STORE_CFG_FILE_NOT_EXIST);
}

// 真实目录树下的 opkernel / opkernel-config / tiling-device 选择
TEST_F(StOpsDeliverableConfig, select_paths_by_soc_and_prefix_on_real_tree) {
  (void)MakeDir("/kernel/ascend910b/ops_nn");
  (void)MakeDir("/kernel/ascend910b/ops_legacy");
  (void)MakeDir("/kernel_config/ascend910b/ops_nn");
  (void)MakeDir("/kernel_config/ascend910b/ops_legacy");
  (void)WriteTextFile("/lib/libtiling_device_nn.so", "");
  (void)WriteTextFile("/lib/libtiling_device_legacy.so", "");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable(
           "opkernel", "obj_file",
           {"{ASCEND_OPP_PATH}/kernel/ascend910b/ops_nn", "{ASCEND_OPP_PATH}/kernel/ascend910b/ops_legacy"}),
       MakeDeliverable("opkernel-config", "json",
                       {"{ASCEND_OPP_PATH}/kernel_config/ascend910b/ops_nn",
                        "{ASCEND_OPP_PATH}/kernel_config/ascend910b/ops_legacy"}),
       MakeDeliverable(
           "tiling-device", "shared_library",
           {"{ASCEND_OPP_PATH}/lib/libtiling_device_nn.so", "{ASCEND_OPP_PATH}/lib/libtiling_device_legacy.so"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  std::vector<std::string> kernel_paths;
  ASSERT_TRUE(manager.SelectOpKernelPaths("ascend910b", true, kernel_paths));
  ASSERT_EQ(kernel_paths.size(), 2U);
  EXPECT_EQ(GetRealPath(root_ + "/kernel/ascend910b/ops_nn"), kernel_paths[0]);
  EXPECT_EQ(GetRealPath(root_ + "/kernel/ascend910b/ops_legacy"), kernel_paths[1]);

  std::vector<std::string> config_paths;
  ASSERT_TRUE(manager.SelectOpKernelPaths("ascend910b", false, config_paths));
  EXPECT_EQ(config_paths.size(), 2U);
  EXPECT_FALSE(manager.SelectOpKernelPaths("ascend310", true, config_paths));

  std::string so_path;
  ASSERT_TRUE(manager.SelectTilingDevicePath("nn", so_path));
  EXPECT_EQ(GetRealPath(root_ + "/lib/libtiling_device_nn.so"), so_path);
  ASSERT_TRUE(manager.SelectTilingDevicePath("legacy", so_path));
  EXPECT_FALSE(manager.SelectTilingDevicePath("transformer", so_path));
}

// op-impl-mode 配置生效时返回配置目录列表；未配置时返回空 list（调用方自行回退内置路径）
TEST_F(StOpsDeliverableConfig, get_impl_mode_paths_from_config) {
  (void)MakeDir("/impl_mode/main");
  (void)MakeDir("/impl_mode/sub");
  WriteDeliverablesConfig(nlohmann::json::array({MakeDeliverable(
      "op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode/main", "{ASCEND_OPP_PATH}/impl_mode/sub"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  std::vector<std::string> impl_mode_paths = manager.GetOpImplModePaths();
  ASSERT_EQ(impl_mode_paths.size(), 2U);
  EXPECT_EQ(GetRealPath(root_ + "/impl_mode/main"), impl_mode_paths[0]);
  EXPECT_EQ(GetRealPath(root_ + "/impl_mode/sub"), impl_mode_paths[1]);

  // 无配置文件时 Initialize 失败，未加载的条目返回空 list
  ResetManager();
  (void)setenv("ASCEND_OPP_PATH", (root_ + "/no_config").c_str(), 1);
  EXPECT_FALSE(manager.Initialize());
  EXPECT_TRUE(manager.GetOpImplModePaths().empty());
}

// opsrc 根目录：正常返回配置根；尾斜杠被剥掉；仅剩 "/" 时返回 false
TEST_F(StOpsDeliverableConfig, get_opsrc_root_strips_trailing_slash) {
  (void)MakeDir("/opsrc/impl");
  WriteDeliverablesConfig(nlohmann::json::array({MakeDeliverable("opsrc", "src_file", {"{ASCEND_OPP_PATH}/opsrc"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());
  std::string src_root;
  ASSERT_TRUE(manager.GetOpSrcRoot(src_root));
  EXPECT_EQ(GetRealPath(root_ + "/opsrc"), src_root);

  // 防御性剥离尾斜杠；剥离后为空则返回 false
  manager.deliverable_paths_map_["opsrc"] = {root_ + "/opsrc/"};
  ASSERT_TRUE(manager.GetOpSrcRoot(src_root));
  EXPECT_EQ(root_ + "/opsrc", src_root);
  manager.deliverable_paths_map_["opsrc"] = {"/"};
  EXPECT_FALSE(manager.GetOpSrcRoot(src_root));
}

// 分包前缀取路径末段：以 ops_ 开头即为分包名，尾斜杠不影响；非分包目录返回空串
TEST_F(StOpsDeliverableConfig, extract_ops_path_name_prefix_from_last_segment) {
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/xxx/kernel/ascend910b/ops_nn"), "ops_nn");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/xxx/kernel/ascend910b/ops_legacy/"), "ops_legacy");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("ops_transformer"), "ops_transformer");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix("/xxx/kernel/ascend910b"), "");
  EXPECT_EQ(OpsDeliverableConfigManager::ExtractOpsPathNamePrefix(""), "");
}

// fusion_ops 目录识别：只看末段，忽略尾斜杠
TEST_F(StOpsDeliverableConfig, fusion_ops_dir_detection) {
  EXPECT_TRUE(OpsDeliverableConfigManager::IsFusionOpsDir("/xxx/kernel/ascend910b/fusion_ops"));
  EXPECT_TRUE(OpsDeliverableConfigManager::IsFusionOpsDir("/xxx/kernel/ascend910b/fusion_ops/"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsFusionOpsDir("/xxx/kernel/ascend910b/ops_nn"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsFusionOpsDir(""));
}

// soc 匹配：空 soc 恒为 false；按完整路径段、忽略大小写匹配
TEST_F(StOpsDeliverableConfig, is_path_match_soc_by_full_segment) {
  EXPECT_FALSE(OpsDeliverableConfigManager::IsPathMatchSoc("/xxx/kernel/ascend910b/ops_nn", ""));
  EXPECT_TRUE(OpsDeliverableConfigManager::IsPathMatchSoc("/xxx/kernel/Ascend910B/ops_nn", "ascend910b"));
  EXPECT_FALSE(OpsDeliverableConfigManager::IsPathMatchSoc("/xxx/kernel/ascend910b1/ops_nn", "ascend910b"));
}

// 带 opsrc 的交付件配置生效时，built-in 信息库的 op_impl_file_path 被替换为 <opsrc>/impl
TEST_F(StOpsDeliverableConfig, configuration_replaces_op_impl_path_with_opsrc_impl) {
  (void)CopyStubOpsInfoJson("/ops_info/aic-ascend910b-ops-info-nn.json");
  (void)MakeDir("/opsrc/impl");
  WriteDeliverablesConfig(nlohmann::json::array(
      {MakeDeliverable("ops-info", "json", {"{ASCEND_OPP_PATH}/ops_info/aic-ascend910b-ops-info-nn.json"}),
       MakeDeliverable("opsrc", "src_file", {"{ASCEND_OPP_PATH}/opsrc"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  OpsDeliverableConfigManager &manager = OpsDeliverableConfigManager::Instance();
  ASSERT_TRUE(manager.Initialize());

  Configuration &configuration = Configuration::Instance(AI_CORE_NAME);
  const std::vector<FEOpsStoreInfo> saved_store_infos = configuration.ops_store_info_vector_;
  configuration.ops_store_info_vector_.clear();
  std::string op_store_name = "tbe-builtin";
  std::vector<std::string> op_store_vector = {"10086",
                                              std::to_string(static_cast<int64_t>(EN_IMPL_HW_TBE)),
                                              "/deliverable/cfg/ascend910b",
                                              "/deliverable/impl",
                                              "true",
                                              "true",
                                              "true"};
  FEOpsStoreInfo op_store_info;
  const Status status = configuration.AssembleEachOpsStoreInfo(op_store_name, op_store_vector, op_store_info);
  configuration.ops_store_info_vector_ = saved_store_infos;
  ASSERT_EQ(status, SUCCESS);
  EXPECT_EQ(GetRealPath(root_ + "/opsrc/impl"), op_store_info.op_impl_file_path);
  // cfg_file_path 仍在 fe.ini 值后拼 short soc 子目录（全路径模式下为配置值 + /<soc>）
  EXPECT_NE(std::string::npos, op_store_info.cfg_file_path.find("/deliverable/cfg/ascend910b/"));
}

// impl_mode 目录来自交付件配置；未命中配置时回退内置相对路径
TEST_F(StOpsDeliverableConfig, impl_mode_parser_reads_dirs_from_deliverable_config) {
  (void)MakeDir("/impl_mode/main");
  (void)MakeDir("/impl_mode/sub");
  WriteDeliverablesConfig(nlohmann::json::array({MakeDeliverable(
      "op-impl-mode", "file_path", {"{ASCEND_OPP_PATH}/impl_mode/main", "{ASCEND_OPP_PATH}/impl_mode/sub"})}));
  (void)setenv("ASCEND_OPP_PATH", root_.c_str(), 1);

  OpImplModeConfigParser parser(root_ + "/");
  const std::vector<std::string> configured_dirs = parser.GetImplModeDirPaths();
  ASSERT_EQ(configured_dirs.size(), 2U);
  EXPECT_EQ(GetRealPath(root_ + "/impl_mode/main"), configured_dirs[0]);
  EXPECT_EQ(GetRealPath(root_ + "/impl_mode/sub"), configured_dirs[1]);

  ResetManager();
  (void)setenv("ASCEND_OPP_PATH", (root_ + "/no_config").c_str(), 1);
  const std::vector<std::string> fallback_dirs = parser.GetImplModeDirPaths();
  ASSERT_EQ(fallback_dirs.size(), 1U);
  EXPECT_EQ(root_ + "/built-in/op_impl/ai_core/tbe/impl_mode/", fallback_dirs[0]);
}
}  // namespace
