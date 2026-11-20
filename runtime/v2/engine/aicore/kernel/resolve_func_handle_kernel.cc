/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "resolve_func_handle_kernel.h"
#include "acl/acl_rt.h"
#include "common/checker.h"
#include "framework/common/debug/log.h"
#include "graph/error_codes.h"
#include "register/kernel_registry.h"

namespace gert {
namespace kernel {
namespace {
enum class ResolveInput { kBinHandle = 0, kTilingKey, kKernelName, kWithHandleFlag, kNum };
enum class ResolveOutput { kFuncHandle = 0, kNum };

ge::graphStatus GetFuncHandleByEntry(const aclrtBinHandle bin_handle, const uint64_t tiling_key,
                                     aclrtFuncHandle &func_handle) {
  aclError ret = aclrtBinaryGetFunctionByEntry(bin_handle, tiling_key, &func_handle);
  if (ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "Get func handle by tiling key [%lu] failed, ret %d.", tiling_key, ret);
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetFuncHandleByName(const aclrtBinHandle bin_handle, const char *kernel_name,
                                    aclrtFuncHandle &func_handle) {
  aclError ret = aclrtBinaryGetFunction(bin_handle, kernel_name, &func_handle);
  if (ret != ACL_SUCCESS) {
    GELOGE(ge::FAILED, "Get func handle by kernel name [%s] failed, ret %d.", kernel_name, ret);
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus ResolveKernelFuncHandle(KernelContext *context) {
  auto bin_handle = context->GetInputValue<void *>(static_cast<int32_t>(ResolveInput::kBinHandle));
  GE_ASSERT_NOTNULL(bin_handle);
  const uint64_t *tiling_key_ptr = context->GetInputPointer<uint64_t>(static_cast<int32_t>(ResolveInput::kTilingKey));
  GE_ASSERT_NOTNULL(tiling_key_ptr);
  const uint64_t tiling_key = *tiling_key_ptr;
  auto kernel_name = context->GetInputValue<char *>(static_cast<int32_t>(ResolveInput::kKernelName));
  GE_ASSERT_NOTNULL(kernel_name);
  const uint32_t with_handle_flag =
      context->GetInputValue<uint32_t>(static_cast<int32_t>(ResolveInput::kWithHandleFlag));

  aclrtFuncHandle func_handle = nullptr;
  if (with_handle_flag != 0U) {
    if (GetFuncHandleByEntry(bin_handle, tiling_key, func_handle) != ge::GRAPH_SUCCESS) {
      return ge::GRAPH_FAILED;
    }
  } else {
    if (GetFuncHandleByName(bin_handle, kernel_name, func_handle) != ge::GRAPH_SUCCESS) {
      return ge::GRAPH_FAILED;
    }
  }
  GE_ASSERT_NOTNULL(func_handle, "Failed to get func handle.");

  auto output = context->GetOutputPointer<void *>(static_cast<int32_t>(ResolveOutput::kFuncHandle));
  GE_ASSERT_NOTNULL(output);
  *output = func_handle;
  return ge::GRAPH_SUCCESS;
}

REGISTER_KERNEL(ResolveKernelFuncHandle).RunFunc(ResolveKernelFuncHandle);
}  // namespace kernel
}  // namespace gert
