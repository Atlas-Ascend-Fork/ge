/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef FUSION_ENGINE_INC_COMMON_OPS_DELIVERABLE_CONFIG_MANAGER_H_
#define FUSION_ENGINE_INC_COMMON_OPS_DELIVERABLE_CONFIG_MANAGER_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

namespace fe {
/**
 * @brief OpsDeliverableConfigManager.
 * 解析 $ASCEND_OPP_PATH/conf/ops_deliverables.json 中的算子交付件路径配置，
 * 文件校验与占位符（{}）替换逻辑与 graph_metadef ops_deliverables_manager 保持一致，
 * 解析的条目为：
 *   {ops-info, json}        算子信息库路径列表
 *   {opkernel, obj_file}    算子二进制路径列表
 *   {opkernel-config, json} 算子二进制索引路径列表
 *   {opsrc, src_file}       算子交付件根目录（单条，配置到 tbe 层；built-in 实现目录为其下的 impl/）
 *   {op-impl-mode, file_path} 精度模式配置目录列表（impl_mode，含 preset 与 allow_hf32 ini）
 *   {tiling-device, shared_library} tiling 下沉 so 目录列表（op_tiling_device/lib 等价物）
 * 配置文件不存在时静默回退原逻辑；json 解析失败仅打 warning 日志，不退出。
 */
class OpsDeliverableConfigManager {
 public:
  static OpsDeliverableConfigManager &Instance();

  /**
   * 初始化：stat 检查配置文件存在性，存在则解析 json 并展开占位符。
   * 幂等，重复调用直接返回首次结果。
   * @return true 表示至少有一个条目加载成功；false 表示无配置或解析失败（走原逻辑）。
   */
  bool Initialize();

  /** 算子信息库（ops-info, json）路径列表，未加载返回空 list。 */
  std::vector<std::string> GetOpsInfoPaths() const;

  /** 算子二进制（opkernel, obj_file）路径列表，未加载返回空 list。 */
  std::vector<std::string> GetOpKernelPaths() const;

  /** 算子二进制索引（opkernel-config, json）路径列表，未加载返回空 list。 */
  std::vector<std::string> GetOpKernelConfigPaths() const;

  /**
   * 从 opkernel / opkernel-config 的路径中提取分包前缀。
   * 路径配置到分包级（如 /xxx/kernel/ascend950/ops_nn），末段以 "ops_" 开头即为分包名；
   * 末段不是分包目录时返回空串，表示主包。
   */
  static std::string ExtractOpsPathNamePrefix(const std::string &path);

  /**
   * 路径末段是否为融合算子 kernel 产物目录 "fusion_ops"（忽略路径尾斜杠）。
   * 该目录名不以 "ops_" 开头，ExtractOpsPathNamePrefix 会把它判为空串；
   * 用于识别由 ops_legacy 分包派生的 kernel 根目录、索引 jsonFilePath 中的同名目录段。
   * 配置侧不区分 fusion/legacy：配置里出现 fusion_ops 目录时与其它空 prefix 路径一样被拒绝。
   */
  static bool IsFusionOpsDir(const std::string &path);

  /** 路径是否包含指定的 soc 版本路径段（小写比较，按完整路径段匹配）。soc 为空时恒为 false。 */
  static bool IsPathMatchSoc(const std::string &path, const std::string &soc_version);

  /**
   * 算子交付件根目录（opsrc, src_file）：配置到 tbe 层、不带尾斜杠，结果只有一条。
   * 调用方拼接固定子目录 "impl" 得到 built-in 算子实现目录；未加载/为空返回 false。
   */
  bool GetOpSrcRoot(std::string &root) const;

  /**
   * 精度模式配置（op-impl-mode, file_path）目录列表，未加载返回空 list。
   * 配置目录等价于 built-in/op_impl/ai_core/tbe/impl_mode/，含 preset ini
   * （high_*.ini 等）与 allow_hf32 ini（ops_allow_hf32_*.ini），
   * 支持多条（分包场景）；调用方（OpImplModeConfigParser）按目录遍历查找。
   */
  std::vector<std::string> GetOpImplModePaths() const;

  /** tiling 下沉 so（tiling-device, shared_library）全路径列表（目录 + so 文件名），未加载返回空 list。 */
  std::vector<std::string> GetTilingDevicePaths() const;

  /**
   * 按算子分包分段名从 tiling-device 配置中查找 so 路径。
   * 配置 paths 为 so 全路径（如 /xxx/lib/libtiling_device_transformer.so），
   * 入参 prefix_segment 为已剥掉 ops_ 前缀的分段名（如 ops_transformer → transformer，
   * 由调用方 fe_gentask_utils 的 IsPrefixOpsPath 剥出），按后缀匹配
   * libtiling_device_<segment>.so，不参与路径拼接；命中且文件存在（GetRealPath 校验）返回其 real path。
   * prefix_segment 为空（主包算子，走 libopmaster 链路）或无命中返回 false，调用方回退原逻辑。
   */
  bool SelectTilingDevicePath(const std::string &prefix_segment, std::string &path) const;

  /**
   * 按 socversion 从 opkernel / opkernel-config 配置中筛选出当前芯片可用的路径列表。
   * 配置 paths 是分包级目录，如 /xxx/kernel/ascend950/ops_nn；按完整路径段匹配 soc（小写比较）。
   * @param[in]  soc_version 当前 shortSocVersion（已小写）
   * @param[in]  is_kernel   true 取 opkernel（kernel 目录），false 取 opkernel-config（config 目录）
   * @param[out] paths       命中 soc 的路径列表
   * @return 是否至少命中一条
   */
  bool SelectOpKernelPaths(const std::string &soc_version, bool is_kernel, std::vector<std::string> &paths) const;

 private:
  OpsDeliverableConfigManager() = default;
  ~OpsDeliverableConfigManager() = default;
  OpsDeliverableConfigManager(const OpsDeliverableConfigManager &) = delete;
  OpsDeliverableConfigManager &operator=(const OpsDeliverableConfigManager &) = delete;

  std::string GetConfigPath() const;
  bool LoadConfig();
  bool ResolveDeliverablePaths(const nlohmann::json &deliverables, const std::string &name,
                               const std::string &expect_type, const std::map<std::string, std::string> &variables,
                               std::vector<std::string> &paths) const;
  std::map<std::string, std::string> MakeVariables() const;
  bool ExpandVariables(const std::string &input, const std::map<std::string, std::string> &variables,
                       std::string &output) const;
  std::vector<std::string> GetPathsByKey(const std::string &key) const;

  std::map<std::string, std::vector<std::string>> deliverable_paths_map_;
  bool inited_ = false;
  bool loaded_ = false;
  mutable std::mutex mutex_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_INC_COMMON_OPS_DELIVERABLE_CONFIG_MANAGER_H_
