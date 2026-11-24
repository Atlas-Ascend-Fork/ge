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

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "common/bg_test.h"
#include "common/share_graph.h"
#include "engine/custom/converter/custom_node_converter.h"
#include "engine/gelocal/inputs_converter.h"
#include "ge/ut/ge/runtime/fast_v2/common/const_data_helper.h"
#include "lowering/placement/placed_lowering_result.h"
#include "common/helper/custom_op_registry_builder.h"
#include "common/helper/custom_op_so_loader.h"
#include "faker/kernel_run_context_facker.h"
#include "framework/common/host_cpu_fusion_attr.h"
#include "graph/custom_op.h"
#include "graph/op_so_bin.h"
#include "graph/partition/optimizer/host_cpu_fusion_codegen.h"
#include "graph/partition/optimizer/host_cpu_fusion_pass.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "kernel/memory/host_mem_allocator.h"
#include "register/kernel_registry.h"

namespace ge {
namespace {

std::string GetToolkitHome() {
  const char *home = std::getenv("ASCEND_HOME_PATH");
  if ((home != nullptr) && (home[0] != '\0')) {
    return home;
  }
  const char *install_path = std::getenv("ASCEND_INSTALL_PATH");
  if ((install_path != nullptr) && (install_path[0] != '\0')) {
    return install_path;
  }
  return "/usr/local/Ascend/cann-9.2.0/x86_64-linux";
}

std::string GetToolkitOpp(const std::string &home) {
  constexpr char kX86Suffix[] = "/x86_64-linux";
  if (home.size() > (sizeof(kX86Suffix) - 1U) &&
      home.compare(home.size() - (sizeof(kX86Suffix) - 1U), sizeof(kX86Suffix) - 1U, kX86Suffix) == 0) {
    return home.substr(0U, home.size() - (sizeof(kX86Suffix) - 1U)) + "/opp";
  }
  return home + "/opp";
}

NodePtr MakeNode(const ComputeGraphPtr &graph, const std::string &name, const std::string &type, const size_t inputs,
                 const size_t outputs, const GeTensorDesc &desc = GeTensorDesc(GeShape({2, 3}), FORMAT_ND, DT_INT64)) {
  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (size_t i = 0U; i < inputs; ++i) {
    EXPECT_EQ(op_desc->AddInputDesc("input" + std::to_string(i), desc), GRAPH_SUCCESS);
  }
  for (size_t i = 0U; i < outputs; ++i) {
    EXPECT_EQ(op_desc->AddOutputDesc("output" + std::to_string(i), desc), GRAPH_SUCCESS);
  }
  return graph->AddNode(op_desc);
}

void Connect(const NodePtr &source, const size_t source_index, const NodePtr &target, const size_t target_index) {
  ASSERT_EQ(GraphUtils::AddEdge(source->GetOutDataAnchor(static_cast<int32_t>(source_index)),
                                target->GetInDataAnchor(static_cast<int32_t>(target_index))),
            GRAPH_SUCCESS);
}

HostCpuFusionRegion MakeRegion(const ComputeGraphPtr &graph) {
  const auto data = MakeNode(graph, "data", "Data", 0U, 1U);
  const auto first = MakeNode(graph, "first", "HostST", 1U, 2U);
  const auto second = MakeNode(graph, "second", "HostST", 1U, 1U);
  const auto side = MakeNode(graph, "side", "DeviceOp", 1U, 1U);
  const auto output = MakeNode(graph, "output", "NetOutput", 2U, 0U);
  Connect(data, 0U, first, 0U);
  Connect(first, 0U, second, 0U);
  Connect(first, 1U, side, 0U);
  Connect(second, 0U, output, 0U);
  Connect(side, 0U, output, 1U);

  HostCpuFusionRegion region;
  region.chain_id = "st_codegen_chain";
  region.nodes = {first, second};
  region.external_inputs = {data->GetOutDataAnchor(0)};
  region.external_outputs = {{first->GetOutDataAnchor(1), {side->GetInDataAnchor(0)}},
                             {second->GetOutDataAnchor(0), {output->GetInDataAnchor(0)}}};
  return region;
}

void MarkHostCpuCandidateForPassSt(const NodePtr &node) {
  node->GetOpDesc()->SetOpEngineName("DNN_VM_HOST_CPU");
  node->GetOpDesc()->SetOpKernelLibName("DNN_VM_HOST_CPU_OP_STORE");
  ASSERT_TRUE(AttrUtils::SetBool(node->GetOpDesc(), "SmallShapeHostcpu", true));
}

ComputeGraphPtr BuildPassGraph() {
  auto graph = std::make_shared<ComputeGraph>("host_cpu_fusion_pass_st");
  const auto data = MakeNode(graph, "pass_data", "Data", 0U, 1U);
  const auto first = MakeNode(graph, "pass_first", "HostPassA", 1U, 1U);
  const auto second = MakeNode(graph, "pass_second", "HostPassB", 1U, 1U);
  const auto third = MakeNode(graph, "pass_third", "HostPassC", 1U, 1U);
  const auto fourth = MakeNode(graph, "pass_fourth", "HostPassD", 1U, 1U);
  const auto fifth = MakeNode(graph, "pass_fifth", "HostPassE", 1U, 1U);
  const auto output = MakeNode(graph, "pass_output", "NetOutput", 1U, 0U);
  Connect(data, 0U, first, 0U);
  Connect(first, 0U, second, 0U);
  Connect(second, 0U, third, 0U);
  Connect(third, 0U, fourth, 0U);
  Connect(fourth, 0U, fifth, 0U);
  Connect(fifth, 0U, output, 0U);
  MarkHostCpuCandidateForPassSt(first);
  MarkHostCpuCandidateForPassSt(second);
  MarkHostCpuCandidateForPassSt(third);
  MarkHostCpuCandidateForPassSt(fourth);
  MarkHostCpuCandidateForPassSt(fifth);
  return graph;
}

class MinimalCustomOpCompilerForPassSt final : public HostCpuFusionCompiler {
 public:
  Status Compile(const std::string &, std::vector<uint8_t> &so_data) const override {
    return HostCpuFusionCompiler::Compile(
        "extern \"C\" __attribute__((visibility(\"default\"))) unsigned int "
        "GetRegisteredCustomOpCreatorAbiVersion() { return 2U; }\n"
        "extern \"C\" __attribute__((visibility(\"default\"))) unsigned long "
        "GetRegisteredCustomOpCreatorNum() { return 0U; }\n"
        "extern \"C\" __attribute__((visibility(\"default\"))) int "
        "GetRegisteredCustomOpCreators(void *, unsigned long, unsigned long) { return 0; }\n",
        so_data);
  }
};

bool SupportAllForPassSt(const std::string &) {
  return true;
}

class EnvGuard final {
 public:
  EnvGuard(const char *name, const char *value) : name_(name), old_(), had_old_(false) {
    const char *old = std::getenv(name);
    if (old != nullptr) {
      old_ = old;
      had_old_ = true;
    }
    EXPECT_EQ(setenv(name, value, 1), 0);
  }
  ~EnvGuard() {
    if (had_old_) {
      (void)setenv(name_.c_str(), old_.c_str(), 1);
    } else {
      (void)unsetenv(name_.c_str());
    }
  }

 private:
  std::string name_;
  std::string old_;
  bool had_old_;
};

}  // namespace

// 用例描述：验证 HostCPU 融合区域能够生成当前 CustomOp 编排源码并编译为 ELF SO。
// 预置条件：使用本机 Toolkit 头文件和 g++，构造带内部中间张量及两个外部输出的融合区域。
// 测试步骤：生成源码，检查输入/输出绑定与 CustomOp ABI，再执行 JIT 编译。
// 预期结果：源码生成成功；工具包提供 HostCPU ABI 时编译产物为有效 ELF 共享对象，否则返回 UNSUPPORTED。
TEST(HostCpuFusionCodegenST, GeneratesAndCompilesCurrentCustomOp) {
#if defined(__linux__)
  const auto toolkit_home = GetToolkitHome();
  EnvGuard home("ASCEND_HOME_PATH", toolkit_home.c_str());
  EnvGuard opp("ASCEND_OPP_PATH", "");
  auto graph = std::make_shared<ComputeGraph>("host_cpu_codegen_st");
  const auto region = MakeRegion(graph);
  HostCpuFusionCodegenResult result;
  ASSERT_EQ(HostCpuFusionCodegen().Generate(region, result), SUCCESS);
  EXPECT_EQ(result.register_name, "FusedHostCpu_st_codegen_chain");
  EXPECT_NE(result.source.find("internal_tensor_0"), std::string::npos);
  EXPECT_NE(result.source.find("std::array<TensorView, 1U>"), std::string::npos);
  EXPECT_NE(result.source.find("std::array<TensorView, 2U>"), std::string::npos);
  EXPECT_NE(result.source.find("REG_OP_BACKEND"), std::string::npos);
  EXPECT_NE(result.source.find("GetRegisteredCustomOpCreatorAbiVersion"), std::string::npos);

  std::vector<uint8_t> so_data;
  const auto compile_status = HostCpuFusionCompiler().Compile(result.source, so_data);
  if (compile_status == SUCCESS) {
    ASSERT_GT(so_data.size(), 20U);
    EXPECT_EQ(so_data[0], 0x7FU);
    EXPECT_EQ(so_data[1], 'E');
    EXPECT_EQ(so_data[2], 'L');
    EXPECT_EQ(so_data[3], 'F');
  } else {
    // Older Toolkit packages may not expose the HostCpuExecuteOp ABI yet.
    EXPECT_EQ(compile_status, UNSUPPORTED);
    EXPECT_TRUE(so_data.empty());
  }
#else
  GTEST_SKIP() << "HostCPU fusion JIT uses Linux memfd.";
#endif
}

// 用例描述：验证融合标量 Add/Mul 的生成产物可通过 RT2 Kernel 执行。
// 预置条件：Toolkit 支持 HostCPU CustomOp JIT。
// 测试步骤：生成并加载 SO，查找融合算子，分两轮执行并读取拆分后的输出。
// 预期结果：两个 Host 输出分别等于 Add 和 Mul 的计算结果。
TEST(HostCpuFusionCodegenST, ExecutesGeneratedScalarChainAndSplitsOutputs) {
#if defined(__linux__)
  const auto toolkit_home = GetToolkitHome();
  EnvGuard home("ASCEND_HOME_PATH", toolkit_home.c_str());
  EnvGuard opp("ASCEND_OPP_PATH", "");
  const auto graph = std::make_shared<ComputeGraph>("host_cpu_scalar_st");
  const GeTensorDesc scalar(GeShape({1}), FORMAT_ND, DT_INT64);
  const auto data = MakeNode(graph, "data", "Data", 0U, 1U, scalar);
  const auto add = MakeNode(graph, "add", "Add", 2U, 1U, scalar);
  const auto mul = MakeNode(graph, "mul", "Mul", 2U, 1U, scalar);
  const auto output = MakeNode(graph, "output", "NetOutput", 2U, 0U, scalar);
  Connect(data, 0U, add, 0U);
  Connect(data, 0U, add, 1U);
  Connect(add, 0U, mul, 0U);
  Connect(data, 0U, mul, 1U);
  Connect(add, 0U, output, 0U);
  Connect(mul, 0U, output, 1U);
  HostCpuFusionRegion region;
  region.chain_id = "st_scalar_chain";
  region.nodes = {add, mul};
  region.external_inputs = {data->GetOutDataAnchor(0)};
  region.external_outputs = {{add->GetOutDataAnchor(0), {output->GetInDataAnchor(0)}},
                             {mul->GetOutDataAnchor(0), {output->GetInDataAnchor(1)}}};
  HostCpuFusionCodegenResult result;
  ASSERT_EQ(HostCpuFusionCodegen().Generate(region, result), SUCCESS);
  std::vector<uint8_t> so_data;
  // Probe the Toolkit with a minimal source so a failure compiling the generated code still fails the test.
  HostCpuFusionCompiler compiler;
  const auto probe = compiler.Compile("extern \"C\" int HostCpuScalarStToolkitProbe() { return 0; }", so_data);
  if (probe == UNSUPPORTED) {
    GTEST_SKIP() << "HostCPU fusion JIT is unavailable in this Toolkit.";
  }
  ASSERT_EQ(probe, SUCCESS);
  ASSERT_EQ(compiler.Compile(result.source, so_data), SUCCESS);
  auto bin_data = std::make_unique<char_t[]>(so_data.size());
  std::copy(so_data.begin(), so_data.end(), bin_data.get());
  auto bin = std::make_shared<OpSoBin>("lib" + result.register_name + ".so", kFusedHostCpuSoVendor, std::move(bin_data),
                                       static_cast<uint32_t>(so_data.size()), SoBinType::kCustomOp);
  std::vector<CustomOpSoHandlePtr> handles;
  ASSERT_EQ(CustomOpSoLoader::GetInstance().DlopenCustomOpSoBins({bin}, handles), SUCCESS);
  auto registry = std::make_shared<CustomOpRegistry>();
  ASSERT_EQ(CustomOpRegistryBuilder::AddCreatorsFromSoHandles(handles, registry), SUCCESS);

  auto lookup = gert::BuildKernelRunContext(2U, 1U);
  lookup.value_holder[0].Set(const_cast<char *>(result.register_name.c_str()), nullptr);
  lookup.value_holder[1].Set(registry.get(), nullptr);
  const auto *find = gert::KernelRegistry::GetInstance().FindKernelFuncs("FindHostCpuFusedCustomOp");
  ASSERT_NE(find, nullptr);
  ASSERT_EQ(find->run_func(lookup), GRAPH_SUCCESS);
  auto *host_op = lookup.GetContext<gert::KernelContext>()->GetOutput(0U)->GetValue<HostCpuExecuteOp *>();
  ASSERT_NE(host_op, nullptr);

  int64_t value = 0;
  gert::Tensor input({{1}, {1}}, {FORMAT_ND, FORMAT_ND, {}}, gert::kOnHost, DT_INT64, &value);
  input.MutableTensorData().SetSize(sizeof(value));
  gert::memory::HostMemAllocator l1_allocator;
  gert::memory::HostGertMemAllocator allocator(&l1_allocator);
  auto run = gert::KernelRunContextFaker()
                 .KernelIONum(3U, 4U)
                 .NodeIoNum(1U, 2U)
                 .IrInstanceNum({1U})
                 .NodeType(result.register_name)
                 .NodeInputTd(0, DT_INT64, FORMAT_ND, FORMAT_ND)
                 .NodeOutputTd(0, DT_INT64, FORMAT_ND, FORMAT_ND)
                 .NodeOutputTd(1, DT_INT64, FORMAT_ND, FORMAT_ND)
                 .Inputs({&input, &allocator, host_op})
                 .Build();
  auto *context = run.GetContext<gert::KernelContext>();
  const auto *execute = gert::KernelRegistry::GetInstance().FindKernelFuncs("ExecuteHostCustomOpAndSplit");
  ASSERT_NE(execute, nullptr);
  ASSERT_EQ(execute->outputs_creator(nullptr, context), GRAPH_SUCCESS);
  for (const int64_t input_value : {3, 7}) {
    value = input_value;
    ASSERT_EQ(execute->run_func(context), GRAPH_SUCCESS);
    const int64_t expected[] = {2 * value, 2 * value * value};
    for (size_t i = 0U; i < 2U; ++i) {
      const auto *data_output = context->GetOutputPointer<gert::GertTensorData>(2U + i);
      ASSERT_NE(data_output, nullptr);
      ASSERT_NE(data_output->GetAddr(), nullptr);
      EXPECT_EQ(data_output->GetPlacement(), gert::kOnHost);
      EXPECT_EQ(data_output->GetSize(), sizeof(int64_t));
      EXPECT_EQ(*static_cast<const int64_t *>(data_output->GetAddr()), expected[i]);
    }
  }
#else
  GTEST_SKIP() << "HostCPU fusion JIT uses Linux memfd.";
#endif
}

// 用例描述：验证 GatherV2 标量分支及 batch_dims 回退生成可编译源码。
// 预置条件：三节点 Gather/Add/Mul 分支图。
// 测试步骤：生成源码，检查绑定与回退，然后使用 Toolkit 编译。
// 预期结果：输出提交完整且生成 SO 成功。
TEST(HostCpuFusionCodegenST, GeneratesScalarGatherAndBranchedArithmetic) {
  auto graph = std::make_shared<ComputeGraph>("scalar_branch");
  const GeTensorDesc scalar(GeShape(), FORMAT_ND, DT_INT64);
  const GeTensorDesc scalar_index(GeShape(), FORMAT_ND, DT_INT32);
  const GeTensorDesc vector(GeShape({3}), FORMAT_ND, DT_INT64);
  const auto params = MakeNode(graph, "params", "Data", 0U, 1U, vector);
  const auto indices = MakeNode(graph, "indices", "Data", 0U, 1U, scalar_index);
  const auto axis = MakeNode(graph, "axis", "Data", 0U, 1U, scalar);
  const auto operand = MakeNode(graph, "operand", "Data", 0U, 1U, scalar);
  const auto gather = MakeNode(graph, "gather", "GatherV2", 3U, 1U, scalar);
  const auto add = MakeNode(graph, "add", "Add", 2U, 1U, scalar);
  const auto mul = MakeNode(graph, "mul", "Mul", 2U, 1U, scalar);
  const auto output = MakeNode(graph, "output", "NetOutput", 2U, 0U, scalar);
  ASSERT_EQ(gather->GetOpDesc()->UpdateInputDesc(0U, vector), GRAPH_SUCCESS);
  ASSERT_EQ(gather->GetOpDesc()->UpdateInputDesc(1U, scalar_index), GRAPH_SUCCESS);
  ASSERT_TRUE(AttrUtils::SetInt(gather->GetOpDesc(), "batch_dims", 0));
  Connect(params, 0U, gather, 0U);
  Connect(indices, 0U, gather, 1U);
  Connect(axis, 0U, gather, 2U);
  Connect(gather, 0U, add, 0U);
  Connect(operand, 0U, add, 1U);
  Connect(gather, 0U, mul, 0U);
  Connect(operand, 0U, mul, 1U);
  Connect(add, 0U, output, 0U);
  Connect(mul, 0U, output, 1U);

  HostCpuFusionRegion region;
  region.chain_id = "scalar_branch";
  region.nodes = {gather, add, mul};
  region.external_inputs = {params->GetOutDataAnchor(0), indices->GetOutDataAnchor(0), axis->GetOutDataAnchor(0),
                            operand->GetOutDataAnchor(0)};
  region.external_outputs = {{add->GetOutDataAnchor(0), {output->GetInDataAnchor(0)}},
                             {mul->GetOutDataAnchor(0), {output->GetInDataAnchor(1)}}};
  HostCpuFusionCodegenResult result;
  ASSERT_EQ(HostCpuFusionCodegen().Generate(region, result), SUCCESS);
  EXPECT_NE(result.source.find("TryScalarGather<int64_t, int32_t, int64_t>(TensorView(external_input_0)"),
            std::string::npos);
  EXPECT_NE(result.source.find("__builtin_add_overflow(scalar_value_0, scalar_input_3, &scalar_value_1)"),
            std::string::npos);
  EXPECT_NE(result.source.find("__builtin_mul_overflow(scalar_value_0, scalar_input_3, &scalar_value_2)"),
            std::string::npos);
  EXPECT_NE(result.source.find("std::memcpy(external_output_0->GetAddr(), &scalar_value_1"), std::string::npos);
  EXPECT_NE(result.source.find("std::memcpy(external_output_1->GetAddr(), &scalar_value_2"), std::string::npos);

  const auto toolkit_home = GetToolkitHome();
  EnvGuard home("ASCEND_HOME_PATH", toolkit_home.c_str());
  EnvGuard opp("ASCEND_OPP_PATH", "");
  std::vector<uint8_t> so_data;
  HostCpuFusionCompiler compiler;
  if (compiler.Compile("extern \"C\" int FusionCoverageProbe() { return 0; }", so_data) == SUCCESS) {
    ASSERT_EQ(compiler.Compile(result.source, so_data), SUCCESS);
    const auto first_so = so_data;
    ASSERT_EQ(compiler.Compile(result.source, so_data), SUCCESS);
    EXPECT_EQ(so_data, first_so);
  }
  ASSERT_TRUE(AttrUtils::SetInt(gather->GetOpDesc(), "batch_dims", 1));
  ASSERT_EQ(HostCpuFusionCodegen().Generate(region, result), SUCCESS);
  EXPECT_EQ(result.source.find("TryScalarGather<int64_t, int32_t, int64_t>"), std::string::npos);
  EXPECT_NE(result.source.find("LocalKernelContext<3U, 1U> kernel_context_0"), std::string::npos);
}

// 用例描述：验证大尺寸内部张量使用堆缓冲。
// 预置条件：中间张量超过 64 KiB。
// 测试步骤：生成融合源码并检查内部缓冲分配及大小。
// 预期结果：选择堆缓冲并为内部张量保留足够空间。
TEST(HostCpuFusionCodegenST, SelectsHeapStorageForLargeInternalTensor) {
  auto graph = std::make_shared<ComputeGraph>("large_internal_tensor");
  const auto region = MakeRegion(graph);
  const GeTensorDesc large_desc(GeShape({8193}), FORMAT_ND, DT_INT64);
  ASSERT_EQ(region.nodes[0]->GetOpDesc()->UpdateOutputDesc(0U, large_desc), GRAPH_SUCCESS);
  ASSERT_EQ(region.nodes[1]->GetOpDesc()->UpdateInputDesc(0U, large_desc), GRAPH_SUCCESS);

  HostCpuFusionCodegenResult result;
  ASSERT_EQ(HostCpuFusionCodegen().Generate(region, result), SUCCESS);
  EXPECT_NE(result.source.find("std::unique_ptr<unsigned char[]> internal_storage("), std::string::npos);
  EXPECT_NE(result.source.find("internal_storage_data + 0U, 65544U"), std::string::npos);
}

TEST(HostCpuFusionCodegenST, ExercisesCodegenRejectionAndNameBoundaries) {
  auto graph = std::make_shared<ComputeGraph>("host_cpu_codegen_rejection");
  const auto valid = MakeRegion(graph);
  HostCpuFusionCodegen codegen;
  HostCpuFusionCodegenResult result;

  auto invalid = valid;
  invalid.chain_id.clear();
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.chain_id = "9bad";
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.chain_id = "has-dash";
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.chain_id.assign(160U, 'a');
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.external_outputs.clear();
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.nodes[0] = nullptr;
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.nodes[1] = invalid.nodes[0];
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.external_inputs[0] = nullptr;
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.external_inputs.push_back(invalid.external_inputs[0]);
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.external_outputs[0].source = nullptr;
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);
  invalid = valid;
  invalid.external_outputs.push_back(invalid.external_outputs[0]);
  EXPECT_EQ(codegen.Generate(invalid, result), PARAM_INVALID);

  invalid = valid;
  invalid.nodes[1]->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({-1}));
  EXPECT_EQ(codegen.Generate(invalid, result), UNSUPPORTED);
  invalid = valid;
  invalid.nodes[0]->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({-1}));
  EXPECT_EQ(codegen.Generate(invalid, result), UNSUPPORTED);
  invalid = valid;
  ASSERT_EQ(GraphUtils::RemoveEdge(invalid.nodes[0]->GetInDataAnchor(0)->GetPeerOutAnchor(),
                                   invalid.nodes[0]->GetInDataAnchor(0)),
            GRAPH_SUCCESS);
  EXPECT_EQ(codegen.Generate(invalid, result), UNSUPPORTED);

  EXPECT_EQ(GetHostCpuFusionInputName(nullptr, 1U), "input_1_tensor");
  const auto data = graph->FindNode("data");
  ASSERT_NE(data, nullptr);
  data->GetOpDesc()->MutableAllOutputName().clear();
  data->GetOpDesc()->MutableAllOutputName().emplace("with-dash", 0U);
  EXPECT_EQ(GetHostCpuFusionInputName(data->GetOutDataAnchor(0), 2U), "input_2_with_dash");
}

TEST(HostCpuFusionCodegenST, CompilerHandlesMissingToolkitAndDiagnostics) {
#if defined(__linux__)
  EnvGuard home("ASCEND_HOME_PATH", "");
  EnvGuard opp("ASCEND_OPP_PATH", "");
  HostCpuFusionCompiler compiler;
  std::vector<uint8_t> so_data;
  EXPECT_EQ(compiler.Compile("int value = 0;", so_data), UNSUPPORTED);
  EXPECT_TRUE(so_data.empty());

  const auto toolkit_home = GetToolkitHome();
  EnvGuard valid_home("ASCEND_HOME_PATH", toolkit_home.c_str());
  EXPECT_EQ(compiler.Compile("this is invalid C++;", so_data), UNSUPPORTED);
  EXPECT_TRUE(so_data.empty());
  {
    EnvGuard no_home("ASCEND_HOME_PATH", "");
    const auto toolkit_opp = GetToolkitOpp(toolkit_home);
    EnvGuard opp_only("ASCEND_OPP_PATH", toolkit_opp.c_str());
    EXPECT_EQ(compiler.Compile("", so_data), UNSUPPORTED);
  }
#else
  GTEST_SKIP() << "HostCPU fusion JIT uses Linux memfd.";
#endif
}

/**
 * 用例描述：验证 HostCPU Fusion Pass 能提交包含五个候选节点的融合链。
 * 预置条件：使用 MinimalCustomOpCompilerForPassSt 生成可加载的最小自定义算子 SO。
 * 测试步骤：构造五节点 HostCPU 候选链，执行 HostCpuFusionPass。
 * 预期结果：五个原始候选节点被一个 FusedHostCpu 节点替换，原子和复合引擎映射各包含一个条目。
 */
TEST(HostCpuFusionCodegenST, CommitsHostCpuFusionPassWithEmbeddedCustomOpSo) {
#if defined(__linux__)
#if defined(__aarch64__) || defined(__arm64__)
  GTEST_SKIP() << "HostCPU fusion pass SO validation is x86_64-only on this test path.";
#else
  const auto toolkit_home = GetToolkitHome();
  EnvGuard home("ASCEND_HOME_PATH", toolkit_home.c_str());
  EnvGuard opp("ASCEND_OPP_PATH", "");
  const auto graph = BuildPassGraph();
  NodeEngineMap atomic_map;
  NodeEngineMap composite_map;
  HostCpuFusionPass pass(std::make_shared<MinimalCustomOpCompilerForPassSt>(), SupportAllForPassSt);
  ASSERT_EQ(pass.Run(graph, atomic_map, composite_map), SUCCESS);
  EXPECT_EQ(graph->FindNode("pass_first"), nullptr);
  EXPECT_EQ(graph->FindNode("pass_second"), nullptr);
  EXPECT_EQ(graph->FindNode("pass_third"), nullptr);
  EXPECT_EQ(graph->FindNode("pass_fourth"), nullptr);
  EXPECT_EQ(graph->FindNode("pass_fifth"), nullptr);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(atomic_map.size(), 1U);
  EXPECT_EQ(composite_map.size(), 1U);
#endif
#else
  GTEST_SKIP() << "HostCPU fusion JIT uses Linux memfd.";
#endif
}

}  // namespace ge

namespace gert {
using namespace ge;
using namespace bg;
namespace {
void LowerDataNodes(const ge::ComputeGraphPtr &graph, const LoweringGlobalData &global_data,
                    std::vector<bg::ValueHolderPtr> &shapes, std::vector<bg::DevMemValueHolderPtr> &addrs) {
  LowerInput data_input = {{}, {}, &const_cast<LoweringGlobalData &>(global_data)};
  for (const auto &name : {"data0", "data1", "data2"}) {
    auto data_ret = LoweringDataNode(graph->FindNode(name), data_input);
    ASSERT_TRUE(data_ret.result.IsSuccess());
    shapes.emplace_back(data_ret.out_shapes[0]);
    addrs.emplace_back(data_ret.out_addrs[0]);
    graph->FindNode(name)->GetOpDesc()->SetExtAttr(
        "_lowering_result", gert::PlacedLoweringResult(graph->FindNode(name), std::move(data_ret)));
  }
}

}  // namespace
class HostCpuFusionConverterST : public bg::BgTestAutoCreate3StageFrame {};

// 用例描述：验证生成融合节点经 Host converter 生成合并执行节点。
// 预置条件：融合标记已设置，输入节点完成 lowering。
// 测试步骤：LoweringHostCustomNode 并检查 shape、地址及释放依赖。
// 预期结果：Host 输出由 ExecuteHostCustomOpAndSplit 提供且带释放 guard。
TEST_F(HostCpuFusionConverterST, fused_host_cpu_custom_op_converts_to_combined_execution) {
  auto graph = ShareGraph::BuildCustomOpGraph();
  auto custom_op = graph->FindNode("custom_op");
  ASSERT_TRUE(AttrUtils::SetBool(custom_op->GetOpDesc(), ge::kFusedHostCpuGenerated, true));
  auto root_model = GeModelBuilder(graph).BuildGeRootModel();
  auto global_data = GlobalDataFaker(root_model).Build();
  global_data.SetExternalAllocator(nullptr, ExecuteGraphType::kInit);
  global_data.SetExternalAllocator(nullptr, ExecuteGraphType::kMain);
  bg::LowerConstDataNode(global_data);
  std::vector<bg::ValueHolderPtr> shapes;
  std::vector<bg::DevMemValueHolderPtr> addrs;
  LowerDataNodes(graph, global_data, shapes, addrs);

  const auto ret = LoweringHostCustomNode(custom_op, {shapes, addrs, &global_data});
  ASSERT_TRUE(ret.result.IsSuccess());
  ASSERT_EQ(ret.out_shapes.size(), 1U);
  ASSERT_EQ(ret.out_addrs.size(), 1U);
  EXPECT_EQ(ret.out_addrs[0]->GetPlacement(), kOnHost);
  EXPECT_EQ(ret.out_shapes[0]->GetOutIndex(), 0);
  EXPECT_EQ(ret.out_addrs[0]->GetOutIndex(), 1);
  ASSERT_NE(ret.out_addrs[0]->GetGuarder(), nullptr);

  auto frame = bg::ValueHolder::PopGraphFrame();
  ASSERT_NE(frame, nullptr);
  auto *exe_graph = frame->GetExecuteGraph().get();
  ASSERT_NE(exe_graph, nullptr);
  auto *execute_node = ExecuteGraphUtils::FindFirstNodeMatchType(exe_graph, "ExecuteHostCustomOpAndSplit");
  ASSERT_NE(execute_node, nullptr);
  EXPECT_EQ(ret.out_shapes[0]->GetFastNode(), execute_node);
  EXPECT_EQ(ret.out_addrs[0]->GetFastNode(), execute_node);
  EXPECT_NE(ExecuteGraphUtils::FindFirstNodeMatchType(init_frame_->GetExecuteGraph().get(), "FindHostCpuFusedCustomOp"),
            nullptr);
}

}  // namespace gert
