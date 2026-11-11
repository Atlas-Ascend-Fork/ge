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

#include "common/ge_common/error_codes_define.h"
#include "common/option_supportion_checker/option_supportion_checker.h"

namespace ge {
namespace {
// 控核参数单值展开会自动派生并写回 ge.vectorcoreNum，三个正式入口必须放行该选项，避免误导性告警。
TEST(OptionSupportionCheckerUT, VectorCoreNumIsSupportedByGlobalEntryOptions) {
  const std::map<std::string, std::string> options{{"ge.vectorcoreNum", "8"}};
  EXPECT_EQ(IrbuildCheckSupportedGlobalOptions(options), SUCCESS);
  EXPECT_EQ(GEAPICheckSupportedGlobalOptions(options), SUCCESS);
  EXPECT_EQ(GEAPICheckSupportedSessionOptions(options), SUCCESS);
}

// graph 级不在本期放行范围：AddGraph/BuildGraph 传该选项仍应告警。
TEST(OptionSupportionCheckerUT, VectorCoreNumIsStillUnsupportedByGraphOptions) {
  const std::map<std::string, std::string> options{{"ge.vectorcoreNum", "8"}};
  EXPECT_NE(GEAPICheckSupportedGraphOptions(options), SUCCESS);
}

TEST(OptionSupportionCheckerUT, UnknownOptionIsStillUnsupported) {
  const std::map<std::string, std::string> options{{"ge.aicoreNumUnsupported", "8"}};
  EXPECT_NE(GEAPICheckSupportedGlobalOptions(options), SUCCESS);
  EXPECT_NE(GEAPICheckSupportedSessionOptions(options), SUCCESS);
}
}  // namespace
}  // namespace ge
