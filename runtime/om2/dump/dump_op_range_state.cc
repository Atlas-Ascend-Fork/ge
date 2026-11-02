/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/runtime/dump/dump_op_range_state.h"
#include "framework/common/debug/ge_log.h"
#include "framework/om2/model_api/om2_model_api.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "framework/runtime/dump/dump_config.h"
#include <array>

namespace ge {
namespace dump {

namespace {
constexpr size_t kDumpProcStateNum = 4U;
constexpr size_t kDumpProcEventNum = 3U;
}  // namespace

void DumpOpRangeState::Reset(const std::string &model_name, const std::string &root_graph_name) {
  dump_fsm_state_.clear();
  dump_op_in_range_.clear();
  has_last_fsm_task_ = false;
  last_fsm_stream_id_ = 0U;
  last_fsm_op_name_.clear();

  dump_op_ranges_ = DumpConfig::Instance().GetOpDumpRanges(model_name, root_graph_name);
  if (!dump_op_ranges_.empty()) {
    GELOGI("Model[%s] root graph[%s] opname range size[%zu].", model_name.c_str(), root_graph_name.c_str(),
           dump_op_ranges_.size());
    dump_fsm_state_.resize(dump_op_ranges_.size(), DumpProcState::kInit);
  }
}

bool DumpOpRangeState::IsInDumpOpRange(const std::string &op_name) const {
  return dump_op_in_range_.count(op_name) == 1U;
}

bool DumpOpRangeState::IsOpInDumpRangeAtCurrentState(const std::string &op_name) const {
  if (dump_fsm_state_.size() != dump_op_ranges_.size()) {
    return false;
  }
  for (size_t index = 0U; index < dump_fsm_state_.size(); ++index) {
    const DumpProcState cur_state = dump_fsm_state_[index];
    if (cur_state == DumpProcState::kError) {  // kError 之后该范围不会再纳入新的 op
      continue;
    }
    const DumpProcEvent event = GetOpProcEvent(op_name, dump_op_ranges_[index]);
    const DumpProcState next_state = DumpProcFsmTransition(cur_state, event);
    if ((next_state == DumpProcState::kStart) || (next_state == DumpProcState::kStop)) {
      return true;
    }
  }
  return false;
}

DumpOpRangeState::DumpProcState DumpOpRangeState::DumpProcFsmTransition(DumpProcState current, DumpProcEvent event) {
  // 状态转移表，术语与 v1 DumpProperties::DumpProcFsmTransition 保持一致
  static constexpr std::array<std::array<DumpProcState, kDumpProcEventNum>, kDumpProcStateNum> kFsmTable = {
      {/* kInit  */
       {DumpProcState::kStart, DumpProcState::kError, DumpProcState::kInit},
       /* kStart */
       {DumpProcState::kStart, DumpProcState::kStop, DumpProcState::kStart},
       /* kStop  */
       {DumpProcState::kError, DumpProcState::kStop, DumpProcState::kInit},
       /* kError */
       {DumpProcState::kError, DumpProcState::kError, DumpProcState::kError}}};
  const size_t state = static_cast<size_t>(current);
  const size_t evt = static_cast<size_t>(event);
  if ((state >= kDumpProcStateNum) || (evt >= kDumpProcEventNum)) {
    return DumpProcState::kError;
  }
  return kFsmTable[state][evt];
}

DumpOpRangeState::DumpProcEvent DumpOpRangeState::GetOpProcEvent(const std::string &op_name,
                                                                 const std::pair<std::string, std::string> &range) {
  if (op_name == range.first) {
    return DumpProcEvent::kRangeStart;
  } else if (op_name == range.second) {
    return DumpProcEvent::kRangeEnd;
  }
  return DumpProcEvent::kOutOfRange;
}

void DumpOpRangeState::Update(const GertModelTaskDesc &task_info) {
  // 同一个 task 在 Preprocess / Postprocess 各触发一次，按 (stream_id, op_name) 去重，
  // 保证状态机每个 task 只推进一次；不使用描述符地址判重，避免调用方复用同一描述符对象导致静默漏推进。
  const std::string op_name = (task_info.op_name != nullptr) ? task_info.op_name : "";
  if (dump_fsm_state_.empty() ||
      (has_last_fsm_task_ && (task_info.stream_id == last_fsm_stream_id_) && (op_name == last_fsm_op_name_))) {
    return;
  }
  has_last_fsm_task_ = true;
  last_fsm_stream_id_ = task_info.stream_id;
  last_fsm_op_name_ = op_name;

  bool is_update_dump_op_range = true;
  if (static_cast<ModelTaskType>(task_info.task_type) == ModelTaskType::MODEL_TASK_FFTS_PLUS) {
    is_update_dump_op_range = false;
    GELOGW("op[%s] task type[%d] no support dump with opname range", op_name.c_str(),
           static_cast<int32_t>(task_info.task_type));
  }

  for (size_t index = 0U; index < dump_op_ranges_.size(); ++index) {
    const DumpProcEvent event = GetOpProcEvent(op_name, dump_op_ranges_[index]);
    const DumpProcState cur_state = dump_fsm_state_[index];
    if (cur_state == DumpProcState::kError) {  // kError 之后 need dump op 的范围不会再增加
      continue;
    }
    DumpProcState next_state = DumpProcFsmTransition(cur_state, event);
    if ((next_state == DumpProcState::kStart) || (next_state == DumpProcState::kStop)) {
      GELOGD("op[%s] cur state[%d] event[%d] next state[%d] opname range id[%zu] need dump[%d].", op_name.c_str(),
             static_cast<int32_t>(cur_state), static_cast<int32_t>(event), static_cast<int32_t>(next_state), index,
             static_cast<int32_t>(is_update_dump_op_range));
      if (is_update_dump_op_range) {
        (void)dump_op_in_range_.insert(op_name);
      }
    } else if (next_state == DumpProcState::kError) {
      GELOGW("opname range[%s, %s] range id[%zu] is invalid.", dump_op_ranges_[index].first.c_str(),
             dump_op_ranges_[index].second.c_str(), index);
    }

    if ((next_state == DumpProcState::kStart) && (dump_op_ranges_[index].first == dump_op_ranges_[index].second)) {
      next_state = DumpProcState::kStop;
      GELOGD("op[%s] range begin[%s] is same with range end[%s], opname range id[%zu].", op_name.c_str(),
             dump_op_ranges_[index].first.c_str(), dump_op_ranges_[index].second.c_str(), index);
    }
    dump_fsm_state_[index] = next_state;
  }
}

}  // namespace dump
}  // namespace ge
