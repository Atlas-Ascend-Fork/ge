/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_RUNTIME_OM2_OM2_HCCL_HOST_SERVICES_H_
#define GE_RUNTIME_OM2_OM2_HCCL_HOST_SERVICES_H_

#include <cstdint>
#include "framework/om2/model_api/om2_model_api.h"

namespace gert {
int32_t HcclHostLoadTask(void *instance_handle, const HcclTaskParams *params);
int32_t HcclHostUnloadTask(void *instance_handle);
}  // namespace gert

#endif  // GE_RUNTIME_OM2_OM2_HCCL_HOST_SERVICES_H_
