/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "formats/format_transfers/format_transfer_fractal_nz.h"

#include <securec.h>
#include <memory>
#include <vector>

#include "graph_metadef/common/ge_common/util.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "formats/utils/formats_definitions.h"
#include "formats/utils/formats_trans_utils.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/framework_types_internal.h"
#include "graph/utils/type_utils.h"
#include "base/err_msg.h"
#include "common/checker.h"

namespace ge {
namespace formats {
namespace {
const size_t kDimDValueBNdFNz = 2U;  // dim d-value between Nd and FractalZz

constexpr int64_t kBitsPerByte = 8;                     // 每字节数
constexpr int64_t kByteAlignAdjust = kBitsPerByte - 1;  // 向上取整到字节的修正量
constexpr int64_t kFractalNzC02 = 2;                    // FORMAT_FRACTAL_NZ_C0_2 的块内轴
constexpr int64_t kFractalNzC04 = 4;                    // FORMAT_FRACTAL_NZ_C0_4 的块内轴
constexpr int64_t kFractalNzC08 = 8;                    // FORMAT_FRACTAL_NZ_C0_8 的块内轴
constexpr int64_t kFractalNzC016 = 16;                  // FORMAT_FRACTAL_NZ_C0_16 的块内轴
constexpr int64_t kFractalNzC032 = 32;                  // FORMAT_FRACTAL_NZ_C0_32 的块内轴

const size_t kFNzDimCountBackwardsW0 = 1U;
const size_t kFNzDimCountBackwardsW0H0 = 2U;
const size_t kFNzDimCountBackwardsW0H0H1 = 3U;
const size_t kFNzDimCountBackwardsW0H0H1W1 = 4U;

bool IsDataTypeSupportForTransShapeToFracNz(const DataType data_type) {
  return GetSizeByDataType(data_type) > 0;
}

// Same rule as formats.cc: bit-width encoded in GetSizeByDataType for sub-byte types (e.g. DT_INT4).
bool IsSubByteDataType(const DataType data_type) {
  return GetSizeByDataType(data_type) > kDataTypeSizeBitOffset;
}

// Resolve the effective C0 of a fractal-NZ format: prefer the composed C0 bit-field,
// fall back to the primary enum variants (e.g. FORMAT_FRACTAL_NZ_C0_32).
int64_t GetEffectiveC0(const Format format) {
  const int64_t c0 = GetC0Value(static_cast<int32_t>(format));
  if (c0 > 0) {
    return c0;
  }
  switch (static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(format)))) {
    case FORMAT_FRACTAL_NZ_C0_2:
      return kFractalNzC02;
    case FORMAT_FRACTAL_NZ_C0_4:
      return kFractalNzC04;
    case FORMAT_FRACTAL_NZ_C0_8:
      return kFractalNzC08;
    case FORMAT_FRACTAL_NZ_C0_16:
      return kFractalNzC016;
    case FORMAT_FRACTAL_NZ_C0_32:
      return kFractalNzC032;
    default:
      return -1;
  }
}

using ShapeVector = std::vector<int64_t>;
bool CheckShapeForTransShapeToFracNz(const Format format, const ShapeVector &shape) {
  const int32_t num_dims_4d = 4;
  bool ret = false;
  switch (format) {
    case FORMAT_ND:
      ret = IsShapeValid(shape);
      break;
    case FORMAT_NCHW:
    case FORMAT_NHWC:
      ret = CheckShapeValid(shape, num_dims_4d);
      break;
    default:
      const std::string error = "Trans format between " + FmtToStr(TypeUtils::FormatToSerialString(format)) +
                                " and FORMAT_FRACTAL_NZ is not supported.";
      GE_ERRORLOG_AND_ERRORMSG(ACL_ERROR_GE_FORMAT_INVALID, error.c_str());
      break;
  }
  return ret;
}

/**
 * After the conversion to two-dimensional matrix, the memory arrangement is small z and large N.
 * @src_shape: N*H*W
 * @dst_shape: N*W1*H1*H0*w0
 * @return
 */
Status TransShapeToFracNz(const ShapeVector &src_shape, const int64_t &w0, ShapeVector &dst_shape,
                          ShapeVector &hw_shape) {
  dst_shape.clear();
  hw_shape.clear();
  const int64_t h0 = kCubeSize;
  const size_t num_dims_single = 1U;
  if (src_shape.size() == num_dims_single) {
    dst_shape.push_back(Ceil(src_shape[static_cast<size_t>(kNhwcN)], w0));
    dst_shape.push_back(DIM_DEFAULT_VALUE);
    dst_shape.push_back(h0);
    dst_shape.push_back(w0);
    hw_shape.push_back(DIM_DEFAULT_VALUE);
    hw_shape.push_back(DIM_DEFAULT_VALUE);
    hw_shape.push_back(src_shape[static_cast<size_t>(kNhwcN)]);
    if (!IsShapeValid(dst_shape)) {
      GELOGE(ACL_ERROR_GE_SHAPE_INVALID, "[Check][DSTShape]Failed, dst shape %s", ShapeToString(dst_shape).c_str());
      REPORT_INNER_ERR_MSG("E19999", "Failed to check dst shape %s", ShapeToString(dst_shape).c_str());
      return ACL_ERROR_GE_SHAPE_INVALID;
    }
  } else {
    const auto size = src_shape.size();
    int64_t times = 1;
    for (size_t i = 0U; i != (size - kDimDValueBNdFNz); i++) {
      dst_shape.push_back(src_shape[i]);
      times *= src_shape[i];
    }
    const size_t num_dims_backwards_w = 1U;
    const size_t num_dims_backwards_wh = 2U;
    dst_shape.push_back(Ceil(src_shape[size - num_dims_backwards_w], w0));
    dst_shape.push_back(Ceil(src_shape[size - num_dims_backwards_wh], h0));
    dst_shape.push_back(h0);
    dst_shape.push_back(w0);
    hw_shape.push_back(times);
    hw_shape.push_back(src_shape[size - num_dims_backwards_wh]);
    hw_shape.push_back(src_shape[size - num_dims_backwards_w]);
    if (!IsShapeValid(dst_shape)) {
      GELOGE(ACL_ERROR_GE_SHAPE_INVALID, "[Check][DSTShape]Failed, dst shape %s", ShapeToString(dst_shape).c_str());
      REPORT_INNER_ERR_MSG("E19999", "Failed to check dst shape %s", ShapeToString(dst_shape).c_str());
      return ACL_ERROR_GE_SHAPE_INVALID;
    }
  }
  return SUCCESS;
}

Status CheckShapeRelationForTransShapeToFracNz(const TransArgs &args, ShapeVector &hw_shape) {
  ShapeVector expect_src_shape;
  const auto ret = TransShapeToFracNz(args.dst_shape, args.src_c0_format, expect_src_shape, hw_shape);
  if (ret != SUCCESS) {
    GELOGE(ret,
           "[Transfer][ShapeToFracNz]Failed, shape from %s to %s, shape %s to %s, "
           "data type %s, error_code:%u",
           TypeUtils::FormatToSerialString(args.dst_format).c_str(),
           TypeUtils::FormatToSerialString(args.src_format).c_str(), ShapeToString(args.dst_shape).c_str(),
           ShapeToString(args.src_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str(), ret);
    return ret;
  }
  if (!IsTransShapeSrcCorrect(args, expect_src_shape)) {
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  return SUCCESS;
}

Status TransFormatFromNdToFracNz(const TransArgs &args, TransResult &result, const ShapeVector &hw_shape) {
  const int32_t size = GetSizeByDataType(args.src_data_type);
  const int64_t dst_size = GetItemNumByShape(args.dst_shape) * size;
  if (dst_size == 0) {
    result.length = static_cast<size_t>(dst_size);
    return SUCCESS;
  }

  const std::shared_ptr<uint8_t> dst(new (std::nothrow) uint8_t[dst_size](), std::default_delete<uint8_t[]>());
  if (dst == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION,
           "[Allocate][DSTMemory]Failed to allocate memory "
           "for dst buf %" PRId64 " when trans format from %s to %s",
           dst_size, TypeUtils::FormatToSerialString(args.src_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_format).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Failed to allocate memory for dst buf %" PRId64
                         " "
                         "trans format from %s to %s",
                         dst_size, TypeUtils::FormatToSerialString(args.src_format).c_str(),
                         TypeUtils::FormatToSerialString(args.dst_format).c_str());
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  // src&dst_shape can be written as times*H*W & times*W1*H1*H0*W0, respectively. dst_shape_size >= kDimNum4D
  const auto times = hw_shape.at(static_cast<size_t>(kNhwcN));
  const auto h = hw_shape.at(static_cast<size_t>(kNhwcH));
  const auto w = hw_shape.at(static_cast<size_t>(kNhwcW));
  const auto hw = h * w;

  const auto shape_size = args.dst_shape.size();
  const auto w1 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0H1W1];
  const auto h1 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0H1];
  const auto h0 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0];
  const auto w0 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto h1h0 = h1 * h0;
  const auto h1h0w0 = h1h0 * w0;
  const auto w1h1h0w0 = w1 * h1h0w0;
  const auto num_w1 = w / w0;

  for (int64_t times_idx = 0; times_idx < times; times_idx++) {
    const auto times_head = times_idx * w1h1h0w0;
    const auto src_times_head = times_idx * hw;
    for (int64_t h1h0_idx = 0; h1h0_idx < h; h1h0_idx++) {
      const auto h1h0_head = times_head + (h1h0_idx * w0);
      const auto src_h_head = src_times_head + (h1h0_idx * w);
      for (int64_t w1_idx = 0; w1_idx < num_w1; w1_idx++) {
        const auto dst_offset = (h1h0_head + (w1_idx * h1h0w0)) * size;
        const auto src_offset = (src_h_head + (w1_idx * w0)) * size;
        const auto protected_size = ((dst_size - dst_offset) < static_cast<int64_t>(SECUREC_MEM_MAX_LEN))
                                        ? (dst_size - dst_offset)
                                        : static_cast<int64_t>(SECUREC_MEM_MAX_LEN);
        GE_CHECK_GE(protected_size, 0);
        const auto ret = memcpy_s(dst.get() + dst_offset, static_cast<size_t>(protected_size), args.data + src_offset,
                                  static_cast<size_t>(size) * static_cast<size_t>(w0));
        if (ret != EOK) {
          GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED,
                 "[Operate][DSTMemory]Failed at offset %" PRId64
                 ", "
                 "error-code %d",
                 dst_offset, ret);
          REPORT_INNER_ERR_MSG("E19999", "Failed to operate dst memory at offset %" PRId64 ", error-code %d",
                               dst_offset, ret);
          return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
        }
      }
      const auto w1_head = num_w1 * w0;
      for (int64_t w0_idx = 0; (w1_head + w0_idx) < w; w0_idx++) {
        const auto src_w_idx = w1_head + w0_idx;
        const auto dst_offset = (h1h0_head + (num_w1 * h1h0w0) + w0_idx) * size;
        const auto src_offset = (src_h_head + src_w_idx) * size;
        const auto protected_size = ((dst_size - dst_offset) < static_cast<int64_t>(SECUREC_MEM_MAX_LEN))
                                        ? (dst_size - dst_offset)
                                        : static_cast<int64_t>(SECUREC_MEM_MAX_LEN);
        GE_CHECK_GE(protected_size, 0);
        const auto ret = memcpy_s(dst.get() + dst_offset, static_cast<size_t>(protected_size), args.data + src_offset,
                                  static_cast<size_t>(size));
        if (ret != EOK) {
          GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED,
                 "[Operate][DSTMemory]Failed at offset %" PRId64
                 ", "
                 "error-code %d",
                 dst_offset, ret);
          REPORT_INNER_ERR_MSG("E19999", "Failed to operate dst memory at offset %" PRId64 ", error-code %d",
                               dst_offset, ret);
          return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
        }
      }
    }
  }
  result.data = dst;
  result.length = static_cast<size_t>(dst_size);
  return SUCCESS;
}

Status TransFormatFromFracNzToNd(const TransArgs &args, TransResult &result, const ShapeVector &dst_hw_shape) {
  const int32_t size = GetSizeByDataType(args.src_data_type);
  const int64_t dst_size = GetItemNumByShape(args.dst_shape) * size;
  if (dst_size == 0) {
    result.length = static_cast<size_t>(dst_size);
    return SUCCESS;
  }

  const std::shared_ptr<uint8_t> dst(new (std::nothrow) uint8_t[dst_size], std::default_delete<uint8_t[]>());
  if (dst == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION,
           "[Allocate][DSTMemory]Failed to trans format "
           "from %s to %s, memory for dst buf %" PRId64 "",
           TypeUtils::FormatToSerialString(args.src_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_format).c_str(), dst_size);
    REPORT_INNER_ERR_MSG("E19999",
                         "Failed to trans format from %s to %s and allocate memory "
                         "for dst buf %" PRId64 "",
                         TypeUtils::FormatToSerialString(args.src_format).c_str(),
                         TypeUtils::FormatToSerialString(args.dst_format).c_str(), dst_size);
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  const auto times = dst_hw_shape.at(static_cast<size_t>(kNhwcN));
  const auto h = dst_hw_shape.at(static_cast<size_t>(kNhwcH));
  const auto w = dst_hw_shape.at(static_cast<size_t>(kNhwcW));
  const auto hw = h * w;

  const auto shape_size = args.src_shape.size();
  const auto w1 = args.src_shape[shape_size - kFNzDimCountBackwardsW0H0H1W1];
  const auto h1 = args.src_shape[shape_size - kFNzDimCountBackwardsW0H0H1];
  const auto h0 = args.src_shape[shape_size - kFNzDimCountBackwardsW0H0];
  const auto w0 = args.src_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto h1h0 = h1 * h0;
  const auto h1h0w0 = h1h0 * w0;
  const auto w1h1h0w0 = w1 * h1h0w0;
  const auto num_w1 = w / w0;

  for (int64_t times_idx = 0; times_idx < times; times_idx++) {
    const auto times_head = times_idx * w1h1h0w0;
    const auto dst_times_head = times_idx * hw;
    for (int64_t h1h0_idx = 0; h1h0_idx < h; h1h0_idx++) {
      const auto h1h0_head = times_head + (h1h0_idx * w0);
      const auto dst_h_head = dst_times_head + (h1h0_idx * w);
      for (int64_t w1_idx = 0; w1_idx < num_w1; w1_idx++) {
        const auto src_offset = (h1h0_head + (w1_idx * h1h0w0)) * size;
        const auto dst_offset = (dst_h_head + (w1_idx * w0)) * size;
        const auto protected_size = ((dst_size - dst_offset) < static_cast<int64_t>(SECUREC_MEM_MAX_LEN))
                                        ? (dst_size - dst_offset)
                                        : static_cast<int64_t>(SECUREC_MEM_MAX_LEN);
        GE_CHECK_GE(protected_size, 0);
        const auto ret = memcpy_s(dst.get() + dst_offset, static_cast<size_t>(protected_size), args.data + src_offset,
                                  (static_cast<size_t>(size) * static_cast<size_t>(w0)));
        if (ret != EOK) {
          GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED,
                 "[Operate][DSTMemory]Failed at offset %" PRId64
                 ", "
                 "error-code %d",
                 dst_offset, ret);
          REPORT_INNER_ERR_MSG("E19999", "Failed to operate dst memory at offset %" PRId64 ", error-code %d",
                               dst_offset, ret);
          return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
        }
      }
      const auto w1_head = num_w1 * w0;
      for (int64_t w0_idx = 0; (w1_head + w0_idx) < w; w0_idx++) {
        const auto dst_w_idx = w1_head + w0_idx;
        const auto src_offset = (h1h0_head + (num_w1 * h1h0w0) + w0_idx) * size;
        const auto dst_offset = (dst_h_head + dst_w_idx) * size;
        const auto protected_size = ((dst_size - dst_offset) < static_cast<int64_t>(SECUREC_MEM_MAX_LEN))
                                        ? (dst_size - dst_offset)
                                        : static_cast<int64_t>(SECUREC_MEM_MAX_LEN);
        GE_CHECK_GE(protected_size, 0);
        const auto ret = memcpy_s(dst.get() + dst_offset, static_cast<size_t>(protected_size), args.data + src_offset,
                                  static_cast<size_t>(size));
        if (ret != EOK) {
          GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED,
                 "[Operate][DSTMemory]Failed at offset %" PRId64
                 ", "
                 "error-code %d",
                 dst_offset, ret);
          REPORT_INNER_ERR_MSG("E19999",
                               "Failed to operate dst memory at offset %" PRId64
                               ", "
                               "error-code %d",
                               dst_offset, ret);
          return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
        }
      }
    }
  }
  result.data = dst;
  result.length = static_cast<size_t>(dst_size);
  return SUCCESS;
}

// ----------------------- Sub-byte (packed) implementations, e.g. DT_INT4 -----------------------
// Sub-byte types encode bit-width in GetSizeByDataType (kDataTypeSizeBitOffset + bits).
// Data is packed with `elem_per_byte` elements per byte, lower index in lower bits.
// The fractal block row (C0 elements) must be byte-aligned: (c0 * bits) % 8 == 0.

int64_t GetSubByteBitWidth(const DataType data_type) {
  uint32_t length = 0U;
  if (!TypeUtils::GetDataTypeLength(data_type, length) || (length <= kDataTypeSizeBitOffset)) {
    return -1;
  }
  return length - kDataTypeSizeBitOffset;
}

uint8_t GetBitsValue(const uint8_t *const data, const int64_t elem_idx, const int64_t bits) {
  const int64_t elem_per_byte = 8 / bits;
  const uint8_t byte = data[elem_idx / elem_per_byte];
  const uint8_t shift = static_cast<uint8_t>((elem_idx % elem_per_byte) * bits);
  return static_cast<uint8_t>((byte >> shift) & static_cast<uint8_t>((1U << bits) - 1U));
}

void SetBitsValue(uint8_t *const data, const int64_t elem_idx, const int64_t bits, const uint8_t value) {
  const int64_t elem_per_byte = 8 / bits;
  uint8_t &byte = data[elem_idx / elem_per_byte];
  const uint8_t shift = static_cast<uint8_t>((elem_idx % elem_per_byte) * bits);
  const uint8_t value_mask = static_cast<uint8_t>((1U << bits) - 1U);
  const uint8_t mask = static_cast<uint8_t>(value_mask << shift);
  const uint8_t safe_value = static_cast<uint8_t>(value & value_mask);
  byte = static_cast<uint8_t>((byte & static_cast<uint8_t>(~mask)) | static_cast<uint8_t>(safe_value << shift));
}

// Copy `count` packed elements from src element offset to dst element offset when byte-aligned,
// otherwise fall back to element-wise bit copy. `dst_size_bytes` bounds the destination buffer.
Status CopyPackedSegment(const uint8_t *const src, const int64_t src_elem, uint8_t *const dst, const int64_t dst_elem,
                         const int64_t count, const int64_t bits, const int64_t dst_size_bytes) {
  if ((count <= 0) || (dst_size_bytes <= 0) ||
      ((((dst_elem + count) * bits) + kByteAlignAdjust) / kBitsPerByte > dst_size_bytes)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Check][DSTMemory]Failed, dst elem %" PRId64 ", count %" PRId64 ", bits %" PRId64
           " exceed dst size %" PRId64,
           dst_elem, count, bits, dst_size_bytes);
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  if ((((src_elem * bits) % kBitsPerByte) != 0) || (((dst_elem * bits) % kBitsPerByte) != 0)) {
    for (int64_t k = 0; k < count; k++) {
      SetBitsValue(dst, dst_elem + k, bits, GetBitsValue(src, src_elem + k, bits));
    }
    return SUCCESS;
  }
  const int64_t byte_len = (count * bits) / kBitsPerByte;
  const int64_t dst_byte = (dst_elem * bits) / kBitsPerByte;
  const auto protected_size = ((dst_size_bytes - dst_byte) < static_cast<int64_t>(SECUREC_MEM_MAX_LEN))
                                  ? (dst_size_bytes - dst_byte)
                                  : static_cast<int64_t>(SECUREC_MEM_MAX_LEN);
  GE_CHECK_GE(protected_size, byte_len);
  const auto ret = memcpy_s(dst + dst_byte, static_cast<size_t>(protected_size), src + (src_elem * bits) / kBitsPerByte,
                            static_cast<size_t>(byte_len));
  if (ret != EOK) {
    GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Operate][DSTMemory]Failed at elem %" PRId64 ", error-code %d",
           dst_elem, ret);
    REPORT_INNER_ERR_MSG("E19999", "Failed to operate dst memory at elem %" PRId64 ", error-code %d", dst_elem, ret);
    return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
  }
  return SUCCESS;
}

// 子字节分支因 M0 平台差异跳过通用形状比对，此处校验数据搬移实际依赖的不变量：
// NZ 形状非负且末 4 维存在、块内轴 w0 为字节对齐的正数、前缀乘积与 hw_shape 一致、
// 分形块容量足以覆盖按 hw_shape 寻址的读写范围，防止除零与越界访问。
Status CheckShapeForSubByteTrans(const ShapeVector &nz_shape, const ShapeVector &hw_shape, const int64_t bits) {
  if ((!IsShapeValid(nz_shape)) || (nz_shape.size() < kFNzDimCountBackwardsW0H0H1W1)) {
    GELOGE(ACL_ERROR_GE_SHAPE_INVALID, "[Check][Shape]Failed, invalid sub-byte fractal nz shape %s",
           ShapeToString(nz_shape).c_str());
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  const auto shape_size = nz_shape.size();
  const auto w1 = nz_shape[shape_size - kFNzDimCountBackwardsW0H0H1W1];
  const auto h1h0w0 = nz_shape[shape_size - kFNzDimCountBackwardsW0H0H1] *
                      nz_shape[shape_size - kFNzDimCountBackwardsW0H0] * nz_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto w0 = nz_shape[shape_size - kFNzDimCountBackwardsW0];
  if ((w0 <= 0) || (((w0 * bits) % kBitsPerByte) != 0)) {
    GELOGE(ACL_ERROR_GE_DATATYPE_INVALID,
           "[Check][Datatype]Failed, sub-byte block inner axis w0 %" PRId64 " with bits %" PRId64
           " is invalid or not byte-aligned",
           w0, bits);
    return ACL_ERROR_GE_DATATYPE_INVALID;
  }
  int64_t times_nz = 1;
  for (size_t i = 0U; i < (shape_size - kFNzDimCountBackwardsW0H0H1W1); i++) {
    times_nz *= nz_shape[i];
  }
  const auto h = hw_shape.at(static_cast<size_t>(kNhwcH));
  const auto w = hw_shape.at(static_cast<size_t>(kNhwcW));
  const auto num_w1 = w / w0;
  const auto block_size = w1 * h1h0w0;
  // 按实际寻址计算最后一个被访问的分形块号与块内元素数：
  // w % w0 == 0 时无尾块，最后一块为 num_w1 - 1、元素数 w0；否则最后一块为 num_w1、元素数 w % w0
  const int64_t tail = w % w0;
  const int64_t last_block = (tail == 0) ? (num_w1 - 1) : num_w1;
  const int64_t last_count = (tail == 0) ? w0 : tail;
  if ((times_nz != hw_shape.at(static_cast<size_t>(kNhwcN))) || (block_size <= 0) ||
      ((((h - 1) * w0) + (last_block * h1h0w0) + last_count) > block_size)) {
    GELOGE(ACL_ERROR_GE_SHAPE_INVALID, "[Check][Shape]Failed, sub-byte fractal nz shape %s does not match hw shape %s",
           ShapeToString(nz_shape).c_str(), ShapeToString(hw_shape).c_str());
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  return SUCCESS;
}

// 计算并分配子字节转换的目标缓冲：按位宽换算 dst 大小、校验并将缓冲写入 result
Status CreateSubByteDstBuffer(const TransArgs &args, TransResult &result, int64_t &bits) {
  bits = GetSubByteBitWidth(args.src_data_type);
  GE_CHECK_GE(bits, 1);
  const int64_t dst_size = ge::GetSizeInBytes(GetItemNumByShape(args.dst_shape), args.src_data_type);
  if (dst_size <= 0) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][DSTSize]Failed, dst size %" PRId64, dst_size);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  const auto dst_buffer = ge::ComGraphMakeShared<std::vector<uint8_t>>(static_cast<size_t>(dst_size), 0U);
  GE_ASSERT_NOTNULL(dst_buffer);
  result.data = std::shared_ptr<uint8_t>(dst_buffer, dst_buffer->data());
  result.length = static_cast<size_t>(dst_size);
  return SUCCESS;
}

Status TransFormatFromNdToFracNzSubByte(const TransArgs &args, TransResult &result, const ShapeVector &hw_shape) {
  int64_t bits = 0;
  const auto status = CreateSubByteDstBuffer(args, result, bits);
  if (status != SUCCESS) {
    return status;
  }
  const auto dst_size = static_cast<int64_t>(result.length);
  uint8_t *dst = result.data.get();

  const auto times = hw_shape.at(static_cast<size_t>(kNhwcN));
  const auto h = hw_shape.at(static_cast<size_t>(kNhwcH));
  const auto w = hw_shape.at(static_cast<size_t>(kNhwcW));

  const auto shape_size = args.dst_shape.size();
  const auto w1 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0H1W1];
  const auto h1h0w0 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0H1] *
                      args.dst_shape[shape_size - kFNzDimCountBackwardsW0H0] *
                      args.dst_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto w0 = args.dst_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto w1h1h0w0 = w1 * h1h0w0;
  const auto num_w1 = w / w0;
  const auto hw = h * w;

  for (int64_t times_idx = 0; times_idx < times; times_idx++) {
    const auto dst_times_head = times_idx * w1h1h0w0;
    const auto src_times_head = times_idx * hw;
    for (int64_t h1h0_idx = 0; h1h0_idx < h; h1h0_idx++) {
      const auto dst_row_head = dst_times_head + (h1h0_idx * w0);
      const auto src_row_head = src_times_head + (h1h0_idx * w);
      for (int64_t w1_idx = 0; w1_idx < num_w1; w1_idx++) {
        const auto ret = CopyPackedSegment(args.data, src_row_head + (w1_idx * w0), dst,
                                           dst_row_head + (w1_idx * h1h0w0), w0, bits, dst_size);
        if (ret != SUCCESS) {
          return ret;
        }
      }
      const auto w1_head = num_w1 * w0;
      for (int64_t w0_idx = 0; (w1_head + w0_idx) < w; w0_idx++) {
        const int64_t dst_elem = dst_row_head + (num_w1 * h1h0w0) + w0_idx;
        if ((((dst_elem + 1) * bits) + kByteAlignAdjust) / kBitsPerByte > dst_size) {
          GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][DSTMemory]Failed, tail elem %" PRId64 " exceed dst size %" PRId64,
                 dst_elem, dst_size);
          return ACL_ERROR_GE_PARAM_INVALID;
        }
        SetBitsValue(dst, dst_elem, bits, GetBitsValue(args.data, src_row_head + w1_head + w0_idx, bits));
      }
    }
  }
  return SUCCESS;
}

Status TransFormatFromFracNzToNdSubByte(const TransArgs &args, TransResult &result, const ShapeVector &dst_hw_shape) {
  int64_t bits = 0;
  const auto status = CreateSubByteDstBuffer(args, result, bits);
  if (status != SUCCESS) {
    return status;
  }
  const auto dst_size = static_cast<int64_t>(result.length);
  uint8_t *dst = result.data.get();

  const auto times = dst_hw_shape.at(static_cast<size_t>(kNhwcN));
  const auto h = dst_hw_shape.at(static_cast<size_t>(kNhwcH));
  const auto w = dst_hw_shape.at(static_cast<size_t>(kNhwcW));

  const auto shape_size = args.src_shape.size();
  const auto w1 = args.src_shape[shape_size - kFNzDimCountBackwardsW0H0H1W1];
  const auto h1h0w0 = args.src_shape[shape_size - kFNzDimCountBackwardsW0H0H1] *
                      args.src_shape[shape_size - kFNzDimCountBackwardsW0H0] *
                      args.src_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto w0 = args.src_shape[shape_size - kFNzDimCountBackwardsW0];
  const auto w1h1h0w0 = w1 * h1h0w0;
  const auto num_w1 = w / w0;
  const auto hw = h * w;

  for (int64_t times_idx = 0; times_idx < times; times_idx++) {
    const auto src_times_head = times_idx * w1h1h0w0;
    const auto dst_times_head = times_idx * hw;
    for (int64_t h1h0_idx = 0; h1h0_idx < h; h1h0_idx++) {
      const auto src_row_head = src_times_head + (h1h0_idx * w0);
      const auto dst_row_head = dst_times_head + (h1h0_idx * w);
      for (int64_t w1_idx = 0; w1_idx < num_w1; w1_idx++) {
        const auto ret = CopyPackedSegment(args.data, src_row_head + (w1_idx * h1h0w0), dst,
                                           dst_row_head + (w1_idx * w0), w0, bits, dst_size);
        if (ret != SUCCESS) {
          return ret;
        }
      }
      const auto w1_head = num_w1 * w0;
      for (int64_t w0_idx = 0; (w1_head + w0_idx) < w; w0_idx++) {
        const int64_t dst_elem = dst_row_head + w1_head + w0_idx;
        if ((((dst_elem + 1) * bits) + kByteAlignAdjust) / kBitsPerByte > dst_size) {
          GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][DSTMemory]Failed, tail elem %" PRId64 " exceed dst size %" PRId64,
                 dst_elem, dst_size);
          return ACL_ERROR_GE_PARAM_INVALID;
        }
        SetBitsValue(dst, dst_elem, bits, GetBitsValue(args.data, src_row_head + (num_w1 * h1h0w0) + w0_idx, bits));
      }
    }
  }
  return SUCCESS;
}

// 校验子字节位宽受支持（仅 1/2/4 位）且与块内轴 c0 字节对齐
Status CheckSubByteBitsAndAlign(const DataType data_type, const int64_t c0, int64_t &bits) {
  bits = GetSubByteBitWidth(data_type);
  if ((bits <= 0) || ((kBitsPerByte % bits) != 0)) {
    GELOGE(ACL_ERROR_GE_DATATYPE_INVALID,
           "[Check][Datatype]Failed, sub-byte data type %s bit width %" PRId64 " is not supported (only 1/2/4 bits)",
           TypeUtils::DataTypeToSerialString(data_type).c_str(), bits);
    return ACL_ERROR_GE_DATATYPE_INVALID;
  }
  if ((c0 * bits) % kBitsPerByte != 0) {
    GELOGE(ACL_ERROR_GE_DATATYPE_INVALID,
           "[Check][Datatype]Failed, sub-byte data type %s with c0 %" PRId64 " is not byte-aligned",
           TypeUtils::DataTypeToSerialString(data_type).c_str(), c0);
    return ACL_ERROR_GE_DATATYPE_INVALID;
  }
  return SUCCESS;
}

// 校验 dtype 受格式转换支持
Status CheckDataTypeSupportForTrans(const TransArgs &args) {
  if (!IsDataTypeSupportForTransShapeToFracNz(args.src_data_type)) {
    GELOGE(ACL_ERROR_GE_DATATYPE_INVALID,
           "[Check][Datatype]Failed, trans format from %s to %s, src shape %s, dst shape %s, "
           "data type %s is not supported",
           TypeUtils::FormatToSerialString(args.src_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_format).c_str(), ShapeToString(args.src_shape).c_str(),
           ShapeToString(args.dst_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Check datatype failed, trans format from %s to %s, src shape %s, "
                         "dst shape %s, data type %s is not supported",
                         TypeUtils::FormatToSerialString(args.src_format).c_str(),
                         TypeUtils::FormatToSerialString(args.dst_format).c_str(),
                         ShapeToString(args.src_shape).c_str(), ShapeToString(args.dst_shape).c_str(),
                         TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    return ACL_ERROR_GE_DATATYPE_INVALID;
  }
  return SUCCESS;
}

// ND -> NZ 子字节入口：位宽支持性/块内轴字节对齐校验、形状配套校验后进入 packed 搬移
Status TransSubByteNdToFracNz(const TransArgs &args, TransResult &result, const int64_t c0,
                              const ShapeVector &hw_shape) {
  int64_t bits = 0;
  const auto bits_ret = CheckSubByteBitsAndAlign(args.src_data_type, c0, bits);
  if (bits_ret != SUCCESS) {
    return bits_ret;
  }
  const auto shape_check_ret = CheckShapeForSubByteTrans(args.dst_shape, hw_shape, bits);
  if (shape_check_ret != SUCCESS) {
    return shape_check_ret;
  }
  return TransFormatFromNdToFracNzSubByte(args, result, hw_shape);
}

// NZ -> ND 子字节入口：位宽支持性/块内轴字节对齐校验、形状推导与配套校验后进入 packed 搬移
Status TransSubByteFracNzToNd(const TransArgs &args, TransResult &result, const int64_t c0, ShapeVector &hw_shape) {
  int64_t bits = 0;
  const auto bits_ret = CheckSubByteBitsAndAlign(args.src_data_type, c0, bits);
  if (bits_ret != SUCCESS) {
    return bits_ret;
  }
  ShapeVector expect_src_shape;
  const auto shape_ret = TransShapeToFracNz(args.dst_shape, c0, expect_src_shape, hw_shape);
  if (shape_ret != SUCCESS) {
    return shape_ret;
  }
  const auto shape_check_ret = CheckShapeForSubByteTrans(args.src_shape, hw_shape, bits);
  if (shape_check_ret != SUCCESS) {
    return shape_check_ret;
  }
  return TransFormatFromFracNzToNdSubByte(args, result, hw_shape);
}
}  // namespace

// ND -> NZ 入口参数校验：dtype 支持性与 src/dst shape 合法性
Status CheckArgsForNdToFracNz(const TransArgs &args) {
  const auto dtype_ret = CheckDataTypeSupportForTrans(args);
  if (dtype_ret != SUCCESS) {
    return dtype_ret;
  }
  if ((!CheckShapeForTransShapeToFracNz(args.src_primary_format, args.src_shape)) || (!IsShapeValid(args.dst_shape))) {
    GELOGE(ACL_ERROR_GE_SHAPE_INVALID,
           "[Check][Shape]Failed, trans format from %s to %s, "
           "src shape %s, dst shape %s, data type %s is not supported",
           TypeUtils::FormatToSerialString(args.src_primary_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_primary_format).c_str(), ShapeToString(args.src_shape).c_str(),
           ShapeToString(args.dst_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Check shape failed, trans format from %s to %s, "
                         "src shape %s, dst shape %s, data type %s is not supported",
                         TypeUtils::FormatToSerialString(args.src_primary_format).c_str(),
                         TypeUtils::FormatToSerialString(args.dst_primary_format).c_str(),
                         ShapeToString(args.src_shape).c_str(), ShapeToString(args.dst_shape).c_str(),
                         TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  return SUCCESS;
}

// NZ -> ND 入口参数校验：dtype 支持性与 src/dst shape 合法性
Status CheckArgsForFracNzToNd(const TransArgs &args) {
  const auto dtype_ret = CheckDataTypeSupportForTrans(args);
  if (dtype_ret != SUCCESS) {
    return dtype_ret;
  }
  if ((!IsShapeValid(args.src_shape)) || (!CheckShapeForTransShapeToFracNz(args.dst_primary_format, args.dst_shape))) {
    GELOGE(ACL_ERROR_GE_SHAPE_INVALID,
           "[Check][Shape]Failed, trans format from %s to %s, src shape %s, dst shape %s, "
           "data type %s is not supported",
           TypeUtils::FormatToSerialString(args.src_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_format).c_str(), ShapeToString(args.src_shape).c_str(),
           ShapeToString(args.dst_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Check shape failed, trans format from %s to %s, src shape %s, "
                         "dst shape %s, data type %s is not supported",
                         TypeUtils::FormatToSerialString(args.src_format).c_str(),
                         TypeUtils::FormatToSerialString(args.dst_format).c_str(),
                         ShapeToString(args.src_shape).c_str(), ShapeToString(args.dst_shape).c_str(),
                         TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  return SUCCESS;
}

Status FormatTransferFractalNz::TransFormat(const TransArgs &args, TransResult &result) {
  const auto check_ret = CheckArgsForNdToFracNz(args);
  if (check_ret != SUCCESS) {
    return check_ret;
  }
  GELOGD("Begin to trans format from %s to %s, src shape %s, dst shape %s, data type %s",
         TypeUtils::FormatToSerialString(args.src_primary_format).c_str(),
         TypeUtils::FormatToSerialString(args.dst_primary_format).c_str(), ShapeToString(args.src_shape).c_str(),
         ShapeToString(args.dst_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());
  ShapeVector expect_shape;
  ShapeVector hw_shape;
  const int64_t c0 = GetEffectiveC0(args.dst_format);
  const auto ret = TransShapeToFracNz(args.src_shape, c0, expect_shape, hw_shape);
  if (ret != SUCCESS) {
    return ret;
  }
  // 子字节 NZ 分形的 M0 由平台 DtypeMKN 配置决定（如 Ascend035 DT_INT4 m0=1），
  // 与 TransShapeToFracNz 硬编码的 kCubeSize=16 不同，因此跳过基于 h0=16 的形状校验，
  // 直接进入子字节折叠分支（该分支使用实际 dst_shape 进行数据搬移）。
  if (IsSubByteDataType(args.src_data_type)) {
    return TransSubByteNdToFracNz(args, result, c0, hw_shape);
  }
  if (!IsTransShapeDstCorrect(args, expect_shape)) {
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  return TransFormatFromNdToFracNz(args, result, hw_shape);
}

Status FormatTransferFractalNz::TransShape(const Format src_format, const std::vector<int64_t> &src_shape,
                                           const DataType data_type, const Format dst_format,
                                           std::vector<int64_t> &dst_shape) {
  const Format src_primary_format = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(src_format)));
  const Format dst_primary_format = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(dst_format)));
  if (!IsDataTypeSupportForTransShapeToFracNz(data_type)) {
    GELOGE(ACL_ERROR_GE_DATATYPE_INVALID,
           "[Check][Datatype]Failed, trans format from %s to %s, src shape %s, "
           "data type %s is not supported",
           TypeUtils::FormatToSerialString(src_primary_format).c_str(),
           TypeUtils::FormatToSerialString(dst_primary_format).c_str(), ShapeToString(src_shape).c_str(),
           TypeUtils::DataTypeToSerialString(data_type).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Check datatype failed, trans format from %s to %s, src shape %s, "
                         "data type %s is not supported",
                         TypeUtils::FormatToSerialString(src_primary_format).c_str(),
                         TypeUtils::FormatToSerialString(dst_primary_format).c_str(), ShapeToString(src_shape).c_str(),
                         TypeUtils::DataTypeToSerialString(data_type).c_str());
    return ACL_ERROR_GE_DATATYPE_INVALID;
  }
  if (!CheckShapeForTransShapeToFracNz(src_primary_format, src_shape)) {
    GELOGE(ACL_ERROR_GE_SHAPE_INVALID,
           "[Check][Shape]Failed, trans format from %s to %s, src shape %s, "
           "data type %s is not supported",
           TypeUtils::FormatToSerialString(src_primary_format).c_str(),
           TypeUtils::FormatToSerialString(dst_primary_format).c_str(), ShapeToString(src_shape).c_str(),
           TypeUtils::DataTypeToSerialString(data_type).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Check shape failed, trans format from %s to %s, src shape %s, "
                         "data type %s is not supported",
                         TypeUtils::FormatToSerialString(src_primary_format).c_str(),
                         TypeUtils::FormatToSerialString(dst_primary_format).c_str(), ShapeToString(src_shape).c_str(),
                         TypeUtils::DataTypeToSerialString(data_type).c_str());
    return ACL_ERROR_GE_SHAPE_INVALID;
  }
  ShapeVector hw_shape;
  const auto w0 = GetEffectiveC0(dst_format);
  return TransShapeToFracNz(src_shape, w0, dst_shape, hw_shape);
}

Status FormatTransferFractalNzND::TransFormat(const TransArgs &args, TransResult &result) {
  const auto check_ret = CheckArgsForFracNzToNd(args);
  if (check_ret != SUCCESS) {
    return check_ret;
  }
  GELOGD("Begin to trans format from %s to %s, src shape %s, dst shape %s, data type %s",
         TypeUtils::FormatToSerialString(args.src_format).c_str(),
         TypeUtils::FormatToSerialString(args.dst_format).c_str(), ShapeToString(args.src_shape).c_str(),
         ShapeToString(args.dst_shape).c_str(), TypeUtils::DataTypeToSerialString(args.src_data_type).c_str());

  ShapeVector hw_shape;
  // src_c0_format 由调用方传入，可能为纯 C0 枚举（无组合位域）导致取值为 -1，这里按 primary 枚举兜底修正
  TransArgs local_args = args;
  if (local_args.src_c0_format <= 0) {
    local_args.src_c0_format = GetEffectiveC0(local_args.src_format);
  }
  // 子字节 NZ 分形的 M0 由平台 DtypeMKN 配置决定（如 Ascend035 DT_INT4 m0=1），
  // 与 TransShapeToFracNz 硬编码的 kCubeSize=16 不同，因此跳过 CheckShapeRelationForTransShapeToFracNz
  // 中的 IsTransShapeSrcCorrect 校验，仅保留 hw_shape 的计算供子字节折叠分支使用。
  if (IsSubByteDataType(args.src_data_type)) {
    return TransSubByteFracNzToNd(args, result, local_args.src_c0_format, hw_shape);
  }
  const Status ret = CheckShapeRelationForTransShapeToFracNz(local_args, hw_shape);
  if (ret != SUCCESS) {
    return ret;
  }
  return TransFormatFromFracNzToNd(args, result, hw_shape);
}

Status FormatTransferFractalNzND::TransShape(const Format src_format, const std::vector<int64_t> &src_shape,
                                             const DataType data_type, const Format dst_format,
                                             std::vector<int64_t> &dst_shape) {
  (void)src_shape;
  (void)data_type;
  (void)dst_shape;
  GELOGD("The shape derivation from %s to %s is not unique. Trans shape is not supported",
         TypeUtils::FormatToSerialString(src_format).c_str(), TypeUtils::FormatToSerialString(dst_format).c_str());
  return ACL_ERROR_GE_FORMAT_INVALID;
}

REGISTER_FORMAT_TRANSFER(FormatTransferFractalNz, FORMAT_ND, FORMAT_FRACTAL_NZ)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNz, FORMAT_NCHW, FORMAT_FRACTAL_NZ)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNz, FORMAT_NHWC, FORMAT_FRACTAL_NZ)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNzND, FORMAT_FRACTAL_NZ, FORMAT_ND)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNzND, FORMAT_FRACTAL_NZ, FORMAT_NCHW)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNzND, FORMAT_FRACTAL_NZ, FORMAT_NHWC)
// C0 显式变体（子字节 dtype 场景使用，如 DT_INT4 权重 ND→NZ_C0_16/C0_32）
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNz, FORMAT_ND, FORMAT_FRACTAL_NZ_C0_16)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNz, FORMAT_ND, FORMAT_FRACTAL_NZ_C0_32)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNzND, FORMAT_FRACTAL_NZ_C0_16, FORMAT_ND)
REGISTER_FORMAT_TRANSFER(FormatTransferFractalNzND, FORMAT_FRACTAL_NZ_C0_32, FORMAT_ND)
}  // namespace formats
}  // namespace ge
