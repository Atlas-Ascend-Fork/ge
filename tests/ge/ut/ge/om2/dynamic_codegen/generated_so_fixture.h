/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef GE_TESTS_OM2_GENERATED_SO_FIXTURE_H_
#define GE_TESTS_OM2_GENERATED_SO_FIXTURE_H_

#include "framework/common/gert_model_data_utils.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "common/helper/om2/om2_utils.h"

namespace gert {
namespace om2 {
inline std::string ReadGeneratedFixture(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    ADD_FAILURE() << "Missing generated fixture: " << path;
    return {};
  }
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// The build invokes the current generator and compiler. Tests still generate every
// source artifact and compare it byte-for-byte before executing the compiled model.
// A stale fixture is a failure, never a reason to silently compile during the test.
inline ge::Status LoadGeneratedSoFixture(const std::vector<GertModelDataFile> &artifacts, const std::string &model_name,
                                         GertModelDataFile &so) {
  const auto directory = std::filesystem::path(OM2_GENERATED_FIXTURE_DIR) / model_name;
  for (const auto &artifact : artifacts) {
    const auto expected = ReadGeneratedFixture(directory / GertGetStr(artifact.file_name));
    const std::string actual(reinterpret_cast<const char *>(artifact.data.get()), artifact.data_size);
    if (expected != actual) {
      ADD_FAILURE() << "Stale generated fixture: " << model_name << "/" << GertGetStr(artifact.file_name);
      return ge::FAILED;
    }
  }
  const auto bytes = ReadGeneratedFixture(directory / (model_name + ".so"));
  if (bytes.empty()) {
    return ge::FAILED;
  }
  so.file_name = GertMakeStr(model_name + ".so");
  so.data = GertMakeFileData(bytes.data(), bytes.size());
  so.data_size = bytes.size();
  return ge::SUCCESS;
}
}  // namespace om2
}  // namespace gert
#endif  // GE_TESTS_OM2_GENERATED_SO_FIXTURE_H_
