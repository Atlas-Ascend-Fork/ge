/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef INC_OM2_REGISTER_REGISTER_FORMAT_TRANSFER_H_
#define INC_OM2_REGISTER_REGISTER_FORMAT_TRANSFER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

#include "graph/types.h"
#include "ge/ge_api_error_codes.h"
#include "common/plugin/ge_make_unique_util.h"
#include "base/err_msg.h"

namespace gert {
namespace formats {
struct TransArgs {
  const uint8_t *data;
  ge::Format src_format;          // src full format
  ge::Format dst_format;          // dst full format
  ge::Format src_primary_format;  // src primary format
  ge::Format dst_primary_format;  // dst primary format
  ge::Format src_sub_format;      // src sub format
  ge::Format dst_sub_format;      // dst sub format
  int64_t src_c0_format;          // src s0 format
  int64_t dst_c0_format;          // dst c0 format
  // For scenes that need to supplement the shape, for example, 5D to 4D
  // It is not possible to convert the format normally if you only get the src_shape,
  // and must get the shape before you mend the shape.
  // So the parameters here need to be passed in both src_shape and dst_shape
  std::vector<int64_t> src_shape;
  std::vector<int64_t> dst_shape;
  ge::DataType src_data_type;
};

struct TransResult {
  std::shared_ptr<uint8_t> data;
  // data length in bytes
  size_t length;
};

struct CastArgs {
  const uint8_t *data;
  size_t src_data_size;
  ge::DataType src_data_type;
  ge::DataType dst_data_type;
};

class FormatTransfer {
 public:
  FormatTransfer() = default;
  virtual ~FormatTransfer() = default;
  virtual ge::Status TransFormat(const TransArgs &args, TransResult &result) = 0;
  virtual ge::Status TransShape(ge::Format src_format, const std::vector<int64_t> &src_shape, ge::DataType data_type,
                                ge::Format dst_format, std::vector<int64_t> &dst_shape) = 0;

 private:
  FormatTransfer(const FormatTransfer &) = delete;
  FormatTransfer &operator=(const FormatTransfer &) = delete;
};

using FormatTransferBuilder = std::function<std::shared_ptr<FormatTransfer>()>;

class FormatTransferRegister {
 public:
  FormatTransferRegister(FormatTransferBuilder builder, const ge::Format src, const ge::Format dst) noexcept;
  ~FormatTransferRegister() = default;
};

/// Build a formattransfer according to 'args'
/// @param args
/// @param result
/// @return
std::shared_ptr<FormatTransfer> BuildFormatTransfer(const TransArgs &args);

bool FormatTransferExists(const TransArgs &args);
}  // namespace formats
}  // namespace gert

// format1/format2 参与 ## 粘贴，调用点必须传裸 token（ge::FORMAT_X 是 3 个预处理 token，粘贴后非法）；
// 值使用处的 ge:: 限定只能写在宏体内。
#define REGISTER_FORMAT_TRANSFER(TransferClass, format1, format2)                                                 \
  namespace {                                                                                                     \
  std::shared_ptr<FormatTransfer> Transfer_##format1##_##format2() {                                              \
    return MakeShared<TransferClass>();                                                                           \
  }                                                                                                               \
  FormatTransferRegister format_transfer_##TransferClass##_##format1##_##format2(&Transfer_##format1##_##format2, \
                                                                                 (ge::format1), (ge::format2));   \
  }
#endif  // INC_OM2_REGISTER_REGISTER_FORMAT_TRANSFER_H_
