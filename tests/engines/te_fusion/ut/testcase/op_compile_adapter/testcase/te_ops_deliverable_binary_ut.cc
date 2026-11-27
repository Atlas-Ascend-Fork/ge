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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#define private public
#define protected public
#include "graph/compute_graph.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "common/common_utils.h"
#include "binary/binary_manager.h"
#include "binary/fusion_binary_info.h"
#include "inc/te_fusion_util_constants.h"
#undef private
#undef protected

namespace {
using te::fusion::BinaryManager;
using te::fusion::OpBuildTaskPtr;
using te::fusion::OPS_PATH_NAME_PREFIX;

constexpr char kBinaryInfoConfigJson[] = R"({
  "Add": {
    "simplifiedKeyMode": 0,
    "dynamicParamMode": "folded_with_desc",
    "optionalInputMode": "gen_placeholder",
    "params": {
      "inputs": [{"name": "x", "index": 0, "paramType": "required", "formatMode": "static_nd_agnostic"}],
      "outputs": [{"name": "y", "index": 0, "paramType": "required", "formatMode": "static_nd_agnostic"}]
    },
    "binaryList": [
      {"coreType": 1, "simplifiedKey": ["Add/d=0,p=0/0,2/0,2"], "jsonPath": "ascend910/add/add.json"}
    ]
  }
})";

// 临时目录：析构时整体清理，避免用例中途失败残留
class TempDir {
 public:
  TempDir() {
    char tmpl[] = "/tmp/te_ops_deliverable_st_XXXXXX";
    char *created = mkdtemp(tmpl);
    if (created != nullptr) {
      path_ = created;
    }
  }

  ~TempDir() {
    if (!path_.empty()) {
      std::error_code error;
      (void)std::filesystem::remove_all(path_, error);
    }
  }

  const std::string &Path() const {
    return path_;
  }

  std::string CreateDir(const std::string &relative_path) const {
    const std::string path = path_ + "/" + relative_path;
    std::error_code error;
    (void)std::filesystem::create_directories(path, error);
    return path;
  }

  void WriteFile(const std::string &relative_path, const std::string &content) const {
    const std::string path = path_ + "/" + relative_path;
    std::ofstream output(path);
    output << content;
  }

 private:
  std::string path_;
};

class TeOpsDeliverableBinaryUt : public testing::Test {
 protected:
  void SetUp() override {
    graph_ = std::make_shared<ge::ComputeGraph>("te_ops_deliverable");
    next_node_index_ = 0U;
    ResetDeliverableState();
  }

  void TearDown() override {
    ResetDeliverableState();
    graph_.reset();
  }

  static void ResetDeliverableState() {
    BinaryManager &manager = BinaryManager::Instance();
    manager.deliverableOpKernelLoaded_ = false;
    manager.deliverableImplType_ = -1;
    manager.binaryOppKernelPathByPrefix_.clear();
    manager.binaryConfigPathByPrefix_.clear();
  }

  OpBuildTaskPtr MakeTask(const std::vector<std::string> &prefixes) {
    std::vector<ge::Node *> nodes;
    for (size_t index = 0U; index < prefixes.size(); ++index) {
      // 同一用例内会多次建图，节点名需要唯一，否则 AddNode 失败
      ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("node_" + std::to_string(next_node_index_++), "Add");
      if (!prefixes[index].empty()) {
        (void)ge::AttrUtils::SetStr(op_desc, OPS_PATH_NAME_PREFIX, prefixes[index]);
      }
      ge::NodePtr node = graph_->AddNode(op_desc);
      if (node == nullptr) {
        return nullptr;
      }
      nodes.emplace_back(node.get());
    }
    OpBuildTaskPtr task = std::make_shared<te::fusion::OpBuildTask>();
    task->opNodes = nodes;
    task->outNode = std::make_shared<ge::OpDesc>("out_node", "Add");
    return task;
  }

  static void EmplacePathByPrefix(const std::string &prefix, const std::string &kernel_path,
                                  const std::string &config_path) {
    BinaryManager &manager = BinaryManager::Instance();
    manager.binaryOppKernelPathByPrefix_[prefix] = kernel_path;
    manager.binaryConfigPathByPrefix_[prefix] = config_path;
    manager.deliverableOpKernelLoaded_ = true;
  }

  ge::ComputeGraphPtr graph_;
  size_t next_node_index_ = 0U;
};

// 分包级 kernel 根：单算子索引 jsonPath 自带 soc 段（3 段）/ 自带 soc 与分包段（4 段）
TEST_F(TeOpsDeliverableBinaryUt, assemble_json_path_in_prefix_root) {
  const std::string root = "/root/kernel/ascend950/ops_nn/";
  std::string json_file_path = root;
  std::string bin_file_path = "ascend950/add/add.json";
  te::fusion::AssembleJsonPath("ops_nn", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/ops_nn/add/add.json");

  json_file_path = root;
  bin_file_path = "ascend950/ops_nn/add/add.json";
  te::fusion::AssembleJsonPath("ops_nn", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/ops_nn/add/add.json");

  // 根目录不带尾斜杠时不能产出双斜杠
  json_file_path = "/root/kernel/ascend950/ops_nn";
  bin_file_path = "ascend950/add/add.json";
  te::fusion::AssembleJsonPath("ops_nn", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/ops_nn/add/add.json");
}

// 融合产物根（fusion_ops）：索引 jsonPath 为 1~3 段
TEST_F(TeOpsDeliverableBinaryUt, assemble_json_path_in_fusion_ops_root) {
  const std::string root = "/root/kernel/ascend950/fusion_ops/";
  std::string json_file_path = root;
  std::string bin_file_path = "ascend950/fusion_op_1.json";
  te::fusion::AssembleJsonPath("", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/fusion_ops/fusion_op_1.json");

  json_file_path = root;
  bin_file_path = "ascend950/fusion_ops/fusion_op_1.json";
  te::fusion::AssembleJsonPath("", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/fusion_ops/fusion_op_1.json");

  json_file_path = root;
  bin_file_path = "te_matmul_2ac1b9067af400eb1.json";
  te::fusion::AssembleJsonPath("", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/fusion_ops/te_matmul_2ac1b9067af400eb1.json");
}

// 索引 soc 与配置 soc 不同源：保守不剥段，行为与设计文档一致
TEST_F(TeOpsDeliverableBinaryUt, assemble_json_path_with_mismatched_soc_is_conservative) {
  std::string json_file_path = "/root/kernel/ascend950/ops_nn/";
  std::string bin_file_path = "ascend910b/add/add.json";
  te::fusion::AssembleJsonPath("ops_nn", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/kernel/ascend950/ops_nn/ascend910b/add/add.json");
}

// 未配置场景回归：kernel 根不含分包，保持原拼接行为
TEST_F(TeOpsDeliverableBinaryUt, assemble_json_path_keeps_legacy_behavior) {
  std::string json_file_path = "/root/built-in/op_impl/ai_core/tbe/kernel/";
  std::string bin_file_path = "ascend910/add/add.json";
  te::fusion::AssembleJsonPath("", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/built-in/op_impl/ai_core/tbe/kernel/ascend910/add/add.json");

  json_file_path = "/root/built-in/op_impl/ai_core/tbe/kernel/";
  bin_file_path = "ascend910/fusion_op_1.json";
  te::fusion::AssembleJsonPath("ops_legacy", json_file_path, bin_file_path);
  EXPECT_EQ(json_file_path, "/root/built-in/op_impl/ai_core/tbe/kernel/ascend910/ops_legacy/fusion_op_1.json");
}

// 单算子任务：按节点分包前缀查 kernel / config 目录
TEST_F(TeOpsDeliverableBinaryUt, get_binary_path_by_prefix_for_single_op) {
  EmplacePathByPrefix("ops_nn", "/root/kernel/ascend950/ops_nn/", "/root/kernel/config/ascend950/ops_nn/");
  OpBuildTaskPtr task = MakeTask({"ops_nn"});
  ASSERT_NE(task, nullptr);

  std::string binary_path;
  ASSERT_TRUE(BinaryManager::Instance().GetBinaryPathByPrefix(task, true, binary_path));
  EXPECT_EQ(binary_path, "/root/kernel/ascend950/ops_nn/");
  ASSERT_TRUE(BinaryManager::Instance().GetBinaryPathByPrefix(task, false, binary_path));
  EXPECT_EQ(binary_path, "/root/kernel/config/ascend950/ops_nn/");
}

// 单算子任务：前缀未配置（含主包空 prefix）时回退原逻辑，不返回错误路径
TEST_F(TeOpsDeliverableBinaryUt, get_binary_path_by_prefix_falls_back_when_prefix_missing) {
  EmplacePathByPrefix("ops_nn", "/root/kernel/ascend950/ops_nn/", "/root/kernel/config/ascend950/ops_nn/");
  OpBuildTaskPtr unknown_task = MakeTask({"ops_transformer"});
  ASSERT_NE(unknown_task, nullptr);
  std::string binary_path;
  EXPECT_FALSE(BinaryManager::Instance().GetBinaryPathByPrefix(unknown_task, true, binary_path));

  OpBuildTaskPtr main_package_task = MakeTask({""});
  ASSERT_NE(main_package_task, nullptr);
  EXPECT_FALSE(BinaryManager::Instance().GetBinaryPathByPrefix(main_package_task, true, binary_path));
}

// 融合任务（node>1）：config 索引直取 ops_legacy 分包，kernel 目录派生 fusion_ops
TEST_F(TeOpsDeliverableBinaryUt, get_binary_path_by_prefix_for_fusion_op) {
  EmplacePathByPrefix("ops_legacy", "/root/kernel/ascend950/ops_legacy/", "/root/kernel/config/ascend950/ops_legacy/");
  EmplacePathByPrefix("ops_nn", "/root/kernel/ascend950/ops_nn/", "/root/kernel/config/ascend950/ops_nn/");
  OpBuildTaskPtr task = MakeTask({"ops_nn", "ops_nn"});
  ASSERT_NE(task, nullptr);

  std::string binary_path;
  ASSERT_TRUE(BinaryManager::Instance().GetBinaryPathByPrefix(task, true, binary_path));
  EXPECT_EQ(binary_path, "/root/kernel/ascend950/fusion_ops/");
  ASSERT_TRUE(BinaryManager::Instance().GetBinaryPathByPrefix(task, false, binary_path));
  EXPECT_EQ(binary_path, "/root/kernel/config/ascend950/ops_legacy/");
}

// 融合任务：ops_legacy 缺配时回退原 prefix 查询（融合节点无 prefix 属性 → 返回 false）
TEST_F(TeOpsDeliverableBinaryUt, get_binary_path_by_prefix_for_fusion_op_without_legacy) {
  EmplacePathByPrefix("ops_nn", "/root/kernel/ascend950/ops_nn/", "/root/kernel/config/ascend950/ops_nn/");
  ASSERT_EQ(BinaryManager::Instance().binaryOppKernelPathByPrefix_.count("ops_legacy"), 0U);
  OpBuildTaskPtr task = MakeTask({"", ""});
  ASSERT_NE(task, nullptr);

  std::string binary_path;
  EXPECT_FALSE(BinaryManager::Instance().GetBinaryPathByPrefix(task, true, binary_path));
}

// 配置未生效时整条链路不介入
TEST_F(TeOpsDeliverableBinaryUt, get_binary_path_by_prefix_is_disabled_without_config) {
  OpBuildTaskPtr task = MakeTask({"ops_nn"});
  ASSERT_NE(task, nullptr);
  std::string binary_path;
  EXPECT_FALSE(BinaryManager::Instance().GetBinaryPathByPrefix(task, true, binary_path));
}

// 任务 prefix 取第一个非空节点属性（与 FE 侧写属性和 GetBinaryFileName 同源）
TEST_F(TeOpsDeliverableBinaryUt, get_task_ops_path_name_prefix) {
  OpBuildTaskPtr task = MakeTask({"", "ops_nn", "ops_legacy"});
  ASSERT_NE(task, nullptr);
  EXPECT_EQ(BinaryManager::GetTaskOpsPathNamePrefix(task), "ops_nn");

  OpBuildTaskPtr empty_task = MakeTask({"", ""});
  ASSERT_NE(empty_task, nullptr);
  EXPECT_EQ(BinaryManager::GetTaskOpsPathNamePrefix(empty_task), "");
}

// 分包级 config 目录：直接读取目录下的 binary_info_config.json，不再拼 soc / 扫描分包
TEST_F(TeOpsDeliverableBinaryUt, parse_binary_info_file_in_prefix_dir) {
  TempDir temp_dir;
  ASSERT_FALSE(temp_dir.Path().empty());
  const std::string prefix_dir = temp_dir.CreateDir("ascend950/ops_nn");
  temp_dir.WriteFile("ascend950/ops_nn/binary_info_config.json", kBinaryInfoConfigJson);

  te::fusion::BinaryInfoBase binary_info;
  ASSERT_TRUE(binary_info.ParseBinaryInfoFile(prefix_dir + "/", false));
  std::string json_path;
  ASSERT_TRUE(binary_info.MatchSimpleKey("Add", "Add/d=0,p=0/0,2/0,2", json_path));
  EXPECT_EQ(json_path, "ascend910/add/add.json");
}

// 分包级 config 目录缺 binary_info_config.json：直接失败，不落到 legacy 拼接（避免重复拼 soc）
TEST_F(TeOpsDeliverableBinaryUt, parse_binary_info_file_without_config_json_fails) {
  TempDir temp_dir;
  ASSERT_FALSE(temp_dir.Path().empty());
  const std::string prefix_dir = temp_dir.CreateDir("ascend950/ops_nn");

  te::fusion::BinaryInfoBase binary_info;
  EXPECT_FALSE(binary_info.ParseBinaryInfoFile(prefix_dir + "/", false));
}

// 空索引内容：配置分支读到了文件但生成索引失败，同样返回 false
TEST_F(TeOpsDeliverableBinaryUt, parse_binary_info_file_with_empty_config_fails) {
  TempDir temp_dir;
  ASSERT_FALSE(temp_dir.Path().empty());
  const std::string prefix_dir = temp_dir.CreateDir("ascend950/ops_legacy");
  temp_dir.WriteFile("ascend950/ops_legacy/binary_info_config.json", "{}");

  te::fusion::BinaryInfoBase binary_info;
  EXPECT_FALSE(binary_info.ParseBinaryInfoFile(prefix_dir + "/", false));
}

// superkernel 索引文件与普通索引并列在同一分包级目录下
TEST_F(TeOpsDeliverableBinaryUt, parse_relocatable_binary_info_file_in_prefix_dir) {
  TempDir temp_dir;
  ASSERT_FALSE(temp_dir.Path().empty());
  const std::string prefix_dir = temp_dir.CreateDir("ascend950/ops_nn");
  temp_dir.WriteFile("ascend950/ops_nn/relocatable_kernel_info_config.json", kBinaryInfoConfigJson);

  te::fusion::BinaryInfoBase binary_info;
  ASSERT_TRUE(binary_info.ParseBinaryInfoFile(prefix_dir + "/", true));
  std::string json_path;
  ASSERT_TRUE(binary_info.MatchSimpleKey("Add", "Add/d=0,p=0/0,2/0,2", json_path));
  EXPECT_EQ(json_path, "ascend910/add/add.json");
}
}  // namespace
