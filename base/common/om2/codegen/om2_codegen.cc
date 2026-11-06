/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_codegen.h"
#include "common/helper/om2/om2_utils.h"
#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_context.h"
#include "common/om2/codegen/om2_codegen_model_builder.h"
#include "common/om2/codegen/om2_codegen_utils.h"
#include "common/om2/codegen/var_fixup_table_builder.h"
#include "program_generator.h"
#include "om2_code_printer.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/om2/model_data/om2_package_contants.h"
#include "graph/compute_graph.h"
#include "graph/ge_context.h"
#include "graph/manager/graph_var_manager.h"

namespace ge {

Status Om2Codegen::Om2CodegenAndCompile(const ge::GeModelPtr &ge_model, const gert::GertModelData &model_data,
                                        gert::GertModelDataModel &unit,
                                        std::unique_ptr<gert::GertModelDataFile> &var_weight_slot,
                                        const size_t model_index) const {
  auto &artifacts = unit.runtime->source_artifacts;
  auto &const_metas = unit.constants_config->consts;
  bool has_custom_kernel = !model_data.custom_ops->binaries.empty();

  artifacts.clear();
  const_metas.clear();
  AstContext ast_ctx;
  AstBuildContext ast(ast_ctx);
  Om2CodegenModel codegen_model;
  std::vector<TaskCodeBuilderPtr> task_code_builders;
  GE_ASSERT_SUCCESS(Om2CodegenModelBuilder::CreateTaskCodeBuilders(ge_model, ast, task_code_builders, codegen_model));
  Om2CodegenModelBuilder builder;
  std::vector<gert::GertModelDataConstMeta> tmp_const_metas;
  // 变量槽位元数据与 tmp_const_metas 同形：由 builder 产出到调用方持有的局部量，不进 Om2CodegenModel。
  // 它是 BuildVarInputs → BuildVarFixupTable 之间的纯中间量，无任何生成器消费，也不落盘。
  std::vector<VarSlotMeta> tmp_var_metas;
  GE_ASSERT_SUCCESS(builder.Build(ge_model, task_code_builders, codegen_model, tmp_const_metas, tmp_var_metas));
  const auto artifact_name = Om2Utils::EscapeArtifactName(ge_model->GetName());
  codegen_model.artifact_name = artifact_name;
  // 固化表必须在源码生成之前构建：Codegen 要固化变量元数据与权重数据块内偏移。
  // 槽位下标取自上一步 builder.Build 产出的 var_metas，二者同一编号空间。
  const auto compute_graph = ge_model->GetGraph();
  auto var_manager = VarManager::Instance(GetContext().SessionId());
  if ((var_manager != nullptr) && (compute_graph != nullptr)) {
    GE_ASSERT_SUCCESS(BuildVarFixupTable(*var_manager, compute_graph, tmp_var_metas, compute_graph->GetGraphID(),
                                         codegen_model.var_fixup),
                      "[OM2] Failed to build variable fixup table for model %s", ge_model->GetName().c_str());
  }
  // 数据块单一来源：固化表算出的 weight_data 直接交给归档槽位，由序列化侧原样写入。
  // 名字同时写进 model_meta（归档内 model_meta.json 的 var_weight_file_name），
  // 执行器加载时按名在 data/variables/ 下查找数据源，与 INTERNAL 常量同一套机制。
  // 必须在下面 std::move(codegen_model) 之前取出。
  if (!codegen_model.var_fixup.weight_data.empty()) {
    const auto weight_size = codegen_model.var_fixup.weight_data.size();
    std::unique_ptr<uint8_t[]> weight_buf(new (std::nothrow) uint8_t[weight_size]);
    GE_ASSERT_NOTNULL(weight_buf, "[OM2] Failed to allocate var weight data, size=%zu.", weight_size);
    (void)memcpy_s(weight_buf.get(), weight_size, codegen_model.var_fixup.weight_data.data(), weight_size);
    const auto file_name = gert::FormatOm2Path("%s%zu", gert::OM2_VAR_WEIGHT_FILE_PREFIX, model_index);
    var_weight_slot = std::make_unique<gert::GertModelDataFile>();
    var_weight_slot->file_name = gert::GertMakeStr(file_name);
    var_weight_slot->data = ReadonlyByteBuffer(weight_buf.release(), ConditionalDeleter{true});
    var_weight_slot->data_size = weight_size;
    unit.model_meta->var_weight_file_name = gert::GertMakeStr(file_name);
    // 拷完即释放源：codegen_model 随后被 std::move 进 ProgramGenerator，而两个生成器只消费
    // var_fixup.entries、不读 weight_data 字节。不释放则整块初始数据在 GenerateProgram 与
    // CompileGeneratedCppToSo（含外部编译器子进程，秒级窗口）期间双份驻留。
    std::vector<uint8_t>().swap(codegen_model.var_fixup.weight_data);
  }
  ProgramGenerator generator(ast, task_code_builders, std::move(codegen_model), has_custom_kernel);

  Om2CodePrinter code_printer(artifact_name);
  GE_ASSERT_SUCCESS(generator.GenerateProgram(code_printer));
  std::vector<gert::GertModelDataFile> source_artifacts;
  code_printer.GetOutputFiles(source_artifacts);

  gert::GertModelDataFile so_artifact;
  const Om2CompileInfo compile_info{ge_model->GetName(), artifact_name, ge_model->GetGraph()->GetGraphID()};
  GE_ASSERT_SUCCESS(Om2Utils::CompileGeneratedCppToSo(source_artifacts, compile_info, so_artifact, false),
                    "[OM2] Failed to compile generated C++ to shared library for model %s",
                    ge_model->GetName().c_str());
  GELOGI("[OM2] Model %s has finished generating source code files and compiling to the shared library.",
         ge_model->GetName().c_str());
  artifacts = std::move(source_artifacts);
  artifacts.push_back(std::move(so_artifact));

  for (auto &meta : tmp_const_metas) {
    const_metas.push_back(std::make_unique<gert::GertModelDataConstMeta>(std::move(meta)));
  }
  return SUCCESS;
}
}  // namespace ge
