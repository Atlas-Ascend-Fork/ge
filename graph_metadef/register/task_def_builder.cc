/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <new>
#include "exe_graph/runtime/task_def_builder.h"
#include "proto/task.pb.h"

namespace gert {
TaskDefBuilder::TaskDefBuilder() : impl_(new (std::nothrow) domi::TaskDef()) {}

KernelDefBuilder::KernelDefBuilder(domi::KernelDef *impl) : impl_(impl) {}

KernelContextBuilder::KernelContextBuilder(domi::KernelContext *impl) : impl_(impl) {}

TaskDefBuilder::~TaskDefBuilder() {
  delete impl_;
  impl_ = nullptr;
}

TaskDefBuilder &TaskDefBuilder::SetStreamId(uint32_t stream_id) {
  if (impl_ != nullptr) {
    impl_->set_stream_id(stream_id);
  }
  return *this;
}

TaskDefBuilder &TaskDefBuilder::SetType(uint32_t type) {
  if (impl_ != nullptr) {
    impl_->set_type(type);
  }
  return *this;
}

KernelDefBuilder TaskDefBuilder::GetKernelBuilder() const {
  return KernelDefBuilder(impl_ != nullptr ? impl_->mutable_kernel() : nullptr);
}

size_t TaskDefBuilder::ByteSizeLong() const {
  return impl_ != nullptr ? static_cast<size_t>(impl_->ByteSizeLong()) : 0U;
}

bool TaskDefBuilder::SerializeToArray(uint8_t *data, size_t size) const {
  if ((impl_ == nullptr) || (data == nullptr) || (size == 0U)) {
    return false;
  }
  return impl_->SerializeToArray(data, static_cast<int>(size));
}

KernelDefBuilder &KernelDefBuilder::SetBlockDim(uint32_t block_dim) {
  if (impl_ != nullptr) {
    impl_->set_block_dim(block_dim);
  }
  return *this;
}

KernelContextBuilder KernelDefBuilder::GetContextBuilder() const {
  return KernelContextBuilder(impl_ != nullptr ? impl_->mutable_context() : nullptr);
}

KernelContextBuilder &KernelContextBuilder::SetArgsFormat(const ge::AscendString &args_format) {
  if (impl_ != nullptr) {
    impl_->set_args_format(args_format.GetString());
  }
  return *this;
}
}  // namespace gert
