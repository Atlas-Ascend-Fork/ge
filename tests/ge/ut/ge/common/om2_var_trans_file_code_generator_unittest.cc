/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>

#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "common/om2/codegen/file_code_generator/var_trans_file_code_generator.h"
#include "common/om2/codegen/om2_codegen_types.h"

namespace ge {
namespace {
// 每个内联单元前的边界注释前缀。用边界注释而非 per-unit #line 的理由见 source_embed_util.h；
// 全文行号由 EmitFile 的文件级 #line 1 "<model>_var_mgr.cpp" 统管。
constexpr char const *kUnitMark = "/* ===== unit: ";

VarFixupEntry MakeEntry(const std::string &var_key) {
  VarFixupEntry entry;
  entry.slot = 0U;
  entry.var_key = var_key;
  entry.size = 96U;
  entry.memory_type = 2U;  // RT_MEMORY_HBM
  entry.data_type = 0U;    // DT_FLOAT
  entry.logic_addr = 0xC000000000ULL;
  return entry;
}

VarTransNodeFixup MakeTransNode(const VarTransKind kind) {
  VarTransNodeFixup node;
  node.kind = kind;
  // 取枚举而非字面量：FORMAT_NHWC 是 1、FORMAT_ND 才是 2，相邻取值极易写反
  node.src_format = static_cast<uint32_t>(FORMAT_NCHW);
  node.dst_format = static_cast<uint32_t>(FORMAT_NHWC);
  node.src_data_type = 0U;
  node.dst_data_type = 0U;
  node.src_dims = {1, 2, 3, 4};
  node.dst_dims = {1, 3, 4, 2};
  return node;
}

// 指定主格式对的格式转换节点。格式转换单元按转换链里出现的格式对选择，格式对决定内嵌哪些单元。
VarTransNodeFixup MakeFormatNode(const Format src, const Format dst) {
  auto node = MakeTransNode(VarTransKind::kFormat);
  node.src_format = static_cast<uint32_t>(src);
  node.dst_format = static_cast<uint32_t>(dst);
  return node;
}

size_t CountSubstr(const std::string &text, const std::string &sub) {
  size_t count = 0U;
  size_t pos = text.find(sub);
  while (pos != std::string::npos) {
    ++count;
    pos = text.find(sub, pos + sub.size());
  }
  return count;
}

// 内嵌的格式转换单元文件数（.h 与 .cc 各算一个）
size_t CountFormatUnitFiles(const std::string &text) {
  return CountSubstr(text, std::string(kUnitMark) + "runtime/om2/formats/format_transfers/");
}

// 产物内联单元总数（FixedUnits、格式转换单元、数据类型转换单元都算；
// gert::var_trans 两个入口的边界注释前缀不同，故不计入）
size_t CountUnits(const std::string &text) {
  return CountSubstr(text, kUnitMark);
}

// 某内联单元的边界注释位置，即它在产物中的物理位置。
size_t UnitPos(const std::string &text, const std::string &unit_path) {
  return text.find(std::string(kUnitMark) + unit_path);
}

// AstContext 是节点的竞技场，必须活到发射结束；AstBuildContext 不可拷贝，故由 fixture 持有。
// 成员声明次序即初始化次序：ctx 在前，ast{ctx} 才有效。
class VarTransFileCodeGeneratorTest : public testing::Test {
 protected:
  AstContext ctx;
  AstBuildContext ast{ctx};

  // BuildSegments 成功时返回发射后的转换部分文本。调用点只关心文本内容，逐个写
  // vector<DeclNode*> + File + CppEmitter + 两次 ASSERT_EQ 会淹没断言主体；本类只产出片段、
  // 不产出 File 节点，故此处自行包一层。include 由 var_mgr 侧统一发射，不在本文本内。
  std::string GenerateOk(const VarTransFileCodeGenerator &generator) {
    std::vector<DeclNode *> items;
    std::set<std::string> angle_includes;
    EXPECT_EQ(generator.BuildSegments(items, angle_includes), SUCCESS);
    if (items.empty()) {
      return "";
    }
    DeclNode *file = ast.File(items);
    if (file == nullptr) {
      return "";
    }
    CppEmitter emitter;
    std::string code;
    EXPECT_EQ(file->Accept(emitter, code), SUCCESS);
    return code;
  }
};

// 有变量但无真实转换需求：转换部分仍要产出（同一 TU 内 var_mgr 的 TransAllVarData 恒定
// 引用两个入口），但 FixedUnits 与两类转换单元一个都不内嵌，两个入口都退化为桩。
TEST_F(VarTransFileCodeGeneratorTest, NoOpRoadGeneratesStubOnlySegments) {
  VarFixupTable table;
  table.graph_id = 11U;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kNoOp));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(ge::HasVariables(table));
  const auto code = GenerateOk(generator);
  // FixedUnits 也不内嵌：注册表与分派器只被转换单元与入口真实体消费，无转换需求时是纯冗余
  EXPECT_EQ(code.find("om2_register_format_transfer.h"), std::string::npos);
  EXPECT_EQ(code.find("om2_formats.cc"), std::string::npos);
  EXPECT_EQ(code.find("om2_datatype_transfer.cc"), std::string::npos);
  EXPECT_EQ(CountFormatUnitFiles(code), 0U);
  // 两个入口都在，但都是桩：签名必须与真实体一致，否则 TransAllVarData 的调用点对不上
  EXPECT_NE(code.find("ge::Status TransFormat("), std::string::npos);
  EXPECT_NE(code.find("ge::Status TransDataType("), std::string::npos);
  EXPECT_EQ(CountSubstr(code, "return UNSUPPORTED;"), 2U);
  EXPECT_EQ(code.find("gert::formats::TransDataFormat(args, result)"), std::string::npos);
  EXPECT_EQ(code.find("gert::formats::TransTensorDataType(args, result)"), std::string::npos);
  // 适配头仍要在：桩体用的 UNSUPPORTED 正是它复刻的
  EXPECT_NE(code.find("static_cast<ge::Status>(0x50100064U)"), std::string::npos);
}

// 转换链里有格式转换节点 ⇒ 内联格式转换单元并生成 TransFormat 入口，不内联数据类型转换单元
TEST_F(VarTransFileCodeGeneratorTest, FormatNodeSelectsFormatUnitsOnly) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(ge::HasVariables(table));

  const auto code = GenerateOk(generator);
  // FixedUnits 固定部件
  EXPECT_NE(code.find("om2_register_format_transfer.h"), std::string::npos);
  EXPECT_NE(code.find("om2_formats.cc"), std::string::npos);
  // 格式转换单元已内联：NCHW -> NHWC 由 transpose 注册
  EXPECT_NE(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  // 数据类型转换单元未内联：转换链里无 CAST 节点、拷贝两侧类型也相同
  EXPECT_EQ(code.find("om2_datatype_transfer.cc"), std::string::npos);
  // 两个入口恒定发射（var_mgr 的 TransAllVarData 恒定调用），差异在体内引用哪个分派器：
  // 无数据类型转换能力时 TransDataType 是桩，不引用 gert::formats::TransTensorDataType
  EXPECT_NE(code.find("gert::formats::TransDataFormat(args, result)"), std::string::npos);
  EXPECT_EQ(code.find("gert::formats::TransTensorDataType(args, result)"), std::string::npos);
}

// 格式转换单元按转换链里出现的格式对选择，不全量内嵌：
// NCHW -> NC1HWC0 只由 nchw_nc1hwc0 注册，其余单元一个都不该进来
TEST_F(VarTransFileCodeGeneratorTest, SelectsOnlyUnitRegisteringNeededPair) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeFormatNode(FORMAT_NCHW, FORMAT_NC1HWC0));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_format_transfer_nchw_nc1hwc0.cc"), std::string::npos);
  EXPECT_EQ(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  EXPECT_EQ(code.find("om2_format_transfer_fractal_z.cc"), std::string::npos);
  EXPECT_EQ(code.find("om2_format_transfer_fz_c04.cc"), std::string::npos);
  // .h 与 .cc 各一个，共 2 个文件；全量内嵌会是 34
  EXPECT_EQ(CountFormatUnitFiles(code), 2U);
}

// 依赖闭包：om2_format_transfer_fz_c04.cc include 了 transpose 的头并直调
// gert::formats::Transpose，其运行期还会经注册表回查 HWCN -> NCHW，
// 该对同样由 transpose 注册 ⇒ 选中 fz_c04 必须连带选中 transpose
TEST_F(VarTransFileCodeGeneratorTest, ClosurePullsTransposeForFzC04) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeFormatNode(FORMAT_HWCN, FORMAT_FRACTAL_Z_C04));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_format_transfer_fz_c04.cc"), std::string::npos);
  EXPECT_NE(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  EXPECT_EQ(CountFormatUnitFiles(code), 4U);
}

// 固化表里的格式值可能带 sub-format 位，选择时必须先经 GetPrimaryFormat 归一，
// 否则查表落空、一个格式转换单元都选不出来
TEST_F(VarTransFileCodeGeneratorTest, SubFormatBitsAreMaskedWhenSelecting) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  auto node = MakeFormatNode(FORMAT_NCHW, FORMAT_NHWC);
  node.src_format = static_cast<uint32_t>(GetFormatFromSub(FORMAT_NCHW, 7));
  node.dst_format = static_cast<uint32_t>(GetFormatFromSub(FORMAT_NHWC, 3));
  entry.trans_road.push_back(node);
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  EXPECT_EQ(CountFormatUnitFiles(code), 2U);
}

// 格式对跨 entry、跨节点取并集
TEST_F(VarTransFileCodeGeneratorTest, UnionsPairsAcrossEntriesAndNodes) {
  VarFixupTable table;
  auto first = MakeEntry("w1_0");
  first.trans_road.push_back(MakeFormatNode(FORMAT_NCHW, FORMAT_NC1HWC0));
  auto second = MakeEntry("w2_0");
  second.trans_road.push_back(MakeTransNode(VarTransKind::kNoOp));
  second.trans_road.push_back(MakeFormatNode(FORMAT_NHWC, FORMAT_NC1HWC0));
  table.entries.push_back(first);
  table.entries.push_back(second);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_format_transfer_nchw_nc1hwc0.cc"), std::string::npos);
  EXPECT_NE(code.find("om2_format_transfer_nhwc_nc1hwc0.cc"), std::string::npos);
  EXPECT_EQ(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  EXPECT_EQ(CountFormatUnitFiles(code), 4U);
}

// 转换链里出现无单元注册的格式对 ⇒ 编译期失败，不留到加载期报
// ACL_ERROR_GE_FORMAT_INVALID。base/formats 与 runtime/om2/formats 两份注册表
// 漂移时就会走到这条路径。
TEST_F(VarTransFileCodeGeneratorTest, UnregisteredPairFailsGenerate) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  auto node = MakeFormatNode(FORMAT_NCHW, FORMAT_NHWC);
  node.src_format = 200U;  // 无任何单元注册的格式对
  node.dst_format = 201U;
  entry.trans_road.push_back(node);
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(ge::HasVariables(table));
  // 失败时不得留下半截段：SelectUnits 先失败即返回，items 保持为空，
  // 调用方（VarMgrFileCodeGenerator::BuildFile）据此不会拼出缺码的产物。
  std::vector<DeclNode *> items;
  std::set<std::string> angle_includes;
  EXPECT_NE(generator.BuildSegments(items, angle_includes), SUCCESS);
  EXPECT_TRUE(items.empty());
  EXPECT_TRUE(angle_includes.empty());
}

// 转换链里有 CAST 节点 ⇒ 内联数据类型转换单元并生成 TransDataType 入口，不内联格式转换单元
TEST_F(VarTransFileCodeGeneratorTest, CastNodeSelectsDataTypeUnitsOnly) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kCast));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(ge::HasVariables(table));

  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_datatype_transfer.cc"), std::string::npos);
  EXPECT_NE(code.find("om2_fp16_t.cc"), std::string::npos);
  // 格式转换单元一个都不内联：转换链里没有格式转换节点
  EXPECT_EQ(CountFormatUnitFiles(code), 0U);
  EXPECT_NE(code.find("gert::formats::TransTensorDataType(args, result)"), std::string::npos);
  EXPECT_EQ(code.find("gert::formats::TransDataFormat(args, result)"), std::string::npos);
}

// 跨类型拷贝（无 CAST 节点）也需要数据类型转换能力
TEST_F(VarTransFileCodeGeneratorTest, CopyAcrossDataTypeNeedsDataTypeCapability) {
  VarFixupTable table;
  auto entry = MakeEntry("dst_0");
  entry.data_type = 0U;  // DT_FLOAT
  entry.copy_src_var_key = "src_0";
  entry.copy_src_data_type = 1U;  // DT_FLOAT16，与目标不同
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(ge::HasVariables(table));
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("gert::formats::TransTensorDataType(args, result)"), std::string::npos);
  EXPECT_EQ(code.find("gert::formats::TransDataFormat(args, result)"), std::string::npos);
}

// 格式转换与数据类型转换两种能力同时需要 ⇒ 两类单元都内联、两个入口都生成
TEST_F(VarTransFileCodeGeneratorTest, BothCapabilitiesSelectBothSegments) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kCast));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("om2_format_transfer_transpose.cc"), std::string::npos);
  EXPECT_NE(code.find("om2_datatype_transfer.cc"), std::string::npos);
  EXPECT_NE(code.find("gert::formats::TransDataFormat(args, result)"), std::string::npos);
  EXPECT_NE(code.find("gert::formats::TransTensorDataType(args, result)"), std::string::npos);
  EXPECT_EQ(code.find("return UNSUPPORTED;"), std::string::npos);
  // FixedUnits 7 + 格式转换单元 2（transpose）+ 数据类型转换单元 6 = 15 个内联单元
  EXPECT_EQ(CountUnits(code), 15U);
}

// 转换部分自身不发射任何 include：它与变量准备同处 <model>_var_mgr.cpp 一个 TU，include 并集
// 由 var_mgr 侧统一发射一遍。本用例守住两件事：① 片段内零 include（任何形式），不产生重复
// 站点；② 内联源码的 quote include 仍被整行剥除（否则单元正文会把工具包里不存在的路径带进
// 产物），angle include 也被剥除并收集回传（否则每个单元各带一份、产物出现十几处站点）。
// include 由 var_mgr 侧发射，故存在性与次序断言在 om2_var_mgr_file_code_generator_unittest.cc。
TEST_F(VarTransFileCodeGeneratorTest, SegmentsCarryNoIncludesAndStripSourceIncludes) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // 确有内联单元，才能证明"零 include"是剥除的结果而非根本没内嵌
  ASSERT_GT(CountFormatUnitFiles(code), 0U);
  EXPECT_EQ(CountSubstr(code, "#include \""), 0U);
  EXPECT_EQ(CountSubstr(code, "#include <"), 0U);
  // om2_model_api.h 只能经 _internal.h 传递取得，任何形式都不得出现在段内
  EXPECT_EQ(code.find("om2_model_api.h"), std::string::npos);
}

// 适配头必须复刻 3 个 SO 可见路径内不存在的符号，否则拼接产物报 27 处未声明
TEST_F(VarTransFileCodeGeneratorTest, AdapterReplicatesSoInvisibleSymbols) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("#define GE_CHECK_GE("), std::string::npos);
  EXPECT_NE(code.find("#define GE_CHECK_NOTNULL("), std::string::npos);
  // 数值须与 fmk_error_codes.h 的 DECLARE_ERRORNO_COMMON(UNSUPPORTED, 100) 一致
  EXPECT_NE(code.find("static_cast<ge::Status>(0x50100064U)"), std::string::npos);
  EXPECT_NE(code.find("std::shared_ptr<T> MakeShared("), std::string::npos);
}

// 格式转换单元的唯一顺序约束：transpose 必须前插到 fz_c04 之前，否则拼接后调用点先于定义出现。
// transpose 只在被选中时才内嵌，故顺序约束由"按 FormatUnitNames()
// 原序过滤"保证——该列表已把 transpose 排在 fz_c04 之前。
TEST_F(VarTransFileCodeGeneratorTest, TransposeUnitPrecedesFzC04) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeFormatNode(FORMAT_HWCN, FORMAT_FRACTAL_Z_C04));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  const auto transpose_pos = UnitPos(code, "runtime/om2/formats/format_transfers/om2_format_transfer_transpose.h");
  const auto fz_c04_pos = UnitPos(code, "runtime/om2/formats/format_transfers/om2_format_transfer_fz_c04.h");
  ASSERT_NE(transpose_pos, std::string::npos);
  ASSERT_NE(fz_c04_pos, std::string::npos);
  EXPECT_LT(transpose_pos, fz_c04_pos);
}

// 数据类型转换单元的文本顺序须与依赖方向相反：fp16_t → hif8_t → datatype_transfer
TEST_F(VarTransFileCodeGeneratorTest, DataTypeUnitsFollowReverseDependencyOrder) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kCast));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  const auto fp16_pos = UnitPos(code, "runtime/om2/common/fp16_t/om2_fp16_t.h");
  const auto hif8_pos = UnitPos(code, "runtime/om2/common/math/om2_hif8_t.h");
  const auto transfer_pos = UnitPos(code, "runtime/om2/common/datatype_transfer/om2_datatype_transfer.h");
  ASSERT_NE(fp16_pos, std::string::npos);
  ASSERT_NE(hif8_pos, std::string::npos);
  ASSERT_NE(transfer_pos, std::string::npos);
  EXPECT_LT(fp16_pos, hif8_pos);
  EXPECT_LT(hif8_pos, transfer_pos);
}

// 每个内联单元前都要有边界注释，标出这段来自哪个上游文件——产物不用 per-unit #line，
// 边界注释是产物里唯一的上游溯源信息。条数同时守住"格式转换单元不全量内嵌"：
// 全量会是 7 + 34 + 6 = 47。
TEST_F(VarTransFileCodeGeneratorTest, EveryUnitCarriesBoundaryComment) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  entry.trans_road.push_back(MakeFormatNode(FORMAT_NCHW, FORMAT_NC1HWC0));
  entry.trans_road.push_back(MakeFormatNode(FORMAT_NHWC, FORMAT_NC1HWC0));
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kCast));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // FixedUnits 7 + 格式转换单元 4（nchw_nc1hwc0、nhwc_nc1hwc0 各 .h+.cc）
  // + 数据类型转换单元 6 = 17 个内联单元
  EXPECT_EQ(CountUnits(code), 17U);
  // 产物内不得残留任何 #line：文件级的那条由 EmitFile
  // （base/common/om2/codegen/program_generator.cc）追加，不在 BuildSegments 的输出里。
  // per-unit #line 会把诊断指向工具包里不存在的仓库路径。
  EXPECT_EQ(CountSubstr(code, "#line"), 0U);
  // 生成入口的边界注释与单元前缀不同，不被 CountUnits 计入，但必须在
  EXPECT_NE(code.find("内部转换入口（本文件生成，非内联单元）"), std::string::npos);
}

// 版权块与守卫宏名都由 EmbedUnits 剥掉（理由见 source_embed_util.h）。剥离必须严格限定在
// 文件头：om2_format_transfer_fz_c04.cc 正文中部还有一个 /** 块注释，一处都不能被误删。
TEST_F(VarTransFileCodeGeneratorTest, EmbedStripsHeadCopyrightAndGuardOnly) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  // 该格式对选中 fz_c04（.h + .cc），即上面那个正文带块注释的单元
  entry.trans_road.push_back(MakeFormatNode(FORMAT_HWCN, FORMAT_FRACTAL_Z_C04));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  ASSERT_NE(code.find("om2_format_transfer_fz_c04.cc"), std::string::npos);
  // 版权块剥净
  EXPECT_EQ(CountSubstr(code, "Copyright"), 0U);
  EXPECT_EQ(CountSubstr(code, "CANN Open Software License"), 0U);
  // 内嵌单元的守卫剥净。不按 "#ifndef 总数为 0" 断言：kVarTransAdapter 自带 GE_CHECK_GE /
  // GE_CHECK_NOTNULL 两对宏重定义守卫，那是 StablePart 而非内嵌单元，与本剥离无关。
  EXPECT_EQ(CountSubstr(code, "#ifndef GE_OM2_"), 0U);
  EXPECT_EQ(CountSubstr(code, "#ifndef GE_RUNTIME_OM2_"), 0U);
  EXPECT_EQ(CountSubstr(code, "#ifndef INC_OM2_"), 0U);
  // 不得半剥：#ifndef 与 #endif 必须配平，否则说明只剥掉了配对的一半。
  // 全部 47 个单元的条件指令实测恰为 24 对 #ifndef/#endif、无其他条件编译，
  // 上游若新增条件编译，EmbedUnits 会保守跳过剥离而让上面三条前缀断言变红。
  EXPECT_EQ(CountSubstr(code, "#ifndef"), CountSubstr(code, "#endif"));
  // 正文中部的块注释必须原样存活
  EXPECT_NE(code.find("Explain about transfer from nchw to FZ_CO4"), std::string::npos);
  // 函数式宏不是守卫的 #define，不得被剥（om2_register_format_transfer.h 属 FixedUnits，恒定内嵌）
  EXPECT_NE(code.find("#define REGISTER_FORMAT_TRANSFER("), std::string::npos);
  // 剥离只动行、不动单元：边界注释条数与未剥离时一致
  EXPECT_EQ(CountUnits(code), 11U);
}

// 产物随 OM 交付给客户，仓库内部的分段编号、阶段编号与使用处数统计对客户不可解析，
// 必须留在生成器侧注释里。kVarTransAdapter 是 StablePart 字符串而非 AST，
// 且 BuildSegments 无条件发射它——连只有两个桩入口的产物也带，故这一侧同样要守。
TEST_F(VarTransFileCodeGeneratorTest, ProductCarriesNoInternalVocabulary) {
  VarFixupTable table;
  auto entry = MakeEntry("w1_0");
  // 取最大的产物形态（fz_c04 闭包），探针覆盖面最广
  entry.trans_road.push_back(MakeFormatNode(FORMAT_HWCN, FORMAT_FRACTAL_Z_C04));
  table.entries.push_back(entry);

  const VarTransFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // 正向对照：适配头确实在产物里，否则下面那组探针是空转（产物为空也会全绿）
  ASSERT_NE(code.find("#define GE_CHECK_GE("), std::string::npos);
  ASSERT_NE(code.find("class TypeUtilsInner {"), std::string::npos);
  // 前一组是仓库内部的分段/阶段/判据编号与使用处数统计，后一组是改动史词汇：产物描述的是
  // 当前形态，客户拿到既无法解析编号、也无从核对改前改后。刻意不收"折叠""不再""不保留"
  // ——它们在产物里可能是正当的技术或运行期陈述（如"编译期折叠"），收进来会误伤。
  for (const auto *leak :
       {"见风险", "风险 R", "段①",   "段②",    "段③",    "段④",        "段⑤",      "阶段 A",  "阶段 B",
        "阶段 C", "阶段 D", "守卫①", "守卫②",  "守卫③",  "守卫④",      "集合 P",   "集合 D",  "级别 1",
        "级别 2", "级别 3", "读点 ", "处使用", "令牌",   "三件套",     "转换段",   "下沉前",  "下沉后",
        "改造前", "改动前", "原先",  "此前",   "已删除", "合并为单一", "早已备好", "设计文档"}) {
    EXPECT_EQ(code.find(leak), std::string::npos) << "内部词汇泄漏进客户产物: " << leak;
  }
}
}  // namespace
}  // namespace ge
