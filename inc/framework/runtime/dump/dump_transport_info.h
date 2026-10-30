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
// 原语、presence 语义与编解码器、公共头 helper）在通用框架 `inc/framework/common/wire/wire_tlv.h`
// 中，其它模块可直接复用该框架并自带 schema。
#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DUMP_TRANSPORT_INFO_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DUMP_TRANSPORT_INFO_H_

#include <algorithm>
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

enum class DumpTransTag : uint16_t {
  kModel = 0x0001U,
  kTask = 0x0002U,
  kInput = 0x0010U,
  kOutput = 0x0011U,
  kWorkspace = 0x0012U,
  kBuffer = 0x0013U,
  kAttr = 0x0014U,
  kContext = 0x0015U,
  kContextInput = 0x0016U,
  kContextOutput = 0x0017U,
  kDimRange = 0x0018U,
  kDumpPath = 0x0101U,
  kModelName = 0x0102U,
  kModelId = 0x0103U,
  kStepIdAddr = 0x0104U,
  kIterationsPerLoopAddr = 0x0105U,
  kLoopCondAddr = 0x0106U,
  kFlag = 0x0107U,
  kDumpStep = 0x0108U,
  kDumpData = 0x0109U,
  kDumpSwitchAddr = 0x010aU,
  kTaskId = 0x0201U,
  kStreamId = 0x0202U,
  kOpName = 0x0203U,
  kOpType = 0x0204U,
  kEndGraph = 0x0205U,
  kTaskType = 0x0206U,
  kTaskContextId = 0x0207U,
  kTaskThreadId = 0x0208U,
  kInputDataType = 0x1001U,
  kInputFormat = 0x1002U,
  kInputShape = 0x1003U,
  kInputAddress = 0x1004U,
  kInputSize = 0x1005U,
  kInputOriginShape = 0x1006U,
  kInputAddrType = 0x1007U,
  kInputOffset = 0x1008U,
  kOutputDataType = 0x1101U,
  kOutputFormat = 0x1102U,
  kOutputShape = 0x1103U,
  kOutputAddress = 0x1104U,
  kOriginalName = 0x1105U,
  kOriginalOutputIndex = 0x1106U,
  kOriginalOutputDataType = 0x1107U,
  kOriginalOutputFormat = 0x1108U,
  kOutputSize = 0x1109U,
  kOutputOriginShape = 0x110aU,
  kOutputAddrType = 0x110bU,
  kOutputOffset = 0x110cU,
  kWorkspaceType = 0x1201U,
  kWorkspaceDataAddr = 0x1202U,
  kWorkspaceSize = 0x1203U,
  kBufferType = 0x1301U,
  kBufferAddress = 0x1302U,
  kBufferSize = 0x1303U,
  kAttrName = 0x1401U,
  kAttrValue = 0x1402U,
  kContextId = 0x1501U,
  kThreadId = 0x1502U,
  kContextAddress = 0x1601U,
  kContextSize = 0x1602U,
  kDimStart = 0x1801U,
  kDimEnd = 0x1802U
};

constexpr uint16_t ToTag(const DumpTransTag tag) {
  return static_cast<uint16_t>(tag);
}

// DumpWire v1 的 schema 策略：向通用框架注入协议身份与 tag 全集（编译期，零运行开销）
struct DumpWireSchema {
  static constexpr uint32_t kMagic = 0x564C5401U;
  static constexpr uint16_t kVersion = 1U;

  // tag 注册表与 DumpTransTag 枚举单点对应：枚举按值升序声明，本表天然有序（binary_search 前置条件），
  // 与码流实际 tag 的双向一致性由 UT DumpWireSchema_ShouldRegisterEveryEncodedTag 核对
  static constexpr uint16_t kRegisteredTags[] = {ToTag(DumpTransTag::kModel),
                                                 ToTag(DumpTransTag::kTask),
                                                 ToTag(DumpTransTag::kInput),
                                                 ToTag(DumpTransTag::kOutput),
                                                 ToTag(DumpTransTag::kWorkspace),
                                                 ToTag(DumpTransTag::kBuffer),
                                                 ToTag(DumpTransTag::kAttr),
                                                 ToTag(DumpTransTag::kContext),
                                                 ToTag(DumpTransTag::kContextInput),
                                                 ToTag(DumpTransTag::kContextOutput),
                                                 ToTag(DumpTransTag::kDimRange),
                                                 ToTag(DumpTransTag::kDumpPath),
                                                 ToTag(DumpTransTag::kModelName),
                                                 ToTag(DumpTransTag::kModelId),
                                                 ToTag(DumpTransTag::kStepIdAddr),
                                                 ToTag(DumpTransTag::kIterationsPerLoopAddr),
                                                 ToTag(DumpTransTag::kLoopCondAddr),
                                                 ToTag(DumpTransTag::kFlag),
                                                 ToTag(DumpTransTag::kDumpStep),
                                                 ToTag(DumpTransTag::kDumpData),
                                                 ToTag(DumpTransTag::kDumpSwitchAddr),
                                                 ToTag(DumpTransTag::kTaskId),
                                                 ToTag(DumpTransTag::kStreamId),
                                                 ToTag(DumpTransTag::kOpName),
                                                 ToTag(DumpTransTag::kOpType),
                                                 ToTag(DumpTransTag::kEndGraph),
                                                 ToTag(DumpTransTag::kTaskType),
                                                 ToTag(DumpTransTag::kTaskContextId),
                                                 ToTag(DumpTransTag::kTaskThreadId),
                                                 ToTag(DumpTransTag::kInputDataType),
                                                 ToTag(DumpTransTag::kInputFormat),
                                                 ToTag(DumpTransTag::kInputShape),
                                                 ToTag(DumpTransTag::kInputAddress),
                                                 ToTag(DumpTransTag::kInputSize),
                                                 ToTag(DumpTransTag::kInputOriginShape),
                                                 ToTag(DumpTransTag::kInputAddrType),
                                                 ToTag(DumpTransTag::kInputOffset),
                                                 ToTag(DumpTransTag::kOutputDataType),
                                                 ToTag(DumpTransTag::kOutputFormat),
                                                 ToTag(DumpTransTag::kOutputShape),
                                                 ToTag(DumpTransTag::kOutputAddress),
                                                 ToTag(DumpTransTag::kOriginalName),
                                                 ToTag(DumpTransTag::kOriginalOutputIndex),
                                                 ToTag(DumpTransTag::kOriginalOutputDataType),
                                                 ToTag(DumpTransTag::kOriginalOutputFormat),
                                                 ToTag(DumpTransTag::kOutputSize),
                                                 ToTag(DumpTransTag::kOutputOriginShape),
                                                 ToTag(DumpTransTag::kOutputAddrType),
                                                 ToTag(DumpTransTag::kOutputOffset),
                                                 ToTag(DumpTransTag::kWorkspaceType),
                                                 ToTag(DumpTransTag::kWorkspaceDataAddr),
                                                 ToTag(DumpTransTag::kWorkspaceSize),
                                                 ToTag(DumpTransTag::kBufferType),
                                                 ToTag(DumpTransTag::kBufferAddress),
                                                 ToTag(DumpTransTag::kBufferSize),
                                                 ToTag(DumpTransTag::kAttrName),
                                                 ToTag(DumpTransTag::kAttrValue),
                                                 ToTag(DumpTransTag::kContextId),
                                                 ToTag(DumpTransTag::kThreadId),
                                                 ToTag(DumpTransTag::kContextAddress),
                                                 ToTag(DumpTransTag::kContextSize),
                                                 ToTag(DumpTransTag::kDimStart),
                                                 ToTag(DumpTransTag::kDimEnd)};
  static constexpr size_t kRegisteredTagCount = sizeof(kRegisteredTags) / sizeof(kRegisteredTags[0]);

  static bool IsKnownTag(uint16_t tag) {
    return std::binary_search(kRegisteredTags, kRegisteredTags + kRegisteredTagCount, tag);
  }
};

// 记录类通过该跳板向编解码器开放字段注册函数。
namespace dump_wire_detail {
using wire::MemberRegHelper;
}  // namespace dump_wire_detail

constexpr uint32_t kModelRecordCount = 1U;

class DumpTransTensorInfo {
 public:
  void SetDataType(const int32_t &value) {
    wire::SetWireTlvField(DataType_, value);
  }
  const int32_t &GetDataType() const {
    return wire::GetWireTlvField(DataType_);
  }
  void SetFormat(const int32_t &value) {
    wire::SetWireTlvField(Format_, value);
  }
  const int32_t &GetFormat() const {
    return wire::GetWireTlvField(Format_);
  }
  void SetShape(const std::vector<uint64_t> &value) {
    wire::SetWireTlvField(Shape_, value);
  }
  const std::vector<uint64_t> &GetShape() const {
    return wire::GetWireTlvField(Shape_);
  }
  void SetAddress(const uint64_t &value) {
    wire::SetWireTlvField(Address_, value);
  }
  const uint64_t &GetAddress() const {
    return wire::GetWireTlvField(Address_);
  }
  void SetSize(const uint64_t &value) {
    wire::SetWireTlvField(Size_, value);
  }
  const uint64_t &GetSize() const {
    return wire::GetWireTlvField(Size_);
  }
  void SetOriginShape(const std::vector<uint64_t> &value) {
    wire::SetWireTlvField(OriginShape_, value);
  }
  const std::vector<uint64_t> &GetOriginShape() const {
    return wire::GetWireTlvField(OriginShape_);
  }
  void SetAddrType(const DumpTransAddressType &value) {
    wire::SetWireTlvField(AddrType_, value);
  }
  const DumpTransAddressType &GetAddrType() const {
    return wire::GetWireTlvField(AddrType_);
  }
  void SetOffset(const uint64_t &value) {
    wire::SetWireTlvField(Offset_, value);
  }
  const uint64_t &GetOffset() const {
    return wire::GetWireTlvField(Offset_);
  }

 protected:
  WireTlvField<int32_t> DataType_;
  WireTlvField<int32_t> Format_;
  WireTlvField<std::vector<uint64_t>> Shape_;
  WireTlvField<uint64_t> Address_;
  WireTlvField<uint64_t> Size_;
  WireTlvField<std::vector<uint64_t>> OriginShape_;
  WireTlvField<DumpTransAddressType> AddrType_;
  WireTlvField<uint64_t> Offset_;
};

class DumpTransModelInfo {
 public:
  void SetDumpPath(const std::string &value) {
    wire::SetWireTlvField(DumpPath_, value);
  }
  const std::string &GetDumpPath() const {
    return wire::GetWireTlvField(DumpPath_);
  }
  // 以下 5 个字段用显式 presence，与 op_mapping.proto 中包在单成员 oneof 里的同名字段一一对应：
  // 消费端（AICPU dump kernel）按“字段是否下发”决定 has* 标志，进而影响落盘目录层级，
  // 因此 GE 侧即使取值为 0 / 空串也必须上线；其余字段沿用 proto3 plain 标量的 implicit presence。
  void SetModelName(const std::string &value) {
    wire::SetWireTlvField(ModelName_, value);
  }
  const std::string &GetModelName() const {
    return wire::GetWireTlvField(ModelName_);
  }
  bool HasModelName() const {
    return wire::HasWireTlvField(ModelName_);
  }
  void SetModelId(const uint32_t &value) {
    wire::SetWireTlvField(ModelId_, value);
  }
  const uint32_t &GetModelId() const {
    return wire::GetWireTlvField(ModelId_);
  }
  bool HasModelId() const {
    return wire::HasWireTlvField(ModelId_);
  }
  void SetStepIdAddr(const uint64_t &value) {
    wire::SetWireTlvField(StepIdAddr_, value);
  }
  const uint64_t &GetStepIdAddr() const {
    return wire::GetWireTlvField(StepIdAddr_);
  }
  bool HasStepIdAddr() const {
    return wire::HasWireTlvField(StepIdAddr_);
  }
  void SetIterationsPerLoopAddr(const uint64_t &value) {
    wire::SetWireTlvField(IterationsPerLoopAddr_, value);
  }
  const uint64_t &GetIterationsPerLoopAddr() const {
    return wire::GetWireTlvField(IterationsPerLoopAddr_);
  }
  bool HasIterationsPerLoopAddr() const {
    return wire::HasWireTlvField(IterationsPerLoopAddr_);
  }
  void SetLoopCondAddr(const uint64_t &value) {
    wire::SetWireTlvField(LoopCondAddr_, value);
  }
  const uint64_t &GetLoopCondAddr() const {
    return wire::GetWireTlvField(LoopCondAddr_);
  }
  bool HasLoopCondAddr() const {
    return wire::HasWireTlvField(LoopCondAddr_);
  }
  void SetFlag(const uint32_t &value) {
    wire::SetWireTlvField(Flag_, value);
  }
  const uint32_t &GetFlag() const {
    return wire::GetWireTlvField(Flag_);
  }
  void SetDumpStep(const std::string &value) {
    wire::SetWireTlvField(DumpStep_, value);
  }
  const std::string &GetDumpStep() const {
    return wire::GetWireTlvField(DumpStep_);
  }
  void SetDumpData(const DumpTransDumpData &value) {
    wire::SetWireTlvField(DumpData_, value);
  }
  const DumpTransDumpData &GetDumpData() const {
    return wire::GetWireTlvField(DumpData_);
  }
  void SetDumpSwitchAddr(const uint64_t &value) {
    wire::SetWireTlvField(DumpSwitchAddr_, value);
  }
  const uint64_t &GetDumpSwitchAddr() const {
    return wire::GetWireTlvField(DumpSwitchAddr_);
  }

 private:
  WireTlvField<std::string> DumpPath_;
  WireTlvField<std::string, true> ModelName_;
  WireTlvField<uint32_t, true> ModelId_;
  WireTlvField<uint64_t, true> StepIdAddr_;
  WireTlvField<uint64_t, true> IterationsPerLoopAddr_;
  WireTlvField<uint64_t, true> LoopCondAddr_;
  WireTlvField<uint32_t> Flag_;
  WireTlvField<std::string> DumpStep_;
  WireTlvField<DumpTransDumpData> DumpData_;
  WireTlvField<uint64_t> DumpSwitchAddr_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kDumpPath), DumpPath_);
    visitor.VisitMember(ToTag(DumpTransTag::kModelName), ModelName_);
    visitor.VisitMember(ToTag(DumpTransTag::kModelId), ModelId_);
    visitor.VisitMember(ToTag(DumpTransTag::kStepIdAddr), StepIdAddr_);
    visitor.VisitMember(ToTag(DumpTransTag::kIterationsPerLoopAddr), IterationsPerLoopAddr_);
    visitor.VisitMember(ToTag(DumpTransTag::kLoopCondAddr), LoopCondAddr_);
    visitor.VisitMember(ToTag(DumpTransTag::kFlag), Flag_);
    visitor.VisitMember(ToTag(DumpTransTag::kDumpStep), DumpStep_);
    visitor.VisitMember(ToTag(DumpTransTag::kDumpData), DumpData_);
    visitor.VisitMember(ToTag(DumpTransTag::kDumpSwitchAddr), DumpSwitchAddr_);
  }
};

class DumpTransDimRangeInfo {
 public:
  void SetDimStart(const uint64_t &value) {
    wire::SetWireTlvField(DimStart_, value);
  }
  const uint64_t &GetDimStart() const {
    return wire::GetWireTlvField(DimStart_);
  }
  void SetDimEnd(const uint64_t &value) {
    wire::SetWireTlvField(DimEnd_, value);
  }
  const uint64_t &GetDimEnd() const {
    return wire::GetWireTlvField(DimEnd_);
  }

 private:
  WireTlvField<uint64_t> DimStart_;
  WireTlvField<uint64_t> DimEnd_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kDimStart), DimStart_);
    visitor.VisitMember(ToTag(DumpTransTag::kDimEnd), DimEnd_);
  }
};

class DumpTransInputInfo : public DumpTransTensorInfo {
 private:
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kInputDataType), DataType_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputFormat), Format_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputShape), Shape_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputAddress), Address_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputSize), Size_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputOriginShape), OriginShape_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputAddrType), AddrType_);
    visitor.VisitMember(ToTag(DumpTransTag::kInputOffset), Offset_);
  }
};

class DumpTransOutputInfo : public DumpTransTensorInfo {
 public:
  void SetOriginalName(const std::string &value) {
    wire::SetWireTlvField(OriginalName_, value);
  }
  const std::string &GetOriginalName() const {
    return wire::GetWireTlvField(OriginalName_);
  }
  void SetOriginalOutputIndex(const int32_t &value) {
    wire::SetWireTlvField(OriginalOutputIndex_, value);
  }
  const int32_t &GetOriginalOutputIndex() const {
    return wire::GetWireTlvField(OriginalOutputIndex_);
  }
  void SetOriginalOutputDataType(const int32_t &value) {
    wire::SetWireTlvField(OriginalOutputDataType_, value);
  }
  const int32_t &GetOriginalOutputDataType() const {
    return wire::GetWireTlvField(OriginalOutputDataType_);
  }
  void SetOriginalOutputFormat(const int32_t &value) {
    wire::SetWireTlvField(OriginalOutputFormat_, value);
  }
  const int32_t &GetOriginalOutputFormat() const {
    return wire::GetWireTlvField(OriginalOutputFormat_);
  }
  DumpTransDimRangeInfo &AddDimRange() {
    return wire::AddWireTlvItem(DimRange_);
  }
  size_t GetDimRangeCount() const {
    return wire::GetWireTlvItemCount(DimRange_);
  }
  DumpTransStatus GetDimRange(const size_t index, const DumpTransDimRangeInfo *&value) const {
    return wire::GetWireTlvItem(DimRange_, index, value);
  }

 private:
  WireTlvField<std::string> OriginalName_;
  WireTlvField<int32_t> OriginalOutputIndex_;
  WireTlvField<int32_t> OriginalOutputDataType_;
  WireTlvField<int32_t> OriginalOutputFormat_;
  std::vector<DumpTransDimRangeInfo> DimRange_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kOutputDataType), DataType_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputFormat), Format_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputShape), Shape_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputAddress), Address_);
    visitor.VisitMember(ToTag(DumpTransTag::kOriginalName), OriginalName_);
    visitor.VisitMember(ToTag(DumpTransTag::kOriginalOutputIndex), OriginalOutputIndex_);
    visitor.VisitMember(ToTag(DumpTransTag::kOriginalOutputDataType), OriginalOutputDataType_);
    visitor.VisitMember(ToTag(DumpTransTag::kOriginalOutputFormat), OriginalOutputFormat_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputSize), Size_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputOriginShape), OriginShape_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputAddrType), AddrType_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutputOffset), Offset_);
    visitor.VisitMember(ToTag(DumpTransTag::kDimRange), DimRange_);
  }
};

class DumpTransWorkspaceInfo {
 public:
  void SetType(const DumpTransWorkspaceType &value) {
    wire::SetWireTlvField(Type_, value);
  }
  const DumpTransWorkspaceType &GetType() const {
    return wire::GetWireTlvField(Type_);
  }
  void SetDataAddr(const uint64_t &value) {
    wire::SetWireTlvField(DataAddr_, value);
  }
  const uint64_t &GetDataAddr() const {
    return wire::GetWireTlvField(DataAddr_);
  }
  void SetSize(const uint64_t &value) {
    wire::SetWireTlvField(Size_, value);
  }
  const uint64_t &GetSize() const {
    return wire::GetWireTlvField(Size_);
  }

 private:
  WireTlvField<DumpTransWorkspaceType> Type_;
  WireTlvField<uint64_t> DataAddr_;
  WireTlvField<uint64_t> Size_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kWorkspaceType), Type_);
    visitor.VisitMember(ToTag(DumpTransTag::kWorkspaceDataAddr), DataAddr_);
    visitor.VisitMember(ToTag(DumpTransTag::kWorkspaceSize), Size_);
  }
};

class DumpTransBufferInfo {
 public:
  void SetType(const DumpTransBufferType &value) {
    wire::SetWireTlvField(Type_, value);
  }
  const DumpTransBufferType &GetType() const {
    return wire::GetWireTlvField(Type_);
  }
  void SetAddress(const uint64_t &value) {
    wire::SetWireTlvField(Address_, value);
  }
  const uint64_t &GetAddress() const {
    return wire::GetWireTlvField(Address_);
  }
  void SetSize(const uint64_t &value) {
    wire::SetWireTlvField(Size_, value);
  }
  const uint64_t &GetSize() const {
    return wire::GetWireTlvField(Size_);
  }

 private:
  WireTlvField<DumpTransBufferType> Type_;
  WireTlvField<uint64_t> Address_;
  WireTlvField<uint64_t> Size_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kBufferType), Type_);
    visitor.VisitMember(ToTag(DumpTransTag::kBufferAddress), Address_);
    visitor.VisitMember(ToTag(DumpTransTag::kBufferSize), Size_);
  }
};

class DumpTransAttrInfo {
 public:
  void SetName(const std::string &value) {
    wire::SetWireTlvField(Name_, value);
  }
  const std::string &GetName() const {
    return wire::GetWireTlvField(Name_);
  }
  void SetValue(const std::string &value) {
    wire::SetWireTlvField(Value_, value);
  }
  const std::string &GetValue() const {
    return wire::GetWireTlvField(Value_);
  }

 private:
  WireTlvField<std::string> Name_;
  WireTlvField<std::string> Value_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kAttrName), Name_);
    visitor.VisitMember(ToTag(DumpTransTag::kAttrValue), Value_);
  }
};

class DumpTransAddrSizeInfo {
 public:
  void SetAddress(const uint64_t &value) {
    wire::SetWireTlvField(Address_, value);
  }
  const uint64_t &GetAddress() const {
    return wire::GetWireTlvField(Address_);
  }
  void SetSize(const uint64_t &value) {
    wire::SetWireTlvField(Size_, value);
  }
  const uint64_t &GetSize() const {
    return wire::GetWireTlvField(Size_);
  }

 private:
  WireTlvField<uint64_t> Address_;
  WireTlvField<uint64_t> Size_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kContextAddress), Address_);
    visitor.VisitMember(ToTag(DumpTransTag::kContextSize), Size_);
  }
};

class DumpTransContextInfo {
 public:
  void SetContextId(const uint32_t &value) {
    wire::SetWireTlvField(ContextId_, value);
  }
  const uint32_t &GetContextId() const {
    return wire::GetWireTlvField(ContextId_);
  }
  void SetThreadId(const uint32_t &value) {
    wire::SetWireTlvField(ThreadId_, value);
  }
  const uint32_t &GetThreadId() const {
    return wire::GetWireTlvField(ThreadId_);
  }
  DumpTransAddrSizeInfo &AddInput() {
    return wire::AddWireTlvItem(Input_);
  }
  size_t GetInputCount() const {
    return wire::GetWireTlvItemCount(Input_);
  }
  DumpTransStatus GetInput(const size_t index, const DumpTransAddrSizeInfo *&value) const {
    return wire::GetWireTlvItem(Input_, index, value);
  }
  DumpTransAddrSizeInfo &AddOutput() {
    return wire::AddWireTlvItem(Output_);
  }
  size_t GetOutputCount() const {
    return wire::GetWireTlvItemCount(Output_);
  }
  DumpTransStatus GetOutput(const size_t index, const DumpTransAddrSizeInfo *&value) const {
    return wire::GetWireTlvItem(Output_, index, value);
  }

 private:
  WireTlvField<uint32_t> ContextId_;
  WireTlvField<uint32_t> ThreadId_;
  std::vector<DumpTransAddrSizeInfo> Input_;
  std::vector<DumpTransAddrSizeInfo> Output_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kContextId), ContextId_);
    visitor.VisitMember(ToTag(DumpTransTag::kThreadId), ThreadId_);
    visitor.VisitMember(ToTag(DumpTransTag::kContextInput), Input_);
    visitor.VisitMember(ToTag(DumpTransTag::kContextOutput), Output_);
  }
};

class DumpTransTaskInfo {
 public:
  void SetTaskId(const uint32_t &value) {
    wire::SetWireTlvField(TaskId_, value);
  }
  const uint32_t &GetTaskId() const {
    return wire::GetWireTlvField(TaskId_);
  }
  void SetStreamId(const uint32_t &value) {
    wire::SetWireTlvField(StreamId_, value);
  }
  const uint32_t &GetStreamId() const {
    return wire::GetWireTlvField(StreamId_);
  }
  void SetOpName(const std::string &value) {
    wire::SetWireTlvField(OpName_, value);
  }
  const std::string &GetOpName() const {
    return wire::GetWireTlvField(OpName_);
  }
  void SetOpType(const std::string &value) {
    wire::SetWireTlvField(OpType_, value);
  }
  const std::string &GetOpType() const {
    return wire::GetWireTlvField(OpType_);
  }
  void SetEndGraph(const bool &value) {
    wire::SetWireTlvField(EndGraph_, value);
  }
  const bool &GetEndGraph() const {
    return wire::GetWireTlvField(EndGraph_);
  }
  void SetTaskType(const DumpTransTaskType &value) {
    wire::SetWireTlvField(TaskType_, value);
  }
  const DumpTransTaskType &GetTaskType() const {
    return wire::GetWireTlvField(TaskType_);
  }
  void SetContextId(const uint32_t &value) {
    wire::SetWireTlvField(ContextId_, value);
  }
  const uint32_t &GetContextId() const {
    return wire::GetWireTlvField(ContextId_);
  }
  void SetThreadId(const uint32_t &value) {
    wire::SetWireTlvField(ThreadId_, value);
  }
  const uint32_t &GetThreadId() const {
    return wire::GetWireTlvField(ThreadId_);
  }

  DumpTransInputInfo &AddInput() {
    return wire::AddWireTlvItem(Input_);
  }
  size_t GetInputCount() const {
    return wire::GetWireTlvItemCount(Input_);
  }
  DumpTransStatus GetInput(const size_t index, const DumpTransInputInfo *&value) const {
    return wire::GetWireTlvItem(Input_, index, value);
  }
  DumpTransOutputInfo &AddOutput() {
    return wire::AddWireTlvItem(Output_);
  }
  size_t GetOutputCount() const {
    return wire::GetWireTlvItemCount(Output_);
  }
  DumpTransStatus GetOutput(const size_t index, const DumpTransOutputInfo *&value) const {
    return wire::GetWireTlvItem(Output_, index, value);
  }
  DumpTransWorkspaceInfo &AddWorkspace() {
    return wire::AddWireTlvItem(Workspace_);
  }
  size_t GetWorkspaceCount() const {
    return wire::GetWireTlvItemCount(Workspace_);
  }
  DumpTransStatus GetWorkspace(const size_t index, const DumpTransWorkspaceInfo *&value) const {
    return wire::GetWireTlvItem(Workspace_, index, value);
  }
  DumpTransBufferInfo &AddBuffer() {
    return wire::AddWireTlvItem(Buffer_);
  }
  size_t GetBufferCount() const {
    return wire::GetWireTlvItemCount(Buffer_);
  }
  DumpTransStatus GetBuffer(const size_t index, const DumpTransBufferInfo *&value) const {
    return wire::GetWireTlvItem(Buffer_, index, value);
  }
  DumpTransAttrInfo &AddAttr() {
    return wire::AddWireTlvItem(Attr_);
  }
  size_t GetAttrCount() const {
    return wire::GetWireTlvItemCount(Attr_);
  }
  DumpTransStatus GetAttr(const size_t index, const DumpTransAttrInfo *&value) const {
    return wire::GetWireTlvItem(Attr_, index, value);
  }
  DumpTransContextInfo &AddContext() {
    return wire::AddWireTlvItem(Context_);
  }
  size_t GetContextCount() const {
    return wire::GetWireTlvItemCount(Context_);
  }
  DumpTransStatus GetContext(const size_t index, const DumpTransContextInfo *&value) const {
    return wire::GetWireTlvItem(Context_, index, value);
  }

 private:
  WireTlvField<uint32_t> TaskId_;
  WireTlvField<uint32_t> StreamId_;
  WireTlvField<std::string> OpName_;
  WireTlvField<std::string> OpType_;
  WireTlvField<bool> EndGraph_;
  WireTlvField<DumpTransTaskType> TaskType_;
  WireTlvField<uint32_t> ContextId_;
  WireTlvField<uint32_t> ThreadId_;
  std::vector<DumpTransInputInfo> Input_;
  std::vector<DumpTransOutputInfo> Output_;
  std::vector<DumpTransWorkspaceInfo> Workspace_;
  std::vector<DumpTransBufferInfo> Buffer_;
  std::vector<DumpTransAttrInfo> Attr_;
  std::vector<DumpTransContextInfo> Context_;
  friend struct dump_wire_detail::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(ToTag(DumpTransTag::kTaskId), TaskId_);
    visitor.VisitMember(ToTag(DumpTransTag::kStreamId), StreamId_);
    visitor.VisitMember(ToTag(DumpTransTag::kOpName), OpName_);
    visitor.VisitMember(ToTag(DumpTransTag::kOpType), OpType_);
    visitor.VisitMember(ToTag(DumpTransTag::kEndGraph), EndGraph_);
    visitor.VisitMember(ToTag(DumpTransTag::kTaskType), TaskType_);
    visitor.VisitMember(ToTag(DumpTransTag::kTaskContextId), ContextId_);
    visitor.VisitMember(ToTag(DumpTransTag::kTaskThreadId), ThreadId_);
    visitor.VisitMember(ToTag(DumpTransTag::kInput), Input_);
    visitor.VisitMember(ToTag(DumpTransTag::kOutput), Output_);
    visitor.VisitMember(ToTag(DumpTransTag::kWorkspace), Workspace_);
    visitor.VisitMember(ToTag(DumpTransTag::kBuffer), Buffer_);
    visitor.VisitMember(ToTag(DumpTransTag::kAttr), Attr_);
    visitor.VisitMember(ToTag(DumpTransTag::kContext), Context_);
  }
};

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

  // 顶层写入顺序：公共头（total_size 占位）→ MODEL → TASK*N → 回填 total_size
  DumpTransStatus Serialize(std::vector<uint8_t> &buffer) {
    if (tasks_.size() >= UINT32_MAX) {
      return DumpTransStatus::kLengthOverflow;
    }
    try {
      wire::WireTlvEncoder encoder;
      wire::WriteCommonHeader<DumpWireSchema>(encoder, static_cast<uint32_t>(tasks_.size() + kModelRecordCount));
      encoder.WriteRecord(ToTag(DumpTransTag::kModel), model_);
      encoder.VisitMember(ToTag(DumpTransTag::kTask), tasks_);
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
            if (tag == ToTag(DumpTransTag::kModel)) {
              if (has_model) {
                return DumpTransStatus::kInvalidFormat;
              }
              has_model = true;
              return wire::WireTlvDecodeRecord<DumpWireSchema>(payload, payload_size, parsed.model_);
            }
            if (tag == ToTag(DumpTransTag::kTask)) {
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
