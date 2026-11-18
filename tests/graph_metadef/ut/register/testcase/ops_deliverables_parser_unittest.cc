/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "common/plugin/ops_deliverables_parser.h"

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "depends/slog/src/slog_stub.h"
#include "securec.h"

namespace ge {
namespace {
class ParserLog : public SlogStub {
 public:
  ParserLog() {
    SetLevelDebug();
  }
  void Log(int, int level, const char *format, va_list args) override {
    if (level == DLOG_WARN || level == DLOG_ERROR) {
      char buffer[4096]{};
      if (vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1U, format, args) < 0) {
        return;
      }
      warnings.emplace_back(buffer);
    }
  }
  std::vector<std::string> warnings;
};

class OpsDeliverablesParserUT : public testing::Test {
 protected:
  void SetUp() override {
    const char *previous = std::getenv("ASCEND_OPP_PATH");
    had_env_ = previous != nullptr;
    old_env_ = previous == nullptr ? "" : previous;
    char pattern[] = "/tmp/ge_ops_deliverables_parser_ut_XXXXXX";
    const auto directory = mkdtemp(pattern);
    ASSERT_NE(directory, nullptr);
    char canonical[PATH_MAX]{};
    root_ = realpath(directory, canonical) == nullptr ? directory : canonical;
    std::filesystem::create_directories(root_ + "/conf");
    ASSERT_EQ(setenv("ASCEND_OPP_PATH", root_.c_str(), 1), 0);
    logs_ = std::make_shared<ParserLog>();
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
  // Creates an empty regular file below the fake OPP root and returns its path.
  std::string Touch(const std::string &relative) {
    const std::string path = root_ + "/" + relative;
    std::filesystem::create_directories(path.substr(0U, path.rfind('/')));
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    return path;
  }
  std::map<std::string, std::string> BaseVariables() const {
    return {{"ASCEND_OPP_PATH", root_}, {"os", "linux"}, {"arch", "aarch64"}};
  }
  bool LoadDefault(OpsDeliverablesParser &parser, std::string &reason) {
    return parser.Load(reason);
  }
  std::string root_;
  std::shared_ptr<ParserLog> logs_;

 private:
  bool had_env_ = false;
  std::string old_env_;
};

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenEnvUnset) {
  ASSERT_EQ(unsetenv("ASCEND_OPP_PATH"), 0);
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(parser.Load(reason));
  EXPECT_NE(reason.find("empty"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_Succeeds_WhenEnvHasTrailingSlashes) {
  const std::string so_path = Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  ASSERT_EQ(setenv("ASCEND_OPP_PATH", (root_ + "//").c_str(), 1), 0);
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
  EXPECT_EQ(path, so_path);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenConfigFileMissing) {
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("does not exist"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenJsonIsBroken) {
  Write(R"({"version": "1.0")");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_FALSE(reason.empty());
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenVersionUnsupported) {
  Write(R"({"version": "2.0", "deliverables": []})");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("unsupported version"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenEntryLacksRequiredField) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library"}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("invalid deliverable entry"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenDuplicateNames) {
  Write(R"({"version": "1.0", "deliverables": [)"
        R"({"name": "ops-host-cpu", "type": "shared_library", "paths": []},)"
        R"({"name": "ops-host-cpu", "type": "shared_library", "paths": []}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("duplicate deliverable name"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenPathTemplateContainsParentDir) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/../libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("safe supported template"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_FailsWithReason_WhenPathTemplateHasUnclosedBrace) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_NE(reason.find("safe supported template"), std::string::npos);
}

TEST_F(OpsDeliverablesParserUT, Load_SucceedsSilently_WhenManifestValid) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/lib/{os}/{arch}/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason = "stale failure";
  EXPECT_TRUE(LoadDefault(parser, reason));
  EXPECT_TRUE(reason.empty());
  EXPECT_TRUE(logs_->warnings.empty());
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenNotLoaded) {
  OpsDeliverablesParser parser;
  std::string path = "stale";
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsCanonicalPath_WhenAllPlaceholdersResolved) {
  const std::string so_path = Touch("built-in/op_impl/host_cpu/lib/linux/aarch64/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/lib/{os}/{arch}/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
  EXPECT_EQ(path, so_path);
}

TEST_F(OpsDeliverablesParserUT, GetPath_FallsBackToNextCandidate_WhenFirstFileAbsent) {
  const std::string so_path = Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/first/libops_host_cpu.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
  EXPECT_EQ(path, so_path);
}

TEST_F(OpsDeliverablesParserUT, GetPath_FallsBackToNextCandidate_WhenFirstPlaceholderMissing) {
  const std::string so_path = Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/{socversion}/libops_host_cpu.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
  EXPECT_EQ(path, so_path);
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenKeyUndefined) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ophost", "type": "shared_library", "paths": []}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenNoCandidateExists) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path = "stale";
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenCandidateIsDirectory) {
  std::filesystem::create_directories(root_ + "/built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenVariableValueIsEmpty) {
  // The file below would match ".../lib//libops_host_cpu.so" if an empty value were wrongly accepted.
  Touch("built-in/op_impl/host_cpu/lib/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/lib/{arch}/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  auto variables = BaseVariables();
  variables["arch"] = "";
  std::string path;
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", variables, path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ReturnsFalse_WhenResolvedPathIsRelative) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["lib/{os}/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPath_ResolvesMultipleKeys_AfterSingleLoad) {
  const std::string folding_path = Touch("built-in/op_impl/host_cpu/libconstant_folding_ops.so");
  const std::string host_cpu_path = Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [)"
        R"({"name": "constant-folding-ops", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libconstant_folding_ops.so"]},)"
        R"({"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  const auto variables = BaseVariables();
  std::string path;
  EXPECT_TRUE(parser.GetPath("constant-folding-ops", variables, path));
  EXPECT_EQ(path, folding_path);
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", variables, path));
  EXPECT_EQ(path, host_cpu_path);
}

TEST_F(OpsDeliverablesParserUT, Load_ClearsStoredEntries_WhenReloadFails) {
  Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", )"
        R"("paths": ["{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::string path;
  EXPECT_TRUE(parser.GetPath("ops-host-cpu", BaseVariables(), path));

  Write("{ not json");
  EXPECT_FALSE(LoadDefault(parser, reason));
  EXPECT_FALSE(parser.GetPath("ops-host-cpu", BaseVariables(), path));
}

TEST_F(OpsDeliverablesParserUT, GetPaths_ReturnsAllValidPaths_InConfiguredOrder) {
  const std::string first = Touch("built-in/op_impl/host_cpu/first.so");
  const std::string second = Touch("built-in/op_impl/host_cpu/second.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/first.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/missing.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/second.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::vector<std::string> paths;
  EXPECT_TRUE(parser.GetPaths("ops-host-cpu", BaseVariables(), paths));
  EXPECT_EQ(paths, (std::vector<std::string>{first, second}));
}

TEST_F(OpsDeliverablesParserUT, GetPaths_DeduplicatesCanonicalDuplicates) {
  const std::string so_path = Touch("built-in/op_impl/host_cpu/libops_host_cpu.so");
  const std::string alias = root_ + "/built-in/op_impl/host_cpu/alias.so";
  ASSERT_EQ(symlink("libops_host_cpu.so", alias.c_str()), 0);
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/libops_host_cpu.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/alias.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::vector<std::string> paths;
  EXPECT_TRUE(parser.GetPaths("ops-host-cpu", BaseVariables(), paths));
  EXPECT_EQ(paths, (std::vector<std::string>{so_path}));
}

TEST_F(OpsDeliverablesParserUT, GetPaths_FailsAndClearsOutput_WhenNoCandidateResolves) {
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/missing.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::vector<std::string> paths{"stale"};
  EXPECT_FALSE(parser.GetPaths("ops-host-cpu", BaseVariables(), paths));
  EXPECT_TRUE(paths.empty());
}

TEST_F(OpsDeliverablesParserUT, GetPaths_OnlyFailsSingleCandidate_WhenPlaceholderMissing) {
  const std::string first = Touch("built-in/op_impl/host_cpu/first.so");
  const std::string second = Touch("built-in/op_impl/host_cpu/second.so");
  Write(R"({"version": "1.0", "deliverables": [{"name": "ops-host-cpu", "type": "shared_library", "paths": [)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/{socversion}/first.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/first.so",)"
        R"("{ASCEND_OPP_PATH}/built-in/op_impl/host_cpu/second.so"]}]})");
  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(LoadDefault(parser, reason));
  std::vector<std::string> paths;
  EXPECT_TRUE(parser.GetPaths("ops-host-cpu", BaseVariables(), paths));
  EXPECT_EQ(paths, (std::vector<std::string>{first, second}));
}
}  // namespace
}  // namespace ge
