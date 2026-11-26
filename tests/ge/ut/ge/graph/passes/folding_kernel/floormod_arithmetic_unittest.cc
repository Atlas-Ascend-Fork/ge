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
#include <limits>
#include "securec.h"
#include "host_kernels/elewise_calculation_ops/floormod_kernel.h"

namespace ge {
namespace {
template <typename T>
void CheckFloorMod(DataType dtype, const std::vector<T> &x, const std::vector<T> &y, const std::vector<T> &expected,
                   bool scalar_x = false, bool scalar_y = false, Status expected_status = SUCCESS) {
  auto shape = [](size_t size, bool scalar) { return scalar ? GeShape() : GeShape({static_cast<int64_t>(size)}); };
  GeTensorDesc x_desc(shape(x.size(), scalar_x), FORMAT_ND, dtype);
  GeTensorDesc y_desc(shape(y.size(), scalar_y), FORMAT_ND, dtype);
  auto op = std::make_shared<OpDesc>("floor_div", "FloorMod");
  op->AddOutputDesc(x_desc);
  std::vector<ConstGeTensorPtr> inputs = {
      std::make_shared<GeTensor>(x_desc, reinterpret_cast<const uint8_t *>(x.data()), x.size() * sizeof(T)),
      std::make_shared<GeTensor>(y_desc, reinterpret_cast<const uint8_t *>(y.data()), y.size() * sizeof(T))};
  FloorModKernel kernel;
  std::vector<GeTensorPtr> outputs;
  ASSERT_EQ(kernel.Compute(op, inputs, outputs), expected_status);
  if (expected_status != SUCCESS) {
    EXPECT_TRUE(outputs.empty());
    return;
  }
  ASSERT_EQ(outputs.size(), 1U);
  EXPECT_EQ(outputs[0]->GetTensorDesc().GetDataType(), dtype);
  const auto expected_shape = scalar_x ? y_desc.GetShape() : x_desc.GetShape();
  EXPECT_EQ(outputs[0]->GetTensorDesc().GetShape().GetDims(), expected_shape.GetDims());
  ASSERT_EQ(outputs[0]->GetData().size(), expected.size() * sizeof(T));
  for (size_t i = 0; i < expected.size(); ++i) {
    T value;
    ASSERT_EQ(memcpy_s(&value, sizeof(value), outputs[0]->GetData().data() + i * sizeof(T), sizeof(T)), EOK);
    EXPECT_EQ(value, expected[i]) << "element " << i;
  }
}

TEST(HostFloorModArithmetic, ZeroNegativeDivisorScalar) {
  CheckFloorMod<int32_t>(DT_INT32, {0}, {-3}, {0}, true, true);
}
TEST(HostFloorModArithmetic, ZeroVectorAndBroadcast) {
  CheckFloorMod<int32_t>(DT_INT32, {0, 0, 0, 0}, {-3, -2, -1, 3}, {0, 0, 0, 0});
  CheckFloorMod<int32_t>(DT_INT32, {0}, {-3, -2, -1, 3}, {0, 0, 0, 0}, true, false);
  CheckFloorMod<int32_t>(DT_INT32, {0, 6, -6}, {-3}, {0, 0, 0}, false, true);
}
TEST(HostFloorModArithmetic, SignsAndExactMultiples) {
  CheckFloorMod<int32_t>(DT_INT32, {4, 4, 4, -4, -4, -4, 6, -6}, {1, -3, 3, -3, 3, -2, -3, 3},
                         {0, -2, 1, -1, 2, 0, 0, 0});
}
TEST(HostFloorModArithmetic, Int32Extremes) {
  CheckFloorMod<int32_t>(DT_INT32, {INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX, 1, -1, 0},
                         {-1, 3, -3, INT32_MIN, -3, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MIN},
                         {0, 1, -2, 0, -2, -1, -2147483647, 2147483646, 0});
}
TEST(HostFloorModArithmetic, ExtremeBroadcastBothWays) {
  CheckFloorMod<int32_t>(DT_INT32, {INT32_MIN}, {-1, 3, -3}, {0, 1, -2}, true, false);
  CheckFloorMod<int32_t>(DT_INT32, {INT32_MIN, INT32_MAX, 0}, {-1}, {0, 0, 0}, false, true);
}
TEST(HostFloorModArithmetic, ZeroDivisorFallsBack) {
  CheckFloorMod<int32_t>(DT_INT32, {1}, {0}, {}, true, true, NOT_CHANGED);
  CheckFloorMod<int32_t>(DT_INT32, {1, 2}, {0}, {}, false, true, NOT_CHANGED);
  CheckFloorMod<int32_t>(DT_INT32, {1}, {2, 0}, {}, true, false, NOT_CHANGED);
  CheckFloorMod<int32_t>(DT_INT32, {1}, {0, 2}, {}, true, false, NOT_CHANGED);
  CheckFloorMod<int32_t>(DT_INT32, {1}, {-3, 0, 5}, {}, true, false, NOT_CHANGED);
}
}  // namespace
}  // namespace ge
