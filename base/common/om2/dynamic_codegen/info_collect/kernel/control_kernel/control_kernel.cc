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

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"

namespace gert {
namespace om2 {
namespace {
class NoOpKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortNames(context, {}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("NoOp", NoOpKernelCodeBuilder);

// Runtime contract (runtime/v2/kernel/control_kernel/control_kernel.cc): the RT2
// SwitchNotify/BranchPivot/BranchDone/WaitAnyone skeleton is contracted into the parent-region
// If step by ExecutionPlanCollector::AnalyzeIf, so only the index/condition generators that feed
// the frame go through normal kernel collection. Their run funcs read the operands directly and
// never touch compute_node_info, and none of them registers an outputs creator, so the outputs
// are graph-bound with explicit value types (the consumer is the contracted If step, which
// declares no input port type of its own).
class GenIndexForIfKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"cond", Om2ValueType::kGertTensorPtr}}, info));
    return SetOutputPortInfos(context, {{"branch_index", Om2ValueType::kUInt32T}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GenIndexForIf", GenIndexForIfKernelCodeBuilder);

class GenIndexForCaseKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"cond", Om2ValueType::kGertTensorPtr}, {"branch_num", Om2ValueType::kUInt32T}}, info));
    return SetOutputPortInfos(context, {{"branch_index", Om2ValueType::kUInt32T}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GenIndexForCase", GenIndexForCaseKernelCodeBuilder);

class GenCondForWhileKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"data", Om2ValueType::kGertTensorDataPtr},
                                               {"shape", Om2ValueType::kGertStorageShapePtr},
                                               {"data_type", Om2ValueType::kGeDataType}},
                                              info));
    return SetOutputPortInfos(context, {{"cond", Om2ValueType::kBool}}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GenCondForWhile", GenCondForWhileKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
