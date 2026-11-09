/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef COMMON_GRAPH_DEBUG_GE_LOG_H_
#define COMMON_GRAPH_DEBUG_GE_LOG_H_
#include "base/err_msg.h"
#include "common/ge_common/debug/ge_log.h"
#include "common/ge_common/string_util.h"
#include "graph_metadef/common/ge_common/util.h"

#define GE_REPORT_UNSUPPORTED_EXECUTE_OPERATION(operation)                                             \
  do {                                                                                                 \
    const char *const ge_unsupported_operation = (operation);                                          \
    GELOGE(::ge::GRAPH_FAILED, "[GertGraph][Unsupported] %s does not support execution graphs.",       \
           ge_unsupported_operation);                                                                  \
    REPORT_INNER_ERR_MSG("E18888", "%s does not support execution graphs.", ge_unsupported_operation); \
  } while (false)

#endif  // COMMON_GRAPH_DEBUG_GE_LOG_H_
