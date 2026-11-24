/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "custom_node_converter.h"
#include "graph_builder/converter_checker.h"
#include "graph_builder/bg_tensor.h"
#include "graph_builder/bg_infer_shape.h"
#include "graph_builder/bg_attached_stream_provider.h"
#include "common/checker.h"
#include "exe_graph/lowering/frame_selector.h"
#include "kernel/common_kernel_impl/build_tensor.h"
#include "graph/custom_op_registry.h"
#include "lowering/placement/placed_lowering_result.h"
#include "exe_graph/lowering/lowering_definitions.h"
#include "exe_graph/runtime/eager_op_execution_context.h"
#include "common/ge_common/ge_types.h"
#include "graph/utils/inference_rule.h"
#include "exe_graph/lowering/data_dependent_interpreter.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "register/core_num_utils.h"
#include "platform/platform_info.h"
#include "acl/acl_rt.h"
#include "framework/common/host_cpu_fusion_attr.h"

#include <cstdint>

namespace gert {
namespace {
// Default threshold for non-tensor InputKind values. When the "_custom_op_non_tensor_kind_base"
// attribute is absent (e.g. older OM models without the attribute), this value is used so that
// InputKind enums starting from 3 (INT, FLOAT, BOOL, ...) are recognised as non-tensor.
constexpr int64_t kDefaultNonTensorInputKindBase = 3L;

bool NeedHostInput(const ge::OpDescPtr &op_desc, size_t instance_index, const gert::DataDependentInterpreter &ddi) {
  std::vector<int64_t> input_kinds;
  if (!ge::AttrUtils::GetListInt(op_desc, "input_kinds", input_kinds) || input_kinds.empty()) {
    return false;
  }
  size_t ir_index = 0U;
  if (ge::OpDescUtils::GetInputIrIndexByInstanceIndex(op_desc, instance_index, ir_index) != ge::SUCCESS) {
    return false;
  }
  // Read the non-tensor kind base from the attribute set by codegen; fall back to default for
  // backward compatibility with OMs that do not carry this attribute.
  int64_t non_tensor_base = kDefaultNonTensorInputKindBase;
  (void)ge::AttrUtils::GetInt(op_desc, "_custom_op_non_tensor_kind_base", non_tensor_base);
  if (ir_index >= input_kinds.size() || input_kinds[ir_index] < non_tensor_base) {
    return false;
  }
  bool is_data_dependent = false;
  if (ddi.IsDataDependent(static_cast<int32_t>(instance_index), is_data_dependent) != ge::GRAPH_SUCCESS) {
    return false;
  }
  if (is_data_dependent) {
    GELOGI("NeedHostInput: op=%s, input instance_index=%zu, ir_index=%zu, input_kind=%ld, non_tensor_base=%ld",
           op_desc->GetNamePtr(), instance_index, ir_index, input_kinds[ir_index], non_tensor_base);
  }
  return is_data_dependent;
}

bool NeedCustomOpInferShape(const ge::NodePtr &node, const LoweringGlobalData &global_data) {
  GE_ASSERT_NOTNULL(node);
  const auto op_desc = node->GetOpDesc();
  GE_ASSERT_NOTNULL(op_desc);
  if (bg::FindShapeInferOpInCustomOpRegistry(node->GetTypePtr(), global_data) != nullptr) {
    return true;
  }
  const std::string infer_rule = ge::InferenceRule::GetInferenceRule(op_desc);
  if (!infer_rule.empty()) {
    return true;
  }
  return false;
}

bg::ValueHolderPtr FindCustomExecutorFunc(const ge::NodePtr &node, const LowerInput &lower_input) {
  auto builder = [&node, &lower_input]() -> std::vector<bg::ValueHolderPtr> {
    return bg::FrameSelector::OnInitRoot([&node, &lower_input]() -> std::vector<bg::ValueHolderPtr> {
      auto node_type = bg::ValueHolder::CreateConst(node->GetTypePtr(), node->GetType().size() + 1, true);
      ge::CustomOpRegistry *custom_op_registry = lower_input.global_data->GetCustomOpRegistry().get();
      auto registry_holder = bg::ValueHolder::CreateConst(&custom_op_registry, sizeof(custom_op_registry));
      return {bg::ValueHolder::CreateSingleDataOutput("FindCustomOp", {node_type, registry_holder})};
    });
  };
  return lower_input.global_data->GetOrCreateUniqueValueHolder(node->GetType() + "_FindCustomOp_", builder)[0];
}

bg::ValueHolderPtr FindHostCpuCustomExecutorFunc(const ge::NodePtr &node, const LowerInput &lower_input,
                                                 const char *kernel_type) {
  auto builder = [&node, &lower_input, kernel_type]() -> std::vector<bg::ValueHolderPtr> {
    return bg::FrameSelector::OnInitRoot([&node, &lower_input, kernel_type]() -> std::vector<bg::ValueHolderPtr> {
      auto node_type = bg::ValueHolder::CreateConst(node->GetTypePtr(), node->GetType().size() + 1, true);
      ge::CustomOpRegistry *custom_op_registry = lower_input.global_data->GetCustomOpRegistry().get();
      auto registry_holder = bg::ValueHolder::CreateConst(&custom_op_registry, sizeof(custom_op_registry));
      return {bg::ValueHolder::CreateSingleDataOutput(kernel_type, {node_type, registry_holder})};
    });
  };
  return lower_input.global_data->GetOrCreateUniqueValueHolder(node->GetType() + "_" + kernel_type + "_", builder)[0];
}

ge::graphStatus BuildInputTensors(const ge::NodePtr &node, const LowerInput &lower_input,
                                  std::vector<bg::ValueHolderPtr> &input_tensor_holders,
                                  std::vector<bg::ValueHolderPtr> &input_addr_holders, const bool host_lowering,
                                  const bool borrow_host_inputs = false) {
  const ge::OpDescPtr op_desc = node->GetOpDesc();
  gert::DataDependentInterpreter ddi(op_desc, lower_input.global_data->GetSpaceRegistriesV2());
  size_t instance_index = 0U;
  for (const auto *in_data_anchor : node->GetAllInDataAnchorsPtr()) {
    GE_ASSERT_NOTNULL(in_data_anchor);
    // optional场景
    ge::OutDataAnchorPtr out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    if (out_data_anchor == nullptr) {
      continue;
    }
    ge::Node *peer_node = out_data_anchor->GetOwnerNodeBarePtr();
    GE_ASSERT_NOTNULL(peer_node);
    auto *lower_result = peer_node->GetOpDesc()->GetExtAttr<PlacedLoweringResult>(kLoweringResult);
    GE_ASSERT_NOTNULL(lower_result, "Lowering result of node [%s, %s] is not found.", peer_node->GetNamePtr(),
                      peer_node->GetTypePtr());
    const int32_t input_placement =
        host_lowering ? kOnHost : (NeedHostInput(op_desc, instance_index, ddi) ? kOnHost : kOnDeviceHbm);
    const TargetAddrDesc target = {input_placement, node->GetOpDesc()->GetStreamId()};
    const OutputLowerResult *result =
        borrow_host_inputs
            ? lower_result->GetOutputResult(*lower_input.global_data, out_data_anchor->GetIdx(), target, false)
            : lower_result->GetOutputTensorResult(*lower_input.global_data, out_data_anchor->GetIdx(), target);
    GE_ASSERT_NOTNULL(result, "Lowering result of node [%s, %s] output[%d] is nullptr.", peer_node->GetNamePtr(),
                      peer_node->GetTypePtr(), out_data_anchor->GetIdx());
    GE_ASSERT_NOTNULL(result->shape);
    auto tensor = borrow_host_inputs ? bg::BuildRefTensor(peer_node->shared_from_this(), out_data_anchor->GetIdx(),
                                                          kOnHost, result->shape, result->address)
                                     : result->shape;
    GE_ASSERT_NOTNULL(tensor);
    input_tensor_holders.emplace_back(tensor);
    input_addr_holders.emplace_back(result->address);
    ++instance_index;
  }
  GE_ASSERT_TRUE(lower_input.input_shapes.size() == input_tensor_holders.size(),
                 "Size[%zu] of input shapes and size[%zu] of input tensor is not same.",
                 lower_input.input_shapes.size(), input_tensor_holders.size());
  return ge::SUCCESS;
}

// 计算自定义算子最终透传的核数：aicore/vector 走算子级 > 全局选项 > 设备合并平台值；
// 无设备上下文时回退平台静态规格；aicpu 取设备值。
// 获取失败时保持 -1（sentinel：未设置），运行期查询返回 false。
void ResolveCustomOpCoreNum(const ge::OpDescPtr &op_desc, const ge::ComputeGraphPtr &root_graph, int64_t &aicore_num,
                            int64_t &vector_core_num, int64_t &aicpu_num) {
  aicore_num = -1;
  vector_core_num = -1;
  aicpu_num = -1;
  int32_t device_id = -1;
  int64_t ai_cpu_cnt = -1;
  const bool has_device = (aclrtGetDevice(&device_id) == ACL_SUCCESS);
  if (has_device &&
      (aclrtGetDeviceInfo(static_cast<uint32_t>(device_id), ACL_DEV_ATTR_AICPU_CORE_NUM, &ai_cpu_cnt) == ACL_SUCCESS) &&
      (ai_cpu_cnt >= 0)) {
    aicpu_num = ai_cpu_cnt;
  }

  const char *soc_name = aclrtGetSocName();
  // 回退基线优先取设备合并后的平台信息，与 V1 口径一致；静态规格仅用于算子级属性的校验上限
  fe::PlatFormInfos platform_infos;
  if (has_device) {
    if (fe::PlatformInfoManager::GeInstance().GetRuntimePlatformInfosByDevice(static_cast<uint32_t>(device_id),
                                                                              platform_infos, true) != 0U) {
      // 有设备但运行平台缓存不可用：保持未设置，避免固化静态规格导致与设备实际核数不一致
      GELOGW("Get runtime platform infos failed, core num of op[%s] will not be injected.", op_desc->GetNamePtr());
      return;
    }
    int32_t graph_aicore_num = -1;
    int32_t graph_vectorcore_num = -1;
    if ((root_graph != nullptr) && (ge::CoreNumUtils::GetCoreNumFromGraph(root_graph, graph_aicore_num,
                                                                          graph_vectorcore_num) == ge::GRAPH_SUCCESS)) {
      std::map<std::string, std::string> soc_info;
      if (platform_infos.GetPlatformResWithLock("SoCInfo", soc_info)) {
        if (graph_aicore_num > 0) {
          soc_info["ai_core_cnt"] = std::to_string(graph_aicore_num);
          soc_info["cube_core_cnt"] = std::to_string(graph_aicore_num);
        }
        if (graph_vectorcore_num > 0) {
          soc_info["vector_core_cnt"] = std::to_string(graph_vectorcore_num);
        }
        platform_infos.SetPlatformResWithLock("SoCInfo", soc_info);
      }
    }
    ge::CoreNumUtils::EffectiveCoreNum effective_core_num;
    if (ge::CoreNumUtils::ResolveSinkOpCoreNum(op_desc, platform_infos, soc_name, effective_core_num) ==
        ge::GRAPH_SUCCESS) {
      aicore_num = effective_core_num.aicore_num;
      vector_core_num = effective_core_num.vector_core_num;
    }
    return;
  }

  // 无设备上下文：回退平台静态规格
  fe::PlatformInfo platform_info;
  if ((soc_name != nullptr) &&
      (ge::CoreNumUtils::GetGeDefaultPlatformInfo(soc_name, platform_info) == ge::GRAPH_SUCCESS)) {
    ge::CoreNumUtils::EffectiveCoreNum effective_core_num;
    if (ge::CoreNumUtils::ResolveCoreNumByOp(op_desc, root_graph, platform_info, effective_core_num) ==
        ge::GRAPH_SUCCESS) {
      aicore_num = effective_core_num.aicore_num;
      vector_core_num = effective_core_num.vector_core_num;
    }
  }
}
}  // namespace

LowerResult LoweringCustomNode(const ge::NodePtr &node, const LowerInput &lower_input) {
  LOWER_REQUIRE_HYPER_SUCCESS(CheckLowerInput(lower_input));
  std::vector<bg::ValueHolderPtr> input_holders;
  std::vector<bg::ValueHolderPtr> input_addr_holders;
  LOWER_REQUIRE_SUCCESS(BuildInputTensors(node, lower_input, input_holders, input_addr_holders, false));
  // Allocate
  auto allocator_holder =
      lower_input.global_data->GetOrCreateAllocator({kOnDeviceHbm, AllocatorUsage::kAllocNodeOutput});
  auto custom_executor_func = FindCustomExecutorFunc(node, lower_input);
  // Create op executeFunc
  input_holders.emplace_back(allocator_holder);
  input_holders.emplace_back(lower_input.global_data->GetStream());
  input_holders.emplace_back(custom_executor_func);
  // 辅流容器：Init 图创建、执行器析构时释放，per-executor 隔离。
  // 必须插在 custom_executor_func 之后、infer 模板 tensor 之前，与 CustomOpInput 枚举顺序保持一致。
  const auto attached_stream_provider = bg::GetAttachedStreamProvider(*lower_input.global_data);
  LOWER_REQUIRE_NOTNULL(attached_stream_provider);
  input_holders.emplace_back(attached_stream_provider);
  // 根图graph id / node id / 核数：随执行节点输入下发（按节点，非按类型），
  // 运行期由ExecuteCustomOp的outputs_creator读取并写入执行上下文伴生对象；
  // 未设置的值用sentinel(-1)表示，须与CustomOpInput枚举顺序一致。
  const auto owner_graph = node->GetOwnerComputeGraph();
  const auto root_graph = (owner_graph == nullptr) ? nullptr : ge::GraphUtils::FindRootGraph(owner_graph);
  const uint32_t root_graph_id = (root_graph == nullptr) ? 0U : root_graph->GetGraphID();
  const int64_t node_id = node->GetOpDesc()->GetId();
  int64_t aicore_num = -1;
  int64_t vector_core_num = -1;
  int64_t aicpu_num = -1;
  ResolveCustomOpCoreNum(node->GetOpDesc(), root_graph, aicore_num, vector_core_num, aicpu_num);
  auto root_graph_id_holder = bg::ValueHolder::CreateConst(&root_graph_id, sizeof(root_graph_id));
  LOWER_REQUIRE_NOTNULL(root_graph_id_holder);
  auto node_id_holder = bg::ValueHolder::CreateConst(&node_id, sizeof(node_id));
  LOWER_REQUIRE_NOTNULL(node_id_holder);
  auto vector_core_num_holder = bg::ValueHolder::CreateConst(&vector_core_num, sizeof(vector_core_num));
  LOWER_REQUIRE_NOTNULL(vector_core_num_holder);
  auto aicore_num_holder = bg::ValueHolder::CreateConst(&aicore_num, sizeof(aicore_num));
  LOWER_REQUIRE_NOTNULL(aicore_num_holder);
  auto aicpu_num_holder = bg::ValueHolder::CreateConst(&aicpu_num, sizeof(aicpu_num));
  LOWER_REQUIRE_NOTNULL(aicpu_num_holder);
  input_holders.emplace_back(root_graph_id_holder);
  input_holders.emplace_back(node_id_holder);
  input_holders.emplace_back(vector_core_num_holder);
  input_holders.emplace_back(aicore_num_holder);
  input_holders.emplace_back(aicpu_num_holder);
  // Check inference_rule
  const auto op_desc = node->GetOpDesc();
  LOWER_REQUIRE_NOTNULL(op_desc);
  std::string kernel_type = "ExecuteCustomOp";
  std::vector<bg::ValueHolderPtr> infer_output_shapes;
  if (NeedCustomOpInferShape(node, *lower_input.global_data)) {
    kernel_type = "ExecuteCustomOpWithInferShape";
    infer_output_shapes = bg::InferCustomOpShape(node, lower_input.input_shapes, *lower_input.global_data);
    input_holders.insert(input_holders.end(), infer_output_shapes.begin(), infer_output_shapes.end());
  }
  // 最后需要workspace地址和args_handler地址
  std::vector<bg::ValueHolderPtr> output_tensor_holders = bg::ValueHolder::CreateDataOutput(
      kernel_type.c_str(), input_holders,
      node->GetAllOutDataAnchorsSize() +
          static_cast<size_t>(gert::EagerOpExecutionContext::AdditionalOutputIndex::kNum));
  std::vector<bg::ValueHolderPtr> output_shapes;
  std::vector<bg::DevMemValueHolderPtr> output_addrs;
  for (size_t i = 0UL; i < node->GetAllOutDataAnchorsSize(); i++) {
    auto split_outputs = bg::DevMemValueHolder::CreateDataOutput(
        kernel::kSplitDataTensor, {output_tensor_holders[i], allocator_holder},
        static_cast<size_t>(kernel::SplitTensorOutputs::kNum), op_desc->GetStreamId());
    CONVERTER_CHECK_HOLDERS_ALL_OK(split_outputs, static_cast<size_t>(kernel::SplitTensorOutputs::kNum));
    LOWER_REQUIRE_NOTNULL(bg::ValueHolder::CreateVoidGuarder(
        "FreeMemory", split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kTensorData)], {}));
    output_shapes.emplace_back(split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kShape)]);
    output_addrs.emplace_back(split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kTensorData)]);
    for (size_t j = 0UL; j < input_addr_holders.size(); j++) {
      auto guarder = input_addr_holders[j]->GetGuarder();
      if (guarder != nullptr) {
        GELOGI("[Dumper] Add dependency from node %s input[%zu] to output[%zu] of node %s.", node->GetNamePtr(), j, i,
               node->GetNamePtr());
        GE_ASSERT_HYPER_SUCCESS(bg::ValueHolder::AddDependency(
            split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kTensorData)], guarder));
      }
    }
  }
  LOWER_REQUIRE_NOTNULL(bg::ValueHolder::CreateVoidGuarder(
      "FreeCustomOpWorkspaces", output_tensor_holders[node->GetAllOutDataAnchorsSize()], {allocator_holder}));
  LOWER_REQUIRE_NOTNULL(bg::ValueHolder::CreateVoidGuarder(
      "FreeArgsGuarder",
      output_tensor_holders[node->GetAllOutDataAnchorsSize() +
                            static_cast<size_t>(gert::EagerOpExecutionContext::AdditionalOutputIndex::kArgsHandler)],
      {}));
  // 输入tensor需要添加对地址guard的依赖边，否则会出现提前释放
  for (auto &addr : input_addr_holders) {
    auto guarder = addr->GetGuarder();
    if (guarder != nullptr) {
      GE_ASSERT_HYPER_SUCCESS(bg::ValueHolder::AddDependency(output_tensor_holders.front(), guarder));
    }
  }
  return {HyperStatus::Success(), {}, output_shapes, output_addrs};
}
REGISTER_NODE_CONVERTER_PLACEMENT(ge::kCustomOpKernelLibName.c_str(), kOnDeviceHbm, LoweringCustomNode);

LowerResult LoweringHostCustomNode(const ge::NodePtr &node, const LowerInput &lower_input) {
  LOWER_REQUIRE_HYPER_SUCCESS(CheckLowerInput(lower_input));
  const auto op_desc = node->GetOpDesc();
  LOWER_REQUIRE_NOTNULL(op_desc);
  const bool need_infer_shape = NeedCustomOpInferShape(node, *lower_input.global_data);
  bool generated_fusion = false;
  (void)ge::AttrUtils::GetBool(op_desc, ge::kFusedHostCpuGenerated, generated_fusion);
  const bool combine_split = generated_fusion && !need_infer_shape;
  std::vector<bg::ValueHolderPtr> input_holders;
  std::vector<bg::ValueHolderPtr> input_addr_holders;
  // Generated outputs never alias inputs. The address guarders below keep borrowed inputs alive.
  LOWER_REQUIRE_SUCCESS(BuildInputTensors(node, lower_input, input_holders, input_addr_holders, true, combine_split));
  // Allocate
  auto allocator_holder = lower_input.global_data->GetOrCreateAllocator({kOnHost, AllocatorUsage::kAllocNodeOutput});
  // Check inference_rule
  std::string kernel_type = "ExecuteHostCustomOp";
  auto custom_executor_func = FindHostCpuCustomExecutorFunc(
      node, lower_input, combine_split ? "FindHostCpuFusedCustomOp" : "FindHostCpuCustomOp");
  input_holders.emplace_back(allocator_holder);
  input_holders.emplace_back(custom_executor_func);
  if (need_infer_shape) {
    kernel_type = "ExecuteHostCustomOpWithInferShape";
    std::vector<bg::ValueHolderPtr> infer_output_shapes =
        bg::InferCustomOpShape(node, lower_input.input_shapes, *lower_input.global_data);
    input_holders.insert(input_holders.end(), infer_output_shapes.begin(), infer_output_shapes.end());
  }
  const size_t output_num = node->GetAllOutDataAnchorsSize();
  std::vector<bg::ValueHolderPtr> output_tensor_holders;
  std::vector<bg::DevMemValueHolderPtr> combined_outputs;
  if (combine_split) {
    combined_outputs = bg::DevMemValueHolder::CreateDataOutput("ExecuteHostCustomOpAndSplit", input_holders,
                                                               output_num * 2U, op_desc->GetStreamId());
    CONVERTER_CHECK_HOLDERS_ALL_OK(combined_outputs, output_num * 2U);
    output_tensor_holders.assign(combined_outputs.begin(), combined_outputs.begin() + output_num);
  } else {
    output_tensor_holders = bg::ValueHolder::CreateDataOutput(kernel_type.c_str(), input_holders, output_num);
  }
  std::vector<bg::ValueHolderPtr> output_shapes;
  std::vector<bg::DevMemValueHolderPtr> output_addrs;
  for (size_t i = 0UL; i < node->GetAllOutDataAnchorsSize(); i++) {
    auto split_outputs =
        combine_split ? std::vector<bg::DevMemValueHolderPtr>{combined_outputs[i], combined_outputs[output_num + i]}
                      : bg::DevMemValueHolder::CreateDataOutput(
                            kernel::kSplitDataTensor, {output_tensor_holders[i], allocator_holder},
                            static_cast<size_t>(kernel::SplitTensorOutputs::kNum), op_desc->GetStreamId());
    CONVERTER_CHECK_HOLDERS_ALL_OK(split_outputs, static_cast<size_t>(kernel::SplitTensorOutputs::kNum));
    auto output_addr = split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kTensorData)];
    output_addr->SetPlacement(kOnHost);
    LOWER_REQUIRE_NOTNULL(bg::ValueHolder::CreateVoidGuarder("FreeMemory", output_addr, {}));
    output_shapes.emplace_back(split_outputs[static_cast<size_t>(kernel::SplitTensorOutputs::kShape)]);
    output_addrs.emplace_back(output_addr);
  }

  for (auto &addr : input_addr_holders) {
    auto guarder = addr->GetGuarder();
    if ((guarder != nullptr) && (!output_tensor_holders.empty())) {
      GE_ASSERT_HYPER_SUCCESS(bg::ValueHolder::AddDependency(output_tensor_holders.front(), guarder));
    }
  }
  return {HyperStatus::Success(), {}, output_shapes, output_addrs};
}
REGISTER_NODE_CONVERTER_PLACEMENT(ge::kHostCpuCustomOpLowerFunc.c_str(), kOnHost, LoweringHostCustomNode);
}  // namespace gert
