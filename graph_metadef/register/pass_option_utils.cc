/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "register/pass_option_utils.h"
#include <cstdint>
#include <vector>
#include "register/optimization_option_registry.h"
#include "ge_common/debug/ge_log.h"
#include "graph/ge_local_context.h"

namespace ge {
namespace {
enum class PassSwitchDecision : uint8_t {
  kUnspecified,
  kEnabled,
  kDisabled,
};

PassSwitchDecision FindOptimizationSwitchOverride(const std::string &pass_name) {
  std::string opt_value;
  if (GetThreadLocalContext().GetOo().GetValue(pass_name, opt_value) != GRAPH_SUCCESS) {
    return PassSwitchDecision::kUnspecified;
  }
  if (opt_value == "on") {
    return PassSwitchDecision::kEnabled;
  }
  if (opt_value == "off") {
    return PassSwitchDecision::kDisabled;
  }
  return PassSwitchDecision::kUnspecified;
}

PassSwitchDecision FindFusionSwitchFileOverride(const std::map<std::string, bool> &file_switches,
                                                const std::string &pass_name) {
  static const std::string kPassSwitchAll = "ALL";
  auto entry = file_switches.find(pass_name);
  if (entry == file_switches.cend()) {
    entry = file_switches.find(kPassSwitchAll);
  }
  if (entry == file_switches.cend()) {
    return PassSwitchDecision::kUnspecified;
  }
  if (!entry->second) {
    GELOGI("[PassSwitch] Pass [%s] is disabled by fusion switch file: entry [%s] is off.", pass_name.c_str(),
           entry->first.c_str());
    return PassSwitchDecision::kDisabled;
  }
  return PassSwitchDecision::kEnabled;
}

bool IsEnabledByRegistrationDefault(const std::string &pass_name, PassSwitch default_switch) {
  if (default_switch == PassSwitch::kOff) {
    GELOGI("[PassSwitch] Pass [%s] is disabled by registration default (DefaultSwitch kOff).", pass_name.c_str());
  }
  return default_switch == PassSwitch::kOn;
}

graphStatus ResolveRegisteredPassSwitch(const std::string &pass_name, bool &is_enabled) {
  std::vector<std::string> opt_names;
  const auto ret = PassOptionRegistry::GetInstance().FindOptionNamesByPassName(pass_name, opt_names);
  if (ret != GRAPH_SUCCESS) {
    // 若Pass未被注册，返回非零错误码，由调用方判断如何处理
    GELOGI("The pass [%s] is not registered", pass_name.c_str());
    return ret;
  }

  // 当前最多支持两级开关，opt_names.size() <= 2
  std::vector<const OoInfo *> opt_infos;
  for (const auto &opt_name : opt_names) {
    const auto info_ptr = OptionRegistry::GetInstance().FindOptInfo(opt_name);
    if (info_ptr == nullptr) {
      GELOGW("Option [%s] of pass [%s] is not registered", opt_name.c_str(), pass_name.c_str());
      continue;
    }
    (void)opt_infos.emplace_back(info_ptr);
  }
  // Pass关联的选项均未注册,说明注册阶段遗漏了选项
  if (opt_infos.empty()) {
    GELOGW("the pass [%s] has no registered option", pass_name.c_str());
    return GRAPH_FAILED;
  }

  is_enabled = false;
  const auto &oo = GetThreadLocalContext().GetOo();
  // 有值的二级选项优先于一级选项；空串和 "true" 表示开启。
  for (auto it = opt_infos.crbegin(); it != opt_infos.crend(); ++it) {
    const auto opt = *it;
    std::string opt_value;
    if (oo.GetValue(opt->name, opt_value) != GRAPH_SUCCESS) {
      continue;
    }
    is_enabled = opt_value.empty() || (opt_value == "true");
    GELOGD("The pass [%s] is %s, option [%s] is [%s]", pass_name.c_str(), is_enabled ? "enabled" : "disabled",
           opt->name.c_str(), opt_value.c_str());
    return GRAPH_SUCCESS;
  }
  // OoTable中没有配置该Pass的开关选项，说明不使能
  GELOGD("the pass [%s] is disabled, option is not in working option table", pass_name.c_str());
  return GRAPH_SUCCESS;
}

}  // namespace

graphStatus PassOptionUtils::CheckIsPassEnabled(const std::string &pass_name, bool &is_enabled) {
  const auto override = FindOptimizationSwitchOverride(pass_name);
  if (override != PassSwitchDecision::kUnspecified) {
    is_enabled = (override == PassSwitchDecision::kEnabled);
    return GRAPH_SUCCESS;
  }
  return ResolveRegisteredPassSwitch(pass_name, is_enabled);
}

graphStatus PassOptionUtils::CheckIsPassEnabledByOption(const std::string &pass_name, bool &is_enabled) {
  const auto override = FindOptimizationSwitchOverride(pass_name);
  if (override == PassSwitchDecision::kUnspecified) {
    return GRAPH_FAILED;
  }

  is_enabled = (override == PassSwitchDecision::kEnabled);
  GELOGD("The pass [%s] is configured by graph options, switch is [%s]", pass_name.c_str(), is_enabled ? "on" : "off");
  return GRAPH_SUCCESS;
}

bool PassOptionUtils::IsPassEnable(const std::map<std::string, bool> &pass_name_2_switches,
                                   const std::string &pass_name, PassSwitch default_switch) {
  const auto optimization_override = FindOptimizationSwitchOverride(pass_name);
  if (optimization_override != PassSwitchDecision::kUnspecified) {
    if (optimization_override == PassSwitchDecision::kDisabled) {
      GELOGI("[PassSwitch] Pass [%s] is disabled by graph option (--optimization_switch).", pass_name.c_str());
    }
    return optimization_override == PassSwitchDecision::kEnabled;
  }
  const auto file_override = FindFusionSwitchFileOverride(pass_name_2_switches, pass_name);
  if (file_override != PassSwitchDecision::kUnspecified) {
    return file_override == PassSwitchDecision::kEnabled;
  }
  return IsEnabledByRegistrationDefault(pass_name, default_switch);
}
}  // namespace ge
