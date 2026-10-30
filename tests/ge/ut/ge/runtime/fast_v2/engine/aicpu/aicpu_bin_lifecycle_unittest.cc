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
#include <map>
#include <thread>
#include <type_traits>
#include "stub/gert_runtime_stub.h"
#include "faker/kernel_run_context_facker.h"
#include "register/kernel_registry.h"
#include "engine/aicpu/kernel/aicpu_args_handler.h"
#include "engine/aicpu/kernel/aicpu_bin_handler.h"
#include "graph/op_kernel_bin.h"
#include "aicpu_engine_struct.h"
#include "framework/common/taskdown_common.h"
#include "common/bg_test.h"
#include "common/share_graph.h"
#include "common/model_v2_executor_test_helper.h"
#include "faker/global_data_faker.h"
#include "faker/aicpu_taskdef_faker.h"
#include "lowering/graph_converter.h"

namespace gert {
namespace {
static_assert(sizeof(CustAicpuBinLease) > sizeof(void *), "Lease must be stored out of line in its Chain");
static_assert(!std::is_copy_constructible<CustAicpuBinLease>::value, "Lease must have unique ownership");

class BinLifecycleAclStub : public AclRuntimeStubImpl {
 public:
  aclError aclrtGetCurrentContext(aclrtContext *context) override {
    *context = current;
    return current == nullptr ? ACL_ERROR_RT_CONTEXT_NULL : get_context_error;
  }
  aclError aclrtSetCurrentContext(aclrtContext context) override {
    switches.push_back(context);
    if ((context == nullptr) || (context == fail_set_context)) {
      return ACL_ERROR_FAILURE;
    }
    current = context;
    return ACL_SUCCESS;
  }
  aclError aclrtBinaryLoadFromData(const void *, size_t, const aclrtBinaryLoadOptions *,
                                   aclrtBinHandle *handle) override {
    if (++load_attempts == fail_load_at) {
      *handle = nullptr;
      return ACL_ERROR_FAILURE;
    }
    *handle = reinterpret_cast<void *>(++loads + 0x1000U);
    owners[*handle] = current;
    return ACL_SUCCESS;
  }
  aclError aclrtRegisterCpuFunc(aclrtBinHandle handle, const char *, const char *, aclrtFuncHandle *func) override {
    ++registration_attempts;
    *func = handle;
    return ACL_SUCCESS;
  }
  aclError aclrtBinaryUnLoad(aclrtBinHandle handle) override {
    EXPECT_EQ(current, owners.at(handle));
    ++unload_attempts;
    attempted_handles.push_back(handle);
    if (fail_unload || (handle == fail_unload_handle)) {
      return ACL_ERROR_FAILURE;
    }
    unloaded.push_back(handle);
    owners.erase(handle);
    return ACL_SUCCESS;
  }
  aclrtContext current = reinterpret_cast<void *>(0x11U);
  aclrtContext fail_set_context = nullptr;
  aclError get_context_error = ACL_SUCCESS;
  bool fail_unload = false;
  aclrtBinHandle fail_unload_handle = nullptr;
  size_t loads = 0U;
  size_t load_attempts = 0U;
  size_t fail_load_at = 0U;
  size_t unload_attempts = 0U;
  size_t registration_attempts = 0U;
  std::map<aclrtBinHandle, aclrtContext> owners;
  std::vector<aclrtBinHandle> unloaded;
  std::vector<aclrtBinHandle> attempted_handles;
  std::vector<aclrtContext> switches;
};

class AicpuBinLifecycleUT : public bg::BgTest {
 protected:
  void SetUp() override {
    auto acl = std::make_unique<BinLifecycleAclStub>();
    stub = acl.get();
    runtime = std::make_unique<GertRuntimeStub>(std::make_unique<RuntimeStubImpl>(), true, std::move(acl));
    op = std::make_shared<ge::OpDesc>("custom", "Custom");
    std::vector<char> bytes{'b', 'i', 'n'};
    op->SetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL, std::make_shared<ge::OpKernelBin>("custom", std::move(bytes)));
  }
  KernelRunContextHolder Context(const std::string &type = "BuildCCArgsBinHandle") {
    auto context =
        KernelRunContextFaker().KernelIONum(4U, 2U).NodeIoNum(0U, 0U).IrInputNum(0U).KernelType(type).Build();
    context.value_holder[0].Set(op.get(), nullptr);
    context.value_holder[1].Set(node_type.data(), nullptr);
    context.value_holder[2].Set(reinterpret_cast<void *>(static_cast<uintptr_t>(ge::ccKernelType::CUST_AI_CPU)),
                                nullptr);
    context.value_holder[3].Set(so.data(), nullptr);
    EXPECT_EQ(KernelRegistry::GetInstance().FindKernelFuncs("BuildCCArgsBinHandle")->outputs_creator(nullptr, context),
              ge::GRAPH_SUCCESS);
    return context;
  }
  ge::graphStatus Run(KernelRunContextHolder &context) {
    return KernelRegistry::GetInstance().FindKernelFuncs("BuildCCArgsBinHandle")->run_func(context);
  }
  ge::graphStatus Release(KernelRunContextHolder &context) {
    auto release = BuildKernelRunContext(1U, 0U);
    release.value_holder[0].Set(context.GetContext<KernelContext>()->GetOutputPointer<CustAicpuBinLease>(1U), nullptr);
    return KernelRegistry::GetInstance().FindKernelFuncs("ReleaseCustAicpuBinHandle")->run_func(release);
  }
  void ConfigureCustomGraph(const ge::ComputeGraphPtr &graph) {
    auto custom = graph->FindNode("add1")->GetOpDesc();
    custom->SetOpKernelLibName(ge::kEngineNameAiCpu.c_str());
    custom->SetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL,
                       op->TryGetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL, ge::OpKernelBinPtr()));
    ASSERT_TRUE(ge::AttrUtils::SetInt(custom, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, 4));
    ASSERT_TRUE(ge::AttrUtils::SetBool(custom, "_cust_aicpu_flag", true));
    ASSERT_TRUE(ge::AttrUtils::SetStr(custom, "kernelSo", "libcust_aicpu_kernels.so"));
    ASSERT_EQ(graph->TopologicalSorting(), ge::GRAPH_SUCCESS);
  }

  BinLifecycleAclStub *stub = nullptr;
  std::unique_ptr<GertRuntimeStub> runtime;
  ge::OpDescPtr op;
  std::string node_type = "Custom";
  std::string so = "lifecycle.so";
};

TEST_F(AicpuBinLifecycleUT, FailedSecondBinaryLoadRetainsFirstLeaseAndRetryUnloadsOnce) {
  std::string other_so = "other_lifecycle.so";
  auto a = Context();
  auto b = Context();
  b.value_holder[3].Set(other_so.data(), nullptr);
  ASSERT_EQ(Run(a), ge::GRAPH_SUCCESS);
  const auto first_bin = *a.GetContext<KernelContext>()->GetOutputPointer<rtBinHandle>(0U);
  stub->fail_load_at = 2U;
  ASSERT_NE(Run(b), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->load_attempts, 2U);
  EXPECT_TRUE(stub->unloaded.empty());
  EXPECT_EQ(*b.GetContext<KernelContext>()->GetOutputPointer<rtBinHandle>(0U), nullptr);
  EXPECT_EQ(stub->owners.size(), 1U);
  EXPECT_EQ(*a.GetContext<KernelContext>()->GetOutputPointer<rtBinHandle>(0U), first_bin);
  stub->fail_load_at = 0U;
  ASSERT_EQ(Run(b), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->loads, 2U);
  EXPECT_EQ(stub->registration_attempts, 0U);
  ASSERT_EQ(Release(a), ge::GRAPH_SUCCESS);
  ASSERT_EQ(Release(b), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 2U);
  EXPECT_EQ(Release(b), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unload_attempts, 2U);
}

TEST_F(AicpuBinLifecycleUT, FailedModelDoesNotReleaseSharedSuccessfulModel) {
  std::string other_so = "failed_model.so";
  auto successful = Context();
  ASSERT_EQ(Run(successful), ge::GRAPH_SUCCESS);
  {
    auto failed_a = Context();
    auto failed_b = Context();
    failed_b.value_holder[3].Set(other_so.data(), nullptr);
    ASSERT_EQ(Run(failed_a), ge::GRAPH_SUCCESS);
    stub->fail_load_at = 2U;
    ASSERT_NE(Run(failed_b), ge::GRAPH_SUCCESS);
  }
  EXPECT_TRUE(stub->unloaded.empty());
  EXPECT_EQ(stub->loads, 1U);
  ASSERT_EQ(Release(successful), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 1U);
}

TEST_F(AicpuBinLifecycleUT, BinaryLoadFailureLeavesNoLeaseAndCanRetry) {
  auto context = Context();
  stub->fail_load_at = 1U;
  ASSERT_NE(Run(context), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(stub->owners.empty());
  EXPECT_TRUE(stub->unloaded.empty());
  EXPECT_EQ(*context.GetContext<KernelContext>()->GetOutputPointer<rtBinHandle>(0U), nullptr);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unload_attempts, 0U);
  stub->fail_load_at = 0U;
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->loads, 1U);
  EXPECT_EQ(stub->registration_attempts, 0U);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 1U);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unload_attempts, 1U);
}

TEST_F(AicpuBinLifecycleUT, MissingBinaryFailsInitBeforeLoadingOrRegistering) {
  auto context = Context();
  op->SetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL, ge::OpKernelBinPtr());
  EXPECT_NE(Run(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->loads, 0U);
  EXPECT_EQ(stub->registration_attempts, 0U);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
}

TEST_F(AicpuBinLifecycleUT, ChainDestructionReleasesInOwnerContextAndRestoresCaller) {
  const auto owner = stub->current;
  const auto caller = reinterpret_cast<void *>(0x22U);
  {
    auto context = Context();
    ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
    stub->current = caller;
  }
  EXPECT_EQ(stub->unloaded.size(), 1U);
  EXPECT_EQ(stub->current, caller);
  EXPECT_EQ(stub->switches, (std::vector<aclrtContext>{owner, caller}));
}

TEST_F(AicpuBinLifecycleUT, UnloadFailureRetainsLeaseAndRunMustResetBeforeAcquire) {
  auto context = Context();
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  stub->fail_unload = true;
  EXPECT_NE(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_NE(Run(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(*context.GetContext<KernelContext>()->GetOutputPointer<rtBinHandle>(0U), nullptr);
  EXPECT_EQ(stub->loads, 1U);
  stub->fail_unload = false;
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unload_attempts, 3U);
  EXPECT_EQ(stub->unloaded.size(), 1U);
}

TEST_F(AicpuBinLifecycleUT, UnboundThreadDestructionReleasesAndKeepsOwnerContext) {
  const auto owner = stub->current;
  auto context = std::make_unique<KernelRunContextHolder>(Context());
  ASSERT_EQ(Run(*context), ge::GRAPH_SUCCESS);
  std::thread worker([&]() {
    ge::AclRuntimeStub::Install(stub);
    stub->current = nullptr;
    context.reset();
    EXPECT_EQ(stub->current, owner);
    ge::AclRuntimeStub::UnInstall(stub);
  });
  worker.join();
  EXPECT_TRUE(stub->owners.empty());
  EXPECT_EQ(stub->unloaded.size(), 1U);
  EXPECT_EQ(stub->switches, (std::vector<aclrtContext>{owner}));
  stub->current = owner;
  auto retry = Context();
  ASSERT_EQ(Run(retry), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->loads, 2U);
  ASSERT_EQ(Release(retry), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 2U);
}

TEST_F(AicpuBinLifecycleUT, UnboundThreadFailedUnloadKeepsOwnerContextAndLeaseForRetry) {
  const auto owner = stub->current;
  auto context = Context();
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  stub->current = nullptr;
  stub->fail_unload = true;
  EXPECT_NE(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->current, owner);
  EXPECT_EQ(stub->owners.size(), 1U);
  stub->fail_unload = false;
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->current, owner);
  EXPECT_TRUE(stub->owners.empty());
  EXPECT_EQ(stub->unload_attempts, 2U);
  EXPECT_EQ(stub->unloaded.size(), 1U);
}

TEST_F(AicpuBinLifecycleUT, ContextQueryFailureRetainsLeaseWithoutSwitching) {
  auto context = Context();
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  stub->get_context_error = ACL_ERROR_FAILURE;
  EXPECT_NE(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(stub->switches.empty());
  EXPECT_EQ(stub->unload_attempts, 0U);
  stub->get_context_error = ACL_SUCCESS;
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(stub->owners.empty());
}

TEST_F(AicpuBinLifecycleUT, RestoreFailureAfterReleaseDoesNotReleaseTokenAgain) {
  auto context = Context();
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  stub->current = reinterpret_cast<void *>(0x22U);
  stub->fail_set_context = stub->current;
  EXPECT_NE(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 1U);
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unload_attempts, 1U);
}

TEST_F(AicpuBinLifecycleUT, FailedUnloadStillRestoresContextAndCanRetry) {
  auto context = Context();
  ASSERT_EQ(Run(context), ge::GRAPH_SUCCESS);
  const auto caller = reinterpret_cast<void *>(0x22U);
  stub->current = caller;
  stub->fail_unload = true;
  EXPECT_NE(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->current, caller);
  stub->fail_unload = false;
  EXPECT_EQ(Release(context), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->current, caller);
  EXPECT_EQ(stub->unload_attempts, 2U);
  EXPECT_EQ(stub->unloaded.size(), 1U);
}

TEST_F(AicpuBinLifecycleUT, FailedModelLoadCanRetryAndDestructionReleasesSharedLeases) {
  auto graph = ShareGraph::BuildSingleNodeGraph();
  ASSERT_NO_FATAL_FAILURE(ConfigureCustomGraph(graph));
  auto root_model = GeModelBuilder(graph).BuildGeRootModel();
  AiCpuCCTaskDefFaker task(static_cast<uint32_t>(ge::ccKernelType::CUST_AI_CPU));
  auto global_data = GlobalDataFaker(root_model).AddTaskDef("Add", task.SetNeedMemcpy(true)).Build();
  auto model_desc = ModelDescHolderFaker().Build();
  model_desc.SetSpaceRegistry(SpaceRegistryFaker().Build());
  auto exe_graph =
      GraphConverter().SetModelDescHolder(&model_desc).ConvertComputeGraphToExecuteGraph(graph, global_data);
  ASSERT_NE(exe_graph, nullptr);
  // Keep the actual binary lifecycle and constant initialization, while unrelated model kernels are no-ops.
  runtime->GetKernelStub().AllKernelRegisteredAndSuccess(
      {"Const", "BuildCCArgsBinHandle", "ReleaseCustAicpuBinHandle"});
  auto executor = ModelV2Executor::Create(exe_graph, root_model);
  ASSERT_NE(executor, nullptr);
  auto builds = ModelV2ExecutorTestHelper::GetNodesByKernelType(executor.get(), "BuildCCArgsBinHandle");
  ASSERT_EQ(builds.size(), 2U);
  for (auto build : builds) {
    ASSERT_NE(ModelV2ExecutorTestHelper::GetOutChain(build, 1U)->GetPointer<CustAicpuBinLease>(), nullptr);
  }

  // The compute and memcpy tasks share one binary, so fail the first actual load.
  stub->fail_load_at = 1U;
  ASSERT_NE(executor->Load(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->load_attempts, 1U);
  EXPECT_EQ(stub->loads, 0U);
  EXPECT_TRUE(stub->unloaded.empty());
  EXPECT_TRUE(stub->owners.empty());

  stub->fail_load_at = 0U;
  ASSERT_EQ(executor->Load(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->registration_attempts, 0U);
  EXPECT_EQ(stub->loads, 1U);
  EXPECT_TRUE(stub->unloaded.empty());
  ASSERT_EQ(executor->UnLoad(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->unloaded.size(), 1U);
  EXPECT_EQ(stub->unload_attempts, 1U);
  EXPECT_TRUE(stub->owners.empty());
  ASSERT_EQ(executor->Load(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(stub->loads, 2U);
  EXPECT_EQ(stub->owners.size(), 1U);
  executor.reset();
  EXPECT_EQ(stub->unload_attempts, 2U);
  EXPECT_EQ(stub->registration_attempts, 0U);
  EXPECT_TRUE(stub->owners.empty());
}
}  // namespace
}  // namespace gert
