/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_BASE_COMMON_GE_INNER_ATTRS_H_
#define AIR_CXX_BASE_COMMON_GE_INNER_ATTRS_H_
#include "graph/types.h"

namespace ge {
constexpr char_t const *kAttrNameSingleOpType = "GE_INNER_SINGLE_OP_TYPE";
// profiling name
constexpr char_t const *kProfilingDeviceConfigData = "PROFILING_DEVICE_CONFIG_DATA";
constexpr char_t const *kProfilingIsExecuteOn = "PROFILING_IS_EXECUTE_ON";
// helper option
constexpr char_t const *kHostMasterPidName = "HOST_MASTER_PID";
constexpr char_t const *kExecutorDevId = "EXECUTOR_DEVICE_ID";
// runtime 2.0
constexpr char_t const *kRequestWatcher = "_request_watcher";
constexpr char_t const *kWatcherAddress = "_watcher_address";
constexpr char_t const *kSubgraphInput = "_subgraph_input";
constexpr char_t const *kSubgraphOutput = "_subgraph_output";
constexpr char_t const *kKnownSubgraph = "_known_subgraph";
constexpr char_t const *kRelativeBranch = "branch";
constexpr char_t const *kConditionGraph = "CondGraph";
constexpr char_t const *kThenGraph = "then_graph";
constexpr char_t const *kElseGraph = "else_graph";
// autofuse: 标记将被下沉 host 执行的节点，使其跳过设备侧 lowering/融合；由 AutoFusePass 打标并回收。
constexpr char_t const *kAttrAutofuseSkipLoweringForHostExec = "_autofuse_skip_lowering_for_host_exec";
// autofuse 融合/lowering 过程中的临时标记，生命周期与本次自动融合一致，结束时统一回收。
constexpr char_t const *kAttrAutofuseDisableScope = "_disable_autofuse_scope";
constexpr char_t const *kAttrAutofuseDisableLifting = "_disable_lifting";
}  // namespace ge
#endif  // AIR_CXX_BASE_COMMON_GE_INNER_ATTRS_H_
