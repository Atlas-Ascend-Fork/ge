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
// map 容器（WireTlvMap：序列化实现由框架收敛，覆盖单容器帧线格式、key tag 边界、K-only/空串 key 条目、畸形边界拒绝）、
// 嵌套单值记录字段（WireTlvRecordField：空 message 也上线的 presence 帧、重复帧就地合并）、
// 记录级 DecodePayload 钩子（oneof 互斥清空、后写生效、层次拒绝由 schema 接管）、
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
#include "securec.h"

namespace wire = ge::wire;  // 本文件局部别名，不使用 using namespace

namespace {
using Bytes = std::vector<uint8_t>;
using TagList = std::vector<uint16_t>;

// ----------------------------------------------
// mini schema：tag 分配（顶层 2 + 集合/容器 4 + 字段 31 = 37 个 tag）
//   顶层记录: DOC=0x0001（恰 1 个）、ITEM=0x0002（0..N）
//   ITEM 集合: CELL=0x0010、PAIR=0x0011；CELL 集合: RANGE=0x0018
//   DOC  字段: Name=0x0101 Label=0x0102(EXPLICIT) Mode=0x0103 Enabled=0x0104
//              Revision=0x0105 Capacity=0x0106 Mark=0x0107(EXPLICIT)
//   DOC map 容器: PROPS=0x0108（条目 PROP，key=PropName）
//   DOC 浮点字段: Ratio=0x0109(float, EXPLICIT) Weight=0x010A(double, implicit)
//   PROP 字段: PropName=0x0111(key, EXPLICIT) PropValue=0x0112(EXPLICIT) PropNote=0x0113(EXPLICIT)
//   ITEM 字段: ItemId=0x0201 SignedVal=0x0202 Dims=0x0203 Note=0x0204
//              Scores=0x0205(i32数组) Ids=0x0206(i64数组) Names=0x0207(string数组)
//              Rates=0x0208(f32数组) Flags=0x0209(bool数组) Masses=0x020A(f64数组)
//              MainPair=0x020B(单值记录) Branch=0x020C(带钩子的单值记录)
//   BRANCH 字段: BranchA=0x1901(uint32,EXPLICIT) BranchB=0x1902(string,EXPLICIT)
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
constexpr uint16_t kTagDocProps = 0x0108U;
constexpr uint16_t kTagPropName = 0x0111U;
constexpr uint16_t kTagPropValue = 0x0112U;
constexpr uint16_t kTagPropNote = 0x0113U;
constexpr uint16_t kTagDocRatio = 0x0109U;
constexpr uint16_t kTagDocWeight = 0x010AU;
constexpr uint16_t kTagItemId = 0x0201U;
constexpr uint16_t kTagItemSigned = 0x0202U;
constexpr uint16_t kTagItemDims = 0x0203U;
constexpr uint16_t kTagItemNote = 0x0204U;
constexpr uint16_t kTagItemScores = 0x0205U;
constexpr uint16_t kTagItemIds = 0x0206U;
constexpr uint16_t kTagItemNames = 0x0207U;
constexpr uint16_t kTagItemRates = 0x0208U;
constexpr uint16_t kTagItemFlags = 0x0209U;
constexpr uint16_t kTagItemMasses = 0x020AU;
constexpr uint16_t kTagItemMainPair = 0x020BU;
constexpr uint16_t kTagItemBranch = 0x020CU;
constexpr uint16_t kTagBranchA = 0x1901U;
constexpr uint16_t kTagBranchB = 0x1902U;
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
      case kTagDocRatio:
      case kTagDocWeight:
      case kTagDocProps:
      case kTagPropName:
      case kTagPropValue:
      case kTagPropNote:
      case kTagItemId:
      case kTagItemSigned:
      case kTagItemDims:
      case kTagItemNote:
      case kTagItemScores:
      case kTagItemIds:
      case kTagItemNames:
      case kTagItemRates:
      case kTagItemFlags:
      case kTagItemMasses:
      case kTagItemMainPair:
      case kTagItemBranch:
      case kTagBranchA:
      case kTagBranchB:
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

// 带记录级 DecodePayload 钩子的 oneof 形记录（BRANCH）：钩子由框架分派（HasDecodePayloadMember），
// 接管本记录载荷的整个解码——任一分支帧出现先清全部分支再写该帧（互斥 + 后写生效，对齐 pb oneof merge），
// 已知他层 tag 漏入判层次错误，未知 tag 跳过（前向兼容）
class TestWireBranch {
 public:
  void SetA(const uint32_t &value) {
    ClearBranches();
    wire::SetWireTlvField(A_, value);
  }
  const uint32_t &GetA() const {
    return wire::GetWireTlvField(A_);
  }
  bool HasA() const {
    return wire::HasWireTlvField(A_);
  }
  void SetB(const std::string &value) {
    ClearBranches();
    wire::SetWireTlvField(B_, value);
  }
  const std::string &GetB() const {
    return wire::GetWireTlvField(B_);
  }
  bool HasB() const {
    return wire::HasWireTlvField(B_);
  }
  void ClearBranches() {
    A_ = WireTlvField<uint32_t, true>{};
    B_ = WireTlvField<std::string, true>{};
  }

  // 记录级钩子（框架经 WireTlvDecodeRecord 分派到这里，不走注册表默认路径）
  WireStatus DecodePayload(const uint8_t *data, size_t size) {
    return wire::ProcessRecords(data, size, [this](uint16_t tag, const uint8_t *payload, size_t payload_size) {
      if ((tag == kTagBranchA) || (tag == kTagBranchB)) {
        ClearBranches();
      }
      wire::WireTlvDecoder<TestWireSchema> decoder(tag, payload, payload_size);
      decoder.VisitMember(kTagBranchA, A_);
      decoder.VisitMember(kTagBranchB, B_);
      if (decoder.status != WireStatus::kOk) {
        return decoder.status;
      }
      if (!decoder.matched && TestWireSchema::IsKnownTag(tag)) {
        return WireStatus::kInvalidFormat;
      }
      return WireStatus::kOk;
    });
  }

 private:
  WireTlvField<uint32_t, true> A_;
  WireTlvField<std::string, true> B_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagBranchA, A_);
    visitor.VisitMember(kTagBranchB, B_);
  }
};

// 深链嵌套专用 schema（复杂度回归防护，独立 tag 与 Schema，不入 mini schema tag 表）：
// 计数器放在测试 schema 的 RegisterMembers 里——注册表由 schema 提供，框架零测试埋点
int64_t g_deep_reg_calls = 0;

constexpr uint16_t kTagDeepRoot = 0x0301U;
constexpr uint16_t kTagDeepChild = 0x0302U;
constexpr uint16_t kTagDeepValue = 0x0303U;

struct TestWireDeepSchema {
  static bool IsKnownTag(uint16_t tag) {
    return (tag == kTagDeepRoot) || (tag == kTagDeepChild) || (tag == kTagDeepValue);
  }
};

class TestWireDeepLeaf {
 public:
  void SetValue(const uint32_t &value) {
    wire::SetWireTlvField(Value_, value);
  }
  const uint32_t &GetValue() const {
    return wire::GetWireTlvField(Value_);
  }

 private:
  WireTlvField<uint32_t> Value_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    ++g_deep_reg_calls;
    visitor.VisitMember(kTagDeepValue, Value_);
  }
};

template <int D>
class TestWireDeepChain {
 public:
  void SetChild(const TestWireDeepChain<D - 1> &child) {
    wire::SetWireTlvRecordField(Child_, child);
  }
  const TestWireDeepChain<D - 1> &GetChild() const {
    return wire::GetWireTlvRecordField(Child_);
  }

 private:
  wire::WireTlvRecordField<TestWireDeepChain<D - 1>> Child_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    ++g_deep_reg_calls;
    visitor.VisitMember(kTagDeepChild, Child_);
  }
};

template <>
class TestWireDeepChain<1> {
 public:
  void SetChild(const TestWireDeepLeaf &leaf) {
    wire::SetWireTlvRecordField(Child_, leaf);
  }
  const TestWireDeepLeaf &GetChild() const {
    return wire::GetWireTlvRecordField(Child_);
  }

 private:
  wire::WireTlvRecordField<TestWireDeepLeaf> Child_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    ++g_deep_reg_calls;
    visitor.VisitMember(kTagDeepChild, Child_);
  }
};

template <int D>
TestWireDeepChain<D> MakeDeepChain(const uint32_t value) {
  TestWireDeepChain<D> node;
  node.SetChild(MakeDeepChain<D - 1>(value));
  return node;
}

template <>
inline TestWireDeepChain<1> MakeDeepChain<1>(const uint32_t value) {
  TestWireDeepLeaf leaf;
  leaf.SetValue(value);
  TestWireDeepChain<1> node;
  node.SetChild(leaf);
  return node;
}

inline const uint32_t &DeepLeafValue(const TestWireDeepLeaf &leaf) {
  return leaf.GetValue();
}

template <int D>
const uint32_t &DeepLeafValue(const TestWireDeepChain<D> &node) {
  return DeepLeafValue(node.GetChild());
}

// map 的条目类型（PROP）：kWireMapKeyTag 声明边界 key；三个字段全部显式 presence——
// key 必须显式（空串 Set 过也上线，否则条目失去边界），value/note 显式用于覆盖 K-only 条目的 Has* 语义
class TestWireProp {
 public:
  static constexpr uint16_t kWireMapKeyTag = kTagPropName;
  void SetName(const std::string &value) {
    wire::SetWireTlvField(Name_, value);
  }
  const std::string &GetName() const {
    return wire::GetWireTlvField(Name_);
  }
  void SetValue(const uint32_t &value) {
    wire::SetWireTlvField(Value_, value);
  }
  const uint32_t &GetValue() const {
    return wire::GetWireTlvField(Value_);
  }
  bool HasValue() const {
    return wire::HasWireTlvField(Value_);
  }
  void SetNote(const std::string &value) {
    wire::SetWireTlvField(Note_, value);
  }
  const std::string &GetNote() const {
    return wire::GetWireTlvField(Note_);
  }
  bool HasNote() const {
    return wire::HasWireTlvField(Note_);
  }

 private:
  WireTlvField<std::string, true> Name_;
  WireTlvField<uint32_t, true> Value_;
  WireTlvField<std::string, true> Note_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagPropName, Name_);
    visitor.VisitMember(kTagPropValue, Value_);
    visitor.VisitMember(kTagPropNote, Note_);
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
  void SetScores(const std::vector<int32_t> &value) {
    wire::SetWireTlvField(Scores_, value);
  }
  const std::vector<int32_t> &GetScores() const {
    return wire::GetWireTlvField(Scores_);
  }
  void SetIds(const std::vector<int64_t> &value) {
    wire::SetWireTlvField(Ids_, value);
  }
  const std::vector<int64_t> &GetIds() const {
    return wire::GetWireTlvField(Ids_);
  }
  void SetNames(const std::vector<std::string> &value) {
    wire::SetWireTlvField(Names_, value);
  }
  const std::vector<std::string> &GetNames() const {
    return wire::GetWireTlvField(Names_);
  }
  void SetRates(const std::vector<float> &value) {
    wire::SetWireTlvField(Rates_, value);
  }
  const std::vector<float> &GetRates() const {
    return wire::GetWireTlvField(Rates_);
  }
  void SetFlags(const std::vector<bool> &value) {
    wire::SetWireTlvField(Flags_, value);
  }
  const std::vector<bool> &GetFlags() const {
    return wire::GetWireTlvField(Flags_);
  }
  void SetMasses(const std::vector<double> &value) {
    wire::SetWireTlvField(Masses_, value);
  }
  const std::vector<double> &GetMasses() const {
    return wire::GetWireTlvField(Masses_);
  }
  void SetMainPair(const TestWirePair &value) {
    wire::SetWireTlvRecordField(MainPair_, value);
  }
  const TestWirePair &GetMainPair() const {
    return wire::GetWireTlvRecordField(MainPair_);
  }
  bool HasMainPair() const {
    return wire::HasWireTlvRecordField(MainPair_);
  }
  void SetBranch(const TestWireBranch &value) {
    wire::SetWireTlvRecordField(Branch_, value);
  }
  const TestWireBranch &GetBranch() const {
    return wire::GetWireTlvRecordField(Branch_);
  }
  bool HasBranch() const {
    return wire::HasWireTlvRecordField(Branch_);
  }

 private:
  WireTlvField<uint32_t> ItemId_;
  WireTlvField<int32_t> SignedVal_;
  WireTlvField<std::vector<uint64_t>> Dims_;
  WireTlvField<std::string> Note_;
  std::vector<TestWireCell> Cell_;
  std::vector<TestWirePair> Pair_;
  WireTlvField<std::vector<int32_t>> Scores_;
  WireTlvField<std::vector<int64_t>> Ids_;
  WireTlvField<std::vector<std::string>> Names_;
  WireTlvField<std::vector<float>> Rates_;
  WireTlvField<std::vector<bool>> Flags_;
  WireTlvField<std::vector<double>> Masses_;
  wire::WireTlvRecordField<TestWirePair> MainPair_;
  wire::WireTlvRecordField<TestWireBranch> Branch_;
  friend struct wire::MemberRegHelper;
  template <typename Visitor>
  void RegisterMembers(Visitor &visitor) {
    visitor.VisitMember(kTagItemId, ItemId_);
    visitor.VisitMember(kTagItemSigned, SignedVal_);
    visitor.VisitMember(kTagItemDims, Dims_);
    visitor.VisitMember(kTagItemNote, Note_);
    visitor.VisitMember(kTagItemCells, Cell_);
    visitor.VisitMember(kTagItemPairs, Pair_);
    visitor.VisitMember(kTagItemScores, Scores_);
    visitor.VisitMember(kTagItemIds, Ids_);
    visitor.VisitMember(kTagItemNames, Names_);
    visitor.VisitMember(kTagItemRates, Rates_);
    visitor.VisitMember(kTagItemFlags, Flags_);
    visitor.VisitMember(kTagItemMasses, Masses_);
    visitor.VisitMember(kTagItemMainPair, MainPair_);
    visitor.VisitMember(kTagItemBranch, Branch_);
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
  void SetRatio(const float &value) {
    wire::SetWireTlvField(Ratio_, value);
  }
  const float &GetRatio() const {
    return wire::GetWireTlvField(Ratio_);
  }
  bool HasRatio() const {
    return wire::HasWireTlvField(Ratio_);
  }
  void SetWeight(const double &value) {
    wire::SetWireTlvField(Weight_, value);
  }
  const double &GetWeight() const {
    return wire::GetWireTlvField(Weight_);
  }
  TestWireProp &AddProp() {
    return wire::AddWireTlvItem(Props_);
  }
  size_t GetPropCount() const {
    return wire::GetWireTlvItemCount(Props_);
  }
  WireStatus GetProp(const size_t index, const TestWireProp *&value) const {
    return wire::GetWireTlvItem(Props_, index, value);
  }

 private:
  WireTlvField<std::string> Name_;
  WireTlvField<std::string, true> Label_;
  WireTlvField<TestWireMode> Mode_;
  WireTlvField<bool> Enabled_;
  WireTlvField<uint32_t> Revision_;
  WireTlvField<uint64_t> Capacity_;
  WireTlvField<uint32_t, true> Mark_;
  wire::WireTlvMap<TestWireProp> Props_;
  WireTlvField<float, true> Ratio_;
  WireTlvField<double> Weight_;
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
    visitor.VisitMember(kTagDocProps, Props_);
    visitor.VisitMember(kTagDocRatio, Ratio_);
    visitor.VisitMember(kTagDocWeight, Weight_);
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

// 记录帧：tag:u16 定宽小端 + length 规范 LEB128 varint（变宽）+ 负载
// length 变宽后不能再用 PokeLe 覆写"第 2~3 字节"来构造畸形帧，畸形长度一律由 TlvWithRawLength 显式给出
Bytes Tlv(uint16_t tag, const Bytes &payload) {
  Bytes result = Le(tag, 2U);
  Append(result, Varint(payload.size()));
  Append(result, payload);
  return result;
}

// 用调用方给定的原始 length 字节构造帧（不做规范编码），专用于畸形 length 样本
Bytes TlvWithRawLength(uint16_t tag, const Bytes &raw_length, const Bytes &payload) {
  Bytes result = Le(tag, 2U);
  Append(result, raw_length);
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

// float 位模式 ↔ 值互转（golden 侧位级校验工具，memcpy_s 等宽位搬运，不经过被测编码路径）
float FloatFromBits(const uint32_t bits) {
  float value = 0.0F;
  if (memcpy_s(&value, sizeof(value), &bits, sizeof(bits)) != EOK) {
    ADD_FAILURE() << "memcpy_s failed";
  }
  return value;
}

uint32_t FloatToBits(const float value) {
  uint32_t bits = 0U;
  if (memcpy_s(&bits, sizeof(bits), &value, sizeof(value)) != EOK) {
    ADD_FAILURE() << "memcpy_s failed";
  }
  return bits;
}

uint64_t DoubleToBits(const double value) {
  uint64_t bits = 0U;
  if (memcpy_s(&bits, sizeof(bits), &value, sizeof(value)) != EOK) {
    ADD_FAILURE() << "memcpy_s failed";
  }
  return bits;
}

// golden 侧位模式常量：按 IEEE 754 字段结构（sign | exponent | mantissa）手工拼装。
// 不写长 hex/十进制字面量（静态检查会把长 0 数字串误判为敏感信息），且字段化更易核对推导
constexpr uint32_t FloatBitsOf(const uint32_t sign, const uint32_t exponent, const uint32_t mantissa) {
  return (sign << 31U) | (exponent << 23U) | mantissa;
}

constexpr uint64_t DoubleBitsOf(const uint64_t sign, const uint64_t exponent, const uint64_t mantissa) {
  return (sign << 63U) | (exponent << 52U) | mantissa;
}

// 单个 PROP 条目的成员帧序列（golden 手工推导：key 显式 presence 恒上线，value/note 由入参决定）
Bytes PropEntryFrames(const std::string &name, const bool has_value, const uint32_t value, const bool has_note,
                      const std::string &note) {
  Bytes frames = Tlv(kTagPropName, Str(name));
  if (has_value) {
    Append(frames, Tlv(kTagPropValue, Varint(value)));
  }
  if (has_note) {
    Append(frames, Tlv(kTagPropNote, Str(note)));
  }
  return frames;
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

// 从无公共头的裸帧序列中取第一条指定 tag 帧的负载（嵌套层反查用，同样只依赖帧切分器）
bool FirstFramePayload(const Bytes &frames, uint16_t tag, Bytes &payload) {
  bool found = false;
  wire::ProcessRecords(frames.data(), frames.size(), [&](uint16_t frame_tag, const uint8_t *data, size_t data_size) {
    if (!found && (frame_tag == tag)) {
      payload.assign(data, data + data_size);
      found = true;
    }
    return wire::WireStatus::kOk;
  });
  return found;
}

// 完整线流中取顶层 DOC 记录内第一条指定 tag 字段帧的负载（两级反查的组合 helper）
bool DocFieldPayload(const Bytes &wire_bytes, uint16_t tag, Bytes &payload) {
  Bytes doc_payload;
  if (!FirstRecordPayload(wire_bytes, kTagDoc, doc_payload)) {
    return false;
  }
  return FirstFramePayload(doc_payload, tag, payload);
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
// 帧头 = tag:u16 定宽小端 + length 规范 LEB128 varint（本文件所有负载均 <128B，故 length 恒为 1B，
// 帧头 3B）；标量 = 规范 LEB128 varint（有符号先 zigzag）；bool = 定宽 1B；string = 裸字节；
// vector<uint64_t> = count varint + 每元素 zigzag varint（-1 表示未知维）。
// DOC 记录（tag=0x0001）负载 38B：
//   Name "abc"          0x0101 len=3  裸字节 "abc"                → 6B
//   Label ""            0x0102 len=0  EXPLICIT 空串默认值也上线    → 3B
//   Mode kSlow(2)       0x0103 len=1  varint(2)={0x02}            → 4B
//   Enabled true        0x0104 len=1  bool 定宽 {0x01}            → 4B
//   Revision 300        0x0105 len=2  varint(300)={0xAC,0x02}     → 5B
//   Capacity 2^63       0x0106 len=10 varint(2^63)={0x80*9,0x01}  → 13B
//   Mark 0              0x0107 len=1  EXPLICIT varint(0)={0x00}   → 4B
// ITEM 记录（tag=0x0002）负载 58B：
//   ItemId 1            0x0201 len=1  varint(1)                   → 4B
//   SignedVal -7        0x0202 len=1  zigzag(-7)=13={0x0D}        → 4B
//   Dims [4,-1,3]       0x0203 len=3  zz(4)={0x08}+zz(-1)={0x01}(未知维单字节)+zz(3)={0x06}（无 count）→ 6B
//   Note "x"            0x0204 len=1  裸字节                       → 4B
//   CELL(9,kRead,5,RANGE[1,2]) 0x0010 负载 23B(4+4+4+11)           → 26B
//     其中 RANGE 0x0018 负载 8B = Begin(4B)+End(4B)                 → 11B
//   PAIR(4096,8)        0x0011 负载 9B = varint(4096)={0x80,0x20}(5B)+varint(8)(4B) → 12B
// 总长 = 16 头 + (3+38) DOC + (3+58) ITEM = 117B，record_count = 2
// ----------------------------------------------
Bytes BuildGoldenDocPayload() {
  Bytes payload = Tlv(kTagDocName, Str("abc"));
  Append(payload, Tlv(kTagDocLabel, {}));
  Append(payload, Tlv(kTagDocMode, Varint(2U)));
  Append(payload, Tlv(kTagDocEnabled, Bytes{1U}));
  Append(payload, Tlv(kTagDocRevision, Varint(300U)));
  Append(payload, Tlv(kTagDocCapacity, Varint(1ULL << 63)));
  Append(payload, Tlv(kTagDocMark, Varint(0U)));
  return payload;  // 38B
}

Bytes BuildGoldenItemPayload() {
  Bytes range = Tlv(kTagRangeBegin, Varint(1U));
  Append(range, Tlv(kTagRangeEnd, Varint(2U)));  // RANGE 负载 8B
  Bytes cell = Tlv(kTagCellId, Varint(9U));
  Append(cell, Tlv(kTagCellKind, Varint(1U)));    // 枚举 kRead 按无符号 varint
  Append(cell, Tlv(kTagCellScore, ZzVarint(5)));  // i32 正值 zigzag(5)=10
  Append(cell, Tlv(kTagCellRanges, range));       // CELL 负载 23B
  Bytes pair = Tlv(kTagPairAddress, Varint(4096U));
  Append(pair, Tlv(kTagPairSize, Varint(8U)));  // PAIR 负载 9B
  Bytes item = Tlv(kTagItemId, Varint(1U));
  Append(item, Tlv(kTagItemSigned, ZzVarint(-7)));  // 负数 zigzag(-7)=13 → 单字节
  Bytes dims = ZzVarint(4);                         // 维度数组：元素 zigzag 直拼，无 count
  Append(dims, ZzVarint(-1));                       // -1（存储为 UINT64_MAX）→ zigzag=1 单字节
  Append(dims, ZzVarint(3));
  Append(item, Tlv(kTagItemDims, dims));  // 维度负载 3B
  Append(item, Tlv(kTagItemNote, Str("x")));
  Append(item, Tlv(kTagItemCells, cell));
  Append(item, Tlv(kTagItemPairs, pair));
  return item;  // 58B
}

Bytes BuildCanonicalGolden() {
  Bytes records = Tlv(kTagDoc, BuildGoldenDocPayload());     // DOC 记录 41B
  Append(records, Tlv(kTagItem, BuildGoldenItemPayload()));  // ITEM 记录 61B
  return Wire(records, 2U);                                  // 117B
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
  // length 改为 varint 后，帧头不再有编译期固定宽度，畸形 length 一律由 TlvWithRawLength 给出原始字节
  Bytes truncated = Wire(Tlv(kTagDoc, {}), 1U);
  truncated.push_back(0xFFU);                   // 尾部残留 1B：不足最小帧头（tag2B + length varint 首字节 = 3B）
  PokeLe(truncated, 8U, truncated.size(), 4U);  // 同步 total_size，隔离出帧切分错误
  samples.push_back({"尾部残留不足最小帧头", wire::WireStatus::kInvalidFormat, truncated});
  // length varint 首字节置续位后流即结束：截断
  Bytes len_truncated = TlvWithRawLength(kTagDoc, Bytes{0x80U}, {});
  samples.push_back({"length varint 截断", wire::WireStatus::kInvalidFormat, Wire(len_truncated, 1U)});
  // 0x80 0x00 是 0 的非规范（overlong）编码，规范写法应为单字节 0x00
  Bytes len_overlong = TlvWithRawLength(kTagDoc, Bytes{0x80U, 0x00U}, {});
  samples.push_back({"length varint 非规范编码", wire::WireStatus::kInvalidFormat, Wire(len_overlong, 1U)});
  // 0xFF 0xFF 0xFF 0xFF 0xFF 0x01 = 2^35，超出 length 的 u32 值域（与公共头 total_size:u32 对齐）
  Bytes len_over_u32 = TlvWithRawLength(kTagDoc, Bytes{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x01U}, {});
  samples.push_back({"length 超出 u32 值域", wire::WireStatus::kInvalidFormat, Wire(len_over_u32, 1U)});
  Bytes over = TlvWithRawLength(kTagDoc, Bytes{0x09U}, {});  // length=9 但负载为空，越出总长边界
  samples.push_back({"记录 length 越总长边界", wire::WireStatus::kInvalidFormat, Wire(over, 1U)});
  Bytes inner = TlvWithRawLength(kTagDocName, Bytes{0x09U}, {});  // DOC 内部字段记录 length 越父记录
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

// 值编码：varint 四道校验、bool 定宽与值域、u32 值域、维度元素截断
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
                     DocWire(Tlv(kTagDocRevision, Varint((1ULL << 33U) - 1U)))});
  samples.push_back({"int32 字段 zigzag 超值域", wire::WireStatus::kInvalidFormat,
                     ItemWire(Tlv(kTagItemSigned, ZzVarint(static_cast<int64_t>(INT32_MAX) + 1)))});
  // 无 count 后维度数组不存在"count 超剩余/负载未恰好消耗"形态：varint 元素自终止，
  // 唯一的畸形模式是元素 varint 截断（尾部 continuation 位悬挂）
  samples.push_back(
      {"维度元素 varint 截断", wire::WireStatus::kInvalidFormat, ItemWire(Tlv(kTagItemDims, {0x01U, 0x80U}))});
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

// 场景：空文档（无 ITEM、DOC 字段全不设）。校验 16B 公共头 + 空 DOC 帧头共 19B 的 golden 与各头字段取值。
TEST(WireTlvFrameTest, WireTlvFrame_ShouldMatchGoldenHeader_WhenDocumentEmpty) {
  // 空文档 = 16B 公共头 + 空 DOC 记录（tag=0x0001 定宽 2B + length varint=0x00 共 1B，负载 0B）= 19B，
  // record_count=1。手写字面量与独立构造器结果互验，再逐字段断言公共头（全部定宽小端）。
  const Bytes golden = Wire(Tlv(kTagDoc, {}), 1U);
  const Bytes literal = {0x54, 0x53, 0x45, 0x54, 1, 0, 16, 0, 19, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0};
  EXPECT_EQ(golden, literal);
  TestWireDocument doc;
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);  // 被测编码与独立 golden 逐字节一致
  ASSERT_EQ(bytes.size(), 19U);
  EXPECT_EQ(wire::ReadLe(bytes.data(), 4U), TestWireSchema::kMagic);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 4U, 2U), TestWireSchema::kVersion);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 6U, 2U), wire::kHeaderSize);
  EXPECT_EQ(wire::ReadLe(bytes.data() + 8U, 4U), 19U);  // total_size == 实际长度
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
  ASSERT_EQ(golden.size(), 117U);  // 16 头 + 41 DOC(3+38) + 61 ITEM(3+58)
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);  // 被测编码与独立 golden 逐字节一致
}

// 场景：解码 117B 手工 golden 字节流，逐字段还原有符号 zigzag、explicit 默认值 presence 与三层嵌套集合。
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
  const Bytes bare_bytes = SerializeOk(bare);  // EXPLICIT 未 Set：不上线（19B 空文档）
  TestWireDocument explicit_defaults;
  explicit_defaults.MutableDoc().SetLabel("");  // EXPLICIT 空串默认值
  explicit_defaults.MutableDoc().SetMark(0U);   // EXPLICIT u32 默认值
  explicit_defaults.MutableDoc().SetName("");   // implicit 对照：默认值不上线
  const Bytes set_bytes = SerializeOk(explicit_defaults);
  // Label 空串记录 3B（len=0）+ Mark 记录 4B（len=1 + varint(0)）→ 比基线大 7B
  EXPECT_EQ(set_bytes.size(), bare_bytes.size() + 7U);
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
  const Bytes bare_bytes = SerializeOk(bare);  // EXPLICIT 未 Set：不上线（19B 空文档）
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
// 场景：length 由 u16 定宽改为 varint 后，此前两处 UINT16_MAX 闸门已移除 —— 超 65535B 的记录与
// 超 65535 维的维度数组都必须能正常编解码（这正是本次改造要解掉的硬上限），并顺带校验 length
// 在 127/128、16383/16384 处的 varint 宽度跳变。
TEST(WireTlvFrameTest, WireTlvEncoder_ShouldEncodeBeyondU16_WhenRecordPayloadExceedsOldLimit) {
  // DOC.Name 塞 70000B 字符串，Name 字段记录内容 70000 > 旧上限 UINT16_MAX(65535)。
  // 不手写 varint 字节（易错），改为与独立构造器 golden 逐字节互验 + 帧切分器反查负载长度
  TestWireDocument big_name;
  big_name.MutableDoc().SetName(std::string(70000U, 'x'));
  const Bytes buffer = SerializeOk(big_name);
  const Bytes golden = Wire(Tlv(kTagDoc, Tlv(kTagDocName, Bytes(70000U, 'x'))), 1U);
  EXPECT_EQ(buffer, golden);

  Bytes name_payload;
  ASSERT_TRUE(DocFieldPayload(buffer, kTagDocName, name_payload));
  EXPECT_EQ(name_payload.size(), 70000U);  // varint length 正确表达并还原了超 u16 的负载长度

  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(buffer.data(), buffer.size()), wire::WireStatus::kOk);
  EXPECT_EQ(parsed.GetDoc().GetName().size(), 70000U);
}

TEST(WireTlvFrameTest, WireTlvEncoder_ShouldEncodeBeyondU16_WhenDimsCountExceedsOldLimit) {
  // 维度元素个数 65536 = 旧 UINT16_MAX + 1。元素取 200 → zigzag(200)=400 → varint 2B，
  // 维度记录负载 = 65536×2B = 131072B（无 count；同样超旧 u16 length 上限 65535）
  TestWireDocument big_dims;
  big_dims.AddItem().SetDims(std::vector<uint64_t>(65536U, 200U));
  const Bytes buffer = SerializeOk(big_dims);
  TestWireDocument parsed;
  ASSERT_EQ(parsed.Deserialize(buffer.data(), buffer.size()), wire::WireStatus::kOk);
  const TestWireItem *item = nullptr;
  ASSERT_EQ(parsed.GetItem(0U, item), wire::WireStatus::kOk);
  ASSERT_EQ(item->GetDims().size(), 65536U);
  EXPECT_EQ(item->GetDims().front(), 200U);
}

TEST(WireTlvFrameTest, WireTlvEncoder_ShouldEncodeExactVarintWidth_WhenLengthCrossesWidthBoundary) {
  // length varint 的宽度跳变边界。不走"多层嵌套推导起点偏移"（DOC 帧头与 Name 帧头的 width 会随长度
  // 各自跳变，手工推导易错），直接构造字节 golden 与帧切分器的切分结果互验。
  // 取 [127,128] 与 [16383,16384] 两对边界：Name 负载恒为纯字符串
  const struct {
    size_t str_len;
    size_t expect_width;
  } kWidthCases[] = {{126U, 1U}, {127U, 1U}, {128U, 2U}, {16382U, 2U}, {16383U, 2U}, {16384U, 3U}};
  for (const auto &c : kWidthCases) {
    // 独立 golden：Name 记录 = tag 2B + varint(str_len) + str_len 个 'y'
    const Bytes name_record = Tlv(kTagDocName, Bytes(c.str_len, 'y'));
    const Bytes golden = Wire(Tlv(kTagDoc, name_record), 1U);
    // varint(str_len) 的实际宽度必须落在期望档上（用 golden 长度反推，避免再写一份宽度公式）
    EXPECT_EQ(name_record.size(), 2U + c.expect_width + c.str_len) << "str_len=" << c.str_len;

    // 被测编码器必须产出同样的字节
    TestWireDocument doc;
    doc.MutableDoc().SetName(std::string(c.str_len, 'y'));
    EXPECT_EQ(SerializeOk(doc), golden) << "str_len=" << c.str_len;

    // 帧切分器按 varint length 精确切分，负载长度与内容都要还原
    Bytes name_payload;
    ASSERT_TRUE(DocFieldPayload(golden, kTagDocName, name_payload)) << "str_len=" << c.str_len;
    ASSERT_EQ(name_payload.size(), c.str_len) << "str_len=" << c.str_len;
    const bool all_y =
        std::all_of(name_payload.begin(), name_payload.end(), [](const uint8_t byte) { return byte == 'y'; });
    EXPECT_TRUE(all_y) << "str_len=" << c.str_len;
  }
}

// ----------------------------------------------
// map 容器（WireTlvMap）：schema 只声明 map 语义，此处 golden 按框架当前线格式
// （单容器帧 + key tag 边界切分）手工推导，锁住实现不回退
// ----------------------------------------------

TEST(WireTlvMapTest, WireTlvEncoder_ShouldWriteSingleMapContainer_WhenMapHasMultipleEntries) {
  TestWireDocument doc;
  auto &first = doc.MutableDoc().AddProp();
  first.SetName("alpha");
  first.SetValue(7U);
  first.SetNote("n1");
  auto &second = doc.MutableDoc().AddProp();
  second.SetName("beta");
  second.SetValue(9U);

  // golden：恰一个 PROPS 容器帧，负载 = 两条目的成员帧直拼（条目无各自帧头）
  Bytes entries = PropEntryFrames("alpha", true, 7U, true, "n1");
  Append(entries, PropEntryFrames("beta", true, 9U, false, ""));
  const Bytes golden = DocWire(Tlv(kTagDocProps, entries));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  // 线格式收益核对：若按逐条目帧编码（每条一个容器帧），比 map 的单容器帧聚合多 (n-1) 个帧头，
  // n=2 且各负载 <128B 时恰多 3B（tag 2B + length varint 1B）；两侧同为 DOC 帧包装后比较
  Bytes per_entry_form = Tlv(kTagDocProps, PropEntryFrames("alpha", true, 7U, true, "n1"));
  Append(per_entry_form, Tlv(kTagDocProps, PropEntryFrames("beta", true, 9U, false, "")));
  EXPECT_EQ(DocWire(per_entry_form).size(), bytes.size() + 3U);

  // roundtrip：条目、字段值、presence 全还原；解码后再编码字节一致（count-first 长度自洽）
  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  ASSERT_EQ(decoded.GetDoc().GetPropCount(), 2U);
  const TestWireProp *prop = nullptr;
  ASSERT_EQ(decoded.GetDoc().GetProp(0U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "alpha");
  EXPECT_TRUE(prop->HasValue());
  EXPECT_EQ(prop->GetValue(), 7U);
  EXPECT_EQ(prop->GetNote(), "n1");
  ASSERT_EQ(decoded.GetDoc().GetProp(1U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "beta");
  EXPECT_EQ(prop->GetValue(), 9U);
  EXPECT_FALSE(prop->HasNote());
  EXPECT_EQ(SerializeOk(decoded), bytes);
}

TEST(WireTlvMapTest, WireTlvEncoder_ShouldKeepEntryBoundary_WhenKeyOnlyOrEmptyKeyEntry) {
  TestWireDocument doc;
  auto &empty_key = doc.MutableDoc().AddProp();
  empty_key.SetName("");  // key 显式 presence：空串也上线，条目边界不丢
  empty_key.SetValue(5U);
  doc.MutableDoc().AddProp().SetName("k2");  // K-only：只 Set key，value/note 不上线 = 空值语义

  Bytes entries = PropEntryFrames("", true, 5U, false, "");
  Append(entries, PropEntryFrames("k2", false, 0U, false, ""));
  const Bytes golden = DocWire(Tlv(kTagDocProps, entries));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  ASSERT_EQ(decoded.GetDoc().GetPropCount(), 2U);
  const TestWireProp *prop = nullptr;
  ASSERT_EQ(decoded.GetDoc().GetProp(0U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "");
  EXPECT_EQ(prop->GetValue(), 5U);
  ASSERT_EQ(decoded.GetDoc().GetProp(1U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "k2");
  EXPECT_FALSE(prop->HasValue());
  EXPECT_FALSE(prop->HasNote());
  EXPECT_EQ(SerializeOk(decoded), bytes);
}

TEST(WireTlvMapTest, WireTlvEncoder_ShouldOmitContainerFrame_WhenMapEmpty) {
  TestWireDocument doc;
  doc.MutableDoc().SetName("only-name");
  // 空 map 不上线：golden 里不存在 0x0108 帧（与空 vector<T> 约定一致）
  const Bytes golden = DocWire(Tlv(kTagDocName, Str("only-name")));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  EXPECT_EQ(decoded.GetDoc().GetPropCount(), 0U);
  EXPECT_EQ(decoded.GetDoc().GetName(), "only-name");
}

TEST(WireTlvMapTest, WireTlvDecoder_ShouldRejectWithoutPartialCommit_WhenContainerBoundaryOrHierarchyMalformed) {
  // a) 容器首帧不是 key tag：条目边界无法建立
  {
    TestWireDocument doc;
    doc.MutableDoc().SetName("keep");
    const Bytes intact = SerializeOk(doc);
    Bytes bad_payload = Tlv(kTagPropValue, Varint(3U));
    Append(bad_payload, PropEntryFrames("x", false, 0U, false, ""));
    const Bytes bad_wire = DocWire(Tlv(kTagDocProps, bad_payload));
    EXPECT_EQ(doc.Deserialize(bad_wire.data(), bad_wire.size()), wire::WireStatus::kInvalidFormat);
    // 无部分提交：原对象状态与序列化结果不变
    EXPECT_EQ(SerializeOk(doc), intact);
    EXPECT_EQ(doc.GetDoc().GetPropCount(), 0U);
  }
  // b) DOC 层级的已知 tag 漏进容器：PROP 字段表匹配不到且 IsKnownTag=true → 层次错误
  {
    Bytes payload = PropEntryFrames("k", false, 0U, false, "");
    Append(payload, Tlv(kTagDocName, Str("leak")));
    const Bytes bad_wire = DocWire(Tlv(kTagDocProps, payload));
    TestWireDocument doc;
    EXPECT_EQ(doc.Deserialize(bad_wire.data(), bad_wire.size()), wire::WireStatus::kInvalidFormat);
  }
  // c) 容器帧 length 声称超过实际负载：帧切分器判截断
  {
    const Bytes short_payload = PropEntryFrames("k", false, 0U, false, "");
    const Bytes bad_wire = DocWire(TlvWithRawLength(kTagDocProps, Varint(short_payload.size() + 9U), short_payload));
    TestWireDocument doc;
    EXPECT_EQ(doc.Deserialize(bad_wire.data(), bad_wire.size()), wire::WireStatus::kInvalidFormat);
  }
  // d) 容器内未知 tag：跳过（前向兼容），边界仍由 key tag 建立，条目正常解出
  {
    Bytes payload = Tlv(kTagPropName, Str("a"));
    Append(payload, Tlv(kTagUnknown, Str("zz")));
    Append(payload, Tlv(kTagPropValue, Varint(1U)));
    const Bytes ok_wire = DocWire(Tlv(kTagDocProps, payload));
    TestWireDocument doc;
    ASSERT_EQ(doc.Deserialize(ok_wire.data(), ok_wire.size()), wire::WireStatus::kOk);
    ASSERT_EQ(doc.GetDoc().GetPropCount(), 1U);
    const TestWireProp *prop = nullptr;
    ASSERT_EQ(doc.GetDoc().GetProp(0U, prop), wire::WireStatus::kOk);
    EXPECT_EQ(prop->GetName(), "a");
    EXPECT_EQ(prop->GetValue(), 1U);
  }
}

TEST(WireTlvMapTest, WireTlvEncoder_ShouldKeepAggregatedLengthExact_WhenContainerPayloadExceedsOneByteVarint) {
  // 条目聚合后超过 127B：容器 length varint 占 2B，count-first 聚合计数必须与实际写出精确一致
  TestWireDocument doc;
  const std::string note(100, 'n');
  Bytes entries;
  for (uint32_t i = 0U; i < 2U; ++i) {
    const std::string name = (i == 0U) ? "k0" : "k1";
    auto &prop = doc.MutableDoc().AddProp();
    prop.SetName(name);
    prop.SetNote(note);
    Append(entries, PropEntryFrames(name, false, 0U, true, note));
  }
  ASSERT_GT(entries.size(), 127U);
  const Bytes golden = DocWire(Tlv(kTagDocProps, entries));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  ASSERT_EQ(decoded.GetDoc().GetPropCount(), 2U);
  const TestWireProp *prop = nullptr;
  ASSERT_EQ(decoded.GetDoc().GetProp(1U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "k1");
  EXPECT_EQ(prop->GetNote(), note);
  EXPECT_EQ(SerializeOk(decoded), bytes);
}

// ----------------------------------------------
// 浮点值编码：IEEE 754 位模式小端定宽（float 4B / double 8B）
// ----------------------------------------------

TEST(WireTlvFrameTest, WireTlvField_ShouldCarryFloatBitPatternExactly_WhenSpecialValuesEncoded) {
  // 位级保真表：±0（符号位）、±Inf、带 payload 的 quiet NaN；Ratio 为 explicit，
  // "+0.0f 默认值 Set 过也上线"一并覆盖。golden = 手推位模式常量，不经任何浮点库函数
  const uint32_t bit_cases[] = {
      FloatBitsOf(0U, 0x7FU, 1U << 22U),         // 1.5f：exp=127(bias)、尾数最高位
      FloatBitsOf(1U, 0U, 0U),                   // -0.0f（符号位保留）
      FloatBitsOf(0U, 0U, 0U),                   // +0.0f（explicit 默认值也上线）
      FloatBitsOf(0U, 0xFFU, 0U),                // +Inf
      FloatBitsOf(1U, 0xFFU, 0U),                // -Inf
      FloatBitsOf(0U, 0xFFU, (1U << 22U) | 1U),  // quiet NaN 带 payload（位模式原样保留）
  };
  for (const uint32_t bits : bit_cases) {
    TestWireDocument doc;
    doc.MutableDoc().SetRatio(FloatFromBits(bits));
    const Bytes golden = DocWire(Tlv(kTagDocRatio, Le(bits, 4U)));
    const Bytes bytes = SerializeOk(doc);
    EXPECT_EQ(bytes, golden) << "bits=" << bits;

    TestWireDocument decoded;
    ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk) << "bits=" << bits;
    EXPECT_EQ(FloatToBits(decoded.GetDoc().GetRatio()), bits) << "bits=" << bits;
    EXPECT_TRUE(decoded.GetDoc().HasRatio()) << "bits=" << bits;
    EXPECT_EQ(SerializeOk(decoded), bytes) << "bits=" << bits;  // 解码再编码位级恒等
  }
}

TEST(WireTlvFrameTest, WireTlvField_ShouldCarryDoubleFixedWidth_WhenImplicitPresenceTrimsDefaults) {
  TestWireDocument doc;
  doc.MutableDoc().SetName("w");
  const Bytes no_weight = DocWire(Tlv(kTagDocName, Str("w")));
  EXPECT_EQ(SerializeOk(doc), no_weight);  // 未 Set：不上线

  doc.MutableDoc().SetWeight(0.0);
  EXPECT_EQ(SerializeOk(doc), no_weight);  // implicit：值==缺省被裁剪

  // 已知坑固化：implicit 浮点以 value==T{} 判缺省，-0.0 == 0.0 比较相等 → 同样不上线、符号丢失。
  // 需保真 -0.0 必须声明 explicit（如 Ratio_）——见头注释「值编码约定」的告警
  doc.MutableDoc().SetWeight(-0.0);
  EXPECT_EQ(SerializeOk(doc), no_weight);

  // 非缺省值：8B 定宽位模式，golden 手推（2.5 = 1.25×2^1 → exp 1024(bias+1)、尾数 bit50）
  doc.MutableDoc().SetWeight(2.5);
  Bytes payload = Tlv(kTagDocName, Str("w"));
  Append(payload, Tlv(kTagDocWeight, Le(DoubleBitsOf(0U, 0x400U, 1ULL << 50U), 8U)));
  const Bytes golden = DocWire(payload);
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);
  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  EXPECT_EQ(decoded.GetDoc().GetWeight(), 2.5);
}

TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRejectMalformedFloat_WhenPayloadWidthMismatch) {
  // 定宽编码：负载宽度 != 类型宽度一律 kInvalidFormat（float 4B / double 8B）
  const size_t bad_float_widths[] = {3U, 5U};
  for (const size_t width : bad_float_widths) {
    const Bytes bad = DocWire(Tlv(kTagDocRatio, Bytes(width, 0U)));
    TestWireDocument doc;
    EXPECT_EQ(doc.Deserialize(bad.data(), bad.size()), wire::WireStatus::kInvalidFormat) << "width=" << width;
  }
  const Bytes bad_double = DocWire(Tlv(kTagDocWeight, Bytes(7U, 0U)));
  TestWireDocument doc;
  EXPECT_EQ(doc.Deserialize(bad_double.data(), bad_double.size()), wire::WireStatus::kInvalidFormat);
}

// ----------------------------------------------
// 通用打包数组（无 count 前缀）：zigzag/裸 varint 自终止、定宽按宽度切分、string 长度前缀
// ----------------------------------------------

TEST(WireTlvFrameTest, WireTlvField_ShouldRoundTripPackedArrays_WhenNoCountAndMixedElementKinds) {
  TestWireDocument doc;
  auto &item = doc.AddItem();
  item.SetScores({1, -2, 300, -1});
  item.SetIds({-1LL, (1LL << 40)});  // -1（未知维语义）+ 2^40（varint 5B）
  item.SetRates({1.5F, -0.0F});
  item.SetMasses({2.5});
  item.SetFlags({true, false, true});

  // golden：元素编码直拼、无 count——zigzag varint / IEEE754 定宽 / bool 1B
  Bytes scores;
  for (const int32_t v : {1, -2, 300, -1}) {
    Append(scores, ZzVarint(v));
  }
  Bytes ids;
  for (const int64_t v : {-1LL, (1LL << 40)}) {
    Append(ids, ZzVarint(v));
  }
  Bytes rates;
  for (const float v : {1.5F, -0.0F}) {
    Append(rates, Le(FloatToBits(v), 4U));
  }
  Bytes payload;
  Append(payload, Tlv(kTagItemScores, scores));
  Append(payload, Tlv(kTagItemIds, ids));
  Append(payload, Tlv(kTagItemRates, rates));
  Append(payload, Tlv(kTagItemFlags, Bytes{1U, 0U, 1U}));
  Append(payload, Tlv(kTagItemMasses, Le(DoubleToBits(2.5), 8U)));
  const Bytes golden = ItemWire(payload);
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(decoded.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_EQ(out->GetScores(), (std::vector<int32_t>{1, -2, 300, -1}));
  EXPECT_EQ(out->GetIds(), (std::vector<int64_t>{-1LL, (1LL << 40)}));
  EXPECT_EQ(out->GetMasses(), (std::vector<double>{2.5}));
  EXPECT_EQ(out->GetFlags(), (std::vector<bool>{true, false, true}));
  ASSERT_EQ(out->GetRates().size(), 2U);
  EXPECT_EQ(FloatToBits(out->GetRates()[0]), FloatToBits(1.5F));
  EXPECT_EQ(FloatToBits(out->GetRates()[1]), FloatBitsOf(1U, 0U, 0U));  // -0.0f 符号位保真
  EXPECT_EQ(SerializeOk(decoded), bytes);
}

TEST(WireTlvFrameTest, WireTlvField_ShouldRoundTripPackedStringArray_WhenEmptyAndMultiElements) {
  TestWireDocument doc;
  doc.AddItem().SetNames({"ab", "", "xyz"});

  // golden：每元素只有长度 varint 前缀，无重复 tag、无 count；空串元素 = 1B 长度前缀 0
  Bytes arr = Varint(2U);
  Append(arr, Str("ab"));
  Append(arr, Varint(0U));
  Append(arr, Varint(3U));
  Append(arr, Str("xyz"));
  const Bytes golden = ItemWire(Tlv(kTagItemNames, arr));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, golden);

  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(decoded.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_EQ(out->GetNames(), (std::vector<std::string>{"ab", "", "xyz"}));
  EXPECT_EQ(SerializeOk(decoded), bytes);
}

TEST(WireTlvFrameTest, WireTlvDecoder_ShouldRejectMalformedPackedArray_WhenWidthOrRangeInvalid) {
  const struct {
    const char *desc;
    uint16_t tag;
    Bytes payload;
  } cases[] = {
      {"float 负载不按 4B 整除", kTagItemRates, Bytes(5U, 0U)},
      {"bool 元素值域超 0/1", kTagItemFlags, Bytes{1U, 2U}},
      {"string 元素长度越过帧负载", kTagItemNames, Varint(9U)},
      {"int32 zigzag 解码越值域", kTagItemScores, ZzVarint(static_cast<int64_t>(INT32_MAX) + 1)},
      {"varint 收尾截断（continuation 位悬挂）", kTagItemScores, Bytes{0x80U}},
  };
  for (const auto &c : cases) {
    const Bytes bad = ItemWire(Tlv(c.tag, c.payload));
    TestWireDocument doc;
    EXPECT_EQ(doc.Deserialize(bad.data(), bad.size()), wire::WireStatus::kInvalidFormat) << c.desc;
  }
  // 空负载 = 0 元素，合法（恰好耗尽的退化情形）
  const Bytes empty_arr = ItemWire(Tlv(kTagItemScores, {}));
  TestWireDocument doc;
  ASSERT_EQ(doc.Deserialize(empty_arr.data(), empty_arr.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(doc.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_TRUE(out->GetScores().empty());
}

// ----------------------------------------------
// 嵌套单值记录字段（WireTlvRecordField）与记录级 DecodePayload 钩子
// ----------------------------------------------

TEST(WireTlvFrameTest, WireTlvRecordField_ShouldEncodePresenceFrame_WhenSetEmptyOrUnset) {
  // 未 Set：整帧不上线
  TestWireDocument bare;
  bare.AddItem();
  const Bytes bare_wire = ItemWire({});
  EXPECT_EQ(SerializeOk(bare), bare_wire);

  // Set 过空 message：tag + 0 长度帧照样上线（= pb has_x()=true 且子字段全缺省的中间态）
  TestWireDocument empty_msg;
  empty_msg.AddItem().SetMainPair(TestWirePair{});
  const Bytes empty_wire = ItemWire(Tlv(kTagItemMainPair, {}));
  const Bytes empty_bytes = SerializeOk(empty_msg);
  EXPECT_EQ(empty_bytes, empty_wire);
  TestWireDocument decoded_empty;
  ASSERT_EQ(decoded_empty.Deserialize(empty_bytes.data(), empty_bytes.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(decoded_empty.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_TRUE(out->HasMainPair());
  EXPECT_EQ(out->GetMainPair().GetAddress(), 0U);

  // Set 非空：子记录注册表帧嵌入外层帧
  TestWirePair pair;
  pair.SetAddress(5U);
  pair.SetSize(6U);
  TestWireDocument doc;
  doc.AddItem().SetMainPair(pair);
  Bytes inner;
  Append(inner, Tlv(kTagPairAddress, Varint(5U)));
  Append(inner, Tlv(kTagPairSize, Varint(6U)));
  const Bytes bytes = SerializeOk(doc);
  EXPECT_EQ(bytes, ItemWire(Tlv(kTagItemMainPair, inner)));
  TestWireDocument decoded;
  ASSERT_EQ(decoded.Deserialize(bytes.data(), bytes.size()), wire::WireStatus::kOk);
  ASSERT_EQ(decoded.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_EQ(out->GetMainPair().GetAddress(), 5U);
  EXPECT_EQ(out->GetMainPair().GetSize(), 6U);
}

TEST(WireTlvFrameTest, WireTlvDecoder_ShouldApplyOneofHookSemantics_WhenBranchFramesRepeatOrMisplaced) {
  // a) 两个分支帧都上线（畸形 oneof 流）：钩子先清再写 → 互斥 + 后写生效，只留 B
  Bytes both;
  Append(both, Tlv(kTagBranchA, Varint(7U)));
  Append(both, Tlv(kTagBranchB, Str("s")));
  const Bytes wire_both = ItemWire(Tlv(kTagItemBranch, both));
  TestWireDocument doc_a;
  ASSERT_EQ(doc_a.Deserialize(wire_both.data(), wire_both.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(doc_a.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_TRUE(out->HasBranch());
  EXPECT_FALSE(out->GetBranch().HasA());
  EXPECT_TRUE(out->GetBranch().HasB());
  EXPECT_EQ(out->GetBranch().GetB(), "s");

  // b) 已知他层 tag 漏进分支记录：钩子判层次错误
  const Bytes misplaced = ItemWire(Tlv(kTagItemBranch, Tlv(kTagCellId, Varint(1U))));
  TestWireDocument doc_b;
  EXPECT_EQ(doc_b.Deserialize(misplaced.data(), misplaced.size()), wire::WireStatus::kInvalidFormat);

  // c) 未知 tag：钩子跳过（前向兼容），正常分支不受影响
  Bytes with_unknown;
  Append(with_unknown, Tlv(kTagUnknown, Str("zz")));
  Append(with_unknown, Tlv(kTagBranchA, Varint(9U)));
  const Bytes wire_c = ItemWire(Tlv(kTagItemBranch, with_unknown));
  TestWireDocument doc_c;
  ASSERT_EQ(doc_c.Deserialize(wire_c.data(), wire_c.size()), wire::WireStatus::kOk);
  ASSERT_EQ(doc_c.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_TRUE(out->GetBranch().HasA());
  EXPECT_EQ(out->GetBranch().GetA(), 9U);
}

TEST(WireTlvFrameTest, WireTlvRecordField_ShouldMergeInPlace_WhenFieldFrameRepeated) {
  // 同一 RecordField tag 出现两帧：就地按帧序合并——出现帧覆盖、缺席帧保持旧值（pb merge 同源语义）
  Bytes frame1;
  Append(frame1, Tlv(kTagPairAddress, Varint(1U)));
  Append(frame1, Tlv(kTagPairSize, Varint(2U)));
  Bytes records = Tlv(kTagItemMainPair, frame1);
  Append(records, Tlv(kTagItemMainPair, Tlv(kTagPairSize, Varint(3U))));
  const Bytes wire = ItemWire(records);
  TestWireDocument doc;
  ASSERT_EQ(doc.Deserialize(wire.data(), wire.size()), wire::WireStatus::kOk);
  const TestWireItem *out = nullptr;
  ASSERT_EQ(doc.GetItem(0U, out), wire::WireStatus::kOk);
  EXPECT_TRUE(out->HasMainPair());
  EXPECT_EQ(out->GetMainPair().GetAddress(), 1U);  // 帧2 未带 Address → 保持帧1 的值
  EXPECT_EQ(out->GetMainPair().GetSize(), 3U);     // 帧2 覆盖
}

// 与计算TLV中value长度时 防护嵌套深度增长下的遍历次数指数增加场景
template <int D>
void ExpectDeepChainBounded(const int64_t limit) {
  g_deep_reg_calls = 0;  // 计数窗口只圈编码段
  auto chain = MakeDeepChain<D>(7U);
  wire::WireTlvEncoder encoder;
  encoder.WriteRecord(kTagDeepRoot, chain);
  ASSERT_EQ(encoder.status, WireStatus::kOk);
  EXPECT_LE(g_deep_reg_calls, limit) << "depth=" << D << " calls=" << g_deep_reg_calls;

  // 短路不得改变线格式：深链 roundtrip 还原叶值，再编码字节恒等
  TestWireDeepChain<D> decoded;
  bool root_seen = false;
  const auto st = wire::ProcessRecords(encoder.bytes.data(), encoder.bytes.size(),
                                       [&](uint16_t tag, const uint8_t *payload, size_t size) {
                                         if (tag != kTagDeepRoot) {
                                           return WireStatus::kInvalidFormat;
                                         }
                                         root_seen = true;
                                         return wire::WireTlvDecodeRecord<TestWireDeepSchema>(payload, size, decoded);
                                       });
  ASSERT_EQ(st, WireStatus::kOk) << "depth=" << D;
  ASSERT_TRUE(root_seen) << "depth=" << D;
  EXPECT_EQ(DeepLeafValue(decoded), 7U) << "depth=" << D;
  wire::WireTlvEncoder re_encoder;
  re_encoder.WriteRecord(kTagDeepRoot, decoded);
  EXPECT_EQ(re_encoder.bytes, encoder.bytes) << "depth=" << D;
}

TEST(WireTlvDeepChainTest, WireTlvEncoder_ShouldKeepTraversalPolynomial_WhenRecordChainDeepens) {
  ExpectDeepChainBounded<5>(100);
  ExpectDeepChainBounded<10>(300);
  ExpectDeepChainBounded<20>(1000);
}

TEST(WireTlvMapTest, WireTlvDecoder_ShouldRejectKeylessContainer_WhenFollowingValidContainer) {
  Bytes valid_container = Tlv(kTagDocProps, PropEntryFrames("k", true, 7U, false, ""));
  Bytes keyless_container = Tlv(kTagDocProps, Tlv(kTagPropValue, Varint(9U)));
  Bytes doc_payload = valid_container;
  Append(doc_payload, keyless_container);
  const Bytes bad_wire = DocWire(doc_payload);

  // 先解成功"仅合法帧"的流，再解畸形流：失败且已解码数据不被污染
  TestWireDocument doc;
  const Bytes ok_wire = DocWire(valid_container);
  ASSERT_EQ(doc.Deserialize(ok_wire.data(), ok_wire.size()), wire::WireStatus::kOk);
  ASSERT_EQ(doc.GetDoc().GetPropCount(), 1U);
  const TestWireProp *prop = nullptr;
  ASSERT_EQ(doc.GetDoc().GetProp(0U, prop), wire::WireStatus::kOk);
  ASSERT_EQ(prop->GetValue(), 7U);

  EXPECT_EQ(doc.Deserialize(bad_wire.data(), bad_wire.size()), wire::WireStatus::kInvalidFormat);
  // 无部分提交：第一帧条目保持 {k,7}，不得被改成 value=9、也不得清空
  ASSERT_EQ(doc.GetDoc().GetPropCount(), 1U);
  ASSERT_EQ(doc.GetDoc().GetProp(0U, prop), wire::WireStatus::kOk);
  EXPECT_EQ(prop->GetName(), "k");
  EXPECT_EQ(prop->GetValue(), 7U);
}
