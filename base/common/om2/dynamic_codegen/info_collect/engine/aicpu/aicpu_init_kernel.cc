/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"

#include <sstream>

#include "common/checker.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "framework/common/taskdown_common.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace om2 {
namespace {
std::string EscapeForStringLiteral(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size() + 8U);
  for (const char c : value) {
    switch (c) {
      case '\\':
      case '"':
        escaped.push_back('\\');
        escaped.push_back(c);
        break;
      case '\n':
        escaped.append("\\n");
        break;
      case '\r':
        escaped.append("\\r");
        break;
      case '\t':
        escaped.append("\\t");
        break;
      default:
        escaped.push_back(c);
        break;
    }
  }
  return escaped;
}

class BuildHostCCArgsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"io_num", Om2ValueType::kSizeT},
                                               {"node_name", Om2ValueType::kCStringPtr},
                                               {"need_device_ext", Om2ValueType::kBool},
                                               {"arg_data", Om2ValueType::kBytePtr},
                                               {"arg_size", Om2ValueType::kSizeT},
                                               {"ext_size", Om2ValueType::kSizeT}},
                                              info));
    return SetOutputPortNames(context, {"args_handler"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildHostCCArgs", BuildHostCCArgsKernelCodeBuilder);

class CreateStepIdKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortNames(context, {}, info));
    return SetOutputPortNames(context, {"step_id", "iteration"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("CreateStepId", CreateStepIdKernelCodeBuilder);

class EnsureCreateTfSessionKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"session_id", Om2ValueType::kUInt64T}}, info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("EnsureCreateTfSession", EnsureCreateTfSessionKernelCodeBuilder);

class IncreaseStepIdKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"step_id", Om2ValueType::kVoidPtr},
                                               {"iteration", Om2ValueType::kInt64T},
                                               {"stream", Om2ValueType::kRtStreamTPtr}},
                                              info));
    return SetOutputPortNames(context, {}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("IncreaseStepId", IncreaseStepIdKernelCodeBuilder);

class BuildCCArgsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"io_num", Om2ValueType::kSizeT},
                                               {"node_name", Om2ValueType::kCStringPtr},
                                               {"need_device_ext", Om2ValueType::kBool},
                                               {"arg_data", Om2ValueType::kBytePtr},
                                               {"arg_size", Om2ValueType::kSizeT},
                                               {"ext_size", Om2ValueType::kSizeT},
                                               {"is_block_op", Om2ValueType::kBool},
                                               {"rt_event", Om2ValueType::kVoidPtr},
                                               {"async_timeout", Om2ValueType::kUInt32T},
                                               {"kernel_name", Om2ValueType::kCStringPtr},
                                               {"kernel_name_size", Om2ValueType::kSizeT},
                                               {"so_name", Om2ValueType::kCStringPtr},
                                               {"so_name_size", Om2ValueType::kSizeT}},
                                              info));
    return SetOutputPortNames(context, {"args_handler"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildCCArgs", BuildCCArgsKernelCodeBuilder);

class BuildTfArgsKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"io_num", Om2ValueType::kSizeT},
                                               {"node_name", Om2ValueType::kCStringPtr},
                                               {"need_device_ext", Om2ValueType::kBool},
                                               {"arg_data", Om2ValueType::kBytePtr},
                                               {"arg_size", Om2ValueType::kSizeT},
                                               {"ext_size", Om2ValueType::kSizeT},
                                               {"is_block_op", Om2ValueType::kBool},
                                               {"rt_event", Om2ValueType::kVoidPtr},
                                               {"async_timeout", Om2ValueType::kUInt32T},
                                               {"task_data", Om2ValueType::kBytePtr},
                                               {"task_size", Om2ValueType::kSizeT},
                                               {"session_id", Om2ValueType::kUInt64T},
                                               {"step_id", Om2ValueType::kVoidPtr}},
                                              info));
    return SetOutputPortNames(context, {"args_handler"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildTfArgs", BuildTfArgsKernelCodeBuilder);

class BuildExtInfoHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kOutputsCreator, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"ext_data", Om2ValueType::kBytePtr},
                                               {"ext_len", Om2ValueType::kSizeT},
                                               {"node_name", Om2ValueType::kCStringPtr},
                                               {"input_num", Om2ValueType::kSizeT},
                                               {"output_num", Om2ValueType::kSizeT},
                                               {"unknown_shape_type", Om2ValueType::kInt32T},
                                               {"host_addr", Om2ValueType::kVoidPtr},
                                               {"device_addr", Om2ValueType::kVoidPtr},
                                               {"session_id", Om2ValueType::kUInt64T},
                                               {"is_block_op", Om2ValueType::kBool},
                                               {"event_id", Om2ValueType::kUInt32T}},
                                              info));
    return SetOutputPortNames(context, {"ext_info_handler"}, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("BuildExtInfoHandle", BuildExtInfoHandleKernelCodeBuilder);

class BuildCCArgsBinHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context,
                                              {{"op_desc", Om2ValueType::kGeOpDescPtr},
                                               {"node_type", Om2ValueType::kCStringPtr},
                                               {"kernel_type", Om2ValueType::kUInt32T},
                                               {"so_name", Om2ValueType::kCStringPtr}},
                                              info));
    GE_ASSERT_GRAPH_SUCCESS(SetOutputPortNames(context, {"bin_handle", "bin_resource"}, info));
    GE_ASSERT_TRUE(context.input_chains.size() == 4U);
    auto *const op_desc = context.input_chains[0U].GetValue<ge::OpDesc *>();
    const auto *const node_type = context.input_chains[1U].GetValue<ge::char_t *>();
    GE_ASSERT_NOTNULL(op_desc);
    GE_ASSERT_NOTNULL(node_type);
    output_chain_index_ = context.output_chain_indices[0U];
    lease_chain_index_ = context.output_chain_indices[1U];
    custom_flag_ =
        context.input_chains[2U].GetValue<uint32_t>() == static_cast<uint32_t>(ge::ccKernelType::CUST_AI_CPU);
    if (custom_flag_) {
      const auto *so_name = context.input_chains[3U].GetValue<ge::char_t *>();
      GE_ASSERT_TRUE((so_name != nullptr) && (so_name[0] != '\0'));
      const auto binary = op_desc->TryGetExtAttr(ge::OP_EXTATTR_CUSTAICPU_KERNEL, ge::OpKernelBinPtr());
      GE_ASSERT_NOTNULL(binary);
      GE_ASSERT_TRUE(binary->GetBinDataSize() > 0U);
      binary_key_ = GetKernelBinaryKey(binary->GetBinData(), binary->GetBinDataSize());
    }
    const auto *const json_path = ge::AttrUtils::GetStr(op_desc, "ops_json_path");
    if (json_path != nullptr) {
      json_path_ = *json_path;
    }
    return ge::GRAPH_SUCCESS;
  }

  KernelImplType GetImplType() const override {
    return KernelImplType::kCodegenImpl;
  }

  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {0U, 1U, 2U, 3U};
  }

  ge::graphStatus GenerateKernelCode(const std::string &function_symbol, std::string &code) const override {
    GE_ASSERT_TRUE(output_chain_index_ != kInvalidProgramIndex);
    std::ostringstream ss;
    if (custom_flag_) {
      ss << "  aclError " << function_symbol << "() {\n"
         << "    return LoadCustAicpuBin(" << output_chain_index_ << "U, " << lease_chain_index_ << "U, \""
         << binary_key_ << "\");\n  }\n";
      code = ss.str();
      return ge::GRAPH_SUCCESS;
    }
    ss << "  aclError " << function_symbol << "() {\n"
       << "    auto &output = chains_[" << output_chain_index_ << "U];\n"
       << "    output.Set(nullptr, nullptr);\n"
       << "    chains_[" << lease_chain_index_ << "U].Set(nullptr, nullptr);\n"
       << "    const char *const json_path = \"" << EscapeForStringLiteral(json_path_) << "\";\n"
       << "    if (json_path[0] == '\\0') {\n"
       << "      return kOk;\n"
       << "    }\n"
       << "    const std::string path(json_path);\n"
       << "    auto iter = aicpu_bin_handles_.find(path);\n"
       << "    if (iter == aicpu_bin_handles_.end()) {\n"
       << "      aclrtBinaryLoadOption option{};\n"
       << "      option.type = ACL_RT_BINARY_LOAD_OPT_CPU_KERNEL_MODE;\n"
       << "      option.value.cpuKernelMode = 0;\n"
       << "      aclrtBinaryLoadOptions options{&option, 1U};\n"
       << "      aclrtBinHandle handle = nullptr;\n"
       << "      if (aclrtBinaryLoadFromFile(json_path, &options, &handle) != ACL_SUCCESS) {\n"
       << "        return kFailed;\n"
       << "      }\n"
       << "      if (handle == nullptr) {\n"
       << "        return kFailed;\n"
       << "      }\n"
       << "      iter = aicpu_bin_handles_.emplace(path, handle).first;\n"
       << "    }\n"
       << "    output.Set(iter->second, nullptr);\n"
       << "    return kOk;\n"
       << "  }\n";
    code = ss.str();
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint32_t output_chain_index_{kInvalidProgramIndex};
  uint32_t lease_chain_index_{kInvalidProgramIndex};
  bool custom_flag_{false};
  std::string json_path_;
  std::string binary_key_;
};
REGISTER_KERNEL_CODE_BUILDER("BuildCCArgsBinHandle", BuildCCArgsBinHandleKernelCodeBuilder);

class ReleaseCustAicpuBinHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(context, {{"bin_resource", Om2ValueType::kVoidPtr}}, info));
    GE_ASSERT_GRAPH_SUCCESS(SetOutputPortNames(context, {}, info));
    lease_chain_index_ = context.input_chain_indices[0U];
    return ge::GRAPH_SUCCESS;
  }

  KernelImplType GetImplType() const override {
    return KernelImplType::kCodegenImpl;
  }

  ge::graphStatus GenerateKernelCode(const std::string &function_symbol, std::string &code) const override {
    GE_ASSERT_TRUE(lease_chain_index_ != kInvalidProgramIndex);
    code = "  aclError " + function_symbol + "() {\n    return ReleaseCustAicpuBin(chains_[" +
           std::to_string(lease_chain_index_) + "U]);\n  }\n";
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint32_t lease_chain_index_{kInvalidProgramIndex};
};
REGISTER_KERNEL_CODE_BUILDER("ReleaseCustAicpuBinHandle", ReleaseCustAicpuBinHandleKernelCodeBuilder);

class BuildTfArgsBinHandleKernelCodeBuilder final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    GE_ASSERT_GRAPH_SUCCESS(InitKernelProgramInfo(context, OutputCreatePolicy::kGraphBound, false, info));
    GE_ASSERT_GRAPH_SUCCESS(SetInputPortInfos(
        context, {{"op_desc", Om2ValueType::kGeOpDescPtr}, {"node_type", Om2ValueType::kCStringPtr}}, info));
    GE_ASSERT_GRAPH_SUCCESS(SetOutputPortNames(context, {"bin_handle"}, info));
    GE_ASSERT_TRUE((context.input_chains.size() == 2U) && (context.output_chain_indices.size() == 1U));
    auto *const op_desc = context.input_chains[0U].GetValue<ge::OpDesc *>();
    const auto *const node_type = context.input_chains[1U].GetValue<ge::char_t *>();
    GE_ASSERT_NOTNULL(op_desc);
    GE_ASSERT_NOTNULL(node_type);
    output_chain_index_ = context.output_chain_indices[0U];
    const auto *const json_path = ge::AttrUtils::GetStr(op_desc, "ops_json_path");
    if (json_path != nullptr) {
      json_path_ = *json_path;
    }
    return ge::GRAPH_SUCCESS;
  }

  KernelImplType GetImplType() const override {
    return KernelImplType::kCodegenImpl;
  }

  std::vector<uint32_t> GetRestoreConstInputIndices() const override {
    return {0U, 1U};
  }

  ge::graphStatus GenerateKernelCode(const std::string &function_symbol, std::string &code) const override {
    GE_ASSERT_TRUE(output_chain_index_ != kInvalidProgramIndex);
    std::ostringstream ss;
    ss << "  aclError " << function_symbol << "() {\n"
       << "    auto &output = chains_[" << output_chain_index_ << "U];\n"
       << "    output.Set(nullptr, nullptr);\n"
       << "    const char *const json_path = \"" << EscapeForStringLiteral(json_path_) << "\";\n"
       << "    if (json_path[0] == '\\0') {\n"
       << "      return kOk;\n"
       << "    }\n"
       << "    const std::string path(json_path);\n"
       << "    auto iter = aicpu_bin_handles_.find(path);\n"
       << "    if (iter == aicpu_bin_handles_.end()) {\n"
       << "      aclrtBinaryLoadOption option{};\n"
       << "      option.type = ACL_RT_BINARY_LOAD_OPT_CPU_KERNEL_MODE;\n"
       << "      option.value.cpuKernelMode = 0;\n"
       << "      aclrtBinaryLoadOptions options{&option, 1U};\n"
       << "      aclrtBinHandle handle = nullptr;\n"
       << "      if (aclrtBinaryLoadFromFile(json_path, &options, &handle) != ACL_SUCCESS) {\n"
       << "        return kFailed;\n"
       << "      }\n"
       << "      if (handle == nullptr) {\n"
       << "        return kFailed;\n"
       << "      }\n"
       << "      iter = aicpu_bin_handles_.emplace(path, handle).first;\n"
       << "    }\n"
       << "    output.Set(iter->second, nullptr);\n"
       << "    return kOk;\n"
       << "  }\n";
    code = ss.str();
    return ge::GRAPH_SUCCESS;
  }

 private:
  uint32_t output_chain_index_{kInvalidProgramIndex};
  std::string json_path_;
};
REGISTER_KERNEL_CODE_BUILDER("BuildTfArgsBinHandle", BuildTfArgsBinHandleKernelCodeBuilder);
}  // namespace
}  // namespace om2
}  // namespace gert
