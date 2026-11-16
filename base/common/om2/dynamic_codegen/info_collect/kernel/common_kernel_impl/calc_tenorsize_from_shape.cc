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
ge::graphStatus CollectTensorSize(const KernelCollectContext &context, const char *const shape_name,
                                  const Om2ValueType shape_type, KernelProgramInfo &info) {
  GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, true, info));
  GE_ASSERT_GRAPH_SUCCESS(
      SetInputPortInfos(context, {{"data_type", Om2ValueType::kGeDataType}, {shape_name, shape_type}}, info));
  return SetOutputPortNames(context, {"tensor_size"}, info);
}

class CalcTensorSizeFromShapeKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectTensorSize(context, "shape", Om2ValueType::kGertShapePtr, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CalcTensorSizeFromShape", CalcTensorSizeFromShapeKernelCodeBuilder);

class CalcTensorSizeFromStorageKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectTensorSize(context, "storage_shape", Om2ValueType::kGertStorageShapePtr, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CalcTensorSizeFromStorage", CalcTensorSizeFromStorageKernelCodeBuilder);

class CalcUnalignedTensorSizeFromStorageKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    return CollectTensorSize(context, "storage_shape", Om2ValueType::kGertStorageShapePtr, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CalcUnalignedTensorSizeFromStorage", CalcUnalignedTensorSizeFromStorageKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
