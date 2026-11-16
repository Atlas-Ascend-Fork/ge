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

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// ge_types.h 声明 gert::Tensor 回调类型却不自带该头，必须先引入运行时 Tensor 定义。
#include "exe_graph/runtime/tensor.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "common/om2/dynamic_codegen/prepare/gert_resources.h"

namespace gert {
namespace {
std::string Key(const char *prefix, const std::string &id, const char *field) {
  return std::string(prefix) + id + "." + field;
}

std::string SoKey(const std::string &id, const char *field) {
  return Key(gert::attr::kSoPrefix, id, field);
}

std::string BinaryKey(const std::string &id, const char *field) {
  return Key(gert::attr::kBinaryPrefix, id, field);
}

std::string WeightKey(const std::string &id, const char *field) {
  return Key(gert::attr::kWeightPrefix, id, field);
}

// 一维 uint8 Tensor 是公共侧的字节载荷承载。
ge::GeTensor MakeBytes(const std::vector<uint8_t> &bytes) {
  ge::GeTensorDesc desc(ge::GeShape({static_cast<int64_t>(bytes.size())}), ge::FORMAT_ND, ge::DT_UINT8);
  return ge::GeTensor(desc, bytes);
}

void SetSo(ge::ExecuteGraph &graph, const std::string &id, const std::string &name, const std::string &purpose,
           const std::vector<uint8_t> &content) {
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey(id, "name"), name));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey(id, "purpose"), purpose));
  ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, SoKey(id, "content"), MakeBytes(content)));
  std::vector<std::string> ids;
  (void)ge::AttrUtils::GetListStr(graph, gert::attr::kSoIds, ids);
  ids.push_back(id);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kSoIds, ids));
}

void SetBinary(ge::ExecuteGraph &graph, const std::string &id, const std::string &name,
               const std::vector<uint8_t> &content) {
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, BinaryKey(id, "name"), name));
  ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, BinaryKey(id, "content"), MakeBytes(content)));
  std::vector<std::string> ids;
  (void)ge::AttrUtils::GetListStr(graph, gert::attr::kBinaryIds, ids);
  ids.push_back(id);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kBinaryIds, ids));
}

void SetWeightHeader(ge::ExecuteGraph &graph, const std::string &id, const std::string &mode,
                     const std::string &placement, const int64_t size) {
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey(id, "mode"), mode));
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey(id, "placement"), placement));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, WeightKey(id, "size"), size));
  std::vector<std::string> ids;
  (void)ge::AttrUtils::GetListStr(graph, gert::attr::kWeightIds, ids);
  ids.push_back(id);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kWeightIds, ids));
}

std::string WriteTempFile(const std::string &name, const std::vector<uint8_t> &content) {
  const std::string path = "/tmp/gert_resources_ut_" + name;
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char *>(content.data()), static_cast<std::streamsize>(content.size()));
  out.close();
  return path;
}

ge::FastNode *AddNode(ge::ExecuteGraph &graph, const std::string &name) {
  auto desc = std::make_shared<ge::OpDesc>(name, "GertKernel");
  desc->AddOutputDesc(ge::GeTensorDesc(ge::GeShape({1}), ge::FORMAT_ND, ge::DT_FLOAT));
  return graph.AddNode(desc);
}
}  // namespace

class GertResourcesUT : public testing::Test {};

// 8.8：未声明任何资源时解析为空表，不报错。
TEST_F(GertResourcesUT, ParsesEmptyResourceTable) {
  ge::ExecuteGraph graph("resource_graph");
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_TRUE(table.sos.empty());
  EXPECT_TRUE(table.binaries.empty());
  EXPECT_TRUE(table.weights.empty());
}

// 8.8：SO 内容复制后不再依赖来源 Tensor。
TEST_F(GertResourcesUT, CopiesSoContent) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libophost_nn.so", "space_registry", {1U, 2U, 3U});

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(table.sos.size(), 1U);
  EXPECT_EQ(table.sos[0].name, "libophost_nn.so");
  EXPECT_EQ(table.sos[0].purpose, GertSoPurpose::kSpaceRegistry);
  EXPECT_EQ(table.sos[0].content, std::vector<uint8_t>({1U, 2U, 3U}));
}

// 8.8：未知用途必须报错，不能默默按 SpaceRegistry 注册。
TEST_F(GertResourcesUT, RejectsUnknownSoPurpose) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libunknown.so", "teleport", {1U});
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.8：content 与 path 二选一，同时给出无法判断以哪个为准。
TEST_F(GertResourcesUT, RejectsSoWithBothContentAndPath) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libophost_nn.so", "space_registry", {1U});
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey("sr0", "path"), "/tmp/not_used.so"));
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.8：SO 依赖必须指向本模型提供的 SO。
TEST_F(GertResourcesUT, RejectsDanglingSoDependency) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libophost_nn.so", "space_registry", {1U});
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SoKey("sr0", "dependencies"), {"sr_missing"}));
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.8：依赖在表内即可，声明顺序不影响校验。
TEST_F(GertResourcesUT, AcceptsSoDependencyWithinModel) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libbase.so", "space_registry", {1U});
  SetSo(graph, "kTiling", "libtiling.so", "tiling", {2U});
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, SoKey("kTiling", "dependencies"), {"sr0"}));
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_EQ(table.sos.size(), 2U);
}

// 8.8：重复 ID 指向同一份属性，去重后只保留一份。
TEST_F(GertResourcesUT, DeduplicatesRepeatedSoId) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr0", "libophost_nn.so", "space_registry", {1U});
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kSoIds, {"sr0", "sr0"}));
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_EQ(table.sos.size(), 1U);
}

// 8.8：条目 ID 只允许 ASCII 字母数字、下划线和连字符。
TEST_F(GertResourcesUT, RejectsInvalidResourceId) {
  ge::ExecuteGraph graph("resource_graph");
  SetSo(graph, "sr.0", "libophost_nn.so", "space_registry", {1U});
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.8：二进制必须有名称和内容。
TEST_F(GertResourcesUT, ParsesDeviceBinary) {
  ge::ExecuteGraph graph("resource_graph");
  SetBinary(graph, "k0", "add_0.o", {0x7FU, 0x45U, 0x4CU});
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(table.binaries.size(), 1U);
  EXPECT_EQ(table.binaries[0].name, "add_0.o");
  EXPECT_EQ(table.binaries[0].content, std::vector<uint8_t>({0x7FU, 0x45U, 0x4CU}));
}

// 8.9：内置权重可从内容导入，大小必须与声明一致。
TEST_F(GertResourcesUT, ParsesEmbeddedWeightFromContent) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "embedded", "device", 4);
  ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, WeightKey("w0", "content"), MakeBytes({1U, 2U, 3U, 4U})));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(table.weights.size(), 1U);
  EXPECT_EQ(table.weights[0].mode, GertWeightMode::kEmbedded);
  EXPECT_TRUE(table.weights[0].device_placement);
  EXPECT_EQ(table.weights[0].content, std::vector<uint8_t>({1U, 2U, 3U, 4U}));
}

// 8.9：声明大小与实际内容不一致会造成紧凑索引错位，必须拒绝。
TEST_F(GertResourcesUT, RejectsEmbeddedWeightSizeMismatch) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "embedded", "device", 8);
  ASSERT_TRUE(ge::AttrUtils::SetTensor(graph, WeightKey("w0", "content"), MakeBytes({1U, 2U, 3U, 4U})));
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.9：内置权重的内容、文件、Host 地址三选一。
TEST_F(GertResourcesUT, RejectsEmbeddedWeightWithoutSource) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "embedded", "device", 4);
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.9：内置权重可从 Host 文件读取，并按 file_offset 切片。
TEST_F(GertResourcesUT, ReadsEmbeddedWeightFromFileOffset) {
  const std::string path = WriteTempFile("slice.bin", {9U, 8U, 7U, 6U, 5U, 4U});
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "embedded", "host", 3);
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey("w0", "path"), path));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, WeightKey("w0", "file_offset"), 2));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(table.weights.size(), 1U);
  EXPECT_EQ(table.weights[0].content, std::vector<uint8_t>({7U, 6U, 5U}));
  (void)std::remove(path.c_str());
}

// 8.9：内置权重不接受 Device 地址当 Host 字节解引用。
TEST_F(GertResourcesUT, RejectsEmbeddedWeightFromDeviceAddress) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "embedded", "device", 4);
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, WeightKey("w0", "address"), static_cast<int64_t>(0x1234U)));
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.9：外置文件权重编译期只保留声明，不读取内容。
TEST_F(GertResourcesUT, KeepsExternalFileWeightUnread) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_file", "device", 4);
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey("w0", "path"), "/tmp/not_read_now.bin"));
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(table.weights.size(), 1U);
  EXPECT_EQ(table.weights[0].mode, GertWeightMode::kExternalFile);
  EXPECT_TRUE(table.weights[0].content.empty());
}

// 8.10：外置内存权重必须声明方式，不能由节点阶段推断。
TEST_F(GertResourcesUT, RejectsWeightWithoutMode) {
  ge::ExecuteGraph graph("resource_graph");
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey("w0", "placement"), "device"));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, WeightKey("w0", "size"), 4));
  ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, gert::attr::kWeightIds, {"w0"}));
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.10：placement 只接受 host/device。
TEST_F(GertResourcesUT, RejectsUnknownPlacement) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_memory", "npu", 4);
  GertResourceTable table;
  EXPECT_NE(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
}

// 8.10：外置内存权重在 Init 前必须给出地址。加载配置是另一张图上的同名键，
// 因此声明图缺地址并不影响编译期解析，只有绑定时才报缺失。
TEST_F(GertResourcesUT, RejectsExternalMemoryWithoutAddress) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_memory", "host", 4);
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));

  ge::ExecuteGraph load_graph("load_config");
  EXPECT_NE(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(load_graph, table));
}

// 8.10：外置内存加载期地址、大小、placement 必须满足声明。
TEST_F(GertResourcesUT, BindsExternalMemoryWeight) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_memory", "device", 4);

  ge::ExecuteGraph load_graph("load_config");
  ASSERT_TRUE(ge::AttrUtils::SetInt(load_graph, WeightKey("w0", "address"), static_cast<int64_t>(0x1000U)));
  ASSERT_TRUE(ge::AttrUtils::SetInt(load_graph, WeightKey("w0", "size"), 8));
  ASSERT_TRUE(ge::AttrUtils::SetStr(load_graph, WeightKey("w0", "placement"), "device"));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(load_graph, table));
  EXPECT_EQ(table.weights[0].load_address, 0x1000U);
  EXPECT_EQ(table.weights[0].load_size, 8);
  EXPECT_TRUE(table.weights[0].has_load_placement);
  EXPECT_TRUE(table.weights[0].load_device_placement);
}

// 8.10：加载期覆盖的 placement 不能与编译期声明矛盾。
TEST_F(GertResourcesUT, RejectsConflictingLoadPlacement) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_memory", "host", 4);

  ge::ExecuteGraph load_graph("load_config");
  ASSERT_TRUE(ge::AttrUtils::SetInt(load_graph, WeightKey("w0", "address"), static_cast<int64_t>(0x1000U)));
  ASSERT_TRUE(ge::AttrUtils::SetStr(load_graph, WeightKey("w0", "placement"), "device"));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_NE(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(load_graph, table));
}

// 8.10：外置内存的实际大小不能小于声明大小。
TEST_F(GertResourcesUT, RejectsUndersizedExternalMemory) {
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_memory", "host", 8);

  ge::ExecuteGraph load_graph("load_config");
  ASSERT_TRUE(ge::AttrUtils::SetInt(load_graph, WeightKey("w0", "address"), static_cast<int64_t>(0x1000U)));
  ASSERT_TRUE(ge::AttrUtils::SetInt(load_graph, WeightKey("w0", "size"), 4));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_NE(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(load_graph, table));
}

// 8.10：外置文件范围必须落在文件内。
TEST_F(GertResourcesUT, RejectsExternalFileOutOfRange) {
  const std::string path = WriteTempFile("short.bin", {1U, 2U, 3U});
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_file", "device", 8);
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey("w0", "path"), path));
  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  EXPECT_NE(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(graph, table));
  (void)std::remove(path.c_str());
}

// 8.10：加载覆盖路径优先于图上的路径。
TEST_F(GertResourcesUT, LoadPathOverridesGraphPath) {
  const std::string graph_path = WriteTempFile("graph_path.bin", {1U, 2U, 3U});
  const std::string load_path = WriteTempFile("load_path.bin", {1U, 2U, 3U, 4U});
  ge::ExecuteGraph graph("resource_graph");
  SetWeightHeader(graph, "w0", "external_file", "device", 3);
  ASSERT_TRUE(ge::AttrUtils::SetStr(graph, WeightKey("w0", "path"), graph_path));

  ge::ExecuteGraph load_graph("load_config");
  ASSERT_TRUE(ge::AttrUtils::SetStr(load_graph, WeightKey("w0", "path"), load_path));

  GertResourceTable table;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ParseGertResources(graph, table));
  ASSERT_EQ(ge::GRAPH_SUCCESS, ApplyGertLoadBindings(load_graph, table));
  EXPECT_EQ(table.weights[0].load_path, load_path);
  (void)std::remove(graph_path.c_str());
  (void)std::remove(load_path.c_str());
}

// 8.8：同名同内容的 SO 去重，同名不同内容拒绝。
TEST_F(GertResourcesUT, MergesIdenticalSoAndRejectsConflict) {
  GertResourceTable table;
  table.sos.push_back({"a", "libx.so", GertSoPurpose::kSpaceRegistry, {1U}});
  table.sos.push_back({"b", "libx.so", GertSoPurpose::kSpaceRegistry, {1U}});

  GertResourceImages images;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));
  ASSERT_EQ(images.sos.size(), 1U);
  EXPECT_EQ(images.sos[0].content, std::vector<uint8_t>({1U}));

  table.sos.push_back({"c", "libx.so", GertSoPurpose::kSpaceRegistry, {2U}});
  GertResourceImages conflicting;
  EXPECT_NE(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, conflicting));
}

// 8.8：同名 SO 的用途也必须一致，否则注册位置不确定。
TEST_F(GertResourcesUT, RejectsSoWithConflictingPurpose) {
  GertResourceTable table;
  table.sos.push_back({"a", "libx.so", GertSoPurpose::kSpaceRegistry, {1U}});
  table.sos.push_back({"b", "libx.so", GertSoPurpose::kTiling, {1U}});
  GertResourceImages images;
  EXPECT_NE(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));
}

// 8.8：二进制 key 由内容算出，内容相同即 key 相同。
TEST_F(GertResourcesUT, ComputesBinaryKeyFromContent) {
  GertResourceTable table;
  table.binaries.push_back({"k0", "add_0.o", {0x7FU, 0x45U, 0x4CU}});
  table.binaries.push_back({"k1", "add_0.o", {0x7FU, 0x45U, 0x4CU}});
  table.binaries.push_back({"k2", "add_1.o", {0x7FU, 0x45U, 0x4DU}});

  GertResourceImages images;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));
  ASSERT_EQ(images.binaries.size(), 2U);
  EXPECT_EQ(images.binaries[0].name, "add_0.o");
  EXPECT_EQ(images.binaries[1].name, "add_1.o");
  EXPECT_NE(images.binaries[0].key, images.binaries[1].key);
  EXPECT_FALSE(images.binaries[0].key.empty());
}

// 8.9：内置权重按声明顺序占用紧凑下标，外置权重不占下标。
TEST_F(GertResourcesUT, AssignsCompactWeightIndex) {
  GertResourceTable table;
  table.weights.push_back({"w0", GertWeightMode::kEmbedded, false, 2, {1U, 2U}, "", 0, 0, 0, 0, false, false, ""});
  table.weights.push_back(
      {"w1", GertWeightMode::kExternalFile, false, 2, {}, "/tmp/f.bin", 0, 0, 0, 0, false, false, ""});
  table.weights.push_back({"w2", GertWeightMode::kEmbedded, true, 1, {3U}, "", 0, 0, 0, 0, false, false, ""});

  GertResourceImages images;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));
  ASSERT_EQ(images.weights.entries.size(), 2U);
  EXPECT_EQ(images.weights.index_by_id.at("w0"), 0U);
  EXPECT_EQ(images.weights.index_by_id.at("w2"), 1U);
  EXPECT_EQ(images.weights.index_by_id.count("w1"), 0U);
  EXPECT_EQ(images.weights.entries[1].data, std::vector<uint8_t>({3U}));
}

// 8.9：同一资源被多个节点共享时切片指向同一下标。
TEST_F(GertResourcesUT, ResolvesSharedWeightSlice) {
  GertResourceTable table;
  table.weights.push_back(
      {"w0", GertWeightMode::kEmbedded, false, 8, {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}, "", 0, 0, 0, 0, false, false, ""});
  GertResourceImages images;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));

  GertWeightSlice first;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w0", 0, 4}, images.weights, first));
  GertWeightSlice second;
  ASSERT_EQ(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w0", 4, 4}, images.weights, second));
  EXPECT_EQ(first.weight_index, 0U);
  EXPECT_EQ(second.weight_index, 0U);
  EXPECT_EQ(first.offset, 0);
  EXPECT_EQ(second.offset, 4);
}

// 8.9：超出资源范围的切片必须拒绝。
TEST_F(GertResourcesUT, RejectsOutOfRangeWeightSlice) {
  GertResourceTable table;
  table.weights.push_back(
      {"w0", GertWeightMode::kEmbedded, false, 4, {1U, 2U, 3U, 4U}, "", 0, 0, 0, 0, false, false, ""});
  GertResourceImages images;
  ASSERT_EQ(ge::GRAPH_SUCCESS, BuildGertResourceImages(table, images));

  GertWeightSlice slice;
  EXPECT_NE(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w0", 2, 4}, images.weights, slice));
  EXPECT_NE(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w0", 5, 0}, images.weights, slice));
  EXPECT_NE(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w0", 0, -1}, images.weights, slice));
  // 外置权重没有紧凑下标，节点引用它属于配置错误。
  EXPECT_NE(ge::GRAPH_SUCCESS, ResolveWeightSlice({"w1", 0, 0}, images.weights, slice));
}

// 8.8：节点上的资源引用按声明原样返回。
TEST_F(GertResourcesUT, ReadsNodeResourceRefs) {
  ge::ExecuteGraph graph("resource_graph");
  auto *node = AddNode(graph, "kernel0");
  ASSERT_NE(node, nullptr);
  auto desc = node->GetOpDescPtr();
  ASSERT_NE(desc, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetListStr(desc, gert::attr::kSoIds, {"sr0"}));
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, gert::attr::kNodeBinaryId, "k0"));
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, gert::attr::kNodeWeightId, "w0"));
  ASSERT_TRUE(ge::AttrUtils::SetInt(desc, "gert.weight.offset", 2));
  ASSERT_TRUE(ge::AttrUtils::SetInt(desc, "gert.weight.length", 4));

  std::vector<std::string> so_ids;
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeSoIds(*node, so_ids));
  EXPECT_EQ(so_ids, std::vector<std::string>({"sr0"}));

  std::string binary_id;
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeBinaryId(*node, binary_id));
  EXPECT_EQ(binary_id, "k0");

  GertNodeWeightRef weight_ref;
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeWeightRef(*node, weight_ref));
  EXPECT_EQ(weight_ref.id, "w0");
  EXPECT_EQ(weight_ref.offset, 2);
  EXPECT_EQ(weight_ref.length, 4);
}

// 8.8：未声明资源引用的节点返回空值，不报错。
TEST_F(GertResourcesUT, TreatsMissingNodeRefsAsAbsent) {
  ge::ExecuteGraph graph("resource_graph");
  auto *node = AddNode(graph, "kernel0");
  ASSERT_NE(node, nullptr);

  std::vector<std::string> so_ids{"stale"};
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeSoIds(*node, so_ids));
  EXPECT_TRUE(so_ids.empty());

  std::string binary_id{"stale"};
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeBinaryId(*node, binary_id));
  EXPECT_TRUE(binary_id.empty());

  GertNodeWeightRef weight_ref;
  ASSERT_EQ(ge::GRAPH_SUCCESS, GetNodeWeightRef(*node, weight_ref));
  EXPECT_TRUE(weight_ref.id.empty());
}

// 8.9：节点声明了权重但缺少切片信息属于配置错误。
TEST_F(GertResourcesUT, RejectsNodeWeightWithoutSlice) {
  ge::ExecuteGraph graph("resource_graph");
  auto *node = AddNode(graph, "kernel0");
  ASSERT_NE(node, nullptr);
  auto desc = node->GetOpDescPtr();
  ASSERT_NE(desc, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetStr(desc, gert::attr::kNodeWeightId, "w0"));

  GertNodeWeightRef weight_ref;
  EXPECT_NE(ge::GRAPH_SUCCESS, GetNodeWeightRef(*node, weight_ref));
}
TEST_F(GertResourcesUT, ReadsSoFilesForKernelAndCustomPurposes) {
  const auto path = WriteTempFile("so_purposes.bin", {0x7FU, 0x45U, 0x4CU, 0x46U});
  struct Purpose {
    const char *name;
    GertSoPurpose expected;
  };
  for (const auto &purpose : {Purpose{"kernel", GertSoPurpose::kKernel}, Purpose{"custom", GertSoPurpose::kCustom}}) {
    ge::ExecuteGraph graph("so_path");
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kSoIds, {"so"}));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey("so", "name"), "libtest.so"));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey("so", "purpose"), purpose.name));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey("so", "path"), path));
    GertResourceTable table;
    ASSERT_EQ(ParseGertResources(graph, table), ge::GRAPH_SUCCESS);
    ASSERT_EQ(table.sos.size(), 1U);
    EXPECT_EQ(table.sos[0].purpose, purpose.expected);
    EXPECT_EQ(table.sos[0].content, (std::vector<uint8_t>{0x7FU, 0x45U, 0x4CU, 0x46U}));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, SoKey("so", "path"), path + "/missing"));
    EXPECT_NE(ParseGertResources(graph, table), ge::GRAPH_SUCCESS);
    EXPECT_EQ(table.sos[0].purpose, purpose.expected);
  }
  EXPECT_EQ(std::remove(path.c_str()), 0);
}

TEST_F(GertResourcesUT, CopiesEmbeddedHostAddressesAndRejectsShortOrMissingFiles) {
  const std::vector<uint8_t> bytes{7U, 0U, 255U};
  ge::ExecuteGraph graph("host_address");
  ASSERT_NO_FATAL_FAILURE(SetWeightHeader(graph, "w", "embedded", "host", bytes.size()));
  ASSERT_TRUE(ge::AttrUtils::SetInt(graph, WeightKey("w", "address"),
                                    static_cast<int64_t>(reinterpret_cast<uintptr_t>(bytes.data()))));
  GertResourceTable table;
  ASSERT_EQ(ParseGertResources(graph, table), ge::GRAPH_SUCCESS);
  ASSERT_EQ(table.weights.size(), 1U);
  EXPECT_EQ(table.weights[0].content, bytes);
  const auto path = WriteTempFile("truncated_weight.bin", {1U});
  for (const auto &file : {path, path + "/missing"}) {
    ge::ExecuteGraph invalid("bad_weight_file");
    ASSERT_NO_FATAL_FAILURE(SetWeightHeader(invalid, "w", "embedded", "host", 3));
    ASSERT_TRUE(ge::AttrUtils::SetStr(invalid, WeightKey("w", "path"), file));
    EXPECT_NE(ParseGertResources(invalid, table), ge::GRAPH_SUCCESS);
    EXPECT_EQ(table.weights[0].content, bytes);
  }
  EXPECT_EQ(std::remove(path.c_str()), 0);
}

TEST_F(GertResourcesUT, RejectsEmptyIdsUnsupportedModesAndNonTensorPayloads) {
  for (const auto *id : {"", "valid"}) {
    ge::ExecuteGraph graph("invalid_payload");
    ASSERT_TRUE(ge::AttrUtils::SetListStr(graph, attr::kBinaryIds, {id}));
    ASSERT_TRUE(ge::AttrUtils::SetStr(graph, BinaryKey(id, "name"), "kernel.o"));
    ASSERT_TRUE(ge::AttrUtils::SetInt(graph, BinaryKey(id, "content"), 3));
    GertResourceTable table;
    EXPECT_NE(ParseGertResources(graph, table), ge::GRAPH_SUCCESS);
    EXPECT_TRUE(table.binaries.empty());
  }
  ge::ExecuteGraph graph("invalid_mode");
  ASSERT_NO_FATAL_FAILURE(SetWeightHeader(graph, "w", "unsupported", "host", 1));
  GertResourceTable table;
  EXPECT_NE(ParseGertResources(graph, table), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(table.weights.empty());
}
}  // namespace gert
