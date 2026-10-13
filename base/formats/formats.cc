/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "formats/formats.h"

#include <securec.h>

#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "formats/utils/formats_trans_utils.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/utils/type_utils.h"

namespace ge {
namespace formats {
namespace {
// Element-level format transfer indexes data by per-element byte size.
// Sub-byte types (e.g. DT_INT4) encode bit-width in GetSizeByDataType (kDataTypeSizeBitOffset + bits),
// which is not a real byte size, so they cannot be processed by existing transfers,
// except the fractal NZ family transfers which handle packed data by bit-width.
bool IsSubByteDataType(const DataType data_type) {
  return GetSizeByDataType(data_type) > kDataTypeSizeBitOffset;
}

bool IsNdFamilyFormat(const Format format) {
  const auto primary = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(format)));
  return (primary == FORMAT_ND) || (primary == FORMAT_NCHW) || (primary == FORMAT_NHWC);
}

bool IsFractalNzFamilyFormat(const Format format) {
  const auto primary = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(format)));
  return (primary == FORMAT_FRACTAL_NZ) || (primary == FORMAT_FRACTAL_NZ_C0_2) || (primary == FORMAT_FRACTAL_NZ_C0_4) ||
         (primary == FORMAT_FRACTAL_NZ_C0_8) || (primary == FORMAT_FRACTAL_NZ_C0_16) ||
         (primary == FORMAT_FRACTAL_NZ_C0_32);
}

// 判断格式是否显式携带 C0：primary 枚举 C0 变体（如 FRACTAL_NZ_C0_16/C0_32）或组合位域
bool HasExplicitC0(const Format format) {
  if (GetC0Value(static_cast<int32_t>(format)) > 0) {
    return true;
  }
  const auto primary = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(format)));
  return (primary == FORMAT_FRACTAL_NZ_C0_2) || (primary == FORMAT_FRACTAL_NZ_C0_4) ||
         (primary == FORMAT_FRACTAL_NZ_C0_8) || (primary == FORMAT_FRACTAL_NZ_C0_16) ||
         (primary == FORMAT_FRACTAL_NZ_C0_32);
}

bool IsSubByteSupportedTransfer(const TransArgs &args) {
  const bool nd_to_nz = IsNdFamilyFormat(args.src_format) && IsFractalNzFamilyFormat(args.dst_format);
  const bool nz_to_nd = IsFractalNzFamilyFormat(args.src_format) && IsNdFamilyFormat(args.dst_format);
  if (!nd_to_nz && !nz_to_nd) {
    return false;
  }
  // 子字节场景要求 NZ 侧显式携带 C0，裸 FORMAT_FRACTAL_NZ 无法确定 C0，直接拒绝
  if (!HasExplicitC0(args.src_format) && !HasExplicitC0(args.dst_format)) {
    return false;
  }
  // 显式 C0 的 NZ 格式仅注册了与 FORMAT_ND 的互转，门禁与注册表口径保持一致，
  // 避免落入 BuildFormatTransfer 后报误导性 FORMAT_INVALID
  return (args.src_format == FORMAT_ND) || (args.dst_format == FORMAT_ND);
}
}  // namespace

Status TransDataFormat(const TransArgs &args, TransResult &result) {
  if (IsSubByteDataType(args.src_data_type)) {
    // 子字节类型仅放行 ND/NCHW/NHWC 与 FRACTAL_NZ 族互转（packed 位级转换），其余拒绝
    GELOGI("TransDataFormat sub-byte dtype %s, src format %s -> dst format %s, supported transfer %d",
           TypeUtils::DataTypeToSerialString(args.src_data_type).c_str(),
           TypeUtils::FormatToSerialString(args.src_format).c_str(),
           TypeUtils::FormatToSerialString(args.dst_format).c_str(), IsSubByteSupportedTransfer(args) ? 1 : 0);
    if (!IsSubByteSupportedTransfer(args)) {
      const std::string error = "Failed to trans data format, sub-byte data type " +
                                FmtToStr(TypeUtils::DataTypeToSerialString(args.src_data_type)) + " transfer from " +
                                TypeUtils::FormatToSerialString(args.src_format) + " to " +
                                TypeUtils::FormatToSerialString(args.dst_format) +
                                " is not supported, only nd <-> fractal nz with explicit c0 is supported";
      GE_ERRORLOG_AND_ERRORMSG(ACL_ERROR_GE_DATATYPE_INVALID, error.c_str());
      return ACL_ERROR_GE_DATATYPE_INVALID;
    }
  }

  const auto transfer = BuildFormatTransfer(args);
  if (transfer == nullptr) {
    const std::string error = "Failed to trans data from format " +
                              FmtToStr(TypeUtils::FormatToSerialString(args.src_format)) + " to " +
                              FmtToStr(TypeUtils::FormatToSerialString(args.dst_format));
    GE_ERRORLOG_AND_ERRORMSG(ACL_ERROR_GE_FORMAT_INVALID, error.c_str());
    return ACL_ERROR_GE_FORMAT_INVALID;
  }

  const auto src_shape_size = GetItemNumByShape(args.src_shape);
  if ((args.data == nullptr) && (src_shape_size != 0)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Check][Shape]Failed, input data is null "
           "or shape size not equal to 0, src_shape %s",
           ShapeToString(args.src_shape).c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "Failed to check shape, input data is null "
                         "or shape size not equal to 0, src_shape %s",
                         ShapeToString(args.src_shape).c_str());
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  return transfer->TransFormat(args, result);
}

Status TransTensorShape(const Format src_format, const std::vector<int64_t> &src_shape, const DataType data_type,
                        const Format dst_format, std::vector<int64_t> &dst_shape) {
  formats::TransArgs args;
  args.src_format = src_format;
  args.dst_format = dst_format;
  args.src_primary_format = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(src_format)));
  args.dst_primary_format = static_cast<Format>(GetPrimaryFormat(static_cast<int32_t>(dst_format)));
  const auto transfer = BuildFormatTransfer(args);
  if (transfer == nullptr) {
    const std::string error = "Failed to trans data from format " +
                              FmtToStr(TypeUtils::FormatToSerialString(args.src_primary_format)) + " to " +
                              FmtToStr(TypeUtils::FormatToSerialString(args.dst_primary_format));
    GE_ERRORLOG_AND_ERRORMSG(ACL_ERROR_GE_FORMAT_INVALID, error.c_str());
    return ACL_ERROR_GE_FORMAT_INVALID;
  }

  return transfer->TransShape(src_format, src_shape, data_type, dst_format, dst_shape);
}

bool IsTransFormatSupport(const TransArgs &args) {
  return FormatTransferExists(args);
}
}  // namespace formats
}  // namespace ge
