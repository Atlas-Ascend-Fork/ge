/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/om2_online_prepare_data.h"
#include <gtest/gtest.h>

#include "framework/om2/model_data/gert_model_data.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/common/json_file.h"
#include "framework/om2/model_data/om2_package_contants.h"
#include "framework/common/zip_archive_writer.h"
#include "framework/common/zip_archive_reader.h"

namespace gert {
namespace {

class Om2ModelDataTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(Om2ModelDataTest, InitPackagePreservesExistingResourcesAndDoesNotAddUnits) {
  GertModelData data;
  InitGertModelData(data);
  ASSERT_NE(data.manifest, nullptr);
  ASSERT_NE(data.constants, nullptr);
  ASSERT_NE(data.kernels, nullptr);
  ASSERT_NE(data.custom_ops, nullptr);
  ASSERT_NE(data.variables, nullptr);
  EXPECT_TRUE(data.models.empty());
  EXPECT_EQ(data.so_bins, nullptr);
  data.manifest->model_num = 3U;
  data.kernels->binaries.emplace_back(std::make_unique<GertModelDataFile>());
  const auto *manifest = data.manifest.get();
  const auto *kernel = data.kernels->binaries[0].get();

  InitGertModelData(data);
  EXPECT_EQ(data.manifest.get(), manifest);
  EXPECT_EQ(data.manifest->model_num, 3U);
  ASSERT_EQ(data.kernels->binaries.size(), 1U);
  EXPECT_EQ(data.kernels->binaries[0].get(), kernel);
  EXPECT_TRUE(data.models.empty());
}

TEST_F(Om2ModelDataTest, InitUnitAppendsWithoutReplacingExistingUnitOrPackageResources) {
  GertModelData data;
  InitGertModelData(data);
  InitGertModelDataUnit(data);
  ASSERT_EQ(data.models.size(), 1U);
  const auto *first = data.models[0].get();
  const auto *manifest = data.manifest.get();
  ASSERT_NE(first->model_meta, nullptr);
  ASSERT_NE(first->runtime, nullptr);
  ASSERT_NE(first->constants_config, nullptr);
  ASSERT_NE(first->debug, nullptr);
  data.models[0]->model_meta->model_name = GertMakeStr("first");
  EXPECT_EQ(GetGertModelExecutionType(*first), GertModelExecutionType::kStatic);
  data.models[0]->execution_type = GertModelExecutionType::kDynamic;

  InitGertModelDataUnit(data);
  ASSERT_EQ(data.models.size(), 2U);
  EXPECT_EQ(data.models[0].get(), first);
  EXPECT_STREQ(GertGetStr(data.models[0]->model_meta->model_name), "first");
  EXPECT_NE(data.models[1].get(), first);
  EXPECT_NE(data.models[1]->runtime, nullptr);
  EXPECT_EQ(GetGertModelExecutionType(*data.models[0]), GertModelExecutionType::kDynamic);
  EXPECT_EQ(GetGertModelExecutionType(*data.models[1]), GertModelExecutionType::kStatic);
  EXPECT_EQ(data.manifest.get(), manifest);
}

GertModelData MakeSharedLibSerializerData() {
  GertModelData model_data;
  InitGertModelData(model_data);
  model_data.manifest = std::make_unique<GertModelDataManifest>();
  model_data.manifest->model_num = 1U;
  model_data.manifest->compatibility.compiler_version = GertMakeStr(GERT_EXECUTOR_VERSION);
  auto unit = std::make_unique<GertModelDataModel>();
  unit->model_meta = std::make_unique<GertModelDataModelMeta>();
  unit->model_meta->model_name = GertMakeStr("shared_lib_serializer");
  unit->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data.models.emplace_back(std::move(unit));
  return model_data;
}

TEST(Om2NotifyMetadataUt, ModelMetaOmitsNotifyCount) {
  for (const auto type : {GertModelExecutionType::kStatic, GertModelExecutionType::kDynamic}) {
    auto data = MakeSharedLibSerializerData();
    data.models[0]->execution_type = type;
    ge::ModelBufferData output;
    ASSERT_EQ(SerializeGertModelData(data, output, false, "notify_metadata"), ge::SUCCESS);
    ZipArchiveReader archive(output.data.get(), output.length);
    ASSERT_TRUE(archive.IsGood());
    const auto path = archive.FindEntry(FormatOm2Path(OM2_MODEL_META_PATH_FORMAT, "0"));
    ASSERT_FALSE(path.empty());
    size_t size = 0U;
    const auto bytes = archive.ExtractToMem(path, size);
    ASSERT_NE(bytes, nullptr);
    const ge::JsonFile json(bytes.get(), size);
    ASSERT_TRUE(json.IsValid());
    EXPECT_FALSE(json.Raw().contains("reusable_notify_num"));
    GertModelData restored;
    InitGertModelData(restored);
    ASSERT_EQ(DeserializeGertModelMeta(output.data.get(), output.length, &restored), ge::SUCCESS);
    ASSERT_EQ(restored.models.size(), 1U);
    ASSERT_NE(restored.models[0]->model_meta, nullptr);
  }
}

std::unique_ptr<GertModelDataSoBin> MakeSharedLibForSerializer(const std::string &file_name) {
  auto library = std::make_unique<GertModelDataSoBin>();
  library->so_name = GertMakeStr("liboriginal.so");
  library->vendor_name = GertMakeStr("vendor/opp_latest/");
  library->so_bin_type = 2U;
  library->binary = std::make_unique<GertModelDataFile>();
  library->binary->file_name = GertMakeStr(file_name);
  const uint8_t content[] = {0x7FU, 0U, 1U};
  library->binary->data = GertMakeFileData(content, sizeof(content));
  library->binary->data_size = sizeof(content);
  return library;
}

TEST(Om2SharedLibSerializerUt, WritesFilesAndOrderedMetadataInSameDirectory) {
  auto model_data = MakeSharedLibSerializerData();
  model_data.so_bins = std::make_unique<GertModelDataSoBins>();
  model_data.so_bins->libraries.emplace_back(MakeSharedLibForSerializer("libarchived.so"));
  auto second = MakeSharedLibForSerializer("libarchived_1.so");
  second->so_name = GertMakeStr("libsecond.so");
  second->vendor_name.reset();
  second->so_bin_type = 0U;
  model_data.so_bins->libraries.emplace_back(std::move(second));
  ge::ModelBufferData output;
  ASSERT_EQ(SerializeGertModelData(model_data, output, false, "shared_lib_serializer"), ge::SUCCESS);
  ZipArchiveReader archive(output.data.get(), output.length);
  ASSERT_TRUE(archive.IsGood());
  const auto json_path = archive.FindEntry(OM2_SO_BINS_JSON_PATH);
  ASSERT_FALSE(json_path.empty());
  size_t json_size = 0U;
  const auto json_buffer = archive.ExtractToMem(json_path, json_size);
  ASSERT_NE(json_buffer, nullptr);
  const ge::JsonFile json(json_buffer.get(), json_size);
  ASSERT_TRUE(json.IsValid());
  const auto &libraries = json["libraries"];
  ASSERT_TRUE(libraries.is_array());
  ASSERT_EQ(libraries.size(), 2U);
  EXPECT_EQ(libraries[0]["file_name"].get<std::string>(), "libarchived.so");
  EXPECT_EQ(libraries[0]["so_name"].get<std::string>(), "liboriginal.so");
  EXPECT_EQ(libraries[0]["so_bin_type"].get<uint64_t>(), 2U);
  EXPECT_EQ(libraries[0]["vendor_name"].get<std::string>(), "vendor/opp_latest/");
  EXPECT_EQ(libraries[1]["so_name"].get<std::string>(), "libsecond.so");
  EXPECT_EQ(libraries[1]["so_bin_type"].get<uint64_t>(), 0U);
  EXPECT_EQ(libraries[1]["vendor_name"].get<std::string>(), "");
  for (const auto &name : {"libarchived.so", "libarchived_1.so"}) {
    const auto path = archive.FindEntry(std::string(OM2_SO_BINS_DIR) + name);
    ASSERT_FALSE(path.empty());
    size_t size = 0U;
    const auto data = archive.ExtractToMem(path, size);
    ASSERT_NE(data, nullptr);
    ASSERT_EQ(size, 3U);
    EXPECT_EQ(data.get()[0], 0x7FU);
    EXPECT_EQ(data.get()[1], 0U);
    EXPECT_EQ(data.get()[2], 1U);
  }
}

TEST(Om2SharedLibSerializerUt, SkipsAbsentEmptyAndOldSizeFields) {
  for (size_t scenario = 0U; scenario < 3U; ++scenario) {
    auto model_data = MakeSharedLibSerializerData();
    if (scenario > 0U) {
      model_data.so_bins = std::make_unique<GertModelDataSoBins>();
    }
    if (scenario == 2U) {
      model_data.so_bins->libraries.emplace_back(MakeSharedLibForSerializer("libarchived.so"));
      model_data.struct_size = 64U;
    }
    ge::ModelBufferData output;
    ASSERT_EQ(SerializeGertModelData(model_data, output, false, "shared_lib_serializer"), ge::SUCCESS);
    ZipArchiveReader archive(output.data.get(), output.length);
    ASSERT_TRUE(archive.IsGood());
    EXPECT_TRUE(archive.ListFilesByRelativePrefix(OM2_SO_BINS_DIR).empty());
  }
}

TEST(Om2SharedLibSerializerUt, RejectsInvalidOrDuplicateArchiveNamesAndEmptyData) {
  for (const auto &name : {"", ".", "../bad.so", "dir/bad.so", "dir\\bad.so", "so_bins.json"}) {
    auto model_data = MakeSharedLibSerializerData();
    model_data.so_bins = std::make_unique<GertModelDataSoBins>();
    model_data.so_bins->libraries.emplace_back(MakeSharedLibForSerializer(name));
    ge::ModelBufferData output;
    EXPECT_NE(SerializeGertModelData(model_data, output, false, "shared_lib_serializer"), ge::SUCCESS);
  }
  for (size_t scenario = 0U; scenario < 8U; ++scenario) {
    auto model_data = MakeSharedLibSerializerData();
    model_data.so_bins = std::make_unique<GertModelDataSoBins>();
    auto library = MakeSharedLibForSerializer("libarchived.so");
    if (scenario == 0U) {
      library.reset();
    } else if (scenario == 1U) {
      library->binary.reset();
    } else if (scenario == 2U) {
      library->binary->data.reset();
    } else if (scenario == 3U) {
      library->binary->data_size = 0U;
    } else if (scenario == 4U) {
      model_data.so_bins->libraries.emplace_back(MakeSharedLibForSerializer("libarchived.so"));
    } else if (scenario == 5U) {
      library->struct_size = sizeof(GertModelDataSoBin) - 1U;
    } else if (scenario == 6U) {
      model_data.so_bins->struct_size = sizeof(GertModelDataSoBins) - 1U;
    } else {
      library->so_name = GertMakeStr("../bad.so");
    }
    model_data.so_bins->libraries.emplace_back(std::move(library));
    ge::ModelBufferData output;
    EXPECT_NE(SerializeGertModelData(model_data, output, false, "shared_lib_serializer"), ge::SUCCESS);
  }
}

TEST(Om2ConstPlacementSerializerUt, WritesExplicitPlacementAndOmitsUnspecifiedOrOldTail) {
  auto model_data = MakeSharedLibSerializerData();
  for (size_t i = 0U; i < 4U; ++i) {
    auto meta = std::make_unique<GertModelDataConstMeta>();
    meta->index = i;
    meta->type = GertMakeStr("INTERNAL");
    if ((i == 0U) || (i == 3U)) {
      meta->placement = GertModelDataConstPlacement::kHost;
    } else if (i == 1U) {
      meta->placement = GertModelDataConstPlacement::kDevice;
    }
    if (i == 3U) {
      meta->struct_size = 64U;
    }
    model_data.models[0]->constants_config->consts.emplace_back(std::move(meta));
  }
  ge::ModelBufferData output;
  ASSERT_EQ(SerializeGertModelData(model_data, output, false, "const_placement_serializer"), ge::SUCCESS);
  ZipArchiveReader archive(output.data.get(), output.length);
  ASSERT_TRUE(archive.IsGood());
  const auto path = archive.FindEntry(FormatOm2Path(OM2_CONSTANTS_CONFIG_PATH_FORMAT, "0"));
  ASSERT_FALSE(path.empty());
  size_t size = 0U;
  const auto data = archive.ExtractToMem(path, size);
  ASSERT_NE(data, nullptr);
  const ge::JsonFile json(data.get(), size);
  ASSERT_TRUE(json.IsValid());
  const auto &consts = json["consts"];
  EXPECT_EQ(consts["constant_0"]["placement"].get<std::string>(), "HOST");
  EXPECT_EQ(consts["constant_1"]["placement"].get<std::string>(), "DEVICE");
  EXPECT_FALSE(consts["constant_2"].contains("placement"));
  EXPECT_FALSE(consts["constant_3"].contains("placement"));
}

TEST(Om2ConstPlacementSerializerUt, RejectsUnknownPlacement) {
  auto model_data = MakeSharedLibSerializerData();
  auto meta = std::make_unique<GertModelDataConstMeta>();
  meta->type = GertMakeStr("INTERNAL");
  meta->placement = static_cast<GertModelDataConstPlacement>(99U);
  model_data.models[0]->constants_config->consts.emplace_back(std::move(meta));
  ge::ModelBufferData output;
  EXPECT_NE(SerializeGertModelData(model_data, output, false, "invalid_const_placement"), ge::SUCCESS);
}

GertModelData MakeHostInternalWeightData() {
  auto data = MakeSharedLibSerializerData();
  auto &unit = *data.models[0];
  unit.runtime = std::make_unique<GertModelDataRuntime>();
  unit.runtime->so_artifact.file_name = GertMakeStr("libhost_weight.so");
  unit.runtime->so_artifact.data = GertMakeFileData("ELF", 3U);
  unit.runtime->so_artifact.data_size = 3U;
  unit.op_attr_json = GertMakeStr("{}");
  unit.constants_config->internal_weight_size = 5U;
  for (const uint64_t index : {0U, 2U}) {
    auto meta = std::make_unique<GertModelDataConstMeta>();
    meta->index = index;
    meta->type = GertMakeStr("INTERNAL");
    meta->placement = GertModelDataConstPlacement::kHost;
    const std::string name = "constant_" + std::to_string(index);
    meta->file_name = GertMakeStr(name);
    meta->size = index == 0U ? 2 : 3;
    auto file = std::make_unique<GertModelDataFile>();
    file->file_name = GertMakeStr(name);
    file->data_size = static_cast<uint64_t>(meta->size);
    file->data = GertMakeFileData(index == 0U ? "ab" : "xyz", file->data_size);
    unit.constants_config->consts.emplace_back(std::move(meta));
    data.constants->constants_data.emplace_back(std::move(file));
  }
  return data;
}

TEST(Om2HostInternalWeightUt, RoundTripsMultipleFilesAndSparseIndices) {
  auto data = MakeHostInternalWeightData();
  ge::ModelBufferData output;
  ASSERT_EQ(SerializeGertModelData(data, output, false, "host_weights"), ge::SUCCESS);
  ZipArchiveReader archive(output.data.get(), output.length);
  ASSERT_TRUE(archive.IsGood());
  EXPECT_FALSE(archive.FindEntry("data/constants/constant_0").empty());
  EXPECT_TRUE(archive.FindEntry("data/constants/constant_1").empty());
  EXPECT_FALSE(archive.FindEntry("data/constants/constant_2").empty());
  size_t manifest_size = 0U;
  auto manifest_data = archive.ExtractToMem(archive.FindEntry(OM2_MANIFEST_PATH), manifest_size);
  ASSERT_NE(manifest_data, nullptr);
  const ge::JsonFile manifest(manifest_data.get(), manifest_size);
  EXPECT_EQ(manifest["compatibility"]["required_executor_version"].get<std::string>(), "");
  EXPECT_FALSE(manifest["compatibility"]["used_features"].contains("host_internal_constants"));
  GertModelData restored;
  ASSERT_EQ(DeserializeGertModelData(output.data.get(), output.length, &restored), 0U);
  ASSERT_EQ(restored.constants->constants_data.size(), 2U);
  ASSERT_EQ(restored.models[0]->constants_config->consts.size(), 2U);
  const auto &metas = restored.models[0]->constants_config->consts;
  EXPECT_EQ(metas[0]->index, 0U);
  EXPECT_EQ(metas[1]->index, 2U);
  for (const auto &meta : metas) EXPECT_EQ(GetGertModelConstPlacement(*meta), GertModelDataConstPlacement::kHost);
  EXPECT_EQ(restored.constants->constants_data[0]->data_size, 2U);
  EXPECT_EQ(restored.constants->constants_data[1]->data_size, 3U);
  EXPECT_EQ(std::memcmp(restored.constants->constants_data[0]->data.get(), "ab", 2U), 0);
  EXPECT_EQ(std::memcmp(restored.constants->constants_data[1]->data.get(), "xyz", 3U), 0);
}

TEST(Om2HostInternalWeightUt, RejectsMissingShortDuplicateOrMixedInternalFiles) {
  for (size_t scenario = 0U; scenario < 4U; ++scenario) {
    auto data = MakeHostInternalWeightData();
    if (scenario == 0U) data.constants->constants_data.pop_back();
    if (scenario == 1U) data.constants->constants_data.back()->data_size = 1U;
    if (scenario == 2U) data.constants->constants_data.back()->file_name = GertMakeStr("constant_0");
    if (scenario == 3U)
      data.models[0]->constants_config->consts.back()->placement = GertModelDataConstPlacement::kDevice;
    ge::ModelBufferData output;
    EXPECT_NE(SerializeGertModelData(data, output, false, "invalid_host_weights"), ge::SUCCESS);
  }
}

TEST(Om2SharedLibDeserializerUt, RoundTripsMetadataAndReplacesCollectionOnRepeatedRead) {
  auto data = MakeHostInternalWeightData();
  data.so_bins = std::make_unique<GertModelDataSoBins>();
  data.so_bins->libraries.push_back(MakeSharedLibForSerializer("libfirst.so"));
  data.so_bins->libraries.push_back(MakeSharedLibForSerializer("libsecond.so"));
  ge::ModelBufferData output;
  ASSERT_EQ(SerializeGertModelData(data, output, false, "shared_lib_roundtrip"), ge::SUCCESS);
  GertModelData restored;
  ASSERT_EQ(DeserializeGertModelData(output.data.get(), output.length, &restored), ge::SUCCESS);
  for (size_t pass = 0U; pass < 2U; ++pass) {
    ASSERT_NE(restored.so_bins, nullptr);
    ASSERT_EQ(restored.so_bins->libraries.size(), 2U);
    const auto &library = *restored.so_bins->libraries[0];
    EXPECT_STREQ(GertGetStr(library.so_name), "liboriginal.so");
    EXPECT_STREQ(GertGetStr(library.vendor_name), "vendor/opp_latest/");
    EXPECT_EQ(library.so_bin_type, 2U);
    EXPECT_STREQ(GertGetStr(library.binary->file_name), "libfirst.so");
    ASSERT_EQ(library.binary->data_size, 3U);
    EXPECT_EQ(library.binary->data.get()[0], 0x7FU);
    ASSERT_EQ(DeserializeGertModelData(output.data.get(), output.length, &restored), ge::SUCCESS);
  }
  auto legacy = MakeHostInternalWeightData();
  ge::ModelBufferData legacy_output;
  ASSERT_EQ(SerializeGertModelData(legacy, legacy_output, false, "shared_lib_absent"), ge::SUCCESS);
  ASSERT_EQ(DeserializeGertModelData(legacy_output.data.get(), legacy_output.length, &restored), ge::SUCCESS);
  EXPECT_EQ(restored.so_bins, nullptr);
}

TEST(Om2SharedLibDeserializerUt, RejectsMalformedDescriptionsOrMissingBinary) {
  const std::vector<std::string> descriptions{
      "{}",
      R"({"libraries":{}})",
      R"({"libraries":[{}]})",
      R"({"libraries":[{"file_name":"../bad.so","so_name":"valid.so","so_bin_type":0}]})",
      R"({"libraries":[{"file_name":"missing.so","so_name":"valid.so","so_bin_type":0}]})",
      R"({"libraries":[{"file_name":"valid.so","so_name":"valid.so","so_bin_type":4}]})",
      R"({"libraries":[{"file_name":"valid.so","so_name":"valid.so","so_bin_type":-1}]})",
      R"({"libraries":[{"file_name":"valid.so","so_name":"valid.so","so_bin_type":0,"vendor_name":1}]})",
      R"({"libraries":[{"file_name":"valid.so","so_name":"valid.so","so_bin_type":0},
                       {"file_name":"valid.so","so_name":"valid.so","so_bin_type":0}]})"};
  auto data = MakeHostInternalWeightData();
  ge::ModelBufferData base;
  ASSERT_EQ(SerializeGertModelData(data, base, false, "shared_lib_invalid_base"), ge::SUCCESS);
  ZipArchiveReader archive(base.data.get(), base.length);
  for (const auto &description : descriptions) {
    ZipArchiveWriter writer("shared_lib_invalid");
    for (const auto &path : archive.ListFilesByRelativePrefix("")) {
      size_t size = 0U;
      auto bytes = archive.ExtractToMem(path, size);
      ASSERT_NE(bytes, nullptr);
      ASSERT_TRUE(writer.WriteBytes(path.substr(path.find('/') + 1U), bytes.get(), size, false));
    }
    ASSERT_TRUE(writer.WriteBytes(OM2_SO_BINS_JSON_PATH, description.data(), description.size(), false));
    ASSERT_TRUE(writer.WriteBytes(std::string(OM2_SO_BINS_DIR) + "valid.so", "SO", 2U, false));
    GertBuffer invalid;
    ASSERT_TRUE(writer.SaveModelData(invalid, false));
    GertModelData restored;
    EXPECT_NE(DeserializeGertModelData(invalid.data.get(), invalid.length, &restored), ge::SUCCESS) << description;
  }
}

// Test default construction
TEST_F(Om2ModelDataTest, DefaultConstruction) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->runtime = std::make_unique<GertModelDataRuntime>();
  model_data.models[0]->model_meta = std::make_unique<GertModelDataModelMeta>();
  model_data.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data.models[0]->debug = std::make_unique<GertModelDataDebug>();

  EXPECT_TRUE(model_data.models[0]->runtime->source_artifacts.empty());
  EXPECT_TRUE(!model_data.models[0]->runtime->so_artifact.file_name);
  EXPECT_TRUE(!model_data.models[0]->runtime->so_artifact.data);

  EXPECT_TRUE(!model_data.models[0]->model_meta->model_name);
  EXPECT_EQ(model_data.models[0]->model_meta->work_size, 0U);
  EXPECT_EQ(model_data.models[0]->model_meta->zero_copy_size, 0);
  EXPECT_TRUE(model_data.models[0]->model_meta->input_desc.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->output_desc.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->input_desc_v2.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->output_desc_v2.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->dynamic_batch_info.empty());
  EXPECT_EQ(model_data.models[0]->model_meta->dynamic_type, 0);
  EXPECT_TRUE(model_data.models[0]->model_meta->dynamic_output_shape.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->user_designate_shape_order.empty());
  EXPECT_TRUE(model_data.models[0]->model_meta->origin_input_dims.empty());

  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 0U);
  EXPECT_TRUE(model_data.models[0]->constants_config->consts.empty());

  EXPECT_EQ(model_data.constants->constants_data[0], nullptr);
  EXPECT_TRUE(model_data.kernels->binaries.empty());

  EXPECT_TRUE(model_data.models[0]->op_attr_json == nullptr);
  EXPECT_TRUE(!model_data.models[0]->debug->visual_json);
}

// Test populating codegen output
TEST_F(Om2ModelDataTest, PopulateCodegenOutput) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->runtime = std::make_unique<GertModelDataRuntime>();

  gert::GertModelDataFile artifact1;
  artifact1.file_name = gert::GertMakeStr("model.cpp");
  const std::string data1 = "int main() { return 0; }";
  artifact1.data = gert::GertMakeFileData(data1.data(), data1.size());
  artifact1.data_size = data1.size();
  model_data.models[0]->runtime->source_artifacts.push_back(std::move(artifact1));

  gert::GertModelDataFile artifact2;
  artifact2.file_name = gert::GertMakeStr("model.h");
  const std::string data2 = "#pragma once\nvoid func();";
  artifact2.data = gert::GertMakeFileData(data2.data(), data2.size());
  artifact2.data_size = data2.size();
  model_data.models[0]->runtime->source_artifacts.push_back(std::move(artifact2));

  model_data.models[0]->runtime->so_artifact.file_name = gert::GertMakeStr("libmodel.so");
  const std::string so_data = "binary data";
  model_data.models[0]->runtime->so_artifact.data = gert::GertMakeFileData(so_data.data(), so_data.size());
  model_data.models[0]->runtime->so_artifact.data_size = so_data.size();

  EXPECT_EQ(model_data.models[0]->runtime->source_artifacts.size(), 2U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->runtime->source_artifacts[0].file_name)), "model.cpp");
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->runtime->source_artifacts[1].file_name)), "model.h");
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->runtime->so_artifact.file_name)), "libmodel.so");
}

// Test populating model metadata
TEST_F(Om2ModelDataTest, PopulateModelMeta) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->model_meta = std::make_unique<GertModelDataModelMeta>();

  model_data.models[0]->model_meta->model_name = gert::GertMakeStr("test_model");
  model_data.models[0]->model_meta->work_size = 1024 * 1024;

  model_data.models[0]->model_meta->input_desc.push_back(
      gert::MakeGertTensorDesc("input", ge::DT_FLOAT, ge::FORMAT_ND, {1, 3, 224, 224}));
  model_data.models[0]->model_meta->output_desc.push_back(
      gert::MakeGertTensorDesc("output", ge::DT_FLOAT, ge::FORMAT_ND, {1, 1000}));

  model_data.models[0]->model_meta->dynamic_batch_info = {{1}, {2}, {4}, {8}};
  model_data.models[0]->model_meta->dynamic_type = 1;

  model_data.models[0]->model_meta->dynamic_output_shape.push_back(gert::GertMakeStr("1,1000"));

  model_data.models[0]->model_meta->origin_input_dims = {{1, 3, 224, 224}};

  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->model_name)), "test_model");
  EXPECT_EQ(model_data.models[0]->model_meta->work_size, 1024 * 1024);
  EXPECT_EQ(model_data.models[0]->model_meta->input_desc.size(), 1U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->input_desc[0].name)), "input");
  EXPECT_EQ(model_data.models[0]->model_meta->output_desc.size(), 1U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->output_desc[0].name)), "output");
  EXPECT_EQ(model_data.models[0]->model_meta->dynamic_batch_info.size(), 4U);
  EXPECT_EQ(model_data.models[0]->model_meta->dynamic_type, 1);
}

// Test populating constants config
TEST_F(Om2ModelDataTest, PopulateConstantsConfig) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();

  model_data.models[0]->constants_config->internal_weight_size = 2048;

  gert::GertModelDataConstMeta const1;
  const1.index = 0;
  const1.type = gert::GertMakeStr("weight");
  const1.file_name = gert::GertMakeStr("weight0.bin");
  const1.offset = 0;
  const1.size = 1024;
  model_data.models[0]->constants_config->consts.push_back(
      std::make_unique<gert::GertModelDataConstMeta>(std::move(const1)));

  gert::GertModelDataConstMeta const2;
  const2.index = 1;
  const2.type = gert::GertMakeStr("bias");
  const2.file_name = gert::GertMakeStr("bias0.bin");
  const2.offset = 1024;
  const2.size = 1024;
  model_data.models[0]->constants_config->consts.push_back(
      std::make_unique<gert::GertModelDataConstMeta>(std::move(const2)));

  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 2048);
  EXPECT_EQ(model_data.models[0]->constants_config->consts.size(), 2U);
  EXPECT_EQ(model_data.models[0]->constants_config->consts[0]->size, 1024);
  EXPECT_EQ(model_data.models[0]->constants_config->consts[1]->size, 1024);
}

// Test populating weight data
TEST_F(Om2ModelDataTest, PopulateWeightData) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data.constants->constants_data[0] = std::make_unique<GertModelDataFile>();

  auto buf = std::make_unique<uint8_t[]>(5);
  buf[0] = 0x01;
  buf[1] = 0x02;
  buf[2] = 0x03;
  buf[3] = 0x04;
  buf[4] = 0x05;
  model_data.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
  model_data.constants->constants_data[0]->data_size = 5U;
  model_data.models[0]->constants_config->internal_weight_size = 5U;

  EXPECT_NE(model_data.constants->constants_data[0]->data, nullptr);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[0], 0x01);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[4], 0x05);
}

// Test populating kernel binaries
TEST_F(Om2ModelDataTest, PopulateKernelBinaries) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);

  GertModelDataFile kernel1;
  kernel1.file_name = gert::GertMakeStr("kernel_add");
  auto buf1 = std::make_unique<uint8_t[]>(3);
  buf1[0] = 0x10;
  buf1[1] = 0x20;
  buf1[2] = 0x30;
  kernel1.data = ge::ReadonlyByteBuffer(buf1.release(), ge::ConditionalDeleter{true});
  kernel1.data_size = 3U;
  model_data.kernels->binaries.push_back(std::make_unique<GertModelDataFile>(std::move(kernel1)));

  GertModelDataFile kernel2;
  kernel2.file_name = gert::GertMakeStr("kernel_mul");
  auto buf2 = std::make_unique<uint8_t[]>(3);
  buf2[0] = 0x40;
  buf2[1] = 0x50;
  buf2[2] = 0x60;
  kernel2.data = ge::ReadonlyByteBuffer(buf2.release(), ge::ConditionalDeleter{true});
  kernel2.data_size = 3U;
  model_data.kernels->binaries.push_back(std::make_unique<GertModelDataFile>(std::move(kernel2)));

  // Verify
  EXPECT_EQ(model_data.kernels->binaries.size(), 2U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.kernels->binaries[0]->file_name)), "kernel_add");
  EXPECT_EQ(model_data.kernels->binaries[0]->data_size, 3U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.kernels->binaries[1]->file_name)), "kernel_mul");
}

// Test populating debug info
TEST_F(Om2ModelDataTest, PopulateDebugInfo) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->debug = std::make_unique<GertModelDataDebug>();

  model_data.models[0]->debug->visual_json = gert::GertMakeStr(R"({"format":"ge_visual_json","format_version":1})");

  model_data.models[0]->op_attr_json = gert::GertMakeStr(R"({"add":{"alpha":"1.0","beta":"1.0"}})");

  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->debug->visual_json)),
            R"({"format":"ge_visual_json","format_version":1})");
  ASSERT_NE(model_data.models[0]->op_attr_json, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->op_attr_json)),
            R"({"add":{"alpha":"1.0","beta":"1.0"}})");
}

// Test populating manifest
TEST_F(Om2ModelDataTest, PopulateManifest) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.manifest = std::make_unique<GertModelDataManifest>();
  model_data.manifest->atc_command = gert::GertMakeStr("");
  model_data.manifest->model_num = 1U;

  EXPECT_EQ(model_data.manifest->model_num, 1U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.manifest->atc_command)), "");
}

TEST_F(Om2ModelDataTest, Manifest_BundleFields_Defaults) {
  GertModelDataManifest manifest;
  EXPECT_EQ(manifest.model_num, 0U);
  EXPECT_EQ(manifest.atc_command, nullptr);
}

TEST_F(Om2ModelDataTest, Manifest_PopulateBundleFields) {
  GertModelDataManifest manifest;
  manifest.model_num = 2U;
  manifest.compatibility.compiler_version = gert::GertMakeStr("1.0");

  EXPECT_EQ(manifest.model_num, 2U);
  EXPECT_EQ(std::string(gert::GertGetStr(manifest.compatibility.compiler_version)), "1.0");
}

TEST_F(Om2ModelDataTest, ModelMeta_VarFields_DefaultAndPopulate) {
  GertModelDataModelMeta model_meta;
  EXPECT_EQ(model_meta.global_shared_var_size, 0U);
  EXPECT_EQ(model_meta.var_weight_file_name, nullptr);
  model_meta.global_shared_var_size = 4096U;
  model_meta.var_weight_file_name = gert::GertMakeStr("var_weight_data_0");
  EXPECT_EQ(model_meta.global_shared_var_size, 4096U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_meta.var_weight_file_name)), "var_weight_data_0");
}

TEST_F(Om2ModelDataTest, Variables_VarWeightData_DefaultEmpty) {
  GertModelDataVariables variables;
  EXPECT_EQ(variables.var_weight_data.size(), 0U);
  // 槽位可以先占位后填充（无初始数据块的模型保持 null）
  variables.var_weight_data.emplace_back();
  EXPECT_EQ(variables.var_weight_data.size(), 1U);
  EXPECT_EQ(variables.var_weight_data[0], nullptr);
  variables.var_weight_data[0] = std::make_unique<GertModelDataFile>();
  EXPECT_NE(variables.var_weight_data[0], nullptr);
}

// Test move semantics
TEST_F(Om2ModelDataTest, MoveSemantics) {
  GertModelData model_data1;
  gert::InitGertModelData(model_data1);
  model_data1.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data1.constants->constants_data.emplace_back();
  model_data1.models[0]->model_meta = std::make_unique<GertModelDataModelMeta>();
  model_data1.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data1.constants->constants_data[0] = std::make_unique<GertModelDataFile>();
  model_data1.models[0]->model_meta->model_name = gert::GertMakeStr("test_model");
  auto buf = std::make_unique<uint8_t[]>(3);
  buf[0] = 0x01;
  buf[1] = 0x02;
  buf[2] = 0x03;
  model_data1.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
  model_data1.constants->constants_data[0]->data_size = 3U;
  model_data1.models[0]->constants_config->internal_weight_size = 3U;

  GertModelData model_data2 = std::move(model_data1);

  EXPECT_EQ(std::string(gert::GertGetStr(model_data2.models[0]->model_meta->model_name)), "test_model");
  EXPECT_EQ(model_data2.models[0]->constants_config->internal_weight_size, 3U);
  EXPECT_EQ(model_data1.constants, nullptr);
}

TEST_F(Om2ModelDataTest, KernelBinary_DefaultConstruction) {
  GertModelDataFile kernel;
  EXPECT_EQ(kernel.data, nullptr);
  EXPECT_EQ(kernel.data_size, 0U);
  EXPECT_TRUE(!kernel.file_name);
}

TEST_F(Om2ModelDataTest, KernelBinary_MoveSemantics) {
  GertModelDataFile kernel1;
  kernel1.file_name = gert::GertMakeStr("kernel_move");
  auto buf = std::make_unique<uint8_t[]>(4);
  buf[0] = 0xAA;
  buf[1] = 0xBB;
  buf[2] = 0xCC;
  buf[3] = 0xDD;
  kernel1.data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
  kernel1.data_size = 4U;

  GertModelDataFile kernel2 = std::move(kernel1);
  EXPECT_EQ(kernel1.data, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(kernel2.file_name)), "kernel_move");
  EXPECT_EQ(kernel2.data_size, 4U);
  EXPECT_NE(kernel2.data, nullptr);
  EXPECT_EQ(kernel2.data.get()[0], 0xAA);
  EXPECT_EQ(kernel2.data.get()[3], 0xDD);
}

TEST_F(Om2ModelDataTest, KernelBinary_NonOwningBuffer) {
  uint8_t raw_data[] = {0x10, 0x20, 0x30, 0x40};
  GertModelDataFile kernel;
  kernel.file_name = gert::GertMakeStr("non_owning_kernel");
  kernel.data = ge::ReadonlyByteBuffer(raw_data, ge::ConditionalDeleter{false});
  kernel.data_size = sizeof(raw_data);

  EXPECT_NE(kernel.data, nullptr);
  EXPECT_EQ(kernel.data.get(), raw_data);
  EXPECT_EQ(kernel.data_size, 4U);
  EXPECT_EQ(kernel.data.get()[0], 0x10);
  EXPECT_EQ(kernel.data.get()[3], 0x40);
}

TEST_F(Om2ModelDataTest, WeightData_NonOwningBuffer) {
  uint8_t raw_weights[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data.constants->constants_data[0] = std::make_unique<GertModelDataFile>();
  model_data.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(raw_weights, ge::ConditionalDeleter{false});
  model_data.constants->constants_data[0]->data_size = sizeof(raw_weights);
  model_data.models[0]->constants_config->internal_weight_size = sizeof(raw_weights);

  EXPECT_NE(model_data.constants->constants_data[0]->data, nullptr);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get(), raw_weights);
  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 6U);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[0], 0x01);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[5], 0x06);
}

TEST_F(Om2ModelDataTest, KernelBinaries_WithNullData) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);

  GertModelDataFile kernel_with_data;
  kernel_with_data.file_name = gert::GertMakeStr("has_data");
  auto buf = std::make_unique<uint8_t[]>(2);
  buf[0] = 0xFF;
  buf[1] = 0xFE;
  kernel_with_data.data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
  kernel_with_data.data_size = 2U;
  model_data.kernels->binaries.push_back(std::make_unique<GertModelDataFile>(std::move(kernel_with_data)));

  GertModelDataFile kernel_without_data;
  kernel_without_data.file_name = gert::GertMakeStr("no_data");
  model_data.kernels->binaries.push_back(std::make_unique<GertModelDataFile>(std::move(kernel_without_data)));

  EXPECT_EQ(model_data.kernels->binaries.size(), 2U);
  EXPECT_NE(model_data.kernels->binaries[0]->data, nullptr);
  EXPECT_EQ(model_data.kernels->binaries[0]->data_size, 2U);
  EXPECT_EQ(model_data.kernels->binaries[1]->data, nullptr);
  EXPECT_EQ(model_data.kernels->binaries[1]->data_size, 0U);
}

TEST_F(Om2ModelDataTest, WeightData_ContentVerification) {
  GertModelData model_data;
  gert::InitGertModelData(model_data);
  model_data.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data.constants->constants_data.emplace_back();
  model_data.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data.constants->constants_data[0] = std::make_unique<GertModelDataFile>();
  constexpr size_t kSize = 8U;
  auto buf = std::make_unique<uint8_t[]>(kSize);
  for (size_t i = 0; i < kSize; ++i) {
    buf[i] = static_cast<uint8_t>(i * 0x11);
  }
  model_data.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
  model_data.constants->constants_data[0]->data_size = kSize;
  model_data.models[0]->constants_config->internal_weight_size = kSize;

  EXPECT_NE(model_data.constants->constants_data[0]->data, nullptr);
  const uint8_t *ptr = model_data.constants->constants_data[0]->data.get();
  for (size_t i = 0; i < kSize; ++i) {
    EXPECT_EQ(ptr[i], static_cast<uint8_t>(i * 0x11));
  }
}

TEST_F(Om2ModelDataTest, ModelData_MoveWithKernelBinaries) {
  GertModelData model_data1;
  gert::InitGertModelData(model_data1);
  model_data1.models.emplace_back(std::make_unique<gert::GertModelDataModel>());
  model_data1.constants->constants_data.emplace_back();
  model_data1.models[0]->model_meta = std::make_unique<GertModelDataModelMeta>();
  model_data1.models[0]->constants_config = std::make_unique<GertModelDataConstantsConfig>();
  model_data1.constants->constants_data[0] = std::make_unique<GertModelDataFile>();
  model_data1.models[0]->model_meta->model_name = gert::GertMakeStr("move_kernels");

  for (int i = 0; i < 3; ++i) {
    GertModelDataFile kb;
    kb.file_name = gert::GertMakeStr("kernel_" + std::to_string(i));
    auto buf = std::make_unique<uint8_t[]>(2);
    buf[0] = static_cast<uint8_t>(i);
    buf[1] = static_cast<uint8_t>(i + 1);
    kb.data = ge::ReadonlyByteBuffer(buf.release(), ge::ConditionalDeleter{true});
    kb.data_size = 2U;
    model_data1.kernels->binaries.push_back(std::make_unique<GertModelDataFile>(std::move(kb)));
  }

  auto wbuf = std::make_unique<uint8_t[]>(3);
  wbuf[0] = 0xAA;
  wbuf[1] = 0xBB;
  wbuf[2] = 0xCC;
  model_data1.constants->constants_data[0]->data = ge::ReadonlyByteBuffer(wbuf.release(), ge::ConditionalDeleter{true});
  model_data1.constants->constants_data[0]->data_size = 3U;
  model_data1.models[0]->constants_config->internal_weight_size = 3U;

  GertModelData model_data2 = std::move(model_data1);

  EXPECT_EQ(std::string(gert::GertGetStr(model_data2.models[0]->model_meta->model_name)), "move_kernels");
  EXPECT_EQ(model_data2.kernels->binaries.size(), 3U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data2.kernels->binaries[0]->file_name)), "kernel_0");
  EXPECT_EQ(model_data2.kernels->binaries[0]->data_size, 2U);
  EXPECT_EQ(model_data2.kernels->binaries[0]->data.get()[0], 0x00);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data2.kernels->binaries[2]->file_name)), "kernel_2");
  EXPECT_EQ(model_data2.kernels->binaries[2]->data.get()[0], 0x02);
  EXPECT_NE(model_data2.constants->constants_data[0]->data, nullptr);
  EXPECT_EQ(model_data2.models[0]->constants_config->internal_weight_size, 3U);

  EXPECT_EQ(model_data1.constants, nullptr);
  EXPECT_EQ(model_data1.kernels, nullptr);
}

// aipp 解析已内化为反序列化内部逻辑，通过公共 C API 集成验证 GertModelDataAippMeta 字段
TEST_F(Om2ModelDataTest, DeserializeModelMetaWithAippFillsGertModelDataAippMeta) {
  const std::string path = "ut_gert_model_data_aipp.om2";
  gert::ZipArchiveWriter writer(path);
  ASSERT_TRUE(writer.IsMemFileOpened());
  const std::string manifest =
      R"({"compatibility":{"compiler_version":"1.0","required_executor_version":"","used_features":{}},"model_num":1})";
  ASSERT_TRUE(writer.WriteBytes(gert::OM2_MANIFEST_PATH, manifest.data(), manifest.size(), false));
  const std::string meta = R"({
    "inputs": [{"name": "x", "index": 0, "shape": [1, 3, 224, 224], "data_type": 0, "format": 0,
                "size": 602112, "shape_range": []}],
    "outputs": [{"name": "y", "index": 0, "shape": [1, 1000], "data_type": 0, "format": 2,
                 "size": 4000, "shape_range": []}],
    "work_size": 4096, "zero_copy_size": 0, "name": "aipp_model",
    "aipp": {"aipp_infos": [{"index": 0, "aipp_type": 1, "aipp_data_index": 0,
                              "aipp_mode": 0, "input_format": 0,
                              "src_image_size_w": 224, "src_image_size_h": 224,
                              "crop": 0, "crop_size_w": 0, "crop_size_h": 0,
                              "resize": 0, "resize_output_w": 0, "resize_output_h": 0,
                              "padding": 0, "csc_switch": 0, "support_rotation": 0,
                              "related_input_rank": 0, "max_src_image_size": 0,
                              "aipp_inputs": ["0:0:data:0:4:1,3,224,224"],
                              "aipp_outputs": ["0:0:data:0:3:1,1,1000"],
                              "orig_input_format": 0, "orig_input_data_type": 0, "orig_input_dim_num": 4}]}
  })";
  ASSERT_TRUE(
      writer.WriteBytes(gert::FormatOm2Path(gert::OM2_MODEL_META_PATH_FORMAT, "0"), meta.data(), meta.size(), false));
  const std::string fake_so(64, 'S');
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_RUNTIME_DIR_FORMAT, "0") + "libaipp_om2.so",
                                fake_so.data(), fake_so.size(), false));
  const std::string op_attr = "{}";
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_OP_ATTR_PATH_FORMAT, "0"), op_attr.data(), op_attr.size(),
                                false));
  const std::string consts = R"({"internal_weight_size": 0, "consts": {}})";
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_CONSTANTS_CONFIG_PATH_FORMAT, "0"), consts.data(),
                                consts.size(), false));
  gert::GertBuffer buf;
  ASSERT_TRUE(writer.SaveModelData(buf, false));
  ASSERT_NE(buf.data, nullptr);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);

  EXPECT_FALSE(model_data.models[0]->model_meta->aipp_infos.empty());
  ASSERT_EQ(model_data.models[0]->model_meta->aipp_infos.size(), 1U);
  ASSERT_NE(model_data.models[0]->model_meta->aipp_infos[0], nullptr);
  const auto &aipp = *model_data.models[0]->model_meta->aipp_infos[0];
  EXPECT_EQ(aipp.aipp_type, ge::DATA_WITH_STATIC_AIPP);
  EXPECT_EQ(aipp.aipp_data_index, 0U);
  ASSERT_NE(aipp.aipp_config_info, nullptr);
  EXPECT_EQ(aipp.aipp_config_info->src_image_size_w, 224);
  EXPECT_EQ(aipp.aipp_config_info->src_image_size_h, 224);
  ASSERT_EQ(aipp.aipp_input_dims.size(), 1U);
  ASSERT_NE(aipp.aipp_input_dims[0], nullptr);
  EXPECT_EQ(aipp.aipp_input_dims[0]->name, "data");
  EXPECT_EQ(aipp.aipp_input_dims[0]->dim_num, 4U);
  ASSERT_EQ(aipp.aipp_input_dims[0]->dims.size(), 4U);
  EXPECT_EQ(aipp.aipp_input_dims[0]->dims[0], 1);
  EXPECT_EQ(aipp.aipp_input_dims[0]->dims[3], 224);
  ASSERT_EQ(aipp.aipp_output_dims.size(), 1U);
  ASSERT_NE(aipp.aipp_output_dims[0], nullptr);
  EXPECT_EQ(aipp.aipp_output_dims[0]->dim_num, 3U);
  ASSERT_EQ(aipp.aipp_output_dims[0]->dims.size(), 3U);
}

// ====== 按类别反序列化接口（DeserializeGert*）与全量接口 ======

// 构造包含全部已知类别的最小 OM2 归档，用于反序列化/全量接口测试
void BuildOm2ArchiveForFileList(const std::string &path, gert::GertBuffer &buf) {
  gert::ZipArchiveWriter writer(path);
  ASSERT_TRUE(writer.IsMemFileOpened());
  const std::string manifest =
      R"({"compatibility":{"compiler_version":"1.0","required_executor_version":"","used_features":{}},"model_num":1})";
  ASSERT_TRUE(writer.WriteBytes(gert::OM2_MANIFEST_PATH, manifest.data(), manifest.size(), false));
  const std::string meta = R"({
    "inputs": [{"name": "x", "index": 0, "shape": [1, 2], "data_type": 0, "format": 2,
                "size": 8, "shape_range": []}],
    "outputs": [{"name": "y", "index": 0, "shape": [1, 2], "data_type": 0, "format": 2,
                 "size": 8, "shape_range": []}],
    "work_size": 8192, "zero_copy_size": 0, "name": "list_model"
  })";
  ASSERT_TRUE(
      writer.WriteBytes(gert::FormatOm2Path(gert::OM2_MODEL_META_PATH_FORMAT, "0"), meta.data(), meta.size(), false));
  const std::string fake_so(32, 'S');
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_RUNTIME_DIR_FORMAT, "0") + "liblist_om2.so",
                                fake_so.data(), fake_so.size(), false));
  const std::string op_attr = R"({"attr": 1})";
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_OP_ATTR_PATH_FORMAT, "0"), op_attr.data(), op_attr.size(),
                                false));
  const std::string consts = R"({"internal_weight_size": 4, "consts": {}})";
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_CONSTANTS_CONFIG_PATH_FORMAT, "0"), consts.data(),
                                consts.size(), false));
  const uint8_t weight_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
  ASSERT_TRUE(writer.WriteBytes(std::string(gert::OM2_CONSTANTS_DIR) + gert::OM2_CONSTANTS_FILE_PREFIX + "0",
                                weight_data, sizeof(weight_data), false));
  const uint8_t kernel_0[] = {0x00};
  ASSERT_TRUE(writer.WriteBytes(std::string(gert::OM2_KERNELS_DIR) + "kernel_0.o", kernel_0, sizeof(kernel_0), false));
  const uint8_t kernel_1[] = {0x01};
  ASSERT_TRUE(writer.WriteBytes(std::string(gert::OM2_KERNELS_DIR) + "kernel_1.o", kernel_1, sizeof(kernel_1), false));
  const std::string visual_json = R"({"format":"ge_visual_json","format_version":1})";
  ASSERT_TRUE(writer.WriteBytes(gert::FormatOm2Path(gert::OM2_VISUAL_JSON_PATH_FORMAT, "0"), visual_json.data(),
                                visual_json.size(), false));
  ASSERT_TRUE(writer.SaveModelData(buf, false));
  ASSERT_NE(buf.data, nullptr);
}

// 反序列化接口：仅反序列化 model_meta，只填充 models[0]->model_meta，其余类别不填充
TEST_F(Om2ModelDataTest, DeserializeModelMeta_OnlyFillsModelMeta) {
  const std::string path = "ut_gert_model_data_files_meta.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelMeta(buf.data.get(), buf.length, &model_data), 0U);

  ASSERT_NE(model_data.models[0]->model_meta, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->model_name)), "list_model");
  EXPECT_EQ(model_data.models[0]->model_meta->work_size, 8192U);
  EXPECT_EQ(model_data.models[0]->model_meta->input_desc.size(), 1U);
  EXPECT_EQ(model_data.models[0]->model_meta->output_desc.size(), 1U);

  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->runtime->so_artifact.file_name)), "");
  EXPECT_TRUE(model_data.kernels->binaries.empty());
  EXPECT_EQ(model_data.models[0]->op_attr_json, nullptr);
  EXPECT_TRUE(model_data.models[0]->constants_config->consts.empty());
  EXPECT_TRUE(model_data.constants->constants_data.empty());
  EXPECT_EQ(model_data.models[0]->debug->visual_json, nullptr);
}

// 反序列化接口：仅反序列化 constants config，填充 internal_weight_size/consts，其余类别不填充
TEST_F(Om2ModelDataTest, DeserializeConstantsConfig_OnlyFillsConstantsConfig) {
  const std::string path = "ut_gert_model_data_files_constcfg.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertConstantsConfig(buf.data.get(), buf.length, &model_data), 0U);

  ASSERT_NE(model_data.models[0]->constants_config, nullptr);
  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 4U);
  EXPECT_TRUE(model_data.models[0]->constants_config->consts.empty());
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->model_name)), "");
  EXPECT_TRUE(model_data.constants->constants_data.empty());
  EXPECT_EQ(model_data.models[0]->debug->visual_json, nullptr);
}

// 全量接口：kernels 目录下全部 kernel binary 均加载
TEST_F(Om2ModelDataTest, DeserializeFull_KernelBinaries) {
  const std::string path = "ut_gert_model_data_files_kernel.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);

  ASSERT_EQ(model_data.kernels->binaries.size(), 2U);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.kernels->binaries[0]->file_name)), "kernel_0.o");
  EXPECT_EQ(model_data.kernels->binaries[0]->data_size, 1U);
  EXPECT_EQ(model_data.kernels->binaries[0]->data.get()[0], 0x00);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.kernels->binaries[1]->file_name)), "kernel_1.o");
  EXPECT_EQ(model_data.kernels->binaries[1]->data.get()[0], 0x01);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->model_name)), "list_model");
}

// 全量接口：op_attr/constants config/weight 均填充且相互校验通过
TEST_F(Om2ModelDataTest, DeserializeFull_OpAttrConstantsWeight) {
  const std::string path = "ut_gert_model_data_files_multi.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);

  EXPECT_NE(model_data.models[0]->op_attr_json, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->op_attr_json)), R"({"attr": 1})");
  EXPECT_EQ(model_data.models[0]->constants_config->internal_weight_size, 4U);
  ASSERT_NE(model_data.constants->constants_data[0]->data, nullptr);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[0], 0xAA);
  EXPECT_EQ(model_data.constants->constants_data[0]->data.get()[3], 0xDD);
  EXPECT_EQ(model_data.kernels->binaries.size(), 2U);
  EXPECT_EQ(model_data.models[0]->model_meta->input_desc.size(), 1U);
}

// 全量接口：可选类别文件在包中缺失时仅告警并返回成功，字段保持为空
TEST_F(Om2ModelDataTest, DeserializeFull_OptionalVariablesMissing) {
  const std::string path = "ut_gert_model_data_files_missing.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);
  // 变量初始数据块缺失属正常：槽位保持空，model_meta 的 var_weight_file_name 也保持空
  ASSERT_EQ(model_data.variables->var_weight_data.size(), 1U);
  EXPECT_EQ(model_data.variables->var_weight_data[0], nullptr);
  EXPECT_EQ(model_data.models[0]->model_meta->var_weight_file_name, nullptr);
}

// 反序列化接口：必选文件在包中缺失时报错（model_meta.json 不在精简归档中）
TEST_F(Om2ModelDataTest, DeserializeModelMeta_RequiredFileMissing) {
  const std::string path = "ut_gert_model_data_files_reqmissing.om2";
  gert::ZipArchiveWriter writer(path);
  ASSERT_TRUE(writer.IsMemFileOpened());
  const std::string manifest =
      R"({"compatibility":{"compiler_version":"1.0","required_executor_version":"","used_features":{}},"model_num":1})";
  ASSERT_TRUE(writer.WriteBytes(gert::OM2_MANIFEST_PATH, manifest.data(), manifest.size(), false));
  gert::GertBuffer buf;
  ASSERT_TRUE(writer.SaveModelData(buf, false));
  ASSERT_NE(buf.data, nullptr);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_NE(gert::DeserializeGertModelMeta(buf.data.get(), buf.length, &model_data), 0U);
}

// 全量接口：目录扫描类别在包中无匹配文件时容忍（custom_ops 目录不存在）
TEST_F(Om2ModelDataTest, DeserializeFull_EmptyDirTolerated) {
  const std::string path = "ut_gert_model_data_files_emptydir.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);
  EXPECT_TRUE(model_data.custom_ops->binaries.empty());
}

// model_index 越界：单模型归档（model_num=1）传 index=1 报参数错
TEST_F(Om2ModelDataTest, DeserializeModelMeta_ModelIndexOutOfRange) {
  const std::string path = "ut_gert_model_data_index_oor.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_NE(gert::DeserializeGertModelMeta(buf.data.get(), buf.length, &model_data, 1U), 0U);
}

// model_index 显式传 0 与默认行为一致（单模型语义）
TEST_F(Om2ModelDataTest, DeserializeModelMeta_ModelIndexZero) {
  const std::string path = "ut_gert_model_data_index_zero.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelMeta(buf.data.get(), buf.length, &model_data, 0U), 0U);
  EXPECT_EQ(model_data.models.size(), 1U);
}

// model_index=1（多模型归档）：单元按下标放置于 models[1]，models[0] 为空占位（下标与 data/model_N 对应）
TEST_F(Om2ModelDataTest, DeserializeModelMeta_ModelIndexOnePlacedByIndex) {
  const std::string path = "ut_gert_model_data_index_one.om2";
  gert::ZipArchiveWriter writer(path);
  ASSERT_TRUE(writer.IsMemFileOpened());
  const std::string manifest =
      R"({"compatibility":{"compiler_version":"1.0","required_executor_version":"","used_features":{}},"model_num":2})";
  ASSERT_TRUE(writer.WriteBytes(gert::OM2_MANIFEST_PATH, manifest.data(), manifest.size(), false));
  const std::string meta = R"({
    "inputs": [{"name": "x", "index": 0, "shape": [1, 2], "data_type": 0, "format": 2,
                "size": 8, "shape_range": []}],
    "outputs": [{"name": "y", "index": 0, "shape": [1, 2], "data_type": 0, "format": 2,
                 "size": 8, "shape_range": []}],
    "work_size": 4096, "zero_copy_size": 0, "name": "model_one"
  })";
  ASSERT_TRUE(
      writer.WriteBytes(gert::FormatOm2Path(gert::OM2_MODEL_META_PATH_FORMAT, "1"), meta.data(), meta.size(), false));
  gert::GertBuffer buf;
  ASSERT_TRUE(writer.SaveModelData(buf, false));
  ASSERT_NE(buf.data, nullptr);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelMeta(buf.data.get(), buf.length, &model_data, 1U), 0U);
  ASSERT_EQ(model_data.models.size(), 2U);
  EXPECT_EQ(model_data.models[0], nullptr);
  ASSERT_NE(model_data.models[1], nullptr);
  ASSERT_NE(model_data.models[1]->model_meta, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[1]->model_meta->model_name)), "model_one");
  EXPECT_EQ(model_data.models[1]->model_meta->work_size, 4096U);
}

// visual json：全量接口不反序列化 debug 目录；DeserializeGertVisualJson 填充原文
TEST_F(Om2ModelDataTest, DeserializeVisualJson_FillsVisualJsonOnly) {
  const std::string path = "ut_gert_model_data_files_visual.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData full_mode;
  gert::InitGertModelData(full_mode);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &full_mode), 0U);
  EXPECT_EQ(full_mode.models[0]->debug->visual_json, nullptr);

  GertModelData visual_mode;
  gert::InitGertModelData(visual_mode);
  EXPECT_EQ(gert::DeserializeGertVisualJson(buf.data.get(), buf.length, &visual_mode), 0U);
  ASSERT_NE(visual_mode.models[0]->debug->visual_json, nullptr);
  EXPECT_EQ(std::string(gert::GertGetStr(visual_mode.models[0]->debug->visual_json)),
            R"({"format":"ge_visual_json","format_version":1})");
  EXPECT_EQ(std::string(gert::GertGetStr(visual_mode.models[0]->model_meta->model_name)), "");
}

// 缺省参数 = 全量模式回归：三参调用填充全部类别（除 debug 目录）并通过必选检查
TEST_F(Om2ModelDataTest, Deserialize_DefaultIsFullMode) {
  const std::string path = "ut_gert_model_data_files_default.om2";
  gert::GertBuffer buf;
  BuildOm2ArchiveForFileList(path, buf);

  GertModelData model_data;
  gert::InitGertModelData(model_data);
  EXPECT_EQ(gert::DeserializeGertModelData(buf.data.get(), buf.length, &model_data), 0U);

  EXPECT_EQ(std::string(gert::GertGetStr(model_data.models[0]->model_meta->model_name)), "list_model");
  EXPECT_NE(model_data.models[0]->op_attr_json, nullptr);
  EXPECT_NE(model_data.constants->constants_data[0]->data, nullptr);
  EXPECT_EQ(model_data.kernels->binaries.size(), 2U);
  EXPECT_NE(std::string(gert::GertGetStr(model_data.models[0]->runtime->so_artifact.file_name)), "");
  EXPECT_EQ(model_data.models[0]->debug->visual_json, nullptr);
}

TEST(Om2SharedLibSerializerUt, OnlinePreparationIsNotRestoredFromArchive) {
  auto data = MakeSharedLibSerializerData();
  data.online_prepare_data = std::make_shared<Om2OnlinePrepareData>();
  data.online_prepare_data->graph_id = 987U;
  data.online_prepare_data->prepare_ge_variables = true;
  ge::ModelBufferData output;
  ASSERT_EQ(SerializeGertModelData(data, output, false, "online_prepare"), ge::SUCCESS);
  GertModelData restored;
  InitGertModelData(restored);
  ASSERT_EQ(DeserializeGertModelMeta(static_cast<const uint8_t *>(output.data.get()), output.length, &restored),
            ge::SUCCESS);
  EXPECT_EQ(restored.online_prepare_data, nullptr);
  ASSERT_NE(data.online_prepare_data, nullptr);
  EXPECT_EQ(data.online_prepare_data->graph_id, 987U);
}

}  // namespace
}  // namespace gert
