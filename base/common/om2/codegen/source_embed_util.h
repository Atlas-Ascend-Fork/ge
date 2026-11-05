/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_COMMON_OM2_CODEGEN_SOURCE_EMBED_UTIL_H_
#define GE_COMMON_OM2_CODEGEN_SOURCE_EMBED_UTIL_H_

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "ge/ge_api_types.h"

namespace ge {
// 把仓库内的源文件全文逐字拼进模型 SO 的产物。SO 只链接 -lacl_rt、只有 6 条 -I，
// 工具包不 install 任何 .cc，故 runtime/om2 下的实现既不能 include 也不能链接，
// 只能在 codegen 期把源码文本内嵌进产物。
// 资源表由 base/CMakeLists.txt 在 configure 阶段生成，键为相对仓库根的路径。

// 是否为需剥掉的 quote 形式 include。这类路径工具包里没有，其符号由调用方另行供给
// （更早内联的兄弟单元、适配 StablePart、或 BuildIncludes 补回的公开头）。
// 逐行解析而非走预处理器：产物是单文件自包含的。
bool IsInternalInclude(const std::string &line);

// 是否为 angle 形式 include，是则取出头名。这类头工具包里有，但每个单元各带一份会让产物
// 出现十几处 include 站点、同一头重复多达 8 次，故剥掉后由调用方在文件头统一发射并集。
bool AngleIncludeHeader(const std::string &line, std::string &header);

// 按 units 的给定次序把源码拼进 text：剥掉两种形式的 include、头部版权块与 include 守卫，
// 每个单元前加边界注释；被剥掉的 angle include 头名收进 angle_includes（去重）。
// 用边界注释而非 #line：模型 SO 在用户机器上编译，#line 指向的 runtime/om2/... 在那儿不存在，
// 会让诊断与日志宏的 __FILE__ 全部失效；全文行号改由 EmitFile 的文件级 #line 统管。
// 剥版权块：csrc 定性为客户模型产物（见 om2_package_contants.h 的 OM2_RUNTIME_CSRC_DIR_FORMAT）。
// 剥守卫：宏名是仓库内部路径的投影，且单 TU 内每单元只内嵌一次，守卫是死代码。
// 上提 angle include 安全：实测全部位于单元头部、不在条件编译块内，且都是自带守卫的标准头。
// 代价是内嵌文本与上游不逐字节相同，故产物段落不能直接 diff 上游；上游修复仍自动跟随。
// 单元缺失即失败：资源表与 units 列表由同一处 CMake 维护，不一致就是构建配置错误。
Status EmbedUnits(const std::map<std::string, std::string_view> &sources, const std::vector<std::string> &units,
                  const std::string &model_name, const std::string &log_tag, std::string &text,
                  std::set<std::string> &angle_includes);
}  // namespace ge

#endif  // GE_COMMON_OM2_CODEGEN_SOURCE_EMBED_UTIL_H_
