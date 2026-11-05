/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/codegen/file_code_generator/var_trans_file_code_generator.h"

#include <map>
#include <set>
#include <string_view>
#include <utility>

#include "common/ge_common/debug/ge_log.h"
#include "common/om2/codegen/source_embed_util.h"

namespace ge {
// 由 base/CMakeLists.txt 在 configure 阶段生成的资源文件提供：
// 47 个转换源文件的全文，键为相对仓库根的路径。
const std::map<std::string, std::string_view> &GetVarTransSources();

namespace {
constexpr char const *kFmtDir = "runtime/om2/formats/";
constexpr char const *kTransDir = "runtime/om2/formats/format_transfers/om2_format_transfer_";
constexpr char const *kCommonDir = "runtime/om2/common/";
// 单元 .cc 内 include 兄弟单元头时用的前缀（不含 runtime/om2/，与资源表的键不同）
constexpr char const *kUnitIncludePrefix = "formats/format_transfers/om2_format_transfer_";
constexpr char const *kRegisterMacro = "REGISTER_FORMAT_TRANSFER(";

// 注册表、定义头、工具与分派器。只要有转换就必须在，顺序即依赖序。
const std::vector<std::string> &FixedUnits() {
  static const std::vector<std::string> kUnits = {
      std::string(kFmtDir) + "om2_register_format_transfer.h",
      std::string(kFmtDir) + "om2_register_format_transfer.cc",
      std::string(kFmtDir) + "utils/om2_formats_definitions.h",
      std::string(kFmtDir) + "utils/om2_formats_trans_utils.h",
      std::string(kFmtDir) + "utils/om2_formats_trans_utils.cc",
      std::string(kFmtDir) + "om2_formats.h",
      std::string(kFmtDir) + "om2_formats.cc",
  };
  return kUnits;
}

// 格式转换单元名（不含 kTransDir 前缀与 .cc 后缀）。唯一顺序约束是 transpose 必须前插到
// fz_c04 之前——om2_format_transfer_fz_c04.cc 引用了 om2_format_transfer_transpose.h，
// 拼接后 include 行被剥离，故被调用的 gert::formats::Transpose 必须在调用点之前已物理出现。
const std::vector<std::string> &FormatUnitNames() {
  static const std::vector<std::string> kNames = {
      "c1hwncoc0_hwcn", "dhwcn_fractal_z_3D", "dhwnc_fractal_z_3D_transpose",
      "fractal_nz",     "fractal_z",          "fractal_z_tbe",
      "fractal_zz",     "fracz_hwcn",         "fracz_nchw",
      "fracz_nhwc",     "transpose",          "fz_c04",
      "hwcn_c1hwncoc0", "nc1hwc0_nchw",       "nc1hwc0_nhwc",
      "nchw_nc1hwc0",   "nhwc_nc1hwc0",
  };
  return kNames;
}

// 数据类型转换单元。文本顺序须与依赖方向相反，
// 因为 om2_hif8_t.h 依赖 om2_fp16_t.h，om2_datatype_transfer.cc 依赖前两者。
const std::vector<std::string> &DataTypeUnits() {
  static const std::vector<std::string> kUnits = {
      std::string(kCommonDir) + "fp16_t/om2_fp16_t.h",
      std::string(kCommonDir) + "fp16_t/om2_fp16_t.cc",
      std::string(kCommonDir) + "math/om2_hif8_t.h",
      std::string(kCommonDir) + "math/om2_hif8_t.cc",
      std::string(kCommonDir) + "datatype_transfer/om2_datatype_transfer.h",
      std::string(kCommonDir) + "datatype_transfer/om2_datatype_transfer.cc",
  };
  return kUnits;
}

// 注册宏实参里出现的格式 token → 枚举。值直接引用枚举常量本身，枚举改值不会静默失配；
// 唯一失效模式是新增注册用了本表没有的 token，由 BuildPairIndex 的 GELOGE 暴露。
// 15 项覆盖当前 42 条 REGISTER_FORMAT_TRANSFER 用到的全部 token。
const std::map<std::string, Format> &FormatByToken() {
  static const std::map<std::string, Format> kMap = {
      {"FORMAT_NCHW", FORMAT_NCHW},
      {"FORMAT_NHWC", FORMAT_NHWC},
      {"FORMAT_ND", FORMAT_ND},
      {"FORMAT_NC1HWC0", FORMAT_NC1HWC0},
      {"FORMAT_FRACTAL_Z", FORMAT_FRACTAL_Z},
      {"FORMAT_FRACTAL_Z_C04", FORMAT_FRACTAL_Z_C04},
      {"FORMAT_CHWN", FORMAT_CHWN},
      {"FORMAT_HWCN", FORMAT_HWCN},
      {"FORMAT_C1HWNCoC0", FORMAT_C1HWNCoC0},
      {"FORMAT_FRACTAL_ZZ", FORMAT_FRACTAL_ZZ},
      {"FORMAT_FRACTAL_NZ", FORMAT_FRACTAL_NZ},
      {"FORMAT_DHWCN", FORMAT_DHWCN},
      {"FORMAT_DHWNC", FORMAT_DHWNC},
      {"FORMAT_FRACTAL_Z_3D", FORMAT_FRACTAL_Z_3D},
      {"FORMAT_FRACTAL_Z_3D_TRANSPOSE", FORMAT_FRACTAL_Z_3D_TRANSPOSE},
  };
  return kMap;
}

// 注册表的键。BuildFormatTransfer（runtime/om2/formats/om2_register_format_transfer.cc）
// 按主格式对查表，与固化表里的全格式值之间隔着一层 GetPrimaryFormat 掩码，故两侧都必须先归一。
using FormatPair = std::pair<uint32_t, uint32_t>;

uint32_t PrimaryOf(const uint32_t format) {
  return static_cast<uint32_t>(GetPrimaryFormat(static_cast<int32_t>(format)));
}

std::string_view Trim(const std::string_view text) {
  size_t begin = 0U;
  size_t end = text.size();
  while ((begin < end) && ((text[begin] == ' ') || (text[begin] == '\t'))) {
    ++begin;
  }
  while ((end > begin) && ((text[end - 1U] == ' ') || (text[end - 1U] == '\t'))) {
    --end;
  }
  return text.substr(begin, end - begin);
}

// 从 pos（指向 '(' 之后的第一个字符）解析以 ')' 结束、',' 分隔的实参列表，
// 返回各实参（已去首尾空白），并把 pos 推进到 ')' 之后。
// 实参都是简单标识符，不含嵌套括号或字符串字面量，故不做配对扫描。
std::vector<std::string> ParseMacroArgs(const std::string_view text, size_t &pos) {
  std::vector<std::string> args;
  size_t begin = pos;
  while (pos < text.size()) {
    const char c = text[pos];
    if ((c != ',') && (c != ')')) {
      ++pos;
      continue;
    }
    args.emplace_back(Trim(text.substr(begin, pos - begin)));
    ++pos;
    begin = pos;
    if (c == ')') {
      return args;
    }
  }
  return args;
}

bool TokenToPrimary(const std::string &token, const std::string &unit, uint32_t &primary) {
  const auto iter = FormatByToken().find(token);
  if (iter == FormatByToken().end()) {
    GELOGE(FAILED, "[OM2][VarTrans] Format token %s registered by unit %s is unknown to FormatByToken.", token.c_str(),
           unit.c_str());
    return false;
  }
  primary = PrimaryOf(static_cast<uint32_t>(iter->second));
  return true;
}

// 扫内嵌源码建「主格式对 → 单元名」索引，不硬编码 42 对：
// 新增 REGISTER_FORMAT_TRANSFER 自动生效，不存在与注册宏静默漂移的可能。
std::map<FormatPair, std::string> BuildPairIndex() {
  std::map<FormatPair, std::string> index;
  const auto &sources = GetVarTransSources();
  for (const auto &name : FormatUnitNames()) {
    const auto iter = sources.find(std::string(kTransDir) + name + ".cc");
    if (iter == sources.end()) {
      GELOGE(FAILED, "[OM2][VarTrans] Embedded source of format unit %s not found.", name.c_str());
      continue;
    }
    const std::string_view text = iter->second;
    const std::string_view macro(kRegisterMacro);
    size_t pos = 0U;
    while ((pos = text.find(macro, pos)) != std::string_view::npos) {
      pos += macro.size();
      const auto args = ParseMacroArgs(text, pos);
      if (args.size() < 3U) {  // 宏定义行本身只有形参名，此处只可能是源码损坏
        GELOGE(FAILED, "[OM2][VarTrans] Malformed REGISTER_FORMAT_TRANSFER in unit %s.", name.c_str());
        continue;
      }
      uint32_t src = 0U;
      uint32_t dst = 0U;
      if (!TokenToPrimary(args[1], name, src) || !TokenToPrimary(args[2], name, dst)) {
        continue;
      }
      const auto inserted = index.emplace(FormatPair{src, dst}, name);
      if (!inserted.second && (inserted.first->second != name)) {
        GELOGE(FAILED, "[OM2][VarTrans] Format pair (%u -> %u) is registered by both %s and %s.", src, dst,
               inserted.first->second.c_str(), name.c_str());
      }
    }
  }
  return index;
}

const std::map<FormatPair, std::string> &PairToUnit() {
  static const std::map<FormatPair, std::string> kIndex = BuildPairIndex();
  return kIndex;
}

// 扫单元 .cc 对兄弟单元头的 include，作为编译期依赖。om2_format_transfer_fz_c04.cc
// include 了 transpose 的头；该依赖同时覆盖 fz_c04 运行期经注册表回查 HWCN -> NCHW 的
// 路径（transpose 注册了该对），故闭包完备。
std::map<std::string, std::set<std::string>> BuildUnitDeps() {
  std::map<std::string, std::set<std::string>> deps;
  const auto &sources = GetVarTransSources();
  const std::string_view prefix(kUnitIncludePrefix);
  for (const auto &name : FormatUnitNames()) {
    const auto iter = sources.find(std::string(kTransDir) + name + ".cc");
    if (iter == sources.end()) {
      continue;  // BuildPairIndex 已报错，此处不重复
    }
    const std::string_view text = iter->second;
    size_t pos = 0U;
    while ((pos = text.find(prefix, pos)) != std::string_view::npos) {
      pos += prefix.size();
      const size_t end = text.find(".h\"", pos);
      if (end == std::string_view::npos) {
        break;
      }
      const std::string dep(text.substr(pos, end - pos));
      if (dep != name) {
        deps[name].insert(dep);
      }
      pos = end;
    }
  }
  return deps;
}

const std::map<std::string, std::set<std::string>> &UnitDeps() {
  static const std::map<std::string, std::set<std::string>> kDeps = BuildUnitDeps();
  return kDeps;
}
}  // namespace

bool VarTransFileCodeGenerator::NeedFormatCapability() const {
  for (const auto &entry : table_.entries) {
    for (const auto &node : entry.trans_road) {
      if (node.kind == VarTransKind::kFormat) {
        return true;
      }
    }
  }
  return false;
}

bool VarTransFileCodeGenerator::NeedDataTypeCapability() const {
  for (const auto &entry : table_.entries) {
    for (const auto &node : entry.trans_road) {
      if (node.kind == VarTransKind::kCast) {
        return true;
      }
    }
    if (!entry.copy_src_var_key.empty() && (entry.copy_src_data_type != entry.data_type)) {
      return true;
    }
  }
  return false;
}

Status VarTransFileCodeGenerator::CollectFormatUnits(std::set<std::string> &needed) const {
  const auto &index = PairToUnit();
  for (const auto &entry : table_.entries) {
    for (const auto &node : entry.trans_road) {
      if (node.kind != VarTransKind::kFormat) {
        continue;
      }
      const FormatPair key{PrimaryOf(node.src_format), PrimaryOf(node.dst_format)};
      const auto iter = index.find(key);
      if (iter == index.end()) {
        // 编译期即拦下：variable_op_pass.cc 的 IsTransSupport 经 formats::IsTransFormatSupport 查
        // base/formats 注册表，此处内嵌的是 runtime/om2/formats 的副本。两份注册表是各自独立的
        // 副本，必须保持同样的格式对集合，否则编译期放行的转换在此处找不到实现单元。触发本分支
        // 有两类来源：两份副本漂移，或调用方绕过 IsTransSupport 直接 SetTransRoad 注入不支持的对。
        GELOGE(FAILED,
               "[OM2][VarTrans] Model %s var %s needs format trans (%u -> %u), but no embedded unit registers it.",
               artifact_name_.c_str(), entry.var_key.c_str(), key.first, key.second);
        return FAILED;
      }
      needed.insert(iter->second);
    }
  }
  // 依赖闭包：被选中单元 include 的兄弟单元必须一并内嵌，否则拼接后符号缺失
  const auto &deps = UnitDeps();
  std::vector<std::string> pending(needed.begin(), needed.end());
  while (!pending.empty()) {
    const std::string unit = pending.back();
    pending.pop_back();
    const auto iter = deps.find(unit);
    if (iter == deps.end()) {
      continue;
    }
    for (const auto &dep : iter->second) {
      if (needed.insert(dep).second) {
        pending.emplace_back(dep);
      }
    }
  }
  return SUCCESS;
}

Status VarTransFileCodeGenerator::SelectUnits(std::vector<std::string> &units) const {
  const bool need_format = NeedFormatCapability();
  const bool need_data_type = NeedDataTypeCapability();
  // FixedUnits 是注册表、定义头、工具与分派器，只被格式/数据类型转换单元与两个入口的
  // 真实体消费。两者都不需要时不内嵌，否则无转换需求的模型要白背这批文本与分派器的 .text。
  if (need_format || need_data_type) {
    units = FixedUnits();
  }
  if (need_format) {
    std::set<std::string> needed;
    GE_CHK_STATUS_RET_NOLOG(CollectFormatUnits(needed));
    // 按 FormatUnitNames() 原序过滤而非按 needed 的字典序：该列表已把 transpose 排在
    // fz_c04 之前，而 om2_format_transfer_fz_c04.cc 引用了 om2_format_transfer_transpose.h，
    // 拼接后 include 行被剥离，故被调用的 gert::formats::Transpose 必须在调用点之前已物理出现。
    for (const auto &name : FormatUnitNames()) {
      if (needed.count(name) == 0U) {
        continue;
      }
      units.emplace_back(std::string(kTransDir) + name + ".h");
      units.emplace_back(std::string(kTransDir) + name + ".cc");
    }
  }
  if (need_data_type) {
    for (const auto &unit : DataTypeUnits()) {
      units.emplace_back(unit);
    }
  }
  return SUCCESS;
}

Status VarTransFileCodeGenerator::BuildUnitsText(const std::vector<std::string> &units, std::string &text,
                                                 std::set<std::string> &angle_includes) const {
  // 被剥掉的 quote include，其内容在本文件由三条途径供给：产物内更早位置已文本内联
  // （FixedUnits、转换单元与兄弟单元）、kVarTransAdapter 复刻（TypeUtilsInner / UNSUPPORTED /
  // MakeShared / 2 个宏）、BuildIncludes 显式补回（graph/types.h、graph/def_types.h 等）。
  // 被剥掉的 angle include 收进 angle_includes，由 BuildIncludes 在文件头统一发射并集。
  return EmbedUnits(GetVarTransSources(), units, artifact_name_, "VarTrans", text, angle_includes);
}

ExprRef VarTransFileCodeGenerator::PrimaryFormatOf(const VarRef &format) const {
  return ast_.StaticCast("ge::Format", ast_.Call("ge::GetPrimaryFormat", {ast_.StaticCast("int32_t", format)}));
}

ExprRef VarTransFileCodeGenerator::SubFormatOf(const VarRef &format) const {
  return ast_.StaticCast("ge::Format", ast_.Call("ge::GetSubFormat", {ast_.StaticCast("int32_t", format)}));
}

ExprRef VarTransFileCodeGenerator::C0ValueOf(const VarRef &format) const {
  return ast_.Call("ge::GetC0Value", {ast_.StaticCast("int32_t", format)});
}

FunctionDef *VarTransFileCodeGenerator::BuildTransFormatEntry(bool need_format) const {
  auto src_data = ast_.Var("const uint8_t *", "src_data");
  auto src_format = ast_.Var("uint32_t", "src_format");
  auto dst_format = ast_.Var("uint32_t", "dst_format");
  auto src_data_type = ast_.Var("uint32_t", "src_data_type");
  auto src_shape = ast_.Var("const std::vector<int64_t> &", "src_shape");
  auto dst_shape = ast_.Var("const std::vector<int64_t> &", "dst_shape");
  auto out_data = ast_.Var("std::shared_ptr<uint8_t> &", "out_data");
  auto out_len = ast_.Var("size_t &", "out_len");
  const std::vector<VarRef> params = {src_data,  src_format, dst_format, src_data_type,
                                      src_shape, dst_shape,  out_data,   out_len};
  if (!need_format) {
    // FixedUnits 与格式转换单元未内嵌 ⇒ gert::formats::TransDataFormat 不存在，只能发桩。
    // 签名必须与真实体一致：var_mgr 的 TransAllVarData 恒定调用两个入口，不随能力裁剪。
    // 该桩不可达——kFormat 节点存在当且仅当 NeedFormatCapability() 为真。
    return ast_.DefineFunction("TransFormat", params, "ge::Status",
                               ast_.Body({ast_.Comment("本模型无格式转换需求，转换单元未内嵌，此入口不应被调用"),
                                          ast_.Return("UNSUPPORTED")}));
  }
  auto args = ast_.Var("gert::formats::TransArgs", "args");
  auto result = ast_.Var("gert::formats::TransResult", "result");
  auto ret = ast_.Var("const ge::Status", "ret");
  const auto src_dt = ast_.StaticCast("ge::DataType", src_data_type);
  // 格式的 6 个分解值在 SO 内直接算：ge::GetPrimaryFormat / GetSubFormat / GetC0Value
  // 都在 graph/types.h 的 namespace ge 内，该头经既有 -I$(CANN_ROOT)/include 可达。
  return ast_.DefineFunction("TransFormat", params, "ge::Status",
                             ast_.Body({
                                 ast_.VarDecl(args),
                                 ast_.Assign(args.Attr("data"), src_data),
                                 ast_.Assign(args.Attr("src_format"), ast_.StaticCast("ge::Format", src_format)),
                                 ast_.Assign(args.Attr("dst_format"), ast_.StaticCast("ge::Format", dst_format)),
                                 ast_.Assign(args.Attr("src_primary_format"), PrimaryFormatOf(src_format)),
                                 ast_.Assign(args.Attr("dst_primary_format"), PrimaryFormatOf(dst_format)),
                                 ast_.Assign(args.Attr("src_sub_format"), SubFormatOf(src_format)),
                                 ast_.Assign(args.Attr("dst_sub_format"), SubFormatOf(dst_format)),
                                 ast_.Assign(args.Attr("src_c0_format"), C0ValueOf(src_format)),
                                 ast_.Assign(args.Attr("dst_c0_format"), C0ValueOf(dst_format)),
                                 ast_.Assign(args.Attr("src_shape"), src_shape),
                                 ast_.Assign(args.Attr("dst_shape"), dst_shape),
                                 ast_.Assign(args.Attr("src_data_type"), src_dt),
                                 ast_.VarDecl(result),
                                 ast_.VarDecl(ret, ast_.Call("gert::formats::TransDataFormat", {args, result})),
                                 ast_.If(ret != "ge::SUCCESS", {ast_.Return(ret)}),
                                 ast_.Assign(out_data, result.Attr("data")),
                                 ast_.Assign(out_len, result.Attr("length")),
                                 ast_.Return("ge::SUCCESS"),
                             }));
}

FunctionDef *VarTransFileCodeGenerator::BuildTransDataTypeEntry(bool need_data_type) const {
  auto src_data = ast_.Var("const uint8_t *", "src_data");
  auto src_size = ast_.Var("size_t", "src_size");
  auto src_data_type = ast_.Var("uint32_t", "src_data_type");
  auto dst_data_type = ast_.Var("uint32_t", "dst_data_type");
  auto out_data = ast_.Var("std::shared_ptr<uint8_t> &", "out_data");
  auto out_len = ast_.Var("size_t &", "out_len");
  const std::vector<VarRef> params = {src_data, src_size, src_data_type, dst_data_type, out_data, out_len};
  if (!need_data_type) {
    // 数据类型转换单元未内嵌 ⇒ gert::formats::TransTensorDataType 不存在，只能发桩。同上，不可达。
    return ast_.DefineFunction("TransDataType", params, "ge::Status",
                               ast_.Body({ast_.Comment("本模型无数据类型转换需求，转换单元未内嵌，此入口不应被调用"),
                                          ast_.Return("UNSUPPORTED")}));
  }
  auto args = ast_.Var("gert::formats::CastArgs", "args");
  auto result = ast_.Var("gert::formats::TransResult", "result");
  auto ret = ast_.Var("const ge::Status", "ret");
  const auto src_dt = ast_.StaticCast("ge::DataType", src_data_type);
  const auto dst_dt = ast_.StaticCast("ge::DataType", dst_data_type);
  return ast_.DefineFunction("TransDataType", params, "ge::Status",
                             ast_.Body({
                                 ast_.VarDecl(args),
                                 ast_.Assign(args.Attr("data"), src_data),
                                 ast_.Assign(args.Attr("src_data_size"), src_size),
                                 ast_.Assign(args.Attr("src_data_type"), src_dt),
                                 ast_.Assign(args.Attr("dst_data_type"), dst_dt),
                                 ast_.VarDecl(result),
                                 ast_.VarDecl(ret, ast_.Call("gert::formats::TransTensorDataType", {args, result})),
                                 ast_.If(ret != "ge::SUCCESS", {ast_.Return(ret)}),
                                 ast_.Assign(out_data, result.Attr("data")),
                                 ast_.Assign(out_len, result.Attr("length")),
                                 ast_.Return("ge::SUCCESS"),
                             }));
}

Status VarTransFileCodeGenerator::BuildSegments(std::vector<DeclNode *> &items,
                                                std::set<std::string> &angle_includes) const {
  std::vector<std::string> units;
  GE_CHK_STATUS_RET_NOLOG(SelectUnits(units));
  const bool need_format = NeedFormatCapability();
  const bool need_data_type = NeedDataTypeCapability();
  // include 不在此发射：并入 <model>_var_mgr.cpp 后由调用方统一发射并集，避免同一 TU 内重复。
  // 适配头必须先于内嵌单元——单元正文用到它复刻的 GE_CHECK_GE / MakeShared / TypeUtilsInner。
  items.push_back(ast_.StablePart(StablePartId::kVarTransAdapter));
  items.push_back(ast_.Space());
  std::string units_text;
  GE_CHK_STATUS_RET_NOLOG(BuildUnitsText(units, units_text, angle_includes));
  items.push_back(ast_.RawText(units_text));
  // 两个入口不需要独立 #line 归位：EmitFile 的文件级 #line 1 "<model>_var_mgr.cpp"
  // 统管全文，行号天然是产物内的真实行号。
  // 保留一条边界注释，免得读者把入口代码误当成最后一个内联单元的一部分。
  items.push_back(ast_.RawText("/* ===== 内部转换入口（本文件生成，非内联单元） ===== */\n"));
  items.push_back(ast_.Namespace("gert", {ast_.Namespace("var_trans", {BuildTransFormatEntry(need_format),
                                                                       BuildTransDataTypeEntry(need_data_type)})}));
  GELOGI("[OM2][VarTrans] Model %s var_trans segments built, %zu units embedded, format=%d data_type=%d.",
         artifact_name_.c_str(), units.size(), static_cast<int>(need_format), static_cast<int>(need_data_type));
  return SUCCESS;
}
}  // namespace ge
