/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/codegen/var_fixup_table_builder.h"

#include <algorithm>
#include <cinttypes>
#include <unordered_map>
#include <unordered_set>

#include "common/const_place_holder_utils/const_place_holder_utils.h"
#include "common/ge_inner_error_codes.h"
#include "common/ge_common/debug/ge_log.h"
#include "common/math/ge_math_util.h"
#include "framework/common/framework_types_internal.h"
#include "framework/common/gert_model_data_utils.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/node.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
namespace {
// 算子类型字符串 → 转换动作种类。用 framework_types_internal.h 的权威常量比较，
// 不用手写字面量：字面量一旦与常量脱钩就静默失配（模型 SO 侧因此只认枚举）。
VarTransKind MapTransKind(const std::string &node_type) {
  if ((node_type == RESHAPE) || (node_type == REFORMAT) || (node_type == SQUEEZEV2) || (node_type == UNSQUEEZEV2)) {
    return VarTransKind::kNoOp;
  }
  if ((node_type == TRANSDATA) || (node_type == TRANSPOSED)) {
    return VarTransKind::kFormat;
  }
  if (node_type == CAST) {
    return VarTransKind::kCast;
  }
  return VarTransKind::kUnsupported;
}

// 变量键：name + format + "_" + dtype。模型 SO 只凭固化好的键向容器查询，
// 键公式所在头文件不在 SO 可见范围内，故必须编译期算好。
std::string BuildVarKey(const std::string &var_name, const gert::GertTensorDesc &desc) {
  return var_name + std::to_string(static_cast<int32_t>(desc.format)) + "_" +
         std::to_string(static_cast<int32_t>(desc.data_type));
}

gert::GertTensorDesc ConvertDesc(const GeTensorDesc &ge_desc) {
  auto desc = gert::MakeGertTensorDesc(ge_desc.GetName(), ge_desc.GetDataType(), ge_desc.GetFormat(),
                                       ge_desc.GetShape().GetDims());
  std::vector<std::pair<int64_t, int64_t>> shape_range;
  if (ge_desc.GetShapeRange(shape_range) == GRAPH_SUCCESS) {
    desc.shape_range = shape_range;
  }
  int64_t size = 0;
  if (TensorUtils::GetTensorSizeInBytes(ge_desc, size) == GRAPH_SUCCESS) {
    desc.size = static_cast<size_t>(size);
  }
  return desc;
}

std::vector<int64_t> ClampDims(const GeShape &shape) {
  std::vector<int64_t> dims(kVarDimsMax, 0);
  const auto &src = shape.GetDims();
  for (size_t i = 0U; (i < src.size()) && (i < kVarDimsMax); ++i) {
    dims[i] = src[i];
  }
  return dims;
}

// 与 ClampDims 配套的真实维数。必须按各自的 shape 分别算：格式转换会改变维数。
uint32_t ClampDimNum(const GeShape &shape) {
  return static_cast<uint32_t>(std::min(shape.GetDims().size(), kVarDimsMax));
}

std::vector<uint8_t> ExtractInitDataFromWeights(const OpDescPtr &op_desc) {
  ConstGeTensorPtr weight = nullptr;
  if (!AttrUtils::GetTensor(*op_desc, ATTR_NAME_WEIGHTS, weight) || (weight == nullptr)) {
    return {};
  }
  const auto &data = weight->GetData();
  if (data.size() == 0U) {
    return {};
  }
  return std::vector<uint8_t>(data.data(), data.data() + data.size());
}

std::vector<uint8_t> ExtractInitValueFromDesc(const GeTensorDesc &ge_desc) {
  ConstGeTensorPtr init_value = nullptr;
  if (!AttrUtils::GetTensor(ge_desc, ATTR_NAME_INIT_VALUE, init_value) || (init_value == nullptr)) {
    return {};
  }
  const auto &data = init_value->GetData();
  if (data.size() == 0U) {
    return {};
  }
  return std::vector<uint8_t>(data.data(), data.data() + data.size());
}

// 拷贝源变量名与其张量描述。源描述取自 _copy_from_var_node 指向节点的 output 0。
Status FillCopySource(const ComputeGraphPtr &compute_graph, const OpDescPtr &op_desc, std::string &src_var_name,
                      GeTensorDesc &src_desc, bool &has_src_desc) {
  const auto *copy_from = AttrUtils::GetStr(*op_desc, "_copy_from_var_node");
  if ((copy_from == nullptr) || copy_from->empty()) {
    return SUCCESS;
  }
  src_var_name = *copy_from;
  if (compute_graph == nullptr) {
    return SUCCESS;
  }
  const auto src_node = compute_graph->FindNode(*copy_from);
  if (src_node == nullptr) {
    return SUCCESS;
  }
  const auto src_op_desc = src_node->GetOpDesc();
  if (src_op_desc == nullptr) {
    return SUCCESS;
  }
  const auto src_output_desc = src_op_desc->MutableOutputDesc(0U);
  if (src_output_desc != nullptr) {
    src_desc = *src_output_desc;
    has_src_desc = true;
  }
  return SUCCESS;
}

Status ConvertTransRoad(const VarTransRoad &road, std::vector<VarTransNodeFixup> &out) {
  out.reserve(road.size());
  for (const auto &node : road) {
    VarTransNodeFixup fixup;
    fixup.kind = MapTransKind(node.node_type);
    if (fixup.kind == VarTransKind::kUnsupported) {
      GELOGE(FAILED, "[OM2][Var][Fixup] unsupported trans node_type=%s.", node.node_type.c_str());
      return FAILED;
    }
    fixup.src_format = static_cast<uint32_t>(node.input.GetFormat());
    fixup.dst_format = static_cast<uint32_t>(node.output.GetFormat());
    fixup.src_data_type = static_cast<uint32_t>(node.input.GetDataType());
    fixup.dst_data_type = static_cast<uint32_t>(node.output.GetDataType());
    fixup.src_dims = ClampDims(node.input.GetShape());
    fixup.dst_dims = ClampDims(node.output.GetShape());
    fixup.src_dim_num = ClampDimNum(node.input.GetShape());
    fixup.dst_dim_num = ClampDimNum(node.output.GetShape());
    out.push_back(std::move(fixup));
  }
  return SUCCESS;
}

// 转换链上是否存在真正改变数据的节点。V1 的 TransVarData 在 D2H 之前做同一扫描：全为
// IsNoNeedTrans（RESHAPE / REFORMAT / SQUEEZEV2 / UNSQUEEZEV2）即 return SUCCESS、从不触碰转换源。
bool NeedRealTrans(const std::vector<VarTransNodeFixup> &road) {
  for (const auto &node : road) {
    if (node.kind != VarTransKind::kNoOp) {
      return true;
    }
  }
  return false;
}

// 三类"有转换链却不转换"的排除计数。逐条依据记进 ATC 日志，此处只为循环后汇总一行。
struct TransExclusionStats {
  size_t no_road_num{0U};
  size_t graph_id_num{0U};
  size_t no_data_move_num{0U};
};

// 构建单条固化表条目。init_data 追加进数据块并回填偏移，二者必须在同一步完成。
// copy_src_var_name 回传拷贝源变量名，供工作列表展开（固化表里存的是源键，展开需要的是名）。
Status BuildFixupEntry(VarManager &var_manager, const ComputeGraphPtr &compute_graph, const std::string &var_name,
                       VarFixupTable &table, VarFixupEntry &entry, std::string &copy_src_var_name,
                       TransExclusionStats &stats) {
  GeTensorDesc cur_desc;
  GE_ASSERT_SUCCESS(var_manager.GetCurVarDesc(var_name, cur_desc));
  const auto gert_desc = ConvertDesc(cur_desc);
  if (gert_desc.size == 0U) {
    GELOGE(FAILED, "[OM2][Var][Fixup] var=%s size is 0, cannot fixate.", var_name.c_str());
    return FAILED;
  }

  entry.var_key = BuildVarKey(var_name, gert_desc);
  entry.size = gert_desc.size;
  entry.data_type = static_cast<uint32_t>(gert_desc.data_type);

  // 逻辑地址与内存类型是容器解析设备地址的必需输入：外置内存区按 logic_addr 算区内偏移，
  // 自行分配按 memory_type 选内存类型。取不到就固化不出正确条目，故编译期直接失败——
  // 静默跳过会让 logic_addr 留 0，直到运行期才由容器报"logic_addr 低于区内基址"。
  // 该不变式上游已强制：VarMemAssignUtil 在编译期为 VARIABLE / CONSTANTOP / FILECONSTANT /
  // CONSTPLACEHOLDER 四类登记地址，并对同一次 GetVarAddr 硬失败
  // （base/graph/build/memory/var_mem_assign_util.cc）。
  uint8_t *dev_ptr = nullptr;
  rtMemType_t memory_type = RT_MEMORY_HBM;
  if (var_manager.GetVarAddr(var_name, cur_desc, dev_ptr, memory_type) != SUCCESS) {
    GELOGE(FAILED, "[OM2][Var][Fixup] var=%s get var addr failed, cannot fixate logic addr.", var_name.c_str());
    return FAILED;
  }
  entry.logic_addr = reinterpret_cast<uint64_t>(dev_ptr);
  entry.memory_type = static_cast<uint64_t>(memory_type);

  std::vector<uint8_t> init_data;
  OpDescPtr op_desc = nullptr;
  if (compute_graph != nullptr) {
    const auto node = compute_graph->FindNode(var_name);
    if (node != nullptr) {
      op_desc = node->GetOpDesc();
    }
  }
  if (op_desc != nullptr) {
    const std::string op_type(op_desc->GetType());
    if (op_type == CONSTPLACEHOLDER) {
      uint8_t *const_addr = nullptr;
      GE_ASSERT_SUCCESS(GetConstPlaceHolderAddr(op_desc, const_addr));
      entry.extern_dev_addr = reinterpret_cast<uint64_t>(const_addr);
    } else if (op_type == CONSTANTOP) {
      init_data = ExtractInitDataFromWeights(op_desc);
    } else if (op_type == VARIABLE) {
      init_data = ExtractInitValueFromDesc(cur_desc);
      GeTensorDesc copy_src_desc;
      bool has_copy_src_desc = false;
      GE_ASSERT_SUCCESS(FillCopySource(compute_graph, op_desc, copy_src_var_name, copy_src_desc, has_copy_src_desc));
      if (has_copy_src_desc) {
        const auto copy_src_gert_desc = ConvertDesc(copy_src_desc);
        entry.copy_src_var_key = BuildVarKey(copy_src_var_name, copy_src_gert_desc);
        entry.copy_src_data_type = static_cast<uint32_t>(copy_src_gert_desc.data_type);
      }
    }
  }

  // 三条静态判据在此一次求值，不满足者根本不写入 trans_road 与 trans_src_var_key，使
  // "trans_road 非空 ⟺ 本模型将执行该转换"成为唯一判据。操作数全是编译期常量（RTVarResource
  // 是编译末封闭的快照、运行期不可变），且两个图标识不进产物 VarMeta、运行期无从复算，
  // 故排除依据只能在此记进 ATC 日志留痕。
  // 判据先于 ConvertTransRoad：被排除条目的链不属于本模型，其非法节点类型不该让本模型编译失败。
  const auto *trans_road = var_manager.GetTransRoad(var_name);
  if ((trans_road == nullptr) || trans_road->empty()) {
    // 无转换链 ⇒ 不是转换目标。这是绝大多数变量的情形，无诊断价值，故只计数不逐条留痕。
    ++stats.no_road_num;
  } else {
    uint32_t changed_graph_id = 0U;
    uint32_t allocated_graph_id = 0U;
    (void)var_manager.GetChangedGraphId(var_name, changed_graph_id);
    (void)var_manager.GetAllocatedGraphId(var_name, allocated_graph_id);
    if ((changed_graph_id != table.graph_id) || (changed_graph_id == allocated_graph_id)) {
      ++stats.graph_id_num;
      GELOGI(
          "[OM2][Var][Fixup] var=%s excluded from transform by graph id: changed_graph=%u, allocated_graph=%u, "
          "model_graph=%" PRIu64 ", %s.",
          var_name.c_str(), changed_graph_id, allocated_graph_id, table.graph_id,
          (changed_graph_id != table.graph_id) ? "changed graph is not this model"
                                               : "changed graph is the allocating graph");
    } else {
      std::vector<VarTransNodeFixup> road;
      GE_ASSERT_SUCCESS(ConvertTransRoad(*trans_road, road));
      if (NeedRealTrans(road)) {
        entry.trans_road = std::move(road);
        // 转换源键：与运行期 BuildVarKey(var_name, trans_road.front().input) 等价，编译期算好固化
        entry.trans_src_var_key = BuildVarKey(var_name, ConvertDesc(trans_road->front().input));
      } else {
        // 链上全是 NoOp，只改描述、不搬数据。
        ++stats.no_data_move_num;
        GELOGI("[OM2][Var][Fixup] var=%s excluded from transform: trans road has %zu node(s), none moves data.",
               var_name.c_str(), trans_road->size());
      }
    }
  }

  if (!init_data.empty()) {
    if (init_data.size() > entry.size) {
      GELOGE(FAILED, "[OM2][Var][Fixup] var=%s init_data %zu bytes > var size %" PRIu64 ".", var_name.c_str(),
             init_data.size(), entry.size);
      return FAILED;
    }
    entry.init_data_offset = table.weight_data.size();
    entry.init_data_size = init_data.size();
    table.weight_data.insert(table.weight_data.end(), init_data.begin(), init_data.end());
  }
  return SUCCESS;
}
}  // namespace

Status BuildVarFixupTable(VarManager &var_manager, const ComputeGraphPtr &compute_graph,
                          const std::vector<VarSlotMeta> &var_metas, const uint64_t graph_id, VarFixupTable &table) {
  table = VarFixupTable();
  table.graph_id = graph_id;
  if (var_metas.empty()) {
    return SUCCESS;
  }

  // 槽位下标来自 Codegen 分配，与生成代码的 OpArgDesc.index 同一编号空间；重复即编译失败
  std::unordered_map<std::string, uint32_t> name_to_slot;
  std::unordered_set<uint32_t> used_slots;
  for (const auto &meta : var_metas) {
    if (!used_slots.insert(meta.slot).second) {
      GELOGE(FAILED, "[OM2][Var][Fixup] duplicated var slot %u for var=%s.", meta.slot, meta.var_name.c_str());
      return FAILED;
    }
    name_to_slot[meta.var_name] = meta.slot;
  }

  // 工作列表沿 copy_info.src_var_name 展开（不沿 trans_road 展开），按 var_name 去重
  std::unordered_set<std::string> processed;
  std::vector<std::string> pending;
  pending.reserve(var_metas.size());
  for (const auto &meta : var_metas) {
    pending.emplace_back(meta.var_name);
  }

  TransExclusionStats stats;
  while (!pending.empty()) {
    const auto var_name = pending.back();
    pending.pop_back();
    if (!processed.insert(var_name).second) {
      continue;
    }
    VarFixupEntry entry;
    const auto slot_it = name_to_slot.find(var_name);
    entry.slot = (slot_it == name_to_slot.end()) ? kVarNoSlot : slot_it->second;
    std::string copy_src_var_name;
    GE_ASSERT_SUCCESS(BuildFixupEntry(var_manager, compute_graph, var_name, table, entry, copy_src_var_name, stats));
    if (!copy_src_var_name.empty()) {
      pending.push_back(copy_src_var_name);
    }
    table.entries.push_back(std::move(entry));
  }

  // 输出有效性：数据块长度等于最后一个偏移加其长度
  uint64_t expected = 0U;
  for (const auto &entry : table.entries) {
    if (entry.init_data_size > 0U) {
      expected = entry.init_data_offset + entry.init_data_size;
    }
  }
  if (expected != table.weight_data.size()) {
    GELOGE(FAILED, "[OM2][Var][Fixup] weight_data size %zu != last offset+len %" PRIu64 ".", table.weight_data.size(),
           expected);
    return FAILED;
  }
  // 转换候选数在此汇总而非逐条推算：判据已把结论写进 trans_road，非空即候选。
  size_t trans_num = 0U;
  for (const auto &entry : table.entries) {
    if (!entry.trans_road.empty()) {
      ++trans_num;
    }
  }
  GELOGI(
      "[OM2][Var][Fixup] transform folded at build time: %zu of %zu var(s) need transform, excluded %zu "
      "(no road) + %zu (graph id) + %zu (road moves no data); only the container is_loaded probe stays "
      "at runtime.",
      trans_num, table.entries.size(), stats.no_road_num, stats.graph_id_num, stats.no_data_move_num);
  GELOGI("[OM2][Var][Fixup] built %zu entries from %zu var_metas, weight_data=%zu bytes.", table.entries.size(),
         var_metas.size(), table.weight_data.size());
  return SUCCESS;
}
}  // namespace ge
