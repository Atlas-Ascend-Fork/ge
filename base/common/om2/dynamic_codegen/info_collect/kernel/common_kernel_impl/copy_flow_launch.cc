/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"

#include <sstream>

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
class CopyFlowLaunchKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    constexpr size_t kFixedInputCount = 5U;
    constexpr size_t kInputGroupSize = 4U;
    const size_t output_count = context.output_chain_indices.size();
    if (context.input_chain_indices.size() != (kFixedInputCount + output_count * kInputGroupSize)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, true, info));
    std::vector<KernelInputPortInfo> inputs = {{"inputs_num", Om2ValueType::kSizeT},
                                               {"inputs_index", Om2ValueType::kGertContinuousVectorVectorInt32Ptr},
                                               {"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr},
                                               {"stream", Om2ValueType::kRtStreamTPtr},
                                               {"allocator", Om2ValueType::kGertAllocatorPtr}};
    for (size_t i = 0U; i < output_count; ++i) {
      const auto suffix = std::to_string(i);
      inputs.push_back({"address_" + suffix, Om2ValueType::kGertTensorDataPtr});
      inputs.push_back({"length_" + suffix, Om2ValueType::kSizeT});
      inputs.push_back({"shape_" + suffix, Om2ValueType::kGertStorageShapePtr});
      inputs.push_back({"data_type_" + suffix, Om2ValueType::kGeDataType});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    return SetOutputPortNames(context, MakeIndexedPortNames("address_", output_count), info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CopyFlowLaunch", CopyFlowLaunchKernelCodeBuilder);

enum class CopyFlowStage { kCalcSizes, kPrepare, kLaunch };

class SplitCopyFlowKernelCodeBuilder : public KernelCodeBuilder {
 public:
  explicit SplitCopyFlowKernelCodeBuilder(const CopyFlowStage stage) : stage_(stage) {}

  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    int64_t count = 0;
    GE_ASSERT_TRUE(ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "copy_flow_count", count));
    GE_ASSERT_TRUE(count > 0);
    count_ = static_cast<size_t>(count);
    const size_t prefix = stage_ == CopyFlowStage::kCalcSizes ? 2U : (stage_ == CopyFlowStage::kPrepare ? 4U : 3U);
    const size_t input_count = context.input_chain_indices.size();
    GE_ASSERT_TRUE(input_count >= prefix);
    GE_ASSERT_TRUE(((input_count - prefix) % 4U == 0U) && ((input_count - prefix) / 4U == count_));
    const size_t output_count =
        stage_ == CopyFlowStage::kCalcSizes ? 1U : (stage_ == CopyFlowStage::kPrepare ? count_ : 0U);
    GE_ASSERT_TRUE(context.output_chain_indices.size() == output_count);
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(
        context, stage_ == CopyFlowStage::kLaunch ? OutputCreatePolicy::kGraphBound : OutputCreatePolicy::kCodegenImpl,
        false, info));
    std::vector<KernelInputPortInfo> inputs = {{"inputs_num", Om2ValueType::kSizeT}};
    if (stage_ == CopyFlowStage::kCalcSizes) {
      inputs.push_back({"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr});
    } else if (stage_ == CopyFlowStage::kPrepare) {
      inputs.push_back({"inputs_index", Om2ValueType::kGertContinuousVectorVectorInt32Ptr});
      inputs.push_back({"rt_arg", Om2ValueType::kGertRtKernelLaunchArgsExPtr});
      inputs.push_back({"allocated_addrs", Om2ValueType::kGertTensorDataVectorPtr});
    } else {
      inputs.push_back({"stream", Om2ValueType::kRtStreamTPtr});
      inputs.push_back({"allocated_addrs", Om2ValueType::kGertTensorDataVectorPtr});
    }
    for (size_t i = 0U; i < count_; ++i) {
      const auto suffix = std::to_string(i);
      inputs.insert(inputs.end(), {{"address_" + suffix, Om2ValueType::kGertTensorDataPtr},
                                   {"length_" + suffix, Om2ValueType::kSizeT},
                                   {"shape_" + suffix, Om2ValueType::kGertStorageShapePtr},
                                   {"data_type_" + suffix, Om2ValueType::kGeDataType}});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    output_indices_ = context.output_chain_indices;
    return SetOutputPortNames(context, MakeIndexedPortNames("copy_flow_output_", output_count), info);
  }

  ge::graphStatus GenerateOutputsCreatorCode(std::string &code) const override {
    GE_ASSERT_TRUE(!output_indices_.empty());
    std::ostringstream ss;
    if (stage_ == CopyFlowStage::kCalcSizes) {
      ss << "InitCopyFlowSizesOutput(chains_[" << output_indices_[0U] << "U], " << count_ << "U)";
    } else {
      ss << "InitCopyFlowTensorOutputs({";
      for (size_t i = 0U; i < output_indices_.size(); ++i) {
        ss << (i == 0U ? "" : ", ") << "&chains_[" << output_indices_[i] << "U]";
      }
      ss << "})";
    }
    code = ss.str();
    return ge::GRAPH_SUCCESS;
  }

 private:
  CopyFlowStage stage_;
  size_t count_{0U};
  std::vector<uint32_t> output_indices_;
};

class CalcCopyFlowAllocSizesKernelCodeBuilder final : public SplitCopyFlowKernelCodeBuilder {
 public:
  CalcCopyFlowAllocSizesKernelCodeBuilder() : SplitCopyFlowKernelCodeBuilder(CopyFlowStage::kCalcSizes) {}
};
REGISTER_KERNEL_CODE_BUILDER("CalcCopyFlowAllocSizes", CalcCopyFlowAllocSizesKernelCodeBuilder);

class PrepareCopyFlowResultKernelCodeBuilder final : public SplitCopyFlowKernelCodeBuilder {
 public:
  PrepareCopyFlowResultKernelCodeBuilder() : SplitCopyFlowKernelCodeBuilder(CopyFlowStage::kPrepare) {}
};
REGISTER_KERNEL_CODE_BUILDER("PrepareCopyFlowResult", PrepareCopyFlowResultKernelCodeBuilder);

class LaunchCopyFlowH2DKernelCodeBuilder final : public SplitCopyFlowKernelCodeBuilder {
 public:
  LaunchCopyFlowH2DKernelCodeBuilder() : SplitCopyFlowKernelCodeBuilder(CopyFlowStage::kLaunch) {}
};
REGISTER_KERNEL_CODE_BUILDER("LaunchCopyFlowH2D", LaunchCopyFlowH2DKernelCodeBuilder);

class AllocCopyFlowHbmKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    int64_t count = 0;
    GE_ASSERT_TRUE(ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "copy_flow_count", count));
    GE_ASSERT_TRUE(count > 0);
    count_ = static_cast<size_t>(count);
    GE_ASSERT_TRUE(context.output_chain_indices.size() == 1U);
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kCodegenImpl, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context,
        {{"allocator", Om2ValueType::kGertAllocatorPtr}, {"sizes", Om2ValueType::kTypedContinuousVectorSizeTPtr}},
        info));
    output_index_ = context.output_chain_indices[0U];
    return SetOutputPortNames(context, {"allocated_addrs"}, info);
  }

  ge::graphStatus GenerateOutputsCreatorCode(std::string &code) const override {
    GE_ASSERT_TRUE(output_index_ != kInvalidProgramIndex);
    code = "InitCopyFlowAllocOutput(chains_[" + std::to_string(output_index_) + "U], " + std::to_string(count_) + "U)";
    return ge::GRAPH_SUCCESS;
  }

 private:
  size_t count_{0U};
  uint32_t output_index_{kInvalidProgramIndex};
};
REGISTER_KERNEL_CODE_BUILDER("AllocCopyFlowHbm", AllocCopyFlowHbmKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
