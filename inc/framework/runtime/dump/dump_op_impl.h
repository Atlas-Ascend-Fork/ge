/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_IMPL_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include "framework/common/ge_types.h"
#include "framework/runtime/dump/model_dump_c_api.h"
#include "proto/op_mapping.pb.h"
#include "framework/runtime/dump/dump_transport_info.h"

namespace ge {
namespace dump {

// ========================================================================
// DumpOp 类，自定义算子的 dump 使用
// ========================================================================
class DumpOp {
 public:
  DumpOp();
  ~DumpOp();
  Status ExecutorDumpOp(const std::string &op_name, aclrtStream stream);
  Status BuildTaskInputs(const GertModelTaskDesc &task_desc);
  Status BuildTaskOutputs(const GertModelTaskDesc &task_desc);
  static Status SetTaskBasicInfo(const GertModelTaskDesc &task_desc, toolkit::aicpu::dump::Task *task);
  toolkit::aicpu::dump::OpMappingInfo &GetOpMappingInfo() {
    return op_mapping_info_;
  }

  static Status SetTaskBasicInfo(const GertModelTaskDesc &task_desc, DumpTransTaskInfo *task);
  DumpTransportInfo &GetDumpTransportInfo() {
    return dump_transport_info_;
  }
  bool IsProtoVer() const {
    return dump_data_version_ == 0U;
  }

 private:
  Status ProtoMallocAndMemcpy(const size_t proto_size, const std::string &proto_msg);
  Status SubmitSerializedPayload(const std::string &op_name);
  Status LaunchDumpKernel(const std::string &op_name, aclrtStream stream);
  Status FillProtoInputTensors(toolkit::aicpu::dump::Task *task, const GertModelTaskDesc &task_desc);
  Status FillProtoOutputTensors(toolkit::aicpu::dump::Task *task, const GertModelTaskDesc &task_desc);
  Status FillTlvInputTensors(DumpTransTaskInfo *task, const GertModelTaskDesc &task_desc);
  Status FillTlvOutputTensors(DumpTransTaskInfo *task, const GertModelTaskDesc &task_desc);

  uint32_t dump_data_version_ = 0;  // 0代表protobuf格式 1代表TLV格式

  void *proto_dev_mem_ = nullptr;
  size_t proto_dev_mem_capacity_ = 0U;
  void *proto_size_dev_mem_ = nullptr;
  toolkit::aicpu::dump::OpMappingInfo op_mapping_info_;

  Status PayloadMallocAndMemcpy(const std::vector<uint8_t> &payload);
  void *payload_dev_mem_ = nullptr;
  size_t payload_dev_mem_capacity_ = 0U;
  void *payload_size_dev_mem_ = nullptr;
  DumpTransportInfo dump_transport_info_;
};

using DumpOpPtr = std::unique_ptr<DumpOp>;

class DumpOpImpl {
 public:
  DumpOpPtr &GetInputDumpOp(const std::string &op_name) {
    return GetDumpOp(op_name, true);
  }

  DumpOpPtr &GetOutputDumpOp(const std::string &op_name) {
    return GetDumpOp(op_name, false);
  }

 private:
  DumpOpPtr &GetDumpOp(const std::string &op_name, bool is_input) {
    auto &item = custom_dump_ops_[op_name];
    auto &dump_op = is_input ? item.first : item.second;
    if (dump_op == nullptr) {
      dump_op.reset(new DumpOp());
    }
    if (dump_op->IsProtoVer()) {
      dump_op->GetOpMappingInfo().clear_task();
    } else {
      dump_op->GetDumpTransportInfo().Clear();
    }
    return dump_op;
  }
  std::map<std::string, std::pair<DumpOpPtr, DumpOpPtr>> custom_dump_ops_;
};

}  // namespace dump
}  // namespace ge

#endif  // GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_IMPL_H_
