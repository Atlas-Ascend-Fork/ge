/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "cpp_emitter.h"

#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/emitter/stable_parts/stable_part_provider.h"

namespace ge {
namespace {
void AppendStringRef(StringRef value, std::string &output) {
  if (!value.Empty()) {
    (void)output.append(value.Data(), value.Length());
  }
}

void AppendTypeNameSeparator(StringRef type_spec, std::string &output) {
  if (type_spec.Empty()) {
    return;
  }
  const char last = type_spec.Data()[type_spec.Length() - 1UL];
  if ((last != '*') && (last != '&') && (last != ' ')) {
    (void)output.push_back(' ');
  }
}

const char *AccessLabel(AccessSectionDecl::Kind kind) {
  switch (kind) {
    case AccessSectionDecl::Kind::kPublic:
      return "public";
    case AccessSectionDecl::Kind::kPrivate:
      return "private";
    case AccessSectionDecl::Kind::kProtected:
      return "protected";
    default:
      return "";
  }
}

const char *CastKeyword(CppCastExpr::Kind kind) {
  switch (kind) {
    case CppCastExpr::Kind::kReinterpret:
      return "reinterpret_cast";
    case CppCastExpr::Kind::kConst:
      return "const_cast";
    case CppCastExpr::Kind::kStatic:
      return "static_cast";
    default:
      return "";
  }
}

const char *BinaryOpToken(BinaryExpr::Op op) {
  switch (op) {
    case BinaryExpr::Op::kEq:
      return "==";
    case BinaryExpr::Op::kNe:
      return "!=";
    case BinaryExpr::Op::kLt:
      return "<";
    case BinaryExpr::Op::kLe:
      return "<=";
    case BinaryExpr::Op::kGt:
      return ">";
    case BinaryExpr::Op::kGe:
      return ">=";
    case BinaryExpr::Op::kLogicalAnd:
      return "&&";
    case BinaryExpr::Op::kLogicalOr:
      return "||";
    case BinaryExpr::Op::kAdd:
      return "+";
    case BinaryExpr::Op::kSub:
      return "-";
    case BinaryExpr::Op::kMul:
      return "*";
    case BinaryExpr::Op::kDiv:
      return "/";
    case BinaryExpr::Op::kMod:
      return "%";
    case BinaryExpr::Op::kBitAnd:
      return "&";
    case BinaryExpr::Op::kBitOr:
      return "|";
    case BinaryExpr::Op::kBitXor:
      return "^";
    case BinaryExpr::Op::kShiftLeft:
      return "<<";
    case BinaryExpr::Op::kShiftRight:
      return ">>";
    default:
      return "";
  }
}

const char *UnaryOpToken(UnaryExpr::Op op) {
  switch (op) {
    case UnaryExpr::Op::kLogicalNot:
      return "!";
    case UnaryExpr::Op::kNegate:
      return "-";
    case UnaryExpr::Op::kBitNot:
      return "~";
    case UnaryExpr::Op::kDeref:
      return "*";
    case UnaryExpr::Op::kPreInc:
    case UnaryExpr::Op::kPostInc:
      return "++";
    default:
      return "";
  }
}

bool IsPrefixUnaryOp(UnaryExpr::Op op) {
  return op != UnaryExpr::Op::kPostInc;
}

const char *ContainerMethodName(ContainerMethodExpr::Method method) {
  switch (method) {
    case ContainerMethodExpr::Method::kClear:
      return "clear";
    case ContainerMethodExpr::Method::kResize:
      return "resize";
    case ContainerMethodExpr::Method::kSize:
      return "size";
    case ContainerMethodExpr::Method::kData:
      return "data";
    case ContainerMethodExpr::Method::kEmpty:
      return "empty";
    case ContainerMethodExpr::Method::kAt:
      return "at";
    case ContainerMethodExpr::Method::kPushBack:
      return "push_back";
    case ContainerMethodExpr::Method::kCStr:
      return "c_str";
    case ContainerMethodExpr::Method::kGetPtr:
      return "get";
    default:
      return "";
  }
}

const char *BuiltinTypeToken(BuiltinType type) {
  switch (type) {
    case BuiltinType::kVoid:
      return "void";
    case BuiltinType::kBool:
      return "bool";
    case BuiltinType::kChar:
      return "char";
    case BuiltinType::kInt8:
      return "int8_t";
    case BuiltinType::kUInt8:
      return "uint8_t";
    case BuiltinType::kInt16:
      return "int16_t";
    case BuiltinType::kUInt16:
      return "uint16_t";
    case BuiltinType::kInt32:
      return "int32_t";
    case BuiltinType::kUInt32:
      return "uint32_t";
    case BuiltinType::kInt64:
      return "int64_t";
    case BuiltinType::kUInt64:
      return "uint64_t";
    case BuiltinType::kFloat:
      return "float";
    case BuiltinType::kDouble:
      return "double";
    default:
      return "";
  }
}

void AppendTypeName(const TypeName &type_name, std::string &output) {
  if (std::holds_alternative<BuiltinType>(type_name)) {
    (void)output.append(BuiltinTypeToken(std::get<BuiltinType>(type_name)));
    return;
  }
  AppendStringRef(std::get<StringRef>(type_name), output);
}

Status EmitForInitStmt(const Stmt &node, CppEmitter &emitter, std::string &output) {
  if (const auto *var_decl = dynamic_cast<const VarDeclStmt *>(&node)) {
    AppendStringRef(var_decl->GetTypeSpec(), output);
    AppendTypeNameSeparator(var_decl->GetTypeSpec(), output);
    AppendStringRef(var_decl->GetName(), output);
    if (var_decl->GetInit() != nullptr) {
      if (var_decl->GetInitStyle() == VarDeclStmt::InitStyle::kAssign) {
        (void)output.append(" = ");
      }
      return var_decl->GetInit()->Accept(emitter, output);
    }
    return SUCCESS;
  }
  if (const auto *expr_stmt = dynamic_cast<const ExprStmt *>(&node)) {
    return expr_stmt->GetExpr()->Accept(emitter, output);
  }
  return FAILED;
}

const char *IntSuffixToken(LiteralExpr::IntSuffix suffix) {
  switch (suffix) {
    case LiteralExpr::IntSuffix::kNone:
      return "";
    case LiteralExpr::IntSuffix::kU:
      return "U";
    case LiteralExpr::IntSuffix::kL:
      return "L";
    case LiteralExpr::IntSuffix::kUL:
      return "UL";
    case LiteralExpr::IntSuffix::kULL:
      return "ULL";
    default:
      return "";
  }
}

// 小写十六进制、不带前导零，与 std::hex 的输出一致；不引 <sstream>
void AppendHexLiteral(uint64_t value, std::string &output) {
  output.append("0x");
  if (value == 0U) {
    output.push_back('0');
    return;
  }
  constexpr char kDigits[] = "0123456789abcdef";
  char buffer[sizeof(uint64_t) * 2U] = {0};
  size_t pos = sizeof(buffer);
  while (value != 0U) {
    buffer[--pos] = kDigits[value & 0xFU];
    value >>= 4U;  // NOLINT: 4 位一组即一个十六进制位
  }
  output.append(buffer + pos, sizeof(buffer) - pos);
}

bool IsSpacingBoundaryDecl(const DeclNode &node) {
  return dynamic_cast<const StablePartDecl *>(&node) != nullptr ||
         dynamic_cast<const TypeAliasDecl *>(&node) != nullptr ||
         dynamic_cast<const NamespaceDecl *>(&node) != nullptr ||
         dynamic_cast<const ExternBlockDecl *>(&node) != nullptr || dynamic_cast<const ClassDecl *>(&node) != nullptr ||
         dynamic_cast<const StructDecl *>(&node) != nullptr || dynamic_cast<const EnumDecl *>(&node) != nullptr ||
         dynamic_cast<const FunctionDecl *>(&node) != nullptr || dynamic_cast<const FunctionDef *>(&node) != nullptr ||
         dynamic_cast<const MethodDef *>(&node) != nullptr;
}
}  // namespace

void CppEmitter::AppendIndentAt(size_t level, std::string &output) const {
  size_t count = level;
  while (count > 0U) {
    (void)output.append(indent_unit_);
    --count;
  }
}

void CppEmitter::AppendIndent(std::string &output) const {
  AppendIndentAt(indent_level_, output);
}

Status CppEmitter::EmitParamList(const ArrayRef<ParamDecl *> &params, std::string &output) {
  for (size_t i = 0; i < params.Size(); ++i) {
    if (i > 0) {
      (void)output.append(", ");
    }
    const auto status = params[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  return SUCCESS;
}

Status CppEmitter::EmitFunctionSignature(StringRef return_type, StringRef name, const ArrayRef<ParamDecl *> &params,
                                         std::string &output) {
  if (!return_type.Empty()) {
    AppendStringRef(return_type, output);
    (void)output.append(" ");
  }
  AppendStringRef(name, output);
  (void)output.append("(");
  const auto status = EmitParamList(params, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::EmitDeclBlock(const ArrayRef<DeclNode *> &items, bool separate_items, std::string &output) {
  bool emitted_decl = false;
  bool prev_is_space_decl = false;
  bool prev_is_spacing_boundary_decl = false;
  bool prev_is_namespace_decl = false;
  for (size_t i = 0; i < items.Size(); ++i) {
    if (separate_items && emitted_decl && !prev_is_space_decl && prev_is_spacing_boundary_decl &&
        !prev_is_namespace_decl && IsSpacingBoundaryDecl(*items[i])) {
      (void)output.append("\n");
    }
    const auto status = items[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
    emitted_decl = true;
    prev_is_space_decl = dynamic_cast<const SpaceDecl *>(items[i]) != nullptr;
    prev_is_spacing_boundary_decl = IsSpacingBoundaryDecl(*items[i]);
    prev_is_namespace_decl = dynamic_cast<const NamespaceDecl *>(items[i]) != nullptr;
  }
  return SUCCESS;
}

Status CppEmitter::EmitRecordDecl(const char *keyword, StringRef name, const ArrayRef<DeclNode *> &items,
                                  std::string &output) {
  bool has_access_section = false;
  for (size_t i = 0; i < items.Size(); ++i) {
    if (dynamic_cast<const AccessSectionDecl *>(items[i]) != nullptr) {
      has_access_section = true;
      break;
    }
  }
  (void)output.append(keyword);
  (void)output.append(" ");
  AppendStringRef(name, output);
  (void)output.append(" {\n");
  const uint32_t indent_step = has_access_section ? 2U : 1U;
  if (indent_level_ <= std::numeric_limits<uint32_t>::max() - indent_step) {
    indent_level_ += indent_step;
  }
  const auto status = EmitDeclBlock(items, false, output);
  if (indent_level_ >= indent_step) {
    indent_level_ -= indent_step;
  }
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("};\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ParamDecl &node, std::string &output) {
  AppendStringRef(node.GetTypeSpec(), output);
  AppendTypeNameSeparator(node.GetTypeSpec(), output);
  AppendStringRef(node.GetName(), output);
  return SUCCESS;
}

Status CppEmitter::Emit(const TranslationUnit &node, std::string &output) {
  return EmitDeclBlock(node.GetItems(), true, output);
}

Status CppEmitter::Emit(const StablePartDecl &node, std::string &output) {
  std::string stable_text;
  const auto status = ResolveStablePart(node.GetId(), stable_text);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(stable_text);
  return SUCCESS;
}

Status CppEmitter::Emit(const RawTextDecl &node, std::string &output) {
  // 原文照发，不加缩进：调用方给的文本自带换行与缩进（如内联的转换单元源码）
  AppendStringRef(node.GetText(), output);
  return SUCCESS;
}

Status CppEmitter::Emit(const IncludeDecl &node, std::string &output) {
  (void)output.append("#include ");
  if (node.GetKind() == IncludeDecl::Kind::kAngle) {
    (void)output.push_back('<');
    AppendStringRef(node.GetPath(), output);
    (void)output.append(">\n");
  } else {
    (void)output.push_back('"');
    AppendStringRef(node.GetPath(), output);
    (void)output.append("\"\n");
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const SpaceDecl &node, std::string &output) {
  (void)node;
  (void)output.append("\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const TypeAliasDecl &node, std::string &output) {
  (void)output.append("typedef ");
  AppendStringRef(node.GetTypeSpec(), output);
  AppendTypeNameSeparator(node.GetTypeSpec(), output);
  AppendStringRef(node.GetName(), output);
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const NamespaceDecl &node, std::string &output) {
  (void)output.append("namespace");
  if (!node.GetName().Empty()) {
    (void)output.append(" ");
    AppendStringRef(node.GetName(), output);
  }
  (void)output.append(" {\n");
  const auto status = EmitDeclBlock(node.GetItems(), true, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("} // namespace");
  if (!node.GetName().Empty()) {
    (void)output.append(" ");
    AppendStringRef(node.GetName(), output);
  }
  (void)output.append("\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ExternBlockDecl &node, std::string &output) {
  (void)output.append("#ifdef __cplusplus\n");
  (void)output.append("extern \"");
  AppendStringRef(node.GetLanguage(), output);
  (void)output.append("\" {\n");
  (void)output.append("#endif\n");
  (void)output.append("\n");
  const auto status = EmitDeclBlock(node.GetItems(), true, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("\n");
  (void)output.append("#ifdef __cplusplus\n");
  (void)output.append("}\n");
  (void)output.append("#endif\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ClassDecl &node, std::string &output) {
  return EmitRecordDecl("class", node.GetName(), node.GetItems(), output);
}

Status CppEmitter::Emit(const StructDecl &node, std::string &output) {
  return EmitRecordDecl("struct", node.GetName(), node.GetItems(), output);
}

Status CppEmitter::Emit(const EnumDecl &node, std::string &output) {
  // 与 EmitRecordDecl 一致：关键字前不加缩进（namespace 体本身不缩进），枚举项按层缩进
  (void)output.append("enum ");
  AppendStringRef(node.GetName(), output);
  if (!node.GetUnderlyingType().Empty()) {
    (void)output.append(" : ");
    AppendStringRef(node.GetUnderlyingType(), output);
  }
  (void)output.append(" {\n");
  if (indent_level_ < std::numeric_limits<uint32_t>::max()) {
    indent_level_++;
  }
  const auto names = node.GetNames();
  const auto values = node.GetValues();
  for (size_t i = 0; i < names.Size(); ++i) {
    AppendIndent(output);
    AppendStringRef(names[i], output);
    if (values[i] != nullptr) {
      (void)output.append(" = ");
      const auto status = values[i]->Accept(*this, output);
      if (status != SUCCESS) {
        if (indent_level_ > 0U) {
          indent_level_--;
        }
        return status;
      }
    }
    (void)output.append(",\n");
  }
  if (indent_level_ > 0U) {
    indent_level_--;
  }
  (void)output.append("};\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const AccessSectionDecl &node, std::string &output) {
  const size_t access_level = indent_level_ == 0 ? 0 : indent_level_ - 1UL;
  AppendIndentAt(access_level, output);
  (void)output.append(AccessLabel(node.GetKind()));
  (void)output.append(":\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const FieldDecl &node, std::string &output) {
  AppendIndent(output);
  AppendStringRef(node.GetTypeSpec(), output);
  AppendTypeNameSeparator(node.GetTypeSpec(), output);
  AppendStringRef(node.GetName(), output);
  if (node.GetInit() != nullptr) {
    (void)output.append(" = ");
    const auto status = node.GetInit()->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const MethodDecl &node, std::string &output) {
  AppendIndent(output);
  const auto status = EmitFunctionSignature(node.GetReturnType(), node.GetName(), node.GetParams(), output);
  if (status != SUCCESS) {
    return status;
  }
  AppendStringRef(node.GetTrailingSpec(), output);
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const FunctionDecl &node, std::string &output) {
  const auto status = EmitFunctionSignature(node.GetReturnType(), node.GetName(), node.GetParams(), output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const FunctionDef &node, std::string &output) {
  const auto status = EmitFunctionSignature(node.GetReturnType(), node.GetName(), node.GetParams(), output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(" ");
  return node.GetBody()->Accept(*this, output);
}

Status CppEmitter::Emit(const MethodDef &node, std::string &output) {
  if (!node.GetReturnType().Empty()) {
    AppendStringRef(node.GetReturnType(), output);
    (void)output.append(" ");
  }
  AppendStringRef(node.GetOwner(), output);
  (void)output.append("::");
  AppendStringRef(node.GetName(), output);
  (void)output.append("(");
  auto status = EmitParamList(node.GetParams(), output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  if (node.GetMemberInitNames().Size() > 0U) {
    (void)output.append("\n");
    AppendIndentAt(1, output);
    (void)output.append(": ");
    for (size_t i = 0; i < node.GetMemberInitNames().Size(); ++i) {
      if (i > 0) {
        (void)output.append(", ");
      }
      AppendStringRef(node.GetMemberInitNames()[i], output);
      (void)output.append("(");
      status = node.GetMemberInitExprs()[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
      (void)output.append(")");
    }
    (void)output.append(" ");
  } else {
    (void)output.append(" ");
  }
  return node.GetBody()->Accept(*this, output);
}

Status CppEmitter::Emit(const IdentifierExpr &node, std::string &output) {
  AppendStringRef(node.GetName(), output);
  return SUCCESS;
}

Status CppEmitter::Emit(const LiteralExpr &node, std::string &output) {
  switch (node.GetKind()) {
    case LiteralExpr::Kind::kInt:
      (void)output.append(std::to_string(node.GetIntValue()));
      (void)output.append(IntSuffixToken(node.GetIntSuffix()));
      return SUCCESS;
    case LiteralExpr::Kind::kHex:
      AppendHexLiteral(static_cast<uint64_t>(node.GetIntValue()), output);
      (void)output.append(IntSuffixToken(node.GetIntSuffix()));
      return SUCCESS;
    case LiteralExpr::Kind::kBool:
      (void)output.append(node.GetBoolValue() ? "true" : "false");
      return SUCCESS;
    case LiteralExpr::Kind::kString:
      // 不转义：LiteralExpr 的字符串值契约是"已转义好的字面量内容"，
      // 由调用方负责（见 AstBuildContext::Str 的说明）。此处再转义会把
      // AICPU 二进制 blob 的 \104\000 八进制序列变成 \\104\\000。
      (void)output.push_back('"');
      AppendStringRef(node.GetStringValue(), output);
      (void)output.push_back('"');
      return SUCCESS;
    case LiteralExpr::Kind::kNullptr:
      (void)output.append("nullptr");
      return SUCCESS;
    default:
      return FAILED;
  }
}

Status CppEmitter::Emit(const AssignExpr &node, std::string &output) {
  const auto status = node.GetLhs()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(" = ");
  return node.GetRhs()->Accept(*this, output);
}

Status CppEmitter::Emit(const BinaryExpr &node, std::string &output) {
  if (node.GetNeedParen()) {
    (void)output.append("(");
  }
  auto status = node.GetLhs()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(" ");
  (void)output.append(BinaryOpToken(node.GetOp()));
  (void)output.append(" ");
  status = node.GetRhs()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  if (node.GetNeedParen()) {
    (void)output.append(")");
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const UnaryExpr &node, std::string &output) {
  if (IsPrefixUnaryOp(node.GetOp())) {
    (void)output.append(UnaryOpToken(node.GetOp()));
  }
  const bool need_paren = dynamic_cast<const BinaryExpr *>(node.GetExpr()) != nullptr;
  if (need_paren) {
    (void)output.append("(");
  }
  const auto status = node.GetExpr()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  if (need_paren) {
    (void)output.append(")");
  }
  if (!IsPrefixUnaryOp(node.GetOp())) {
    (void)output.append(UnaryOpToken(node.GetOp()));
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const CallExpr &node, std::string &output) {
  auto status = node.GetCallee()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("(");
  const auto args = node.GetArgs();
  for (size_t i = 0; i < args.Size(); ++i) {
    if (i > 0) {
      (void)output.append(", ");
    }
    status = args[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  if ((args.Size() > 0) && (dynamic_cast<const LambdaExpr *>(args[args.Size() - 1UL]) != nullptr) && !output.empty() &&
      output.back() == '\n') {
    output.pop_back();
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const MakeUniqueArrayExpr &node, std::string &output) {
  (void)output.append("std::make_unique<");
  AppendTypeName(node.GetElemType(), output);
  (void)output.append("[]>(");
  const auto status = node.GetCount()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const ToStrExpr &node, std::string &output) {
  (void)output.append("std::string(");
  const auto status = node.GetExpr()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const MemcpyExpr &node, std::string &output) {
  (void)output.append("std::memcpy(");
  auto status = node.GetDst()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(", ");
  status = node.GetSrc()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(", ");
  status = node.GetSize()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const SizeofExpr &node, std::string &output) {
  (void)output.append("sizeof(");
  const auto status = node.GetExpr()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const RemoveFileExpr &node, std::string &output) {
  (void)output.append("std::remove(");
  const auto status = node.GetPath()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const IgnoreOutputExpr &node, std::string &output) {
  (void)output.append("(void)");
  return node.GetExpr()->Accept(*this, output);
}

Status CppEmitter::Emit(const ContainerMethodExpr &node, std::string &output) {
  auto status = node.GetContainer()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(".");
  (void)output.append(ContainerMethodName(node.GetMethod()));
  (void)output.append("(");
  const auto args = node.GetArgs();
  for (size_t i = 0; i < args.Size(); ++i) {
    if (i > 0) {
      (void)output.append(", ");
    }
    status = args[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const AddrOfExpr &node, std::string &output) {
  (void)output.append("&");
  return node.GetExpr()->Accept(*this, output);
}

Status CppEmitter::Emit(const SubscriptExpr &node, std::string &output) {
  auto status = node.GetBase()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("[");
  status = node.GetIndex()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("]");
  return SUCCESS;
}

Status CppEmitter::Emit(const MemberExpr &node, std::string &output) {
  const auto status = node.GetObject()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(".");
  AppendStringRef(node.GetField(), output);
  return SUCCESS;
}

Status CppEmitter::Emit(const CppArrowMemberExpr &node, std::string &output) {
  const auto status = node.GetObject()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("->");
  AppendStringRef(node.GetField(), output);
  return SUCCESS;
}

Status CppEmitter::Emit(const CppCastExpr &node, std::string &output) {
  (void)output.append(CastKeyword(node.GetKind()));
  (void)output.append("<");
  AppendStringRef(node.GetTargetType(), output);
  (void)output.append(">(");
  const auto status = node.GetExpr()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  return SUCCESS;
}

Status CppEmitter::Emit(const LambdaExpr &node, std::string &output) {
  (void)output.append("[");
  const auto captures = node.GetCaptures();
  for (size_t i = 0; i < captures.Size(); ++i) {
    if (i > 0) {
      (void)output.append(", ");
    }
    AppendStringRef(captures[i], output);
  }
  (void)output.append("](");
  const auto status = EmitParamList(node.GetParams(), output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(")");
  if (!node.GetReturnType().Empty()) {
    (void)output.append(" -> ");
    AppendStringRef(node.GetReturnType(), output);
  }
  (void)output.append(" ");
  return node.GetBody()->Accept(*this, output);
}

Status CppEmitter::Emit(const CCastExpr &node, std::string &output) {
  output.append("(");
  AppendStringRef(node.GetTargetType(), output);
  output.append(")");
  return node.GetExpr()->Accept(*this, output);
}

Status CppEmitter::Emit(const InitListExpr &node, std::string &output) {
  const auto elements = node.GetElements();
  const bool use_paren = (node.GetDelimiter() == InitListExpr::Delimiter::kParen);
  // 括号内不允许尾逗号，故 compact 展开只对花括号生效
  if (node.IsCompact() && !use_paren) {
    output.append("{\n");
    if (indent_level_ < std::numeric_limits<size_t>::max()) {
      indent_level_++;
    }
    for (size_t i = 0; i < elements.Size(); ++i) {
      AppendIndent(output);
      const auto status = elements[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
      output.append(",\n");
    }
    if (indent_level_ > 0U) {
      indent_level_--;
    }
    AppendIndent(output);
    output.append("}");
  } else {
    output.append(use_paren ? "(" : "{");
    for (size_t i = 0; i < elements.Size(); ++i) {
      if (i > 0) {
        output.append(", ");
      }
      const auto status = elements[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
    }
    output.append(use_paren ? ")" : "}");
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const DesignatedInitListExpr &node, std::string &output) {
  const auto names = node.GetNames();
  const auto values = node.GetValues();
  if (node.IsCompact()) {
    output.append("{");
    for (size_t i = 0; i < names.Size(); ++i) {
      if (i > 0) {
        output.append(", ");
      }
      output.append(".");
      AppendStringRef(names[i], output);
      output.append(" = ");
      const auto status = values[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
    }
    output.append("}");
    return SUCCESS;
  }
  output.append("{\n");
  if (indent_level_ < std::numeric_limits<size_t>::max()) {
    indent_level_++;
  }
  for (size_t i = 0; i < names.Size(); ++i) {
    AppendIndent(output);
    output.append(".");
    AppendStringRef(names[i], output);
    output.append(" = ");
    const auto status = values[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
    output.append(",\n");
  }
  if (indent_level_ > 0U) {
    indent_level_--;
  }
  AppendIndent(output);
  output.append("}");
  return SUCCESS;
}

Status CppEmitter::Emit(const CommentStmt &node, std::string &output) {
  AppendIndent(output);
  (void)output.append("// ");
  AppendStringRef(node.GetText(), output);
  (void)output.append("\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const BlankLineStmt &node, std::string &output) {
  (void)node;
  (void)output.append("\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const VarDeclStmt &node, std::string &output) {
  AppendIndent(output);
  AppendStringRef(node.GetTypeSpec(), output);
  AppendTypeNameSeparator(node.GetTypeSpec(), output);
  AppendStringRef(node.GetName(), output);
  if (node.GetInit() != nullptr) {
    // kDirect 由 init 自带定界符（{…} 或 (…)），既不能插 " = " 也不能插空格
    if (node.GetInitStyle() == VarDeclStmt::InitStyle::kAssign) {
      (void)output.append(" = ");
    }
    const auto status = node.GetInit()->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  // init 以换行结尾（lambda、多行块）时吃掉它，否则分号会孤零零另起一行
  if (!output.empty() && output.back() == '\n') {
    output.pop_back();
  }
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ExprStmt &node, std::string &output) {
  AppendIndent(output);
  const auto status = node.GetExpr()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ReturnStmt &node, std::string &output) {
  AppendIndent(output);
  (void)output.append("return");
  if (node.GetValue() != nullptr) {
    (void)output.append(" ");
    const auto status = node.GetValue()->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  (void)output.append(";\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const BlockStmt &node, std::string &output) {
  if (!output.empty() && output.back() == '\n') {
    AppendIndent(output);
  }
  (void)output.append("{\n");
  if (indent_level_ < std::numeric_limits<uint32_t>::max()) {
    indent_level_++;
  }
  const auto statements = node.GetStatements();
  bool in_case_body = false;
  for (size_t i = 0; i < statements.Size(); ++i) {
    if (dynamic_cast<const CaseStmt *>(statements[i]) != nullptr) {
      if (in_case_body) {
        --indent_level_;
      }
      const auto status = statements[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
      if (indent_level_ < std::numeric_limits<uint32_t>::max()) {
        indent_level_++;
      }
      in_case_body = true;
      continue;
    }
    const auto status = statements[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  if (in_case_body) {
    --indent_level_;
  }
  if (indent_level_ > 0U) {
    indent_level_--;
  }
  AppendIndent(output);
  (void)output.append("}\n");
  return SUCCESS;
}

Status CppEmitter::EmitIfBody(const IfStmt &node, std::string &output) {
  (void)output.append("if (");
  auto status = node.GetCond()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(") ");
  status = node.GetThenBlock()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  if (node.GetElseBlock() != nullptr) {
    if (!output.empty() && output.back() == '\n') {
      output.pop_back();
    }
    (void)output.append(" else ");
    // else 块只含一条非预处理 if 时扁平化为 else if，避免产出 else { if (…) {…} }。
    // 递归调用 EmitIfBody 使 else if … else if … 链逐级展开。
    const auto else_stmts = node.GetElseBlock()->GetStatements();
    const auto *nested_if = (else_stmts.Size() == 1U) ? dynamic_cast<const IfStmt *>(else_stmts[0]) : nullptr;
    if ((nested_if != nullptr) && !nested_if->IsPreprocessor()) {
      return EmitIfBody(*nested_if, output);
    }
    status = node.GetElseBlock()->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const IfStmt &node, std::string &output) {
  if (node.IsPreprocessor()) {
    return EmitPreprocessorIf(node, output);
  }
  AppendIndent(output);
  return EmitIfBody(node, output);
}

Status CppEmitter::EmitPreprocessorIf(const IfStmt &node, std::string &output) {
  (void)output.append("#if ");
  auto status = node.GetCond()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("\n");
  const auto then_stmts = node.GetThenBlock()->GetStatements();
  for (size_t i = 0U; i < then_stmts.Size(); ++i) {
    status = then_stmts[i]->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
  }
  if (node.GetElseBlock() != nullptr) {
    (void)output.append("#else\n");
    const auto else_stmts = node.GetElseBlock()->GetStatements();
    for (size_t i = 0U; i < else_stmts.Size(); ++i) {
      status = else_stmts[i]->Accept(*this, output);
      if (status != SUCCESS) {
        return status;
      }
    }
  }
  (void)output.append("#endif\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ForStmt &node, std::string &output) {
  AppendIndent(output);
  (void)output.append("for (");
  auto status = EmitForInitStmt(*node.GetInit(), *this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("; ");
  status = node.GetCond()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append("; ");
  status = node.GetStep()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(") ");
  return node.GetBody()->Accept(*this, output);
}

Status CppEmitter::Emit(const RangeForStmt &node, std::string &output) {
  AppendIndent(output);
  (void)output.append("for (");
  AppendStringRef(node.GetTypeSpec(), output);
  AppendTypeNameSeparator(node.GetTypeSpec(), output);
  AppendStringRef(node.GetName(), output);
  (void)output.append(" : ");
  const auto status = node.GetRange()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  (void)output.append(") ");
  return node.GetBody()->Accept(*this, output);
}

Status CppEmitter::Emit(const CaseStmt &node, std::string &output) {
  // Remove trailing newline from previous statement for proper label formatting
  if (!output.empty() && output.back() == '\n') {
    output.pop_back();
  }
  output.append("\n");
  AppendIndent(output);
  if (node.GetValue() != nullptr) {
    output.append("case ");
    const auto status = node.GetValue()->Accept(*this, output);
    if (status != SUCCESS) {
      return status;
    }
    output.append(":");
  } else {
    output.append("default:");
  }
  output.append("\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const BreakStmt &node, std::string &output) {
  (void)node;
  AppendIndent(output);
  output.append("break;\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const ContinueStmt &node, std::string &output) {
  (void)node;
  AppendIndent(output);
  output.append("continue;\n");
  return SUCCESS;
}

Status CppEmitter::Emit(const TryCatchStmt &node, std::string &output) {
  AppendIndent(output);
  (void)output.append("try ");
  auto status = node.GetTryBody()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  // 与 IfStmt 的 else 同一手法：吃掉块尾换行后接 catch，BlockStmt 见末字符非 '\n' 便不再缩进
  if (!output.empty() && output.back() == '\n') {
    output.pop_back();
  }
  (void)output.append(" catch (...) ");
  status = node.GetCatchBody()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  return SUCCESS;
}

Status CppEmitter::Emit(const SwitchStmt &node, std::string &output) {
  AppendIndent(output);
  output.append("switch (");
  const auto status = node.GetCond()->Accept(*this, output);
  if (status != SUCCESS) {
    return status;
  }
  // Delegate to BlockStmt which emits { ... }
  output.append(") ");
  return node.GetBody()->Accept(*this, output);
}
}  // namespace ge
