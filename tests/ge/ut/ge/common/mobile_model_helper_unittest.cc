/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <gtest/gtest.h>

#include "framework/common/helper/mobile_model_helper.h"
#include "framework/omg/ge_init.h"
#include "framework/common/framework_types_internal.h"

#include "graph/custom_op_factory.h"

#include "common/env_path.h"
#include "common/op_so_store/op_so_store_utils.h"
#include "common/plugin/plugin_manager.h"
#include "mmpa/mmpa_api.h"
#include "graph_metadef/common/ge_common/util.h"
#include "common/model/ge_model.h"
#include "common/opskernel/ops_kernel_info_types.h"

#include "common/file_constant_utils/file_constant_utils.h"
#include "common/helper/mobile/compiled_model.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/utils/file_utils.h"
#include "proto/task_mobile.pb.h"

#include "graph/buffer/buffer_impl.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "securec.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_local_context.h"
#include "proto/task.pb.h"

namespace {

std::vector<char> CreateStubBin() {
  auto ascend_install_path = ge::EnvPath().GetAscendInstallPath();
  GELOGI("ascend_install_path: %s.", ascend_install_path.c_str());
  std::string op_bin_path = ascend_install_path + "/fwkacllib/lib64/switch_by_index.o";
  std::vector<char> buf;
  std::ifstream file(op_bin_path.c_str(), std::ios::binary | std::ios::in);
  if (!file.is_open()) {
    GELOGW("file: %s does not exist or is unaccessible.", op_bin_path.c_str());
    return buf;
  }
  GE_MAKE_GUARD(file_guard, [&file]() { (void)file.close(); });
  const std::streampos begin = file.tellg();
  (void)file.seekg(0, std::ios::end);
  const std::streampos end = file.tellg();
  auto buf_len = static_cast<uint32_t>(end - begin);
  buf.resize(buf_len + 1);
  (void)file.seekg(0, std::ios::beg);
  (void)file.read(const_cast<char *>(buf.data()), buf_len);
  return buf;
}

ge::NodePtr CreateNode(ge::ComputeGraph &graph, const std::string &name, const std::string &type, int in_num,
                       int out_num, ge::DataType data_type = ge::DT_INT64) {
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  ge::GeTensorDesc tensor(ge::GeShape({8}), ge::FORMAT_ND, data_type);
  ge::TensorUtils::SetSize(tensor, 64);
  std::vector<int64_t> input_offset;
  for (int i = 0; i < in_num; i++) {
    op_desc->AddInputDesc(tensor);
    input_offset.emplace_back(index * 64 + i * 64);
  }
  op_desc->SetInputOffset(input_offset);

  std::vector<int64_t> output_offset;
  for (int i = 0; i < out_num; i++) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(index * 64 + in_num * 64 + i * 64);
  }
  op_desc->SetOutputOffset(output_offset);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});

  const static std::set<std::string> kGeLocalTypes{"Data", "Constant", "NetOutput"};
  op_desc->SetOpKernelLibName((kGeLocalTypes.count(type) > 0U) ? "DNN_VM_GE_LOCAL_OP_STORE" : "DNN_VM_RTS_OP_STORE");
  return graph.AddNode(op_desc);
}

ge::GeRootModelPtr GenGeRootModel(bool is_invaild_data_type = false, const std::string &custom_op_type = "Add") {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");

  ge::NodePtr input0_node = CreateNode(*graph, "input0", "Data", 1, 1);
  ge::OpDescPtr input0_op_desc = input0_node->GetOpDesc();
  EXPECT_NE(input0_op_desc, nullptr);

  ge::NodePtr input1_node = CreateNode(*graph, "input1", "Data", 1, 1);
  ge::OpDescPtr input1_op_desc = input1_node->GetOpDesc();
  EXPECT_NE(input1_op_desc, nullptr);

  // not support data type
  if (is_invaild_data_type) {
    ge::NodePtr input2_node = CreateNode(*graph, "input2", "Data", 1, 1, ge::DT_FLOAT4_E1M2);
    ge::OpDescPtr input2_op_desc = input2_node->GetOpDesc();
    EXPECT_NE(input2_op_desc, nullptr);
  }

  ge::NodePtr add_node = CreateNode(*graph, "add", custom_op_type, 2, 1);
  ge::OpDescPtr add_op_desc = add_node->GetOpDesc();
  EXPECT_NE(add_op_desc, nullptr);

  std::string test_float_attr_str = "test_float_attr";
  float test_float_value = 1.0;
  ge::AttrUtils::SetFloat(add_op_desc, test_float_attr_str, test_float_value);

  std::string test_bytes_attr_str = "test_bytes_attr";
  ge::Buffer test_bytes_buffer(10, 1);
  ge::AttrUtils::SetBytes(add_op_desc, test_bytes_attr_str, test_bytes_buffer);

  std::string test_list_attr_str = "test_list_attr";
  std::vector<int> test_list_attr_value = {1, 2, 3, 4};
  ge::AttrUtils::SetListInt(add_op_desc, test_list_attr_str, test_list_attr_value);

  std::string test_list_list_int_attr_str = "test_list_list_int_attr";
  std::vector<std::vector<int64_t>> test_list_list_int_value = {{1, 2, 3}, {1, 2, 3}};
  ge::AttrUtils::SetListListInt(add_op_desc, test_list_list_int_attr_str, test_list_list_int_value);

  std::string test_list_list_float_attr_str = "test_list_list_float_attr";
  std::vector<std::vector<float>> test_list_list_float_value = {{1.1, 2.34, 3.4}, {1.1, 2.8, 3.5}};
  ge::AttrUtils::SetListListFloat(add_op_desc, test_list_list_float_attr_str, test_list_list_float_value);

  ge::GeAttrValue::NAMED_ATTRS test_named_attrs;
  std::string named_attr_str0 = "named_attr_test_str";
  std::string named_attr_str_value = "testtest";
  ge::AttrUtils::SetStr(test_named_attrs, named_attr_str0, named_attr_str_value);
  std::string named_attr_str1 = "named_attr_test_int";
  int named_attr_int_value = 1232;
  ge::AttrUtils::SetInt(test_named_attrs, named_attr_str1, named_attr_int_value);
  std::string named_attr = "named_attr";
  (void)ge::AttrUtils::SetNamedAttrs(add_op_desc, named_attr, test_named_attrs);

  std::string dynamic_inputs_indexes_str = "_dynamic_inputs_indexes";
  std::vector<std::vector<int64_t>> dynamic_inputs_indexes = {{0, 1}};
  ge::AttrUtils::SetListListInt(add_op_desc, dynamic_inputs_indexes_str, dynamic_inputs_indexes);

  std::string dynamic_outputs_indexes_str = "_dynamic_outputs_indexes";
  std::vector<std::vector<int64_t>> dynamic_outputs_indexes = {{0}};
  ge::AttrUtils::SetListListInt(add_op_desc, dynamic_outputs_indexes_str, dynamic_outputs_indexes);

  const std::string kernel_name = "test";
  std::string kernel_name_str = "_kernelname";
  ge::AttrUtils::SetStr(add_op_desc, kernel_name_str, kernel_name);

  ge::NodePtr netoutput_node = CreateNode(*graph, "output0", "NetOutput", 1, 1);
  ge::OpDescPtr netoutput_op_desc = netoutput_node->GetOpDesc();
  EXPECT_NE(netoutput_op_desc, nullptr);

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");

  ge::TBEKernelStore tbe_kernel_store;
  const auto kernel = ge::MakeShared<ge::OpKernelBin>(kernel_name, CreateStubBin());
  tbe_kernel_store.AddTBEKernel(kernel);
  ge_model->SetTBEKernelStore(tbe_kernel_store);

  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task);

  domi::TaskDef *task_def0 = model_task->add_task();
  task_def0->set_type(static_cast<uint32_t>(ge::ModelTaskType::MODEL_TASK_KERNEL));
  task_def0->set_stream_id(add_op_desc->GetStreamId());
  domi::KernelDef *kernel_def0 = task_def0->mutable_kernel();
  kernel_def0->set_stub_func("stub_func_add");
  kernel_def0->set_args_size(64);
  std::string args(64, '1');
  kernel_def0->set_args(args.data(), 64);
  kernel_def0->set_kernel_name("test");

  domi::KernelContext *context0 = kernel_def0->mutable_context();
  context0->set_kernel_type(0);
  context0->set_op_index(add_op_desc->GetId());
  uint16_t args_offset[9] = {0};
  context0->set_args_offset(args_offset, 9 * sizeof(uint16_t));
  std::string args_format = "{i_desc0}{o_desc0}";
  context0->set_args_format(args_format);

  ge::GeRootModelPtr ge_root_model = std::make_shared<ge::GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(graph), ge::SUCCESS);
  ge_root_model->SetModelName(ge_model->GetName());
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  return ge_root_model;
}

constexpr size_t kMobileModelFileHeaderSize = 256U;
constexpr size_t kMobilePartitionTableHeaderSize = sizeof(uint32_t);
constexpr size_t kMobilePartitionMemInfoSize = sizeof(uint32_t) * 3U;
constexpr uint32_t kMobileModelDefPartition = 0U;
constexpr uint32_t kMobileWeightsDataPartition = 1U;
constexpr uint32_t kMobileTaskInfoPartition = 2U;
constexpr uint32_t kMobileTaskGraphPartition = 3U;
constexpr uint32_t kMobileSignatureInfoPartition = 4U;
constexpr uint32_t kMobileModelConfigPartition = 5U;
constexpr uint32_t kMobileAippCustomInfoPartition = 6U;
constexpr uint32_t kMobileEncryptInfoPartition = 7U;
constexpr uint32_t kMobileWeightInfoPartition = 8U;
constexpr const char *kAscendCustomOppPathEnv = "ASCEND_CUSTOM_OPP_PATH";
constexpr const char *kMobileCustomOpSoPayloadMarker = "MOBILE_CUSTOM_OP_SO_PAYLOAD_MARKER";
constexpr uint32_t kStandardOmCustomOpsPartition = static_cast<uint32_t>(ge::CUSTOM_OPS);

struct MobilePartitionMemInfoForTest {
  uint32_t type;
  uint32_t mem_offset;
  uint32_t mem_size;
};

struct MobileOmcFileGuardForTest {
  MobileOmcFileGuardForTest(const std::string &output_file_name, const std::string &omc_file_name)
      : output_file(output_file_name), omc_file(omc_file_name) {}

  ~MobileOmcFileGuardForTest() {
    (void)std::remove(output_file.c_str());
    (void)std::remove(omc_file.c_str());
  }

  std::string output_file;
  std::string omc_file;
};

std::string GetCompileArchForTest() {
#if defined(__aarch64__) || defined(__arm64__)
  return "aarch64";
#elif defined(__x86_64__) || defined(__amd64__)
  return "x86_64";
#else
  return "";
#endif
}

std::string GetCrossCompileTargetArchForTest() {
  std::string current_env_os;
  std::string current_env_cpu;
  ge::PluginManager::GetCurEnvPackageOsAndCpuType(current_env_os, current_env_cpu);
  if (current_env_cpu.empty()) {
    current_env_cpu = GetCompileArchForTest();
  }
  std::transform(current_env_cpu.begin(), current_env_cpu.end(), current_env_cpu.begin(),
                 [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
  return ((current_env_cpu == "aarch64") || (current_env_cpu == "arm64")) ? "x86_64" : "aarch64";
}

class MobileHostEnvGuardForTest {
 public:
  MobileHostEnvGuardForTest(const std::string &host_env_os, const std::string &host_env_cpu) {
    (void)ge::GetThreadLocalContext().GetOption(ge::OPTION_HOST_ENV_OS, old_host_env_os_);
    (void)ge::GetThreadLocalContext().GetOption(ge::OPTION_HOST_ENV_CPU, old_host_env_cpu_);
    std::map<std::string, std::string> options;
    options[ge::OPTION_HOST_ENV_OS] = host_env_os;
    options[ge::OPTION_HOST_ENV_CPU] = host_env_cpu;
    (void)ge::GetThreadLocalContext().SetGlobalOption(options);
  }

  ~MobileHostEnvGuardForTest() {
    std::map<std::string, std::string> options;
    options[ge::OPTION_HOST_ENV_OS] = old_host_env_os_;
    options[ge::OPTION_HOST_ENV_CPU] = old_host_env_cpu_;
    (void)ge::GetThreadLocalContext().SetGlobalOption(options);
  }

 private:
  std::string old_host_env_os_;
  std::string old_host_env_cpu_;
};

class MobileEnvVarGuardForTest {
 public:
  explicit MobileEnvVarGuardForTest(std::string env_name) : env_name_(std::move(env_name)) {
    const char *env_value = std::getenv(env_name_.c_str());
    if (env_value != nullptr) {
      has_old_value_ = true;
      old_value_ = env_value;
    }
  }

  ~MobileEnvVarGuardForTest() {
    if (has_old_value_) {
      (void)mmSetEnv(env_name_.c_str(), old_value_.c_str(), 1);
      return;
    }
    (void)unsetenv(env_name_.c_str());
  }

 private:
  std::string env_name_;
  std::string old_value_;
  bool has_old_value_ = false;
};

class MobileTmpDirGuardForTest {
 public:
  explicit MobileTmpDirGuardForTest(std::string dir_path) : dir_path_(std::move(dir_path)) {
    Cleanup();
  }

  ~MobileTmpDirGuardForTest() {
    Cleanup();
  }

 private:
  void Cleanup() const {
    if (!dir_path_.empty()) {
      (void)mmRmdir(dir_path_.c_str());
    }
  }

  std::string dir_path_;
};

bool GetExpectedElfMachineForCpuForTest(const std::string &target_cpu, uint16_t &expected_machine) {
  std::string normalized_target_cpu = target_cpu;
  std::transform(normalized_target_cpu.begin(), normalized_target_cpu.end(), normalized_target_cpu.begin(),
                 [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
  if ((normalized_target_cpu == "x86_64") || (normalized_target_cpu == "amd64")) {
    expected_machine = 62U;
    return true;
  }
  if ((normalized_target_cpu == "aarch64") || (normalized_target_cpu == "arm64")) {
    expected_machine = 183U;
    return true;
  }
  return false;
}

bool WriteElfHeaderForTest(const std::string &so_path, const uint16_t e_machine) {
  std::vector<uint8_t> elf_header(64U, 0U);
  elf_header[0] = 0x7FU;
  elf_header[1] = static_cast<uint8_t>('E');
  elf_header[2] = static_cast<uint8_t>('L');
  elf_header[3] = static_cast<uint8_t>('F');
  elf_header[4] = 2U;
  elf_header[5] = 1U;
  elf_header[6] = 1U;
  elf_header[16] = 3U;
  elf_header[18] = static_cast<uint8_t>(e_machine & 0xFFU);
  elf_header[19] = static_cast<uint8_t>((e_machine >> 8U) & 0xFFU);

  std::ofstream file(so_path, std::ios::binary | std::ios::trunc);
  if (!file.good()) {
    return false;
  }
  file.write(reinterpret_cast<const char *>(elf_header.data()), static_cast<std::streamsize>(elf_header.size()));
  file.write(kMobileCustomOpSoPayloadMarker, strlen(kMobileCustomOpSoPayloadMarker));
  return file.good();
}

bool ReadUint32Value(const std::vector<char> &data, const size_t offset, uint32_t &value) {
  if ((offset > data.size()) || (data.size() - offset < sizeof(value))) {
    return false;
  }
  return memcpy_s(&value, sizeof(value), data.data() + offset, sizeof(value)) == EOK;
}

std::vector<MobilePartitionMemInfoForTest> ParseMobilePartitions(const std::string &omc_file) {
  std::vector<MobilePartitionMemInfoForTest> partitions;
  std::ifstream file(omc_file.c_str(), std::ios::binary | std::ios::ate);
  EXPECT_TRUE(file.is_open());
  if (!file.is_open()) {
    return partitions;
  }

  const std::streampos file_size_pos = file.tellg();
  EXPECT_GT(file_size_pos, std::streampos(0));
  if (file_size_pos <= std::streampos(0)) {
    return partitions;
  }
  const auto file_size = static_cast<size_t>(file_size_pos);
  std::vector<char> data(file_size);
  (void)file.seekg(0, std::ios::beg);
  (void)file.read(data.data(), static_cast<std::streamsize>(data.size()));
  EXPECT_EQ(file.gcount(), static_cast<std::streamsize>(data.size()));
  if (file.gcount() != static_cast<std::streamsize>(data.size())) {
    return partitions;
  }

  EXPECT_GE(data.size(), kMobileModelFileHeaderSize + kMobilePartitionTableHeaderSize);
  if (data.size() < kMobileModelFileHeaderSize + kMobilePartitionTableHeaderSize) {
    return partitions;
  }

  uint32_t partition_num = 0U;
  const bool has_partition_num = ReadUint32Value(data, kMobileModelFileHeaderSize, partition_num);
  EXPECT_TRUE(has_partition_num);
  if (!has_partition_num) {
    return partitions;
  }

  constexpr uint32_t kMaxMobilePartitionNumForTest = 16U;
  EXPECT_LE(partition_num, kMaxMobilePartitionNumForTest);
  if (partition_num > kMaxMobilePartitionNumForTest) {
    return partitions;
  }

  const size_t table_size =
      kMobilePartitionTableHeaderSize + (kMobilePartitionMemInfoSize * static_cast<size_t>(partition_num));
  EXPECT_LE(kMobileModelFileHeaderSize + table_size, data.size());
  if (kMobileModelFileHeaderSize + table_size > data.size()) {
    return partitions;
  }

  size_t partition_offset = kMobileModelFileHeaderSize + kMobilePartitionTableHeaderSize;
  for (uint32_t i = 0U; i < partition_num; ++i) {
    MobilePartitionMemInfoForTest partition{};
    EXPECT_TRUE(ReadUint32Value(data, partition_offset, partition.type));
    EXPECT_TRUE(ReadUint32Value(data, partition_offset + sizeof(uint32_t), partition.mem_offset));
    EXPECT_TRUE(ReadUint32Value(data, partition_offset + (sizeof(uint32_t) * 2U), partition.mem_size));
    partitions.emplace_back(partition);
    partition_offset += kMobilePartitionMemInfoSize;
  }
  return partitions;
}

bool HasPartitionType(const std::vector<MobilePartitionMemInfoForTest> &partitions, const uint32_t type) {
  for (const auto &partition : partitions) {
    if (partition.type == type) {
      return true;
    }
  }
  return false;
}

bool FileContainsStringForTest(const std::string &file_path, const std::string &target) {
  std::ifstream file(file_path.c_str(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }
  const auto file_size_pos = file.tellg();
  if (file_size_pos <= std::streampos(0)) {
    return target.empty();
  }
  std::vector<char> data(static_cast<size_t>(file_size_pos));
  (void)file.seekg(0, std::ios::beg);
  (void)file.read(data.data(), static_cast<std::streamsize>(data.size()));
  if (file.gcount() != static_cast<std::streamsize>(data.size())) {
    return false;
  }
  return std::search(data.cbegin(), data.cend(), target.cbegin(), target.cend()) != data.cend();
}

bool IsExpectedMobilePartitionType(const uint32_t type) {
  return (type == kMobileModelDefPartition) || (type == kMobileWeightsDataPartition) ||
         (type == kMobileTaskInfoPartition) || (type == kMobileTaskGraphPartition) ||
         (type == kMobileSignatureInfoPartition) || (type == kMobileModelConfigPartition) ||
         (type == kMobileAippCustomInfoPartition) || (type == kMobileEncryptInfoPartition) ||
         (type == kMobileWeightInfoPartition);
}

class MobileNoSoPortableOpForTest : public ge::EagerExecuteOp, public ge::PortableOp {
 public:
  ge::graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    (void)ctx;
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus Serialize(std::vector<uint8_t> &buffer) override {
    buffer = {1U, 2U, 3U, 4U};
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus Deserialize(const std::vector<uint8_t> &buffer) override {
    (void)buffer;
    return ge::GRAPH_SUCCESS;
  }
};

void RegisterMobileNoSoPortableOpForTest(const std::string &op_type) {
  const auto ret = ge::CustomOpFactory::RegisterCustomOpCreator(
      op_type.c_str(),
      []() -> std::unique_ptr<ge::BaseCustomOp> { return std::make_unique<MobileNoSoPortableOpForTest>(); });
  EXPECT_TRUE((ret == ge::GRAPH_SUCCESS) || (ret == ge::GRAPH_FAILED));
}

constexpr uint32_t kMobileTaskInfoTlvHeaderSize = 8U;  // Tlv {type, len}
constexpr uint32_t kMobileTaskInfoHeadSize = 32U;      // Head {magic, ver, total_len, optional_num, resv[4]}
constexpr uint32_t kMobileSectionTypeModelDef = 1U;    // CompiledTarget SectionType::SECTION_TYPE_MODELDEF
constexpr const char *kMobileWeightShareOpsEnv = "GE_MOBILE_WEIGHT_SHARE_OPS";
constexpr const char *kMobileExternalWeightOption = "ge.externalWeight";

class MobileExternalWeightOptionGuardForTest {
 public:
  explicit MobileExternalWeightOptionGuardForTest(const std::string &mode) {
    (void)ge::GetThreadLocalContext().SetGraphOption({{kMobileExternalWeightOption, mode}});
  }

  ~MobileExternalWeightOptionGuardForTest() {
    (void)ge::GetThreadLocalContext().SetGraphOption({});
  }
};

bool WriteBinaryFileForTest(const std::string &path, const std::vector<uint8_t> &data) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }
  file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
  file.close();
  return file.good();
}

std::vector<uint8_t> CreateWeightBytesForTest(const size_t size, const uint8_t fill) {
  return std::vector<uint8_t>(size, fill);
}

bool ReadBinaryFileForTest(const std::string &path, std::vector<uint8_t> &data) {
  std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }
  const auto file_size = static_cast<size_t>(file.tellg());
  if (file_size == 0U) {
    return false;
  }
  data.resize(file_size);
  (void)file.seekg(0, std::ios::beg);
  (void)file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(file_size));
  return file.gcount() == static_cast<std::streamsize>(file_size);
}

std::vector<std::string> ListFilesInDirForTest(const std::string &dir_path) {
  std::vector<std::string> files;
  DIR *dir = opendir(dir_path.c_str());
  if (dir == nullptr) {
    return files;
  }
  while (true) {
    const dirent *entry = readdir(dir);
    if (entry == nullptr) {
      break;
    }
    const std::string name = entry->d_name;
    if ((name == ".") || (name == "..")) {
      continue;
    }
    files.emplace_back(name);
  }
  (void)closedir(dir);
  return files;
}

void RemoveFilesInDirForTest(const std::string &dir_path) {
  for (const auto &name : ListFilesInDirForTest(dir_path)) {
    (void)std::remove((dir_path + "/" + name).c_str());
  }
  (void)mmRmdir(dir_path.c_str());
}

ge::NodePtr CreateFileConstantNode(ge::ComputeGraph &graph, const std::string &name, const std::string &weight_path,
                                   int64_t weight_offset, int64_t weight_length, int64_t var_addr) {
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>(name, "FileConstant");
  op_desc->SetStreamId(0);
  static int32_t fc_id = 100000;
  op_desc->SetId(fc_id++);
  ge::GeTensorDesc tensor(ge::GeShape({weight_length}), ge::FORMAT_ND, ge::DT_UINT8);
  ge::TensorUtils::SetSize(tensor, weight_length);
  (void)op_desc->AddOutputDesc(tensor);
  op_desc->SetOutputOffset({var_addr});
  ge::FileConstantUtils::SetFileConstantPath(op_desc, weight_path, weight_offset, weight_length);
  return graph.AddNode(op_desc);
}

// 读取 omc 文件中指定分区的原始数据
// 分区数据绝对偏移 = header(256) + partition table + 分区 table 中记录的 mem_offset
bool ReadMobilePartitionData(const std::string &omc_file, const uint32_t partition_type, std::vector<char> &data) {
  const auto partitions = ParseMobilePartitions(omc_file);
  for (const auto &partition : partitions) {
    if (partition.type != partition_type) {
      continue;
    }
    std::ifstream file(omc_file.c_str(), std::ios::binary);
    EXPECT_TRUE(file.is_open());
    if (!file.is_open()) {
      return false;
    }
    const size_t table_size =
        kMobilePartitionTableHeaderSize + (kMobilePartitionMemInfoSize * static_cast<size_t>(partitions.size()));
    const size_t abs_offset = kMobileModelFileHeaderSize + table_size + partition.mem_offset;
    data.resize(partition.mem_size);
    (void)file.seekg(static_cast<std::streamoff>(abs_offset), std::ios::beg);
    (void)file.read(data.data(), static_cast<std::streamsize>(data.size()));
    EXPECT_EQ(file.gcount(), static_cast<std::streamsize>(data.size()));
    return file.gcount() == static_cast<std::streamsize>(data.size());
  }
  return false;
}

// 解析 omc 文件 TASK_INFO 分区: Head(32B) + Tlv(SECTION_TYPE_MODELDEF) + ModelTaskDef proto + ...
bool ParseMobileTaskDefFromOmc(const std::string &omc_file, ge::mobile::proto::ModelTaskDef &task_def) {
  std::vector<char> data;
  if (!ReadMobilePartitionData(omc_file, kMobileTaskInfoPartition, data)) {
    return false;
  }
  if (data.size() < (kMobileTaskInfoHeadSize + kMobileTaskInfoTlvHeaderSize)) {
    return false;
  }
  uint32_t tlv_type = 0U;
  if (!ReadUint32Value(data, kMobileTaskInfoHeadSize, tlv_type) || (tlv_type != kMobileSectionTypeModelDef)) {
    return false;
  }
  uint32_t tlv_len = 0U;
  if (!ReadUint32Value(data, kMobileTaskInfoHeadSize + sizeof(uint32_t), tlv_len)) {
    return false;
  }
  if ((tlv_len < kMobileTaskInfoTlvHeaderSize) || ((kMobileTaskInfoHeadSize + tlv_len) > data.size())) {
    return false;
  }
  const auto proto_size = static_cast<int32_t>(tlv_len - kMobileTaskInfoTlvHeaderSize);
  return task_def.ParseFromArray(data.data() + kMobileTaskInfoHeadSize + kMobileTaskInfoTlvHeaderSize, proto_size);
}

// 解析 omc 文件 MODEL_DEF 分区(裸 ModelDef proto, 无 TLV 包装)
bool ParseMobileModelDefFromOmc(const std::string &omc_file, ge::mobile::proto::ModelDef &model_def) {
  std::vector<char> data;
  if (!ReadMobilePartitionData(omc_file, kMobileModelDefPartition, data)) {
    return false;
  }
  return model_def.ParseFromArray(data.data(), static_cast<int32_t>(data.size()));
}

// 读取主图 GraphOp 节点的 int64 attr
bool FindGraphOpIntAttr(const ge::mobile::proto::ModelDef &model_def, const std::string &attr_name, int64_t &value) {
  for (const auto &op : model_def.graph(0).op()) {
    if (op.type() != "GraphOp") {
      continue;
    }
    const auto it = op.attr().find(attr_name);
    if ((it == op.attr().end()) || (!it->second.has_i())) {
      return false;
    }
    value = it->second.i();
    return true;
  }
  return false;
}

uint64_t ReadMobileTaskArg(const ge::mobile::proto::ModelTaskDef &task_def, const size_t task_index,
                           const size_t arg_index) {
  const auto &args = task_def.task(static_cast<int32_t>(task_index)).kernel().args();
  uint64_t value = 0UL;
  const auto offset = arg_index * sizeof(uint64_t);
  if ((offset + sizeof(uint64_t)) > args.size()) {
    return UINT64_MAX;
  }
  if (memcpy_s(&value, sizeof(value), args.data() + offset, sizeof(value)) != EOK) {
    return UINT64_MAX;
  }
  return value;
}

uint64_t ReadMobileTaskFlowtableU64(const ge::mobile::proto::ModelTaskDef &task_def, const size_t task_index,
                                    const size_t u64_index) {
  const auto &flowtable = task_def.task(static_cast<int32_t>(task_index)).kernel().flowtable();
  const auto offset = u64_index * sizeof(uint64_t);
  if ((offset + sizeof(uint64_t)) > flowtable.size()) {
    return UINT64_MAX;
  }
  uint64_t value = 0UL;
  if (memcpy_s(&value, sizeof(value), flowtable.data() + offset, sizeof(value)) != EOK) {
    return UINT64_MAX;
  }
  return value;
}

struct MobileFileConstantNodeSpecForTest {
  std::string name;
  int64_t weight_offset;
  int64_t weight_length;
  int64_t var_addr;
  bool connect_to_add = false;       // 连到 add 节点 input0
  bool connect_to_extra_op = false;  // 连到 extra_op 节点 input0
};

// 构造基础图: input0(Data), input1(Data), add(Add, 2in 1out), [extra_op(extra_op_type)], output0(NetOutput),
// FileConstant...(按 fc_specs, 可连边到 add/extra_op), 返回 add 节点
ge::NodePtr CreateGraphWithFileConstantsForTest(ge::ComputeGraphPtr &graph, const std::string &weight_path,
                                                const std::vector<MobileFileConstantNodeSpecForTest> &fc_specs,
                                                const std::string &extra_op_type) {
  (void)CreateNode(*graph, "input0", "Data", 1, 1);
  (void)CreateNode(*graph, "input1", "Data", 1, 1);
  ge::NodePtr add_node = CreateNode(*graph, "add", "Add", 2, 1);
  ge::NodePtr extra_op_node = nullptr;
  if (!extra_op_type.empty()) {
    extra_op_node = CreateNode(*graph, "extra_op", extra_op_type, 1, 1);
  }
  (void)CreateNode(*graph, "output0", "NetOutput", 1, 1);

  for (const auto &spec : fc_specs) {
    auto fc_node =
        CreateFileConstantNode(*graph, spec.name, weight_path, spec.weight_offset, spec.weight_length, spec.var_addr);
    if (spec.connect_to_add) {
      (void)ge::GraphUtils::AddEdge(fc_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
    }
    if (spec.connect_to_extra_op && (extra_op_node != nullptr)) {
      (void)ge::GraphUtils::AddEdge(fc_node->GetOutDataAnchor(0), extra_op_node->GetInDataAnchor(0));
    }
  }
  return add_node;
}

// 向 model_task 追加 kernel task: stub_func_add, args 64B(args[0]=task_arg0, args[1]=task_arg1), args_format 指定
void AppendKernelTaskForTest(const std::shared_ptr<domi::ModelTaskDef> &model_task, const ge::OpDescPtr &add_op_desc,
                             const std::string &args_format, uint64_t task_arg0, uint64_t task_arg1) {
  domi::TaskDef *task_def = model_task->add_task();
  task_def->set_type(static_cast<uint32_t>(ge::ModelTaskType::MODEL_TASK_KERNEL));
  task_def->set_stream_id(add_op_desc->GetStreamId());
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  kernel_def->set_stub_func("stub_func_add");
  kernel_def->set_args_size(64);
  std::vector<uint8_t> args(64U, '1');
  EXPECT_EQ(memcpy_s(args.data(), args.size(), &task_arg0, sizeof(task_arg0)), EOK);
  EXPECT_EQ(memcpy_s(args.data() + sizeof(task_arg0), args.size() - sizeof(task_arg0), &task_arg1, sizeof(task_arg1)),
            EOK);
  kernel_def->set_args(reinterpret_cast<const char *>(args.data()), 64);
  kernel_def->set_kernel_name("test");

  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(0);
  context->set_op_index(add_op_desc->GetId());
  uint16_t args_offset[9] = {0};
  context->set_args_offset(args_offset, 9 * sizeof(uint16_t));
  context->set_args_format(args_format);
}

ge::GeRootModelPtr WrapGeRootModelForTest(const ge::ComputeGraphPtr &graph, const ge::GeModelPtr &ge_model) {
  ge::GeRootModelPtr ge_root_model = std::make_shared<ge::GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(graph), ge::SUCCESS);
  ge_root_model->SetModelName(ge_model->GetName());
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  return ge_root_model;
}

// 构造带 FileConstant 的模型:
// 图: input0(Data), input1(Data), add(Add), [extra_op(extra_op_type)], output0(NetOutput), FileConstant...
// task: args 64B, args[0]=task_arg0, args[1]=task_arg1, args_format 无 desc(不触发 flowtable)
ge::GeRootModelPtr GenGeRootModelWithFileConstants(const std::string &weight_path,
                                                   const std::vector<MobileFileConstantNodeSpecForTest> &fc_specs,
                                                   uint64_t task_arg0 = 0UL, uint64_t task_arg1 = 0UL,
                                                   const std::string &extra_op_type = "") {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr add_node = CreateGraphWithFileConstantsForTest(graph, weight_path, fc_specs, extra_op_type);

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");

  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task);
  AppendKernelTaskForTest(model_task, add_node->GetOpDesc(), "{arg0}{arg1}", task_arg0, task_arg1);
  return WrapGeRootModelForTest(graph, ge_model);
}

// 构造带 FileConstant 且动态 shape 触发 flowtable 的模型:
// 图: input0(Data), input1(Data), add(Add, 动态输入0/动态输出0), output0(NetOutput), FileConstant...
// add input0/output0 偏移取 fc_specs[1]/fc_specs[0] 的变量区地址, kernel args_format 为 {i_desc0}{o_desc0}
// (触发 flowtable), args 64B: args[0]/args[1] 为输入/输出组的 flowtable 段偏移标记(task_arg0/task_arg1
// 所在槽位被标记覆盖), args[2..7] 为直接拷贝槽; memory_size 非零, 保证 weight_addr != 2G 基址
ge::GeRootModelPtr GenGeRootModelWithFileConstantsAndDynamicShape(
    const std::string &weight_path, const std::vector<MobileFileConstantNodeSpecForTest> &fc_specs,
    uint64_t task_arg0 = 0UL, uint64_t task_arg1 = 0UL) {
  EXPECT_GE(fc_specs.size(), 2U);
  if (fc_specs.size() < 2U) {
    return nullptr;
  }
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr add_node = CreateGraphWithFileConstantsForTest(graph, weight_path, fc_specs, "");

  // 动态输入0/动态输出0 的 ptr 槽地址落在变量区, 触发 flowtable ptr 槽重映射
  ge::OpDescPtr add_op_desc = add_node->GetOpDesc();
  EXPECT_NE(add_op_desc, nullptr);
  if (add_op_desc == nullptr) {
    return nullptr;
  }
  add_op_desc->SetInputOffset({fc_specs[1].var_addr, 0x200});
  add_op_desc->SetOutputOffset({fc_specs[0].var_addr});
  std::vector<std::vector<int64_t>> dynamic_inputs_indexes = {{0}};
  std::vector<std::vector<int64_t>> dynamic_outputs_indexes = {{0}};
  (void)ge::AttrUtils::SetListListInt(add_op_desc, "_dynamic_inputs_indexes", dynamic_inputs_indexes);
  (void)ge::AttrUtils::SetListListInt(add_op_desc, "_dynamic_outputs_indexes", dynamic_outputs_indexes);

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");

  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  model_task->set_memory_size(0x100000);
  ge_model->SetModelTaskDef(model_task);
  AppendKernelTaskForTest(model_task, add_op_desc, "{i_desc0}{o_desc0}", task_arg0, task_arg1);
  return WrapGeRootModelForTest(graph, ge_model);
}

// 构造无 FileConstant 且动态 shape 触发 flowtable 的模型:
// 图: input0(Data), input1(Data), add(Add, 动态输入0/动态输出0), output0(NetOutput)
// add input0/output0 偏移落在变量区(无 FileConstant 映射), kernel args_format 为 {i_desc0}{o_desc0}
ge::GeRootModelPtr GenGeRootModelWithDynamicShapeVarAddr() {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  (void)CreateNode(*graph, "input0", "Data", 1, 1);
  (void)CreateNode(*graph, "input1", "Data", 1, 1);
  ge::NodePtr add_node = CreateNode(*graph, "add", "Add", 2, 1);
  ge::OpDescPtr add_op_desc = add_node->GetOpDesc();
  EXPECT_NE(add_op_desc, nullptr);
  if (add_op_desc == nullptr) {
    return nullptr;
  }
  // 动态输入0/动态输出0 的 ptr 槽地址落在变量区, 触发 flowtable ptr 槽变量区地址分支
  add_op_desc->SetInputOffset({static_cast<int64_t>(ge::kVarMemoryLogicBase), 0x200});
  add_op_desc->SetOutputOffset({static_cast<int64_t>(ge::kVarMemoryLogicBase) + 0x400});
  std::vector<std::vector<int64_t>> dynamic_inputs_indexes = {{0}};
  std::vector<std::vector<int64_t>> dynamic_outputs_indexes = {{0}};
  (void)ge::AttrUtils::SetListListInt(add_op_desc, "_dynamic_inputs_indexes", dynamic_inputs_indexes);
  (void)ge::AttrUtils::SetListListInt(add_op_desc, "_dynamic_outputs_indexes", dynamic_outputs_indexes);
  (void)ge::AttrUtils::SetStr(add_op_desc, "_kernelname", "test");
  (void)CreateNode(*graph, "output0", "NetOutput", 1, 1);

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");
  ge::TBEKernelStore tbe_kernel_store;
  (void)tbe_kernel_store.AddTBEKernel(ge::MakeShared<ge::OpKernelBin>("test", CreateStubBin()));
  ge_model->SetTBEKernelStore(tbe_kernel_store);
  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task);
  AppendKernelTaskForTest(model_task, add_op_desc, "{i_desc0}{o_desc0}", 0UL, 0UL);
  return WrapGeRootModelForTest(graph, ge_model);
}

// 构造两个相似节点名(add/add_1)且各带 kernel bin 的模型, 验证 stub_func 严格后缀匹配
ge::GeRootModelPtr GenGeRootModelWithSimilarNodeNames() {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  (void)CreateNode(*graph, "input0", "Data", 1, 1);
  ge::NodePtr add_node = CreateNode(*graph, "add", "Add", 1, 1);
  ge::NodePtr add1_node = CreateNode(*graph, "add_1", "Add", 1, 1);
  (void)CreateNode(*graph, "output0", "NetOutput", 1, 1);

  (void)ge::AttrUtils::SetStr(add_node->GetOpDesc(), "_kernelname", "test_a");
  (void)ge::AttrUtils::SetStr(add1_node->GetOpDesc(), "_kernelname", "test_b");

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");

  ge::TBEKernelStore tbe_kernel_store;
  (void)tbe_kernel_store.AddTBEKernel(ge::MakeShared<ge::OpKernelBin>("test_a", CreateStubBin()));
  (void)tbe_kernel_store.AddTBEKernel(ge::MakeShared<ge::OpKernelBin>("test_b", CreateStubBin()));
  ge_model->SetTBEKernelStore(tbe_kernel_store);

  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task);

  const std::vector<std::string> stub_funcs = {"stub_func_add", "stub_func_add_1"};
  for (const auto &stub_func : stub_funcs) {
    domi::TaskDef *task_def = model_task->add_task();
    task_def->set_type(static_cast<uint32_t>(ge::ModelTaskType::MODEL_TASK_KERNEL));
    task_def->set_stream_id(0);
    domi::KernelDef *kernel_def = task_def->mutable_kernel();
    kernel_def->set_stub_func(stub_func);
    kernel_def->set_args_size(64);
    std::string args(64, '1');
    kernel_def->set_args(args.data(), 64);
    kernel_def->set_kernel_name("test");

    domi::KernelContext *context = kernel_def->mutable_context();
    context->set_kernel_type(0);
    context->set_op_index(add_node->GetOpDesc()->GetId());
    uint16_t args_offset[9] = {0};
    context->set_args_offset(args_offset, 9 * sizeof(uint16_t));
    context->set_args_format("{arg0}{arg1}");
  }

  ge::GeRootModelPtr ge_root_model = std::make_shared<ge::GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(graph), ge::SUCCESS);
  ge_root_model->SetModelName(ge_model->GetName());
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  return ge_root_model;
}

}  // namespace

namespace ge {

class UtestMobileModelHelper : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtestMobileModelHelper, ModelToMobile) {
  std::string output_file = "mobile_model.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, true), ge::SUCCESS);
  GeRootModelPtr ge_root_model_invaild_data_type = GenGeRootModel(true);
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model_invaild_data_type, output_file, model, false),
            ge::SUCCESS);
  system("rm -rf mobile_model.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelDoesNotWriteSoOrCustomOpsPartitions) {
  const std::string test_case_name = "UtestMobileModelHelper_SaveToOmRootModelDoesNotWriteSoOrCustomOpsPartitions";
  EnvPath().RemoveRfCaseTmpPath(test_case_name);
  const std::string test_work_dir = EnvPath().GetOrCreateCaseTmpPath(test_case_name);
  ASSERT_FALSE(test_work_dir.empty());
  const MobileTmpDirGuardForTest test_work_dir_guard(test_work_dir);

  const std::string output_file = test_work_dir + "/mobile_custom_op_no_so.om";
  const std::string omc_file = output_file + "c";
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  const MobileOmcFileGuardForTest clean_file_guard(output_file, omc_file);

  const std::string op_type = "MobileNoSoPortableOpForTest";
  RegisterMobileNoSoPortableOpForTest(op_type);
  ASSERT_TRUE(CustomOpFactory::IsExistOp(op_type.c_str()));
  BaseCustomOp *custom_op = CustomOpFactory::CreateOrGetCustomOp(op_type.c_str(), OpBackend::kDevice);
  ASSERT_NE(custom_op, nullptr);
  ASSERT_NE(dynamic_cast<PortableOp *>(custom_op), nullptr);

  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel(false, op_type);
  ASSERT_NE(ge_root_model, nullptr);
  ge_root_model->SetCustomOpRegistry(CustomOpFactory::GetGlobalRegistryPtr());
  const std::string target_arch = GetCrossCompileTargetArchForTest();
  ASSERT_FALSE(target_arch.empty());
  const MobileHostEnvGuardForTest host_env_guard("linux", target_arch);
  const std::string custom_opp_path = test_work_dir + "/custom_opp";
  const MobileTmpDirGuardForTest custom_opp_dir_guard(custom_opp_path);
  const MobileEnvVarGuardForTest custom_opp_env_guard(kAscendCustomOppPathEnv);
  const std::string target_so_dir = custom_opp_path + "/op_graph/lib/linux/" + target_arch;
  ASSERT_EQ(CreateDirectory(target_so_dir), 0);
  const std::string target_so_path = target_so_dir + "/libmobile_custom_op_no_so_ut.so";
  uint16_t expected_machine = 0U;
  ASSERT_TRUE(GetExpectedElfMachineForCpuForTest(target_arch, expected_machine));
  ASSERT_TRUE(WriteElfHeaderForTest(target_so_path, expected_machine));
  ASSERT_EQ(mmSetEnv(kAscendCustomOppPathEnv, custom_opp_path.c_str(), 1), EN_OK);

  ASSERT_EQ(ge_root_model->CheckAndSetNeedSoInOM(), ge::SUCCESS);
  EXPECT_TRUE(OpSoStoreUtils::IsSoBinType(ge_root_model->GetSoInOmFlag(), SoBinType::kCustomOp));
  EXPECT_FALSE(ge_root_model->GetCustomOpSoSet().empty());
  EXPECT_TRUE(std::any_of(
      ge_root_model->GetCustomOpSoSet().cbegin(), ge_root_model->GetCustomOpSoSet().cend(),
      [](const std::string &so_path) { return so_path.find("libmobile_custom_op_no_so_ut.so") != std::string::npos; }));

  ASSERT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  const auto partitions = ParseMobilePartitions(omc_file);
  ASSERT_FALSE(partitions.empty());
  EXPECT_TRUE(HasPartitionType(partitions, kMobileModelDefPartition));
  EXPECT_TRUE(HasPartitionType(partitions, kMobileWeightsDataPartition));
  EXPECT_TRUE(HasPartitionType(partitions, kMobileTaskInfoPartition));
  for (const auto &partition : partitions) {
    EXPECT_TRUE(IsExpectedMobilePartitionType(partition.type))
        << "Unexpected mobile partition type: " << partition.type;
  }
  EXPECT_FALSE(HasPartitionType(partitions, kStandardOmCustomOpsPartition));
  EXPECT_FALSE(FileContainsStringForTest(omc_file, "libmobile_custom_op_no_so_ut.so"));
  EXPECT_FALSE(FileContainsStringForTest(omc_file, kMobileCustomOpSoPayloadMarker));
}

TEST_F(UtestMobileModelHelper, ModelToMobileWithInvalidDataType) {
  std::string output_file = "mobile_model_invalid.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel(true);
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_invalid.omc");
}

TEST_F(UtestMobileModelHelper, SetSaveMode) {
  MobileModelHelper model_save_helper;
  model_save_helper.SetSaveMode(true);
  model_save_helper.SetSaveMode(false);
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithConstNode) {
  std::string output_file = "mobile_model_const.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  auto const_op_desc = std::make_shared<ge::OpDesc>("const_node", "Const");
  ge::GeTensorDesc tensor_desc(ge::GeShape({8}), ge::FORMAT_ND, ge::DT_INT64);
  (void)const_op_desc->AddOutputDesc(tensor_desc);
  (void)graph->AddNode(const_op_desc);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_const.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithNetOutputMissingOutputDesc) {
  std::string output_file = "mobile_model_netout.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "NetOutput") {
      auto op_desc = node->GetOpDesc();
      ge::GeTensorDesc extra_desc(ge::GeShape({8}), ge::FORMAT_ND, ge::DT_INT64);
      (void)op_desc->AddInputDesc(extra_desc);
    }
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_netout.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithVariousAttrTypes_CovEnhance) {
  std::string output_file = "mobile_model_attrs.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    if (op_desc == nullptr) {
      continue;
    }
    if (node->GetType() == "Add") {
      std::vector<float> float_list = {1.0F, 2.0F, 3.0F};
      ge::AttrUtils::SetListFloat(op_desc, "test_list_float_attr", float_list);
      std::vector<bool> bool_list = {true, false, true};
      ge::AttrUtils::SetListBool(op_desc, "test_list_bool_attr", bool_list);
      std::vector<ge::Buffer> bytes_list = {ge::Buffer(10, 1), ge::Buffer(5, 0)};
      ge::AttrUtils::SetListBytes(op_desc, "test_list_bytes_attr", bytes_list);
      ge::GeTensorDesc tensor_desc(ge::GeShape({8}), ge::FORMAT_ND, ge::DT_INT64);
      ge::GeTensor tensor(tensor_desc);
      ge::AttrUtils::SetTensor(op_desc, "test_tensor_attr", tensor);
      std::vector<ge::GeTensor> tensor_list = {tensor};
      ge::AttrUtils::SetListTensor(op_desc, "test_list_tensor_attr", tensor_list);
      op_desc->SetWorkspace({1024, 2048});
      op_desc->SetWorkspaceBytes({1024, 2048});
      break;
    }
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_attrs.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithConnectedNodes_CovEnhance) {
  std::string output_file = "mobile_model_connected.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  ge::NodePtr data_node = nullptr;
  ge::NodePtr add_node = nullptr;
  ge::NodePtr netoutput_node = nullptr;
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Data") {
      data_node = node;
    } else if (node->GetType() == "Add") {
      add_node = node;
    } else if (node->GetType() == "NetOutput") {
      netoutput_node = node;
    }
  }
  if ((data_node != nullptr) && (add_node != nullptr)) {
    (void)ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
  }
  if ((add_node != nullptr) && (netoutput_node != nullptr)) {
    (void)ge::GraphUtils::AddEdge(add_node->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_connected.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithNullRootModel) {
  std::string output_file = "mobile_model_null.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr null_root_model = nullptr;
  EXPECT_NE(model_save_helper.SaveToOmRootModel(null_root_model, output_file, model, false), ge::SUCCESS);
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithEmptyOutputFile) {
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, "", model, false), ge::SUCCESS);
}

TEST_F(UtestMobileModelHelper, SaveToOmModelWithEmptyOutputFile) {
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto &name_to_ge_model = ge_root_model->GetSubgraphInstanceNameToModel();
  auto &model_root = name_to_ge_model.begin()->second;
  EXPECT_EQ(model_save_helper.SaveToOmModel(model_root, "", model, ge_root_model), ge::FAILED);
}

namespace {
// 变长自定义数据的 mobile 权威兜底：负例必须失败且不产生半成品 .omc，正例（CUSTOM_KERNEL 任务但
// args_format 不含 CUSTOM_DATA_BLOCK）不被误伤。两个用例均直接改写 ModelTaskDef，模拟
// 编译期检查因命中 kAnnotatedArgsTaskPlan 缓存被绕过后、携带 CUSTOM_DATA_BLOCK 的 task 到达保存路径的形态
bool FileExistsForTest(const std::string &file_path) {
  std::ifstream file(file_path.c_str());
  return file.is_open();
}

void SetTaskAsCustomKernelWithArgsFormat(const GeRootModelPtr &ge_root_model, const std::string &args_format) {
  auto &name_to_ge_model = ge_root_model->GetSubgraphInstanceNameToModel();
  auto &model_root = name_to_ge_model.begin()->second;
  const auto model_task = model_root->GetModelTaskDefPtr();
  ASSERT_NE(model_task, nullptr);
  ASSERT_GT(model_task->task_size(), 0);
  auto *task = model_task->mutable_task(0);
  task->set_type(static_cast<uint32_t>(ge::ModelTaskType::MODEL_TASK_CUSTOM_KERNEL));
  task->mutable_kernel()->mutable_context()->set_args_format(args_format);
}
}  // namespace

TEST_F(UtestMobileModelHelper, SaveToOmModelWithCustomDataArgsRejected) {
  std::string output_file = "mobile_model_custom_data.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  SetTaskAsCustomKernelWithArgsFormat(ge_root_model, "{custom_data_block0.32bytes}");
  // GE_ASSERT_SUCCESS 会经错误管理器返回具体错误码（如 0x50100001），不能断言等于裸的 ge::FAILED
  EXPECT_NE(model_save_helper.SaveToOmModel(ge_root_model->GetSubgraphInstanceNameToModel().begin()->second,
                                            output_file, model, ge_root_model),
            ge::SUCCESS);
  // 权威兜底必须发生在文件写出之前，不产生半成品 .omc
  EXPECT_FALSE(FileExistsForTest(output_file + "c"));
  system(("rm -rf " + output_file + "c").c_str());
}

TEST_F(UtestMobileModelHelper, SaveToOmModelWithCustomKernelTaskWithoutCustomData) {
  std::string output_file = "mobile_model_no_custom_data.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  // 同为 CUSTOM_KERNEL 任务，但 args_format 不含 CUSTOM_DATA_BLOCK，不应被兜底误伤
  SetTaskAsCustomKernelWithArgsFormat(ge_root_model, "{i_desc0}{o_desc0}");
  EXPECT_EQ(model_save_helper.SaveToOmModel(ge_root_model->GetSubgraphInstanceNameToModel().begin()->second,
                                            output_file, model, ge_root_model),
            ge::SUCCESS);
  // 正例确实会产出 .omc，作为负例中「不产生半成品 .omc」断言的对照
  EXPECT_TRUE(FileExistsForTest(output_file + "c"));
  system(("rm -rf " + output_file + "c").c_str());
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithEmptySubgraphModels) {
  std::string output_file = "mobile_model_empty_sub.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = std::make_shared<ge::GeRootModel>();
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithKernelArgsSizeZero) {
  std::string output_file = "mobile_model_zero_args.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  auto model_task = ge_model->GetModelTaskDefPtr();
  if (model_task != nullptr && model_task->task_size() > 0) {
    auto *task = model_task->mutable_task(0);
    auto *kernel = task->mutable_kernel();
    kernel->set_args_size(0);
    kernel->clear_args();
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_zero_args.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithNoKernelNameAttr) {
  std::string output_file = "mobile_model_no_kernelname.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Add") {
      (void)node->GetOpDesc()->DelAttr("_kernelname");
      break;
    }
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_no_kernelname.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithTaskDefAttrs) {
  std::string output_file = "mobile_model_task_attrs.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  auto model_task = ge_model->GetModelTaskDefPtr();
  if (model_task != nullptr) {
    auto *attr_map = model_task->mutable_attr();
    (*attr_map)["test_attr_key"] = "test_attr_value";
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_task_attrs.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithMultipleDynamicInputs) {
  std::string output_file = "mobile_model_multi_dynamic.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Add") {
      auto op_desc = node->GetOpDesc();
      std::vector<std::vector<int64_t>> dynamic_inputs_indexes = {{0, 1}, {0}};
      ge::AttrUtils::SetListListInt(op_desc, "_dynamic_inputs_indexes", dynamic_inputs_indexes);
      std::vector<std::vector<int64_t>> dynamic_outputs_indexes = {{0}, {0}};
      ge::AttrUtils::SetListListInt(op_desc, "_dynamic_outputs_indexes", dynamic_outputs_indexes);
      break;
    }
  }
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_multi_dynamic.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithInvalidDynamicInputIndex) {
  std::string output_file = "mobile_model_invalid_dynamic.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Add") {
      auto op_desc = node->GetOpDesc();
      std::vector<std::vector<int64_t>> dynamic_inputs_indexes = {{99}};
      ge::AttrUtils::SetListListInt(op_desc, "_dynamic_inputs_indexes", dynamic_inputs_indexes);
      break;
    }
  }
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_invalid_dynamic.omc");
}

TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithInvalidDynamicOutputIndex) {
  std::string output_file = "mobile_model_invalid_dynamic_out.om";
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModel();
  auto graph = ge_root_model->GetRootGraph();
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Add") {
      auto op_desc = node->GetOpDesc();
      std::vector<std::vector<int64_t>> dynamic_outputs_indexes = {{99}};
      ge::AttrUtils::SetListListInt(op_desc, "_dynamic_outputs_indexes", dynamic_outputs_indexes);
      break;
    }
  }
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
  system("rm -rf mobile_model_invalid_dynamic_out.omc");
}

// 验证 stub_func 与节点名的严格后缀匹配: 节点 "add" 不应误匹配 task 的 "stub_func_add_1"
TEST_F(UtestMobileModelHelper, SaveToOmRootModelWithSimilarNodeNameStubFunc) {
  const std::string output_file = "mobile_model_similar_node.om";
  const std::string omc_file = output_file + "c";
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  const MobileOmcFileGuardForTest clean_file_guard(output_file, omc_file);

  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithSimilarNodeNames();
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ASSERT_EQ(task_def.task_size(), 2);
  // 节点 "add"(kernel test_a) 只替换 stub_func_add, 节点 "add_1"(kernel test_b) 只替换 stub_func_add_1
  EXPECT_EQ(task_def.task(0).kernel().stub_func(), "test_a");
  EXPECT_EQ(task_def.task(1).kernel().stub_func(), "test_b");
}

// 外置权重模式1(每个权重单独导出)不被移动端 omc 支持, 保存应失败
TEST_F(UtestMobileModelHelper, SaveToOmModelWithExternalWeightMode1Failed) {
  const std::string output_file = "mobile_model_ext_weight_mode1.om";
  const std::string omc_file = output_file + "c";
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  const MobileOmcFileGuardForTest clean_file_guard(output_file, omc_file);

  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightEnabled);
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithFileConstants("", {}, 0UL);
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);
}

// 外置权重模式2(合并导出): 权重取自合并权重文件, weight_size 为文件大小, kernel args 中
// 变量区地址重映射为 权重区基址+文件偏移, 权重落盘到 <om目录>/weight/
TEST_F(UtestMobileModelHelper, SaveToOmModelWithExternalWeightCombined) {
  const std::string test_dir = "./mobile_ut_combined_weight";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_combined.om";
  const std::string omc_file = output_file + "c";
  const std::string weight_save_dir = test_dir + "/weight";
  (void)RemoveFilesInDirForTest(weight_save_dir);
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(weight_save_dir.c_str());
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  constexpr int64_t kWeightLength = 512;
  const std::vector<uint8_t> weight_content = CreateWeightBytesForTest(kWeightLength, 0x5AU);
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {
      {"fc_weight", 0, kWeightLength, static_cast<int64_t>(ge::kVarMemoryLogicBase), false, false}};
  GeRootModelPtr ge_root_model =
      GenGeRootModelWithFileConstants(weight_src_file, fc_specs, static_cast<uint64_t>(ge::kVarMemoryLogicBase));
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // 权重文件落盘到 <om目录>/weight/, 内容与源权重文件一致
  const auto weight_files = ListFilesInDirForTest(weight_save_dir);
  ASSERT_EQ(weight_files.size(), 1U);
  std::vector<uint8_t> saved_weight;
  ASSERT_TRUE(ReadBinaryFileForTest(weight_save_dir + "/" + weight_files[0], saved_weight));
  EXPECT_EQ(saved_weight, weight_content);

  // TASK_INFO: weight_size 为外置权重文件大小, args 中的变量区地址重映射为 weight_addr + file_offset
  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ASSERT_EQ(task_def.task_size(), 1);
  EXPECT_EQ(task_def.weight_size(), static_cast<uint64_t>(kWeightLength));
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 0U), task_def.weight_addr() + 0UL);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(weight_save_dir);
}

// 外置权重模式2: 合并权重文件实际大小小于 FileConstant 所需最大范围时, 保存应失败
TEST_F(UtestMobileModelHelper, SaveToOmModelWithExternalWeightCombinedSmallFileFailed) {
  const std::string test_dir = "./mobile_ut_combined_weight_small";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_combined_small.om";
  const std::string omc_file = output_file + "c";
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(test_dir);
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  const std::vector<uint8_t> weight_content(100U, 0x5AU);
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {
      {"fc_weight", 0, 512, static_cast<int64_t>(ge::kVarMemoryLogicBase), false, false}};
  GeRootModelPtr ge_root_model =
      GenGeRootModelWithFileConstants(weight_src_file, fc_specs, static_cast<uint64_t>(ge::kVarMemoryLogicBase));
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_NE(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(test_dir.c_str());
}

// 合并权重文件内容: [0,512) 为普通权重 0x11(fc_weight), [512,1024) 为共享权重 0x22(fc_shared)
std::vector<uint8_t> CreateSharedWeightBytesForTest() {
  std::vector<uint8_t> weight_content(1024U, 0U);
  for (size_t i = 0U; i < 512U; i++) {
    weight_content[i] = static_cast<uint8_t>(0x11U);
  }
  for (size_t i = 512U; i < 1024U; i++) {
    weight_content[i] = static_cast<uint8_t>(0x22U);
  }
  return weight_content;
}

// 验证重排落盘: weight_0.weight(共享段, 512B 全 0x22) + SubGraph_*.weight(非共享段, 4K 页对齐, 前 512B 0x11 余 0)
void ExpectReorderedWeightFilesForTest(const std::string &weight_save_dir) {
  const auto weight_files = ListFilesInDirForTest(weight_save_dir);
  ASSERT_EQ(weight_files.size(), 2U);
  std::string nonshared_file_name;
  bool has_shared_file = false;
  for (const auto &name : weight_files) {
    if (name == "weight_0.weight") {
      has_shared_file = true;
    } else if ((name.find("SubGraph_") == 0U) && (name.find(".weight") != std::string::npos)) {
      nonshared_file_name = name;
    }
  }
  ASSERT_FALSE(nonshared_file_name.empty());
  EXPECT_TRUE(has_shared_file);

  std::vector<uint8_t> nonshared_weight;
  ASSERT_TRUE(ReadBinaryFileForTest(weight_save_dir + "/" + nonshared_file_name, nonshared_weight));
  EXPECT_EQ(nonshared_weight.size(), 4096U);
  for (size_t i = 0U; i < 512U; i++) {
    EXPECT_EQ(nonshared_weight[i], static_cast<uint8_t>(0x11U));
  }
  for (size_t i = 512U; i < 4096U; i++) {
    EXPECT_EQ(nonshared_weight[i], 0U);
  }

  std::vector<uint8_t> shared_weight;
  ASSERT_TRUE(ReadBinaryFileForTest(weight_save_dir + "/weight_0.weight", shared_weight));
  EXPECT_EQ(shared_weight.size(), 512U);
  for (size_t i = 0U; i < 512U; i++) {
    EXPECT_EQ(shared_weight[i], static_cast<uint8_t>(0x22U));
  }
}

// 验证重排后 TASK_INFO: weight_size 为重排后总大小(4608), 两个变量区地址分别重映射到新文件偏移
void ExpectSharedReorderTaskArgsForTest(const ge::mobile::proto::ModelTaskDef &task_def) {
  EXPECT_EQ(task_def.task_size(), 1);
  EXPECT_EQ(task_def.weight_size(), 4608UL);
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 0U), task_def.weight_addr() + 0UL);
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 1U), task_def.weight_addr() + 4096UL);
}

// 验证 flowtable 重映射: flowtable ptr 槽与 args 中的变量区地址均重映射为 weight_addr + 文件偏移
void ExpectFlowtableRemapForTest(const ge::mobile::proto::ModelTaskDef &task_def) {
  ASSERT_EQ(task_def.task_size(), 1);
  EXPECT_EQ(task_def.weight_size(), 1024UL);
  const uint64_t weight_addr = task_def.weight_addr();
  // flowtable: 每组 TLV 为 [ptr_offset, dim, shape..., ptr...], shape {8} 时每组 32B
  // args[0]/args[1] 为输入/输出组的 flowtable 段偏移标记, args[2] 为直接拷贝槽(非变量区, 不重映射)
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 0U), 0x6B6B6B6B00000000UL);
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 1U), 0x6B6B6B6B00000000UL | 32UL);
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 2U), 0x3131313131313131UL);
  // flowtable 段1(动态输入组): ptr 槽 var_addr1 重映射为 weight_addr + 512(fc_shared 文件偏移)
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 0U), 24UL);
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 3U), 0x80000000UL | (weight_addr + 512UL));
  // flowtable 段2(动态输出组): ptr 槽 var_addr0 重映射为 weight_addr + 0(fc_weight 文件偏移)
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 4U), 24UL);
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 7U), 0x80000000UL | weight_addr);
}

// 外置权重合并模式 + 动态 shape(args_format 含 desc, 触发 flowtable):
// flowtable ptr 槽中的变量区地址须重映射为 weight_addr + 文件偏移(与普通 args 槽一致),
// 防止 ConvertTaskDef 读到未赋值的 weight_addr(默认 0)导致 flowtable 槽被重映射成 file_offset
TEST_F(UtestMobileModelHelper, SaveToOmModelWithExternalWeightCombinedFlowtable) {
  const std::string test_dir = "./mobile_ut_combined_weight_flowtable";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_combined_flowtable.om";
  const std::string omc_file = output_file + "c";
  const std::string weight_save_dir = test_dir + "/weight";
  (void)RemoveFilesInDirForTest(weight_save_dir);
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(weight_save_dir.c_str());
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  // 合并权重文件: [0,512) 为普通权重(fc_weight), [512,1024) 为共享权重(fc_shared)
  const std::vector<uint8_t> weight_content = CreateSharedWeightBytesForTest();
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  // 未设置 GE_MOBILE_WEIGHT_SHARE_OPS, Add 不在默认白名单(MatMulV3)中, 不触发共享权重重排
  const MobileEnvVarGuardForTest share_ops_env_guard(kMobileWeightShareOpsEnv);
  (void)unsetenv(kMobileWeightShareOpsEnv);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);

  const int64_t var_addr0 = static_cast<int64_t>(ge::kVarMemoryLogicBase);
  const int64_t var_addr1 = static_cast<int64_t>(ge::kVarMemoryLogicBase) + 4096;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {{"fc_weight", 0, 512, var_addr0, false, false},
                                                                   {"fc_shared", 512, 512, var_addr1, false, false}};
  // task_arg0/task_arg1 所在槽位(args[0]/args[1])会被 flowtable 段偏移标记覆盖, 传默认值仅占位
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithFileConstantsAndDynamicShape(weight_src_file, fc_specs);
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // TASK_INFO: weight_size 为权重文件大小, flowtable ptr 槽与普通槽均重映射为 weight_addr + 文件偏移
  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ExpectFlowtableRemapForTest(task_def);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(weight_save_dir);
}

// 非外置权重模式 + 动态 shape(args_format 含 desc, 触发 flowtable):
// 无 FileConstant 映射(var_addr_to_file_offset 为空)时, flowtable ptr 槽中的变量区地址
// 应保持原样(0x80000000 | offset)写入, 不触发重映射断言失败
TEST_F(UtestMobileModelHelper, SaveToOmModelWithoutExternalWeightFlowtableKeepsVarAddr) {
  const std::string output_file = "mobile_no_external_flowtable.om";
  const std::string omc_file = output_file + "c";
  const MobileOmcFileGuardForTest clean_file_guard(output_file, omc_file);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightDisabled);
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithDynamicShapeVarAddr();
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ASSERT_EQ(task_def.task_size(), 1);
  // flowtable: 每组 TLV 为 [ptr_offset, dim, shape, ptr], shape {8} 时每组 4 个 u64
  // 段1(动态输入组) ptr 槽在 u64 index 3, 段2(动态输出组) ptr 槽在 u64 index 7, 均保持原始变量区地址
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 3U),
            0x80000000UL | static_cast<uint64_t>(ge::kVarMemoryLogicBase));
  EXPECT_EQ(ReadMobileTaskFlowtableU64(task_def, 0U, 7U),
            0x80000000UL | (static_cast<uint64_t>(ge::kVarMemoryLogicBase) + 0x400UL));
}

// 外置权重模式2 online 场景: output_file 不含目录时, 权重落盘目录取自 FileConstant location 所在目录
TEST_F(UtestMobileModelHelper, SaveToOmModelWithExternalWeightCombinedOnlineDir) {
  const std::string test_dir = "./mobile_ut_combined_weight_online";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = "mobile_combined_online.om";
  const std::string omc_file = output_file + "c";
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(test_dir);
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  constexpr int64_t kWeightLength = 512;
  const std::vector<uint8_t> weight_content = CreateWeightBytesForTest(kWeightLength, 0x5AU);
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {
      {"fc_weight", 0, kWeightLength, static_cast<int64_t>(ge::kVarMemoryLogicBase), false, false}};
  GeRootModelPtr ge_root_model =
      GenGeRootModelWithFileConstants(weight_src_file, fc_specs, static_cast<uint64_t>(ge::kVarMemoryLogicBase));
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // 权重文件保存到 FileConstant location 所在目录(online 场景无 /weight 子目录)
  // test_dir 中还有用例预置的 weight_src.bin, 统计其中 .weight 文件
  const auto saved_files = ListFilesInDirForTest(test_dir);
  size_t weight_file_num = 0U;
  std::string saved_weight_file;
  for (const auto &name : saved_files) {
    if (name.find(".weight") != std::string::npos) {
      weight_file_num++;
      saved_weight_file = name;
    }
  }
  EXPECT_EQ(weight_file_num, 1U);
  std::vector<uint8_t> saved_weight;
  ASSERT_TRUE(ReadBinaryFileForTest(test_dir + "/" + saved_weight_file, saved_weight));
  EXPECT_EQ(saved_weight, weight_content);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(test_dir);
}

// 共享权重重排: 白名单算子(GE_MOBILE_WEIGHT_SHARE_OPS)的 FileConstant 输入重排到权重尾部,
// 权重分区保存为 SubGraph_<hash>.weight(非共享段, 4K 页对齐) + weight_0.weight(共享段)
TEST_F(UtestMobileModelHelper, SaveToOmModelWithSharedWeightReorder) {
  const std::string test_dir = "./mobile_ut_shared_weight";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_shared.om";
  const std::string omc_file = output_file + "c";
  const std::string weight_save_dir = test_dir + "/weight";
  (void)RemoveFilesInDirForTest(weight_save_dir);
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(weight_save_dir.c_str());
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  // 合并权重文件: [0,512) 为普通权重(fc_weight), [512,1024) 为共享权重(fc_shared)
  const std::vector<uint8_t> weight_content = CreateSharedWeightBytesForTest();
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  const MobileEnvVarGuardForTest share_ops_env_guard(kMobileWeightShareOpsEnv);
  ASSERT_EQ(mmSetEnv(kMobileWeightShareOpsEnv, "CustomShareOp", 1), EN_OK);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);

  const int64_t var_addr0 = static_cast<int64_t>(ge::kVarMemoryLogicBase);
  const int64_t var_addr1 = static_cast<int64_t>(ge::kVarMemoryLogicBase) + 4096;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {{"fc_weight", 0, 512, var_addr0, true, false},
                                                                   {"fc_shared", 512, 512, var_addr1, false, true}};
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithFileConstants(
      weight_src_file, fc_specs, static_cast<uint64_t>(var_addr0), static_cast<uint64_t>(var_addr1), "CustomShareOp");
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // 权重分区: 非共享段 512 对齐后 4K 页对齐, 共享段 512 字节
  ExpectReorderedWeightFilesForTest(weight_save_dir);

  // TASK_INFO: weight_size 为重排后总大小(4608), 两个变量区地址分别重映射到新文件偏移
  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ExpectSharedReorderTaskArgsForTest(task_def);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(weight_save_dir);
}

// 共享权重重排: 未设置 GE_MOBILE_WEIGHT_SHARE_OPS 时, 默认白名单算子(MatMulV3)同样触发重排
TEST_F(UtestMobileModelHelper, SaveToOmModelWithDefaultSharedWeightOps) {
  const std::string test_dir = "./mobile_ut_default_shared_weight";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_default_shared.om";
  const std::string omc_file = output_file + "c";
  const std::string weight_save_dir = test_dir + "/weight";
  (void)RemoveFilesInDirForTest(weight_save_dir);
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(weight_save_dir.c_str());
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  // 合并权重文件: [0,512) 为普通权重(fc_weight), [512,1024) 为共享权重(fc_shared)
  const std::vector<uint8_t> weight_content = CreateSharedWeightBytesForTest();
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  // 未设置 GE_MOBILE_WEIGHT_SHARE_OPS, MatMulV3 为默认白名单算子, 重排默认生效
  const MobileEnvVarGuardForTest share_ops_env_guard(kMobileWeightShareOpsEnv);
  (void)unsetenv(kMobileWeightShareOpsEnv);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);

  const int64_t var_addr0 = static_cast<int64_t>(ge::kVarMemoryLogicBase);
  const int64_t var_addr1 = static_cast<int64_t>(ge::kVarMemoryLogicBase) + 4096;
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {{"fc_weight", 0, 512, var_addr0, true, false},
                                                                   {"fc_shared", 512, 512, var_addr1, false, true}};
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithFileConstants(
      weight_src_file, fc_specs, static_cast<uint64_t>(var_addr0), static_cast<uint64_t>(var_addr1), "MatMulV3");
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // 共享段 fc_shared(MatMulV3 输入) 重排到 weight_0.weight, 非共享段 4K 页对齐
  ExpectReorderedWeightFilesForTest(weight_save_dir);

  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  ExpectSharedReorderTaskArgsForTest(task_def);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(weight_save_dir);
}

// CompiledModel::SaveToBuffer: 设置共享权重偏移后, 外置权重按分区导出为
// SubGraph_<非共享段sha256>.weight + weight_0.weight
TEST_F(UtestMobileModelHelper, CompiledModelSaveToBufferWithWeightPartition) {
  GeRootModelPtr ge_root_model = GenGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);
  auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  ASSERT_NE(ge_model, nullptr);

  std::vector<uint8_t> weight_data(1024U, 0U);
  for (size_t i = 0U; i < 512U; i++) {
    weight_data[i] = static_cast<uint8_t>(0xAAU);
  }
  for (size_t i = 512U; i < 1024U; i++) {
    weight_data[i] = static_cast<uint8_t>(0xBBU);
  }

  auto compiled_model = std::make_shared<ge::CompiledModel>();
  compiled_model->SetGeModel(ge_model);
  ge::BaseBuffer weight_buffer;
  weight_buffer.SetData(weight_data.data());
  weight_buffer.SetSize(weight_data.size());
  compiled_model->AddWeight(weight_buffer);
  compiled_model->SetWeightPartitionEnabled(true);
  compiled_model->SetSharedWeightSize(512UL);
  auto compiled_target = std::make_shared<ge::CompiledTarget>();
  compiled_target->AddModelTaskDef(std::make_shared<ge::mobile::proto::ModelTaskDef>());
  compiled_model->AddCompiledTarget(compiled_target);

  ge::BaseBuffer buffer;
  std::map<std::string, ge::BaseBuffer> weights_external;
  EXPECT_EQ(compiled_model->SaveToBuffer(buffer, true, &weights_external), ge::SUCCESS);
  ASSERT_EQ(weights_external.size(), 2U);

  // 非共享段文件名取前 512 字节(0xAA)的 sha256
  const std::string expected_hash = "799edf40e8115dc980109a64ff0a7ae2c6b62e20313c4a01f9871d0e189aa7c2";
  const auto nonshared_it = weights_external.find("SubGraph_" + expected_hash + ".weight");
  ASSERT_NE(nonshared_it, weights_external.end());
  EXPECT_EQ(nonshared_it->second.GetSize(), 512U);
  ASSERT_NE(nonshared_it->second.GetData(), nullptr);
  for (size_t i = 0U; i < 512U; i++) {
    EXPECT_EQ(*(nonshared_it->second.GetData() + i), static_cast<uint8_t>(0xAAU));
  }

  const auto shared_it = weights_external.find("weight_0.weight");
  ASSERT_NE(shared_it, weights_external.end());
  EXPECT_EQ(shared_it->second.GetSize(), 512U);
  ASSERT_NE(shared_it->second.GetData(), nullptr);
  for (size_t i = 0U; i < 512U; i++) {
    EXPECT_EQ(*(shared_it->second.GetData() + i), static_cast<uint8_t>(0xBBU));
  }
}

// CompiledModel::SaveToBuffer: 未开启权重分区时, 外置权重导出为单个 SubGraph_<sha256>.weight
TEST_F(UtestMobileModelHelper, CompiledModelSaveToBufferWithoutWeightPartition) {
  GeRootModelPtr ge_root_model = GenGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);
  auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  ASSERT_NE(ge_model, nullptr);

  std::vector<uint8_t> weight_data(1024U, 0x11U);

  auto compiled_model = std::make_shared<ge::CompiledModel>();
  compiled_model->SetGeModel(ge_model);
  ge::BaseBuffer weight_buffer;
  weight_buffer.SetData(weight_data.data());
  weight_buffer.SetSize(weight_data.size());
  compiled_model->AddWeight(weight_buffer);
  auto compiled_target = std::make_shared<ge::CompiledTarget>();
  compiled_target->AddModelTaskDef(std::make_shared<ge::mobile::proto::ModelTaskDef>());
  compiled_model->AddCompiledTarget(compiled_target);

  ge::BaseBuffer buffer;
  std::map<std::string, ge::BaseBuffer> weights_external;
  EXPECT_EQ(compiled_model->SaveToBuffer(buffer, true, &weights_external), ge::SUCCESS);
  ASSERT_EQ(weights_external.size(), 1U);

  // 文件名取全部权重(1024 字节 0x11)的 sha256
  const std::string expected_hash = "9f36749c5fb3b23ed904ad1582f24a6a65ef3b9e263b1be28af4f792ea269f43";
  const auto weight_it = weights_external.find("SubGraph_" + expected_hash + ".weight");
  ASSERT_NE(weight_it, weights_external.end());
  EXPECT_EQ(weight_it->second.GetSize(), 1024U);
  ASSERT_NE(weight_it->second.GetData(), nullptr);
  for (size_t i = 0U; i < 1024U; i++) {
    EXPECT_EQ(*(weight_it->second.GetData() + i), static_cast<uint8_t>(0x11U));
  }
}

// CompiledModel::SaveToBuffer: 权重全为共享(非共享段为空, 共享偏移为 0)时仍按分区导出,
// 避免共享分区被静默丢弃: 非共享段无文件(FileSaver 不支持空文件), 仅导出全量 weight_0.weight
TEST_F(UtestMobileModelHelper, CompiledModelSaveToBufferWithAllSharedWeightPartition) {
  GeRootModelPtr ge_root_model = GenGeRootModel();
  ASSERT_NE(ge_root_model, nullptr);
  auto ge_model = ge_root_model->GetSubgraphInstanceNameToModel().begin()->second;
  ASSERT_NE(ge_model, nullptr);

  std::vector<uint8_t> weight_data(1024U, 0x22U);

  auto compiled_model = std::make_shared<ge::CompiledModel>();
  compiled_model->SetGeModel(ge_model);
  ge::BaseBuffer weight_buffer;
  weight_buffer.SetData(weight_data.data());
  weight_buffer.SetSize(weight_data.size());
  compiled_model->AddWeight(weight_buffer);
  // 权重全为共享: 分区生效但共享偏移为 0(非共享段为空)
  compiled_model->SetWeightPartitionEnabled(true);
  compiled_model->SetSharedWeightSize(0UL);
  auto compiled_target = std::make_shared<ge::CompiledTarget>();
  compiled_target->AddModelTaskDef(std::make_shared<ge::mobile::proto::ModelTaskDef>());
  compiled_model->AddCompiledTarget(compiled_target);

  ge::BaseBuffer buffer;
  std::map<std::string, ge::BaseBuffer> weights_external;
  EXPECT_EQ(compiled_model->SaveToBuffer(buffer, true, &weights_external), ge::SUCCESS);
  ASSERT_EQ(weights_external.size(), 1U);

  // 共享段为全量
  const auto shared_it = weights_external.find("weight_0.weight");
  ASSERT_NE(shared_it, weights_external.end());
  EXPECT_EQ(shared_it->second.GetSize(), 1024U);
  ASSERT_NE(shared_it->second.GetData(), nullptr);
  for (size_t i = 0U; i < 1024U; i++) {
    EXPECT_EQ(*(shared_it->second.GetData() + i), static_cast<uint8_t>(0x22U));
  }
}

// 权重全为共享: 所有 FileConstant 均为白名单算子输入, 非共享段为空(shared_weight_size=0),
// 共享分区仍须导出: 仅 weight_0.weight(共享段全量), 共享权重不被静默丢弃
TEST_F(UtestMobileModelHelper, SaveToOmModelWithAllSharedWeightReorder) {
  const std::string test_dir = "./mobile_ut_all_shared_weight";
  const std::string weight_src_file = test_dir + "/weight_src.bin";
  const std::string output_file = test_dir + "/mobile_all_shared.om";
  const std::string omc_file = output_file + "c";
  const std::string weight_save_dir = test_dir + "/weight";
  (void)RemoveFilesInDirForTest(weight_save_dir);
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)mmRmdir(weight_save_dir.c_str());
  (void)mmRmdir(test_dir.c_str());
  ASSERT_EQ(CreateDirectory(test_dir), 0);

  // 合并权重文件: [0,512) 全为共享权重(fc_shared), 无普通权重
  const std::vector<uint8_t> weight_content = CreateWeightBytesForTest(512U, 0x33U);
  ASSERT_TRUE(WriteBinaryFileForTest(weight_src_file, weight_content));

  const MobileEnvVarGuardForTest share_ops_env_guard(kMobileWeightShareOpsEnv);
  ASSERT_EQ(mmSetEnv(kMobileWeightShareOpsEnv, "CustomShareOp", 1), EN_OK);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightCombined);

  const int64_t var_addr0 = static_cast<int64_t>(ge::kVarMemoryLogicBase);
  const std::vector<MobileFileConstantNodeSpecForTest> fc_specs = {{"fc_shared", 0, 512, var_addr0, false, true}};
  ModelBufferData model;
  MobileModelHelper model_save_helper;
  GeRootModelPtr ge_root_model = GenGeRootModelWithFileConstants(
      weight_src_file, fc_specs, static_cast<uint64_t>(var_addr0), 0UL, "CustomShareOp");
  ASSERT_NE(ge_root_model, nullptr);
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // 共享分区仍导出共享权重文件: weight_0.weight(共享段全量 512B 0x33)
  // 非共享段为空, 不导出非共享权重文件(FileSaver 不支持空文件)
  const auto weight_files = ListFilesInDirForTest(weight_save_dir);
  ASSERT_EQ(weight_files.size(), 1U);
  EXPECT_EQ(weight_files[0], "weight_0.weight");
  std::vector<uint8_t> shared_weight;
  ASSERT_TRUE(ReadBinaryFileForTest(weight_save_dir + "/weight_0.weight", shared_weight));
  EXPECT_EQ(shared_weight, weight_content);

  // TASK_INFO: weight_size 为重排后总大小(512), 变量区地址重映射到共享段起始(文件偏移 0)
  ge::mobile::proto::ModelTaskDef task_def;
  ASSERT_TRUE(ParseMobileTaskDefFromOmc(omc_file, task_def));
  EXPECT_EQ(task_def.weight_size(), 512UL);
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 0U), task_def.weight_addr() + 0UL);
  // args[1] 传 0, 非变量区地址, 不重映射
  EXPECT_EQ(ReadMobileTaskArg(task_def, 0U, 1U), 0UL);

  (void)std::remove(weight_src_file.c_str());
  (void)std::remove(output_file.c_str());
  (void)std::remove(omc_file.c_str());
  (void)RemoveFilesInDirForTest(weight_save_dir);
}

// NetOutput 输入落在变量区(模型输出直接来自权重)时, input_output_size 同步过滤变量区块:
// memory_size = fm峰值 - input_output_size 不会因变量区 IO 块多扣而成负数
TEST_F(UtestMobileModelHelper, SaveToOmModelWithVarAddrOutputKeepsMemorySizeNonNegative) {
  const std::string output_file = "mobile_var_addr_output.om";
  const std::string omc_file = output_file + "c";
  const MobileOmcFileGuardForTest clean_file_guard(output_file, omc_file);
  const MobileExternalWeightOptionGuardForTest option_guard(ge::kExternalWeightDisabled);

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr input0_node = CreateNode(*graph, "input0", "Data", 1, 1);
  ge::NodePtr input1_node = CreateNode(*graph, "input1", "Data", 1, 1);
  ge::NodePtr add_node = CreateNode(*graph, "add", "Add", 2, 1);
  ge::NodePtr netoutput_node = CreateNode(*graph, "output0", "NetOutput", 1, 1);
  ASSERT_NE(input0_node->GetOpDesc(), nullptr);
  ASSERT_NE(input1_node->GetOpDesc(), nullptr);
  ASSERT_NE(add_node->GetOpDesc(), nullptr);
  ASSERT_NE(netoutput_node->GetOpDesc(), nullptr);
  // fm 区 IO 块: input0@0x100, input1@0x200, add 输出@0x300; NetOutput 输入落在变量区(权重地址)
  input0_node->GetOpDesc()->SetOutputOffset({0x100});
  input1_node->GetOpDesc()->SetOutputOffset({0x200});
  add_node->GetOpDesc()->SetInputOffset({0x100, 0x200});
  add_node->GetOpDesc()->SetOutputOffset({0x300});
  netoutput_node->GetOpDesc()->SetInputOffset({static_cast<int64_t>(ge::kVarMemoryLogicBase)});

  ge::GeModelPtr ge_model = std::make_shared<ge::GeModel>();
  ge_model->SetGraph(graph);
  ge_model->SetName("test_model");
  std::shared_ptr<domi::ModelTaskDef> model_task = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task);
  AppendKernelTaskForTest(model_task, add_node->GetOpDesc(), "{arg0}{arg1}", 0UL, 0UL);
  GeRootModelPtr ge_root_model = WrapGeRootModelForTest(graph, ge_model);
  ASSERT_NE(ge_root_model, nullptr);

  ModelBufferData model;
  MobileModelHelper model_save_helper;
  EXPECT_EQ(model_save_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), ge::SUCCESS);

  // tensor size 64, 512 对齐后为 512:
  // fm 峰值 = max(0x100+512, 0x200+512, 0x300+512) = 1280
  // 变量区 NetOutput 输入不计入 input_output_size: input_output_size = 512 + 512 = 1024
  // memory_size = 1280 - 1024 = 256, 不为负
  ge::mobile::proto::ModelDef model_def;
  ASSERT_TRUE(ParseMobileModelDefFromOmc(omc_file, model_def));
  int64_t memory_size = 0;
  int64_t input_output_size = 0;
  ASSERT_TRUE(FindGraphOpIntAttr(model_def, "memory_size", memory_size));
  ASSERT_TRUE(FindGraphOpIntAttr(model_def, "input_output_size", input_output_size));
  EXPECT_EQ(input_output_size, 1024);
  EXPECT_EQ(memory_size, 256);
}
}  // namespace ge
