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
#include "host_kernels/elewise_calculation_ops/floordiv_kernel.h"

namespace ge {
namespace {
template <typename T>
void CheckFloorDiv(DataType dtype, const std::vector<T> &x, const std::vector<T> &y, const std::vector<T> &expected,
                   bool scalar_x = false, bool scalar_y = false, Status expected_status = SUCCESS) {
  auto shape = [](size_t size, bool scalar) { return scalar ? GeShape() : GeShape({static_cast<int64_t>(size)}); };
  GeTensorDesc x_desc(shape(x.size(), scalar_x), FORMAT_ND, dtype);
  GeTensorDesc y_desc(shape(y.size(), scalar_y), FORMAT_ND, dtype);
  auto op = std::make_shared<OpDesc>("floor_div", "FloorDiv");
  op->AddOutputDesc(x_desc);
  std::vector<ConstGeTensorPtr> inputs = {
      std::make_shared<GeTensor>(x_desc, reinterpret_cast<const uint8_t *>(x.data()), x.size() * sizeof(T)),
      std::make_shared<GeTensor>(y_desc, reinterpret_cast<const uint8_t *>(y.data()), y.size() * sizeof(T))};
  FloorDivKernel kernel;
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

TEST(HostFloorDivArithmetic, NegativeFractions) {
  CheckFloorDiv<float>(DT_FLOAT, {-2.5F, -0.5F, 2.5F, -2.5F, 0.F}, {2.F, 2.F, -2.F, -2.F, -2.F},
                       {-2.F, -1.F, -2.F, 1.F, 0.F});
  CheckFloorDiv<double>(DT_DOUBLE, {-2.5, -0.5, 2.5, -2.5, 0.}, {2., 2., -2., -2., -2.}, {-2., -1., -2., 1., 0.});
}
TEST(HostFloorDivArithmetic, FloatBroadcastBothWays) {
  CheckFloorDiv<float>(DT_FLOAT, {-2.5F, -0.5F}, {2.F}, {-2.F, -1.F}, false, true);
  CheckFloorDiv<double>(DT_DOUBLE, {0.5}, {-2., 2.}, {-1., 0.}, true, false);
}
TEST(HostFloorDivArithmetic, WideInt64Quotients) {
  CheckFloorDiv<int64_t>(DT_INT64, {INT64_C(4294967296), INT64_C(9007199254740993), INT64_MAX}, {1, 1, 3},
                         {INT64_C(4294967296), INT64_C(9007199254740993), INT64_C(3074457345618258602)});
}
TEST(HostFloorDivArithmetic, IntegerSignsAndWidths) {
  CheckFloorDiv<int8_t>(DT_INT8, {-9, 9, -9, 8, 0}, {5, -5, -5, 2, -2}, {-2, -2, 1, 4, 0});
  CheckFloorDiv<int16_t>(DT_INT16, {-9, 9, -9}, {5, -5, -5}, {-2, -2, 1});
  CheckFloorDiv<int32_t>(DT_INT32, {-9, 9, -9}, {5, -5, -5}, {-2, -2, 1});
  CheckFloorDiv<uint8_t>(DT_UINT8, {0, 255}, {1, 2}, {0, 127});
  CheckFloorDiv<uint16_t>(DT_UINT16, {0, 65535}, {1, 2}, {0, 32767});
}
TEST(HostFloorDivArithmetic, RepresentableExtremes) {
  CheckFloorDiv<int64_t>(DT_INT64, {INT64_MIN, INT64_MIN, INT64_MIN, INT64_MAX, INT64_MAX},
                         {1, 3, INT64_MIN, -2, INT64_MIN},
                         {INT64_MIN, INT64_C(-3074457345618258603), 1, INT64_C(-4611686018427387904), -1});
  CheckFloorDiv<int32_t>(DT_INT32, {INT32_MIN, INT32_MIN, INT32_MAX}, {1, 3, INT32_MIN}, {INT32_MIN, -715827883, -1});
}
TEST(HostFloorDivArithmetic, IntegerBroadcastBothWays) {
  CheckFloorDiv<int64_t>(DT_INT64, {INT64_MIN, INT64_MAX}, {3},
                         {INT64_C(-3074457345618258603), INT64_C(3074457345618258602)}, false, true);
  CheckFloorDiv<int64_t>(DT_INT64, {INT64_MIN}, {1, 3}, {INT64_MIN, INT64_C(-3074457345618258603)}, true, false);
}
TEST(HostFloorDivArithmetic, UnrepresentableQuotientFallsBack) {
  CheckFloorDiv<int64_t>(DT_INT64, {INT64_MIN}, {-1}, {}, true, true, NOT_CHANGED);
  CheckFloorDiv<int32_t>(DT_INT32, {INT32_MIN, 1}, {-1}, {}, false, true, NOT_CHANGED);
  CheckFloorDiv<int16_t>(DT_INT16, {INT16_MIN}, {-1, 1}, {}, true, false, NOT_CHANGED);
  CheckFloorDiv<int8_t>(DT_INT8, {INT8_MIN}, {-1}, {}, false, false, NOT_CHANGED);
}
TEST(HostFloorDivArithmetic, ExistingZeroFallback) {
  CheckFloorDiv<int64_t>(DT_INT64, {1}, {0}, {}, true, true, NOT_CHANGED);
  CheckFloorDiv<float>(DT_FLOAT, {1.F, 2.F}, {std::numeric_limits<float>::epsilon() / 2.F}, {}, false, true,
                       NOT_CHANGED);
  CheckFloorDiv<double>(DT_DOUBLE, {1.}, {0., std::numeric_limits<double>::epsilon() / 2.}, {}, true, false,
                        NOT_CHANGED);
}
}  // namespace
}  // namespace ge
