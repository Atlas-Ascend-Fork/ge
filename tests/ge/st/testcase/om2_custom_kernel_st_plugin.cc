/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "ge/ge_ir_build.h"

// Export only the FXRT-style one-context ABI. Registration is driven by the graph attributes.
extern "C" __attribute__((visibility("default"))) ge::graphStatus CreateChecksum(gert::KernelContext *ctx) {
  auto *checksum = ctx->GetOutputPointer<int64_t>(0U);
  if (checksum == nullptr) {
    return ge::GRAPH_FAILED;
  }
  *checksum = 11;
  return ge::GRAPH_SUCCESS;
}

extern "C" __attribute__((visibility("default"))) ge::graphStatus ReadBuffer(gert::KernelContext *ctx) {
  const auto *data = ctx->GetInputPointer<ge::ModelBufferData>(0U);
  const auto *empty = ctx->GetInputPointer<ge::ModelBufferData>(1U);
  auto *checksum = ctx->GetOutputPointer<int64_t>(0U);
  if (data == nullptr || data->length != 3U || !data->data || empty == nullptr || empty->length != 0U || empty->data ||
      checksum == nullptr) {
    return ge::GRAPH_FAILED;
  }
  for (size_t i = 0U; i < data->length; ++i) {
    *checksum += static_cast<int64_t>(i + 1U) * data->data.get()[i];
  }
  return ge::GRAPH_SUCCESS;
}

extern "C" __attribute__((visibility("default"))) ge::graphStatus WriteChecksum(gert::KernelContext *ctx) {
  const auto *checksum = ctx->GetInputPointer<int64_t>(0U);
  auto *tensor = ctx->MutableInputPointer<gert::Tensor>(1U);
  if (checksum == nullptr || tensor == nullptr || tensor->GetAddr() == nullptr) {
    return ge::GRAPH_FAILED;
  }
  *tensor->GetData<int64_t>() = *checksum;
  return ge::GRAPH_SUCCESS;
}
