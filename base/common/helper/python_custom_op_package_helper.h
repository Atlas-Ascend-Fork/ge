/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef BASE_COMMON_HELPER_PYTHON_CUSTOM_OP_PACKAGE_HELPER_H_
#define BASE_COMMON_HELPER_PYTHON_CUSTOM_OP_PACKAGE_HELPER_H_

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ge/ge_api_error_codes.h"
#include "graph/custom_op/artifact_provider.h"

namespace ge {
// PYTHON_CUSTOM_OPS 分区的固定资源限制。集中定义，不新增用户配置项。
struct PythonCustomOpPackageLimits {
  static constexpr uint32_t kFormatVersion = 1U;
  static constexpr uint64_t kMaxPartitionBytes = 64ULL * 1024ULL * 1024ULL;
  static constexpr uint64_t kMaxSourceBytes = 8ULL * 1024ULL * 1024ULL;
  static constexpr size_t kMaxArtifactCount = 256U;
  static constexpr size_t kMaxDescriptorsPerArtifact = 1024U;
  static constexpr size_t kMaxTotalDescriptors = 4096U;
  static constexpr size_t kMaxOpTypeBytes = 1024U;
  static constexpr size_t kMaxClassNameBytes = 1024U;
  static constexpr size_t kMaxSourceNameBytes = 4096U;
};

// PYTHON_CUSTOM_OPS 分区的构造器：收集去重后的 Python 部署 artifact，
// 校验资源限制与全局 op_type 唯一性，按稳定顺序生成 deterministic Protobuf bytes。
// 所有保存入口（ATC 离线、在线保存、模型缓存首次落盘）复用同一个 builder。
class PythonCustomOpPackageBuilder {
 public:
  // 收集一个 artifact。artifact_id 相同时要求内容完全一致，否则视为内部冲突并失败。
  Status AddArtifact(const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &artifact);

  bool Empty() const {
    return artifacts_.empty();
  }

  size_t GetArtifactCount() const {
    return artifacts_.size();
  }

  // 校验 + 排序 + deterministic 序列化。out 在成功时被整体替换。
  // package 头部的 bridge ABI 与 producer Python 版本取自 artifact 自身，
  // 并要求全部 artifact 一致，避免 base 层依赖 runtime/custom_op 的 ABI 常量。
  Status Build(std::vector<uint8_t> &out) const;

  // 单个 artifact 的静态校验，供 AddArtifact 与 UT 复用。
  static Status ValidateArtifact(const PythonCustomOpDeploymentArtifact &artifact);

  // 逐字段比较两个部署 artifact（含 descriptors 的 IR 与身份字段、环境字段）。
  // 供保存侧 AddArtifact 与加载侧 IngestDeploymentArtifact 共用同一"同 id 必须同内容"不变量。
  static bool IsSameDeploymentArtifact(const PythonCustomOpDeploymentArtifact &lhs,
                                       const PythonCustomOpDeploymentArtifact &rhs);

  // artifact_id 必须是 source_sha256 的 64 字符小写十六进制形式。
  static bool IsArtifactIdConsistentWithSha256(
      const std::string &artifact_id,
      const std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> &source_sha256);

 private:
  Status CheckGlobalLimits(std::set<std::string> &seen_op_types) const;

  // key 为 artifact_id，std::map 天然提供升序遍历，满足 sources 按 artifact_id 升序的格式契约
  std::map<std::string, std::shared_ptr<const PythonCustomOpDeploymentArtifact>> artifacts_;
};

// 解析并结构校验后的 PYTHON_CUSTOM_OPS package 内存视图（Build 的逆产物）。
// 由加载侧恢复 hook 调用 ParseAndValidatePythonCustomOpPackage 产出；本身不含任何运行时
// 副作用（不执行 source、不加载 bridge、不触碰任何注册表），只承载校验通过的只读数据。
struct ParsedPythonCustomOpPackage {
  uint32_t format_version{0U};
  uint32_t bridge_abi_version{0U};
  uint32_t producer_python_major{0U};  // 仅诊断，加载侧不做相等或大小比较
  uint32_t producer_python_minor{0U};  // 仅诊断，加载侧不做相等或大小比较
  // 按 artifact_id 升序，与 Build 的排序契约一致；每个 artifact 的 source 为原始 bytes。
  std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> artifacts;
};

// Build 的镜像逆操作 + 无副作用结构校验，供加载侧恢复 hook 在执行任何 embedded source
// 之前调用（校验顺序即安全边界：全部结构校验通过后才产出 out）。
// bridge 函数表形状不属于 OM 的字节契约，分区里的 bridge_abi_version 与 producer Python
// 版本同样仅诊断、不参与相等比较，因此本函数不需要调用方的 ABI 常量。
// 校验项：分区非空与 64 MiB 上限、Protobuf 可解析、format_version 精确为 1、
// artifact/descriptor 数量与字符串长度上限、
// artifact_id 与 source_sha256 内部一致、sources 按 artifact_id 升序、
// descriptors 按 (op_type, class_name) 升序、op_type 全局唯一（原型声明条目豁免）、
// descriptor 总数上限与算术溢出。成功时 out 被整体填充；失败时 out 内容未定义，调用方不得使用。
Status ParseAndValidatePythonCustomOpPackage(const uint8_t *data, size_t len, ParsedPythonCustomOpPackage &out);

// 原型声明条目的判定：capabilities 恰为 kShapeInfer|kInferMeta（无任何实现位）。
// 分文件原型（proto.py 声明、impl.py 实现）随独立 artifact 携带，其 manifest 条目只
// 描述声明本身；加载侧据此识别原型条目并跳过实现身份比对，打包侧豁免 op_type 唯一性。
bool IsProtoDescriptor(const PythonOpDescriptorSnapshot &descriptor);
}  // namespace ge

#endif  // BASE_COMMON_HELPER_PYTHON_CUSTOM_OP_PACKAGE_HELPER_H_
