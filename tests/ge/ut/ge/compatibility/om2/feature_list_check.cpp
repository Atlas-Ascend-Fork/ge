/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software: you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "framework/common/om2_feature_list.h"
#include "framework/om2/model_data/gert_model_data.h"

namespace {
TEST(Om2FeatureListCheck, FeatureList_ShouldSatisfyInvariants_WhenEntriesRegistered) {
  const uint32_t executor_version = gert::ParseVersion(gert::GERT_EXECUTOR_VERSION);
  ASSERT_GT(executor_version, 0U);

  // 清单为空是允许的；清单一旦有条目，就必须满足下面的全部不变量
  uint32_t max_feature_version = 0U;
  const char *max_version_feature = "";
  std::vector<std::string> names;
  for (const auto &feature : gert::kOm2FeatureList) {
    ASSERT_NE(feature.name, nullptr);
    ASSERT_NE(feature.version, nullptr);
    EXPECT_NE(feature.name[0], '\0');
    const uint32_t version = gert::ParseVersion(feature.version);
    EXPECT_GT(version, 0U) << "invalid version of feature " << feature.name;
    if (version > max_feature_version) {
      max_feature_version = version;
      max_version_feature = feature.name;
    }
    EXPECT_EQ(std::find(names.begin(), names.end(), feature.name), names.end()) << "duplicated feature name";
    names.emplace_back(feature.name);
  }

  EXPECT_LE(max_feature_version, executor_version)
      << "feature " << max_version_feature << " requires a newer executor; "
      << "bump GERT_EXECUTOR_VERSION or fix the kOm2FeatureList entry";
}

TEST(Om2FeatureListCheck, ParseVersion_ShouldEncodeMajorAndMinor_WhenInputIsWellFormed) {
  EXPECT_EQ(gert::ParseVersion("1.0"), 10000U);
  EXPECT_EQ(gert::ParseVersion("1.9"), 10009U);
  EXPECT_EQ(gert::ParseVersion("2.0"), 20000U);
  EXPECT_EQ(gert::ParseVersion("abc"), 0U);
  EXPECT_EQ(gert::ParseVersion(""), 0U);
}
}  // namespace
