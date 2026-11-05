/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/helper/python_custom_op_package_helper.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "common/math/ge_math_util.h"
#include "common/proto_util/proto_util.h"
#include "framework/common/debug/ge_log.h"
#include "proto/python_custom_op.pb.h"

namespace ge {
namespace {
constexpr char kHexDigits[] = "0123456789abcdef";

std::string ToLowerHex(const std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> &digest) {
  std::string hex;
  hex.reserve(PythonCustomOpDeploymentArtifact::kArtifactIdLen);
  for (const auto byte : digest) {
    hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
    hex.push_back(kHexDigits[byte & 0x0FU]);
  }
  return hex;
}

bool IsLowerHexChar(const char ch) {
  return ((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'f'));
}

Status ValidateDescriptorList(const PythonCustomOpDeploymentArtifact &artifact) {
  if (artifact.descriptors.empty()) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] has no descriptor.", artifact.artifact_id.c_str());
    return FAILED;
  }
  if (artifact.descriptors.size() > PythonCustomOpPackageLimits::kMaxDescriptorsPerArtifact) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] descriptor count[%zu] exceeds limit[%zu].",
           artifact.artifact_id.c_str(), artifact.descriptors.size(),
           PythonCustomOpPackageLimits::kMaxDescriptorsPerArtifact);
    return FAILED;
  }
  std::set<std::string> op_types_in_artifact;
  for (const auto &descriptor : artifact.descriptors) {
    if (descriptor.op_type.empty() || (descriptor.op_type.size() > PythonCustomOpPackageLimits::kMaxOpTypeBytes)) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] has an op_type with invalid length[%zu].",
             artifact.artifact_id.c_str(), descriptor.op_type.size());
      return FAILED;
    }
    if (descriptor.class_name.empty() ||
        (descriptor.class_name.size() > PythonCustomOpPackageLimits::kMaxClassNameBytes)) {
      GELOGE(FAILED,
             "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] op_type[%s] has a class_name with invalid "
             "length[%zu].",
             artifact.artifact_id.c_str(), descriptor.op_type.c_str(), descriptor.class_name.size());
      return FAILED;
    }
    if (!op_types_in_artifact.insert(descriptor.op_type).second) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] declares op_type[%s] more than once.",
             artifact.artifact_id.c_str(), descriptor.op_type.c_str());
      return FAILED;
    }
    // canonical IR 快照：与 op_type 同级的字符串长度上限；kind 不做枚举判定
    // （加载侧 bridge 按位定义解释，未知 kind 在绑定参数时才点名失败）。
    for (const auto &field : descriptor.ir_inputs) {
      if (field.name.empty() || (field.name.size() > PythonCustomOpPackageLimits::kMaxOpTypeBytes)) {
        GELOGE(FAILED,
               "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] op_type[%s] has an ir input name with invalid "
               "length[%zu].",
               artifact.artifact_id.c_str(), descriptor.op_type.c_str(), field.name.size());
        return FAILED;
      }
    }
    for (const auto &field : descriptor.ir_attrs) {
      if (field.name.empty() || (field.name.size() > PythonCustomOpPackageLimits::kMaxOpTypeBytes) ||
          (field.type.size() > PythonCustomOpPackageLimits::kMaxOpTypeBytes)) {
        GELOGE(FAILED,
               "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] op_type[%s] has an ir attr with invalid "
               "name length[%zu] or type length[%zu].",
               artifact.artifact_id.c_str(), descriptor.op_type.c_str(), field.name.size(), field.type.size());
        return FAILED;
      }
    }
    for (const auto &field : descriptor.ir_outputs) {
      if (field.name.empty() || (field.name.size() > PythonCustomOpPackageLimits::kMaxOpTypeBytes)) {
        GELOGE(FAILED,
               "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] op_type[%s] has an ir output name with invalid "
               "length[%zu].",
               artifact.artifact_id.c_str(), descriptor.op_type.c_str(), field.name.size());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

void FillDescriptors(const PythonCustomOpDeploymentArtifact &artifact,
                     ::ge::python_custom_op::proto::PythonSourceArtifactDef &def) {
  // 格式契约要求 descriptors 按 (op_type, class_name) 升序，输入顺序不作要求，此处统一排序。
  std::vector<const PythonOpDescriptorSnapshot *> ordered;
  ordered.reserve(artifact.descriptors.size());
  for (const auto &descriptor : artifact.descriptors) {
    ordered.push_back(&descriptor);
  }
  (void)std::sort(ordered.begin(), ordered.end(),
                  [](const PythonOpDescriptorSnapshot *lhs, const PythonOpDescriptorSnapshot *rhs) {
                    if (lhs->op_type != rhs->op_type) {
                      return lhs->op_type < rhs->op_type;
                    }
                    return lhs->class_name < rhs->class_name;
                  });
  for (const auto *descriptor : ordered) {
    auto *item = def.add_descriptors();
    item->set_op_type(descriptor->op_type);
    item->set_class_name(descriptor->class_name);
    item->set_capabilities(descriptor->capabilities);
    // canonical IR 快照：老 artifact（未注入 IR）字段缺省，序列化结果与旧版本一致。
    for (const auto &input : descriptor->ir_inputs) {
      auto *ir = item->add_inputs();
      ir->set_name(input.name);
      ir->set_kind(input.kind);
    }
    for (const auto &attr : descriptor->ir_attrs) {
      auto *ir = item->add_attrs();
      ir->set_name(attr.name);
      ir->set_type(attr.type);
    }
    for (const auto &output : descriptor->ir_outputs) {
      auto *ir = item->add_outputs();
      ir->set_name(output.name);
      ir->set_kind(output.kind);
    }
  }
}

// 把一条 proto source 还原为 immutable artifact，供 ValidateArtifact 复用保存侧的全部单项校验。
// 仅做无损字段搬运与 sha256 定长校验，不做任何限制判定（限制判定统一交给 ValidateArtifact）。
// 环境字段（bridge ABI、producer Python 版本）只在 package 顶层存一份，解包时必须回填到每个
// artifact：恢复出的 adapter 携带 artifact，模型「加载→再保存」（缓存落盘/级联导出）时这个
// artifact 会被原样交给 Build()，而 Build 拒绝 ABI 为 0 的 artifact。
Status ArtifactFromProtoDef(const ::ge::python_custom_op::proto::PythonSourceArtifactDef &def,
                            const ParsedPythonCustomOpPackage &env,
                            std::shared_ptr<PythonCustomOpDeploymentArtifact> &out) {
  if (def.source_sha256().size() != PythonCustomOpDeploymentArtifact::kSha256Len) {
    GELOGE(FAILED,
           "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] artifact[%s] source_sha256 length[%zu] is not the required [%zu].",
           def.artifact_id().c_str(), def.source_sha256().size(), PythonCustomOpDeploymentArtifact::kSha256Len);
    return FAILED;
  }
  auto artifact = std::make_shared<PythonCustomOpDeploymentArtifact>();
  artifact->artifact_id = def.artifact_id();
  artifact->source_name = def.source_name();
  (void)std::copy_n(def.source_sha256().begin(), PythonCustomOpDeploymentArtifact::kSha256Len,
                    artifact->source_sha256.begin());
  artifact->source = std::make_shared<std::vector<uint8_t>>(def.source().begin(), def.source().end());
  artifact->bridge_abi_version = env.bridge_abi_version;
  artifact->producer_python_major = env.producer_python_major;
  artifact->producer_python_minor = env.producer_python_minor;
  artifact->descriptors.reserve(static_cast<size_t>(def.descriptors_size()));
  for (const auto &descriptor : def.descriptors()) {
    PythonOpDescriptorSnapshot snapshot;
    snapshot.op_type = descriptor.op_type();
    snapshot.class_name = descriptor.class_name();
    snapshot.capabilities = descriptor.capabilities();
    snapshot.ir_inputs.reserve(static_cast<size_t>(descriptor.inputs_size()));
    for (const auto &input : descriptor.inputs()) {
      snapshot.ir_inputs.push_back(PythonIrInputSnapshot{input.name(), input.kind()});
    }
    snapshot.ir_attrs.reserve(static_cast<size_t>(descriptor.attrs_size()));
    for (const auto &attr : descriptor.attrs()) {
      snapshot.ir_attrs.push_back(PythonIrAttrSnapshot{attr.name(), attr.type()});
    }
    snapshot.ir_outputs.reserve(static_cast<size_t>(descriptor.outputs_size()));
    for (const auto &output : descriptor.outputs()) {
      snapshot.ir_outputs.push_back(PythonIrOutputSnapshot{output.name(), output.kind()});
    }
    artifact->descriptors.push_back(std::move(snapshot));
  }
  out = std::move(artifact);
  return SUCCESS;
}
}  // namespace

// 原型声明条目的固定 capability 位：kShapeInfer | kInferMeta（无任何实现位）。
// 分文件原型（proto.py 声明、impl.py 实现）随独立 artifact 携带，其 manifest 条目
// 只描述声明本身；恢复侧据此识别原型条目并跳过实现身份比对（impl descriptor 不存在）。
// 声明在公共头（package_helper.h）供 restore_hook 与打包侧共用。
bool IsProtoDescriptor(const PythonOpDescriptorSnapshot &descriptor) {
  const auto infer_capabilities = static_cast<CustomOpCapabilityMask>(CustomOpCapability::kShapeInfer) |
                                  static_cast<CustomOpCapabilityMask>(CustomOpCapability::kInferMeta);
  return (descriptor.capabilities == infer_capabilities);
}

bool PythonCustomOpPackageBuilder::IsArtifactIdConsistentWithSha256(
    const std::string &artifact_id,
    const std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> &source_sha256) {
  if (artifact_id.size() != PythonCustomOpDeploymentArtifact::kArtifactIdLen) {
    return false;
  }
  for (const auto ch : artifact_id) {
    if (!IsLowerHexChar(ch)) {
      return false;
    }
  }
  return artifact_id == ToLowerHex(source_sha256);
}

Status PythonCustomOpPackageBuilder::ValidateArtifact(const PythonCustomOpDeploymentArtifact &artifact) {
  if (!IsArtifactIdConsistentWithSha256(artifact.artifact_id, artifact.source_sha256)) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact id is not the lowercase hex of its sha256 digest.");
    return FAILED;
  }
  if ((artifact.source == nullptr) || artifact.source->empty()) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] source is empty.", artifact.artifact_id.c_str());
    return FAILED;
  }
  if (artifact.source->size() > PythonCustomOpPackageLimits::kMaxSourceBytes) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] source size[%zu] exceeds limit[%llu].",
           artifact.artifact_id.c_str(), artifact.source->size(),
           static_cast<unsigned long long>(PythonCustomOpPackageLimits::kMaxSourceBytes));
    return FAILED;
  }
  if (artifact.source_name.size() > PythonCustomOpPackageLimits::kMaxSourceNameBytes) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact[%s] source_name length[%zu] exceeds limit[%zu].",
           artifact.artifact_id.c_str(), artifact.source_name.size(), PythonCustomOpPackageLimits::kMaxSourceNameBytes);
    return FAILED;
  }
  return ValidateDescriptorList(artifact);
}

Status PythonCustomOpPackageBuilder::AddArtifact(
    const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &artifact) {
  if (artifact == nullptr) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] deployment artifact is null.");
    return FAILED;
  }
  GE_CHK_STATUS_RET(ValidateArtifact(*artifact), "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] validate artifact failed.");

  const auto existing = artifacts_.find(artifact->artifact_id);
  if (existing == artifacts_.cend()) {
    if (artifacts_.size() >= PythonCustomOpPackageLimits::kMaxArtifactCount) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact count exceeds limit[%zu].",
             PythonCustomOpPackageLimits::kMaxArtifactCount);
      return FAILED;
    }
    (void)artifacts_.emplace(artifact->artifact_id, artifact);
    return SUCCESS;
  }

  // 同 artifact_id 必须对应完全相同的内容（全字段，与加载侧 IngestDeploymentArtifact 共用
  // IsSameDeploymentArtifact），否则说明摘要或冻结逻辑存在内部不一致。
  const auto &incumbent = *(existing->second);
  if (!IsSameDeploymentArtifact(incumbent, *artifact)) {
    GELOGE(FAILED,
           "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact id[%s] is reused by inconsistent content, source_name[%s] vs "
           "source_name[%s].",
           artifact->artifact_id.c_str(), incumbent.source_name.c_str(), artifact->source_name.c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool PythonCustomOpPackageBuilder::IsSameDeploymentArtifact(const PythonCustomOpDeploymentArtifact &lhs,
                                                            const PythonCustomOpDeploymentArtifact &rhs) {
  if ((lhs.artifact_id != rhs.artifact_id) || (lhs.source_name != rhs.source_name) ||
      (lhs.source_sha256 != rhs.source_sha256) || (lhs.bridge_abi_version != rhs.bridge_abi_version) ||
      (lhs.producer_python_major != rhs.producer_python_major) ||
      (lhs.producer_python_minor != rhs.producer_python_minor)) {
    return false;
  }
  if ((lhs.source == nullptr) || (rhs.source == nullptr) || (*lhs.source != *rhs.source)) {
    return false;
  }
  if (lhs.descriptors.size() != rhs.descriptors.size()) {
    return false;
  }
  for (size_t i = 0U; i < lhs.descriptors.size(); ++i) {
    const auto &left = lhs.descriptors[i];
    const auto &right = rhs.descriptors[i];
    if ((left.op_type != right.op_type) || (left.class_name != right.class_name) ||
        (left.capabilities != right.capabilities) || (left.ir_inputs != right.ir_inputs) ||
        (left.ir_attrs != right.ir_attrs) || (left.ir_outputs != right.ir_outputs)) {
      return false;
    }
  }
  return true;
}

Status PythonCustomOpPackageBuilder::CheckGlobalLimits(std::set<std::string> &seen_op_types) const {
  size_t total_descriptors = 0U;
  for (const auto &entry : artifacts_) {
    const auto &artifact = entry.second;
    if (CheckSizeTAddOverflow(total_descriptors, artifact->descriptors.size()) != SUCCESS) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] descriptor count accumulation overflowed.");
      return FAILED;
    }
    total_descriptors += artifact->descriptors.size();
    if (total_descriptors > PythonCustomOpPackageLimits::kMaxTotalDescriptors) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] total descriptor count[%zu] exceeds limit[%zu].",
             total_descriptors, PythonCustomOpPackageLimits::kMaxTotalDescriptors);
      return FAILED;
    }
    for (const auto &descriptor : artifact->descriptors) {
      // 原型条目豁免 op_type 唯一性：同 op_type 可同时出现在原型 artifact（声明）
      // 与实现 artifact（实现身份）—— 分文件原型的常态。原型条目不参与实现身份比对。
      if (IsProtoDescriptor(descriptor)) {
        continue;
      }
      if (!seen_op_types.insert(descriptor.op_type).second) {
        GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] op_type[%s] is declared by more than one artifact.",
               descriptor.op_type.c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status PythonCustomOpPackageBuilder::Build(std::vector<uint8_t> &out) const {
  if (artifacts_.empty()) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] no python deployment artifact to pack.");
    return FAILED;
  }
  std::set<std::string> seen_op_types;
  GE_CHK_STATUS_RET(CheckGlobalLimits(seen_op_types), "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] check global limits failed.");

  // 全部 artifact 由同一次 bridge 冻结产生，环境字段必须一致
  const auto &first = *(artifacts_.cbegin()->second);
  for (const auto &entry : artifacts_) {
    const auto &artifact = *(entry.second);
    if ((artifact.bridge_abi_version != first.bridge_abi_version) ||
        (artifact.producer_python_major != first.producer_python_major) ||
        (artifact.producer_python_minor != first.producer_python_minor)) {
      GELOGE(FAILED,
             "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifacts were frozen by inconsistent environments, bridge abi[%u] "
             "vs [%u], python[%u.%u] vs [%u.%u].",
             first.bridge_abi_version, artifact.bridge_abi_version, first.producer_python_major,
             first.producer_python_minor, artifact.producer_python_major, artifact.producer_python_minor);
      return FAILED;
    }
  }
  if (first.bridge_abi_version == 0U) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] artifact carries no bridge abi version.");
    return FAILED;
  }

  ::ge::python_custom_op::proto::PythonCustomOpPackageDef package;
  package.set_format_version(PythonCustomOpPackageLimits::kFormatVersion);
  package.set_bridge_abi_version(first.bridge_abi_version);
  package.set_producer_python_major(first.producer_python_major);
  package.set_producer_python_minor(first.producer_python_minor);
  // std::map 按 artifact_id 升序遍历，直接满足 sources 的排序契约。
  for (const auto &entry : artifacts_) {
    const auto &artifact = entry.second;
    auto *def = package.add_sources();
    def->set_artifact_id(artifact->artifact_id);
    def->set_source_name(artifact->source_name);
    def->set_source_sha256(artifact->source_sha256.data(), artifact->source_sha256.size());
    def->set_source(artifact->source->data(), artifact->source->size());
    FillDescriptors(*artifact, *def);
  }

  // 无 map 字段，deterministic 序列化不改变体积，ByteSizeLong 即为精确长度。
  const size_t exact_size = package.ByteSizeLong();
  if (exact_size > PythonCustomOpPackageLimits::kMaxPartitionBytes) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] serialized partition size[%zu] exceeds limit[%llu].", exact_size,
           static_cast<unsigned long long>(PythonCustomOpPackageLimits::kMaxPartitionBytes));
    return FAILED;
  }

  // 上方已按 kMaxPartitionBytes(64 MiB) 拒绝超限，此处窄化到 int32_t 不会溢出。
  std::vector<uint8_t> buffer(exact_size, 0U);
  {
    google::protobuf::io::ArrayOutputStream array_stream(buffer.data(), static_cast<int32_t>(exact_size));
    google::protobuf::io::CodedOutputStream coded_stream(&array_stream);
    coded_stream.SetSerializationDeterministic(true);
    if (!package.SerializeToCodedStream(&coded_stream)) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] serialize python custom op package failed.");
      return FAILED;
    }
  }

  out = std::move(buffer);
  GELOGI("[PYTHON_CUSTOM_OP][PACKAGE_BUILD] build package success, artifacts[%zu], descriptors[%zu], bytes[%zu].",
         artifacts_.size(), seen_op_types.size(), out.size());
  return SUCCESS;
}

Status ParseAndValidatePythonCustomOpPackage(const uint8_t *data, size_t len, ParsedPythonCustomOpPackage &out) {
  // 步骤 2：Protobuf 解析前检查分区非空与 64 MiB 总大小（无副作用，先于任何解析）。
  if ((data == nullptr) || (len == 0U)) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] partition is empty, data %p, len %zu.", data, len);
    return FAILED;
  }
  if (static_cast<uint64_t>(len) > PythonCustomOpPackageLimits::kMaxPartitionBytes) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] partition size[%zu] exceeds limit[%llu].", len,
           static_cast<unsigned long long>(PythonCustomOpPackageLimits::kMaxPartitionBytes));
    return FAILED;
  }

  ::ge::python_custom_op::proto::PythonCustomOpPackageDef package;
  // 步骤 3：Protobuf 解析（ReadProtoFromArray 内部 SetTotalBytesLimit 放宽到 INT_MAX，覆盖 64 MiB 上限）。
  // 上方已按 kMaxPartitionBytes 拒绝超限，此处窄化到 int32_t 不会溢出。
  if (!ReadProtoFromArray(data, static_cast<int32_t>(len), &package)) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] parse python custom op package failed, len %zu.", len);
    return FAILED;
  }

  // format_version 首期精确为 1；bridge_abi_version 与 producer Python 版本均为产出方
  // 环境信息，仅诊断。
  if (package.format_version() != PythonCustomOpPackageLimits::kFormatVersion) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] unsupported format_version[%u], expected[%u].",
           package.format_version(), PythonCustomOpPackageLimits::kFormatVersion);
    return FAILED;
  }

  ParsedPythonCustomOpPackage parsed;
  parsed.format_version = package.format_version();
  parsed.bridge_abi_version = package.bridge_abi_version();
  parsed.producer_python_major = package.producer_python_major();
  parsed.producer_python_minor = package.producer_python_minor();

  std::set<std::string> seen_op_types;
  size_t total_descriptors = 0U;
  std::string prev_artifact_id;
  for (int i = 0; i < package.sources_size(); ++i) {
    const auto &def = package.sources(i);
    std::shared_ptr<PythonCustomOpDeploymentArtifact> artifact;
    GE_CHK_STATUS_RET(ArtifactFromProtoDef(def, parsed, artifact),
                      "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] convert source[%d] to artifact failed.", i);
    // 复用保存侧单项校验：artifact_id↔sha256 一致、source 非空且≤上限、source_name≤上限、descriptor 列表合法。
    GE_CHK_STATUS_RET(PythonCustomOpPackageBuilder::ValidateArtifact(*artifact),
                      "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] validate artifact[%s] failed.", artifact->artifact_id.c_str());

    // 格式契约：sources 按 artifact_id 升序；descriptors 按 (op_type, class_name) 升序。
    if ((i > 0) && (def.artifact_id() <= prev_artifact_id)) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] sources are not strictly ascending by artifact_id at [%d].", i);
      return FAILED;
    }
    prev_artifact_id = def.artifact_id();
    for (int d = 1; d < def.descriptors_size(); ++d) {
      const auto &prev = def.descriptors(d - 1);
      const auto &cur = def.descriptors(d);
      const bool ordered = (prev.op_type() < cur.op_type()) ||
                           ((prev.op_type() == cur.op_type()) && (prev.class_name() < cur.class_name()));
      if (!ordered) {
        GELOGE(FAILED,
               "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] artifact[%s] descriptors are not strictly ascending by "
               "(op_type, class_name) at [%d].",
               def.artifact_id().c_str(), d);
        return FAILED;
      }
    }

    // 全局限制：artifact 数量、descriptor 总数（防溢出累加）、op_type 全局唯一。
    if (parsed.artifacts.size() >= PythonCustomOpPackageLimits::kMaxArtifactCount) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] artifact count exceeds limit[%zu].",
             PythonCustomOpPackageLimits::kMaxArtifactCount);
      return FAILED;
    }
    if (CheckSizeTAddOverflow(total_descriptors, artifact->descriptors.size()) != SUCCESS) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] descriptor count accumulation overflowed.");
      return FAILED;
    }
    total_descriptors += artifact->descriptors.size();
    if (total_descriptors > PythonCustomOpPackageLimits::kMaxTotalDescriptors) {
      GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] total descriptor count[%zu] exceeds limit[%zu].",
             total_descriptors, PythonCustomOpPackageLimits::kMaxTotalDescriptors);
      return FAILED;
    }
    for (const auto &descriptor : artifact->descriptors) {
      // 原型条目豁免 op_type 全局唯一（与 build 侧 CheckGlobalLimits 对称）：
      // 分文件原型下，同一 op_type 合法地同时出现在原型 artifact 与实现 artifact。
      if (IsProtoDescriptor(descriptor)) {
        continue;
      }
      if (!seen_op_types.insert(descriptor.op_type).second) {
        GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] op_type[%s] is declared by more than one artifact.",
               descriptor.op_type.c_str());
        return FAILED;
      }
    }
    parsed.artifacts.push_back(std::move(artifact));
  }

  if (parsed.artifacts.empty()) {
    GELOGE(FAILED, "[PYTHON_CUSTOM_OP][PACKAGE_PARSE] package carries no source artifact.");
    return FAILED;
  }

  out = std::move(parsed);
  GELOGI("[PYTHON_CUSTOM_OP][PACKAGE_PARSE] parse package success, artifacts[%zu], descriptors[%zu], abi[%u].",
         out.artifacts.size(), total_descriptors, out.bridge_abi_version);
  return SUCCESS;
}
}  // namespace ge
