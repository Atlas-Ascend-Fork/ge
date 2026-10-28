/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// 通用嵌套 TLV 线格式框架（WireTLV）：16 字节公共头 + `tag:u16 + length:u16 + value` 记录 + LEB128 varint 值编码。
//
// 本头只承载与业务无关的机制：帧切分、小端/varint/zigzag 原语、按 RegisterMembers 表驱动的编解码、
// 两类 presence 的上线规则（implicit 裁剪类型默认值 / explicit 不裁剪）、错误码。
// 业务侧通过 **Schema 策略模板参数** 注入协议身份与 tag 全集：
//
//   struct XxxSchema {
//     static constexpr uint32_t kMagic;      // 公共头 offset 0，小端 u32
//     static constexpr uint16_t kVersion;    // 公共头 offset 4，小端 u16；消费端版本门禁
//     static bool IsKnownTag(uint16_t tag);  // 该协议的全部 tag，用于"已知 tag 出现在非法层次"检测
//   };
//
// 使用方式：记录类用 WIRE_FIELD/WIRE_FIELD_EXPLICIT/WIRE_FIELD_COLLECTION 声明字段，并私有实现
// `template <typename Visitor> void RegisterMembers(Visitor &visitor)`（按 `visitor.VisitMember(tag, 成员)`
// 顺序列出字段清单，编码与解码共用同一张表）；顶层文档对象负责 magic/version/record_count 与自身顶层 tag 的分派，
// 可复用 WriteCommonHeader / FillWireTlvTotalSize / CheckWireTlvHeader / DecodeWireTlvRecords / WireTlvDecodeRecord。
//
// 与 inc/framework/common/tlv/tlv.h 不是同一代格式：那里是 `TlvHead{type:u32, len:u32, data[0]}` 的扁平
// 定宽 TLV（nano_common_desc / nano_dbg_desc / aicpu_resources 等在用），无嵌套记录、无 varint、无公共头，
// 两者不可混用。
//
// 值编码约定（对全部使用该框架的协议一致）：
//   标量     = 规范 LEB128 varint；有符号类型先 zigzag 折叠
//   bool     = 定宽 1 字节，只允许 0/1（定宽使其不存在非规范编码）
//   string   = 裸字节，长度由所在记录 length 给出
//   vector<uint64_t> = count varint + 每元素 zigzag varint（即"维度"语义：元素按 int64 解释，-1 表示未知维）
// 若将来某模块需要"不做 zigzag 的裸 u64 数组"或新值类型，必须新增值编码并走版本门禁，
// 不得就地修改 vector<uint64_t> 这条重载的语义。
#ifndef FRAMEWORK_COMMON_WIRE_WIRE_TLV_H_
#define FRAMEWORK_COMMON_WIRE_WIRE_TLV_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ge {
namespace wire {
enum class WireStatus : uint32_t {
  kOk = 0,
  kInvalidParam,
  kInvalidFormat,
  kUnsupportedVersion,
  kLengthOverflow,
  kNoMemory
};

// 公共头与帧头布局常量（与业务无关）：
//   公共头 16B = magic:u32 | version:u16 | header_size:u16 | total_size:u32 | record_count:u32
//   记录帧头 4B = tag:u16 | length:u16（单条记录内容上限 65535B）
constexpr size_t kHeaderSize = 16U;
constexpr size_t kRecordHeaderSize = 4U;
constexpr size_t kMaxVarintLen = 10U;

// 单值字段的内存表示；kExplicitPresence=true 表示显式 presence（对齐 protobuf 单成员 oneof/optional 字段）：
// Set 过即上线（值等于类型默认值也写），消费端可用 Has* 判"是否下发"。
// 为 false 时对齐 proto3 plain 标量的 implicit presence：值等于类型默认值不上线，也不提供 Has*；
// 此时 present 只由解码器写入、编码器不读（编码判据是"值是否为类型默认值"），对外不可观测，
// 保留它是为了让两类字段共用同一份存储与访问器实现。
template <typename T, bool kExplicitPresence = false>
struct WireTlvField {
  bool present = false;
  T value{};
};

struct MemberRegHelper {
  template <typename Record, typename Visitor>
  static void Register(Record &record, Visitor &visitor) {
    record.RegisterMembers(visitor);
  }
};

// presence 语义与 protobuf 严格同构，单值字段分两类：
//   WIRE_FIELD          = proto3 plain 标量（implicit presence）：值等于类型默认值（0/false/""）不上线，
//                         不提供 Has*；
//   WIRE_FIELD_EXPLICIT = 单成员 oneof / optional 字段（explicit presence）：Set 过即上线（默认值也写），
//                         并提供 Has*，消费端据此区分“未下发”与“下发了默认值”。
// 两类字段的 Get* 均恒可用（无值时返回类型默认值，等价 protobuf 的 xxx()）；解码只还原线上出现的字段，
// 不回填缺省值。集合字段缺省即空集合，以 Get*Count() 判空。
// 单值字段声明宏（implicit presence）：默认选它。
// 使用场景：消费端只关心字段取值、不需要区分“未下发”与“下发了默认值”的绝大多数字段（等价 proto3 plain 标量）。
// 优点：值等于类型默认值时不上线，载荷更紧凑；代价：没有 Has*，无法判 presence。
#define WIRE_FIELD(Type, Name)        \
 public:                              \
  void Set##Name(const Type &value) { \
    Name##_.value = value;            \
    Name##_.present = true;           \
  }                                   \
  const Type &Get##Name() const {     \
    return Name##_.value;             \
  }                                   \
                                      \
 protected:                           \
  WireTlvField<Type> Name##_;

// 单值字段声明宏（explicit presence）：按需使用，不要滥用。
// 使用场景：消费端必须区分“未下发”与“下发了默认值”的字段（等价 protobuf 单成员 oneof / optional），
// 典型如用“字段是否出现”驱动分支、开关或标识拼接的场景。
// 代价：字段即使为默认值也占一条记录（4B 帧头 + varint），且消费端需按 Has* 判定，故只对确有此需求的字段使用。
#define WIRE_FIELD_EXPLICIT(Type, Name) \
 public:                                \
  void Set##Name(const Type &value) {   \
    Name##_.value = value;              \
    Name##_.present = true;             \
  }                                     \
  const Type &Get##Name() const {       \
    return Name##_.value;               \
  }                                     \
  bool Has##Name() const {              \
    return Name##_.present;             \
  }                                     \
                                        \
 protected:                             \
  WireTlvField<Type, true> Name##_;

#define WIRE_FIELD_COLLECTION(Type, Name)                        \
 public:                                                         \
  Type &Add##Name() {                                            \
    Name##_.emplace_back();                                      \
    return Name##_.back();                                       \
  }                                                              \
  size_t Get##Name##Count() const {                              \
    return Name##_.size();                                       \
  }                                                              \
  WireStatus Get##Name(size_t index, const Type *&value) const { \
    if (index >= Name##_.size()) {                               \
      return WireStatus::kInvalidParam;                          \
    }                                                            \
    value = &Name##_[index];                                     \
    return WireStatus::kOk;                                      \
  }                                                              \
                                                                 \
 private:                                                        \
  std::vector<Type> Name##_;

inline uint64_t ReadLe(const uint8_t *data, size_t width) {
  uint64_t value = 0U;
  for (size_t i = 0U; i < width; ++i) {
    value |= static_cast<uint64_t>(data[i]) << (i * 8U);
  }
  return value;
}

inline void WriteLe(uint8_t *data, uint64_t value, size_t width) {
  for (size_t i = 0U; i < width; ++i) {
    data[i] = static_cast<uint8_t>(value >> (i * 8U));
  }
}

// 有符号值(含"负值表示未知维"的语义)一律经 zigzag 折叠为无符号后再 varint,避免负数占满 10 字节。
inline uint64_t ZigZagEncode(const int64_t value) {
  return (static_cast<uint64_t>(value) << 1U) ^ static_cast<uint64_t>(value >> 63);
}

inline int64_t ZigZagDecode(const uint64_t value) {
  return static_cast<int64_t>((value >> 1U) ^ (0U - (value & 1U)));
}

// 解析一个规范(canonical)LEB128 varint。校验四件事,任一不满足即视为格式非法:
// 1) 长度不超过 10 字节;2) 第 10 字节负载不得超出 uint64 可表达范围(防移位溢出);
// 3) 拒绝非规范编码(overlong,如 0x80 0x00 表示 0),保证"解码后再编码字节一致";4) 缓冲尾不得截断。
inline WireStatus ReadVarint(const uint8_t *data, const size_t size, uint64_t &value, size_t &consumed) {
  uint64_t result = 0U;
  for (size_t i = 0U; i < size; ++i) {
    if (i >= kMaxVarintLen) {
      return WireStatus::kInvalidFormat;
    }
    const auto byte = data[i];
    const auto payload = static_cast<uint64_t>(byte & 0x7FU);
    const size_t shift = i * 7U;
    if ((shift == 63U) && (payload > 1U)) {
      return WireStatus::kInvalidFormat;
    }
    result |= payload << shift;
    if ((byte & 0x80U) == 0U) {
      if ((i > 0U) && (payload == 0U)) {
        return WireStatus::kInvalidFormat;
      }
      value = result;
      consumed = i + 1U;
      return WireStatus::kOk;
    }
  }
  return WireStatus::kInvalidFormat;
}

// 帧切分器：只做"读帧头 + 边界校验 + 回调 + 按声明长度前进"，不解释 tag 语义（语义全在 handler）。
// 因此同一函数既用于顶层文档，也用于每条记录内部（嵌套由此递归形成）；子记录的容器大小即父记录
// 的 length，孩子永远读不出父亲的范围。handler 返回非 kOk 立即上抛（短路），游标不会错位。
template <typename Handler>
WireStatus ProcessRecords(const uint8_t *data, size_t size, Handler &&handler) {
  size_t pos = 0U;
  while (pos < size) {
    if (size - pos < kRecordHeaderSize) {
      return WireStatus::kInvalidFormat;
    }
    const auto tag = static_cast<uint16_t>(ReadLe(data + pos, 2U));
    const auto length = static_cast<size_t>(ReadLe(data + pos + 2U, 2U));
    pos += kRecordHeaderSize;
    if (length > size - pos) {
      return WireStatus::kInvalidFormat;
    }
    const auto status = handler(tag, data + pos, length);
    if (status != WireStatus::kOk) {
      return status;
    }
    pos += length;
  }
  return WireStatus::kOk;
}

class WireTlvEncoder {
 public:
  std::vector<uint8_t> bytes;
  WireStatus status = WireStatus::kOk;

  void Append(uint64_t value, size_t width) {
    const size_t pos = bytes.size();
    if (Grow(width)) {
      WriteLe(bytes.data() + pos, value, width);
    }
  }

  void AppendVarint(uint64_t value) {
    while (status == WireStatus::kOk) {
      const auto byte = static_cast<uint8_t>(value & 0x7FU);
      value >>= 7U;
      Append(value != 0U ? static_cast<uint64_t>(byte | 0x80U) : static_cast<uint64_t>(byte), 1U);
      if (value == 0U) {
        break;
      }
    }
  }

  template <typename T>
  void WriteRecord(uint16_t tag, T &record) {
    const size_t pos = PreprocessRecord(tag);
    MemberRegHelper::Register(record, *this);
    PostprocessRecord(pos);
  }

  template <typename T, bool kExplicitPresence>
  void VisitMember(uint16_t tag, const WireTlvField<T, kExplicitPresence> &field) {
    // 显式 presence：Set 过即上线（默认值也写，用于表达“下发了默认值”）
    // implicit：对齐 proto3 plain 标量，值等于类型默认值时不上线；
    //           值非默认已隐含 Set 过，故无需再判 present（implicit 字段也不提供 Has*）
    bool need_write = false;
    if constexpr (kExplicitPresence) {
      need_write = field.present;
    } else {
      need_write = !(field.value == T{});
    }
    if (need_write && status == WireStatus::kOk) {
      const size_t pos = PreprocessRecord(tag);
      WriteValue(field.value);
      PostprocessRecord(pos);
    }
  }

  template <typename T>
  void VisitMember(uint16_t tag, std::vector<T> &records) {
    for (auto &record : records) {
      if (status != WireStatus::kOk) {
        break;
      }
      WriteRecord(tag, record);
    }
  }

 private:
  bool Grow(size_t count) {
    if (status != WireStatus::kOk) {
      return false;
    }
    if (count > UINT32_MAX - bytes.size()) {
      status = WireStatus::kLengthOverflow;
      return false;
    }
    bytes.resize(bytes.size() + count);
    return true;
  }

  size_t PreprocessRecord(uint16_t tag) {
    const size_t pos = bytes.size();
    Append(tag, 2U);
    // 数据段可能因为varint zigzag编码导致长度变短 这里先预留uint16用于回填记录长度
    Append(0U, 2U);
    return pos;
  }

  void PostprocessRecord(size_t pos) {
    if (status != WireStatus::kOk) {
      return;
    }
    const size_t length = bytes.size() - pos - kRecordHeaderSize;
    if (length > UINT16_MAX) {
      // 单条记录内容超出 length:u16 表达范围，整体序列化失败（无部分提交）
      status = WireStatus::kLengthOverflow;
      return;
    }
    WriteLe(bytes.data() + pos + 2U, length, 2U);
  }

  // 标量值走 varint:有符号类型先 zigzag 折叠,无符号类型与枚举(底层均为 uint32_t)直接按无符号编码
  template <typename T>
  void WriteValue(T value) {
    if constexpr (std::is_signed<T>::value) {
      AppendVarint(ZigZagEncode(static_cast<int64_t>(value)));
    } else {
      AppendVarint(static_cast<uint64_t>(value));
    }
  }

  void WriteValue(bool value) {
    Append(value ? 1U : 0U, 1U);
  }

  void WriteValue(const std::string &value) {
    const size_t pos = bytes.size();
    if (Grow(value.size())) {
      std::copy(value.begin(), value.end(), bytes.begin() + pos);
    }
  }

  // 打包的 u64 数组:count varint + 每元素 zigzag varint(维度语义,元素按 int64 解释,-1 表示未知维)
  void WriteValue(const std::vector<uint64_t> &value) {
    // 单条记录受 length:u16 上限约束且每维至少占 1 字节,超界即失败,不产出残缺字节流
    if (value.size() > UINT16_MAX) {
      status = WireStatus::kLengthOverflow;
      return;
    }
    AppendVarint(value.size());
    for (const auto dim : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(ZigZagEncode(static_cast<int64_t>(dim)));
    }
  }
};

template <typename Schema, typename Record>
WireStatus WireTlvDecodeRecord(const uint8_t *data, size_t size, Record &record);

// 单条记录的字段匹配器：用线流里的 tag 去匹配记录类型自己的 RegisterMembers 清单（类型信息来自 schema 的
// 静态声明，而非码流）。Schema 策略提供 IsKnownTag，用于区分"未知 tag（跳过，前向兼容）"与
// "已知 tag 出现在错误层次（判错，损坏检测）"。
template <typename Schema>
class WireTlvDecoder {
 public:
  WireTlvDecoder(uint16_t tag, const uint8_t *data, size_t size) : tag_(tag), data_(data), size_(size) {}
  bool matched = false;
  WireStatus status = WireStatus::kOk;

  template <typename T, bool kExplicitPresence>
  void VisitMember(uint16_t tag, WireTlvField<T, kExplicitPresence> &field) {
    if (tag == tag_) {
      matched = true;
      status = ReadValue(field.value);
      field.present = (status == WireStatus::kOk);
    }
  }

  template <typename T>
  void VisitMember(uint16_t tag, std::vector<T> &records) {
    if (tag == tag_) {
      matched = true;
      records.emplace_back();
      status = WireTlvDecodeRecord<Schema>(data_, size_, records.back());
    }
  }

 private:
  // 标量值走 varint:整条记录负载必须恰好被一个规范 varint 消耗完毕(消耗字节数 != size_ 即非法),
  // 再按目标类型做值域校验,避免定宽改可变宽后出现静默截断
  template <typename T>
  WireStatus ReadValue(T &value) const {
    uint64_t raw = 0U;
    size_t consumed = 0U;
    const auto status = ReadVarint(data_, size_, raw, consumed);
    if ((status != WireStatus::kOk) || (consumed != size_)) {
      return WireStatus::kInvalidFormat;
    }
    if constexpr (std::is_signed<T>::value) {
      const auto decoded = ZigZagDecode(raw);
      if ((decoded < static_cast<int64_t>(std::numeric_limits<T>::min())) ||
          (decoded > static_cast<int64_t>(std::numeric_limits<T>::max()))) {
        return WireStatus::kInvalidFormat;
      }
      value = static_cast<T>(decoded);
    } else {
      if constexpr (sizeof(T) < sizeof(uint64_t)) {
        if ((raw >> (sizeof(T) * 8U)) != 0U) {
          return WireStatus::kInvalidFormat;
        }
      }
      value = static_cast<T>(raw);
    }
    return WireStatus::kOk;
  }

  WireStatus ReadValue(bool &value) const {
    if (size_ != 1U || data_[0] > 1U) {
      return WireStatus::kInvalidFormat;
    }
    value = data_[0] != 0U;
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::string &value) const {
    value.assign(reinterpret_cast<const char *>(data_), size_);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<uint64_t> &value) const {
    uint64_t count = 0U;
    size_t pos = 0U;
    if (ReadVarint(data_, size_, count, pos) != WireStatus::kOk) {
      return WireStatus::kInvalidFormat;
    }
    // 每维至少占 1 字节:count 超出剩余可解析字节数即为长度溢出,同时避免为畸形 count 预留巨额内存
    if (count > static_cast<uint64_t>(size_ - pos)) {
      return WireStatus::kLengthOverflow;
    }
    // 解到临时 vector、全部成功后再交换：中途失败不触碰目标字段，避免留下已 clear/部分填充的半成品
    std::vector<uint64_t> dims;
    dims.reserve(static_cast<size_t>(count));
    for (uint64_t i = 0U; i < count; ++i) {
      uint64_t raw = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, raw, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      dims.push_back(static_cast<uint64_t>(ZigZagDecode(raw)));
    }
    if (pos != size_) {
      return WireStatus::kInvalidFormat;
    }
    value.swap(dims);
    return WireStatus::kOk;
  }

  uint16_t tag_;
  const uint8_t *data_;
  size_t size_;
};

// 解码只还原线上出现的字段（不回填缺省值）：未上线的单值字段在解码后的对象里保持 absent，
// 消费端以 Has* 判 presence（Get* 恒返回类型默认值）；集合字段缺省即空集合。
// 注意：本函数**就地**写入 record，且集合元素是逐个 emplace_back 追加的，失败即短路返回，
// 因此它本身不具备原子性（record 可能已被部分修改）。需要"失败无部分提交"的调用方
// 应解到临时对象、成功后再 swap/move 提交（参考 DumpTransportInfo::Deserialize）。
template <typename Schema, typename Record>
WireStatus WireTlvDecodeRecord(const uint8_t *data, size_t size, Record &record) {
  return ProcessRecords(data, size, [&record](uint16_t tag, const uint8_t *payload, size_t payload_size) {
    WireTlvDecoder<Schema> decoder(tag, payload, payload_size);
    MemberRegHelper::Register(record, decoder);
    if (decoder.status != WireStatus::kOk) {
      return decoder.status;
    }
    if (!decoder.matched && Schema::IsKnownTag(tag)) {
      return WireStatus::kInvalidFormat;
    }
    return WireStatus::kOk;
  });
}

// ---- 文档级（公共头）helper：magic/version 来自 Schema，record_count 与顶层 tag 分派由业务侧决定 ----

// 写 16 字节公共头；total_size 先占位 0，全部记录写完后由 FillWireTlvTotalSize 回填。
// record_count 只占 4 字节，故形参即为 uint32_t（超出请在调用侧先判并返回 kLengthOverflow，避免静默截断）
template <typename Schema>
void WriteCommonHeader(WireTlvEncoder &encoder, uint32_t record_count) {
  encoder.Append(Schema::kMagic, 4U);
  encoder.Append(Schema::kVersion, 2U);
  encoder.Append(kHeaderSize, 2U);
  encoder.Append(0U, 4U);
  encoder.Append(record_count, 4U);
}

inline void FillWireTlvTotalSize(std::vector<uint8_t> &bytes) {
  if (bytes.size() < kHeaderSize) {
    // 公共头尚未写全（编码已失败）时不回填，避免越界写
    return;
  }
  WriteLe(bytes.data() + 8U, bytes.size(), 4U);
}

// 前置条件：data 已通过 CheckWireTlvHeader（非空且 size >= kHeaderSize）；不满足时返回 0，
// 调用方按"记录数不符"判 kInvalidFormat
inline uint64_t GetRecordCountFromHeader(const uint8_t *data, size_t size) {
  if ((data == nullptr) || (size < kHeaderSize)) {
    return 0U;
  }
  return ReadLe(data + 12U, 4U);
}

// 公共头四道门：非空、(size>=16 && magic)、version、(header_size && total_size==size)
template <typename Schema>
WireStatus CheckWireTlvHeader(const uint8_t *data, size_t size) {
  if (data == nullptr) {
    return WireStatus::kInvalidParam;
  }
  if (size < kHeaderSize || ReadLe(data, 4U) != Schema::kMagic) {
    return WireStatus::kInvalidFormat;
  }
  if (ReadLe(data + 4U, 2U) != Schema::kVersion) {
    return WireStatus::kUnsupportedVersion;
  }
  if (ReadLe(data + 6U, 2U) != kHeaderSize || ReadLe(data + 8U, 4U) != size) {
    return WireStatus::kInvalidFormat;
  }
  return WireStatus::kOk;
}

// 校验公共头后按帧切分顶层记录，逐个回调 handler（顶层 tag 分派与 record_count 一致性由 Schema 侧负责）
template <typename Schema, typename Handler>
WireStatus DecodeWireTlvRecords(const uint8_t *data, size_t size, Handler &&handler) {
  const auto header_status = CheckWireTlvHeader<Schema>(data, size);
  if (header_status != WireStatus::kOk) {
    return header_status;
  }
  return ProcessRecords(data + kHeaderSize, size - kHeaderSize, std::forward<Handler>(handler));
}
}  // namespace wire
}  // namespace ge

#endif  // FRAMEWORK_COMMON_WIRE_WIRE_TLV_H_
