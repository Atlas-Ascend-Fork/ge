/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/common/helper/mobile_model_helper.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <fstream>
#include <regex>
#include <set>
#include <climits>
#include <cstdlib>
#include <cstring>

#include "common/checker.h"
#include "common/model/ge_model.h"
#include "common/model/ge_root_model.h"
#include "common/op_so_store/op_so_store_utils.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "graph_metadef/common/plugin/plugin_manager.h"
#include "framework/omg/version.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/unfold/graph_unfolder.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "framework/omg/omg_inner_types.h"
#include "mmpa/mmpa_api.h"
#include "common/proto_util/proto_util.h"
#include "graph/ge_local_context.h"
#include "graph_metadef/graph/utils/file_utils.h"
#include "graph/utils/op_type_utils.h"
#include "graph/build/memory/var_mem_assign_util.h"
#include "graph/manager/graph_var_manager.h"
#include "common/math/ge_math_util.h"
#include "common/file_constant_utils/file_constant_utils.h"
#include "common/helper/file_saver.h"
#include "common/model/model_introduction.h"
#include "common/model/model_compress_manager.h"
#include "common/host_resource_center/host_resource_serializer.h"
#include "graph/utils/math_util.h"
#include "ge_context.h"
#include "proto/task.pb.h"
#include "graph/compute_graph.h"
#include "graph/node.h"
#include "base/err_msg.h"
#include "graph_metadef/graph/debug/ge_util.h"

#include "mobile/base_buffer.h"
#include "mobile/compiled_model.h"
#include "mobile/model_file_saver.h"
#include "mobile/compiled_target.h"
#include "mobile/compatible_info_v2.h"
#include "mobile/kernelbin_info.h"
#include "proto/ge_ir_mobile.pb.h"
#include "proto/task_mobile.pb.h"

namespace {

constexpr uint64_t PER_U64_BYTES = 8;
constexpr int32_t RT_MODEL_TASK_KERNEL = 0;
constexpr int32_t ASCENDC_AI_CORE = 6;
constexpr int32_t PER_ARG_BYTES = 8;
constexpr uint32_t MOBILE_RT_DEV_BINARY_MAGIC_ELF = 0x43554245U;

// 严格匹配：node_name 必须是 stub_func 的后缀，且前一个字符为 '_'（组件分隔符）或字符串开头。
// stub_func 格式: task_id_stream_id_event_id_node_name，node_name 是最后一个组件，可含 '_'。
// 后缀匹配避免 "Add" 误匹配 "Add_1"（"Add_1" 不以 "Add" 结尾）。
bool StubFuncMatchNodeName(const std::string &stub_func, const std::string &node_name) {
  if (stub_func.size() < node_name.size()) {
    return false;
  }
  const size_t start = stub_func.size() - node_name.size();
  if (stub_func.compare(start, node_name.size(), node_name) != 0) {
    return false;
  }
  return (start == 0U) || (stub_func[start - 1U] == '_');
}

class ShapeInfo {
 public:
  int64_t dim_num;
  std::vector<int64_t> dims;
  int64_t offset;
  std::string ToString() const {
    std::string msg = "";
    msg += "dim_num: " + std::to_string(dim_num) + ", shape: ";
    for (const auto &d : dims) {
      msg += std::to_string(d) + " ";
    }
    msg += ", offset: " + std::to_string(offset);
    return msg;
  }
};

class DynamicInputsOutputsShapeInfo {
 public:
  std::vector<std::vector<ShapeInfo>> &dynamic_inputs_shape;
  std::vector<std::vector<ShapeInfo>> &dynamic_outputs_shape;

  const std::vector<ShapeInfo> &GetShapeInfo(const std::string &io_type, uint64_t group_index) const {
    static std::vector<ShapeInfo> dynamic_shape_empty;
    if ((io_type == "i" && static_cast<size_t>(group_index) >= dynamic_inputs_shape.size()) ||
        (io_type == "o" && static_cast<size_t>(group_index) >= dynamic_outputs_shape.size())) {
      GELOGE(ge::FAILED, "[Mobile] group index: %d , io_type: %s, in size: %d, out size: %d, is invalid, return.",
             group_index, io_type.c_str(), dynamic_inputs_shape.size(), dynamic_outputs_shape.size());
      return dynamic_shape_empty;
    }
    if (io_type == "i") {
      return dynamic_inputs_shape[group_index];
    } else if (io_type == "o") {
      return dynamic_outputs_shape[group_index];
    } else {
      GELOGE(ge::FAILED, "[Mobile] io_type is not supported");
    }
    return dynamic_shape_empty;
  }
};

// 变量区地址(FileConstant output offset)判断: 与 RemapVarAddrArgsToWeightAddr 共用同一合法性窗口
bool IsVarMemoryAddr(const uint64_t addr) {
  return (addr >= ge::kVarMemoryLogicBase) && (addr < (ge::kVarMemoryLogicBase + ge::kMemoryVarManagerMallocSize));
}

uint64_t ConstructSecondaryAddrContext(std::vector<uint8_t> &flowtable, const std::vector<ShapeInfo> &shape_infos,
                                       const std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                                       const uint64_t weight_addr) {
  // cal real size
  // ptr_offset
  uint64_t real_size = PER_U64_BYTES;
  // shape info(dim + shape)
  for (const auto &shape_info : shape_infos) {
    GE_ASSERT_TRUE(
        (static_cast<uint64_t>(shape_info.dim_num) <= UINT64_MAX / PER_U64_BYTES) &&
            (PER_U64_BYTES <= UINT64_MAX - (static_cast<uint64_t>(shape_info.dim_num) * PER_U64_BYTES)) &&
            (real_size <= UINT64_MAX - (PER_U64_BYTES + (static_cast<uint64_t>(shape_info.dim_num) * PER_U64_BYTES))),
        "[Mobile] overflow, failed.");
    real_size += PER_U64_BYTES + static_cast<uint64_t>(shape_info.dim_num) * PER_U64_BYTES;
  }
  // ptr
  real_size += static_cast<uint64_t>(shape_infos.size()) * PER_U64_BYTES;
  GELOGI("[Mobile] real size: %d", real_size);

  std::vector<uint64_t> tlv(real_size / PER_U64_BYTES);
  uint64_t tlv_index = 0;
  // ptr_offset
  tlv[tlv_index] = PER_U64_BYTES;
  for (const auto &shape_info : shape_infos) {
    if (static_cast<uint64_t>(shape_info.dim_num) > (UINT64_MAX / PER_U64_BYTES) ||
        (static_cast<uint64_t>(shape_info.dim_num) * PER_U64_BYTES) > (UINT64_MAX - PER_U64_BYTES)) {
      GELOGE(ge::FAILED, "[Mobile] cal overflow, failed.");
      return 0;
    }
    tlv[tlv_index] += PER_U64_BYTES + static_cast<uint64_t>(shape_info.dim_num) * PER_U64_BYTES;
  }
  GELOGI("[Mobile] tlv ptr offset: 0x%llx", tlv[tlv_index]);
  tlv_index++;
  GELOGI("[Mobile] tlv_index after fill ptr offset: %d", tlv_index);
  // shape info(dim + shape)
  for (size_t i = 0; i < shape_infos.size(); i++) {
    constexpr uint64_t shift_32_bits = 32;
    // dim
    tlv[tlv_index++] = static_cast<uint64_t>(i) << shift_32_bits | static_cast<uint64_t>(shape_infos[i].dim_num);
    // shape
    for (const auto &s : shape_infos[i].dims) {
      tlv[tlv_index++] = static_cast<uint64_t>(s);
    }
    GELOGI("[Mobile] tlv_index after shape index: %d , tlv_index: %d", i, tlv_index);
  }
  // ptr
  for (size_t i = 0; i < shape_infos.size(); i++) {
    uint64_t offset = static_cast<uint64_t>(shape_infos[i].offset);
    // flowtable TLV ptr 槽中的变量区绝对地址设备侧无法刷新(与 kernel args 同理),
    // 外置权重合并模式(var_addr_to_file_offset 非空)下重映射为 权重区基址+文件偏移;
    // 查不到映射直接报错, 把运行期地址校验失败前置到编译期;
    // map 为空(非外置权重模式或无 FileConstant)时保持原行为, 直接写原始变量区地址
    if (!var_addr_to_file_offset.empty() && IsVarMemoryAddr(offset)) {
      const auto it = var_addr_to_file_offset.find(offset);
      GE_ASSERT_TRUE(it != var_addr_to_file_offset.end(),
                     "[Mobile] flowtable ptr slot addr 0x%llx is in var memory region but has no fileconstant mapping.",
                     offset);
      offset = weight_addr + it->second;
      GELOGI("[Mobile] flowtable ptr slot remap var addr 0x%llx to weight addr 0x%llx, file offset: %llu.",
             static_cast<uint64_t>(shape_infos[i].offset), offset, it->second);
    }
    // add 2G
    tlv[tlv_index++] = static_cast<uint64_t>(0x80000000U) | offset;
    GELOGI("[Mobile] tlv_index after ptr index: %d , tlv_index: %d", i, tlv_index);
  }
  const uint8_t *tlv_data = reinterpret_cast<uint8_t *>(tlv.data());
  (void)flowtable.insert(flowtable.end(), tlv_data, &tlv_data[real_size]);
  return real_size;
}

ge::Status GetDynamicInputsInfo(const ge::NodePtr &node, std::vector<int64_t> &input_offset,
                                std::vector<std::vector<ShapeInfo>> &dynamic_inputs_shape) {
  std::vector<std::vector<int64_t>> dynamic_inputs_indexes;
  const std::string dynamic_inputs_indexes_str = "_dynamic_inputs_indexes";
  (void)ge::AttrUtils::GetListListInt(node->GetOpDesc(), dynamic_inputs_indexes_str, dynamic_inputs_indexes);
  input_offset = node->GetOpDesc()->GetInputOffset();
  for (const auto &i_offset : input_offset) {
    GELOGI("[Mobile] input offset: %d ", i_offset);
  }
  GELOGI("[Mobile] dynamic inputs group count: %d ", dynamic_inputs_indexes.size());
  for (size_t i = 0; i < dynamic_inputs_indexes.size(); i++) {
    GELOGI("[Mobile] dynamic inputs group: %d", i);
    for (const auto &d : dynamic_inputs_indexes[i]) {
      GELOGI("[Mobile] input indexes: %d ", d);
    }
    std::vector<ShapeInfo> shape_infos;
    for (const auto &d : dynamic_inputs_indexes[i]) {
      if (static_cast<size_t>(d) >= node->GetOpDesc()->GetInputsSize() ||
          static_cast<size_t>(d) >= input_offset.size()) {
        GELOGE(ge::FAILED, "[Mobile] dynamic inputs index: %d is invalid, return", d);
        return ge::FAILED;
      }
      ShapeInfo shape_info;
      shape_info.dim_num =
          static_cast<int64_t>(node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(d)).GetShape().GetDimNum());
      shape_info.dims = node->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(d)).GetShape().GetDims();
      shape_info.offset = input_offset[static_cast<uint64_t>(d)];
      shape_infos.push_back(shape_info);
    }
    dynamic_inputs_shape.push_back(shape_infos);
  }
  for (size_t i = 0; i < dynamic_inputs_shape.size(); i++) {
    GELOGI("[Mobile] dynamic inputs group: %d", i);
    for (const auto &s : dynamic_inputs_shape[i]) {
      GELOGI("[Mobile] %s", s.ToString().c_str());
    }
  }
  return ge::SUCCESS;
}

ge::Status GetDynamicOutputsInfo(const ge::NodePtr &node, std::vector<int64_t> &output_offset,
                                 std::vector<std::vector<ShapeInfo>> &dynamic_outputs_shape) {
  std::vector<std::vector<int64_t>> dynamic_outputs_indexes;
  const std::string dynamic_outputs_indexes_str = "_dynamic_outputs_indexes";
  (void)ge::AttrUtils::GetListListInt(node->GetOpDesc(), dynamic_outputs_indexes_str, dynamic_outputs_indexes);
  output_offset = node->GetOpDesc()->GetOutputOffset();
  for (const auto &o_offset : output_offset) {
    GELOGI("[Mobile] output offset: %d ", o_offset);
  }
  GELOGI("[Mobile] dynamic outputs group count: %d", dynamic_outputs_indexes.size());
  for (size_t i = 0; i < dynamic_outputs_indexes.size(); i++) {
    GELOGI("[Mobile] dynamic outputs group: %d", i);
    for (const auto &d : dynamic_outputs_indexes[i]) {
      GELOGI("[Mobile] output indexes: %d ", d);
    }
    std::vector<ShapeInfo> shape_infos;
    for (const auto &d : dynamic_outputs_indexes[i]) {
      if (static_cast<size_t>(d) >= node->GetOpDesc()->GetOutputsSize() ||
          static_cast<size_t>(d) >= output_offset.size()) {
        GELOGE(ge::FAILED, "[Mobile] dynamic outputs index: %d is invalid, return", d);
        return ge::FAILED;
      }
      ShapeInfo shape_info;
      shape_info.dim_num =
          static_cast<int64_t>(node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(d)).GetShape().GetDimNum());
      shape_info.dims = node->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(d)).GetShape().GetDims();
      shape_info.offset = output_offset[static_cast<uint64_t>(d)];
      shape_infos.push_back(shape_info);
    }
    dynamic_outputs_shape.push_back(shape_infos);
  }
  for (size_t i = 0; i < dynamic_outputs_shape.size(); i++) {
    GELOGI("[Mobile] dynamic outputs group: %d", i);
    for (const auto &s : dynamic_outputs_shape[i]) {
      GELOGI("[Mobile] %s", s.ToString().c_str());
    }
  }
  return ge::SUCCESS;
}

class KernelArgInfoWithShapeInfo {
 public:
  // 'i' is input
  // 'o' is output
  // 'ws' is workspace
  std::string type;
  // '*' is x1 x2 y1 y2 ...
  // ''  is [x1,x2,x3], [y1,y2,y3], ...
  // 'desc*' is x1 shape, x2 shape, x1 ptr, x2, ptr, y1 shape, y2 shape, y1 ptr, y2, ptr, ...
  // 'desc' is [x1 shape, x2 shape, x1 ptr, x2, ptr], [y1 shape, y2 shape, y1 ptr, y2, ptr], ...
  std::string format;
  int32_t group_index;
  std::set<int32_t> arg_indexes;
  int32_t add;

  KernelArgInfoWithShapeInfo(const std::string &type_in, const std::string &format_in, int32_t group_index_in,
                             int32_t add_in)
      : type(type_in), format(format_in), group_index(group_index_in), arg_indexes({}), add(add_in) {}

  std::string ToString() const {
    std::string msg = "type: " + type + ", format: " + format + ", add: " + std::to_string(add) +
                      ", group_index: " + std::to_string(group_index) + ", arg_indexes: ";
    for (const auto &i : arg_indexes) {
      msg += std::to_string(i) + " ";
    }
    return msg;
  }
};

ge::Status ParseArgsFormat(const std::string &args_format,
                           std::vector<KernelArgInfoWithShapeInfo> &arg_infos_with_shape_info) {
  // custom_data_block 含 '.'，下面的 regex 不匹配，若不显式拒绝会被静默跳过并产出可错跑的 .omc
  if (args_format.find("custom_data_block") != std::string::npos) {
    GELOGE(ge::FAILED, "[Mobile] args format [%s] contains unsupported CUSTOM_DATA_BLOCK.", args_format.c_str());
    const std::string reason = "Variable length custom data args is not supported for mobile OMC.";
    REPORT_PREDEFINED_ERR_MSG("E10055", std::vector<const char_t *>({"reason"}),
                              std::vector<const char_t *>({reason.c_str()}));
    return ge::FAILED;
  }
  std::regex pattern("\\{[a-z0-9_\\*]+\\}");
  std::smatch sm;
  auto s_start = args_format.cbegin();
  int32_t input_group_index = 0;
  int32_t output_group_index = 0;
  int32_t add = 0;
  while (std::regex_search(s_start, args_format.cend(), sm, pattern)) {
    if (sm.size() != 1) {
      GELOGW("[Mobile] sm size is not 1, break.");
      break;
    }
    std::string arg_format = std::string(sm[0]);
    if (arg_format.empty()) {
      GELOGW("[Mobile] arg format is empty, break.");
      break;
    }
    GELOGI("[Mobile] arg_format string is: %s", arg_format.c_str());
    if (arg_format.find("desc") != std::string::npos) {
      std::regex pattern_with_shape_info("([a-z])_desc([0-9]+)(\\*?)");
      std::smatch sm_with_shape_info;
      (void)std::regex_search(arg_format, sm_with_shape_info, pattern_with_shape_info);
      constexpr size_t shape_info_size_needed = 4;
      GE_ASSERT_TRUE(sm_with_shape_info.size() == shape_info_size_needed, "[Mobile] shape info size need > 4.");
      for (size_t idx = 0; idx < shape_info_size_needed; idx++) {
        GELOGI("[Mobile] -> [%d]: %s", idx, sm_with_shape_info[idx].str().c_str());
      }
      GE_ASSERT_TRUE(sm_with_shape_info[shape_info_size_needed - 1UL] != "*",
                     "[Mobile] desc with '*' is not supported");
      int32_t group_index = -1;
      if (sm_with_shape_info[1] == "i") {
        group_index = input_group_index++;
      } else if (sm_with_shape_info[1] == "o") {
        group_index = output_group_index++;
      } else {
        GELOGE(ge::FAILED, "[Mobile] sm_with_shape_info[1] is not supported, failed.");
        return ge::FAILED;
      }
      arg_infos_with_shape_info.push_back(KernelArgInfoWithShapeInfo(sm_with_shape_info[1], "desc", group_index, add));
    } else {
      add++;
      GELOGI("[Mobile] arg without shape info, skip");
    }
    s_start = sm.suffix().first;
  }
  for (const auto &i : arg_infos_with_shape_info) {
    GELOGI("[Mobile] %s", i.ToString().c_str());
  }
  return ge::SUCCESS;
}

ge::Status SetArgInfosArgsIndex(std::vector<KernelArgInfoWithShapeInfo> &arg_infos_with_shape_info,
                                DynamicInputsOutputsShapeInfo &dynamic_inputsoutputs_shapeinfo) {
  auto &dynamic_inputs_shape = dynamic_inputsoutputs_shapeinfo.dynamic_inputs_shape;
  auto &dynamic_outputs_shape = dynamic_inputsoutputs_shapeinfo.dynamic_outputs_shape;

  int32_t args_index = 0;
  for (auto &arg_info : arg_infos_with_shape_info) {
    int32_t group_index = arg_info.group_index;
    std::string io_type = arg_info.type;
    std::vector<ShapeInfo> *dynamic_inout_shape = nullptr;
    if (io_type == "i") {
      GE_ASSERT_TRUE(static_cast<size_t>(group_index) < dynamic_inputs_shape.size(),
                     "[Mobile] group_index is invalid, group_index: %d, dynamic_inputs_shape.size: %d", group_index,
                     dynamic_inputs_shape.size());
      dynamic_inout_shape = &(dynamic_inputs_shape[static_cast<size_t>(group_index)]);
    } else if (io_type == "o") {
      GE_ASSERT_TRUE(static_cast<size_t>(group_index) < dynamic_outputs_shape.size(),
                     "[Mobile] group_index is invalid, group_index: %d, dynamic_outputs_shape.size: %d", group_index,
                     dynamic_outputs_shape.size());
      dynamic_inout_shape = &(dynamic_outputs_shape[static_cast<size_t>(group_index)]);
    } else {
      GELOGE(ge::FAILED, "[Mobile] unknown io type: %s.", io_type.c_str());
      return ge::FAILED;
    }
    GE_ASSERT_NOTNULL(dynamic_inout_shape, "[Mobile] dynamic inout shape is nullptr.");
    size_t i = 0;
    while (i < dynamic_inout_shape->size()) {
      (void)arg_info.arg_indexes.insert(arg_info.add + args_index++);
      i++;
    }
  }
  return ge::SUCCESS;
}

ge::Status FindArgInfosWithShapeInfoIndex(int32_t i, std::vector<KernelArgInfoWithShapeInfo> &arg_infos_with_shape_info,
                                          int32_t &arg_infos_with_shape_info_index) {
  if (arg_infos_with_shape_info.size() > INT32_MAX) {
    GELOGE(ge::FAILED, "[Mobile] overflow, failed.");
    return ge::FAILED;
  }
  for (int32_t j = 0; j < static_cast<int32_t>(arg_infos_with_shape_info.size()); j++) {
    if (arg_infos_with_shape_info[static_cast<uint64_t>(j)].arg_indexes.find(i) !=
        arg_infos_with_shape_info[static_cast<uint64_t>(j)].arg_indexes.end()) {
      arg_infos_with_shape_info_index = j;
      break;
    }
  }
  return ge::SUCCESS;
}

ge::Status SetArgsAndFlowTable(const domi::KernelDef &kernel, ge::mobile::proto::KernelDef *mobile_kernel,
                               std::vector<KernelArgInfoWithShapeInfo> &arg_infos_with_shape_info,
                               DynamicInputsOutputsShapeInfo &dynamic_inputsoutputs_shapeinfo, bool &is_flowtable,
                               const std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                               const uint64_t weight_addr) {
  const uint64_t *args_data = reinterpret_cast<const uint64_t *>(kernel.args().data());
  std::vector<uint8_t> flowtable;
  std::vector<uint64_t> args(kernel.args_size() / static_cast<uint32_t>(PER_ARG_BYTES));
  int32_t args_index_new = 0;
  uint64_t offset = 0;
  int32_t group_index_pre = -1;
  std::string io_type_pre = "";
  for (uint32_t i = 0; i < kernel.args_size() / static_cast<uint32_t>(PER_ARG_BYTES); i++) {
    int32_t arg_infos_with_shape_info_index = -1;
    GE_ASSERT_TRUE(FindArgInfosWithShapeInfoIndex(static_cast<int32_t>(i), arg_infos_with_shape_info,
                                                  arg_infos_with_shape_info_index) == ge::SUCCESS,
                   "[Mobile] find arg infos with shape info index failed.");
    GELOGI("[Mobile] args index: %d, value: 0x%llx, arg_infos_with_shape_info_index: %d", i, *args_data,
           arg_infos_with_shape_info_index);
    if ((arg_infos_with_shape_info_index != -1) &&
        (arg_infos_with_shape_info[static_cast<uint64_t>(arg_infos_with_shape_info_index)].format == "desc")) {
      GELOGI("[Mobile] modify for secondary addr args.");
      GELOGI("[Mobile] arg_infos_with_shape_info: %s",
             arg_infos_with_shape_info[static_cast<uint64_t>(arg_infos_with_shape_info_index)].ToString().c_str());
      is_flowtable = true;
      const auto io_type = arg_infos_with_shape_info[static_cast<uint64_t>(arg_infos_with_shape_info_index)].type;
      const int32_t group_index =
          arg_infos_with_shape_info[static_cast<uint64_t>(arg_infos_with_shape_info_index)].group_index;
      if ((group_index_pre == group_index) && (io_type_pre == io_type)) {
        GELOGI("[Mobile] in same group, group_index_pre: %d, group_index: %d, io_type_pre: %s, io_type: %s.",
               group_index_pre, group_index, io_type_pre.c_str(), io_type.c_str());
        args_data++;
        continue;
      }
      const uint64_t real_size = ConstructSecondaryAddrContext(
          flowtable, dynamic_inputsoutputs_shapeinfo.GetShapeInfo(io_type, static_cast<uint64_t>(group_index)),
          var_addr_to_file_offset, weight_addr);
      GELOGI("[Mobile] real_size: %d", real_size);
      args[args_index_new++] = 0x6B6B6B6B00000000UL | offset;
      offset += real_size;
      group_index_pre = group_index;
      io_type_pre = io_type;
    } else {
      GELOGI("[Mobile] copy for direct addr args.");
      args[args_index_new++] = *args_data;
    }
    args_data++;
  }
  GELOGI("[Mobile] args_index_new: %d", args_index_new);
  mobile_kernel->set_flowtable(static_cast<const void *>(flowtable.data()), flowtable.size());
  mobile_kernel->set_args(static_cast<const void *>(args.data()),
                          static_cast<uint32_t>(args_index_new * PER_ARG_BYTES));
  mobile_kernel->set_args_size(static_cast<uint32_t>(args_index_new * PER_ARG_BYTES));
  return ge::SUCCESS;
}

ge::Status ConvertKernelDef(const ge::GeModelPtr &ge_model, const domi::KernelDef &kernel,
                            ge::mobile::proto::KernelDef *mobile_kernel, bool &is_flowtable,
                            const std::map<uint64_t, uint64_t> &var_addr_to_file_offset, const uint64_t weight_addr) {
  mobile_kernel->set_stub_func(kernel.stub_func());
  mobile_kernel->set_block_dim(kernel.block_dim());

  // node dynamic inputs outputs
  std::vector<std::vector<ShapeInfo>> dynamic_inputs_shape;
  std::vector<std::vector<ShapeInfo>> dynamic_outputs_shape;
  const auto &graph = ge_model->GetGraph();
  for (const ge::NodePtr &node : graph->GetNodes(graph->GetGraphUnknownFlag())) {
    const std::string &node_name = node->GetName();
    if (!StubFuncMatchNodeName(kernel.stub_func(), node_name)) {
      continue;
    }
    GELOGI("[Mobile] process node: %s dynamic inputs and outputs.", node_name.c_str());
    std::vector<int64_t> input_offset;
    GE_ASSERT_TRUE(GetDynamicInputsInfo(node, input_offset, dynamic_inputs_shape) == ge::SUCCESS,
                   "[Mobile] get dynamic inputs info failed.");
    std::vector<int64_t> output_offset;
    GE_ASSERT_TRUE(GetDynamicOutputsInfo(node, output_offset, dynamic_outputs_shape) == ge::SUCCESS,
                   "[Mobile] get dynamic outputs info failed.");
    break;
  }

  // parse args_format
  std::string args_format = kernel.context().args_format();
  GELOGI("[Mobile] kernel.context.args_format: %s ", args_format.c_str());
  std::vector<KernelArgInfoWithShapeInfo> arg_infos_with_shape_info;
  GE_ASSERT_TRUE(ParseArgsFormat(args_format, arg_infos_with_shape_info) == ge::SUCCESS,
                 "[Mobile] parse args format failed.");

  // set args info args index
  DynamicInputsOutputsShapeInfo dynamic_inputsoutputs_shapeinfo = {dynamic_inputs_shape, dynamic_outputs_shape};
  GE_ASSERT_TRUE(SetArgInfosArgsIndex(arg_infos_with_shape_info, dynamic_inputsoutputs_shapeinfo) == ge::SUCCESS,
                 "[Mobile] set arg infos args index failed.");

  // set args and flowtable
  GE_ASSERT_TRUE(SetArgsAndFlowTable(kernel, mobile_kernel, arg_infos_with_shape_info, dynamic_inputsoutputs_shapeinfo,
                                     is_flowtable, var_addr_to_file_offset, weight_addr) == ge::SUCCESS,
                 "[Mobile] get args and flowtable failed.");
  return ge::SUCCESS;
}

ge::Status ConvertTaskDef(const ge::GeModelPtr &ge_model, const std::shared_ptr<domi::ModelTaskDef> &model_task_def,
                          std::shared_ptr<ge::mobile::proto::ModelTaskDef> &mobile_model_task_def,
                          const std::map<uint64_t, uint64_t> &var_addr_to_file_offset, const uint64_t weight_addr) {
  for (const auto &task : model_task_def->task()) {
    const auto &kernel = task.kernel();
    if (kernel.args_size() == 0) {
      GELOGI("[Mobile] skip kernel args size is 0");
      continue;
    }
    auto *mobile_task = mobile_model_task_def->add_task();
    mobile_task->set_id(task.id());
    mobile_task->set_type(static_cast<uint32_t>(RT_MODEL_TASK_KERNEL));
    mobile_task->set_stream_id(task.stream_id());
    GELOGI("[Mobile] stream_id: %d", static_cast<int32_t>(task.stream_id()));
    mobile_task->set_event_id(task.event_id());

    // KernelDef
    auto *mobile_kernel = mobile_task->mutable_kernel();
    bool is_flowtable = false;
    GELOGD("[Mobile] stub_func: %s , kernel_name:%s", kernel.stub_func().c_str(), kernel.kernel_name().c_str());
    GE_ASSERT_TRUE(ConvertKernelDef(ge_model, kernel, mobile_kernel, is_flowtable, var_addr_to_file_offset,
                                    weight_addr) == ge::SUCCESS,
                   "[Mobile] convert kernel def failed.");

    // KernelContext
    const auto &kernel_context = kernel.context();
    auto *mobile_kernel_context = mobile_kernel->mutable_context();
    mobile_kernel_context->set_kernel_type(static_cast<uint32_t>(ASCENDC_AI_CORE));
    mobile_kernel_context->set_kernel_func_id(kernel_context.kernel_func_id());
    GELOGI("[Mobile] kernel_func_id: %d", static_cast<int32_t>(kernel_context.kernel_func_id()));
    mobile_kernel_context->set_is_flowtable(is_flowtable);
    mobile_kernel_context->set_args_offset(kernel_context.args_offset());
    mobile_kernel_context->set_args_count(mobile_kernel->args_size() / static_cast<uint32_t>(PER_ARG_BYTES));

    // set args offset use args_count
    std::vector<char> args_offset(mobile_kernel_context->args_count() * sizeof(uint16_t), static_cast<char>(0));
    mobile_kernel_context->set_args_offset(args_offset.data(), args_offset.size());
    GELOGI("[Mobile] mobile args_count: %d", mobile_kernel_context->args_count());
  }
  return ge::SUCCESS;
}

ge::Status AddCompatibleInfoToCompiledTarget(ge::CompiledTargetPtr &compiled_target) {
  ge::CompatibleInfoV2 compatible_info_v2;
  const size_t compatible_info_size = compatible_info_v2.GetSize();
  std::unique_ptr<uint8_t[]> compatible_info_data = std::make_unique<uint8_t[]>(compatible_info_size);
  GE_ASSERT_NOTNULL(compatible_info_data, "[Mobile] compatible_info_data is nullptr.");
  ge::BaseBuffer compatible_info_buffer(compatible_info_data.get(), compatible_info_size);
  const auto ret = compatible_info_v2.GetCompatibleInfo(compatible_info_buffer);
  GE_ASSERT_TRUE(ret == ge::SUCCESS, "[Mobile] GetCompatibleInfo failed.");
  GELOGI("[Mobile] compatible_info_buffer size: %d", compatible_info_buffer.GetSize());

  ge::SectionHolder compatible_info_section;
  compatible_info_section.type = static_cast<uint32_t>(ge::SectionType::SECTION_TYPE_COMPATIBLEV2);
  GE_ASSERT_TRUE(compatible_info_size <= UINT32_MAX, "[Mobile] overflow, failed.");
  compatible_info_section.size = static_cast<uint32_t>(compatible_info_size);
  compatible_info_section.data = std::move(compatible_info_data);
  compiled_target->AddSection(compatible_info_section);
  return ge::SUCCESS;
}

ge::Status AddKernelBinToManager(const ge::GeModelPtr &ge_model, ge::mobile::KernelBinManager &kernelbin_manager,
                                 std::shared_ptr<ge::mobile::proto::ModelTaskDef> &mobile_model_task_def) {
  const auto &tbe_kernel_store = ge_model->GetTBEKernelStore();
  // -> <name, offset>
  const auto &graph = ge_model->GetGraph();
  GELOGI("[Mobile] convert kernel bin.");
  std::set<std::string> added_kernels;
  for (const ge::NodePtr &node : graph->GetNodes(graph->GetGraphUnknownFlag())) {
    const std::string kernel_name_str = "_kernelname";
    std::string kernel_name = "";
    const string *kernel_name_ptr = ge::AttrUtils::GetStr(node->GetOpDesc(), kernel_name_str);
    if (kernel_name_ptr != nullptr) {
      kernel_name = *kernel_name_ptr;
    }
    const std::string &node_name = node->GetName();
    GELOGI("[Mobile] node name: %s, kernel name: %s", node_name.c_str(), kernel_name.c_str());
    auto kernel_bin = tbe_kernel_store.FindKernel(kernel_name);
    if (kernel_bin == nullptr) {
      GELOGI("[Mobile] kernel bin not found.");
      continue;
    }
    GELOGI("[Mobile] kernel bin data size: %d", kernel_bin->GetBinDataSize());
    if (added_kernels.count(kernel_name) == 0U) {
      GELOGI("[Mobile] add kernel bin for kernel name: %s", kernel_name.c_str());
      ge::mobile::KernelBin mobile_kernel_bin;
      mobile_kernel_bin.kernel_info.func_mode = 0U;
      mobile_kernel_bin.kernel_info.magic = MOBILE_RT_DEV_BINARY_MAGIC_ELF;
      GE_ASSERT_TRUE(kernel_bin->GetBinDataSize() <= UINT32_MAX, "[Mobile] overflow, failed.");
      mobile_kernel_bin.kernel_info.kernel_size = static_cast<uint32_t>(kernel_bin->GetBinDataSize());
      mobile_kernel_bin.kernel_info.kernel_offset = 0U;
      mobile_kernel_bin.stub_func = kernel_bin->GetBinData();
      mobile_kernel_bin.stub_name = kernel_name;
      GELOGI("[Mobile] stub_name: %s", kernel_name.c_str());
      kernelbin_manager.AddKernelBin(mobile_kernel_bin);
      (void)added_kernels.insert(kernel_name);
    } else {
      GELOGI("[Mobile] kernel bin already added, skip. kernel name: %s", kernel_name.c_str());
    }
    // replace task->kernel->stub_func -> kernel_name
    for (int i = 0; i < mobile_model_task_def->task_size(); i++) {
      auto *task = mobile_model_task_def->mutable_task(i);
      const auto &stub = task->kernel().stub_func();
      const bool matched = StubFuncMatchNodeName(stub, node_name);
      GELOGD("[Mobile] replace kernel stub func %s with %s, node_name %s, matched: %d", stub.c_str(),
             kernel_name.c_str(), node_name.c_str(), static_cast<int32_t>(matched));
      if (matched) {
        GELOGI("[Mobile] replace kernel stub func %s with %s", stub.c_str(), kernel_name.c_str());
        task->mutable_kernel()->set_stub_func(kernel_name);
      } else {
        GELOGI("[Mobile] can not find node name: %s in stub_func: %s", node_name.c_str(), stub.c_str());
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status AddKernelBinToCompiledTarget(ge::mobile::KernelBinManager &kernelbin_manager,
                                        ge::CompiledTargetPtr &compiled_target) {
  const size_t kernelbin_section_size = kernelbin_manager.GetBinSectionSize();
  std::unique_ptr<uint8_t[]> kernelbin_section_data = std::make_unique<uint8_t[]>(kernelbin_section_size);
  GE_ASSERT_NOTNULL(kernelbin_section_data, "[Mobile] kernelbin_section_data is nullptr.");
  ge::BaseBuffer kernelbin_section_buffer(kernelbin_section_data.get(), kernelbin_section_size);
  const auto ret = kernelbin_manager.SaveKernelBinToBuffer(kernelbin_section_buffer);
  GE_ASSERT_TRUE(ret == ge::SUCCESS, "[Mobile] SaveKernelBinToBuffer failed.");
  GELOGI("[Mobile] kernelbin_section_buffer size: %d", kernelbin_section_buffer.GetSize());

  ge::SectionHolder kernelbins_section;
  kernelbins_section.type = static_cast<uint32_t>(ge::SectionType::SECTION_TYPE_BIN);
  GE_ASSERT_TRUE(kernelbin_section_size <= UINT32_MAX, "[Mobile] overflow, failed.");
  kernelbins_section.size = static_cast<uint32_t>(kernelbin_section_size);
  kernelbins_section.data = std::move(kernelbin_section_data);
  compiled_target->AddSection(kernelbins_section);
  return ge::SUCCESS;
}

// 收集外置权重合并模式下 FileConstant 节点的映射信息:
// 变量区绝对地址(output offset) -> 权重文件内偏移, 同时取权重文件路径及所需最大文件长度
ge::Status CollectFileConstantVarAddrMap(const ge::GeModelPtr &ge_model,
                                         std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                                         std::string &weight_file_path, uint64_t &max_weight_end) {
  const auto &graph = ge_model->GetGraph();
  GE_ASSERT_NOTNULL(graph, "[Mobile] compute graph is nullptr.");
  for (const ge::NodePtr &node : graph->GetNodes(graph->GetGraphUnknownFlag())) {
    if (node->GetType() != "FileConstant") {
      continue;
    }
    const auto &op_desc = node->GetOpDesc();
    GE_ASSERT_NOTNULL(op_desc, "[Mobile] fileconstant %s op desc is nullptr.", node->GetName().c_str());
    const auto &fileconstant_info = ge::FileConstantUtils::GetFileConstantInfo(op_desc);
    GE_ASSERT_TRUE(!fileconstant_info.weight_path.empty(), "[Mobile] fileconstant %s has empty location.",
                   node->GetName().c_str());
    if (weight_file_path.empty()) {
      weight_file_path = fileconstant_info.weight_path;
    } else {
      GE_ASSERT_TRUE(
          weight_file_path == fileconstant_info.weight_path,
          "[Mobile] fileconstant %s location %s differs from %s, only one combined weight file is supported.",
          node->GetName().c_str(), fileconstant_info.weight_path.c_str(), weight_file_path.c_str());
    }
    const auto &output_offset = op_desc->GetOutputOffset();
    GE_ASSERT_TRUE(!output_offset.empty(), "[Mobile] fileconstant %s output offset is empty.", node->GetName().c_str());
    const uint64_t var_addr = static_cast<uint64_t>(output_offset[0]);
    GE_ASSERT_TRUE(var_addr >= ge::kVarMemoryLogicBase,
                   "[Mobile] fileconstant %s output offset 0x%llx is not in var memory region.",
                   node->GetName().c_str(), var_addr);
    const auto ret = var_addr_to_file_offset.insert({var_addr, fileconstant_info.weight_offset});
    if (!ret.second) {
      // 变量管理器对 (路径,偏移) 相同的 FileConstant 复用同一变量区地址, 属合法权重共享;
      // 仅当同一地址映射到不同文件偏移时才为真冲突
      GE_ASSERT_TRUE(ret.first->second == fileconstant_info.weight_offset,
                     "[Mobile] fileconstant %s output offset 0x%llx is reused but maps to different file offset "
                     "%llu, already mapped to %llu.",
                     node->GetName().c_str(), var_addr, fileconstant_info.weight_offset, ret.first->second);
    }
    const uint64_t weight_end =
        static_cast<uint64_t>(fileconstant_info.weight_offset) + static_cast<uint64_t>(fileconstant_info.weight_length);
    if (weight_end > max_weight_end) {
      max_weight_end = weight_end;
    }
    GELOGI("[Mobile] fileconstant %s var addr: 0x%llx, file offset: %llu, length: %llu", node->GetName().c_str(),
           var_addr, fileconstant_info.weight_offset, fileconstant_info.weight_length);
  }
  return ge::SUCCESS;
}

ge::Status LoadCombinedWeightFile(const std::string &weight_file_path, std::string &weight_data) {
  std::ifstream ifs(weight_file_path, std::ios::binary | std::ios::ate);
  GE_ASSERT_TRUE(ifs.is_open(), "[Mobile] open external weight file %s failed.", weight_file_path.c_str());
  const std::streampos end_pos = ifs.tellg();
  GE_ASSERT_TRUE((end_pos >= 0) && (static_cast<uint64_t>(end_pos) > 0UL), "[Mobile] external weight file %s is empty.",
                 weight_file_path.c_str());
  const auto file_size = static_cast<uint64_t>(end_pos);
  weight_data.resize(static_cast<size_t>(file_size));
  ifs.seekg(0, std::ios::beg);
  (void)ifs.read(weight_data.data(), static_cast<std::streamsize>(file_size));
  GE_ASSERT_TRUE(ifs.good(), "[Mobile] read external weight file %s failed.", weight_file_path.c_str());
  GELOGI("[Mobile] load combined weight file %s, size: %llu", weight_file_path.c_str(), file_size);
  return ge::SUCCESS;
}

// 外置权重模式下 kernel args 中的权重地址是变量区绝对地址, 设备侧无法刷新, 编译期重映射为 权重区基址+文件偏移
ge::Status RemapVarAddrArgsToWeightAddr(std::shared_ptr<ge::mobile::proto::ModelTaskDef> &mobile_model_task_def,
                                        const std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                                        const uint64_t weight_addr) {
  for (int i = 0; i < mobile_model_task_def->task_size(); i++) {
    auto *task = mobile_model_task_def->mutable_task(i);
    if (!task->has_kernel()) {
      continue;
    }
    auto *kernel = task->mutable_kernel();
    if (kernel->args_size() == 0) {
      continue;
    }
    GE_ASSERT_TRUE((kernel->args_size() % static_cast<uint32_t>(PER_ARG_BYTES)) == 0U,
                   "[Mobile] kernel %s args size %u is not %d bytes aligned.", kernel->stub_func().c_str(),
                   kernel->args_size(), PER_ARG_BYTES);
    const uint32_t args_count = kernel->args_size() / static_cast<uint32_t>(PER_ARG_BYTES);
    // args 内容按 8 字节解析
    const uint64_t *args_data = reinterpret_cast<const uint64_t *>(kernel->args().data());
    std::vector<uint64_t> args(args_data, args_data + args_count);
    bool is_modified = false;
    for (uint32_t j = 0; j < args_count; j++) {
      const uint64_t arg_val = args[j];
      const bool in_var_region = (arg_val >= ge::kVarMemoryLogicBase) &&
                                 (arg_val < (ge::kVarMemoryLogicBase + ge::kMemoryVarManagerMallocSize));
      if (!in_var_region) {
        continue;
      }
      const auto it = var_addr_to_file_offset.find(arg_val);
      GE_ASSERT_TRUE(it != var_addr_to_file_offset.end(),
                     "[Mobile] kernel %s args[%u] addr 0x%llx is in var memory region but has no fileconstant mapping.",
                     kernel->stub_func().c_str(), j, arg_val);
      args[j] = weight_addr + it->second;
      is_modified = true;
      GELOGI("[Mobile] kernel %s args[%u] remap var addr 0x%llx to weight addr 0x%llx, file offset: %llu.",
             kernel->stub_func().c_str(), j, arg_val, args[j], it->second);
    }
    if (is_modified) {
      kernel->set_args(static_cast<const void *>(args.data()), static_cast<uint32_t>(args_count * PER_ARG_BYTES));
    }
  }
  return ge::SUCCESS;
}

constexpr uint64_t WEIGHT_ALIGNMENT = 512U;
// 共享权重起始地址(非共享段末尾)需 4K 页对齐, 否则设备侧 mmap 权重区时地址非页对齐导致失败
constexpr uint64_t WEIGHT_PAGE_ALIGNMENT = 4096U;

inline uint64_t AlignUpToWeight(uint64_t size) {
  return ((size + WEIGHT_ALIGNMENT - 1U) / WEIGHT_ALIGNMENT) * WEIGHT_ALIGNMENT;
}

inline uint64_t AlignUpToPage(uint64_t size) {
  return ((size + WEIGHT_PAGE_ALIGNMENT - 1U) / WEIGHT_PAGE_ALIGNMENT) * WEIGHT_PAGE_ALIGNMENT;
}

struct UniqueWeight {
  uint64_t old_file_offset;
  uint64_t weight_length;
  bool is_shared = false;
  uint64_t new_file_offset = 0UL;
  std::vector<std::pair<std::string, uint64_t>> entries;  // (node_name, var_addr)
};

// 共享权重白名单: 默认支持 MatMulV3/QuantBatchMatmulV4/RmsNorm,
// 可通过环境变量 GE_MOBILE_WEIGHT_SHARE_OPS(逗号分隔)追加算子类型
std::set<std::string> GetSharedWeightOps() {
  std::set<std::string> ops = {"MatMulV3", "QuantBatchMatmulV4", "RmsNorm"};
  const char *env = std::getenv("GE_MOBILE_WEIGHT_SHARE_OPS");
  if (env == nullptr) {
    GELOGI("[Mobile] env GE_MOBILE_WEIGHT_SHARE_OPS not set, use default shared weight ops.");
    return ops;
  }
  std::string env_str(env);
  if (env_str.empty()) {
    GELOGI("[Mobile] env GE_MOBILE_WEIGHT_SHARE_OPS is empty, use default shared weight ops.");
    return ops;
  }
  size_t start = 0U;
  size_t pos;
  while ((pos = env_str.find(',', start)) != std::string::npos) {
    std::string op = env_str.substr(start, pos - start);
    if (!op.empty()) {
      ops.insert(op);
    }
    start = pos + 1U;
  }
  if (start < env_str.size()) {
    std::string op = env_str.substr(start);
    if (!op.empty()) {
      ops.insert(op);
    }
  }
  GELOGD("[Mobile] shared weight ops (default + env GE_MOBILE_WEIGHT_SHARE_OPS): %zu ops", ops.size());
  for (const auto &op : ops) {
    GELOGI("[Mobile]   shared op type: %s", op.c_str());
  }
  return ops;
}

// 遍历图中白名单算子, 追溯其所有 Const/FileConstant 数据输入, 标记为共享权重
void ClassifyFileConstantNodes(const ge::ComputeGraphPtr &graph, const std::set<std::string> &shared_ops,
                               std::set<std::string> &shared_fileconstant_names) {
  for (const ge::NodePtr &node : graph->GetNodes(graph->GetGraphUnknownFlag())) {
    if (shared_ops.find(node->GetType()) == shared_ops.end()) {
      continue;
    }
    GELOGI("[Mobile] whitelist op %s (type %s), tracing inputs for shared weights.", node->GetName().c_str(),
           node->GetType().c_str());
    for (auto *in_anchor : node->GetAllInDataAnchorsPtr()) {
      if (in_anchor == nullptr) {
        continue;
      }
      const auto peer_anchor = in_anchor->GetPeerOutAnchor();
      if (peer_anchor == nullptr) {
        continue;
      }
      auto *src_node = peer_anchor->GetOwnerNodeBarePtr();
      if ((src_node != nullptr) && (src_node->GetType() == "FileConstant")) {
        shared_fileconstant_names.insert(src_node->GetName());
        GELOGI("[Mobile] shared weight: op %s (type %s) input from FileConstant %s", node->GetName().c_str(),
               node->GetType().c_str(), src_node->GetName().c_str());
      }
    }
  }
}

// 权重重排: 非共享权重排前, 共享排后(白名单算子的Const输入), 权重块 512字节对齐,
// 非共享段末尾 4K 页对齐(共享段起始地址设备侧 mmap 要求), 连续布局
// 权重数据按块从权重文件流式读取填充, 避免整份原始文件与重排缓冲两份大内存同时常驻(峰值 2N -> N)
// 更新 var_addr_to_file_offset 映射为新的文件偏移
ge::Status ReorderWeights(const ge::ComputeGraphPtr &graph, const std::set<std::string> &shared_fileconstant_names,
                          const std::string &weight_file_path, std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                          std::string &reordered_weight_data, uint64_t &shared_weight_size,
                          uint64_t &total_weight_size) {
  // Step 1: 收集所有 FileConstant 节点信息, 按 old_file_offset 分组(hash去重: 相同 offset = 相同权重)
  std::map<uint64_t, UniqueWeight> unique_weights;  // sorted by old_file_offset for deterministic order
  for (const ge::NodePtr &node : graph->GetNodes(graph->GetGraphUnknownFlag())) {
    if (node->GetType() != "FileConstant") {
      continue;
    }
    const auto &op_desc = node->GetOpDesc();
    GE_ASSERT_NOTNULL(op_desc, "[Mobile] fileconstant %s op desc is nullptr.", node->GetName().c_str());
    const auto &fileconstant_info = ge::FileConstantUtils::GetFileConstantInfo(op_desc);
    const auto &output_offset = op_desc->GetOutputOffset();
    GE_ASSERT_TRUE(!output_offset.empty(), "[Mobile] fileconstant %s output offset is empty.", node->GetName().c_str());
    const uint64_t var_addr = static_cast<uint64_t>(output_offset[0]);
    const uint64_t old_file_offset = static_cast<uint64_t>(fileconstant_info.weight_offset);
    const uint64_t weight_length = static_cast<uint64_t>(fileconstant_info.weight_length);
    const bool is_shared = (shared_fileconstant_names.count(node->GetName()) > 0U);
    auto &uw = unique_weights[old_file_offset];
    uw.old_file_offset = old_file_offset;
    uw.weight_length = weight_length;
    if (is_shared) {
      uw.is_shared = true;
    }
    uw.entries.emplace_back(node->GetName(), var_addr);
  }

  // Step 2: 分配新偏移 — 非共享权重在前, 共享在后, 各 512 字节对齐
  // shared_weight_size 记录共享段起始偏移(即非共享段大小), 下游按此偏移切分两个权重文件:
  // [0, shared_weight_size) = 非共享段, [shared_weight_size, total) = 共享段
  // 权重全为共享时非共享段为空, shared_weight_size 为 0 是合法值(分区是否生效由上游显式标记,
  // 不以偏移是否大于 0 判定), 此时共享段从 0 开始
  // 共享权重起始地址(非共享段末尾)必须 4K 页对齐(设备侧 mmap 权重区要求), 故非共享段末尾补齐到 4K
  uint64_t current_offset = 0UL;
  for (auto &kv : unique_weights) {
    auto &uw = kv.second;
    if (uw.is_shared) {
      continue;
    }
    uw.new_file_offset = current_offset;
    current_offset += AlignUpToWeight(uw.weight_length);
  }
  shared_weight_size = AlignUpToPage(current_offset);
  current_offset = shared_weight_size;

  for (auto &kv : unique_weights) {
    auto &uw = kv.second;
    if (!uw.is_shared) {
      continue;
    }
    uw.new_file_offset = current_offset;
    current_offset += AlignUpToWeight(uw.weight_length);
  }
  total_weight_size = current_offset;

  // Step 3: 构建重排后的权重 buffer, 按块从权重文件流式读取对应偏移数据, 整份原始文件不常驻内存
  reordered_weight_data.resize(static_cast<size_t>(total_weight_size), '\0');
  std::ifstream ifs(weight_file_path, std::ios::binary | std::ios::ate);
  GE_ASSERT_TRUE(ifs.is_open(), "[Mobile] open external weight file %s failed.", weight_file_path.c_str());
  const std::streampos end_pos = ifs.tellg();
  GE_ASSERT_TRUE((end_pos >= 0) && (static_cast<uint64_t>(end_pos) > 0UL), "[Mobile] external weight file %s is empty.",
                 weight_file_path.c_str());
  const auto weight_file_size = static_cast<uint64_t>(end_pos);
  ifs.seekg(0, std::ios::beg);
  for (const auto &kv : unique_weights) {
    const auto &uw = kv.second;
    GE_ASSERT_TRUE(uw.old_file_offset + uw.weight_length <= weight_file_size,
                   "[Mobile] weight old_offset %llu + length %llu exceeds weight file %s size %llu.",
                   uw.old_file_offset, uw.weight_length, weight_file_path.c_str(), weight_file_size);
    GE_ASSERT_TRUE(uw.new_file_offset + uw.weight_length <= reordered_weight_data.size(),
                   "[Mobile] weight new_offset %llu + length %llu exceeds reordered buffer size %zu.",
                   uw.new_file_offset, uw.weight_length, reordered_weight_data.size());
    if (uw.weight_length > 0UL) {
      ifs.seekg(static_cast<std::streamoff>(uw.old_file_offset), std::ios::beg);
      GE_ASSERT_TRUE(ifs.good(), "[Mobile] seek weight file %s to offset %llu failed.", weight_file_path.c_str(),
                     uw.old_file_offset);
      ifs.read(reordered_weight_data.data() + uw.new_file_offset, static_cast<std::streamsize>(uw.weight_length));
      GE_ASSERT_TRUE(ifs.good(), "[Mobile] read weight %llu bytes from %s offset %llu failed.", uw.weight_length,
                     weight_file_path.c_str(), uw.old_file_offset);
    }
  }

  // Step 4: 打印刷新前后的权重逻辑地址, 更新 var_addr_to_file_offset 映射
  GELOGI("[Mobile] === Weight Reorder: old_offset -> new_offset ===");
  for (const auto &kv : unique_weights) {
    const auto &uw = kv.second;
    for (const auto &entry : uw.entries) {
      GELOGI(
          "[Mobile] weight reorder: node %s, var_addr 0x%llx, old_offset %llu -> new_offset %llu, "
          "length %llu, %s",
          entry.first.c_str(), entry.second, uw.old_file_offset, uw.new_file_offset, uw.weight_length,
          uw.is_shared ? "shared" : "nonshared");
    }
  }
  GELOGI("[Mobile] shared_weight_size: %llu, total_weight_size: %llu", shared_weight_size, total_weight_size);

  var_addr_to_file_offset.clear();
  for (const auto &kv : unique_weights) {
    const auto &uw = kv.second;
    for (const auto &entry : uw.entries) {
      var_addr_to_file_offset[entry.second] = uw.new_file_offset;
    }
  }
  return ge::SUCCESS;
}

ge::CompiledTargetPtr ConstructCompiledTarget(const ge::GeModelPtr &ge_model,
                                              const std::map<uint64_t, uint64_t> &var_addr_to_file_offset,
                                              const uint64_t external_weight_size) {
  ge::CompiledTargetPtr compiled_target = std::make_shared<ge::CompiledTarget>();

  // add model task def
  const std::shared_ptr<domi::ModelTaskDef> model_task_def = ge_model->GetModelTaskDefPtr();

  std::shared_ptr<ge::mobile::proto::ModelTaskDef> mobile_model_task_def =
      std::make_shared<ge::mobile::proto::ModelTaskDef>();

  const auto &model_task_def_attr = model_task_def->attr();
  for (const auto &it : model_task_def_attr) {
    GELOGI("[Mobile] attr %s : %s ", it.first.c_str(), it.second.c_str());
    (void)mobile_model_task_def->mutable_attr()->insert({it.first, it.second});
  }

  mobile_model_task_def->set_memory_size(model_task_def->memory_size());
  GELOGI("[Mobile] model task def memory size: %d", mobile_model_task_def->memory_size());
  GELOGI("[Mobile] stream num(ori): %d", static_cast<int32_t>(model_task_def->stream_num()));
  mobile_model_task_def->set_stream_num(1);
  GELOGI("[Mobile] stream num: %d", static_cast<int32_t>(mobile_model_task_def->stream_num()));
  mobile_model_task_def->set_event_num(model_task_def->event_num());
  // 外置权重(合并模式)时 weight_size 必须为实际权重文件大小, 供设备侧按 weight_size 分配权重区内存
  mobile_model_task_def->set_weight_size(
      (external_weight_size > 0UL) ? external_weight_size : static_cast<uint64_t>(model_task_def->weight_size()));

  // 2G size
  constexpr uint64_t data_mem_base = 2147483648;
  // set fm base addr
  mobile_model_task_def->set_base_addr(data_mem_base);
  // weight_addr should use data_mem_base + memory size(model task def)
  const uint64_t weight_addr = data_mem_base + mobile_model_task_def->memory_size();
  mobile_model_task_def->set_weight_addr(weight_addr);

  // -> taskdef
  const auto ret =
      ConvertTaskDef(ge_model, model_task_def, mobile_model_task_def, var_addr_to_file_offset, weight_addr);
  GE_ASSERT_TRUE(ret == ge::SUCCESS, "[Mobile] convert task def failed.");

  // 外置权重(合并模式)时把 kernel args 中的变量区地址重映射到权重区
  if (!var_addr_to_file_offset.empty()) {
    GE_ASSERT_TRUE(RemapVarAddrArgsToWeightAddr(mobile_model_task_def, var_addr_to_file_offset,
                                                mobile_model_task_def->weight_addr()) == ge::SUCCESS,
                   "[Mobile] remap var addr args to weight addr failed.");
  }
  mobile_model_task_def->set_batch_num(model_task_def->batch_num());

  // add compatible info section
  GE_ASSERT_TRUE(AddCompatibleInfoToCompiledTarget(compiled_target) == ge::SUCCESS,
                 "add compatible info to compiled target failed.");

  // add kernelbin section
  ge::mobile::KernelBinManager kernelbin_manager;
  GE_ASSERT_TRUE(AddKernelBinToManager(ge_model, kernelbin_manager, mobile_model_task_def) == ge::SUCCESS,
                 "add kernel bin to manager failed.");
  compiled_target->AddModelTaskDef(mobile_model_task_def);
  GE_ASSERT_TRUE(AddKernelBinToCompiledTarget(kernelbin_manager, compiled_target) == ge::SUCCESS,
                 "add kernel bin to manager failed.");
  return compiled_target;
}

ge::CompiledModelPtr ConstructCompiledModel(const ge::GeModelPtr &ge_model, const bool is_external_weight_combined,
                                            std::string &external_weight_data) {
  auto compiled_model_ptr = std::make_shared<ge::CompiledModel>();
  GE_ASSERT_NOTNULL(compiled_model_ptr, "[Mobile] compiled_model_ptr is nullptr");

  // add ge model(MODEL_DEF)
  compiled_model_ptr->SetGeModel(ge_model);

  // 外置权重(合并模式)时 FileConstant 已经外置落盘, ge_model 内置权重数据残缺,
  // 收集权重文件映射并用文件实际内容作为权重 buffer, 保证 graphop_weight_size/外置落盘大小正确
  std::map<uint64_t, uint64_t> var_addr_to_file_offset;
  std::string weight_file_path = "";
  uint64_t max_weight_end = 0UL;
  uint64_t external_weight_size = 0UL;
  uint64_t shared_weight_size = 0UL;
  bool weight_partition_enabled = false;
  if (is_external_weight_combined) {
    GE_ASSERT_TRUE(CollectFileConstantVarAddrMap(ge_model, var_addr_to_file_offset, weight_file_path, max_weight_end) ==
                       ge::SUCCESS,
                   "[Mobile] collect fileconstant var addr map failed.");
    if (!var_addr_to_file_offset.empty()) {
      // 权重重排: 非共享权重排前, 共享(白名单算子的Const输入)排后, 连续布局
      auto shared_ops = GetSharedWeightOps();
      std::set<std::string> shared_fileconstant_names;
      if (!shared_ops.empty()) {
        ClassifyFileConstantNodes(ge_model->GetGraph(), shared_ops, shared_fileconstant_names);
      }
      if (!shared_fileconstant_names.empty()) {
        // 流式重排: 按块从权重文件读取直接填充 external_weight_data, 避免整份原始文件与重排缓冲同时常驻内存
        GE_ASSERT_TRUE(
            ReorderWeights(ge_model->GetGraph(), shared_fileconstant_names, weight_file_path, var_addr_to_file_offset,
                           external_weight_data, shared_weight_size, external_weight_size) == ge::SUCCESS,
            "[Mobile] reorder weights failed.");
        // 进入重排分支即存在共享段(至少一个共享权重), 分区生效; 权重全为共享时非共享段为空,
        // shared_weight_size 为 0, 须显式置位分区标记, 避免下游以偏移大于 0 判定而静默丢弃共享分区
        weight_partition_enabled = true;
      } else {
        GE_ASSERT_TRUE(LoadCombinedWeightFile(weight_file_path, external_weight_data) == ge::SUCCESS,
                       "[Mobile] load combined weight file failed.");
        // 文件实际大小不能小于所有 FileConstant 所需的最大范围
        GE_ASSERT_TRUE(external_weight_data.size() >= max_weight_end,
                       "[Mobile] combined weight file %s size %llu is smaller than required %llu.",
                       weight_file_path.c_str(), external_weight_data.size(), max_weight_end);
        external_weight_size = external_weight_data.size();
      }
    }
  }

  // add weights buffer(WEIGHTS_DATA)
  ge::BaseBuffer weight_buffer;
  if (external_weight_size > 0UL) {
    // BaseBuffer 为浅指针容器, external_weight_data 由调用方持有, 生命期覆盖 SaveToOmModel 整个保存过程
    weight_buffer.SetData(external_weight_data.data());
    weight_buffer.SetSize(external_weight_data.size());
  } else {
    weight_buffer.SetData(ge_model->GetWeightData());
    weight_buffer.SetSize(ge_model->GetWeightSize());
  }
  compiled_model_ptr->AddWeight(weight_buffer);

  // add compiled targets(TASK_INFO)
  compiled_model_ptr->AddCompiledTarget(
      ConstructCompiledTarget(ge_model, var_addr_to_file_offset, external_weight_size));
  compiled_model_ptr->SetWeightPartitionEnabled(weight_partition_enabled);
  compiled_model_ptr->SetSharedWeightSize(shared_weight_size);
  return compiled_model_ptr;
}

}  // namespace

namespace ge {

Status MobileModelHelper::SaveToOmRootModel(const GeRootModelPtr &ge_root_model, const std::string &output_file,
                                            ModelBufferData &model, const bool is_unknown_shape) {
  GE_ASSERT_NOTNULL(ge_root_model, "[Mobile] ge_root_model is nullptr");
  GE_ASSERT_TRUE(!output_file.empty(), "[Mobile] SaveModel received invalid file name prefix.");
  const auto &name_to_ge_model = ge_root_model->GetSubgraphInstanceNameToModel();
  GE_ASSERT_TRUE(!name_to_ge_model.empty(), "[Mobile] ge_root_model has no sub model.");
  if (!is_unknown_shape) {
    auto &model_root = name_to_ge_model.begin()->second;
    return SaveToOmModel(model_root, output_file, model, ge_root_model);
  }
  GELOGE(FAILED, "[Mobile] mobile does not support unknown shape model to om.");
  REPORT_INNER_ERR_MSG("E19999", "[Mobile] mobile does not support unknown shape model to om.");
  return FAILED;
}

Status MobileModelHelper::SaveToOmModel(const GeModelPtr &ge_model, const std::string &output_file,
                                        ModelBufferData &model, const GeRootModelPtr &ge_root_model) {
  (void)ge_root_model;
  (void)model;
  GE_ASSERT_NOTNULL(ge_model, "ge_model is nullptr");
  if (output_file.empty()) {
    GELOGE(FAILED,
           "[Mobile] SaveModel received invalid file name prefix, "
           "model %s",
           ge_model->GetName().c_str());
    REPORT_INNER_ERR_MSG("E19999",
                         "[Mobile] SaveModel received invalid file name prefix, "
                         "model %s",
                         ge_model->GetName().c_str());
    return FAILED;
  }

  std::string external_weight_str = "0";
  (void)ge::GetContext().GetOption(ge::EXTERNAL_WEIGHT, external_weight_str);
  GELOGI("[Mobile] external weight str: %s", external_weight_str.c_str());
  // 模式1(每个权重单独导出)不支持移动端 omc, 仅模式2(合并导出)支持
  GE_ASSERT_TRUE(external_weight_str != ge::kExternalWeightEnabled,
                 "[Mobile] mobile omc only supports external weight mode 2(combined), mode %s is not supported.",
                 external_weight_str.c_str());
  const bool is_external_weight_combined = (external_weight_str == ge::kExternalWeightCombined);
  std::string external_weight_data;
  auto compiled_model_ptr = ConstructCompiledModel(ge_model, is_external_weight_combined, external_weight_data);
  GE_ASSERT_NOTNULL(compiled_model_ptr, "[Mobile] compiled_model_ptr is nullptr");
  GELOGI("[Mobile] output file: %s", output_file.c_str());

  // 权重落盘目录与 FileConstant location 刷新目录保持一致: om 目录下的 weight 子目录
  std::string output_weight_dir = "";
  const auto pos = output_file.rfind("/");
  if (pos != std::string::npos) {
    output_weight_dir = output_file.substr(0, pos) + std::string("/weight");
  }
  if (is_external_weight_combined && output_weight_dir.empty()) {
    // online 场景 output_file 不含目录信息, 从 FileConstant location(已是绝对路径)取权重文件所在目录
    for (const ge::NodePtr &node : ge_model->GetGraph()->GetAllNodes()) {
      if (node->GetType() == "FileConstant") {
        const auto &location = ge::FileConstantUtils::GetFileConstantInfo(node->GetOpDesc()).weight_path;
        if (!location.empty()) {
          const auto dir_pos = location.rfind("/");
          GE_ASSERT_TRUE(dir_pos != std::string::npos, "[Mobile] fileconstant %s location %s has no dir part.",
                         node->GetName().c_str(), location.c_str());
          output_weight_dir = location.substr(0, dir_pos);
        }
        break;
      }
    }
    GE_ASSERT_TRUE(!output_weight_dir.empty(), "[Mobile] can not get weight output dir from fileconstant location.");
    GELOGI("[Mobile] get weight output dir from fileconstant location: %s", output_weight_dir.c_str());
  }
  GELOGI("[Mobile] output weight dir: %s", output_weight_dir.c_str());

  ge::ModelFileSaver model_file_saver;
  const auto ret = model_file_saver.SaveCompiledModelToFile(compiled_model_ptr, (output_file + "c").c_str(),
                                                            output_weight_dir.c_str(), is_external_weight_combined);

  GE_ASSERT_TRUE(ret == SUCCESS, "[Mobile] model file saver, save failed.");
  return SUCCESS;
}

void MobileModelHelper::SetSaveMode(const bool val) {
  is_offline_ = val;
}

REGISTER_MODEL_SAVE_HELPER(OM_FORMAT_MOBILE, MobileModelHelper);

}  // namespace ge
