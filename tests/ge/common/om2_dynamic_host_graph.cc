/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_dynamic_host_graph.h"
#include "common/memory/tensor_trans_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/runtime/extended_kernel_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "ge/ge_api_error_codes.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "register/kernel_registry.h"

namespace gert {
namespace om2_test {
// A Host test Kernel is the computational substitute. All graph preparation,
// collection, code generation, compilation and exported model functions are real.
ge::graphStatus Om2ExternalGeneratorAffine(KernelContext *context) {
  const auto *input = context->GetInputPointer<Tensor>(0U);
  const auto *parameters = context->GetInputPointer<Tensor>(1U);
  auto *output = context->GetInputValue<Tensor *>(2U);
  if (input == nullptr || parameters == nullptr || output == nullptr || input->GetDataType() != ge::DT_FLOAT ||
      parameters->GetDataType() != ge::DT_FLOAT || input->GetPlacement() != kOnHost ||
      parameters->GetPlacement() != kOnHost || input->GetAddr() == nullptr || parameters->GetAddr() == nullptr ||
      parameters->GetSize() < 2U * sizeof(float) || input->GetShapeSize() < 0) {
    return ge::GRAPH_PARAM_INVALID;
  }
  // Session RunGraph supplies an unallocated Tensor for a dynamic output.
  // Give that output owned Host storage; direct model runs may supply their own buffer.
  if (output->GetAddr() == nullptr) {
    ge::GeTensor tensor;
    tensor.MutableTensorDesc().SetPlacement(ge::Placement::kPlacementHost);
    if (tensor.SetData(std::vector<uint8_t>(input->GetSize())) != ge::GRAPH_SUCCESS ||
        ge::TensorTransUtils::GeTensor2GertTensor(tensor, *output) != ge::SUCCESS) {
      return ge::GRAPH_FAILED;
    }
    output->SetOriginFormat(ge::FORMAT_ND);
    output->SetStorageFormat(ge::FORMAT_ND);
  }
  if (output->GetSize() < input->GetSize()) {
    return ge::GRAPH_PARAM_INVALID;
  }
  const auto *values = input->GetData<float>();
  const auto *coefficients = parameters->GetData<float>();
  auto *result = output->GetData<float>();
  for (int64_t i = 0; i < input->GetShapeSize(); ++i) {
    result[i] = values[i] * coefficients[0] + coefficients[1];
  }
  output->MutableOriginShape() = input->GetOriginShape();
  output->MutableStorageShape() = input->GetStorageShape();
  output->SetDataType(ge::DT_FLOAT);
  output->SetSize(input->GetSize());
  return ge::GRAPH_SUCCESS;
}
REGISTER_KERNEL(Om2ExternalGeneratorAffine).RunFunc(Om2ExternalGeneratorAffine);

// Separate test Kernel keeps the successful affine operation's contract unchanged.
ge::graphStatus Om2ExternalGeneratorFailingAffine(KernelContext *context) {
  const auto *parameters = context->GetInputPointer<Tensor>(1U);
  auto *output = context->GetInputValue<Tensor *>(2U);
  if (parameters != nullptr && parameters->GetAddr() != nullptr && parameters->GetSize() >= 2U * sizeof(float) &&
      parameters->GetData<float>()[0] == -9999.0F) {
    // Keep the direct-run partial-write check; Session may supply an unallocated output.
    if (output != nullptr && output->GetAddr() != nullptr && output->GetSize() >= sizeof(float)) {
      output->GetData<float>()[0] = -1234.0F;
    }
    return ge::GRAPH_FAILED;
  }
  return Om2ExternalGeneratorAffine(context);
}
REGISTER_KERNEL(Om2ExternalGeneratorFailingAffine).RunFunc(Om2ExternalGeneratorFailingAffine);

// The same test kernel can consume metadata produced by either compilation path.
ge::graphStatus Om2ExternalGeneratorSourceAffine(KernelContext *context) {
  const auto *extended = reinterpret_cast<const ExtendedKernelContext *>(context);
  const auto *info = extended->GetComputeNodeInfo();
  if (info == nullptr || info->GetNodeName() == nullptr || info->GetNodeType() == nullptr ||
      std::string(info->GetNodeName()) != "matmul_source" || std::string(info->GetNodeType()) != "MatMul" ||
      info->GetInputsNum() != 2U || info->GetOutputsNum() != 2U || info->GetIrInputsNum() != 1U ||
      info->GetIrOutputsNum() != 0U || info->GetInputInstanceInfo(0U)->GetInstanceStart() != 0U ||
      info->GetInputInstanceInfo(0U)->GetInstanceNum() != 2U) {
    return ge::GRAPH_PARAM_INVALID;
  }
  const auto *attrs = info->GetAttrs();
  const auto *axis = attrs->GetInt(0U);
  const auto *axes = attrs->GetListInt(1U);
  if (axis == nullptr || *axis != -7 || axes == nullptr || axes->GetSize() != 0U) {
    return ge::GRAPH_PARAM_INVALID;
  }
  return Om2ExternalGeneratorAffine(context);
}
REGISTER_KERNEL(Om2ExternalGeneratorSourceAffine).RunFunc(Om2ExternalGeneratorSourceAffine);

class AffineCollector final : public om2::KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const om2::KernelCollectContext &context, om2::KernelProgramInfo &info) override {
    if (context.input_chain_indices.size() != 3U || !context.output_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    const bool requires_source = context.node.GetType() == "Om2ExternalGeneratorSourceAffine";
    auto status = om2::InitKernelProgramInfo(context, om2::OutputCreatePolicy::kGraphBound, requires_source, info);
    if (status != ge::GRAPH_SUCCESS) {
      return status;
    }
    status = om2::SetInputPortInfos(context,
                                    {{"input", om2::Om2ValueType::kGertTensorPtr},
                                     {"parameters", om2::Om2ValueType::kGertTensorPtr},
                                     {"output", om2::Om2ValueType::kGertTensorPtr}},
                                    info);
    if (status != ge::GRAPH_SUCCESS) {
      return status;
    }
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ExternalGeneratorAffine", AffineCollector);
REGISTER_KERNEL_CODE_BUILDER("Om2ExternalGeneratorSourceAffine", AffineCollector);
REGISTER_KERNEL_CODE_BUILDER("Om2ExternalGeneratorFailingAffine", AffineCollector);

ge::FastNode *AddNode(ge::ExecuteGraph &graph, const char *name, const char *type,
                      const std::vector<ge::GeTensorDesc> &inputs, const std::vector<ge::GeTensorDesc> &outputs) {
  auto desc = std::make_shared<ge::OpDesc>(name, type);
  for (const auto &input : inputs) {
    if (desc->AddInputDesc(input) != ge::GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  for (const auto &output : outputs) {
    if (desc->AddOutputDesc(output) != ge::GRAPH_SUCCESS) {
      return nullptr;
    }
  }
  return graph.AddNode(desc);
}

namespace {
bool SetTensorInput(ge::FastNode &node, const int64_t index) {
  return ge::AttrUtils::SetInt(node.GetOpDescPtr(), attr::kIndex, index);
}

bool BindAffineParametersAtLoad(ge::FastNode &parameters) {
  const auto desc = parameters.GetOpDescPtr();
  auto output_desc = desc->GetOutputDesc(0U);
  // Use the same descriptor metadata as a Const emitted by GE lowering.
  if (!ge::AttrUtils::SetInt(output_desc, "om2_weight_index", 0)) {
    return false;
  }
  ge::TensorUtils::SetDataOffset(output_desc, sizeof(float));
  Tensor tensor({{2}, {2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_FLOAT, nullptr);
  tensor.SetSize(2U * sizeof(float));
  return desc->UpdateOutputDesc(0U, output_desc) == ge::GRAPH_SUCCESS &&
         ge::AttrUtils::SetStr(desc, attr::kStage, attr::kStageInit) &&
         ge::AttrUtils::SetZeroCopyBytes(
             parameters.GetOpDescBarePtr(), kConstValue,
             ge::Buffer::CopyFrom(reinterpret_cast<const uint8_t *>(&tensor), sizeof(tensor)));
}

bool SetAffineGraphIo(ge::ExecuteGraph &graph, const std::vector<ge::GeTensor> &input_descs,
                      ge::GeTensorDesc output_desc, ge::FastNode *output) {
  output_desc.SetName("result");
  return ge::AttrUtils::SetListTensor(graph, attr::kModelInputs, input_descs) &&
         ge::AttrUtils::SetListTensor(graph, attr::kModelOutputs, {ge::GeTensor(output_desc)}) &&
         ge::ExecuteGraphUtils::SetGraphOutputs(&graph, {{output, 0}}) == ge::GRAPH_SUCCESS;
}
}  // namespace

std::shared_ptr<ge::ExecuteGraph> AffineGraph(const bool load_bound_parameters, const bool fail_run) {
  auto graph =
      std::make_shared<ge::ExecuteGraph>(load_bound_parameters ? "external_load_bound_affine" : "external_affine");
  ge::GeTensorDesc dynamic_desc(ge::GeShape({-1}), ge::FORMAT_ND, ge::DT_FLOAT);
  // Keep shape dynamic while supplying the supported no-tiling memory bound.
  // Packaging records its maximum byte size; each Run still supplies its actual shape.
  if (dynamic_desc.SetShapeRange({{1, 8}}) != ge::GRAPH_SUCCESS ||
      !ge::AttrUtils::SetBool(dynamic_desc, ge::ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true)) {
    return nullptr;
  }
  const ge::GeTensorDesc parameters_desc(ge::GeShape({2}), ge::FORMAT_ND, ge::DT_FLOAT);
  auto *input = AddNode(*graph, "input", "Data", {}, {dynamic_desc});
  auto *parameters = AddNode(*graph, "parameters", load_bound_parameters ? "Const" : "Data", {}, {parameters_desc});
  auto *output = AddNode(*graph, "output", "OutputData", {}, {dynamic_desc});
  auto *affine =
      AddNode(*graph, "affine", fail_run ? "Om2ExternalGeneratorFailingAffine" : "Om2ExternalGeneratorAffine",
              {dynamic_desc, parameters_desc, dynamic_desc}, {});
  if (input == nullptr || parameters == nullptr || output == nullptr || affine == nullptr) {
    return nullptr;
  }
  if (!SetTensorInput(*input, 0)) {
    return nullptr;
  }
  std::vector<ge::GeTensor> input_descs{ge::GeTensor(dynamic_desc)};
  input_descs.front().MutableTensorDesc().SetName("input");
  if (load_bound_parameters) {
    if (!BindAffineParametersAtLoad(*parameters)) {
      return nullptr;
    }
  } else {
    if (!SetTensorInput(*parameters, 1)) {
      return nullptr;
    }
    input_descs.emplace_back(parameters_desc);
    input_descs.back().MutableTensorDesc().SetName("parameters");
  }
  if (graph->AddEdge(input, 0, affine, 0) == nullptr || graph->AddEdge(parameters, 0, affine, 1) == nullptr ||
      graph->AddEdge(output, 0, affine, 2) == nullptr || !SetAffineGraphIo(*graph, input_descs, dynamic_desc, output)) {
    return nullptr;
  }
  return graph;
}

}  // namespace om2_test
}  // namespace gert
