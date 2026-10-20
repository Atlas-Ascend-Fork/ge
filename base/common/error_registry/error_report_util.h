/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_COMMON_ERROR_REGISTRY_ERROR_REPORT_UTIL_H_
#define GE_COMMON_ERROR_REGISTRY_ERROR_REPORT_UTIL_H_

#include <string>
#include <vector>

#include "base/err_msg.h"

namespace ge {
inline void ReportOpsKernelUnsupportedReason(const std::string &op_name, const std::string &op_type,
                                             const std::string &ops_kernel, const std::string &reason) {
  if (reason.empty()) {
    REPORT_INNER_ERR_MSG("E19999", "unsupported reason of op:%s(%s) from ops kernel:%s is empty, check invalid",
                         op_name.c_str(), op_type.c_str(), ops_kernel.c_str());
  } else {
    REPORT_PREDEFINED_ERR_MSG(
        "EZ3004", std::vector<const char *>({"opskernel", "opname", "optype", "reason"}),
        std::vector<const char *>({ops_kernel.c_str(), op_name.c_str(), op_type.c_str(), reason.c_str()}));
  }
}
}  // namespace ge

#endif  // GE_COMMON_ERROR_REGISTRY_ERROR_REPORT_UTIL_H_
