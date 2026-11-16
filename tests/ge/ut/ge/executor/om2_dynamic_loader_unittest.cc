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
#include <dlfcn.h>
#include "mockcpp/mockcpp.hpp"
#include "exe_graph/runtime/tensor.h"
#include "om2_dynamic_loader.h"
#include "framework/common/gert_model_data_utils.h"

namespace gert {
namespace {
class Om2DynamicLoaderFailureUT : public testing::Test {
 protected:
  void SetUp() override {
    library_handle_ = dlopen("libc.so.6", RTLD_NOW | RTLD_LOCAL);
    ASSERT_NE(library_handle_, nullptr);
    // ASAN's dlopen/dlclose trampolines can be shorter than mockcpp's jump patch and overlap.
    // Patch the libc implementations instead, leaving the sanitizer trampolines intact.
    open_library_ = reinterpret_cast<decltype(&dlopen)>(dlsym(library_handle_, "dlopen"));
    close_library_ = reinterpret_cast<decltype(&dlclose)>(dlsym(library_handle_, "dlclose"));
    ASSERT_NE(open_library_, nullptr);
    ASSERT_NE(close_library_, nullptr);
  }

  void TearDown() override {
    EXPECT_NO_THROW(GlobalMockObject::verify());
    GlobalMockObject::reset();
    if (library_handle_ != nullptr) {
      EXPECT_EQ(dlclose(library_handle_), 0);
      library_handle_ = nullptr;
    }
  }
  void *library_handle_ = nullptr;
  decltype(&dlopen) open_library_ = nullptr;
  decltype(&dlclose) close_library_ = nullptr;
};

TEST_F(Om2DynamicLoaderFailureUT, MissingPluginDoesNotPublishContextOrCallMissingFunctions) {
  // Use a local loader so its once_flag cannot poison other tests' singleton.
  Om2DynamicApi api;
  MOCKER(open_library_).expects(once()).will(returnValue(static_cast<void *>(nullptr)));
  GertModelData data;
  InitGertModelData(data);
  InitGertModelDataUnit(data);
  Om2DynamicCreateInput input;
  input.model_data = &data;
  Om2DynamicCreateOutput output;
  EXPECT_NE(api.Create(input, output), ge::SUCCESS);
  EXPECT_EQ(output.context, nullptr);
  EXPECT_NE(api.Create(input, output), ge::SUCCESS);
  EXPECT_EQ(output.context, nullptr);
  api.Destroy(nullptr);
}

TEST_F(Om2DynamicLoaderFailureUT, MissingExportClosesLibraryWithoutPublishingPartialApi) {
  Om2DynamicApi api;
  // Use a real library without OM2 exports; globally mocking dlsym can break runtime symbol resolution.
  for (const auto *symbol : {"Om2DynamicCreate", "Om2DynamicPrepareConstants", "Om2DynamicPrepareLoad",
                             "Om2DynamicPrepareRun", "Om2DynamicDestroy"}) {
    ASSERT_EQ(dlsym(library_handle_, symbol), nullptr);
  }
  MOCKER(open_library_).expects(once()).will(returnValue(library_handle_));
  // Verify the close request here; TearDown releases the real handle after resetting the mocks.
  MOCKER(close_library_).expects(once()).with(eq(library_handle_)).will(returnValue(0));
  GertModelData data;
  InitGertModelData(data);
  InitGertModelDataUnit(data);
  Om2DynamicCreateInput input;
  input.model_data = &data;
  Om2DynamicCreateOutput output;
  EXPECT_NE(api.Create(input, output), ge::SUCCESS);
  EXPECT_EQ(output.context, nullptr);
  api.Destroy(nullptr);
}
}  // namespace
}  // namespace gert
