/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <string>

#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"

// AST 发射器的节点级契约测试。该框架被多个文件生成器与全部 StablePart 共用，若只靠
// program_generator_unittest.cc 里数千行的产物全文比对覆盖，一旦发射格式被扰动，失败信息是
// 一整篇生成代码的 diff，定位成本极高。本文件把每个节点的输出形态钉死在最小可复现单元上。
namespace ge {
namespace {
std::string EmitNode(const AstNode &node) {
  CppEmitter emitter;
  std::string output;
  EXPECT_EQ(node.Accept(emitter, output), SUCCESS);
  return output;
}

class AstEmitterUt : public testing::Test {
 protected:
  AstContext ctx;
  AstBuildContext ast{ctx};
};

TEST_F(AstEmitterUt, ContinueEmitsKeyword) {
  EXPECT_EQ(EmitNode(*ast.Block({ast.Continue()})), "{\n  continue;\n}\n");
}

// 与既有产物里的 for (uint32_t j = 0U; (j < n); j++) 同形
TEST_F(AstEmitterUt, ForWithAllClausesKeepsExistingForm) {
  auto i = ast.Var("uint32_t", "i");
  const auto *loop = ast.For(ast.VarDecl(i, ast.UInt(0)), i < ast.UInt(3), ast.PostInc(i), {ast.Continue()});
  EXPECT_EQ(EmitNode(*loop), "for (uint32_t i = 0U; (i < 3U); i++) {\n  continue;\n}\n");
}

TEST_F(AstEmitterUt, ElseIfChainIsFlattened) {
  const auto *stmt = ast.If("c1", {ast.Return(1)}, {ast.If("c2", {ast.Return(2)}, {ast.If("c3", {ast.Return(3)})})});
  EXPECT_EQ(EmitNode(*stmt),
            "if (c1) {\n  return 1;\n} else if (c2) {\n  return 2;\n} else if (c3) {\n  return 3;\n}\n");
}

// else 块含多条语句时必须保留花括号，否则语义改变
TEST_F(AstEmitterUt, ElseWithMultipleStatementsIsNotFlattened) {
  const auto *stmt = ast.If("c1", {ast.Return(1)}, {ast.If("c2", {ast.Return(2)}), ast.Return(3)});
  EXPECT_EQ(EmitNode(*stmt), "if (c1) {\n  return 1;\n} else {\n  if (c2) {\n    return 2;\n  }\n  return 3;\n}\n");
}

TEST_F(AstEmitterUt, IntegerSuffixCoversUnsignedLongLong) {
  EXPECT_EQ(EmitNode(*ast.Return(ast.ULongLong(96))), "return 96ULL;\n");
  EXPECT_EQ(EmitNode(*ast.Return(ast.ULong(96))), "return 96UL;\n");
  EXPECT_EQ(EmitNode(*ast.Return(ast.UInt(96))), "return 96U;\n");
}

// 地址类取值按位阅读，必须能发十六进制；小写、无前导零，与 std::hex 一致
TEST_F(AstEmitterUt, HexLiteralIsLowercaseWithoutLeadingZeros) {
  EXPECT_EQ(EmitNode(*ast.Return(ast.Hex(0xC000000000ULL, LiteralExpr::IntSuffix::kULL))), "return 0xc000000000ULL;\n");
  EXPECT_EQ(EmitNode(*ast.Return(ast.Hex(0x7F0000001000ULL, LiteralExpr::IntSuffix::kULL))),
            "return 0x7f0000001000ULL;\n");
  EXPECT_EQ(EmitNode(*ast.Return(ast.Hex(0U))), "return 0x0;\n");
  EXPECT_EQ(EmitNode(*ast.Return(ast.Hex(0xFU))), "return 0xf;\n");
}

TEST_F(AstEmitterUt, VarDeclAssignStyleIsUnchanged) {
  EXPECT_EQ(EmitNode(*ast.VarDecl("int32_t", "x", Arg(1))), "int32_t x = 1;\n");
  EXPECT_EQ(EmitNode(*ast.VarDecl("char", "name[16]", ast.InitList({Arg(0)}))), "char name[16] = {0};\n");
}

// 直接初始化：定界符由 init 自带，既不能插 " = " 也不能插空格
TEST_F(AstEmitterUt, VarDeclDirectStyleSupportsBraceAndParenInit) {
  EXPECT_EQ(EmitNode(*ast.VarDecl("GertVarDesc", "desc", ast.InitList({}), VarDeclStmt::InitStyle::kDirect)),
            "GertVarDesc desc{};\n");
  EXPECT_EQ(EmitNode(*ast.VarDecl("std::atomic<size_t>", "next_task", ast.InitList({ast.UInt(0)}),
                                  VarDeclStmt::InitStyle::kDirect)),
            "std::atomic<size_t> next_task{0U};\n");
  // 括号双参与花括号语义不同：std::vector<uint8_t> v(n, 0U) 是 n 个 0，= {n, 0U} 是 2 个元素
  EXPECT_EQ(EmitNode(*ast.VarDecl("std::vector<uint8_t>", "buf",
                                  ast.InitList({Arg("n"), ast.UInt(0)}, false, InitListExpr::Delimiter::kParen),
                                  VarDeclStmt::InitStyle::kDirect)),
            "std::vector<uint8_t> buf(n, 0U);\n");
}

// 圆括号定界不允许尾逗号，故 compact 展开只对花括号生效
TEST_F(AstEmitterUt, ParenInitListIgnoresCompactFlag) {
  EXPECT_EQ(EmitNode(*ast.Return(ast.InitList({Arg("a"), Arg("b")}, true, InitListExpr::Delimiter::kParen))),
            "return (a, b);\n");
}

TEST_F(AstEmitterUt, LambdaWithoutParamsKeepsExistingForm) {
  // CaptureRef 是具名按引用捕获，产出 [&ctx]；既有 5 处 guard 用法均为此形态
  auto ctx_var = ast.Var("const Ctx &", "ctx");
  EXPECT_EQ(EmitNode(*ast.Lambda({ast.CaptureRef(ctx_var)}, {ast.Return(0)}).Get()), "[&ctx]() {\n  return 0;\n}\n");
}

// 工作线程任务的形态：多个显式按值捕获 + 尾置返回类型，即产物里的
// pool.commit([ctx, parent_ctx, i]() -> aclError {…})。不得按引用捕获——任务经
// packaged_task 在池线程执行，按引用会读到循环变量终值、且外层局部可能已析构
TEST_F(AstEmitterUt, LambdaSupportsParamsAndTrailingReturnType) {
  const LambdaCaptureSpec ctx{"ctx", LambdaCaptureSpec::Kind::kByValue};
  const LambdaCaptureSpec parent_ctx{"parent_ctx", LambdaCaptureSpec::Kind::kByValue};
  const LambdaCaptureSpec idx{"i", LambdaCaptureSpec::Kind::kByValue};
  EXPECT_EQ(EmitNode(*ast.Lambda({ctx, parent_ctx, idx}, {}, "aclError", {ast.Return(-1)}).Get()),
            "[ctx, parent_ctx, i]() -> aclError {\n  return -1;\n}\n");
  auto i = ast.Var("uint32_t", "i");
  EXPECT_EQ(EmitNode(*ast.Lambda({ctx}, {i}, "int32_t", {ast.Return(-1)}).Get()),
            "[ctx](uint32_t i) -> int32_t {\n  return -1;\n}\n");
}

// 原文节点照发，不加缩进、不解析：承载文本内联的转换单元源码
TEST_F(AstEmitterUt, RawTextIsEmittedVerbatim) {
  EXPECT_EQ(EmitNode(*ast.RawText("#line 1 \"a.cc\"\nint F() { return 0; }\n")),
            "#line 1 \"a.cc\"\nint F() { return 0; }\n");
}

TEST_F(AstEmitterUt, EnumWithUnderlyingTypeAndExplicitValues) {
  const auto *e = ast.Enum("VarTransKind", "uint8_t",
                           {{"kVarTransNoOp", Arg(0)}, {"kVarTransFormat", Arg(1)}, {"kVarTransCast", Arg(2)}});
  EXPECT_EQ(EmitNode(*e),
            "enum VarTransKind : uint8_t {\n  kVarTransNoOp = 0,\n  kVarTransFormat = 1,\n  kVarTransCast = 2,\n};\n");
}

// 无底层类型、无显式取值两种退化形态；末项尾逗号自 C++11 起合法
TEST_F(AstEmitterUt, EnumOmitsUnderlyingTypeAndValueWhenAbsent) {
  EXPECT_EQ(EmitNode(*ast.Enum("Plain", "", {{"kA", Arg()}, {"kB", Arg()}})), "enum Plain {\n  kA,\n  kB,\n};\n");
}

TEST_F(AstEmitterUt, TryCatchAllWrapsBothBlocks) {
  EXPECT_EQ(EmitNode(*ast.TryCatchAll({ast.Return(0)}, {ast.Return(-1)})),
            "try {\n  return 0;\n} catch (...) {\n  return -1;\n}\n");
}

// 组合形态：完整 for 的循环体内混合 拷贝初始化 + if/return + 下标赋值
TEST_F(AstEmitterUt, ComposesForLoopBodyShapes) {
  auto next_task = ast.Var("std::atomic<size_t>", "next_task");
  auto trans_tasks = ast.Var("const std::vector<uint32_t> &", "trans_tasks");
  auto task_status = ast.Var("std::vector<int32_t>", "task_status");
  auto idx = ast.Var("const size_t", "idx");
  auto i = ast.Var("uint32_t", "i");
  const auto *loop = ast.For(ast.VarDecl(i, ast.UInt(0)), i < trans_tasks.Size(), ast.PostInc(i),
                             {
                                 ast.VarDecl(idx, next_task.Attr("fetch_add")(ast.UInt(1))),
                                 ast.If(idx >= trans_tasks.Size(), {ast.Return()}),
                                 ast.Assign(task_status[idx], Arg("trans_one(trans_tasks[idx])")),
                             });
  const auto code = EmitNode(*loop);
  EXPECT_NE(code.find("for (uint32_t i = 0U; (i < trans_tasks.size()); i++) {"), std::string::npos);
  EXPECT_NE(code.find("const size_t idx = next_task.fetch_add(1U);"), std::string::npos);
  // IfStmt::Create 对顶层条件调 SetNeedParen(false)，故不再多套一层括号；
  // 内层子表达式仍各自带括号（如既有产物里的 if ((a.type == X) || (a.type == Y))）
  EXPECT_NE(code.find("if (idx >= trans_tasks.size()) {"), std::string::npos);
  EXPECT_NE(code.find("return;\n"), std::string::npos);
  EXPECT_NE(code.find("task_status[idx] = trans_one(trans_tasks[idx]);"), std::string::npos);
}

// 组合形态：var_trans 的产物骨架——原文节点夹在两个命名空间之间
TEST_F(AstEmitterUt, ComposesRawTextBetweenNamespaces) {
  const auto *unit = ast.File({
      ast.Namespace("ge", {ast.Field("constexpr int32_t", "kA", Arg(1))}),
      ast.Space(),
      ast.RawText("#line 1 \"u.cc\"\nvoid U() {}\n"),
      ast.Namespace("om2", {ast.Namespace("var_trans", {ast.DeclareFunction("TransFormat", {}, "int32_t")})}),
  });
  const auto code = EmitNode(*unit);
  EXPECT_NE(code.find("namespace ge {\nconstexpr int32_t kA = 1;\n} // namespace ge\n"), std::string::npos);
  EXPECT_NE(code.find("#line 1 \"u.cc\"\nvoid U() {}\n"), std::string::npos);
  EXPECT_NE(code.find("int32_t TransFormat();\n} // namespace var_trans\n} // namespace om2\n"), std::string::npos);
}
}  // namespace
}  // namespace ge
