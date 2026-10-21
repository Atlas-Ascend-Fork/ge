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
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engines/custom_engine/custom_graph_optimizer.h"
#include "engines/custom_engine/custom_ops_kernel_builder.h"
#include "engines/custom_engine/custom_ops_kernel_info_store.h"
#include "common/checker.h"
#include "common/ge_common/ge_types.h"
#include "exe_graph/runtime/annotated_args_context.h"
#include "graph/compute_graph.h"
#include "graph/custom_op_factory.h"
#include "graph/custom_op/args_refresh.h"
#include "graph/ascend_string.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_kernel_bin.h"
#include "graph/op_desc.h"
#include "graph/operator_reg.h"
#include "graph/custom_op.h"
#include "graph/ge_context.h"
#include "graph/ge_local_context.h"
#include "debug/ge_util.h"
#include "graph/utils/args_format_desc_utils.h"
#include "graph/args_format_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph/build/stream/declarative_stream_registry.h"
#include "graph/build/task_generator_utils.h"
#include "graph/utils/tensor_utils.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "securec.h"

namespace ge {
namespace custom {
namespace {

class MockHostCpuCustomOp : public HostCpuExecuteOp {
 public:
  graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
    (void)ctx;
    return GRAPH_SUCCESS;
  }
};

class MockCompilableCustomOp : public EagerExecuteOp, public CompilableOp {
 public:
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    (void)ctx;
    return GRAPH_SUCCESS;
  }

  graphStatus Compile(gert::OpCompileContext *ctx) override {
    (void)ctx;
    ++compile_count_;
    return GRAPH_SUCCESS;
  }

  static void ResetCompileCount() {
    compile_count_ = 0;
  }

  static int32_t GetCompileCount() {
    return compile_count_;
  }

 private:
  static int32_t compile_count_;
};

int32_t MockCompilableCustomOp::compile_count_ = 0;

class MockHostCpuCompilableCustomOp : public HostCpuExecuteOp, public CompilableOp {
 public:
  graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override {
    (void)ctx;
    return GRAPH_SUCCESS;
  }

  graphStatus Compile(gert::OpCompileContext *ctx) override {
    (void)ctx;
    ++compile_count_;
    return GRAPH_SUCCESS;
  }

  static void ResetCompileCount() {
    compile_count_ = 0;
  }

  static int32_t GetCompileCount() {
    return compile_count_;
  }

 private:
  static int32_t compile_count_;
};

int32_t MockHostCpuCompilableCustomOp::compile_count_ = 0;

class MockBaseOnlyCustomOp : public BaseCustomOp {};

std::atomic_bool g_compile_context_output_called{false};

class MockCompileContextOutputOp : public EagerExecuteOp, public CompilableOp {
 public:
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    (void)ctx;
    return GRAPH_SUCCESS;
  }

  graphStatus Compile(gert::OpCompileContext *ctx) override {
    g_compile_context_output_called.store(true);
    if (ctx == nullptr) {
      return GRAPH_FAILED;
    }
    if (ctx->GetInputTensor(0U) != nullptr) {
      return GRAPH_FAILED;
    }

    const auto output0 = ctx->GetOutputTensor(0U);
    const auto output1 = ctx->GetOutputTensor(1U);
    const auto output2 = ctx->GetOutputTensor(2U);
    if ((output0 == nullptr) || (output1 == nullptr) || (output2 == nullptr)) {
      return GRAPH_FAILED;
    }
    if ((output0 != ctx->GetRequiredOutputTensor(0U)) || (output1 != ctx->GetDynamicOutputTensor(1U, 0U)) ||
        (output2 != ctx->GetDynamicOutputTensor(1U, 1U))) {
      return GRAPH_FAILED;
    }
    if ((ctx->GetOutputTensor(3U) != nullptr) || (ctx->GetRequiredOutputTensor(2U) != nullptr) ||
        (ctx->GetDynamicOutputTensor(1U, 2U) != nullptr)) {
      return GRAPH_FAILED;
    }
    if ((output0->GetStorageShape() != gert::Shape({8, 16})) || (output0->GetDataType() != DT_FLOAT16) ||
        (output0->GetStorageFormat() != FORMAT_ND)) {
      return GRAPH_FAILED;
    }
    if ((output1->GetStorageShape() != gert::Shape({16, 16})) || (output1->GetDataType() != DT_FLOAT)) {
      return GRAPH_FAILED;
    }
    if ((output2->GetStorageShape() != gert::Shape({32, 16})) || (output2->GetDataType() != DT_INT32)) {
      return GRAPH_FAILED;
    }
    return GRAPH_SUCCESS;
  }
};

}  // namespace

class UtestCustomOpsKernelInfoStore : public testing::Test {
 protected:
  void SetUp() override {
    graph_options_ = GetThreadLocalContext().GetAllGraphOptions();
    session_options_ = GetThreadLocalContext().GetAllSessionOptions();
    global_options_ = GetThreadLocalContext().GetAllGlobalOptions();
    GetThreadLocalContext().SetGraphOption({{ge::SOC_VERSION, "KirinX90"}});
  }

  void TearDown() override {
    GetThreadLocalContext().SetGraphOption(graph_options_);
    GetThreadLocalContext().SetSessionOption(session_options_);
    GetThreadLocalContext().SetGlobalOption(global_options_);
  }

 private:
  std::map<std::string, std::string> graph_options_;
  std::map<std::string, std::string> session_options_;
  std::map<std::string, std::string> global_options_;
};

namespace {

// 自 custom_ops_kernel_info_store_unittest.cc 拆分：CustomGraphOptimizer（custom_graph_optimizer.cc）相关用例

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerCompileRunsDuringWholeGraphOnly) {
  const std::string kTestOpType = "TestCompilableOp_CompileDuringWholeGraphOnly";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockCompilableCustomOp>(); };
  const auto register_ret = CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator);
  EXPECT_TRUE((register_ret == GRAPH_SUCCESS) || (register_ret == GRAPH_FAILED));

  auto graph = ComGraphMakeShared<ComputeGraph>("custom_compile_hook_graph");
  ASSERT_NE(graph, nullptr);
  auto op_desc = ComGraphMakeShared<OpDesc>("custom_compile_hook_node", kTestOpType);
  ASSERT_NE(op_desc, nullptr);
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  MockCompilableCustomOp::ResetCompileCount();
  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
  EXPECT_EQ(MockCompilableCustomOp::GetCompileCount(), 1);

  EXPECT_EQ(optimizer.OptimizeGraphBeforeBuild(*graph), SUCCESS);
  EXPECT_EQ(MockCompilableCustomOp::GetCompileCount(), 1);
}

TEST_F(UtestCustomOpsKernelInfoStore, OOptimizeWholeGraphConstructsCompileContextOutputs) {
  const std::string kTestOpType = "TestCompileContextOutputOp_OptimizerTest";
  auto graph = std::make_shared<ComputeGraph>("compile_context_output_graph");
  auto op_desc = std::make_shared<OpDesc>("compile_context_output_op", kTestOpType);
  op_desc->AppendIrOutput("y", kIrOutputRequired);
  op_desc->AppendIrOutput("dy", kIrOutputDynamic);

  GeTensorDesc required_output_desc(GeShape({8, 16}), FORMAT_ND, DT_FLOAT16);
  op_desc->AddOutputDesc("y", required_output_desc);
  GeTensorDesc dynamic_output_desc0(GeShape({16, 16}), FORMAT_ND, DT_FLOAT);
  op_desc->AddOutputDesc("dy0", dynamic_output_desc0);
  GeTensorDesc dynamic_output_desc1(GeShape({32, 16}), FORMAT_ND, DT_INT32);
  op_desc->AddOutputDesc("dy1", dynamic_output_desc1);
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  auto creator = []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockCompileContextOutputOp>(); };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  g_compile_context_output_called.store(false);
  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
  EXPECT_TRUE(g_compile_context_output_called.load());
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerCompilesHostCpuExecCustomOp) {
  const std::string kTestOpType = "TestHostCpuCustomOp_CompileDeviceImpl";
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kDevice,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockCompilableCustomOp>(); }),
            GRAPH_SUCCESS);
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kHostCPU,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockHostCpuCustomOp>(); }),
            GRAPH_SUCCESS);

  auto graph = std::make_shared<ComputeGraph>("host_cpu_custom_compile_graph");
  auto op_desc = std::make_shared<OpDesc>("host_cpu_custom_node", kTestOpType);
  op_desc->SetOpEngineName(kEngineNameCustom);
  op_desc->SetOpKernelLibName(kCustomOpKernelLibName);
  ASSERT_TRUE(AttrUtils::SetStr(op_desc, kAttrLowingFunc, kHostCpuCustomOpLowerFunc));
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  MockCompilableCustomOp::ResetCompileCount();
  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
  EXPECT_EQ(MockCompilableCustomOp::GetCompileCount(), 1);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerCompilesHostOnlyCustomOp) {
  const std::string kTestOpType = "TestHostOnlyCompilableOp_CompileTest";
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kHostCPU,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockHostCpuCompilableCustomOp>(); }),
            GRAPH_SUCCESS);

  auto graph = std::make_shared<ComputeGraph>("host_only_custom_compile_graph");
  auto op_desc = std::make_shared<OpDesc>("host_only_custom_node", kTestOpType);
  op_desc->SetOpEngineName(kEngineNameCustom);
  op_desc->SetOpKernelLibName(kCustomOpKernelLibName);
  ASSERT_TRUE(AttrUtils::SetStr(op_desc, kAttrLowingFunc, kHostCpuCustomOpLowerFunc));
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  MockHostCpuCompilableCustomOp::ResetCompileCount();
  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
  EXPECT_EQ(MockHostCpuCompilableCustomOp::GetCompileCount(), 1);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerSkipsHostOnlyOpWithoutCompile) {
  const std::string kTestOpType = "TestHostOnlyBaseOp_SkipCompile";
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kHostCPU,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockHostCpuCustomOp>(); }),
            GRAPH_SUCCESS);

  auto graph = std::make_shared<ComputeGraph>("host_only_base_op_graph");
  auto op_desc = std::make_shared<OpDesc>("host_only_base_node", kTestOpType);
  op_desc->SetOpEngineName(kEngineNameCustom);
  op_desc->SetOpKernelLibName(kCustomOpKernelLibName);
  ASSERT_TRUE(AttrUtils::SetStr(op_desc, kAttrLowingFunc, kHostCpuCustomOpLowerFunc));
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerSkipsCompileWhenMultipleProviders) {
  const std::string kTestOpType = "TestDualCompilableOp_MultiProviderConflict";
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kDevice,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockCompilableCustomOp>(); }),
            GRAPH_SUCCESS);
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(
                AscendString(kTestOpType.c_str()), OpBackend::kHostCPU,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockHostCpuCompilableCustomOp>(); }),
            GRAPH_SUCCESS);

  auto graph = std::make_shared<ComputeGraph>("multi_provider_compile_graph");
  auto op_desc = std::make_shared<OpDesc>("multi_provider_node", kTestOpType);
  op_desc->SetOpEngineName(kEngineNameCustom);
  op_desc->SetOpKernelLibName(kCustomOpKernelLibName);
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  MockCompilableCustomOp::ResetCompileCount();
  MockHostCpuCompilableCustomOp::ResetCompileCount();
  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
  EXPECT_EQ(MockCompilableCustomOp::GetCompileCount(), 0);
  EXPECT_EQ(MockHostCpuCompilableCustomOp::GetCompileCount(), 0);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerInitializeAndFinalize) {
  CustomGraphOptimizer optimizer;
  std::map<std::string, std::string> options;
  EXPECT_EQ(optimizer.Initialize(options, nullptr), SUCCESS);
  EXPECT_EQ(optimizer.Finalize(), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerOptimizeOriginalGraph) {
  CustomGraphOptimizer optimizer;
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  EXPECT_EQ(optimizer.OptimizeOriginalGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerOptimizeFusedGraph) {
  CustomGraphOptimizer optimizer;
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  EXPECT_EQ(optimizer.OptimizeFusedGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerOptimizeWholeGraphEmptyGraph) {
  CustomGraphOptimizer optimizer;
  auto graph = std::make_shared<ComputeGraph>("empty_graph");
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerOptimizeWholeGraphNonCustomOp) {
  CustomGraphOptimizer optimizer;
  auto graph = std::make_shared<ComputeGraph>("non_custom_graph");
  auto op_desc = std::make_shared<OpDesc>("non_custom_node", "NonCustomType");
  GeTensorDesc input_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  GeTensorDesc output_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  (void)op_desc->AddInputDesc("x", input_desc);
  (void)op_desc->AddOutputDesc("y", output_desc);
  ASSERT_NE(graph->AddNode(op_desc), nullptr);
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerOptimizeWholeGraphBaseOnlyOp) {
  const std::string kTestOpType = "TestBaseOnlyOp_OptimizerTest";
  auto creator = []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<MockBaseOnlyCustomOp>(); };
  ASSERT_EQ(CustomOpFactory::RegisterCustomOpCreator(AscendString(kTestOpType.c_str()), creator), GRAPH_SUCCESS);

  auto graph = std::make_shared<ComputeGraph>("base_only_graph");
  auto op_desc = std::make_shared<OpDesc>("base_only_node", kTestOpType);
  GeTensorDesc input_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  GeTensorDesc output_desc(GeShape({1, 16}), FORMAT_ND, DT_FLOAT16);
  (void)op_desc->AddInputDesc("x", input_desc);
  (void)op_desc->AddOutputDesc("y", output_desc);
  ASSERT_NE(graph->AddNode(op_desc), nullptr);

  CustomGraphOptimizer optimizer;
  EXPECT_EQ(optimizer.OptimizeWholeGraph(*graph), SUCCESS);
}

TEST_F(UtestCustomOpsKernelInfoStore, CustomGraphOptimizerGetAttributes) {
  CustomGraphOptimizer optimizer;
  GraphOptimizerAttribute attrs;
  EXPECT_EQ(optimizer.GetAttributes(attrs), SUCCESS);
  EXPECT_EQ(attrs.engineName, "DNN_VM_CUSTOM");
}

}  // namespace
}  // namespace custom
}  // namespace ge
