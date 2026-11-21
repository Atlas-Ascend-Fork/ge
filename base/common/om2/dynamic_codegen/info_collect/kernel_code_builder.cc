/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "kernel_code_builder.h"

#include <utility>

#include "framework/common/debug/ge_log.h"

namespace gert {
namespace om2 {
KernelImplType KernelCodeBuilder::GetImplType() const {
  return KernelImplType::kRunFuncImpl;
}

std::vector<uint32_t> KernelCodeBuilder::GetRestoreConstInputIndices() const {
  return {};
}

std::vector<uint32_t> KernelCodeBuilder::GetCustomRestoreConstInputIndices() const {
  return {};
}

ge::graphStatus KernelCodeBuilder::RestoreConstInput(const KernelCollectContext &context, uint32_t input_index,
                                                     CustomConstRestoreDescriptor &descriptor) const {
  (void)context;
  (void)input_index;
  (void)descriptor;
  return ge::GRAPH_PARAM_INVALID;
}

ge::graphStatus KernelCodeBuilder::GenerateKernelCode(const std::string &function_symbol, std::string &code) const {
  (void)function_symbol;
  (void)code;
  return ge::GRAPH_PARAM_INVALID;
}

ge::graphStatus KernelCodeBuilder::GenerateOutputsCreatorCode(std::string &code) const {
  (void)code;
  return ge::GRAPH_PARAM_INVALID;
}

KernelCodeBuilderFactory &KernelCodeBuilderFactory::GetInstance() {
  static KernelCodeBuilderFactory factory;
  return factory;
}

ge::graphStatus KernelCodeBuilderFactory::Register(const char *kernel_type, Creator creator) {
  if ((kernel_type == nullptr) || !creator) {
    return ge::GRAPH_PARAM_INVALID;
  }
  const std::lock_guard<std::mutex> lock(mutex_);
  if (!creators_.emplace(kernel_type, std::move(creator)).second) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Duplicated OM2 kernel code builder for %s", kernel_type);
    return ge::GRAPH_PARAM_INVALID;
  }
  return ge::GRAPH_SUCCESS;
}

std::shared_ptr<KernelCodeBuilder> KernelCodeBuilderFactory::Create(const char *kernel_type) const {
  if (kernel_type == nullptr) {
    return nullptr;
  }
  Creator creator;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = creators_.find(kernel_type);
    if (iter == creators_.cend()) {
      return nullptr;
    }
    creator = iter->second;
  }
  return creator();
}

KernelCodeBuilderRegistrar::KernelCodeBuilderRegistrar(const char *kernel_type,
                                                       KernelCodeBuilderFactory::Creator creator) {
  const auto ret = KernelCodeBuilderFactory::GetInstance().Register(kernel_type, std::move(creator));
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ret, "Failed to register OM2 kernel code builder %s", kernel_type == nullptr ? "<null>" : kernel_type);
  }
}
}  // namespace om2
}  // namespace gert
