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
#include "securec.h"
#include "graph/utils/attr_utils.h"
#include "host_kernels/reduce_ops/reduce_prod_kernel.h"

namespace ge {
namespace {
void CheckReduceProd(const std::vector<int32_t> &axes, bool scalar_axis, bool no_axis, bool keep_dims, Status status,
                     const std::vector<int64_t> &shape, const std::vector<int32_t> &expected) {
  auto op = std::make_shared<OpDesc>("reduce_prod", "ReduceProd");
  ASSERT_TRUE(AttrUtils::SetBool(op, "keep_dims", keep_dims));
  GeTensorDesc data_desc(GeShape({2, 3}), FORMAT_ND, DT_INT32);
  op->AddOutputDesc(GeTensorDesc(GeShape(shape), FORMAT_ND, DT_INT32));
  const std::vector<int32_t> data = {1, 2, 3, 4, 5, 6};
  std::vector<ConstGeTensorPtr> inputs = {std::make_shared<GeTensor>(
      data_desc, reinterpret_cast<const uint8_t *>(data.data()), data.size() * sizeof(int32_t))};
  if (!no_axis) {
    GeTensorDesc axis_desc(scalar_axis ? GeShape() : GeShape({static_cast<int64_t>(axes.size())}), FORMAT_ND, DT_INT32);
    auto axis_tensor = std::make_shared<GeTensor>(axis_desc);
    if (!axes.empty()) {
      ASSERT_EQ(axis_tensor->SetData(reinterpret_cast<const uint8_t *>(axes.data()), axes.size() * sizeof(int32_t)),
                GRAPH_SUCCESS);
    }
    inputs.push_back(axis_tensor);
  }
  ReduceProdKernel kernel;
  std::vector<GeTensorPtr> outputs;
  ASSERT_EQ(kernel.Compute(op, inputs, outputs), status);
  if (status != SUCCESS) {
    EXPECT_TRUE(outputs.empty());
    return;
  }
  ASSERT_EQ(outputs.size(), 1U);
  EXPECT_EQ(outputs[0]->GetTensorDesc().GetShape().GetDims(), shape);
  EXPECT_EQ(outputs[0]->GetTensorDesc().GetDataType(), DT_INT32);
  ASSERT_EQ(outputs[0]->GetData().size(), expected.size() * sizeof(int32_t));
  for (size_t i = 0; i < expected.size(); ++i) {
    int32_t value;
    ASSERT_EQ(memcpy_s(&value, sizeof(value), outputs[0]->GetData().data() + i * sizeof(value), sizeof(value)), EOK);
    EXPECT_EQ(value, expected[i]);
  }
}
TEST(HostReduceProdAxisCount, MultipleAxesFallBack) {
  CheckReduceProd({0, 1}, false, false, false, NOT_CHANGED, {}, {});
  CheckReduceProd({1, 0}, false, false, false, NOT_CHANGED, {}, {});
}
TEST(HostReduceProdAxisCount, MultipleAxesKeepDimsFallBack) {
  CheckReduceProd({0, 1}, false, false, true, NOT_CHANGED, {}, {});
  CheckReduceProd({1, 0}, false, false, true, NOT_CHANGED, {}, {});
}
TEST(HostReduceProdAxisCount, SingleScalarAndVectorAxes) {
  CheckReduceProd({0}, true, false, false, SUCCESS, {3}, {4, 10, 18});
  CheckReduceProd({1}, false, false, false, SUCCESS, {2}, {6, 120});
  CheckReduceProd({0}, false, false, true, SUCCESS, {1, 3}, {4, 10, 18});
  CheckReduceProd({1}, true, false, true, SUCCESS, {2, 1}, {6, 120});
}
TEST(HostReduceProdAxisCount, EmptyAxesKeepInput) {
  CheckReduceProd({}, false, false, false, SUCCESS, {2, 3}, {1, 2, 3, 4, 5, 6});
  CheckReduceProd({}, false, false, true, SUCCESS, {2, 3}, {1, 2, 3, 4, 5, 6});
}
TEST(HostReduceProdAxisCount, MissingAxesReduceAll) {
  CheckReduceProd({}, false, true, false, SUCCESS, {}, {720});
}
}  // namespace
}  // namespace ge
