/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "count_batch_flow_func.h"
#include "flow_func/flow_func_timer.h"
#include "common/udf_log.h"
#include "securec.h"
#include "flow_func/flow_func_dumper.h"

using namespace std;

namespace FlowFunc {
CountBatchFlowFunc::~CountBatchFlowFunc() {
  timer_flag_ = false;
  batch_flow_msg_.clear();
  if (timer_handle_ != nullptr) {
    (void)FlowFuncTimer::Instance().DeleteTimer(timer_handle_);
  }
}

int32_t CountBatchFlowFunc::Init() {
  auto err_code = GetBatchAttr();
  if (err_code != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]GetBatchAttr Failed. err_code=%d.", err_code);
    return err_code;
  }
  const auto timeout_proc = [this]() {
    std::unique_lock<std::mutex> lk(mutex_);
    uint64_t current_time = FlowFuncTimer::Instance().GetCurrentTimestamp();
    if (!timer_flag_ && (current_time - start_time_ < static_cast<uint64_t>(timeout_ * kMsToUsCast))) {
      UDF_LOG_DEBUG("[CountBatch]timeout_proc: Interval[%lu] is less than %ld, return.", current_time - start_time_,
                    timeout_);
      return;
    }
    std::shared_ptr<FlowMsg> output_flow_msg;
    int32_t ret = FLOW_FUNC_SUCCESS;
    for (size_t i = 0; i < batch_flow_msg_.size(); i++) {
      if (batch_flow_msg_[i].empty()) {
        UDF_LOG_DEBUG("[CountBatch]batch_flow_msg_[%zu] is null, no need to construct output.", i);
        timer_flag_ = false;
        return;
      }
      ret = PaddingInputCache(batch_flow_msg_[i].size());
      if (ret != FLOW_FUNC_SUCCESS) {
        AbnormalProc(ret);
        return;
      }
      UDF_LOG_DEBUG("[CountBatch]CountBatchTimeoutProc, batch_flow_msg_[%zu].size()=%zu", i, batch_flow_msg_[i].size());
      ret = ConstructOutputTensor(batch_flow_msg_[i], output_flow_msg);
      if (ret != FLOW_FUNC_SUCCESS) {
        AbnormalProc(ret);
        UDF_LOG_ERROR("[CountBatch]ConstructOutputTensor failed, ret=%u.", ret);
        return;
      }

      ret = context_->SetOutput(i, output_flow_msg);
      if (ret != FLOW_FUNC_SUCCESS) {
        UDF_LOG_ERROR("[CountBatch]SetOutput failed, ret=%u.", ret);
        AbnormalProc(ret);
        return;
      }
    }
    (void)FlowFuncTimer::Instance().StartTimer(timer_handle_, static_cast<uint32_t>(timeout_), true);
    start_time_ = FlowFuncTimer::Instance().GetCurrentTimestamp();
    return;
  };
  if (timeout_ != 0UL) {
    timer_handle_ = FlowFuncTimer::Instance().CreateTimer(timeout_proc);
  }
  batch_flow_msg_.clear();
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::PaddingInputCache(size_t cache_size) {
  if (static_cast<int64_t>(cache_size) < batch_size_) {
    if (padding_) {
      auto ret = PaddingToBatchSize((batch_size_ - static_cast<int64_t>(cache_size)));
      if (ret != FLOW_FUNC_SUCCESS) {
        UDF_LOG_ERROR("[CountBatch] padding failed, ret=%d", ret);
        return FLOW_FUNC_FAILED;
      }
    }
  }
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::PaddingToBatchSize(int64_t padding_cnt) {
  UDF_LOG_DEBUG("[CountBatch]PaddingToBatchSize, padding_cnt= %ld", padding_cnt);
  for (size_t i = 0; i < batch_flow_msg_.size(); ++i) {
    if ((batch_flow_msg_[i].empty()) || (batch_flow_msg_[i].front().first == nullptr) ||
        (batch_flow_msg_[i].front().first->GetTensor() == nullptr)) {
      UDF_LOG_ERROR("[CountBatch]PaddingToBatchSize: cache tensor is invalid, index:%zu.", i);
      return FLOW_FUNC_FAILED;
    }
    auto cache_tensor = batch_flow_msg_[i].front().first->GetTensor();
    for (int64_t j = 0; j < padding_cnt; ++j) {
      auto output_flow_msg = context_->AllocTensorMsg(cache_tensor->GetShape(), cache_tensor->GetDataType());
      if (output_flow_msg == nullptr) {
        UDF_LOG_ERROR("[CountBatch]PaddingToBatchSize:AllocTensorMsg failed.");
        return FLOW_FUNC_FAILED;
      }
      auto output_tensor = output_flow_msg->GetTensor();
      if (output_tensor == nullptr) {
        UDF_LOG_ERROR("[CountBatch]PaddingToBatchSize: output tensor is nullptr.");
        return FLOW_FUNC_FAILED;
      }
      auto data = output_tensor->GetData();
      auto data_size = output_tensor->GetDataSize();
      if ((data_size > 0U) && (data == nullptr)) {
        UDF_LOG_ERROR("[CountBatch]PaddingToBatchSize: output tensor data is nullptr.");
        return FLOW_FUNC_FAILED;
      }
      auto error = (data_size == 0U) ? EOK : memset_s(data, data_size, 0, data_size);
      if (error != EOK) {
        UDF_LOG_ERROR("[CountBatch]memset_s failed.");
        return FLOW_FUNC_FAILED;
      }
      batch_flow_msg_[i].push_back(std::make_pair(output_flow_msg, false));
    }
  }
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::GetBatchAttr() {
  auto get_ret = context_->GetAttr("batch_size", batch_size_);
  if (get_ret != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]Failed to get attr[batch_size].");
    return get_ret;
  }
  get_ret = context_->GetAttr("timeout", timeout_);
  if (get_ret != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]Failed to get attr[timeout].");
    return get_ret;
  }
  if ((timeout_ < 0L) || (timeout_ >= static_cast<int64_t>(UINT32_MAX))) {
    UDF_LOG_ERROR("[CountBatch]Attr[timeout] is invalid[%ld], valid range is[0, %u).", timeout_, UINT32_MAX);
    return FLOW_FUNC_ERR_PARAM_INVALID;
  }
  get_ret = context_->GetAttr("padding", padding_);
  if (get_ret != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]Failed to get attr[padding].");
    return get_ret;
  }
  get_ret = context_->GetAttr("slide_stride", slide_stride_);
  if (get_ret != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]Failed to get attr[slide_stride].");
    return get_ret;
  }
  UDF_LOG_DEBUG(
      "[CountBatch]GetBatchAttr success, batch_size_ = %ld, timeout_ = %ld, slide_stride_ = %ld, padding_ = %d.",
      batch_size_, timeout_, slide_stride_, padding_);
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::CheckTensorInfo(const std::vector<std::shared_ptr<FlowMsg>> &input_msgs) const {
  bool is_cache_empty = batch_flow_msg_.empty();
  for (size_t i = 0U; i < input_msgs.size(); ++i) {
    const auto input_tensor = input_msgs[i]->GetTensor();
    if (input_tensor == nullptr) {
      UDF_LOG_ERROR("[CountBatch]Input[%zu] tensor is nullptr.", i);
      return FLOW_FUNC_ERR_PARAM_INVALID;
    }
    if ((input_tensor->GetDataSize() > 0U) && (input_tensor->GetData() == nullptr)) {
      UDF_LOG_ERROR("[CountBatch]Input[%zu] tensor data is nullptr.", i);
      return FLOW_FUNC_ERR_PARAM_INVALID;
    }
    if (is_cache_empty) {
      UDF_LOG_DEBUG("batch_flow_msg_ is empty.");
      continue;
    }
    if (!batch_flow_msg_[i].empty()) {
      if ((batch_flow_msg_[i].front().first == nullptr) || (batch_flow_msg_[i].front().first->GetTensor() == nullptr)) {
        UDF_LOG_ERROR("[CountBatch]Input[%zu] cache tensor is nullptr.", i);
        return FLOW_FUNC_ERR_PARAM_INVALID;
      }
      auto cache_tensor = batch_flow_msg_[i].front().first->GetTensor();
      if (cache_tensor->GetShape() != input_tensor->GetShape()) {
        UDF_LOG_ERROR("[CountBatch]Input[%zu] shape is invalid for auto batch.", i);
        return FLOW_FUNC_ERR_PARAM_INVALID;
      }
      if (input_tensor->GetDataType() != cache_tensor->GetDataType()) {
        UDF_LOG_ERROR("[CountBatch]Input[%zu] data type[%d] is not equal to last input data type[%d].", i,
                      input_tensor->GetDataType(), cache_tensor->GetDataType());
        return FLOW_FUNC_ERR_PARAM_INVALID;
      }
    }
  }
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::CheckInput(const std::vector<std::shared_ptr<FlowMsg>> &input_msgs) const {
  if (input_msgs.size() < 1U) {
    UDF_LOG_ERROR("[CountBatch]Input msg size[%zu] must be at least 1.", input_msgs.size());
    return FLOW_FUNC_ERR_PARAM_INVALID;
  }
  for (size_t i = 0; i < input_msgs.size(); ++i) {
    if (input_msgs[i] == nullptr) {
      UDF_LOG_ERROR("[CountBatch]Input[%zu] msg is nullptr.", i);
      return FLOW_FUNC_ERR_PARAM_INVALID;
    }
    const auto ret_code = input_msgs[i]->GetRetCode();
    if (ret_code != 0) {
      UDF_LOG_ERROR("[CountBatch]Input[%zu] is invalid, error code[%d].", i, ret_code);
      return ret_code;
    }
  }
  if ((!batch_flow_msg_.empty()) && (input_msgs.size() != batch_flow_msg_.size())) {
    UDF_LOG_ERROR("[CountBatch]Input current input num is %zu, but first %zu times input num is %zu.",
                  input_msgs.size(), batch_flow_msg_[0].size(), batch_flow_msg_.size());
    return FLOW_FUNC_ERR_PARAM_INVALID;
  }
  return CheckTensorInfo(input_msgs);
}

void CountBatchFlowFunc::AbnormalProc(int32_t error_code) {
  // alloc size 1 output for error report
  auto error_output_msg = context_->AllocTensorMsg({1}, TensorDataType::DT_INT8);
  if (error_output_msg != nullptr) {
    error_output_msg->SetRetCode(error_code);
    for (uint32_t i = published_output_num_; i < total_output_num_; ++i) {
      (void)context_->SetOutput(i, error_output_msg);
    }
  }

  batch_flow_msg_.clear();
  timer_flag_ = false;
  UDF_LOG_DEBUG("[CountBatch]AbnormalProc finished.");
}

int32_t CountBatchFlowFunc::Proc(const std::vector<std::shared_ptr<FlowMsg>> &input_msgs) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (batch_flow_msg_.empty()) {
    total_output_num_ = input_msgs.size();
  }
  published_output_num_ = 0U;
  auto ret = CheckInput(input_msgs);
  if (ret != FLOW_FUNC_SUCCESS) {
    UDF_LOG_ERROR("[CountBatch]CheckInput failed, ret=%d.", ret);
    AbnormalProc(ret);
    return FLOW_FUNC_SUCCESS;
  }
  if (!timer_flag_) {
    timer_flag_ = true;
    batch_flow_msg_.resize(input_msgs.size());
    if (timeout_ != 0UL) {
      (void)FlowFuncTimer::Instance().StartTimer(timer_handle_, static_cast<uint32_t>(timeout_), true);
      start_time_ = FlowFuncTimer::Instance().GetCurrentTimestamp();
    }
  }
  std::shared_ptr<FlowMsg> output_flow_msg;
  for (size_t i = 0; i < input_msgs.size(); i++) {
    batch_flow_msg_[i].push_back(std::make_pair(input_msgs[i], true));
    UDF_LOG_DEBUG("[CountBatch]batch_flow_msg_[%zu].size()=%zu", i, batch_flow_msg_[i].size());
    if (batch_flow_msg_[i].size() >= static_cast<size_t>(batch_size_)) {
      ret = ConstructOutputTensor(batch_flow_msg_[i], output_flow_msg);
      if (ret != FLOW_FUNC_SUCCESS) {
        UDF_LOG_ERROR("[CountBatch]ConstructOutputTensor failed, ret=%u", ret);
        AbnormalProc(ret);
        return FLOW_FUNC_SUCCESS;
      }
      ret = context_->SetOutput(i, output_flow_msg);
      if (ret != FLOW_FUNC_SUCCESS) {
        UDF_LOG_ERROR("[CountBatch]SetOutput failed, ret=%u", ret);
        AbnormalProc(ret);
        return FLOW_FUNC_SUCCESS;
      }
      timer_flag_ = false;
    }
    published_output_num_++;
  }
  return FLOW_FUNC_SUCCESS;
}

int32_t CountBatchFlowFunc::ConstructOutputTensor(std::deque<std::pair<std::shared_ptr<FlowMsg>, bool>> &batch_tensor,
                                                  std::shared_ptr<FlowMsg> &output_flow_msg) const {
  UDF_LOG_DEBUG("[CountBatch]ConstructOutputTensor enter, batch_tensor.size = %ld", batch_tensor.size());
  if (batch_tensor.empty() || (batch_tensor.front().first == nullptr) ||
      (batch_tensor.front().first->GetTensor() == nullptr)) {
    UDF_LOG_ERROR("[CountBatch]batch tensor is empty.");
    return FLOW_FUNC_FAILED;
  }
  std::deque<std::pair<std::shared_ptr<FlowMsg>, bool>> temp_que(batch_tensor);
  auto input_tensor = batch_tensor.front().first->GetTensor();
  auto output_data_type = input_tensor->GetDataType();
  auto output_shape = input_tensor->GetShape();
  output_shape.insert(output_shape.cbegin(), batch_tensor.size());

  output_flow_msg = context_->AllocTensorMsg(output_shape, output_data_type);
  if (output_flow_msg == nullptr) {
    UDF_LOG_ERROR("[CountBatch]alloc tensor failed.");
    return FLOW_FUNC_FAILED;
  }
  auto output_tensor = output_flow_msg->GetTensor();
  if (output_tensor == nullptr) {
    UDF_LOG_ERROR("[CountBatch]output tensor is nullptr.");
    return FLOW_FUNC_FAILED;
  }
  if ((output_tensor->GetDataSize() > 0U) && (output_tensor->GetData() == nullptr)) {
    UDF_LOG_ERROR("[CountBatch]output tensor data is nullptr.");
    return FLOW_FUNC_FAILED;
  }
  auto data = output_tensor->GetData();
  auto data_size = output_tensor->GetDataSize();
  uint64_t used_size = 0;
  uint32_t max_step = 0;
  while (!batch_tensor.empty()) {
    if (batch_tensor.front().first == nullptr) {
      UDF_LOG_ERROR("[CountBatch]batch tensor is invalid.");
      return FLOW_FUNC_FAILED;
    }
    auto temp_tensor = batch_tensor.front().first->GetTensor();
    const auto step_msg = std::dynamic_pointer_cast<MbufFlowMsg>(batch_tensor.front().first);
    if ((temp_tensor == nullptr) || (step_msg == nullptr)) {
      UDF_LOG_ERROR("[CountBatch]batch tensor is invalid.");
      return FLOW_FUNC_FAILED;
    }
    const auto step = step_msg->GetStepId();
    max_step = ((FlowFuncDumpManager::IsInDumpStep(step)) && (step > max_step)) ? step : max_step;
    const auto copy_size = temp_tensor->GetDataSize();
    if (((copy_size > 0U) && (temp_tensor->GetData() == nullptr)) || (copy_size > data_size) ||
        (used_size > (data_size - copy_size))) {
      UDF_LOG_ERROR("[CountBatch]used_size[%lu] add copy_size[%lu] is larger than data_size[%lu].", used_size,
                    copy_size, data_size);
      return FLOW_FUNC_FAILED;
    }
    errno_t ret = EOK;
    if (copy_size > 0U) {
      ret = memcpy_s(data, data_size - used_size, temp_tensor->GetData(), copy_size);
    }
    if (ret != EOK) {
      UDF_LOG_ERROR("[CountBatch]memcpy_s failed.");
      return FLOW_FUNC_FAILED;
    }
    data = static_cast<void *>(static_cast<uint8_t *>(data) + temp_tensor->GetDataSize());
    used_size += temp_tensor->GetDataSize();
    batch_tensor.pop_front();
    if (used_size > data_size) {
      UDF_LOG_ERROR("[CountBatch]used_size[%lu] is larger than data_size[%lu]", used_size, data_size);
      return FLOW_FUNC_FAILED;
    }
  }
  const auto output_mbuf = std::dynamic_pointer_cast<MbufFlowMsg>(output_flow_msg);
  if (output_mbuf == nullptr) {
    UDF_LOG_ERROR("[CountBatch]output msg type is invalid.");
    return FLOW_FUNC_FAILED;
  }
  output_mbuf->SetStepId(max_step);

  if (slide_stride_ != 0) {
    int64_t min_size =
        slide_stride_ > static_cast<int64_t>(temp_que.size()) ? static_cast<int64_t>(temp_que.size()) : slide_stride_;
    for (int64_t i = 0; i < min_size; i++) {
      temp_que.pop_front();
    }
    // skip padding data in cache vector.
    while (!temp_que.empty()) {
      if (temp_que.back().second) {
        break;
      }
      temp_que.pop_back();
    }
    batch_tensor = temp_que;
  }
  return FLOW_FUNC_SUCCESS;
}

REGISTER_FLOW_FUNC("_BuiltIn_CountBatch", CountBatchFlowFunc);
}  // namespace FlowFunc
