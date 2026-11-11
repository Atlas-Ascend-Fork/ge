/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "register/core_num_utils.h"

#include <cstdint>
#include <mutex>
#include <string>

#include "ge_common/debug/ge_log.h"
#include "common/checker.h"
#include "graph/ge_context.h"
#include "graph/ge_local_context.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph_metadef/common/plugin/plugin_manager.h"
#include "mmpa/mmpa_api.h"
#include "platform/platform_info.h"

namespace ge {
namespace {
constexpr int32_t kRtErrorNone = 0;
constexpr uint32_t kSocSpecValueLen = 32U;
const std::string kSocSpecLabel = "SoCInfo";
const std::string kSocSpecKeyAiCoreCnt = "ai_core_cnt";
const std::string kSocSpecKeyCubeCoreCnt = "cube_core_cnt";
const std::string kSocSpecKeyVectorCoreCnt = "vector_core_cnt";
const std::string kRuntimeLibName = "libruntime.so";
const std::string kRtGetSocSpecName = "rtGetSocSpec";

// rtGetSocSpec 是 runtime 侧 C 接口，形参为字符指针；这里按仓库惯例用 char_t（即 char）描述该 ABI。
using RtGetSocSpecFunc = int32_t (*)(const char_t *, const char_t *, char_t *, const uint32_t);

// 解析阶段临时数据，不入库。cube_core_cnt 查询失败时回退 ai_core_cnt。
struct SocCoreSpec {
  uint32_t ai_core_cnt;
  uint32_t cube_core_cnt;
  uint32_t vector_core_cnt;
};

enum class CoreNumCheckResult {
  kOk,
  kNotInteger,
  kInvalidFormat,
  kOutOfRange,
};

void LogInvalidAicoreNumFormat(const std::string &value) {
  GELOGW("Invalid format for ge.aicoreNum: %s. Expected format: aicore_num|vectorcore_num.", value.c_str());
}

std::string GetRuntimeLibPath() {
  const std::string lib_dir = ge::GetModelPath();
  if (lib_dir.empty()) {
    GELOGW("[SocSpec][Path]Failed to get lib directory path.");
    return "";
  }
  return lib_dir + kRuntimeLibName;
}

RtGetSocSpecFunc GetRtGetSocSpecFunc() {
  static std::once_flag load_flag;
  static RtGetSocSpecFunc func = nullptr;
  static void *runtime_handle = nullptr;

  std::call_once(load_flag, []() {
    const std::string runtime_path = GetRuntimeLibPath();
    if (runtime_path.empty()) {
      return;
    }

    GELOGI("[SocSpec][Load]Loading runtime from: %s", runtime_path.c_str());
    runtime_handle = mmDlopen(runtime_path.c_str(), MMPA_RTLD_NOW);
    if (runtime_handle == nullptr) {
      const char_t *error = mmDlerror();
      GELOGW("[SocSpec][Load]mmDlopen failed, path=%s, error=%s", runtime_path.c_str(),
             (error == nullptr) ? "" : error);
      return;
    }

    func = reinterpret_cast<RtGetSocSpecFunc>(mmDlsym(runtime_handle, kRtGetSocSpecName.c_str()));
    if (func == nullptr) {
      const char_t *error = mmDlerror();
      GELOGW("[SocSpec][Symbol]mmDlsym rtGetSocSpec failed, error=%s", (error == nullptr) ? "" : error);
      // 符号查询失败说明句柄后续不会再使用，关闭句柄避免库常驻内存；成功分支则保留句柄使函数指针长期有效。
      (void)mmDlclose(runtime_handle);
      runtime_handle = nullptr;
      return;
    }
    GELOGI("[SocSpec][Load]Successfully loaded rtGetSocSpec.");
  });

  return func;
}

bool ParseUint32(const std::string &value, uint32_t &parsed) {
  if (value.empty()) {
    return false;
  }
  uint64_t result = 0U;
  for (const char ch : value) {
    // 只接受十进制数字：空白、正负号、小数点等一律判失败，避免解析出错误核数
    if ((ch < '0') || (ch > '9')) {
      return false;
    }
    result = (result * 10U) + static_cast<uint64_t>(ch - '0');
    if (result > static_cast<uint64_t>(UINT32_MAX)) {
      return false;
    }
  }
  parsed = static_cast<uint32_t>(result);
  return true;
}

// 查询单个 SoC 规格键。warn_on_failure 为 false 时用于可回退的键（cube_core_cnt），失败不留告警。
bool QuerySocSpecValue(const std::string &key, bool warn_on_failure, uint32_t &core_cnt) {
  const RtGetSocSpecFunc rt_get_soc_spec = GetRtGetSocSpecFunc();
  if (rt_get_soc_spec == nullptr) {
    if (warn_on_failure) {
      GELOGW("skip deriving ge.vectorcoreNum: failed to load libruntime.so or symbol rtGetSocSpec.");
    }
    return false;
  }

  std::string value(kSocSpecValueLen, '\0');
  const int32_t ret = rt_get_soc_spec(kSocSpecLabel.c_str(), key.c_str(), value.data(), kSocSpecValueLen);
  value[kSocSpecValueLen - 1U] = '\0';
  // 查询结果按 C 字符串写到缓冲区，去掉尾部填充的 '\0' 后再解析
  value.resize(std::char_traits<char>::length(value.c_str()));
  if ((ret == kRtErrorNone) && ParseUint32(value, core_cnt)) {
    GELOGD("[SocSpec][Query]label: %s, key: %s, value: %s, core_cnt: %u.", kSocSpecLabel.c_str(), key.c_str(),
           value.c_str(), core_cnt);
    return true;
  }

  if (warn_on_failure) {
    GELOGW("skip deriving ge.vectorcoreNum: failed to get SoC spec, label=%s, key=%s, ret=0x%X.", kSocSpecLabel.c_str(),
           key.c_str(), static_cast<uint32_t>(ret));
  }
  return false;
}

// 查询派生所需的核数。ai_core_cnt 必须成功，cube_core_cnt 失败回退 ai_core_cnt，任一失败返回 false。
bool QuerySocCoreSpec(SocCoreSpec &spec) {
  spec.ai_core_cnt = 0U;
  spec.cube_core_cnt = 0U;
  spec.vector_core_cnt = 0U;
  const bool ai_core_cnt_ok = QuerySocSpecValue(kSocSpecKeyAiCoreCnt, true, spec.ai_core_cnt);
  if (!ai_core_cnt_ok) {
    return false;
  }
  if (spec.ai_core_cnt == 0U) {
    GELOGW("skip deriving ge.vectorcoreNum: failed to get SoC spec, label=%s, key=%s.", kSocSpecLabel.c_str(),
           kSocSpecKeyAiCoreCnt.c_str());
    return false;
  }
  if (!QuerySocSpecValue(kSocSpecKeyCubeCoreCnt, false, spec.cube_core_cnt)) {
    spec.cube_core_cnt = spec.ai_core_cnt;
    GELOGD("[SocSpec][Query]cube_core_cnt is unavailable, fallback to ai_core_cnt: %u.", spec.ai_core_cnt);
  }
  return QuerySocSpecValue(kSocSpecKeyVectorCoreCnt, true, spec.vector_core_cnt);
}

// 核数规则单一来源：整数解析、规范格式、范围校验。report_error 为 true 时按旧行为上报 E10060。
CoreNumCheckResult CheckCoreNum(const std::string &value, int32_t min_value, int32_t max_value, int32_t &parsed_value,
                                bool report_error) {
  try {
    parsed_value = std::stoi(value);
  } catch (const std::invalid_argument &) {
    if (report_error) {
      GELOGE(GRAPH_FAILED, "[Parse][Param]Failed, digit str:%s is invalid", value.c_str());
      const std::string reason = "digit str:" + value + " is invalid";
      (void)REPORT_PREDEFINED_ERR_MSG("E10060", std::vector<const char *>({"reason"}),
                                      std::vector<const char *>({reason.c_str()}));
    }
    return CoreNumCheckResult::kNotInteger;
  } catch (const std::out_of_range &) {
    if (report_error) {
      GELOGE(GRAPH_FAILED, "[Parse][Param]Failed, digit str:%s cannot change to int", value.c_str());
      const std::string reason = "digit str:" + value + " cannot change to int";
      (void)REPORT_PREDEFINED_ERR_MSG("E10060", std::vector<const char *>({"reason"}),
                                      std::vector<const char *>({reason.c_str()}));
    }
    return CoreNumCheckResult::kNotInteger;
  } catch (...) {
    if (report_error) {
      GELOGE(GRAPH_FAILED, "[Parse][Param]Failed, digit str:%s cannot change to int", value.c_str());
      const std::string reason = "digit str:" + value + " cannot change to int";
      (void)REPORT_PREDEFINED_ERR_MSG("E10060", std::vector<const char *>({"reason"}),
                                      std::vector<const char *>({reason.c_str()}));
    }
    return CoreNumCheckResult::kNotInteger;
  }

  if (std::to_string(parsed_value) != value) {
    return CoreNumCheckResult::kInvalidFormat;
  }
  if ((parsed_value < min_value) || (parsed_value > max_value)) {
    return CoreNumCheckResult::kOutOfRange;
  }
  return CoreNumCheckResult::kOk;
}

// 静默校验：只判断能否作为合法核数解析，不产生任何 error。用于解析阶段判断"能否派生"。
graphStatus TryParseCoreNum(const std::string &value, int32_t min_value, int32_t max_value, int32_t &parsed_value) {
  return (CheckCoreNum(value, min_value, max_value, parsed_value, false) == CoreNumCheckResult::kOk) ? GRAPH_SUCCESS
                                                                                                     : GRAPH_FAILED;
}

// 显式 ge.vectorcoreNum 查询：本入口 options 优先，其次线程上下文（graph/session/global）。
bool LookupExplicitVectorCoreNum(const std::map<std::string, std::string> &options, std::string &value) {
  const auto iter = options.find(kVectorCoreNum);
  if (iter != options.end()) {
    value = iter->second;
    return true;
  }
  return GetThreadLocalContext().GetOption(kVectorCoreNum, value) == GRAPH_SUCCESS;
}

graphStatus ReportParamError(const std::string &param_name, const std::string &param_value_str,
                             const std::string &reason) {
  (void)REPORT_PREDEFINED_ERR_MSG(
      "E10001", std::vector<const char *>({"parameter", "value", "reason"}),
      std::vector<const char *>({param_name.c_str(), param_value_str.c_str(), reason.c_str()}));
  GELOGE(PARAM_INVALID, "Value %s for parameter %s is invalid. Reason: %s", param_value_str.c_str(), param_name.c_str(),
         reason.c_str());
  return PARAM_INVALID;
}

graphStatus GetOneCoreNumFromGraph(const ge::ComputeGraphPtr &compute_graph, const std::string &attr_name,
                                   int32_t &core_num) {
  std::string core_num_str;
  if ((!ge::AttrUtils::GetStr(compute_graph, attr_name, core_num_str)) || core_num_str.empty()) {
    return GRAPH_SUCCESS;  // 未配置，保持调用方传入的默认值
  }
  return CoreNumUtils::ParseAndValidateCoreNum(attr_name, core_num_str, 0, INT32_MAX, core_num);
}

// 成对分支：X|Y 是"同时设置两个键"的捷径，拆分规则与改前保持一致，只新增冲突校验与显式优先。
graphStatus ParsePairCoreNum(std::map<std::string, std::string> &options, const std::string &origin_value,
                             const std::string &aicore_num_str, const std::string &vectorcore_num_str) {
  std::string explicit_value;
  const bool has_explicit = LookupExplicitVectorCoreNum(options, explicit_value);
  int32_t pair_vectorcore_num = 0;
  int32_t explicit_vectorcore_num = 0;
  const bool pair_is_positive =
      (TryParseCoreNum(vectorcore_num_str, 0, INT32_MAX, pair_vectorcore_num) == GRAPH_SUCCESS) &&
      (pair_vectorcore_num > 0);
  const bool explicit_is_positive =
      has_explicit && (TryParseCoreNum(explicit_value, 0, INT32_MAX, explicit_vectorcore_num) == GRAPH_SUCCESS) &&
      (explicit_vectorcore_num > 0);
  if (pair_is_positive && explicit_is_positive && (pair_vectorcore_num != explicit_vectorcore_num)) {
    const std::string reason = "Conflict: ge.vectorcoreNum is set to " + explicit_value + " explicitly, while " +
                               AICORE_NUM + " \"" + origin_value + "\" implies " + vectorcore_num_str + ".";
    return ReportParamError(ge::GetContext().GetReadableName(AICORE_NUM), origin_value, reason);
  }

  options[AICORE_NUM] = aicore_num_str;
  // 右侧为空/0 且显式值可解析为正整数时保留显式值（显式优先），其余情况沿用改前的写回规则。
  if (((vectorcore_num_str.empty()) || (vectorcore_num_str == "0")) && explicit_is_positive) {
    GELOGI("ge.vectorcoreNum is configured explicitly (%s), skip overriding by ge.aicoreNum.", explicit_value.c_str());
    return GRAPH_SUCCESS;
  }
  options[kVectorCoreNum] = vectorcore_num_str;
  return GRAPH_SUCCESS;
}

// 显式 ge.vectorcoreNum 优先：返回 true 表示允许继续派生，false 表示跳过派生（原因已记录）。
bool IsDeriveAllowedByExplicitCoreNum(const std::map<std::string, std::string> &options) {
  std::string explicit_value;
  if (!LookupExplicitVectorCoreNum(options, explicit_value)) {
    return true;
  }
  if (explicit_value.empty()) {
    GELOGI("ge.vectorcoreNum is empty or 0, treated as unconfigured.");
    return true;
  }
  int32_t explicit_vectorcore_num = 0;
  if (TryParseCoreNum(explicit_value, 0, INT32_MAX, explicit_vectorcore_num) != GRAPH_SUCCESS) {
    // 显式值非法：不派生、不覆盖，交由 platform info 报 E10001。
    GELOGD("ge.vectorcoreNum is invalid (%s), skip deriving.", explicit_value.c_str());
    return false;
  }
  if (explicit_vectorcore_num > 0) {
    GELOGI("ge.vectorcoreNum is configured explicitly (%s), skip deriving.", explicit_value.c_str());
    return false;
  }
  GELOGI("ge.vectorcoreNum is empty or 0, treated as unconfigured.");
  return true;
}

// 按芯片 Cube:Vector 比例计算派生值；无法派生时打印告警/错误并返回 -1。
int32_t CalcDerivedVectorCoreNum(const SocCoreSpec &spec, int32_t aicore_num, const std::string &aicore_num_str) {
  const uint32_t cube_core_cnt = (spec.cube_core_cnt > 0U) ? spec.cube_core_cnt : spec.ai_core_cnt;
  if (static_cast<uint32_t>(aicore_num) > cube_core_cnt) {
    LogInvalidAicoreNumFormat(aicore_num_str);
    return -1;
  }
  if (spec.vector_core_cnt == 0U) {
    GELOGW("skip deriving ge.vectorcoreNum: vector_core_cnt is 0 (no independent vector core).");
    return -1;
  }

  const uint64_t cube_core_cnt_u64 = static_cast<uint64_t>(cube_core_cnt);
  const uint64_t vector_core_cnt_u64 = static_cast<uint64_t>(spec.vector_core_cnt);
  uint32_t ratio = 0U;
  if (vector_core_cnt_u64 == cube_core_cnt_u64) {
    ratio = 1U;
  } else if (vector_core_cnt_u64 == (cube_core_cnt_u64 * 2U)) {
    ratio = 2U;
  } else {
    GELOGW("skip deriving ge.vectorcoreNum: ratio of vector_core_cnt(%u) to cube_core_cnt(%u) is not supported.",
           spec.vector_core_cnt, cube_core_cnt);
    return -1;
  }

  const int64_t derived_value = static_cast<int64_t>(aicore_num) * static_cast<int64_t>(ratio);
  if (derived_value > static_cast<int64_t>(INT32_MAX)) {
    LogInvalidAicoreNumFormat(aicore_num_str);
    return -1;
  }
  GELOGI(
      "Derive ge.vectorcoreNum: aicore_num=%d, cube_core_cnt=%u, vector_core_cnt=%u, ratio=1:%u, "
      "derived_vectorcore_num=%d.",
      aicore_num, cube_core_cnt, spec.vector_core_cnt, ratio, static_cast<int32_t>(derived_value));
  return static_cast<int32_t>(derived_value);
}

// 单值分支：显式优先 → 查询规格 → 按比例派生并写回。
graphStatus ParseSingleCoreNum(std::map<std::string, std::string> &options, const std::string &aicore_num_str,
                               int32_t aicore_num) {
  if (aicore_num == 0) {
    LogInvalidAicoreNumFormat(aicore_num_str);
    return GRAPH_SUCCESS;
  }
  if (!IsDeriveAllowedByExplicitCoreNum(options)) {
    return GRAPH_SUCCESS;
  }

  SocCoreSpec spec;
  if (!QuerySocCoreSpec(spec)) {
    return GRAPH_SUCCESS;
  }
  const int32_t derived_value = CalcDerivedVectorCoreNum(spec, aicore_num, aicore_num_str);
  if (derived_value < 0) {
    return GRAPH_SUCCESS;
  }
  options[kVectorCoreNum] = std::to_string(derived_value);
  return GRAPH_SUCCESS;
}
}  // namespace

graphStatus CoreNumUtils::ParseAicoreNumFromOption(std::map<std::string, std::string> &options) {
  const auto iter = options.find(AICORE_NUM);
  if (iter == options.end()) {
    return GRAPH_SUCCESS;
  }
  const std::string aicore_num_option_str = iter->second;
  if (aicore_num_option_str.empty()) {
    return GRAPH_SUCCESS;
  }

  const auto delimiter_pos = aicore_num_option_str.find('|');
  if (delimiter_pos != std::string::npos) {
    GELOGI("origin ge.aicoreNum in options, value: %s.", aicore_num_option_str.c_str());
    const std::string aicore_num_str = aicore_num_option_str.substr(0U, delimiter_pos);
    const std::string vectorcore_num_str = aicore_num_option_str.substr(delimiter_pos + 1U);
    return ParsePairCoreNum(options, aicore_num_option_str, aicore_num_str, vectorcore_num_str);
  }

  int32_t aicore_num = 0;
  if (TryParseCoreNum(aicore_num_option_str, 0, INT32_MAX, aicore_num) != GRAPH_SUCCESS) {
    // 静默校验失败：保持旧文案与旧行为（只告警、不派生），格式/范围由 platform info 报 E10001。
    LogInvalidAicoreNumFormat(aicore_num_option_str);
    return GRAPH_SUCCESS;
  }
  return ParseSingleCoreNum(options, aicore_num_option_str, aicore_num);
}

graphStatus CoreNumUtils::ParseAndValidateCoreNum(const std::string &param_name, const std::string &param_value_str,
                                                  int32_t min_value, int32_t max_value, int32_t &parsed_value) {
  const CoreNumCheckResult check_result = CheckCoreNum(param_value_str, min_value, max_value, parsed_value, true);
  if (check_result == CoreNumCheckResult::kNotInteger) {
    return ReportParamError(param_name, param_value_str, "It is not integer.");
  }
  if (check_result == CoreNumCheckResult::kInvalidFormat) {
    return ReportParamError(param_name, param_value_str,
                            "Invalid integer format: only non-negative decimal digits are allowed "
                            "(no leading zeros, spaces, or mixed characters).");
  }
  if (check_result == CoreNumCheckResult::kOutOfRange) {
    return ReportParamError(param_name, param_value_str,
                            "It is out of range, value must be in range (" + std::to_string(min_value) + ", " +
                                std::to_string(max_value) + "].");
  }

  GELOGD("Parse and validate core num success, param_name: %s, param_value_str: %s, parsed_value: %d",
         param_name.c_str(), param_value_str.c_str(), parsed_value);
  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::GetGeDefaultPlatformInfo(const std::string &soc_version, fe::PlatformInfo &platform_info) {
  GE_ASSERT_TRUE(fe::PlatformInfoManager::GeInstance().InitializePlatformInfo() == 0U,
                 "Initialize platform info failed.");

  fe::OptionalInfo optional_info;
  GE_ASSERT_TRUE(fe::PlatformInfoManager::GeInstance().GetPlatformInfo(soc_version, platform_info, optional_info) == 0U,
                 "Unable to get platform info.");
  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::ValidateCoreNumWithOpDesc(const fe::PlatformInfo &platform_info,
                                                    const ge::OpDescPtr &op_desc) {
  GE_ASSERT_NOTNULL(op_desc);

  if (op_desc->HasAttr(kAiCoreNumOp)) {
    std::string aicore_num_str;
    if (!ge::AttrUtils::GetStr(op_desc, kAiCoreNumOp, aicore_num_str)) {
      return ReportParamError(kAiCoreNumOp, "<non-string>", "It is not string.");
    }
    GELOGD("Validate %s for op[%s], value: %s, platform ai_core_cnt: %d.", kAiCoreNumOp.c_str(),
           op_desc->GetName().c_str(), aicore_num_str.c_str(),
           static_cast<int32_t>(platform_info.soc_info.ai_core_cnt));
    int32_t op_core_num = -1;
    GE_ASSERT_SUCCESS(ParseAndValidateCoreNum(kAiCoreNumOp, aicore_num_str, 0,
                                              static_cast<int32_t>(platform_info.soc_info.ai_core_cnt), op_core_num));
  }

  if (op_desc->HasAttr(kVectorCoreNumOp)) {
    std::string vector_core_num_str;
    if (!ge::AttrUtils::GetStr(op_desc, kVectorCoreNumOp, vector_core_num_str)) {
      return ReportParamError(kVectorCoreNumOp, "<non-string>", "It is not string.");
    }
    GELOGD("Validate %s for op[%s], value: %s, platform vector_core_cnt: %d.", kVectorCoreNumOp.c_str(),
           op_desc->GetName().c_str(), vector_core_num_str.c_str(),
           static_cast<int32_t>(platform_info.soc_info.vector_core_cnt));
    int32_t op_core_num = -1;
    GE_ASSERT_SUCCESS(ParseAndValidateCoreNum(kVectorCoreNumOp, vector_core_num_str, 0,
                                              static_cast<int32_t>(platform_info.soc_info.vector_core_cnt),
                                              op_core_num));
  }

  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::ValidateCoreNumWithGraph(const ge::ComputeGraphPtr &compute_graph) {
  GE_ASSERT_NOTNULL(compute_graph);

  bool is_platform_info_initialized = false;
  fe::PlatformInfo platform_info;
  size_t validated_op_count = 0U;
  for (const auto &node : compute_graph->GetAllNodes()) {
    GE_ASSERT_NOTNULL(node);
    const auto op_desc = node->GetOpDesc();
    GE_ASSERT_NOTNULL(op_desc);
    if (!op_desc->HasAttr(kAiCoreNumOp) && !op_desc->HasAttr(kVectorCoreNumOp)) {
      continue;
    }

    if (!is_platform_info_initialized) {
      std::string soc_version;
      GE_ASSERT_SUCCESS(GetThreadLocalContext().GetOption(ge::SOC_VERSION, soc_version), "Get SOC_VERSION failed.");
      GE_ASSERT_TRUE(!soc_version.empty(), "SOC_VERSION is not set before validating op core num.");
      GE_ASSERT_SUCCESS(GetGeDefaultPlatformInfo(soc_version, platform_info));
      is_platform_info_initialized = true;
      GELOGD(
          "Init platform info for core num validation, graph: %s, soc_version: %s, ai_core_cnt: %d, "
          "vector_core_cnt: %d.",
          compute_graph->GetName().c_str(), soc_version.c_str(),
          static_cast<int32_t>(platform_info.soc_info.ai_core_cnt),
          static_cast<int32_t>(platform_info.soc_info.vector_core_cnt));
    }
    GE_ASSERT_SUCCESS(ValidateCoreNumWithOpDesc(platform_info, op_desc));
    ++validated_op_count;
  }
  if (validated_op_count > 0U) {
    GELOGD("Validate op core num with graph success, graph: %s, validated op count: %zu.",
           compute_graph->GetName().c_str(), validated_op_count);
  }
  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::GetCoreNumFromGraph(const ge::ComputeGraphPtr &compute_graph, int32_t &aicore_num,
                                              int32_t &vectorcore_num) {
  GE_ASSERT_NOTNULL(compute_graph);
  GE_ASSERT_SUCCESS(GetOneCoreNumFromGraph(compute_graph, AICORE_NUM, aicore_num), "Get %s from graph %s failed.",
                    AICORE_NUM.c_str(), compute_graph->GetName().c_str());
  GE_ASSERT_SUCCESS(GetOneCoreNumFromGraph(compute_graph, kVectorCoreNum, vectorcore_num),
                    "Get %s from graph %s failed.", kVectorCoreNum.c_str(), compute_graph->GetName().c_str());
  if ((aicore_num >= 0) || (vectorcore_num >= 0)) {
    GELOGI("Get core num from graph %s, aicore_num: %d, vectorcore_num: %d.", compute_graph->GetName().c_str(),
           aicore_num, vectorcore_num);
  }
  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::FillCoreNumOptions(int32_t aicore_num, int32_t vectorcore_num,
                                             std::map<std::string, std::string> &options) {
  if (aicore_num >= 0) {
    options[AICORE_NUM] = std::to_string(aicore_num);
  }
  if (vectorcore_num >= 0) {
    options[kVectorCoreNum] = std::to_string(vectorcore_num);
  }
  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::GetCoreNumOptionsFromGraph(const ge::ComputeGraphPtr &compute_graph,
                                                     std::map<std::string, std::string> &options) {
  GE_ASSERT_NOTNULL(compute_graph);
  // 模型级核数只写在根图上(见compiler/graph/manager/graph_manager.cc PersistCoreNumOptionsToRootGraph),
  // 静态编译子图的GeModel持有的是根图的子图, 需要上溯。
  const auto root_graph = ge::GraphUtils::FindRootGraph(compute_graph);
  GE_ASSERT_NOTNULL(root_graph, "Find root graph of %s failed.", compute_graph->GetName().c_str());
  int32_t aicore_num = -1;
  int32_t vectorcore_num = -1;
  GE_ASSERT_SUCCESS(GetCoreNumFromGraph(root_graph, aicore_num, vectorcore_num));
  return FillCoreNumOptions(aicore_num, vectorcore_num, options);
}

graphStatus CoreNumUtils::UpdateCoreCountWithOpDesc(const std::string &param_name, const std::string &op_core_num_str,
                                                    int32_t soc_core_num, const std::string &res_key,
                                                    std::map<std::string, std::string> &res) {
  int32_t op_core_num = -1;
  GE_ASSERT_SUCCESS(ParseAndValidateCoreNum(param_name, op_core_num_str, 0, soc_core_num, op_core_num));
  if (op_core_num > 0) {
    GELOGD("Change %s from platform %d to op_desc %d.", res_key.c_str(), soc_core_num, op_core_num);
    res[res_key] = std::to_string(op_core_num);
  }

  return GRAPH_SUCCESS;
}

graphStatus CoreNumUtils::UpdatePlatformInfosWithOpDesc(const fe::PlatformInfo &platform_info,
                                                        const ge::OpDescPtr &op_desc, fe::PlatFormInfos &platform_infos,
                                                        bool &is_op_core_num_set) {
  std::map<std::string, std::string> res;
  (void)platform_infos.GetPlatformResWithLock("SoCInfo", res);

  std::string aicore_num_str;
  if (ge::AttrUtils::GetStr(op_desc, kAiCoreNumOp, aicore_num_str)) {
    GELOGD("Attr: %s exists in op_desc, opName: %s", kAiCoreNumOp.c_str(), op_desc->GetName().c_str());
    GE_ASSERT_SUCCESS(UpdateCoreCountWithOpDesc(
        kAiCoreNumOp, aicore_num_str, static_cast<int32_t>(platform_info.soc_info.ai_core_cnt), kAiCoreCntIni, res));
    res[kCubeCoreCntIni] = res[kAiCoreCntIni];
    is_op_core_num_set = true;
  }

  std::string vector_core_num_str;
  if (ge::AttrUtils::GetStr(op_desc, kVectorCoreNumOp, vector_core_num_str)) {
    GELOGD("Attr: %s exists in op_desc, opName: %s", kVectorCoreNumOp.c_str(), op_desc->GetName().c_str());
    GE_ASSERT_SUCCESS(UpdateCoreCountWithOpDesc(kVectorCoreNumOp, vector_core_num_str,
                                                static_cast<int32_t>(platform_info.soc_info.vector_core_cnt),
                                                kVectorCoreCntIni, res));
    is_op_core_num_set = true;
  }

  if (is_op_core_num_set) {
    GELOGI("Need to update core cnt in platform infos.");
    platform_infos.SetPlatformResWithLock(kSocInfo, res);
  }

  return GRAPH_SUCCESS;
}
}  // namespace ge
