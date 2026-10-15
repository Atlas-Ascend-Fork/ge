/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "register/op_impl_kernel_registry.h"
#include "graph/error_codes.h"

struct TypesToImpl {
  const char *op_type;
  gert::OpImplKernelRegistry::OpImplFunctions funcs;
};
struct TypesToImplV2 {
  const char *op_type;
  gert::OpImplKernelRegistry::OpImplFunctionsV2 funcs;
};

namespace {
ge::graphStatus InferShape(gert::InferShapeContext *) {
  return ge::GRAPH_SUCCESS;
}
}  // namespace

__attribute__((constructor)) static void OpsDeliverablesFixtureInit(void) {
  const char *marker = std::getenv("GE_OPS_DELIVERABLES_MARKER");
  if (marker == nullptr) return;
  FILE *fp = std::fopen(marker, "a");
  if (fp != nullptr) {
    std::fputs("fixture\n", fp);
    std::fclose(fp);
  }
}
extern "C" __attribute__((visibility("default"))) size_t GetRegisteredOpNum(void) {
  return 1U;
}
extern "C" __attribute__((visibility("default"))) size_t GetRegisteredOpCtNum(void) {
  return 0U;
}
extern "C" __attribute__((visibility("default"))) int32_t GetOpImplFunctionsV2(TypesToImplV2 *impl, size_t impl_num) {
  if (impl == nullptr || impl_num < 1U) return -1;
  impl[0].op_type = "OpsDeliverablesFixture";
  impl[0].funcs.infer_shape = InferShape;
  return 0;
}
extern "C" __attribute__((visibility("default"))) int32_t GetOpImplFunctions(TypesToImpl *, size_t) {
  return 0;
}
