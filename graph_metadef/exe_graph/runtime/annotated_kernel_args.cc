/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "exe_graph/runtime/annotated_args_context.h"

#include <limits>
#include <new>
#include <utility>
#include <securec.h>

#include "annotated_kernel_args_impl.h"
#include "graph/utils/args_format_desc_utils.h"

namespace gert {
namespace {
void AppendSlot(const uint64_t value, internal::AnnotatedKernelArgsImpl &impl) {
  const auto old_size = impl.args.size();
  impl.args.resize(old_size + internal::kKernelArgSlotSize);
  static_assert(sizeof(value) == internal::kKernelArgSlotSize, "kernel arg slot must be 8 bytes");
  auto ret = memcpy_s(impl.args.data() + old_size, internal::kKernelArgSlotSize, &value, sizeof(value));
  if (ret != EOK) {
    GELOGE(ge::FAILED, "Append kernel arg slot failed, memcpy_s failed, ret=%d", ret);
    impl.status = ge::GRAPH_FAILED;
  }
}

bool IsValidIndex(const uint32_t index) {
  return index <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
}

ge::graphStatus AppendAddrArg(const ge::AddrType addr_type, const uint32_t index, const void *const addr,
                              internal::AnnotatedKernelArgsImpl *const impl) {
  if (impl == nullptr) {
    return ge::GRAPH_FAILED;
  }
  if (!IsValidIndex(index)) {
    impl->status = ge::GRAPH_FAILED;
    return impl->status;
  }
  ge::ArgsFormatDescUtils::Append(impl->arg_descs, addr_type, static_cast<int32_t>(index));
  AppendSlot(reinterpret_cast<uint64_t>(addr), *impl);
  return impl->status;
}
}  // namespace

AnnotatedKernelArgs::AnnotatedKernelArgs() : impl_(new (std::nothrow) internal::AnnotatedKernelArgsImpl()) {}

AnnotatedKernelArgs::AnnotatedKernelArgs(const AnnotatedKernelArgs &other)
    : impl_(new (std::nothrow) internal::AnnotatedKernelArgsImpl()) {
  const auto *other_impl = static_cast<const internal::AnnotatedKernelArgsImpl *>(other.impl_);
  auto *impl = static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  if ((impl != nullptr) && (other_impl != nullptr)) {
    *impl = *other_impl;
  } else if (impl != nullptr) {
    impl->status = ge::GRAPH_FAILED;
  }
}

AnnotatedKernelArgs::AnnotatedKernelArgs(AnnotatedKernelArgs &&other) noexcept : impl_(other.impl_) {
  other.impl_ = nullptr;
}

AnnotatedKernelArgs &AnnotatedKernelArgs::operator=(const AnnotatedKernelArgs &other) {
  if (this == &other) {
    return *this;
  }
  if (impl_ == nullptr) {
    impl_ = new (std::nothrow) internal::AnnotatedKernelArgsImpl();
  }
  const auto *other_impl = static_cast<const internal::AnnotatedKernelArgsImpl *>(other.impl_);
  auto *impl = static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  if ((impl != nullptr) && (other_impl != nullptr)) {
    *impl = *other_impl;
  } else if (impl != nullptr) {
    impl->status = ge::GRAPH_FAILED;
  }
  return *this;
}

AnnotatedKernelArgs &AnnotatedKernelArgs::operator=(AnnotatedKernelArgs &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  delete static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  impl_ = other.impl_;
  other.impl_ = nullptr;
  return *this;
}

AnnotatedKernelArgs::~AnnotatedKernelArgs() {
  delete static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  impl_ = nullptr;
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const InputAddr &addr) {
  return AppendAddrArg(ge::AddrType::INPUT_INSTANCE, addr.index, addr.addr,
                       static_cast<internal::AnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const OutputAddr &addr) {
  return AppendAddrArg(ge::AddrType::OUTPUT_INSTANCE, addr.index, addr.addr,
                       static_cast<internal::AnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const WorkspaceAddr &addr) {
  return AppendAddrArg(ge::AddrType::WORKSPACE, addr.index, addr.addr,
                       static_cast<internal::AnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(uint64_t value) {
  auto *impl = static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  if (impl == nullptr) {
    return ge::GRAPH_FAILED;
  }
  impl->status = ge::ArgsFormatDescUtils::InsertCustomValue(impl->arg_descs, -1, value);
  if (impl->status != ge::GRAPH_SUCCESS) {
    return impl->status;
  }
  AppendSlot(value, *impl);
  return impl->status;
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const CustomDataBlock &data) {
  auto *impl = static_cast<internal::AnnotatedKernelArgsImpl *>(impl_);
  if (impl == nullptr) {
    GELOGE(ge::GRAPH_FAILED, "Append custom data failed, kernel args has been moved.");
    return ge::GRAPH_FAILED;
  }
  if (impl->status != ge::GRAPH_SUCCESS) {
    return impl->status;
  }
  if (data.size == 0UL) {
    impl->status = ge::GRAPH_FAILED;
    GELOGE(impl->status, "Append custom data failed, data size must be greater than zero.");
    return impl->status;
  }
  if (data.data == nullptr) {
    impl->status = ge::GRAPH_FAILED;
    GELOGE(impl->status, "Append custom data failed, data is null while size is %zu.", data.size);
    return impl->status;
  }
  // reserved 是未来扩展的占位字段，当前必须全 0：后续版本启用该字段后，非 0 的历史脏值会被误判为显式语义
  for (size_t i = 0U; i < sizeof(data.reserved); ++i) {
    if (data.reserved[i] != 0U) {
      impl->status = ge::GRAPH_FAILED;
      GELOGE(impl->status, "Append custom data failed, reserved must be all zero but reserved[%zu] is %u.", i,
             static_cast<uint32_t>(data.reserved[i]));
      return impl->status;
    }
  }
  if (impl->custom_data_blobs.size() >= static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
    impl->status = ge::INTERNAL_ERROR;
    GELOGE(impl->status, "Append custom data failed, blob count %zu overflows int32.", impl->custom_data_blobs.size());
    return impl->status;
  }
  if (impl->args.size() > (std::numeric_limits<size_t>::max() - internal::kKernelArgSlotSize)) {
    impl->status = ge::INTERNAL_ERROR;
    GELOGE(impl->status, "Append custom data failed, args size %zu overflows.", impl->args.size());
    return impl->status;
  }
  // 先完成可能抛 bad_alloc 的字节拷贝，避免失败时已改动 args/arg_descs 而破坏 8 字节槽不变式
  const auto *const begin = static_cast<const uint8_t *>(data.data);
  std::vector<uint8_t> blob(begin, begin + data.size);

  const auto size = static_cast<uint64_t>(data.size);
  ge::ArgDesc arg_desc = {
      ge::AddrType::CUSTOM_DATA_BLOCK, static_cast<int32_t>(impl->custom_data_blobs.size()), false, {0}};
  // reserved 位于 ArgDesc 偏移 9，非 8 字节对齐，禁止直接解引用
  if (memcpy_s(arg_desc.reserved, sizeof(arg_desc.reserved), &size, sizeof(size)) != EOK) {
    impl->status = ge::GRAPH_FAILED;
    GELOGE(impl->status, "Append custom data failed, write size %zu into reserved failed.", data.size);
    return impl->status;
  }

  const auto args_size_before = impl->args.size();
  impl->arg_descs.emplace_back(arg_desc);
  // 编译期先占一个值为 0 的 8 字节槽，运行期由 CustomTaskInfo 填入真实 device 地址
  AppendSlot(0UL, *impl);
  if (impl->status != ge::GRAPH_SUCCESS) {
    impl->arg_descs.pop_back();
    impl->args.resize(args_size_before);
    return impl->status;
  }
  impl->custom_data_blobs.emplace_back(std::move(blob));
  return impl->status;
}

ge::graphStatus AnnotatedKernelArgs::ExtractArgsData(std::vector<uint8_t> &args_data,
                                                     std::vector<ge::ArgDesc> &arg_descs) const {
  return internal::ExtractArgsData(static_cast<const internal::AnnotatedKernelArgsImpl *>(impl_), args_data, arg_descs);
}

}  // namespace gert
