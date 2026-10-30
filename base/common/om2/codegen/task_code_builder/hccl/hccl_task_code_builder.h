/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_BUILDER_HCCL_HCCL_TASK_CODE_BUILDER_H_
#define AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_BUILDER_HCCL_HCCL_TASK_CODE_BUILDER_H_

#include "common/om2/codegen/task_code_builder/task_code_builder.h"
#include "common/om2/codegen/om2_codegen_types.h"
#include "common/om2/codegen/task_args_manager/om2_task_args_io_addrs_updater.h"

namespace ge {
struct HcclTaskData {
  std::string hccl_type;                     // KernelHcclDef.hccl_type（如 "HcomAllReduce"/"HorovodAllreduce"）
  std::string op_type;                       // 算子类型字符串（dump 任务信息上报）
  std::vector<uint8_t> private_def;          // TaskDef.private_def 原样字节
  int64_t used_stream_num = 0;               // op attr "used_stream_num"（仅 HCOM 类算子生效）
  int32_t root_id = 0;                       // op attr "root_rank"（仅 Broadcast/Gather/Reduce/HVD-Broadcast 解析）
  int32_t hccl_data_type = 0;                // HCCL 数据类型枚举数值（编译期转换）
  int32_t hccl_reduce_op = 0;                // HCCL 归约类型枚举数值（编译期转换，非 Reduce 类固化 SUM）
  uint64_t count = 0U;                       // 编译期按 hccl_type 分支解析（对齐 GetHcclCount 全规则）
  uint64_t workspace_size = 0U;              // op workspace bytes[0]
  std::vector<int64_t> attached_stream_ids;  // op attached 从流逻辑 id
  bool is_hcom_op = false;                   // 对齐 HcomOmeUtil::IsHCOMOp（从流段仅 HCOM 类生成）
  uint8_t need_refresh = 0U;                 // args 槽位间接寻址使能（Init 布局遍按 IO 可刷新性置位）
  // HVD（Horovod）多 info 支持（每输入一条 meta）
  bool is_hvd = false;                         // IsHorovodOpType（HorovodAllreduce/Allgather/Broadcast/Wait）
  bool is_hvd_wait = false;                    // HVDWAIT：无输入输出纯同步（仅 1 条 info 记 hccl_type）
  std::vector<std::string> hvd_input_names;    // 每输入 GetInputNameByIndex(i)（TF tensor 名，回调对接）
  std::vector<std::vector<int64_t>> hvd_dims;  // 每输入 shape dims
  std::vector<int32_t> hvd_dtypes;             // 每输入 HCCL 数据类型枚举（编译期转换）
  std::vector<uint64_t> hvd_counts;            // 每输入 count（GetHorovodCount 规则）
  // 零拷贝（proto flag 槽位化）
  std::vector<int32_t> input_zero_copy_flags;   // KernelHcclDef.input_zero_copy_flag（数量校验=input_num）
  std::vector<int32_t> output_zero_copy_flags;  // KernelHcclDef.output_zero_copy_flag（数量校验=output_num）
  bool is_collective = false;                   // IsCollectiveCommType（LoadTask 后 aclrtCacheLastTaskExtendInfo）
  bool has_global_workspace = false;            // op 含 GLOBALWORKSPACE_TYPE attr（溢出 global workspace）
  std::vector<AddrSemantic> input_addrs;
  std::vector<AddrSemantic> output_addrs;
  std::vector<AddrSemantic> workspace_addrs;
  std::vector<OpArgDesc> ordered_args;  // input->output->workspace 指针槽位
  std::optional<ArgsTableEntrySemantic> args_table_entry;
};

class HcclTaskCodeBuilder : public TaskCodeBuilder {
  static constexpr const char *kDispatchFuncName = "DispatchHccl";

 public:
  using TaskCodeBuilder::TaskCodeBuilder;
  std::string GetFuncName() const override;
  Status Contribute(TaskSemanticContributeContext &context) override;
  Status RenderDistHelper(std::vector<DeclNode *> &items) override;
  Status RenderOpDefTableFields(std::vector<std::pair<std::string, Arg>> &fields) override;
  Status ParseTaskRunParam(const domi::TaskDef &task_def, const om2::RuntimeParam &rts_param, OpDescPtr op_desc,
                           om2::TaskRunParam &task_run_param) override;
  Status Init(const domi::TaskDef &task_def, std::vector<om2::MemAllocation> &logical_mem_allocations,
              const om2::PisToArgs &args, const om2::IowAddrs &iow_addrs) override;
  Status GetTaskArgsRefreshInfos(std::vector<om2::TaskArgsRefreshInfo> &infos) override;
  int64_t ParseOpIndex(const domi::TaskDef &task_def) override;

 private:
  void InitArgsTableEntry(const TaskSemanticContributeContext &context, const uint64_t args_size);
  Status ParseHcclAttrs(const domi::TaskDef &task_def, const OpDescPtr &op_desc);
  Status ParseHorovodAttrs(const OpDescPtr &op_desc);
  Status ParseZeroCopyFlags(const domi::TaskDef &task_def, const OpDescPtr &op_desc);
  Status ParseHcclDataType(const OpDescPtr &op_desc);
  Status ParseHcclReduceOp(const OpDescPtr &op_desc);
  Status ParseHcclRootId(const OpDescPtr &op_desc);
  Status ComputeHcclCount(const OpDescPtr &op_desc);
  Status AssembleOrderedArgs();
  Arg RenderPrivateDefArg() const;
  Arg RenderAttachedStreamIdsArg() const;
  Arg RenderZeroCopyFlagsArg(const std::vector<int32_t> &flags) const;
  Arg RenderHvdMetasArg() const;

  std::vector<BodyItem> RenderIoAddrResolve(ExprRef hccl, const VarRef &ctx) const;
  std::vector<BodyItem> RenderTaskParamsAssemble(ExprRef hccl, const VarRef &ctx, const VarRef &iow_addrs) const;
  std::vector<BodyItem> RenderFollowStreams(ExprRef hccl, const VarRef &ctx, const VarRef &params) const;
  std::vector<BodyItem> RenderLaunch(ExprRef hccl, const VarRef &ctx, const VarRef &params, const VarRef &op) const;

  HcclTaskData build_data_;
  OpDescPtr op_desc_ = nullptr;
  om2::ArgsIoAddrsUpdater args_io_addrs_updater_;
};
}  // namespace ge

#endif  // AIR_CXX_BASE_COMMON_OM2_CODEGEN_TASK_CODE_BUILDER_HCCL_HCCL_TASK_CODE_BUILDER_H_
