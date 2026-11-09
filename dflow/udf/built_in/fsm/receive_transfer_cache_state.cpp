/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "fsm/receive_transfer_cache_state.h"
#include <cstdint>
#include <mutex>
#include "common/data_utils.h"
#include "fsm/probe_state.h"
#include "entity/llm_comm_entity_mgr.h"
#include "llm_common/cache_manager.h"
#include "llm_common/hccl_proxy.h"

namespace FlowFunc {

FsmStatus ReceiveTransferCacheState::Preprocess(LlmCommEntity &entity) {
  return Process(entity);
}

FsmStatus ReceiveTransferCacheState::RecvAsync(LlmCommEntity &entity, uint64_t real_slot_nums,
                                               std::vector<void *> &buffs_vec, std::vector<int32_t> &counts_vec) {
  LlmCommEntity::RecvTransferKvRecordInfo &record_info = entity.GetRecvTransferKvRecordInfo();
  auto &transfer_index = record_info.recv_call_count;
  auto &probe_msgs = entity.GetProbeMsgs();
  auto &receive_reqs = entity.GetReceiveRequests();
  HcclResult hccl_ret;
  HcclRequest request;
  const auto buffs = buffs_vec.data();
  const auto counts = counts_vec.data();
  if (real_slot_nums == 1U) {
    hccl_ret = HcclRawImrecv(buffs[0], counts[0], HCCL_DATA_TYPE_INT8, &probe_msgs.at(transfer_index), &request);
  } else {
    hccl_ret = HcclRawImrecvScatter(buffs, counts, static_cast<int32_t>(real_slot_nums), HCCL_DATA_TYPE_INT8,
                                    &probe_msgs.at(transfer_index), &request);
  }
  entity.GetStatisticInfo().call_recv_total_times++;
  if (hccl_ret == HCCL_E_AGAIN) {
    UDF_RUN_LOG_INFO("Single kv transfer need try again, ret:%d.", hccl_ret);
    int32_t current_comp_count = 0;
    auto ret = entity.TestCompleteAsync(receive_reqs.data(), receive_reqs.size(), current_comp_count);
    if (ret != FsmStatus::kFsmSuccess) {
      UDF_LOG_ERROR("Fail to call TestCompleteAsync, ret:%d, entity:%s.", ret, entity.GetDesc().c_str());
      return ret;
    }
    record_info.recv_suc_count += current_comp_count;
    return FsmStatus::kFsmKeepState;
  } else if (hccl_ret != HCCL_SUCCESS) {
    entity.GetStatisticInfo().call_recv_fail_times++;
    UDF_LOG_ERROR("Fail to receive kv, ret:%d, entity:%s.", hccl_ret, entity.GetDesc().c_str());
    return FsmStatus::kFsmHcclFailed;
  }
  receive_reqs.emplace_back(request);
  entity.GetStatisticInfo().call_recv_success_times++;
  return FsmStatus::kFsmSuccess;
}

FsmStatus ReceiveTransferCacheState::RecvKv(LlmCommEntity &entity) {
  auto *transfer_req_info = static_cast<TransferToRemoteReq *>(entity.GetTransferKvAddrInfo().transfer_kv_req_addr);
  if ((transfer_req_info == nullptr) || (transfer_req_info->send_nums_per_tensor == 0U)) {
    UDF_LOG_ERROR("transfer req is invalid, entity:%s.", entity.GetDesc().c_str());
    return FsmStatus::kFsmParamInvalid;
  }
  if ((transfer_req_info->tensor_num_per_layer != 0U) &&
      (transfer_req_info->send_nums_per_tensor > (UINT64_MAX / transfer_req_info->tensor_num_per_layer))) {
    UDF_LOG_ERROR("transfer count overflow, entity:%s.", entity.GetDesc().c_str());
    return FsmStatus::kFsmParamInvalid;
  }
  LlmCommEntity::RecvTransferKvRecordInfo &record_info = entity.GetRecvTransferKvRecordInfo();
  auto &transfer_index = record_info.recv_call_count;
  auto send_nums_per_layer = transfer_req_info->tensor_num_per_layer * transfer_req_info->send_nums_per_tensor;
  auto &slot_index = record_info.recv_slot_index;
  auto &count = record_info.last_probed_count;
  auto &dst_addrs = entity.GetPushDstAddrs();
  while (transfer_index < send_nums_per_layer) {
    // if last probed msg not receive complete, cannot need to probe again.
    if (count == -1) {
      auto probe_ret = ProbeState::ProbeAndGetCountAsync(entity, count);
      if (probe_ret != FsmStatus::kFsmSuccess) {
        return probe_ret;
      }
      UDF_LOG_INFO("Probed count:%d", count);
    }
    if (slot_index >= transfer_req_info->total_slot_nums) {
      UDF_LOG_ERROR("slot index:%lu is over total:%lu, entity:%s.", slot_index, transfer_req_info->total_slot_nums,
                    entity.GetDesc().c_str());
      return FsmStatus::kFsmParamInvalid;
    }
    auto local_slot_index = slot_index;
    auto &buffer_info = transfer_req_info->slot_infos[local_slot_index];
    if ((buffer_info.slot_nums_per_transfer == 0U) ||
        (buffer_info.slot_nums_per_transfer > static_cast<uint64_t>(UINT32_MAX)) ||
        (buffer_info.slot_nums_per_transfer > (transfer_req_info->total_slot_nums - slot_index))) {
      UDF_LOG_ERROR("slot_nums_per_transfer:%lu is invalid, entity:%s.", buffer_info.slot_nums_per_transfer,
                    entity.GetDesc().c_str());
      return FsmStatus::kFsmParamInvalid;
    }
    std::vector<void *> buffs_vec(buffer_info.slot_nums_per_transfer);
    std::vector<int32_t> counts_vec(buffer_info.slot_nums_per_transfer);
    int32_t expected_count = 0;
    auto tensor_idx = static_cast<uint64_t>(transfer_index) / transfer_req_info->send_nums_per_tensor;
    if (tensor_idx >= dst_addrs.size()) {
      UDF_LOG_ERROR("tensor index:%lu is over dst addr num:%zu, entity:%s.", tensor_idx, dst_addrs.size(),
                    entity.GetDesc().c_str());
      return FsmStatus::kFsmParamInvalid;
    }
    auto base_addr = dst_addrs[tensor_idx];
    for (uint32_t i = 0; i < buffer_info.slot_nums_per_transfer; ++i) {
      const auto &slot_info = transfer_req_info->slot_infos[local_slot_index];
      if ((slot_info.slot_size > static_cast<uint64_t>(INT32_MAX)) ||
          CheckAddOverflowUint64(base_addr, slot_info.slot_offset) ||
          (expected_count > (INT32_MAX - static_cast<int32_t>(slot_info.slot_size)))) {
        UDF_LOG_ERROR("slot size:%lu or offset:%lu is invalid, entity:%s.", slot_info.slot_size, slot_info.slot_offset,
                      entity.GetDesc().c_str());
        return FsmStatus::kFsmParamInvalid;
      }
      buffs_vec[i] = reinterpret_cast<void *>(base_addr + slot_info.slot_offset);
      counts_vec[i] = static_cast<int32_t>(slot_info.slot_size);
      expected_count += counts_vec[i];
      local_slot_index++;
    }
    if (count != expected_count) {
      UDF_LOG_ERROR("Param invalid, hccl get count ret:%d, expected:%lu", count, expected_count);
      return FsmStatus::kFsmParamInvalid;
    }
    auto ret = RecvAsync(entity, buffer_info.slot_nums_per_transfer, buffs_vec, counts_vec);
    if (ret != FsmStatus::kFsmSuccess) {
      return ret;
    }
    // receive complete.
    transfer_index++;
    UDF_LOG_INFO("Receive num:%lu", transfer_index);
    slot_index += buffer_info.slot_nums_per_transfer;
    if (slot_index == transfer_req_info->total_slot_nums) {
      UDF_LOG_INFO("Reset slot_index for next transfer.");
      slot_index = 0U;
    }
    count = -1;
  }
  if (transfer_index >= send_nums_per_layer) {
    UDF_LOG_INFO("Already completed count:%lu", record_info.recv_suc_count);
    auto &receive_reqs = entity.GetReceiveRequests();
    while (record_info.recv_suc_count < send_nums_per_layer) {
      int32_t current_comp_count = 0;
      auto ret = entity.TestCompleteAsync(receive_reqs.data(), receive_reqs.size(), current_comp_count);
      if (ret != FsmStatus::kFsmSuccess) {
        UDF_LOG_ERROR("Fail to call TestCompleteAsync, ret:%d, entity:%s.", ret, entity.GetDesc().c_str());
        return ret;
      }
      record_info.recv_suc_count += current_comp_count;
    }
  }
  return FsmStatus::kFsmSuccess;
}

FsmStatus ReceiveTransferCacheState::Process(LlmCommEntity &entity) {
  auto ret = RecvKv(entity);
  if (ret != FsmStatus::kFsmSuccess) {
    return ret;
  }
  return Postprocess(entity);
}

FsmStatus ReceiveTransferCacheState::Postprocess(LlmCommEntity &entity) {
  entity.GetReceiveRequests().clear();
  entity.GetProbeMsgs().clear();
  EntityStatisticInfo &statInfo = entity.GetStatisticInfo();
  const uint64_t current_tick_cost =
      StatisticManager::GetInstance().GetCpuTick() - entity.GetServerTickRecord().probe_req_start_tick;
  bool max_tick_cost_flag = false;
  UpdateTickCost(current_tick_cost, statInfo.recv_transfer_kv_total_times, statInfo.recv_transfer_kv_min_tick_cost,
                 statInfo.recv_transfer_kv_max_tick_cost, statInfo.recv_transfer_kv_total_tick_cost,
                 max_tick_cost_flag);
  if (max_tick_cost_flag) {
    entity.SetForcePrintFlag(true);
  }
  const double timeCost = StatisticManager::GetInstance().GetTimeCost(current_tick_cost);
  UDF_LOG_INFO("Finish receive transfer state, time cost:%.2f us, entity:%s.", timeCost, entity.GetDesc().c_str());
  return entity.ChangeState(FsmState::kFsmIdleState);
}
}  // namespace FlowFunc
