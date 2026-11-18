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

#include <map>
#include <string>

#include "common/helper/om2/om2_utils.h"

namespace ge {
namespace {

class Om2UtilsTest : public testing::Test {};

TEST_F(Om2UtilsTest, OptionNotSet_ReturnsFalse) {
  EXPECT_FALSE(IsOm2OnlineMode({}));
}

TEST_F(Om2UtilsTest, ModelFormatOm_ReturnsFalse) {
  EXPECT_FALSE(IsOm2OnlineMode({{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM)}}, false));
}

TEST_F(Om2UtilsTest, ModelFormatOm2_ReturnsTrue) {
  EXPECT_TRUE(IsOm2OnlineMode({{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM2)}}, false));
}

TEST_F(Om2UtilsTest, InvalidModelFormat_ReturnsFalse) {
  EXPECT_FALSE(IsOm2OnlineMode({{OPTION_MODEL_FORMAT, "invalid"}}, false));
}

TEST_F(Om2UtilsTest, ExecuteGraph_ReturnsTrue) {
  EXPECT_TRUE(IsOm2OnlineMode({}, true));
}

TEST_F(Om2UtilsTest, ThreadLocalModelFormatOm2_ReturnsTrue) {
  GetThreadLocalContext().SetGraphOption({{OPTION_MODEL_FORMAT, std::to_string(MODEL_FORMAT_OM2)}});
  EXPECT_TRUE(IsOm2OnlineMode());
  GetThreadLocalContext().SetGraphOption({});
}

}  // namespace
}  // namespace ge
