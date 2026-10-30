/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cinttypes>
#include "framework/runtime/dump/data_dump_impl.h"
#include "framework/runtime/dump/dump_config.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/framework_types_internal.h"
#include "common/plugin/datatype_util.h"
#include "graph/def_types.h"
#include "rt_external.h"
#include "acl/acl_rt.h"

namespace ge {
namespace dump {
namespace {
constexpr uint32_t kAicpuLoadFlag = 1U;
constexpr uint32_t kAddrLength = static_cast<uint32_t>(sizeof(void *));
constexpr uint64_t kOpDebugShape = 2048U;
constexpr uint64_t kOpDebugSize = 2048U;
const std::string OP_DEBUG_NAME = "Node_OpDebug";
const std::string OP_DEBUG_TYPE = "Opdebug";

// TLV 载荷元数据日志：Get* 恒返回字段值（未 Set 即类型默认值），Has* 判 presence
std::string DumpTransShapeToStr(const std::vector<uint64_t> &shape) {
  std::string shape_str;
  for (size_t i = 0U; i < shape.size(); ++i) {
    if (i > 0U) {
      shape_str += ", ";
    }
    // 维度按 int64 语义解释，-1 表示未知维
    shape_str += std::to_string(static_cast<int64_t>(shape[i]));
  }
  return shape_str;
}

void DumpTransTensorToLog(const char *tag, size_t index, const DumpTransTensorInfo &tensor) {
  GELOGD("  %s[%zu]: addr=0x%" PRIx64 ", size=%" PRIu64
         ", format=%d, data_type=%d, "
         "addr_type=%u, offset=%" PRIu64 ", shape=[%s]",
         tag, index, tensor.GetAddress(), tensor.GetSize(), tensor.GetFormat(), tensor.GetDataType(),
         static_cast<uint32_t>(tensor.GetAddrType()), tensor.GetOffset(),
         DumpTransShapeToStr(tensor.GetShape()).c_str());
}

void DumpTransTaskToLog(size_t index, const DumpTransTaskInfo &task) {
  const size_t input_num = task.GetInputCount();
  const size_t output_num = task.GetOutputCount();
  const size_t workspace_num = task.GetWorkspaceCount();
  const size_t context_num = task.GetContextCount();
  GELOGD(
      "---------- Task[%zu]: task_id=%u, stream_id=%u, context_id=%u, thread_id=%u, "
      "task_type=%u, end_graph=%d",
      index, task.GetTaskId(), task.GetStreamId(), task.GetContextId(), task.GetThreadId(),
      static_cast<uint32_t>(task.GetTaskType()), task.GetEndGraph());
  GELOGD("Task[%zu]: op_name=%s, op_type=%s, input=%zu, output=%zu, workspace=%zu, context=%zu", index,
         task.GetOpName().c_str(), task.GetOpType().c_str(), input_num, output_num, workspace_num, context_num);

  for (size_t i = 0U; i < input_num; ++i) {
    const DumpTransInputInfo *input = nullptr;
    if ((task.GetInput(i, input) != DumpTransStatus::kOk) || (input == nullptr)) {
      continue;
    }
    DumpTransTensorToLog("input", i, *input);
  }
  for (size_t i = 0U; i < output_num; ++i) {
    const DumpTransOutputInfo *output = nullptr;
    if ((task.GetOutput(i, output) != DumpTransStatus::kOk) || (output == nullptr)) {
      continue;
    }
    DumpTransTensorToLog("output", i, *output);
    GELOGD("  output[%zu] original: name=%s, index=%d, data_type=%d, format=%d", i, output->GetOriginalName().c_str(),
           output->GetOriginalOutputIndex(), output->GetOriginalOutputDataType(), output->GetOriginalOutputFormat());
  }
  for (size_t i = 0U; i < workspace_num; ++i) {
    const DumpTransWorkspaceInfo *workspace = nullptr;
    if ((task.GetWorkspace(i, workspace) != DumpTransStatus::kOk) || (workspace == nullptr)) {
      continue;
    }
    GELOGD("  workspace[%zu]: addr=0x%" PRIx64 ", size=%" PRIu64 ", type=%u", i, workspace->GetDataAddr(),
           workspace->GetSize(), static_cast<uint32_t>(workspace->GetType()));
  }
  for (size_t i = 0U; i < context_num; ++i) {
    const DumpTransContextInfo *context = nullptr;
    if ((task.GetContext(i, context) != DumpTransStatus::kOk) || (context == nullptr)) {
      continue;
    }
    GELOGD("  context[%zu]: context_id=%u, thread_id=%u", i, context->GetContextId(), context->GetThreadId());
  }
}

void DumpTransportInfoToLog(const DumpTransportInfo &info) {
  GELOGD("========== Dump TransportInfo(TLV) Start ==========");
  const auto &model = info.GetModel();
  GELOGD("dump_path=%s, model_name=%s, model_id=%u, dump_step=%s, flag=%u, dump_data=%u", model.GetDumpPath().c_str(),
         model.GetModelName().c_str(), model.GetModelId(), model.GetDumpStep().c_str(), model.GetFlag(),
         static_cast<uint32_t>(model.GetDumpData()));
  GELOGD("step_id_addr=0x%" PRIx64 ", loop_cond_addr=0x%" PRIx64 ", iterations_per_loop_addr=0x%" PRIx64
         ", dump_switch_addr=0x%" PRIx64,
         model.GetStepIdAddr(), model.GetLoopCondAddr(), model.GetIterationsPerLoopAddr(), model.GetDumpSwitchAddr());

  const size_t task_num = info.GetTaskCount();
  GELOGD("task_count=%zu", task_num);
  for (size_t i = 0U; i < task_num; ++i) {
    const DumpTransTaskInfo *task = nullptr;
    if ((info.GetTask(i, task) != DumpTransStatus::kOk) || (task == nullptr)) {
      continue;
    }
    DumpTransTaskToLog(i, *task);
  }
  GELOGD("========== Dump TransportInfo(TLV) End ==========");
}
}  // namespace

DataDumpImpl::DataDumpImpl() : data_dump_version_(GetDumpDataVersionFromEnv()) {
  GELOGI("DataDumpImpl dump data version=%u (0=protobuf, 1=TLV).", data_dump_version_);
}

DataDumpImpl::~DataDumpImpl() {
  Clear();
}

Status DataDumpImpl::SaveTask(const GertModelTaskDesc &task_info, ModelTaskType task_type, rtStream_t stream,
                              bool is_op_debug) {
  const char *op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";
  const char *op_type = (task_info.op_type != nullptr) ? task_info.op_type : "";
  GELOGD("SaveTask: op_name=%s, task_id=%u, stream_id=%u, is_op_debug=%d", op_name, task_info.task_id,
         task_info.stream_id, is_op_debug);

  InnerDumpInfo dump_info = {};
  dump_info.task_id = static_cast<uint32_t>(task_info.task_id);
  dump_info.stream_id = static_cast<uint32_t>(task_info.stream_id);
  dump_info.context_id = static_cast<uint32_t>(task_info.context_id);
  dump_info.thread_id = static_cast<uint32_t>(task_info.thread_id);
  dump_info.task_type = task_type;
  dump_info.stream = stream;
  dump_info.is_op_debug = is_op_debug;
  dump_info.args_base = task_info.args_base;
  dump_info.args_size = task_info.args_size;
  dump_info.op_name = op_name;
  dump_info.op_type = op_type;
  dump_info.is_raw_address = task_info.is_raw_address;

  const auto copy_io_entries = [&op_name](const GertModelTaskIoEntry *entries, const uint32_t entry_num,
                                          std::vector<InnerTensorInfo> &inner_tensors) -> Status {
    if ((entry_num > 0U) && (entries == nullptr)) {
      GELOGE(PARAM_INVALID, "[Check][Param] OM2 task io entries is null, entry_num=%u.", entry_num);
      return PARAM_INVALID;
    }
    inner_tensors.reserve(entry_num);
    for (uint32_t i = 0U; i < entry_num; ++i) {
      const auto &entry = entries[i];
      if (entry.tensor == nullptr) {
        GELOGE(PARAM_INVALID, "[Check][Param] OM2 task io tensor is null, index=%u.", i);
        return PARAM_INVALID;
      }
      const auto &tensor = *entry.tensor;
      InnerTensorInfo inner_tensor{};
      inner_tensor.offset = entry.offset;
      inner_tensor.device_address = PtrToValue(tensor.GetAddr());
      inner_tensor.size = tensor.GetSize();
      // dump proto 的 data_type 写 IR(proto) 枚举，与 ge::DataType 数值错位
      inner_tensor.data_type = DataTypeUtil::GetIrDataType(tensor.GetDataType());
      inner_tensor.format = tensor.GetStorageFormat();
      if (tensor.GetStorageShape().GetDimNum() > 0U) {
        inner_tensor.shape_dims.clear();
        inner_tensor.shape_dims.reserve(tensor.GetStorageShape().GetDimNum());
        for (auto i = 0U; i < tensor.GetStorageShape().GetDimNum(); ++i) {
          inner_tensor.shape_dims.push_back(tensor.GetStorageShape().GetDim(i));
        }
      }
      inner_tensors.push_back(inner_tensor);
    }
    return SUCCESS;
  };

  Status ret = copy_io_entries(task_info.inputs, task_info.input_num, dump_info.inputs);
  if (ret != SUCCESS) {
    return ret;
  }
  ret = copy_io_entries(task_info.outputs, task_info.output_num, dump_info.outputs);
  if (ret != SUCCESS) {
    return ret;
  }

  if ((task_info.workspace_num > 0U) &&
      ((task_info.workspace_addrs == nullptr) || (task_info.workspace_sizes == nullptr))) {
    GELOGE(PARAM_INVALID, "[Check][Param] OM2 task workspace info is null, workspace_num=%u.", task_info.workspace_num);
    return PARAM_INVALID;
  }
  for (uint32_t i = 0U; i < task_info.workspace_num; ++i) {
    dump_info.workspace_addrs.push_back(task_info.workspace_addrs[i]);
    dump_info.workspace_sizes.push_back(task_info.workspace_sizes[i]);
  }

  task_list_.push_back(dump_info);
  return SUCCESS;
}

Status DataDumpImpl::ExecuteLoadDumpInfo(const toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) {
  std::string proto_str;
  const size_t proto_size = op_mapping_info.ByteSizeLong();
  const bool ret = op_mapping_info.SerializeToString(&proto_str);
  if ((!ret) || (proto_size == 0U)) {
    GELOGE(PARAM_INVALID, "[Call][SerializeToString] failed, proto size %zu.", proto_size);
    return PARAM_INVALID;
  }

  if (dev_mem_load_ != nullptr) {
    GELOGW("dev_mem_load_ has been used.");
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
  }

  aclError rt_ret = aclrtMalloc(&dev_mem_load_, proto_size, ACL_MEM_MALLOC_HUGE_FIRST);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "[Call][aclrtMalloc] failed, size:%zu, ret:%d", proto_size, rt_ret);
    return RT_FAILED;
  }

  rt_ret = aclrtMemcpy(dev_mem_load_, proto_size, proto_str.c_str(), proto_size, ACL_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "[Call][aclrtMemcpy] failed, size:%zu, ret:%d", proto_size, rt_ret);
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
    return RT_FAILED;
  }

  rt_ret = rtDatadumpInfoLoad(dev_mem_load_, static_cast<uint32_t>(proto_size));
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Call][rtDatadumpInfoLoad] failed, length:%zu, ret:%d", proto_size, rt_ret);
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
    return RT_FAILED;
  }

  load_flag_ = true;
  GELOGI("LoadDumpInfo success, proto size is: %zu.", proto_size);
  return SUCCESS;
}

Status DataDumpImpl::ExecuteLoadDumpInfo(const std::vector<uint8_t> &payload) {
  const size_t payload_size = payload.size();
  if (payload_size == 0U || payload_size > UINT32_MAX) {
    GELOGE(PARAM_INVALID, "Invalid dump payload size %zu.", payload_size);
    return PARAM_INVALID;
  }

  if (dev_mem_load_ != nullptr) {
    GELOGW("dev_mem_load_ has been used.");
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
  }

  aclError rt_ret = aclrtMalloc(&dev_mem_load_, payload_size, ACL_MEM_MALLOC_HUGE_FIRST);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "[Call][aclrtMalloc] failed, size:%zu, ret:%d", payload_size, rt_ret);
    return RT_FAILED;
  }

  rt_ret = aclrtMemcpy(dev_mem_load_, payload_size, payload.data(), payload_size, ACL_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "[Call][aclrtMemcpy] failed, size:%zu, ret:%d", payload_size, rt_ret);
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
    return RT_FAILED;
  }

  rt_ret = rtDatadumpInfoLoad(dev_mem_load_, static_cast<uint32_t>(payload_size));
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Call][rtDatadumpInfoLoad] failed, length:%zu, ret:%d", payload_size, rt_ret);
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
    return RT_FAILED;
  }

  load_flag_ = true;
  GELOGI("LoadDumpInfo success, payload size is: %zu.", payload_size);
  return SUCCESS;
}

Status DataDumpImpl::BuildOpMappingBasicInfo(const ModelDumpInfo &model_info,
                                             toolkit::aicpu::dump::OpMappingInfo &dump_op_mapping_info) {
  if (op_mapping_base_info_initialized_) {
    dump_op_mapping_info = op_mapping_base_info_;
    return ge::SUCCESS;
  }
  auto &op_mapping_info = op_mapping_base_info_;
  const char *model_name = (model_info.model_name != nullptr) ? model_info.model_name : "";
  op_mapping_info.set_dump_path(DumpConfig::Instance().GetDumpPath() + std::to_string(model_info.device_id) + "/");
  op_mapping_info.set_model_name(model_name);
  op_mapping_info.set_model_id(model_info.model_id);
  op_mapping_info.set_dump_step(DumpConfig::Instance().GetDumpStep());
  op_mapping_info.set_flag(kAicpuLoadFlag);

  // step_id_addr 分配设备内存并初始化为 0，先释放旧的
  // AICPU dump kernel 以 uint64_t 解引用 step_id_addr（dump_task.cpp: *(uint64_t *)stepIdAddr）
  // 计算 step 落盘目录，此处必须分配 8 字节，否则高 4 字节为脏数据会导致 step 目录错乱
  if (step_id_dev_addr_ != nullptr) {
    (void)aclrtFree(step_id_dev_addr_);
    step_id_dev_addr_ = nullptr;
  }

  void *step_id_dev_addr = nullptr;
  const aclError ret = aclrtMalloc(&step_id_dev_addr, sizeof(uint64_t), ACL_MEM_MALLOC_HUGE_FIRST);
  if (ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "Malloc step_id_addr failed, ret=%d", ret);
    return RT_FAILED;
  }
  const uint64_t zero_val = 0U;
  const aclError cpy_ret =
      aclrtMemcpy(step_id_dev_addr, sizeof(uint64_t), &zero_val, sizeof(uint64_t), ACL_MEMCPY_HOST_TO_DEVICE);
  if (cpy_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "Memcpy step_id_addr failed, ret=%d", cpy_ret);
    (void)aclrtFree(step_id_dev_addr);
    return RT_FAILED;
  }
  step_id_dev_addr_ = step_id_dev_addr;
  op_mapping_info.set_step_id_addr(PtrToValue(step_id_dev_addr));

  // loop_cond_addr 和 iterations_per_loop_addr 保持原逻辑
  if (model_info.loop_cond_addr != 0U) {
    op_mapping_info.set_loop_cond_addr(model_info.loop_cond_addr);
  }
  if (model_info.iterations_per_loop_addr != 0U) {
    op_mapping_info.set_iterations_per_loop_addr(model_info.iterations_per_loop_addr);
  }

  // 设置 dump_data
  const std::string dump_data_str = DumpConfig::Instance().GetDumpData();
  if (dump_data_str == "stats") {
    op_mapping_info.set_dump_data(toolkit::aicpu::dump::DumpData::STATS_DUMP_DATA);
  } else {
    op_mapping_info.set_dump_data(toolkit::aicpu::dump::DumpData::TENSOR_DUMP_DATA);
  }
  dump_op_mapping_info = op_mapping_base_info_;
  op_mapping_base_info_initialized_ = true;
  return ge::SUCCESS;
}

Status DataDumpImpl::BuildDumpTransportBasicInfo(const ModelDumpInfo &model_info, DumpTransportInfo &result) {
  if (dump_transport_base_info_initialized_) {
    result = dump_transport_base_info_;
    return ge::SUCCESS;
  }
  auto &dump_transport_info = dump_transport_base_info_.MutableModel();
  const char *model_name = (model_info.model_name != nullptr) ? model_info.model_name : "";
  dump_transport_info.SetDumpPath(DumpConfig::Instance().GetDumpPath() + std::to_string(model_info.device_id) + "/");
  dump_transport_info.SetModelName(model_name);
  dump_transport_info.SetModelId(model_info.model_id);
  dump_transport_info.SetDumpStep(DumpConfig::Instance().GetDumpStep());
  dump_transport_info.SetFlag(kAicpuLoadFlag);

  // step_id_addr 分配设备内存并初始化为 0，先释放旧的
  // AICPU dump kernel 以 uint64_t 解引用 step_id_addr（dump_task.cpp: *(uint64_t *)stepIdAddr）
  // 计算 step 落盘目录，此处必须分配 8 字节，否则高 4 字节为脏数据会导致 step 目录错乱
  if (step_id_dev_addr_ != nullptr) {
    (void)aclrtFree(step_id_dev_addr_);
    step_id_dev_addr_ = nullptr;
  }

  void *step_id_dev_addr = nullptr;
  const aclError ret = aclrtMalloc(&step_id_dev_addr, sizeof(uint64_t), ACL_MEM_MALLOC_HUGE_FIRST);
  if (ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "Malloc step_id_addr failed, ret=%d", ret);
    return RT_FAILED;
  }
  const uint64_t zero_val = 0U;
  const aclError cpy_ret =
      aclrtMemcpy(step_id_dev_addr, sizeof(uint64_t), &zero_val, sizeof(uint64_t), ACL_MEMCPY_HOST_TO_DEVICE);
  if (cpy_ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "Memcpy step_id_addr failed, ret=%d", cpy_ret);
    (void)aclrtFree(step_id_dev_addr);
    return RT_FAILED;
  }
  step_id_dev_addr_ = step_id_dev_addr;
  dump_transport_info.SetStepIdAddr(PtrToValue(step_id_dev_addr));

  // loop_cond_addr 和 iterations_per_loop_addr 保持原逻辑
  if (model_info.loop_cond_addr != 0U) {
    dump_transport_info.SetLoopCondAddr(model_info.loop_cond_addr);
  }
  if (model_info.iterations_per_loop_addr != 0U) {
    dump_transport_info.SetIterationsPerLoopAddr(model_info.iterations_per_loop_addr);
  }

  // 设置 dump_data
  const std::string dump_data_str = DumpConfig::Instance().GetDumpData();
  if (dump_data_str == "stats") {
    dump_transport_info.SetDumpData(DumpTransDumpData::kStats);
  } else {
    dump_transport_info.SetDumpData(DumpTransDumpData::kTensor);
  }
  result = dump_transport_base_info_;
  dump_transport_base_info_initialized_ = true;
  return ge::SUCCESS;
}

Status DataDumpImpl::UpdateStepId(uint64_t step_id, rtStream_t stream) {
  if (step_id_dev_addr_ == nullptr) {
    // 未开启 data dump 时 step 设备内存未分配，无需刷新
    return SUCCESS;
  }

  // 每次执行前把当前 step 刷到设备内存，
  // AICPU dump kernel 解引用 step_id_addr 计算 step 落盘目录
  step_id_host_val_ = step_id;
  aclError ret = ACL_SUCCESS;
  if (stream != nullptr) {
    ret = aclrtMemcpyAsync(step_id_dev_addr_, sizeof(uint64_t), &step_id_host_val_, sizeof(uint64_t),
                           ACL_MEMCPY_HOST_TO_BUF_TO_DEVICE, stream);
  } else {
    ret = aclrtMemcpy(step_id_dev_addr_, sizeof(uint64_t), &step_id_host_val_, sizeof(uint64_t),
                      ACL_MEMCPY_HOST_TO_DEVICE);
  }
  if (ret != ACL_SUCCESS) {
    GELOGE(RT_FAILED, "Update step_id_addr failed, step_id=%" PRIu64 ", ret=%d", step_id, ret);
    return RT_FAILED;
  }
  GELOGD("Update step_id to device, step_id=%" PRIu64 ", step_id_addr=%p, stream=%p", step_id, step_id_dev_addr_,
         stream);
  return SUCCESS;
}

Status DataDumpImpl::BuildTaskList(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const {
  for (const auto &dump_info : task_list_) {
    toolkit::aicpu::dump::Task *task = op_mapping_info.add_task();
    GE_CHECK_NOTNULL(task);
    GELOGD("BuildTaskList: task_id=%u, stream_id=%u, args_base=0x%lx, args_size=%zu, is_raw_address=%u",
           dump_info.task_id, dump_info.stream_id, dump_info.args_base, dump_info.args_size, dump_info.is_raw_address);
    task->set_task_id(dump_info.task_id);
    task->set_stream_id(dump_info.stream_id);
    task->set_context_id(dump_info.context_id);
    task->set_thread_id(dump_info.thread_id);

    // 设置 op 信息
    toolkit::aicpu::dump::Op *op = task->mutable_op();
    if (op != nullptr) {
      op->set_op_name(dump_info.op_name);
      op->set_op_type(dump_info.op_type);
    }

    BuildTaskInputs(dump_info, *task);
    BuildTaskOutputs(dump_info, *task);
    BuildTaskWorkspaces(dump_info, *task);
  }
  return SUCCESS;
}

void DataDumpImpl::BuildTaskInputs(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const {
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_input =
      (dump_mode == GE_DUMP_MODE_INPUT) || (dump_mode == GE_DUMP_MODE_ALL) || dump_info.is_op_debug;
  if (!need_dump_input) {
    GELOGD("Skip dump input for task_id=%u, dump_mode=%s, is_op_debug=%u", dump_info.task_id, dump_mode.c_str(),
           dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.inputs.size(); ++i) {
    toolkit::aicpu::dump::Input *input_tensor = task.add_input();
    const auto &tensor = dump_info.inputs[i];

    // 直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = tensor.device_address;
    auto addr_type = toolkit::aicpu::dump::AddressType::TRADITIONAL_ADDR;
    if (dump_info.is_raw_address) {
      addr_type = toolkit::aicpu::dump::AddressType::RAW_ADDR;
    } else if (dump_info.args_base != 0U && tensor.offset != UINT64_MAX) {
      device_address = dump_info.args_base + tensor.offset;
    }

    GELOGD(
        "BuildTaskInputs: task_id=%u, input[%zu], args_base=0x%lx, offset=0x%lx, device_address=0x%lx, size=%lu, "
        "is_raw=%d",
        dump_info.task_id, i, dump_info.args_base, static_cast<uint64_t>(tensor.offset), device_address, tensor.size,
        dump_info.is_raw_address);

    input_tensor->set_data_type(tensor.data_type);
    input_tensor->set_format(tensor.format);
    input_tensor->set_address(device_address);
    input_tensor->set_size(tensor.size);
    input_tensor->set_addr_type(addr_type);
    for (const auto dim : tensor.shape_dims) {
      input_tensor->mutable_shape()->add_dim(static_cast<uint64_t>(dim));
    }
  }
}

void DataDumpImpl::BuildTaskOutputs(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const {
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_output =
      (dump_mode == GE_DUMP_MODE_OUTPUT) || (dump_mode == GE_DUMP_MODE_ALL) || dump_info.is_op_debug;
  if (!need_dump_output) {
    GELOGD("Skip dump output for task_id=%u, dump_mode=%s, is_op_debug=%u", dump_info.task_id, dump_mode.c_str(),
           dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.outputs.size(); ++i) {
    toolkit::aicpu::dump::Output *output_tensor = task.add_output();
    const auto &tensor = dump_info.outputs[i];

    // 直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = tensor.device_address;
    auto addr_type = toolkit::aicpu::dump::AddressType::TRADITIONAL_ADDR;
    if (dump_info.is_raw_address) {
      addr_type = toolkit::aicpu::dump::AddressType::RAW_ADDR;
    } else if (dump_info.args_base != 0U && tensor.offset != UINT64_MAX) {
      device_address = dump_info.args_base + tensor.offset;
    }

    GELOGD(
        "BuildTaskOutputs: task_id=%u, output[%zu], args_base=0x%lx, offset=0x%lx, device_address=0x%lx, size=%lu, "
        "is_raw=%d",
        dump_info.task_id, i, dump_info.args_base, static_cast<uint64_t>(tensor.offset), device_address, tensor.size,
        dump_info.is_raw_address);

    output_tensor->set_data_type(tensor.data_type);
    output_tensor->set_format(tensor.format);
    output_tensor->set_address(device_address);
    output_tensor->set_size(tensor.size);
    output_tensor->set_addr_type(addr_type);
    for (const auto dim : tensor.shape_dims) {
      output_tensor->mutable_shape()->add_dim(static_cast<uint64_t>(dim));
    }
  }
}

void DataDumpImpl::BuildTaskWorkspaces(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const {
  // workspace 只在 op_debug 模式下才需要 dump（用于溢出/异常调试）
  if (!dump_info.is_op_debug) {
    GELOGD("Skip dump workspace for task_id=%u, is_op_debug=%u", dump_info.task_id, dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.workspace_addrs.size(); ++i) {
    toolkit::aicpu::dump::Workspace *workspace = task.add_space();
    GELOGD("BuildTaskWorkspaces: task_id=%u, workspace[%zu], data_addr=0x%lx, size=%lu", dump_info.task_id, i,
           dump_info.workspace_addrs[i], dump_info.workspace_sizes[i]);
    workspace->set_data_addr(dump_info.workspace_addrs[i]);
    workspace->set_size(dump_info.workspace_sizes[i]);
    workspace->set_type(toolkit::aicpu::dump::Workspace::LOG);
  }
}

Status DataDumpImpl::BuildTaskList(DumpTransportInfo &dump_transport_info) const {
  for (const auto &dump_info : task_list_) {
    DumpTransTaskInfo *task = &dump_transport_info.AddTask();
    GE_CHECK_NOTNULL(task);
    GELOGD("BuildTaskList: task_id=%u, stream_id=%u, args_base=0x%lx, args_size=%zu, is_raw_address=%u",
           dump_info.task_id, dump_info.stream_id, dump_info.args_base, dump_info.args_size, dump_info.is_raw_address);
    task->SetTaskId(dump_info.task_id);
    task->SetStreamId(dump_info.stream_id);
    task->SetContextId(dump_info.context_id);
    task->SetThreadId(dump_info.thread_id);

    task->SetOpName(dump_info.op_name);
    task->SetOpType(dump_info.op_type);

    BuildTaskInputs(dump_info, *task);
    BuildTaskOutputs(dump_info, *task);
    BuildTaskWorkspaces(dump_info, *task);
  }
  return SUCCESS;
}

void DataDumpImpl::BuildTaskInputs(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const {
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_input =
      (dump_mode == GE_DUMP_MODE_INPUT) || (dump_mode == GE_DUMP_MODE_ALL) || dump_info.is_op_debug;
  if (!need_dump_input) {
    GELOGD("Skip dump input for task_id=%u, dump_mode=%s, is_op_debug=%u", dump_info.task_id, dump_mode.c_str(),
           dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.inputs.size(); ++i) {
    DumpTransInputInfo *input_tensor = &task.AddInput();
    const auto &tensor = dump_info.inputs[i];

    // 对齐 v1：直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = tensor.device_address;
    auto addr_type = DumpTransAddressType::kTraditional;
    if (dump_info.is_raw_address) {
      addr_type = DumpTransAddressType::kRaw;
    } else if (dump_info.args_base != 0U && tensor.offset != UINT64_MAX) {
      device_address = dump_info.args_base + tensor.offset;
    }

    GELOGD(
        "BuildTaskInputs: task_id=%u, input[%zu], args_base=0x%lx, offset=0x%lx, device_address=0x%lx, size=%lu, "
        "is_raw=%d",
        dump_info.task_id, i, dump_info.args_base, static_cast<uint64_t>(tensor.offset), device_address, tensor.size,
        dump_info.is_raw_address);

    input_tensor->SetDataType(tensor.data_type);
    input_tensor->SetFormat(tensor.format);
    input_tensor->SetAddress(device_address);
    input_tensor->SetSize(tensor.size);
    input_tensor->SetAddrType(addr_type);
    input_tensor->SetShape(std::vector<uint64_t>(tensor.shape_dims.begin(), tensor.shape_dims.end()));
  }
}

void DataDumpImpl::BuildTaskOutputs(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const {
  const std::string &dump_mode = DumpConfig::Instance().GetDumpMode();
  const bool need_dump_output =
      (dump_mode == GE_DUMP_MODE_OUTPUT) || (dump_mode == GE_DUMP_MODE_ALL) || dump_info.is_op_debug;
  if (!need_dump_output) {
    GELOGD("Skip dump output for task_id=%u, dump_mode=%s, is_op_debug=%u", dump_info.task_id, dump_mode.c_str(),
           dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.outputs.size(); ++i) {
    DumpTransOutputInfo *output_tensor = &task.AddOutput();
    const auto &tensor = dump_info.outputs[i];

    // 对齐 v1：直接写 args + offset 地址给 AICPU，不解引用
    uint64_t device_address = tensor.device_address;
    auto addr_type = DumpTransAddressType::kTraditional;
    if (dump_info.is_raw_address) {
      addr_type = DumpTransAddressType::kRaw;
    } else if (dump_info.args_base != 0U && tensor.offset != UINT64_MAX) {
      device_address = dump_info.args_base + tensor.offset;
    }

    GELOGD(
        "BuildTaskOutputs: task_id=%u, output[%zu], args_base=0x%lx, offset=0x%lx, device_address=0x%lx, size=%lu, "
        "is_raw=%d",
        dump_info.task_id, i, dump_info.args_base, static_cast<uint64_t>(tensor.offset), device_address, tensor.size,
        dump_info.is_raw_address);

    output_tensor->SetDataType(tensor.data_type);
    output_tensor->SetFormat(tensor.format);
    output_tensor->SetAddress(device_address);
    output_tensor->SetSize(tensor.size);
    output_tensor->SetAddrType(addr_type);
    output_tensor->SetShape(std::vector<uint64_t>(tensor.shape_dims.begin(), tensor.shape_dims.end()));
  }
}

void DataDumpImpl::BuildTaskWorkspaces(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const {
  // workspace 只在 op_debug 模式下才需要 dump（用于溢出/异常分析）
  if (!dump_info.is_op_debug) {
    GELOGD("Skip dump workspace for task_id=%u, is_op_debug=%u", dump_info.task_id, dump_info.is_op_debug);
    return;
  }

  for (size_t i = 0; i < dump_info.workspace_addrs.size(); ++i) {
    DumpTransWorkspaceInfo *workspace = &task.AddWorkspace();
    GELOGD("BuildTaskWorkspaces: task_id=%u, workspace[%zu], data_addr=0x%lx, size=%lu", dump_info.task_id, i,
           dump_info.workspace_addrs[i], dump_info.workspace_sizes[i]);
    workspace->SetDataAddr(dump_info.workspace_addrs[i]);
    workspace->SetSize(dump_info.workspace_sizes[i]);
    workspace->SetType(DumpTransWorkspaceType::kLog);
  }
}

void DataDumpImpl::SetOpDebugInfo(uint32_t task_id, uint32_t stream_id, void *debug_addr) {
  is_op_debug_ = true;
  op_debug_task_id_ = task_id;
  op_debug_stream_id_ = stream_id;
  op_debug_addr_ = debug_addr;
}

void DataDumpImpl::BuildOpDebugTask(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const {
  if (!is_op_debug_) {
    return;
  }

  GELOGI("Add op_debug_info to aicpu, task_id=%u, stream_id=%u", op_debug_task_id_, op_debug_stream_id_);

  toolkit::aicpu::dump::Task task;
  task.set_end_graph(false);
  task.set_task_id(op_debug_task_id_);
  task.set_stream_id(op_debug_stream_id_);
  task.mutable_op()->set_op_name(OP_DEBUG_NAME);
  task.mutable_op()->set_op_type(OP_DEBUG_TYPE);

  // set output
  toolkit::aicpu::dump::Output output;
  output.set_original_name(OP_DEBUG_NAME);
  output.set_original_output_index(0);
  output.set_original_output_format(FORMAT_ND);
  output.set_original_output_data_type(DT_UINT8);
  output.set_data_type(DT_UINT8);
  output.set_format(FORMAT_ND);
  output.mutable_shape()->add_dim(kOpDebugShape);
  output.set_address(PtrToValue(op_debug_addr_));
  output.set_size(kOpDebugSize);
  output.set_addr_type(toolkit::aicpu::dump::AddressType::TRADITIONAL_ADDR);

  task.mutable_output()->Add(std::move(output));
  op_mapping_info.mutable_task()->Add(std::move(task));
}

void DataDumpImpl::BuildOpDebugTask(DumpTransportInfo &dump_transport_info) const {
  if (!is_op_debug_) {
    return;
  }

  GELOGI("Add op_debug_info to aicpu, task_id=%u, stream_id=%u", op_debug_task_id_, op_debug_stream_id_);

  auto &task = dump_transport_info.AddTask();
  task.SetEndGraph(false);
  task.SetTaskId(op_debug_task_id_);
  task.SetStreamId(op_debug_stream_id_);
  task.SetOpName(OP_DEBUG_NAME);
  task.SetOpType(OP_DEBUG_TYPE);

  // set output
  auto &output = task.AddOutput();
  output.SetOriginalName(OP_DEBUG_NAME);
  output.SetOriginalOutputIndex(0);
  output.SetOriginalOutputFormat(FORMAT_ND);
  output.SetOriginalOutputDataType(DT_UINT8);
  output.SetDataType(DT_UINT8);
  output.SetFormat(FORMAT_ND);
  output.SetShape({kOpDebugShape});
  output.SetAddress(PtrToValue(op_debug_addr_));
  output.SetSize(kOpDebugSize);
  output.SetAddrType(DumpTransAddressType::kTraditional);
}

Status DataDumpImpl::BuildAndLoadOpMappingInfo(const ModelDumpInfo &model_info) {
  GELOGI("BuildAndLoadOpMappingInfo: model_id=%u, task_count=%zu, dump_data=%s, dump_mode=%s, is_op_debug=%d",
         model_info.model_id, task_list_.size(), DumpConfig::Instance().GetDumpData().c_str(),
         DumpConfig::Instance().GetDumpMode().c_str(), is_op_debug_);

  // 如果没有普通算子要 dump，也没有 overflow dump，直接返回
  if (task_list_.empty() && !is_op_debug_) {
    GELOGI("No task to dump, skip build and load op mapping info");
    return SUCCESS;
  }

  toolkit::aicpu::dump::OpMappingInfo op_mapping_info;
  Status ret = BuildOpMappingBasicInfo(model_info, op_mapping_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "Build op mapping basic info failed, ret=%u", ret);
    return ret;
  }

  ret = BuildTaskList(op_mapping_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][TaskList] failed, ret:%u", ret);
    return ret;
  }

  // 添加 overflow dump 的特殊 Task（包含 p2p_debug_addr）
  BuildOpDebugTask(op_mapping_info);

  // 打印 op_mapping_info 便于调试（和 test1.cpp 格式对齐）
  GELOGD("========== Dump OpMappingInfo Start ==========");
  GELOGD("dump_path: %s", op_mapping_info.dump_path().c_str());
  GELOGD("model_name: %s", op_mapping_info.model_name().c_str());
  GELOGD("model_id: %u", op_mapping_info.model_id());
  GELOGD("step_id_addr: 0x%lx", op_mapping_info.step_id_addr());
  GELOGD("loop_cond_addr: 0x%lx", op_mapping_info.loop_cond_addr());
  GELOGD("iterations_per_loop_addr: 0x%lx", op_mapping_info.iterations_per_loop_addr());
  GELOGD("flag: %u", op_mapping_info.flag());
  GELOGD("dump_step: %s", op_mapping_info.dump_step().c_str());
  GELOGD("dump_data: %d", op_mapping_info.dump_data());
  GELOGD("task count: %d", op_mapping_info.task_size());

  for (int32_t i = 0; i < op_mapping_info.task_size(); ++i) {
    const auto &task = op_mapping_info.task(i);
    GELOGD("---------- Task[%d] ----------", i);
    GELOGD("  task_id: %u", task.task_id());
    GELOGD("  stream_id: %u", task.stream_id());
    GELOGD("  context_id: %u", task.context_id());
    GELOGD("  thread_id: %u", task.thread_id());
    GELOGD("  op_name: %s", task.op().op_name().c_str());
    GELOGD("  op_type: %s", task.op().op_type().c_str());
    GELOGD("  end_graph: %d", task.end_graph());
    GELOGD("  input count: %d", task.input_size());
    for (int32_t j = 0; j < task.input_size(); ++j) {
      const auto &input = task.input(j);
      std::string shape_str;
      for (int32_t k = 0; k < input.shape().dim_size(); ++k) {
        shape_str += (k == 0 ? "" : ", ") + std::to_string(input.shape().dim(k));
      }
      GELOGD(
          "    input[%d]: addr=0x%lx, size=%u, format=%d, data_type=%d, "
          "addr_type=%d, offset=%lu, shape=[%s]",
          j, input.address(), static_cast<uint32_t>(input.size()), input.format(), input.data_type(), input.addr_type(),
          input.offset(), shape_str.c_str());
    }
    GELOGD("  output count: %d", task.output_size());
    for (int32_t j = 0; j < task.output_size(); ++j) {
      const auto &output = task.output(j);
      std::string shape_str;
      for (int32_t k = 0; k < output.shape().dim_size(); ++k) {
        shape_str += (k == 0 ? "" : ", ") + std::to_string(output.shape().dim(k));
      }
      GELOGD(
          "    output[%d]: addr=0x%lx, size=%u, format=%d, data_type=%d, "
          "addr_type=%d, offset=%lu, shape=[%s]",
          j, output.address(), static_cast<uint32_t>(output.size()), output.format(), output.data_type(),
          output.addr_type(), output.offset(), shape_str.c_str());
    }
    GELOGD("  context count: %d", task.context_size());
    for (int32_t j = 0; j < task.context_size(); ++j) {
      const auto &context = task.context(j);
      GELOGD("    context[%d]: context_id=%u, thread_id=%u", j, context.context_id(), context.thread_id());
    }
    GELOGD("  workspace count: %d", task.space_size());
    for (int32_t j = 0; j < task.space_size(); ++j) {
      const auto &ws = task.space(j);
      GELOGD("    workspace[%d]: addr=0x%lx, size=%u, type=%d", j, ws.data_addr(), static_cast<uint32_t>(ws.size()),
             ws.type());
    }
  }
  GELOGD("========== Dump OpMappingInfo End ==========");

  ret = ExecuteLoadDumpInfo(op_mapping_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Execute][LoadDumpInfo] failed, ret:%u", ret);
    return ret;
  }

  op_mapping_info_ = std::move(op_mapping_info);
  GELOGI("BuildAndLoadOpMappingInfo success, task_count=%zu", task_list_.size());
  return SUCCESS;
}

Status DataDumpImpl::BuildAndLoadDumpTransportInfo(const ModelDumpInfo &model_info) {
  GELOGI("BuildAndLoadDumpTransportInfo: model_id=%u, task_count=%zu, dump_data=%s, dump_mode=%s, is_op_debug=%d",
         model_info.model_id, task_list_.size(), DumpConfig::Instance().GetDumpData().c_str(),
         DumpConfig::Instance().GetDumpMode().c_str(), is_op_debug_);

  // 如果没有普通算子要 dump，也没有 overflow dump，直接返回
  if (task_list_.empty() && !is_op_debug_) {
    GELOGI("No task to dump, skip build and load op mapping info");
    return SUCCESS;
  }

  DumpTransportInfo dump_transport_info;
  Status ret = BuildDumpTransportBasicInfo(model_info, dump_transport_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "Build op mapping basic info failed, ret=%u", ret);
    return ret;
  }

  ret = BuildTaskList(dump_transport_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][TaskList] failed, ret:%u", ret);
    return ret;
  }

  // 添加 overflow dump 的特殊 Task（包含 p2p_debug_addr）
  BuildOpDebugTask(dump_transport_info);

  // 打印 TLV 载荷元数据日志（与 payload 一一对应，序列化前打印）
  DumpTransportInfoToLog(dump_transport_info);

  std::vector<uint8_t> payload;
  const auto encode_status = dump_transport_info.Serialize(payload);
  if (encode_status != DumpTransStatus::kOk) {
    GELOGE(PARAM_INVALID, "Serialize dump payload failed, model_id=%u, version=%u, status=%u.", model_info.model_id,
           dump_transport_info.GetVersion(), static_cast<uint32_t>(encode_status));
    return PARAM_INVALID;
  }
  ret = ExecuteLoadDumpInfo(payload);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Execute][LoadDumpInfo] failed, ret:%u", ret);
    return ret;
  }

  dump_transport_info_ = std::move(dump_transport_info);
  GELOGI("BuildAndLoadDumpTransportInfo success, task_count=%zu", task_list_.size());
  return SUCCESS;
}

void DataDumpImpl::Clear() {
  if (dev_mem_load_ != nullptr) {
    (void)aclrtFree(dev_mem_load_);
    dev_mem_load_ = nullptr;
  }
  if (step_id_dev_addr_ != nullptr) {
    (void)aclrtFree(step_id_dev_addr_);
    step_id_dev_addr_ = nullptr;
  }
  task_list_.clear();
  op_mapping_info_.Clear();
  op_mapping_base_info_.Clear();
  op_mapping_base_info_initialized_ = false;
  dump_transport_info_.Clear();
  dump_transport_base_info_.Clear();
  dump_transport_base_info_initialized_ = false;
  load_flag_ = false;
}

}  // namespace dump
}  // namespace ge
