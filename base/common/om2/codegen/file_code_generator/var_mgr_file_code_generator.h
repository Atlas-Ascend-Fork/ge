/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_MGR_FILE_CODE_GENERATOR_H_
#define AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_MGR_FILE_CODE_GENERATOR_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "common/om2/codegen/code_generator_base.h"
#include "common/om2/codegen/om2_codegen_types.h"

namespace ge {
// 生成 <model>_var_mgr.cpp：编译期固化的变量元数据表与三步变量准备
// （格式转换 → 变量间拷贝 → 全量地址表），以及内嵌的转换单元与线程池。
//
// 生成条件是 HasVariables()，故文件内没有"按变量有无"的裁剪——三步准备、线程池与转换入口
// 恒定发射，kVarNum 恒 >= 1。
//
// 转换实现由 VarTransFileCodeGenerator 产出为若干段拼在本文件内，与变量准备同处一个 TU：
// 两者门控相同，分两个 TU 只会让重头文件被预处理两遍并迫使本类 extern 声明入口；
// 同 TU 内段序即依赖序，无需前置声明。
//
// 变量设备地址一律惰性申请：GetVarDevAddr 是唯一与容器对话的入口，容器侧幂等。
// SO 不留地址缓存，故没有前置的全量解析阶段。
class VarMgrFileCodeGenerator : public CodeGeneratorBase {
 public:
  VarMgrFileCodeGenerator(AstBuildContext &ast, std::string artifact_name, const VarFixupTable &table)
      : CodeGeneratorBase(ast), artifact_name_(std::move(artifact_name)), table_(table) {}
  ~VarMgrFileCodeGenerator() override = default;

  // 委托 om2_codegen_types.h 里的唯一判据。必须写 ge:: 限定：类成员名会遮蔽同名的
  // 命名空间级函数，无限定时按参数个数匹配不上本成员即编译失败。
  bool HasVariables() const {
    return ge::HasVariables(table_);
  }
  // 构建整个文件的 AST。失败路径只有一条：内嵌单元不在资源表里（构建配置错误）。
  Status BuildFile(DeclNode *&file) const;

 private:
  // include 并集一次发射。hoisted 是 EmbedUnits 从内嵌单元剥下并收集的 angle include 头名，
  // 与本类自身直接依赖的头取并集后按字典序发射 ⇒ 产物只有一处 include 站点、同一头不重复。
  std::vector<DeclNode *> BuildIncludes(const std::set<std::string> &hoisted) const;
  // 线程池本体：逐字内嵌 runtime/om2/om2_thread_pool.{h,cc}，理由同 source_embed_util.h。
  Status BuildThreadPoolUnits(std::string &text, std::set<std::string> &angle_includes) const;
  DeclNode *BuildVarTransKindEnum() const;
  StructDecl *BuildTransNodeStruct() const;
  StructDecl *BuildVarMetaStruct() const;
  StructDecl *BuildVarAddrStruct() const;
  std::vector<DeclNode *> BuildTransRoadTables() const;
  // 是否有条目需要转换。三条静态判据已在构表期折叠进 trans_road（见 var_fixup_table_builder.cc
  // 的 BuildFixupEntry），故这里只问"有没有非空的链"，不再复算判据、也不再需要候选下标数组。
  // 唯一用途是决定 BuildTransAllVarData 要不要发射遍历循环。
  bool HasAnyTransRoad() const;
  std::vector<DeclNode *> BuildConstants() const;
  DeclNode *BuildVarMetaTable() const;
  Arg BuildEntryRow(const VarFixupEntry &entry, size_t road_index) const;
  // 变量准备各函数。发射顺序即产物内的定义顺序，也就是调用依赖顺序（C++ 要求先定义后使用）。
  FunctionDef *BuildGetVarDevAddr() const;
  FunctionDef *BuildCopyVarFromDevice() const;
  FunctionDef *BuildCommitVarData() const;
  FunctionDef *BuildTransVarOnHost() const;
  FunctionDef *BuildTransSingleVarData() const;
  FunctionDef *BuildTransAllVarData() const;
  // 转换的串/并行分派。两条路径共用 TransSingleVarData，避免同一套转换逻辑发射两遍而日后走偏。
  std::vector<BodyItem> BuildTransDispatch() const;
  FunctionDef *BuildCopyVarData() const;
  FunctionDef *BuildFillVarAddrTable() const;
  FunctionDef *BuildGetSlotNum() const;
  FunctionDef *BuildPrepareVars() const;
  // 数据整形：把固化表的值转成 AST 实参
  Arg DimsArg(const std::vector<int64_t> &dims) const;
  Arg OptionalKeyArg(const std::string &key) const;

  std::string artifact_name_;
  const VarFixupTable &table_;
};
}  // namespace ge

#endif  // AIR_CXX_BASE_COMMON_OM2_CODEGEN_FILE_CODE_GENERATOR_VAR_MGR_FILE_CODE_GENERATOR_H_
