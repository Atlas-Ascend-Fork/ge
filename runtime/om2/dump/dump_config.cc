/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/runtime/dump/dump_config.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "base/err_msg.h"
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <limits>

namespace ge {
namespace dump {

namespace {
// 本地实现时间格式化函数，避免依赖外部 so
// format: 20171122042550
std::string GetCurrentTimeStr() {
  const std::time_t now = std::time(nullptr);
  const std::tm *const ptm = std::localtime(&now);
  if (ptm == nullptr) {
    GELOGE(ge::FAILED, "localtime failed, return empty time string");
    return "";
  }

  constexpr int32_t kTimeBufferLen = 32;
  char buffer[kTimeBufferLen + 1] = {};
  (void)std::strftime(buffer, static_cast<size_t>(kTimeBufferLen), "%Y%m%d%H%M%S", ptm);
  return std::string(buffer);
}

// 解析黑名单 pos 字段（格式 inputN/outputN），对齐 v1 DumpManager::IsValidFormat/ExtractNumber 的语义
bool ParseBlacklistPos(const std::string &pos, bool &is_input, uint32_t &index) {
  const std::string kInputPrefix = "input";
  const std::string kOutputPrefix = "output";
  size_t num_begin = 0U;
  if (pos.rfind(kInputPrefix, 0U) == 0U) {
    is_input = true;
    num_begin = kInputPrefix.size();
  } else if (pos.rfind(kOutputPrefix, 0U) == 0U) {
    is_input = false;
    num_begin = kOutputPrefix.size();
  } else {
    return false;
  }

  if (num_begin >= pos.size()) {
    return false;
  }
  for (size_t i = num_begin; i < pos.size(); ++i) {
    if (std::isdigit(static_cast<unsigned char>(pos[i])) == 0) {
      return false;
    }
  }

  const unsigned long value = std::strtoul(pos.c_str() + num_begin, nullptr, DECIMAL);
  if (value > static_cast<unsigned long>(std::numeric_limits<uint32_t>::max())) {
    return false;
  }
  index = static_cast<uint32_t>(value);
  return true;
}
}  // namespace

DumpConfig &DumpConfig::Instance() {
  static DumpConfig instance;
  return instance;
}

bool DumpConfig::IsDataDumpEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return data_dump_enabled_;
}

bool DumpConfig::IsExceptionDumpEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return exception_dump_enabled_;
}

bool DumpConfig::IsOverflowDumpEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return overflow_dump_enabled_;
}

void DumpConfig::SetDataDumpEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_dump_enabled_ = enabled;
}

void DumpConfig::SetExceptionDumpEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  exception_dump_enabled_ = enabled;
}

void DumpConfig::SetOverflowDumpEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  overflow_dump_enabled_ = enabled;
}

const std::string &DumpConfig::GetDumpPath() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_path_;
}

const std::string &DumpConfig::GetDumpStep() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_step_;
}

const std::string &DumpConfig::GetDumpMode() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_mode_;
}

const std::string &DumpConfig::GetDumpData() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_data_;
}

const std::unordered_set<std::string> &DumpConfig::GetOpList() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return op_list_;
}

void DumpConfig::SetDumpPath(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  dump_path_ = path;
}

void DumpConfig::SetDumpStep(const std::string &step) {
  std::lock_guard<std::mutex> lock(mutex_);
  dump_step_ = step;
}

void DumpConfig::SetDumpMode(const std::string &mode) {
  std::lock_guard<std::mutex> lock(mutex_);
  dump_mode_ = mode;
}

void DumpConfig::SetDumpData(const std::string &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  dump_data_ = data;
}

void DumpConfig::SetOpList(const std::unordered_set<std::string> &op_list) {
  std::lock_guard<std::mutex> lock(mutex_);
  op_list_ = op_list;
}

void DumpConfig::Reset() {
  std::lock_guard<std::mutex> lock(mutex_);
  data_dump_enabled_ = false;
  exception_dump_enabled_ = false;
  overflow_dump_enabled_ = false;
  dump_path_.clear();
  dump_step_.clear();
  dump_mode_.clear();
  dump_data_.clear();
  op_list_.clear();
  dump_level_.clear();
  dump_status_.clear();
  dump_op_switch_.clear();
  dump_debug_.clear();
  dump_stats_.clear();
  dump_scene_.clear();
  model_dump_config_list_.clear();
  optype_blacklist_map_.clear();
  opname_blacklist_map_.clear();
  op_debug_mode_ = 0U;
}

const std::string &DumpConfig::GetDumpLevel() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_level_;
}

const std::string &DumpConfig::GetDumpStatus() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_status_;
}

const std::string &DumpConfig::GetDumpOpSwitch() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_op_switch_;
}

const std::string &DumpConfig::GetDumpDebug() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_debug_;
}

uint32_t DumpConfig::GetOpDebugMode() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return op_debug_mode_;
}

const std::vector<std::string> &DumpConfig::GetDumpStats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_stats_;
}

const std::string &DumpConfig::GetDumpScene() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_scene_;
}

const std::vector<ModelDumpConfig> &DumpConfig::GetModelDumpConfigList() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return model_dump_config_list_;
}

bool DumpConfig::NeedDump() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if ((!model_dump_config_list_.empty()) || (dump_op_switch_ == GE_DUMP_STATUS_ON) ||
      (dump_debug_ == GE_DUMP_STATUS_ON) || (!dump_scene_.empty())) {
    return true;
  }
  return false;
}

// 规则1：没有配置 dump_list 时，按全局 data dump 开关生效，默认该 op 需要 dump
// 规则2：model_name 为空时，对齐 OM1 的 DUMP_LAYER_OP_MODEL，表示全局 layer 配置，不绑定具体模型
// 规则3：非全局 layer 配置时，只有 model_name 或 root_graph_name 命中当前模型，layer 才生效
// 规则4：模型不匹配时，即使 layer 命中当前 op，也不允许 dump
// 规则5：模型匹配且 layer/watcher_nodes/opname_range 为空时，表示 dump 该模型所有 op
//        （对齐 v1 DumpProperties::IsNeedDump：配置了 opname_range 时不再按空 layer 全量 dump）
// 规则6：模型匹配且 layer 非空时，只 dump layer 中显式配置的 op
// 规则7：watcher 模式（配置了 watcher_nodes）下，只 dump watcher 节点，其余算子仅作为 layer 算子被记录
//        （对齐 v1 DavinciModel::OpNeedDump：watcher 使能时返回 false，由 OpNeedDumpOnWatcherModel 决定 dump）
// 规则8：所有 dump_list 项均未匹配当前模型/op 时，不 dump
// 注意：opname_range 的命中判断由 ModelDumpManager 的状态机完成，不在本函数内处理
bool DumpConfig::IsOpNeedDump(const std::string &model_name, const std::string &root_graph_name,
                              const std::string &op_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (model_dump_config_list_.empty()) {
    return true;
  }

  for (const auto &model_config : model_dump_config_list_) {
    if (!IsModelConfigMatched(model_config, model_name, root_graph_name)) {
      continue;
    }

    if (!model_config.watcher_nodes.empty()) {
      if (std::find(model_config.watcher_nodes.begin(), model_config.watcher_nodes.end(), op_name) !=
          model_config.watcher_nodes.end()) {
        return true;
      }
      continue;
    }

    if (model_config.layers.empty() && model_config.dump_op_ranges.empty()) {
      return true;
    }

    if (std::find(model_config.layers.begin(), model_config.layers.end(), op_name) != model_config.layers.end()) {
      return true;
    }
  }

  return false;
}

bool DumpConfig::IsModelConfigMatched(const ModelDumpConfig &model_config, const std::string &model_name,
                                      const std::string &root_graph_name) {
  return model_config.model_name.empty() || (model_config.model_name == model_name) ||
         (model_config.model_name == root_graph_name);
}

bool DumpConfig::IsWatcherDumpEnabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dump_scene_ == GE_DUMP_SCENE_WATCHER;
}

// 对齐 v1 DumpProperties::IsWatcherNode：watcher_nodes 命中即为 watcher 节点
bool DumpConfig::IsWatcherNode(const std::string &model_name, const std::string &root_graph_name,
                               const std::string &op_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto &model_config : model_dump_config_list_) {
    if (!IsModelConfigMatched(model_config, model_name, root_graph_name)) {
      continue;
    }
    if (std::find(model_config.watcher_nodes.begin(), model_config.watcher_nodes.end(), op_name) !=
        model_config.watcher_nodes.end()) {
      return true;
    }
  }
  return false;
}

// 对齐 v1 DumpProperties::IsLayerOpOnWatcherMode 方法
// 1. 必须配置了 watcher_nodes（DUMP_WATCHER_MODEL）
// 2. 必须存在全局 layer 配置（model_name 为空，对应 DUMP_LAYER_OP_MODEL），layer 为空表示所有算子
bool DumpConfig::IsLayerOpOnWatcherMode(const std::string &model_name, const std::string &root_graph_name,
                                        const std::string &op_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  bool watcher_enabled = false;
  for (const auto &model_config : model_dump_config_list_) {
    if (!model_config.watcher_nodes.empty() && IsModelConfigMatched(model_config, model_name, root_graph_name)) {
      watcher_enabled = true;
      break;
    }
  }
  if (!watcher_enabled) {
    return false;
  }

  for (const auto &model_config : model_dump_config_list_) {
    if (!model_config.model_name.empty()) {
      continue;
    }
    if (model_config.layers.empty() ||
        (std::find(model_config.layers.begin(), model_config.layers.end(), op_name) != model_config.layers.end())) {
      return true;
    }
  }
  return false;
}

Status DumpConfig::ParseAndValidate(const char *dumpData, int32_t size) {
  GELOGI("Start to parse and validate dump config, size: %d", size);
  if (dumpData == nullptr || size <= 0) {
    GELOGI("Dump data is null or empty, no dump config to parse");
    return SUCCESS;
  }

  std::string jsonStr(dumpData, size);
  nlohmann::json js;
  try {
    js = nlohmann::json::parse(jsonStr);
  } catch (...) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Parse][DumpConfig]Failed to parse dump json string");
    REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                              std::vector<const char *>({"json", "dump_config", "Failed to parse dump json string."}));
    return FAILED;
  }

  if (!IsValidDumpConfig(js)) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpConfig]Dump config validation failed");
    return FAILED;
  }

  Status ret = ParseDumpConfigFromJson(js);
  if (ret != SUCCESS) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Parse][DumpConfig]Failed to parse dump config from json");
    return ret;
  }

  // 检查 OM2 暂不支持的配置项并打印 warning
  const nlohmann::json &jsDumpConfig = js.contains(GE_DUMP) ? js[GE_DUMP] : js;
  CheckUnsupportedConfigs(jsDumpConfig);

  GELOGI("Parse and validate dump config successfully");
  return SUCCESS;
}

bool DumpConfig::IsValidDumpConfig(const nlohmann::json &js) const {
  GELOGI("Start to execute IsValidDumpConfig.");
  if (!js.contains(GE_DUMP)) {
    GELOGI("no dump item, no need to do dump!");
    return true;
  }

  const nlohmann::json &jsDumpConfig = js[GE_DUMP];

  if (jsDumpConfig.contains(GE_DUMP_SCENE)) {
    std::string dumpScene;
    return CheckDumpSceneSwitch(jsDumpConfig, dumpScene);
  }

  if (IsDumpDebugEnabled(jsDumpConfig)) {
    return DumpDebugCheck(jsDumpConfig);
  }

  return ValidateNormalDumpConfig(jsDumpConfig);
}

bool DumpConfig::IsDumpDebugEnabled(const nlohmann::json &jsDumpConfig) {
  std::string dumpDebug = GE_DUMP_DEBUG_DEFAULT;
  if (jsDumpConfig.contains(GE_DUMP_DEBUG)) {
    dumpDebug = jsDumpConfig[GE_DUMP_DEBUG].get<std::string>();
  }
  return (dumpDebug == GE_DUMP_STATUS_ON);
}

bool DumpConfig::ValidateNormalDumpConfig(const nlohmann::json &jsDumpConfig) {
  if (!ValidateDumpPath(jsDumpConfig)) {
    return false;
  }

  if (!ValidateDumpMode(jsDumpConfig)) {
    return false;
  }

  if (!ValidateDumpLevel(jsDumpConfig)) {
    return false;
  }

  return ValidateOtherDumpConfigs(jsDumpConfig);
}

bool DumpConfig::ValidateDumpPath(const nlohmann::json &jsDumpConfig) {
  if (!jsDumpConfig.contains(GE_DUMP_PATH)) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpConfig]dump_path field in dump config does not exist");
    REPORT_PREDEFINED_ERR_MSG(
        "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
        std::vector<const char *>({"NULL", "dump_path", "dump_path field in dump config does not exist."}));
    return false;
  }

  const std::string dumpPath = jsDumpConfig[GE_DUMP_PATH].get<std::string>();
  if (dumpPath.empty()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpConfig]dump_path field is null in config");
    REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                              std::vector<const char *>({"NULL", "dump_path", "dump_path field is null in config."}));
    return false;
  }

  return CheckDumpPath(jsDumpConfig);
}

bool DumpConfig::ValidateDumpMode(const nlohmann::json &jsDumpConfig) {
  std::string dumpMode = GE_DUMP_MODE_DEFAULT;
  if (jsDumpConfig.contains(GE_DUMP_MODE)) {
    dumpMode = jsDumpConfig[GE_DUMP_MODE].get<std::string>();
  }

  const std::vector<std::string> validModes = {GE_DUMP_MODE_INPUT, GE_DUMP_MODE_OUTPUT, GE_DUMP_MODE_ALL};

  if (std::find(validModes.begin(), validModes.end(), dumpMode) == validModes.end()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG,
           "[Check][DumpConfig]dump_mode value[%s] error in config, only supports input/output/all", dumpMode.c_str());
    REPORT_PREDEFINED_ERR_MSG(
        "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
        std::vector<const char *>({dumpMode.c_str(), "dump_mode", "only supports input/output/all."}));
    return false;
  }
  GELOGD("dump_mode value[%s] is valid", dumpMode.c_str());
  return true;
}

bool DumpConfig::ValidateDumpLevel(const nlohmann::json &jsDumpConfig) {
  std::string dumpLevel = GE_DUMP_LEVEL_DEFAULT;
  if (jsDumpConfig.contains(GE_DUMP_LEVEL)) {
    dumpLevel = jsDumpConfig[GE_DUMP_LEVEL].get<std::string>();
  }

  const std::vector<std::string> validLevels = {GE_DUMP_LEVEL_OP, GE_DUMP_LEVEL_KERNEL, GE_DUMP_LEVEL_ALL};

  if (std::find(validLevels.begin(), validLevels.end(), dumpLevel) == validLevels.end()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG,
           "[Check][DumpConfig]dump_level value[%s] error in config, only supports op/kernel/all", dumpLevel.c_str());
    REPORT_PREDEFINED_ERR_MSG(
        "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
        std::vector<const char *>({dumpLevel.c_str(), "dump_level", "only supports op/kernel/all."}));
    return false;
  }
  GELOGD("dump_level value[%s] is valid", dumpLevel.c_str());
  return true;
}

bool DumpConfig::ValidateOtherDumpConfigs(const nlohmann::json &jsDumpConfig) {
  return CheckDumpStep(jsDumpConfig) && CheckDumplist(jsDumpConfig, GetDumpLevel(jsDumpConfig)) &&
         DumpStatsCheck(jsDumpConfig);
}

std::string DumpConfig::GetDumpLevel(const nlohmann::json &jsDumpConfig) {
  if (jsDumpConfig.contains(GE_DUMP_LEVEL)) {
    return jsDumpConfig[GE_DUMP_LEVEL].get<std::string>();
  }
  return GE_DUMP_LEVEL_DEFAULT;
}

void DumpConfig::CheckUnsupportedConfigs(const nlohmann::json &jsDumpConfig) {
  if (jsDumpConfig.contains(GE_DUMP_OP_SWITCH)) {
    GELOGW("[OM2 Dump] dump_op_switch is not supported in OM2 dump mode, configuration will be ignored");
  }
}

bool DumpConfig::CheckDumpSceneSwitch(const nlohmann::json &jsDumpConfig, std::string &dumpScene) {
  if (!jsDumpConfig.contains(GE_DUMP_SCENE)) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpConfig]dump_scene field does not exist");
    return false;
  }

  dumpScene = jsDumpConfig[GE_DUMP_SCENE].get<std::string>();
  const std::vector<std::string> validScenes = {GE_DUMP_LITE_EXCEPTION, GE_DUMP_EXCEPTION_AIC_ERR_BRIEF,
                                                GE_DUMP_EXCEPTION_AIC_ERR_NORM, GE_DUMP_EXCEPTION_AIC_ERR_DETAIL,
                                                GE_DUMP_SCENE_WATCHER};

  if (std::find(validScenes.begin(), validScenes.end(), dumpScene) == validScenes.end()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpConfig]dump_scene value[%s] is invalid", dumpScene.c_str());
    return false;
  }

  // 对齐 v1 DumpConfigValidator::CheckDumpSceneSwitch：watcher 是正常数据 dump 场景，
  // 与 dump_debug / dump_op_switch 互斥，且 dump_mode 只支持 output
  if (dumpScene == GE_DUMP_SCENE_WATCHER) {
    if (GetConfigWithDefault(jsDumpConfig, GE_DUMP_DEBUG, GE_DUMP_DEBUG_DEFAULT) == GE_DUMP_STATUS_ON) {
      GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpScene]dump_scene is %s when dump_debug is on is unsupported.",
             dumpScene.c_str());
      REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                                std::vector<const char *>({dumpScene.c_str(), "dump_scene",
                                                           "dump_scene watcher conflicts with dump_debug."}));
      return false;
    }
    if (GetConfigWithDefault(jsDumpConfig, GE_DUMP_OP_SWITCH, GE_DUMP_OP_SWITCH_DEFAULT) == GE_DUMP_STATUS_ON) {
      GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpScene]dump_scene is %s when dump_op_switch is on is unsupported.",
             dumpScene.c_str());
      REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                                std::vector<const char *>({dumpScene.c_str(), "dump_scene",
                                                           "dump_scene watcher conflicts with dump_op_switch."}));
      return false;
    }
    if (GetConfigWithDefault(jsDumpConfig, GE_DUMP_MODE, GE_DUMP_MODE_DEFAULT) != GE_DUMP_MODE_OUTPUT) {
      GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpScene]dump_mode only supports output when dump_scene is %s.",
             dumpScene.c_str());
      REPORT_PREDEFINED_ERR_MSG(
          "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
          std::vector<const char *>({dumpScene.c_str(), "dump_mode", "dump_mode only supports output for watcher."}));
      return false;
    }
    // watcher 依赖 dump_path 落盘目录，缺失时无法产出 dump 文件（对齐 v1 对 dump_path 的必填要求）
    if (GetConfigWithDefault(jsDumpConfig, GE_DUMP_PATH, "").empty()) {
      GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpScene]dump_path is required when dump_scene is %s.",
             dumpScene.c_str());
      return false;
    }
    if (!jsDumpConfig.contains(GE_DUMP_LIST)) {
      GELOGW("[OM2 Dump] dump_scene is %s without dump_list.watcher_nodes, no watcher node to dump", dumpScene.c_str());
    }
  }
  GELOGD("dump_scene value[%s] is valid", dumpScene.c_str());
  return true;
}

bool DumpConfig::CheckDumpPath(const nlohmann::json &jsDumpConfig) {
  (void)jsDumpConfig;
  return true;
}

bool DumpConfig::CheckDumpStep(const nlohmann::json &jsDumpConfig) {
  (void)jsDumpConfig;
  return true;
}

// 对齐 v1 DumpConfigValidator::CheckDumpOpNameRange 方法
// 配置了 opname_range 时，dump_level 只支持 op，model_name 不能为空，range 的 begin/end 不能为空
bool DumpConfig::CheckDumplist(const nlohmann::json &jsDumpConfig, const std::string &dumpLevel) {
  if (!jsDumpConfig.contains(GE_DUMP_LIST) || !jsDumpConfig[GE_DUMP_LIST].is_array()) {
    return true;
  }

  for (const auto &model_json : jsDumpConfig[GE_DUMP_LIST]) {
    if (!model_json.contains(GE_DUMP_OPNAME_RANGE) || !model_json[GE_DUMP_OPNAME_RANGE].is_array() ||
        model_json[GE_DUMP_OPNAME_RANGE].empty()) {
      continue;
    }
    if (!CheckDumpOpnameRange(model_json, dumpLevel)) {
      return false;
    }
  }

  GELOGI("successfully execute CheckDumplist.");
  return true;
}

bool DumpConfig::CheckDumpOpnameRange(const nlohmann::json &model_json, const std::string &dumpLevel) {
  if (dumpLevel != GE_DUMP_LEVEL_OP) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG,
           "[Check][DumpOpNameRange]dump level only support op when op name range is enable.");
    REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                              std::vector<const char *>({dumpLevel.c_str(), "dump_level",
                                                         "dump level only support op when op name range is enable."}));
    return false;
  }

  std::string model_name;
  if (model_json.contains(GE_DUMP_MODEL_NAME) && model_json[GE_DUMP_MODEL_NAME].is_string()) {
    model_name = model_json[GE_DUMP_MODEL_NAME].get<std::string>();
  }
  if (model_name.empty()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG,
           "[Check][DumpOpNameRange]model name cannot be empty when op name range is "
           "enable.");
    REPORT_PREDEFINED_ERR_MSG(
        "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
        std::vector<const char *>({"NULL", "model_name", "model name cannot be empty when op name range is enable."}));
    return false;
  }

  return CheckDumpOpnameRangeItems(model_json[GE_DUMP_OPNAME_RANGE]);
}

bool DumpConfig::CheckDumpOpnameRangeItems(const nlohmann::json &rangeArray) {
  for (const auto &range_json : rangeArray) {
    std::string begin;
    std::string end;
    if (range_json.contains(GE_DUMP_OPNAME_RANGE_BEGIN) && range_json[GE_DUMP_OPNAME_RANGE_BEGIN].is_string()) {
      begin = range_json[GE_DUMP_OPNAME_RANGE_BEGIN].get<std::string>();
    }
    if (range_json.contains(GE_DUMP_OPNAME_RANGE_END) && range_json[GE_DUMP_OPNAME_RANGE_END].is_string()) {
      end = range_json[GE_DUMP_OPNAME_RANGE_END].get<std::string>();
    }
    if (begin.empty() || end.empty()) {
      GELOGE(ACL_GE_INVALID_DUMP_CONFIG,
             "[Check][DumpOpNameRange]op name range is incomplete, "
             "op name range begin[%s] end[%s].",
             begin.c_str(), end.c_str());
      REPORT_PREDEFINED_ERR_MSG(
          "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
          std::vector<const char *>({begin.c_str(), "opname_range", "op name range is incomplete."}));
      return false;
    }
  }
  return true;
}

bool DumpConfig::CheckDumpOpSwitch(const std::string &dumpOpSwitch) {
  const std::vector<std::string> validSwitches = {GE_DUMP_STATUS_ON, GE_DUMP_STATUS_OFF};
  return std::find(validSwitches.begin(), validSwitches.end(), dumpOpSwitch) != validSwitches.end();
}

bool DumpConfig::CheckIpAddress(const DumpConfig &config) {
  (void)config;
  return true;
}

bool DumpConfig::ValidateIpAddress(const std::string &ipAddress) {
  (void)ipAddress;
  return true;
}

bool DumpConfig::IsDumpPathValid(const std::string &dumpPath) {
  return dumpPath.size() <= MAX_DUMP_PATH_LENGTH;
}

bool DumpConfig::DumpDebugCheck(const nlohmann::json &jsDumpConfig) {
  (void)jsDumpConfig;
  return true;
}

bool DumpConfig::DumpStatsCheck(const nlohmann::json &jsDumpConfig) {
  // 对齐 v1 DumpConfigValidator::DumpStatsCheck
  if (!jsDumpConfig.contains(GE_DUMP_STATS)) {
    return true;
  }

  std::vector<std::string> dump_stats;
  if (jsDumpConfig[GE_DUMP_STATS].is_array()) {
    ParseStringArray(jsDumpConfig[GE_DUMP_STATS], dump_stats);
  }
  if (dump_stats.empty()) {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpStats]dump_stats exists but is empty.");
    REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                              std::vector<const char *>({"NULL", "dump_stats", "dump_stats exists but is empty."}));
    return false;
  }

  // OM2 不使用 dump_stats 列表内容，仅对齐 v1 的配置约束：stats 需与 dump_data=stats 配合使用
  if (GetConfigWithDefault(jsDumpConfig, GE_DUMP_DATA, GE_DUMP_DATA_DEFAULT) != "stats") {
    GELOGE(ACL_GE_INVALID_DUMP_CONFIG, "[Check][DumpStats]nonsupport to enable dump_stats when dump_data is tensor.");
    REPORT_PREDEFINED_ERR_MSG(
        "E10001", std::vector<const char *>({"value", "parameter", "reason"}),
        std::vector<const char *>({GetConfigWithDefault(jsDumpConfig, GE_DUMP_DATA, GE_DUMP_DATA_DEFAULT).c_str(),
                                   "dump_data", "nonsupport to enable dump_stats when dump_data is tensor."}));
    return false;
  }
  return true;
}

Status DumpConfig::ParseDumpConfigFromJson(const nlohmann::json &dumpJson) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!dumpJson.contains(GE_DUMP)) {
    GELOGI("no dump item, no need to parse dump config!");
    return SUCCESS;
  }

  const nlohmann::json &jsDumpConfig = dumpJson[GE_DUMP];
  ParseBasicConfigurations(jsDumpConfig);
  ParseComplexConfigs(jsDumpConfig);

  // 对齐 V1：dump_path 末尾加上时间戳
  if (!dump_path_.empty()) {
    // 确保路径以 / 结尾
    if (dump_path_[dump_path_.size() - 1U] != '/') {
      dump_path_ += "/";
    }
    dump_path_ += GetCurrentTimeStr() + "/";
  }

  // 根据配置设置开关（顺序对齐 v1：先解析场景，再按 dump_debug 覆盖 dump_status，最后推导 data dump）：
  // 1. 异常场景只开 exception dump；watcher 属正常数据 dump 场景
  // 2. dump_debug: on 只开 overflow dump，并强制关闭 data dump（对齐 v1 HandleDumpDebugConfig）
  // 3. data dump 只由 dump_status 决定（dump_status 已由 dump_level 推导，见 ParseBasicConfigurations），
  //    对齐 v1 DumpManager::IsDataDumpOpen，不再以 dump_list 非空兜底
  if (!dump_scene_.empty()) {
    // dump_scene=watcher 对齐 v1：watcher 属于正常数据 dump 场景，走 data dump 链路，
    // 只有异常场景（lite_exception/aic_err_*）才开启 exception dump
    if (dump_scene_ == GE_DUMP_SCENE_WATCHER) {
      GELOGI("Watcher dump configured, dump_path: %s", dump_path_.c_str());
    } else {
      exception_dump_enabled_ = true;
      GELOGI("Exception dump enabled, dump_scene: %s", dump_scene_.c_str());
    }
  }
  if (dump_debug_ == GE_DUMP_STATUS_ON) {
    dump_status_ = GE_DUMP_STATUS_OFF;
    overflow_dump_enabled_ = true;
    GELOGI("Overflow dump enabled, dump_path: %s", dump_path_.c_str());
  }
  if ((dump_status_ == GE_DUMP_STATUS_ON) && !exception_dump_enabled_) {
    data_dump_enabled_ = true;
    GELOGI("Data dump enabled, dump_status: %s, dump_level: %s, model config count: %zu", dump_status_.c_str(),
           dump_level_.c_str(), model_dump_config_list_.size());
  } else {
    GELOGI("Data dump not enabled, dump_status: %s, dump_level: %s, dump_scene: %s, dump_debug: %s",
           dump_status_.c_str(), dump_level_.c_str(), dump_scene_.c_str(), dump_debug_.c_str());
  }

  GELOGI("Parse dump config from json successfully, dump_mode: %s, dump_level: %s, dump_step: %s", dump_mode_.c_str(),
         dump_level_.c_str(), dump_step_.c_str());
  return SUCCESS;
}

void DumpConfig::ParseBasicConfigurations(const nlohmann::json &dumpJson) {
  dump_path_ = GetConfigWithDefault(dumpJson, GE_DUMP_PATH, "");
  dump_mode_ = GetConfigWithDefault(dumpJson, GE_DUMP_MODE, GE_DUMP_MODE_DEFAULT);
  dump_status_ = GetConfigWithDefault(dumpJson, GE_DUMP_STATUS, GE_DUMP_STATUS_DEFAULT);
  dump_op_switch_ = GetConfigWithDefault(dumpJson, GE_DUMP_OP_SWITCH, GE_DUMP_OP_SWITCH_DEFAULT);
  dump_debug_ = GetConfigWithDefault(dumpJson, GE_DUMP_DEBUG, GE_DUMP_DEBUG_DEFAULT);
  dump_step_ = GetConfigWithDefault(dumpJson, GE_DUMP_STEP, GE_DUMP_STEP_DEFAULT);
  dump_data_ = GetConfigWithDefault(dumpJson, GE_DUMP_DATA, GE_DUMP_DATA_DEFAULT);
  dump_level_ = GetConfigWithDefault(dumpJson, GE_DUMP_LEVEL, GE_DUMP_LEVEL_DEFAULT);
  dump_scene_ = GetConfigWithDefault(dumpJson, GE_DUMP_SCENE, GE_DUMP_SCENE_DEFAULT);

  // 对齐 v1 DumpConfigValidator::ParseComplexConfigs：acl 配置里 dump_status 由 dump_level 推导
  // （op/all -> on，kernel -> off），显式配置的 dump_status 不生效
  dump_status_ = ((dump_level_ == GE_DUMP_LEVEL_OP) || (dump_level_ == GE_DUMP_LEVEL_ALL)) ? GE_DUMP_STATUS_ON
                                                                                           : GE_DUMP_STATUS_OFF;

  // 对齐 v1 DumpManager::SetDumpDebugConf：dump_debug=on 时启用全部溢出检测；
  // dump_status 的强制关闭统一放在开关推导处（ParseDumpConfigFromJson），避免两处重复
  if (dump_debug_ == GE_DUMP_STATUS_ON) {
    op_debug_mode_ = kAllOverflow;
  } else {
    op_debug_mode_ = 0U;
  }
}

void DumpConfig::ParseComplexConfigs(const nlohmann::json &dumpJson) {
  if (dumpJson.contains(GE_DUMP_STATS) && dumpJson[GE_DUMP_STATS].is_array()) {
    ParseStringArray(dumpJson[GE_DUMP_STATS], dump_stats_);
  }

  if (dumpJson.contains(GE_DUMP_LIST) && dumpJson[GE_DUMP_LIST].is_array()) {
    ParseModelDumpConfigList(dumpJson[GE_DUMP_LIST], model_dump_config_list_);
    // 预构建黑名单索引，供 dump 时按下标快速过滤（调用方已持有 mutex_）
    ExtractBlacklists();
  }
}

std::string DumpConfig::GetConfigWithDefault(const nlohmann::json &json, const std::string &key,
                                             const std::string &defaultValue) {
  if (json.contains(key)) {
    return json[key].get<std::string>();
  }
  return defaultValue;
}

void DumpConfig::ParseStringArray(const nlohmann::json &jsonArray, std::vector<std::string> &output) {
  output.clear();
  for (const auto &item : jsonArray) {
    output.push_back(item.get<std::string>());
  }
}

void DumpConfig::ParseModelDumpConfigList(const nlohmann::json &jsonArray, std::vector<ModelDumpConfig> &output) {
  output.clear();
  for (const auto &modelJson : jsonArray) {
    ModelDumpConfig modelConfig;
    if (ParseModelDumpConfig(modelJson, modelConfig)) {
      output.push_back(modelConfig);
    }
  }
}

bool DumpConfig::ParseModelDumpConfig(const nlohmann::json &modelJson, ModelDumpConfig &modelConfig) {
  if (!CheckDumpModelConfig(modelJson)) {
    return false;
  }

  if (modelJson.contains(GE_DUMP_MODEL_NAME)) {
    modelConfig.model_name = modelJson[GE_DUMP_MODEL_NAME].get<std::string>();
  }

  ParseStringArrayField(modelJson, GE_DUMP_LAYER, modelConfig.layers);
  ParseStringArrayField(modelJson, GE_DUMP_WATCHER_NODES, modelConfig.watcher_nodes);
  ParseBlacklistArrayField(modelJson, GE_DUMP_OPTYPE_BLACKLIST, modelConfig.optype_blacklist);
  ParseBlacklistArrayField(modelJson, GE_DUMP_OPNAME_BLACKLIST, modelConfig.opname_blacklist);
  ParseOpnameRangeField(modelJson, modelConfig);

  return true;
}

void DumpConfig::ParseStringArrayField(const nlohmann::json &modelJson, const std::string &key,
                                       std::vector<std::string> &output) {
  if (modelJson.contains(key) && modelJson[key].is_array()) {
    ParseStringArray(modelJson[key], output);
  }
}

void DumpConfig::ParseBlacklistArrayField(const nlohmann::json &modelJson, const std::string &key,
                                          std::vector<DumpBlacklist> &output) {
  if (!modelJson.contains(key) || !modelJson[key].is_array()) {
    return;
  }
  output.clear();
  for (const auto &blacklistJson : modelJson[key]) {
    DumpBlacklist blacklist;
    ParseBlacklist(blacklistJson, blacklist);
    output.push_back(blacklist);
  }
}

bool DumpConfig::IsValidOpnameRangeItem(const nlohmann::json &rangeJson) {
  return rangeJson.contains(GE_DUMP_OPNAME_RANGE_BEGIN) && rangeJson[GE_DUMP_OPNAME_RANGE_BEGIN].is_string() &&
         rangeJson.contains(GE_DUMP_OPNAME_RANGE_END) && rangeJson[GE_DUMP_OPNAME_RANGE_END].is_string();
}

void DumpConfig::ParseOpnameRangeField(const nlohmann::json &modelJson, ModelDumpConfig &modelConfig) {
  if (!modelJson.contains(GE_DUMP_OPNAME_RANGE) || !modelJson[GE_DUMP_OPNAME_RANGE].is_array()) {
    return;
  }
  modelConfig.dump_op_ranges.clear();
  for (const auto &rangeJson : modelJson[GE_DUMP_OPNAME_RANGE]) {
    // 显式校验 begin/end 均为字符串，避免非字符串字段在 get<std::string>() 时抛未捕获异常
    // （dump_scene 等场景下 IsValidDumpConfig 不校验 dump_list，此处在解析期兜底）
    if (!IsValidOpnameRangeItem(rangeJson)) {
      GELOGW("[OM2 Dump] opname_range begin/end is missing or not string, ignore this range item.");
      continue;
    }
    modelConfig.dump_op_ranges.emplace_back(rangeJson[GE_DUMP_OPNAME_RANGE_BEGIN].get<std::string>(),
                                            rangeJson[GE_DUMP_OPNAME_RANGE_END].get<std::string>());
  }
}

bool DumpConfig::CheckDumpModelConfig(const nlohmann::json &modelJson) {
  (void)modelJson;
  return true;
}

void DumpConfig::ParseBlacklist(const nlohmann::json &blacklistJson, DumpBlacklist &blacklist) {
  if (blacklistJson.contains(GE_DUMP_BLACKLIST_NAME)) {
    blacklist.name = blacklistJson[GE_DUMP_BLACKLIST_NAME].get<std::string>();
  }

  if (blacklistJson.contains(GE_DUMP_BLACKLIST_POS) && blacklistJson[GE_DUMP_BLACKLIST_POS].is_array()) {
    blacklist.pos.clear();
    for (const auto &pos : blacklistJson[GE_DUMP_BLACKLIST_POS]) {
      blacklist.pos.push_back(pos.get<std::string>());
    }
  }
}

// 对齐 v1 DumpManager::ExtractBlacklist：把 pos 字符串解析为 input/output 下标集合
void DumpConfig::ExtractBlacklist(const std::vector<DumpBlacklist> &blacklists,
                                  std::map<std::string, OpIndexBlacklist> &op_blacklist) {
  for (const auto &blacklist : blacklists) {
    // 同名黑名单的多个条目合并到同一集合：直接取 map 槽位引用就地累加，省去临时集合与二次 range-insert 合并
    OpIndexBlacklist &indices = op_blacklist[blacklist.name];
    for (const auto &pos : blacklist.pos) {
      bool is_input = false;
      uint32_t index = 0U;
      if (!ParseBlacklistPos(pos, is_input, index)) {
        GELOGW("[OM2 Dump] blacklist pos %s is invalid, ignore.", pos.c_str());
        continue;
      }
      if (is_input) {
        (void)indices.input_indices.insert(index);
      } else {
        (void)indices.output_indices.insert(index);
      }
    }
  }
}

void DumpConfig::ExtractBlacklists() {
  optype_blacklist_map_.clear();
  opname_blacklist_map_.clear();
  for (const auto &model_config : model_dump_config_list_) {
    if (!model_config.optype_blacklist.empty()) {
      ExtractBlacklist(model_config.optype_blacklist, optype_blacklist_map_[model_config.model_name]);
    }
    if (!model_config.opname_blacklist.empty()) {
      ExtractBlacklist(model_config.opname_blacklist, opname_blacklist_map_[model_config.model_name]);
    }
  }
}

bool DumpConfig::IsInIndexBlacklist(const ModelIndexBlacklistMap &blacklist_map, const std::string &model_name,
                                    const std::string &root_graph_name, const std::string &op_key, uint32_t index,
                                    bool is_input) {
  // 对齐 v1 DataDumper：依次按 model_name、root_graph_name、全局配置（model_name 为空）匹配
  const std::string match_keys[] = {model_name, root_graph_name, std::string()};
  for (const auto &match_key : match_keys) {
    const auto model_it = blacklist_map.find(match_key);
    if (model_it == blacklist_map.end()) {
      continue;
    }
    const auto op_it = model_it->second.find(op_key);
    if (op_it == model_it->second.end()) {
      continue;
    }
    const auto &indices = is_input ? op_it->second.input_indices : op_it->second.output_indices;
    if (indices.count(index) > 0U) {
      return true;
    }
  }
  return false;
}

void DumpConfig::GetInputBlacklistIndices(const std::string &model_name, const std::string &root_graph_name,
                                          const std::string &op_name, const std::string &op_type, uint32_t io_num,
                                          std::set<uint32_t> &indices) const {
  indices.clear();
  std::lock_guard<std::mutex> lock(mutex_);
  for (uint32_t index = 0U; index < io_num; ++index) {
    if (IsInIndexBlacklist(opname_blacklist_map_, model_name, root_graph_name, op_name, index, true)) {
      GELOGI("[Dumper] Node name %s, Node type: %s, input index %u is in opname-blacklist, skip to dump this input.",
             op_name.c_str(), op_type.c_str(), index);
      (void)indices.insert(index);
      continue;
    }
    if (IsInIndexBlacklist(optype_blacklist_map_, model_name, root_graph_name, op_type, index, true)) {
      GELOGI("[Dumper] Node name %s, Node type: %s, input index %u is in optype-blacklist, skip to dump this input.",
             op_name.c_str(), op_type.c_str(), index);
      (void)indices.insert(index);
    }
  }
}

void DumpConfig::GetOutputBlacklistIndices(const std::string &model_name, const std::string &root_graph_name,
                                           const std::string &op_name, const std::string &op_type, uint32_t io_num,
                                           std::set<uint32_t> &indices) const {
  indices.clear();
  std::lock_guard<std::mutex> lock(mutex_);
  for (uint32_t index = 0U; index < io_num; ++index) {
    if (IsInIndexBlacklist(opname_blacklist_map_, model_name, root_graph_name, op_name, index, false)) {
      GELOGI("[Dumper] Node name %s, Node type: %s, output index %u is in opname-blacklist, skip to dump this output.",
             op_name.c_str(), op_type.c_str(), index);
      (void)indices.insert(index);
      continue;
    }
    if (IsInIndexBlacklist(optype_blacklist_map_, model_name, root_graph_name, op_type, index, false)) {
      GELOGI("[Dumper] Node name %s, Node type: %s, output index %u is in optype-blacklist, skip to dump this output.",
             op_name.c_str(), op_type.c_str(), index);
      (void)indices.insert(index);
    }
  }
}

std::vector<std::pair<std::string, std::string>> DumpConfig::GetOpDumpRanges(const std::string &model_name,
                                                                             const std::string &root_graph_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::pair<std::string, std::string>> ranges;
  // 对齐 v1 DumpProperties::GetDumpOpRangeModelName 的匹配优先级：优先精确模型名，其次 root graph 名；
  // 同名条目对齐 v1 DumpProperties::SetOpDumpRange 的 clear+assign 语义，后写覆盖先写（last-wins）。
  const std::string match_keys[] = {model_name, root_graph_name};
  for (const auto &match_key : match_keys) {
    if (match_key.empty()) {
      continue;
    }
    for (const auto &model_config : model_dump_config_list_) {
      if ((model_config.model_name == match_key) && !model_config.dump_op_ranges.empty()) {
        ranges = model_config.dump_op_ranges;
      }
    }
    if (!ranges.empty()) {
      return ranges;
    }
  }
  return ranges;
}

std::vector<std::string> DumpConfig::Split(const std::string &str, const char_t *const delimiter) {
  std::vector<std::string> result;
  size_t start = 0;
  size_t end = str.find(delimiter);
  while (end != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + 1;
    end = str.find(delimiter, start);
  }
  result.push_back(str.substr(start));
  return result;
}

}  // namespace dump
}  // namespace ge
