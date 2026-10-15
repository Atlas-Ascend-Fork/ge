/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/plugin/ops_deliverables_manager.h"
#include "base/registry/op_impl_space_registry_v2.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <map>

#include "gtest/gtest.h"

namespace ge {
namespace {
class OpsDeliverablesST : public testing::Test {
 protected:
  void SetUp() override {
    char name[] = "/tmp/ge_ops_deliverables_st_XXXXXX";
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
    unlink((root_ + "/a.so").c_str());
    unlink((root_ + "/b.so").c_str());
    unlink((root_ + "/ops_deliverables_fixture.so").c_str());
    unlink((root_ + "/conf/ops_deliverables.json").c_str());
    rmdir((root_ + "/conf").c_str());
    rmdir(root_.c_str());
    unsetenv("ASCEND_OPP_PATH");
  }
  std::string root_;
};

/**
 * 用例描述：验证交付件配置驱动的路径展开、通配符排序和重复路径消除。
 * 预置条件：创建真实配置文件和两个可访问的共享对象占位文件。
 * 测试步骤：加载配置并使用生产 ResolvePaths 展开路径。
 * 预期结果：路径按配置顺序返回且重复项只保留一次。
 */
TEST_F(OpsDeliverablesST, ResolveConfiguredPathsInOrderAndDeduplicate) {
  std::ofstream(root_ + "/a.so").put('\0');
  std::ofstream(root_ + "/b.so").put('\0');
  std::ofstream config(root_ + "/conf/ops_deliverables.json");
  config
      << R"({"version":"1.0","deliverables":[{"name":"opgraph","type":"shared_library","paths":["{ASCEND_OPP_PATH}/a.so","{ASCEND_OPP_PATH}/*.so","{ASCEND_OPP_PATH}/a.so"]}]})";
  config.close();

  std::vector<OpsDeliverable> deliverables;
  std::string reason;
  ASSERT_TRUE(OpsDeliverablesManager::Load(deliverables, reason)) << reason;
  std::vector<std::string> paths;
  ASSERT_TRUE(OpsDeliverablesManager::ResolvePaths(deliverables, {{"ASCEND_OPP_PATH", root_}}, paths, reason))
      << reason;
  ASSERT_EQ(paths.size(), 2U);
  EXPECT_NE(paths[0].find("a.so"), std::string::npos);
  EXPECT_NE(paths[1].find("b.so"), std::string::npos);
}

/**
 * 用例描述：验证配置文件损坏时生产加载器报告原因并允许上层回退旧流程。
 * 预置条件：在 OPP conf 目录写入不可解析的 JSON。
 * 测试步骤：调用生产 Load 接口。
 * 预期结果：返回失败、结果为空且原因非空，不产生部分交付件。
 */
TEST_F(OpsDeliverablesST, InvalidConfigReturnsReasonForLegacyFallback) {
  std::ofstream config(root_ + "/conf/ops_deliverables.json");
  config << "{invalid json";
  config.close();
  std::vector<OpsDeliverable> deliverables;
  std::string reason;
  EXPECT_FALSE(OpsDeliverablesManager::Load(deliverables, reason));
  EXPECT_TRUE(deliverables.empty());
  EXPECT_FALSE(reason.empty());
}

/**
 * 用例描述：验证有效配置会实际 dlopen 交付件 SO。
 * 预置条件：CMake 构建本目录的 ops_fixture.so，并将其路径写入配置。
 * 测试步骤：调用生产 LoadOppPackages 接口。
 * 预期结果：SO 构造函数写入标记文件，证明配置路径已被实际加载。
 */
TEST_F(OpsDeliverablesST, ValidConfigLoadsSharedLibrary) {
#ifndef GE_OPS_DELIVERABLES_FIXTURE
  GTEST_SKIP() << "fixture path is not configured by the ST CMake target";
#else
  const char *fixture = GE_OPS_DELIVERABLES_FIXTURE;
  const std::string copied_fixture = root_ + "/ops_deliverables_fixture.so";
  ASSERT_TRUE(std::filesystem::copy_file(fixture, copied_fixture));
  const std::string marker = root_ + "/loaded.marker";
  setenv("GE_OPS_DELIVERABLES_MARKER", marker.c_str(), 1);
  std::ofstream config(root_ + "/conf/ops_deliverables.json");
  config
      << R"({"version":"1.0","deliverables":[{"name":"opgraph","type":"shared_library","paths":["{ASCEND_OPP_PATH}/ops_deliverables_fixture.so"]}]})";
  config.close();
  EXPECT_EQ(OpsDeliverablesManager::LoadOppPackages({{"ASCEND_OPP_PATH", root_}}), GRAPH_SUCCESS);
  std::ifstream loaded(marker);
  EXPECT_TRUE(loaded.good());
  auto registry = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
  ASSERT_NE(registry, nullptr);
  auto funcs = registry->CreateOrGetOpImpl("OpsDeliverablesFixture");
  ASSERT_NE(funcs, nullptr);
  ASSERT_NE(funcs->infer_shape, nullptr);
  EXPECT_EQ(funcs->infer_shape(nullptr), GRAPH_SUCCESS);
  unsetenv("GE_OPS_DELIVERABLES_MARKER");
#endif
}
}  // namespace
}  // namespace ge
