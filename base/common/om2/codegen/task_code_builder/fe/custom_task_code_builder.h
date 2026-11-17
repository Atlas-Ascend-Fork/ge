/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_GENERATOR_FE_CUSTOM_TASK_CODE_BUILDER_H_
#define AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_GENERATOR_FE_CUSTOM_TASK_CODE_BUILDER_H_

#include "common/om2/codegen/task_code_builder/task_code_builder.h"
#include "common/om2/codegen/task_args_manager/om2_task_args_io_addrs_updater.h"
#include "graph/custom_op/args_refresh.h"

namespace ge {
struct CustomBuildData {
  std::vector<OpArgDesc> ordered_args;
  KernelTaskSemantic semantic{};
  CustomKernelSemantic custom_kernel;
  // B1 解析出的有效刷新策略。仅 kUpdateCallback 进入 Callback 新增接线，其余沿用原路由。
  ArgsRefreshStrategy args_refresh_strategy{ArgsRefreshStrategy::kNone};
  // kOpDefs 稳定下标，由 Contribute 从 context 取得。RenderOpDefTableFields 无 context 入参，故须缓存。
  // 不可用 args_table_entry->table_index（即 args_idx）替代：后者是普通参数表下标，语义为 dump 上报。
  uint32_t task_index{0U};
  // B3 产出：与运行期 io_tensors 下标严格对应（仅含产生 io_tensor 的参数项，workspace 与
  // optional-empty 不在内）。非 Callback 任务恒为空。
  std::vector<TensorRefreshInfo> tensor_refresh_infos;
};

class CustomTaskCodeBuilder : public TaskCodeBuilder {
  static constexpr const char *kDispatchFuncName = "DispatchCustomKernel";

 public:
  using TaskCodeBuilder::TaskCodeBuilder;

  // ── Public overrides & accessors ──
  int64_t ParseOpIndex(const domi::TaskDef &task_def) override;
  Status ParseTaskRunParam(const domi::TaskDef &task_def, const om2::RuntimeParam &rts_param, OpDescPtr op_desc,
                           om2::TaskRunParam &task_run_param) override;
  Status Init(const domi::TaskDef &task_def, std::vector<om2::MemAllocation> &logical_mem_allocations,
              const om2::PisToArgs &args, const om2::IowAddrs &iow_addrs) override;
  Status Contribute(TaskSemanticContributeContext &context) override;
  Status RenderDistHelper(std::vector<DeclNode *> &items) override;
  Status RenderOpDefTableFields(std::vector<std::pair<std::string, Arg>> &fields) override;
  std::string GetFuncName() const override;

 private:
  // ── Build data assembly ──
  Status RenderDispatchCustomKernel(const VarRef &op, const VarRef &ctx, std::vector<DeclNode *> &items) const;
  std::vector<BodyItem> RenderDispatchSetup(const VarRef &op, const VarRef &ctx) const;
  BodyItem RenderDispatchLoop(const VarRef &op, const VarRef &ctx) const;
  std::vector<BodyItem> RenderDistribution(const VarRef &op, const VarRef &ctx) const;
  std::vector<BodyItem> HandleExecuteCallback(const VarRef &op, const VarRef &ctx) const;
  std::vector<BodyItem> HandleInputOutputArg(const VarRef &a, const VarRef &ctx) const;
  void AssignTaskLocalIoNames();
  void InitArgsTableEntry(const TaskSemanticContributeContext &context, const uint64_t args_size);
  Status BuildCustomKernelIrInfo(const OpDescPtr &op_desc);

  // ── Callback 地址刷新（B1/B3/B4）──
  Status ResolveArgsRefreshStrategy(const OpDescPtr &op_desc, ArgsRefreshStrategy &strategy) const;
  Status BuildTensorRefreshInfos(const TaskSemanticContributeContext &context);

  // ── Member variables ──
  CustomBuildData build_data_;
  OpDispatchType::Value dispatch_type_{OpDispatchType::DISPATCH_CUSTOM_KERNEL};
  OpDescPtr op_desc_{nullptr};
  // B2 中间态：Init 收到的最终 I/O 地址与刷新属性，按"全部输入、全部输出"排列（不含 workspace）。
  // 与 ArgsIoAddrsUpdater 内部记录同序，供 Contribute 期 B3 来源转换消费。
  std::vector<uint64_t> callback_io_addrs_;
  std::vector<uint8_t> callback_io_refreshable_;
  om2::ArgsIoAddrsUpdater args_io_addrs_updater_;
};
}  // namespace ge

#endif  // AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_GENERATOR_FE_CUSTOM_TASK_CODE_BUILDER_H_
