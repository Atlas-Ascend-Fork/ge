/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "securec.h"

#include "common/ge_common/ge_inner_error_codes.h"
#include "framework/runtime/dump/dump_config.h"
#include "framework/common/wire/wire_tlv.h"
#include "framework/runtime/dump/dump_transport_info.h"
#include "framework/runtime/dump/data_dump_impl.h"
#include "framework/runtime/dump/dump_op_impl.h"
#include "framework/runtime/dump/model_dump_manager.h"
#include "framework/runtime/dump/profiling_config.h"
#include "common/plugin/datatype_util.h"
#include "depends/runtime/src/runtime_stub.h"
#include "depends/ascendcl/src/ascendcl_stub.h"
#include "aicpu_task_struct.h"
#include "proto/op_mapping.pb.h"

using namespace testing;
using namespace ge::dump;

namespace ge {
namespace dump {
namespace {
class DumpWireMemoryStub : public AclRuntimeStub {
 public:
  std::map<void *, size_t> live;
  size_t allocations = 0U;
  size_t releases = 0U;
  size_t fail_allocation = 0U;
  bool fail_copy = false;

  aclError aclrtMalloc(void **ptr, size_t size, aclrtMemMallocPolicy) override {
    ++allocations;
    if (allocations == fail_allocation) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    *ptr = new uint8_t[size]{};
    live[*ptr] = size;
    return ACL_SUCCESS;
  }
  aclError aclrtFree(void *ptr) override {
    EXPECT_EQ(live.erase(ptr), 1U);
    ++releases;
    delete[] static_cast<uint8_t *>(ptr);
    return ACL_SUCCESS;
  }
  aclError aclrtMemcpy(void *dst, size_t capacity, const void *src, size_t size, aclrtMemcpyKind kind) override {
    EXPECT_EQ(kind, ACL_MEMCPY_HOST_TO_DEVICE);
    EXPECT_LE(size, capacity);
    if (fail_copy) {
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    // 安全拷贝失败只可能是夹具传参非法，视为测试失败并按错误码返回，不改变正常路径语义
    if (memcpy_s(dst, capacity, src, size) != EOK) {
      ADD_FAILURE() << "memcpy_s payload to device failed.";
      return ACL_ERROR_RT_INTERNAL_ERROR;
    }
    return ACL_SUCCESS;
  }
};

class DumpWireRuntimeStub : public RuntimeStub {
 public:
  struct Submission {
    const uint8_t *payload;
    const void *length_address;
    std::vector<uint8_t> bytes;
  };
  std::vector<Submission> models;
  std::vector<Submission> custom;
  rtError_t load_result = RT_ERROR_NONE;
  rtError_t launch_result = RT_ERROR_NONE;
  rtStream_t expected_stream = nullptr;

  rtError_t rtDatadumpInfoLoad(const void *data, uint32_t size) override {
    const auto bytes = static_cast<const uint8_t *>(data);
    models.push_back({bytes, nullptr, {bytes, bytes + size}});
    return load_result;
  }
  // 从 AICPU 启动参数中拷出参数头与两个设备地址；拷贝失败按非 RT_ERROR_NONE 返回（等同启动失败）
  bool CopyKernelArgs(const rtArgsEx_t *args, aicpu::AicpuParamHead &head, uint64_t (&addresses)[2]) {
    if (memcpy_s(&head, sizeof(head), args->args, sizeof(head)) != EOK) {
      ADD_FAILURE() << "memcpy_s AicpuParamHead failed.";
      return false;
    }
    if (memcpy_s(addresses, sizeof(addresses), static_cast<const uint8_t *>(args->args) + sizeof(head),
                 sizeof(addresses)) != EOK) {
      ADD_FAILURE() << "memcpy_s dump kernel addresses failed.";
      return false;
    }
    return true;
  }
  rtError_t rtCpuKernelLaunchWithFlag(const void *so, const void *kernel, uint32_t block_dim, const rtArgsEx_t *args,
                                      rtSmDesc_t *sm, rtStream_t stream, uint32_t flags) override {
    EXPECT_EQ(so, nullptr);
    EXPECT_EQ(sm, nullptr);
    EXPECT_STREQ(static_cast<const char *>(kernel), "DumpDataInfo");
    EXPECT_EQ(block_dim, 1U);
    EXPECT_EQ(flags, RT_KERNEL_DEFAULT);
    EXPECT_EQ(stream, expected_stream);
    aicpu::AicpuParamHead head{};
    uint64_t addresses[2]{};
    if (!CopyKernelArgs(args, head, addresses)) {
      return static_cast<rtError_t>(1);
    }
    EXPECT_EQ(head.ioAddrNum, 2U);
    EXPECT_EQ(head.length, sizeof(head) + 2U * sizeof(uint64_t));
    EXPECT_EQ(args->argsSize, head.length);
    EXPECT_EQ(args->isNoNeedH2DCopy, 0U);
    const auto payload = reinterpret_cast<const uint8_t *>(addresses[0]);
    const auto length = reinterpret_cast<const uint8_t *>(addresses[1]);
    uint64_t size = 0U;
    for (size_t i = 0; i < sizeof(uint64_t); ++i) {
      size |= static_cast<uint64_t>(length[i]) << (8U * i);
    }
    custom.push_back({payload, length, {payload, payload + size}});
    return launch_result;
  }
};

// 真机 HBM 设备地址量级：虚拟地址位于 4GB+ 区间(33 位)，varint 占字节数与真机口径一致
constexpr uint64_t kDevTensorAddr = 0x11C080000UL;       // tensor 原始设备地址(raw 地址场景)
constexpr uint64_t kDevArgsBase = 0x11C000000UL;         // args buffer 设备基地址
constexpr uint64_t kDevInputAddr = kDevArgsBase + 16U;   // args_base + input.offset
constexpr uint64_t kDevOutputAddr = kDevArgsBase + 24U;  // args_base + output.offset
constexpr uint64_t kDevWsAddr0 = 0x11C100000UL;
constexpr uint64_t kDevWsAddr1 = 0x11C180000UL;
constexpr uint64_t kDevLoopCondAddr = 0x11C200000UL;
constexpr uint64_t kDevIterationsAddr = 0x11C280000UL;
constexpr uint64_t kDevOpDebugAddr = 0x11C300000UL;

// hexdump 行缓冲容量：偏移列("%08zx ")9 字符 + 16 字节列每列 3 字符，80 字节足够容纳一行
constexpr size_t kHexLineCapacity = 80U;

// 载荷体积分析的产物目录：每个 scene 保存原始输入描述(.input.txt)、
// proto/TLV 序列化原始字节(.proto.bin/.tlv.bin)与带 ASCII 对照的 hexdump(.proto.hex.txt/.tlv.hex.txt)
// 目录可通过环境变量 GE_DUMP_WIRE_ANALYSIS_DIR 指定，未设置时落在 TempDir()/proto_tlv_size_analysis
std::string WireAnalysisDir() {
  const char *env = std::getenv("GE_DUMP_WIRE_ANALYSIS_DIR");
  std::string dir;
  if ((env != nullptr) && (env[0] != '\0')) {
    dir = env;
  } else {
    dir = ::testing::TempDir();
    if (!dir.empty() && (dir.back() != '/')) {
      dir += '/';
    }
    dir += "proto_tlv_size_analysis";
  }
  (void)::mkdir(dir.c_str(), 0755);
  return dir;
}

// 向行缓冲写入偏移列("%08zx ")，返回已写入长度；安全写入返回负值时按 0 处理（行缓冲被置空）
size_t AppendHexOffset(char *line, size_t line_capacity, size_t pos) {
  const int written = snprintf_s(line, line_capacity, line_capacity - 1U, "%08zx ", pos);
  return (written < 0) ? 0U : static_cast<size_t>(written);
}

// 在行缓冲 len 处追加一个字节列("%02x ")，返回新的已写入长度；
// 剩余空间不足一列或安全写入失败时长度不变，等价于原 snprintf 的截断保护
size_t AppendHexByte(char *line, size_t line_capacity, size_t len, uint8_t byte) {
  if ((len + 3U) >= line_capacity) {
    return len;
  }
  const int written = snprintf_s(line + len, line_capacity - len, line_capacity - len - 1U, "%02x ", byte);
  return (written < 0) ? len : len + static_cast<size_t>(written);
}

void SavePayloadFiles(const std::string &path_base, const std::vector<uint8_t> &bytes) {
  std::ofstream bin(path_base + ".bin", std::ios::binary | std::ios::trunc);
  bin.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  std::ofstream hex(path_base + ".hex.txt", std::ios::trunc);
  char line[kHexLineCapacity]{};
  for (size_t pos = 0U; pos < bytes.size(); pos += 16U) {
    size_t len = AppendHexOffset(line, sizeof(line), pos);
    std::string ascii;
    for (size_t i = pos; (i < pos + 16U) && (i < bytes.size()); ++i) {
      len = AppendHexByte(line, sizeof(line), len, bytes[i]);
      const bool printable = (bytes[i] >= 0x20U) && (bytes[i] < 0x7FU);
      ascii.push_back(printable ? static_cast<char>(bytes[i]) : '.');
    }
    hex << line << "|" << ascii << "|\n";
  }
}

// 同一份输入分别经 protobuf 与 DumpWire TLV 序列化后的载荷长度
struct WireSizes {
  size_t proto = 0U;
  size_t tlv = 0U;
};

template <typename ProtoIo, typename TlvIo>
void ExpectWireIoConsistent(const ProtoIo &proto_io, const TlvIo &tlv_io) {
  const DumpTransTensorInfo &base = tlv_io;
  EXPECT_EQ(proto_io.data_type(), base.GetDataType());
  EXPECT_EQ(proto_io.format(), base.GetFormat());
  EXPECT_EQ(proto_io.address(), base.GetAddress());
  EXPECT_EQ(proto_io.size(), base.GetSize());
  EXPECT_EQ(static_cast<uint32_t>(proto_io.addr_type()), static_cast<uint32_t>(base.GetAddrType()));
  const auto &shape = base.GetShape();
  ASSERT_EQ(static_cast<size_t>(proto_io.shape().dim_size()), shape.size());
  for (size_t i = 0U; i < shape.size(); ++i) {
    EXPECT_EQ(proto_io.shape().dim(static_cast<int>(i)), shape[i]);
  }
  // OriginShape/Offset 两侧同为 implicit，默认值不上线：只比值
  const auto &origin_shape = base.GetOriginShape();
  ASSERT_EQ(static_cast<size_t>(proto_io.origin_shape().dim_size()), origin_shape.size());
  for (size_t i = 0U; i < origin_shape.size(); ++i) {
    EXPECT_EQ(proto_io.origin_shape().dim(static_cast<int>(i)), origin_shape[i]);
  }
  EXPECT_EQ(proto_io.offset(), base.GetOffset());
}

void ExpectWireTaskConsistent(const toolkit::aicpu::dump::Task &proto_task, const DumpTransTaskInfo &tlv_task) {
  // Task 下全部字段两侧同为 implicit（无 oneof/explicit）：默认值不上线，只比值，getter 均读回类型默认值
  EXPECT_EQ(proto_task.task_id(), tlv_task.GetTaskId());
  EXPECT_EQ(proto_task.stream_id(), tlv_task.GetStreamId());
  EXPECT_EQ(proto_task.context_id(), tlv_task.GetContextId());
  EXPECT_EQ(proto_task.thread_id(), tlv_task.GetThreadId());
  EXPECT_EQ(proto_task.op().op_name(), tlv_task.GetOpName());
  EXPECT_EQ(proto_task.op().op_type(), tlv_task.GetOpType());
  EXPECT_EQ(proto_task.end_graph(), tlv_task.GetEndGraph());
  EXPECT_EQ(static_cast<uint32_t>(proto_task.task_type()), static_cast<uint32_t>(tlv_task.GetTaskType()));
  ASSERT_EQ(static_cast<size_t>(proto_task.input_size()), tlv_task.GetInputCount());
  for (size_t i = 0U; i < tlv_task.GetInputCount(); ++i) {
    const DumpTransInputInfo *io = nullptr;
    ASSERT_EQ(tlv_task.GetInput(i, io), DumpTransStatus::kOk);
    ExpectWireIoConsistent(proto_task.input(static_cast<int>(i)), *io);
  }
  ASSERT_EQ(static_cast<size_t>(proto_task.output_size()), tlv_task.GetOutputCount());
  for (size_t i = 0U; i < tlv_task.GetOutputCount(); ++i) {
    const DumpTransOutputInfo *io = nullptr;
    ASSERT_EQ(tlv_task.GetOutput(i, io), DumpTransStatus::kOk);
    const auto &proto_output = proto_task.output(static_cast<int>(i));
    ExpectWireIoConsistent(proto_output, *io);
    // Original* 字段按任务类型对称下发（普通不下发/OpDebug 下发）：只比值
    EXPECT_EQ(proto_output.original_name(), io->GetOriginalName());
    EXPECT_EQ(proto_output.original_output_index(), io->GetOriginalOutputIndex());
    EXPECT_EQ(proto_output.original_output_data_type(), io->GetOriginalOutputDataType());
    EXPECT_EQ(proto_output.original_output_format(), io->GetOriginalOutputFormat());
    EXPECT_EQ(io->GetDimRangeCount(), 0U);
  }
  ASSERT_EQ(static_cast<size_t>(proto_task.space_size()), tlv_task.GetWorkspaceCount());
  for (size_t i = 0U; i < tlv_task.GetWorkspaceCount(); ++i) {
    const DumpTransWorkspaceInfo *space = nullptr;
    ASSERT_EQ(tlv_task.GetWorkspace(i, space), DumpTransStatus::kOk);
    EXPECT_EQ(static_cast<uint32_t>(proto_task.space(static_cast<int>(i)).type()),
              static_cast<uint32_t>(space->GetType()));
    EXPECT_EQ(proto_task.space(static_cast<int>(i)).data_addr(), space->GetDataAddr());
    EXPECT_EQ(proto_task.space(static_cast<int>(i)).size(), space->GetSize());
  }
  EXPECT_EQ(static_cast<size_t>(proto_task.buffer_size()), tlv_task.GetBufferCount());
  EXPECT_EQ(static_cast<size_t>(proto_task.attr_size()), tlv_task.GetAttrCount());
  EXPECT_EQ(static_cast<size_t>(proto_task.context_size()), tlv_task.GetContextCount());
}

void ExpectWireModelConsistent(const toolkit::aicpu::dump::OpMappingInfo &proto_info,
                               const DumpTransModelInfo &tlv_model) {
  // model_name 等 5 字段：pb 单成员 oneof ↔ TLV explicit，两侧生产者条件一致，presence 严格对齐：has_xxx() ⟺ Has*()
  EXPECT_EQ(proto_info.has_model_name(), tlv_model.HasModelName());
  EXPECT_EQ(proto_info.has_model_id(), tlv_model.HasModelId());
  EXPECT_EQ(proto_info.has_step_id_addr(), tlv_model.HasStepIdAddr());
  EXPECT_EQ(proto_info.has_loop_cond_addr(), tlv_model.HasLoopCondAddr());
  EXPECT_EQ(proto_info.has_iterations_per_loop_addr(), tlv_model.HasIterationsPerLoopAddr());
  // 其余字段两侧同为 implicit：只比值
  EXPECT_EQ(proto_info.model_id(), tlv_model.GetModelId());
  EXPECT_EQ(proto_info.model_name(), tlv_model.GetModelName());
  EXPECT_EQ(proto_info.dump_path(), tlv_model.GetDumpPath());
  EXPECT_EQ(proto_info.dump_step(), tlv_model.GetDumpStep());
  EXPECT_EQ(proto_info.flag(), tlv_model.GetFlag());
  EXPECT_EQ(static_cast<uint32_t>(proto_info.dump_data()), static_cast<uint32_t>(tlv_model.GetDumpData()));
  EXPECT_EQ(proto_info.loop_cond_addr(), tlv_model.GetLoopCondAddr());
  EXPECT_EQ(proto_info.iterations_per_loop_addr(), tlv_model.GetIterationsPerLoopAddr());
  // pb 无 dump_switch_addr；TLV 生产者未 Set：Get* 恒为 0
  EXPECT_EQ(tlv_model.GetDumpSwitchAddr(), 0U);
}

class Om2DumpWireTest : public Test {
 protected:
  void SetUp() override {
    setenv("OM2_DUMP_DATA_VERSION", "1", 1);
    DumpConfig::Instance().Reset();
    ProfilingConfig::Instance().Disable();
    memory = std::make_shared<DumpWireMemoryStub>();
    runtime = std::make_shared<DumpWireRuntimeStub>();
    AclRuntimeStub::SetInstance(memory);
    RuntimeStub::SetInstance(runtime);
    tensor.SetData(gert::TensorData{reinterpret_cast<void *>(kDevTensorAddr), nullptr});
    tensor.SetSize(128U);
    tensor.SetDataType(DT_FLOAT);
    tensor.SetStorageFormat(FORMAT_ND);
    tensor.MutableStorageShape().AppendDim(2);
    tensor.MutableStorageShape().AppendDim(-1);
    tensor.MutableOriginShape().AppendDim(999);
    input.tensor = &tensor;
    input.offset = 16U;
    output.tensor = &tensor;
    output.offset = 24U;
    task.op_name = "layer1";
    task.op_type = "Custom";
    task.task_id = (1UL << 32U) + 7U;
    task.stream_id = (1UL << 32U) + 8U;
    task.context_id = (1UL << 32U) + 9U;
    task.thread_id = (1UL << 32U) + 10U;
    task.task_type = static_cast<uint32_t>(ModelTaskType::MODEL_TASK_KERNEL);
    task.inputs = &input;
    task.input_num = 1;
    task.outputs = &output;
    task.output_num = 1;
    task.args_base = kDevArgsBase;
    model.model_id = 42U;
    model.model_name = "test_model";
    model.device_id = 3U;
  }
  void TearDown() override {
    EXPECT_TRUE(memory->live.empty());
    AclRuntimeStub::Reset();
    RuntimeStub::Reset();
    DumpConfig::Instance().Reset();
    ProfilingConfig::Instance().Disable();
    unsetenv("OM2_DUMP_DATA_VERSION");
  }
  void Configure(const std::string &mode = "all", const std::string &data = "tensor") {
    const std::string config = "{\"dump\":{\"dump_path\":\"/tmp/om2_wire\",\"dump_mode\":\"" + mode +
                               "\",\"dump_step\":\"1|3-5\",\"dump_data\":\"" + data +
                               "\",\"dump_list\":[{\"model_name\":\"test_model\",\"layers\":[\"layer1\"]}]}}";
    ASSERT_EQ(DumpConfig::Instance().ParseAndValidate(config.c_str(), config.size()), SUCCESS);
  }
  void Decode(const DumpWireRuntimeStub::Submission &submission, DumpTransportInfo &info) {
    ASSERT_EQ(info.Deserialize(submission.bytes.data(), submission.bytes.size()), DumpTransStatus::kOk);
    ASSERT_EQ(memory->live.count(const_cast<uint8_t *>(submission.payload)), 1U);
    EXPECT_EQ(std::memcmp(submission.payload, submission.bytes.data(), submission.bytes.size()), 0);
    EXPECT_EQ(info.GetModel().GetModelId(), 42U);
    EXPECT_EQ(info.GetModel().GetModelName(), "test_model");
    EXPECT_EQ(info.GetModel().GetDumpPath(), DumpConfig::Instance().GetDumpPath() + "3/");
    EXPECT_EQ(info.GetModel().GetDumpStep(), "1|3-5");
    EXPECT_EQ(info.GetModel().GetFlag(), 1U);
    // StepIdAddr 是 explicit：生产者恒 Set，解码后 Has* 必为 true；step 设备内存为 8 字节（按 uint64_t 解引用）且初值 0
    ASSERT_TRUE(info.GetModel().HasStepIdAddr());
    const uint64_t step = info.GetModel().GetStepIdAddr();
    EXPECT_EQ(memory->live.at(reinterpret_cast<void *>(step)), sizeof(uint64_t));
    EXPECT_EQ(*reinterpret_cast<const uint64_t *>(step), 0U);
    // DumpSwitchAddr 是 implicit 且生产者未 Set：Get* 恒为 0
    EXPECT_EQ(info.GetModel().GetDumpSwitchAddr(), 0U);
  }
  void CheckTask(const DumpTransTaskInfo &info) {
    EXPECT_EQ(info.GetTaskId(), 7U);
    EXPECT_EQ(info.GetStreamId(), 8U);
    EXPECT_EQ(info.GetContextId(), 9U);
    EXPECT_EQ(info.GetThreadId(), 10U);
    EXPECT_EQ(info.GetOpName(), "layer1");
    EXPECT_EQ(info.GetOpType(), "Custom");
    // EndGraph/TaskType 是 implicit 且生产者未 Set：Get* 返回默认值 false / kAiCore(=0)
    EXPECT_FALSE(info.GetEndGraph());
    EXPECT_EQ(info.GetTaskType(), DumpTransTaskType::kAiCore);
    EXPECT_EQ(info.GetBufferCount(), 0U);
    EXPECT_EQ(info.GetAttrCount(), 0U);
    EXPECT_EQ(info.GetContextCount(), 0U);
  }
  void CheckTensor(const DumpTransTensorInfo &info, uint64_t address, DumpTransAddressType address_type) {
    EXPECT_EQ(info.GetAddress(), address);
    EXPECT_EQ(info.GetAddrType(), address_type);
    EXPECT_EQ(info.GetSize(), 128U);
    // dump 载荷的 data_type 写 IR(proto) 枚举(proto DT_FLOAT=1)，与 ge::DT_FLOAT=0 数值错位
    EXPECT_EQ(info.GetDataType(), DataTypeUtil::GetIrDataType(DT_FLOAT));
    EXPECT_EQ(info.GetFormat(), FORMAT_ND);
    EXPECT_EQ(info.GetShape(), (std::vector<uint64_t>{2U, UINT64_MAX}));
    // OriginShape/Offset 未 Set 不上线：Get* 返回默认空集合/0
    EXPECT_TRUE(info.GetOriginShape().empty());
    EXPECT_EQ(info.GetOffset(), 0U);
  }
  void CheckOutputDefaults(const DumpTransOutputInfo &info) {
    // 普通任务的 Output：生产者未 Set Original* 字段（implicit），不上线：解码后 Get* 返回类型默认值
    EXPECT_EQ(info.GetOriginalName(), "");
    EXPECT_EQ(info.GetOriginalOutputIndex(), 0);
    EXPECT_EQ(info.GetOriginalOutputDataType(), 0);
    EXPECT_EQ(info.GetOriginalOutputFormat(), 0);
    EXPECT_EQ(info.GetDimRangeCount(), 0U);
  }
  // 给 task 挂上 2 个 workspace（数组必须在调用方存活期内有效，故用引用形参）
  void SetupTwoWorkspaces(const uint64_t (&addresses)[2], const uint64_t (&sizes)[2]) {
    task.workspace_addrs = addresses;
    task.workspace_sizes = sizes;
    task.workspace_num = 2U;
  }
  // 交叉比对用例复用：同一输入分别走 proto 与 TLV 生产路径再解析回对象；版本号在构造 impl 时读取，
  // 故两个 impl 均在本函数内 setenv 之后构造，以 unique_ptr 交回调用方（用例内显式 Clear 校验归还）
  void BuildBothPayloadVariants(std::unique_ptr<DataDumpImpl> &proto_impl, std::unique_ptr<DataDumpImpl> &tlv_impl,
                                toolkit::aicpu::dump::OpMappingInfo &proto_info, DumpTransportInfo &tlv_info) {
    setenv("OM2_DUMP_DATA_VERSION", "0", 1);
    proto_impl = std::make_unique<DataDumpImpl>();
    ASSERT_TRUE(proto_impl->IsProtoVer());
    ASSERT_EQ(proto_impl->SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, true), SUCCESS);
    proto_impl->SetOpDebugInfo(20U, 21U, reinterpret_cast<void *>(kDevOpDebugAddr));
    ASSERT_EQ(proto_impl->BuildAndLoadOpMappingInfo(model), SUCCESS);
    setenv("OM2_DUMP_DATA_VERSION", "1", 1);
    tlv_impl = std::make_unique<DataDumpImpl>();
    ASSERT_FALSE(tlv_impl->IsProtoVer());
    ASSERT_EQ(tlv_impl->SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, true), SUCCESS);
    tlv_impl->SetOpDebugInfo(20U, 21U, reinterpret_cast<void *>(kDevOpDebugAddr));
    ASSERT_EQ(tlv_impl->BuildAndLoadDumpTransportInfo(model), SUCCESS);
    ASSERT_EQ(runtime->models.size(), 2U);
    const auto &proto_bytes = runtime->models[0].bytes;
    ASSERT_TRUE(proto_info.ParseFromArray(proto_bytes.data(), static_cast<int>(proto_bytes.size())));
    const auto &tlv_bytes = runtime->models[1].bytes;
    ASSERT_EQ(tlv_info.Deserialize(tlv_bytes.data(), tlv_bytes.size()), DumpTransStatus::kOk);
  }
  // 普通任务 + OpDebug 特殊任务，两种格式的 task 数量与内容应一致
  void ExpectTasksWireConsistent(const toolkit::aicpu::dump::OpMappingInfo &proto_info,
                                 const DumpTransportInfo &tlv_info) {
    ASSERT_EQ(static_cast<size_t>(proto_info.task_size()), tlv_info.GetTaskCount());
    ASSERT_EQ(tlv_info.GetTaskCount(), 2U);
    for (size_t i = 0U; i < tlv_info.GetTaskCount(); ++i) {
      const DumpTransTaskInfo *read = nullptr;
      ASSERT_EQ(tlv_info.GetTask(i, read), DumpTransStatus::kOk);
      ExpectWireTaskConsistent(proto_info.task(static_cast<int>(i)), *read);
    }
  }
  std::shared_ptr<DumpWireMemoryStub> memory;
  std::shared_ptr<DumpWireRuntimeStub> runtime;
  gert::Tensor tensor{};
  GertModelTaskIoEntry input{};
  GertModelTaskIoEntry output{};
  GertModelTaskDesc task{};
  ModelDumpInfo model{};
};

// 场景：模型级 dump，dump_mode(input/output/all) × 地址三态(args 折算、raw、offset 未知回退)，校验载荷字段编码
TEST_F(Om2DumpWireTest, ModelDump_ShouldEncodeModeAndAddrFields_WhenModeAndAddrKindVary) {
  for (const std::string mode : {"input", "output", "all"}) {
    for (const int address_case : {0, 1, 2}) {
      Configure(mode, "stats");
      task.is_raw_address = address_case == 1;
      input.offset = address_case == 2 ? UINT64_MAX : 16U;
      output.offset = address_case == 2 ? UINT64_MAX : 24U;
      DataDumpImpl impl;
      ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
      ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
      ASSERT_FALSE(runtime->models.empty());
      DumpTransportInfo decoded;
      Decode(runtime->models.back(), decoded);
      EXPECT_EQ(decoded.GetModel().GetDumpData(), DumpTransDumpData::kStats);
      // loop 地址为 0 时生产者不 Set(与 proto 侧的 set 条件一致),不上线:解码后 Has* 为 false,Get* 返回类型默认值
      EXPECT_FALSE(decoded.GetModel().HasLoopCondAddr());
      EXPECT_EQ(decoded.GetModel().GetLoopCondAddr(), 0U);
      EXPECT_FALSE(decoded.GetModel().HasIterationsPerLoopAddr());
      EXPECT_EQ(decoded.GetModel().GetIterationsPerLoopAddr(), 0U);
      ASSERT_EQ(decoded.GetTaskCount(), 1U);
      const DumpTransTaskInfo *read = nullptr;
      ASSERT_EQ(decoded.GetTask(0, read), DumpTransStatus::kOk);
      CheckTask(*read);
      EXPECT_EQ(read->GetWorkspaceCount(), 0U);
      EXPECT_EQ(read->GetInputCount(), mode == "output" ? 0U : 1U);
      EXPECT_EQ(read->GetOutputCount(), mode == "input" ? 0U : 1U);
      const auto type = address_case == 1 ? DumpTransAddressType::kRaw : DumpTransAddressType::kTraditional;
      if (read->GetInputCount() != 0U) {
        const DumpTransInputInfo *io = nullptr;
        ASSERT_EQ(read->GetInput(0, io), DumpTransStatus::kOk);
        CheckTensor(*io, address_case == 0 ? kDevInputAddr : kDevTensorAddr, type);
      }
      if (read->GetOutputCount() != 0U) {
        const DumpTransOutputInfo *io = nullptr;
        ASSERT_EQ(read->GetOutput(0, io), DumpTransStatus::kOk);
        CheckTensor(*io, address_case == 0 ? kDevOutputAddr : kDevTensorAddr, type);
        CheckOutputDefaults(*io);
      }
    }
  }
}

// 场景：stats 模式下重复 Build 释放上一份载荷、Clear 后可再次 Save/Build，校验设备内存生命周期与分配记账
TEST_F(Om2DumpWireTest, ModelDump_ShouldReleaseAndReuseDeviceMem_WhenRebuildAndClear) {
  Configure("all", "stats");
  DataDumpImpl impl;
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  DumpTransportInfo decoded;
  EXPECT_EQ(memory->live.size(), 2U);
  const auto releases = memory->releases;
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  EXPECT_EQ(memory->releases, releases + 1U);
  EXPECT_EQ(memory->live.size(), 2U);
  impl.Clear();
  EXPECT_TRUE(memory->live.empty());
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  Decode(runtime->models.back(), decoded);
}

// 场景：custom 算子在三种 dump_mode 下经前/后处理下发，前处理只发输入、后处理只发输出，校验参数头与两个设备指针
TEST_F(Om2DumpWireTest, CustomDump_ShouldSubmitIoAndOwnDeviceMem_WhenPrePostPerMode) {
  for (const std::string mode : {"input", "output", "all"}) {
    Configure(mode);
    task.task_type = static_cast<uint32_t>(ModelTaskType::MODEL_TASK_CUSTOM_KERNEL);
    task.is_raw_address = true;
    runtime->custom.clear();
    {
      ModelDumpManager manager(42U);
      ASSERT_EQ(manager.SetModelDumpInfo(model), SUCCESS);
      ASSERT_EQ(manager.PreprocessOm2TaskInfo(task), SUCCESS);
      EXPECT_EQ(runtime->custom.size(), mode == "output" ? 0U : 1U);
      ASSERT_EQ(manager.PostprocessOm2TaskInfo(task), SUCCESS);
      ASSERT_EQ(runtime->custom.size(), mode == "all" ? 2U : 1U);
      for (size_t index = 0; index < runtime->custom.size(); ++index) {
        const auto &submission = runtime->custom[index];
        EXPECT_EQ(memory->live.at(const_cast<void *>(submission.length_address)), sizeof(uint64_t));
        DumpTransportInfo decoded;
        Decode(submission, decoded);
        // DumpData 是 implicit：生产者恒 Set，但 kTensor 恰为类型默认值 0，不上线，解码后 Get* 返回默认值
        EXPECT_EQ(decoded.GetModel().GetDumpData(), DumpTransDumpData::kTensor);
        ASSERT_EQ(decoded.GetTaskCount(), 1U);
        const DumpTransTaskInfo *read = nullptr;
        ASSERT_EQ(decoded.GetTask(0, read), DumpTransStatus::kOk);
        CheckTask(*read);
        const bool is_input = mode == "input" || (mode == "all" && index == 0U);
        EXPECT_EQ(read->GetInputCount(), is_input ? 1U : 0U);
        EXPECT_EQ(read->GetOutputCount(), is_input ? 0U : 1U);
        EXPECT_EQ(read->GetWorkspaceCount(), 0U);
        if (is_input) {
          const DumpTransInputInfo *io = nullptr;
          ASSERT_EQ(read->GetInput(0, io), DumpTransStatus::kOk);
          CheckTensor(*io, kDevTensorAddr, DumpTransAddressType::kTraditional);
        } else {
          const DumpTransOutputInfo *io = nullptr;
          ASSERT_EQ(read->GetOutput(0, io), DumpTransStatus::kOk);
          CheckTensor(*io, kDevTensorAddr, DumpTransAddressType::kTraditional);
          CheckOutputDefaults(*io);
        }
      }
      const auto releases = memory->releases;
      ASSERT_EQ(manager.PreprocessOm2TaskInfo(task), SUCCESS);
      ASSERT_EQ(manager.PostprocessOm2TaskInfo(task), SUCCESS);
      EXPECT_EQ(memory->releases - releases, mode == "all" ? 4U : 2U);
    }
    EXPECT_TRUE(memory->live.empty());
  }
}

// 场景：溢出 dump(is_op_debug)且 dump_mode=input。普通任务被强制补齐输入输出，两个 workspace 上线，loop 地址透传
TEST_F(Om2DumpWireTest, OpDebugDump_ShouldForceIoAndBringUpWorkspace_WhenOverflowEnabled) {
  Configure("input");
  const uint64_t addresses[] = {kDevWsAddr0, kDevWsAddr1};
  const uint64_t sizes[] = {32U, 64U};
  SetupTwoWorkspaces(addresses, sizes);
  model.loop_cond_addr = kDevLoopCondAddr;
  model.iterations_per_loop_addr = kDevIterationsAddr;
  DataDumpImpl impl;
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, true), SUCCESS);
  impl.SetOpDebugInfo(20U, 21U, reinterpret_cast<void *>(kDevOpDebugAddr));
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  DumpTransportInfo decoded;
  Decode(runtime->models.back(), decoded);
  EXPECT_EQ(decoded.GetModel().GetLoopCondAddr(), kDevLoopCondAddr);
  EXPECT_EQ(decoded.GetModel().GetIterationsPerLoopAddr(), kDevIterationsAddr);
  ASSERT_EQ(decoded.GetTaskCount(), 2U);
  const DumpTransTaskInfo *read = nullptr;
  ASSERT_EQ(decoded.GetTask(0, read), DumpTransStatus::kOk);
  CheckTask(*read);
  EXPECT_EQ(read->GetInputCount(), 1U);
  EXPECT_EQ(read->GetOutputCount(), 1U);
  ASSERT_EQ(read->GetWorkspaceCount(), 2U);
  for (size_t i = 0; i < 2; ++i) {
    const DumpTransWorkspaceInfo *space = nullptr;
    ASSERT_EQ(read->GetWorkspace(i, space), DumpTransStatus::kOk);
    // Type 是 implicit：生产者 Set(kLog)，但其值恰为类型默认值 0，不上线，解码后 Get* 返回默认值
    EXPECT_EQ(space->GetType(), DumpTransWorkspaceType::kLog);
    EXPECT_EQ(space->GetDataAddr(), addresses[i]);
    EXPECT_EQ(space->GetSize(), sizes[i]);
  }
}

// 场景：溢出 dump 末尾追加的 Node_OpDebug 特殊任务(task_id=20、stream_id=21)，校验其标量字段与 debug buffer 输出明细
TEST_F(Om2DumpWireTest, OpDebugDump_ShouldAppendNodeOpDebugTaskDetail_WhenOverflowEnabled) {
  Configure("input");
  const uint64_t addresses[] = {kDevWsAddr0, kDevWsAddr1};
  const uint64_t sizes[] = {32U, 64U};
  SetupTwoWorkspaces(addresses, sizes);
  model.loop_cond_addr = kDevLoopCondAddr;
  model.iterations_per_loop_addr = kDevIterationsAddr;
  DataDumpImpl impl;
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, true), SUCCESS);
  impl.SetOpDebugInfo(20U, 21U, reinterpret_cast<void *>(kDevOpDebugAddr));
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  DumpTransportInfo decoded;
  Decode(runtime->models.back(), decoded);
  const DumpTransTaskInfo *read = nullptr;
  ASSERT_EQ(decoded.GetTask(1, read), DumpTransStatus::kOk);
  EXPECT_EQ(read->GetTaskId(), 20U);
  EXPECT_EQ(read->GetStreamId(), 21U);
  // OpDebug 任务：ContextId/ThreadId/TaskType 生产者未 Set；EndGraph Set(false) 为 implicit 默认值，
  // 均不上线：解码后 Get* 返回类型默认值
  EXPECT_EQ(read->GetContextId(), 0U);
  EXPECT_EQ(read->GetThreadId(), 0U);
  EXPECT_EQ(read->GetTaskType(), DumpTransTaskType::kAiCore);
  EXPECT_EQ(read->GetOpName(), "Node_OpDebug");
  EXPECT_EQ(read->GetOpType(), "Opdebug");
  EXPECT_FALSE(read->GetEndGraph());
  EXPECT_EQ(read->GetInputCount(), 0U);
  ASSERT_EQ(read->GetOutputCount(), 1U);
  const DumpTransOutputInfo *io = nullptr;
  ASSERT_EQ(read->GetOutput(0, io), DumpTransStatus::kOk);
  // OriginalName 非默认值上线；OriginalOutputIndex Set(0) 为 implicit 默认值不上线，Get* 返回默认值
  EXPECT_EQ(io->GetOriginalName(), "Node_OpDebug");
  EXPECT_EQ(io->GetOriginalOutputIndex(), 0);
  EXPECT_EQ(io->GetOriginalOutputDataType(), DT_UINT8);
  EXPECT_EQ(io->GetOriginalOutputFormat(), FORMAT_ND);
  const DumpTransTensorInfo &tensor_info = *io;
  EXPECT_EQ(tensor_info.GetDataType(), DT_UINT8);
  EXPECT_EQ(tensor_info.GetFormat(), FORMAT_ND);
  EXPECT_EQ(tensor_info.GetShape(), (std::vector<uint64_t>{2048U}));
  EXPECT_EQ(tensor_info.GetAddress(), kDevOpDebugAddr);
  EXPECT_EQ(tensor_info.GetSize(), 2048U);
  // AddrType Set(kTraditional=0)/OriginShape/Offset 均为 implicit 默认值或未 Set，不上线：Get* 返回类型默认值
  EXPECT_EQ(tensor_info.GetAddrType(), DumpTransAddressType::kTraditional);
  EXPECT_TRUE(tensor_info.GetOriginShape().empty());
  EXPECT_EQ(tensor_info.GetOffset(), 0U);
  EXPECT_EQ(io->GetDimRangeCount(), 0U);
}

// 场景：data dump 未开启（异常 dump 开/关两轮、profiling 打开），前/后处理与 DispatchDumpInfo 后不下发载荷也不申请内存
TEST_F(Om2DumpWireTest, ModelDump_ShouldNotSubmitOrAllocate_WhenDataDumpDisabled) {
  for (const bool exception : {false, true}) {
    DumpConfig::Instance().SetExceptionDumpEnabled(exception);
    ProfilingOptions options;
    options.task_time_enabled = true;
    ASSERT_EQ(ProfilingConfig::Instance().Enable(options), SUCCESS);
    ModelDumpManager manager(42U);
    ASSERT_EQ(manager.SetModelDumpInfo(model), SUCCESS);
    ASSERT_EQ(manager.PreprocessOm2TaskInfo(task), SUCCESS);
    ASSERT_EQ(manager.PostprocessOm2TaskInfo(task), SUCCESS);
    ASSERT_EQ(manager.DispatchDumpInfo(), SUCCESS);
    EXPECT_TRUE(runtime->models.empty());
    EXPECT_TRUE(runtime->custom.empty());
    EXPECT_EQ(memory->allocations, 0U);
  }
}

// 场景：任务列表为空，以及任务无任何 I/O 与算子名。前者跳过下发；后者集合为空、空串等默认值不上线。
TEST_F(Om2DumpWireTest, ModelDump_ShouldSkipSubmit_WhenNoTaskToDump) {
  Configure();
  DataDumpImpl impl;
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  EXPECT_TRUE(runtime->models.empty());
  EXPECT_EQ(memory->allocations, 0U);
  task.input_num = 0;
  task.output_num = 0;
  task.inputs = nullptr;
  task.outputs = nullptr;
  task.op_name = nullptr;
  task.op_type = nullptr;
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  DumpTransportInfo decoded;
  Decode(runtime->models.back(), decoded);
  const DumpTransTaskInfo *read = nullptr;
  ASSERT_EQ(decoded.GetTask(0, read), DumpTransStatus::kOk);
  EXPECT_EQ(read->GetInputCount(), 0U);
  EXPECT_EQ(read->GetOutputCount(), 0U);
  // op_name/op_type 是 implicit：生产者 Set 为空串，默认值不上线，解码后 Get* 返回默认空串
  EXPECT_EQ(read->GetOpName(), "");
  EXPECT_EQ(read->GetOpType(), "");
}

// 场景：rtDatadumpInfoLoad 返回失败。释放载荷内存只留 step 计数器，恢复后重试可成功，Clear 后设备内存全部归还。
TEST_F(Om2DumpWireTest, ModelDump_ShouldReleasePayloadAndAllowRetry_WhenRtLoadFails) {
  Configure();
  DataDumpImpl impl;
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
  runtime->load_result = static_cast<rtError_t>(1);
  EXPECT_EQ(impl.BuildAndLoadDumpTransportInfo(model), RT_FAILED);
  EXPECT_EQ(memory->live.size(), 1U);  // Only the cached step counter survives.
  runtime->load_result = RT_ERROR_NONE;
  ASSERT_EQ(impl.BuildAndLoadDumpTransportInfo(model), SUCCESS);
  EXPECT_EQ(memory->live.size(), 2U);
  impl.Clear();
  EXPECT_TRUE(memory->live.empty());
}

// 场景：custom 的 kernel launch 失败。载荷与 8 字节长度两块设备内存保留至 DumpOp 析构，且长度内存恒为 8 字节。
TEST_F(Om2DumpWireTest, CustomDump_ShouldRetainBothBuffers_WhenKernelLaunchFails) {
  Configure();
  {
    DataDumpImpl impl;
    DumpOp op;
    ASSERT_EQ(impl.BuildDumpTransportBasicInfo(model, op.GetDumpTransportInfo()), SUCCESS);
    ASSERT_EQ(op.BuildTaskInputs(task), SUCCESS);
    runtime->launch_result = static_cast<rtError_t>(1);
    EXPECT_NE(op.ExecutorDumpOp("layer1", nullptr), SUCCESS);
    ASSERT_EQ(runtime->custom.size(), 1U);
    EXPECT_EQ(memory->live.size(), 3U);
    EXPECT_EQ(memory->live.at(const_cast<void *>(runtime->custom.back().length_address)), sizeof(uint64_t));
  }
  EXPECT_TRUE(memory->live.empty());
}

// 场景：OM2_DUMP_DATA_VERSION 未设置 / 置 1 / 置非法值。依次得到 protobuf(0)、TLV(1)、非法值回落 protobuf(0)。
TEST(DumpDataVersionTest, DumpDataVersion_ShouldFollowEnvVar_WhenEnvVarSet) {
  unsetenv("OM2_DUMP_DATA_VERSION");
  EXPECT_EQ(GetDumpDataVersionFromEnv(), GE_DUMP_DATA_VERSION_PROTO);
  setenv("OM2_DUMP_DATA_VERSION", "1", 1);
  EXPECT_EQ(GetDumpDataVersionFromEnv(), GE_DUMP_DATA_VERSION_TLV);
  setenv("OM2_DUMP_DATA_VERSION", "0", 1);
  EXPECT_EQ(GetDumpDataVersionFromEnv(), GE_DUMP_DATA_VERSION_PROTO);
  setenv("OM2_DUMP_DATA_VERSION", "bad", 1);
  EXPECT_EQ(GetDumpDataVersionFromEnv(), GE_DUMP_DATA_VERSION_PROTO);
  unsetenv("OM2_DUMP_DATA_VERSION");
}

// 场景：版本环境变量置 0 走 protobuf 分支。模型级下发改产 OpMappingInfo，捕获的载荷可被 ParseFromString 解析。
TEST_F(Om2DumpWireTest, ModelDump_ShouldBuildProtoPayload_WhenVersionEnvIsProto) {
  setenv("OM2_DUMP_DATA_VERSION", "0", 1);
  Configure();
  DataDumpImpl impl;
  EXPECT_TRUE(impl.IsProtoVer());
  ASSERT_EQ(impl.SaveTask(task, ModelTaskType::MODEL_TASK_KERNEL, nullptr, false), SUCCESS);
  ASSERT_EQ(impl.BuildAndLoadOpMappingInfo(model), SUCCESS);
  ASSERT_FALSE(runtime->models.empty());
  const auto &submission = runtime->models.back();
  EXPECT_EQ(memory->live.count(const_cast<uint8_t *>(submission.payload)), 1U);
  toolkit::aicpu::dump::OpMappingInfo info;
  ASSERT_TRUE(info.ParseFromArray(submission.bytes.data(), static_cast<int>(submission.bytes.size())));
  EXPECT_EQ(info.model_id(), 42U);
  EXPECT_EQ(info.model_name(), "test_model");
  EXPECT_EQ(info.dump_path(), DumpConfig::Instance().GetDumpPath() + "3/");
  EXPECT_EQ(info.dump_step(), "1|3-5");
  EXPECT_EQ(info.flag(), 1U);
  EXPECT_EQ(info.dump_data(), toolkit::aicpu::dump::DumpData::TENSOR_DUMP_DATA);
  ASSERT_EQ(info.task_size(), 1);
  const auto &read = info.task(0);
  EXPECT_EQ(read.task_id(), 7U);
  EXPECT_EQ(read.stream_id(), 8U);
  EXPECT_EQ(read.context_id(), 9U);
  EXPECT_EQ(read.thread_id(), 10U);
  EXPECT_EQ(read.op().op_name(), "layer1");
  EXPECT_EQ(read.op().op_type(), "Custom");
  ASSERT_EQ(read.input_size(), 1);
  EXPECT_EQ(read.input(0).address(), kDevInputAddr);
  EXPECT_EQ(read.input(0).size(), 128U);
  // 与 CheckTensor 同理：载荷写 IR(proto) 枚举，proto DT_FLOAT=1 而 ge::DT_FLOAT=0
  EXPECT_EQ(read.input(0).data_type(), DataTypeUtil::GetIrDataType(DT_FLOAT));
  EXPECT_EQ(read.input(0).format(), FORMAT_ND);
  EXPECT_EQ(read.input(0).addr_type(), toolkit::aicpu::dump::AddressType::TRADITIONAL_ADDR);
  ASSERT_EQ(read.input(0).shape().dim_size(), 2);
  EXPECT_EQ(read.input(0).shape().dim(0), 2U);
  EXPECT_EQ(read.input(0).shape().dim(1), UINT64_MAX);
  ASSERT_EQ(read.output_size(), 1);
  EXPECT_EQ(read.output(0).address(), kDevOutputAddr);
  EXPECT_EQ(read.output(0).size(), 128U);
  EXPECT_EQ(read.space_size(), 0);
}

// 场景：版本环境变量置 0。custom 前/后处理与 ModelDumpManager 的三处分派全部走 protobuf 分支。
TEST_F(Om2DumpWireTest, CustomDump_ShouldUseProtoPath_WhenVersionEnvIsProto) {
  setenv("OM2_DUMP_DATA_VERSION", "0", 1);
  Configure();
  task.task_type = static_cast<uint32_t>(ModelTaskType::MODEL_TASK_CUSTOM_KERNEL);
  {
    ModelDumpManager manager(42U);
    ASSERT_EQ(manager.SetModelDumpInfo(model), SUCCESS);
    ASSERT_EQ(manager.PreprocessOm2TaskInfo(task), SUCCESS);
    ASSERT_EQ(manager.PostprocessOm2TaskInfo(task), SUCCESS);
    ASSERT_EQ(runtime->custom.size(), 2U);
    toolkit::aicpu::dump::OpMappingInfo input_info;
    const auto &input_bytes = runtime->custom[0].bytes;
    ASSERT_TRUE(input_info.ParseFromArray(input_bytes.data(), static_cast<int>(input_bytes.size())));
    EXPECT_EQ(input_info.model_id(), 42U);
    ASSERT_EQ(input_info.task_size(), 1);
    EXPECT_EQ(input_info.task(0).input_size(), 1);
    EXPECT_EQ(input_info.task(0).output_size(), 0);
    EXPECT_EQ(input_info.task(0).input(0).address(), kDevTensorAddr);
    toolkit::aicpu::dump::OpMappingInfo output_info;
    const auto &output_bytes = runtime->custom[1].bytes;
    ASSERT_TRUE(output_info.ParseFromArray(output_bytes.data(), static_cast<int>(output_bytes.size())));
    ASSERT_EQ(output_info.task_size(), 1);
    EXPECT_EQ(output_info.task(0).input_size(), 0);
    EXPECT_EQ(output_info.task(0).output_size(), 1);
    EXPECT_EQ(output_info.task(0).output(0).address(), kDevTensorAddr);
  }
  EXPECT_TRUE(memory->live.empty());

  task.task_type = static_cast<uint32_t>(ModelTaskType::MODEL_TASK_KERNEL);
  {
    ModelDumpManager manager(42U);
    ASSERT_EQ(manager.SetModelDumpInfo(model), SUCCESS);
    ASSERT_EQ(manager.PostprocessOm2TaskInfo(task), SUCCESS);
    ASSERT_EQ(manager.DispatchDumpInfo(), SUCCESS);
    ASSERT_FALSE(runtime->models.empty());
    const auto &submission = runtime->models.back();
    toolkit::aicpu::dump::OpMappingInfo info;
    ASSERT_TRUE(info.ParseFromArray(submission.bytes.data(), static_cast<int>(submission.bytes.size())));
    ASSERT_EQ(info.task_size(), 1);
    EXPECT_EQ(info.task(0).op().op_name(), "layer1");
  }
  EXPECT_TRUE(memory->live.empty());
}

// 场景：同一输入分别走 protobuf 与 TLV 生产路径下发：解码后逐字段交叉比对（5 个 oneof/explicit 字段
// presence 严格对齐），并校验两个 impl 的 step_id 计数器设备内存相互独立（各自分配 8 字节、初值 0、
// 地址不同）。两种载荷的体积大小关系与 golden 长度由文末 Om2DumpWireBbitTest 的 4 个真机用例断言
TEST_F(Om2DumpWireTest, DumpPayload_ShouldKeepProtoTlvFieldEquivalentAndStepIdIndependent_WhenSameInput) {
  Configure();
  const uint64_t addresses[] = {kDevWsAddr0, kDevWsAddr1};
  const uint64_t sizes[] = {32U, 64U};
  SetupTwoWorkspaces(addresses, sizes);
  model.loop_cond_addr = kDevLoopCondAddr;
  model.iterations_per_loop_addr = kDevIterationsAddr;
  std::unique_ptr<DataDumpImpl> proto_impl;
  std::unique_ptr<DataDumpImpl> tlv_impl;
  toolkit::aicpu::dump::OpMappingInfo proto_info;
  DumpTransportInfo tlv_info;
  BuildBothPayloadVariants(proto_impl, tlv_impl, proto_info, tlv_info);
  // helper 内断言失败会提前返回，这里挡住后续对 runtime->models / impl 的解引用
  ASSERT_EQ(runtime->models.size(), 2U);
  ExpectWireModelConsistent(proto_info, tlv_info.GetModel());
  ExpectTasksWireConsistent(proto_info, tlv_info);
  // step_id_addr 指向各自 impl 独立分配的设备内存，地址必然不同，只比较存活性和初始值
  ASSERT_TRUE(proto_info.has_step_id_addr());
  const auto proto_step = reinterpret_cast<const uint64_t *>(static_cast<uintptr_t>(proto_info.step_id_addr()));
  EXPECT_EQ(memory->live.at(const_cast<uint64_t *>(proto_step)), sizeof(uint64_t));
  EXPECT_EQ(*proto_step, 0U);
  ASSERT_TRUE(tlv_info.GetModel().HasStepIdAddr());
  const uint64_t tlv_step = tlv_info.GetModel().GetStepIdAddr();
  const auto tlv_step_ptr = reinterpret_cast<const uint64_t *>(static_cast<uintptr_t>(tlv_step));
  EXPECT_EQ(memory->live.at(const_cast<uint64_t *>(tlv_step_ptr)), sizeof(uint64_t));
  EXPECT_EQ(*tlv_step_ptr, 0U);
  EXPECT_NE(proto_info.step_id_addr(), tlv_step);
  proto_impl->Clear();
  tlv_impl->Clear();
  EXPECT_TRUE(memory->live.empty());
}

// 场景：model_name=nullptr 的防御分支——生产路径两条分支都按空串写入：pb set_model_name("") 与
// TLV explicit SetModelName("") 对称上线，解码后 presence 一致（两侧 Has 均 true）且值为空串
TEST_F(Om2DumpWireTest, ModelDump_ShouldTreatNullModelNameAsEmpty_WhenNamePtrIsNull) {
  Configure();
  model.model_name = nullptr;
  std::unique_ptr<DataDumpImpl> proto_impl;
  std::unique_ptr<DataDumpImpl> tlv_impl;
  toolkit::aicpu::dump::OpMappingInfo proto_info;
  DumpTransportInfo tlv_info;
  BuildBothPayloadVariants(proto_impl, tlv_impl, proto_info, tlv_info);
  EXPECT_TRUE(proto_info.has_model_name());
  EXPECT_EQ(proto_info.model_name(), "");
  EXPECT_TRUE(tlv_info.GetModel().HasModelName());
  EXPECT_EQ(tlv_info.GetModel().GetModelName(), "");
  proto_impl->Clear();
  tlv_impl->Clear();
  EXPECT_TRUE(memory->live.empty());
}

// ==================== DumpWire v1 schema 层用例（协议身份、tag 映射、presence、畸形码流） ====================
using Bytes = std::vector<uint8_t>;
// canonical 用例的 dump_path：字符串字段以裸字节上线，golden 直接由字符串构造
constexpr char kCanonicalDumpPath[] = "/home/test/dump_path";

// Independent fixture writer: never use the production codec to build golden inputs.
Bytes Le(uint64_t value, size_t width) {
  Bytes bytes;
  for (size_t i = 0U; i < width; ++i) {
    bytes.push_back(static_cast<uint8_t>(value & 255U));
    value >>= 8U;
  }
  return bytes;
}
// 值编码为规范 LEB128 varint(帧头仍是定宽 u16 tag + u16 length,由 Tlv/Wire 负责)
Bytes Varint(uint64_t value) {
  Bytes bytes;
  do {
    auto byte = static_cast<uint8_t>(value & 0x7FU);
    value >>= 7U;
    if (value != 0U) {
      byte |= 0x80U;
    }
    bytes.push_back(byte);
  } while (value != 0U);
  return bytes;
}
// 有符号值(含维度的"负值表示未知维"语义)先 zigzag 折叠再 varint
Bytes ZzVarint(int64_t value) {
  return Varint((static_cast<uint64_t>(value) << 1U) ^ static_cast<uint64_t>(value >> 63));
}
void Append(Bytes &to, const Bytes &from) {
  to.insert(to.end(), from.begin(), from.end());
}
Bytes Tlv(uint16_t tag, const Bytes &payload) {
  Bytes result = Le(tag, 2U);
  Append(result, Le(payload.size(), 2U));  // 记录头 length 为 u16
  Append(result, payload);
  return result;
}
Bytes Wire(const Bytes &records, uint32_t count) {
  Bytes result = {0x01, 0x54, 0x4c, 0x56, 1, 0, 16, 0};
  Append(result, Le(16U + records.size(), 4U));
  Append(result, Le(count, 4U));
  Append(result, records);
  return result;
}
Bytes WithTask(const Bytes &task) {
  Bytes records = Tlv(1U, {});
  Append(records, Tlv(2U, task));
  return Wire(records, 2U);
}

// 场景：空 DumpTransportInfo（无 TASK、MODEL 全不设）。校验 20B golden 与魔术字，以及解码/Clear 后的字段缺省行为。
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldMatchMagicAndDumpInfo_WhenNoModelAndTask) {
  // 公共头16B(magic4+version2+header_size2+total_size4+record_count4) + MODEL记录(tag2+length2=4B)，total_size=20。
  // MODEL 记录载荷为空即"全部字段未上线"的证据：未 Set 的 explicit 字段与 implicit 字段都不占任何字节
  const Bytes golden = {1, 0x54, 0x4c, 0x56, 1, 0, 16, 0, 20, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  DumpTransportInfo info;
  Bytes bytes{99};
  ASSERT_EQ(info.Serialize(bytes), DumpTransStatus::kOk);
  EXPECT_EQ(bytes, golden);
  // 未经 Set/反序列化的内存对象：implicit 字段(无 Has*)Get* 恒返回类型默认值；
  // explicit 字段 Has* 为 false，Get* 返回类型默认值
  EXPECT_EQ(info.GetModel().GetDumpSwitchAddr(), 0U);
  EXPECT_FALSE(info.GetModel().HasModelName());
  ASSERT_EQ(info.Deserialize(golden.data(), golden.size()), DumpTransStatus::kOk);
  EXPECT_EQ(info.GetVersion(), 1U);
  EXPECT_EQ(info.GetTaskCount(), 0U);
  // 解码不回填缺省值：空 MODEL 记录解码后 explicit 字段 Has* 仍为 false，Get* 返回类型默认值
  EXPECT_FALSE(info.GetModel().HasModelName());
  EXPECT_EQ(info.GetModel().GetModelName(), "");
  EXPECT_EQ(info.GetModel().GetDumpSwitchAddr(), 0U);
  // implicit 字段 Set 成默认值 0 也不上线：重编码仍与 golden 逐字节一致
  info.MutableModel().SetDumpSwitchAddr(0U);
  EXPECT_EQ(info.GetModel().GetDumpSwitchAddr(), 0U);
  Bytes reencoded;
  ASSERT_EQ(info.Serialize(reencoded), DumpTransStatus::kOk);
  EXPECT_EQ(reencoded, golden);
  const DumpTransTaskInfo *task = nullptr;
  EXPECT_EQ(info.GetTask(0U, task), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(task, nullptr);
  info.Clear();
  EXPECT_FALSE(info.GetModel().HasModelName());
  EXPECT_EQ(info.GetModel().GetDumpSwitchAddr(), 0U);
}

void BuildCanonicalInfo(DumpTransportInfo &info) {
  info.MutableModel().SetDumpPath(kCanonicalDumpPath);
  info.MutableModel().SetModelId(0x12345678U);
  info.MutableModel().SetModelName("");
  auto &task = info.AddTask();
  task.SetTaskId(888U);
  task.SetStreamId(889U);
  task.SetTaskType(DumpTransTaskType::kAiCore);
  task.SetEndGraph(false);
  auto &input = task.AddInput();
  input.SetShape({UINT64_MAX, 2U});
  input.SetDataType(-1);
}

// golden 逐条推导（帧头 tag:u16+length:u16 定宽小端；标量为规范 LEB128 varint，有符号先 zigzag；字符串裸字节）：
//   DumpPath "/home/test/dump_path"  implicit 非默认值 → 0x0101: tag2+len2+裸字节20 = 24B
//   ModelName ""       EXPLICIT presence(单成员 oneof 语义)：Set 过即上线 → 0x0102: tag2+len2(=0)+空串0B = 4B
//   ModelId 0x12345678 EXPLICIT 非默认值 → 0x0103: tag2+len2+varint5 = 9B
//   EndGraph(false)/TaskType(kAiCore=0) 为 implicit 且值等于类型默认值 → 不上线，0 字节
Bytes BuildCanonicalModel() {
  // 字符串字段的线上形态是裸字节（长度由所在记录 length 给出），golden 直接取字符字节
  const std::string dump_path(kCanonicalDumpPath);
  Bytes model = Tlv(0x0101, Bytes(dump_path.begin(), dump_path.end()));
  Append(model, Tlv(0x0102, {}));
  Append(model, Tlv(0x0103, Varint(0x12345678U)));
  return model;
}

Bytes BuildCanonicalTask() {
  Bytes task_bytes = Tlv(0x0201, Varint(888U));
  Append(task_bytes, Tlv(0x0202, Varint(889U)));
  // DataType=-1 走 zigzag:zigzag(-1)=1,单字节(定宽时代为 4 字节补码)
  Bytes input_bytes = Tlv(0x1001, ZzVarint(-1));
  // shape=[UINT64_MAX, 2] 的线上语义即 [-1, 2]:count varint + 每维 zigzag varint
  Bytes shape = Varint(2U);
  Append(shape, ZzVarint(-1));
  Append(shape, ZzVarint(2));
  Append(input_bytes, Tlv(0x1003, shape));
  Append(task_bytes, Tlv(0x0010, input_bytes));
  return task_bytes;
}

Bytes BuildCanonicalRecords() {
  Bytes records = Tlv(1, BuildCanonicalModel());
  Append(records, Tlv(2, BuildCanonicalTask()));
  return records;
}

// 场景：1 MODEL + 1 TASK + 1 INPUT 的规范载荷。逐字节比对 golden，含有符号 DataType(-1) 的 zigzag 与 shape 未知维 -1。
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldMatchGoldenBytes_WhenSignedAndDefaultFields) {
  DumpTransportInfo info;
  BuildCanonicalInfo(info);
  Bytes encoded;
  ASSERT_EQ(info.Serialize(encoded), DumpTransStatus::kOk);
  EXPECT_EQ(encoded, Wire(BuildCanonicalRecords(), 2));
}

// 场景：规范载荷解码回读。zigzag 有符号语义(DataType=-1、shape=[UINT64_MAX, 2] 即 [-1, 2])、
// 两类 presence 的解码语义、帧头魔术字与任务遍历
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldDecodeCanonicalFieldsAndFrame_WhenRoundTripGolden) {
  DumpTransportInfo info;
  BuildCanonicalInfo(info);
  Bytes encoded;
  ASSERT_EQ(info.Serialize(encoded), DumpTransStatus::kOk);
  DumpTransportInfo decoded;
  ASSERT_EQ(decoded.Deserialize(encoded.data(), encoded.size()), DumpTransStatus::kOk);
  const DumpTransTaskInfo *decoded_task = nullptr;
  ASSERT_EQ(decoded.GetTask(0, decoded_task), DumpTransStatus::kOk);
  const DumpTransInputInfo *decoded_input = nullptr;
  ASSERT_EQ(decoded_task->GetInput(0, decoded_input), DumpTransStatus::kOk);
  EXPECT_EQ(decoded_input->GetDataType(), -1);
  EXPECT_EQ(decoded_input->GetShape(), (std::vector<uint64_t>{UINT64_MAX, 2}));
  // ModelName 是 explicit presence：Set("") 确实上线，解码后 Has* 为 true + Get* 返回写入的默认值空串
  EXPECT_TRUE(decoded.GetModel().HasModelName());
  EXPECT_EQ(decoded.GetModel().GetModelName(), "");
  // EndGraph/TaskType 是 implicit 且值为类型默认值：不上线，解码后(无 Has*)Get* 返回类型默认值 false / kAiCore(=0)
  EXPECT_FALSE(decoded_task->GetEndGraph());
  EXPECT_EQ(decoded_task->GetTaskType(), DumpTransTaskType::kAiCore);
  // 通过首字符是不是tlv版本的魔术字来判断
  EXPECT_EQ(ge::wire::ReadLe(encoded.data(), sizeof(uint32_t)), ge::dump::DumpWireSchema::kMagic);
  ge::dump::DumpTransportInfo transportInfo;
  ASSERT_EQ(transportInfo.Deserialize(encoded.data(), encoded.size()), DumpTransStatus::kOk);
  EXPECT_EQ(transportInfo.GetModel().GetDumpPath(), kCanonicalDumpPath);
  EXPECT_EQ(transportInfo.GetTaskCount(), 1);
  for (size_t i = 0U; i < transportInfo.GetTaskCount(); ++i) {
    const ge::dump::DumpTransTaskInfo *taskRecv = nullptr;
    EXPECT_EQ(transportInfo.GetTask(i, taskRecv), ge::dump::DumpTransStatus::kOk);
    EXPECT_EQ(taskRecv->GetTaskId(), 888U);
    EXPECT_EQ(taskRecv->GetStreamId(), 889U);
  }
}

void FillModel(DumpTransModelInfo &record) {
  record.SetDumpPath(std::string("a\0b", 3));
  record.SetModelName(std::string("a\0b", 3));
  record.SetModelId(259U);
  record.SetStepIdAddr(260U);
  record.SetIterationsPerLoopAddr(261U);
  record.SetLoopCondAddr(262U);
  record.SetFlag(263U);
  record.SetDumpStep(std::string("a\0b", 3));
  record.SetDumpData(static_cast<DumpTransDumpData>(99U));
  record.SetDumpSwitchAddr(266U);
}
void CheckModel(const DumpTransModelInfo &record) {
  EXPECT_EQ(record.GetDumpPath(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetModelName(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetModelId(), 259U);
  EXPECT_EQ(record.GetStepIdAddr(), 260U);
  EXPECT_EQ(record.GetIterationsPerLoopAddr(), 261U);
  EXPECT_EQ(record.GetLoopCondAddr(), 262U);
  EXPECT_EQ(record.GetFlag(), 263U);
  EXPECT_EQ(record.GetDumpStep(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetDumpData(), static_cast<DumpTransDumpData>(99U));
  EXPECT_EQ(record.GetDumpSwitchAddr(), 266U);
}

void FillDimRange(DumpTransDimRangeInfo &record) {
  record.SetDimStart(2561U);
  record.SetDimEnd(2562U);
}
void CheckDimRange(const DumpTransDimRangeInfo &record) {
  EXPECT_EQ(record.GetDimStart(), 2561U);
  EXPECT_EQ(record.GetDimEnd(), 2562U);
}

void FillInput(DumpTransInputInfo &record) {
  record.SetDataType(-123);
  record.SetFormat(-123);
  record.SetShape(std::vector<uint64_t>{0U, UINT64_MAX, 42U});
  record.SetAddress(772U);
  record.SetSize(773U);
  record.SetOriginShape(std::vector<uint64_t>{0U, UINT64_MAX, 42U});
  record.SetAddrType(static_cast<DumpTransAddressType>(99U));
  record.SetOffset(776U);
}
void CheckInput(const DumpTransInputInfo &record) {
  EXPECT_EQ(record.GetDataType(), -123);
  EXPECT_EQ(record.GetFormat(), -123);
  EXPECT_EQ(record.GetShape(), (std::vector<uint64_t>{0U, UINT64_MAX, 42U}));
  EXPECT_EQ(record.GetAddress(), 772U);
  EXPECT_EQ(record.GetSize(), 773U);
  EXPECT_EQ(record.GetOriginShape(), (std::vector<uint64_t>{0U, UINT64_MAX, 42U}));
  EXPECT_EQ(record.GetAddrType(), static_cast<DumpTransAddressType>(99U));
  EXPECT_EQ(record.GetOffset(), 776U);
}

void FillOutput(DumpTransOutputInfo &record) {
  record.SetDataType(-123);
  record.SetFormat(-123);
  record.SetShape(std::vector<uint64_t>{0U, UINT64_MAX, 42U});
  record.SetAddress(1028U);
  record.SetOriginalName(std::string("a\0b", 3));
  record.SetOriginalOutputIndex(-123);
  record.SetOriginalOutputDataType(-123);
  record.SetOriginalOutputFormat(-123);
  record.SetSize(1033U);
  record.SetOriginShape(std::vector<uint64_t>{0U, UINT64_MAX, 42U});
  record.SetAddrType(static_cast<DumpTransAddressType>(99U));
  record.SetOffset(1036U);
}
void CheckOutput(const DumpTransOutputInfo &record) {
  EXPECT_EQ(record.GetDataType(), -123);
  EXPECT_EQ(record.GetFormat(), -123);
  EXPECT_EQ(record.GetShape(), (std::vector<uint64_t>{0U, UINT64_MAX, 42U}));
  EXPECT_EQ(record.GetAddress(), 1028U);
  EXPECT_EQ(record.GetOriginalName(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetOriginalOutputIndex(), -123);
  EXPECT_EQ(record.GetOriginalOutputDataType(), -123);
  EXPECT_EQ(record.GetOriginalOutputFormat(), -123);
  EXPECT_EQ(record.GetSize(), 1033U);
  EXPECT_EQ(record.GetOriginShape(), (std::vector<uint64_t>{0U, UINT64_MAX, 42U}));
  EXPECT_EQ(record.GetAddrType(), static_cast<DumpTransAddressType>(99U));
  EXPECT_EQ(record.GetOffset(), 1036U);
}

void FillWorkspace(DumpTransWorkspaceInfo &record) {
  record.SetType(static_cast<DumpTransWorkspaceType>(99U));
  record.SetDataAddr(1282U);
  record.SetSize(1283U);
}
void CheckWorkspace(const DumpTransWorkspaceInfo &record) {
  EXPECT_EQ(record.GetType(), static_cast<DumpTransWorkspaceType>(99U));
  EXPECT_EQ(record.GetDataAddr(), 1282U);
  EXPECT_EQ(record.GetSize(), 1283U);
}

void FillBuffer(DumpTransBufferInfo &record) {
  record.SetType(static_cast<DumpTransBufferType>(99U));
  record.SetAddress(1538U);
  record.SetSize(1539U);
}
void CheckBuffer(const DumpTransBufferInfo &record) {
  EXPECT_EQ(record.GetType(), static_cast<DumpTransBufferType>(99U));
  EXPECT_EQ(record.GetAddress(), 1538U);
  EXPECT_EQ(record.GetSize(), 1539U);
}

void FillAttr(DumpTransAttrInfo &record) {
  record.SetName(std::string("a\0b", 3));
  record.SetValue(std::string("a\0b", 3));
}
void CheckAttr(const DumpTransAttrInfo &record) {
  EXPECT_EQ(record.GetName(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetValue(), std::string("a\0b", 3));
}

void FillAddrSize(DumpTransAddrSizeInfo &record) {
  record.SetAddress(2305U);
  record.SetSize(2306U);
}
void CheckAddrSize(const DumpTransAddrSizeInfo &record) {
  EXPECT_EQ(record.GetAddress(), 2305U);
  EXPECT_EQ(record.GetSize(), 2306U);
}

void FillContext(DumpTransContextInfo &record) {
  record.SetContextId(2049U);
  record.SetThreadId(2050U);
}
void CheckContext(const DumpTransContextInfo &record) {
  EXPECT_EQ(record.GetContextId(), 2049U);
  EXPECT_EQ(record.GetThreadId(), 2050U);
}

void FillDumpTransTask(DumpTransTaskInfo &record) {
  record.SetTaskId(513U);
  record.SetStreamId(514U);
  record.SetOpName(std::string("a\0b", 3));
  record.SetOpType(std::string("a\0b", 3));
  record.SetEndGraph(true);
  record.SetTaskType(static_cast<DumpTransTaskType>(99U));
  record.SetContextId(519U);
  record.SetThreadId(520U);
}
void CheckDumpTransTask(const DumpTransTaskInfo &record) {
  EXPECT_EQ(record.GetTaskId(), 513U);
  EXPECT_EQ(record.GetStreamId(), 514U);
  EXPECT_EQ(record.GetOpName(), std::string("a\0b", 3));
  EXPECT_EQ(record.GetOpType(), std::string("a\0b", 3));
  EXPECT_TRUE(record.GetEndGraph());
  EXPECT_EQ(record.GetTaskType(), static_cast<DumpTransTaskType>(99U));
  EXPECT_EQ(record.GetContextId(), 519U);
  EXPECT_EQ(record.GetThreadId(), 520U);
}

void FillAllFieldsInfo(DumpTransportInfo &info) {
  FillModel(info.MutableModel());
  auto &task = info.AddTask();
  FillDumpTransTask(task);
  FillInput(task.AddInput());
  auto &output = task.AddOutput();
  FillOutput(output);
  FillDimRange(output.AddDimRange());
  output.AddDimRange().SetDimStart(99U);
  FillWorkspace(task.AddWorkspace());
  FillBuffer(task.AddBuffer());
  FillAttr(task.AddAttr());
  auto &context = task.AddContext();
  FillContext(context);
  FillAddrSize(context.AddInput());
  FillAddrSize(context.AddOutput());
  info.AddTask().SetTaskId(999U);
}

// 场景：dump schema 全部记录类型填满（含 BUFFER/ATTR/CONTEXT/DIM_RANGE）+ 未登记枚举值，往返后六个集合逐字段相等
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldRoundTripCollectionsAndUnknownEnums_WhenAllFieldsFilled) {
  DumpTransportInfo info;
  FillAllFieldsInfo(info);
  Bytes wire;
  ASSERT_EQ(info.Serialize(wire), DumpTransStatus::kOk);
  DumpTransportInfo decoded;
  ASSERT_EQ(decoded.Deserialize(wire.data(), wire.size()), DumpTransStatus::kOk);
  CheckModel(decoded.GetModel());
  EXPECT_EQ(decoded.GetTaskCount(), 2U);
  const DumpTransTaskInfo *read_task = nullptr;
  ASSERT_EQ(decoded.GetTask(0, read_task), DumpTransStatus::kOk);
  CheckDumpTransTask(*read_task);
  // 逐个集合字段：count=1、索引 0 可读且内容一致、索引 1 越界返回 kInvalidParam
  EXPECT_EQ(read_task->GetInputCount(), 1U);
  const DumpTransInputInfo *read_Input = nullptr;
  ASSERT_EQ(read_task->GetInput(0, read_Input), DumpTransStatus::kOk);
  CheckInput(*read_Input);
  EXPECT_EQ(read_task->GetInput(1, read_Input), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(read_task->GetOutputCount(), 1U);
  const DumpTransOutputInfo *read_Output = nullptr;
  ASSERT_EQ(read_task->GetOutput(0, read_Output), DumpTransStatus::kOk);
  CheckOutput(*read_Output);
  EXPECT_EQ(read_task->GetOutput(1, read_Output), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(read_task->GetWorkspaceCount(), 1U);
  const DumpTransWorkspaceInfo *read_Workspace = nullptr;
  ASSERT_EQ(read_task->GetWorkspace(0, read_Workspace), DumpTransStatus::kOk);
  CheckWorkspace(*read_Workspace);
  EXPECT_EQ(read_task->GetWorkspace(1, read_Workspace), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(read_task->GetBufferCount(), 1U);
  const DumpTransBufferInfo *read_Buffer = nullptr;
  ASSERT_EQ(read_task->GetBuffer(0, read_Buffer), DumpTransStatus::kOk);
  CheckBuffer(*read_Buffer);
  EXPECT_EQ(read_task->GetBuffer(1, read_Buffer), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(read_task->GetAttrCount(), 1U);
  const DumpTransAttrInfo *read_Attr = nullptr;
  ASSERT_EQ(read_task->GetAttr(0, read_Attr), DumpTransStatus::kOk);
  CheckAttr(*read_Attr);
  EXPECT_EQ(read_task->GetAttr(1, read_Attr), DumpTransStatus::kInvalidParam);
  EXPECT_EQ(read_task->GetContextCount(), 1U);
  const DumpTransContextInfo *read_Context = nullptr;
  ASSERT_EQ(read_task->GetContext(0, read_Context), DumpTransStatus::kOk);
  CheckContext(*read_Context);
  EXPECT_EQ(read_task->GetContext(1, read_Context), DumpTransStatus::kInvalidParam);
  ASSERT_EQ(decoded.GetTask(1, read_task), DumpTransStatus::kOk);
  EXPECT_EQ(read_task->GetTaskId(), 999U);
}

// 场景：同一份全字段码流的 DIM_RANGE 明细、CONTEXT 输入/输出地址对，以及未知枚举值经再编码后与原码流逐字节一致
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldRoundTripDimRangeAndContextAddrs_WhenAllFieldsFilled) {
  DumpTransportInfo info;
  FillAllFieldsInfo(info);
  Bytes wire;
  ASSERT_EQ(info.Serialize(wire), DumpTransStatus::kOk);
  DumpTransportInfo decoded;
  ASSERT_EQ(decoded.Deserialize(wire.data(), wire.size()), DumpTransStatus::kOk);
  const DumpTransTaskInfo *read_task = nullptr;
  ASSERT_EQ(decoded.GetTask(0, read_task), DumpTransStatus::kOk);
  const DumpTransOutputInfo *read_Output = nullptr;
  ASSERT_EQ(read_task->GetOutput(0, read_Output), DumpTransStatus::kOk);
  EXPECT_EQ(read_Output->GetDimRangeCount(), 2U);
  const DumpTransDimRangeInfo *range = nullptr;
  ASSERT_EQ(read_Output->GetDimRange(0, range), DumpTransStatus::kOk);
  CheckDimRange(*range);
  ASSERT_EQ(read_Output->GetDimRange(1, range), DumpTransStatus::kOk);
  EXPECT_EQ(range->GetDimStart(), 99U);
  // DimEnd 未 Set → 不上线 → 解码不回填：implicit 字段(无 Has*)，Get* 返回类型默认值 0
  EXPECT_EQ(range->GetDimEnd(), 0U);
  const DumpTransContextInfo *read_Context = nullptr;
  ASSERT_EQ(read_task->GetContext(0, read_Context), DumpTransStatus::kOk);
  const DumpTransAddrSizeInfo *address = nullptr;
  EXPECT_EQ(read_Context->GetInputCount(), 1U);
  EXPECT_EQ(read_Context->GetOutputCount(), 1U);
  ASSERT_EQ(read_Context->GetInput(0, address), DumpTransStatus::kOk);
  CheckAddrSize(*address);
  ASSERT_EQ(read_Context->GetOutput(0, address), DumpTransStatus::kOk);
  CheckAddrSize(*address);
  // 未知枚举值与两类 presence 语义都被完整保留：解码对象再编码与原码流逐字节一致
  Bytes reencoded;
  ASSERT_EQ(decoded.Serialize(reencoded), DumpTransStatus::kOk);
  EXPECT_EQ(wire, reencoded);
}

// 场景：手工构造含未知 tag、ModelId 重复出现、TASK 排在 MODEL 之前的码流。未知 tag 跳过，重复标量取末次值，乱序可解。
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldTolerateUnknownTagAndDupScalar_WhenRecordsReordered) {
  Bytes model = Tlv(0x0103, Varint(7U));
  Append(model, Tlv(0xffff, {1, 2, 3}));
  Append(model, Tlv(0x0103, Varint(9U)));
  Bytes records = Tlv(2, {});
  Append(records, Tlv(0xfffe, {0, 1}));
  Append(records, Tlv(1, model));
  const auto wire = Wire(records, 3);
  DumpTransportInfo info;
  ASSERT_EQ(info.Deserialize(wire.data(), wire.size()), DumpTransStatus::kOk);
  EXPECT_EQ(info.GetModel().GetModelId(), 9U);
  EXPECT_EQ(info.GetTaskCount(), 1U);
  Bytes encoded;
  ASSERT_EQ(info.Serialize(encoded), DumpTransStatus::kOk);
  Bytes expected = Tlv(1, Tlv(0x0103, Varint(9U)));
  Append(expected, Tlv(2, {}));
  EXPECT_EQ(encoded, Wire(expected, 2));
}

struct MalformedCase {
  Bytes wire;
  DumpTransStatus expected;
};

// 单个畸形样本：断言解码错误码 + 断言原对象未被修改（重编码后仍与 unchanged 逐字节一致，即无部分提交）
void RejectOneMalformed(DumpTransportInfo &info, const Bytes &unchanged, const Bytes &wire, DumpTransStatus expected) {
  EXPECT_EQ(info.Deserialize(wire.data(), wire.size()), expected);
  Bytes after;
  ASSERT_EQ(info.Serialize(after), DumpTransStatus::kOk);
  EXPECT_EQ(after, unchanged);
}

// 帧头与记录级畸形：版本非法 / 空记录集 / 重复 MODEL 记录 / 非法记录 tag / 跨层次标签 / 非法 bool 值
void AddFrameMalformedCases(std::vector<MalformedCase> &cases) {
  const Bytes valid = Wire(Tlv(1, {}), 1);
  Bytes bad_version = valid;
  // version 是 u16 小端字段：两字节同时置满取 UINT16_MAX，覆盖高低字节（只改低字节等价于 version=2）
  bad_version[4] = 0xFFU;
  bad_version[5] = 0xFFU;
  cases.push_back({bad_version, DumpTransStatus::kUnsupportedVersion});
  cases.push_back({Wire({}, 0), DumpTransStatus::kInvalidFormat});
  Bytes duplicate_model = Tlv(1, {});
  Append(duplicate_model, Tlv(1, {}));
  cases.push_back({Wire(duplicate_model, 2), DumpTransStatus::kInvalidFormat});
  // 只有 TASK 缺 MODEL：Deserialize 要求 has_model，缺失判格式非法
  cases.push_back({Wire(Tlv(2, {}), 1), DumpTransStatus::kInvalidFormat});
  cases.push_back({Wire(Tlv(0x10, {}), 1), DumpTransStatus::kInvalidFormat});
  // 跨层次标签:MODEL 内出现 TaskId。值用规范 varint,确保命中的是层次校验而非长度校验
  cases.push_back({Wire(Tlv(1, Tlv(0x0201, Varint(1U))), 1), DumpTransStatus::kInvalidFormat});
  // bool 仍为定宽 1 字节且只允许 0/1
  cases.push_back({WithTask(Tlv(0x0205, {2})), DumpTransStatus::kInvalidFormat});
}

// 维度集合:count 超出剩余字节可容纳的维数 → 长度溢出;尾部多余字节 → 格式非法
void AddCollectionMalformedCases(std::vector<MalformedCase> &cases) {
  cases.push_back({WithTask(Tlv(0x0010, Tlv(0x1003, Varint(1U)))), DumpTransStatus::kLengthOverflow});
  cases.push_back({WithTask(Tlv(0x0010, Tlv(0x1003, Varint(UINT32_MAX)))), DumpTransStatus::kLengthOverflow});
  Bytes bad_shape = Varint(1U);
  Append(bad_shape, ZzVarint(2));
  bad_shape.push_back(0U);
  cases.push_back({WithTask(Tlv(0x0010, Tlv(0x1003, bad_shape))), DumpTransStatus::kInvalidFormat});
  // 层次校验:Input 内不含 DimRange;Output 内 DimRange 的字段值为截断 varint
  cases.push_back({WithTask(Tlv(0x0010, Tlv(0x0018, {}))), DumpTransStatus::kInvalidFormat});
  cases.push_back({WithTask(Tlv(0x0011, Tlv(0x0018, Tlv(0x1801, Bytes{0x80U})))), DumpTransStatus::kInvalidFormat});
  cases.push_back({WithTask({0x10, 0, 255, 255, 255, 255}), DumpTransStatus::kInvalidFormat});
}

// 场景：帧与层次类畸形（公共头版本非法、任意前缀截断、重复 MODEL、非法记录 tag、跨层次标签、非法 bool）。
// 公共头/帧头字段级畸形样本（magic、header_size、total_size、record_count、length 越界）由框架层
// wire_tlv_frame_unittest.cc 的 header/frame 电池覆盖，此处只保留 dump 文档分派特有类别
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldRejectFrameMalformed_WhenTruncatedFlippedOrBadRecord) {
  const Bytes valid = Wire(Tlv(1, {}), 1);
  DumpTransportInfo info;
  info.MutableModel().SetModelId(77U);
  Bytes unchanged;
  ASSERT_EQ(info.Serialize(unchanged), DumpTransStatus::kOk);
  EXPECT_EQ(info.Deserialize(nullptr, 16), DumpTransStatus::kInvalidParam);
  // 截断：合法码流的任意非空前缀都不是完整码流
  for (size_t i = 1; i < valid.size(); ++i) {
    RejectOneMalformed(info, unchanged, Bytes(valid.begin(), valid.begin() + i), DumpTransStatus::kInvalidFormat);
  }
  std::vector<MalformedCase> cases;
  AddFrameMalformedCases(cases);
  for (const auto &malformed : cases) {
    RejectOneMalformed(info, unchanged, malformed.wire, malformed.expected);
  }
}

// 场景：集合与维度类畸形（维数 count 溢出、维度尾部多余字节、层次非法、DIM_RANGE 字段截断、record_count 不符）。
// varint/标量值编码类通用畸形（超长、移位溢出、overlong、截断、bool/u32/i32 值域）由框架层
// wire_tlv_frame_unittest.cc 的 malformed 电池覆盖，schema 层只保留 dump 文档分派特有的类别
TEST(DumpTransportInfoTest, DumpTransInfo_ShouldRejectCollectionMalformed_WhenDimCountOrHierarchyBroken) {
  DumpTransportInfo info;
  info.MutableModel().SetModelId(77U);
  Bytes unchanged;
  ASSERT_EQ(info.Serialize(unchanged), DumpTransStatus::kOk);
  std::vector<MalformedCase> cases;
  AddCollectionMalformedCases(cases);
  for (const auto &malformed : cases) {
    RejectOneMalformed(info, unchanged, malformed.wire, malformed.expected);
  }
}

// presence 与 protobuf 同构：explicit 字段（WireTlvField<T, true>）≙ 单成员 oneof，Set 过即上线并提供 Has*；
// implicit 字段 ≙ proto3 plain 标量，默认值不上线。详见 dump_transport_info.h
void FillExplicitDefaults(DumpTransportInfo &info) {
  info.MutableModel().SetModelName("");              // 0x0102, explicit
  info.MutableModel().SetModelId(0U);                // 0x0103, explicit
  info.MutableModel().SetStepIdAddr(0U);             // 0x0104, explicit
  info.MutableModel().SetIterationsPerLoopAddr(0U);  // 0x0105, explicit
  info.MutableModel().SetLoopCondAddr(0U);           // 0x0106, explicit
  // 对照组：implicit 字段 Set 成类型默认值，编码器跳过
  info.MutableModel().SetDumpPath("");                          // 0x0101
  info.MutableModel().SetFlag(0U);                              // 0x0107
  info.MutableModel().SetDumpData(DumpTransDumpData::kTensor);  // 0x0109，枚举值恰为 0
}

// 场景：MODEL 的 5 个 explicit 字段 Set 成 0/空串（implicit 同设默认值作对照）后序列化-解码往返。
// 未 Set 字段不上线由空 MODEL golden 用例（WhenNoModelAndTask）覆盖；explicit 机制在框架层已有
// 同构用例，此处只校验 dump schema 5 个 MODEL 字段的接线
TEST(DumpTransportInfoTest, DumpTransModelInfo_ShouldRoundTripExplicitDefaults_WhenSetToDefault) {
  DumpTransportInfo info;
  FillExplicitDefaults(info);
  Bytes encoded;
  ASSERT_EQ(info.Serialize(encoded), DumpTransStatus::kOk);
  // 证据一：比空 MODEL（16B 公共头 + 4B 空记录帧头）多出 24B = 空串 4B + 4×(4B 帧头 + 1B varint(0))；
  // 3 个 Set 成默认值的 implicit 字段贡献 0 字节
  EXPECT_EQ(encoded.size(), wire::kHeaderSize + wire::kRecordHeaderSize + 4U + 5U + 5U + 5U + 5U);
  // 证据二：MODEL(0x0001) 记录内线上 tag 仅 5 个 explicit 字段，不含 implicit 的 0x0101/0x0107/0x0109
  std::vector<uint16_t> model_tags;
  const auto scan = wire::ProcessRecords(
      encoded.data() + wire::kHeaderSize, encoded.size() - wire::kHeaderSize,
      [&model_tags](uint16_t tag, const uint8_t *payload, size_t payload_size) {
        if (tag != 0x0001U) {
          return wire::WireStatus::kOk;
        }
        return wire::ProcessRecords(payload, payload_size, [&model_tags](uint16_t field_tag, const uint8_t *, size_t) {
          model_tags.push_back(field_tag);
          return wire::WireStatus::kOk;
        });
      });
  ASSERT_EQ(scan, wire::WireStatus::kOk);
  const std::vector<uint16_t> expected_tags = {0x0102U, 0x0103U, 0x0104U, 0x0105U, 0x0106U};
  EXPECT_EQ(model_tags, expected_tags);
  // 解码回读：Set 过默认值的 explicit 字段 Has* 为 true + Get* 返回默认值；implicit 字段只比值
  DumpTransportInfo decoded;
  ASSERT_EQ(decoded.Deserialize(encoded.data(), encoded.size()), DumpTransStatus::kOk);
  EXPECT_TRUE(decoded.GetModel().HasModelName());
  EXPECT_EQ(decoded.GetModel().GetModelName(), "");
  EXPECT_TRUE(decoded.GetModel().HasModelId());
  EXPECT_EQ(decoded.GetModel().GetModelId(), 0U);
  EXPECT_TRUE(decoded.GetModel().HasStepIdAddr());
  EXPECT_EQ(decoded.GetModel().GetStepIdAddr(), 0U);
  EXPECT_TRUE(decoded.GetModel().HasIterationsPerLoopAddr());
  EXPECT_EQ(decoded.GetModel().GetIterationsPerLoopAddr(), 0U);
  EXPECT_TRUE(decoded.GetModel().HasLoopCondAddr());
  EXPECT_EQ(decoded.GetModel().GetLoopCondAddr(), 0U);
  EXPECT_EQ(decoded.GetModel().GetDumpPath(), "");
  EXPECT_EQ(decoded.GetModel().GetFlag(), 0U);
  EXPECT_EQ(decoded.GetModel().GetDumpData(), DumpTransDumpData::kTensor);
}

// 集合/子记录 tag：只有这些 tag 的 payload 内部仍是 TLV 帧结构，可继续下探；
// 标量字段的 payload 是值编码（varint/裸字节），不可再按帧切分
bool IsDumpContainerTag(uint16_t tag) {
  return (tag == 0x0001U) || (tag == 0x0002U) || ((tag >= 0x0010U) && (tag <= 0x0018U));
}

// 递归收集码流中所有层级实际出现的 tag
wire::WireStatus CollectWireTags(const uint8_t *data, size_t size, std::set<uint16_t> &tags) {
  return wire::ProcessRecords(data, size, [&tags](uint16_t tag, const uint8_t *payload, size_t payload_size) {
    tags.insert(tag);
    if (IsDumpContainerTag(tag)) {
      return CollectWireTags(payload, payload_size, tags);
    }
    return wire::WireStatus::kOk;
  });
}

// 场景：全字段填满后逐层扫描码流出现的 tag，与 schema 注册表双向比对，防止"新增字段忘记登记
// IsKnownTag 导致解码侧把它当未知 tag 静默跳过（数据静默丢失）"，以及"注册表残留已删字段"
TEST(DumpTransportInfoTest, DumpWireSchema_ShouldRegisterEveryEncodedTag_WhenAllRecordsFilled) {
  DumpTransportInfo info;
  FillAllFieldsInfo(info);
  Bytes wire;
  ASSERT_EQ(info.Serialize(wire), DumpTransStatus::kOk);
  std::set<uint16_t> encoded;
  ASSERT_EQ(CollectWireTags(wire.data() + wire::kHeaderSize, wire.size() - wire::kHeaderSize, encoded),
            wire::WireStatus::kOk);
  // schema 登记的全部 tag：遍历 u16 全域询问 IsKnownTag，避免再手工维护一份清单（清单本身就会漂移）
  std::set<uint16_t> registered;
  for (uint32_t tag = 0U; tag <= 0xFFFFU; ++tag) {
    if (DumpWireSchema::IsKnownTag(static_cast<uint16_t>(tag))) {
      registered.insert(static_cast<uint16_t>(tag));
    }
  }
  EXPECT_EQ(registered.size(), 63U);
  // 正向：码流里出现的每个 tag 都必须已登记
  for (const auto tag : encoded) {
    EXPECT_TRUE(DumpWireSchema::IsKnownTag(tag)) << "tag 未登记到 DumpWireSchema::IsKnownTag: " << tag;
  }
  // 反向：登记的每个 tag 都应能由某个字段产生（不一致即清单与注册表脱节）
  EXPECT_EQ(encoded, registered);
}

// ==================== BBIT 真机 4 用例：protobuf vs DumpWire v1 TLV 载荷体积对比 ====================
// 用例数据取自 protobuf 版本真机执行日志（docs/om2_protobuf_decoupling/BBIT_dump_case_protobuf_info.md
// case1~case4；dump 根目录/用例名/模型名已替换为等长示意串）。字段存在性与生产路径一致：
// loop_cond/iterations_per_loop 真机为 0 不设置，其余默认值字段两侧都不上线，对比的是同语义载荷。
// io 的 format 字段为 CANN 私有格式编码：0x5000000 基址 + 公共枚举序号（3=NC1HWC0、4=FRACTAL_Z），透传自真机日志。
// 汇总（golden 长度 + analyze_wire.py 逐字节归类，①~④合计与 diff 精确相符）：
//   case    proto    tlv    diff   tlv/proto  ①公共头  ②默认值  ③头开销差  ④值编码差
//   case1   1871B  2591B  +720B    ≈138%      +16B     +0B     +724B     -20B
//   case2   1614B  2005B  +391B    ≈124%      +16B     +0B     +387B     -12B
//   case3   2463B  3415B  +952B    ≈139%      +16B     +0B     +980B     -44B
//   case4   2456B  3408B  +952B    ≈139%      +16B     +0B     +980B     -44B
// 结论：TLV 大 24%~39%，几乎全部来自③（每字段 4B 定宽头 vs pb 1~3B tag+len）；④恒为小幅负值
// （Shape 以"count+zigzag 维度"内联，省 pb packed 子消息的 tag+len）；②恒为 0（explicit 与 pb
// oneof 严格对称，无缺省值多上线）。字符串占比越高相对开销越低；diff 随 io 字段数线性增长。
struct BbitIo {
  uint64_t addr = 0U;
  uint64_t size = 0U;
  int32_t format = 0;
  int32_t data_type = 0;
  uint32_t addr_type = 0U;
  std::vector<uint64_t> shape;
};

struct BbitTask {
  uint32_t task_id = 0U;
  uint32_t stream_id = 0U;
  uint32_t thread_id = 0U;
  std::string op_name;
  std::string op_type;
  std::vector<BbitIo> inputs;
  std::vector<BbitIo> outputs;
};

struct BbitCase {
  std::string scene;
  std::string dump_path;
  std::string model_name;
  uint32_t model_id = 0U;
  uint64_t step_id_addr = 0U;
  std::string dump_step;
  uint32_t dump_data = 0U;  // 0=tensor（默认值，两侧都不上线），1=stats（两侧都上线）
  std::vector<BbitTask> tasks;
};

BbitIo MakeBbitIo(uint64_t addr, uint64_t size, int32_t format, int32_t data_type, uint32_t addr_type,
                  std::vector<uint64_t> shape = {}) {
  return BbitIo{addr, size, format, data_type, addr_type, std::move(shape)};
}

BbitTask MakeBbitTask(uint32_t task_id, uint32_t stream_id, uint32_t thread_id, const char *op_name,
                      const char *op_type, std::vector<BbitIo> inputs = {}, std::vector<BbitIo> outputs = {}) {
  BbitTask task;
  task.task_id = task_id;
  task.stream_id = stream_id;
  task.thread_id = thread_id;
  task.op_name = op_name;
  task.op_type = op_type;
  task.inputs = std::move(inputs);
  task.outputs = std::move(outputs);
  return task;
}

// 公共 dump 根目录为脱敏示意路径：字节数与真机日志一致（78 字节），dump_path 上线长度不变，
// 保证 golden 体积与真机口径一致；model_id 4 个用例一致
constexpr char kBbitDumpRoot[] = "/npu/dumpuser0/Om2Dump-main/feature/GE/atc/SampleDevice01_atc_acl_support_om2/";
constexpr uint32_t kBbitModelId = 0x8000000aU;

template <typename PbIo>
void FillBbitIoProto(PbIo &io_pb, const BbitIo &io) {
  io_pb.set_data_type(io.data_type);
  io_pb.set_format(io.format);
  io_pb.set_address(io.addr);
  io_pb.set_size(io.size);
  io_pb.set_addr_type(static_cast<toolkit::aicpu::dump::AddressType>(io.addr_type));
  for (const auto dim : io.shape) {
    io_pb.mutable_shape()->add_dim(dim);
  }
}

template <typename TlvIo>
void FillBbitIoTlv(TlvIo &io_tlv, const BbitIo &io) {
  io_tlv.SetDataType(io.data_type);
  io_tlv.SetFormat(io.format);
  io_tlv.SetAddress(io.addr);
  io_tlv.SetSize(io.size);
  io_tlv.SetAddrType(static_cast<DumpTransAddressType>(io.addr_type));
  io_tlv.SetShape(io.shape);
}

void FillBbitProto(const BbitCase &c, toolkit::aicpu::dump::OpMappingInfo &pb) {
  pb.set_dump_path(c.dump_path);
  pb.set_model_name(c.model_name);
  pb.set_model_id(c.model_id);
  pb.set_step_id_addr(c.step_id_addr);
  pb.set_flag(1U);
  pb.set_dump_step(c.dump_step);
  pb.set_dump_data(static_cast<toolkit::aicpu::dump::DumpData>(c.dump_data));
  for (const auto &t : c.tasks) {
    auto *task = pb.add_task();
    task->set_task_id(t.task_id);
    task->set_stream_id(t.stream_id);
    task->set_thread_id(t.thread_id);
    task->mutable_op()->set_op_name(t.op_name);
    task->mutable_op()->set_op_type(t.op_type);
    for (const auto &io : t.inputs) {
      FillBbitIoProto(*task->add_input(), io);
    }
    for (const auto &io : t.outputs) {
      FillBbitIoProto(*task->add_output(), io);
    }
  }
}

void FillBbitTlv(const BbitCase &c, DumpTransportInfo &info) {
  auto &model = info.MutableModel();
  model.SetDumpPath(c.dump_path);
  model.SetModelName(c.model_name);
  model.SetModelId(c.model_id);
  model.SetStepIdAddr(c.step_id_addr);
  model.SetFlag(1U);
  model.SetDumpStep(c.dump_step);
  model.SetDumpData(static_cast<DumpTransDumpData>(c.dump_data));
  for (const auto &t : c.tasks) {
    auto &task = info.AddTask();
    task.SetTaskId(t.task_id);
    task.SetStreamId(t.stream_id);
    task.SetThreadId(t.thread_id);
    task.SetOpName(t.op_name);
    task.SetOpType(t.op_type);
    for (const auto &io : t.inputs) {
      FillBbitIoTlv(task.AddInput(), io);
    }
    for (const auto &io : t.outputs) {
      FillBbitIoTlv(task.AddOutput(), io);
    }
  }
}

void AppendBbitIoDesc(std::ostringstream &os, const char *kind, const std::vector<BbitIo> &entries) {
  os << "    " << kind << "(" << entries.size() << "):\n";
  for (size_t i = 0U; i < entries.size(); ++i) {
    const auto &io = entries[i];
    os << "      [" << i << "] addr=0x" << std::hex << io.addr << std::dec << " size=" << io.size
       << " format=" << io.format << " data_type=" << io.data_type << " addr_type=" << io.addr_type << " shape=[";
    for (size_t dim = 0U; dim < io.shape.size(); ++dim) {
      os << (dim == 0U ? "" : ",") << io.shape[dim];
    }
    os << "]\n";
  }
}

void SaveBbitCaseArtifacts(const BbitCase &c, const WireSizes &sizes, const std::string &pb_bytes,
                           const Bytes &tlv_bytes) {
  const std::string base = WireAnalysisDir() + "/bbit-" + c.scene;
  std::ostringstream os;
  os << "scene: bbit-" << c.scene << " (source: BBIT_dump_case_protobuf_info.md, real device log)\n";
  os << "model: dump_path=" << c.dump_path << " model_name=" << c.model_name << " model_id=" << c.model_id
     << " step_id_addr=0x" << std::hex << c.step_id_addr << std::dec << " flag=1 dump_step=" << c.dump_step
     << " dump_data=" << c.dump_data << "\n";
  os << "task_num=" << c.tasks.size() << "\n";
  for (const auto &t : c.tasks) {
    os << "  task: task_id=" << t.task_id << " stream_id=" << t.stream_id << " thread_id=" << t.thread_id
       << " op_name=" << t.op_name << " op_type=" << t.op_type << "\n";
    AppendBbitIoDesc(os, "inputs", t.inputs);
    AppendBbitIoDesc(os, "outputs", t.outputs);
  }
  os << "payload_size: proto=" << sizes.proto << "B tlv=" << sizes.tlv
     << "B diff(tlv-proto)=" << static_cast<int64_t>(sizes.tlv) - static_cast<int64_t>(sizes.proto) << "B\n";
  std::ofstream input(base + ".input.txt", std::ios::trunc);
  input << os.str();
  SavePayloadFiles(base + ".proto", Bytes(pb_bytes.begin(), pb_bytes.end()));
  SavePayloadFiles(base + ".tlv", tlv_bytes);
}

// 同一份真机输入分别填充 protobuf 与 TLV 并序列化：回读逐字段交叉校验后对比体积，
// 产物落盘供 ProtobufDecoup/analyze_wire.py 离线逐字段归类
void RunBbitCase(const BbitCase &c, WireSizes &sizes) {
  toolkit::aicpu::dump::OpMappingInfo pb;
  FillBbitProto(c, pb);
  std::string pb_bytes;
  ASSERT_TRUE(pb.SerializeToString(&pb_bytes));
  DumpTransportInfo info;
  FillBbitTlv(c, info);
  Bytes tlv_bytes;
  ASSERT_EQ(info.Serialize(tlv_bytes), DumpTransStatus::kOk);
  // 两侧回读后逐字段交叉校验（presence 与非默认值语义均须一致）
  toolkit::aicpu::dump::OpMappingInfo pb_read;
  ASSERT_TRUE(pb_read.ParseFromString(pb_bytes));
  DumpTransportInfo tlv_read;
  ASSERT_EQ(tlv_read.Deserialize(tlv_bytes.data(), tlv_bytes.size()), DumpTransStatus::kOk);
  ExpectWireModelConsistent(pb_read, tlv_read.GetModel());
  EXPECT_EQ(pb_read.step_id_addr(), tlv_read.GetModel().GetStepIdAddr());
  ASSERT_EQ(pb_read.task_size(), static_cast<int>(c.tasks.size()));
  ASSERT_EQ(tlv_read.GetTaskCount(), c.tasks.size());
  for (size_t i = 0U; i < c.tasks.size(); ++i) {
    const DumpTransTaskInfo *task = nullptr;
    ASSERT_EQ(tlv_read.GetTask(i, task), DumpTransStatus::kOk);
    ExpectWireTaskConsistent(pb_read.task(static_cast<int>(i)), *task);
  }
  sizes.proto = pb_bytes.size();
  sizes.tlv = tlv_bytes.size();
  SaveBbitCaseArtifacts(c, sizes, pb_bytes, tlv_bytes);
  std::cout << "[size] bbit-" << c.scene << ": tasks=" << c.tasks.size() << " proto=" << sizes.proto
            << "B tlv=" << sizes.tlv << "B diff(tlv-proto)=+" << (sizes.tlv - sizes.proto)
            << "B tlv/proto=" << (sizes.tlv * 100U / sizes.proto) << "% per-task-diff=+"
            << (sizes.tlv - sizes.proto) / c.tasks.size() << "B artifacts=" << WireAnalysisDir() << std::endl;
  // 同语义载荷下 TLV 恒大于 proto，差异构成见文首汇总表
  EXPECT_GT(sizes.tlv, sizes.proto);
}

// case1 前 10 个任务：卷积/归约/格式转换前段（含 1 个无 io 的 TfSessionTask 与 1 个 FunctionOp）
std::vector<BbitTask> BuildBbitCase1HeadTasks(uint32_t thread_id) {
  return {MakeBbitTask(0U, 59U, thread_id,
                       "convolutionfunction_graph_0/convolution"
                       "convolutionfunction_graph_0/convolution_pass.1"
                       "convolutionfunction_graph_0/convolution_pass.0",
                       "Conv2D",
                       {MakeBbitIo(0x12c0c00211c0UL, 18816U, 1, 2, 0U, {4U, 28U, 28U, 3U}),
                        MakeBbitIo(0x12c0c00211c8UL, 512U, 0x5000004, 2, 0U, {1U, 1U, 16U, 16U})}),
          MakeBbitTask(1U, 59U, thread_id, "MaxSelu", "ReduceMaxD",
                       {MakeBbitIo(0x12c0c0021000UL, 100352U, 0x5000003, 2, 0U, {4U, 1U, 28U, 28U, 16U})}),
          MakeBbitTask(2U, 59U, thread_id, "convolution_1", "Conv2D",
                       {MakeBbitIo(0x12c0c0021010UL, 100352U, 0x5000003, 2, 0U, {4U, 1U, 28U, 28U, 16U}),
                        MakeBbitIo(0x12c0c0021018UL, 512U, 0x5000004, 2, 0U, {1U, 1U, 16U, 16U})}),
          MakeBbitTask(3U, 59U, thread_id, "SqrtAdd", "AutomaticBufferFusionOp",
                       {MakeBbitIo(0x12c0c0021028UL, 100352U, 0x5000003, 2, 0U, {4U, 1U, 28U, 28U, 16U}),
                        MakeBbitIo(0x12c0c0021030UL, 25088U, 0x5000003, 2, 0U, {1U, 1U, 28U, 28U, 16U})}),
          MakeBbitTask(0U, 0U, thread_id, "TfSessionTask", "TfSessionTask"),
          MakeBbitTask(1U, 58U, thread_id, "StackV2", "FunctionOp",
                       {MakeBbitIo(0x12c0c00211e0UL, 4U, 1, 7, 0U),
                        MakeBbitIo(0x12c0c00211e8UL, 18816U, 1, 2, 0U, {4U, 28U, 28U, 3U})}),
          MakeBbitTask(6U, 59U, thread_id, "IsFinite", "IsFinite",
                       {MakeBbitIo(0x12c0c0021040UL, 18816U, 1, 2, 0U, {4U, 28U, 28U, 3U})}),
          MakeBbitTask(7U, 59U, thread_id, "Cast", "Cast",
                       {MakeBbitIo(0x12c0c0021060UL, 9408U, 1, 11, 0U, {4U, 28U, 28U, 3U})}),
          MakeBbitTask(8U, 59U, thread_id, "trans_TransData_8", "TransData",
                       {MakeBbitIo(0x12c0c0021070UL, 18816U, 1, 2, 0U, {4U, 28U, 28U, 3U})}),
          MakeBbitTask(9U, 59U, thread_id, "trans_TransData_15", "TransData",
                       {MakeBbitIo(0x12c0c0021200UL, 200704U, 0x5000003, 2, 0U, {8U, 1U, 28U, 28U, 16U})})};
}

// case1 后 5 个任务：DSA 随机数任务（两个任务共用同一份 5 输入 raw 地址表）与 dropout/random_normal 融合段
std::vector<BbitTask> BuildBbitCase1TailTasks(uint32_t thread_id) {
  const std::vector<BbitIo> dsa_inputs = {
      MakeBbitIo(0x12c0c0017200UL, 8U, 2, 8, 2U, {1U}), MakeBbitIo(0x12c0c0018000UL, 8U, 2, 10, 2U),
      MakeBbitIo(0x12c0c0017600UL, 2U, 2, 2, 2U), MakeBbitIo(0x12c0c0017400UL, 2U, 2, 2, 2U),
      MakeBbitIo(0x12c0c0018200UL, 16U, 2, 10, 2U, {2U})};
  return {MakeBbitTask(0U, 57U, thread_id, "dropout/random_uniform/RandomUniform/DSASTATELESSRANDOMUNIFOTM",
                       "DSAStatelessRandomUniform", dsa_inputs),
          MakeBbitTask(12U, 59U, thread_id, "dropout/random_uniform/muldropout/random_uniformdropout/GreaterEqual",
                       "AutomaticBufferFusionOp",
                       {MakeBbitIo(0x12c0c00210e0UL, 18816U, 2, 2, 0U, {4U, 28U, 28U, 3U}),
                        MakeBbitIo(0x12c0c00210e8UL, 2U, 2, 2, 0U), MakeBbitIo(0x12c0c00210f0UL, 2U, 2, 2, 0U),
                        MakeBbitIo(0x12c0c00210f8UL, 2U, 2, 2, 0U)}),
          MakeBbitTask(13U, 59U, thread_id, "dropout/Cast", "Cast",
                       {MakeBbitIo(0x12c0c0021108UL, 9408U, 2, 11, 0U, {4U, 28U, 28U, 3U})}),
          MakeBbitTask(2U, 57U, thread_id, "random_normal/RandomStandardNormal/DSASTATELESSRANDOMNORMAL",
                       "DSAStatelessRandomNormal", dsa_inputs),
          MakeBbitTask(16U, 59U, thread_id, "random_normal/mulrandom_normalAbsdropout/muldropout/mul_1Relu_2",
                       "AutomaticBufferFusionOp",
                       {MakeBbitIo(0x12c0c0021210UL, 18816U, 2, 2, 0U, {4U, 28U, 28U, 3U}),
                        MakeBbitIo(0x12c0c0021218UL, 2U, 2, 2, 0U), MakeBbitIo(0x12c0c0021220UL, 2U, 2, 2, 0U),
                        MakeBbitIo(0x12c0c0021228UL, 2U, 2, 2, 0U),
                        MakeBbitIo(0x12c0c0021230UL, 18816U, 2, 2, 0U, {4U, 28U, 28U, 3U})})};
}

// case1：15 任务、纯输入侧 dump（dump_mode=input）、dump_data=tensor、dump_step 为空
BbitCase BuildBbitCase1() {
  BbitCase c;
  c.scene = "case1";
  c.dump_path = std::string(kBbitDumpRoot) + "ut_sample_model_dump_case_name_0002/om2_dump/20261016145036/0/";
  c.model_name = "ut_sample_model_dump_case_name_0002";
  c.model_id = kBbitModelId;
  c.step_id_addr = 0x12c0c002b000UL;
  c.dump_step = "";
  c.dump_data = 0U;
  c.tasks = BuildBbitCase1HeadTasks(470530U);
  const auto tail = BuildBbitCase1TailTasks(470530U);
  c.tasks.insert(c.tasks.end(), tail.begin(), tail.end());
  return c;
}

// case2：13 任务、纯输出侧 dump、stats 且 dump_step 非空，唯一命中 While 循环体，模型级字段上线最全
BbitCase BuildBbitCase2() {
  BbitCase c;
  c.scene = "case2";
  c.dump_path = std::string(kBbitDumpRoot) + "ut_sample_model_dump_case_name_0003/om2_dump/20261016144643/0/";
  c.model_name = "ut_sample_model_dump_case_name_0003";
  c.model_id = kBbitModelId;
  c.step_id_addr = 0x12c0c0075000UL;
  c.dump_step = "0|3-5|10";
  c.dump_data = 1U;
  constexpr uint32_t kThreadId = 463156U;
  c.tasks = {
      MakeBbitTask(0U, 59U, kThreadId, "PowXdivy", "AutomaticBufferFusionOp", {},
                   {MakeBbitIo(0x12c0c0073118UL, 28672U, 1, 2, 0U, {8U, 32U, 7U, 8U})}),
      MakeBbitTask(1U, 59U, kThreadId, "trans_TransData_0", "TransData", {},
                   {MakeBbitIo(0x12c0c0073008UL, 57344U, 0x5000003, 2, 0U, {8U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(10U, 59U, kThreadId, "trans_TransData_4", "TransData", {},
                   {MakeBbitIo(0x12c0c0073028UL, 50176U, 0x5000003, 2, 0U, {7U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(11U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/If0subgraph_fA5Xdbohjuw3/Mean", "ReduceMeanD",
                   {}, {MakeBbitIo(0x12c0c0073038UL, 7168U, 0x5000003, 2, 0U, {1U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(12U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/If0subgraph_fA5Xdbohjuw3/Add", "Add", {},
                   {MakeBbitIo(0x12c0c0073050UL, 57344U, 0x5000003, 2, 0U, {8U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(13U, 59U, kThreadId, "trans_TransData_12", "TransData", {},
                   {MakeBbitIo(0x12c0c0073060UL, 28672U, 1, 2, 0U, {8U, 32U, 7U, 8U})}),
      MakeBbitTask(14U, 59U, kThreadId,
                   "While1while_body_MNZ8FGiKS780/If0subgraph_fA5Xdbohjuw3/Cos"
                   "While1while_body_MNZ8FGiKS780/If0subgraph_fA5Xdbohjuw3/Rsqrt"
                   "While1while_body_MNZ8FGiKS780/If0subgraph_fA5Xdbohjuw3/Sub",
                   "AutomaticBufferFusionOp", {}, {MakeBbitIo(0x12c0c0073170UL, 28672U, 1, 2, 0U, {8U, 32U, 7U, 8U})}),
      MakeBbitTask(15U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/packed_0", "Pack", {},
                   {MakeBbitIo(0x12c0c0073070UL, 28672U, 1, 2, 0U, {1U, 8U, 32U, 7U, 8U})}),
      MakeBbitTask(16U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/Mean", "ReduceMeanD", {},
                   {MakeBbitIo(0x12c0c0073080UL, 7168U, 0x5000003, 2, 0U, {1U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(17U, 59U, kThreadId,
                   "While1while_body_MNZ8FGiKS780/convolutionfunction_graph_0/"
                   "While1while_body_MNZ8FGiKS780/convolution"
                   "While1while_body_MNZ8FGiKS780/convolutionfunction_graph_0/"
                   "While1while_body_MNZ8FGiKS780/convolution_pass.1",
                   "Conv2D", {}, {MakeBbitIo(0x12c0c0073188UL, 7168U, 0x5000003, 2, 0U, {1U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(18U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/Sub", "Sub", {},
                   {MakeBbitIo(0x12c0c0073098UL, 57344U, 0x5000003, 2, 0U, {8U, 1U, 32U, 7U, 16U})}),
      MakeBbitTask(19U, 59U, kThreadId, "While1while_body_MNZ8FGiKS780/sub_1_0", "Sub", {},
                   {MakeBbitIo(0x12c0c00731a0UL, 4U, 2, 7, 0U)}),
      MakeBbitTask(26U, 59U, kThreadId, "trans_TransData_1", "TransData", {},
                   {MakeBbitIo(0x12c0c00731e0UL, 28672U, 1, 2, 0U, {8U, 32U, 7U, 8U})})};
  return c;
}

// case3/4 前 11 个任务：Cast/TransData/Mul 前段 + 无 io 的 TfSessionTask + 副流 MatrixInverse + Conv/Sum
std::vector<BbitTask> BuildBbitCase34HeadTasks(uint32_t thread_id, uint32_t main_stream,
                                               uint32_t matrix_inverse_stream) {
  return {MakeBbitTask(0U, main_stream, thread_id, "trans_Cast_0", "Cast",
                       {MakeBbitIo(0x12c0c001b140UL, 409600U, 1, 1, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b148UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})}),
          MakeBbitTask(1U, main_stream, thread_id, "trans_TransData_1", "TransData",
                       {MakeBbitIo(0x12c0c001b000UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b008UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})}),
          MakeBbitTask(2U, main_stream, thread_id, "trans_Cast_2", "Cast",
                       {MakeBbitIo(0x12c0c001b150UL, 409600U, 1, 1, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b158UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})}),
          MakeBbitTask(3U, main_stream, thread_id, "trans_TransData_3", "TransData",
                       {MakeBbitIo(0x12c0c001b010UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b018UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})}),
          MakeBbitTask(4U, main_stream, thread_id, "Mul", "Mul",
                       {MakeBbitIo(0x12c0c001b020UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U}),
                        MakeBbitIo(0x12c0c001b028UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b030UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})}),
          MakeBbitTask(0U, 0U, thread_id, "TfSessionTask", "TfSessionTask"),
          MakeBbitTask(1U, matrix_inverse_stream, thread_id, "MatrixInverse", "MatrixInverse",
                       {MakeBbitIo(0x12c0c001b160UL, 409600U, 1, 1, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b168UL, 409600U, 1, 1, 0U, {8U, 50U, 16U, 16U})}),
          MakeBbitTask(7U, main_stream, thread_id, "trans_Cast_4", "Cast",
                       {MakeBbitIo(0x12c0c001b038UL, 409600U, 1, 1, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b040UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})}),
          MakeBbitTask(8U, main_stream, thread_id, "Mean", "ReduceMeanD",
                       {MakeBbitIo(0x12c0c001b048UL, 204800U, 1, 2, 0U, {8U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b050UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U})}),
          MakeBbitTask(9U, main_stream, thread_id,
                       "convolutionfunction_graph_0/convolution"
                       "convolutionfunction_graph_0/convolution_pass.1",
                       "Conv2D",
                       {MakeBbitIo(0x12c0c001b058UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U}),
                        MakeBbitIo(0x12c0c001b060UL, 2048U, 0x5000004, 2, 0U, {4U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b068UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})}),
          MakeBbitTask(10U, main_stream, thread_id, "Sum", "ReduceSumD",
                       {MakeBbitIo(0x12c0c001b070UL, 204800U, 0x5000003, 2, 0U, {8U, 1U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b078UL, 25600U, 0x5000003, 2, 0U, {1U, 1U, 50U, 16U, 16U})})};
}

// case3/4 后 10 个任务：If 分支子图（Conv/Min/Mean/Abs）、Pack/Concat 段与收尾 Cast
std::vector<BbitTask> BuildBbitCase34TailTasks(uint32_t thread_id, uint32_t main_stream) {
  return {MakeBbitTask(11U, main_stream, thread_id,
                       "If_10subgraph2_3grthxrtEOE3/convolutionfunction_graph_1/"
                       "If_10subgraph2_3grthxrtEOE3/convolution"
                       "If_10subgraph2_3grthxrtEOE3/convolutionfunction_graph_1/"
                       "If_10subgraph2_3grthxrtEOE3/convolution_pass.1",
                       "Conv2D",
                       {MakeBbitIo(0x12c0c001b080UL, 25600U, 0x5000003, 2, 0U, {1U, 1U, 50U, 16U, 16U}),
                        MakeBbitIo(0x12c0c001b088UL, 2048U, 0x5000004, 2, 0U, {4U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b090UL, 25600U, 0x5000003, 2, 0U, {1U, 1U, 50U, 16U, 16U})}),
          MakeBbitTask(12U, main_stream, thread_id, "If_10subgraph2_3grthxrtEOE3/Min", "ReduceMinD",
                       {MakeBbitIo(0x12c0c001b098UL, 25600U, 0x5000003, 2, 0U, {1U, 1U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0a0UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})}),
          MakeBbitTask(13U, main_stream, thread_id, "trans_TransData_11", "TransData",
                       {MakeBbitIo(0x12c0c001b0a8UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0b0UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U})}),
          MakeBbitTask(14U, main_stream, thread_id, "Concat/values_1", "Pack",
                       {MakeBbitIo(0x12c0c001b0b8UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0c0UL, 512U, 2, 2, 0U, {1U, 1U, 1U, 16U, 16U})}),
          MakeBbitTask(15U, main_stream, thread_id, "If1subgraph1_V6XzximAq6I0/Mean", "ReduceMeanD",
                       {MakeBbitIo(0x12c0c001b0c8UL, 25600U, 0x5000003, 2, 0U, {1U, 1U, 50U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0d0UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})}),
          MakeBbitTask(16U, main_stream, thread_id, "If1subgraph1_V6XzximAq6I0/Abs", "Abs",
                       {MakeBbitIo(0x12c0c001b0d8UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0e0UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})}),
          MakeBbitTask(17U, main_stream, thread_id, "trans_TransData_14", "TransData",
                       {MakeBbitIo(0x12c0c001b0e8UL, 512U, 0x5000003, 2, 0U, {1U, 1U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b0f0UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U})}),
          MakeBbitTask(18U, main_stream, thread_id, "Concat/values_0", "Pack",
                       {MakeBbitIo(0x12c0c001b0f8UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b100UL, 512U, 2, 2, 0U, {1U, 1U, 1U, 16U, 16U})}),
          MakeBbitTask(19U, main_stream, thread_id, "Concat_1", "ConcatD",
                       {MakeBbitIo(0x12c0c001b108UL, 512U, 1, 2, 0U, {1U, 1U, 16U, 16U}),
                        MakeBbitIo(0x12c0c001b110UL, 1024U, 1, 2, 0U, {1U, 2U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b118UL, 1536U, 1, 2, 0U, {1U, 3U, 16U, 16U})}),
          MakeBbitTask(20U, main_stream, thread_id, "trans_Cast_15", "Cast",
                       {MakeBbitIo(0x12c0c001b178UL, 1536U, 1, 2, 0U, {1U, 3U, 16U, 16U})},
                       {MakeBbitIo(0x12c0c001b180UL, 3072U, 1, 1, 0U, {1U, 3U, 16U, 16U})})};
}

// case3 与 case4：同构的 21 任务全 io dump，仅模型名/dump 目录/thread_id/主副流编号不同，4 例中载荷最大的两档
BbitCase BuildBbitCase3LikeCase4(const char *scene, const char *case_dir, const char *model_name, uint32_t thread_id,
                                 uint32_t main_stream, uint32_t matrix_inverse_stream) {
  BbitCase c;
  c.scene = scene;
  c.dump_path = std::string(kBbitDumpRoot) + case_dir;
  c.model_name = model_name;
  c.model_id = kBbitModelId;
  c.step_id_addr = 0x12c0c0025000UL;
  c.dump_step = "";
  c.dump_data = 0U;
  c.tasks = BuildBbitCase34HeadTasks(thread_id, main_stream, matrix_inverse_stream);
  const auto tail = BuildBbitCase34TailTasks(thread_id, main_stream);
  c.tasks.insert(c.tasks.end(), tail.begin(), tail.end());
  return c;
}

// 场景：BBIT 真机 case1（15 任务、33 个输入、无输出），体积对比。golden：proto=1871B tlv=2591B diff=+720B。
// 归类：③+724B（33 输入×4 标量字段 +396B 为大头）、④-20B（20 处 Shape）；长 op_name 使 pb Op 子消息前缀升 2B
TEST(Om2DumpWireBbitTest, DumpPayload_ShouldCompareProtoAndTlvSizes_WhenBbitCase1InputOnlyDump) {
  const BbitCase c = BuildBbitCase1();
  ASSERT_EQ(c.tasks.size(), 15U);
  WireSizes sizes{};
  RunBbitCase(c, sizes);
  EXPECT_EQ(sizes.proto, 1871U);
  EXPECT_EQ(sizes.tlv, 2591U);
  EXPECT_EQ(sizes.tlv - sizes.proto, 720U);
}

// 场景：BBIT 真机 case2（13 任务每任务 1 输出、stats+dump_step 非空），体积对比。golden：proto=1614B
// tlv=2005B diff=+391B，占比 4 例最低（≈124%）：io 字段少、长 op_name 字符串占比高摊薄定宽头劣势。
// 归类：③+387B、④-12B；dump_data/dump_step 两侧对称上线不构成差异
TEST(Om2DumpWireBbitTest, DumpPayload_ShouldCompareProtoAndTlvSizes_WhenBbitCase2OutputStatsDump) {
  const BbitCase c = BuildBbitCase2();
  ASSERT_EQ(c.tasks.size(), 13U);
  WireSizes sizes{};
  RunBbitCase(c, sizes);
  EXPECT_EQ(sizes.proto, 1614U);
  EXPECT_EQ(sizes.tlv, 2005U);
  EXPECT_EQ(sizes.tlv - sizes.proto, 391U);
}

// 场景：BBIT 真机 case3（21 任务、24 输入+20 输出全 io、双流 60/61），体积对比。golden：proto=2463B
// tlv=3415B diff=+952B，4 例中载荷最大；io 字段翻倍后 diff 近翻倍（vs case1），随字段数线性增长。③+980B ④-44B
TEST(Om2DumpWireBbitTest, DumpPayload_ShouldCompareProtoAndTlvSizes_WhenBbitCase3FullIoDump) {
  const BbitCase c = BuildBbitCase3LikeCase4("case3", "ut_sample_model_dump_case_name_0004/om2_dump/20261016144154/0/",
                                             "ut_sample_model_dump_case_name_0004_model2", 446792U, 60U, 61U);
  ASSERT_EQ(c.tasks.size(), 21U);
  WireSizes sizes{};
  RunBbitCase(c, sizes);
  EXPECT_EQ(sizes.proto, 2463U);
  EXPECT_EQ(sizes.tlv, 3415U);
  EXPECT_EQ(sizes.tlv - sizes.proto, 952U);
}

// 场景：BBIT 真机 case4（与 case3 同构、主副流对调 61/60），体积对比。golden：proto=2456B tlv=3408B
// diff=+952B。比 case3 各小 7B = model_name 短 7 字符，字符串增量两格式相同、逐字节平移；归类与 case3
// 一致（③+980B ④-44B），验证同构输入下差异归类的稳定性
TEST(Om2DumpWireBbitTest, DumpPayload_ShouldCompareProtoAndTlvSizes_WhenBbitCase4FullIoDumpVariant) {
  const BbitCase c = BuildBbitCase3LikeCase4("case4", "ut_sample_model_dump_case_name_0005/om2_dump/20261016144358/0/",
                                             "ut_sample_model_dump_case_name_0005", 452163U, 61U, 60U);
  ASSERT_EQ(c.tasks.size(), 21U);
  WireSizes sizes{};
  RunBbitCase(c, sizes);
  EXPECT_EQ(sizes.proto, 2456U);
  EXPECT_EQ(sizes.tlv, 3408U);
  EXPECT_EQ(sizes.tlv - sizes.proto, 952U);
}

}  // namespace
}  // namespace dump
}  // namespace ge
