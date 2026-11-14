/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstddef>

#include "gtest/gtest.h"
#include "framework/om2/model_data/gert_model_data.h"
#include "framework/common/gert_model_data_utils.h"
#include "om2/model_api/om2_model_api.h"
#include "common/om2/om2_model_api_utils.h"

namespace {
TEST(Om2ModelExecutionTypeCompatibility, OldAndPartialModelSizesDefaultToStatic) {
  gert::GertModelDataModel model;
  model.execution_type = gert::GertModelExecutionType::kDynamic;
  for (const uint64_t old_size : {48U, 55U}) {
    model.struct_size = old_size;
    EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kStatic);
  }
}

TEST(Om2ModelExecutionTypeCompatibility, NewModelDefaultsToStaticAndPreservesDynamic) {
  gert::GertModelDataModel model;
  EXPECT_EQ(model.struct_size, sizeof(gert::GertModelDataModel));
  EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kStatic);
  model.execution_type = gert::GertModelExecutionType::kDynamic;
  EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kDynamic);
  model.struct_size += sizeof(uint64_t);
  EXPECT_EQ(gert::GetGertModelExecutionType(model), gert::GertModelExecutionType::kDynamic);
}

TEST(Om2ConstPlacementAbiCompatibility, OldAndPartialMetadataDoNotExposePlacement) {
  gert::GertModelDataConstMeta meta;
  meta.placement = gert::GertModelDataConstPlacement::kHost;
  for (const uint64_t old_size : {64U, 71U}) {
    meta.struct_size = old_size;
    EXPECT_EQ(gert::GetGertModelConstPlacement(meta), gert::GertModelDataConstPlacement::kUnspecified);
  }
}

TEST(Om2ConstPlacementAbiCompatibility, DefaultsToUnspecifiedAndPreservesExplicitPlacement) {
  gert::GertModelDataConstMeta meta;
  EXPECT_EQ(gert::GetGertModelConstPlacement(meta), gert::GertModelDataConstPlacement::kUnspecified);
  meta.placement = gert::GertModelDataConstPlacement::kDevice;
  EXPECT_EQ(gert::GetGertModelConstPlacement(meta), gert::GertModelDataConstPlacement::kDevice);
  meta.placement = gert::GertModelDataConstPlacement::kHost;
  meta.struct_size = sizeof(meta) + sizeof(uint64_t);
  EXPECT_EQ(gert::GetGertModelConstPlacement(meta), gert::GertModelDataConstPlacement::kHost);
}

TEST(Om2ExternalAllocatorAbiCompatibility, OldAndPartialLoadConfigDoNotExposeAllocator) {
  uint64_t allocator_storage = 0U;
  GertModelLoadConfig config;
  config.external_allocator = &allocator_storage;
  for (const uint64_t old_size : {184U, 191U}) {
    config.struct_size = old_size;
    EXPECT_EQ(gert::om2::GetExternalAllocator(config), nullptr);
  }
}

TEST(Om2ExternalAllocatorAbiCompatibility, DynamicConfigDoesNotChangeAllocatorAccess) {
  GertModelLoadConfig config;
  uint64_t allocator_storage = 0U;
  config.external_allocator = &allocator_storage;
  GertModelDynamicLoadConfig dynamic;
  config.dynamic_config = &dynamic;
  config.struct_size = offsetof(GertModelLoadConfig, dynamic_config);
  EXPECT_EQ(gert::om2::GetExternalAllocator(config), &allocator_storage);
  EXPECT_EQ(gert::om2::GetDynamicConfig(config), nullptr);
  config.struct_size = sizeof(config) + sizeof(uint64_t);
  EXPECT_EQ(gert::om2::GetExternalAllocator(config), &allocator_storage);
  EXPECT_EQ(gert::om2::GetDynamicConfig(config), &dynamic);
}

TEST(Om2ExternalAllocatorAbiCompatibility, RunConfigReadsOnlyCompleteOptionalAllocatorField) {
  GertModelRunConfig config;
  EXPECT_EQ(gert::om2::GetExternalAllocator(config), nullptr);
  uint64_t allocator_storage = 0U;
  config.external_allocator = &allocator_storage;
  for (const uint64_t old_size : {56U, 63U}) {
    config.struct_size = old_size;
    EXPECT_EQ(gert::om2::GetExternalAllocator(config), nullptr);
  }
  config.struct_size = sizeof(GertModelRunConfig);
  EXPECT_EQ(gert::om2::GetExternalAllocator(config), &allocator_storage);
  config.struct_size += sizeof(uint64_t);
  EXPECT_EQ(gert::om2::GetExternalAllocator(config), &allocator_storage);
}

TEST(Om2DynamicLoadAbiCompatibility, OldAndPartialOuterSizesNeverDereferenceDynamicPointer) {
  GertModelLoadConfig config;
  config.dynamic_config = reinterpret_cast<const GertModelDynamicLoadConfig *>(uintptr_t{1U});
  for (const uint64_t old_size : {184U, 192U, 199U}) {
    config.struct_size = old_size;
    EXPECT_EQ(gert::om2::GetDynamicConfig(config), nullptr);
    EXPECT_EQ(gert::om2::GetSharedLibraries(config), nullptr);
    EXPECT_EQ(gert::om2::GetRtSession(config), nullptr);
  }
}

TEST(Om2DynamicLoadAbiCompatibility, NullAndDefaultDynamicConfigExposeNoResources) {
  GertModelLoadConfig config;
  EXPECT_EQ(gert::om2::GetDynamicConfig(config), nullptr);
  EXPECT_EQ(gert::om2::GetSharedLibraries(config), nullptr);
  EXPECT_EQ(gert::om2::GetRtSession(config), nullptr);
  GertModelDynamicLoadConfig dynamic;
  EXPECT_EQ(dynamic.struct_size, sizeof(dynamic));
  config.dynamic_config = &dynamic;
  EXPECT_EQ(gert::om2::GetSharedLibraries(config), nullptr);
  EXPECT_EQ(gert::om2::GetRtSession(config), nullptr);
}

TEST(Om2DynamicLoadAbiCompatibility, InnerSizesGuardEachFieldIndependently) {
  GertModelLoadConfig config;
  GertModelDynamicLoadConfig dynamic;
  GertModelSharedLibraries libraries;
  uint64_t session_storage = 0U;
  dynamic.shared_libraries = &libraries;
  dynamic.rt_session = &session_storage;
  config.dynamic_config = &dynamic;
  for (const uint64_t size : {0U, 8U, 15U}) {
    dynamic.struct_size = size;
    EXPECT_EQ(gert::om2::GetSharedLibraries(config), nullptr);
    EXPECT_EQ(gert::om2::GetRtSession(config), nullptr);
  }
  for (const uint64_t size : {16U, 23U}) {
    dynamic.struct_size = size;
    EXPECT_EQ(gert::om2::GetSharedLibraries(config), &libraries);
    EXPECT_EQ(gert::om2::GetRtSession(config), nullptr);
  }
  for (const uint64_t size : {24U, 32U}) {
    dynamic.struct_size = size;
    EXPECT_EQ(gert::om2::GetSharedLibraries(config), &libraries);
    EXPECT_EQ(gert::om2::GetRtSession(config), &session_storage);
  }
  config.struct_size += sizeof(uint64_t);
  EXPECT_EQ(gert::om2::GetSharedLibraries(config), &libraries);
  EXPECT_EQ(gert::om2::GetRtSession(config), &session_storage);
}

TEST(Om2SharedLibraryAbiCompatibility, OptionalListDefaultsAndExplicitEmptyList) {
  GertModelLoadConfig config;
  GertModelDynamicLoadConfig dynamic;
  config.dynamic_config = &dynamic;
  EXPECT_EQ(gert::om2::GetSharedLibraries(config), nullptr);
  GertModelSharedLibraries libraries;
  EXPECT_TRUE(gert::om2::IsValidSharedLibraries(libraries));
  dynamic.shared_libraries = &libraries;
  EXPECT_EQ(gert::om2::GetSharedLibraries(config), &libraries);
  EXPECT_TRUE(gert::om2::IsValidSharedLibraries(*gert::om2::GetSharedLibraries(config)));
}

TEST(Om2SharedLibraryAbiCompatibility, ValidDescriptorsBorrowOriginalDataAndSupportLargerStructs) {
  const uint8_t data[] = {1U, 0U, 2U};
  GertModelSharedLibrary library;
  library.so_name = "libtest.so";
  library.so_bin_type = GERT_MODEL_SHARED_LIBRARY_SPACE_REGISTRY;
  library.vendor_name = "/opp_latest/vendor";
  library.data = data;
  library.data_size = sizeof(data);
  ASSERT_TRUE(gert::om2::IsValidSharedLibrary(library));
  EXPECT_EQ(library.data, data);
  struct ExtendedLibrary {
    GertModelSharedLibrary base;
    uint64_t extra = 0U;
  };
  ExtendedLibrary extended;
  extended.base = library;
  extended.base.struct_size = sizeof(ExtendedLibrary);
  const GertModelSharedLibrary *entries[] = {&library, &extended.base};
  GertModelSharedLibraries libraries;
  libraries.library_num = 2U;
  libraries.libraries = entries;
  EXPECT_TRUE(gert::om2::IsValidSharedLibraries(libraries));
}

TEST(Om2SharedLibraryAbiCompatibility, RejectsMissingFieldsAndInvalidDescriptors) {
  const uint8_t data[] = {1U};
  GertModelSharedLibrary library;
  library.so_name = "libtest.so";
  library.so_bin_type = GERT_MODEL_SHARED_LIBRARY_CUSTOM_OP;
  library.data = data;
  library.data_size = sizeof(data);
  ASSERT_TRUE(gert::om2::IsValidSharedLibrary(library));
  for (size_t scenario = 0U; scenario < 6U; ++scenario) {
    auto invalid = library;
    if (scenario == 0U) {
      invalid.struct_size = sizeof(GertModelSharedLibrary) - 1U;
    } else if (scenario == 1U) {
      invalid.so_name = nullptr;
    } else if (scenario == 2U) {
      invalid.so_name = "";
    } else if (scenario == 3U) {
      invalid.so_bin_type = GERT_MODEL_SHARED_LIBRARY_UNSPECIFIED;
    } else if (scenario == 4U) {
      invalid.data = nullptr;
    } else {
      invalid.data_size = 0U;
    }
    EXPECT_FALSE(gert::om2::IsValidSharedLibrary(invalid));
  }
  const GertModelSharedLibrary *entries[] = {nullptr};
  GertModelSharedLibraries libraries;
  libraries.library_num = 1U;
  EXPECT_FALSE(gert::om2::IsValidSharedLibraries(libraries));
  libraries.libraries = entries;
  EXPECT_FALSE(gert::om2::IsValidSharedLibraries(libraries));
  entries[0] = &library;
  EXPECT_TRUE(gert::om2::IsValidSharedLibraries(libraries));
  libraries.struct_size = sizeof(GertModelSharedLibraries) - 1U;
  EXPECT_FALSE(gert::om2::IsValidSharedLibraries(libraries));
}

TEST(Om2ModelDataStructSizeCompatibility, OldRootSizeDoesNotExposeSoBins) {
  gert::GertModelData model_data;
  model_data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  for (const uint64_t old_size : {64U, 72U}) {
    model_data.struct_size = old_size;
    EXPECT_EQ(gert::GetGertModelSoBins(model_data), nullptr);
  }
}

TEST(Om2ModelDataStructSizeCompatibility, PartialTailDoesNotExposeSoBins) {
  gert::GertModelData model_data;
  model_data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  model_data.struct_size = offsetof(gert::GertModelData, so_bins) + sizeof(model_data.so_bins) - 1U;
  EXPECT_EQ(gert::GetGertModelSoBins(model_data), nullptr);
}

TEST(Om2ModelDataStructSizeCompatibility, NewRootDefaultsToNoSoBins) {
  gert::GertModelData model_data;
  EXPECT_EQ(model_data.struct_size, sizeof(gert::GertModelData));
  EXPECT_EQ(gert::GetGertModelSoBins(model_data), nullptr);
}

TEST(Om2ModelDataStructSizeCompatibility, NewRootExposesProvidedSoBins) {
  gert::GertModelData model_data;
  model_data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  EXPECT_EQ(gert::GetGertModelSoBins(model_data), model_data.so_bins.get());
}
}  // namespace
