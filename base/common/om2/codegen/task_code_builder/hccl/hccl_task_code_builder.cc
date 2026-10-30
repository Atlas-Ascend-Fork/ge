/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "hccl_task_code_builder.h"
#include <cinttypes>
#include <map>
#include <string>
#include "hccl/base.h"
#include "common/om2/codegen/om2_codegen_utils.h"
#include "common/om2/codegen/task_code_builder/task_code_builder_util.h"
#include "common/om2/codegen/task_code_builder_factory.h"
#include "common/om2/codegen/om2_model_utils.h"
#include "common/om2/codegen/task_args_manager/om2_model_args_utils.h"
#include "framework/common/framework_types_internal.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
namespace {
constexpr uint64_t kAddrLen = sizeof(uint64_t);
constexpr int64_t kHcclAlignSize = 512;

const std::map<HorovodReduceOp, HcclReduceOp> kHorovodRedOpToHcclRedOp = {
    {HOROVOD_REDUCE_SUM, HCCL_REDUCE_SUM},
    {HOROVOD_REDUCE_MIN, HCCL_REDUCE_MIN},
    {HOROVOD_REDUCE_MAX, HCCL_REDUCE_MAX},
    {HOROVOD_REDUCE_PROD, HCCL_REDUCE_PROD},
    {HOROVOD_REDUCE_RESERVED, HCCL_REDUCE_RESERVED}};

const std::map<ge::DataType, HcclDataType> kGeToHcclDataType = {
    {ge::DT_FLOAT, HCCL_DATA_TYPE_FP32},          {ge::DT_FLOAT16, HCCL_DATA_TYPE_FP16},
    {ge::DT_DOUBLE, HCCL_DATA_TYPE_FP64},         {ge::DT_BOOL, HCCL_DATA_TYPE_INT8},
    {ge::DT_INT8, HCCL_DATA_TYPE_INT8},           {ge::DT_INT16, HCCL_DATA_TYPE_INT16},
    {ge::DT_INT32, HCCL_DATA_TYPE_INT32},         {ge::DT_INT64, HCCL_DATA_TYPE_INT64},
    {ge::DT_UINT8, HCCL_DATA_TYPE_UINT8},         {ge::DT_UINT16, HCCL_DATA_TYPE_UINT16},
    {ge::DT_UINT32, HCCL_DATA_TYPE_UINT32},       {ge::DT_UINT64, HCCL_DATA_TYPE_UINT64},
    {ge::DT_BF16, HCCL_DATA_TYPE_BFP16},          {ge::DT_HIFLOAT8, HCCL_DATA_TYPE_HIF8},
    {ge::DT_FLOAT8_E5M2, HCCL_DATA_TYPE_FP8E5M2}, {ge::DT_FLOAT8_E4M3FN, HCCL_DATA_TYPE_FP8E4M3}};

bool IsRootOpType(const std::string &op_type) {
  return (op_type == HCOMBROADCAST) || (op_type == HCOMGATHER) || (op_type == HVDCALLBACKBROADCAST) ||
         (op_type == HCOMREDUCE);
}

bool IsReduceOpType(const std::string &op_type) {
  return (op_type == HCOMALLREDUCE) || (op_type == HCOMREDUCESCATTER) || (op_type == HCOMREDUCESCATTERV) ||
         (op_type == HVDCALLBACKALLREDUCE) || (op_type == HCOMREDUCE);
}

Status MapToHcclReduceOp(const int64_t reduce_op, int32_t &hccl_reduce_op) {
  const auto iter = kHorovodRedOpToHcclRedOp.find(static_cast<HorovodReduceOp>(reduce_op));
  GE_ASSERT_TRUE(iter != kHorovodRedOpToHcclRedOp.end(), "[OM2] unsupported horovod reduce op [%ld].",
                 static_cast<long>(reduce_op));
  hccl_reduce_op = static_cast<int32_t>(iter->second);
  return SUCCESS;
}

Status MapToHcclDataType(const ge::DataType ge_data_type, int32_t &hccl_data_type) {
  const auto iter = kGeToHcclDataType.find(ge_data_type);
  GE_ASSERT_TRUE(iter != kGeToHcclDataType.end(), "[OM2] unsupported ge data type %d for hccl task.",
                 static_cast<int32_t>(ge_data_type));
  hccl_data_type = static_cast<int32_t>(iter->second);
  return SUCCESS;
}

Status MapToHcclReduceOp(const std::string &reduce_op, int32_t &hccl_reduce_op) {
  if (reduce_op == "min") {
    hccl_reduce_op = static_cast<int32_t>(HCCL_REDUCE_MIN);
  } else if (reduce_op == "max") {
    hccl_reduce_op = static_cast<int32_t>(HCCL_REDUCE_MAX);
  } else if (reduce_op == "prod") {
    hccl_reduce_op = static_cast<int32_t>(HCCL_REDUCE_PROD);
  } else if (reduce_op == "sum") {
    hccl_reduce_op = static_cast<int32_t>(HCCL_REDUCE_SUM);
  } else {
    GE_ASSERT_TRUE(false, "[OM2] unsupported hccl reduce op [%s].", reduce_op.c_str());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

int64_t GetAlignedSize(const int64_t size) {
  return (size + kHcclAlignSize - 1) / kHcclAlignSize * kHcclAlignSize;
}
}  // namespace

std::string HcclTaskCodeBuilder::GetFuncName() const {
  return kDispatchFuncName;
}

int64_t HcclTaskCodeBuilder::ParseOpIndex(const domi::TaskDef &task_def) {
  const auto &hccl_def = task_def.kernel_hccl();
  return static_cast<int64_t>(hccl_def.op_index());
}

Status HcclTaskCodeBuilder::ParseHcclAttrs(const domi::TaskDef &task_def, const OpDescPtr &op_desc) {
  const auto &hccl_def = task_def.kernel_hccl();
  build_data_.hccl_type = hccl_def.hccl_type();
  GE_CHECK_NOTNULL(op_desc);
  build_data_.op_type = op_desc->GetType();
  const std::string &private_def = task_def.private_def();
  build_data_.private_def.assign(private_def.begin(), private_def.end());

  build_data_.is_hcom_op = Om2CodegenUtils::IsHcomOpType(build_data_.hccl_type);
  build_data_.is_hvd = Om2CodegenUtils::IsHorovodOpType(build_data_.hccl_type);
  GE_ASSERT_TRUE(build_data_.is_hcom_op || build_data_.is_hvd,
                 "[OM2] HCCL op [%s] type [%s] is not in the hcom/horovod op whitelist.", op_desc->GetName().c_str(),
                 build_data_.hccl_type.c_str());

  (void)AttrUtils::GetInt(op_desc, "used_stream_num", build_data_.used_stream_num);
  if (!build_data_.is_hcom_op || (build_data_.used_stream_num < 0)) {
    build_data_.used_stream_num = 0;
  }
  build_data_.attached_stream_ids = op_desc->GetAttachedStreamIds();

  build_data_.is_collective = TaskCodeBuilderUtil::IsCollectiveCommType(build_data_.hccl_type);
  build_data_.has_global_workspace = AttrUtils::HasAttr(op_desc, GLOBALWORKSPACE_TYPE);

  if (build_data_.is_hvd) {
    GE_ASSERT_SUCCESS(ParseHorovodAttrs(op_desc));
  }

  GE_ASSERT_SUCCESS(ParseHcclRootId(op_desc));
  GE_ASSERT_SUCCESS(ParseHcclDataType(op_desc));
  GE_ASSERT_SUCCESS(ParseHcclReduceOp(op_desc));
  GE_ASSERT_SUCCESS(ComputeHcclCount(op_desc));

  const std::vector<int64_t> &workspace_bytes = op_desc->GetWorkspaceBytes();
  build_data_.workspace_size = workspace_bytes.empty() ? 0U : static_cast<uint64_t>(workspace_bytes[0U]);

  GE_ASSERT_SUCCESS(ParseZeroCopyFlags(task_def, op_desc));
  return SUCCESS;
}

Status HcclTaskCodeBuilder::ParseHorovodAttrs(const OpDescPtr &op_desc) {
  build_data_.is_hvd_wait = (build_data_.hccl_type == HVDWAIT);
  if (build_data_.is_hvd_wait) {
    return SUCCESS;
  }
  build_data_.hvd_input_names.reserve(op_desc->GetInputsSize());
  build_data_.hvd_dims.reserve(op_desc->GetInputsSize());
  build_data_.hvd_dtypes.reserve(op_desc->GetInputsSize());
  build_data_.hvd_counts.reserve(op_desc->GetInputsSize());
  for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
    build_data_.hvd_input_names.push_back(op_desc->GetInputNameByIndex(static_cast<uint32_t>(i)));
    const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
    GE_CHECK_NOTNULL(input_desc);
    build_data_.hvd_dims.push_back(input_desc->GetShape().GetDims());
  }
  return SUCCESS;
}

Status HcclTaskCodeBuilder::ParseZeroCopyFlags(const domi::TaskDef &task_def, const OpDescPtr &op_desc) {
  const auto &hccl_def = task_def.kernel_hccl();
  const auto input_flag_size = hccl_def.input_zero_copy_flag_size();
  GE_ASSERT_TRUE((input_flag_size == 0) || (static_cast<size_t>(input_flag_size) == op_desc->GetInputsSize()),
                 "[OM2] HCCL op [%s] input_zero_copy_flag_size %d is not equal with op input size %zu.",
                 op_desc->GetName().c_str(), input_flag_size, op_desc->GetInputsSize());
  for (int32_t i = 0; i < input_flag_size; ++i) {
    build_data_.input_zero_copy_flags.push_back(hccl_def.input_zero_copy_flag(i));
  }
  const auto output_flag_size = hccl_def.output_zero_copy_flag_size();
  GE_ASSERT_TRUE((output_flag_size == 0) || (static_cast<size_t>(output_flag_size) == op_desc->GetOutputsSize()),
                 "[OM2] HCCL op [%s] output_zero_copy_flag_size %d is not equal with op output size %zu.",
                 op_desc->GetName().c_str(), output_flag_size, op_desc->GetOutputsSize());
  for (int32_t i = 0; i < output_flag_size; ++i) {
    build_data_.output_zero_copy_flags.push_back(hccl_def.output_zero_copy_flag(i));
  }
  return SUCCESS;
}

Status HcclTaskCodeBuilder::ParseHcclRootId(const OpDescPtr &op_desc) {
  if (!IsRootOpType(build_data_.hccl_type)) {
    return SUCCESS;
  }
  GE_ASSERT_TRUE(AttrUtils::GetInt(op_desc, HCOM_ATTR_ROOT_RANK, build_data_.root_id),
                 "[OM2] HCCL op [%s] get attr [%s] failed.", op_desc->GetName().c_str(), HCOM_ATTR_ROOT_RANK.c_str());
  return SUCCESS;
}

Status HcclTaskCodeBuilder::ParseHcclDataType(const OpDescPtr &op_desc) {
  if (build_data_.is_hvd) {
    if (build_data_.is_hvd_wait) {
      return SUCCESS;
    }
    for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
      const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
      GE_CHECK_NOTNULL(input_desc);
      int32_t hccl_data_type = 0;
      GE_ASSERT_SUCCESS(MapToHcclDataType(input_desc->GetDataType(), hccl_data_type));
      build_data_.hvd_dtypes.push_back(hccl_data_type);
    }
    if (!build_data_.hvd_dtypes.empty()) {
      build_data_.hccl_data_type = build_data_.hvd_dtypes[0U];
    }
    return SUCCESS;
  }
  ge::DataType ge_data_type = ge::DT_FLOAT;
  if (build_data_.hccl_type == HCOMRECEIVE) {
    GE_ASSERT_TRUE(AttrUtils::GetDataType(op_desc, HCOM_ATTR_DATA_TYPE, ge_data_type),
                   "[OM2] HCCL op [%s] get attr [%s] failed.", op_desc->GetName().c_str(), HCOM_ATTR_DATA_TYPE.c_str());
  } else {
    const auto input_desc = op_desc->GetInputDescPtr(0U);
    GE_ASSERT_NOTNULL(input_desc, "[OM2] HCCL op [%s] has no input desc.", op_desc->GetName().c_str());
    ge_data_type = input_desc->GetDataType();
  }
  return MapToHcclDataType(ge_data_type, build_data_.hccl_data_type);
}

Status HcclTaskCodeBuilder::ParseHcclReduceOp(const OpDescPtr &op_desc) {
  if (!IsReduceOpType(build_data_.hccl_type)) {
    build_data_.hccl_reduce_op = static_cast<int32_t>(HCCL_REDUCE_SUM);
    return SUCCESS;
  }
  if (build_data_.is_hvd) {
    int64_t reduce_op = 0;
    GE_ASSERT_TRUE(AttrUtils::GetInt(op_desc, ATTR_HOROVOD_ATTR_REDUCE_TYPE, reduce_op),
                   "[OM2] HCCL op [%s] get attr [%s] failed.", op_desc->GetName().c_str(),
                   ATTR_HOROVOD_ATTR_REDUCE_TYPE.c_str());
    return MapToHcclReduceOp(reduce_op, build_data_.hccl_reduce_op);
  }
  std::string reduce_op;
  GE_ASSERT_TRUE(AttrUtils::GetStr(op_desc, HCOM_ATTR_REDUCE_TYPE, reduce_op),
                 "[OM2] HCCL op [%s] get attr [%s] failed.", op_desc->GetName().c_str(), HCOM_ATTR_REDUCE_TYPE.c_str());
  return MapToHcclReduceOp(reduce_op, build_data_.hccl_reduce_op);
}

Status HcclTaskCodeBuilder::ComputeHcclCount(const OpDescPtr &op_desc) {
  if (build_data_.is_hvd) {
    if (build_data_.is_hvd_wait) {
      return SUCCESS;
    }
    for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
      const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
      GE_CHECK_NOTNULL(input_desc);
      const int64_t dtype_size = GetSizeByDataType(input_desc->GetDataType());
      GE_ASSERT_TRUE(dtype_size > 0, "[OM2] HCCL op [%s] invalid dtype size %ld.", op_desc->GetName().c_str(),
                     static_cast<long>(dtype_size));
      int64_t input_size = 0;
      GE_ASSERT_TRUE(TensorUtils::GetSize(*input_desc, input_size) == GRAPH_SUCCESS,
                     "[OM2] HCCL op [%s] get input %zu size failed.", op_desc->GetName().c_str(), i);
      const int64_t shape_size = input_desc->GetShape().IsScalar() ? 1 : input_desc->GetShape().GetShapeSize();
      const int64_t block_size =
          (build_data_.hccl_type == HVDCALLBACKALLGATHER) ? (shape_size * dtype_size) : GetAlignedSize(input_size);
      GE_ASSERT_TRUE((block_size % dtype_size) == 0,
                     "[OM2] HCCL op [%s] block size %ld is not divisible by dtype size %ld.",
                     op_desc->GetName().c_str(), static_cast<long>(block_size), static_cast<long>(dtype_size));
      build_data_.hvd_counts.push_back(static_cast<uint64_t>(block_size / dtype_size));
    }
    return SUCCESS;
  }

  ge::DataType ge_data_type = ge::DT_FLOAT;
  if (build_data_.hccl_type == HCOMRECEIVE) {
    (void)AttrUtils::GetDataType(op_desc, HCOM_ATTR_DATA_TYPE, ge_data_type);
  } else {
    const auto input_desc = op_desc->GetInputDescPtr(0U);
    GE_CHECK_NOTNULL(input_desc);
    ge_data_type = input_desc->GetDataType();
  }
  const int64_t dtype_size = GetSizeByDataType(ge_data_type);
  GE_ASSERT_TRUE(dtype_size > 0, "[OM2] HCCL op [%s] invalid dtype size %ld.", op_desc->GetName().c_str(),
                 static_cast<long>(dtype_size));

  bool is_continuous_input = false;
  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_CONTINUOUS_INPUT, is_continuous_input);
  bool is_unknown_shape = false;
  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_IS_UNKNOWN_SHAPE, is_unknown_shape);

  int64_t total_size = 0;
  if (build_data_.hccl_type == HCOMRECEIVE) {
    for (size_t i = 0U; i < op_desc->GetOutputsSize(); ++i) {
      const auto output_desc = op_desc->GetOutputDescPtr(static_cast<uint32_t>(i));
      GE_CHECK_NOTNULL(output_desc);
      int64_t output_size = 0;
      GE_ASSERT_TRUE(TensorUtils::GetSize(*output_desc, output_size) == GRAPH_SUCCESS,
                     "[OM2] HCCL op [%s] get output %zu size failed.", op_desc->GetName().c_str(), i);
      total_size += GetAlignedSize(output_size);
    }
  } else {
    for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
      const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
      GE_CHECK_NOTNULL(input_desc);
      int64_t input_size = 0;
      GE_ASSERT_TRUE(TensorUtils::GetSize(*input_desc, input_size) == GRAPH_SUCCESS,
                     "[OM2] HCCL op [%s] get input %zu size failed.", op_desc->GetName().c_str(), i);
      const int64_t shape_size = input_desc->GetShape().IsScalar() ? 1 : input_desc->GetShape().GetShapeSize();
      int64_t block_size = 0;
      if (build_data_.hccl_type == HCOMREDUCESCATTER) {
        int64_t rank_size = 0;
        GE_ASSERT_TRUE(AttrUtils::GetInt(op_desc, HCOM_ATTR_RANK_SIZE, rank_size) && (rank_size != 0),
                       "[OM2] HCCL op [%s] get attr [%s] failed or rank size is zero.", op_desc->GetName().c_str(),
                       HCOM_ATTR_RANK_SIZE.c_str());
        block_size =
            is_continuous_input ? (GetAlignedSize(input_size) / rank_size) : ((shape_size * dtype_size) / rank_size);
      } else {
        if (is_unknown_shape && (op_desc->GetOutputDescPtr(static_cast<uint32_t>(i)) != nullptr)) {
          input_size = shape_size * dtype_size;
        }
        if (build_data_.hccl_type == HCOMALLTOALL) {
          block_size = shape_size * dtype_size;
        } else if (build_data_.hccl_type == HCOMALLGATHER) {
          block_size = is_continuous_input ? GetAlignedSize(input_size) : (shape_size * dtype_size);
        } else {
          block_size = GetAlignedSize(input_size);
        }
      }
      total_size += block_size;
    }
  }
  GE_ASSERT_TRUE((total_size % dtype_size) == 0,
                 "[OM2] HCCL op [%s] total size %ld is not divisible by dtype size %ld.", op_desc->GetName().c_str(),
                 static_cast<long>(total_size), static_cast<long>(dtype_size));
  build_data_.count = static_cast<uint64_t>(total_size / dtype_size);
  return SUCCESS;
}

Status HcclTaskCodeBuilder::Contribute(TaskSemanticContributeContext &context) {
  FillTaskSemanticHeader(context, header_);
  GE_ASSERT_NOTNULL(context.next_args_table_index);
  GE_ASSERT_NOTNULL(context.next_host_args_offset);
  GE_ASSERT_NOTNULL(context.op_desc);

  GE_ASSERT_SUCCESS(ParseHcclAttrs(context.task_def, context.op_desc));

  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveWorkspaceAddrs(context, build_data_.workspace_addrs));
  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveInputAddrs(context, build_data_.input_addrs));
  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveOutputAddrs(context, true, build_data_.output_addrs));

  const uint64_t args_size = kAddrLen * (static_cast<uint64_t>(build_data_.input_addrs.size()) +
                                         static_cast<uint64_t>(build_data_.output_addrs.size()) +
                                         static_cast<uint64_t>(build_data_.workspace_addrs.size()));
  if (args_size > 0U) {
    InitArgsTableEntry(context, args_size);
    GE_ASSERT_SUCCESS(AssembleOrderedArgs());
  }
  GELOGI("[OM2] hccl task compiled, op=%s, type=%s, stream_id=%u, in=%zu, out=%zu, ws=%zu, count=%" PRIu64
         ", dtype=%d, reduce_op=%d, root=%d, need_refresh=%u, hvd=%d, hvd_wait=%d, collective=%d, "
         "used_stream_num=%" PRId64 ", attached=%zu, workspace_size=%" PRIu64,
         header_.op_name.c_str(), build_data_.hccl_type.c_str(), header_.stream_id, build_data_.input_addrs.size(),
         build_data_.output_addrs.size(), build_data_.workspace_addrs.size(), build_data_.count,
         build_data_.hccl_data_type, build_data_.hccl_reduce_op, build_data_.root_id,
         static_cast<uint32_t>(build_data_.need_refresh), static_cast<int32_t>(build_data_.is_hvd),
         static_cast<int32_t>(build_data_.is_hvd_wait), static_cast<int32_t>(build_data_.is_collective),
         build_data_.used_stream_num, build_data_.attached_stream_ids.size(), build_data_.workspace_size);
  return SUCCESS;
}

void HcclTaskCodeBuilder::InitArgsTableEntry(const TaskSemanticContributeContext &context, const uint64_t args_size) {
  args_table_entry_ = TaskCodeBuilderUtil::InitArgsTableEntry(context, build_data_.args_table_entry, args_size);
}

Status HcclTaskCodeBuilder::AssembleOrderedArgs() {
  const auto append = [this](const std::vector<AddrSemantic> &addrs, uint64_t &args_offset) {
    for (const auto &addr : addrs) {
      OpArgDesc arg = TaskCodeBuilderUtil::ConvertAddrDesc(addr);
      arg.args_offset = args_offset;
      args_offset += kAddrLen;
      build_data_.ordered_args.push_back(std::move(arg));
    }
  };
  uint64_t args_offset = 0U;
  append(build_data_.input_addrs, args_offset);
  append(build_data_.output_addrs, args_offset);
  append(build_data_.workspace_addrs, args_offset);
  return SUCCESS;
}

Status HcclTaskCodeBuilder::ParseTaskRunParam(const domi::TaskDef &task_def, const om2::RuntimeParam &rts_param,
                                              OpDescPtr op_desc, om2::TaskRunParam &task_run_param) {
  (void)task_def;
  GE_CHECK_NOTNULL(op_desc);
  op_desc_ = op_desc;
  const uint64_t args_size =
      kAddrLen * (static_cast<uint64_t>(op_desc->GetInputsSize()) + static_cast<uint64_t>(op_desc->GetOutputsSize()) +
                  static_cast<uint64_t>(op_desc->GetWorkspaceBytes().size()));
  if (args_size > 0U) {
    task_run_param.args_descs.push_back({static_cast<int64_t>(args_size), om2::ArgsPlacement::kArgsPlacementHbm});
  }
  std::vector<uint64_t> input_mem_types;
  std::vector<uint64_t> output_mem_types;
  std::vector<uint64_t> workspace_mem_types;
  const auto input_addrs = om2::ModelUtils::GetInputAddrsValue(rts_param, op_desc, input_mem_types);
  const auto output_addrs = om2::ModelUtils::GetOutputAddrsValue(rts_param, op_desc, output_mem_types);
  const auto workspace_addrs = om2::ModelUtils::GetWorkspaceDataAddrsValue(rts_param, op_desc, workspace_mem_types);
  for (size_t i = 0UL; i < input_addrs.size(); ++i) {
    task_run_param.parsed_input_addrs.push_back({input_addrs[i], input_mem_types[i], true, {0}});
  }
  for (size_t i = 0UL; i < output_addrs.size(); ++i) {
    task_run_param.parsed_output_addrs.push_back({output_addrs[i], output_mem_types[i], true, {0}});
  }
  for (size_t i = 0UL; i < workspace_addrs.size(); ++i) {
    task_run_param.parsed_workspace_addrs.push_back({workspace_addrs[i], workspace_mem_types[i], true, {0}});
  }
  return SUCCESS;
}

Status HcclTaskCodeBuilder::Init(const domi::TaskDef &task_def,
                                 std::vector<om2::MemAllocation> &logical_mem_allocations, const om2::PisToArgs &args,
                                 const om2::IowAddrs &iow_addrs) {
  (void)task_def;
  (void)args;
  std::vector<uint64_t> io_addrs;
  std::vector<uint64_t> io_mem_types;
  bool any_refresh = false;
  auto AppendAddrs = [&io_addrs, &io_mem_types, &any_refresh](const std::vector<om2::AddrDesc> &addrs) {
    for (const auto &a : addrs) {
      io_addrs.push_back(a.logic_addr);
      io_mem_types.push_back(a.memory_type);
      // 仅 FeatureMap 刷新（option 门控）计入槽位使能；
      // ModelIo 零拷贝刷新不计入（直连 HCCL 的模型 IO 走拷贝路径，地址恒定）
      any_refresh =
          any_refresh ||
          (a.support_refresh && (a.memory_type == static_cast<uint64_t>(om2::MemoryAppType::kMemoryTypeFeatureMap)));
    }
  };
  AppendAddrs(iow_addrs.input_logic_addrs);
  AppendAddrs(iow_addrs.output_logic_addrs);
  AppendAddrs(iow_addrs.workspace_logic_addrs);
  build_data_.need_refresh = any_refresh ? 1U : 0U;
  GE_ASSERT_NOTNULL(op_desc_);
  GE_ASSERT_SUCCESS(args_io_addrs_updater_.Init(logical_mem_allocations, io_addrs, io_mem_types,
                                                {op_desc_->GetName(), op_desc_->GetType()}));
  return SUCCESS;
}

Status HcclTaskCodeBuilder::GetTaskArgsRefreshInfos(std::vector<om2::TaskArgsRefreshInfo> &infos) {
  args_io_addrs_updater_.GenArgsRefreshInfos(infos, 0UL, om2::ArgsPlacement::kArgsPlacementHbm);
  return SUCCESS;
}

Arg HcclTaskCodeBuilder::RenderPrivateDefArg() const {
  if (build_data_.private_def.empty()) {
    return Arg(nullptr);
  }
  return ast_.ReinterpretCast("const uint8_t *",
                              Arg::StringLiteral(TaskCodeBuilderUtil::EscapeToOctalString(build_data_.private_def)));
}

Arg HcclTaskCodeBuilder::RenderAttachedStreamIdsArg() const {
  if (build_data_.attached_stream_ids.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> values;
  values.reserve(build_data_.attached_stream_ids.size());
  for (const auto stream_id : build_data_.attached_stream_ids) {
    values.emplace_back(std::to_string(stream_id));
  }
  return ast_.CCast("const int32_t[]", ast_.InitList(values));
}

Status HcclTaskCodeBuilder::RenderOpDefTableFields(std::vector<std::pair<std::string, Arg>> &fields) {
  fields.push_back(
      {"dispatch_type", ast_.StaticCast("OpDispatchType", static_cast<int64_t>(OpDispatchType::DISPATCH_HCCL))});
  fields.push_back({"op_name", Arg::StringLiteral(header_.op_name)});

  const auto table_index =
      build_data_.args_table_entry.has_value() ? static_cast<int64_t>(build_data_.args_table_entry->table_index) : 0LL;
  std::vector<Arg> comm_values = {
      Arg::StringLiteral(build_data_.hccl_type),
      RenderPrivateDefArg(),
      static_cast<int64_t>(build_data_.private_def.size()),
      static_cast<int64_t>(build_data_.root_id),
      static_cast<int64_t>(build_data_.hccl_data_type),
      static_cast<int64_t>(build_data_.hccl_reduce_op),
      Arg(std::to_string(build_data_.count) + "UL"),
      Arg(std::to_string(build_data_.workspace_size) + "UL"),
  };
  std::vector<Arg> stream_values = {
      build_data_.used_stream_num,
      RenderAttachedStreamIdsArg(),
      static_cast<int64_t>(build_data_.attached_stream_ids.size()),
  };
  std::vector<Arg> hvd_values = {
      RenderHvdMetasArg(),
      static_cast<int64_t>(build_data_.is_hvd ? build_data_.hvd_input_names.size() : 0U),
      static_cast<int64_t>(build_data_.is_hvd_wait),
  };
  fields.push_back(
      {"dispatch_info",
       ast_.DesignatedInit(
           {{"hccl", ast_.DesignatedInit({
                         {"args_info", TaskCodeBuilderUtil::RenderOpArgDesc(ast_, build_data_.ordered_args)},
                         {"args_info_num", static_cast<int64_t>(build_data_.ordered_args.size())},
                         {"input_num", static_cast<int64_t>(build_data_.input_addrs.size())},
                         {"output_num", static_cast<int64_t>(build_data_.output_addrs.size())},
                         {"stream_id", static_cast<uint32_t>(header_.stream_id)},
                         {"args_table_idx", table_index},
                         {"need_refresh", static_cast<int64_t>(build_data_.need_refresh)},
                         {"op_type", Arg::StringLiteral(build_data_.op_type)},
                         {"task_type", static_cast<int64_t>(ModelTaskType::MODEL_TASK_HCCL)},
                         {"input_zero_copy_flags", RenderZeroCopyFlagsArg(build_data_.input_zero_copy_flags)},
                         {"output_zero_copy_flags", RenderZeroCopyFlagsArg(build_data_.output_zero_copy_flags)},
                         {"is_collective", static_cast<int64_t>(build_data_.is_collective)},
                         {"has_global_workspace", static_cast<int64_t>(build_data_.has_global_workspace)},
                         {"comm", ast_.InitList(comm_values)},
                         {"streams", ast_.InitList(stream_values)},
                         {"hvd", ast_.InitList(hvd_values)},
                     })}})});
  return SUCCESS;
}

Arg HcclTaskCodeBuilder::RenderZeroCopyFlagsArg(const std::vector<int32_t> &flags) const {
  if (flags.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> values;
  values.reserve(flags.size());
  for (const auto flag : flags) {
    values.emplace_back(std::to_string(flag));
  }
  return ast_.CCast("const int32_t[]", ast_.InitList(values));
}

Arg HcclTaskCodeBuilder::RenderHvdMetasArg() const {
  if (!build_data_.is_hvd || build_data_.is_hvd_wait || build_data_.hvd_input_names.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> metas;
  metas.reserve(build_data_.hvd_input_names.size());
  for (size_t i = 0U; i < build_data_.hvd_input_names.size(); ++i) {
    std::vector<Arg> dims;
    dims.reserve(build_data_.hvd_dims[i].size());
    for (const auto dim : build_data_.hvd_dims[i]) {
      dims.emplace_back(std::to_string(dim));
    }
    const Arg dims_arg =
        build_data_.hvd_dims[i].empty() ? Arg(nullptr) : ast_.CCast("const int64_t[]", ast_.InitList(dims));
    metas.emplace_back(ast_.InitList({
        ast_.Sizeof("HcclHorovodMeta"),
        Arg::StringLiteral(build_data_.hvd_input_names[i]),
        dims_arg,
        static_cast<int64_t>(build_data_.hvd_dims[i].size()),
        ast_.UInt(0U),
        static_cast<int64_t>(build_data_.hvd_dtypes[i]),
        ast_.UInt(0U),
        Arg(std::to_string(build_data_.hvd_counts[i]) + "UL"),
    }));
  }
  return ast_.CCast("const HcclHorovodMeta[]", ast_.InitList(metas));
}

std::vector<BodyItem> HcclTaskCodeBuilder::RenderIoAddrResolve(ExprRef hccl, const VarRef &ctx) const {
  std::vector<BodyItem> body;
  auto v_a = ast_.Var("const auto &", "a");
  auto v_iow = ast_.Var("", "iow_addrs");
  auto resolve_addr = ast_.ReinterpretCast(
      "uint64_t",
      ast_.Call("ResolveOpAddr", {v_a.Attr("addr").Attr("mem_src"), v_a.Attr("addr").Attr("index"),
                                  v_a.Attr("addr").Attr("offset"), ctx.Attr("total_dev_mem_ptr"),
                                  ctx.Attr("session_scope_mem_ptr"), ctx.Attr("constants"), ctx.Attr("var_addrs")}));
  std::initializer_list<BodyItem> loop_body = {
      ast_.VarDecl(v_a, hccl.Attr("args_info")[ast_.Var("", "i")]),
      ast_.Switch(v_a.Attr("type"),
                  std::vector<BodyItem>{
                      ast_.Case(ast_.Var("", "OP_ARG_INPUT")),
                      ast_.Case(ast_.Var("", "OP_ARG_OUTPUT")),
                      ast_.Case(ast_.Var("", "OP_ARG_CONST_TENSOR")),
                      ast_.Case(ast_.Var("", "OP_ARG_VAR_TENSOR")),
                      ast_.Case(ast_.Var("", "OP_ARG_WORKSPACE")),
                      ast_.Block(std::vector<BodyItem>{ast_.Assign(v_iow[ast_.Var("", "i")], resolve_addr)}),
                      ast_.Case(nullptr),
                      ast_.Block(std::vector<BodyItem>{ast_.Break()}),
                  }),
  };
  body.push_back(ast_.VarDecl(ast_.Var("std::vector<uint64_t>", "iow_addrs")));
  body.push_back(ast_.Var("", "iow_addrs").Resize(hccl.Attr("args_info_num")));
  body.push_back(ast_.For(ast_.VarDecl("uint32_t", "i", ast_.UInt(0U)), ast_.Var("", "i") < hccl.Attr("args_info_num"),
                          ast_.PostInc(ast_.Var("", "i")), loop_body));
  body.push_back(ast_.VarDecl(ast_.Var("ArgsInfo *", "args_info"),
                              ctx.Attr("args_table").Attr("GetArgsInfo")(hccl.Attr("args_table_idx"))));
  body.push_back(
      ast_.If(ast_.Var("", "args_info") != "nullptr",
              {ChkStatus(MemcpyS(ast_.Var("", "args_info").Arrow("host_addr"), ast_.Var("", "args_info").Arrow("size"),
                                 v_iow.Data(), ast_.Sizeof("uint64_t") * v_iow.Size()))}));
  return body;
}

std::vector<BodyItem> HcclTaskCodeBuilder::RenderTaskParamsAssemble(ExprRef hccl, const VarRef &ctx,
                                                                    const VarRef &iow_addrs) const {
  std::vector<BodyItem> body;
  auto v_params = ast_.Var("HcclTaskParams", "params");
  auto v_in = ast_.Var("std::vector<void *>", "hccl_input_addrs");
  auto v_out = ast_.Var("std::vector<void *>", "hccl_output_addrs");
  body.push_back(ast_.VarDecl(v_in, ast_.InitList({})));
  body.push_back(v_in.Resize(hccl.Attr("input_num")));
  body.push_back(
      ast_.For(ast_.VarDecl("uint32_t", "i", ast_.UInt(0U)), ast_.Var("", "i") < hccl.Attr("input_num"),
               ast_.PostInc(ast_.Var("", "i")),
               {ast_.Assign(v_in[ast_.Var("", "i")], ast_.ReinterpretCast("void *", iow_addrs[ast_.Var("", "i")]))}));
  body.push_back(ast_.VarDecl(v_out, ast_.InitList({})));
  body.push_back(v_out.Resize(hccl.Attr("output_num")));
  body.push_back(
      ast_.For(ast_.VarDecl("uint32_t", "i", ast_.UInt(0U)), ast_.Var("", "i") < hccl.Attr("output_num"),
               ast_.PostInc(ast_.Var("", "i")),
               {ast_.Assign(v_out[ast_.Var("", "i")],
                            ast_.ReinterpretCast("void *", iow_addrs[hccl.Attr("input_num") + ast_.Var("", "i")]))}));

  body.push_back(ast_.VarDecl(v_params, ast_.InitList({})));
  body.push_back(ast_.Assign(v_params.Attr("struct_size"), ast_.Sizeof("HcclTaskParams")));
  body.push_back(ast_.Assign(v_params.Attr("hccl_type"), hccl.Attr("comm").Attr("hccl_type")));
  body.push_back(ast_.Assign(v_params.Attr("private_def"), hccl.Attr("comm").Attr("private_def")));
  body.push_back(ast_.Assign(v_params.Attr("private_def_len"), hccl.Attr("comm").Attr("private_def_len")));
  body.push_back(ast_.Assign(v_params.Attr("main_stream"), ctx.Attr("stream_list")[hccl.Attr("stream_id")]));
  body.push_back(ast_.Assign(v_params.Attr("input_addrs"), v_in.Data()));
  body.push_back(ast_.Assign(v_params.Attr("input_num"), hccl.Attr("input_num")));
  body.push_back(ast_.Assign(v_params.Attr("output_addrs"), v_out.Data()));
  body.push_back(ast_.Assign(v_params.Attr("output_num"), hccl.Attr("output_num")));
  body.push_back(ast_.If(
      hccl.Attr("comm").Attr("workspace_size") != 0U,
      {ast_.Assign(v_params.Attr("workspace_addr"),
                   ast_.ReinterpretCast("void *", iow_addrs[hccl.Attr("input_num") + hccl.Attr("output_num")]))}));
  body.push_back(ast_.Assign(v_params.Attr("workspace_size"), hccl.Attr("comm").Attr("workspace_size")));
  body.push_back(ast_.Assign(v_params.Attr("hccl_data_type"), hccl.Attr("comm").Attr("hccl_data_type")));
  body.push_back(ast_.Assign(v_params.Attr("hccl_reduce_op"), hccl.Attr("comm").Attr("hccl_reduce_op")));
  body.push_back(ast_.Assign(v_params.Attr("count"), hccl.Attr("comm").Attr("count")));
  body.push_back(ast_.Assign(v_params.Attr("root_id"), hccl.Attr("comm").Attr("root_id")));
  body.push_back(ast_.Assign(v_params.Attr("need_refresh"), hccl.Attr("need_refresh")));
  body.push_back(ast_.If(ast_.Var("", "args_info") != "nullptr",
                         {ast_.Assign(v_params.Attr("args_base"), ast_.Var("", "args_info").Arrow("dev_addr"))}));
  body.push_back(ast_.Assign(v_params.Attr("hvd_metas"), hccl.Attr("hvd").Attr("metas")));
  body.push_back(ast_.Assign(v_params.Attr("hvd_wait"), hccl.Attr("hvd").Attr("is_wait")));
  body.push_back(ast_.Assign(v_params.Attr("is_collective"), hccl.Attr("is_collective")));
  body.push_back(ast_.Assign(v_params.Attr("input_zero_copy_flags"), hccl.Attr("input_zero_copy_flags")));
  body.push_back(ast_.Assign(v_params.Attr("output_zero_copy_flags"), hccl.Attr("output_zero_copy_flags")));
  body.push_back(ast_.If(hccl.Attr("has_global_workspace") != ast_.UInt(0U),
                         {ast_.Assign(v_params.Attr("overflow_addr"), ctx.Attr("overflow_addr"))}));
  return body;
}

std::vector<BodyItem> HcclTaskCodeBuilder::RenderFollowStreams(ExprRef hccl, const VarRef &ctx,
                                                               const VarRef &params) const {
  std::vector<BodyItem> body;
  auto v_need = ast_.Var("const uint32_t", "need");
  auto v_pool = ast_.Var("auto &", "pool");
  body.push_back(ast_.VarDecl(v_need, ast_.StaticCast("uint32_t", hccl.Attr("streams").Attr("used_stream_num"))));
  body.push_back(ast_.VarDecl(v_pool, ctx.Attr("main_follow_stream_mapping")[hccl.Attr("stream_id")]));
  body.push_back(ast_.Assign(params.Attr("follow_streams"), v_pool.Data()));
  body.push_back(ast_.Assign(params.Attr("follow_stream_num"), v_need));
  return body;
}

std::vector<BodyItem> HcclTaskCodeBuilder::RenderLaunch(ExprRef hccl, const VarRef &ctx, const VarRef &params,
                                                        const VarRef &op) const {
  std::vector<BodyItem> body;
  body.push_back(ast_.If(ctx.Attr("launch_func") == "nullptr",
                         {
                             ChkStatus(ast_.Call("ctx.hccl_load_task", {ctx.Attr("instance_handle"), params.Addr()})),
                             ast_.Return("ACL_SUCCESS"),
                         }));

  auto v_task_info = ast_.Var("GertModelTaskDesc", "task_info");
  auto v_lp = ast_.Var("GertModelTaskLaunchParams", "launch_params");
  auto v_li = ast_.Var("GertModelTaskLaunchInfo", "launch_info");
  auto v_stream = ctx.Attr("stream_list")[hccl.Attr("stream_id")];
  // dump 侧（SaveOpInfo/SaveTask/DumpOp）会解引用 io entry 的 tensor 指针，须按 kernel 任务模式经 BuildTensor
  // 构造真实 gert::Tensor（禁止将设备地址伪 cast 为指针，dump 侧直传真实地址）
  auto v_io_tensors = ast_.Var("", "hccl_io_tensors");
  auto v_io_in = ast_.Var("", "hccl_report_inputs");
  auto v_io_out = ast_.Var("", "hccl_report_outputs");
  auto v_entry = ast_.Var("GertModelTaskIoEntry", "_entry");
  body.push_back(ast_.VarDecl(ast_.Var("std::vector<gert::Tensor>", "hccl_io_tensors")));
  // tensor 指针稳定性：预留容量，避免循环内 push_back 扩容搬移导致 &back() 悬空
  // （dump 侧 SaveOpInfo/BuildTaskInputs 解引用悬空 Tensor 会 coredump，对齐 kernel/custom 同款 reserve）
  body.push_back(v_io_tensors.Attr("reserve")(hccl.Attr("input_num") + hccl.Attr("output_num")));
  body.push_back(ast_.VarDecl(ast_.Var("std::vector<GertModelTaskIoEntry>", "hccl_report_inputs")));
  body.push_back(ast_.VarDecl(ast_.Var("std::vector<GertModelTaskIoEntry>", "hccl_report_outputs")));
  body.push_back(ast_.For(
      ast_.VarDecl("uint32_t", "i", ast_.UInt(0U)),
      ast_.Var("", "i") < (hccl.Attr("input_num") + hccl.Attr("output_num")), ast_.PostInc(ast_.Var("", "i")),
      {ast_.VarDecl(ast_.Var("const auto &", "a"), hccl.Attr("args_info")[ast_.Var("", "i")]),
       v_io_tensors.PushBack(
           ast_.Call("BuildTensor", {ast_.ReinterpretCast("void *", ast_.Var("", "iow_addrs")[ast_.Var("", "i")]),
                                     ast_.Var("", "a").Attr("data").Attr("tensor").Attr("size"),
                                     ast_.Var("", "a").Attr("data").Attr("tensor").Attr("data_type"),
                                     ast_.Var("", "a").Attr("data").Attr("tensor").Attr("format"),
                                     ast_.Var("", "a").Attr("data").Attr("tensor").Attr("shape"),
                                     ast_.Var("", "a").Attr("data").Attr("tensor").Attr("shape_dims")})),
       ast_.VarDecl(v_entry,
                    ast_.InitList({ast_.Var("", "sizeof(GertModelTaskIoEntry)"), v_io_tensors.Attr("back")().Addr(),
                                   ast_.Var("", "a").Attr("data").Attr("tensor").Attr("args_offset")})),
       ast_.If(ast_.Var("", "i") < hccl.Attr("input_num"), {v_io_in.PushBack(v_entry)},
               {v_io_out.PushBack(v_entry)})}));
  body.push_back(ast_.VarDecl(v_task_info, ast_.InitList({})));
  body.push_back(ChkStatus(
      ast_.Call("AssembleOm2TaskInfo", {v_task_info.Addr(),
                                        op.Arrow("op_name"),
                                        hccl.Attr("op_type"),
                                        ast_.UInt(0U),
                                        hccl.Attr("stream_id"),
                                        ast_.UInt(0U),
                                        ast_.UInt(0U),
                                        ast_.ReinterpretCast("uintptr_t", ast_.Var("", "args_info").Arrow("dev_addr")),
                                        ast_.Var("", "args_info").Arrow("size"),
                                        v_io_in.Data(),
                                        ast_.StaticCast("uint64_t", v_io_in.Size()),
                                        v_io_out.Data(),
                                        ast_.StaticCast("uint32_t", v_io_out.Size()),
                                        "nullptr",
                                        "nullptr",
                                        ast_.UInt(0U),
                                        hccl.Attr("task_type"),
                                        v_stream,
                                        ast_.UInt(0U),
                                        ast_.UInt(0U)})));
  body.push_back(ChkStatus(ast_.Call(
      "aclrtStreamGetId",
      {v_task_info.Attr("stream"), ast_.ReinterpretCast("int32_t *", v_task_info.Attr("stream_id").Addr())})));
  body.push_back(ast_.Assign(v_task_info.Attr("task_raw_info"), Arg(nullptr)));

  body.push_back(ast_.VarDecl(v_lp, ast_.InitList({})));
  body.push_back(
      ast_.Assign(v_lp.Attr("launch_hccl_params").Attr("struct_size"), ast_.Sizeof("GertModelLaunchHcclParams")));
  body.push_back(ast_.Assign(v_lp.Attr("launch_hccl_params").Attr("func_load_hccl"), ctx.Attr("hccl_load_task")));
  body.push_back(ast_.Assign(v_lp.Attr("launch_hccl_params").Attr("task_params"), params.Addr()));
  body.push_back(ast_.VarDecl(v_li, ast_.InitList({})));
  body.push_back(ast_.Assign(v_li.Attr("launch_type"), ast_.Var("", "ACL_RT_LAUNCH_HCCL")));
  body.push_back(ast_.Assign(v_li.Attr("task_info"), v_task_info.Addr()));
  body.push_back(ast_.Assign(v_li.Attr("launch_params"), v_lp.Addr()));
  body.push_back(ChkStatus(ast_.Call("ctx.launch_func", {ctx.Attr("instance_handle"), v_li.Addr()})));
  return body;
}

Status HcclTaskCodeBuilder::RenderDistHelper(std::vector<DeclNode *> &items) {
  std::vector<BodyItem> body;
  auto op = ast_.Var("const TaskDispatchInfo *", "op");
  auto ctx = ast_.Var("const DispatchOpContext &", "ctx");
  auto hccl = op.Arrow("dispatch_info").Attr("hccl");

  body.push_back(ast_.Call(
      "OM2_LOGD", {ast_.Str("DispatchHccl op=%s, stream_id=%u"), op.Arrow("op_name"), hccl.Attr("stream_id")}));

  body.push_back(ChkRt(RtSetTaskTag(op.Arrow("op_name"))));

  body.push_back(
      ast_.If(ctx.Attr("hccl_load_task") == "nullptr",
              {ast_.Call("OM2_LOGE", {ast_.Str("hccl host services are not available, op=%s"), op.Arrow("op_name")}),
               ast_.Return("ACL_ERROR_FAILURE")}));

  auto io_body = RenderIoAddrResolve(hccl, ctx);
  (void)body.insert(body.end(), io_body.begin(), io_body.end());

  auto v_iow = ast_.Var("", "iow_addrs");
  auto params_body = RenderTaskParamsAssemble(hccl, ctx, v_iow);
  (void)body.insert(body.end(), params_body.begin(), params_body.end());
  auto v_params = ast_.Var("", "params");

  if (build_data_.is_hcom_op && (build_data_.used_stream_num > 0)) {
    auto follow_body = RenderFollowStreams(hccl, ctx, v_params);
    (void)body.insert(body.end(), follow_body.begin(), follow_body.end());
  }

  auto v_att = ast_.Var("", "attached_streams");
  body.push_back(ast_.VarDecl(ast_.Var("std::vector<void *>", "attached_streams")));
  body.push_back(v_att.Resize(hccl.Attr("streams").Attr("attached_stream_num")));
  body.push_back(ast_.For(
      ast_.VarDecl("uint32_t", "i", ast_.UInt(0U)),
      ast_.Var("", "i") < hccl.Attr("streams").Attr("attached_stream_num"), ast_.PostInc(ast_.Var("", "i")),
      {ast_.Assign(v_att[ast_.Var("", "i")],
                   ctx.Attr("stream_list")[hccl.Attr("streams").Attr("attached_stream_ids")[ast_.Var("", "i")]])}));
  body.push_back(ast_.Assign(v_params.Attr("attached_streams"), v_att.Data()));
  body.push_back(ast_.Assign(v_params.Attr("attached_stream_num"), hccl.Attr("streams").Attr("attached_stream_num")));

  body.push_back(
      ast_.Call("OM2_LOGD",
                {ast_.Str("hccl params: type=%s, count=%llu, dtype=%d, reduce=%d, root=%d, need_refresh=%u, follow=%u, "
                          "attached=%u, ws=%p/%llu, hvd=%p"),
                 v_params.Attr("hccl_type"), ast_.StaticCast("unsigned long long", v_params.Attr("count")),
                 v_params.Attr("hccl_data_type"), v_params.Attr("hccl_reduce_op"), v_params.Attr("root_id"),
                 ast_.StaticCast("uint32_t", v_params.Attr("need_refresh")), v_params.Attr("follow_stream_num"),
                 v_params.Attr("attached_stream_num"), v_params.Attr("workspace_addr"),
                 ast_.StaticCast("unsigned long long", v_params.Attr("workspace_size")),
                 ast_.StaticCast("const void *", v_params.Attr("hvd_metas"))}));

  auto launch_body = RenderLaunch(hccl, ctx, v_params, op);
  (void)body.insert(body.end(), launch_body.begin(), launch_body.end());

  GE_ASSERT_SUCCESS(TaskCodeBuilderUtil::RenderDispatchFunc(ast_, kDispatchFuncName, body, items));
  return SUCCESS;
}

REGISTER_TASK_CODE_BUILDER(MODEL_TASK_HCCL, HcclTaskCodeBuilder);
}  // namespace ge
