/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// OM2 dump 载荷的 DumpWire v1 **schema**：本文件只承载协议身份（magic/version）、tag 注册表、
// 记录对象（tag ↔ 字段的 RegisterMembers 清单）与顶层 MODEL/TASK 分派。线格式机制（帧切分、小端/varint/zigzag
// 原语、presence 语义与编解码器、公共头 helper）在通用框架
// `inc/framework/common/wire/wire_tlv.h` 中，其它模块可直接复用该框架并自带 schema。
#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DUMP_TRANSPORT_INFO_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DUMP_TRANSPORT_INFO_H_

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "framework/common/wire/wire_tlv.h"

namespace ge {
namespace dump {
// 错误码沿用通用框架定义；保留 DumpTransStatus 别名以兼容既有调用点与用例
using DumpTransStatus = wire::WireStatus;
// 记录类宏（WIRE_FIELD/WIRE_FIELD_COLLECTION）内部按非限定名引用这两个符号，在 schema 命名空间引入
using wire::WireStatus;
using wire::WireTlvField;

enum class DumpTransDumpData : uint32_t { kTensor = 0, kStats = 1 };
enum class DumpTransTaskType : uint32_t { kAiCore = 0, kAiCpu = 1, kDebug = 2, kSdma = 3, kFftsPlus = 4, kDsa = 5 };
enum class DumpTransAddressType : uint32_t {
  kTraditional = 0,
  kNoTiling = 1,
  kRaw = 2,
  kNanoIo = 3,
  kNanoWeight = 4,
  kNanoWork = 5
};
enum class DumpTransWorkspaceType : uint32_t { kLog = 0 };
enum class DumpTransBufferType : uint32_t { kL1 = 0 };

// DumpWire v1 的 schema 策略：向通用框架注入协议身份与 tag 全集（编译期，零运行开销）
struct DumpWireSchema {
  static constexpr uint32_t kMagic = 0x564C5401U;
  static constexpr uint16_t kVersion = 1U;

  static bool IsKnownTag(uint16_t tag) {
    switch (tag) {
      case 0x0001U:
      case 0x0002U:
      case 0x0010U:
      case 0x0011U:
      case 0x0012U:
      case 0x0013U:
      case 0x0014U:
      case 0x0015U:
      case 0x0016U:
      case 0x0017U:
      case 0x0018U:
      case 0x0101U:
      case 0x0102U:
      case 0x0103U:
      case 0x0104U:
      case 0x0105U:
      case 0x0106U:
      case 0x0107U:
      case 0x0108U:
      case 0x0109U:
      case 0x010aU:
      case 0x0201U:
      case 0x0202U:
      case 0x0203U:
      case 0x0204U:
      case 0x0205U:
      case 0x0206U:
      case 0x0207U:
      case 0x0208U:
      case 0x1001U:
      case 0x1002U:
      case 0x1003U:
      case 0x1004U:
      case 0x1005U:
      case 0x1006U:
      case 0x1007U:
      case 0x1008U:
      case 0x1101U:
      case 0x1102U:
      case 0x1103U:
      case 0x1104U:
      case 0x1105U:
      case 0x1106U:
      case 0x1107U:
      case 0x1108U:
      case 0x1109U:
      case 0x110aU:
      case 0x110bU:
      case 0x110cU:
      case 0x1201U:
      case 0x1202U:
      case 0x1203U:
      case 0x1301U:
      case 0x1302U:
      case 0x1303U:
      case 0x1401U:
      case 0x1402U:
      case 0x1501U:
      case 0x1502U:
      case 0x1601U:
      case 0x1602U:
      case 0x1801U:
      case 0x1802U:
        return true;
      default:
        return false;
    }
  }
};

// 兼容别名层：只保留 schema 记录类 friend 声明需要的跳板类型与协议身份常量；
// 其余框架符号（编解码器、原语）请直接按 ge::wire:: 路径引用，避免通用名被搬进 dump 命名空间
namespace dump_wire_detail {
using wire::MemberRegHelper;
constexpr uint32_t kMagic = DumpWireSchema::kMagic;
constexpr uint16_t kVersion = DumpWireSchema::kVersion;
inline bool IsKnownTag(uint16_t tag) {
  return DumpWireSchema::IsKnownTag(tag);
}
}  // namespace dump_wire_detail

// schema 内的字段/集合宏桥接到通用框架宏（末尾 #undef，不外泄宏名）
// 单值字段默认 implicit presence（proto3 plain 标量语义，默认值不上线）；
// 需要消费端区分“未下发”与“下发了默认值”的字段才用 EXPLICIT（protobuf 单成员 oneof 语义）
#define DUMP_TRANS_FIELD(Type, Name) WIRE_FIELD(Type, Name)
#define DUMP_TRANS_FIELD_EXPLICIT(Type, Name) WIRE_FIELD_EXPLICIT(Type, Name)
#define DUMP_TRANS_FIELD_COLLECTION(Type, Name) WIRE_FIELD_COLLECTION(Type, Name)

class DumpTransTensorInfo {
  DUMP_TRANS_FIELD(int32_t, DataType)
  DUMP_TRANS_FIELD(int32_t, Format)
  DUMP_TRANS_FIELD(std::vector<uint64_t>, Shape)
  DUMP_TRANS_FIELD(uint64_t, Address)
  DUMP_TRANS_FIELD(uint64_t, Size)
  DUMP_TRANS_FIELD(std::vector<uint64_t>, OriginShape)
  DUMP_TRANS_FIELD(DumpTransAddressType, AddrType)
  DUMP_TRANS_FIELD(uint64_t, Offset)
};

class DumpTransModelInfo {
  DUMP_TRANS_FIELD(std::string, DumpPath)
  // 以下 5 个字段用显式 presence，与 op_mapping.proto 中包在单成员 oneof 里的同名字段一一对应：
  // 消费端（AICPU dump kernel）按“字段是否下发”决定 has* 标志，进而影响落盘目录层级，
  // 因此 GE 侧即使取值为 0 / 空串也必须上线；其余字段沿用 proto3 plain 标量的 implicit presence。
  DUMP_TRANS_FIELD_EXPLICIT(std::string, ModelName)
  DUMP_TRANS_FIELD_EXPLICIT(uint32_t, ModelId)
  DUMP_TRANS_FIELD_EXPLICIT(uint64_t, StepIdAddr)
  DUMP_TRANS_FIELD_EXPLICIT(uint64_t, IterationsPerLoopAddr)
  DUMP_TRANS_FIELD_EXPLICIT(uint64_t, LoopCondAddr)
  DUMP_TRANS_FIELD(uint32_t, Flag)
  DUMP_TRANS_FIELD(std::string, DumpStep)
  DUMP_TRANS_FIELD(DumpTransDumpData, DumpData)
  DUMP_TRANS_FIELD(uint64_t, DumpSwitchAddr)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x0101U, DumpPath_);
    visitor.VisitMember(0x0102U, ModelName_);
    visitor.VisitMember(0x0103U, ModelId_);
    visitor.VisitMember(0x0104U, StepIdAddr_);
    visitor.VisitMember(0x0105U, IterationsPerLoopAddr_);
    visitor.VisitMember(0x0106U, LoopCondAddr_);
    visitor.VisitMember(0x0107U, Flag_);
    visitor.VisitMember(0x0108U, DumpStep_);
    visitor.VisitMember(0x0109U, DumpData_);
    visitor.VisitMember(0x010aU, DumpSwitchAddr_);
  }
};

class DumpTransDimRangeInfo {
  DUMP_TRANS_FIELD(uint64_t, DimStart)
  DUMP_TRANS_FIELD(uint64_t, DimEnd)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1801U, DimStart_);
    visitor.VisitMember(0x1802U, DimEnd_);
  }
};

class DumpTransInputInfo : public DumpTransTensorInfo {
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1001U, DataType_);
    visitor.VisitMember(0x1002U, Format_);
    visitor.VisitMember(0x1003U, Shape_);
    visitor.VisitMember(0x1004U, Address_);
    visitor.VisitMember(0x1005U, Size_);
    visitor.VisitMember(0x1006U, OriginShape_);
    visitor.VisitMember(0x1007U, AddrType_);
    visitor.VisitMember(0x1008U, Offset_);
  }
};

class DumpTransOutputInfo : public DumpTransTensorInfo {
  DUMP_TRANS_FIELD(std::string, OriginalName)
  DUMP_TRANS_FIELD(int32_t, OriginalOutputIndex)
  DUMP_TRANS_FIELD(int32_t, OriginalOutputDataType)
  DUMP_TRANS_FIELD(int32_t, OriginalOutputFormat)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransDimRangeInfo, DimRange)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1101U, DataType_);
    visitor.VisitMember(0x1102U, Format_);
    visitor.VisitMember(0x1103U, Shape_);
    visitor.VisitMember(0x1104U, Address_);
    visitor.VisitMember(0x1105U, OriginalName_);
    visitor.VisitMember(0x1106U, OriginalOutputIndex_);
    visitor.VisitMember(0x1107U, OriginalOutputDataType_);
    visitor.VisitMember(0x1108U, OriginalOutputFormat_);
    visitor.VisitMember(0x1109U, Size_);
    visitor.VisitMember(0x110aU, OriginShape_);
    visitor.VisitMember(0x110bU, AddrType_);
    visitor.VisitMember(0x110cU, Offset_);
    visitor.VisitMember(0x0018U, DimRange_);
  }
};

class DumpTransWorkspaceInfo {
  DUMP_TRANS_FIELD(DumpTransWorkspaceType, Type)
  DUMP_TRANS_FIELD(uint64_t, DataAddr)
  DUMP_TRANS_FIELD(uint64_t, Size)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1201U, Type_);
    visitor.VisitMember(0x1202U, DataAddr_);
    visitor.VisitMember(0x1203U, Size_);
  }
};

class DumpTransBufferInfo {
  DUMP_TRANS_FIELD(DumpTransBufferType, Type)
  DUMP_TRANS_FIELD(uint64_t, Address)
  DUMP_TRANS_FIELD(uint64_t, Size)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1301U, Type_);
    visitor.VisitMember(0x1302U, Address_);
    visitor.VisitMember(0x1303U, Size_);
  }
};

class DumpTransAttrInfo {
  DUMP_TRANS_FIELD(std::string, Name)
  DUMP_TRANS_FIELD(std::string, Value)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1401U, Name_);
    visitor.VisitMember(0x1402U, Value_);
  }
};

class DumpTransAddrSizeInfo {
  DUMP_TRANS_FIELD(uint64_t, Address)
  DUMP_TRANS_FIELD(uint64_t, Size)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1601U, Address_);
    visitor.VisitMember(0x1602U, Size_);
  }
};

class DumpTransContextInfo {
  DUMP_TRANS_FIELD(uint32_t, ContextId)
  DUMP_TRANS_FIELD(uint32_t, ThreadId)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransAddrSizeInfo, Input)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransAddrSizeInfo, Output)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x1501U, ContextId_);
    visitor.VisitMember(0x1502U, ThreadId_);
    visitor.VisitMember(0x0016U, Input_);
    visitor.VisitMember(0x0017U, Output_);
  }
};

class DumpTransTaskInfo {
  DUMP_TRANS_FIELD(uint32_t, TaskId)
  DUMP_TRANS_FIELD(uint32_t, StreamId)
  DUMP_TRANS_FIELD(std::string, OpName)
  DUMP_TRANS_FIELD(std::string, OpType)
  DUMP_TRANS_FIELD(bool, EndGraph)
  DUMP_TRANS_FIELD(DumpTransTaskType, TaskType)
  DUMP_TRANS_FIELD(uint32_t, ContextId)
  DUMP_TRANS_FIELD(uint32_t, ThreadId)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransInputInfo, Input)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransOutputInfo, Output)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransWorkspaceInfo, Workspace)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransBufferInfo, Buffer)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransAttrInfo, Attr)
  DUMP_TRANS_FIELD_COLLECTION(DumpTransContextInfo, Context)
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(0x0201U, TaskId_);
    visitor.VisitMember(0x0202U, StreamId_);
    visitor.VisitMember(0x0203U, OpName_);
    visitor.VisitMember(0x0204U, OpType_);
    visitor.VisitMember(0x0205U, EndGraph_);
    visitor.VisitMember(0x0206U, TaskType_);
    visitor.VisitMember(0x0207U, ContextId_);
    visitor.VisitMember(0x0208U, ThreadId_);
    visitor.VisitMember(0x0010U, Input_);
    visitor.VisitMember(0x0011U, Output_);
    visitor.VisitMember(0x0012U, Workspace_);
    visitor.VisitMember(0x0013U, Buffer_);
    visitor.VisitMember(0x0014U, Attr_);
    visitor.VisitMember(0x0015U, Context_);
  }
};

#undef DUMP_TRANS_FIELD
#undef DUMP_TRANS_FIELD_EXPLICIT
#undef DUMP_TRANS_FIELD_COLLECTION

class DumpTransportInfo {
 public:
  uint16_t GetVersion() const {
    return DumpWireSchema::kVersion;
  }
  DumpTransModelInfo &MutableModel() {
    return model_;
  }
  const DumpTransModelInfo &GetModel() const {
    return model_;
  }
  DumpTransTaskInfo &AddTask() {
    tasks_.emplace_back();
    return tasks_.back();
  }
  size_t GetTaskCount() const {
    return tasks_.size();
  }
  DumpTransStatus GetTask(size_t index, const DumpTransTaskInfo *&task) const {
    if (index >= tasks_.size()) {
      return DumpTransStatus::kInvalidParam;
    }
    task = &tasks_[index];
    return DumpTransStatus::kOk;
  }
  void Clear() {
    model_ = DumpTransModelInfo{};
    tasks_.clear();
  }

  // 顶层写入顺序：公共头（total_size 占位）→ MODEL(0x0001) → TASK(0x0002)*N → 回填 total_size
  DumpTransStatus Serialize(std::vector<uint8_t> &buffer) {
    if (tasks_.size() >= UINT32_MAX) {
      return DumpTransStatus::kLengthOverflow;
    }
    try {
      wire::WireTlvEncoder encoder;
      wire::WriteCommonHeader<DumpWireSchema>(encoder, static_cast<uint32_t>(tasks_.size() + 1U));
      encoder.WriteRecord(0x0001U, model_);
      encoder.VisitMember(0x0002U, tasks_);
      if (encoder.status != DumpTransStatus::kOk) {
        return encoder.status;
      }
      wire::FillWireTlvTotalSize(encoder.bytes);
      buffer.swap(encoder.bytes);
      return DumpTransStatus::kOk;
    } catch (const std::bad_alloc &) {
      return DumpTransStatus::kNoMemory;
    } catch (const std::length_error &) {
      return DumpTransStatus::kLengthOverflow;
    }
  }

  DumpTransStatus Deserialize(const uint8_t *data, size_t size) {
    try {
      DumpTransportInfo parsed;
      uint64_t count = 0U;
      bool has_model = false;
      const auto status = wire::DecodeWireTlvRecords<DumpWireSchema>(
          data, size, [&parsed, &count, &has_model](uint16_t tag, const uint8_t *payload, size_t payload_size) {
            ++count;
            if (tag == 0x0001U) {
              if (has_model) {
                return DumpTransStatus::kInvalidFormat;
              }
              has_model = true;
              return wire::WireTlvDecodeRecord<DumpWireSchema>(payload, payload_size, parsed.model_);
            }
            if (tag == 0x0002U) {
              return wire::WireTlvDecodeRecord<DumpWireSchema>(payload, payload_size, parsed.AddTask());
            }
            // 已知子级标签漏到顶层 = 层次非法；未知标签跳过（前向兼容）
            if (DumpWireSchema::IsKnownTag(tag)) {
              return DumpTransStatus::kInvalidFormat;
            }
            return DumpTransStatus::kOk;
          });
      if (status != DumpTransStatus::kOk) {
        return status;
      }
      if (!has_model || count != wire::GetRecordCountFromHeader(data, size)) {
        return DumpTransStatus::kInvalidFormat;
      }
      std::swap(model_, parsed.model_);
      tasks_.swap(parsed.tasks_);
      return DumpTransStatus::kOk;
    } catch (const std::bad_alloc &) {
      return DumpTransStatus::kNoMemory;
    } catch (const std::length_error &) {
      return DumpTransStatus::kLengthOverflow;
    }
  }

 private:
  DumpTransModelInfo model_;
  std::vector<DumpTransTaskInfo> tasks_;
};
}  // namespace dump
}  // namespace ge
#endif  // GE_FRAMEWORK_RUNTIME_DUMP_DUMP_TRANSPORT_INFO_H_
