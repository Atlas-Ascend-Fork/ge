/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/**
 * Registration lifecycle for Python resource-plan passes.
 *
 * RegisterPythonResourcePlanPass() is the only entry the bridge may use.  It
 * takes the helper's write lock itself, refuses a name/stage that a C++ pass
 * already owns, and records the registration source so that
 * ClearPythonResourcePlanPasses() removes only what the bridge added.  A
 * Load/Unload/Load cycle has to end with exactly one registration, no stale
 * lambda and no leftover de-duplication key.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <graph_utils.h>
#include <graph_utils_ex.h>

#include "register/register_custom_pass.h"
#include "register/custom_pass_context_impl.h"
#include "register/custom_pass_helper.h"
#include "register/pass_option_utils.h"
#include "framework/common/debug/ge_log.h"
#include "graph/common/share_graph.h"

namespace ge {
namespace {
std::atomic<int32_t> g_run_count{0};

Status CountingStreamPass(const ConstGraphPtr &graph, StreamPassContext &context) {
  (void)graph;
  (void)context;
  g_run_count.fetch_add(1);
  return SUCCESS;
}

Status FailingStreamPass(const ConstGraphPtr &graph, StreamPassContext &context) {
  (void)graph;
  context.SetErrorMessage("python resource-plan pass failed");
  return FAILED;
}

PassRegistrationData MakeStreamRegistration(const std::string &pass_name,
                                            Status (*fn)(const ConstGraphPtr &, StreamPassContext &)) {
  PassRegistrationData reg_data(pass_name);
  reg_data.CustomAllocateStreamPassFn(fn).Stage(CustomPassStage::kAfterAssignLogicStream);
  return reg_data;
}
}  // namespace

class UtestRegisterPythonResourcePlanPass : public testing::Test {
 protected:
  void SetUp() override {
    CustomPassHelper::Instance().Unload();
    g_run_count.store(0);
  }
  void TearDown() override {
    CustomPassHelper::Instance().Unload();
  }

  static GraphPtr BuildGraph() {
    auto compute_graph = SharedGraph<ComputeGraphPtr, ut::GraphBuilder>::BuildGraphWithControlEdge();
    compute_graph->TopologicalSorting();
    auto graph = GraphUtilsEx::CreateGraphFromComputeGraph(compute_graph);
    return std::make_shared<Graph>(graph);
  }

  static Status RunStreamStage(StreamPassContext &context) {
    auto graph_ptr = BuildGraph();
    return CustomPassHelper::Instance().Run(graph_ptr, context, CustomPassStage::kAfterAssignLogicStream);
  }
};

// ── 注册与执行 ────────────────────────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, RegisterThenRun_Success) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyStreamPass", CountingStreamPass)));

  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

TEST_F(UtestRegisterPythonResourcePlanPass, PassFailure_AbortsTheStage) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyFailingPass", FailingStreamPass)));

  StreamPassContext context(-1);
  EXPECT_NE(RunStreamStage(context), SUCCESS);
}

// ── 拒绝畸形注册 ──────────────────────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, EmptyPassName_Rejected) {
  EXPECT_FALSE(
      CustomPassHelper::Instance().RegisterPythonResourcePlanPass(MakeStreamRegistration("", CountingStreamPass)));
}

TEST_F(UtestRegisterPythonResourcePlanPass, MissingCallback_Rejected) {
  // No CustomAllocateStreamPassFn was set, so there is nothing to execute.
  PassRegistrationData reg_data("PyNoCallbackPass");
  reg_data.Stage(CustomPassStage::kAfterAssignLogicStream);
  EXPECT_FALSE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(reg_data));
}

// ── 去重：禁止 C++/Python 双执行 ──────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, DuplicatePythonRegistration_Rejected) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyDuplicatePass", CountingStreamPass)));
  EXPECT_FALSE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyDuplicatePass", CountingStreamPass)));

  // The rejected attempt must not have added a second executable entry.
  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

TEST_F(UtestRegisterPythonResourcePlanPass, SameNameAsCppPass_Rejected) {
  // A C++ pass owns this name/stage first; the Python registrar must not add
  // a second one, or the same pass name would run twice.
  CustomPassHelper::Instance().Insert(MakeStreamRegistration("SharedName", CountingStreamPass));
  EXPECT_FALSE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("SharedName", CountingStreamPass)));

  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

// ── 按来源清理：只删 Python 注册项 ───────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, ClearRemovesOnlyPythonRegistrations) {
  CustomPassHelper::Instance().Insert(MakeStreamRegistration("CppKeptPass", CountingStreamPass));
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyRemovedPass", CountingStreamPass)));

  StreamPassContext before(-1);
  EXPECT_EQ(RunStreamStage(before), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 2);

  CustomPassHelper::Instance().ClearPythonResourcePlanPasses();

  // The C++ registration survives; only the Python one is gone.
  g_run_count.store(0);
  StreamPassContext after(-1);
  EXPECT_EQ(RunStreamStage(after), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

TEST_F(UtestRegisterPythonResourcePlanPass, ClearIsIdempotent) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyIdempotentPass", CountingStreamPass)));
  CustomPassHelper::Instance().ClearPythonResourcePlanPasses();
  CustomPassHelper::Instance().ClearPythonResourcePlanPasses();

  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 0);
}

// ── Load / Unload / Load 生命周期 ────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, RegisterClearRegister_LeavesExactlyOneEntry) {
  for (int32_t round = 0; round < 3; ++round) {
    EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
        MakeStreamRegistration("PyReloadedPass", CountingStreamPass)))
        << "re-registration failed in round " << round;

    g_run_count.store(0);
    StreamPassContext context(-1);
    EXPECT_EQ(RunStreamStage(context), SUCCESS);
    // Exactly one entry each round: a stale de-duplication key would have
    // blocked the re-registration, a stale vector entry would run twice.
    EXPECT_EQ(g_run_count.load(), 1) << "unexpected run count in round " << round;

    CustomPassHelper::Instance().ClearPythonResourcePlanPasses();
  }
}

TEST_F(UtestRegisterPythonResourcePlanPass, UnloadAlsoDropsPythonRegistrations) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyUnloadedPass", CountingStreamPass)));
  CustomPassHelper::Instance().Unload();

  // Unload clears the source bookkeeping too, so the next round registers.
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyUnloadedPass", CountingStreamPass)));
  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

// ── 执行序：同阶段按 pass 名字典序 ───────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, StreamPassesRunInPassNameOrder) {
  static std::vector<std::string> order;
  order.clear();

  auto make_recorder = [](const char *name) {
    return [name](const ConstGraphPtr &, StreamPassContext &) -> Status {
      order.emplace_back(name);
      return SUCCESS;
    };
  };

  // Register out of alphabetical order on purpose.
  PassRegistrationData zebra("ZebraPass");
  zebra.CustomAllocateStreamPassFn(make_recorder("ZebraPass")).Stage(CustomPassStage::kAfterAssignLogicStream);
  PassRegistrationData alpha("AlphaPass");
  alpha.CustomAllocateStreamPassFn(make_recorder("AlphaPass")).Stage(CustomPassStage::kAfterAssignLogicStream);
  PassRegistrationData mike("MikePass");
  mike.CustomAllocateStreamPassFn(make_recorder("MikePass")).Stage(CustomPassStage::kAfterAssignLogicStream);

  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(zebra));
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(alpha));
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(mike));

  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  ASSERT_EQ(order.size(), 3U);
  EXPECT_EQ(order[0], "AlphaPass");
  EXPECT_EQ(order[1], "MikePass");
  EXPECT_EQ(order[2], "ZebraPass");
}

// ── fusion switch 对 Python 资源规划注册项生效 ────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, SwitchDisablesPythonResourcePlanPass) {
  CustomPassHelper::Instance().Insert(MakeStreamRegistration("CppDefaultOn", CountingStreamPass));
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PySwitchable", CountingStreamPass)));

  // The Python registration is switched off while the C++ registration keeps
  // its default-on switch, so only the C++ pass runs.
  const std::map<std::string, bool> switches{{"PySwitchable", false}};
  auto graph_ptr = BuildGraph();
  StreamPassContext context(-1);
  EXPECT_EQ(CustomPassHelper::Instance().Run(graph_ptr, context, CustomPassStage::kAfterAssignLogicStream, switches),
            SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

// ── 并发注册：接口自己加锁 ───────────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, ConcurrentRegistrationIsSerialized) {
  // The design forbids relying on "LoadPassPlugins happens to run first".
  // Only one of the racing threads may win a given name/stage.
  constexpr int32_t kThreads = 8;
  std::atomic<int32_t> accepted{0};
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int32_t i = 0; i < kThreads; ++i) {
    threads.emplace_back([&accepted]() {
      if (CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
              MakeStreamRegistration("PyRacingPass", CountingStreamPass))) {
        accepted.fetch_add(1);
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
  EXPECT_EQ(accepted.load(), 1);

  StreamPassContext context(-1);
  EXPECT_EQ(RunStreamStage(context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

// ── 索引清理：Python 项之后插入 C++ 项也不能误删 ─────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, ClearUsesSourceIndexAndKeepsLaterCppInsert) {
  EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
      MakeStreamRegistration("PyIndexedEntry", CountingStreamPass)));

  // ``Insert`` intentionally has no de-duplication: this models a legacy
  // plugin registering a C++ pass after the bridge has installed Python
  // callbacks.  Cleanup must erase the exact Python vector index, not match a
  // name/key and accidentally remove the C++ suffix.
  CustomPassHelper::Instance().Insert(MakeStreamRegistration("CppInsertedAfter", CountingStreamPass));

  StreamPassContext before(-1);
  EXPECT_EQ(RunStreamStage(before), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 2);

  CustomPassHelper::Instance().ClearPythonResourcePlanPasses();
  g_run_count.store(0);
  StreamPassContext after(-1);
  EXPECT_EQ(RunStreamStage(after), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

// ── 无 switch 文件时的默认策略 ───────────────────────────────────────

TEST_F(UtestRegisterPythonResourcePlanPass, MissingFusionSwitchDefaultsPythonPassToEnabled) {
  // An empty parsed switch table represents a missing/empty fusion-switch
  // file.  PassOptionUtils must fall back to the registration's default switch
  // rather than treating the absence of a switch as disabled.
  const std::map<std::string, bool> no_switches;
  EXPECT_TRUE(PassOptionUtils::IsPassEnable(no_switches, "NoSwitchPythonPass", PassSwitch::kOn));
}

TEST_F(UtestRegisterPythonResourcePlanPass, LoadUnloadLoadCyclesLeaveOneExecutableEntry) {
  for (int32_t round = 0; round < 3; ++round) {
    EXPECT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(
        MakeStreamRegistration("PyLoadUnloadCycle", CountingStreamPass)))
        << "registration failed in round " << round;
    g_run_count.store(0);
    StreamPassContext context(-1);
    EXPECT_EQ(RunStreamStage(context), SUCCESS);
    EXPECT_EQ(g_run_count.load(), 1);
    EXPECT_EQ(CustomPassHelper::Instance().Unload(), SUCCESS);
  }
}

TEST_F(UtestRegisterPythonResourcePlanPass, CallbackReceivesBorrowedGraphAndContext) {
  const Graph *seen_graph = nullptr;
  CustomPassContext *seen_context = nullptr;
  auto callback = [&seen_graph, &seen_context](const ConstGraphPtr &graph, StreamPassContext &context) -> Status {
    seen_graph = graph.get();
    seen_context = &context;
    return SUCCESS;
  };
  PassRegistrationData registration("PyBorrowedCallback");
  registration.CustomAllocateStreamPassFn(callback).Stage(CustomPassStage::kAfterAssignLogicStream);
  ASSERT_TRUE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(registration));

  auto graph = BuildGraph();
  StreamPassContext context(-1);
  EXPECT_EQ(CustomPassHelper::Instance().Run(graph, context, CustomPassStage::kAfterAssignLogicStream), SUCCESS);
  EXPECT_EQ(seen_graph, graph.get());
  EXPECT_EQ(seen_context, static_cast<CustomPassContext *>(&context));
}
}  // namespace ge
