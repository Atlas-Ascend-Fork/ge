/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "exe_graph/runtime/eager_op_execution_context.h"
#include "exe_graph/runtime/host_cpu_op_execution_context.h"
#include "exe_graph/runtime/annotated_args_context.h"
#include "exe_graph/runtime/resource_usage_context.h"
#include "runtime/annotated_args_handler.h"

#include <limits>
#include <new>
#include <securec.h>
#include <utility>
#include <vector>

#include "graph/utils/args_format_desc_utils.h"

namespace gert {
namespace {
constexpr size_t kKernelArgSlotSize = sizeof(uint64_t);
constexpr int32_t kCustomValueWidthBit64 = -1;

struct StubAnnotatedKernelArgsImpl {
  std::vector<uint8_t> args;
  std::vector<ge::ArgDesc> arg_descs;
  std::vector<std::vector<uint8_t>> custom_data_blobs;
  ge::graphStatus status = ge::GRAPH_SUCCESS;
};

void AppendSlot(const uint64_t value, StubAnnotatedKernelArgsImpl &impl) {
  const auto old_size = impl.args.size();
  impl.args.resize(old_size + kKernelArgSlotSize);
  if (memcpy_s(impl.args.data() + old_size, kKernelArgSlotSize, &value, sizeof(value)) != EOK) {
    impl.status = ge::GRAPH_FAILED;
  }
}

bool IsValidIndex(const uint32_t index) {
  return index <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
}

ge::graphStatus AppendAddrArg(const ge::AddrType addr_type, const uint32_t index, const void *const addr,
                              StubAnnotatedKernelArgsImpl *const impl) {
  if (impl == nullptr) {
    return ge::GRAPH_FAILED;
  }
  if (!IsValidIndex(index)) {
    impl->status = ge::GRAPH_FAILED;
    return impl->status;
  }
  impl->arg_descs.push_back({addr_type, static_cast<int32_t>(index), false, {0}});
  AppendSlot(reinterpret_cast<uint64_t>(addr), *impl);
  return impl->status;
}

}  // namespace

AnnotatedKernelArgs::AnnotatedKernelArgs(const AnnotatedKernelArgs &other)
    : impl_(new (std::nothrow) StubAnnotatedKernelArgsImpl()) {
  const auto *other_impl = static_cast<const StubAnnotatedKernelArgsImpl *>(other.impl_);
  auto *impl = static_cast<StubAnnotatedKernelArgsImpl *>(impl_);
  if ((impl != nullptr) && (other_impl != nullptr)) {
    *impl = *other_impl;
  } else if (impl != nullptr) {
    impl->status = ge::GRAPH_FAILED;
  }
}

AnnotatedKernelArgs::AnnotatedKernelArgs() : impl_(new (std::nothrow) StubAnnotatedKernelArgsImpl()) {}

AnnotatedKernelArgs::AnnotatedKernelArgs(AnnotatedKernelArgs &&other) noexcept : impl_(other.impl_) {
  other.impl_ = nullptr;
}

AnnotatedKernelArgs &AnnotatedKernelArgs::operator=(const AnnotatedKernelArgs &other) {
  if (this == &other) {
    return *this;
  }
  if (impl_ == nullptr) {
    impl_ = new (std::nothrow) StubAnnotatedKernelArgsImpl();
  }
  const auto *other_impl = static_cast<const StubAnnotatedKernelArgsImpl *>(other.impl_);
  auto *impl = static_cast<StubAnnotatedKernelArgsImpl *>(impl_);
  if ((impl != nullptr) && (other_impl != nullptr)) {
    *impl = *other_impl;
  } else if (impl != nullptr) {
    impl->status = ge::GRAPH_FAILED;
  }
  return *this;
}

AnnotatedKernelArgs &AnnotatedKernelArgs::operator=(AnnotatedKernelArgs &&other) noexcept {
  if (this != &other) {
    delete static_cast<StubAnnotatedKernelArgsImpl *>(impl_);
    impl_ = other.impl_;
    other.impl_ = nullptr;
  }
  return *this;
}

AnnotatedKernelArgs::~AnnotatedKernelArgs() {
  delete static_cast<StubAnnotatedKernelArgsImpl *>(impl_);
  impl_ = nullptr;
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const InputAddr &addr) {
  return AppendAddrArg(ge::AddrType::INPUT_INSTANCE, addr.index, addr.addr,
                       static_cast<StubAnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const OutputAddr &addr) {
  return AppendAddrArg(ge::AddrType::OUTPUT_INSTANCE, addr.index, addr.addr,
                       static_cast<StubAnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const WorkspaceAddr &addr) {
  return AppendAddrArg(ge::AddrType::WORKSPACE, addr.index, addr.addr,
                       static_cast<StubAnnotatedKernelArgsImpl *>(impl_));
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(uint64_t value) {
  auto *impl = static_cast<StubAnnotatedKernelArgsImpl *>(impl_);
  if (impl == nullptr) {
    return ge::GRAPH_FAILED;
  }
  ge::ArgDesc arg = {ge::AddrType::CUSTOM_VALUE, kCustomValueWidthBit64, false, {0}};
  if (memcpy_s(arg.reserved, sizeof(arg.reserved), &value, sizeof(value)) != EOK) {
    impl->status = ge::GRAPH_FAILED;
    return impl->status;
  }
  impl->arg_descs.emplace_back(arg);
  AppendSlot(value, *impl);
  return impl->status;
}

ge::graphStatus AnnotatedKernelArgs::AppendArg(const CustomDataBlock &data) {
  (void)data;
  return ge::GRAPH_FAILED;
}

ge::graphStatus AnnotatedKernelArgs::ExtractArgsData(std::vector<uint8_t> &args_data,
                                                     std::vector<ge::ArgDesc> &arg_descs) const {
  const auto *const impl = static_cast<const StubAnnotatedKernelArgsImpl *>(impl_);
  if ((impl == nullptr) || (impl->status != ge::GRAPH_SUCCESS) || impl->args.empty() ||
      (impl->arg_descs.size() > (std::numeric_limits<size_t>::max() / kKernelArgSlotSize)) ||
      (impl->args.size() != (impl->arg_descs.size() * kKernelArgSlotSize))) {
    return ge::GRAPH_FAILED;
  }
  args_data = impl->args;
  arg_descs = impl->arg_descs;
  return ge::GRAPH_SUCCESS;
}

rtStream EagerOpExecutionContext::GetStream() const {
  return nullptr;
}

Tensor *EagerOpExecutionContext::MallocOutputTensor(size_t index, const StorageShape &shape,
                                                    const StorageFormat &format, ge::DataType dtype) {
  (void)index;
  (void)shape;
  (void)format;
  (void)dtype;
  return nullptr;
}

Tensor *EagerOpExecutionContext::MakeOutputRefInput(size_t output_index, size_t input_index) const {
  (void)output_index;
  (void)input_index;
  return nullptr;
}

void *EagerOpExecutionContext::MallocWorkSpace(size_t size) {
  (void)size;
  return nullptr;
}

const KernelArgs *EagerOpExecutionContext::MallocReadOnlyDevArgs(void *host_args, size_t args_size) const {
  (void)host_args;
  (void)args_size;
  return nullptr;
}

bool EagerOpExecutionContext::GetContextValue(ContextParam key, uint64_t &value) const {
  (void)key;
  (void)value;
  return false;
}

bool EagerOpExecutionContext::GetOption(const ge::AscendString &option_key, ge::AscendString &option_value) const {
  (void)option_key;
  (void)option_value;
  return false;
}

WorkspaceAddr AnnotatedArgsContext::MallocWorkSpace(size_t size) {
  (void)size;
  return WorkspaceAddr{0U, nullptr};
}

uint32_t AnnotatedArgsContext::GetStreamId() const {
  return 0U;
}

uint32_t AnnotatedArgsContext::RequestAttachedStream(const ge::AscendString &key) {
  (void)key;
  return 0U;
}

ge::graphStatus AnnotatedArgsContext::AddLaunch(const AnnotatedKernelLaunchInfo &launch_info,
                                                AnnotatedKernelArgs &&args) {
  (void)launch_info;
  (void)args;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ResourceUsageContext::ReportAttachedStream(const std::vector<ge::AscendString> &keys) {
  (void)keys;
  return ge::GRAPH_FAILED;
}

AnnotatedLaunchToken AnnotatedArgsContext::AddLaunch(const AnnotatedKernelLaunchInfo &launch_info,
                                                     AnnotatedKernelArgs &&args,
                                                     const std::vector<AnnotatedLaunchToken> &predecessors) {
  (void)launch_info;
  (void)args;
  (void)predecessors;
  return 0U;
}

Tensor *HostCpuOpExecutionContext::MallocOutputTensor(size_t index, const StorageShape &shape,
                                                      const StorageFormat &format, ge::DataType dtype) {
  (void)index;
  (void)shape;
  (void)format;
  (void)dtype;
  return nullptr;
}

Tensor *HostCpuOpExecutionContext::MakeOutputRefInput(size_t output_index, size_t input_index) {
  (void)output_index;
  (void)input_index;
  return nullptr;
}
}  // namespace gert
