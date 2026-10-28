/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "compiled_model.h"

#include <iomanip>
#include <memory>
#include <set>
#include <sstream>

#include <openssl/sha.h>

#include "common/checker.h"
#include "compiled_target_saver.h"
#include "common/scope_guard.h"
#include "graph/manager/graph_var_manager.h"
#include "model_buffer_helper.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_type_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/model.h"
#include "graph/buffer.h"
#include "graph/op_desc.h"
#include "graph/node.h"
#include "proto/ge_ir.pb.h"
#include "mobile_model.h"

namespace {

template <typename T>
auto MemSizeCeil(T size) -> T {
  constexpr uint32_t align_bytes = 512;
  return ((size + static_cast<T>(align_bytes - 1U)) / static_cast<T>(align_bytes)) * static_cast<T>(align_bytes);
}

// 变量区(权重)逻辑地址窗口判断: 除 Const/FileConstant 外, Bitcast 等重解释节点会复用权重输出地址
// (IsRefFromInput reuse), 其 output offset 落在变量区, 计入 memory_size 会把 128GB 基址拼进 featureMapSize
bool IsVarMemoryAddr(const int64_t addr) {
  return (static_cast<uint64_t>(addr) >= ge::kVarMemoryLogicBase) &&
         (static_cast<uint64_t>(addr) < (ge::kVarMemoryLogicBase + ge::kMemoryVarManagerMallocSize));
}

const std::string GRAPH_OP_TYPE = "GraphOp";

std::string CalcConstWeightSha256(const uint8_t *data, const size_t size) {
  if ((data == nullptr) || (size == 0U)) {
    return "";
  }
  unsigned char sha256[SHA256_DIGEST_LENGTH];
  (void)SHA256(reinterpret_cast<const unsigned char *>(data), size, sha256);
  constexpr int32_t hex_digits_per_byte = 2;  // 每字节输出2个十六进制字符
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (const auto &item : sha256) {
    oss << std::setw(hex_digits_per_byte) << static_cast<int32_t>(item);
  }
  return oss.str();
}

ge::Status SetGraphOpAttrs(ge::OpDescPtr &graph_op_desc, const std::vector<ge::BaseBuffer> &weights_list,
                           const std::vector<ge::CompiledTargetPtr> &compiled_targets, int64_t weight_partition_offset,
                           bool weight_partition_enabled, int64_t input_output_size, int64_t memory_size,
                           const std::string &subgraph_name) {
  // only support one graph op node dispatch to npucl
  // align attr to mobile
  const std::string attr_flush_output_cache_str = "attr_flush_output_cache";
  constexpr bool attr_flush_output_cache_value = false;
  (void)ge::AttrUtils::SetBool(graph_op_desc, attr_flush_output_cache_str, attr_flush_output_cache_value);

  const std::string attr_invalid_feature_map_cache_str = "attr_invalid_feature_map_cache";
  constexpr bool attr_invalid_feature_map_cache_value = false;
  (void)ge::AttrUtils::SetBool(graph_op_desc, attr_invalid_feature_map_cache_str, attr_invalid_feature_map_cache_value);

  const std::string attr_invalid_input_cache_str = "attr_invalid_input_cache";
  constexpr bool attr_invalid_input_cache_value = false;
  (void)ge::AttrUtils::SetBool(graph_op_desc, attr_invalid_input_cache_str, attr_invalid_input_cache_value);

  const std::string cl_name_str = "cl_name";
  const std::string cl_name_value = "NPUCL";
  (void)ge::AttrUtils::SetStr(graph_op_desc, cl_name_str, cl_name_value);

  // task info
  const std::string graphop_task_offset_str = "graphop_task_offset";
  constexpr int64_t graphop_task_offset_value = 0;
  (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_task_offset_str, graphop_task_offset_value);

  const std::string graphop_task_size_str = "graphop_task_size";
  GE_ASSERT_NOTNULL(compiled_targets[0], "[Mobile] compiled_targets[0] is nullptr.");
  const int64_t graphop_task_size_value = static_cast<int64_t>(compiled_targets[0]->GetSize());
  (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_task_size_str, graphop_task_size_value);

  // weight info
  const std::string graphop_weight_offset_str = "graphop_weight_offset";
  constexpr int64_t graphop_weight_offset_value = 0;
  (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_weight_offset_str, graphop_weight_offset_value);

  const std::string graphop_weight_size_str = "graphop_weight_size";
  const int64_t graphop_weight_size_value = static_cast<int64_t>(weights_list[0].GetSize());
  (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_weight_size_str, graphop_weight_size_value);

  // weight partition info, write only when partition enabled
  // no lora(variable size = 0): variable attrs not written, runtime derives constSize from specialShareOffset,
  // specialShareSize = total - partition, shared weight file name is a root graph level attr (weight_0.weight)
  // 权重全为共享时 weight_partition_offset 为 0(非共享段为空), 依赖显式的 weight_partition_enabled 判定
  if (weight_partition_enabled && (static_cast<std::size_t>(weight_partition_offset) < weights_list[0].GetSize())) {
    const std::string graphop_special_share_weight_offset_str = "graphop_special_share_weight_offset";
    (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_special_share_weight_offset_str, weight_partition_offset);
    const std::string graphop_special_share_weight_size_str = "graphop_special_share_weight_size";
    const int64_t graphop_special_share_weight_size_value = graphop_weight_size_value - weight_partition_offset;
    (void)ge::AttrUtils::SetInt(graph_op_desc, graphop_special_share_weight_size_str,
                                graphop_special_share_weight_size_value);
  }

  // in and out and fm info
  const std::string input_output_size_str = "input_output_size";
  (void)ge::AttrUtils::SetInt(graph_op_desc, input_output_size_str, input_output_size);

  const std::string memory_size_str = "memory_size";
  (void)ge::AttrUtils::SetInt(graph_op_desc, memory_size_str, memory_size);

  // sc info
  const std::string model_cache_offset_str = "model_cache_offset";
  constexpr int64_t model_cache_offset_value = 0;
  (void)ge::AttrUtils::SetInt(graph_op_desc, model_cache_offset_str, model_cache_offset_value);

  const std::string subgraph_name_str = "subgraph_name";
  (void)ge::AttrUtils::SetStr(graph_op_desc, subgraph_name_str, subgraph_name);
  return ge::SUCCESS;
}

ge::ComputeGraphPtr ConvertComputeGraphToMobile(const ge::ComputeGraphPtr &ori_compute_graph,
                                                const std::vector<ge::BaseBuffer> &weights_list,
                                                const std::vector<ge::CompiledTargetPtr> &compiled_targets,
                                                int64_t weight_partition_offset, bool weight_partition_enabled,
                                                const std::string &subgraph_name) {
  // copy subgraph from ori graph and set name
  ge::ComputeGraphPtr mobile_subgraph = std::make_shared<ge::ComputeGraph>(ori_compute_graph->GetName());
  if (mobile_subgraph == nullptr) {
    GELOGE(ge::FAILED, "[Mobile] mobile compute graph is nullptr.");
    return nullptr;
  }
  if (ge::GraphUtils::CopyComputeGraph(ori_compute_graph, mobile_subgraph) != ge::SUCCESS) {
    GELOGE(ge::FAILED, "[Mobile] copy compute graph failed.");
    return nullptr;
  }
  std::string main_graph_name = "ge_default";
  mobile_subgraph->SetName(main_graph_name + "/" + subgraph_name);

  // acquire all data and netoutput op desc in subgraph
  std::vector<ge::OpDescPtr> data_op_descs;
  ge::OpDescPtr netoutput_op_desc;
  // also get input and output size and fm size(exclude data and netoutput op)
  int64_t input_output_size = 0;
  // use set to avoid double-counting memory that is reused between DataNode outputs and NetOutput inputs
  std::set<std::pair<int64_t, int64_t>> counted_io_blocks;
  int64_t memory_size = 0;
  // also get weight info
  for (const ge::NodePtr &node : mobile_subgraph->GetNodes(mobile_subgraph->GetGraphUnknownFlag())) {
    GE_ASSERT_NOTNULL(node, "[Mobile] node is nullptr.");
    // const is weight not process
    // FileConstant is weight too: its output offset is a var-memory logic address (128GB base),
    // counting it into memory_size would corrupt the featuremap size, so skip it as well.
    if (ge::OpTypeUtils::IsConstNode(node->GetType()) || (node->GetType() == "FileConstant")) {
      GELOGI("[Mobile] skip const node name: %s, type: %s", node->GetName().c_str(), node->GetType().c_str());
      continue;
    }
    GE_ASSERT_NOTNULL(node->GetOpDesc(), "[Mobile] node->GetOpDesc() is nullptr.");
    // acquire op input meminfo
    std::vector<int64_t> input_offset = node->GetOpDesc()->GetInputOffset();
    std::vector<int64_t> input_data_size;
    for (size_t i = 0; i < input_offset.size(); i++) {
      int64_t size_of_byte = 0;
      (void)ge::TensorUtils::GetSize(node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(i)), size_of_byte);
      (void)input_data_size.emplace_back(size_of_byte);
    }
    // acquire op output meminfo
    std::vector<int64_t> output_offset = node->GetOpDesc()->GetOutputOffset();
    std::vector<int64_t> output_data_size;
    for (size_t i = 0; i < output_offset.size(); i++) {
      int64_t size_of_byte = 0;
      (void)ge::TensorUtils::GetSize(node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(i)), size_of_byte);
      (void)output_data_size.emplace_back(size_of_byte);
    }
    // debug info
    GELOGD("[Mobile] op name: %s, mem info: ", node->GetName().c_str());
    for (size_t i = 0; i < input_offset.size(); i++) {
      GELOGD("[Mobile]    input[%d] data size: %d , offset: %d", i, input_data_size[i], input_offset[i]);
    }
    for (size_t i = 0; i < output_offset.size(); i++) {
      GELOGD("[Mobile]    output[%d] data size: %d , offset: %d", i, output_data_size[i], output_offset[i]);
    }
    // acquire data and netoutput op desc and input output mem size
    if (ge::OpTypeUtils::IsDataNode(node->GetType())) {
      data_op_descs.push_back(node->GetOpDesc());
      // 与 fm 统计同步过滤变量区地址: 变量区 IO 块不占 featuremap 空间, 若计入 input_output_size,
      // memory_size(fm峰值) - input_output_size 会被多扣成负数
      if (IsVarMemoryAddr(output_offset[0])) {
        GELOGI("[Mobile] skip var memory addr in input_output_size, node: %s, output[0] offset: %ld",
               node->GetName().c_str(), output_offset[0]);
      } else {
        std::pair<int64_t, int64_t> block(output_offset[0], MemSizeCeil(output_data_size[0]));
        if (counted_io_blocks.find(block) == counted_io_blocks.end()) {
          input_output_size += MemSizeCeil(output_data_size[0]);
          counted_io_blocks.insert(block);
          GELOGD("[Mobile] output_data_size[0]: %d", output_data_size[0]);
        } else {
          GELOGD("[Mobile] DataNode output block(offset=%ld, size=%ld) already counted, skip duplicate",
                 output_offset[0], MemSizeCeil(output_data_size[0]));
        }
      }
    }
    if (ge::OpTypeUtils::IsGraphOutputNode(node->GetType())) {
      netoutput_op_desc = node->GetOpDesc();
      for (size_t i = 0; i < input_offset.size(); i++) {
        if (IsVarMemoryAddr(input_offset[i])) {
          GELOGI("[Mobile] skip var memory addr in input_output_size, node: %s, input[%d] offset: %ld",
                 node->GetName().c_str(), static_cast<int32_t>(i), input_offset[i]);
          continue;
        }
        std::pair<int64_t, int64_t> block(input_offset[i], MemSizeCeil(input_data_size[i]));
        if (counted_io_blocks.find(block) == counted_io_blocks.end()) {
          input_output_size += MemSizeCeil(input_data_size[i]);
          counted_io_blocks.insert(block);
          GELOGD("[Mobile] input_data_size: %d", input_data_size[i]);
        } else {
          GELOGD("[Mobile] NetOutput input block(offset=%ld, size=%ld) already counted, skip duplicate",
                 input_offset[i], MemSizeCeil(input_data_size[i]));
        }
      }
    }
    // acquire fm size
    if (ge::OpTypeUtils::IsGraphOutputNode(node->GetType())) {
      for (size_t i = 0; i < input_offset.size(); i++) {
        if (IsVarMemoryAddr(input_offset[i])) {
          GELOGI("[Mobile] skip var memory addr in fm size, node: %s, input[%d] offset: %ld", node->GetName().c_str(),
                 static_cast<int32_t>(i), input_offset[i]);
          continue;
        }
        const int64_t size_of_byte = input_offset[i] + MemSizeCeil(input_data_size[i]);
        memory_size = std::max(memory_size, size_of_byte);
      }
    } else {
      for (size_t i = 0; i < output_offset.size(); i++) {
        if (IsVarMemoryAddr(output_offset[i])) {
          GELOGI("[Mobile] skip var memory addr in fm size, node: %s, output[%d] offset: %ld", node->GetName().c_str(),
                 static_cast<int32_t>(i), output_offset[i]);
          continue;
        }
        const int64_t size_of_byte = output_offset[i] + MemSizeCeil(output_data_size[i]);
        memory_size = std::max(memory_size, size_of_byte);
      }
    }
  }
  memory_size -= input_output_size;
  GELOGI("[Mobile] final memory_size: %d, input_output_size: %d", memory_size, input_output_size);

  // add main graph
  ge::ComputeGraphPtr mobile_main_graph = std::make_shared<ge::ComputeGraph>(main_graph_name);
  if (mobile_main_graph == nullptr) {
    GELOGE(ge::FAILED, "[Mobile] mobile main subgraph is nullptr.");
    return nullptr;
  }

  // special share weight file name is a root graph level attr consumed by runtime (graph->HasAttr),
  // not a graphop node attr
  if (weight_partition_enabled && (static_cast<std::size_t>(weight_partition_offset) < weights_list[0].GetSize())) {
    const std::string special_share_weight_file_name_str = "special_share_weight_file_name";
    const std::string special_share_weight_file_name_value = "weight_0";
    (void)ge::AttrUtils::SetStr(*mobile_main_graph, special_share_weight_file_name_str,
                                special_share_weight_file_name_value);
  }

  // add graph op to main graph
  ge::OpDescPtr graph_op_desc = std::make_shared<ge::OpDesc>(subgraph_name, GRAPH_OP_TYPE);
  if (graph_op_desc == nullptr) {
    GELOGE(ge::FAILED, "[Mobile] graph op des is nullptr.");
    return nullptr;
  }
  for (const auto &data_op_desc : data_op_descs) {
    (void)graph_op_desc->AddInputDesc(data_op_desc->GetOutputDesc(0));
  }
  GE_ASSERT_NOTNULL(netoutput_op_desc, "netoutput op des is nullptr");
  for (uint32_t i = 0; i < netoutput_op_desc->GetInputsSize(); i++) {
    (void)graph_op_desc->AddOutputDesc(netoutput_op_desc->GetInputDesc(i));
    if (netoutput_op_desc->GetOutputsSize() < netoutput_op_desc->GetInputsSize()) {
      GELOGI("[Mobile] netoutput op output size is smaller than input size, use input desc as output desc");
      (void)netoutput_op_desc->AddOutputDesc(netoutput_op_desc->GetInputDesc(i));
    }
  }
  GELOGD("[Mobile] netoutput_op_desc input size: %d", netoutput_op_desc->GetInputsSize());
  GELOGD("[Mobile] netoutput_op_desc output size: %d", netoutput_op_desc->GetOutputsSize());
  const auto ret = SetGraphOpAttrs(graph_op_desc, weights_list, compiled_targets, weight_partition_offset,
                                   weight_partition_enabled, input_output_size, memory_size, subgraph_name);
  GE_ASSERT_TRUE(ret == ge::SUCCESS, "[Mobile] set graph op attr failed.");
  auto graph_op_node = mobile_main_graph->AddNode(graph_op_desc);
  (void)ge::NodeUtils::AddSubgraph(*graph_op_node, subgraph_name, mobile_subgraph);

  // add data and netoutput to main graph and link
  int32_t graph_op_input_idx = 0;
  for (const auto &data_op_desc : data_op_descs) {
    auto data_node = mobile_main_graph->AddNode(data_op_desc);
    if (data_node == nullptr) {
      GELOGE(ge::FAILED, "[Mobile] data node is nullptr.");
      return nullptr;
    }
    (void)ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), graph_op_node->GetInDataAnchor(graph_op_input_idx++));
  }
  auto netoutput_node = mobile_main_graph->AddNode(netoutput_op_desc);
  if (netoutput_node == nullptr) {
    GELOGE(ge::FAILED, "[Mobile] data node is nullptr.");
    return nullptr;
  }
  for (size_t i = 0; i < netoutput_op_desc->GetInputsSize(); i++) {
    (void)ge::GraphUtils::AddEdge(graph_op_node->GetOutDataAnchor(static_cast<int32_t>(i)),
                                  netoutput_node->GetInDataAnchor(static_cast<int32_t>(i)));
  }
  return mobile_main_graph;
}
}  // namespace

namespace ge {

Status CompiledModel::GetCompiledTargetsBuffer(std::vector<ge::BaseBuffer> &all_targets_buffer) {
  compiled_targets_buffer_.clear();
  CompiledTargetSaver saver;
  for (size_t i = 0; i < compiled_targets_.size(); i++) {
    GE_ASSERT_NOTNULL(compiled_targets_[i], "[Mobile] compiled_targets_[%d] is nullptr.", i);
    GE_ASSERT_TRUE(compiled_targets_[i]->UpdateModelModelTaskDef(mobile_model_def_) == ge::SUCCESS,
                   "[Mobile] update model task def failed.");
    const size_t task_size = compiled_targets_[i]->GetSize();
    GELOGI("[Mobile] task size: %u", task_size);
    compiled_targets_buffer_.emplace_back();
    compiled_targets_buffer_.back().resize(task_size);
    ge::BaseBuffer buffer(compiled_targets_buffer_.back().data(), task_size);
    const auto ret = saver.SaveToBuffer(compiled_targets_[i], buffer);
    GE_ASSERT_TRUE((ret == SUCCESS) && (buffer.GetData() != nullptr) && (buffer.GetSize() == task_size),
                   "[Mobile] compiled target[%d] save to buffer failed.", i);
    (void)all_targets_buffer.emplace_back(std::move(buffer));
  }
  return SUCCESS;
}

Status CompiledModel::SaveToBuffer(ge::BaseBuffer &buffer, bool save_weights_as_external_data,
                                   std::map<std::string, ge::BaseBuffer> *weights_list_external) {
  // 分区偏移来源: shared_weight_size_ (由 mobile_model_helper.cc 通过 SetSharedWeightSize 设置),
  // 是否分区由 weight_partition_enabled_ 显式标记(权重全为共享时偏移为 0, 不能以偏移大于 0 判定)
  const int64_t weight_partition_offset = static_cast<int64_t>(shared_weight_size_);

  // non-shared segment weight hash is used as subgraph name, so that weights with different
  // content map to different subgraph/weight file names
  // 非共享段在重排后位于权重文件头部 [0, partition), 共享段位于尾部 [partition, total)
  std::string subgraph_name = "SubGraph_0";
  if (!weights_list_.empty() && (weights_list_[0].GetData() != nullptr) && (weights_list_[0].GetSize() != 0U)) {
    const std::size_t weight_size = weights_list_[0].GetSize();
    auto *weight_data = weights_list_[0].GetData();
    std::size_t nonshared_segment_size = weight_size;
    const uint8_t *nonshared_data = weight_data;
    if (weight_partition_enabled_ && (static_cast<std::size_t>(weight_partition_offset) < weight_size)) {
      nonshared_segment_size = static_cast<std::size_t>(weight_partition_offset);
      nonshared_data = weight_data;
    }
    const std::string hash = CalcConstWeightSha256(nonshared_data, nonshared_segment_size);
    if (!hash.empty()) {
      subgraph_name = "SubGraph_" + hash;
    }
  }
  GELOGI("[Mobile] subgraph name: %s", subgraph_name.c_str());

  // convert computer graph and set attr
  ge_model_->SetGraph(ConvertComputeGraphToMobile(ge_model_->GetGraph(), weights_list_, compiled_targets_,
                                                  weight_partition_offset, weight_partition_enabled_, subgraph_name));

  // convert to mobile model def
  const auto model = std::make_unique<ge::Model>(ge_model_->GetName(), ge_model_->GetPlatformVersion());
  GE_ASSERT_NOTNULL(model, "[Mobile] model is nullptr");
  model->SetGraph(ge_model_->GetGraph());
  model->SetVersion(ge_model_->GetVersion());
  model->SetAttr(ge_model_->MutableAttrMap());
  ge::proto::ModelDef model_def;
  GE_ASSERT_TRUE((model->Save(model_def) == ge::SUCCESS), "[Mobile] save to model def failed.");
  GE_ASSERT_TRUE(ge::MobileModel::ConvertToMobileModelDef(model_def, mobile_model_def_) == ge::SUCCESS,
                 "[Mobile] convert to mobile model def failed.");

  // compiled target buffer
  std::vector<ge::BaseBuffer> all_targets_buffer;
  auto ret = GetCompiledTargetsBuffer(all_targets_buffer);
  GE_ASSERT_TRUE(ret == SUCCESS, "[Mobile] get all targets buffer failed.");

  // save weight external
  if (save_weights_as_external_data && (weights_list_external != nullptr)) {
    // only support graph op num == 1
    GE_ASSERT_TRUE(weights_list_.size() == 1, "[Mobile] weight list num %zu is not supported (only 1 is supported).",
                   weights_list_.size());
    if ((weights_list_[0].GetData() != nullptr) && (weights_list_[0].GetSize() != 0U)) {
      const std::size_t weight_size = weights_list_[0].GetSize();
      auto *weight_data = weights_list_[0].GetData();
      if (weight_partition_enabled_ && (static_cast<std::size_t>(weight_partition_offset) < weight_size)) {
        // partition enabled: 非共享在前 [0, partition), 共享在后 [partition, total)
        // SubGraph_<hash>.weight = [0, partition) non-shared, weight_0.weight = [partition, total) shared
        // 权重全为共享时 partition 为 0: 非共享段为空, 不导出非共享权重文件(FileSaver 不支持空文件),
        // 仅导出全量共享段 weight_0.weight
        GELOGI("[Mobile] save weights with partition, partition offset: %ld, total size: %zu", weight_partition_offset,
               weight_size);
        if (weight_partition_offset > 0) {
          std::string nonshared_file_name = subgraph_name + std::string(".weight");
          (void)weights_list_external->insert(std::make_pair(
              nonshared_file_name, ge::BaseBuffer(weight_data, static_cast<std::size_t>(weight_partition_offset))));
        }
        std::string shared_file_name = std::string("weight_0") + std::string(".weight");
        (void)weights_list_external->insert(std::make_pair(
            shared_file_name, ge::BaseBuffer(weight_data + weight_partition_offset,
                                             weight_size - static_cast<std::size_t>(weight_partition_offset))));
      } else {
        // SubGraph_<hash>.weight
        GELOGI("[Mobile] save weights without partition, total size: %zu", weight_size);
        std::string weight_file_name = subgraph_name + std::string(".weight");
        (void)weights_list_external->insert(std::make_pair(weight_file_name, weights_list_[0]));
      }
      weights_list_.clear();
    }
  }

  // not support save weight info
  ge::BaseBuffer weights_info_buffer;
  (void)weights_info_buffer;

  // save to tlv buffer
  ret = saver_.SaveCompiledModelToBuffer(ge_model_, mobile_model_def_, weights_list_, all_targets_buffer,
                                         weights_info_buffer, buffer);
  GE_ASSERT_TRUE(ret == SUCCESS, "[Mobile] save compiled model to buffer failed.");
  return SUCCESS;
}

}  // namespace ge
