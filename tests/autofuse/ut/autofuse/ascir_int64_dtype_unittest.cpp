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
#include "ascir_ops.h"

namespace ge {
namespace {
class AscirInt64DtypeTest : public testing::TestWithParam<std::string> {};

TEST_P(AscirInt64DtypeTest, BroadcastPreservesInt64) {
  std::vector<DataType> outputs;
  ASSERT_EQ(af::ascir_op::Broadcast::InferDataType({DT_INT64}, outputs, GetParam()), SUCCESS);
  EXPECT_EQ(outputs, std::vector<DataType>({DT_INT64}));
  EXPECT_EQ(af::ascir_op::Broadcast::InferDataType({DT_INT64}, outputs, GetParam()), SUCCESS);
  outputs = {DT_INT32};
  EXPECT_NE(af::ascir_op::Broadcast::InferDataType({DT_INT64}, outputs, GetParam()), SUCCESS);
}

TEST_P(AscirInt64DtypeTest, CastUint8ToInt64) {
  std::vector<DataType> outputs = {DT_INT64};
  ASSERT_EQ(af::ascir_op::Cast::InferDataType({DT_UINT8}, outputs, GetParam()), SUCCESS);
  EXPECT_EQ(outputs, std::vector<DataType>({DT_INT64}));
}

TEST_P(AscirInt64DtypeTest, LogicalInt64ProducesUint8) {
  for (const auto infer : {af::ascir_op::LogicalAnd::InferDataType, af::ascir_op::LogicalOr::InferDataType}) {
    std::vector<DataType> outputs;
    ASSERT_EQ(infer({DT_INT64, DT_INT64}, outputs, GetParam()), SUCCESS);
    EXPECT_EQ(outputs, std::vector<DataType>({DT_UINT8}));
    EXPECT_EQ(infer({DT_INT64, DT_INT64}, outputs, GetParam()), SUCCESS);
    outputs = {DT_INT64};
    EXPECT_NE(infer({DT_INT64, DT_INT64}, outputs, GetParam()), SUCCESS);
    outputs.clear();
    EXPECT_NE(infer({DT_INT64, DT_INT32}, outputs, GetParam()), SUCCESS);
  }
}

INSTANTIATE_TEST_SUITE_P(NpuArch, AscirInt64DtypeTest, testing::Values("2201", "3510", "5102"));

TEST(AscirInt64DtypeScopeTest, ArithmeticRemainsUnsupportedOn2201) {
  std::vector<DataType> outputs;
  EXPECT_NE(af::ascir_op::Add::InferDataType({DT_INT64, DT_INT64}, outputs, "2201"), SUCCESS);
  outputs.clear();
  EXPECT_NE(af::ascir_op::Mul::InferDataType({DT_INT64, DT_INT64}, outputs, "2201"), SUCCESS);
  outputs.clear();
  EXPECT_NE(af::ascir_op::Sum::InferDataType({DT_INT64}, outputs, "2201"), SUCCESS);
}
}  // namespace
}  // namespace ge
