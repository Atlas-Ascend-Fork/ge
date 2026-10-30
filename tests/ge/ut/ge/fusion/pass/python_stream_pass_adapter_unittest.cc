/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "compiler/graph/fusion/pass/pass_registry.h"
#include "compiler/graph/fusion/pass/python_pass_adapter.h"
#include "graph/compute_graph.h"
#include "graph/utils/graph_utils_ex.h"
#include "register/custom_pass_helper.h"
#include "register/register_custom_pass.h"

namespace ge {
namespace fusion {
namespace {
std::atomic<int32_t> g_destroy_count{0};
std::atomic<int32_t> g_run_count{0};
int g_holder = 0;

// The framework entry (CustomPassHelper::Run -> RunAllocateStreamPass) requires
// the Graph to carry a real ComputeGraph, so a default-constructed Graph would
// be rejected before reaching the adapter behaviour under test.
GraphPtr MakeGraph() {
  const auto compute_graph = std::make_shared<ComputeGraph>("python_stream_pass_adapter_graph");
  return GraphUtilsEx::CreateGraphPtrFromComputeGraph(compute_graph);
}

PythonPassDescriptor MakeDescriptor(const char *name = "PyStreamAdapterPass") {
  PythonPassDescriptor descriptor;
  descriptor.descriptor_key = std::string("ut.module:Class:") + name;
  descriptor.pass_name = name;
  descriptor.module_name = "ut.module";
  descriptor.class_name = "Class";
  descriptor.stage = CustomPassStage::kAfterAssignLogicStream;
  descriptor.kind = PythonPassKind::kStream;
  return descriptor;
}

void *CreateHolder(const PythonPassDescriptor *) {
  return &g_holder;
}

void DestroyHolder(void *) {
  g_destroy_count.fetch_add(1);
}

Status RunHolder(const void *, const ConstGraphPtr &, CustomPassContext &) {
  g_run_count.fetch_add(1);
  return SUCCESS;
}

Status FailHolder(const void *, const ConstGraphPtr &, CustomPassContext &context) {
  g_run_count.fetch_add(1);
  context.SetErrorMessage("resource-plan callback failed");
  return FAILED;
}

PythonResourcePlanPassCallbacks MakeCallbacks(PythonResourcePlanPassRunFn run = &RunHolder,
                                              PythonFusionBasePassHolderCreateFn create = &CreateHolder,
                                              PythonFusionBasePassHolderDestroyFn destroy = &DestroyHolder) {
  PythonResourcePlanPassCallbacks callbacks;
  callbacks.create = create;
  callbacks.destroy = destroy;
  callbacks.run_resource_plan = run;
  return callbacks;
}
}  // namespace

// RunStreamResourcePlan is the stream handler that RegisterStreamResourcePlan
// wraps into the registration callback.  It is not declared in
// python_pass_adapter.h because production callers always reach it through
// CustomPassHelper, whose CustomAllocateStreamPassFunc signature already fixes
// the context type.  Declare it here so its own context contract is testable.
Status RunStreamResourcePlan(const PythonPassDescriptor &pass_desc, const ConstGraphPtr &graph,
                             CustomPassContext &pass_context);

class UtestPythonStreamPassAdapter : public testing::Test {
 protected:
  void SetUp() override {
    CustomPassHelper::Instance().Unload();
    ClearPythonResourcePlanRuntimeRegistry();
    g_destroy_count.store(0);
    g_run_count.store(0);
  }

  void TearDown() override {
    ClearPythonResourcePlanRuntimeRegistry();
    CustomPassHelper::Instance().Unload();
  }
};

TEST_F(UtestPythonStreamPassAdapter, NullGraphReturnsFailedWithDiagnostic) {
  auto context = StreamPassContext(-1);
  EXPECT_EQ(RunPythonResourcePlanPass("missing", nullptr, context), FAILED);
  EXPECT_STREQ(context.GetErrorMessage().GetString(), "python resource-plan pass graph is null");
}

TEST_F(UtestPythonStreamPassAdapter, UnregisteredDescriptorReturnsFailed) {
  auto context = StreamPassContext(-1);
  EXPECT_EQ(RunPythonResourcePlanPass("missing", MakeGraph(), context), FAILED);
  EXPECT_STREQ(context.GetErrorMessage().GetString(), "python resource-plan pass descriptor is not registered");
}

TEST_F(UtestPythonStreamPassAdapter, HolderIsDestroyedAfterSuccessAndFailure) {
  const auto descriptor = MakeDescriptor("PyHolderGuardPass");
  ASSERT_TRUE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));
  auto context = StreamPassContext(-1);
  EXPECT_EQ(RunPythonResourcePlanPass(descriptor.descriptor_key, MakeGraph(), context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
  EXPECT_EQ(g_destroy_count.load(), 1);

  ClearPythonResourcePlanRuntimeRegistry();
  CustomPassHelper::Instance().Unload();
  const auto failed_descriptor = MakeDescriptor("PyHolderGuardFailPass");
  ASSERT_TRUE(RegisterPythonResourcePlanPass(failed_descriptor, MakeCallbacks(&FailHolder)));
  EXPECT_EQ(RunPythonResourcePlanPass(failed_descriptor.descriptor_key, MakeGraph(), context), FAILED);
  EXPECT_EQ(g_run_count.load(), 2);
  EXPECT_EQ(g_destroy_count.load(), 2);
  EXPECT_STREQ(context.GetErrorMessage().GetString(), "resource-plan callback failed");
}

TEST_F(UtestPythonStreamPassAdapter, NullHolderReturnsFailedAndNeverRunsCallback) {
  const auto descriptor = MakeDescriptor("PyNullHolderPass");
  auto callbacks = MakeCallbacks();
  callbacks.create = [](const PythonPassDescriptor *) -> void * { return nullptr; };
  ASSERT_TRUE(RegisterPythonResourcePlanPass(descriptor, callbacks));
  auto context = StreamPassContext(-1);
  EXPECT_EQ(RunPythonResourcePlanPass(descriptor.descriptor_key, MakeGraph(), context), FAILED);
  EXPECT_STREQ(context.GetErrorMessage().GetString(), "python resource-plan pass holder creation failed");
  EXPECT_EQ(g_run_count.load(), 0);
  EXPECT_EQ(g_destroy_count.load(), 0);
}

TEST_F(UtestPythonStreamPassAdapter, StreamHandlerRejectsBaseContext) {
  const auto descriptor = MakeDescriptor("PyContextContractPass");
  ASSERT_TRUE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));
  // The C ABI transports the base CustomPassContext, so the stream handler has
  // to reject anything that is not the promised concrete context, and it must
  // report why instead of letting the Python callback observe a stream-less
  // context.
  CustomPassContext base_context;
  EXPECT_EQ(RunStreamResourcePlan(descriptor, MakeGraph(), base_context), FAILED);
  EXPECT_STREQ(base_context.GetErrorMessage().GetString(), "resource-plan stream pass requires StreamPassContext");
  EXPECT_EQ(g_run_count.load(), 0);

  // The very same descriptor runs once the concrete context is supplied.
  auto stream_context = StreamPassContext(-1);
  EXPECT_EQ(RunStreamResourcePlan(descriptor, MakeGraph(), stream_context), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

TEST_F(UtestPythonStreamPassAdapter, FrameworkRejectsBaseContextBeforeCallback) {
  const auto descriptor = MakeDescriptor("PyFrameworkContextPass");
  ASSERT_TRUE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));
  auto graph = MakeGraph();
  CustomPassContext context;
  // RunAllocateStreamPass down-casts the context before it calls any registered
  // stream pass, so a caller that breaks the stage contract is stopped by the
  // framework and the Python callback is never reached.  GE_ASSERT_SUCCESS maps
  // that failure to GRAPH_FAILED, hence the EXPECT_NE against SUCCESS.
  EXPECT_NE(CustomPassHelper::Instance().Run(graph, context, CustomPassStage::kAfterAssignLogicStream), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 0);
}

TEST_F(UtestPythonStreamPassAdapter, UnknownKindOrStageIsRejectedBeforeRuntimeRegistration) {
  auto callbacks = MakeCallbacks();
  auto unknown = MakeDescriptor("PyUnknownKindPass");
  unknown.kind = static_cast<PythonPassKind>(99U);
  EXPECT_FALSE(RegisterPythonResourcePlanPass(unknown, callbacks));

  auto wrong_stage = MakeDescriptor("PyWrongStagePass");
  wrong_stage.stage = CustomPassStage::kAfterInferShape;
  EXPECT_FALSE(RegisterPythonResourcePlanPass(wrong_stage, callbacks));
}

TEST_F(UtestPythonStreamPassAdapter, RegistrationFailureRollsBackRuntimeEntry) {
  // A C++ custom pass already owns this name/stage identity, so the runtime
  // registry accepts the descriptor but CustomPassHelper's global name check
  // rejects it.  The adapter must then unregister the runtime entry rather than
  // leave a half-registration behind.
  PassRegistrationData cpp_reg_data("PyRollbackPass");
  cpp_reg_data.CustomAllocateStreamPassFn([](const ConstGraphPtr &, StreamPassContext &) -> Status { return SUCCESS; })
      .Stage(CustomPassStage::kAfterAssignLogicStream);
  CustomPassHelper::Instance().Insert(cpp_reg_data);

  const auto descriptor = MakeDescriptor("PyRollbackPass");
  EXPECT_FALSE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));

  // The rolled-back descriptor must no longer be resolvable at run time.
  auto context = StreamPassContext(-1);
  EXPECT_EQ(RunPythonResourcePlanPass(descriptor.descriptor_key, MakeGraph(), context), FAILED);
  EXPECT_STREQ(context.GetErrorMessage().GetString(), "python resource-plan pass descriptor is not registered");

  // Only the C++ pass remains registered, so the stage still runs exactly once
  // and never calls into the Python callbacks.
  auto graph = MakeGraph();
  auto stage_context = StreamPassContext(-1);
  EXPECT_EQ(CustomPassHelper::Instance().Run(graph, stage_context, CustomPassStage::kAfterAssignLogicStream), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 0);
}

TEST_F(UtestPythonStreamPassAdapter, DuplicatePassNameIsRejectedAndFirstPassKeepsRunning) {
  const auto descriptor = MakeDescriptor("PyDuplicateNamePass");
  ASSERT_TRUE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));
  // Same pass name and stage under a different descriptor key: the runtime
  // registry refuses it so a second Python instance cannot shadow the first.
  auto duplicate_name = descriptor;
  duplicate_name.descriptor_key += ":second";
  EXPECT_FALSE(RegisterPythonResourcePlanPass(duplicate_name, MakeCallbacks()));
  // The identical descriptor key is refused too, which keeps a repeated bridge
  // registration deterministic.
  EXPECT_FALSE(RegisterPythonResourcePlanPass(descriptor, MakeCallbacks()));

  auto context = StreamPassContext(-1);
  auto graph = MakeGraph();
  EXPECT_EQ(CustomPassHelper::Instance().Run(graph, context, CustomPassStage::kAfterAssignLogicStream), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 1);
}

TEST_F(UtestPythonStreamPassAdapter, MalformedRegistrationDataIsRejectedByHelper) {
  // PassRegistrationData is default-constructible and then carries no impl, the
  // state the bridge would hand over if the impl allocation had failed.  Every
  // getter must stay usable and the helper must refuse the registration.
  PassRegistrationData empty_reg_data;
  EXPECT_TRUE(empty_reg_data.GetPassName().empty());
  EXPECT_EQ(empty_reg_data.GetStage(), CustomPassStage::kInvalid);
  EXPECT_FALSE(static_cast<bool>(empty_reg_data.GetCustomAllocateStreamPass()));
  EXPECT_FALSE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(empty_reg_data));

  // A named registration without a stream callback is malformed as well.
  PassRegistrationData no_callback("PyNoCallbackPass");
  no_callback.Stage(CustomPassStage::kAfterAssignLogicStream);
  EXPECT_FALSE(CustomPassHelper::Instance().RegisterPythonResourcePlanPass(no_callback));

  // PassRegistrationData::Stage() silently ignores kInvalid, so a registration
  // that owns an impl can never report an out-of-range stage; only the
  // impl-less case above reaches that part of the malformed check.
  PassRegistrationData invalid_stage("PyInvalidStagePass");
  invalid_stage.Stage(CustomPassStage::kInvalid);
  EXPECT_EQ(invalid_stage.GetStage(), CustomPassStage::kBeforeInferShape);

  // Nothing was registered, so the stage is a no-op.
  auto graph = MakeGraph();
  auto context = StreamPassContext(-1);
  EXPECT_EQ(CustomPassHelper::Instance().Run(graph, context, CustomPassStage::kAfterAssignLogicStream), SUCCESS);
  EXPECT_EQ(g_run_count.load(), 0);
}
}  // namespace fusion
}  // namespace ge
