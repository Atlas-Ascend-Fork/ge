/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_TRANS_FILE_CODE_GENERATOR_H_
#define AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_TRANS_FILE_CODE_GENERATOR_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "common/om2/codegen/code_generator_base.h"
#include "common/om2/codegen/om2_codegen_types.h"
#include "graph/types.h"

namespace ge {
// 产出 <model>_var_mgr.cpp 内的转换部分：把格式转换与数据类型转换的源码按依赖序文本内联，
// 末尾追加内部转换入口，供同一文件内 gert::var_mgr 的 TransAllVarData 直接调用。
//
// 本类只产出片段、不产出独立文件，由 VarMgrFileCodeGenerator 拼进 <model>_var_mgr.cpp
// （同处一个 TU 而非拆成两个的理由见该类文档）。能力差异体现在内嵌哪些单元、入口是真实体
// 还是桩，不体现在是否产出。
//
// 下面各判据一律扫描全表条目，这与"本模型实际会转换哪些条目"严格同域：不满足转换判据的条目
// 在构表期就不带 trans_road（见 var_fixup_table_builder.cc 的 BuildFixupEntry），故全表扫描
// 即候选集扫描，无需再传一份候选下标，也不存在两处判据走偏的余地。
//
// 内嵌单元本质是源文件的文本拼接、无法 AST 化，故经 RawTextDecl 承载；适配头与两个入口走
// AST。include 由调用方统一发射，本类不产出，避免同 TU 内重复。内部入口不导出，故不参与
// <model>_internal.h 的声明。
class VarTransFileCodeGenerator : public CodeGeneratorBase {
 public:
  VarTransFileCodeGenerator(AstBuildContext &ast, std::string artifact_name, const VarFixupTable &table)
      : CodeGeneratorBase(ast), artifact_name_(std::move(artifact_name)), table_(table) {}
  ~VarTransFileCodeGenerator() override = default;

  // 把转换部分按依赖序压入 items：适配头 → 内嵌单元原文 → gert::var_trans 两个入口。
  // 转换链里出现无单元注册的格式对时返回失败——编译期即拦下，不留到加载期报
  // ACL_ERROR_GE_FORMAT_INVALID。剥掉的 angle include 头名收进 angle_includes 交调用方发射。
  Status BuildSegments(std::vector<DeclNode *> &items, std::set<std::string> &angle_includes) const;

 private:
  // 转换链中存在格式转换节点
  bool NeedFormatCapability() const;
  // 转换链中存在 CAST 节点，或拷贝源与目标数据类型不同
  bool NeedDataTypeCapability() const;
  // 收集转换链中全部 kFormat 节点的主格式对，以及这些对所属单元的依赖闭包
  Status CollectFormatUnits(std::set<std::string> &needed) const;
  // 按依赖序选出需要内联的源文件（相对仓库根的路径，即资源表的键）
  Status SelectUnits(std::vector<std::string> &units) const;
  // 把选中的单元逐个剥掉内部 include、加上单元边界注释后拼成一段原文。内嵌源缺失即失败：
  // 否则产物照常生成，缺码要延迟到用户机器上编 SO 时才以晦涩的未定义符号暴露。
  Status BuildUnitsText(const std::vector<std::string> &units, std::string &text,
                        std::set<std::string> &angle_includes) const;
  // gert::var_trans 的两个入口。能力缺失时发桩：签名必须与真实体一致，否则与 var_mgr 的调用不匹配
  FunctionDef *BuildTransFormatEntry(bool need_format) const;
  FunctionDef *BuildTransDataTypeEntry(bool need_data_type) const;
  ExprRef PrimaryFormatOf(const VarRef &format) const;
  ExprRef SubFormatOf(const VarRef &format) const;
  ExprRef C0ValueOf(const VarRef &format) const;

  std::string artifact_name_;
  const VarFixupTable &table_;
};
}  // namespace ge

#endif  // AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_TRANS_FILE_CODE_GENERATOR_H_
