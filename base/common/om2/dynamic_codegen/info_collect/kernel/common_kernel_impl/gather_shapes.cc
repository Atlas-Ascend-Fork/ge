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
#include "exe_graph/runtime/runtime_tensor.h"
#include "graph/def_types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/math_util.h"
#include "graph/utils/type_utils.h"

namespace gert {
namespace om2 {
namespace {
class GatherShapesKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    if (context.output_chain_indices.size() != 1U) {
      return ge::GRAPH_PARAM_INVALID;
    }
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kCodegenImpl, true, info));
    std::vector<KernelInputPortInfo> inputs;
    inputs.reserve(context.input_chain_indices.size());
    for (size_t index = 0U; index < context.input_chain_indices.size(); ++index) {
      inputs.push_back({"shape_" + std::to_string(index), Om2ValueType::kGertStorageShapePtr});
    }
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, inputs, info));
    GE_ASSERT_GRAPH_SUCCESS(SetOutputPortInfos(context, {{"tensor_data", Om2ValueType::kGertTensorDataPtr}}, info));

    uint32_t data_type = ge::DT_INT32;
    (void)ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "dtype", data_type);
    GE_ASSERT_TRUE((data_type == ge::DT_INT32) || (data_type == ge::DT_INT64));
    const auto data_type_size = ge::GetSizeByDataType(static_cast<ge::DataType>(data_type));
    GE_ASSERT_TRUE(data_type_size > 0, "get data type size failed, data_type_size:%d", data_type_size);
    size_t data_size = 0U;
    GE_ASSERT_TRUE(!ge::MulOverflow(static_cast<size_t>(data_type_size), gert::Shape::kMaxDimNum, data_size));

    output_spec_ = {context.output_chain_indices[0U], data_size, gert::kOnHost,
                    context.node.GetOpDescBarePtr()->GetStreamId()};
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus GenerateOutputsCreatorCode(std::string &code) const override {
    GE_ASSERT_TRUE(output_spec_.chain_index != kInvalidProgramIndex);
    std::ostringstream ss;
    ss << "InitInlineGertTensorDataOutput(chains_[" << output_spec_.chain_index << "U], " << output_spec_.data_size
       << "U, static_cast<gert::TensorPlacement>(" << static_cast<int32_t>(output_spec_.placement) << "), "
       << output_spec_.stream_id << ")";
    code = ss.str();
    return ge::GRAPH_SUCCESS;
  }

 private:
  struct OutputSpec {
    size_t chain_index{kInvalidProgramIndex};
    size_t data_size{0U};
    gert::TensorPlacement placement{gert::kOnHost};
    int64_t stream_id{-1};
  };

  OutputSpec output_spec_;
};
REGISTER_KERNEL_CODE_BUILDER("GatherShapesKernel", GatherShapesKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
