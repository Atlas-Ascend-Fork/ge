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
#include <cstdlib>
#include <string>
#include "parser/onnx/onnx_constant_parser.h"
#include "parser/onnx/onnx_util.h"
#include "proto/onnx/ge_onnx.pb.h"

namespace ge {
class UtestOnnxConstantParser : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// 覆盖率补充用例：IncCov_OnnxConstantRawDataTooSmall
// 目的：覆盖 raw_data 字节数小于 shape 所需时的拒绝分支。
// 说明：仅通过公开接口构造边界输入，不改动被测逻辑。
TEST_F(UtestOnnxConstantParser, IncCov_OnnxConstantRawDataTooSmall) {
  OnnxConstantParser constant_parser;
  ge::onnx::TensorProto tensor_proto;
  tensor_proto.set_data_type(1);  // ONNX FLOAT
  tensor_proto.add_dims(4);
  tensor_proto.set_raw_data(std::string(4U, '\0'));  // 4 字节 < 16 字节
  ge::Tensor tensor;
  (void)constant_parser.ParseConvertDataType(tensor_proto, tensor);
  EXPECT_EQ(constant_parser.ParseConvertTensor(tensor_proto, tensor), FAILED);
}

// 覆盖率补充用例：IncCov_OnnxConstantInt32DataPath
// 目的：覆盖 int32_data 元素解析路径。
// 说明：仅通过公开接口构造边界输入，不改动被测逻辑。
TEST_F(UtestOnnxConstantParser, IncCov_OnnxConstantInt32DataPath) {
  OnnxConstantParser constant_parser;
  ge::onnx::TensorProto tensor_proto;
  tensor_proto.set_data_type(OnnxDataType::INT32);
  tensor_proto.add_dims(2);
  tensor_proto.add_int32_data(1);
  tensor_proto.add_int32_data(2);
  ge::Tensor tensor;
  EXPECT_EQ(constant_parser.ParseConvertDataType(tensor_proto, tensor), SUCCESS);
  EXPECT_EQ(constant_parser.ParseConvertTensor(tensor_proto, tensor), SUCCESS);
}

}  // namespace ge
