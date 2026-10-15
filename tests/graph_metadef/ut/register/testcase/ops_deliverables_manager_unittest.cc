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

#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include "depends/slog/src/slog_stub.h"
#include "securec.h"

namespace ge {
namespace {
class DeliverablesLog : public SlogStub {
 public:
  DeliverablesLog() {
    SetLevelDebug();
  }
  void Log(int, int level, const char *format, va_list args) override {
    if (level == DLOG_WARN || level == DLOG_ERROR) {
      char buffer[4096]{};
      if (vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1U, format, args) < 0) {
        return;
      }
      if (level == DLOG_WARN) {
        warnings.emplace_back(buffer);
      } else if (std::string(buffer).find("Configured SO load failed") != std::string::npos) {
        load_errors.emplace_back(buffer);
      }
    }
  }
  std::vector<std::string> warnings;
  std::vector<std::string> load_errors;
};

class OpsDeliverablesManagerUT : public testing::Test {
 protected:
  void SetUp() override {
    const char *previous = std::getenv("ASCEND_OPP_PATH");
    had_env_ = previous != nullptr;
    old_env_ = previous == nullptr ? "" : previous;
    char pattern[] = "/tmp/ge_ops_deliverables_ut_XXXXXX";
    const auto directory = mkdtemp(pattern);
    ASSERT_NE(directory, nullptr);
    root_ = directory;
    std::filesystem::create_directories(root_ + "/conf");
    ASSERT_EQ(setenv("ASCEND_OPP_PATH", root_.c_str(), 1), 0);
    logs_ = std::make_shared<DeliverablesLog>();
    SlogStub::SetInstance(logs_);
  }
  void TearDown() override {
    if (had_env_) {
      setenv("ASCEND_OPP_PATH", old_env_.c_str(), 1);
    } else {
      unsetenv("ASCEND_OPP_PATH");
    }
    SlogStub::SetInstance(nullptr);
    if (!root_.empty()) {
      std::filesystem::remove_all(root_);
    }
  }
  void Write(const std::string &text) {
    std::ofstream config(root_ + "/conf/ops_deliverables.json");
    ASSERT_TRUE(config.is_open());
    config << text;
    config.close();
    ASSERT_TRUE(config.good());
  }
  std::string root_;
  std::shared_ptr<DeliverablesLog> logs_;

 private:
  bool had_env_ = false;
  std::string old_env_;
};

TEST_F(OpsDeliverablesManagerUT, MissingConfigIsSilentAndClearsOutput) {
  std::vector<OpsDeliverable> result{{"stale", "shared_library", {"old.so"}}};
  std::string reason;
  EXPECT_FALSE(OpsDeliverablesManager::Load(result, reason));
  EXPECT_TRUE(result.empty());
  EXPECT_NE(reason.find("does not exist"), std::string::npos);
  EXPECT_TRUE(logs_->warnings.empty());
}

TEST_F(OpsDeliverablesManagerUT, RootTrailingSlashUsesSameConfig) {
  ASSERT_EQ(setenv("ASCEND_OPP_PATH", (root_ + "/").c_str(), 1), 0);
  EXPECT_EQ(OpsDeliverablesManager::GetConfigPath(), root_ + "/conf/ops_deliverables.json");
}

TEST_F(OpsDeliverablesManagerUT, ParsesAllGroupsAndPreservesTemplates) {
  Write(R"({"version":"1.0","deliverables":[
    {"name":"ophost","type":"shared_library","paths":["{ASCEND_OPP_PATH}/b.so","{ASCEND_OPP_PATH}/a.so"]},
    {"name":"opgraph","type":"shared_library","paths":["{ASCEND_OPP_PATH}/lib.so"]},
    {"name":"tf-plugin","type":"shared_library","paths":["{ASCEND_OPP_PATH}/framework/tf.so"]},
    {"name":"onnx-plugin","type":"shared_library","paths":["{ASCEND_OPP_PATH}/framework/onnx.so"]},
    {"name":"opkernel","type":"obj_file","paths":["{ASCEND_OPP_PATH}/kernel/{socversion}"]},
    {"name":"opsrc","type":"src_file","paths":["{ASCEND_OPP_PATH}/src/{os}/{arch}"]}]})");
  std::vector<OpsDeliverable> result;
  std::string reason = "old failure";
  ASSERT_TRUE(OpsDeliverablesManager::Load(result, reason));
  ASSERT_EQ(result.size(), 6U);
  EXPECT_EQ(result[0].name, "ophost");
  EXPECT_EQ(result[0].type, "shared_library");
  EXPECT_EQ(result[0].paths, (std::vector<std::string>{"{ASCEND_OPP_PATH}/b.so", "{ASCEND_OPP_PATH}/a.so"}));
  EXPECT_EQ(result[1].paths, (std::vector<std::string>{"{ASCEND_OPP_PATH}/lib.so"}));
  EXPECT_EQ(result[2].name, "tf-plugin");
  EXPECT_EQ(result[3].name, "onnx-plugin");
  EXPECT_EQ(result[4].type, "obj_file");
  EXPECT_EQ(result[5].type, "src_file");
  EXPECT_TRUE(reason.empty());
  EXPECT_TRUE(logs_->warnings.empty());
}

TEST_F(OpsDeliverablesManagerUT, InvalidConfigsNeverPublishPartialResultsAndWarn) {
  const std::vector<std::string> invalid{
      "{invalid",
      "[]",
      R"({"version":"2.0","deliverables":[]})",
      R"({"version":1,"deliverables":[]})",
      R"({"version":"1.0"})",
      R"({"version":"1.0","deliverables":{}})",
      R"({"version":"1.0","deliverables":[{"name":5,"type":"shared_library","paths":[]}]})",
      R"({"version":"1.0","deliverables":[{"name":"a","type":"shared_library","paths":["a.so",1]}]})",
      R"({"version":"1.0","deliverables":[{"name":"a","type":"shared_library","paths":[]},{"name":"a","type":"shared_library","paths":[]}]})",
      R"({"version":"1.0","deliverables":[{"name":"a","type":"shared_library","paths":[]},{"name":"b","type":"shared_library","paths":[""]}]})"};
  for (const auto &config : invalid) {
    SCOPED_TRACE(config);
    Write(config);
    logs_->warnings.clear();
    std::vector<OpsDeliverable> result{{"stale", "shared_library", {"old.so"}}};
    std::string reason;
    EXPECT_FALSE(OpsDeliverablesManager::Load(result, reason));
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(reason.empty());
    ASSERT_EQ(logs_->warnings.size(), 1U);
    EXPECT_NE(logs_->warnings[0].find(root_), std::string::npos);
    EXPECT_NE(logs_->warnings[0].find(reason), std::string::npos);
    EXPECT_NE(logs_->warnings[0].find("legacy"), std::string::npos);
  }
}

TEST_F(OpsDeliverablesManagerUT, RejectsUnsafeOrUnknownPathTemplates) {
  for (const auto &path : {"/tmp/libopgraph.so", "{ASCEND_OPP_PATH}/../outside.so",
                           "{ASCEND_OPP_PATH}/{unknown}/lib.so", "{ASCEND_OPP_PATH}/{socversion/lib.so"}) {
    SCOPED_TRACE(path);
    Write(
        std::string(
            "{\"version\":\"1.0\",\"deliverables\":[{\"name\":\"opgraph\",\"type\":\"shared_library\",\"paths\":[\"") +
        path + "\"]}]}");
    std::vector<OpsDeliverable> result;
    std::string reason;
    EXPECT_FALSE(OpsDeliverablesManager::Load(result, reason));
    EXPECT_TRUE(result.empty());
  }
}

TEST_F(OpsDeliverablesManagerUT, DirectoryInsteadOfConfigWarns) {
  std::filesystem::create_directory(root_ + "/conf/ops_deliverables.json");
  std::vector<OpsDeliverable> result;
  std::string reason;
  EXPECT_FALSE(OpsDeliverablesManager::Load(result, reason));
  EXPECT_TRUE(result.empty());
  EXPECT_FALSE(reason.empty());
  EXPECT_EQ(logs_->warnings.size(), 1U);
}

TEST_F(OpsDeliverablesManagerUT, InvalidThenValidLoadDoesNotCacheFailure) {
  Write("{");
  std::vector<OpsDeliverable> result;
  std::string reason;
  EXPECT_FALSE(OpsDeliverablesManager::Load(result, reason));
  Write(R"({"version":"1.0","deliverables":[]})");
  EXPECT_TRUE(OpsDeliverablesManager::Load(result, reason));
  EXPECT_TRUE(result.empty());
  EXPECT_TRUE(reason.empty());
}

TEST_F(OpsDeliverablesManagerUT, ExpandsAllVariablesAndDeduplicatesInGraphHostOrder) {
  std::filesystem::create_directories(root_ + "/soc/linux/arch");
  std::ofstream(root_ + "/soc/linux/arch/b.so").put('b');
  std::ofstream(root_ + "/soc/linux/arch/a.so").put('a');
  std::ofstream(root_ + "/host.so").put('h');
  const std::vector<OpsDeliverable> entries{
      {"ophost", "shared_library", {"{ASCEND_OPP_PATH}/host.so"}},
      {"opgraph",
       "shared_library",
       {"{ASCEND_OPP_PATH}/{socversion}/{os}/{arch}/b.so", "{ASCEND_OPP_PATH}/{socversion}/{os}/{arch}/*.so"}},
      {"tf-plugin", "shared_library", {"{unknown}/plugin.so"}}};
  const std::map<std::string, std::string> variables{
      {"ASCEND_OPP_PATH", root_}, {"socversion", "soc"}, {"os", "linux"}, {"arch", "arch"}};
  std::vector<std::string> paths;
  std::string reason;
  ASSERT_TRUE(OpsDeliverablesManager::ResolvePaths(entries, variables, paths, reason));
  EXPECT_EQ(paths, (std::vector<std::string>{root_ + "/soc/linux/arch/b.so", root_ + "/soc/linux/arch/a.so",
                                             root_ + "/host.so"}));
  EXPECT_TRUE(reason.empty());
}

TEST_F(OpsDeliverablesManagerUT, MissingOptionalLibraryIsSkipped) {
  std::ofstream(root_ + "/present.so").put('x');
  const std::vector<OpsDeliverable> entries{
      {"opgraph", "shared_library", {root_ + "/missing*.so", root_ + "/present.so"}}};
  std::vector<std::string> paths;
  std::string reason;
  EXPECT_TRUE(OpsDeliverablesManager::ResolvePaths(entries, {{"ASCEND_OPP_PATH", root_}}, paths, reason));
  EXPECT_EQ(paths, (std::vector<std::string>{root_ + "/present.so"}));
}

TEST_F(OpsDeliverablesManagerUT, LegacyLibrariesFollowAllGraphAndHostLibraries) {
  for (const auto *name : {"graph_legacy.so", "graph.so", "host.so"}) {
    std::ofstream(root_ + "/" + name).put('x');
  }
  const std::vector<OpsDeliverable> entries{
      {"opgraph", "shared_library", {root_ + "/graph_legacy.so", root_ + "/graph.so"}},
      {"ophost", "shared_library", {root_ + "/host.so"}}};
  std::vector<std::string> paths;
  std::string reason;
  ASSERT_TRUE(OpsDeliverablesManager::ResolvePaths(entries, {{"ASCEND_OPP_PATH", root_}}, paths, reason));
  EXPECT_EQ(paths, (std::vector<std::string>{root_ + "/graph.so", root_ + "/host.so", root_ + "/graph_legacy.so"}));
}

TEST_F(OpsDeliverablesManagerUT, UnresolvedAndInvalidPathsDiscardPlanBeforeLoading) {
  std::ofstream(root_ + "/present.so").put('x');
  for (const auto &invalid : {"{socversion}/x.so", "{unknown}/x.so", "{os/x.so", "x}.so", root_.c_str()}) {
    SCOPED_TRACE(invalid);
    std::vector<std::string> paths{"stale"};
    std::string reason;
    const std::vector<OpsDeliverable> entries{{"opgraph", "shared_library", {root_ + "/present.so", invalid}}};
    EXPECT_FALSE(OpsDeliverablesManager::ResolvePaths(entries, {{"ASCEND_OPP_PATH", root_}}, paths, reason));
    EXPECT_TRUE(paths.empty());
    EXPECT_FALSE(reason.empty());
  }
}

TEST_F(OpsDeliverablesManagerUT, MissingOptionalGroupsNeedNoLegacyDiscovery) {
  const std::vector<OpsDeliverable> entries{{"opsrc", "src_file", {"{ASCEND_OPP_PATH}/src"}}};
  std::vector<std::string> paths;
  std::string reason;
  EXPECT_TRUE(OpsDeliverablesManager::ResolvePaths(entries, {{"ASCEND_OPP_PATH", root_}}, paths, reason));
  EXPECT_TRUE(paths.empty());
  EXPECT_TRUE(reason.empty());
}

TEST_F(OpsDeliverablesManagerUT, ResolvesFrameworkPluginPathsFromManifestInDeclaredOrder) {
  std::filesystem::create_directories(root_ + "/framework");
  std::ofstream(root_ + "/framework/tf_first.so").put('a');
  std::ofstream(root_ + "/framework/tf_second_legacy.so").put('b');
  std::ofstream(root_ + "/framework/onnx.so").put('c');
  Write(R"({"version":"1.0","deliverables":[
    {"name":"tf-plugin","type":"shared_library","paths":["{ASCEND_OPP_PATH}/framework/tf_first.so","{ASCEND_OPP_PATH}/framework/tf_second_legacy.so"]},
    {"name":"onnx-plugin","type":"shared_library","paths":["{ASCEND_OPP_PATH}/framework/onnx.so"]}]})");
  std::vector<std::string> paths;
  std::string reason;
  ASSERT_TRUE(OpsDeliverablesManager::ResolveFrameworkPluginPaths({}, "tensorflow", paths, reason));
  EXPECT_EQ(paths,
            (std::vector<std::string>{root_ + "/framework/tf_first.so", root_ + "/framework/tf_second_legacy.so"}));
  EXPECT_TRUE(reason.empty());

  paths.assign(1U, "stale");
  EXPECT_FALSE(OpsDeliverablesManager::ResolveFrameworkPluginPaths({}, "caffe", paths, reason));
  EXPECT_TRUE(paths.empty());
  EXPECT_NE(reason.find("caffe-plugin"), std::string::npos);
}

TEST_F(OpsDeliverablesManagerUT, LoadOppPackagesSkipsInvalidLibraries) {
  std::ofstream(root_ + "/bad_first.so").put('a');
  std::ofstream(root_ + "/bad_second.so").put('b');
  Write(R"({"version":"1.0","deliverables":[
    {"name":"opgraph","type":"shared_library","paths":["{ASCEND_OPP_PATH}/bad_first.so",
                                                       "{ASCEND_OPP_PATH}/bad_second.so"]}]})");

  EXPECT_EQ(OpsDeliverablesManager::LoadOppPackages(), GRAPH_SUCCESS);
  ASSERT_EQ(logs_->load_errors.size(), 2U);
  EXPECT_NE(logs_->load_errors[0].find("bad_first.so"), std::string::npos);
  EXPECT_NE(logs_->load_errors[1].find("bad_second.so"), std::string::npos);
}
}  // namespace
}  // namespace ge
