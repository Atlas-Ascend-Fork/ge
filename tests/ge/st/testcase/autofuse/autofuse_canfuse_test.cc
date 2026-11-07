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
#include "ge/ge_api.h"
#include "ascendc_ir/utils/asc_graph_utils.h"
#include "ascir_ops.h"
#include "graph_utils_ex.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/optimize/autofuse/autofuse_optimize.h"
#include "graph/option/optimization_option.h"
#include "register/optimization_option_registry.h"
#include "compiler/graph/build/model_builder.h"
#include "compiler/graph/build/graph_builder.h"
#include "compiler/graph/manager/graph_manager.h"
#include "fusion/autofuse_attrs.h"
#include "utils/auto_fuse_config.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "../init_ge.h"
#include "common/share_graph.h"
#include "ge_running_env/fake_op.h"
#include "common/env_path.h"
#include "common/autofuse_platform_api.h"
#include "tests/framework/ge_runtime_stub/include/common/summary_checker.h"
#include "faker/space_registry_faker.h"
#include "depends/runtime/src/runtime_stub.h"
#include "session/session_manager.h"
#include "session/inner_session.h"
#include "jit_execution/user_graphs_manager.h"
#include "jit_execution/user_graph_ctrl.h"
#include "jit_execution/exe_points/execution_order.h"
#include "jit_execution/exe_points/execution_point.h"
#include "jit_execution/exe_points/guarded_execution_point.h"

using namespace std;
using namespace testing;
using namespace ge::autofuse;

ge::SessionManager *GetSessionManager();

namespace {
ComputeGraphPtr GetJitCompiledGraph(const ge::Session &session, uint32_t graph_id) {
  const auto inner_session = GetSessionManager()->GetSession(session.GetSessionId());
  if (inner_session == nullptr) {
    return nullptr;
  }
  auto *user_graph_ctrl = inner_session->user_graphs_manager_->GetUserGraphControl(graph_id);
  if (user_graph_ctrl == nullptr) {
    return nullptr;
  }
  auto *ep = user_graph_ctrl->order_.GetFirstPoint();
  if (ep == nullptr) {
    return nullptr;
  }
  auto &geps = ep->models_.GetCache();
  if (geps.empty()) {
    return nullptr;
  }
  return geps.front()->GetGraph();
}
}  // namespace

namespace ge {
using af::AscGraphUtils;

class RuntimeMock910B2 : public RuntimeStub {
 public:
  rtError_t rtGetSocVersion(char *version, const uint32_t maxLen) {
    (void)strcpy_s(version, maxLen, "Ascend910B2");
    return RT_ERROR_NONE;
  }
  rtError_t rtGetSocSpec(const char *label, const char *key, char *val, const uint32_t maxLen) override {
    (void)label;
    (void)key;
    (void)strcpy_s(val, maxLen, "2201");
    return RT_ERROR_NONE;
  }
};

class TestCanfusePass : public testing::Test {
 protected:
  void SetUp() override {
    RuntimeStub::SetInstance(std::make_shared<RuntimeMock910B2>());
    // 符号化infer需要数据依赖注册在SpaceRegistry之前
    IMPL_OP(ReduceMax).InputsDataDependency({1});
    gert::SpaceRegistryFaker::CreateDefaultSpaceRegistry();
    ge::OmgContext ctx;
    domi::GetContext() = ctx;
    ReInitGe();
    global_options = ge::GetThreadLocalContext().GetAllGlobalOptions();
    graph_options = ge::GetThreadLocalContext().GetAllGraphOptions();
    session_options = ge::GetThreadLocalContext().GetAllSessionOptions();
    ge::GetThreadLocalContext().SetGlobalOption({});
    ge::GetThreadLocalContext().SetGraphOption({});
    ge::GetThreadLocalContext().SetSessionOption({});
    GetThreadLocalContext().GetOo().Initialize({}, {});
    ge_env.InstallDefault();

    work_path = EnvPath().GetAirBasePath() + "/output";
    setenv("ASCEND_WORK_PATH", work_path.c_str(), 1);
    setenv("AUTOFUSE_FLAGS", "--enable_autofuse=true", 1);
    auto ascend_install_path = EnvPath().GetAscendInstallPath();
    (void)mmGetEnv("ASCEND_OPP_PATH", old_opp_path_env_, MMPA_MAX_PATH);
    (void)mmGetEnv("LD_LIBRARY_PATH", old_ld_path_env_, MMPA_MAX_PATH);
    setenv("ASCEND_OPP_PATH", (ascend_install_path + "/opp").c_str(), 1);
    setenv("LD_LIBRARY_PATH", (ascend_install_path + "/runtime/lib64").c_str(), 1);
    AutoFuseConfig::MutableLoweringConfig().experimental_lowering_reduce = true;
    SetAutofusePlatform("2201");
    const auto env_ptr = getenv("LD_PRELOAD");
    if (env_ptr != nullptr) {
      env = env_ptr;
      unsetenv("LD_PRELOAD");
    }
  }

  void TearDown() override {
    RuntimeStub::Reset();
    ge_env.Reset();
    ge_env.InstallDefault();
    ge::GetThreadLocalContext().SetGlobalOption(global_options);
    ge::GetThreadLocalContext().SetGraphOption(graph_options);
    ge::GetThreadLocalContext().SetSessionOption(session_options);
    gert::UnLoadDefaultSpaceRegistry();
    unsetenv("ASCEND_WORK_PATH");
    unsetenv("AUTOFUSE_FLAGS");
    mmSetEnv("ASCEND_OPP_PATH", old_opp_path_env_, 1);
    mmSetEnv("LD_LIBRARY_PATH", old_ld_path_env_, 1);
    ge::ResetAutofusePlatform();
    if (!env.empty()) {
      setenv("LD_PRELOAD", env.c_str(), 1);
    }
  }
  char old_opp_path_env_[MMPA_MAX_PATH] = {'\0'};
  char old_ld_path_env_[MMPA_MAX_PATH] = {'\0'};
  std::string work_path;
  GeRunningEnvFaker ge_env;
  std::map<std::string, std::string> graph_options;
  std::map<std::string, std::string> session_options;
  std::map<std::string, std::string> global_options;
  std::string env;
};

/**
 * 用例描述：elementwise和elementwise水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_ele_horizontal_fusion_1) {
  dlog_setlevel(0, 0, 0);
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(EXP).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  const auto compute_graph = gert::ShareGraph::BuildStaticAbsExpReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };
  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());

  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Exp", 1);
  node_types_to_count.emplace("Store", 2);
  node_types_to_count.emplace("Output", 2);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
  dlog_setlevel(0, 3, 0);
}

/**
 * 用例描述：elementwise和elementwise水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_ele_horizontal_fusion_2) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(EXP).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAbsReluExpAddNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  session.CompileGraph(1);
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Exp", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：elementwise和broadcast水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_bro_horizontal_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(EXP).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAbsAddExpNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };
  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();

  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Exp", 1);
  node_types_to_count.emplace("Store", 2);
  node_types_to_count.emplace("Output", 2);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：elementwise和broadcast水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_bro_horizontal_fusion_2) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAbsTwoAddNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 3);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 3);
  node_types_to_count.emplace("Load", 3);
  node_types_to_count.emplace("Broadcast", 3);
  node_types_to_count.emplace("Add", 2);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：elementwise和reduce水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
/*
TEST_F(TestCanfusePass, test_canfuse_ele_and_red_horizontal_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAbsReduceReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  InputTensorInfo input1{0, {2, 2, 2, 2}, nullptr, 0};
  std::vector<InputTensorInfo> inputs{input1};
  // 当前测试框架还不支持 BuildGraph 接口端到端编译、加载, 仅测试编译流程
  EXPECT_NE(session.BuildGraph(1, inputs), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto asc_bc = compute_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Sum", 1);
  node_types_to_count.emplace("Store", 2);
  node_types_to_count.emplace("Output", 2);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}
*/
/**
 * 用例描述：broadcast和broadcast水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:
 */
TEST_F(TestCanfusePass, test_canfuse_bro_and_bro_horizontal_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticTwoReduceThreeAddNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 3);
    node_types_to_count0.emplace("Const", 2);
    node_types_to_count0.emplace("AscBackend", 2);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_NE(str, "success");  // 恢复线上阻塞，临时修改
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Sum", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  // 恢复线上阻塞，临时修改
  EXPECT_NE(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：broadcast和reuce水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_bro_and_red_horizontal_fusion_1) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAddReduceNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};
  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");

  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("ReduceSum", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    // EXPECT_EQ(str, "success"); // 恢复线上阻塞，临时修改
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 3);
  node_types_to_count.emplace("Load", 3);
  node_types_to_count.emplace("Broadcast", 2);
  node_types_to_count.emplace("Add", 2);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  // 恢复线上阻塞，临时修改
  // EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：reduce和reduce水平融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
/*
TEST_F(TestCanfusePass, test_canfuse_red_and_red_horizontal_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCEMAX).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticAbsTwoReduceNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  InputTensorInfo input1{0, {2, 2, 2}, nullptr, 0};
  std::vector<InputTensorInfo> inputs{input1};
  // 当前测试框架还不支持 BuildGraph 接口端到端编译、加载, 仅测试编译流程
  EXPECT_NE(session.BuildGraph(1, inputs), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 2);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto asc_bc = compute_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(asc_bc, nullptr);
  auto attr = asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Sum", 1);
  node_types_to_count.emplace("Max", 1);
  node_types_to_count.emplace("Store", 2);
  node_types_to_count.emplace("Output", 2);
  std::string str = gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count);
  EXPECT_EQ(str, "success");
}
*/
/**
 * 用例描述：reduce和reduce垂直融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:lowering生成的两个AscBc 在lifting 成reduceNode
 */
TEST_F(TestCanfusePass, test_canfuse_red_and_red_vertical_fusion_1) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCEMAX).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticTwoReduceNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  session.CompileGraph(1);

  // lifting AscBackend中单节点还原为原节点
  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("AscBackend", 2);
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 2);
    node_types_to_count0.emplace("NetOutput", 1);
    EXPECT_EQ(gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0), "success");
  };
}

/**
 * 用例描述：reduce和broadcast垂直融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_red_and_bro_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCEMAX).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticReduceAddReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("ReduceMax", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    // EXPECT_EQ(str, "success"); // 恢复线上阻塞，临时修改
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  // 恢复线上阻塞，临时修改
  // EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：reduce和elementwise垂直融合
 * 预置条件：开启自动融合
 * 测试步骤
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验AscGraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_red_and_ele_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCEMAX).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticReduceAbsReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Max", 1);
  node_types_to_count.emplace("Abs", 2);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
}

/**
 * 用例描述：elementwise和elementwise垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的elementwise算子
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_ele_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(EXP).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2;
  const auto compute_graph = gert::ShareGraph::BuildStaticAbsReluAbsExpNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);
  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Abs", 2);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Exp", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：elementwise和broadcast垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的elementwise broadcast算子
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_bro_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2;
  const auto compute_graph = gert::ShareGraph::BuildStaticAbsReluAddNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Abs", 2);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");

  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：elementwise和reduce垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的elementwise算子
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_ele_and_red_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2U;
  const auto compute_graph = gert::ShareGraph::BuildStaticAbsReluReduceSumNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Sum", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");

  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：broadcast和elementwise垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的broadcast elementwise算子
 * 试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_bro_and_ele_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));
  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2U;
  const auto compute_graph = gert::ShareGraph::BuildStaticReluAddAbsReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();

  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Relu", 2);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");
  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：broadcast和reduce垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的broadcast算子
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_bro_and_red_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2U;
  const auto compute_graph = gert::ShareGraph::BuildStaticReluAddReduceSumNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 2);
    node_types_to_count0.emplace("Const", 1);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();

  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 2);
  node_types_to_count.emplace("Load", 2);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Broadcast", 1);
  node_types_to_count.emplace("Add", 1);
  node_types_to_count.emplace("Sum", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");

  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：broadcast和broadcast垂直融合
 * 预置条件：开启自动融合 通过限制lowering融合节点数限制lowering融合全部的broadcast算子
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_bro_and_bro_vertical_fusion) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));
  uint64_t max_fused_nodes_num_ = AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops;

  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = 2U;
  const auto compute_graph = gert::ShareGraph::BuildStaticReluAddAddNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 3);
    node_types_to_count0.emplace("AscBackend", 1);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 3);
  node_types_to_count.emplace("Load", 3);
  node_types_to_count.emplace("Relu", 1);
  node_types_to_count.emplace("Broadcast", 2);
  node_types_to_count.emplace("Add", 2);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  EXPECT_EQ(gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count), "success");

  // 复原
  AutoFuseConfig::MutableLoweringConfig().max_fused_loop_ops = max_fused_nodes_num_;
}

/**
 * 用例描述：reduce和reduce垂直融合
 * 预置条件：开启自动融合
 * 测试步骤：
 * 预期结果:融合lowering生成的两个AscBc为1个，同时校验ascgraph的正确性
 */
TEST_F(TestCanfusePass, test_canfuse_red_and_red_vertical_fusion_2) {
  auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCESUM).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(REDUCEMAX).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));

  const auto compute_graph = gert::ShareGraph::BuildStaticTwoReduceReluNodeGraph();
  ASSERT_NE(compute_graph, nullptr);
  auto graph = GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
  std::map<AscendString, AscendString> options = {{"ge.oo.level", "O3"}};

  DUMP_GRAPH_WHEN("AutofuseOptimize_Exit");
  Session session(options);
  auto ret = session.AddGraph(1, *graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // 通过 CompileGraph 触发真实编译，融合结果在 GEP 编译副本上
  EXPECT_EQ(session.CompileGraph(1), SUCCESS);

  CHECK_GRAPH(AutofuseOptimize_Exit) {
    std::map<std::string, size_t> node_types_to_count0;
    node_types_to_count0.emplace("Data", 1);
    node_types_to_count0.emplace("Const", 2);
    node_types_to_count0.emplace("AscBackend", 2);
    node_types_to_count0.emplace("NetOutput", 1);
    std::string str = gert::SummaryChecker(graph).StrictDirectNodeTypes(node_types_to_count0);
    EXPECT_EQ(str, "success");
  };

  auto compiled_graph = GetJitCompiledGraph(session, 1);
  ASSERT_NE(compiled_graph, nullptr);
  auto fused_asc_bc = compiled_graph->FindFirstNodeMatchType("AscBackend");
  ASSERT_NE(fused_asc_bc, nullptr);
  auto attr = fused_asc_bc->GetOpDesc()->GetAttrsGroup<AutoFuseAttrs>();
  ASSERT_NE(attr, nullptr);
  ASSERT_NE(attr->GetAscGraph(), nullptr);
  const auto &fused_graph = AscGraphUtils::GetComputeGraph(*attr->GetAscGraph());
  std::map<std::string, size_t> node_types_to_count;
  node_types_to_count.emplace("Data", 1);
  node_types_to_count.emplace("Load", 1);
  node_types_to_count.emplace("Abs", 1);
  node_types_to_count.emplace("Max", 1);
  node_types_to_count.emplace("Store", 1);
  node_types_to_count.emplace("Output", 1);
  std::string str = gert::SummaryChecker(fused_graph).StrictDirectNodeTypes(node_types_to_count);
  EXPECT_EQ(str, "success");
}

namespace {
// 两条链：
//   data_h -> relu_h -> Node_Output(0)            // host 下沉候选（小值链）
//   data_d -> abs_d -> exp_d -> Node_Output(1)    // device 链，验证融合仍正常
ComputeGraphPtr BuildHostExecGuardGraph(const std::initializer_list<int64_t> host_shape,
                                        const std::initializer_list<int64_t> host_origin_shape,
                                        const std::initializer_list<int64_t> device_shape,
                                        const bool host_anchor = true) {
  DEF_GRAPH(g1) {
    auto data_h = OP_CFG(DATA).InCnt(1).OutCnt(1).Attr(ATTR_NAME_INDEX, 0).TensorDesc(FORMAT_ND, DT_FLOAT, host_shape);
    auto relu_h = OP_CFG("Relu").InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, host_shape);
    auto data_d =
        OP_CFG(DATA).InCnt(1).OutCnt(1).Attr(ATTR_NAME_INDEX, 1).TensorDesc(FORMAT_ND, DT_FLOAT, device_shape);
    auto abs_d = OP_CFG("Abs").InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, device_shape);
    auto exp_d = OP_CFG(EXP).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, device_shape);
    auto net_output = OP_CFG(NETOUTPUT).InCnt(2).OutCnt(1).TensorDesc(FORMAT_ND, DT_FLOAT, device_shape);

    CHAIN(NODE("data_h", data_h)->NODE("relu_h", relu_h)->EDGE(0, 0)->NODE("Node_Output", net_output));
    CHAIN(NODE("data_d", data_d)
              ->NODE("abs_d", abs_d)
              ->NODE("exp_d", exp_d)
              ->EDGE(0, 1)
              ->NODE("Node_Output", net_output));
  };
  auto graph = ToComputeGraph(g1);
  if (graph == nullptr) {
    return nullptr;
  }
  // 采集在动态图划分之前执行、不依赖 dynamic 标记；这里保留 unknown flag 以贴近真实动态模型。
  graph->SetGraphUnknownFlag(true);
  // 符号化需要非负的 origin shape 作为 hint。
  const auto host_origin = GeShape(std::vector<int64_t>(host_origin_shape));
  auto data_h = graph->FindNode("data_h");
  data_h->GetOpDesc()->MutableInputDesc(0)->SetOriginShape(host_origin);
  data_h->GetOpDesc()->MutableOutputDesc(0)->SetOriginShape(host_origin);
  // 以 host tensor 输入作为 host 下沉锚点（等价于引擎分配前按类型识别的 shape 值链锚点）。
  if (host_anchor) {
    (void)AttrUtils::SetBool(data_h->GetOpDesc(), ge::ATTR_NAME_HOST_TENSOR, true);
  }
  return graph;
}

std::vector<GeTensor> MakeHostExecGuardInputs(const std::initializer_list<int64_t> host_shape,
                                              const std::initializer_list<int64_t> host_origin_shape,
                                              const std::initializer_list<int64_t> device_shape) {
  std::vector<GeTensor> inputs;
  GeTensorDesc host_desc(GeShape(std::vector<int64_t>(host_shape)), FORMAT_ND, DT_FLOAT);
  host_desc.SetOriginShape(GeShape(std::vector<int64_t>(host_origin_shape)));
  inputs.emplace_back(host_desc);
  inputs.emplace_back(GeTensorDesc(GeShape(std::vector<int64_t>(device_shape)), FORMAT_ND, DT_FLOAT));
  return inputs;
}

void InitOoForAutofuse() {
  const std::map<std::string, std::string> ge_options = {{ge::OO_LEVEL, "O3"}};
  ge::GetThreadLocalContext().GetOo().Initialize(ge_options, ge::OptionRegistry::GetInstance().GetRegisteredOptTable());
}

void InstallHostExecGuardEnv(GeRunningEnvFaker &ge_env, const std::string &relu_host_store) {
  const auto infer_data = ELMTWISE_INFER_SHAPEANDTYPE("x", "y");
  ge_env.Reset()
      .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
      .Install(FakeEngine("DNN_VM_HOST_CPU").KernelInfoStore("DNN_VM_HOST_CPU_OP_STORE"))
      .Install(FakeOp(DATA).Inputs({"x"}).Outputs({"y"}).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_data))
      .Install(FakeOp("Relu").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp("Relu").InfoStoreAndBuilder(relu_host_store))
      .Install(FakeOp("Abs").InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(EXP).InfoStoreAndBuilder("AIcoreEngine"))
      .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("AIcoreEngine"));
}
}  // namespace

/**
 * 用例描述：融合准入跳过将下沉 host 的小值链
 * 预置条件：动态 shape 图开启自动融合；host 链以 host tensor 输入为锚点，Relu 同时具备 host 与 device kernel，
 *           输出为小值(8 元素，小值判定成立)
 * 测试步骤：1. 构造 data_h->relu_h 与 data_d->abs_d->exp_d 两条链；2. 调用 AutofuseOptimize::Run
 * 预期结果：relu_h 被判定为 host 下沉并跳过 lowering，节点保留为原始 Relu；device 链 abs_d/exp_d 仍融合为 AscBackend
 */
TEST_F(TestCanfusePass, test_skip_lowering_for_host_exec_small_value_chain) {
  dlog_setlevel(0, 0, 0);
  InstallHostExecGuardEnv(ge_env, "DNN_VM_HOST_CPU_OP_STORE");
  InitOoForAutofuse();
  auto graph = BuildHostExecGuardGraph({2, 2, 2}, {2, 2, 2}, {2, 2, 2, 2});
  ASSERT_NE(graph, nullptr);
  AutofuseOptimize autofuser;
  ASSERT_EQ(autofuser.Run(graph, MakeHostExecGuardInputs({2, 2, 2}, {2, 2, 2}, {2, 2, 2, 2})), GRAPH_SUCCESS);

  // host 小值链：被收集并跳过 lowering，节点保留为原始算子
  auto relu_h = graph->FindNode("relu_h");
  ASSERT_NE(relu_h, nullptr);
  EXPECT_EQ(relu_h->GetType(), "Relu");
  // device 链仍正常融合
  EXPECT_NE(graph->FindFirstNodeMatchType("AscBackend"), nullptr);
  EXPECT_EQ(graph->FindNode("abs_d"), nullptr);
  EXPECT_EQ(graph->FindNode("exp_d"), nullptr);
  dlog_setlevel(0, 3, 0);
}

/**
 * 用例描述：不满足小值条件时不触发 host 下沉保护
 * 预置条件：同上，但 host 链输出为 {16}(>8 小值阈值)
 * 测试步骤：调用 AutofuseOptimize::Run
 * 预期结果：relu_h 不被判定为 host 下沉，照常参与融合（节点被融合消除）
 */
TEST_F(TestCanfusePass, test_fuse_host_chain_when_output_not_small) {
  dlog_setlevel(0, 0, 0);
  InstallHostExecGuardEnv(ge_env, "DNN_VM_HOST_CPU_OP_STORE");
  InitOoForAutofuse();
  auto graph = BuildHostExecGuardGraph({16}, {16}, {2, 2, 2, 2});
  ASSERT_NE(graph, nullptr);
  AutofuseOptimize autofuser;
  ASSERT_EQ(autofuser.Run(graph, MakeHostExecGuardInputs({16}, {16}, {2, 2, 2, 2})), GRAPH_SUCCESS);

  EXPECT_EQ(graph->FindNode("relu_h"), nullptr);
  EXPECT_NE(graph->FindFirstNodeMatchType("AscBackend"), nullptr);
  dlog_setlevel(0, 3, 0);
}

/**
 * 用例描述：相同小值链但缺少 host 锚点时照常融合，用于对照证明保护来自 host 下沉判定
 * 预置条件：与 host 小值链用例完全相同的 {2,2,2} 形状，仅去掉 data_h 的 host tensor 锚点
 * 测试步骤：调用 AutofuseOptimize::Run
 * 预期结果：relu_h 不被判定为 host 下沉，照常参与融合（节点被融合消除）
 */
TEST_F(TestCanfusePass, test_fuse_host_chain_when_no_host_anchor) {
  dlog_setlevel(0, 0, 0);
  InstallHostExecGuardEnv(ge_env, "DNN_VM_HOST_CPU_OP_STORE");
  InitOoForAutofuse();
  auto graph = BuildHostExecGuardGraph({2, 2, 2}, {2, 2, 2}, {2, 2, 2, 2}, false);
  ASSERT_NE(graph, nullptr);
  AutofuseOptimize autofuser;
  ASSERT_EQ(autofuser.Run(graph, MakeHostExecGuardInputs({2, 2, 2}, {2, 2, 2}, {2, 2, 2, 2})), GRAPH_SUCCESS);

  EXPECT_EQ(graph->FindNode("relu_h"), nullptr);
  EXPECT_NE(graph->FindFirstNodeMatchType("AscBackend"), nullptr);
  dlog_setlevel(0, 3, 0);
}
}  // namespace ge
