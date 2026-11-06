/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/helper/python_custom_op_package_helper.h"
#include "proto/python_custom_op.pb.h"

namespace ge {
namespace {
constexpr char kHexDigits[] = "0123456789abcdef";
constexpr uint32_t kTestBridgeAbi = 4U;
constexpr uint32_t kTestPythonMajor = 3U;
constexpr uint32_t kTestPythonMinor = 12U;

std::string ToLowerHex(const std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> &digest) {
  std::string hex;
  hex.reserve(PythonCustomOpDeploymentArtifact::kArtifactIdLen);
  for (const auto byte : digest) {
    hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
    hex.push_back(kHexDigits[byte & 0x0FU]);
  }
  return hex;
}

// 用递增字节填充出一个确定的伪摘要。builder 只校验 hex(sha256) 与 artifact_id 一致，
// 不重新计算摘要，因此测试无需引入真实的 SHA-256 实现。
// seed 取 uint32 并混入下标，保证任意数量的 artifact 都能得到互不相同的伪摘要
std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> MakeDigest(const uint32_t seed) {
  std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> digest{};
  for (size_t i = 0U; i < digest.size(); ++i) {
    digest[i] = static_cast<uint8_t>((seed >> ((i % 4U) * 8U)) ^ (seed + static_cast<uint32_t>(i)));
  }
  return digest;
}

PythonOpDescriptorSnapshot MakeSnapshot(const std::string &op_type, const std::string &class_name,
                                        const uint32_t capabilities = 8U) {
  return PythonOpDescriptorSnapshot{op_type, class_name, capabilities};
}

std::shared_ptr<PythonCustomOpDeploymentArtifact> MakeArtifact(
    const uint32_t seed, const std::vector<uint8_t> &source, const std::vector<PythonOpDescriptorSnapshot> &descriptors,
    const std::string &source_name = "plugin.py") {
  auto artifact = std::make_shared<PythonCustomOpDeploymentArtifact>();
  artifact->bridge_abi_version = kTestBridgeAbi;
  artifact->producer_python_major = kTestPythonMajor;
  artifact->producer_python_minor = kTestPythonMinor;
  artifact->source_sha256 = MakeDigest(seed);
  artifact->artifact_id = ToLowerHex(artifact->source_sha256);
  artifact->source_name = source_name;
  artifact->source = std::make_shared<std::vector<uint8_t>>(source);
  artifact->descriptors = descriptors;
  return artifact;
}

std::shared_ptr<PythonCustomOpDeploymentArtifact> MakeSimpleArtifact(const uint32_t seed, const std::string &op_type) {
  return MakeArtifact(seed, {'p', 'r', 'i', 'n', 't', '\n'}, {MakeSnapshot(op_type, op_type)});
}

bool ParsePackage(const std::vector<uint8_t> &bytes, python_custom_op::proto::PythonCustomOpPackageDef &package) {
  return package.ParseFromArray(bytes.data(), static_cast<int32_t>(bytes.size()));
}
}  // namespace

TEST(UtestPythonCustomOpPackage, validate_artifact_accepts_well_formed) {
  const auto artifact = MakeSimpleArtifact(0x10U, "PortableOp");
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), SUCCESS);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_bad_artifact_id_length) {
  auto artifact = MakeSimpleArtifact(0x10U, "PortableOp");
  artifact->artifact_id = "abc";
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_uppercase_artifact_id) {
  auto artifact = MakeSimpleArtifact(0x10U, "PortableOp");
  artifact->artifact_id = ToLowerHex(artifact->source_sha256);
  for (auto &ch : artifact->artifact_id) {
    ch = static_cast<char>(toupper(ch));
  }
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_id_not_matching_digest) {
  auto artifact = MakeSimpleArtifact(0x10U, "PortableOp");
  artifact->artifact_id = ToLowerHex(MakeDigest(0x20U));
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_empty_source) {
  const auto artifact = MakeArtifact(0x10U, {}, {MakeSnapshot("PortableOp", "PortableOp")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_null_source) {
  auto artifact = MakeSimpleArtifact(0x10U, "PortableOp");
  artifact->source = nullptr;
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_oversize_source) {
  const std::vector<uint8_t> big_source(PythonCustomOpPackageLimits::kMaxSourceBytes + 1U, 0x61U);
  const auto artifact = MakeArtifact(0x10U, big_source, {MakeSnapshot("PortableOp", "PortableOp")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_oversize_source_name) {
  const std::string long_name(PythonCustomOpPackageLimits::kMaxSourceNameBytes + 1U, 'a');
  const auto artifact = MakeArtifact(0x10U, {'x'}, {MakeSnapshot("PortableOp", "PortableOp")}, long_name);
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_empty_descriptors) {
  const auto artifact = MakeArtifact(0x10U, {'x'}, {});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_too_many_descriptors) {
  std::vector<PythonOpDescriptorSnapshot> descriptors;
  descriptors.reserve(PythonCustomOpPackageLimits::kMaxDescriptorsPerArtifact + 1U);
  for (size_t i = 0U; i <= PythonCustomOpPackageLimits::kMaxDescriptorsPerArtifact; ++i) {
    descriptors.push_back(MakeSnapshot("Op" + std::to_string(i), "Cls" + std::to_string(i)));
  }
  const auto artifact = MakeArtifact(0x10U, {'x'}, descriptors);
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_empty_op_type_or_class_name) {
  const auto no_op_type = MakeArtifact(0x10U, {'x'}, {MakeSnapshot("", "Cls")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*no_op_type), FAILED);

  const auto no_class_name = MakeArtifact(0x11U, {'x'}, {MakeSnapshot("Op", "")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*no_class_name), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_oversize_op_type) {
  const std::string long_op_type(PythonCustomOpPackageLimits::kMaxOpTypeBytes + 1U, 'a');
  const auto artifact = MakeArtifact(0x10U, {'x'}, {MakeSnapshot(long_op_type, "Cls")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, validate_artifact_rejects_duplicate_op_type_in_one_artifact) {
  const auto artifact = MakeArtifact(0x10U, {'x'}, {MakeSnapshot("SameOp", "ClsA"), MakeSnapshot("SameOp", "ClsB")});
  EXPECT_EQ(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact), FAILED);
}

TEST(UtestPythonCustomOpPackage, add_artifact_rejects_null) {
  PythonCustomOpPackageBuilder builder;
  EXPECT_EQ(builder.AddArtifact(nullptr), FAILED);
  EXPECT_TRUE(builder.Empty());
}

TEST(UtestPythonCustomOpPackage, add_artifact_deduplicates_identical_content) {
  PythonCustomOpPackageBuilder builder;
  const auto first = MakeSimpleArtifact(0x10U, "PortableOp");
  const auto second = MakeSimpleArtifact(0x10U, "PortableOp");
  ASSERT_EQ(builder.AddArtifact(first), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(second), SUCCESS);
  EXPECT_EQ(builder.GetArtifactCount(), 1U);
}

TEST(UtestPythonCustomOpPackage, add_artifact_rejects_same_id_with_different_source_name) {
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "PortableOp")), SUCCESS);
  const auto conflicting =
      MakeArtifact(0x10U, {'p', 'r', 'i', 'n', 't', '\n'}, {MakeSnapshot("PortableOp", "PortableOp")}, "other_name.py");
  EXPECT_EQ(builder.AddArtifact(conflicting), FAILED);
}

TEST(UtestPythonCustomOpPackage, add_artifact_rejects_same_id_with_different_source) {
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "PortableOp")), SUCCESS);
  const auto conflicting = MakeArtifact(0x10U, {'d', 'i', 'f', 'f'}, {MakeSnapshot("PortableOp", "PortableOp")});
  EXPECT_EQ(builder.AddArtifact(conflicting), FAILED);
}

TEST(UtestPythonCustomOpPackage, add_artifact_rejects_exceeding_artifact_count) {
  PythonCustomOpPackageBuilder builder;
  for (size_t i = 0U; i < PythonCustomOpPackageLimits::kMaxArtifactCount; ++i) {
    ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(static_cast<uint32_t>(i), "Op" + std::to_string(i))), SUCCESS);
  }
  EXPECT_EQ(builder.GetArtifactCount(), PythonCustomOpPackageLimits::kMaxArtifactCount);
  // 第 257 个不同 artifact 必须被拒绝；seed 取 0x1000 以避开上方 0..255 的摘要空间，
  // 确保失败原因是数量超限而不是 artifact_id 冲突
  const auto overflow =
      MakeArtifact(0x1000U, {'o', 'v', 'e', 'r', 'f', 'l', 'o', 'w'}, {MakeSnapshot("OverflowOp", "OverflowOp")});
  EXPECT_EQ(builder.AddArtifact(overflow), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_rejects_empty_builder) {
  PythonCustomOpPackageBuilder builder;
  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
  EXPECT_TRUE(out.empty());
}

TEST(UtestPythonCustomOpPackage, build_rejects_inconsistent_bridge_abi) {
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "OpA")), SUCCESS);
  auto other = MakeSimpleArtifact(0x20U, "OpB");
  other->bridge_abi_version = kTestBridgeAbi + 1U;
  ASSERT_EQ(builder.AddArtifact(other), SUCCESS);

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_rejects_inconsistent_producer_python) {
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "OpA")), SUCCESS);
  auto other = MakeSimpleArtifact(0x20U, "OpB");
  other->producer_python_minor = kTestPythonMinor + 1U;
  ASSERT_EQ(builder.AddArtifact(other), SUCCESS);

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_rejects_zero_bridge_abi) {
  PythonCustomOpPackageBuilder builder;
  auto artifact = MakeSimpleArtifact(0x10U, "OpA");
  artifact->bridge_abi_version = 0U;
  ASSERT_EQ(builder.AddArtifact(artifact), SUCCESS);

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_rejects_duplicate_op_type_across_artifacts) {
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "SameOp")), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x20U, "SameOp")), SUCCESS);

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_rejects_exceeding_total_descriptor_count) {
  PythonCustomOpPackageBuilder builder;
  // 5 个 artifact × 1024 descriptor = 5120 > 4096 上限
  const size_t descriptors_per_artifact = PythonCustomOpPackageLimits::kMaxDescriptorsPerArtifact;
  for (size_t artifact_index = 0U; artifact_index < 5U; ++artifact_index) {
    std::vector<PythonOpDescriptorSnapshot> descriptors;
    descriptors.reserve(descriptors_per_artifact);
    for (size_t i = 0U; i < descriptors_per_artifact; ++i) {
      descriptors.push_back(
          MakeSnapshot("A" + std::to_string(artifact_index) + "Op" + std::to_string(i), "Cls" + std::to_string(i)));
    }
    ASSERT_EQ(builder.AddArtifact(MakeArtifact(static_cast<uint32_t>(artifact_index), {'x'}, descriptors)), SUCCESS);
  }

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

TEST(UtestPythonCustomOpPackage, build_round_trips_single_artifact) {
  PythonCustomOpPackageBuilder builder;
  const std::vector<uint8_t> source = {'i', 'm', 'p', 'o', 'r', 't', '\n'};
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x10U, source, {MakeSnapshot("PortableOp", "Outer.Nested", 8U)})),
            SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);
  ASSERT_FALSE(out.empty());

  python_custom_op::proto::PythonCustomOpPackageDef package;
  ASSERT_TRUE(ParsePackage(out, package));
  EXPECT_EQ(package.format_version(), PythonCustomOpPackageLimits::kFormatVersion);
  EXPECT_EQ(package.bridge_abi_version(), kTestBridgeAbi);
  EXPECT_EQ(package.producer_python_major(), kTestPythonMajor);
  EXPECT_EQ(package.producer_python_minor(), kTestPythonMinor);
  ASSERT_EQ(package.sources_size(), 1);

  const auto &def = package.sources(0);
  EXPECT_EQ(def.artifact_id(), ToLowerHex(MakeDigest(0x10U)));
  EXPECT_EQ(def.source_name(), "plugin.py");
  const std::vector<uint8_t> sha256_bytes(def.source_sha256().begin(), def.source_sha256().end());
  EXPECT_EQ(sha256_bytes.size(), PythonCustomOpDeploymentArtifact::kSha256Len);
  const std::vector<uint8_t> source_bytes(def.source().begin(), def.source().end());
  EXPECT_EQ(source_bytes, source);
  ASSERT_EQ(def.descriptors_size(), 1);
  EXPECT_EQ(def.descriptors(0).op_type(), "PortableOp");
  EXPECT_EQ(def.descriptors(0).class_name(), "Outer.Nested");
  EXPECT_EQ(def.descriptors(0).capabilities(), 8U);
}

TEST(UtestPythonCustomOpPackage, build_is_deterministic) {
  std::vector<uint8_t> first_out;
  std::vector<uint8_t> second_out;
  for (auto *out : {&first_out, &second_out}) {
    PythonCustomOpPackageBuilder builder;
    ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x20U, "OpB")), SUCCESS);
    ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "OpA")), SUCCESS);
    ASSERT_EQ(builder.Build(*out), SUCCESS);
  }
  EXPECT_EQ(first_out, second_out);
  EXPECT_FALSE(first_out.empty());
}

TEST(UtestPythonCustomOpPackage, build_sorts_sources_by_artifact_id) {
  PythonCustomOpPackageBuilder builder;
  // 故意乱序加入：digest 0x20 的 artifact_id 大于 digest 0x10 的
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x20U, "OpB")), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(MakeSimpleArtifact(0x10U, "OpA")), SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);

  python_custom_op::proto::PythonCustomOpPackageDef package;
  ASSERT_TRUE(ParsePackage(out, package));
  ASSERT_EQ(package.sources_size(), 2);
  EXPECT_EQ(package.sources(0).artifact_id(), ToLowerHex(MakeDigest(0x10U)));
  EXPECT_EQ(package.sources(1).artifact_id(), ToLowerHex(MakeDigest(0x20U)));
  EXPECT_LT(package.sources(0).artifact_id(), package.sources(1).artifact_id());
}

// op_type 全局唯一（模型级 registry 的 key），descriptor 按 (op_type, class_name) 升序输出；
// 同 op_type 多 descriptor 的拒绝路径由 validate_artifact_rejects_duplicate_op_type_in_one_artifact 覆盖。
TEST(UtestPythonCustomOpPackage, build_sorts_descriptors_by_op_type_then_class_name) {
  std::vector<PythonOpDescriptorSnapshot> descriptors = {
      MakeSnapshot("Zebra", "ZebraCls"),
      MakeSnapshot("Cherry", "Mid"),
      MakeSnapshot("Apple", "AppleCls"),
  };
  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x10U, {'x'}, descriptors)), SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);

  python_custom_op::proto::PythonCustomOpPackageDef package;
  ASSERT_TRUE(ParsePackage(out, package));
  ASSERT_EQ(package.sources_size(), 1);
  const auto &def = package.sources(0);
  ASSERT_EQ(def.descriptors_size(), 3);
  EXPECT_EQ(def.descriptors(0).op_type(), "Apple");
  EXPECT_EQ(def.descriptors(0).class_name(), "AppleCls");
  EXPECT_EQ(def.descriptors(1).op_type(), "Cherry");
  EXPECT_EQ(def.descriptors(1).class_name(), "Mid");
  EXPECT_EQ(def.descriptors(2).op_type(), "Zebra");
  EXPECT_EQ(def.descriptors(2).class_name(), "ZebraCls");
}

TEST(UtestPythonCustomOpPackage, build_rejects_exceeding_partition_size_before_serializing) {
  // 9 个 8 MiB 源码 = 72 MiB > 64 MiB 分区上限，序列化前按 ByteSizeLong 精确判定并拒绝
  const std::vector<uint8_t> max_source(PythonCustomOpPackageLimits::kMaxSourceBytes, 0x61U);
  PythonCustomOpPackageBuilder builder;
  for (size_t i = 0U; i < 9U; ++i) {
    const auto artifact = MakeArtifact(static_cast<uint32_t>(i), max_source,
                                       {MakeSnapshot("Op" + std::to_string(i), "Cls" + std::to_string(i))});
    ASSERT_EQ(builder.AddArtifact(artifact), SUCCESS);
  }

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
  EXPECT_TRUE(out.empty());
}

TEST(UtestPythonCustomOpPackage, artifact_id_consistency_helper) {
  const auto digest = MakeDigest(0x30U);
  EXPECT_TRUE(PythonCustomOpPackageBuilder::IsArtifactIdConsistentWithSha256(ToLowerHex(digest), digest));
  EXPECT_FALSE(PythonCustomOpPackageBuilder::IsArtifactIdConsistentWithSha256("too_short", digest));
  EXPECT_FALSE(PythonCustomOpPackageBuilder::IsArtifactIdConsistentWithSha256(ToLowerHex(MakeDigest(0x31U)), digest));
  const std::string non_hex(PythonCustomOpDeploymentArtifact::kArtifactIdLen, 'z');
  EXPECT_FALSE(PythonCustomOpPackageBuilder::IsArtifactIdConsistentWithSha256(non_hex, digest));
}

// ---- ParseAndValidatePythonCustomOpPackage（加载侧 codec，Build 的镜像逆）----

// 用 builder 产出一份合法 package bytes，作为 parse 测试的基准输入。
std::vector<uint8_t> BuildValidPackageBytesForParseUt(
    const std::vector<std::shared_ptr<PythonCustomOpDeploymentArtifact>> &artifacts) {
  PythonCustomOpPackageBuilder builder;
  for (const auto &artifact : artifacts) {
    if (builder.AddArtifact(artifact) != SUCCESS) {
      return {};
    }
  }
  std::vector<uint8_t> out;
  if (builder.Build(out) != SUCCESS) {
    return {};
  }
  return out;
}

// 把合法 package bytes 解析成 proto、按 mutator 篡改、再序列化，用于构造各类非法输入。
// parse 侧不校验 deterministic，故此处普通序列化即可。
std::vector<uint8_t> MutatePackageBytesForParseUt(
    const std::vector<uint8_t> &bytes,
    const std::function<void(python_custom_op::proto::PythonCustomOpPackageDef &)> &mutator) {
  python_custom_op::proto::PythonCustomOpPackageDef package;
  if (!package.ParseFromArray(bytes.data(), static_cast<int32_t>(bytes.size()))) {
    return {};
  }
  mutator(package);
  std::string serialized;
  if (!package.SerializeToString(&serialized)) {
    return {};
  }
  return std::vector<uint8_t>(serialized.begin(), serialized.end());
}

TEST(UtestPythonCustomOpPackage, parse_round_trips_valid_package) {
  const std::vector<uint8_t> source = {'i', 'm', 'p', 'o', 'r', 't', '\n'};
  const auto bytes = BuildValidPackageBytesForParseUt(
      {MakeArtifact(0x10U, source, {MakeSnapshot("OpA", "Outer.A", 8U)}),
       MakeArtifact(0x20U, source, {MakeSnapshot("OpB", "Outer.B", 8U), MakeSnapshot("OpC", "Outer.C", 0U)})});
  ASSERT_FALSE(bytes.empty());

  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(bytes.data(), bytes.size(), parsed), SUCCESS);
  EXPECT_EQ(parsed.format_version, PythonCustomOpPackageLimits::kFormatVersion);
  EXPECT_EQ(parsed.bridge_abi_version, kTestBridgeAbi);
  EXPECT_EQ(parsed.producer_python_major, kTestPythonMajor);
  EXPECT_EQ(parsed.producer_python_minor, kTestPythonMinor);
  ASSERT_EQ(parsed.artifacts.size(), 2U);
  // sources 升序：0x10 的 artifact_id 小于 0x20
  EXPECT_EQ(parsed.artifacts[0]->artifact_id, ToLowerHex(MakeDigest(0x10U)));
  EXPECT_EQ(parsed.artifacts[0]->source_name, "plugin.py");
  EXPECT_EQ(*(parsed.artifacts[0]->source), source);
  ASSERT_EQ(parsed.artifacts[0]->descriptors.size(), 1U);
  EXPECT_EQ(parsed.artifacts[0]->descriptors[0].op_type, "OpA");
  EXPECT_EQ(parsed.artifacts[1]->descriptors.size(), 2U);
  EXPECT_EQ(parsed.artifacts[1]->descriptors[1].op_type, "OpC");
  // 环境字段在 package 顶层只有一份，解包时必须回填到每个 artifact：恢复出的
  // adapter 携带 artifact，模型「加载→再保存」时该 artifact 原样交给 Build()，
  // 而 Build 拒绝 ABI 为 0 的 artifact（回归：只填顶层导致二次保存必定失败）。
  for (const auto &artifact : parsed.artifacts) {
    EXPECT_EQ(artifact->bridge_abi_version, kTestBridgeAbi);
    EXPECT_EQ(artifact->producer_python_major, kTestPythonMajor);
    EXPECT_EQ(artifact->producer_python_minor, kTestPythonMinor);
  }
}

// 模型「加载→再保存」路径：加载侧恢复出的 adapter 携带 deployment_artifacts（来自本次
// parse），再次保存时这个 artifact 被原样交给 Build()。Build 拒绝 ABI 为 0 的 artifact，
// 因此解析必须把顶层环境字段回填到每个 artifact，否则缓存落盘/级联导出必定失败。
TEST(UtestPythonCustomOpPackage, parse_then_rebuild_round_trips_parsed_artifacts) {
  const std::vector<uint8_t> source = {'i', 'm', 'p', 'o', 'r', 't', '\n'};
  const auto bytes =
      BuildValidPackageBytesForParseUt({MakeArtifact(0x10U, source, {MakeSnapshot("OpA", "Outer.A", 8U)}),
                                        MakeArtifact(0x20U, source, {MakeSnapshot("OpB", "Outer.B", 8U)})});
  ASSERT_FALSE(bytes.empty());

  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(bytes.data(), bytes.size(), parsed), SUCCESS);
  ASSERT_EQ(parsed.artifacts.size(), 2U);

  PythonCustomOpPackageBuilder rebuild;
  for (const auto &artifact : parsed.artifacts) {
    ASSERT_EQ(rebuild.AddArtifact(artifact), SUCCESS);
  }
  std::vector<uint8_t> rebuilt;
  ASSERT_EQ(rebuild.Build(rebuilt), SUCCESS);
  // 确定性序列化：同一批 artifact 重建出逐字节相同的分区
  EXPECT_EQ(rebuilt, bytes);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_null_or_empty) {
  ParsedPythonCustomOpPackage parsed;
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(nullptr, bytes.size(), parsed), FAILED);
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(bytes.data(), 0U, parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_oversize_partition_before_parsing) {
  // 上限检查先于任何解析：传合法 buffer 但声明超限 len，必须直接 FAILED（不读越界）。
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(
      ParseAndValidatePythonCustomOpPackage(bytes.data(), PythonCustomOpPackageLimits::kMaxPartitionBytes + 1U, parsed),
      FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_bad_protobuf) {
  const std::vector<uint8_t> garbage = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(garbage.data(), garbage.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_wrong_format_version) {
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        package.set_format_version(PythonCustomOpPackageLimits::kFormatVersion + 1U);
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_accepts_abi_mismatch_as_diagnostic_only) {
  // 构造一个「由另一套 bridge 函数表形状产出」的包：把顶层 abi 改成当前运行时不认识的
  // 取值，模拟跨版本产出。bridge 函数表形状不属于 OM 的字节契约，解析必须照常成功，
  // 只把这个值当诊断信息保留。若在此硬失败，则「只增删一个函数表槽位」就会让此前产出的
  // 全部 OM 存档与模型缓存无法加载，而 OM 字节一个都没变。格式闸由 format_version 负责。
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  constexpr uint32_t kForeignAbi = kTestBridgeAbi + 1U;
  const auto mutated = MutatePackageBytesForParseUt(
      bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &pkg) { pkg.set_bridge_abi_version(kForeignAbi); });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), SUCCESS);
  EXPECT_EQ(parsed.bridge_abi_version, kForeignAbi);
  ASSERT_EQ(parsed.artifacts.size(), 1U);
  EXPECT_EQ(parsed.artifacts[0]->descriptors[0].op_type, "OpA");
  // 环境字段仍按 package 顶层原值回填，供「加载→再保存」与排查使用
  EXPECT_EQ(parsed.artifacts[0]->bridge_abi_version, kForeignAbi);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_unsorted_sources) {
  const std::vector<uint8_t> source = {'p', 'a', 's', 's'};
  const auto bytes = BuildValidPackageBytesForParseUt({MakeArtifact(0x10U, source, {MakeSnapshot("OpA", "A")}),
                                                       MakeArtifact(0x20U, source, {MakeSnapshot("OpB", "B")})});
  ASSERT_FALSE(bytes.empty());
  // 交换两条 source 顺序，破坏 artifact_id 升序契约
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        ASSERT_EQ(package.sources_size(), 2);
        auto first = package.sources(0);
        package.mutable_sources(0)->CopyFrom(package.sources(1));
        package.mutable_sources(1)->CopyFrom(first);
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_unsorted_descriptors) {
  const std::vector<uint8_t> source = {'p', 'a', 's', 's'};
  const auto bytes = BuildValidPackageBytesForParseUt(
      {MakeArtifact(0x10U, source, {MakeSnapshot("OpA", "A"), MakeSnapshot("OpB", "B")})});
  ASSERT_FALSE(bytes.empty());
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        auto *def = package.mutable_sources(0);
        ASSERT_EQ(def->descriptors_size(), 2);
        auto first = def->descriptors(0);
        def->mutable_descriptors(0)->CopyFrom(def->descriptors(1));
        def->mutable_descriptors(1)->CopyFrom(first);
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_duplicate_op_type_across_artifacts) {
  const std::vector<uint8_t> source = {'p', 'a', 's', 's'};
  // builder 自身拒绝跨 artifact 重复 op_type，故先产一份合法的（OpA/Other），再篡改 proto 注入重复。
  const auto bytes = BuildValidPackageBytesForParseUt({MakeArtifact(0x10U, source, {MakeSnapshot("Dup", "A")}),
                                                       MakeArtifact(0x20U, source, {MakeSnapshot("Other", "B")})});
  ASSERT_FALSE(bytes.empty());
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        // 把第二个 artifact 的 op_type 改成与第一个相同（保持 descriptors 单条，无需重排序）
        package.mutable_sources(1)->mutable_descriptors(0)->set_op_type("Dup");
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_empty_sources) {
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  const auto mutated = MutatePackageBytesForParseUt(
      bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) { package.clear_sources(); });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_bad_sha256_length) {
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        package.mutable_sources(0)->set_source_sha256(std::string(8U, '\x01'));
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

TEST(UtestPythonCustomOpPackage, parse_rejects_artifact_id_inconsistent_with_sha256) {
  const auto bytes = BuildValidPackageBytesForParseUt({MakeSimpleArtifact(0x10U, "OpA")});
  ASSERT_FALSE(bytes.empty());
  const auto mutated =
      MutatePackageBytesForParseUt(bytes, [](python_custom_op::proto::PythonCustomOpPackageDef &package) {
        // artifact_id 仍是合法 64 hex，但不再等于 source_sha256 的 hex
        package.mutable_sources(0)->set_artifact_id(ToLowerHex(MakeDigest(0x99U)));
      });
  ASSERT_FALSE(mutated.empty());
  ParsedPythonCustomOpPackage parsed;
  EXPECT_EQ(ParseAndValidatePythonCustomOpPackage(mutated.data(), mutated.size(), parsed), FAILED);
}

// canonical IR 快照（inputs/attrs/outputs）必须在 build/parse 全链路无损往返：
// 保存侧 FreezeArtifactIrSnapshots 从 OperatorFactory 冻结进 descriptor，
// 加载侧 IngestDeploymentArtifact 经 view 深拷贝，字段缺失即 schema-bound 绑定失败。
// 序列化侧：proto 字段逐项一致
static void ExpectProtoIrSnapshotsMatch(const python_custom_op::proto::PythonCustomOpPackageDef &package) {
  ASSERT_EQ(package.sources(0).descriptors_size(), 1);
  const auto &desc = package.sources(0).descriptors(0);
  ASSERT_EQ(desc.inputs_size(), 3);
  EXPECT_EQ(desc.inputs(0).name(), "x");
  EXPECT_EQ(desc.inputs(0).kind(), 0U);
  EXPECT_EQ(desc.inputs(1).name(), "y");
  EXPECT_EQ(desc.inputs(1).kind(), 1U);
  EXPECT_EQ(desc.inputs(2).name(), "dyn");
  EXPECT_EQ(desc.inputs(2).kind(), 2U);
  ASSERT_EQ(desc.attrs_size(), 2);
  EXPECT_EQ(desc.attrs(0).name(), "factor");
  EXPECT_EQ(desc.attrs(0).type(), "required_float");
  EXPECT_EQ(desc.attrs(1).name(), "mode");
  EXPECT_EQ(desc.attrs(1).type(), "int");
  ASSERT_EQ(desc.outputs_size(), 2);
  EXPECT_EQ(desc.outputs(0).name(), "z");
  EXPECT_EQ(desc.outputs(0).kind(), 0U);
  EXPECT_EQ(desc.outputs(1).name(), "dyn_out");
  EXPECT_EQ(desc.outputs(1).kind(), 1U);
}

// 反序列化侧：ParsedPythonCustomOpPackage 恢复出等价快照
static void ExpectParsedIrSnapshotsMatch(const ParsedPythonCustomOpPackage &parsed) {
  ASSERT_EQ(parsed.artifacts.size(), 1U);
  ASSERT_EQ(parsed.artifacts[0]->descriptors.size(), 1U);
  const auto &snapshot = parsed.artifacts[0]->descriptors[0];
  ASSERT_EQ(snapshot.ir_inputs.size(), 3U);
  EXPECT_EQ(snapshot.ir_inputs[0].name, "x");
  EXPECT_EQ(snapshot.ir_inputs[0].kind, 0U);
  EXPECT_EQ(snapshot.ir_inputs[1].name, "y");
  EXPECT_EQ(snapshot.ir_inputs[1].kind, 1U);
  EXPECT_EQ(snapshot.ir_inputs[2].name, "dyn");
  EXPECT_EQ(snapshot.ir_inputs[2].kind, 2U);
  ASSERT_EQ(snapshot.ir_attrs.size(), 2U);
  EXPECT_EQ(snapshot.ir_attrs[0].name, "factor");
  EXPECT_EQ(snapshot.ir_attrs[0].type, "required_float");
  ASSERT_EQ(snapshot.ir_outputs.size(), 2U);
  EXPECT_EQ(snapshot.ir_outputs[1].name, "dyn_out");
  EXPECT_EQ(snapshot.ir_outputs[1].kind, 1U);
}

TEST(UtestPythonCustomOpPackage, build_round_trips_ir_snapshots) {
  PythonOpDescriptorSnapshot with_ir = MakeSnapshot("OpIr", "Outer.Ir", 8U);
  with_ir.ir_inputs = {PythonIrInputSnapshot{"x", 0U}, PythonIrInputSnapshot{"y", 1U},
                       PythonIrInputSnapshot{"dyn", 2U}};
  with_ir.ir_attrs = {PythonIrAttrSnapshot{"factor", "required_float"}, PythonIrAttrSnapshot{"mode", "int"}};
  with_ir.ir_outputs = {PythonIrOutputSnapshot{"z", 0U}, PythonIrOutputSnapshot{"dyn_out", 1U}};

  PythonCustomOpPackageBuilder builder;
  const std::vector<uint8_t> source = {'p', 'a', 's', 's', '\n'};
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x30U, source, {with_ir})), SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);
  ASSERT_FALSE(out.empty());

  python_custom_op::proto::PythonCustomOpPackageDef package;
  ASSERT_TRUE(ParsePackage(out, package));
  ExpectProtoIrSnapshotsMatch(package);

  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(out.data(), out.size(), parsed), SUCCESS);
  ExpectParsedIrSnapshotsMatch(parsed);
}

// 无 IR 的 descriptor（老格式/查不到 OperatorFactory 注册）必须照常通过：
// 字段缺省时 proto3 不写出，parse 后保持空 vector，加载侧按既有语义处理。
TEST(UtestPythonCustomOpPackage, build_accepts_descriptor_without_ir) {
  PythonCustomOpPackageBuilder builder;
  const std::vector<uint8_t> source = {'p', 'a', 's', 's', '\n'};
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x40U, source, {MakeSnapshot("OpPlain", "Outer.Plain", 8U)})), SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);
  ASSERT_FALSE(out.empty());

  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(out.data(), out.size(), parsed), SUCCESS);
  ASSERT_EQ(parsed.artifacts.size(), 1U);
  ASSERT_EQ(parsed.artifacts[0]->descriptors.size(), 1U);
  EXPECT_TRUE(parsed.artifacts[0]->descriptors[0].ir_inputs.empty());
  EXPECT_TRUE(parsed.artifacts[0]->descriptors[0].ir_attrs.empty());
  EXPECT_TRUE(parsed.artifacts[0]->descriptors[0].ir_outputs.empty());
}
}  // namespace ge
