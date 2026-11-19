/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/task_info/rts/stream_active_task_info.h"
#include "depends/ascendcl/src/ascendcl_stub.h"

namespace ge {
namespace {
class AclForStreamActiveTest final : public AclRuntimeStub {
 public:
  aclError aclrtActiveStream(aclrtStream activeStream, aclrtStream stream) override {
    active_calls.push_back(std::make_pair(activeStream, stream));
    if ((fail_on != nullptr) && (activeStream == fail_on)) {
      return ACL_ERROR_RT_FAILURE;
    }
    return ACL_SUCCESS;
  }

  std::vector<std::pair<aclrtStream, aclrtStream>> active_calls;
  aclrtStream fail_on{nullptr};
};

OpDescPtr MakeStreamActiveOp(const std::string &name, const std::vector<int64_t> &active_list) {
  auto op_desc = std::make_shared<OpDesc>(name, STREAMACTIVE);
  if (!active_list.empty()) {
    AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_list);
  }
  op_desc->SetId(2);
  return op_desc;
}

void SetUpModelWithActiveOp(DavinciModel &model, const OpDescPtr &op_desc) {
  model.runtime_param_.mem_size = 0x40000;
  if (model.reusable_stream_allocator_ == nullptr) {
    model.reusable_stream_allocator_ = ReusableStreamAllocator::Create();
  }
  rtStream_t stream = nullptr;
  model.reusable_stream_allocator_->GetOrCreateRtStream(stream, 0, 0, 0);
  model.stream_list_.push_back(stream);
  op_desc->SetStreamId(0);
  model.op_list_[op_desc->GetId()] = op_desc;
}

void InitStreamActiveTask(StreamActiveTaskInfo &task_info, DavinciModel &model, const OpDescPtr &op_desc) {
  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::StreamActiveDef *stream_active_def = task_def.mutable_stream_active();
  stream_active_def->set_op_index(op_desc->GetId());
  EXPECT_EQ(task_info.Init(task_def, &model, {}, {}, {}), SUCCESS);
}

// 循环激活 task（op 名含 IteratorCtrl_StreamSwitch_StreamActive）需把执行期登记的 attach 流
// 逐个以同一载体流追加激活，保证循环重放（第二遍迭代起）attach 流上任务每遍恰好执行一次
TEST(UtestStreamActiveTaskInfo, loop_active_task_appends_registered_streams) {
  auto acl = std::make_shared<AclForStreamActiveTest>();
  AclRuntimeStub::SetInstance(acl);

  DavinciModel model(0, nullptr);
  // 实际命名形如 <pre>_IteratorCtrl_StreamSwitch_StreamActive（flow_ctrl_pass 生成）
  auto op_desc = MakeStreamActiveOp("Node_Output_IteratorCtrl_StreamSwitch_StreamActive", {0});
  SetUpModelWithActiveOp(model, op_desc);

  const rtStream_t attached_a = reinterpret_cast<rtStream_t>(0x9001U);
  const rtStream_t attached_b = reinterpret_cast<rtStream_t>(0x9002U);
  model.PushLoopActiveStream(attached_a);
  model.PushLoopActiveStream(attached_b);

  StreamActiveTaskInfo task_info;
  InitStreamActiveTask(task_info, model, op_desc);
  domi::GetContext().is_online_model = true;
  EXPECT_EQ(task_info.Distribute(), SUCCESS);
  domi::GetContext().is_online_model = false;

  const auto carrier = model.stream_list_[0];
  ASSERT_EQ(acl->active_calls.size(), 3U);
  // 首个调用是编译期目标流（与载体同流），其后按登记顺序追加 attach 流，载体流一致
  EXPECT_EQ(acl->active_calls[0].first, carrier);
  EXPECT_EQ(acl->active_calls[0].second, carrier);
  EXPECT_EQ(acl->active_calls[1].first, attached_a);
  EXPECT_EQ(acl->active_calls[1].second, carrier);
  EXPECT_EQ(acl->active_calls[2].first, attached_b);
  EXPECT_EQ(acl->active_calls[2].second, carrier);

  AclRuntimeStub::Reset();
}

// CAS 防重：循环激活 op 的多个 task 中仅首个 Distribute 录制追加 SQE，其余 task 只下发自身激活
TEST(UtestStreamActiveTaskInfo, claim_is_one_shot_across_tasks_of_same_op) {
  auto acl = std::make_shared<AclForStreamActiveTest>();
  AclRuntimeStub::SetInstance(acl);

  DavinciModel model(0, nullptr);
  auto op_desc = MakeStreamActiveOp("pre_IteratorCtrl_StreamSwitch_StreamActive", {0, 0});
  SetUpModelWithActiveOp(model, op_desc);

  const rtStream_t attached = reinterpret_cast<rtStream_t>(0x9001U);
  model.PushLoopActiveStream(attached);

  StreamActiveTaskInfo first_task;
  StreamActiveTaskInfo second_task;
  InitStreamActiveTask(first_task, model, op_desc);
  InitStreamActiveTask(second_task, model, op_desc);
  domi::GetContext().is_online_model = true;
  EXPECT_EQ(first_task.Distribute(), SUCCESS);
  EXPECT_EQ(second_task.Distribute(), SUCCESS);
  domi::GetContext().is_online_model = false;

  // 每个 task 各自 1 次基础激活，追加激活仅首个 task 录制 1 次
  ASSERT_EQ(acl->active_calls.size(), 3U);
  EXPECT_EQ(acl->active_calls[1].first, attached);
  EXPECT_EQ(acl->active_calls[2].second, model.stream_list_[0]);

  AclRuntimeStub::Reset();
}

// 名字甄别：退出激活（<switch>_StreamExitActive，不含目标子串）与普通 label 激活不追加；
// 循环激活 op 在登记表为空时同样不产生额外调用
TEST(UtestStreamActiveTaskInfo, non_loop_active_names_skip_append) {
  auto acl = std::make_shared<AclForStreamActiveTest>();
  AclRuntimeStub::SetInstance(acl);

  DavinciModel model(0, nullptr);
  auto exit_active_op = MakeStreamActiveOp("pre_IteratorCtrl_StreamSwitch_StreamExitActive", {0});
  auto label_active_op = MakeStreamActiveOp("Less_StreamActive", {0});
  auto loop_active_op = MakeStreamActiveOp("pre_IteratorCtrl_StreamSwitch_StreamActive", {0});
  label_active_op->SetId(3);
  loop_active_op->SetId(4);
  SetUpModelWithActiveOp(model, exit_active_op);
  SetUpModelWithActiveOp(model, label_active_op);
  SetUpModelWithActiveOp(model, loop_active_op);

  model.PushLoopActiveStream(reinterpret_cast<rtStream_t>(0x9001U));

  StreamActiveTaskInfo exit_task;
  StreamActiveTaskInfo label_task;
  StreamActiveTaskInfo loop_task;
  InitStreamActiveTask(exit_task, model, exit_active_op);
  InitStreamActiveTask(label_task, model, label_active_op);
  InitStreamActiveTask(loop_task, model, loop_active_op);
  domi::GetContext().is_online_model = true;
  EXPECT_EQ(exit_task.Distribute(), SUCCESS);
  EXPECT_EQ(label_task.Distribute(), SUCCESS);
  EXPECT_EQ(loop_task.Distribute(), SUCCESS);
  domi::GetContext().is_online_model = false;

  // exit/label 各 1 次基础激活，均不追加登记流；loop_task 1 次基础 + 1 次追加登记流
  ASSERT_EQ(acl->active_calls.size(), 4U);
  EXPECT_NE(acl->active_calls[0].first, reinterpret_cast<rtStream_t>(0x9001U));
  EXPECT_NE(acl->active_calls[1].first, reinterpret_cast<rtStream_t>(0x9001U));
  EXPECT_EQ(acl->active_calls[2].first, model.stream_list_[0]);
  EXPECT_EQ(acl->active_calls[3].first, reinterpret_cast<rtStream_t>(0x9001U));
  EXPECT_EQ(acl->active_calls[3].second, model.stream_list_[0]);

  AclRuntimeStub::Reset();
}

// 追加激活失败需让 Distribute 整体失败，阻止半激活状态的模型加载成功
TEST(UtestStreamActiveTaskInfo, append_failure_fails_distribute) {
  auto acl = std::make_shared<AclForStreamActiveTest>();
  AclRuntimeStub::SetInstance(acl);

  DavinciModel model(0, nullptr);
  auto op_desc = MakeStreamActiveOp("pre_IteratorCtrl_StreamSwitch_StreamActive", {0});
  SetUpModelWithActiveOp(model, op_desc);

  const rtStream_t attached = reinterpret_cast<rtStream_t>(0x9001U);
  model.PushLoopActiveStream(attached);
  acl->fail_on = attached;

  StreamActiveTaskInfo task_info;
  InitStreamActiveTask(task_info, model, op_desc);
  domi::GetContext().is_online_model = true;
  EXPECT_NE(task_info.Distribute(), SUCCESS);
  domi::GetContext().is_online_model = false;

  // 基础激活成功后，追加登记流失败
  ASSERT_EQ(acl->active_calls.size(), 2U);
  EXPECT_EQ(acl->active_calls[1].first, attached);

  AclRuntimeStub::Reset();
}
}  // namespace
}  // namespace ge
