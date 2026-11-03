/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DATA_DUMP_IMPL_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DATA_DUMP_IMPL_H_

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include "framework/common/ge_types.h"
#include "framework/runtime/dump/model_dump_manager.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "rt_external_base.h"

// protobuf版本头文件
#include "proto/op_mapping.pb.h"
// TLV格式的头文件
#include "framework/runtime/dump/dump_transport_info.h"

namespace ge {
namespace dump {

class DataDumpImpl {
 public:
  DataDumpImpl();
  ~DataDumpImpl();

  Status SaveTask(const GertModelTaskDesc &task_info, ModelTaskType task_type, rtStream_t stream, bool is_op_debug);

  // watcher 场景：记录非 watcher 算子的 task_id/stream_id/op_name（对齐 v1
  // DataDumper::SaveLayerOpInfoOnWatcherMode），用于给 watcher 节点生成 <layerOp>_To_<watcherNode> 的 dump task
  void SaveLayerOpInfo(uint32_t task_id, uint32_t stream_id, const std::string &op_name);

  // 每次模型执行前刷新当前 step 到设备内存，AICPU dump kernel 解引用 step_id_addr 计算 step 落盘目录
  Status UpdateStepId(uint64_t step_id, rtStream_t stream);

  void Clear();

  // Overflow dump 相关信息
  void SetOpDebugInfo(uint32_t task_id, uint32_t stream_id, void *debug_addr);

  Status BuildAndLoadOpMappingInfo(const ModelDumpInfo &model_info);
  Status BuildOpMappingBasicInfo(const ModelDumpInfo &model_info,
                                 toolkit::aicpu::dump::OpMappingInfo &dump_op_mapping_info);

  Status BuildAndLoadDumpTransportInfo(const ModelDumpInfo &model_info);
  Status BuildDumpTransportBasicInfo(const ModelDumpInfo &model_info, DumpTransportInfo &dump_transport_info);

  bool IsProtoVer() const {
    return data_dump_version_ == 0U;
  }

 private:
  struct InnerTensorInfo {
    uint64_t offset;
    uint64_t device_address;
    uint64_t size;
    int32_t data_type;
    int32_t format;
    // 对齐 v1 GenerateOutput/GenerateInput：原始格式/原始 dtype/proto 侧 origin_shape
    int32_t origin_format;
    int32_t origin_data_type;
    std::vector<int64_t> shape_dims;
    std::vector<int64_t> origin_shape_dims;
  };

  struct InnerDumpInfo {
    uint32_t task_id;
    uint32_t stream_id;
    uint32_t context_id;
    uint32_t thread_id;
    ModelTaskType task_type;
    rtStream_t stream;
    bool is_op_debug;
    uintptr_t args_base;
    size_t args_size;
    std::string op_name;
    std::string op_type;
    bool is_raw_address;
    std::vector<InnerTensorInfo> inputs;
    std::vector<InnerTensorInfo> outputs;
    std::vector<uint64_t> workspace_addrs;
    std::vector<uint64_t> workspace_sizes;
  };

  // protobuf 构造modelinfo、taskinfo相关接口
  Status ExecuteLoadDumpInfo(const toolkit::aicpu::dump::OpMappingInfo &op_mapping_info);

  void InitDumpInfo(const GertModelTaskDesc &task_info, ModelTaskType task_type, rtStream_t stream, bool is_op_debug,
                    InnerDumpInfo &dump_info) const;

  Status CopyTaskIoEntries(const GertModelTaskIoEntry *entries, uint32_t entry_num,
                           std::vector<InnerTensorInfo> &inner_tensors) const;

  Status CopyTaskWorkspaces(const GertModelTaskDesc &task_info, InnerDumpInfo &dump_info) const;

  Status BuildTaskList(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const;
  void BuildTaskInputs(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const;
  void BuildTaskOutputs(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const;
  void BuildTaskWorkspaces(const InnerDumpInfo &dump_info, toolkit::aicpu::dump::Task &task) const;
  void BuildOpDebugTask(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const;

  // 对齐 v1 DataDumper::BuildTaskInfoForDumpOutput：watcher 节点的 dump task 额外复制一份给每个被记录的
  // layer 算子，task_id/stream_id 换成 layer 算子的，op 名改为 "<layerOpName>_To_<watcherNode>"
  void BuildWatcherLayerTasks(const InnerDumpInfo &dump_info, const toolkit::aicpu::dump::Task &task,
                              toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const;

  toolkit::aicpu::dump::OpMappingInfo op_mapping_info_;
  toolkit::aicpu::dump::OpMappingInfo op_mapping_base_info_;
  bool op_mapping_base_info_initialized_ = false;

  // TLV 构造modelinfo、taskinfo相关接口
  Status ExecuteLoadDumpInfo(const std::vector<uint8_t> &payload);
  Status BuildTaskList(DumpTransportInfo &dump_transport_info) const;
  void BuildTaskInputs(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const;
  void BuildTaskOutputs(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const;
  void BuildTaskWorkspaces(const InnerDumpInfo &dump_info, DumpTransTaskInfo &task) const;
  void BuildOpDebugTask(DumpTransportInfo &dump_transport_info) const;
  // TLV 版本的 watcher _To_ task 生成，语义与 protobuf 版本一致
  void BuildWatcherLayerTasks(const InnerDumpInfo &dump_info, const DumpTransTaskInfo &task,
                              DumpTransportInfo &dump_transport_info) const;
  DumpTransportInfo dump_transport_info_;
  DumpTransportInfo dump_transport_base_info_;
  bool dump_transport_base_info_initialized_ = false;

  std::vector<InnerDumpInfo> task_list_;
  // watcher 模式下记录的非 watcher 算子（仅用于生成 _To_ task，不落自身数据）
  struct InnerLayerOpInfo {
    uint32_t task_id;
    uint32_t stream_id;
    std::string op_name;
  };
  std::vector<InnerLayerOpInfo> layer_op_list_;
  // 当前模型信息，用于黑名单（optype_blacklist / opname_blacklist）过滤
  std::string model_name_;
  std::string root_graph_name_;
  void *dev_mem_load_ = nullptr;
  void *step_id_dev_addr_ = nullptr;
  // 异步拷贝 step_id 时的主机侧源地址（ACL_MEMCPY_HOST_TO_BUF_TO_DEVICE 需保证源内存在拷贝期间有效）
  uint64_t step_id_host_val_ = 0U;
  bool load_flag_ = false;
  uint32_t data_dump_version_ = 0;  // 0代表protobuf格式 1代表TLV格式

  // Overflow dump 相关成员
  bool is_op_debug_ = false;
  uint32_t op_debug_task_id_ = 0U;
  uint32_t op_debug_stream_id_ = 0U;
  void *op_debug_addr_ = nullptr;
};

}  // namespace dump
}  // namespace ge

#endif  // GE_COMMON_DUMP_INTERNAL_DATA_DUMP_IMPL_H_
