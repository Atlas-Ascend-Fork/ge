/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "engine/aicore/autofuse_dispatcher/dispatcher_table.h"

namespace ge {
namespace autofuse_dispatcher {
namespace {
std::string ReadFile(const std::string &path) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}
}  // namespace

class AutofuseDispatcherTableUT : public testing::Test {};

TEST_F(AutofuseDispatcherTableUT, EmptySoDisablesDispatcher) {
  DispatcherTable table;
  EXPECT_EQ(table.LoadGuardSo(""), ge::GRAPH_SUCCESS);
  std::vector<gert::Tensor *> inputs;
  char_t reason[16] = {};
  EXPECT_FALSE(table.Match(inputs, reason, sizeof(reason)));
}

TEST_F(AutofuseDispatcherTableUT, InvalidSoLoadFailsAndMatchMiss) {
  DispatcherTable table;
  EXPECT_NE(table.LoadGuardSo("not a valid so"), ge::GRAPH_SUCCESS);
  std::vector<gert::Tensor *> inputs;
  char_t reason[16] = {};
  EXPECT_FALSE(table.Match(inputs, reason, sizeof(reason)));
}

TEST_F(AutofuseDispatcherTableUT, LoadAndMatchHit) {
  const auto so_data = ReadFile(GUARD_CHECK_UT_STUB_PATH);
  ASSERT_FALSE(so_data.empty());

  DispatcherTable table;
  EXPECT_EQ(table.LoadGuardSo(so_data), ge::GRAPH_SUCCESS);
  std::vector<gert::Tensor *> inputs;
  char_t reason[16] = {};
  EXPECT_TRUE(table.Match(inputs, reason, sizeof(reason)));
}

TEST_F(AutofuseDispatcherTableUT, LoadAndMatchMiss) {
  const auto so_data = ReadFile(GUARD_CHECK_UT_STUB_PATH);
  ASSERT_FALSE(so_data.empty());

  DispatcherTable table;
  EXPECT_EQ(table.LoadGuardSo(so_data), ge::GRAPH_SUCCESS);
  std::vector<gert::Tensor *> inputs(1U, nullptr);
  char_t reason[16] = {};
  EXPECT_FALSE(table.Match(inputs, reason, sizeof(reason)));
}

TEST_F(AutofuseDispatcherTableUT, LoadSoWithoutGuardSymbolFails) {
  const std::vector<std::string> candidates = {"/lib/x86_64-linux-gnu/libm.so.6", "/lib64/libm.so.6",
                                               "/usr/lib/x86_64-linux-gnu/libm.so.6"};
  std::string so_data;
  for (const auto &path : candidates) {
    so_data = ReadFile(path);
    if (!so_data.empty()) {
      break;
    }
  }
  if (so_data.empty()) {
    GTEST_SKIP();
  }
  DispatcherTable table;
  EXPECT_NE(table.LoadGuardSo(so_data), ge::GRAPH_SUCCESS);
}

}  // namespace autofuse_dispatcher
}  // namespace ge
