/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_TESTS_OM2_COMPILE_STUB_H_
#define GE_TESTS_OM2_COMPILE_STUB_H_

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "common/helper/om2/om2_utils.h"
#include "framework/common/gert_model_data_utils.h"

namespace ge {
namespace om2_test {
// Assembly tests retain collection, code generation and source publication. Only the
// compiler boundary returns a placeholder; these bytes must never be loaded as an SO.
class Om2CompileStub {
 public:
  Om2CompileStub() {
    MOCKER(Om2Utils::CompileGeneratedCppToSo).expects(once()).will(invoke(Compile));
  }
  ~Om2CompileStub() {
    EXPECT_NO_THROW(GlobalMockObject::verify());
    GlobalMockObject::reset();
  }

 private:
  static Status Compile(const std::vector<gert::GertModelDataFile> &artifacts, const Om2CompileInfo &info,
                        gert::GertModelDataFile &so, const bool is_release) {
    EXPECT_FALSE(artifacts.empty());
    EXPECT_FALSE(info.model_name.empty());
    EXPECT_FALSE(info.artifact_name.empty());
    EXPECT_FALSE(is_release);
    for (const auto &artifact : artifacts) {
      EXPECT_NE(artifact.file_name, nullptr);
      EXPECT_NE(artifact.data, nullptr);
      EXPECT_GT(artifact.data_size, 0U);
    }
    constexpr uint8_t placeholder[] = {'O', 'M', '2'};
    so.file_name = gert::GertMakeStr("lib" + info.artifact_name + "_om2.so");
    so.data = gert::GertMakeFileData(placeholder, sizeof(placeholder));
    so.data_size = sizeof(placeholder);
    return SUCCESS;
  }
};
}  // namespace om2_test
}  // namespace ge
#endif  // GE_TESTS_OM2_COMPILE_STUB_H_
