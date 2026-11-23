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
#include <numeric>
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/storage_shape.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace gert {
namespace om2 {
namespace {
class Om2KernelContractUT : public testing::Test {
 protected:
  ge::FastNode *Node(const char *type) {
    auto *node = graph_.AddNode(std::make_shared<ge::OpDesc>(type, type));
    if (node != nullptr) {
      ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), kComputeNodeIndex, 0);
    }
    return node;
  }
  ge::ExecuteGraph graph_{"kernel_contracts"};
};

TEST_F(Om2KernelContractUT, SymbolInferShapeUsesTensorInputsInsteadOfOrdinaryShapeInputs) {
  auto *node = Node("InferShape");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("InferShape");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{11, 17, 23, 29, 31}, outputs{37};
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo ordinary, symbol;
  ASSERT_EQ(builder->Collect(context, ordinary), ge::GRAPH_SUCCESS);
  ASSERT_EQ(ordinary.inputs.size(), inputs.size());
  EXPECT_EQ(ordinary.inputs[0].value_type, Om2ValueType::kGertStorageShapePtr);
  ASSERT_TRUE(ge::AttrUtils::SetBool(node->GetOpDescBarePtr(), kSymbolInferShape, true));
  ASSERT_EQ(builder->Collect(context, symbol), ge::GRAPH_SUCCESS);
  ASSERT_EQ(symbol.inputs.size(), inputs.size());
  EXPECT_EQ(symbol.inputs[0].value_type, Om2ValueType::kSizeT);
  EXPECT_EQ(symbol.inputs[1].value_type, Om2ValueType::kGertTensorPtr);
  EXPECT_EQ(symbol.inputs[1].chain_index, 17U);
  EXPECT_EQ(symbol.inputs.back().value_type, Om2ValueType::kKernelFuncPtr);
  ASSERT_EQ(symbol.outputs.size(), 1U);
  EXPECT_EQ(symbol.outputs[0].chain_index, 37U);
  const std::vector<uint32_t> too_short{1, 2, 3};
  KernelCollectContext malformed(*node, too_short, outputs);
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(malformed, invalid), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelContractUT, RequestsResourcesAtTheDeclaredPortAndPhase) {
  struct ResourcePort {
    const char *kernel;
    size_t inputs;
    size_t outputs;
    size_t port;
    ChainBindingKind resource;
    ChainBindingPhase phase;
  };
  const ResourcePort cases[]{
      {"SelectL1Allocator", 4, 1, 1, ChainBindingKind::kExternalAllocator, ChainBindingPhase::kLoad},
      {"GetOrCreateWeightMem", 3, 1, 1, ChainBindingKind::kOuterWeightMem, ChainBindingPhase::kLoad},
      {"GetSessionId", 1, 1, 0, ChainBindingKind::kRtSession, ChainBindingPhase::kLoad},
      {"SplitVariable", 3, 2, 0, ChainBindingKind::kRtSession, ChainBindingPhase::kLoad},
      {"GetSpaceRegistry", 2, 1, 0, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad},
      {"GetSpaceRegistries", 1, 1, 0, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad},
      {"GetFileConstantWeightDir", 1, 1, 0, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad},
      {"SplitRtStreams", 2, 2, 0, ChainBindingKind::kStream, ChainBindingPhase::kLoad},
      {"CreateNotifies", 2, 2, 0, ChainBindingKind::kNotifies, ChainBindingPhase::kRun},
  };
  for (const auto &item : cases) {
    SCOPED_TRACE(item.kernel);
    auto *node = Node(item.kernel);
    ASSERT_NE(node, nullptr);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(item.kernel);
    ASSERT_NE(builder, nullptr);
    std::vector<uint32_t> inputs(item.inputs), outputs(item.outputs);
    std::iota(inputs.begin(), inputs.end(), 10U);
    std::iota(outputs.begin(), outputs.end(), 20U);
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(KernelCollectContext(*node, inputs, outputs), info), ge::GRAPH_SUCCESS);
    size_t requests = 0U;
    for (const auto &binding : info.chain_updates) {
      if (binding.source_kind == ChainSourceKind::kInputBindingRequest) {
        ++requests;
        EXPECT_EQ(binding.input_port, static_cast<int64_t>(item.port));
        EXPECT_EQ(binding.chain_index, inputs[item.port]);
        EXPECT_EQ(binding.binding_kind, item.resource);
        EXPECT_EQ(binding.binding_phase, item.phase);
      }
    }
    EXPECT_EQ(requests, 1U);
  }
}

TEST_F(Om2KernelContractUT, AicpuCcLaunchPreservesHandleAndStreamPortsAndRejectsMissingPort) {
  auto *node = Node("AicpuLaunchCCKernel");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("AicpuLaunchCCKernel");
  ASSERT_NE(builder, nullptr);
  std::vector<uint32_t> inputs{3, 5, 7, 11, 13, 17, 19, 23};
  const std::vector<uint32_t> outputs{29};
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.inputs.size(), 8U);
  EXPECT_EQ(info.inputs[0].value_type, Om2ValueType::kAicpuArgsHandlerPtr);
  EXPECT_EQ(info.inputs[1].value_type, Om2ValueType::kRtStreamTPtr);
  EXPECT_EQ(info.inputs[4].value_type, Om2ValueType::kGertAicpuExtInfoHandlerPtr);
  EXPECT_EQ(info.inputs[5].chain_index, 17U);
  EXPECT_EQ(info.inputs[6].value_type, Om2ValueType::kCStringPtr);
  ASSERT_EQ(info.outputs.size(), 1U);
  EXPECT_EQ(info.outputs[0].chain_index, 29U);
  inputs.pop_back();
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(context, invalid), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelContractUT, HostFunctionRestorationRequiresOriginalTypeAndSerializesNameOnly) {
  auto *node = Node("AicpuHostExecFunc");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("AicpuHostExecFunc");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{3, 5, 7}, outputs{11, 13};
  KernelCollectContext context(*node, inputs, outputs);
  CustomConstRestoreDescriptor restored;
  EXPECT_NE(builder->RestoreConstInput(context, 2U, restored), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(ge::AttrUtils::SetStr(node->GetOpDescBarePtr(), ge::ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "Add"));
  EXPECT_NE(builder->RestoreConstInput(context, 0U, restored), ge::GRAPH_SUCCESS);
  ASSERT_EQ(builder->RestoreConstInput(context, 2U, restored), ge::GRAPH_SUCCESS);
  EXPECT_EQ(restored.initializer, "aicpu_host_proc_func");
  EXPECT_EQ(restored.payload, (std::vector<uint8_t>{'A', 'd', 'd', 0}));
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.inputs.size(), 3U);
  EXPECT_EQ(info.inputs[0].value_type, Om2ValueType::kGertStorageShapePtr);
  EXPECT_EQ(info.inputs[1].value_type, Om2ValueType::kGertTensorDataPtr);
  EXPECT_EQ(info.inputs[2].value_type, Om2ValueType::kAicpuHostProcFuncPtr);
  ASSERT_EQ(info.outputs.size(), 2U);
  EXPECT_EQ(info.outputs[0].semantic_name, "output_shape_0");
  EXPECT_EQ(info.outputs[1].semantic_name, "output_data_0");
}

TEST_F(Om2KernelContractUT, AclnnLaunchKeepsTensorPrefixAndWorkspaceStreamTail) {
  auto *node = Node("ExecuteOpLaunch");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("ExecuteOpLaunch");
  ASSERT_NE(builder, nullptr);
  const std::vector<uint32_t> inputs{3, 5, 7, 11, 13, 17, 19}, outputs;
  KernelCollectContext context(*node, inputs, outputs);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.inputs.size(), 7U);
  EXPECT_EQ(info.inputs[0].value_type, Om2ValueType::kGertTensorPtr);
  EXPECT_EQ(info.inputs[1].value_type, Om2ValueType::kGertTensorPtr);
  EXPECT_EQ(info.inputs[3].value_type, Om2ValueType::kGertTensorDataVectorPtr);
  EXPECT_EQ(info.inputs[4].value_type, Om2ValueType::kTypedContinuousVectorSizeTPtr);
  EXPECT_EQ(info.inputs[5].value_type, Om2ValueType::kRtStreamTPtr);
  EXPECT_EQ(info.inputs[5].chain_index, 17U);
  EXPECT_TRUE(info.outputs.empty());
  const std::vector<uint32_t> too_short{1, 2, 3, 4};
  KernelCollectContext malformed(*node, too_short, outputs);
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(malformed, invalid), ge::GRAPH_SUCCESS);
}

TEST_F(Om2KernelContractUT, AicoreLaunchSeparatesAddressesShapesAndMixTail) {
  auto *node = Node("LaunchMixKernelV2");
  ASSERT_NE(node, nullptr);
  auto builder = KernelCodeBuilderFactory::GetInstance().Create("LaunchMixKernelV2");
  ASSERT_NE(builder, nullptr);
  constexpr size_t prefix = static_cast<size_t>(kernel::InputCommon::kNum);
  constexpr size_t io_index = static_cast<size_t>(kernel::InputCommon::kIoNum);
  std::vector<uint32_t> inputs(prefix + 2U * 2U + 4U);
  std::iota(inputs.begin(), inputs.end(), 10U);
  const std::vector<uint32_t> outputs;
  std::vector<Chain> values(inputs.size());
  const std::vector<Chain> output_values;
  values[io_index].GetValue<size_t>() = 2U;
  KernelCollectContext context(*node, inputs, outputs, values, output_values);
  KernelProgramInfo info;
  ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  ASSERT_EQ(info.inputs.size(), inputs.size());
  EXPECT_EQ(info.inputs[prefix].value_type, Om2ValueType::kGertTensorDataPtr);
  EXPECT_EQ(info.inputs[prefix + 1U].semantic_name, "io_address_1");
  EXPECT_EQ(info.inputs[prefix + 2U].value_type, Om2ValueType::kGertStorageShapePtr);
  EXPECT_EQ(info.inputs[prefix + 3U].semantic_name, "io_shape_1");
  EXPECT_EQ(info.inputs[prefix + 4U].value_type, Om2ValueType::kGertMixCoreArgsPtr);
  EXPECT_EQ(info.inputs[prefix + 5U].value_type, Om2ValueType::kRtStreamTPtr);
  EXPECT_EQ(info.inputs.back().value_type, Om2ValueType::kRtNotifyTPtr);
  EXPECT_EQ(builder->GetRestoreConstInputIndices(), (std::vector<uint32_t>{static_cast<uint32_t>(io_index)}));
  inputs.pop_back();
  KernelProgramInfo invalid;
  EXPECT_NE(builder->Collect(context, invalid), ge::GRAPH_SUCCESS);
  auto ordinary = KernelCodeBuilderFactory::GetInstance().Create("LaunchKernelV2");
  ASSERT_NE(ordinary, nullptr);
  KernelProgramInfo fallback;
  // Ordinary launch accepts the partial tail left by the lowering fallback.
  ASSERT_EQ(ordinary->Collect(context, fallback), ge::GRAPH_SUCCESS);
  EXPECT_EQ(fallback.inputs.back().semantic_name, "notify_0");
}

TEST_F(Om2KernelContractUT, LaunchV2VariantsValidateOptionalAddressAndShapeConstantSizes) {
  constexpr size_t prefix = static_cast<size_t>(kernel::InputCommon::kNum);
  constexpr size_t io_index = static_cast<size_t>(kernel::InputCommon::kIoNum);
  constexpr size_t io_count = 2U;
  std::vector<uint32_t> inputs(prefix + 2U * io_count + 4U);
  std::iota(inputs.begin(), inputs.end(), 100U);
  std::vector<Chain> values(inputs.size());
  values[io_index].GetValue<size_t>() = io_count;
  const std::vector<uint32_t> outputs;
  for (const char *type : {"LaunchKernelV2", "LaunchMixKernelV2"}) {
    auto op_desc = std::make_shared<ge::OpDesc>(type, type);
    for (size_t i = 0U; i < inputs.size(); ++i) {
      ASSERT_EQ(op_desc->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    }
    auto *node = graph_.AddNode(op_desc);
    ASSERT_NE(node, nullptr);
    ASSERT_TRUE(ge::AttrUtils::SetInt(node->GetOpDescBarePtr(), kComputeNodeIndex, 0));
    for (size_t i = 0U; i < inputs.size(); ++i) {
      ASSERT_EQ(node->GetOpDescBarePtr()->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    }
    auto builder = KernelCodeBuilderFactory::GetInstance().Create(type);
    ASSERT_NE(builder, nullptr);
    std::vector<Chain> output_values;
    KernelCollectContext context(*node, inputs, outputs, values, output_values);
    for (const size_t port : {prefix, prefix + 1U, prefix + 2U, prefix + 3U}) {
      auto const_desc = std::make_shared<ge::OpDesc>((std::string("const_") + std::to_string(port)), "Const");
      ASSERT_EQ(const_desc->AddOutputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
      auto *constant = graph_.AddNode(const_desc);
      ASSERT_NE(constant, nullptr);
      const bool is_address = port < prefix + io_count;
      const auto valid_size = is_address ? sizeof(gert::GertTensorData) : sizeof(gert::StorageShape);
      const std::vector<uint8_t> bytes(valid_size, 0U);
      ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(constant->GetOpDescBarePtr(), kConstValue,
                                                  ge::Buffer::CopyFrom(bytes.data(), bytes.size())));
      ASSERT_NE(graph_.AddEdge(constant, 0, node, static_cast<int32_t>(port)), nullptr);
    }
    KernelProgramInfo info;
    ASSERT_EQ(builder->Collect(context, info), ge::GRAPH_SUCCESS);
    const size_t bad_port = prefix + io_count;
    const auto *bad_edge = node->GetInDataEdgeByIndex(static_cast<int32_t>(bad_port));
    ASSERT_NE(bad_edge, nullptr);
    auto *bad_constant = bad_edge->src;
    ASSERT_NE(bad_constant, nullptr);
    const std::vector<uint8_t> invalid_bytes(16U, 0U);
    ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(bad_constant->GetOpDescBarePtr(), kConstValue,
                                                ge::Buffer::CopyFrom(invalid_bytes.data(), invalid_bytes.size())));
    EXPECT_NE(builder->Collect(context, info), ge::GRAPH_SUCCESS);
  }
}
}  // namespace
}  // namespace om2
}  // namespace gert
