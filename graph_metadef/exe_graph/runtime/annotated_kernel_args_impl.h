/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_EXE_GRAPH_RUNTIME_ANNOTATED_KERNEL_ARGS_IMPL_H_
#define METADEF_CXX_EXE_GRAPH_RUNTIME_ANNOTATED_KERNEL_ARGS_IMPL_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "common/checker.h"
#include "graph/error_codes.h"
#include "graph/utils/args_format_desc_utils.h"

namespace gert {
namespace internal {
constexpr size_t kKernelArgSlotSize = sizeof(uint64_t);

struct AnnotatedKernelArgsImpl {
  ge::graphStatus status = ge::GRAPH_SUCCESS;
  std::vector<uint8_t> args;
  std::vector<ge::ArgDesc> arg_descs;
  // 通过 AppendArg(CustomDataBlock) 追加的变长数据字节，按追加顺序排列；
  // arg_descs 中 CUSTOM_DATA_BLOCK 项的 ir_idx 即为该 blob 在本向量中的下标
  std::vector<std::vector<uint8_t>> custom_data_blobs;
};

inline ge::graphStatus ExtractArgsData(const AnnotatedKernelArgsImpl *const args_impl, std::vector<uint8_t> &args_data,
                                       std::vector<ge::ArgDesc> &arg_descs) {
  GE_ASSERT_NOTNULL(args_impl);
  GE_ASSERT_SUCCESS(args_impl->status, "Annotated kernel args is invalid.");
  GE_ASSERT_TRUE(!args_impl->args.empty(), "Annotated kernel args is empty.");
  GE_ASSERT_TRUE(args_impl->arg_descs.size() <= (std::numeric_limits<size_t>::max() / kKernelArgSlotSize),
                 "Annotated kernel arg desc size %zu overflow.", args_impl->arg_descs.size());
  GE_ASSERT_TRUE(args_impl->args.size() == (args_impl->arg_descs.size() * kKernelArgSlotSize),
                 "Annotated kernel args size %zu does not match arg desc size %zu.", args_impl->args.size(),
                 args_impl->arg_descs.size());
  args_data = args_impl->args;
  arg_descs = args_impl->arg_descs;
  return ge::GRAPH_SUCCESS;
}

inline ge::graphStatus ExtractArgsData(const AnnotatedKernelArgsImpl *const args_impl, std::vector<uint8_t> &args_data,
                                       std::vector<ge::ArgDesc> &arg_descs,
                                       std::vector<std::vector<uint8_t>> &custom_data_blobs) {
  GE_ASSERT_NOTNULL(args_impl);
  GE_ASSERT_SUCCESS(ExtractArgsData(args_impl, args_data, arg_descs));
  custom_data_blobs = args_impl->custom_data_blobs;
  return ge::GRAPH_SUCCESS;
}
}  // namespace internal
}  // namespace gert

#endif  // METADEF_CXX_EXE_GRAPH_RUNTIME_ANNOTATED_KERNEL_ARGS_IMPL_H_
