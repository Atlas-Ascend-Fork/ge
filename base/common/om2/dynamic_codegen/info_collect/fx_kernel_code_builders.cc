/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <algorithm>
#include <iterator>
#include <sstream>

#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"

namespace gert {
namespace om2 {
namespace {
class CreateNotifiesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kGraphBound, false, i));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        c, {{"notifies", Om2ValueType::kTypedContinuousVectorRtNotifyTPtr}, {"notify_num", Om2ValueType::kInt64T}}, i));
    GE_ASSERT_GRAPH_SUCCESS(SetOutputPortInfos(
        c,
        [&]() {
          std::vector<KernelInputPortInfo> out;
          out.reserve(c.output_chain_indices.size());
          for (size_t n = 0U; n < c.output_chain_indices.size(); ++n) {
            out.push_back({"notify_" + std::to_string(n), Om2ValueType::kRtNotifyTPtr});
          }
          return out;
        }(),
        i));
    GE_ASSERT_TRUE(c.input_chain_indices.size() == 2U);
    notifies_ = c.input_chain_indices[0U];
    count_ = c.input_chain_indices[1U];
    outputs_ = c.output_chain_indices;
    return ge::GRAPH_SUCCESS;
  }
  KernelImplType GetImplType() const override {
    return KernelImplType::kCodegenImpl;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    GE_ASSERT_TRUE(notifies_ != kInvalidProgramIndex && count_ != kInvalidProgramIndex);
    std::ostringstream ss;
    ss << "  aclError " << symbol << "() {\n"
       << "    auto *const notifies = chains_[" << notifies_
       << "U].GetValue<gert::TypedContinuousVector<aclrtNotify> *>();\n"
       << "    const auto *const notify_num = chains_[" << count_ << "U].GetPointer<int64_t>();\n"
       << "    if ((notifies == nullptr) || (notify_num == nullptr) || (*notify_num <= 0) ||\n"
       << "        (notifies->GetSize() < static_cast<size_t>(*notify_num)) ||\n"
       << "        (static_cast<size_t>(*notify_num) != " << outputs_.size() << "U)) {\n"
       << "      return kFailed;\n    }\n";
    for (size_t n = 0U; n < outputs_.size(); ++n) {
      ss << "    chains_[" << outputs_[n] << "U].Set(static_cast<void *>(notifies->GetData()[" << n
         << "]), nullptr);\n";
    }
    ss << "    return kOk;\n  }\n";
    code = ss.str();
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint32_t notifies_{kInvalidProgramIndex};
  uint32_t count_{kInvalidProgramIndex};
  std::vector<uint32_t> outputs_;
};
REGISTER_KERNEL_CODE_BUILDER("CreateNotifies", CreateNotifiesKernelCodeBuilder);

class CreateAttachedStreamProviderKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (!context.input_chain_indices.empty() || context.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    return SetOutputPortInfos(context, {{"attached_stream_provider", Om2ValueType::kVoidPtr}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateAttachedStreamProvider", CreateAttachedStreamProviderKernelCodeBuilder);

// Match CustomOpInput and EagerOpExecutionContext in the developing runtime.
constexpr size_t kLaunchTailInputs = 7U;
constexpr size_t kLaunchExtraOutputs = 3U;
void AppendTensorPorts(std::vector<KernelInputPortInfo> *ports, const char *prefix, size_t count) {
  const auto names = MakeIndexedPortNames(prefix, count);
  std::transform(names.begin(), names.end(), std::back_inserter(*ports),
                 [](const std::string &name) { return KernelInputPortInfo{name, Om2ValueType::kGertTensorPtr}; });
}
void AppendLaunchTail(std::vector<KernelInputPortInfo> *ports) {
  ports->push_back({"allocator", Om2ValueType::kGertAllocatorPtr});
  ports->push_back({"stream", Om2ValueType::kRtStreamTPtr});
  ports->push_back({"custom_op", Om2ValueType::kVoidPtr});
  ports->push_back({"attached_stream_provider", Om2ValueType::kVoidPtr});
  ports->push_back({"root_graph_id", Om2ValueType::kUInt32T});
  ports->push_back({"node_id", Om2ValueType::kInt64T});
  ports->push_back({"vector_core_num", Om2ValueType::kInt64T});
}
ge::graphStatus CollectExecuteCustomOp(const KernelCollectContext &c, KernelProgramInfo &i, bool with_shapes) {
  if (c.output_chain_indices.size() < kLaunchExtraOutputs) {
    return ge::GRAPH_PARAM_INVALID;
  }
  const size_t tensor_outputs = c.output_chain_indices.size() - kLaunchExtraOutputs;
  const size_t tail = kLaunchTailInputs + (with_shapes ? tensor_outputs : 0U);
  if (c.input_chain_indices.size() < tail) {
    return ge::GRAPH_PARAM_INVALID;
  }
  const size_t tensor_inputs = c.input_chain_indices.size() - tail;
  GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kOutputsCreator, true, i));
  std::vector<KernelInputPortInfo> inputs;
  inputs.reserve(c.input_chain_indices.size());
  AppendTensorPorts(&inputs, "input_tensor_", tensor_inputs);
  AppendLaunchTail(&inputs);
  if (with_shapes) {
    AppendTensorPorts(&inputs, "template_tensor_", tensor_outputs);
  }
  GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(c, inputs, i));
  auto outputs = MakeIndexedPortNames("output_tensor_", tensor_outputs);
  outputs.push_back("workspace");
  outputs.push_back("args_handler");
  outputs.push_back("context_info");
  return SetOutputPortNames(c, outputs, i);
}
class FindCustomOpKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    if (c.input_chain_indices.size() != 2U || c.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kGraphBound, false, i));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        c, {{"node_type", Om2ValueType::kCStringPtr}, {"custom_op_registry", Om2ValueType::kVoidPtr}}, i));
    return SetOutputPortNames(c, {"custom_op"}, i);
  }
};
class ExecuteCustomOpKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    return CollectExecuteCustomOp(c, i, false);
  }
};
class ExecuteCustomOpWithInferShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    return CollectExecuteCustomOp(c, i, true);
  }
};
class FreeCustomOpWorkspacesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    if (c.input_chain_indices.size() != 2U || !c.output_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kGraphBound, false, i));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        c, {{"workspaces", Om2ValueType::kVoidPtr}, {"allocator", Om2ValueType::kGertAllocatorPtr}}, i));
    return SetOutputPortNames(c, {}, i);
  }
};
class FreeArgsGuarderKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    if (c.input_chain_indices.size() != 1U || !c.output_chain_indices.empty()) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kGraphBound, false, i));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(c, {{"args_handler", Om2ValueType::kVoidPtr}}, i));
    return SetOutputPortNames(c, {}, i);
  }
};
REGISTER_KERNEL_CODE_BUILDER("FindCustomOp", FindCustomOpKernelCodeBuilder);
REGISTER_KERNEL_CODE_BUILDER("ExecuteCustomOp", ExecuteCustomOpKernelCodeBuilder);
REGISTER_KERNEL_CODE_BUILDER("ExecuteCustomOpWithInferShape", ExecuteCustomOpWithInferShapeKernelCodeBuilder);
REGISTER_KERNEL_CODE_BUILDER("FreeCustomOpWorkspaces", FreeCustomOpWorkspacesKernelCodeBuilder);
REGISTER_KERNEL_CODE_BUILDER("FreeArgsGuarder", FreeArgsGuarderKernelCodeBuilder);

class AllocMemoryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &c, KernelProgramInfo &i) override {
    if (c.input_chain_indices.size() != 2U || c.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(c, OutputCreatePolicy::kOutputsCreator, true, i));
    GE_ASSERT_GRAPH_SUCCESS(
        SetInputPortInfos(c, {{"allocator", Om2ValueType::kGertAllocatorPtr}, {"size", Om2ValueType::kSizeT}}, i));
    return SetOutputPortNames(c, {"tensor_data"}, i);
  }
};
REGISTER_KERNEL_CODE_BUILDER("AllocMemory", AllocMemoryKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
