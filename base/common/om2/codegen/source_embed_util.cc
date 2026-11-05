/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/codegen/source_embed_util.h"

#include <cctype>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "common/ge_common/debug/ge_log.h"

namespace ge {
namespace {
// 恰好一对 #ifndef/#endif 才认定存在 include 守卫。上游若给某单元加了别的条件编译
// （#ifdef __cplusplus、特性宏等），计数就不再是 2，此时整体跳过守卫剥离——宁可少剥，
// 也不能只剥掉配对的一半而让产物编译不过。少剥会被 UT 的"产物 0 处 #ifndef"断言逮到。
constexpr size_t kGuardDirectiveNum = 2U;

bool IsBlank(const std::string &line) {
  return line.find_first_not_of(" \t\r") == std::string::npos;
}

bool IsIdentChar(char c) {
  return (isalnum(static_cast<unsigned char>(c)) != 0) || (c == '_');
}

bool IsConditional(const std::string &name) {
  return (name == "if") || (name == "ifdef") || (name == "ifndef") || (name == "else") || (name == "elif") ||
         (name == "endif");
}

// 拆出预处理行的指令名与其后内容（已去前导空白）。非预处理行返回 false。
// 按指令名整词比较，故 #include 不会被误认成 #if 一族。
bool SplitDirective(const std::string &line, std::string &name, std::string &rest) {
  const size_t hash = line.find_first_not_of(" \t");
  if ((hash == std::string::npos) || (line[hash] != '#')) {
    return false;
  }
  const size_t begin = line.find_first_not_of(" \t", hash + 1U);
  if (begin == std::string::npos) {
    return false;
  }
  size_t end = begin;
  while ((end < line.size()) && (isalpha(static_cast<unsigned char>(line[end])) != 0)) {
    ++end;
  }
  name = line.substr(begin, end - begin);
  const size_t rest_begin = line.find_first_not_of(" \t", end);
  rest = (rest_begin == std::string::npos) ? "" : line.substr(rest_begin);
  return true;
}

// rest 是否为一个裸标识符（无值、无参数列表），用以排除 #define F(x) 与 #define K 1
bool IsBareIdent(const std::string &rest) {
  size_t end = rest.size();
  while ((end > 0U) && ((rest[end - 1U] == ' ') || (rest[end - 1U] == '\t') || (rest[end - 1U] == '\r'))) {
    --end;
  }
  if ((end == 0U) || (isdigit(static_cast<unsigned char>(rest[0])) != 0)) {
    return false;
  }
  for (size_t i = 0U; i < end; ++i) {
    if (!IsIdentChar(rest[i])) {
      return false;
    }
  }
  return true;
}

// 匹配 "#<kw> <IDENT>" 且 IDENT 之后整行无其他内容
bool MatchBareIdent(const std::string &line, const std::string &kw, std::string &ident) {
  std::string name;
  std::string rest;
  if (!SplitDirective(line, name, rest) || (name != kw) || !IsBareIdent(rest)) {
    return false;
  }
  ident = rest.substr(0U, rest.find_last_not_of(" \t\r") + 1U);
  return true;
}

// #endif 允许带尾注释。不用注释文本去匹配宏名：上游
// om2_format_transfer_fz_c04.h 的 #endif 注释就写错了（比 #ifndef 多出 NCHW_）。
bool IsEndif(const std::string &line) {
  std::string name;
  std::string rest;
  if (!SplitDirective(line, name, rest) || (name != "endif")) {
    return false;
  }
  return rest.empty() || (rest.compare(0U, 2U, "//") == 0) || (rest.compare(0U, 2U, "/*") == 0);
}

size_t CountConditionals(const std::vector<std::string> &lines) {
  size_t num = 0U;
  std::string name;
  std::string rest;
  for (const auto &line : lines) {
    if (SplitDirective(line, name, rest) && IsConditional(name)) {
      ++num;
    }
  }
  return num;
}

size_t SkipBlanks(const std::vector<std::string> &lines, size_t from) {
  size_t i = from;
  while ((i < lines.size()) && IsBlank(lines[i])) {
    ++i;
  }
  return i;
}

// 最后一个非空行的下标；全空时返回 lines.size() 作哨兵
size_t LastNonBlank(const std::vector<std::string> &lines) {
  size_t i = lines.size();
  while (i > 0U) {
    if (!IsBlank(lines[i - 1U])) {
      return i - 1U;
    }
    --i;
  }
  return lines.size();
}

std::vector<std::string> SplitLines(std::string_view source) {
  std::vector<std::string> lines;
  size_t begin = 0U;
  while (begin <= source.size()) {
    const size_t end = source.find('\n', begin);
    const size_t stop = (end == std::string_view::npos) ? source.size() : end;
    lines.emplace_back(source.substr(begin, stop - begin));
    if (end == std::string_view::npos) {
      break;
    }
    begin = end + 1U;
  }
  return lines;
}

// 头部版权块的结束下标（exclusive）；无版权块则返回 0，即从原处开始。
// 只在文件开头（允许前导空行）识别至多一个块注释，且必须含 Copyright 才剥。
// 正文中部的块注释结构上不可能被触及——内嵌单元正文中部的块注释（含 fz_c04 的 Doxygen
// 说明）都必须原样保留。
size_t SkipHeadCopyright(const std::vector<std::string> &lines) {
  const size_t first = SkipBlanks(lines, 0U);
  if (first >= lines.size()) {
    return 0U;
  }
  const size_t slash = lines[first].find("/*");
  if ((slash == std::string::npos) || (lines[first].find_first_not_of(" \t") != slash)) {
    return 0U;
  }
  bool has_copyright = false;
  size_t i = first;
  while (i < lines.size()) {
    if (lines[i].find("Copyright") != std::string::npos) {
      has_copyright = true;
    }
    if (lines[i].find("*/") != std::string::npos) {
      return has_copyright ? (i + 1U) : 0U;
    }
    ++i;
  }
  return 0U;  // 块注释未闭合，不剥
}
}  // namespace

bool IsInternalInclude(const std::string &line) {
  std::string name;
  std::string rest;
  if (!SplitDirective(line, name, rest) || (name != "include")) {
    return false;
  }
  return (!rest.empty()) && (rest[0] == '"');
}

bool AngleIncludeHeader(const std::string &line, std::string &header) {
  std::string name;
  std::string rest;
  if (!SplitDirective(line, name, rest) || (name != "include") || rest.empty() || (rest[0] != '<')) {
    return false;
  }
  const size_t close = rest.find('>');
  // 头名必须非空且 '>' 之后无其他内容：形如 "#include <a> <b>" 的行不认，
  // 宁可整行保留（多一处 include 站点）也不能剥掉半条而丢依赖。
  if ((close == std::string::npos) || (close < 2U)) {
    return false;
  }
  std::string tail = rest.substr(close + 1U);
  const size_t last = tail.find_last_not_of(" \t\r");
  tail = (last == std::string::npos) ? "" : tail.substr(0U, last + 1U);
  if (!tail.empty()) {
    return false;
  }
  header = rest.substr(1U, close - 1U);
  return !header.empty();
}

Status EmbedUnits(const std::map<std::string, std::string_view> &sources, const std::vector<std::string> &units,
                  const std::string &model_name, const std::string &log_tag, std::string &text,
                  std::set<std::string> &angle_includes) {
  for (const auto &unit : units) {
    const auto iter = sources.find(unit);
    if (iter == sources.end()) {
      GELOGE(FAILED, "[OM2][%s] Model %s needs embedded unit %s, but it is not in the resource table.", log_tag.c_str(),
             model_name.c_str(), unit.c_str());
      return FAILED;
    }
    text.append("/* ===== unit: " + unit + " ===== */\n");
    const auto lines = SplitLines(iter->second);
    size_t begin = SkipHeadCopyright(lines);
    size_t stop = lines.size();
    // include 守卫：产物是单个自包含 TU、每个单元只内嵌一次，守卫是死代码；
    // 且守卫宏名是仓库内部路径的投影（GE_RUNTIME_OM2_..._H_），不该出现在客户产物里。
    // 头尾必须同时匹配才剥，避免只剥一半留下孤立的 #endif。
    if (CountConditionals(lines) == kGuardDirectiveNum) {
      std::string guarded;
      std::string defined;
      const size_t ifndef_line = SkipBlanks(lines, begin);
      const size_t define_line = (ifndef_line < lines.size()) ? SkipBlanks(lines, ifndef_line + 1U) : lines.size();
      const size_t last = LastNonBlank(lines);
      if ((define_line < lines.size()) && MatchBareIdent(lines[ifndef_line], "ifndef", guarded) &&
          MatchBareIdent(lines[define_line], "define", defined) && (defined == guarded) && (last < lines.size()) &&
          IsEndif(lines[last])) {
        begin = define_line + 1U;
        stop = last;
      }
    }
    while ((begin < stop) && IsBlank(lines[begin])) {
      ++begin;
    }
    while ((stop > begin) && IsBlank(lines[stop - 1U])) {
      --stop;
    }
    for (size_t i = begin; i < stop; ++i) {
      // 两种形式的 include 都整行删除。quote 形式的符号由调用方另行供给；angle 形式的头名收进
      // angle_includes，由调用方在文件头统一发射并集。删除不影响条件编译块的配对：全部内嵌单元
      // 的 include 都在单元头部、剥掉守卫后无一条落在 #if 一族之内，也不在实质代码之后。
      if (IsInternalInclude(lines[i])) {
        continue;
      }
      std::string header;
      if (AngleIncludeHeader(lines[i], header)) {
        angle_includes.insert(header);
        continue;
      }
      text.append(lines[i]);
      text.push_back('\n');
    }
    text.push_back('\n');
  }
  return SUCCESS;
}
}  // namespace ge
