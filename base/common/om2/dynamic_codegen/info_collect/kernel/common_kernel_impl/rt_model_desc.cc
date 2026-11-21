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
class GetSpaceRegistryKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"model_desc", Om2ValueType::kGertModelDescPtr},
                                               // GetSpaceRegistry's public run_func reads this scalar as size_t, even
                                               // though the lowering holder stores the enum's 32-bit representation.
                                               {"opp_impl_version", Om2ValueType::kSizeT}},
                                              info));
    RequestInputBinding(context, 0U, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad, info);
    return SetOutputPortNames(context, {"space_registry"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetSpaceRegistry", GetSpaceRegistryKernelCodeBuilder);

class GetSpaceRegistriesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"model_desc", Om2ValueType::kGertModelDescPtr}}, info));
    RequestInputBinding(context, 0U, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad, info);
    return SetOutputPortNames(context, {"space_registries"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetSpaceRegistries", GetSpaceRegistriesKernelCodeBuilder);

class GetFileConstantWeightDirKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"model_desc", Om2ValueType::kGertModelDescPtr}}, info));
    RequestInputBinding(context, 0U, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad, info);
    return SetOutputPortNames(context, {"file_constant_weight_dir"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("GetFileConstantWeightDir", GetFileConstantWeightDirKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
