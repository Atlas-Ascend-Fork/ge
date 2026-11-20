/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// 通用嵌套 TLV 线格式框架（WireTLV）：16 字节公共头 + `tag:u16 + length:varint + value` 记录 + LEB128 varint 值编码。
//
// 本头只承载与业务无关的机制：帧切分、小端/varint/zigzag 原语、按 RegisterMembers 表驱动的编解码、
// 两类 presence 的上线规则（implicit 裁剪类型默认值 / explicit 不裁剪）、错误码。
// 业务侧通过 **Schema 策略模板参数** 注入协议身份与 tag 全集：
// 使用方式：记录类用 WireTlvField（标量/string/打包数组）、WireTlvRecordField（嵌套单值记录，
// 显式 presence）、std::vector（repeated message，逐元素帧）或 WireTlvMap（map 容器，序列化实现由框架
// 收敛）声明字段，并私有实现
// `template <typename Visitor> void RegisterMembers(Visitor &visitor)`（按 `visitor.VisitMember(tag, 成员)`
// 顺序列出字段清单，编码与解码共用同一张表）；记录类型另可实现 public `DecodePayload(const uint8_t*, size_t)`
// 钩子接管本记录载荷的解码（oneof 互斥、map upsert 等 schema 语义，框架经 HasDecodePayloadMember 自动分派）；
// 顶层文档对象负责 magic/version/record_count 与自身顶层 tag 的分派，
// 可复用 WriteCommonHeader / FillWireTlvTotalSize / CheckWireTlvHeader / DecodeWireTlvRecords / WireTlvDecodeRecord。
//
// 值编码约定（对全部使用该框架的协议一致）：
//   标量     = 规范 LEB128 varint；有符号类型先 zigzag 折叠
//   bool     = 定宽 1 字节，只允许 0/1（定宽使其不存在非规范编码）
//   float/double = IEEE 754 位模式小端定宽（4B/8B）：不经数值转换，NaN/±Inf/-0.0 等特殊值位级保真
//   string   = 裸字节，长度由所在记录 length 给出
//   vector<uint64_t> = 每元素 zigzag varint 直拼（"维度"语义：元素按 int64 解释，-1 表示未知维；
//              无 count 前缀，与通用打包数组同形态——u64 类型被维度语义独占，故 zigzag 而非裸 varint）
//   vector<T> 通用打包数组（T = int64/int32/uint32/bool/float/double/string）= 元素按packed方式直拼：
//              帧 length 界定元素区，varint 元素靠规范编码自终止切分、定宽元素按宽度切分、
//              有符号元素 zigzag、无符号裸 varint、bool 定宽 1B、浮点 IEEE754 定宽
// 若将来某模块需要"不做 zigzag 的裸 u64 数组"或新值类型，必须新增值编码并走版本门禁，
// 不得就地修改 vector<uint64_t> 这条重载的语义。
// [注意]浮点数字段必须声明为 explicit presence，否则会导致 NaN/±Inf/-0.0 语义丢失。
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

#include "securec.h"

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
//   记录帧头 = tag:u16 定宽 + length 规范 LEB128 varint（宽度 1~5B 随记录长度浮动，见 kMaxU32VarintLen）
// length 的值域与公共头 total_size:u32 对齐（≤ UINT32_MAX），故不存在能表达却装不进 total_size 的长度；
// 解码侧对更宽的 varint（超过 5B）一律判 kInvalidFormat，不放宽到 u64。
constexpr size_t kBitsPerByte = 8U;
constexpr size_t kMagicSize = sizeof(uint32_t);
constexpr size_t kVersionSize = sizeof(uint16_t);
constexpr size_t kHeaderSizeFieldSize = sizeof(uint16_t);
constexpr size_t kTotalSizeFieldSize = sizeof(uint32_t);
constexpr size_t kRecordCountFieldSize = sizeof(uint32_t);
constexpr size_t kMagicOffset = 0U;
constexpr size_t kVersionOffset = kMagicOffset + kMagicSize;
constexpr size_t kHeaderSizeOffset = kVersionOffset + kVersionSize;
constexpr size_t kTotalSizeOffset = kHeaderSizeOffset + kHeaderSizeFieldSize;
constexpr size_t kRecordCountOffset = kTotalSizeOffset + kTotalSizeFieldSize;
constexpr size_t kHeaderSize = kRecordCountOffset + kRecordCountFieldSize;
constexpr size_t kRecordTagSize = sizeof(uint16_t);
// 帧头最小宽度 = tag:u16 定宽 + length varint 最短 1B；length 变宽后不存在编译期固定的帧头宽度/偏移
constexpr size_t kRecordMinHeaderSize = kRecordTagSize + sizeof(uint8_t);
constexpr size_t kVarintPayloadBits = 7U;
// length 是规范 LEB128 varint：u32 值域最多占 5B，故 kRecordLengthSize / kRecordLengthOffset
constexpr size_t kUint32Bits = sizeof(uint32_t) * kBitsPerByte;
constexpr size_t kMaxU32VarintLen = (kUint32Bits + kVarintPayloadBits - 1U) / kVarintPayloadBits;
constexpr size_t kUint64TopBitShift = std::numeric_limits<uint64_t>::digits - 1U;
constexpr size_t kMaxVarintLen = (std::numeric_limits<uint64_t>::digits + kVarintPayloadBits - 1U) / kVarintPayloadBits;
constexpr uint8_t kVarintPayloadMask = 0x7FU;
constexpr uint8_t kVarintContinuationMask = 0x80U;
constexpr size_t kBoolEncodedSize = sizeof(uint8_t);
constexpr uint8_t kBoolFalse = 0U;
constexpr uint8_t kBoolTrue = 1U;

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

// 嵌套单值记录字段（对齐 protobuf singular message 字段）：**天然 explicit presence**——
// Set 过即上线，空 message 也写（tag + 0 长度帧），帧出现本身就是 presence 信号
// （即 pb 的 has_xxx()=true 而子字段全缺省的中间态）；消费端用 Has* 判"是否下发"。
// 解码为就地写入：同一 tag 帧重复出现时按帧序合并（单值字段覆盖、集合字段追加，
// 与 WireTlvDecodeRecord 的就地语义同源）；oneof 互斥清空等更强语义由 DecodePayload 钩子承载。
template <typename T>
struct WireTlvRecordField {
  bool present = false;
  T value{};
};

// map 容器成员：框架内部实现
// 整个 map 编码为恰一个容器帧（容器 tag + varint(length) + key1 + value1 + key2 + value2 + ...）
// 容器内以「必现 key tag 再现」划分条目边界：每见到 Entry::kWireMapKeyTag 即开启新条目；
// Entry 类型必须满足的三条约定（框架不做运行时校验也不兜底，由 schema 声明与 golden UT 约束保证）：
//   1. 提供 `static constexpr uint16_t kWireMapKeyTag`
//      key 字段声明为显式 presence且 key 成员在注册顺序中排第一。
//   2. Entry 的直接成员 tag 相互唯一；嵌套子记录的 tag 不受此约束
//   3. 仅 map 语义集合可用本类型；无必现边界字段的普通结构体不能作map进行注册
// 解码结果与 vector<Entry> 同语义：同 key 多条目按线序全部保留，「同 key 后写生效」的 map upsert
template <typename Entry>
struct WireTlvMap {
  std::vector<Entry> entries;
};

struct MemberRegHelper {
  template <typename Record, typename Visitor>
  static void Register(Record &record, Visitor &visitor) {
    record.RegisterMembers(visitor);
  }
};

// 记录级解码钩子探测：记录类型实现 public `WireStatus DecodePayload(const uint8_t *, size_t)` 时，
// WireTlvDecodeRecord 改为调用钩子、由 schema 层接管该记录载荷的整个解码
// （oneof 分支互斥清空、map 同 key upsert、退役 tag 拒绝门禁等语义，见 AICPU schema v3 用法）。
// 钩子内部不得再对同一类型调用 WireTlvDecodeRecord（无限递归）；解码子记录请对相应子对象调用。
template <typename T, typename = void>
struct HasDecodePayloadMember : std::false_type {};

template <typename T>
struct HasDecodePayloadMember<T, std::void_t<decltype(std::declval<T &>().DecodePayload(
                                     std::declval<const uint8_t *>(), std::declval<size_t>()))>> : std::true_type {};

// 字段辅助函数集中维护读写、presence 和集合越界检查，schema 类只提供具有业务语义的访问函数。
template <typename T, bool kExplicitPresence>
inline void SetWireTlvField(WireTlvField<T, kExplicitPresence> &field, const T &value) {
  field.value = value;
  field.present = true;
}

template <typename T, bool kExplicitPresence>
inline const T &GetWireTlvField(const WireTlvField<T, kExplicitPresence> &field) {
  return field.value;
}

template <typename T>
inline bool HasWireTlvField(const WireTlvField<T, true> &field) {
  return field.present;
}

template <typename T>
inline void SetWireTlvRecordField(WireTlvRecordField<T> &field, T value) {
  field.value = std::move(value);
  field.present = true;
}

template <typename T>
inline const T &GetWireTlvRecordField(const WireTlvRecordField<T> &field) {
  return field.value;
}

template <typename T>
inline bool HasWireTlvRecordField(const WireTlvRecordField<T> &field) {
  return field.present;
}

template <typename T>
inline T &AddWireTlvItem(std::vector<T> &items) {
  items.emplace_back();
  return items.back();
}

template <typename T>
inline size_t GetWireTlvItemCount(const std::vector<T> &items) {
  return items.size();
}

template <typename T>
inline WireStatus GetWireTlvItem(const std::vector<T> &items, const size_t index, const T *&value) {
  if (index >= items.size()) {
    return WireStatus::kInvalidParam;
  }
  value = &items[index];
  return WireStatus::kOk;
}

// map 容器的条目访问器：与 std::vector 版同名同语义，schema 把成员类型从 vector<T> 换成
// WireTlvMap<T> 后，Add*/Get* 访问器代码无需改动
template <typename T>
inline T &AddWireTlvItem(WireTlvMap<T> &map) {
  map.entries.emplace_back();
  return map.entries.back();
}

template <typename T>
inline size_t GetWireTlvItemCount(const WireTlvMap<T> &map) {
  return map.entries.size();
}

template <typename T>
inline WireStatus GetWireTlvItem(const WireTlvMap<T> &map, const size_t index, const T *&value) {
  if (index >= map.entries.size()) {
    return WireStatus::kInvalidParam;
  }
  value = &map.entries[index];
  return WireStatus::kOk;
}

inline uint64_t ReadLe(const uint8_t *data, size_t width) {
  uint64_t value = 0U;
  for (size_t i = 0U; i < width; ++i) {
    value |= static_cast<uint64_t>(data[i]) << (i * kBitsPerByte);
  }
  return value;
}

inline void WriteLe(uint8_t *data, uint64_t value, size_t width) {
  for (size_t i = 0U; i < width; ++i) {
    data[i] = static_cast<uint8_t>(value >> (i * kBitsPerByte));
  }
}

// 有符号值(含"负值表示未知维"的语义)一律经 zigzag 折叠为无符号后再 varint,避免负数占满 10 字节。
inline uint64_t ZigZagEncode(const int64_t value) {
  const uint64_t unsignedValue = static_cast<uint64_t>(value);
  const uint64_t signMask = 0U - (unsignedValue >> kUint64TopBitShift);
  return (unsignedValue << 1U) ^ signMask;
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
    const auto payload = static_cast<uint64_t>(byte & kVarintPayloadMask);
    const size_t shift = i * kVarintPayloadBits;
    if ((shift == kUint64TopBitShift) && (payload > 1U)) {
      return WireStatus::kInvalidFormat;
    }
    result |= payload << shift;
    if ((byte & kVarintContinuationMask) == 0U) {
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

// 规范 LEB128 varint 的编码字节宽度计算 用于计数态直接累加"帧头 + length + 负载"尺寸
inline size_t VarintWidth(const uint64_t value) {
  uint64_t rest = value;
  size_t width = 1U;
  while (rest >= (1ULL << kVarintPayloadBits)) {
    rest >>= kVarintPayloadBits;
    ++width;
  }
  return width;
}

// 帧切分器：只做"读帧头 + 边界校验 + 回调 + 按声明长度前进"，不解释 tag 语义
// 因此同一函数既用于顶层文档，也用于每条记录内部（嵌套由此递归形成）；子记录的容器大小即父记录
// 的 length，孩子永远读不出父亲的范围。handler 返回非 kOk 立即上抛（短路），游标不会错位。
// 帧头 = tag:u16 定宽 + length:规范 LEB128 varint（变宽），故帧头只保证最小宽度 kRecordMinHeaderSize，
// length varint 自身的截断与非规范编码由 ReadVarint 判定。
template <typename Handler>
WireStatus ProcessRecords(const uint8_t *data, size_t size, Handler &&handler) {
  size_t pos = 0U;
  while (pos < size) {
    if (size - pos < kRecordMinHeaderSize) {
      // 连最小帧头（tag 2B + length varint 首字节）都放不下
      return WireStatus::kInvalidFormat;
    }
    const auto tag = static_cast<uint16_t>(ReadLe(data + pos, kRecordTagSize));
    // length varint 可越过 kRecordMinHeaderSize 继续延伸，故按剩余全部字节交给 ReadVarint 判截断
    uint64_t length = 0U;
    size_t length_bytes = 0U;
    const auto length_status =
        ReadVarint(data + pos + kRecordTagSize, size - pos - kRecordTagSize, length, length_bytes);
    if (length_status != WireStatus::kOk) {
      return length_status;
    }
    if (length > UINT32_MAX) {
      // TLV 中的 length 不可能超出 u32 值域
      return WireStatus::kInvalidFormat;
    }
    pos += kRecordTagSize + length_bytes;
    if (length > size - pos) {
      return WireStatus::kInvalidFormat;
    }
    const auto status = handler(tag, data + pos, static_cast<size_t>(length));
    if (status != WireStatus::kOk) {
      return status;
    }
    pos += static_cast<size_t>(length);
  }
  return WireStatus::kOk;
}

class WireTlvEncoder {
 public:
  std::vector<uint8_t> bytes;
  WireStatus status = WireStatus::kOk;

  void Append(uint64_t value, size_t width) {
    if (status != WireStatus::kOk) {
      return;
    }
    const size_t pos = bytes.size();
    if (Grow(width)) {
      if (!in_count_mode_) {
        WriteLe(bytes.data() + pos, value, width);
      }
    }
  }

  void AppendVarint(uint64_t value) {
    while (status == WireStatus::kOk) {
      const auto byte = static_cast<uint8_t>(value & kVarintPayloadMask);
      value >>= kVarintPayloadBits;
      Append(value != 0U ? static_cast<uint64_t>(byte | kVarintContinuationMask) : static_cast<uint64_t>(byte),
             sizeof(uint8_t));
      if (value == 0U) {
        break;
      }
    }
  }

  // length 是变宽 varint，宽度取决于负载长度，无法先占位再回填，故用"先数后写"：方式
  // 第一遍以计数模式量出负载字节数，再一次性写 tag + varint(length) + 负载。
  template <typename T>
  void WriteRecord(uint16_t tag, T &record) {
    const size_t payload_size = CountRecordSize(record);
    if (status != WireStatus::kOk) {
      return;
    }
    // 计数态不需要再遍历子记录了 直接根据 count 结果返回即可
    if (in_count_mode_) {
      (void)Grow(kRecordTagSize + VarintWidth(payload_size) + payload_size);
      return;
    }
    Append(tag, kRecordTagSize);
    AppendVarint(payload_size);
    MemberRegHelper::Register(record, *this);
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
      EncodeRecord(tag, field.value);
    }
  }

  // 嵌套单值记录字段：Set 过即写"字段 tag + 记录帧 + 子字段"（空 message 也上线，负载 0 的帧
  // 就是 presence 信号）。count-first 复用 WriteRecord 同一机制，子记录负载先数后写
  template <typename T>
  void VisitMember(uint16_t tag, WireTlvRecordField<T> &field) {
    if (field.present) {
      WriteRecord(tag, field.value);
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

  // map 容器编码：多个 KV对 共用一个 TLV 帧头。
  template <typename Entry>
  void VisitMember(uint16_t tag, WireTlvMap<Entry> &map) {
    if (map.entries.empty()) {
      return;
    }
    const size_t payload_size = CountMapEntriesSize(map);
    if (status != WireStatus::kOk) {
      return;
    }
    if (in_count_mode_) {
      // 计数态不需要再遍历子记录了 直接根据 count 结果返回即可
      (void)Grow(kRecordTagSize + VarintWidth(payload_size) + payload_size);
      return;
    }
    Append(tag, kRecordTagSize);
    AppendVarint(payload_size);
    for (auto &entry : map.entries) {
      if (status != WireStatus::kOk) {
        break;
      }
      MemberRegHelper::Register(entry, *this);
    }
  }

 private:
  template <typename T>
  void EncodeRecord(uint16_t tag, const T &value) {
    const size_t value_size = CountValueSize(value);
    if (status != WireStatus::kOk) {
      return;
    }
    Append(tag, kRecordTagSize);
    AppendVarint(value_size);
    WriteValue(value);
  }

  // record 容器大小计算
  template <typename T>
  size_t CountRecordSize(T &record) {
    const size_t saved_size = counted_size_;
    const bool saved_mode = in_count_mode_;
    counted_size_ = 0U;
    in_count_mode_ = true;
    MemberRegHelper::Register(record, *this);
    in_count_mode_ = saved_mode;
    const size_t result = counted_size_;
    counted_size_ = saved_size;
    return result;
  }

  template <typename T>
  size_t CountValueSize(const T &value) {
    const size_t saved_size = counted_size_;
    const bool saved_mode = in_count_mode_;
    counted_size_ = 0U;
    in_count_mode_ = true;
    WriteValue(value);
    in_count_mode_ = saved_mode;
    const size_t result = counted_size_;
    counted_size_ = saved_size;
    return result;
  }

  // map 容器大小计算
  template <typename Entry>
  size_t CountMapEntriesSize(WireTlvMap<Entry> &map) {
    const size_t saved_size = counted_size_;
    const bool saved_mode = in_count_mode_;
    counted_size_ = 0U;
    in_count_mode_ = true;
    for (auto &entry : map.entries) {
      if (status != WireStatus::kOk) {
        break;
      }
      MemberRegHelper::Register(entry, *this);
    }
    in_count_mode_ = saved_mode;
    const size_t result = counted_size_;
    counted_size_ = saved_size;
    return result;
  }

  bool Grow(size_t count) {
    if (status != WireStatus::kOk) {
      return false;
    }
    if (count > UINT32_MAX - bytes.size() - counted_size_) {
      status = WireStatus::kLengthOverflow;
      return false;
    }
    if (in_count_mode_) {
      counted_size_ += count;
      return true;
    }
    bytes.resize(bytes.size() + count);
    return true;
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
    Append(value ? kBoolTrue : kBoolFalse, kBoolEncodedSize);
  }

  // 浮点按 IEEE 754 位模式小端定宽传输：不经数值转换，NaN/±Inf/-0.0 位级保真
  void WriteValue(float value) {
    uint32_t bits = 0U;
    if (memcpy_s(&bits, sizeof(bits), &value, sizeof(value)) != EOK) {
      status = WireStatus::kInvalidParam;
      return;
    }
    Append(bits, sizeof(bits));
  }

  void WriteValue(double value) {
    uint64_t bits = 0U;
    if (memcpy_s(&bits, sizeof(bits), &value, sizeof(value)) != EOK) {
      status = WireStatus::kInvalidParam;
      return;
    }
    Append(bits, sizeof(bits));
  }

  void WriteValue(const std::string &value) {
    if (in_count_mode_) {
      // 计数态只累加长度，不触碰 bytes
      (void)Grow(value.size());
      return;
    }
    const size_t pos = bytes.size();
    if (Grow(value.size())) {
      std::copy(value.begin(), value.end(), bytes.begin() + pos);
    }
  }

  // 通用打包数组：有符号 zigzag varint、无符号裸 varint、bool/浮点定宽、string 带长度前缀；
  // 均无 count 前缀，帧 length 界定元素区（解码侧切分机制见 ReadValue 各重载）
  // vector<uint64_t> 被维度语义独占：元素按 int64 解释 zigzag（-1 表示未知维）；需要"裸 varint 的 u64 数组"
  // 必须新增值编码并走版本门禁，不得就地修改本重载语义
  void WriteValue(const std::vector<uint64_t> &value) {
    for (const auto dim : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(ZigZagEncode(static_cast<int64_t>(dim)));
    }
  }

  void WriteValue(const std::vector<int64_t> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(ZigZagEncode(item));
    }
  }

  void WriteValue(const std::vector<int32_t> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(ZigZagEncode(static_cast<int64_t>(item)));
    }
  }

  void WriteValue(const std::vector<uint32_t> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(static_cast<uint64_t>(item));
    }
  }

  void WriteValue(const std::vector<bool> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      Append(item ? kBoolTrue : kBoolFalse, kBoolEncodedSize);
    }
  }

  void WriteValue(const std::vector<float> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      uint32_t bits = 0U;
      if (memcpy_s(&bits, sizeof(bits), &item, sizeof(item)) != EOK) {
        status = WireStatus::kInvalidParam;
        return;
      }
      Append(bits, sizeof(bits));
    }
  }

  void WriteValue(const std::vector<double> &value) {
    for (const auto item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      uint64_t bits = 0U;
      if (memcpy_s(&bits, sizeof(bits), &item, sizeof(item)) != EOK) {
        status = WireStatus::kInvalidParam;
        return;
      }
      Append(bits, sizeof(bits));
    }
  }

  void WriteValue(const std::vector<std::string> &value) {
    for (const auto &item : value) {
      if (status != WireStatus::kOk) {
        break;
      }
      AppendVarint(item.size());
      const size_t pos = bytes.size();
      if (Grow(item.size())) {
        if (!in_count_mode_) {
          std::copy(item.begin(), item.end(), bytes.begin() + pos);
        }
      }
    }
  }

  // 计数模式状态：in_count_mode_ 期间的 Append/Grow 只累加 counted_size_，不写 bytes。
  bool in_count_mode_ = false;
  size_t counted_size_ = 0U;
};

template <typename Schema, typename Record>
WireStatus WireTlvDecodeRecord(const uint8_t *data, size_t size, Record &record);

template <typename Schema, typename Record>
WireStatus WireTlvDecodeFrame(uint16_t tag, const uint8_t *payload, size_t payload_size, Record &record);

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

  // 嵌套单值记录字段：tag 命中即就地解码进 field.value；解码成功置 present（帧出现 = Set 过）
  template <typename T>
  void VisitMember(uint16_t tag, WireTlvRecordField<T> &field) {
    if (tag == tag_) {
      matched = true;
      status = WireTlvDecodeRecord<Schema>(data_, size_, field.value);
      if (status == WireStatus::kOk) {
        field.present = true;
      }
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

  // map 容器解码：容器负载内按帧切分，见到 Entry::kWireMapKeyTag 即开启新条目，其余帧归入当前条目；
  // 首帧不是 key tag = 条目边界无法建立，判 kInvalidFormat。
  template <typename Entry>
  void VisitMember(uint16_t tag, WireTlvMap<Entry> &map) {
    if (tag != tag_) {
      return;
    }
    matched = true;
    bool has_map_key = false;
    status = ProcessRecords(data_, size_, [&](uint16_t inner_tag, const uint8_t *payload, size_t payload_size) {
      if (inner_tag == Entry::kWireMapKeyTag) {
        map.entries.emplace_back();
        has_map_key = true;
      }
      // 每个 map 容器必须有 key tag 否则为不合法
      if (!has_map_key) {
        return WireStatus::kInvalidFormat;
      }
      return WireTlvDecodeFrame<Schema>(inner_tag, payload, payload_size, map.entries.back());
    });
  }

 private:
  // 标量值走 varint:整条记录负载必须恰好被一个规范 varint 消耗完毕(消耗字节数 != size_ 即非法),
  // 再按目标类型做值域校验,避免定宽改可变宽后出现静默截断
  template <typename T>
  WireStatus ReadValue(T &value) const {
    uint64_t raw = 0U;
    size_t consumed = 0U;
    const auto readStatus = ReadVarint(data_, size_, raw, consumed);
    if ((readStatus != WireStatus::kOk) || (consumed != size_)) {
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
        if ((raw >> (sizeof(T) * kBitsPerByte)) != 0U) {
          return WireStatus::kInvalidFormat;
        }
      }
      value = static_cast<T>(raw);
    }
    return WireStatus::kOk;
  }

  WireStatus ReadValue(bool &value) const {
    if ((size_ != kBoolEncodedSize) || (data_[0] > kBoolTrue)) {
      return WireStatus::kInvalidFormat;
    }
    value = data_[0] != kBoolFalse;
    return WireStatus::kOk;
  }

  // 浮点定宽：负载宽度不等于类型宽度即非法（定宽编码不存在非规范形式，NaN 多态位模式全部合法且位级保真）
  WireStatus ReadValue(float &value) const {
    return ReadFloatValue(value);
  }

  WireStatus ReadValue(double &value) const {
    return ReadFloatValue(value);
  }

  // 浮点位模式读取：小端定宽字节 → 同宽无符号整数 → 位搬运进目标浮点（与编码路径的位模式互逆）
  template <typename T>
  WireStatus ReadFloatValue(T &value) const {
    if (size_ != sizeof(T)) {
      return WireStatus::kInvalidFormat;
    }
    return ReadFloatBits(data_, value);
  }

  // 位模式搬运基元：单值与定宽数组路径共用
  template <typename T>
  static WireStatus ReadFloatBits(const uint8_t *data, T &value) {
    using BitsType = std::conditional_t<sizeof(T) == sizeof(uint32_t), uint32_t, uint64_t>;
    const auto bits = static_cast<BitsType>(ReadLe(data, sizeof(T)));
    if (memcpy_s(&value, sizeof(value), &bits, sizeof(bits)) != EOK) {
      return WireStatus::kInvalidParam;
    }
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::string &value) const {
    value.assign(data_, data_ + size_);
    return WireStatus::kOk;
  }
  /* 通用打包数组解码 */
  // 存在维度数组应为 int 但是定义为 uint 的场景
  // 所以需要经 zigzag 还原 int64 后按位视图存入 u64。
  WireStatus ReadValue(std::vector<uint64_t> &value) const {
    std::vector<uint64_t> dims;
    size_t pos = 0U;
    while (pos < size_) {
      uint64_t raw = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, raw, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      dims.push_back(static_cast<uint64_t>(ZigZagDecode(raw)));
    }
    value.swap(dims);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<int64_t> &value) const {
    std::vector<int64_t> items;
    size_t pos = 0U;
    while (pos < size_) {
      uint64_t raw = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, raw, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      items.push_back(ZigZagDecode(raw));
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<int32_t> &value) const {
    std::vector<int32_t> items;
    size_t pos = 0U;
    while (pos < size_) {
      uint64_t raw = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, raw, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      const auto decoded = ZigZagDecode(raw);
      if ((decoded < static_cast<int64_t>(std::numeric_limits<int32_t>::min())) ||
          (decoded > static_cast<int64_t>(std::numeric_limits<int32_t>::max()))) {
        return WireStatus::kInvalidFormat;
      }
      items.push_back(static_cast<int32_t>(decoded));
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<uint32_t> &value) const {
    std::vector<uint32_t> items;
    size_t pos = 0U;
    while (pos < size_) {
      uint64_t raw = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, raw, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      if ((raw >> (sizeof(uint32_t) * kBitsPerByte)) != 0U) {
        return WireStatus::kInvalidFormat;
      }
      items.push_back(static_cast<uint32_t>(raw));
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<bool> &value) const {
    std::vector<bool> items;
    items.reserve(size_);  // 定宽 1B：负载字节数即元素个数
    for (size_t i = 0U; i < size_; ++i) {
      if (data_[i] > kBoolTrue) {
        return WireStatus::kInvalidFormat;
      }
      items.push_back(data_[i] != kBoolFalse);
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<float> &value) const {
    return ReadFloatArray(value);
  }

  WireStatus ReadValue(std::vector<double> &value) const {
    return ReadFloatArray(value);
  }

  // 定宽浮点数组：负载长度不整除元素宽度即判非法
  template <typename T>
  WireStatus ReadFloatArray(std::vector<T> &value) const {
    if ((size_ % sizeof(T)) != 0U) {
      return WireStatus::kInvalidFormat;
    }
    std::vector<T> items;
    items.reserve(size_ / sizeof(T));
    for (size_t pos = 0U; pos < size_; pos += sizeof(T)) {
      T item{};
      const auto status = ReadFloatBits(data_ + pos, item);
      if (status != WireStatus::kOk) {
        return status;
      }
      items.push_back(item);
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  WireStatus ReadValue(std::vector<std::string> &value) const {
    std::vector<std::string> items;
    size_t pos = 0U;
    while (pos < size_) {
      uint64_t length = 0U;
      size_t consumed = 0U;
      if (ReadVarint(data_ + pos, size_ - pos, length, consumed) != WireStatus::kOk) {
        return WireStatus::kInvalidFormat;
      }
      pos += consumed;
      // 元素声称长度越过帧负载即非法（长度前缀自带边界校验，无需 count 参与）
      if (length > static_cast<uint64_t>(size_ - pos)) {
        return WireStatus::kInvalidFormat;
      }
      items.emplace_back(reinterpret_cast<const char *>(data_ + pos), static_cast<size_t>(length));
      pos += static_cast<size_t>(length);
    }
    value.swap(items);
    return WireStatus::kOk;
  }

  uint16_t tag_;
  const uint8_t *data_;
  size_t size_;
};

// 注意：本函数解码 record 时集合元素是逐个 emplace_back 追加的，失败即返回，不具备原子性
// 需要调用方先解析到临时对象、成功后再 swap/move 提交（参考 DumpTransportInfo::Deserialize）。
template <typename Schema, typename Record>
WireStatus WireTlvDecodeRecord(const uint8_t *data, size_t size, Record &record) {
  // 记录级钩子存在时由 schema 层接管解码，否则走注册表逐帧匹配
  if constexpr (HasDecodePayloadMember<Record>::value) {
    return record.DecodePayload(data, size);
  } else {
    return ProcessRecords(data, size, [&record](uint16_t tag, const uint8_t *payload, size_t payload_size) {
      return WireTlvDecodeFrame<Schema>(tag, payload, payload_size, record);
    });
  }
}

// 将单个帧解码进 record：按 tag 匹配 RegisterMembers 表，未知 tag 直接跳过
template <typename Schema, typename Record>
WireStatus WireTlvDecodeFrame(uint16_t tag, const uint8_t *payload, size_t payload_size, Record &record) {
  WireTlvDecoder<Schema> decoder(tag, payload, payload_size);
  MemberRegHelper::Register(record, decoder);
  if (decoder.status != WireStatus::kOk) {
    return decoder.status;
  }
  if (!decoder.matched && Schema::IsKnownTag(tag)) {
    return WireStatus::kInvalidFormat;
  }
  return WireStatus::kOk;
}

// ---- 文档级（公共头）helper：magic/version 来自 Schema，record_count 与顶层 tag 分派由业务侧决定 ----

// 写 16 字节公共头；total_size 先占位 0，全部记录写完后由 FillWireTlvTotalSize 回填。
// record_count 只占 4 字节，故形参即为 uint32_t（超出请在调用侧先判并返回 kLengthOverflow，避免静默截断）
template <typename Schema>
void WriteCommonHeader(WireTlvEncoder &encoder, uint32_t record_count) {
  encoder.Append(Schema::kMagic, kMagicSize);
  encoder.Append(Schema::kVersion, kVersionSize);
  encoder.Append(kHeaderSize, kHeaderSizeFieldSize);
  encoder.Append(0U, kTotalSizeFieldSize);
  encoder.Append(record_count, kRecordCountFieldSize);
}

inline void FillWireTlvTotalSize(std::vector<uint8_t> &bytes) {
  if (bytes.size() < kHeaderSize) {
    // 公共头尚未写全（编码已失败）时不回填，避免越界写
    return;
  }
  WriteLe(bytes.data() + kTotalSizeOffset, bytes.size(), kTotalSizeFieldSize);
}

// 前置条件：data 已通过 CheckWireTlvHeader（非空且 size >= kHeaderSize）；不满足时返回 0，
// 调用方按"记录数不符"判 kInvalidFormat
inline uint64_t GetRecordCountFromHeader(const uint8_t *data, size_t size) {
  if ((data == nullptr) || (size < kHeaderSize)) {
    return 0U;
  }
  return ReadLe(data + kRecordCountOffset, kRecordCountFieldSize);
}

// 公共头四道门：非空、(size>=16 && magic)、version、(header_size && total_size==size)
template <typename Schema>
WireStatus CheckWireTlvHeader(const uint8_t *data, size_t size) {
  if (data == nullptr) {
    return WireStatus::kInvalidParam;
  }
  if ((size < kHeaderSize) || (ReadLe(data + kMagicOffset, kMagicSize) != Schema::kMagic)) {
    return WireStatus::kInvalidFormat;
  }
  if (ReadLe(data + kVersionOffset, kVersionSize) != Schema::kVersion) {
    return WireStatus::kUnsupportedVersion;
  }
  if ((ReadLe(data + kHeaderSizeOffset, kHeaderSizeFieldSize) != kHeaderSize) ||
      (ReadLe(data + kTotalSizeOffset, kTotalSizeFieldSize) != size)) {
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
