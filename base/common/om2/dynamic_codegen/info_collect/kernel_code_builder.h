/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_CODE_BUILDER_H_
#define AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_CODE_BUILDER_H_

#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "graph/error_codes.h"
#include "kernel_collect_context.h"
#include "program_info.h"

namespace gert {
namespace om2 {
constexpr uint32_t kKernelLastInputIndex = std::numeric_limits<uint32_t>::max();

class KernelCodeBuilder {
 public:
  virtual ~KernelCodeBuilder() = default;

  virtual ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) = 0;
  virtual KernelImplType GetImplType() const;
  virtual std::vector<uint32_t> GetRestoreConstInputIndices() const;
  virtual std::vector<uint32_t> GetCustomRestoreConstInputIndices() const;
  virtual ge::graphStatus RestoreConstInput(const KernelCollectContext &context, uint32_t input_index,
                                            CustomConstRestoreDescriptor &descriptor) const;
  virtual ge::graphStatus GenerateKernelCode(const std::string &function_symbol, std::string &code) const;
  virtual ge::graphStatus GenerateOutputsCreatorCode(std::string &code) const;
};

class KernelCodeBuilderFactory {
 public:
  using Creator = std::function<std::shared_ptr<KernelCodeBuilder>()>;

  KernelCodeBuilderFactory() = default;
  static KernelCodeBuilderFactory &GetInstance();

  ge::graphStatus Register(const char *kernel_type, Creator creator);
  std::shared_ptr<KernelCodeBuilder> Create(const char *kernel_type) const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, Creator> creators_;
};

class KernelCodeBuilderRegistrar {
 public:
  KernelCodeBuilderRegistrar(const char *kernel_type, KernelCodeBuilderFactory::Creator creator);
};

#define OM2_CODE_BUILDER_CONCAT_INNER(x, y) x##y
#define OM2_CODE_BUILDER_CONCAT(x, y) OM2_CODE_BUILDER_CONCAT_INNER(x, y)
#define REGISTER_KERNEL_CODE_BUILDER(kernel_type, builder_type)                                               \
  static const ::gert::om2::KernelCodeBuilderRegistrar OM2_CODE_BUILDER_CONCAT(g_om2_code_builder_registrar_, \
                                                                               __COUNTER__)(                  \
      kernel_type,                                                                                            \
      []() -> std::shared_ptr<::gert::om2::KernelCodeBuilder> { return std::make_shared<builder_type>(); })
}  // namespace om2
}  // namespace gert

#endif  // AIR_CXX_BASE_COMMON_OM2_DYNAMIC_CODEGEN_INFO_COLLECT_KERNEL_CODE_BUILDER_H_
