/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef INC_REGISTER_PASS_OPTION_UTILS_H
#define INC_REGISTER_PASS_OPTION_UTILS_H

#include <map>
#include <string>
#include "optimization_option_registry.h"
#include "graph/error_codes.h"
#include "register/register_custom_pass.h"
namespace ge {
class PassOptionUtils {
 public:
  // 查询 GE 优化选项：同名 on/off 优先，否则按 Pass 绑定的注册选项判定。
  // 未找到有效的绑定不等同于关闭，此时返回错误且不修改 is_enabled。不检查开关文件或 PassSwitch。
  static graphStatus CheckIsPassEnabled(const std::string &pass_name, bool &is_enabled);

  // 只读取有效选项表中的同名 on/off；没有显式开关时返回 GRAPH_FAILED，不修改 is_enabled。
  static graphStatus CheckIsPassEnabledByOption(const std::string &pass_name, bool &is_enabled);

  /**
   * 决定自定义 Pass 是否执行，不检查分级编译或 Pass 绑定的注册选项。
   * 优先级：显式 on/off > JSON 精确匹配 > JSON ALL 通配 > 注册默认值。
   * @param pass_name_2_switches JSON开关配置map
   * @param pass_name pass名称
   * @param default_switch 注册时声明的默认开关状态
   * @return true表示执行，false表示跳过
   * @since 9.3.0(2026-09)
   */
  static bool IsPassEnable(const std::map<std::string, bool> &pass_name_2_switches, const std::string &pass_name,
                           PassSwitch default_switch);
};
}  // namespace ge

#endif  // INC_REGISTER_PASS_OPTION_UTILS_H
