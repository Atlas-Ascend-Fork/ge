/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DUMP_CONFIG_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DUMP_CONFIG_H_

#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <mutex>
#include <utility>
#include <vector>
#include "common/ge_common/ge_types.h"
#include "nlohmann/json.hpp"

namespace ge {
namespace dump {

const uint32_t ACL_GE_INVALID_DUMP_CONFIG = 100009U;
constexpr int32_t MAX_DUMP_PATH_LENGTH = 512;
constexpr int32_t MAX_IPV4_ADDRESS_LENGTH = 4;
constexpr int32_t MAX_IPV4_ADDRESS_VALUE = 255;
constexpr int32_t DECIMAL = 10;
const uint64_t AIC_ERR_BRIEF_DUMP_BIT = 0x4;
const uint64_t AIC_ERR_NORM_DUMP_BIT = 0x8;

const std::string GE_DUMP_MODE_INPUT = "input";
const std::string GE_DUMP_MODE_OUTPUT = "output";
const std::string GE_DUMP_MODE_ALL = "all";
const std::string GE_DUMP_STATUS_ON = "on";
const std::string GE_DUMP_STATUS_OFF = "off";
const std::string GE_DUMP_LEVEL_OP = "op";
const std::string GE_DUMP_LEVEL_KERNEL = "kernel";
const std::string GE_DUMP_LEVEL_ALL = "all";
const std::string GE_DUMP = "dump";

constexpr uint32_t kAicoreOverflow = 0x1U;
constexpr uint32_t kAtomicOverflow = 0x2U;
constexpr uint32_t kAllOverflow = kAicoreOverflow | kAtomicOverflow;

const std::string GE_DUMP_PATH = "dump_path";
const std::string GE_DUMP_MODE = "dump_mode";
const std::string GE_DUMP_STATUS = "dump_status";
const std::string GE_DUMP_OP_SWITCH = "dump_op_switch";
const std::string GE_DUMP_DEBUG = "dump_debug";
const std::string GE_DUMP_STEP = "dump_step";
const std::string GE_DUMP_DATA = "dump_data";
const std::string GE_DUMP_LEVEL = "dump_level";
const std::string GE_DUMP_STATS = "dump_stats";
const std::string GE_DUMP_LIST = "dump_list";
const std::string GE_DUMP_MODEL_NAME = "model_name";
const std::string GE_DUMP_LAYER = "layer";
const std::string GE_DUMP_WATCHER_NODES = "watcher_nodes";
const std::string GE_DUMP_OPTYPE_BLACKLIST = "optype_blacklist";
const std::string GE_DUMP_OPNAME_BLACKLIST = "opname_blacklist";
const std::string GE_DUMP_BLACKLIST_NAME = "name";
const std::string GE_DUMP_BLACKLIST_POS = "pos";
const std::string GE_DUMP_OPNAME_RANGE = "opname_range";
const std::string GE_DUMP_OPNAME_RANGE_BEGIN = "begin";
const std::string GE_DUMP_OPNAME_RANGE_END = "end";
const std::string GE_DUMP_SCENE = "dump_scene";
const std::string GE_DUMP_LITE_EXCEPTION = "lite_exception";
const std::string GE_DUMP_EXCEPTION_AIC_ERR_BRIEF = "aic_err_brief_dump";
const std::string GE_DUMP_EXCEPTION_AIC_ERR_NORM = "aic_err_norm_dump";
const std::string GE_DUMP_EXCEPTION_AIC_ERR_DETAIL = "aic_err_detail_dump";
const std::string GE_DUMP_SCENE_WATCHER = "watcher";

const std::string GE_DUMP_MODE_DEFAULT = "output";
const std::string GE_DUMP_STATUS_DEFAULT = "on";
const std::string GE_DUMP_OP_SWITCH_DEFAULT = "off";
const std::string GE_DUMP_DEBUG_DEFAULT = "off";
const std::string GE_DUMP_STEP_DEFAULT = "";
const std::string GE_DUMP_DATA_DEFAULT = "tensor";
const std::string GE_DUMP_LEVEL_DEFAULT = "all";
const std::string GE_DUMP_SCENE_DEFAULT = "";

// dump 载荷格式版本环境变量：0=protobuf（默认），1=TLV
const std::string GE_DUMP_DATA_VERSION_ENV = "OM2_DUMP_DATA_VERSION";
constexpr uint32_t GE_DUMP_DATA_VERSION_PROTO = 0U;
constexpr uint32_t GE_DUMP_DATA_VERSION_TLV = 1U;

inline uint32_t GetDumpDataVersionFromEnv() {
  const char *env_value = std::getenv(GE_DUMP_DATA_VERSION_ENV.c_str());
  if ((env_value != nullptr) && (std::string(env_value) == std::to_string(GE_DUMP_DATA_VERSION_TLV))) {
    return GE_DUMP_DATA_VERSION_TLV;
  }
  return GE_DUMP_DATA_VERSION_PROTO;
}

class DumpConfig {
 public:
  static DumpConfig &Instance();

  // 开关
  bool IsDataDumpEnabled() const;
  bool IsExceptionDumpEnabled() const;
  bool IsOverflowDumpEnabled() const;

  void SetDataDumpEnabled(bool enabled);
  void SetExceptionDumpEnabled(bool enabled);
  void SetOverflowDumpEnabled(bool enabled);

  // 配置项
  const std::string &GetDumpPath() const;
  const std::string &GetDumpStep() const;
  const std::string &GetDumpMode() const;
  const std::string &GetDumpData() const;
  const std::unordered_set<std::string> &GetOpList() const;

  void SetDumpPath(const std::string &path);
  void SetDumpStep(const std::string &step);
  void SetDumpMode(const std::string &mode);
  void SetDumpData(const std::string &data);
  void SetOpList(const std::unordered_set<std::string> &op_list);

  // 重置配置
  void Reset();

  // 新增：完整配置解析与校验
  Status ParseAndValidate(const char *dumpData, int32_t size);
  bool IsValidDumpConfig(const nlohmann::json &js) const;

  // 新增：完整 Dump 支持的配置获取接口
  const std::string &GetDumpLevel() const;
  const std::string &GetDumpStatus() const;
  const std::string &GetDumpOpSwitch() const;
  const std::string &GetDumpDebug() const;
  uint32_t GetOpDebugMode() const;
  const std::vector<std::string> &GetDumpStats() const;
  const std::string &GetDumpScene() const;
  const std::vector<ModelDumpConfig> &GetModelDumpConfigList() const;

  // 配置是否需要 Dump
  bool NeedDump() const;

  // 判断该算子是否需要 dump
  bool IsOpNeedDump(const std::string &model_name, const std::string &root_graph_name,
                    const std::string &op_name) const;

  // watcher 场景查询（对齐 v1 DumpProperties 的 DUMP_WATCHER_MODEL / DUMP_LAYER_OP_MODEL 语义）：
  // dump_scene=watcher 走 data dump 链路，不是异常 dump 场景
  bool IsWatcherDumpEnabled() const;
  // 是否为 watcher 节点
  bool IsWatcherNode(const std::string &model_name, const std::string &root_graph_name,
                     const std::string &op_name) const;
  // 是否为 watcher 模式下的 layer 算子（非 watcher 节点，仅记录 task/stream 用于生成 <layerOp>_To_<watcherNode>）
  bool IsLayerOpOnWatcherMode(const std::string &model_name, const std::string &root_graph_name,
                              const std::string &op_name) const;

  // 批量化黑名单查询：一次加锁计算 [0, io_num) 内被 opname/optype 黑名单命中的下标，
  // 供 dump 热路径按 task 批量查询，避免按 IO 逐次加锁。
  void GetInputBlacklistIndices(const std::string &model_name, const std::string &root_graph_name,
                                const std::string &op_name, const std::string &op_type, uint32_t io_num,
                                std::set<uint32_t> &indices) const;
  void GetOutputBlacklistIndices(const std::string &model_name, const std::string &root_graph_name,
                                 const std::string &op_name, const std::string &op_type, uint32_t io_num,
                                 std::set<uint32_t> &indices) const;

  // 获取指定模型配置的 opname_range（对齐 v1 DumpProperties::SetOpDumpRange 的模型匹配规则），
  // 优先精确匹配 model_name，其次匹配 root_graph_name；同名条目对齐 v1 的 clear+assign 语义，后写覆盖先写。
  std::vector<std::pair<std::string, std::string>> GetOpDumpRanges(const std::string &model_name,
                                                                   const std::string &root_graph_name) const;

 private:
  DumpConfig() = default;
  ~DumpConfig() = default;

  // 检查 OM2 不支持的配置项并打印 warning
  static void CheckUnsupportedConfigs(const nlohmann::json &jsDumpConfig);

  // 校验辅助函数
  static bool CheckDumpSceneSwitch(const nlohmann::json &jsDumpConfig, std::string &dumpScene);
  static bool CheckDumpPath(const nlohmann::json &jsDumpConfig);
  static bool CheckDumpStep(const nlohmann::json &jsDumpConfig);
  static bool CheckDumplist(const nlohmann::json &jsDumpConfig, const std::string &dumpLevel);
  static bool CheckDumpOpnameRange(const nlohmann::json &model_json, const std::string &dumpLevel);
  static bool CheckDumpOpnameRangeItems(const nlohmann::json &rangeArray);
  static bool CheckDumpOpSwitch(const std::string &dumpOpSwitch);
  static bool CheckIpAddress(const DumpConfig &config);
  static bool ValidateIpAddress(const std::string &ipAddress);
  static bool IsDumpPathValid(const std::string &dumpPath);
  static bool DumpDebugCheck(const nlohmann::json &jsDumpConfig);
  static bool DumpStatsCheck(const nlohmann::json &jsDumpConfig);
  static bool IsDumpDebugEnabled(const nlohmann::json &jsDumpConfig);
  static bool ValidateNormalDumpConfig(const nlohmann::json &jsDumpConfig);
  static bool ValidateDumpPath(const nlohmann::json &jsDumpConfig);
  static bool ValidateDumpMode(const nlohmann::json &jsDumpConfig);
  static bool ValidateDumpLevel(const nlohmann::json &jsDumpConfig);
  static bool ValidateOtherDumpConfigs(const nlohmann::json &jsDumpConfig);
  static std::string GetDumpLevel(const nlohmann::json &jsDumpConfig);
  static bool CheckDumpModelConfig(const nlohmann::json &modelJson);

  // 解析辅助函数
  Status ParseDumpConfigFromJson(const nlohmann::json &dumpJson);
  void ParseBasicConfigurations(const nlohmann::json &dumpJson);
  void ParseComplexConfigs(const nlohmann::json &dumpJson);
  static bool ParseModelDumpConfig(const nlohmann::json &modelJson, ModelDumpConfig &modelConfig);
  static void ParseBlacklist(const nlohmann::json &blacklistJson, DumpBlacklist &blacklist);
  static void ParseStringArray(const nlohmann::json &jsonArray, std::vector<std::string> &output);
  // 解析 dump_list 条目中可选的字符串数组字段：字段不存在或非数组时保持输出为空
  static void ParseStringArrayField(const nlohmann::json &modelJson, const std::string &key,
                                    std::vector<std::string> &output);
  // 解析 dump_list 条目中可选的黑名单数组字段：字段不存在或非数组时保持输出为空
  static void ParseBlacklistArrayField(const nlohmann::json &modelJson, const std::string &key,
                                       std::vector<DumpBlacklist> &output);
  // 解析 dump_list 条目中的 opname_range 字段，逐项校验 begin/end 均为字符串
  static void ParseOpnameRangeField(const nlohmann::json &modelJson, ModelDumpConfig &modelConfig);
  static bool IsValidOpnameRangeItem(const nlohmann::json &rangeJson);
  static void ParseModelDumpConfigList(const nlohmann::json &jsonArray, std::vector<ModelDumpConfig> &output);
  static std::string GetConfigWithDefault(const nlohmann::json &json, const std::string &key,
                                          const std::string &defaultValue);
  static std::vector<std::string> Split(const std::string &str, const char_t *const delimiter);

  // 黑名单解析与查询辅助（对齐 v1 DumpManager::ExtractBlacklist，pos 格式为 inputN/outputN）
  struct OpIndexBlacklist {
    std::set<uint32_t> input_indices;
    std::set<uint32_t> output_indices;
  };
  // <model_name, <op_name/op_type, 黑名单下标集合>>，model_name 为空表示全局配置
  using ModelIndexBlacklistMap = std::map<std::string, std::map<std::string, OpIndexBlacklist>>;

  // dump_list 条目是否匹配当前模型：model_name 为空表示全局配置，否则匹配 model_name 或 root_graph_name
  static bool IsModelConfigMatched(const ModelDumpConfig &model_config, const std::string &model_name,
                                   const std::string &root_graph_name);

  void ExtractBlacklists();
  static void ExtractBlacklist(const std::vector<DumpBlacklist> &blacklists,
                               std::map<std::string, OpIndexBlacklist> &op_blacklist);
  static bool IsInIndexBlacklist(const ModelIndexBlacklistMap &blacklist_map, const std::string &model_name,
                                 const std::string &root_graph_name, const std::string &op_key, uint32_t index,
                                 bool is_input);

  mutable std::mutex mutex_;

  bool data_dump_enabled_ = false;
  bool exception_dump_enabled_ = false;
  bool overflow_dump_enabled_ = false;

  std::string dump_path_;
  std::string dump_step_;
  std::string dump_mode_;
  std::string dump_data_;
  std::unordered_set<std::string> op_list_;

  // 新增：扩展配置字段
  std::string dump_level_;
  std::string dump_status_;
  std::string dump_op_switch_;
  std::string dump_debug_;
  uint32_t op_debug_mode_ = 0U;
  std::vector<std::string> dump_stats_;
  std::string dump_scene_;  // 对应原始 dump_exception
  std::vector<ModelDumpConfig> model_dump_config_list_;
  // dump_list 解析后预构建的黑名单索引（避免 dump 时反复解析 pos 字符串）
  ModelIndexBlacklistMap optype_blacklist_map_;
  ModelIndexBlacklistMap opname_blacklist_map_;
};

}  // namespace dump
}  // namespace ge

#endif  // GE_COMMON_DUMP_DUMP_CONFIG_H_
