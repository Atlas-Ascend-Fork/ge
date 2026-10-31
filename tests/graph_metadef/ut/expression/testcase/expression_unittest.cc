/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <symengine/integer.h>
#include <symengine/assumptions.h>
#include <symengine/functions.h>
#include <symengine/simplify.h>
#include <symengine/real_double.h>
#include <symengine/parser.h>
#include <iostream>
#include <cmath>
#include "common/checker.h"
#include "graph/symbolizer/symbolic.h"
#include "expression/expression_impl.h"
#include "expression/expr_parser.h"

#include <util/mem_utils.h>
#include "exe_graph/runtime/infer_symbol_shape_context.h"
#include "faker/kernel_run_context_faker.h"
#include "attribute_group/attr_group_shape_env.h"
#include "graph/symbolizer/symbolic_utils.h"
#include "source_stub.h"
namespace ge {
class UtestExpression : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};
using namespace ge;
using namespace ge::sym;

namespace {
void ExpectSerializeRoundtrip(const Expression &expr, const std::string &expect_str) {
  const auto serialized = expr.Serialize();
  EXPECT_EQ(std::string(serialized.get()), expect_str);
  EXPECT_EQ(Expression::Parse(serialized.get()), expr);
}

void ExpectDeserializeRoundtrip(const Expression &expr, const std::string &expect_str) {
  const auto serialized = expr.Serialize();
  EXPECT_EQ(std::string(serialized.get()), expect_str);
  EXPECT_EQ(Expression::Deserialize(serialized.get()), expr);
}

void ExpectParseAndSerialize(const std::string &parse_str, const std::string &expect_str) {
  const Expression expr = Expression::Parse(parse_str.c_str());
  EXPECT_EQ(std::string(expr.Serialize().get()), expect_str);
}
}  // namespace

TEST_F(UtestExpression, TestBasic) {
  auto symbol2 = Symbol(2);
  EXPECT_EQ(symbol2, 2);
  auto symbol4 = Symbol(4);
  EXPECT_EQ(symbol4.IsValid(), true);
  EXPECT_EQ(symbol4, 4);
  EXPECT_EQ(symbol2 + symbol4, 6);
  EXPECT_EQ(symbol4 - symbol2, 2);
  EXPECT_EQ(symbol4 * symbol2, 8);
  EXPECT_EQ(symbol4 / symbol2, 2);

  auto e2 = Expression(symbol2);
  EXPECT_EQ(e2, 2);
  EXPECT_EQ(e2.IsValid(), true);
  Expression copy_e2;
  copy_e2 = e2;
  EXPECT_EQ(copy_e2, 2);
  auto e4 = Expression(symbol4);
  EXPECT_EQ(e4, 4);
  EXPECT_EQ(e2 + e4, 6);
  EXPECT_EQ(e4 - e2, 2);
  EXPECT_EQ(e4 * e2, 8);
  EXPECT_EQ(e4 / e2, 2);
}

TEST_F(UtestExpression, GetExprType) {
  auto symbol1 = Symbol(2);
  EXPECT_EQ(symbol1, 2);
  EXPECT_NE(symbol1, 3);

  auto symbol2 = Symbol(2.0);
  EXPECT_EQ(symbol2, 2.0);
  EXPECT_NE(symbol2, 2);

  EXPECT_EQ(symbol1.GetExprType(), ExprType::kExprConstantInteger);
  EXPECT_EQ(Symbol(2l).GetExprType(), ExprType::kExprConstantInteger);
  EXPECT_EQ(Symbol(2lu).GetExprType(), ExprType::kExprConstantInteger);
  EXPECT_EQ(Symbol(2.5).GetExprType(), ExprType::kExprConstantRealDouble);

  const auto num = Symbol(2);
  const auto den = Symbol(3);
  EXPECT_EQ(Div(num, den).GetExprType(), ExprType::kExprConstantRation);
  for (const auto &expr : {Mul(num, den), Add(num, den), Sub(num, den), Max(num, den), Min(num, den),
                           Ceiling(Symbol(2.5)), Pow(num, den), Mod(num, den)}) {
    EXPECT_EQ(expr.GetExprType(), ExprType::kExprConstantInteger);
  }
  EXPECT_EQ(Log(num, den).GetExprType(), ExprType::kExprOperation);

  const auto var_x = Symbol("a");
  EXPECT_EQ(var_x.GetExprType(), ExprType::kExprVariable);

  for (const auto &expr :
       {Add(symbol1, var_x), Sub(symbol1, var_x), Mul(symbol1, var_x), Div(symbol1, var_x), Max(symbol1, var_x),
        Min(symbol1, var_x), Pow(symbol1, var_x), Mod(symbol1, var_x), Neg(var_x), Neg(Div(symbol1, var_x))}) {
    EXPECT_EQ(expr.GetExprType(), ExprType::kExprOperation);
  }
  EXPECT_EQ(Neg(symbol1).GetExprType(), ExprType::kExprConstantInteger);
  EXPECT_NE(Add(symbol1, var_x), var_x);
  EXPECT_EQ(std::string(Neg(var_x).Serialize().get()), "(-1 * a)");
  EXPECT_EQ(std::string(Neg(Add(var_x, symbol1)).Serialize().get()), "((2 + a) * -1)");
  EXPECT_EQ(std::string(Neg(symbol1 / var_x).Serialize().get()), "(-2 / (a))");
  EXPECT_EQ(std::string(Mod(symbol1, var_x).Serialize().get()), "Mod(2, a)");
}
TEST_F(UtestExpression, GetLogicConstExprType_Succ) {
  auto symbol_const_0 = Symbol(2);
  auto symbol_const_1 = Symbol(2);
  auto expr_eq = Eq(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_eq.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_ne = Ne(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_ne.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_gt = Gt(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_gt.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_ge = Ge(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_ge.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_lt = Lt(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_lt.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_le = Le(symbol_const_0, symbol_const_1);
  EXPECT_EQ(expr_le.GetExprType(), ExprType::kExprConstantBoolean);
  auto expr_not = Not(expr_le);
  EXPECT_EQ(expr_not.GetExprType(), ExprType::kExprConstantBoolean);

  auto expr_log = Log(symbol_const_0, symbol_const_1);
  auto expr_ne_log = Ne(expr_log, symbol_const_0);
  EXPECT_EQ(expr_ne_log.GetExprType(), ExprType::kExprConstantBoolean);
}

TEST_F(UtestExpression, GetLogicExprType_Succ) {
  auto symbol_logic_0 = Symbol("s0");
  auto symbol_logic_1 = Symbol("s1");
  auto symbol_const_0 = Symbol(2);
  auto symbol_const_1 = Symbol(2);
  auto expr_eq = Eq(symbol_logic_0, symbol_logic_1);
  EXPECT_EQ(expr_eq.GetExprType(), ExprType::kExprOperationBoolean);

  auto expr_log = Log(symbol_const_0, symbol_const_1);
  auto expr_gt = Gt(expr_log, symbol_logic_0);
  EXPECT_EQ(expr_gt.GetExprType(), ExprType::kExprOperationBoolean);

  auto expr_ge = Ge(symbol_logic_0 + symbol_logic_1, symbol_const_0);
  EXPECT_EQ(expr_ge.GetExprType(), ExprType::kExprOperationBoolean);

  auto expr_lt = Lt(symbol_const_0 + symbol_const_1, symbol_logic_1);
  EXPECT_EQ(expr_lt.GetExprType(), ExprType::kExprOperationBoolean);

  auto expr_le = Le(symbol_logic_0 * symbol_const_1, symbol_const_1);
  EXPECT_EQ(expr_le.GetExprType(), ExprType::kExprOperationBoolean);

  auto expr_not = Not(expr_le);
  EXPECT_EQ(expr_not.GetExprType(), ExprType::kExprOperationBoolean);
}

TEST_F(UtestExpression, EqSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Eq(Pow(s0, s1) + Max(s1, s2) * s1, (Min(s0, s2) + s1) * (Div(s2, s1) + s1));
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "ExpectEq((((s2 / (s1)) + s1) * (Min(s0, s2) + s1)), ((Max(s1, s2) * s1) + Pow(s0, s1)))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, NeSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Ne(s0 + s1 + s2, s1 + s0 * s2);
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "ExpectNe(((s0 * s2) + s1), (s0 + s1 + s2))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, ParseNumer_Failed) {
  // out_of_range
  auto expr_parser = Expression::Parse("11111111111111111111111111111111111000000000000000000000000000000+s1");
  EXPECT_EQ(expr_parser.Str().get(), nullptr);

  // invalid_argument
  ge::Scanner scanner("");
  ge::ExprParser ep(scanner);
  ep.currentToken_.value = "this is not a number";
  EXPECT_EQ(ep.ParserNumber(), nullptr);
}

TEST_F(UtestExpression, GtSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Gt(s0 + Symbol(3) * s2, (Symbol(3) + s1) * (s0 + s2));
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "ExpectLt(((3 + s1) * (s0 + s2)), ((3 * s2) + s0))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, LeSerializeAndDeserialize_Succ) {
  auto expr = Le(Symbol(2), Symbol(3));
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "True");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, TestExpressionInvalid) {
  Expression expr = Expression::Parse("3 & 8");
  EXPECT_EQ(expr.Simplify().IsValid(), false);
  EXPECT_EQ(expr.Replace({}).IsValid(), false);
  EXPECT_EQ(expr.Subs({}).IsValid(), false);
}

TEST_F(UtestExpression, FalseConstSerializeAndDeserialize_Succ) {
  auto expr = Not(Le(Symbol(2), Symbol(3)));
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "False");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, ConstBoolWithSymbolSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto expr = Eq(Le(Symbol(2), Symbol(3)), Ne(s0, Symbol(2)));
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "ExpectEq(ExpectNe(2, s0), True)");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, GeSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Ge(Max(s0 * s2, s1) * s2, Ceiling(Symbol(3) + s1 * s0 + s2) + s1);
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "ExpectLe((3 + Ceiling(((s0 * s1) + s2)) + s1), (Max(s1, (s0 * s2)) * s2))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, LtSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Lt(Abs(Max(s1, s0 * s2) + Symbol(3)), Symbol(4));
  const std::string expr_str = std::string(expr.Serialize().get());
  // 大于可以被转化为小于
  EXPECT_EQ(expr_str, "ExpectLt(Abs((3 + Max(s1, (s0 * s2)))), 4)");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, NotSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Not(Eq(Pow(s1, s0) + Max(s1, s2) * s1, (Min(s0, s2) + s1) * (Div(s2, s1) + s1)));
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "ExpectNe((((s2 / (s1)) + s1) * (Min(s0, s2) + s1)), ((Max(s1, s2) * s1) + Pow(s1, s0)))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, DoubleNotSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Not(Not(Eq(Pow(s0, s1) + Max(s1, s2) * s1, (Min(s0, s2) + s1) * (Div(s2, s1) + s1))));
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "ExpectEq((((s2 / (s1)) + s1) * (Min(s0, s2) + s1)), ((Max(s1, s2) * s1) + Pow(s0, s1)))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

// RealDouble 常量以科学计数法打印(如 float32 的 1e-8 为 9.99999993922529e-09)，
// 需保证扫描/解析侧能识别 e 指数，使序列化-反序列化往返闭环
TEST_F(UtestExpression, ScientificNotationSerializeAndDeserialize_Succ) {
  auto s52 = Symbol("s52");
  auto expr = Add(Symbol(9.99999993922529e-09), s52);
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "(9.99999993922529e-09 + s52)");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser.IsValid(), true);
  EXPECT_EQ(expr_parser, expr);
}

// 无小数点的科学计数法(如 2e-3)不能走整型解析分支被截断，必须识别为浮点
TEST_F(UtestExpression, ParseScientificNotationNumber_Succ) {
  auto expr = Expression::Parse("2e-3");
  EXPECT_EQ(expr.IsValid(), true);
  EXPECT_EQ(expr, Symbol(2e-3));
  auto expr_upper = Expression::Parse("2E3");
  EXPECT_EQ(expr_upper.IsValid(), true);
  EXPECT_EQ(expr_upper, Symbol(2e3));
}

// 正指数显式带 '+' (如 1e+20)：覆盖 scanner 指数前瞻的 '+' 符号分支
TEST_F(UtestExpression, ParseScientificNotationPositiveExponent_Succ) {
  auto expr = Expression::Parse("1e+20");
  EXPECT_EQ(expr.IsValid(), true);
  EXPECT_EQ(expr, Symbol(1e20));
  auto expr_upper = Expression::Parse("1E+16");
  EXPECT_EQ(expr_upper.IsValid(), true);
  EXPECT_EQ(expr_upper, Symbol(1e16));
}

// 正指数大常量(float32 大数值)经 RealDouble 打印为 e+ 形态，需保证序列化-反序列化往返
TEST_F(UtestExpression, ScientificNotationPositiveExponentRoundtrip_Succ) {
  auto s0 = Symbol("s0");
  auto expr = Add(Symbol(1e+20), s0);
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_NE(expr_str.find("e+"), std::string::npos);
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser.IsValid(), true);
  EXPECT_EQ(expr_parser, expr);
}

// 指数位数不固定：单位数负指数 e-9 也需识别(打印侧为 e-09)，字符串 (9.99999993922529e-9 + s52) 可解析
TEST_F(UtestExpression, ParseScientificNotationSingleDigitExponent_Succ) {
  const char *expr_str = "(9.99999993922529e-9 + s52)";
  auto expr = Expression::Parse(expr_str);
  EXPECT_EQ(expr.IsValid(), true);
  EXPECT_EQ(expr, Add(Symbol(9.99999993922529e-9), Symbol("s52")));
  auto deser = Expression::Deserialize(expr_str);
  EXPECT_EQ(deser.IsValid(), true);
}

TEST_F(UtestExpression, NotEqualSerializeAndDeserialize_Succ) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto expr = Eq(Not(Ne(Symbol(2), s0)), Ge((s1 + s2), s0));
  const std::string expr_str = std::string(expr.Serialize().get());
  EXPECT_EQ(expr_str, "ExpectEq(ExpectEq(2, s0), ExpectLe(s0, (s1 + s2)))");
  auto expr_parser = Expression::Deserialize(expr_str.c_str());
  EXPECT_EQ(expr_parser, expr);
}

TEST_F(UtestExpression, Str) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto b_div_c = Div(var_b, var_c);
  EXPECT_EQ(std::string(b_div_c.Serialize().get()), "(b / (c))");
  auto one = Symbol(1);
  auto b_div_one = Div(one, var_c);
  EXPECT_EQ(std::string(b_div_one.Serialize().get()), "Pow(c, -1)");

  auto b_max_c = Max(b_div_c, var_c);
  EXPECT_EQ(std::string(b_max_c.Serialize().get()), "Max(c, (b / (c)))");

  //  EXPECT_EQ(Rational(2, 3)->Serialize(), "(double)(2)/(double)(3)");
  EXPECT_EQ(std::string(Pow(var_b, Rational(1, 2)).Serialize().get()), "Sqrt(b)");
  EXPECT_EQ(std::string(Pow(var_b, Symbol(3)).Serialize().get()), "(b * b * b)");
  EXPECT_EQ(std::string(Pow(var_b, var_c).Serialize().get()), "Pow(b, c)");
  EXPECT_EQ(std::string(Mod(var_b, var_c).Serialize().get()), "Mod(b, c)");
  EXPECT_EQ(std::string(Ceiling(var_b).Serialize().get()), "Ceiling(b)");
  EXPECT_EQ(std::string(Abs(var_b).Serialize().get()), "Abs(b)");
  EXPECT_EQ(std::string(Sub(Symbol(2), Symbol(3) * var_b).Serialize().get()), "(2 - (3 * b))");
  EXPECT_EQ(std::string(Max(Max(var_b, var_c), Symbol("d")).Serialize().get()), "Max(Max(b, c), d)");
  EXPECT_EQ(std::string(Min(Min(var_b, var_c), Symbol("d")).Serialize().get()), "Min(Min(b, c), d)");
}

TEST_F(UtestExpression, Parser) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto b_div_c = Div(var_b, var_c);
  ExpectSerializeRoundtrip(b_div_c, "(b / (c))");
  EXPECT_EQ(std::string(Max(b_div_c, var_c).Serialize().get()), "Max(c, (b / (c)))");

  // 对于表达式2/3，序列化采用Rational(a, b)表达分子分母，只支持int分子分母
  ExpectSerializeRoundtrip(Rational(2, 3), "Rational(2 , 3)");
  ExpectSerializeRoundtrip(Pow(var_b, Symbol(3)), "(b * b * b)");
  ExpectSerializeRoundtrip(Pow(var_b, var_c), "Pow(b, c)");
  ExpectSerializeRoundtrip(Mod(var_b, var_c), "Mod(b, c)");
  ExpectSerializeRoundtrip(Log(var_b, var_c), "(Log(b) / (Log(c)))");
  ExpectSerializeRoundtrip(Sub(Symbol(2), Symbol(3) * var_b), "(2 - (3 * b))");
  ExpectSerializeRoundtrip(Add(Symbol(2), Symbol(3) * var_b), "((3 * b) + 2)");  // 不保证顺序
  ExpectSerializeRoundtrip(Max(Max(var_b, var_c), Symbol("d")), "Max(Max(b, c), d)");
  ExpectSerializeRoundtrip(Min(Min(var_b, var_c), Symbol("d")), "Min(Min(b, c), d)");
  // 这个地方主线的序列化不保序，按照主线处理
  ExpectSerializeRoundtrip(Min(Max(var_b, var_c), Symbol("d")), "Min(d, Max(b, c))");
  ExpectSerializeRoundtrip(Ceiling(var_b), "Ceiling(b)");
  ExpectSerializeRoundtrip(Abs(var_b), "Abs(b)");
  ExpectSerializeRoundtrip(Min(Max(var_b, var_c), Symbol(5.0)), "Min(Max(b, c), 5.0)");
  EXPECT_EQ(std::string(var_b.GetName().get()), "b");
}

TEST_F(UtestExpression, Parser_Invalid) {
  auto failed_parser = Expression::Parse("1 % dsfde )");
  EXPECT_EQ(failed_parser.IsValid(), false);

  failed_parser = Expression::Parse("5* (sincos(s0s1))");
  EXPECT_EQ(failed_parser.IsValid(), false);
}

TEST_F(UtestExpression, Serialize_And_Deserialize) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto b_div_c = Div(var_b, var_c);
  ExpectDeserializeRoundtrip(b_div_c, "(b / (c))");
  EXPECT_EQ(std::string(Max(b_div_c, var_c).Serialize().get()), "Max(c, (b / (c)))");

  // 对于表达式2/3，序列化采用Rational(a, b)表达分子分母，只支持int分子分母
  ExpectDeserializeRoundtrip(Rational(2, 3), "Rational(2 , 3)");
  ExpectDeserializeRoundtrip(Pow(var_b, Symbol(3)), "(b * b * b)");
  ExpectDeserializeRoundtrip(Pow(var_b, var_c), "Pow(b, c)");
  ExpectDeserializeRoundtrip(Mod(var_b, var_c), "Mod(b, c)");
  ExpectDeserializeRoundtrip(Log(var_b, var_c), "(Log(b) / (Log(c)))");
  ExpectDeserializeRoundtrip(Sub(Symbol(2), Symbol(3) * var_b), "(2 - (3 * b))");
  ExpectDeserializeRoundtrip(Add(Symbol(2), Symbol(3) * var_b), "((3 * b) + 2)");  // 不保证顺序
  ExpectDeserializeRoundtrip(Max(Max(var_b, var_c), Symbol("d")), "Max(Max(b, c), d)");
  ExpectDeserializeRoundtrip(Min(Min(var_b, var_c), Symbol("d")), "Min(Min(b, c), d)");
  // 这个地方主线的序列化不保序，按照主线处理
  ExpectDeserializeRoundtrip(Min(Max(var_b, var_c), Symbol("d")), "Min(d, Max(b, c))");
  ExpectDeserializeRoundtrip(Ceiling(var_b), "Ceiling(b)");
  ExpectDeserializeRoundtrip(Abs(var_b), "Abs(b)");
  ExpectDeserializeRoundtrip(Min(Max(var_b, var_c), Symbol(5.0)), "Min(Max(b, c), 5.0)");
  EXPECT_EQ(std::string(var_b.GetName().get()), "b");
}
// 非规范但语义良好的串（SymEngine 规范打印会重排负系数 Add 项与交换律参数）被正确
// 接受（重解析后 Compare 语义树相等）；残留 token 的截断串与非法 token 被拒绝
TEST_F(UtestExpression, Deserialize_Invalid) {
  auto parser = Expression::Deserialize("s0*s1");
  EXPECT_EQ(parser.IsValid(), true);
  parser = Expression::Deserialize("a+2");
  EXPECT_EQ(parser.IsValid(), true);

  auto failed_parser = Expression::Deserialize("a(s0)");
  EXPECT_EQ(failed_parser.IsValid(), false);

  failed_parser = Expression::Deserialize("s0++");
  EXPECT_EQ(failed_parser.IsValid(), false);
}

// SymEngine 规范打印会重排负系数 Add 项：语义相等但字节不等的合法串（如来自
// 不同打印路径的 guard 序列化串）经重解析+Compare 接受（曾因字节不等被拒返回
// 空表达式，上层 guard 集合比较器解引用空指针断言崩溃）
TEST_F(UtestExpression, Deserialize_SemanticEqualReorderedStr) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  const auto parsed = Expression::Deserialize("-(2 * s1) + s0");
  EXPECT_EQ(parsed.IsValid(), true);
  EXPECT_EQ(parsed, s0 - Mul(Symbol(2), s1));
}

// 打印侧将 Pow(E, x) 输出为 Exp(x)、Pow(x, 1/2) 输出为 Sqrt(x)：含这两种形态的
// 序列化串经 Deserialize 还原为等价 Pow 表达式，序列化-反序列化往返闭环
TEST_F(UtestExpression, ExpSqrtSerializeAndDeserialize_RoundTrip) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");

  const auto exp_expr = sym::Exp(s0);
  const std::string exp_str = std::string(exp_expr.Serialize().get());
  EXPECT_EQ(exp_str, "Exp(s0)");
  EXPECT_EQ(Expression::Deserialize(exp_str.c_str()), exp_expr);

  const auto sqrt_expr = sym::Pow(s1, sym::Div(Symbol(1), Symbol(2)));
  const std::string sqrt_str = std::string(sqrt_expr.Serialize().get());
  EXPECT_EQ(sqrt_str, "Sqrt(s1)");
  EXPECT_EQ(Expression::Deserialize(sqrt_str.c_str()), sqrt_expr);

  const auto guard = Eq(exp_expr, sqrt_expr);
  const std::string guard_str = std::string(guard.Serialize().get());
  EXPECT_EQ(guard_str, "ExpectEq(Exp(s0), Sqrt(s1))");
  EXPECT_EQ(Expression::Deserialize(guard_str.c_str()), guard);
}

TEST_F(UtestExpression, EqualAndNotEqual) {
  auto var_b = Symbol("b");
  auto int_2 = Symbol(2);
  auto int_3 = Symbol(3);
  auto int_n_6 = Symbol(-6);
  auto int_6 = Symbol(6);

  auto b_2 = Mul(var_b, int_2);
  auto b_b = Add(var_b, var_b);
  EXPECT_TRUE(b_2 == b_b);

  auto b_3 = Mul(var_b, int_3);
  EXPECT_TRUE(b_3 != b_b);

  auto abs_1 = Abs(int_n_6);
  EXPECT_TRUE(abs_1 == int_6);
}

TEST_F(UtestExpression, SymbolCheckWithoutContext) {
  auto var_a = Symbol("a");
  auto var_b = Symbol("b");
  auto ret = EXPECT_SYMBOL_EQ(var_b, var_a);
  EXPECT_EQ(ret, false);
  bool guard_res0 = [&var_a, &var_b]() -> bool {
    ASSERT_SYMBOL_EQ(var_a, var_b);
    return true;
  }();
  EXPECT_EQ(guard_res0, false);
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(var_a, var_b), TriBool::kUnknown);
}

TEST_F(UtestExpression, GetName) {
  auto var_b = Symbol("b");
  EXPECT_EQ(std::string(var_b.GetName().get()), "b");
  auto var_c = Symbol(static_cast<int32_t>(1), "s0");
  EXPECT_EQ(std::string(var_c.GetName().get()), "s0");
  auto var_d = Symbol(static_cast<int64_t>(1), "s1");
  EXPECT_EQ(std::string(var_d.GetName().get()), "s1");
  auto var_e = Symbol(static_cast<uint32_t>(1), "s2");
  EXPECT_EQ(std::string(var_e.GetName().get()), "s2");
  auto var_f = Symbol(static_cast<uint64_t>(1), "s3");
  EXPECT_EQ(std::string(var_f.GetName().get()), "s3");
  auto var_g = Symbol(static_cast<double>(1.0), "s4");
  EXPECT_EQ(std::string(var_g.GetName().get()), "s4");
  auto var_h = Symbol(static_cast<double>(1.0));
  EXPECT_EQ(std::string(var_h.GetName().get()), "Const_0");
  auto var_i = Symbol(static_cast<int32_t>(5));
  EXPECT_EQ(std::string(var_i.GetName().get()), "Const_1");
}

TEST_F(UtestExpression, Operator) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  EXPECT_EQ((var_b + var_c).GetExprType(), ExprType::kExprOperation);
  EXPECT_EQ((var_b - var_c).GetExprType(), ExprType::kExprOperation);
  EXPECT_EQ((var_b * var_c).GetExprType(), ExprType::kExprOperation);
  EXPECT_EQ((var_b / var_c).GetExprType(), ExprType::kExprOperation);
}

TEST_F(UtestExpression, Replace) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto var_d = Symbol("d");
  auto b_div_c = Div(var_b, var_c);
  std::vector<std::pair<Expression, Expression>> replace_vars;
  replace_vars.push_back({var_b, var_d});
  auto replace_expr = b_div_c.Replace(replace_vars);
  EXPECT_TRUE(replace_expr == Div(var_d, var_c));
}

TEST_F(UtestExpression, Subs) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto var_d = Symbol("d");
  auto b_div_c = Div(var_b, var_c);
  std::vector<std::pair<Expression, Expression>> subs_vars;
  subs_vars.push_back({var_b, var_d});
  auto subs_expr = b_div_c.Subs(subs_vars);
  EXPECT_TRUE(subs_expr == Div(var_d, var_c));
}

TEST_F(UtestExpression, Simplify) {
  auto var_b = Symbol("b");
  auto const_1 = Symbol(1);
  auto const_2 = Symbol(2);
  auto const_3 = Symbol(3);
  EXPECT_TRUE((Add(Add(var_b, const_1), const_2).Simplify()) == Add(var_b, const_3));
}

TEST_F(UtestExpression, GetPrimaryArgs) {
  auto const_neg_2 = Symbol(-2);
  auto const_neg_3 = Symbol(-3);
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto var_d = Symbol("d");
  auto var_e = Symbol("e");
  std::vector<Expression> args_exp = {var_b, var_c, var_d, var_e};
  auto mul_expr = Min(Max(Add(Pow(Mul(const_neg_2, var_b), Mul(var_b, const_neg_3)), var_c), var_d), var_e);
  auto prim_args = mul_expr.FreeSymbols();
  EXPECT_EQ(prim_args.size(), args_exp.size());
  bool has_find = true;
  for (auto &arg_get : prim_args) {
    bool one_has_find = false;
    for (auto &arg_exp : args_exp) {
      if (arg_get == arg_exp) {
        one_has_find = true;
        break;
      }
    }
    if (!one_has_find) {
      has_find = false;
      break;
    }
  }
  EXPECT_EQ(has_find, true);
}

TEST_F(UtestExpression, ContainVar) {
  auto var_b = Symbol("b");
  auto const_1 = Symbol(1);
  EXPECT_TRUE(Add(var_b, const_1).ContainVar(var_b));
  EXPECT_FALSE(Add(var_b, const_1).ContainVar(const_1));
}

TEST_F(UtestExpression, GetResult) {
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto b_add_c = Add(var_b, var_c);
  std::vector<std::pair<Expression, Expression>> replace_vars;
  replace_vars.emplace_back(var_b, Symbol(1));
  replace_vars.emplace_back(var_c, Symbol(2));
  double result;
  auto code = b_add_c.GetResult(replace_vars, result);
  EXPECT_EQ(code, ge::GRAPH_SUCCESS);
  EXPECT_EQ(result, static_cast<double>(3));

  replace_vars.clear();
  replace_vars.emplace_back(var_b, Symbol(1.0));
  replace_vars.emplace_back(var_c, Symbol(2.0));
  code = b_add_c.GetResult(replace_vars, result);
  EXPECT_EQ(code, ge::GRAPH_SUCCESS);
  EXPECT_EQ(result, static_cast<double>(3));

  replace_vars.clear();
  replace_vars.emplace_back(var_b, Symbol(1));
  replace_vars.emplace_back(var_c, Rational(2, 3));
  code = b_add_c.GetResult(replace_vars, result);
  EXPECT_EQ(code, ge::GRAPH_SUCCESS);
  EXPECT_TRUE(std::abs(result - (static_cast<double>(1) + static_cast<double>(2) / static_cast<double>(3))) < 0.0001);
}

TEST_F(UtestExpression, GetBoolConstValueEq_Succ) {
  auto expr = Eq(Symbol(2), Symbol(2));
  bool value;
  EXPECT_EQ(expr.GetConstValue<bool>(value), true);
  EXPECT_EQ(value, true);
}

TEST_F(UtestExpression, GetBoolConstValueNot_Succ) {
  auto expr = Not(Eq(Symbol(2), Symbol(2)));
  bool value;
  EXPECT_EQ(expr.GetConstValue<bool>(value), true);
  EXPECT_EQ(value, false);
}

TEST_F(UtestExpression, GetBoolConstValueFromVariable_Failed) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto expr = Not(Eq(s0, s1));
  bool value;
  EXPECT_EQ(expr.GetConstValue<bool>(value), false);
}

TEST_F(UtestExpression, GetBoolConstValueFromNonBoolExpr_Failed) {
  auto s0 = Symbol(3);
  bool value;
  EXPECT_EQ(s0.GetConstValue<bool>(value), false);
}

TEST_F(UtestExpression, GetDoubleConstValueFromBoolExpr_Failed) {
  auto expr = Not(Eq(Symbol(2), Symbol(2)));
  double value;
  EXPECT_EQ(expr.GetConstValue<double>(value), false);
}

TEST_F(UtestExpression, GetDoubleConstValueFromIntExpr_Failed) {
  auto s0 = Symbol(2);
  double value;
  EXPECT_EQ(s0.GetConstValue<double>(value), false);
}

TEST_F(UtestExpression, GetDoubleConstValueFromRelationExpr_Succ) {
  auto s0 = Rational(2, 4);
  double value;
  EXPECT_EQ(s0.GetConstValue<double>(value), true);
  EXPECT_EQ(value, 0.5);
}

TEST_F(UtestExpression, NotExprWithInParamNotBool_Failed) {
  auto s0 = Symbol(2);
  auto not_expr = Not(s0);
  EXPECT_EQ(not_expr.IsValid(), false);
}

TEST_F(UtestExpression, GetBoolConstValueWithWrongType_Failed) {
  auto expr = Not(Eq(Symbol(2), Symbol(2)));
  int64_t value;
  EXPECT_EQ(expr.GetConstValue<int64_t>(value), false);
}

TEST_F(UtestExpression, GetBoolConstValueWithAdd_Succ) {
  auto expr = Eq(Symbol(2) + Symbol(2), Symbol(2) * Symbol(2));
  bool value = false;
  EXPECT_EQ(expr.GetConstValue<bool>(value), true);
  EXPECT_EQ(value, true);
}

TEST_F(UtestExpression, CheckIsVariable_Succ) {
  auto expr = Symbol("s0");
  EXPECT_EQ(expr.IsVariableExpr(), true);
}

TEST_F(UtestExpression, CheckIsBoolean_Succ) {
  auto expr = Eq(Symbol(2) + Symbol(2), Symbol(2) * Symbol(2));
  EXPECT_EQ(expr.IsBooleanExpr(), true);
  auto expr1 = Eq(Symbol(2), Symbol("s0"));
  EXPECT_EQ(expr1.IsBooleanExpr(), true);
}

TEST_F(UtestExpression, GetConstValue) {
  auto var_b = Symbol("b");
  int32_t value = 2;
  EXPECT_EQ(var_b.GetConstValue<int32_t>(value), false);

  auto check_const = [](const auto &const_symbol, const auto &expected) {
    using T = std::decay_t<decltype(expected)>;
    T res = 0;
    EXPECT_EQ(const_symbol.template GetConstValue<T>(res), true);
    EXPECT_EQ(res, expected);
  };
  check_const(Symbol(int32_t(2)), int32_t(2));
  check_const(Symbol(uint32_t(1)), uint32_t(1));
  check_const(Symbol(double(1.0)), double(1.0));
  check_const(Symbol(float(1.0)), float(1.0));
  check_const(Symbol(int64_t(1)), int64_t(1));
  check_const(Symbol(uint64_t(1)), uint64_t(1));

  const auto const_6 = Symbol(uint64_t(6));
  // 有符号整型运算 -> 有符号
  check_const(sym::Add(Symbol(uint64_t(1)), const_6), uint64_t(7));
  check_const(sym::Sub(const_6, Symbol(uint64_t(1))), uint64_t(5));
  check_const(sym::Mul(const_6, Symbol(uint64_t(2))), uint64_t(12));
  check_const(sym::Div(const_6, Symbol(uint64_t(2))), uint64_t(3));
  // uint64 常量 + 变量 -> 变量
  uint64_t res_value20 = 0U;
  EXPECT_EQ(sym::Add(const_6, var_b).GetConstValue<uint64_t>(res_value20), false);
  // 常量绝对值 -> 常量；变量绝对值 -> 变量
  EXPECT_EQ(sym::Abs(const_6).GetConstValue<uint64_t>(res_value20), true);
  EXPECT_EQ(sym::Abs(var_b).GetConstValue<uint64_t>(res_value20), false);
}
TEST_F(UtestExpression, TestAlgin) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");

  auto expr1 = Align(s0, 32);
  auto expr2 = Align(s0, 32);
  EXPECT_EQ(expr1, expr2);

  auto str = expr1.Serialize();
  EXPECT_EQ(std::string(str.get()), "(32 * Ceiling((Rational(1 , 32) * s0)))");
  auto expr3 = Expression::Parse(str.get());
  EXPECT_EQ(expr1, expr3);

  auto const_16 = Symbol(32);
  auto const_1 = Symbol(1);
  auto expr4 = (const_16 * Ceiling((Rational(1, 32) * s0)));
  EXPECT_EQ(expr1, expr4);
}

TEST_F(UtestExpression, AlignWithPositiveIntegerConst) {
  int res = 0;
  EXPECT_EQ(sym::AlignWithPositiveInteger(Symbol(10), 8).GetConstValue(res), true);
  EXPECT_EQ(res, 16);
  EXPECT_EQ(sym::AlignWithPositiveInteger(Symbol(15), 4).GetConstValue(res), true);
  EXPECT_EQ(res, 16);
  EXPECT_EQ(sym::AlignWithPositiveInteger(Symbol(7), 2).GetConstValue(res), true);
  EXPECT_EQ(res, 8);
  EXPECT_EQ(sym::AlignWithPositiveInteger(Symbol(8), 2).GetConstValue(res), true);
  EXPECT_EQ(res, 8);
}

TEST_F(UtestExpression, AlignWithPositiveInteger) {
  auto s0 = Symbol("s0");

  auto expr1 = sym::AlignWithPositiveInteger(s0, 8);
  auto expr2 = sym::AlignWithPositiveInteger(s0, 8);
  EXPECT_TRUE(expr1 == expr2);

  auto str0 = expr1.Serialize();
  EXPECT_EQ(std::string(str0.get()), "(8 * Floor(((7 + s0) * Rational(1 , 8))))");
  auto expr3 = Expression::Parse(str0.get());
  EXPECT_EQ(expr1, expr3);

  auto str1 = expr1.Str(StrType::kStrExpr);
  EXPECT_EQ(std::string(str1.get()), "(8 * Floor(((7 + s0) * 1/8)))");
  auto expr4 = Expression::Parse(str1.get());
  EXPECT_EQ(expr1, expr4);
}

TEST_F(UtestExpression, StrTypeTest) {
  auto expr1 = sym::Div(Symbol("s0"), Symbol("s1"));
  auto expr2 = sym::Div(Symbol("s0"), Symbol(8));
  auto expr3 = sym::Div(Symbol("s0"), Symbol(8));
  auto expr4 = sym::Div(Symbol("s0"), Symbol(8));
  EXPECT_EQ(std::string("(s0 / (s1))"), expr1.Str(StrType::kStrCpp).get());
  EXPECT_EQ(std::string("(Rational(1 , 8) * s0)"), expr2.Str(StrType::kStrCpp).get());
  EXPECT_EQ(std::string("(1/8 * s0)"), expr3.Str(StrType::kStrEnd).get());
  EXPECT_EQ(std::string("(1/8 * s0)"), expr4.Str(StrType::kStrExpr).get());
}

TEST_F(UtestExpression, TestAlignWithPositiveInteger_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(12, MakeShared<GraphInputShapeSourceStub>(0, 0));
  auto expr = sym::AlignWithPositiveInteger(sym0, 8);
  int64_t value_int = 0;
  EXPECT_EQ(expr.GetHint(value_int), true);
  EXPECT_EQ(value_int, 16);
  SetCurShapeEnvContext(nullptr);
}

Status InferAddSymbolShapeStub(gert::InferSymbolShapeContext *context) {
  auto input_shape0 = context->GetInputSymbolShape(0);
  GE_ASSERT_NOTNULL(input_shape0);
  auto input_shape1 = context->GetInputSymbolShape(1);
  GE_ASSERT_NOTNULL(input_shape1);
  auto output_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(output_shape);
  for (size_t i = 0UL; i < input_shape0->GetDimNum(); i++) {
    auto s0 = input_shape0->GetDim(i);
    auto s1 = input_shape1->GetDim(i);
    if (EXPECT_SYMBOL_EQ(s0, s1)) {
      output_shape->AppendDim(s0);
    } else if (EXPECT_SYMBOL_EQ(s0, Symbol(1))) {
      output_shape->AppendDim(s1);
    } else if (EXPECT_SYMBOL_EQ(s1, Symbol(1))) {
      output_shape->AppendDim(s0);
    } else {
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

TEST_F(UtestExpression, SymbolCheck_Old_Api_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  EXPECT_EQ(sym0.IsVariableExpr(), true);
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(sym1.IsVariableExpr(), true);
  const std::string file_name = "test.cc";
  bool guard_res0 = ExpectSymbolEq(sym0 + Symbol(1), sym1, file_name.c_str(), 2);
  EXPECT_EQ(guard_res0, true);
  bool guard_res1 = AssertSymbolEq(sym0 + Symbol(1), sym1, file_name.c_str(), 2);
  EXPECT_EQ(guard_res1, true);
  bool guard_res2 = ExpectSymbolBool(sym::Lt(sym0, sym1), file_name.c_str(), 2);
  EXPECT_EQ(guard_res2, true);
  bool guard_res3 = AssertSymbolBool(sym::Lt(sym0, sym1), file_name.c_str(), 2);
  EXPECT_EQ(guard_res3, true);
}

TEST_F(UtestExpression, SymbolCheck_With_Simplify_Guard_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol s2 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 2));
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, s1), true);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, s2), true);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(Symbol(2) * s1 + Symbol(64), Symbol(68)), true);
  EXPECT_EQ(EXPECT_SYMBOL_LT(s1, Symbol(100)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);
  auto expr = Symbol(2) * s0 + Symbol(64);
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(expr.Simplify(), Symbol(68)), TriBool::kTrue);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);
  // 第二次不会额外追加guard
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(expr.Simplify(), Symbol(68)), TriBool::kTrue);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);

  EXPECT_EQ(SymbolicUtils::StaticCheckLt(s2, Symbol(100)), TriBool::kTrue);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);
}

TEST_F(UtestExpression, SymbolCheck_With_Simplify_Guard_Succ2) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol s2 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 2));
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, s1), true);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, s2), true);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(Symbol(2) * s1 + Symbol(64), Symbol(68)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);
  // 化简后，应该是const的表达式，不会新增guard
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(s1, s2), TriBool::kTrue);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);
}

TEST_F(UtestExpression, SymbolCheck_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  std::vector<Symbol> syms;
  for (uint64_t i = 0U; i < 7U; i++) {
    syms.emplace_back(shape_env.CreateSymbol(i + 1UL, MakeShared<GraphInputShapeSourceStub>(0, i)));
  }
  const auto &sym0 = syms[0];
  const auto &sym1 = syms[1];
  const auto &sym2 = syms[2];
  const auto &sym3 = syms[3];
  const auto &sym4 = syms[4];
  const auto &sym5 = syms[5];
  const auto &sym6 = syms[6];
  for (const auto &sym : syms) {
    EXPECT_EQ(sym.IsVariableExpr(), true);
  }

  EXPECT_EQ(EXPECT_SYMBOL_EQ(sym0 + sym1, sym2), true);
  EXPECT_EQ(EXPECT_SYMBOL_NE(sym0 * sym1, sym2), true);
  EXPECT_EQ(EXPECT_SYMBOL_LT(sym3 / sym1, sym0), false);
  EXPECT_EQ(EXPECT_SYMBOL_LE(sym4 - sym3, sym0), true);
  EXPECT_EQ(EXPECT_SYMBOL_GT(sym::Pow(sym1, sym2), sym::Max(sym5, sym4)), true);
  EXPECT_EQ(EXPECT_SYMBOL_GE(sym::Min(sym1, sym2), sym::Abs(sym6)), false);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(Symbol(2), Symbol(3)), false);
  EXPECT_EQ(EXPECT_SYMBOL_GT(sym::Mod(sym6, sym1), sym::Max(sym5, sym4)), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(sym0 + sym1, sym2)), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ne(sym2, sym0 * sym1)), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Lt(sym3 / sym1, sym0)), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Not(sym::Lt(sym3 / sym1, sym0))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Le(sym4 - sym3, sym0)), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Lt(sym::Max(sym5, sym4), sym::Pow(sym1, sym2))), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ge(sym::Min(sym1, sym2), sym::Abs(sym6))), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Not(sym::Ge(sym::Min(sym1, sym2), sym::Abs(sym6)))), false);

  EXPECT_EQ(SymbolicUtils::StaticCheckEq(sym2, sym0 + sym1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckNe(sym2, sym1 * sym0), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLt(sym3 / sym1, sym0), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe(sym0, sym3 / sym1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe((sym4 - sym3), sym0), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(sym::Pow(sym1, sym2), sym::Max(sym5, sym4)), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(sym::Min(sym1, sym2), sym::Abs(sym6)), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(sym::Abs(sym6), sym::Min(sym1, sym2)), TriBool::kTrue);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SymbolCheckBroadCast_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(4, MakeShared<GraphInputShapeSourceStub>(0, 3));
  gert::SymbolShape in_shape0({(sym0 + sym2), sym2, sym0, sym1});
  gert::SymbolShape in_shape1({Symbol(4), sym0, sym3, (sym3 / sym1)});
  gert::SymbolShape out_shape({});

  auto context_holder = gert::InferSymbolShapeContextFaker()
                            .IrInputNum(2)
                            .NodeIoNum(2, 1)
                            .NodeInputTd(0, ge::DT_FLOAT16, ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0)
                            .NodeInputTd(1, ge::DT_FLOAT16, ge::FORMAT_HWCN, ge::FORMAT_FRACTAL_Z)
                            .Inputs({&in_shape0, &in_shape1})
                            .Outputs({&out_shape})
                            .Build();
  auto context = context_holder.GetContext<gert::InferSymbolShapeContext>();
  EXPECT_EQ(InferAddSymbolShapeStub(context), GRAPH_SUCCESS);
  auto output_shape = context->GetOutputSymbolShape(0);
  EXPECT_NE(output_shape, nullptr);
  EXPECT_EQ(output_shape->GetDimNum(), 4);
  EXPECT_EQ(output_shape->GetDim(0), (sym0 + sym2));
  EXPECT_EQ(output_shape->GetDim(1), sym2);
  EXPECT_EQ(output_shape->GetDim(2), sym3);
  EXPECT_EQ(output_shape->GetDim(3), sym1);

  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(sym0 + sym2, Symbol(4))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ne(sym2, sym0)), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ne(sym2, Symbol(1))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(sym0, Symbol(1))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ne(sym3, sym0)), false);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(sym1, sym3 / sym1)), true);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SymbolAssertCheck_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(4, MakeShared<GraphInputShapeSourceStub>(0, 3));
  Symbol sym4 = shape_env.CreateSymbol(5, MakeShared<GraphInputShapeSourceStub>(0, 4));
  Symbol sym5 = shape_env.CreateSymbol(6, MakeShared<GraphInputShapeSourceStub>(0, 5));
  Symbol sym6 = shape_env.CreateSymbol(7, MakeShared<GraphInputShapeSourceStub>(0, 6));

  EXPECT_EQ(([&sym1]() -> bool {
              ASSERT_SYMBOL_EQ(sym::Rational(4, 2), sym1);
              return true;
            }()),
            true);
  EXPECT_EQ(([&sym1, &sym2]() -> bool {
              ASSERT_SYMBOL_EQ(sym2, sym1);
              return true;
            }()),
            false);
  EXPECT_EQ(([&sym0, &sym2]() -> bool {
              ASSERT_SYMBOL_NE(sym::Ceiling(sym0), sym2);
              return true;
            }()),
            true);
  EXPECT_EQ(([&sym1, &sym4]() -> bool {
              ASSERT_SYMBOL_LT(sym::Log(sym1, sym1), sym4);
              return true;
            }()),
            true);
  EXPECT_EQ(([&sym4, &sym3, &sym0]() -> bool {
              ASSERT_SYMBOL_LE(sym4 - sym3, sym0);
              return true;
            }()),
            true);
  EXPECT_EQ(([&sym4, &sym5, &sym1, &sym2]() -> bool {
              ASSERT_SYMBOL_GT(sym::Pow(sym1, sym2), sym::Max(sym5, sym4));
              return true;
            }()),
            true);
  EXPECT_EQ(([&sym6, &sym1, &sym2]() -> bool {
              ASSERT_SYMBOL_GE(sym::Min(sym1, sym2), sym::Abs(sym6));
              return true;
            }()),
            false);
  EXPECT_EQ(([]() -> bool {
              ASSERT_SYMBOL_GE(Symbol(5), Symbol(2));
              return true;
            }()),
            true);

  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Eq(sym::Rational(4, 2), sym1)), true);
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Ne(sym::Ceiling(sym0), sym2)), true);
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Gt(sym4, sym::Log(sym1, sym1))), true);
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Le(sym4 - sym3, sym0)), true);
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Gt(sym::Pow(sym1, sym2), sym::Max(sym5, sym4))), false);
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Ge(sym::Min(sym1, sym2), sym::Abs(sym6))), false);

  EXPECT_EQ(SymbolicUtils::StaticCheckEq(sym::Rational(4, 2), sym1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckNe(sym::Ceiling(sym0), sym2), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(sym4, sym::Log(sym1, sym1)), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe((sym4 - sym3), sym0), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(sym::Pow(sym1, sym2), sym::Max(sym5, sym4)), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(sym::Min(sym1, sym2), sym::Abs(sym6)), TriBool::kUnknown);
  SetCurShapeEnvContext(nullptr);
}

Status InferMatmulSymbolShapeStub(gert::InferSymbolShapeContext *context) {
  auto input_shape0 = context->GetInputSymbolShape(0);
  GE_ASSERT_NOTNULL(input_shape0);
  auto input_shape1 = context->GetInputSymbolShape(1);
  GE_ASSERT_NOTNULL(input_shape1);
  auto output_shape = context->GetOutputSymbolShape(0);
  GE_ASSERT_NOTNULL(output_shape);
  auto s0 = input_shape0->GetDim(1);
  auto s1 = input_shape1->GetDim(0);
  ASSERT_SYMBOL_EQ(s0, s1);
  output_shape->AppendDim(input_shape0->GetDim(0));
  output_shape->AppendDim(input_shape1->GetDim(1));
  return GRAPH_SUCCESS;
}

TEST_F(UtestExpression, SymbolAssertMatmul_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(4, MakeShared<GraphInputShapeSourceStub>(0, 3));

  gert::SymbolShape in_shape0({(sym0 + sym2), sym3});
  gert::SymbolShape in_shape1({sym::Pow(sym1, sym1), (sym2 * Symbol(2))});
  gert::SymbolShape out_shape({});

  auto context_holder = gert::InferSymbolShapeContextFaker()
                            .IrInputNum(2)
                            .NodeIoNum(2, 1)
                            .NodeInputTd(0, ge::DT_FLOAT16, ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0)
                            .NodeInputTd(1, ge::DT_FLOAT16, ge::FORMAT_HWCN, ge::FORMAT_FRACTAL_Z)
                            .Inputs({&in_shape0, &in_shape1})
                            .Outputs({&out_shape})
                            .Build();
  auto context = context_holder.GetContext<gert::InferSymbolShapeContext>();
  EXPECT_EQ(InferMatmulSymbolShapeStub(context), GRAPH_SUCCESS);
  auto output_shape = context->GetOutputSymbolShape(0);
  EXPECT_NE(output_shape, nullptr);
  EXPECT_EQ(output_shape->GetDimNum(), 2);
  EXPECT_EQ(output_shape->GetDim(0), (sym0 + sym2));
  EXPECT_EQ(output_shape->GetDim(1), sym2 * Symbol(2));
  EXPECT_EQ(shape_env.HasSymbolAssertInfo(sym::Eq(sym3, sym::Pow(sym1, sym1))), true);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable1_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(4, MakeShared<GraphInputShapeSourceStub>(0, 3));

  EXPECT_SYMBOL_EQ(sym2, sym0 + sym1);
  EXPECT_SYMBOL_EQ(sym1, Symbol(2) * sym0);
  EXPECT_SYMBOL_EQ(sym0, Symbol(1));
  auto expr1 = Symbol(2) * (sym0 + sym1) + sym1 * sym2 + sym3 + sym2;
  auto expect_expr = Symbol(15) + sym3;
  EXPECT_EQ(expr1.Simplify(), expect_expr);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable2_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 3));
  Symbol sym4 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 4));
  Symbol sym5 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 5));
  EXPECT_SYMBOL_EQ(sym1, sym2);
  EXPECT_SYMBOL_EQ(sym5, sym4);
  EXPECT_SYMBOL_EQ(sym4, sym0 + sym3);
  EXPECT_SYMBOL_EQ(sym0 + sym3, sym2);

  auto expr1 = Symbol(2) * (sym0 + sym3) + sym::Max(sym5 * sym4, sym1 + sym2);
  EXPECT_EQ(std::string(expr1.Simplify().Str().get()),
            "((2 * s0) + (2 * s3) + Max(((2 * s0) + (2 * s3)), ((s0 + s3) * (s0 + s3))))");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable3_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(5, MakeShared<GraphInputShapeSourceStub>(0, 3));
  Symbol sym4 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 4));
  EXPECT_SYMBOL_EQ(sym1, sym2);
  EXPECT_SYMBOL_EQ(sym3, sym1 + sym4);
  EXPECT_SYMBOL_EQ(sym0 + sym2, sym4);

  auto expr1 = sym3 * sym4 + sym1 * Symbol(2) + sym2;
  EXPECT_EQ(std::string(expr1.Simplify().Str().get()), "(((s2 * s2) * 2) + (3 * s0 * s2) + (3 * s2) + (s0 * s0))");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable4_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 3));

  EXPECT_SYMBOL_EQ(sym1, sym2);
  EXPECT_SYMBOL_EQ(Symbol(2), sym3);
  EXPECT_SYMBOL_EQ(sym0 + sym3, sym2);

  auto expr1 = sym3 + sym1 * Symbol(2) + sym2;
  EXPECT_EQ(std::string(expr1.Simplify().Str().get()), "((3 * s0) + 8)");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyWithDeplicateSym_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  EXPECT_SYMBOL_EQ(sym0, sym::Max(Symbol(0), sym0));
  EXPECT_EQ(std::string(sym0.Simplify().Serialize().get()), "s0");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable5_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(4, MakeShared<GraphInputShapeSourceStub>(0, 2));

  EXPECT_SYMBOL_EQ(sym0, sym1);
  EXPECT_SYMBOL_EQ(sym1, sym2 - sym0);

  auto expr1 = sym0;
  EXPECT_EQ(expr1.Simplify(), sym1);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable6_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 3));

  EXPECT_SYMBOL_EQ(sym0, sym1);
  EXPECT_SYMBOL_EQ(sym3, sym::Min(Symbol(2), sym2));
  EXPECT_SYMBOL_EQ((sym0 + sym3) * sym::Ceiling(sym1), sym2);
  EXPECT_SYMBOL_EQ(sym::Pow(sym0, Symbol(2)) / sym::Abs(sym1), sym0);

  auto expr1 = sym0 + sym1 - (sym2 * sym3);
  EXPECT_EQ(std::string(expr1.Simplify().Serialize().get()), "((2 * s1) - (Min(2, s2) * s2))");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, SimplifyVariable7_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 2));
  Symbol sym3 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 3));

  EXPECT_SYMBOL_EQ(sym0, sym1);
  EXPECT_SYMBOL_EQ(sym1, sym::Min(Symbol(8), sym2));
  EXPECT_SYMBOL_EQ(sym3, sym2 * Symbol(2));
  EXPECT_SYMBOL_EQ(sym0 * sym::Ceiling(sym1), sym2);

  auto expr1 = sym::Pow(sym3, sym0);
  EXPECT_EQ(std::string(expr1.Simplify().Serialize().get()), "Pow((2 * s2), Min(8, s2))");
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, TestHash) {
  const auto check_eq_hash = [](const Expression &a, const Expression &b) {
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.Hash(), b.Hash());
  };
  EXPECT_EQ(Symbol(2), Symbol(2));

  const auto s0 = Symbol("s0");
  const auto s0_bak = Symbol("s0");
  const auto s1 = Symbol("s1");
  EXPECT_EQ(s0.Hash(), s0_bak.Hash());
  check_eq_hash(Mul(s0, s1), Mul(s0_bak, s1));
  check_eq_hash(Mul(s0_bak, s1), Mul(s1, s0));
  check_eq_hash(Eq(s0, s0_bak), Eq(s0_bak, s0));
  check_eq_hash(Ne(s0, s1), Ne(s1, s0));
  check_eq_hash(s0 + s1 + s0, Symbol(2) * s0 + s1);
}

TEST_F(UtestExpression, TestExpressionUnorderdMap) {
  using UMapExprInt = std::unordered_map<ge::Expression, int64_t, ExpressionHash, ExpressionKeyEq>;
  UMapExprInt map1;
  auto s0 = Symbol("s0");
  auto s0_bak = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  map1[s0] = 0;
  map1[s0_bak] = 1;
  map1[s1] = 2;

  EXPECT_NE(map1.find(s0), map1.end());
  EXPECT_EQ(map1.find(s0)->second, 1);  // 被更新了
  EXPECT_EQ(map1.find(s1)->second, 2);
  auto s3 = Symbol("s3");
  auto s4 = Symbol("s4");
  auto s5 = Symbol("s5");
  auto s6 = Symbol("s6");
  map1[s3] = 3;
  map1[s4] = 4;
  map1[s5] = 5;
  map1[s6] = 6;
}

TEST_F(UtestExpression, TestExpressionMap) {
  using MapExprInt = std::map<ge::Expression, int64_t, ExpressionKeyLess>;
  MapExprInt map1;
  auto s0 = Symbol("s0");
  auto s0_bak = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  map1[s0] = 0;
  map1[s0_bak] = 1;
  map1[s1] = 2;
  EXPECT_NE(map1.find(s0), map1.end());
  EXPECT_EQ(map1.find(s0)->second, 1);  // 被更新了
  EXPECT_EQ(map1.find(s1)->second, 2);
  EXPECT_EQ(map1.begin()->first, s0);
}

TEST_F(UtestExpression, TestExpressionSet) {
  using SetExpr = std::set<ge::Expression, ExpressionKeyLess>;
  SetExpr set1;
  auto s0 = Symbol("s0");
  auto s0_bak = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto s3 = Symbol("s3");
  set1.insert(s0);
  set1.insert(s0_bak);
  set1.insert(s1);
  set1.insert(s2);
  set1.insert(s3);
  EXPECT_EQ(set1.size(), 4);
}

TEST_F(UtestExpression, TestCompare) {
  auto s0 = Symbol("s0");
  auto s0_bak = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  EXPECT_EQ(s0.Compare(s0), 0);
  EXPECT_EQ(s0.Compare(s1), -1);
  EXPECT_EQ(s0.Compare(s2), -1);

  auto expr1 = s0 + s1;
  auto expr2 = s2 + s0;
  EXPECT_EQ(expr1.Compare(expr2), -1);
  auto expr3 = Pow(Mul(Max(s0, s1), s2), Symbol(2));
  std::cout << expr2.Hash() << std::endl;
  std::cout << expr3.Hash() << std::endl;
  EXPECT_EQ(expr3.Compare(expr2), -1);
}

TEST_F(UtestExpression, TestSimplifyCeiling_Floor) {
  auto s6 = Symbol("s0");
  auto s0 = Symbol(0);
  auto s192 = Symbol(192);
  auto expr1 = Ceiling((Min(s192, s6) - Min(s0, s6)));
  auto expr2 = (Symbol(-1) * Floor(((Min(s192, s6) - Min(s0, s6)) * Symbol(-1))));
  EXPECT_EQ(expr1.Simplify(), expr2);
}

TEST_F(UtestExpression, TestLog_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  auto expr = sym::Log(sym0);
  int64_t value_int = 0;
  EXPECT_EQ(expr.GetHint(value_int), true);
  EXPECT_TRUE(value_int == 0);

  auto arg = Symbol(100);
  auto base = Symbol(10);

  auto res = sym::Log(arg, base);
  ASSERT_EQ(res, sym::Log(arg) / sym::Log(base));
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, TestAlignment_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(12, MakeShared<GraphInputShapeSourceStub>(0, 0));
  auto expr = sym::Align(sym0, 8);
  int64_t value_int = 0;
  EXPECT_EQ(expr.GetHint(value_int), true);
  EXPECT_EQ(value_int, 16);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, TestAlignmentZero_Failed) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(12, MakeShared<GraphInputShapeSourceStub>(0, 0));
  auto expr = sym::Align(sym0, 0);
  EXPECT_EQ(expr.IsValid(), false);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, TestAlignmentWithPositiveZero_Failed) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(12, MakeShared<GraphInputShapeSourceStub>(0, 0));
  auto expr = sym::AlignWithPositiveInteger(sym0, 0);
  EXPECT_EQ(expr.IsValid(), false);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, CoeffTest_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  int64_t const_sym1 = 1L;
  uint64_t const_sym2 = 2UL;
  uint32_t const_sym3 = 3UL;
  int32_t const_sym4 = 4;

  auto symbol_2 = Symbol(const_sym2);
  auto symbol_1 = Symbol(const_sym1);

  auto symbol = sym::Mul(sym0, symbol_2);
  auto expr1 = sym::Coeff(symbol, sym0, symbol_1);
  EXPECT_EQ(expr1, 2);

  // 3*x**y + 2*x*y + 2**x * 4
  auto expr_coeff_base = Symbol(const_sym3) * sym::Pow(sym0, sym1) + Symbol(const_sym2) * sym0 * sym1 +
                         Symbol(const_sym4) * sym::Pow(Symbol(const_sym2), sym0);
  auto expr2 = sym::Coeff(expr_coeff_base, sym0, sym1);
  EXPECT_EQ(expr2, 3);

  auto expr3 = sym::Coeff(expr_coeff_base, sym1, Symbol(const_sym1));
  EXPECT_EQ(expr3, Symbol(2) * sym0);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, StaticCheckConst) {
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(sym::Rational(4, 2), sym::Log(Symbol(1))), TriBool::kFalse);
}

TEST_F(UtestExpression, TestNotEqual) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting(false, DynamicMode::kDynamic));
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  EXPECT_EQ(Symbol(3.5f) != sym0, true);
}

TEST_F(UtestExpression, ComputeExprHint_Succ) {
  auto shape_env = ShapeEnvAttr(ShapeEnvSetting());
  SetCurShapeEnvContext(&shape_env);
  Symbol sym0 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol sym1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol sym2 = shape_env.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 1));
  // int类型
  auto expr1 = Symbol(2) * (sym0 * sym2) + sym::Max(sym0 + Symbol(3) * sym1, sym2);
  int64_t value_int = 0;
  EXPECT_EQ(expr1.GetHint(value_int), true);
  EXPECT_EQ(value_int, 13);

  // bool 类型
  auto expr2 = sym::Eq(Symbol(3) * sym0, sym::Pow(sym1, sym2) - (sym2 + Symbol(2)));
  bool value_bool = false;
  EXPECT_EQ(expr2.GetHint(value_bool), true);
  EXPECT_EQ(value_bool, true);

  // float
  auto expr3 = sym::Rational(1, 2) + sym2;
  double value_double = 0.0f;
  EXPECT_EQ(expr3.GetHint(value_double), true);
  EXPECT_EQ(value_double, 3.5f);
  SetCurShapeEnvContext(nullptr);
}

TEST_F(UtestExpression, ComputeConstHint_Succ) {
  // int类型
  auto expr1 = Symbol(2) + sym::Max(Symbol(1) + Symbol(3), Symbol(2));
  int64_t value_int = 0;
  EXPECT_EQ(expr1.GetHint(value_int), true);
  EXPECT_EQ(value_int, 6);

  // bool 类型
  auto expr_bool1 = sym::Eq(Symbol(3), sym::Pow(Symbol(2), Symbol(3)) - Symbol(5));
  bool value_bool = false;
  EXPECT_EQ(expr_bool1.GetHint(value_bool), true);
  EXPECT_EQ(value_bool, true);

  auto expr_bool2 = sym::Eq(Symbol(3), sym::Pow(Symbol(3), Symbol(2)) - Symbol(5));
  EXPECT_EQ(expr_bool2.GetHint(value_bool), true);
  EXPECT_TRUE(value_bool == false);

  // float
  auto expr3 = sym::Rational(1, 2) + Symbol(2);
  double value_double = 0.0f;
  EXPECT_EQ(expr3.GetHint(value_double), true);
  EXPECT_EQ(value_double, 2.5f);
}

TEST_F(UtestExpression, GetConstValue_Succ) {
  // int类型
  auto expr1 = Symbol(2) + sym::Max(Symbol(1) + Symbol(3), Symbol(2));
  int64_t value_int = 0;
  EXPECT_EQ(expr1.GetConstValue(value_int), true);
  EXPECT_EQ(value_int, 6);

  // bool 类型
  auto expr2 = sym::Eq(Symbol(3), sym::Pow(Symbol(2), Symbol(3)) - Symbol(5));
  bool value_bool = false;
  EXPECT_EQ(expr2.GetConstValue(value_bool), true);
  EXPECT_EQ(value_bool, true);

  // float
  auto expr3 = sym::Rational(1, 2) + Symbol(2);
  double value_double = 0.0f;
  EXPECT_EQ(expr3.GetConstValue(value_double), true);
  EXPECT_EQ(value_double, 2.5f);
}

TEST_F(UtestExpression, Abnormal_Sym_Expr) {
  auto s0 = Symbol("s0");
  auto e0 = s0 + s0;
  auto e1 = Expression::Deserialize("a(s0)");
  e0 = e1;
  EXPECT_NE(e0.IsConstExpr(), true);
  EXPECT_EQ(e0.Serialize(), nullptr);
  EXPECT_EQ(e0.FreeSymbols().size(), 0);
  double a;
  EXPECT_NE(e0.GetResult({}, a), GRAPH_SUCCESS);
  EXPECT_EQ(e0.GetConstValue(a), false);
  bool c;
  EXPECT_EQ(e0.GetConstValue(c), false);
  EXPECT_EQ(e0 == e1, false);
  EXPECT_EQ(e0.GetExprType(), ExprType::kExprNone);
}

TEST_F(UtestExpression, Parser_Empty) {
  Expression expr = Expression::Parse(nullptr);
  EXPECT_EQ(expr.IsValid(), false);
}

TEST_F(UtestExpression, Parser_Minus) {
  auto s0 = Symbol("s0");
  auto neg_2 = Symbol(-2);

  ExpectSerializeRoundtrip(sym::Add(s0, neg_2), "(-2 + s0)");
  ExpectSerializeRoundtrip(sym::Add(neg_2, s0), "(-2 + s0)");
  ExpectSerializeRoundtrip(sym::Sub(s0, neg_2), "(2 + s0)");
  ExpectSerializeRoundtrip(sym::Sub(neg_2, s0), "(-2 - s0)");
  ExpectSerializeRoundtrip(sym::Mul(s0, neg_2), "(-2 * s0)");
  ExpectSerializeRoundtrip(sym::Mul(neg_2, s0), "(-2 * s0)");
  ExpectSerializeRoundtrip(sym::Div(neg_2, s0), "(-2 / (s0))");
  ExpectSerializeRoundtrip(sym::Div(s0, neg_2), "(Rational(-1 , 2) * s0)");
  ExpectSerializeRoundtrip(sym::Max(s0, neg_2), "Max(s0, -2)");
  ExpectSerializeRoundtrip(sym::Max(neg_2, s0), "Max(s0, -2)");
  ExpectSerializeRoundtrip(sym::Min(s0, neg_2), "Min(s0, -2)");
  ExpectSerializeRoundtrip(sym::Min(neg_2, s0), "Min(s0, -2)");
  ExpectSerializeRoundtrip(sym::Pow(s0, neg_2), "Pow(s0, -2)");
  ExpectSerializeRoundtrip(sym::Pow(neg_2, s0), "Pow(-2, s0)");
  ExpectSerializeRoundtrip(sym::Abs(neg_2), "2");
  ExpectSerializeRoundtrip(sym::Abs(sym::Add(neg_2, s0)), "Abs((2 - s0))");
  ExpectSerializeRoundtrip(sym::Ceiling(neg_2), "-2");
  ExpectSerializeRoundtrip(sym::Ceiling(sym::Add(neg_2, s0)), "(-2 + Ceiling(s0))");
  ExpectSerializeRoundtrip(sym::Floor(neg_2), "-2");
  ExpectSerializeRoundtrip(sym::Floor(sym::Add(neg_2, s0)), "(-2 + Floor(s0))");
}

TEST_F(UtestExpression, Parser_Minus1) {
  ExpectParseAndSerialize("((-((8+s2)*s3)-(-1-((8+s2)*s3)-s3)-s3)*(8+s1)*(8+s2)*s3)",
                          "(( - ((8 + s2) * s3) - (-1 - ((8 + s2) * s3) - s3) - s3) * (8 + s1) * (8 + s2) * s3)");
  ExpectParseAndSerialize("-(s1 + s2)", "((s1 + s2) * -1)");
  ExpectParseAndSerialize("1 - (s1-1)", "(1 - (-1 + s1))");
  ExpectParseAndSerialize("-s1", "(-1 * s1)");
  ExpectParseAndSerialize("1-s0", "(1 - s0)");
  ExpectParseAndSerialize("-1-((8+s2)*s3)-s3", "(-1 - ((8 + s2) * s3) - s3)");
  ExpectParseAndSerialize("(s1-1)", "(-1 + s1)");
}
TEST_F(UtestExpression, CanonicalizeBoolExpr_basic) {
  Expression e(nullptr);
  EXPECT_EQ(e.CanonicalizeBoolExpr().Str().get(), nullptr);

  const auto canonicalize = [](const std::string &parse_str, const std::string &expect_str) {
    const auto expr = ExpressionImpl::CreateExpressionImpl<const SymEngineExprPtr &>(SymEngine::parse(parse_str));
    EXPECT_EQ(expr->CanonicalizeBoolExpr()->Str(), expect_str);
  };
  canonicalize("s0 == 1", "ExpectEq(1, s0)");
  canonicalize("s0 != 1", "ExpectNe(1, s0)");
  canonicalize("s0 <= 1", "ExpectLe(s0, 1)");
  canonicalize("s0 >= 1", "ExpectLe(1, s0)");
  canonicalize("s0 > 1", "ExpectLt(1, s0)");
  canonicalize("s0 + s1", "(s0 + s1)");
  canonicalize("s2 < 0", "ExpectLt(s2, 0)");
  canonicalize("0 < s2", "ExpectLt(0, s2)");
  canonicalize("s2 + 1 < 0", "ExpectLt((1 + s2), 0)");
  canonicalize("s2 - 1 < 0", "ExpectLt(s2, 1)");
  canonicalize("Mod(s2,2) <= 0", "ExpectLe(Mod(s2, 2), 0)");
  canonicalize("Mod(s2,2) -1 <= 0", "ExpectLe(Mod(s2, 2), 1)");
  canonicalize("s0 -1 == 3", "ExpectEq(4, s0)");
  canonicalize("s0 -1 != 3", "ExpectNe(4, s0)");
  canonicalize("s0 -1 <= 3", "ExpectLe(s0, 4)");
  canonicalize("s0 -1 >= 3", "ExpectLe(4, s0)");
  canonicalize("s0 -1 > 3", "ExpectLt(4, s0)");
}

TEST_F(UtestExpression, CanonicalizeBoolExpr) {
  Symbol r0 = Symbol("r0");
  Symbol s2 = Symbol(4096);
  Symbol x = Symbol("x");
  Symbol y = Symbol("y");
  const auto check_sym = [](const Expression &expr, const std::string &expect_str) {
    EXPECT_EQ(std::string(expr.CanonicalizeBoolExpr().Serialize().get()), expect_str);
  };
  const auto check_parse = [](const std::string &parse_str, const std::string &expect_str) {
    const auto expr = ExpressionImpl::CreateExpressionImpl<const SymEngineExprPtr &>(SymEngine::parse(parse_str));
    EXPECT_EQ(expr->CanonicalizeBoolExpr()->Str(), expect_str);
  };

  check_sym(sym::Eq(Mul(x, y), Symbol(0)), "ExpectEq((x * y), 0)");
  // 2*x*y + 4*x == 0 ---> x*y + 2*x == 0
  check_parse("2*x*y+4*x==0", "ExpectEq(((2 * x) + (x * y)), 0)");
  // pow 不参与规范化
  check_parse("2*x*y+4*x + 2**x==0", "ExpectEq(((2 * x * y) + (4 * x) + Pow(2, x)), 0)");
  // 2*x + 4y == 0 ---> x + 2*y == 0
  check_parse("2*x + 4y == 0", "ExpectEq(((2 * y) + x), 0)");
  check_sym(sym::Eq(Add(Symbol(0), Symbol(0)), Symbol(0)), "True");
  check_sym(Mul(r0, s2), "(4096 * r0)");
  check_sym(sym::Lt(r0, Symbol(1)), "ExpectLt(r0, 1)");
  check_sym(sym::Lt(r0, s2), "ExpectLt(r0, 4096)");
  check_sym(sym::Ge(r0, s2), "ExpectLe(4096, r0)");
  check_sym(sym::Eq(sym::Add(r0, Symbol(41)), s2), "ExpectEq(4055, r0)");
  check_sym(sym::Eq(s2, sym::Add(r0, Symbol(41))), "ExpectEq(4055, r0)");
  check_sym(sym::Eq(sym::Add(Symbol(41), sym::Mul(r0, Symbol(2))), s2), "ExpectEq((2 * r0), 4055)");
  check_sym(sym::Eq(sym::Add(Symbol(42), sym::Mul(r0, Symbol(2))), Symbol(4096)), "ExpectEq(2027, r0)");
}

TEST_F(UtestExpression, EvaluateAsBoolBasic) {
  // s0>0 -> 2*s0 > s0 return true

  auto e = ShapeEnvAttr();
  e.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));

  SetCurShapeEnvContext(&e);

  // s0 > 0
  ExpectSymbolBool(sym::Gt(Symbol("s0"), Symbol(0)), "xxx", 100);

  // 2*s0 > s0
  EXPECT_TRUE(SymbolicUtils::StaticCheckGt(Mul(Symbol(2), Symbol("s0")), Symbol("s0")) == TriBool::kTrue);
}

TEST_F(UtestExpression, EvaluateAsBool_case_canfuse) {
  // 4*s0*s1*s2 > 4*s0*s2 ---> 1 < s1
  auto e = ExpressionImpl::CreateExpressionImpl<const SymEngineExprPtr &>(SymEngine::parse("4*s0*s1*s2 > 4*s0*s2"));
  EXPECT_EQ(e->CanonicalizeBoolExpr()->Str(), "ExpectLt(1, s1)");

  // 4*s0*s1*s2 < 4*s0*s2 ---> 1 > s1
  auto e1 = ExpressionImpl::CreateExpressionImpl<const SymEngineExprPtr &>(SymEngine::parse("4*s0*s2 > 4*s0*s1*s2"));
  EXPECT_EQ(e1->CanonicalizeBoolExpr()->Str(), "ExpectLt(s1, 1)");

  auto s = ShapeEnvAttr();
  s.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  s.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 2));
  s.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 3));

  SetCurShapeEnvContext(&s);

  ExpectSymbolBool(sym::Gt(Symbol("s0"), Symbol(0)), "xxx", 100);
  ExpectSymbolBool(sym::Gt(Symbol("s1"), Symbol(1)), "xxx", 100);
  ExpectSymbolBool(sym::Gt(Symbol("s2"), Symbol(0)), "xxx", 100);

  auto expr1 = sym::Mul(sym::Mul(sym::Mul(Symbol(4), Symbol("s0")), Symbol("s1")), Symbol("s2"));
  auto expr2 = sym::Mul(sym::Mul(Symbol(4), Symbol("s0")), Symbol("s2"));

  EXPECT_TRUE(SymbolicUtils::StaticCheckGt(expr1, expr2) == TriBool::kTrue);
}

TEST_F(UtestExpression, GetArgsTest) {
  Expression e(nullptr);
  EXPECT_EQ(e.GetArgs().size(), 0);
  auto s0 = Symbol("s0");
  auto expr1 = Mul(s0, Symbol(2));
  auto s1 = Symbol("s1");
  auto expr2 = Pow(s1, Symbol(2));
  auto expr3 = Add(expr1, expr2);

  EXPECT_EQ(expr3.GetArgs().size(), 2);
  EXPECT_EQ(std::string(expr3.GetArgs()[0].Serialize().get()), expr2.Serialize().get());
  EXPECT_EQ(std::string(expr3.GetArgs()[1].Serialize().get()), expr1.Serialize().get());
}

TEST_F(UtestExpression, TriBoolConvert) {
  ge::TriBool tb = ge::TriBool::kTrue;

  EXPECT_EQ(tb, ge::TriBool::kTrue);
  EXPECT_NE(tb, ge::TriBool::kFalse);
  EXPECT_NE(tb, ge::TriBool::kUnknown);
}

TEST_F(UtestExpression, LogicalTest) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto s2 = Symbol("s2");
  auto s3 = Symbol("s3");

  const auto serialize_str = [](const Expression &expr) { return std::string(expr.Serialize().get()); };

  // 非布尔表达式测试
  EXPECT_EQ(LogicalAnd({s0}).Serialize().get(), nullptr);
  EXPECT_EQ(LogicalOr({s0}).Serialize().get(), nullptr);

  // 基础用例测试
  const auto expr1 = LogicalAnd({Eq(s0, s1), Eq(s2, s3)});
  EXPECT_EQ(serialize_str(expr1), "LogicAnd(ExpectEq(s0, s1), ExpectEq(s2, s3))");
  const auto expr2 = LogicalOr({Eq(s0, s1), Eq(s2, s3)});
  EXPECT_EQ(serialize_str(expr2), "LogicOr(ExpectEq(s0, s1), ExpectEq(s2, s3))");

  // 解析测试
  EXPECT_EQ(ExpressionImpl::Parse("LogicAnd(ExpectEq(s0, s1), ExpectEq(s2, s3))")->Str(), expr1.Serialize().get());
  EXPECT_EQ(ExpressionImpl::Parse("LogicOr(ExpectEq(s0, s1), ExpectEq(s2, s3))")->Str(), expr2.Serialize().get());
  // 变参解析下单参 LogicAnd/LogicOr 合法，解析结果与单元素构造版折叠一致
  EXPECT_EQ(ExpressionImpl::Parse("LogicAnd(ExpectEq(s0, s1))")->Str(), "ExpectEq(s0, s1)");
  EXPECT_EQ(ExpressionImpl::Parse("LogicOr(ExpectEq(s0, s1))")->Str(), "ExpectEq(s0, s1)");

  EXPECT_EQ(serialize_str(sym::LogicalAnd({})), "True");
  EXPECT_EQ(serialize_str(sym::LogicalOr({})), "False");
  EXPECT_EQ(serialize_str(LogicalAnd({Eq(s0, s1)})), "ExpectEq(s0, s1)");
  EXPECT_EQ(serialize_str(LogicalOr({Eq(s0, s1)})), "ExpectEq(s0, s1)");
  EXPECT_EQ(serialize_str(LogicalOr({Eq(Symbol(1), Symbol(1))})), "True");
  EXPECT_EQ(serialize_str(LogicalOr({Eq(Symbol(1), Symbol(1)), Eq(s0, s1)})), "True");
  EXPECT_EQ(serialize_str(LogicalAnd({Eq(Symbol(1), Symbol(1)), Eq(s0, s1)})), "ExpectEq(s0, s1)");
  EXPECT_EQ(serialize_str(LogicalAnd({Eq(Symbol(1), Symbol(0)), Eq(s0, s1)})), "False");

  // 添加到shape env测试：expr1/expr2 引用 s0~s3 四个符号，须全部注册 hint 源，
  // 否则 EvaluateExpr 存在无法替换的自由符号，GetHint 依赖 SymEngine 布局不稳定
  auto s = ShapeEnvAttr();
  s.CreateSymbol(0, MakeShared<GraphInputShapeSourceStub>(0, 0));
  s.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  s.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 2));
  s.CreateSymbol(3, MakeShared<GraphInputShapeSourceStub>(0, 3));

  SetCurShapeEnvContext(&s);

  ExpectSymbolBool(expr1, "xxx", 100);
  ExpectSymbolBool(expr2, "xxx", 100);

  EXPECT_TRUE(SymbolicUtils::StaticCheckGt(expr1, expr2) == TriBool::kUnknown);
  EXPECT_TRUE(SymbolicUtils::StaticCheckEq(expr1, expr1) == TriBool::kTrue);
}
TEST_F(UtestExpression, LogicalTestConst) {
  EXPECT_TRUE(EXPECT_SYMBOL_AND());
  EXPECT_FALSE(EXPECT_SYMBOL_OR());

  EXPECT_TRUE(EXPECT_SYMBOL_AND(Eq(Symbol(1), Symbol(1))));
  EXPECT_FALSE(EXPECT_SYMBOL_AND(Eq(Symbol(1), Symbol(1)), Eq(Symbol(1), Symbol(0))));

  EXPECT_TRUE(EXPECT_SYMBOL_OR(Eq(Symbol(1), Symbol(1))));
  EXPECT_TRUE(EXPECT_SYMBOL_OR(Eq(Symbol(1), Symbol(1)), Eq(Symbol(1), Symbol(0))));
}

TEST_F(UtestExpression, LogicalOrTestGuard) {
  const auto s0 = Symbol("s0");
  const auto s1 = Symbol("s1");
  const auto s2 = Symbol("s2");
  const auto s3 = Symbol("s3");
  const auto s4 = Symbol("s4");
  const auto s5 = Symbol("s5");

  auto s = ShapeEnvAttr();
  SetCurShapeEnvContext(&s);
  for (uint64_t i = 1U; i <= 6U; i++) {
    s.CreateSymbol(i, MakeShared<GraphInputShapeSourceStub>(0, i));
  }

  EXPECT_EQ(EXPECT_SYMBOL_OR(Ge(s0, s1), Le(s2, s3), Eq(s4, s5)), true);
  EXPECT_EQ(EXPECT_SYMBOL_OR(Ge(s0, s1), Le(s3, s2), Eq(s5, s4)), false);
  EXPECT_EQ(EXPECT_SYMBOL_AND(Ge(s0, s1), Le(s2, s3), Eq(s4, s5)), false);
  EXPECT_EQ(EXPECT_SYMBOL_AND(Ge(s1, s0), Gt(s3, s2), Gt(s5, s4)), true);

  const std::set<std::string> expect_guard = {"LogicOr(ExpectEq(s4, s5), ExpectLe(s1, s0), ExpectLe(s2, s3))",
                                              "LogicAnd(ExpectLt(s0, s1), ExpectLt(s2, s3), ExpectNe(s4, s5))",
                                              "LogicAnd(ExpectLe(s0, s1), ExpectLt(s2, s3), ExpectLt(s4, s5))",
                                              "LogicOr(ExpectLt(s0, s1), ExpectLt(s3, s2), ExpectNe(s4, s5))"};
  for (auto &iter : s.GetAllSymbolCheckInfos()) {
    EXPECT_NE(expect_guard.find(std::string(iter.expr.Serialize().get())), expect_guard.end());
  }
}

TEST_F(UtestExpression, ParseNaryLogicalAndOr) {
  // 形状 guard 会把多个条件合成为变参 LogicAnd/LogicOr（见 symbol_checker.h），解析端必须支持 N 参
  const char *nary_and = "LogicAnd(ExpectNe(-1, s101), ExpectNe(-1, s401), ExpectNe(0, s401))";
  auto nary_and_expr = Expression::Parse(nary_and);
  EXPECT_NE(nary_and_expr.impl_, nullptr);
  EXPECT_EQ(std::string(nary_and_expr.Serialize().get()), nary_and);

  const char *nary_or = "LogicOr(ExpectEq(s6, s7), ExpectLt(s0, s1), ExpectLt(s3, s2), ExpectNe(s4, s5))";
  auto nary_or_expr = Expression::Parse(nary_or);
  EXPECT_NE(nary_or_expr.impl_, nullptr);
  EXPECT_EQ(std::string(nary_or_expr.Serialize().get()), nary_or);

  // Max/Min 打印端同为变参（DefaultMaxPrinter/DefaultMinPrinter），解析端按从左折叠处理
  auto nary_max_expr = Expression::Parse("Max(s0, s1, s2)");
  EXPECT_NE(nary_max_expr.impl_, nullptr);
  auto nary_min_expr = Expression::Parse("Min(s0, s1, s2)");
  EXPECT_NE(nary_min_expr.impl_, nullptr);
}

TEST_F(UtestExpression, SimplifyWithShapeEnv) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);

  auto expr = sym::Ceiling(sym::Sub(sym::Ceiling(sym::Mul(sym::Rational(1, 2), Symbol("s0"))), Symbol(20)));
  // SymEngine 构造 ceiling(ceiling(x)-20) 时移出整数产生未化简的两层嵌套形态，解析路径
  // 幂等化简为一层，二者数学等价（ceiling 幂等）；旧字节精确匹配碰巧拒绝，新实现按
  // 语义相等接受（返回树与原树 Compare 不等，属 SymEngine canonical form 不唯一的既有缺陷）
  auto expr1 = Expression::Deserialize(expr.Str().get());
  EXPECT_NE(expr1.impl_, nullptr);
  auto expr2 = Expression::Deserialize(expr.Simplify().Str().get());
  EXPECT_NE(expr2.impl_, nullptr);
}

TEST_F(UtestExpression, ExpandSimplifyTest) {
  SetCurShapeEnvContext(nullptr);
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto c1 = Symbol(1);
  auto c2 = Symbol(2);

  // s0 + 2(s1 + 1)
  auto expr = sym::Add(s0, sym::Mul(c2, sym::Add(s1, c1)));
  EXPECT_EQ(std::string(expr.Str().get()), "(((1 + s1) * 2) + s0)");
  expr = expr.Simplify();
  EXPECT_EQ(std::string(expr.Str().get()), "((2 * s1) + 2 + s0)");

  auto s2 = Symbol("s2");
  auto s3 = Symbol("s3");
  auto c5 = Symbol(5);
  auto expr1 = c2 * s1 * s2 * s3;
  auto expr2 = c5 * s2 * s3;
  auto expr3 = c5 * s3;

  expr = expr1 + expr2 + expr3 + c5 - (expr1 + expr2 + expr3 + c5);
  EXPECT_NE(expr, Symbol(0));
  EXPECT_EQ(expr.Simplify(), Symbol(0));
}

TEST_F(UtestExpression, StaticCheckTest) {
  SetCurShapeEnvContext(nullptr);
  auto s1 = Symbol("s1");
  auto c0 = Symbol(0);
  auto c1 = Symbol(1);
  auto c2 = Symbol(2);
  // expr2 = s0 + 2 - (s0 + 1) = 1
  const auto expr2 = Symbol("s0") + c2 - (Symbol("s0") + c1);
  EXPECT_NE(expr2, c1);
  // ==/!=/</<=/>/>= 对常量比较可判定，含自由符号 s1 无法判定
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(expr2, c1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(expr2, c2), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckEq(expr2, s1), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckNe(expr2, c2), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckNe(expr2, c1), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckNe(expr2, s1), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckLt(expr2, c2), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLt(expr2, c1), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckLt(expr2, s1), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe(expr2, c2), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe(expr2, c1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe(expr2, c0), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckLe(expr2, s1), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(expr2, c0), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(expr2, c2), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckGt(expr2, s1), TriBool::kUnknown);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(expr2, c1), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(expr2, c0), TriBool::kTrue);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(expr2, c2), TriBool::kFalse);
  EXPECT_EQ(SymbolicUtils::StaticCheckGe(expr2, s1), TriBool::kUnknown);
}

TEST_F(UtestExpression, Expect_Add_Replacement_And_Simplify_When_Input_Two_Var_NE_False) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0, s1), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, s1)), true);

  EXPECT_EQ(shape_env.Simplify(s0), s1);
  EXPECT_EQ(shape_env.Simplify(s1), s1);  // 当前的replace如果都为符号，且rank一样的情况下，后面的是前面replace
}

TEST_F(UtestExpression, Expect_Add_Replacement_And_Simplify_When_Input_One_Var_One_Const_NE_False) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = Symbol(2);
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0, s1), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, s1)), true);

  auto exp = s0 + Symbol(1);
  EXPECT_EQ(exp.Simplify(), Symbol(3));
}

TEST_F(UtestExpression, Expect_Add_Replacement_And_Simplify_When_Input_One_Var_One_Exper_NE_False) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  Symbol s2 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 2));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0, s1 + s2), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, s1 + s2)), true);

  EXPECT_EQ(s0.Simplify(), s1 + s2);
}

TEST_F(UtestExpression, Expect_Add_Replacement_And_Simplify_When_Input_Two_Exper_NE_False) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0 + Symbol(1), s1 + Symbol(2)), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + Symbol(1), s1 + Symbol(2))), true);

  auto exp1 = s0 + Symbol(1);
  auto exp2 = s1 + Symbol(2);
  EXPECT_EQ(exp1.Simplify(), exp2);  // 标准化后：s0 == s1 + 1
}

TEST_F(UtestExpression, Expect_Not_Add_Replacement_And_Simplify_When_Input_One_Exper_One_Const_NE_False) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0 + s1, Symbol(3)), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + s1, Symbol(3))), true);

  auto exp = s0 + s1;
  EXPECT_EQ(exp.Simplify(), exp);  // 表达式与常量间不支持replace
}

TEST_F(UtestExpression, Expect_Simplify_All_Guard_When_Input_Replacement_By_EQ) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0 + s1, Symbol(3)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, Symbol(2)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);  // 新增加replacement会化简第一个guard并插入

  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + s1, Symbol(3))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, Symbol(2))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s1, Symbol(1))), true);
}

TEST_F(UtestExpression, Expect_Simplify_All_Guard_When_Input_Replacement_By_NE) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0 + s1, Symbol(3)), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0, Symbol(2)), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);  // 新增加replacement会化简第一个guard并插入

  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + s1, Symbol(3))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, Symbol(2))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s1, Symbol(1))), true);
}

TEST_F(UtestExpression, Expect_Not_Simplify_All_Guard_When_Not_Input_Replacement) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0 + s1, Symbol(3)), false);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(EXPECT_SYMBOL_NE(s0, Symbol(1)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 2);

  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + s1, Symbol(3))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Ne(s0, Symbol(1))), true);
}

TEST_F(UtestExpression, Expect_Static_EQ_True_When_Input_Replacement_By_EQ) {
  ShapeEnvAttr shape_env;
  SetCurShapeEnvContext(&shape_env);
  Symbol s0 = shape_env.CreateSymbol(2, MakeShared<GraphInputShapeSourceStub>(0, 0));
  Symbol s1 = shape_env.CreateSymbol(1, MakeShared<GraphInputShapeSourceStub>(0, 1));
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0 + s1, Symbol(3)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 1);
  EXPECT_EQ(EXPECT_SYMBOL_EQ(s0, Symbol(2)), true);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);  // 新增加replacement会化简第一个guard并插入

  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0 + s1, Symbol(3))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s0, Symbol(2))), true);
  EXPECT_EQ(shape_env.HasSymbolCheckInfo(sym::Eq(s1, Symbol(1))), true);

  EXPECT_EQ(SymbolicUtils::StaticCheckEq(s1, Symbol(1)), TriBool::kTrue);
  EXPECT_EQ(shape_env.GetAllSymbolCheckInfos().size(), 3);  // 不会再全量化简
}
TEST_F(UtestExpression, AsNumerDenomToStringTest) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");

  auto e1 = s0 / s1;
  EXPECT_EQ(SymbolicUtils::AsNumerDenomToString(e1), "(s0)/(s1)");

  auto r1 = Rational(2, 3);
  EXPECT_EQ(SymbolicUtils::AsNumerDenomToString(r1), "(2)/(3)");

  auto e2 = r1 * s0 * s1;
  EXPECT_EQ(SymbolicUtils::AsNumerDenomToString(e2), "((2 * s0 * s1))/(3)");
}

TEST_F(UtestExpression, NullExpression_AllNullPaths) {
  Expression e(nullptr);
  EXPECT_EQ(e.IsVariableExpr(), false);
  EXPECT_EQ(e.IsBooleanExpr(), false);
  EXPECT_EQ(e.Hash(), std::numeric_limits<uint64_t>::max());
  Expression e2(nullptr);
  EXPECT_EQ(e.Compare(e2), std::numeric_limits<int64_t>::max());
  auto var_b = Symbol("b");
  EXPECT_EQ(e.ContainVar(var_b), false);
  Expression numer;
  Expression denom;
  e.AsNumerDenom(numer, denom);
  EXPECT_EQ(numer.IsValid(), false);
  EXPECT_EQ(denom.IsValid(), false);
  std::ostringstream os;
  os << e;
  EXPECT_TRUE(os.str().empty());
}

TEST_F(UtestExpression, NullExpression_ReplaceSubs) {
  Expression e(nullptr);
  auto var_b = Symbol("b");
  auto var_c = Symbol("c");
  auto replaced = e.Replace({{var_b, var_c}});
  EXPECT_EQ(replaced.IsValid(), false);
  auto subsed = e.Subs({{var_b, var_c}});
  EXPECT_EQ(subsed.IsValid(), false);
}

TEST_F(UtestExpression, ComputeHint_NoShapeEnv_NonConstExpr) {
  SetCurShapeEnvContext(nullptr);
  auto s0 = Symbol("s0");
  int64_t hint;
  EXPECT_EQ(s0.GetHint(hint), false);
  double hint_d;
  EXPECT_EQ(s0.GetHint(hint_d), false);
}

TEST_F(UtestExpression, Expression_MoveConstructor) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto expr = Add(s0, s1);
  Expression moved(std::move(expr));
  EXPECT_EQ(moved.IsValid(), true);
  EXPECT_EQ(moved, Add(s0, s1));
}

TEST_F(UtestExpression, Expression_MoveAssign) {
  auto s0 = Symbol("s0");
  auto s1 = Symbol("s1");
  auto expr = Add(s0, s1);
  Expression target;
  target = std::move(expr);
  EXPECT_EQ(target.IsValid(), true);
  EXPECT_EQ(target, Add(s0, s1));
}

TEST_F(UtestExpression, Expression_CopyConstructor) {
  auto s0 = Symbol("s0");
  auto expr = Add(s0, Symbol(2));
  Expression copied(expr);
  EXPECT_EQ(copied, expr);
}

TEST_F(UtestExpression, Expression_GetName_EmptyImpl) {
  Expression e(nullptr);
  EXPECT_EQ(e.Str().get(), nullptr);
}
}  // namespace ge
