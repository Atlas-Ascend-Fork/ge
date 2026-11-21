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
#include "securec.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "core/executor/multi_thread_topological/executor/schedule/producer/producers/kernel_tags/critical_section_config.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/common/helper/om2_package_helper.h"
#include "common/helper/om2/om2_utils.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "register/kernel_registry.h"
#include "stub/gert_runtime_stub.h"
#include "../../common/om2_generated_model_runner.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

#include "../../common/om2_dynamic_host_graph.h"

namespace gert {
namespace {
using om2_test::AddNode;
using om2_test::AffineGraph;

using Om2DynamicGraphGeneratorST = ge::om2_test::DynamicPackageTest;

// Two generated pass-through kernels expose a single-use intermediate chain to
// the real serial/worker fusion pass. The final affine checks both pointer flow and values.
class TensorForwardCollector final : public om2::KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const om2::KernelCollectContext &context, om2::KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != 1U || context.output_chain_indices.size() > 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    input_ = context.input_chain_indices[0];
    output_ = context.output_chain_indices.empty() ? om2::kInvalidProgramIndex : context.output_chain_indices[0];
    auto status = om2::InitKernelProgramInfo(context, om2::OutputCreatePolicy::kGraphBound, false, info);
    if (status != ge::GRAPH_SUCCESS) {
      return status;
    }
    status = om2::SetInputPortInfos(context, {{"tensor", om2::Om2ValueType::kGertTensorPtr}}, info);
    if (status != ge::GRAPH_SUCCESS) {
      return status;
    }
    if (context.output_chain_indices.empty()) {
      return ge::GRAPH_SUCCESS;
    }
    return om2::SetOutputPortInfos(context, {{"tensor", om2::Om2ValueType::kGertTensorPtr}}, info);
  }
  om2::KernelImplType GetImplType() const override {
    return om2::KernelImplType::kCodegenImpl;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol + "() {\n" + "    auto *tensor = chains_[" + std::to_string(input_) +
           "U].GetValue<gert::Tensor *>();\n" +
           "    if (tensor == nullptr || tensor->GetAddr() == nullptr) { return kFailed; }\n";
    if (output_ != om2::kInvalidProgramIndex) {
      code += "    chains_[" + std::to_string(output_) + "U].Set(tensor, nullptr);\n";
    }
    code += "    return kOk;\n  }\n";
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint32_t input_ = 0U;
  uint32_t output_ = 0U;
};
REGISTER_KERNEL_CODE_BUILDER("Om2StTensorForward", TensorForwardCollector);
REGISTER_KERNEL_CODE_BUILDER("Om2StTensorCheck", TensorForwardCollector);
// Chain pointer writes are memory work; keep the two forwards on the same worker.
REGISTER_KERNEL(Om2StTensorForward).ConcurrentCriticalSectionKey(kKernelUseMemory);

void InsertFusableTensorForwards(ge::ExecuteGraph &graph) {
  auto *input = ge::ExecuteGraphUtils::FindNodeFromAllNodes(&graph, "input");
  auto *affine = ge::ExecuteGraphUtils::FindNodeFromAllNodes(&graph, "affine");
  ASSERT_NE(input, nullptr);
  ASSERT_NE(affine, nullptr);
  const auto desc = input->GetOpDescBarePtr()->GetOutputDesc(0);
  auto *first = AddNode(graph, "forward_first", "Om2StTensorForward", {desc}, {desc});
  auto *second = AddNode(graph, "forward_second", "Om2StTensorForward", {desc}, {desc});
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  // A real address check is the second consumer of the final forwarded tensor.
  // Its shared chain stops serial fusion before the NORMAL affine kernel.
  auto *check = AddNode(graph, "forward_check", "Om2StTensorCheck", {desc}, {});
  ASSERT_NE(check, nullptr);
  ASSERT_NE(graph.AddEdge(second, 0, check, 0), nullptr);
  ASSERT_EQ(graph.RemoveEdge(affine->GetInDataEdgeByIndex(0)), ge::GRAPH_SUCCESS);
  ASSERT_NE(graph.AddEdge(input, 0, first, 0), nullptr);
  ASSERT_NE(graph.AddEdge(first, 0, second, 0), nullptr);
  ASSERT_NE(graph.AddEdge(second, 0, affine, 0), nullptr);
}

bool HasGeneratedText(const GertModelData &data, const std::string &needle) {
  for (const auto &artifact : data.models[0]->runtime->source_artifacts) {
    if (artifact.data != nullptr && artifact.data_size != 0U) {
      const std::string text(reinterpret_cast<const char *>(artifact.data.get()), artifact.data_size);
      if (text.find(needle) != std::string::npos) {
        return true;
      }
    }
  }
  return false;
}

void CheckRepeatedRuns(ge::om2_test::GeneratedModelRunner &runner, const bool load_bound_parameters,
                       std::vector<std::vector<float>> *collected = nullptr) {
  const std::vector<std::vector<float>> values{{1.0F, -2.0F, 0.5F}, {4.0F, 0.0F, -1.0F, 3.5F, -7.0F}};
  for (const auto &value : values) {
    std::vector<float> input_values = value;
    std::vector<float> output_values(value.size(), 999.0F);
    std::vector<float> coefficients{2.0F, -3.0F};
    const int64_t count = static_cast<int64_t>(value.size());
    Tensor input({{count}, {count}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, input_values.data());
    Tensor parameters({{2}, {2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, coefficients.data());
    Tensor output({{0}, {0}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT32, output_values.data());
    output.SetSize(output_values.size() * sizeof(float));
    Tensor *inputs[] = {&input, &parameters};
    Tensor *outputs[] = {&output};
    GertModelRunConfig config;
    config.input_count = load_bound_parameters ? 1U : 2U;
    config.input_data = inputs;
    config.output_count = 1U;
    config.output_data = outputs;
    ASSERT_EQ(runner.Run(config), ACL_SUCCESS);
    EXPECT_EQ(output.GetDataType(), ge::DT_FLOAT);
    ASSERT_EQ(output.GetOriginShape().GetDimNum(), 1U);
    EXPECT_EQ(output.GetOriginShape().GetDim(0U), count);
    EXPECT_EQ(output.GetStorageShape().GetDim(0U), count);
    EXPECT_EQ(output.GetSize(), output_values.size() * sizeof(float));
    for (size_t i = 0U; i < value.size(); ++i) {
      const float expected = value[i] * 2.0F - 3.0F;
      EXPECT_FLOAT_EQ(output_values[i], expected) << "element " << i;
    }
    if (collected != nullptr) {
      collected->push_back(output_values);
    }
  }
}

void CheckControlledRunFailure(ge::om2_test::GeneratedModelRunner &runner) {
  std::vector<float> input_values{1.0F, 2.0F, 3.0F};
  std::vector<float> coefficients{-9999.0F, -3.0F};
  std::vector<float> output_values(3U, 999.0F);
  Tensor input({{3}, {3}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, input_values.data());
  Tensor parameters({{2}, {2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, coefficients.data());
  Tensor output({{3}, {3}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, output_values.data());
  Tensor *inputs[] = {&input, &parameters};
  Tensor *outputs[] = {&output};
  GertModelRunConfig config;
  config.input_count = 2U;
  config.input_data = inputs;
  config.output_count = 1U;
  config.output_data = outputs;
  EXPECT_NE(runner.Run(config), ACL_SUCCESS);
  EXPECT_FLOAT_EQ(output_values[0U], -1234.0F);  // The failure came from the executed Host Kernel.
  EXPECT_FLOAT_EQ(output_values[1U], 999.0F);
  EXPECT_FLOAT_EQ(output_values[2U], 999.0F);
}

void CheckGeneratedModel(const GertModelData &built, const bool expect_fusion, const bool expect_workers) {
  ASSERT_EQ(built.models.size(), 1U);
  ASSERT_NE(built.models[0], nullptr);
  EXPECT_EQ(GetGertModelExecutionType(*built.models[0]), GertModelExecutionType::kDynamic);
  EXPECT_EQ(built.models[0]->debug, nullptr);
  ASSERT_NE(built.models[0]->runtime, nullptr);
  ASSERT_GT(built.models[0]->runtime->so_artifact.data_size, 0U);
  if (expect_fusion) {
    EXPECT_TRUE(HasGeneratedText(built, expect_workers ? "main_fused_w" : "main_fused_r"));
    EXPECT_TRUE(HasGeneratedText(built, "fused_chain_"));
  }
  if (expect_workers) {
    // A requested configuration must not silently exercise the serial fallback.
    ASSERT_TRUE(HasGeneratedText(built, "aclError RunMainWorkers()"));
  }
}

void RunRestoredModel(const GertModelData &restored, const bool load_bound_parameters, const bool fail_run,
                      std::vector<std::vector<float>> *collected) {
  std::vector<float> weights{1234.0F, 2.0F, -3.0F, -5678.0F};
  void *constants[]{weights.data()};
  if (load_bound_parameters) {
    ASSERT_NE(restored.constants, nullptr);
    ASSERT_EQ(restored.constants->constants_data.size(), 1U);
    EXPECT_EQ(restored.constants->constants_data.front(), nullptr);
    ASSERT_NE(restored.models[0]->constants_config, nullptr);
    EXPECT_EQ(restored.models[0]->constants_config->internal_weight_size, 0U);
  }
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0]->runtime->so_artifact)) << runner.Error();
  GertModelLoadConfig load;
  load.constants = load_bound_parameters ? constants : nullptr;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckRepeatedRuns(runner, load_bound_parameters, collected));
  if (fail_run) {
    ASSERT_NO_FATAL_FAILURE(CheckControlledRunFailure(runner));
    ASSERT_NO_FATAL_FAILURE(CheckRepeatedRuns(runner, load_bound_parameters));
  }
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

void GenerateRoundTripAndRun(const bool load_bound_parameters, const bool fail_run = false,
                             std::vector<std::vector<float>> *collected = nullptr, const bool expect_workers = false) {
  auto graph = AffineGraph(load_bound_parameters, fail_run);
  ASSERT_NE(graph, nullptr);
  if (collected != nullptr) {
    ASSERT_NO_FATAL_FAILURE(InsertFusableTensorForwards(*graph));
  }
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertModelData built;
  ASSERT_EQ(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, built), ge::SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckGeneratedModel(built, collected != nullptr, expect_workers));
  EXPECT_EQ(graph->GetAllSubgraphs().size(), 3U);
  ge::ModelBufferData archive;
  ASSERT_EQ(SerializeGertModelData(built, archive, false), ge::SUCCESS);
  ASSERT_NE(archive.data.get(), nullptr);
  ASSERT_GT(archive.length, 0U);
  GertModelData restored;
  ASSERT_EQ(DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            ge::SUCCESS);
  ASSERT_EQ(restored.models.size(), 1U);
  ASSERT_NE(restored.models[0], nullptr);
  EXPECT_EQ(GetGertModelExecutionType(*restored.models[0]), GertModelExecutionType::kStatic);
  ASSERT_NE(restored.models[0]->runtime, nullptr);
  ASSERT_NO_FATAL_FAILURE(RunRestoredModel(restored, load_bound_parameters, fail_run, collected));
}

/**
 * 用例描述：SE-02 加载期绑定的 Host 权重切片跨阶段参与多次动态 shape 计算。
 * 预置条件：调用方提供的 Host 权重含四个 float，Init Const 只引用中间两个作为 scale/bias，
 *           首尾故意放置不同数值；仅运算 Kernel 为测试替身，整条生成/打包流水线真实。
 * 测试步骤：外部构图、阶段拆分、生成编译、归档往返；在 Load 时绑定调用方权重，随后
 *           连续运行长度 3/5 的两组输入，再 Unload。
 * 预期结果：实际结果按权重切片中的 2/-3 计算，shape/type 与本轮输入一致，
 *           未消费首尾权重，也没有复用上一轮输出或重新编译模型。
 */
TEST_F(Om2DynamicGraphGeneratorST, LoadBoundHostWeightSliceIsConsumedAcrossRepeatedDynamicShapes) {
  // slow_test_limit = 3000
  // End-to-end execution includes compiling generated model SOs.
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GenerateRoundTripAndRun(true);
}

/**
 * 用例描述：SE-01/M-02 外部执行图归档往返、动态 shape 执行及单/多线程结果对照。
 * 预置条件：两个内存类生成式转发 Kernel 串联，末端由 NORMAL affine 和地址检查共同消费；
 *           单消费者中间链用于融合，动态 shape 范围 [1,8]。
 * 测试步骤：分别以 MAX_RUNTIME_CORE_NUMBER=1/4 真实生成、归档并加载模型；各自只 Load
 *           一次，连续 Run 长度 3/5 的相同两组输入，然后 Unload；作用域结束恢复环境变量。
 * 预期结果：两种配置逐元素满足独立 CPU 参考，输出 dtype、origin/storage shape 正确；
 *           源码含串行/worker 融合函数及局部链；输出哨兵被覆盖，两个模型结果一致。
 *           执行此过滤集时沿用 timeout 120 的进程级超时，不声明解决一般 RT2 活性问题。
 */
TEST_F(Om2DynamicGraphGeneratorST, SingleAndMultiThreadModelsMatchCpuReferenceAcrossShapes) {
  // slow_test_limit = 5000
  // End-to-end execution includes compiling generated model SOs.
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  std::vector<std::vector<float>> serial;
  std::vector<std::vector<float>> parallel;
  {
    ge::om2_test::ScopedEnvVar workers("MAX_RUNTIME_CORE_NUMBER", "1");
    GenerateRoundTripAndRun(false, false, &serial);
  }
  {
    ge::om2_test::ScopedEnvVar workers("MAX_RUNTIME_CORE_NUMBER", "4");
    GenerateRoundTripAndRun(false, false, &parallel, true);
  }
  ASSERT_EQ(serial.size(), 2U);
  ASSERT_EQ(parallel.size(), serial.size());
  for (size_t run = 0U; run < serial.size(); ++run) {
    ASSERT_EQ(parallel[run].size(), serial[run].size());
    for (size_t element = 0U; element < serial[run].size(); ++element) {
      EXPECT_FLOAT_EQ(parallel[run][element], serial[run][element]);
    }
  }
}

/**
 * 用例描述：M-02 多线程模型实际 Kernel 失败传播及之后卸载。
 * 预置条件：专属测试 Kernel 仅在输入系数为 -9999 时写入执行标记并返回失败；
 *           其他输入复用正常 affine 运算；不替代生成器和生命周期 C ABI。
 * 测试步骤：以四线程配置真实生成、归档、Load，先 Run 两组正常动态输入，再用故障
 *           系数 Run，检查错误及 Kernel 执行标记，最后 Unload 并恢复环境变量。
 * 预期结果：正常轮次与 CPU 一致；故障不返回成功且不继续写剩余输出，Unload 成功。
 *           后续正式执行使用 timeout 120；当前用例不扩大已有调度支持范围。
 */
TEST_F(Om2DynamicGraphGeneratorST, MultiThreadKernelFailurePropagatesAndModelUnloads) {
  // slow_test_limit = 3000
  // End-to-end execution includes compiling generated model SOs.
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  ge::om2_test::ScopedEnvVar workers("MAX_RUNTIME_CORE_NUMBER", "4");
  GenerateRoundTripAndRun(false, true, nullptr, true);
}
// F-01 route rejection boundaries: preserve the real external preparation and
// collector, and require the precise semantic rejection rather than any failure.
// `const_input_count` supplies real Const producers for builders that restore a
// serialized const input before they can reach their own rejection check.
void ExpectUnsupportedControlNode(const char *type, const char *message, const size_t const_input_count = 0U) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  GertRuntimeStub runtime_stub;
  auto graph = AffineGraph(false);
  ASSERT_NE(graph, nullptr);
  const ge::GeTensorDesc scalar_desc(ge::GeShape({1}), ge::FORMAT_ND, ge::DT_INT32);
  const std::vector<ge::GeTensorDesc> node_inputs(const_input_count, scalar_desc);
  auto *unsupported = AddNode(*graph, "unsupported_control", type, node_inputs, {});
  ASSERT_NE(unsupported, nullptr);
  const int32_t placeholder = 0;
  for (size_t index = 0U; index < const_input_count; ++index) {
    auto *producer =
        AddNode(*graph, ("unsupported_const_" + std::to_string(index)).c_str(), "Const", {}, {scalar_desc});
    ASSERT_NE(producer, nullptr);
    ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(
        producer->GetOpDescBarePtr(), kConstValue,
        ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&placeholder), sizeof(placeholder))));
    ASSERT_NE(graph->AddEdge(producer, 0, unsupported, static_cast<int32_t>(index)), nullptr);
  }
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertModelData output;
  output.manifest = std::make_unique<GertModelDataManifest>();
  auto *const original = output.manifest.get();
  output.manifest->model_num = 17U;
  runtime_stub.GetSlogStub().Clear();
  EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, output), ge::SUCCESS);
  EXPECT_NE(runtime_stub.GetSlogStub().FindLog(DLOG_ERROR, message), -1);
  EXPECT_EQ(output.manifest.get(), original);
  EXPECT_EQ(output.manifest->model_num, 17U);
  EXPECT_TRUE(output.models.empty());
}

TEST_F(Om2DynamicGraphGeneratorST, UnsupportedWhileIsRejectedWithoutPublishingModel) {
  ExpectUnsupportedControlNode("While", "contains unsupported While node unsupported_control");
}

TEST_F(Om2DynamicGraphGeneratorST, OfflineKnownSubgraphIsRejectedWithoutPublishingModel) {
  const auto graph_options = ge::GetThreadLocalContext().GetAllGraphOptions();
  ge::GetThreadLocalContext().SetGraphOption({{"ge.modelFormat", std::to_string(ge::MODEL_FORMAT_OM)}});
  // DavinciModelCreate restores input 0 as a process-local GeModel pointer, so the route has
  // to give it a real Const producer; a node without that input is rejected for a missing
  // const input before the offline-mode check under test is reached.
  ExpectUnsupportedControlNode("DavinciModelCreate",
                               "OM2 DavinciModelCreate unsupported_control is only supported in online mode", 1U);
  ge::GetThreadLocalContext().SetGraphOption(graph_options);
}

}  // namespace
}  // namespace gert
