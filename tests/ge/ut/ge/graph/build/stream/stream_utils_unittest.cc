/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

#include "graph/build/stream/stream_utils.h"

#include <graph_utils_ex.h>
#include <debug/ge_attr_define.h>

#include "graph/ge_local_context.h"
#include "common/ge_common/ge_types.h"
#include "common/multi_stream_share_graph.h"
#include "register/custom_pass_helper.h"
#include "register/register_custom_pass.h"

namespace ge {
namespace {
constexpr const char_t *kMixCoreType = "MIX";
constexpr const char_t *kMixIsAivAttr = "_mix_is_aiv";

// core_type == nullptr leaves the node without the cube/vector annotation,
// mix_is_aiv == nullptr leaves the MIX sub-type flag unset.
NodePtr AddNodeWithCoreType(const ComputeGraphPtr &graph, const std::string &name, const char_t *core_type,
                            const bool *mix_is_aiv) {
  const auto op_desc = std::make_shared<OpDesc>(name, "Relu");
  if (core_type != nullptr) {
    (void)AttrUtils::SetStr(op_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  }
  if (mix_is_aiv != nullptr) {
    (void)AttrUtils::SetBool(op_desc, kMixIsAivAttr, *mix_is_aiv);
  }
  return graph->AddNode(op_desc);
}
}  // namespace

class UtestStreamUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestStreamUtils, ConvertUserStreamLabelToInnerStreamLabel) {
  auto graph = MultiStreamShareGraph::TwoNodeGraphWithUserStreamLabel();
  auto compute_graph = GraphUtilsEx::GetComputeGraph(graph);
  auto trans1 = compute_graph->FindNode("trans1");
  std::string trans1_user_stream_label;
  ASSERT_TRUE(AttrUtils::GetStr(trans1->GetOpDesc(), "_user_stream_label", trans1_user_stream_label));
  ASSERT_STREQ(trans1_user_stream_label.c_str(), "test_label");

  EXPECT_EQ(StreamUtils::TransUserStreamLabel(compute_graph), SUCCESS);

  std::string trans1_final_stream_label;
  EXPECT_TRUE(AttrUtils::GetStr(trans1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, trans1_final_stream_label));
  EXPECT_STREQ(trans1_final_stream_label.c_str(), "test_label");
}

TEST_F(UtestStreamUtils, Convert_Both_UserStreamLabel_InnerStreamLabel) {
  auto graph = MultiStreamShareGraph::TwoNodeGraphWithUserStreamLabel();
  auto compute_graph = GraphUtilsEx::GetComputeGraph(graph);
  auto trans1 = compute_graph->FindNode("trans1");
  AttrUtils::SetStr(trans1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "inner_label");

  std::string trans1_inner_stream_label;
  ASSERT_TRUE(AttrUtils::GetStr(trans1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, trans1_inner_stream_label));
  ASSERT_STREQ(trans1_inner_stream_label.c_str(), "inner_label");
  std::string trans1_user_stream_label;
  ASSERT_TRUE(AttrUtils::GetStr(trans1->GetOpDesc(), "_user_stream_label", trans1_user_stream_label));
  ASSERT_STREQ(trans1_user_stream_label.c_str(), "test_label");

  EXPECT_EQ(StreamUtils::TransUserStreamLabel(compute_graph), SUCCESS);

  std::string trans1_final_stream_label;
  EXPECT_TRUE(AttrUtils::GetStr(trans1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, trans1_final_stream_label));
  EXPECT_STREQ(trans1_final_stream_label.c_str(), trans1_user_stream_label.c_str());
}

TEST_F(UtestStreamUtils, EnableDynamicShapeMultiStream_WithOptionModesAndNoEnv_ReturnsFalse) {
  const char *const env_name = "ENABLE_DYNAMIC_SHAPE_MULTI_STREAM";
  const char *const env_value = std::getenv(env_name);
  const bool had_env = (env_value != nullptr);
  const std::string env_backup = had_env ? env_value : "";
  (void)unsetenv(env_name);

  const auto back_session_options = GetThreadLocalContext().GetAllSessionOptions();
  const auto back_graph_options = GetThreadLocalContext().GetAllGraphOptions();
  auto session_options = back_session_options;
  auto graph_options = back_graph_options;
  const std::string auto_multistream_modes[] = {"cv", "LoadBalance:8", "MainStream:8", "LoadBalance"};
  for (const auto &mode : auto_multistream_modes) {
    session_options["ge.autoMultistreamParallelMode"] = mode;
    graph_options["ge.autoMultistreamParallelMode"] = mode;
    GetThreadLocalContext().SetSessionOption(session_options);
    GetThreadLocalContext().SetGraphOption(graph_options);
    EXPECT_FALSE(StreamUtils::EnableDynamicShapeMultiStream()) << mode;
  }

  GetThreadLocalContext().SetSessionOption(back_session_options);
  GetThreadLocalContext().SetGraphOption(back_graph_options);
  if (had_env) {
    (void)setenv(env_name, env_backup.c_str(), 1);
  } else {
    (void)unsetenv(env_name);
  }
}

TEST_F(UtestStreamUtils, GetAutoMultistreamParallelMode_ReadsOptionMode) {
  const auto option_bak = GetThreadLocalContext().GetAllGraphOptions();
  GetThreadLocalContext().SetGraphOption({{"ge.autoMultistreamParallelMode", "LoadBalance:8"}});
  std::string mode;
  EXPECT_EQ(StreamUtils::GetAutoMultistreamParallelMode(mode), GRAPH_SUCCESS);
  EXPECT_EQ(mode, "LoadBalance:8");
  GetThreadLocalContext().SetGraphOption(option_bak);
}

TEST_F(UtestStreamUtils, GetAutoMultistreamParallelMode_GraphAttributeHasPriorityOverOption) {
  const auto option_bak = GetThreadLocalContext().GetAllGraphOptions();
  GetThreadLocalContext().SetGraphOption({{"ge.autoMultistreamParallelMode", "MainStream:4"}});
  const auto graph = std::make_shared<ComputeGraph>("graph_attr_priority");
  ASSERT_TRUE(AttrUtils::SetStr(graph, "ge.autoMultistreamParallelMode", "LoadBalance:8"));

  std::string mode;
  bool from_graph = false;
  EXPECT_EQ(StreamUtils::GetAutoMultistreamParallelMode(graph, mode, from_graph), GRAPH_SUCCESS);
  EXPECT_EQ(mode, "LoadBalance:8");
  EXPECT_TRUE(from_graph);

  GetThreadLocalContext().SetGraphOption(option_bak);
}

TEST_F(UtestStreamUtils, ParseAutoMultistreamParallelMode_ValidModes) {
  const std::vector<std::tuple<std::string, AutoMultistreamMode, int32_t>> cases = {
      {"", AutoMultistreamMode::kUnset, 0},
      {"cv", AutoMultistreamMode::kCv, 0},
      {"LoadBalance:1", AutoMultistreamMode::kLoadBalance, 1},
      {"MainStream:64", AutoMultistreamMode::kMainStream, 64},
      {"WeightedLoadBalance:8", AutoMultistreamMode::kWeightedLoadBalance, 8},
  };
  for (const auto &test_case : cases) {
    AutoMultistreamConfig config;
    EXPECT_EQ(StreamUtils::ParseAutoMultistreamParallelMode(std::get<0>(test_case), config), GRAPH_SUCCESS)
        << std::get<0>(test_case);
    EXPECT_EQ(config.mode, std::get<1>(test_case)) << std::get<0>(test_case);
    EXPECT_EQ(config.max_stream_num, std::get<2>(test_case)) << std::get<0>(test_case);
  }
}

TEST_F(UtestStreamUtils, ParseAutoMultistreamParallelMode_DefaultOnlyAllowedForGraphAttribute) {
  AutoMultistreamConfig config;
  EXPECT_NE(StreamUtils::ParseAutoMultistreamParallelMode("default", config), GRAPH_SUCCESS);
  // The third argument marks the value as coming from the graph attribute set by a custom pass.
  EXPECT_EQ(StreamUtils::ParseAutoMultistreamParallelMode("default", config, true), GRAPH_SUCCESS);
  EXPECT_EQ(config.mode, AutoMultistreamMode::kDefault);
  EXPECT_EQ(config.max_stream_num, 0);
}

TEST_F(UtestStreamUtils, ParseAutoMultistreamParallelMode_InvalidModes) {
  const std::vector<std::string> cases = {
      "LoadBalance",         "MainStream",
      "WeightedLoadBalance", "default",
      "default:2",           "cv:2",
      "Unknown:2",           "LoadBalance:0",
      "LoadBalance:65",      "LoadBalance:-1",
      "LoadBalance:+1",      "LoadBalance:1.0",
      "LoadBalance:",        ":2",
      "LoadBalance:2:3",     " LoadBalance:2",
      "LoadBalance:2 ",      "LoadBalance:99999999999999999999",
  };
  for (const auto &mode : cases) {
    AutoMultistreamConfig config;
    EXPECT_NE(StreamUtils::ParseAutoMultistreamParallelMode(mode, config), GRAPH_SUCCESS) << mode;
  }
}
TEST_F(UtestStreamUtils, DynamicShapeStaticSubgraphReceivesRootCoreOptionsBeforeCallback) {
  const auto root = std::make_shared<ComputeGraph>("dynamic_root");
  const auto middle = std::make_shared<ComputeGraph>("dynamic_body");
  const auto subgraph = std::make_shared<ComputeGraph>("known_shape_subgraph");
  root->SetGraphUnknownFlag(true);
  middle->SetGraphUnknownFlag(true);
  subgraph->SetGraphUnknownFlag(false);
  middle->SetParentGraph(root);
  subgraph->SetParentGraph(middle);
  ASSERT_TRUE(AttrUtils::SetStr(root, "ge.aicoreNum", "8"));
  ASSERT_TRUE(AttrUtils::SetStr(root, "ge.vectorcoreNum", "16"));
  // Intermediate graphs are not the source of model options.
  ASSERT_TRUE(AttrUtils::SetStr(middle, "ge.aicoreNum", "2"));

  size_t calls = 0;
  PassRegistrationData registration("CheckStaticSubgraphCoreOptions");
  registration.Stage(CustomPassStage::kAfterAssignLogicStream)
      .CustomAllocateStreamPassFn([&](const ConstGraphPtr &graph, StreamPassContext &) -> Status {
        const auto current = GraphUtilsEx::GetComputeGraph(*graph);
        EXPECT_EQ(current, subgraph);
        std::string value;
        if (calls == 0U) {
          EXPECT_TRUE(AttrUtils::GetStr(current, "ge.aicoreNum", value));
          EXPECT_EQ(value, "8");
          EXPECT_TRUE(AttrUtils::GetStr(current, "ge.vectorcoreNum", value));
          EXPECT_EQ(value, "16");
        } else {
          EXPECT_FALSE(current->HasAttr("ge.aicoreNum"));
          EXPECT_TRUE(AttrUtils::GetStr(current, "ge.vectorcoreNum", value));
          EXPECT_EQ(value, "4");
        }
        ++calls;
        return SUCCESS;
      });
  CustomPassHelper::Instance().Insert(registration);
  int64_t next_stream_id = 1;
  EXPECT_EQ(StreamUtils::RunCustomStreamPass(subgraph, next_stream_id), SUCCESS);
  // Recompiling after withdrawing one option must not reuse the child's copy.
  (void)root->DelAttr("ge.aicoreNum");
  EXPECT_TRUE(AttrUtils::SetStr(root, "ge.vectorcoreNum", "4"));
  EXPECT_EQ(StreamUtils::RunCustomStreamPass(subgraph, next_stream_id), SUCCESS);
  EXPECT_EQ(calls, 2U);
  EXPECT_EQ(next_stream_id, 1);
  CustomPassHelper::Instance().Unload();
}

TEST_F(UtestStreamUtils, RootGraphCoreOptionsRemainAvailable) {
  const auto root = std::make_shared<ComputeGraph>("root_core_options");
  ASSERT_TRUE(AttrUtils::SetStr(root, "ge.aicoreNum", "8"));
  int64_t next_stream_id = 1;
  EXPECT_EQ(StreamUtils::RunCustomStreamPass(root, next_stream_id), SUCCESS);
  std::string value;
  EXPECT_TRUE(AttrUtils::GetStr(root, "ge.aicoreNum", value));
  EXPECT_EQ(value, "8");
  EXPECT_FALSE(root->HasAttr("ge.vectorcoreNum"));
}

TEST_F(UtestStreamUtils, DagModeWithoutMiniDagStreamPassFails) {
  // The registry must reject DAG dispatch if the required pass is absent.
  // Test it directly: StreamUtils now loads the pass on demand before dispatch.
  (void)CustomPassHelper::Instance().Unload();
  const auto graph = std::make_shared<ComputeGraph>("dag_without_minidag");
  ASSERT_TRUE(AttrUtils::SetStr(graph, "ge.autoMultistreamParallelMode", "LoadBalance:2"));
  auto public_graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(graph);
  StreamPassContext context(0);
  EXPECT_NE(CustomPassHelper::Instance().Run(public_graph, context, CustomPassStage::kAfterAssignLogicStream, {}),
            SUCCESS);
}

TEST_F(UtestStreamUtils, NonDagModeDoesNotRequireMiniDagStreamPass) {
  (void)CustomPassHelper::Instance().Unload();
  const auto unset = std::make_shared<ComputeGraph>("mode_unset");
  int64_t next_stream_id = 1;
  EXPECT_EQ(StreamUtils::RunCustomStreamPass(unset, next_stream_id), SUCCESS);
  for (const std::string mode : {"cv", "default"}) {
    const auto graph = std::make_shared<ComputeGraph>("mode_" + mode);
    ASSERT_TRUE(AttrUtils::SetStr(graph, "ge.autoMultistreamParallelMode", mode));
    EXPECT_EQ(StreamUtils::RunCustomStreamPass(graph, next_stream_id), SUCCESS) << mode;
  }
  EXPECT_EQ(next_stream_id, 1);
}

TEST_F(UtestStreamUtils, DagModeRunsRegisteredMiniDagStreamPass) {
  (void)CustomPassHelper::Instance().Unload();
  size_t calls = 0;
  PassRegistrationData registration("MiniDAGStreamPass");
  registration.Stage(CustomPassStage::kAfterAssignLogicStream)
      .CustomAllocateStreamPassFn([&calls](const ConstGraphPtr &, StreamPassContext &) -> Status {
        ++calls;
        return SUCCESS;
      });
  CustomPassHelper::Instance().Insert(registration);
  const auto graph = std::make_shared<ComputeGraph>("dag_with_minidag");
  ASSERT_TRUE(AttrUtils::SetStr(graph, "ge.autoMultistreamParallelMode", "LoadBalance:2"));
  auto public_graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(graph);
  StreamPassContext context(0);
  EXPECT_EQ(CustomPassHelper::Instance().Run(public_graph, context, CustomPassStage::kAfterAssignLogicStream, {}),
            SUCCESS);
  EXPECT_EQ(calls, 1U);
  (void)CustomPassHelper::Instance().Unload();
}

TEST_F(UtestStreamUtils, IsAivNodeClassifiesNodeByCubeVectorCoreType) {
  const auto graph = std::make_shared<ComputeGraph>("core_type_classification");
  const bool mix_is_aiv = true;
  const bool mix_is_not_aiv = false;

  // Pure vector core types are AIV nodes.
  EXPECT_TRUE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "aiv", kTaskTypeAiv.c_str(), nullptr)));
  EXPECT_TRUE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "mix_aiv", kTaskTypeMixAiv.c_str(), nullptr)));

  // A MIX node follows its _mix_is_aiv sub-type flag, and stays non-AIV when the
  // flag is false or was never written.
  EXPECT_TRUE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "mix_aiv_true", kMixCoreType, &mix_is_aiv)));
  EXPECT_FALSE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "mix_aiv_false", kMixCoreType, &mix_is_not_aiv)));
  EXPECT_FALSE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "mix_aiv_unset", kMixCoreType, nullptr)));

  // Any other core type is not AIV, and neither is a node that carries no
  // cube/vector annotation at all.
  EXPECT_FALSE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "aic", kTaskTypeAicore.c_str(), nullptr)));
  EXPECT_FALSE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "plain", nullptr, nullptr)));
  // The flag alone does not promote a node that never declared MIX.
  EXPECT_FALSE(StreamUtils::IsAivNode(AddNodeWithCoreType(graph, "flag_only", nullptr, &mix_is_aiv)));
}

TEST_F(UtestStreamUtils, IsAicNodeCoversTheAnnotatedNonAivNodes) {
  const auto graph = std::make_shared<ComputeGraph>("core_type_complement");
  const bool mix_is_aiv = true;
  const bool mix_is_not_aiv = false;

  // An annotated node that is not AIV runs on the cube core.
  EXPECT_TRUE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "aic", kTaskTypeAicore.c_str(), nullptr)));
  EXPECT_TRUE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "mix_aiv_false", kMixCoreType, &mix_is_not_aiv)));
  EXPECT_TRUE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "mix_aiv_unset", kMixCoreType, nullptr)));

  // AIV nodes are excluded, so the two predicates never both hold.
  EXPECT_FALSE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "aiv", kTaskTypeAiv.c_str(), nullptr)));
  EXPECT_FALSE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "mix_aiv", kTaskTypeMixAiv.c_str(), nullptr)));
  EXPECT_FALSE(StreamUtils::IsAicNode(AddNodeWithCoreType(graph, "mix_aiv_true", kMixCoreType, &mix_is_aiv)));

  // Without the annotation the node is neither AIC nor AIV: the cv-parallel
  // marking must not treat unknown nodes as cube nodes.
  const auto plain = AddNodeWithCoreType(graph, "plain", nullptr, nullptr);
  EXPECT_FALSE(StreamUtils::IsAicNode(plain));
  EXPECT_FALSE(StreamUtils::IsAivNode(plain));
}
}  // namespace ge
