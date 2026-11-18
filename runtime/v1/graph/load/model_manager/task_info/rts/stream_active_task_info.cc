/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "graph/load/model_manager/task_info/rts/stream_active_task_info.h"

#include "graph/load/model_manager/davinci_model.h"

namespace ge {
Status StreamActiveTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model,
                                  const PisToArgs &args, const PisToPersistentWorkspace &persistent_workspace,
                                  const IowAddrs &iow_addrs) {
  GELOGI("StreamActiveTaskInfo Init Start.");
  (void)args;
  (void)persistent_workspace;
  (void)iow_addrs;
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  const auto &stream_active_def = task_def.stream_active();
  const uint32_t op_index = stream_active_def.op_index();

  const size_t internal_index = static_cast<size_t>(davinci_model_->GetFlowctrlIndex(op_index));

  // get StreamActive op
  const OpDescPtr op_desc = davinci_model_->GetOpByIndex(op_index);
  GE_CHECK_NOTNULL(op_desc);
  op_index_ = op_index;
  op_desc_ = op_desc;
  std::vector<uint32_t> active_stream_index_list;
  if (!AttrUtils::GetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_index_list)) {
    REPORT_INNER_ERR_MSG("E19999", "Get Attr:%s in op:%s(%s) fail", ATTR_NAME_ACTIVE_STREAM_LIST.c_str(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Get][Attr] %s in op:%s(%s) fail", ATTR_NAME_ACTIVE_STREAM_LIST.c_str(),
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return INTERNAL_ERROR;
  }

  if (internal_index >= active_stream_index_list.size()) {
    REPORT_INNER_ERR_MSG("E19999", "flowctrl index:%zu >= active_stream_list size:%zu in op:%s, check invalid",
                         internal_index, active_stream_index_list.size(), op_desc->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] stream id index invalid. index:%zu, list size:%zu, op:%s", internal_index,
           active_stream_index_list.size(), op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }

  if (active_stream_index_list[internal_index] >= davinci_model_->GetStreamList().size()) {
    REPORT_INNER_ERR_MSG("E19999", "active_stream_index:%u in op:%s(%s) >= stream size:%zu in model, check invalid",
                         active_stream_index_list[internal_index], op_desc->GetName().c_str(),
                         op_desc->GetType().c_str(), davinci_model_->GetStreamList().size());
    GELOGE(INTERNAL_ERROR, "[Check][Param] active_stream_index:%u in op:%s(%s) >= stream size:%zu in model",
           active_stream_index_list[internal_index], op_desc->GetName().c_str(), op_desc->GetType().c_str(),
           davinci_model_->GetStreamList().size());
    return INTERNAL_ERROR;
  }

  active_stream_ = davinci_model_->GetStreamList()[static_cast<size_t>(active_stream_index_list[internal_index])];
  active_stream_id_ = stream_active_def.active_stream_id();
  // 仅 IteratorCtrl 循环激活 op 追加执行期登记流；label 激活（同名 op 的其它 task）与
  // StreamExitActive（退出激活，名字不含本子串）语义不同，不追加
  is_loop_active_task_ = (op_desc->GetName().find("IteratorCtrl_StreamSwitch_StreamActive") != std::string::npos);

  GELOGI(
      "InitStreamActiveTaskInfo Init Success, index:%zu, active stream: %p, active stream id: %u, "
      "logic stream id: %u, stream: %p.",
      internal_index, active_stream_, active_stream_id_, task_def.stream_id(), stream_);

  return SUCCESS;
}

Status StreamActiveTaskInfo::Distribute() {
  GE_ASSERT_NOTNULL(op_desc_);
  GELOGI("StreamActiveTaskInfo op %s Distribute Start.", op_desc_->GetNamePtr());
  SetTaskTag(op_desc_->GetName().c_str());
  const auto rt_ret = aclrtActiveStream(active_stream_, stream_);
  if (rt_ret != ACL_SUCCESS) {
    REPORT_INNER_ERR_MSG("E19999", "Call aclrtActiveStream failed, ret:%d", rt_ret);
    GELOGE(RT_FAILED, "[Call][aclrtActiveStream] failed, ret:%d", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  if (is_loop_active_task_ && davinci_model_->TryClaimLoopActiveAppend()) {
    // 循环图重放（第二遍迭代起）时激活执行期（自定义算子）申请的 attach 流：
    // 与编译期目标流同载体（stream_），SQE 随载体流每遍重放，attach 流每遍恰好被激活一次
    // （CAS 防重：循环激活 op 的多个 task 中仅首个 Distribute 负责录制追加 SQE）
    for (const auto &appended_stream : davinci_model_->GetLoopActiveStreams()) {
      const auto append_ret = aclrtActiveStream(appended_stream, stream_);
      if (append_ret != ACL_SUCCESS) {
        REPORT_INNER_ERR_MSG("E19999", "Call aclrtActiveStream for appended stream failed, ret:%d", append_ret);
        GELOGE(RT_FAILED, "[Call][aclrtActiveStream] for appended stream failed, ret:%d", append_ret);
        return RT_ERROR_TO_GE_STATUS(append_ret);
      }
      GELOGI("Loop active task op %s also activates appended stream: %p on carrier stream: %p.", op_desc_->GetNamePtr(),
             appended_stream, stream_);
    }
  }

  if (!domi::GetContext().is_online_model) {
    op_desc_.reset();  // Release OpDesc after Distribute.
  }
  is_support_redistribute_ = true;
  GELOGI("StreamActiveTaskInfo Distribute Success, stream: %p.", stream_);
  return SUCCESS;
}

REGISTER_TASK_INFO(MODEL_TASK_STREAM_ACTIVE, StreamActiveTaskInfo);
}  // namespace ge
