/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <set>
#include "framework/runtime/dump/dump_op_impl.h"
#include "framework/runtime/dump/dump_config.h"
#include "framework/common/debug/ge_log.h"
#include "common/plugin/datatype_util.h"
#include "rt_external.h"
#include "acl/acl_rt.h"
#include "aicpu_task_struct.h"
#include <array>

namespace ge {
namespace dump {
namespace {
const std::string kDumpKernelsDumpOp = "DumpDataInfo";

// Local replacement of ge::AclrtMalloc: device (HBM) memory is allocated through the
// ACL high-bandwidth allocation policy, which is the only memory type ever requested here.
aclError DumpAclrtMalloc(void **ptr, size_t size) {
  if (ptr == nullptr) {
    return ACL_ERROR_INVALID_PARAM;
  }
  *ptr = nullptr;
  return aclrtMalloc(ptr, size, ACL_MEM_TYPE_HIGH_BAND_WIDTH);
}
}  // namespace

DumpOp::~DumpOp() {
  if (payload_dev_mem_ != nullptr) {
    (void)aclrtFree(payload_dev_mem_);
    payload_dev_mem_ = nullptr;
  }
  payload_dev_mem_capacity_ = 0U;

  if (payload_size_dev_mem_ != nullptr) {
    (void)aclrtFree(payload_size_dev_mem_);
    payload_size_dev_mem_ = nullptr;
  }
}

Status DumpOp::ExecutorDumpOp(const std::string &op_name, aclrtStream stream) {
  const Status submit_status = SubmitSerializedPayload(op_name);
  if (submit_status != SUCCESS) {
    return submit_status;
  }
  return LaunchDumpKernel(op_name, stream);
}

// 序列化 TLV 载荷并搬运到 Device（DumpTransportInfo::Serialize）
Status DumpOp::SubmitSerializedPayload(const std::string &op_name) {
  std::vector<uint8_t> payload;
  const auto encode_status = dump_transport_info_.Serialize(payload);
  if (encode_status != DumpTransStatus::kOk) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "Serialize dump payload failed, op=%s, version=%u, status=%u.", op_name.c_str(),
           dump_transport_info_.GetVersion(), static_cast<uint32_t>(encode_status));
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  return PayloadMallocAndMemcpy(payload);
}

// 组装 AicpuParamHead + 载荷/长度两个设备指针，拉起 AICPU dump kernel
Status DumpOp::LaunchDumpKernel(const std::string &op_name, aclrtStream stream) {
  void *data_dev_mem = payload_dev_mem_;
  void *size_dev_mem = payload_size_dev_mem_;

  constexpr uint32_t io_addr_num = 2U;
  constexpr uint32_t args_size =
      static_cast<uint32_t>(sizeof(aicpu::AicpuParamHead)) + (io_addr_num * static_cast<uint32_t>(sizeof(uint64_t)));
  std::array<uint8_t, args_size> args = {};
  size_t args_pos = 0UL;
  aicpu::AicpuParamHead &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(&args[args_pos])));
  args_pos += sizeof(aicpu::AicpuParamHead);
  param_head.length = args_size;
  param_head.ioAddrNum = io_addr_num;
  *(static_cast<uint64_t *>(static_cast<void *>(&args[args_pos]))) =
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(data_dev_mem));
  args_pos += sizeof(uint64_t);
  *(reinterpret_cast<uint64_t *>(static_cast<void *>(&args[args_pos]))) =
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(size_dev_mem));
  rtArgsEx_t args_for_launch = {};
  args_for_launch.args = &args[0U];
  args_for_launch.isNoNeedH2DCopy = 0U;
  args_for_launch.argsSize = args_size;
  const rtError_t rt_ret = rtCpuKernelLaunchWithFlag(nullptr, kDumpKernelsDumpOp.c_str(), 1U, &args_for_launch, nullptr,
                                                     stream, RT_KERNEL_DEFAULT);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][rtCpuKernelLaunchWithFlag]Failed, ret %d", rt_ret);
    REPORT_INNER_ERR_MSG("E19999", "Call rtCpuKernelLaunchWithFlag failed, ret %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  GELOGI("Kernel launch dump op %s success", op_name.c_str());
  return SUCCESS;
}

Status DumpOp::PayloadMallocAndMemcpy(const std::vector<uint8_t> &payload) {
  const size_t payload_size = payload.size();
  size_t payload_capacity = payload_size;

  GE_FREE_RT_LOG(payload_dev_mem_);
  payload_dev_mem_capacity_ = 0U;
  aclError rt_ret = DumpAclrtMalloc(&payload_dev_mem_, payload_capacity);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][aclrtMalloc]Failed, ret: %d", rt_ret);
    REPORT_INNER_ERR_MSG("E19999", "Call aclrtMalloc failed, ret: %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  payload_dev_mem_capacity_ = payload_capacity;

  rt_ret =
      aclrtMemcpy(payload_dev_mem_, payload_dev_mem_capacity_, payload.data(), payload_size, ACL_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][aclrtMemcpy]Failed, ret: %d", rt_ret);
    REPORT_INNER_ERR_MSG("E19999", "Call aclrtMemcpy failed, ret: %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  GE_FREE_RT_LOG(payload_size_dev_mem_);
  rt_ret = DumpAclrtMalloc(&payload_size_dev_mem_, sizeof(size_t));
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][aclrtMalloc]Failed, ret: %d", rt_ret);
    REPORT_INNER_ERR_MSG("E19999", "Call aclrtMalloc failed, ret: %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  rt_ret = aclrtMemcpy(payload_size_dev_mem_, sizeof(size_t), &payload_size, sizeof(size_t), ACL_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][aclrtMemcpy]Failed, ret %d", rt_ret);
    REPORT_INNER_ERR_MSG("E19999", "Call aclrtMemcpy failed, ret %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return SUCCESS;
}

Status DumpOp::BuildTaskInputs(const GertModelTaskDesc &task_desc, const std::string &model_name,
                               const std::string &root_graph_name) {
  DumpTransTaskInfo *task = &dump_transport_info_.AddTask();
  GE_CHECK_NOTNULL(task);
  aclError rt_ret = SetTaskBasicInfo(task_desc, task);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][SetTaskBasicInfo]Failed, ret: %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return FillTlvInputTensors(task, task_desc, model_name, root_graph_name);
}

Status DumpOp::FillTlvInputTensors(DumpTransTaskInfo *task, const GertModelTaskDesc &task_desc,
                                   const std::string &model_name, const std::string &root_graph_name) const {
  if (task_desc.inputs == nullptr) {
    return SUCCESS;
  }
  const char *op_name = (task_desc.op_name != nullptr) ? task_desc.op_name : "";
  const char *op_type = (task_desc.op_type != nullptr) ? task_desc.op_type : "";
  std::set<uint32_t> input_blacklist;
  DumpConfig::Instance().GetInputBlacklistIndices(model_name, root_graph_name, op_name, op_type,
                                                  static_cast<uint32_t>(task_desc.input_num), input_blacklist);
  for (uint32_t i = 0U; i < task_desc.input_num; ++i) {
    if (input_blacklist.count(i) != 0U) {
      GELOGI("[OM2 Dump] Node %s input[%u] is in blacklist for model %s, skip to dump this input.", op_name, i,
             model_name.c_str());
      continue;
    }
    const auto &entry = task_desc.inputs[i];
    if (entry.tensor == nullptr) {
      GELOGE(PARAM_INVALID, "[Check][Param] OM2 task io tensor is null, index=%u.", i);
      return PARAM_INVALID;
    }
    DumpTransInputInfo *input_tensor = &task->AddInput();
    const auto &tensor = *entry.tensor;

    // 对齐 v1：直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = reinterpret_cast<uint64_t>(tensor.GetAddr());
    auto addr_type = DumpTransAddressType::kTraditional;

    GELOGD("BuildTaskInputs: task_id=%u, input[%zu], device_address=0x%lx, size=%lu", task_desc.task_id, i,
           device_address, tensor.GetSize());
    input_tensor->SetDataType(DataTypeUtil::GetIrDataType(tensor.GetDataType()));
    input_tensor->SetFormat(tensor.GetStorageFormat());
    input_tensor->SetAddress(device_address);
    input_tensor->SetSize(tensor.GetSize());
    input_tensor->SetAddrType(addr_type);
    std::vector<uint64_t> shape;
    std::vector<uint64_t> origin_shape;
    for (size_t dim = 0U; dim < tensor.GetStorageShape().GetDimNum(); ++dim) {
      shape.push_back(static_cast<uint64_t>(tensor.GetStorageShape().GetDim(dim)));
    }
    for (size_t dim = 0U; dim < tensor.GetOriginShape().GetDimNum(); ++dim) {
      origin_shape.push_back(static_cast<uint64_t>(tensor.GetOriginShape().GetDim(dim)));
    }
    input_tensor->SetShape(shape);
    // 对齐 v1 DumpOp::DumpInput 的 origin_shape
    input_tensor->SetOriginShape(origin_shape);
  }
  return SUCCESS;
}

Status DumpOp::BuildTaskOutputs(const GertModelTaskDesc &task_desc, const std::string &model_name,
                                const std::string &root_graph_name) {
  DumpTransTaskInfo *task = &dump_transport_info_.AddTask();
  GE_CHECK_NOTNULL(task);
  aclError rt_ret = SetTaskBasicInfo(task_desc, task);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_ERROR_TO_GE_STATUS(rt_ret), "[Call][SetTaskBasicInfo]Failed, ret: %d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return FillTlvOutputTensors(task, task_desc, model_name, root_graph_name);
}

Status DumpOp::FillTlvOutputTensors(DumpTransTaskInfo *task, const GertModelTaskDesc &task_desc,
                                    const std::string &model_name, const std::string &root_graph_name) const {
  if (task_desc.outputs == nullptr) {
    return SUCCESS;
  }
  const char *op_name = (task_desc.op_name != nullptr) ? task_desc.op_name : "";
  const char *op_type = (task_desc.op_type != nullptr) ? task_desc.op_type : "";
  std::set<uint32_t> output_blacklist;
  DumpConfig::Instance().GetOutputBlacklistIndices(model_name, root_graph_name, op_name, op_type,
                                                   static_cast<uint32_t>(task_desc.output_num), output_blacklist);
  for (uint32_t i = 0U; i < task_desc.output_num; ++i) {
    if (output_blacklist.count(i) != 0U) {
      GELOGI("[OM2 Dump] Node %s output[%u] is in blacklist for model %s, skip to dump this output.", op_name, i,
             model_name.c_str());
      continue;
    }
    const auto &entry = task_desc.outputs[i];
    if (entry.tensor == nullptr) {
      GELOGE(PARAM_INVALID, "[Check][Param] OM2 task io tensor is null, index=%u.", i);
      return PARAM_INVALID;
    }
    DumpTransOutputInfo *output_tensor = &task->AddOutput();
    const auto &tensor = *entry.tensor;

    // 对齐 v1：直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = reinterpret_cast<uint64_t>(tensor.GetAddr());
    auto addr_type = DumpTransAddressType::kTraditional;

    GELOGD("BuildTaskOutputs: task_id=%u, input[%zu], device_address=0x%lx, size=%lu", task_desc.task_id, i,
           device_address, tensor.GetSize());
    output_tensor->SetDataType(DataTypeUtil::GetIrDataType(tensor.GetDataType()));
    output_tensor->SetFormat(tensor.GetStorageFormat());
    output_tensor->SetAddress(device_address);
    output_tensor->SetSize(tensor.GetSize());
    output_tensor->SetAddrType(addr_type);
    std::vector<uint64_t> shape;
    std::vector<uint64_t> origin_shape;
    for (size_t dim = 0U; dim < tensor.GetStorageShape().GetDimNum(); ++dim) {
      shape.push_back(static_cast<uint64_t>(tensor.GetStorageShape().GetDim(dim)));
    }
    for (size_t dim = 0U; dim < tensor.GetOriginShape().GetDimNum(); ++dim) {
      origin_shape.push_back(static_cast<uint64_t>(tensor.GetOriginShape().GetDim(dim)));
    }
    output_tensor->SetShape(shape);
    // 对齐 v1 DumpOp::DumpOutput 的 origin_shape
    output_tensor->SetOriginShape(origin_shape);
  }
  return SUCCESS;
}

Status DumpOp::SetTaskBasicInfo(const GertModelTaskDesc &task_desc, DumpTransTaskInfo *task) {
  const char *op_name = (task_desc.op_name != nullptr) ? task_desc.op_name : "";
  const char *op_type = (task_desc.op_type != nullptr) ? task_desc.op_type : "";
  GELOGD("SetTaskBasicInfo: op_name=%s, task_id=%u, stream_id=%u", op_name, task_desc.task_id, task_desc.stream_id);
  GE_CHECK_NOTNULL(task);
  task->SetTaskId(static_cast<uint32_t>(task_desc.task_id));
  task->SetStreamId(static_cast<uint32_t>(task_desc.stream_id));
  task->SetContextId(static_cast<uint32_t>(task_desc.context_id));
  task->SetThreadId(static_cast<uint32_t>(task_desc.thread_id));

  task->SetOpName(op_name);
  task->SetOpType(op_type);
  return SUCCESS;
}
}  // namespace dump
}  // namespace ge
