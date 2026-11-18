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

#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#include "gtest/gtest.h"

namespace ge {
namespace {
class OpsDeliverablesParserST : public testing::Test {
 protected:
  void SetUp() override {
    char name[] = "/tmp/ge_ops_deliverables_parser_st_XXXXXX";
    const int fd = mkstemp(name);
    ASSERT_GE(fd, 0);
    close(fd);
    root_ = name;
    unlink(root_.c_str());
    ASSERT_EQ(mkdir(root_.c_str(), 0700), 0);
    ASSERT_EQ(mkdir((root_ + "/conf").c_str(), 0700), 0);
    setenv("ASCEND_OPP_PATH", root_.c_str(), 1);
  }
  void TearDown() override {
    unlink((root_ + "/libconstant_folding_ops.so").c_str());
    unlink((root_ + "/conf/ops_deliverables.json").c_str());
    rmdir((root_ + "/conf").c_str());
    rmdir(root_.c_str());
    unsetenv("ASCEND_OPP_PATH");
  }
  std::string root_;
};

/**
 * 用例描述：验证真实环境下 parser 从配置解析出 constant-folding-ops 条目指向的路径。
 * 预置条件：在 OPP conf 目录写入合法配置，条目路径指向 OPP 根下真实存在的占位文件。
 * 测试步骤：调用生产 Load 与 GetPath 接口。
 * 预期结果：GetPath 返回占位文件的绝对路径。
 */
TEST_F(OpsDeliverablesParserST, ConfiguredPathIsResolved) {
  std::ofstream(root_ + "/libconstant_folding_ops.so").put('\0');
  std::ofstream config(root_ + "/conf/ops_deliverables.json");
  config << R"({"version":"1.0","deliverables":[{"name":"constant-folding-ops","type":"shared_library",)"
         << R"("paths":["{ASCEND_OPP_PATH}/libconstant_folding_ops.so"]}]})";
  config.close();

  OpsDeliverablesParser parser;
  std::string reason;
  ASSERT_TRUE(parser.Load(reason)) << reason;
  std::string path;
  ASSERT_TRUE(parser.GetPath("constant-folding-ops", {{"ASCEND_OPP_PATH", root_}}, path));
  EXPECT_EQ(path, root_ + "/libconstant_folding_ops.so");
}

/**
 * 用例描述：验证 OpsDeliverablesParser 在配置文件损坏时报告原因并允许上层回退旧流程。
 * 预置条件：在 OPP conf 目录写入不可解析的 JSON。
 * 测试步骤：调用生产 Load 与 GetPath 接口。
 * 预期结果：
 *   1. Load 返回失败且原因非空
 *   2. GetPath 不产生任何路径，调用方按生产逻辑回退硬编码目录发现
 */
TEST_F(OpsDeliverablesParserST, InvalidConfigReturnsReasonForLegacyFallback) {
  std::ofstream config(root_ + "/conf/ops_deliverables.json");
  config << "{invalid json";
  config.close();
  OpsDeliverablesParser parser;
  std::string reason;
  EXPECT_FALSE(parser.Load(reason));
  EXPECT_FALSE(reason.empty());
  std::string path;
  EXPECT_FALSE(parser.GetPath("constant-folding-ops", {{"ASCEND_OPP_PATH", root_}}, path));
}
}  // namespace
}  // namespace ge
