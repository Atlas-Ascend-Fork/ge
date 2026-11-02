/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_RANGE_STATE_H_
#define GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_RANGE_STATE_H_

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

struct GertModelTaskDesc;

namespace ge {
namespace dump {

// ============================================================
//            opname_range 状态机（每个模型一份实例）
// ============================================================
// 对齐 v1 DavinciModel::ResetDumpFsmState/SetDumpFsmState 与
// DumpProperties::DumpProcFsmTransition/SetDumpFsmState：模型加载按 task 下发顺序
// 推进状态机，命中 [begin, end] 范围的 op 记入 dump_op_in_range_，dump 判定时与
// layer/watcher 配置取并集。状态为 per-model 运行态，由 ModelDumpManager 持有一份。
struct DumpOpRangeState {
  enum class DumpProcState : int32_t { kInit = 0, kStart, kStop, kError };
  enum class DumpProcEvent : int32_t { kRangeStart = 0, kRangeEnd, kOutOfRange };

  void Reset(const std::string &model_name, const std::string &root_graph_name);
  bool IsInDumpOpRange(const std::string &op_name) const;
  // 在状态机尚未推进到当前 task 时（如无 Preprocess 回调、launch 前才查询的 DSA 等 task），
  // 按当前状态预演该 op 的转移，判断其是否会被纳入 dump 范围，避免 launch 前判定早于状态迁移。
  bool IsOpInDumpRangeAtCurrentState(const std::string &op_name) const;
  // 按 (stream_id, op_name) 去重，保证每个 task 只推进一次状态机（不依赖 task 描述符地址，
  // 避免调用方复用同一描述符对象导致静默漏推进）。
  // AICore/自定义算子 task 先进 Preprocess 再进 Postprocess；DSA task 无 Preprocess，由 Postprocess 推进。
  void Update(const GertModelTaskDesc &task_info);

  static DumpProcState DumpProcFsmTransition(DumpProcState current, DumpProcEvent event);
  static DumpProcEvent GetOpProcEvent(const std::string &op_name, const std::pair<std::string, std::string> &range);

  std::vector<std::pair<std::string, std::string>> dump_op_ranges_;
  std::vector<DumpProcState> dump_fsm_state_;
  std::unordered_set<std::string> dump_op_in_range_;
  bool has_last_fsm_task_ = false;
  uint64_t last_fsm_stream_id_ = 0U;
  std::string last_fsm_op_name_;
};

}  // namespace dump
}  // namespace ge

#endif  // GE_FRAMEWORK_RUNTIME_DUMP_DUMP_OP_RANGE_STATE_H_
