/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// 通用 WireTLV 框架（inc/framework/common/wire/wire_tlv.h）的单元测试。
// 本文件只测试与业务无关的框架机制，使用测试自建 mini schema（TestWireSchema），不引入任何业务语义。
// 覆盖点：16B 公共头与帧切分、两类 presence 上线规则（implicit 裁剪默认值 / explicit 默认值也上线）、
// 全部值编码（string 裸字节、无符号/有符号 zigzag varint、bool 定宽 1B、维度集合、枚举）、三层嵌套集合、
// 未知 tag 跳过与重复 tag 取最后、畸形码流拒绝且无部分提交、编码长度溢出。
// golden 字节一律由本文件独立构造器（Le/Varint/ZzVarint/Append/Str/Tlv/PokeLe/Wire）手工推导产生，
// 绝不复用被测的 WireTlvEncoder/Serialize，避免"期望值"与"被测实现"互相印证。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "framework/common/wire/wire_tlv.h"

namespace wire = ge::wire;  // 本文件局部别名，不使用 using namespace

namespace {
using Bytes = std::vector<uint8_t>;
using TagList = std::vector<uint16_t>;

// ----------------------------------------------
// mini schema：tag 分配（顶层 2 + 集合 3 + 字段 18 = 23 个 tag）
//   顶层记录: DOC=0x0001（恰 1 个）、ITEM=0x0002（0..N）
//   ITEM 集合: CELL=0x0010、PAIR=0x0011；CELL 集合: RANGE=0x0018
//   DOC  字段: Name=0x0101 Label=0x0102(EXPLICIT) Mode=0x0103 Enabled=0x0104
//              Revision=0x0105 Capacity=0x0106 Mark=0x0107(EXPLICIT)
//   ITEM 字段: ItemId=0x0201 SignedVal=0x0202 Dims=0x0203 Note=0x0204
//   CELL 字段: CellId=0x1001 Kind=0x1002 Score=0x1003
//   PAIR 字段: Address=0x1101 Size=0x1102
//   RANGE 字段: Begin=0x1801 End=0x1802
// ----------------------------------------------
constexpr uint16_t kTagDoc = 0x0001U;
constexpr uint16_t kTagItem = 0x0002U;
constexpr uint16_t kTagItemCells = 0x0010U;
constexpr uint16_t kTagItemPairs = 0x0011U;
constexpr uint16_t kTagCellRanges = 0x0018U;
constexpr uint16_t kTagDocName = 0x0101U;
constexpr uint16_t kTagDocLabel = 0x0102U;
constexpr uint16_t kTagDocMode = 0x0103U;
constexpr uint16_t kTagDocEnabled = 0x0104U;
constexpr uint16_t kTagDocRevision = 0x0105U;
constexpr uint16_t kTagDocCapacity = 0x0106U;
constexpr uint16_t kTagDocMark = 0x0107U;
constexpr uint16_t kTagItemId = 0x0201U;
constexpr uint16_t kTagItemSigned = 0x0202U;
constexpr uint16_t kTagItemDims = 0x0203U;
constexpr uint16_t kTagItemNote = 0x0204U;
constexpr uint16_t kTagCellId = 0x1001U;
constexpr uint16_t kTagCellKind = 0x1002U;
constexpr uint16_t kTagCellScore = 0x1003U;
constexpr uint16_t kTagPairAddress = 0x1101U;
constexpr uint16_t kTagPairSize = 0x1102U;
constexpr uint16_t kTagRangeBegin = 0x1801U;
constexpr uint16_t kTagRangeEnd = 0x1802U;
constexpr uint16_t kTagUnknown = 0x7F7FU;  // 故意不登记的未知 tag（前向兼容跳过路径）

// 测试枚举：覆盖"枚举按无符号标量 varint 编码"，并故意留未登记值（如 77）给往返用例
enum class TestWireMode : uint32_t { kDefault = 0, kFast = 1, kSlow = 2 };
enum class TestWireKind : uint32_t { kUnknown = 0, kRead = 1, kWrite = 2 };

// mini schema 策略：向框架注入协议身份（magic/version）与 tag 全集（IsKnownTag）
struct TestWireSchema {
  // 魔术字由 ASCII "TEST" 逐字符拼装，小端落盘字节序为 54 53 45 54
  static constexpr uint32_t kMagic = (static_cast<uint32_t>('T') << 24U) | (static_cast<uint32_t>('E') << 16U) |
                                     (static_cast<uint32_t>('S') << 8U) | static_cast<uint32_t>('T');
  static constexpr uint16_t kVersion = 1U;

  static bool IsKnownTag(uint16_t tag) {
    switch (tag) {
      case kTagDoc:
      case kTagItem:
      case kTagItemCells:
      case kTagItemPairs:
      case kTagCellRanges:
      case kTagDocName:
      case kTagDocLabel:
      case kTagDocMode:
      case kTagDocEnabled:
      case kTagDocRevision:
      case kTagDocCapacity:
      case kTagDocMark:
      case kTagItemId:
      case kTagItemSigned:
      case kTagItemDims:
      case kTagItemNote:
      case kTagCellId:
      case kTagCellKind:
      case kTagCellScore:
      case kTagPairAddress:
      case kTagPairSize:
      case kTagRangeBegin:
      case kTagRangeEnd:
        return true;
      default:
        return false;
    }
  }
};

// 记录类以 WireTlvField 成员 + wire::Set/Get/Has/Add 辅助函数声明字段（与 dump schema 副本同构），
// WireTlvField/WireStatus 符号在本 TU 命名空间引入
using wire::WireStatus;
using wire::WireTlvField;

class TestWireRange {
 public:
  void SetBegin(const uint64_t &value) {
    wire::SetWireTlvField(Begin_, value);
  }
  const uint64_t &GetBegin() const {
    return wire::GetWireTlvField(Begin_);
  }
  void SetEnd(const uint64_t &value) {
    wire::SetWireTlvField(End_, value);
  }
  const uint64_t &GetEnd() const {
    return wire::GetWireTlvField(End_);
  }

 private:
  WireTlvField<uint64_t> Begin_;
  WireTlvField<uint64_t> End_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagRangeBegin, Begin_);
    visitor.VisitMember(kTagRangeEnd, End_);
  }
};

class TestWirePair {
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
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagPairAddress, Address_);
    visitor.VisitMember(kTagPairSize, Size_);
  }
};

class TestWireCell {
 public:
  void SetCellId(const uint32_t &value) {
    wire::SetWireTlvField(CellId_, value);
  }
  const uint32_t &GetCellId() const {
    return wire::GetWireTlvField(CellId_);
  }
  void SetKind(const TestWireKind &value) {
    wire::SetWireTlvField(Kind_, value);
  }
  const TestWireKind &GetKind() const {
    return wire::GetWireTlvField(Kind_);
  }
  void SetScore(const int32_t &value) {
    wire::SetWireTlvField(Score_, value);
  }
  const int32_t &GetScore() const {
    return wire::GetWireTlvField(Score_);
  }
  TestWireRange &AddRange() {
    return wire::AddWireTlvItem(Range_);
  }
  size_t GetRangeCount() const {
    return wire::GetWireTlvItemCount(Range_);
  }
  WireStatus GetRange(const size_t index, const TestWireRange *&value) const {
    return wire::GetWireTlvItem(Range_, index, value);
  }

 private:
  WireTlvField<uint32_t> CellId_;
  WireTlvField<TestWireKind> Kind_;
  WireTlvField<int32_t> Score_;
  std::vector<TestWireRange> Range_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagCellId, CellId_);
    visitor.VisitMember(kTagCellKind, Kind_);
    visitor.VisitMember(kTagCellScore, Score_);
    visitor.VisitMember(kTagCellRanges, Range_);
  }
};

class TestWireItem {
 public:
  void SetItemId(const uint32_t &value) {
    wire::SetWireTlvField(ItemId_, value);
  }
  const uint32_t &GetItemId() const {
    return wire::GetWireTlvField(ItemId_);
  }
  void SetSignedVal(const int32_t &value) {
    wire::SetWireTlvField(SignedVal_, value);
  }
  const int32_t &GetSignedVal() const {
    return wire::GetWireTlvField(SignedVal_);
  }
  void SetDims(const std::vector<uint64_t> &value) {
    wire::SetWireTlvField(Dims_, value);
  }
  const std::vector<uint64_t> &GetDims() const {
    return wire::GetWireTlvField(Dims_);
  }
  void SetNote(const std::string &value) {
    wire::SetWireTlvField(Note_, value);
  }
  const std::string &GetNote() const {
    return wire::GetWireTlvField(Note_);
  }
  TestWireCell &AddCell() {
    return wire::AddWireTlvItem(Cell_);
  }
  size_t GetCellCount() const {
    return wire::GetWireTlvItemCount(Cell_);
  }
  WireStatus GetCell(const size_t index, const TestWireCell *&value) const {
    return wire::GetWireTlvItem(Cell_, index, value);
  }
  TestWirePair &AddPair() {
    return wire::AddWireTlvItem(Pair_);
  }
  size_t GetPairCount() const {
    return wire::GetWireTlvItemCount(Pair_);
  }
  WireStatus GetPair(const size_t index, const TestWirePair *&value) const {
    return wire::GetWireTlvItem(Pair_, index, value);
  }

 private:
  WireTlvField<uint32_t> ItemId_;
  WireTlvField<int32_t> SignedVal_;
  WireTlvField<std::vector<uint64_t>> Dims_;
  WireTlvField<std::string> Note_;
  std::vector<TestWireCell> Cell_;
  std::vector<TestWirePair> Pair_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagItemId, ItemId_);
    visitor.VisitMember(kTagItemSigned, SignedVal_);
    visitor.VisitMember(kTagItemDims, Dims_);
    visitor.VisitMember(kTagItemNote, Note_);
    visitor.VisitMember(kTagItemCells, Cell_);
    visitor.VisitMember(kTagItemPairs, Pair_);
  }
};

class TestWireDoc {
 public:
  void SetName(const std::string &value) {
    wire::SetWireTlvField(Name_, value);
  }
  const std::string &GetName() const {
    return wire::GetWireTlvField(Name_);
  }
  void SetLabel(const std::string &value) {
    wire::SetWireTlvField(Label_, value);
  }
  const std::string &GetLabel() const {
    return wire::GetWireTlvField(Label_);
  }
  bool HasLabel() const {
    return wire::HasWireTlvField(Label_);
  }
  void SetMode(const TestWireMode &value) {
    wire::SetWireTlvField(Mode_, value);
  }
  const TestWireMode &GetMode() const {
    return wire::GetWireTlvField(Mode_);
  }
  void SetEnabled(const bool &value) {
    wire::SetWireTlvField(Enabled_, value);
  }
  const bool &GetEnabled() const {
    return wire::GetWireTlvField(Enabled_);
  }
  void SetRevision(const uint32_t &value) {
    wire::SetWireTlvField(Revision_, value);
  }
  const uint32_t &GetRevision() const {
    return wire::GetWireTlvField(Revision_);
  }
  void SetCapacity(const uint64_t &value) {
    wire::SetWireTlvField(Capacity_, value);
  }
  const uint64_t &GetCapacity() const {
    return wire::GetWireTlvField(Capacity_);
  }
  void SetMark(const uint32_t &value) {
    wire::SetWireTlvField(Mark_, value);
  }
  const uint32_t &GetMark() const {
    return wire::GetWireTlvField(Mark_);
  }
  bool HasMark() const {
    return wire::HasWireTlvField(Mark_);
  }

 private:
  WireTlvField<std::string> Name_;
  WireTlvField<std::string, true> Label_;
  WireTlvField<TestWireMode> Mode_;
  WireTlvField<bool> Enabled_;
  WireTlvField<uint32_t> Revision_;
  WireTlvField<uint64_t> Capacity_;
  WireTlvField<uint32_t, true> Mark_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagDocName, Name_);
    visitor.VisitMember(kTagDocLabel, Label_);
    visitor.VisitMember(kTagDocMode, Mode_);
    visitor.VisitMember(kTagDocEnabled, Enabled_);
    visitor.VisitMember(kTagDocRevision, Revision_);
    visitor.VisitMember(kTagDocCapacity, Capacity_);
    visitor.VisitMember(kTagDocMark, Mark_);
  }
};

// 文档容器：顶层 DOC(0x0001) 恰 1 个 + ITEM(0x0002) 0..N 个，编解码范式与参考实现同构（仅机制，无业务）
class TestWireDocument {
 public:
  TestWireDoc &MutableDoc() {
    return doc_;
  }
  const TestWireDoc &GetDoc() const {
    return doc_;
  }
  TestWireItem &AddItem() {
    items_.emplace_back();
    return items_.back();
  }
  size_t GetItemCount() const {
    return items_.size();
  }
  wire::WireStatus GetItem(size_t index, const TestWireItem *&item) const {
    if (index >= items_.size()) {
      return wire::WireStatus::kInvalidParam;
    }
    item = &items_[index];
    return wire::WireStatus::kOk;
  }

  // 顶层写入顺序：公共头（total_size 占位）→ DOC → ITEM*N → 回填 total_size → swap 接管输出缓冲
  wire::WireStatus Serialize(std::vector<uint8_t> &buffer) {
    if (items_.size() >= UINT32_MAX) {
      return wire::WireStatus::kLengthOverflow;
    }
    try {
      wire::WireTlvEncoder encoder;
      wire::WriteCommonHeader<TestWireSchema>(encoder, items_.size() + 1U);
      encoder.WriteRecord(kTagDoc, doc_);
      encoder.VisitMember(kTagItem, items_);
      if (encoder.status != wire::WireStatus::kOk) {
        return encoder.status;  // 失败时不触碰 buffer：无部分提交
      }
      wire::FillWireTlvTotalSize(encoder.bytes);
      buffer.swap(encoder.bytes);
      return wire::WireStatus::kOk;
    } catch (const std::bad_alloc &) {
      return wire::WireStatus::kNoMemory;
    } catch (const std::length_error &) {
      return wire::WireStatus::kLengthOverflow;
    }
  }

  // 解码到局部对象，全部校验通过后才交换提交；任何失败不影响调用者可见状态（无部分提交）
  wire::WireStatus Deserialize(const uint8_t *data, size_t size) {
    try {
      TestWireDocument parsed;
      uint64_t count = 0U;
      bool has_doc = false;
      const auto status = wire::DecodeWireTlvRecords<TestWireSchema>(
          data, size, [&parsed, &count, &has_doc](uint16_t tag, const uint8_t *payload, size_t payload_size) {
            ++count;
            if (tag == kTagDoc) {
              if (has_doc) {
                return wire::WireStatus::kInvalidFormat;  // 顶层 DOC 必须恰 1 个
              }
              has_doc = true;
              return wire::WireTlvDecodeRecord<TestWireSchema>(payload, payload_size, parsed.doc_);
            }
            if (tag == kTagItem) {
              return wire::WireTlvDecodeRecord<TestWireSchema>(payload, payload_size, parsed.AddItem());
            }
            if (TestWireSchema::IsKnownTag(tag)) {
              return wire::WireStatus::kInvalidFormat;  // 已知子级 tag 漏到顶层 = 层次非法
            }
            return wire::WireStatus::kOk;  // 未知 tag 跳过（前向兼容）
          });
      if (status != wire::WireStatus::kOk) {
        return status;
      }
      if (!has_doc || (count != wire::GetRecordCountFromHeader(data, size))) {
        return wire::WireStatus::kInvalidFormat;
      }
      std::swap(doc_, parsed.doc_);
      items_.swap(parsed.items_);
      return wire::WireStatus::kOk;
    } catch (const std::bad_alloc &) {
      return wire::WireStatus::kNoMemory;
    } catch (const std::length_error &) {
      return wire::WireStatus::kLengthOverflow;
    }
  }

 private:
  TestWireDoc doc_;
  std::vector<TestWireItem> items_;
};

// ----------------------------------------------
// 独立 golden 构造器：绝不调用被测的 WireTlvEncoder/Serialize
// ----------------------------------------------

// 定宽小端字节（公共头字段、tag/length 帧头）
Bytes Le(uint64_t value, size_t width) {
  Bytes bytes;
  for (size_t i = 0U; i < width; ++i) {
    bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    value >>= 8U;
  }
  return bytes;
}

// 规范 LEB128 varint（无符号值编码）
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

// 有符号值（含维度的"-1 表示未知维"语义）先 zigzag 折叠再 varint
Bytes ZzVarint(int64_t value) {
  return Varint((static_cast<uint64_t>(value) << 1U) ^ static_cast<uint64_t>(value >> 63));
}

void Append(Bytes &to, const Bytes &from) {
  to.insert(to.end(), from.begin(), from.end());
}

// string 字段线上形态是裸字节，长度由所在记录 length 给出
Bytes Str(const std::string &text) {
  return Bytes(text.begin(), text.end());
}

// 按小端覆写已有缓冲的定宽字段（构造畸形样本用）
void PokeLe(Bytes &bytes, size_t offset, uint64_t value, size_t width) {
  for (size_t i = 0U; i < width; ++i) {
    bytes[offset + i] = static_cast<uint8_t>((value >> (i * 8U)) & 0xFFU);
  }
}

// 记录帧：tag:u16 + length:u16（定宽小端）+ 负载
Bytes Tlv(uint16_t tag, const Bytes &payload) {
  Bytes result = Le(tag, 2U);
  Append(result, Le(payload.size(), 2U));
  Append(result, payload);
  return result;
}

// 完整线流：16B 公共头（magic|version|header_size|total_size|record_count）+ 顶层记录序列
Bytes Wire(const Bytes &records, uint32_t count) {
  Bytes result = Le(TestWireSchema::kMagic, 4U);
  Append(result, Le(TestWireSchema::kVersion, 2U));
  Append(result, Le(wire::kHeaderSize, 2U));
  Append(result, Le(0U, 4U));  // total_size 占位，尾部回填
  Append(result, Le(count, 4U));
  Append(result, records);
  PokeLe(result, 8U, result.size(), 4U);
  return result;
}

// 单 DOC 文档样本（畸形值编码用例的载体）
Bytes DocWire(const Bytes &doc_payload) {
  return Wire(Tlv(kTagDoc, doc_payload), 1U);
}

// DOC + ITEM 双记录样本
Bytes ItemWire(const Bytes &item_payload) {
  Bytes records = Tlv(kTagDoc, {});
  Append(records, Tlv(kTagItem, item_payload));
  return Wire(records, 2U);
}

// ----------------------------------------------
// 测试支撑 helper
// ----------------------------------------------

// 序列化必须成功；失败即测试失败并返回已产出字节
Bytes SerializeOk(TestWireDocument &doc) {
  Bytes bytes;
  if (doc.Serialize(bytes) != wire::WireStatus::kOk) {
    ADD_FAILURE() << "Serialize should succeed";
  }
  return bytes;
}

// 取顶层第一条指定 tag 记录的负载（按帧头 length 切出）
bool FirstRecordPayload(const Bytes &bytes, uint16_t tag, Bytes &payload) {
  if (bytes.size() < wire::kHeaderSize) {
    return false;
  }
  bool found = false;
  wire::ProcessRecords(bytes.data() + wire::kHeaderSize, bytes.size() - wire::kHeaderSize,
                       [&](uint16_t record_tag, const uint8_t *data, size_t data_size) {
                         if (!found && (record_tag == tag)) {
                           payload.assign(data, data + data_size);
                           found = true;
                         }
                         return wire::WireStatus::kOk;
                       });
  return found;
}

// 用框架帧切分器扫出一条记录负载内的字段 tag 序列（只读扫描，不构造期望值）
TagList PayloadTags(const Bytes &payload) {
  TagList tags;
  wire::ProcessRecords(payload.data(), payload.size(), [&tags](uint16_t tag, const uint8_t *, size_t) {
    tags.push_back(tag);
    return wire::WireStatus::kOk;
  });
  return tags;
}

bool HasTag(const TagList &tags, uint16_t tag) {
  return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

// ----------------------------------------------
// canonical golden 逐条推导（golden 字节比对与解码字段校验两个用例共用）
// 帧头 tag:u16 + len:u16 定宽小端；标量 = 规范 LEB128 varint（有符号先 zigzag）；bool = 定宽 1B；
// string = 裸字节；vector<uint64_t> = count varint + 每元素 zigzag varint（-1 表示未知维）。
// DOC 记录（tag=0x0001）负载 46B：
//   Name "abc"          0x0101 len=3  裸字节 "abc"                → 7B
//   Label ""            0x0102 len=0  EXPLICIT 空串默认值也上线    → 4B
//   Mode kSlow(2)       0x0103 len=1  varint(2)={0x02}            → 5B
//   Enabled true        0x0104 len=1  bool 定宽 {0x01}            → 5B
//   Revision 300        0x0105 len=2  varint(300)={0xAC,0x02}     → 6B
//   Capacity 2^63       0x0106 len=10 varint(2^63)={0x80*9,0x01}  → 14B
//   Mark 0              0x0107 len=1  EXPLICIT varint(0)={0x00}    → 5B
// ITEM 记录（tag=0x0002）负载 71B：
//   ItemId 1            0x0201 len=1  varint(1)                   → 5B
//   SignedVal -7        0x0202 len=1  zigzag(-7)=13={0x0D}        → 5B
//   Dims [4,-1,3]       0x0203 len=4  count={0x03}+zz(4)={0x08}+zz(-1)={0x01}(未知维单字节)+zz(3)={0x06} → 8B
//   Note "x"            0x0204 len=1  裸字节                       → 5B
//   CELL(9,kRead,5,RANGE[1,2]) 0x0010 负载 29B(5+5+5+14)           → 33B
//     其中 RANGE 0x0018 负载 10B = Begin(5B)+End(5B)               → 14B
//   PAIR(4096,8)        0x0011 负载 11B = varint(4096)={0x80,0x20}(6B)+varint(8)(5B) → 15B
// 总长 = 16 头 + (4+46) DOC + (4+71) ITEM = 141B，record_count = 2
// ----------------------------------------------
Bytes BuildGoldenDocPayload() {
  Bytes payload = Tlv(kTagDocName, Str("abc"));
  Append(payload, Tlv(kTagDocLabel, {}));
  Append(payload, Tlv(kTagDocMode, Varint(2U)));
  Append(payload, Tlv(kTagDocEnabled, Bytes{1U}));
  Append(payload, Tlv(kTagDocRevision, Varint(300U)));
  Append(payload, Tlv(kTagDocCapacity, Varint(1ULL << 63)));
  Append(payload, Tlv(kTagDocMark, Varint(0U)));
  return payload;  // 46B
}

Bytes BuildGoldenItemPayload() {
  Bytes range = Tlv(kTagRangeBegin, Varint(1U));
  Append(range, Tlv(kTagRangeEnd, Varint(2U)));  // RANGE 负载 10B
  Bytes cell = Tlv(kTagCellId, Varint(9U));
  Append(cell, Tlv(kTagCellKind, Varint(1U)));    // 枚举 kRead 按无符号 varint
  Append(cell, Tlv(kTagCellScore, ZzVarint(5)));  // i32 正值 zigzag(5)=10
  Append(cell, Tlv(kTagCellRanges, range));       // CELL 负载 29B
  Bytes pair = Tlv(kTagPairAddress, Varint(4096U));
  Append(pair, Tlv(kTagPairSize, Varint(8U)));  // PAIR 负载 11B
  Bytes item = Tlv(kTagItemId, Varint(1U));
  Append(item, Tlv(kTagItemSigned, ZzVarint(-7)));  // 负数 zigzag(-7)=13 → 单字节
  Bytes dims = Varint(3U);                          // 维度 count=3
  Append(dims, ZzVarint(4));
  Append(dims, ZzVarint(-1));  // -1（存储为 UINT64_MAX）→ zigzag=1 单字节
  Append(dims, ZzVarint(3));
  Append(item, Tlv(kTagItemDims, dims));  // 维度负载 4B
  Append(item, Tlv(kTagItemNote, Str("x")));
  Append(item, Tlv(kTagItemCells, cell));
  Append(item, Tlv(kTagItemPairs, pair));
  return item;  // 71B
}

Bytes BuildCanonicalGolden() {
  Bytes records = Tlv(kTagDoc, BuildGoldenDocPayload());     // DOC 记录 50B
  Append(records, Tlv(kTagItem, BuildGoldenItemPayload()));  // ITEM 记录 75B
  return Wire(records, 2U);                                  // 141B
}

void BuildCanonicalDoc(TestWireDocument &doc) {
  auto &d = doc.MutableDoc();
  d.SetName("abc");
  d.SetLabel("");  // EXPLICIT 空串：默认值也要上线
  d.SetMode(TestWireMode::kSlow);
  d.SetEnabled(true);
  d.SetRevision(300U);
  d.SetCapacity(1ULL << 63);
  d.SetMark(0U);  // EXPLICIT 0：默认值也要上线
  auto &item = doc.AddItem();
  item.SetItemId(1U);
  item.SetSignedVal(-7);
  item.SetDims({4U, UINT64_MAX, 3U});  // UINT64_MAX 即维度语义的 -1（未知维）
  item.SetNote("x");
  auto &cell = item.AddCell();
  cell.SetCellId(9U);
  cell.SetKind(TestWireKind::kRead);
  cell.SetScore(5);
  auto &range = cell.AddRange();
  range.SetBegin(1U);
  range.SetEnd(2U);
  auto &pair = item.AddPair();
  pair.SetAddress(4096U);
  pair.SetSize(8U);
}

// ----------------------------------------------
// implicit 默认值裁剪用例与嵌套往返用例的文档构造 helper
// ----------------------------------------------

// implicit 字段全部 Set 成类型默认值（0/false/""/空维度）
void SetImplicitDefaults(TestWireDocument &doc) {
  auto &d = doc.MutableDoc();
  d.SetName("");
  d.SetMode(TestWireMode::kDefault);
  d.SetEnabled(false);
  d.SetRevision(0U);
  d.SetCapacity(0U);
  auto &item = doc.AddItem();
  item.SetItemId(0U);
  item.SetSignedVal(0);
  item.SetDims({});
  item.SetNote("");
}

// 对照组：同样的字段 Set 成非默认值后必须上线
void SetImplicitNonDefaults(TestWireDocument &doc) {
  auto &d = doc.MutableDoc();
  d.SetName("n");
  d.SetEnabled(true);
  d.SetRevision(7U);
  auto &item = doc.AddItem();
  item.SetItemId(3U);
  item.SetSignedVal(-1);
  item.SetDims({2U});
}

// 全字段 + 多元素集合 + 三层嵌套（ITEM→CELL→RANGE、ITEM→PAIR）+ 边界值 + 未登记枚举
void BuildNestedDoc(TestWireDocument &doc) {
  auto &d = doc.MutableDoc();
  d.SetName("doc-name");
  d.SetLabel("");  // EXPLICIT 默认值
  d.SetMode(TestWireMode::kFast);
  d.SetEnabled(true);
  d.SetRevision(UINT32_MAX);  // u32 上边界
  d.SetCapacity(UINT64_MAX);  // u64 上边界 → 10B varint
  d.SetMark(0U);              // EXPLICIT 默认值
  auto &item0 = doc.AddItem();
  item0.SetItemId(UINT32_MAX);
  item0.SetSignedVal(INT32_MIN);  // zigzag 下边界
  item0.SetDims({7U, UINT64_MAX, 0U});
  item0.SetNote("note-0");
  auto &cell0 = item0.AddCell();
  cell0.SetCellId(1U);
  cell0.SetKind(static_cast<TestWireKind>(77));  // 未登记枚举值，应原样保留
  cell0.SetScore(-1);
  auto &range0 = cell0.AddRange();
  range0.SetBegin(10U);
  range0.SetEnd(20U);
  auto &range1 = cell0.AddRange();
  range1.SetBegin(0U);  // implicit 默认值不上线，往返后仍应还原为 0
  range1.SetEnd(UINT64_MAX);
  auto &cell1 = item0.AddCell();
  cell1.SetCellId(2U);
  cell1.SetKind(TestWireKind::kWrite);
  cell1.SetScore(INT32_MAX);  // zigzag 上边界
  item0.AddPair();            // 全默认元素：空 PAIR 记录也应保留（集合计数不变）
  auto &pair1 = item0.AddPair();
  pair1.SetAddress(UINT64_MAX);
  pair1.SetSize(1U);
  doc.AddItem();  // 第二个 ITEM：全默认空记录
}

// ----------------------------------------------
// 畸形样本表与共用断言（三个拒绝用例各取一类样本，RejectOneMalformed 统一循环断言）
// ----------------------------------------------
struct MalformedSample {
  const char *desc;
  wire::WireStatus expected;
  Bytes bytes;
};
using Samples = std::vector<MalformedSample>;

// 公共头五道门：magic、总长、header_size、total_size、version
void AddHeaderSamples(Samples &samples) {
  Bytes bad_magic = Wire(Tlv(kTagDoc, {}), 1U);
  bad_magic[0] = static_cast<uint8_t>(bad_magic[0] ^ 0xFFU);
  samples.push_back({"magic 不符", wire::WireStatus::kInvalidFormat, bad_magic});
  Bytes too_short = Wire(Tlv(kTagDoc, {}), 1U);
  too_short.resize(wire::kHeaderSize - 1U);  // 总长不足 16B
  samples.push_back({"总长不足公共头", wire::WireStatus::kInvalidFormat, too_short});
  Bytes bad_header = Wire(Tlv(kTagDoc, {}), 1U);
  PokeLe(bad_header, 6U, wire::kHeaderSize + 1U, 2U);
  samples.push_back({"header_size 非 16", wire::WireStatus::kInvalidFormat, bad_header});
  Bytes bad_total = Wire(Tlv(kTagDoc, {}), 1U);
  PokeLe(bad_total, 8U, bad_total.size() - 1U, 4U);
  samples.push_back({"total_size 与实际长度不符", wire::WireStatus::kInvalidFormat, bad_total});
  Bytes bad_version = Wire(Tlv(kTagDoc, {}), 1U);
  PokeLe(bad_version, 4U, TestWireSchema::kVersion + 1U, 2U);
  samples.push_back({"版本不支持", wire::WireStatus::kUnsupportedVersion, bad_version});
}

// 帧切分与顶层层次：帧头截断、length 越界（总长/父记录）、已知 tag 非法层次、DOC 恰一个、count 一致
void AddFrameSamples(Samples &samples) {
  Bytes truncated = Wire(Tlv(kTagDoc, {}), 1U);
  truncated.push_back(0xFFU);
  truncated.push_back(0x00U);                   // 尾部残留 2B：记录帧头（4B）不完整
  PokeLe(truncated, 8U, truncated.size(), 4U);  // 同步 total_size，隔离出帧切分错误
  samples.push_back({"记录帧头截断", wire::WireStatus::kInvalidFormat, truncated});
  Bytes over = Tlv(kTagDoc, {});
  PokeLe(over, 2U, 9U, 2U);  // length=9 越出总长边界
  samples.push_back({"记录 length 越总长边界", wire::WireStatus::kInvalidFormat, Wire(over, 1U)});
  Bytes inner = Tlv(kTagDocName, {});
  PokeLe(inner, 2U, 9U, 2U);  // DOC 内部字段记录 length 越父记录
  samples.push_back({"内部记录 length 越父记录", wire::WireStatus::kInvalidFormat, Wire(Tlv(kTagDoc, inner), 1U)});
  samples.push_back({"已知字段 tag 出现在顶层", wire::WireStatus::kInvalidFormat, Wire(Tlv(kTagDocName, {}), 1U)});
  Bytes wrong_level = Tlv(kTagCellId, Varint(1U));  // CELL 的字段 tag 出现在 DOC 记录内
  samples.push_back({"已知字段 tag 层次非法", wire::WireStatus::kInvalidFormat, Wire(Tlv(kTagDoc, wrong_level), 1U)});
  Bytes dup = Tlv(kTagDoc, {});
  Append(dup, Tlv(kTagDoc, {}));
  samples.push_back({"顶层重复 DOC", wire::WireStatus::kInvalidFormat, Wire(dup, 2U)});
  samples.push_back({"顶层缺 DOC", wire::WireStatus::kInvalidFormat, Wire(Tlv(kTagItem, {}), 1U)});
  Bytes count_mismatch = Wire(Tlv(kTagDoc, {}), 2U);  // 实际 1 条记录，头部声明 2 条
  samples.push_back({"record_count 与实际不符", wire::WireStatus::kInvalidFormat, count_mismatch});
}

// 值编码：varint 四道校验、bool 定宽与值域、u32 值域、维度 count/元素/精确消耗
void AddValueSamples(Samples &samples) {
  samples.push_back(
      {"varint 超 10 字节", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocRevision, Bytes(11U, 0x81U)))});
  Bytes tenth = Bytes(9U, 0xFFU);
  tenth.push_back(0x02U);  // 第 10 字节（shift==63）负载 = 2 > 1 → 移位溢出保护
  samples.push_back(
      {"varint 第 10 字节负载超 1", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocRevision, tenth))});
  samples.push_back(
      {"varint overlong 编码", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocRevision, {0x80U, 0x00U}))});
  samples.push_back(
      {"标量未被恰好消耗", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocRevision, {0x05U, 0x05U}))});
  samples.push_back({"bool 负载长度非 1（空）", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocEnabled, {}))});
  samples.push_back(
      {"bool 负载长度非 1（2B）", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocEnabled, {0x01U, 0x00U}))});
  samples.push_back({"bool 字节值大于 1", wire::WireStatus::kInvalidFormat, DocWire(Tlv(kTagDocEnabled, {0x02U}))});
  samples.push_back({"uint32 字段 varint 超值域", wire::WireStatus::kInvalidFormat,
                     DocWire(Tlv(kTagDocRevision, Varint(0x1FFFFFFFFULL)))});
  samples.push_back({"int32 字段 zigzag 超值域", wire::WireStatus::kInvalidFormat,
                     ItemWire(Tlv(kTagItemSigned, ZzVarint(static_cast<int64_t>(INT32_MAX) + 1)))});
  samples.push_back({"维度 count 超剩余字节", wire::WireStatus::kLengthOverflow, ItemWire(Tlv(kTagItemDims, {0x7FU}))});
  samples.push_back(
      {"维度元素 varint 截断", wire::WireStatus::kInvalidFormat, ItemWire(Tlv(kTagItemDims, {0x01U, 0x80U}))});
  samples.push_back(
      {"维度负载未被恰好消耗", wire::WireStatus::kInvalidFormat, ItemWire(Tlv(kTagItemDims, {0x01U, 0x08U, 0x09U}))});
}
// 畸形样本统一断言（1:1 迁移自原用例循环体）：错误码与预期相符，且原对象与输出缓冲均未被修改（无部分提交）
void RejectOneMalformed(const Samples &samples) {
  for (const auto &sample : samples) {
    SCOPED_TRACE(sample.desc);
    TestWireDocument doc;  // 预先 Set 特征值：失败后必须原样保留（无部分提交）
    doc.MutableDoc().SetName("sentinel");
    doc.AddItem().SetItemId(7U);
    Bytes input = sample.bytes;  // 可变副本兼作输出缓冲哨兵：失败后必须未被改写
    EXPECT_EQ(doc.Deserialize(input.data(), input.size()), sample.expected);
    EXPECT_EQ(input, sample.bytes);
    EXPECT_EQ(doc.GetItemCount(), 1U);
    EXPECT_EQ(doc.GetDoc().GetName(), "sentinel");
    EXPECT_FALSE(doc.GetDoc().HasLabel());
    const TestWireItem *item = nullptr;
    ASSERT_EQ(doc.GetItem(0U, item), wire::WireStatus::kOk);
    EXPECT_EQ(item->GetItemId(), 7U);
    EXPECT_EQ(item->GetCellCount(), 0U);
  }
}
}  // namespace

// 场景：空文档（无 ITEM、DOC 字段全不设）。校验 16B 公共头 + 空 DOC 帧头共 20B 的 golden 与各头字段取值。
TEST(WireTlvFrameTest, WireTlvFrame_ShouldMatchGoldenHeader_WhenDocumentEmpty) {
  // 空文档 = 16B 公共头 + 空 DOC 记录（tag=0x0001, length=0，帧头 4B）= 20B，record_count=1。
  // 手写字面量与独立构造器结果互验，再逐字段断言公共头（全部定宽小端）。
  const Bytes golden = Wire(Tlv(kTagDoc, {}), 1U);
  const Bytes literal = {0x54, 0x53, 0x45, 0x54, 1, 0, 16, 0, 20, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  EXPECT_EQ(golden, literal);
  TestWireDocument doc;
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);  // 被测编码与独立 golden 逐字节一致
  ASSERT_EQ(bytes.size(), 20U);
  EXPECT_EQ(wire::ReadLe(bytes.data(), 4U), TestWireSchema::kMagic);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 4U, 2U), TestWireSchema::kVersion);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 6U, 2U), wire::kHeaderSize);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 8U, 4U), 20U);  // total_size == 实际长度
  EXPECT_EQ(wire::ReadLe(bytes.data() + 12U, 4U), 1U);  // record_count == 1（仅 DOC）
  EXPECT_EQ(wire::CheckWireTlvHeader<TestWireSchema>(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  EXPECT_EQ(wire::GetRecordCountFromHeader(bytes.data(), bytes.size()), 1U);
  // 入参非法（空指针/长度不足公共头）时返回 0，由调用方按记录数不符判错，不做越界读
  EXPECT_EQ(wire::GetRecordCountFromHeader(nullptr, 0U), 0U);
  EXPECT_EQ(wire::GetRecordCountFromHeader(bytes.data(), wire::kHeaderSize - 1U), 0U);
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  EXPECT_EQ(parsed.GetItemCount(), 0U);
}

// 场景：FillWireTlvTotalSize 的防御边界——缓冲不足 16B 公共头（编码中途已失败）时不回填、避免越界写；
// 恰为 16B 时正常回填 total_size。与上文 GetRecordCountFromHeader 的入参防御同批加固
TEST(WireTlvFrameTest, WireTlvHelper_ShouldNotWriteBackTotalSize_WhenBufferShorterThanHeader) {
  Bytes short_buf(wire::kHeaderSize - 1U, 0xA5U);
  const Bytes sentinel = short_buf;
  wire::FillWireTlvTotalSize(short_buf);
  EXPECT_EQ(short_buf, sentinel);
  Bytes header_buf(wire::kHeaderSize, 0x5AU);
  wire::FillWireTlvTotalSize(header_buf);
  EXPECT_EQ(wire::ReadLe(header_buf.data() + 8U, 4U), wire::kHeaderSize);
}

// 场景：覆盖全部值编码（裸字节字符串、varint、zigzag 负数、bool 定宽、维度集合、explicit 默认值），逐字节比对 golden
TEST(WireTlvFrameTest, WireTlvField_ShouldMatchGoldenBytes_WhenEachValueTypeEncoded) {
  TestWireDocument doc;
  BuildCanonicalDoc(doc);
  // golden 逐条推导见 BuildCanonicalGolden 上方注释块（每条记录 tag/length/值编码/记录字节数/总长）
  const Bytes golden = BuildCanonicalGolden();
  ASSERT_EQ(golden.size(), 141U);  // 16 头 + 50 DOC(4+46) + 75 ITEM(4+71)
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);  // 被测编码与独立 golden 逐字节一致
}

// 场景：解码 141B 手工 golden 字节流，逐字段还原有符号 zigzag、explicit 默认值 presence 与三层嵌套集合。
TEST(WireTlvFrameTest, WireTlvField_ShouldDecodeGoldenFields_WhenSignedAndExplicitDefaults) {
  const Bytes golden = BuildCanonicalGolden();  // golden 逐条推导见上方构造器注释块
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(golden.data(), golden.size()), wire::WireStatus::kOk);
  const auto &d = parsed.GetDoc();
  EXPECT_EQ(d.GetName(), "abc");
  EXPECT_TRUE(d.HasLabel());
  EXPECT_EQ(d.GetLabel(), "");
  EXPECT_EQ(d.GetMode(), TestWireMode::kSlow);
  EXPECT_TRUE(d.GetEnabled());
  EXPECT_EQ(d.GetRevision(), 300U);
  EXPECT_EQ(d.GetCapacity(), 1ULL << 63);
  EXPECT_TRUE(d.HasMark());
  EXPECT_EQ(d.GetMark(), 0U);
  ASSERT_EQ(parsed.GetItemCount(), 1U);
  const TestWireItem *item = nullptr;
  ASSERT_EQ(parsed.GetItem(0U, item), wire::WireStatus::kOk);
  EXPECT_EQ(item->GetItemId(), 1U);
  EXPECT_EQ(item->GetSignedVal(), -7);
  const std::vector<uint64_t> expect_dims = {4U, UINT64_MAX, 3U};
  EXPECT_EQ(item->GetDims(), expect_dims);
  EXPECT_EQ(item->GetNote(), "x");
  ASSERT_EQ(item->GetCellCount(), 1U);
  const TestWireCell *cell = nullptr;
  ASSERT_EQ(item->GetCell(0U, cell), wire::WireStatus::kOk);
  EXPECT_EQ(cell->GetCellId(), 9U);
  EXPECT_EQ(cell->GetKind(), TestWireKind::kRead);
  EXPECT_EQ(cell->GetScore(), 5);
  ASSERT_EQ(cell->GetRangeCount(), 1U);
  const TestWireRange *range = nullptr;
  ASSERT_EQ(cell->GetRange(0U, range), wire::WireStatus::kOk);
  EXPECT_EQ(range->GetBegin(), 1U);
  EXPECT_EQ(range->GetEnd(), 2U);
  ASSERT_EQ(item->GetPairCount(), 1U);
  const TestWirePair *pair = nullptr;
  ASSERT_EQ(item->GetPair(0U, pair), wire::WireStatus::kOk);
  EXPECT_EQ(pair->GetAddress(), 4096U);
  EXPECT_EQ(pair->GetSize(), 8U);
}

// 场景：implicit 字段全部 Set 成类型默认值。载荷与“完全不 Set”逐字节一致，记录内不出现任何字段 tag。
TEST(WireTlvFrameTest, WireTlvField_ShouldOmitDefaults_WhenImplicitFieldsSetToDefault) {
  TestWireDocument base;  // 基线：结构相同（DOC + 1 个 ITEM）但完全不 Set 任何字段
  base.AddItem();
  const Bytes base_bytes = SerializeOk(base);
  TestWireDocument defaults;
  SetImplicitDefaults(defaults);  // implicit 字段全部 Set 成类型默认值（0/false/""/空维度）
  const Bytes default_bytes = SerializeOk(defaults);
  EXPECT_EQ(default_bytes, base_bytes);  // 体积与内容与"完全不 Set"逐字节一致 → 默认值不上线
  Bytes doc_payload;
  ASSERT_TRUE(FirstRecordPayload(default_bytes, kTagDoc, doc_payload));
  EXPECT_TRUE(PayloadTags(doc_payload).empty());  // DOC 记录内没有任何字段 tag
  Bytes item_payload;
  ASSERT_TRUE(FirstRecordPayload(default_bytes, kTagItem, item_payload));
  EXPECT_TRUE(PayloadTags(item_payload).empty());
  TestWireDocument contrast;  // 对照组：Set 成非默认值后 tag 立即出现
  SetImplicitNonDefaults(contrast);
  const Bytes contrast_bytes = SerializeOk(contrast);
  EXPECT_GT(contrast_bytes.size(), base_bytes.size());
  Bytes contrast_doc;
  ASSERT_TRUE(FirstRecordPayload(contrast_bytes, kTagDoc, contrast_doc));
  const TagList doc_tags = PayloadTags(contrast_doc);
  EXPECT_TRUE(HasTag(doc_tags, kTagDocName));
  EXPECT_TRUE(HasTag(doc_tags, kTagDocEnabled));
  EXPECT_TRUE(HasTag(doc_tags, kTagDocRevision));
  EXPECT_FALSE(HasTag(doc_tags, kTagDocLabel));  // 未 Set 的 EXPLICIT 字段不上线
  EXPECT_FALSE(HasTag(doc_tags, kTagDocMode));   // 未 Set 的 implicit 字段不上线
  Bytes contrast_item;
  ASSERT_TRUE(FirstRecordPayload(contrast_bytes, kTagItem, contrast_item));
  const TagList item_tags = PayloadTags(contrast_item);
  EXPECT_TRUE(HasTag(item_tags, kTagItemId));
  EXPECT_TRUE(HasTag(item_tags, kTagItemSigned));
  EXPECT_TRUE(HasTag(item_tags, kTagItemDims));
}

// 场景：explicit 字段 Set 成 0/空串默认值仍上线（两份载荷字节差 + tag 集合双重证据），implicit 默认值不上线。
TEST(WireTlvFrameTest, WireTlvField_ShouldEncodeExplicitDefaultsOnWire_WhenSetToDefaultValue) {
  TestWireDocument bare;
  const Bytes bare_bytes = SerializeOk(bare);  // EXPLICIT 未 Set：不上线（20B 空文档）
  TestWireDocument explicit_defaults;
  explicit_defaults.MutableDoc().SetLabel("");  // EXPLICIT 空串默认值
  explicit_defaults.MutableDoc().SetMark(0U);   // EXPLICIT u32 默认值
  explicit_defaults.MutableDoc().SetName("");   // implicit 对照：默认值不上线
  const Bytes set_bytes = SerializeOk(explicit_defaults);
  // Label 空串记录 4B（len=0）+ Mark 记录 5B（varint(0) 1B）→ 比基线大 9B
  EXPECT_EQ(set_bytes.size(), bare_bytes.size() + 9U);
  Bytes doc_payload;
  ASSERT_TRUE(FirstRecordPayload(set_bytes, kTagDoc, doc_payload));
  const TagList tags = PayloadTags(doc_payload);
  EXPECT_TRUE(HasTag(tags, kTagDocLabel));  // EXPLICIT 默认值确实上线（字节差 + tag 集合双重证据）
  EXPECT_TRUE(HasTag(tags, kTagDocMark));
  EXPECT_FALSE(HasTag(tags, kTagDocName));  // implicit 默认值不上线 → 体现两类 presence 差异
}

// 场景：解码 explicit 默认值记录后 Has* 为真、Get* 还原；未 Set 的 explicit 字段 Has* 为假、Get* 恒返回类型默认值。
TEST(WireTlvFrameTest, WireTlvField_ShouldReportPresenceAndDefaults_WhenExplicitDefaultsDecoded) {
  TestWireDocument bare;
  const Bytes bare_bytes = SerializeOk(bare);  // EXPLICIT 未 Set：不上线（20B 空文档）
  TestWireDocument explicit_defaults;
  explicit_defaults.MutableDoc().SetLabel("");  // EXPLICIT 空串默认值
  explicit_defaults.MutableDoc().SetMark(0U);   // EXPLICIT u32 默认值
  explicit_defaults.MutableDoc().SetName("");   // implicit 对照：默认值不上线
  const Bytes set_bytes = SerializeOk(explicit_defaults);
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(set_bytes.data(), set_bytes.size()), wire::WireStatus::kOk);
  EXPECT_TRUE(parsed.GetDoc().HasLabel());  // 解码后 presence 还原：Set 过默认值 → Has* 为 true
  EXPECT_EQ(parsed.GetDoc().GetLabel(), "");
  EXPECT_TRUE(parsed.GetDoc().HasMark());
  EXPECT_EQ(parsed.GetDoc().GetMark(), 0U);
  TestWireDocument parsed_bare;
  ASSERT_EQ(parsed_bare.Deserialize(bare_bytes.data(), bare_bytes.size()), wire::WireStatus::kOk);
  EXPECT_FALSE(parsed_bare.GetDoc().HasLabel());   // 未 Set → Has* 为 false
  EXPECT_EQ(parsed_bare.GetDoc().GetLabel(), "");  // 无值时 Get* 恒返回类型默认值
  EXPECT_FALSE(parsed_bare.GetDoc().HasMark());
  EXPECT_EQ(parsed_bare.GetDoc().GetMark(), 0U);
}

// 场景：全字段嵌套样本往返后，DOC 全字段（explicit 默认值 presence、u32/u64 上边界）与 ITEM 标量、集合计数逐一还原。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRoundTripDocAndItemScalars_WhenNestedCollectionsAndUnknownEnum) {
  TestWireDocument source;
  BuildNestedDoc(source);
  Bytes bytes = SerializeOk(source);
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  const TestWireDoc &d = parsed.GetDoc();
  EXPECT_EQ(d.GetName(), "doc-name");
  EXPECT_TRUE(d.HasLabel());
  EXPECT_EQ(d.GetLabel(), "");
  EXPECT_EQ(d.GetMode(), TestWireMode::kFast);
  EXPECT_TRUE(d.GetEnabled());
  EXPECT_EQ(d.GetRevision(), UINT32_MAX);
  EXPECT_EQ(d.GetCapacity(), UINT64_MAX);
  EXPECT_TRUE(d.HasMark());
  EXPECT_EQ(d.GetMark(), 0U);
  ASSERT_EQ(parsed.GetItemCount(), 2U);
  const TestWireItem *item = nullptr;
  ASSERT_EQ(parsed.GetItem(0U, item), wire::WireStatus::kOk);
  EXPECT_EQ(item->GetItemId(), UINT32_MAX);
  EXPECT_EQ(item->GetSignedVal(), INT32_MIN);
  const std::vector<uint64_t> dims = {7U, UINT64_MAX, 0U};
  EXPECT_EQ(item->GetDims(), dims);
  EXPECT_EQ(item->GetNote(), "note-0");
  const TestWireItem *empty = nullptr;
  ASSERT_EQ(parsed.GetItem(1U, empty), wire::WireStatus::kOk);
  EXPECT_EQ(empty->GetItemId(), 0U);
  EXPECT_EQ(empty->GetCellCount(), 0U);
  EXPECT_EQ(empty->GetPairCount(), 0U);
  EXPECT_TRUE(empty->GetDims().empty());
}

// 场景：三层嵌套（ITEM→CELL→RANGE、ITEM→PAIR）与全默认元素往返保持，未登记枚举原样保留，解码后再编码字节一致。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRoundTripNestedAndReencodeIdentical_WhenDeepNestingAndUnknownEnum) {
  TestWireDocument source;
  BuildNestedDoc(source);
  Bytes bytes = SerializeOk(source);
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  const TestWireItem *item = nullptr;
  ASSERT_EQ(parsed.GetItem(0U, item), wire::WireStatus::kOk);
  ASSERT_EQ(item->GetCellCount(), 2U);
  ASSERT_EQ(item->GetPairCount(), 2U);
  const TestWireCell *cell = nullptr;
  ASSERT_EQ(item->GetCell(0U, cell), wire::WireStatus::kOk);
  EXPECT_EQ(cell->GetCellId(), 1U);
  EXPECT_EQ(cell->GetKind(), static_cast<TestWireKind>(77));  // 未登记枚举原样保留
  EXPECT_EQ(cell->GetScore(), -1);
  ASSERT_EQ(cell->GetRangeCount(), 2U);
  const TestWireRange *range = nullptr;
  ASSERT_EQ(cell->GetRange(0U, range), wire::WireStatus::kOk);
  EXPECT_EQ(range->GetBegin(), 10U);
  EXPECT_EQ(range->GetEnd(), 20U);
  ASSERT_EQ(cell->GetRange(1U, range), wire::WireStatus::kOk);
  EXPECT_EQ(range->GetBegin(), 0U);
  EXPECT_EQ(range->GetEnd(), UINT64_MAX);
  ASSERT_EQ(item->GetCell(1U, cell), wire::WireStatus::kOk);
  EXPECT_EQ(cell->GetKind(), TestWireKind::kWrite);
  EXPECT_EQ(cell->GetScore(), INT32_MAX);
  const TestWirePair *pair = nullptr;
  ASSERT_EQ(item->GetPair(0U, pair), wire::WireStatus::kOk);
  EXPECT_EQ(pair->GetAddress(), 0U);  // 全默认元素：空记录往返保留
  EXPECT_EQ(pair->GetSize(), 0U);
  ASSERT_EQ(item->GetPair(1U, pair), wire::WireStatus::kOk);
  EXPECT_EQ(pair->GetAddress(), UINT64_MAX);
  EXPECT_EQ(pair->GetSize(), 1U);
  Bytes reencoded = SerializeOk(parsed);
  EXPECT_EQ(reencoded, bytes);  // 解码后再编码逐字节一致（规范编码无重排/无抖动）
}

// 场景：码流夹带未知 tag、同一标量 tag 重复出现、顶层记录乱序。未知 tag 跳过，重复标量取末次值，乱序结果一致。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldSkipUnknownTagAndKeepLastScalar_WhenRecordsReorderedOrDuplicated) {
  // DOC 记录内：重复的 0x0101（Name）取最后一次；记录内未知 tag 跳过
  Bytes doc_payload = Tlv(kTagDocName, Str("first"));
  Append(doc_payload, Tlv(kTagDocName, Str("last")));
  Append(doc_payload, Tlv(kTagUnknown, Bytes{0xDEU, 0xADU}));
  const Bytes doc_record = Tlv(kTagDoc, doc_payload);
  const Bytes item_record = Tlv(kTagItem, Tlv(kTagItemId, Varint(9U)));
  const Bytes unknown_record = Tlv(kTagUnknown, Varint(1U));  // 顶层未知 tag → 跳过不报错
  Bytes records = doc_record;
  Append(records, unknown_record);
  Append(records, item_record);
  const Bytes wire_a = Wire(records, 3U);
  Bytes reordered = unknown_record;  // 顶层记录顺序打乱
  Append(reordered, item_record);
  Append(reordered, doc_record);
  const Bytes wire_b = Wire(reordered, 3U);
  TestWireDocument doc_a;
  TestWireDocument doc_b;
  ASSERT_EQ(doc_a.Deserialize(wire_a.data(), wire_a.size()), wire::WireStatus::kOk);
  ASSERT_EQ(doc_b.Deserialize(wire_b.data(), wire_b.size()), wire::WireStatus::kOk);
  // doc_a：重复 tag 取末次值、未知 tag（记录内与顶层）跳过
  EXPECT_EQ(doc_a.GetDoc().GetName(), "last");  // 重复的 0x0101 取最后一次的值
  EXPECT_FALSE(doc_a.GetDoc().HasLabel());
  ASSERT_EQ(doc_a.GetItemCount(), 1U);
  const TestWireItem *item_a = nullptr;
  ASSERT_EQ(doc_a.GetItem(0U, item_a), wire::WireStatus::kOk);
  EXPECT_EQ(item_a->GetItemId(), 9U);
  // doc_b：顶层记录顺序打乱，解码结果与 doc_a 一致
  EXPECT_EQ(doc_b.GetDoc().GetName(), "last");
  EXPECT_FALSE(doc_b.GetDoc().HasLabel());
  ASSERT_EQ(doc_b.GetItemCount(), 1U);
  const TestWireItem *item_b = nullptr;
  ASSERT_EQ(doc_b.GetItem(0U, item_b), wire::WireStatus::kOk);
  EXPECT_EQ(item_b->GetItemId(), 9U);
}

// 场景：公共头五道门畸形样本（magic 错、总长不足、header_size 非 16、total_size 不符、版本不支持）拒绝且无部分提交。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRejectWithoutPartialCommit_WhenCommonHeaderMalformed) {
  Samples samples;  // 表驱动：样本由分类 helper 构造，RejectOneMalformed 统一循环断言
  AddHeaderSamples(samples);
  RejectOneMalformed(samples);
}

// 场景：帧切分与顶层层次畸形样本（帧头截断、length 越界、层次非法、重复/缺失 DOC、count 不符）拒绝且无部分提交。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRejectWithoutPartialCommit_WhenFrameOrHierarchyMalformed) {
  Samples samples;
  AddFrameSamples(samples);
  RejectOneMalformed(samples);
}

// 场景：值编码与维度畸形样本（varint 超长/溢出/overlong、bool 值域、u32 值域、维度超限）拒绝且无部分提交。
TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRejectWithoutPartialCommit_WhenValueEncodingOrDimsMalformed) {
  Samples samples;
  AddValueSamples(samples);
  RejectOneMalformed(samples);
}

// 场景：单条记录内容超 length:u16、维度元素个数超上限。返回 kLengthOverflow 且不产出残缺字节流（哨兵缓冲保持原样）。
TEST(WireTlvFrameTest, WireTlvEncoder_ShouldFailWithoutTruncatedOutput_WhenLengthLimitsExceeded) {
  // 分支 1：单条记录内容超 length:u16 上限 —— DOC.Name 塞 70000B 字符串（成本可控，不构造几百 MB 数据），
  // Name 字段记录内容 70000 > UINT16_MAX(65535) → kLengthOverflow
  TestWireDocument big_name;
  big_name.MutableDoc().SetName(std::string(70000U, 'x'));
  Bytes buffer_a(16U, 0xA5U);  // 输出缓冲预填哨兵字节
  EXPECT_EQ(big_name.Serialize(buffer_a), wire::WireStatus::kLengthOverflow);
  EXPECT_EQ(buffer_a, Bytes(16U, 0xA5U));  // swap 语义：失败时不产出残缺字节流
  // 分支 2：维度元素个数超 UINT16_MAX —— WriteValue(vector) 编码前先检查 size，65536 个元素仅 512KB
  TestWireDocument big_dims;
  big_dims.AddItem().SetDims(std::vector<uint64_t>(65536U, 0U));  // 65536 = UINT16_MAX + 1
  Bytes buffer_b(16U, 0x5AU);
  EXPECT_EQ(big_dims.Serialize(buffer_b), wire::WireStatus::kLengthOverflow);
  EXPECT_EQ(buffer_b, Bytes(16U, 0x5AU));
  // 编码失败不污染文档对象：正常文档仍可序列化出 20B 空文档
  TestWireDocument small;
  const Bytes recovered = SerializeOk(small);
  EXPECT_EQ(recovered.size(), 20U);
}
