/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_COMMON_HELPER_MODEL_SAVE_UTILS_H_
#define GE_COMMON_HELPER_MODEL_SAVE_UTILS_H_

#include <map>
#include <string>
#include <unordered_set>

#include "common/model/ge_model.h"
#include "common/model/ge_root_model.h"
#include "common/op_so_store/op_so_store.h"

namespace ge {
GE_FUNC_VISIBILITY Status InitFirstGeModel(const GeRootModelPtr &ge_root_model,
                                           const std::map<std::string, GeModelPtr> &name_to_ge_model,
                                           GeModelPtr &first_ge_model);
GE_FUNC_VISIBILITY Status GetOppVersion(std::string &version);
GE_FUNC_VISIBILITY Status LoadOppSoBinaries(const std::unordered_set<std::string> &op_so_set, SoBinType so_bin_type,
                                            OpSoStore &so_store);
GE_FUNC_VISIBILITY Status CollectSpaceRegistrySoBins(const std::string &cpu_info, const std::string &os_info,
                                                     OpSoStore &so_store,
                                                     std::unordered_set<std::string> &compiler_versions);
}  // namespace ge

#endif  // GE_COMMON_HELPER_MODEL_SAVE_UTILS_H_
