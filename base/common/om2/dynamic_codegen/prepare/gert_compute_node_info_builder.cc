/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_compute_node_info_builder.h"

#include "securec.h"
#include <numeric>
#include <utility>

#include "common/checker.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "framework/common/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/utils/attr_utils.h"
#include "graph_metadef/base/attr/attrs_to_buffer.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace gert {
namespace {
// 属性缓冲在 CNI 之后单独分配，由 CNI 记录其长度。复用既有 attrs_to_buffer，
// 保证公开属性路径和 GE lowering 产出的运行时属性布局完全一致。
bool AppendAttrValues(const std::vector<ge::AnyValue> &attrs, std::vector<std::vector<uint8_t>> &buffers) {
  for (const auto &attr : attrs) {
    if (!bg::AppendAttr(attr, buffers)) {
      GELOGE(ge::GRAPH_PARAM_INVALID, "Unsupported attribute type %d for a compute node info", attr.GetValueType());
      return false;
    }
  }
  return true;
}

ge::graphStatus FillTd(const ge::GeTensorDesc &source, CompileTimeTensorDesc &td) {
  td.SetDataType(source.GetDataType());
  td.SetOriginFormat(source.GetOriginFormat());
  td.SetStorageFormat(source.GetFormat());
  int64_t reshape_type_mask = 0;
  if (ge::AttrUtils::GetInt(source, ge::ATTR_NAME_RESHAPE_TYPE_MASK, reshape_type_mask)) {
    td.SetExpandDimsType(ExpandDimsType(reshape_type_mask));
  }
  bool is_null_output = false;
  if (ge::AttrUtils::GetBool(source, ge::ATTR_NAME_IS_NULL_OUTPUT, is_null_output) && is_null_output) {
    td.SetExist(false);
  }
  return ge::GRAPH_SUCCESS;
}

// 由 IR 端口的实例数推导每个实例在全局端口中的起点：实例是连续排列的，起点即前缀和。
void SetInstanceInfo(const std::vector<int64_t> &counts, ComputeNodeInfo &info, bool input) {
  size_t start = 0U;
  for (size_t i = 0U; i < counts.size(); ++i) {
    auto *ins = input ? info.MutableInputInstanceInfo(i) : info.MutableOutputInstanceInfo(i);
    if (ins == nullptr) {
      continue;
    }
    ins->SetInstanceStart(start);
    ins->SetInstantiationNum(static_cast<size_t>(counts[i]));
    start += static_cast<size_t>(counts[i]);
  }
}
}  // namespace

ge::graphStatus BuildComputeNodeDef(const ge::FastNode &node, const GertSourceInfo *source, GertComputeNodeDef &def) {
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  GertComputeNodeDef parsed;
  parsed.source = source;
  parsed.name = node.GetName();
  parsed.type = node.GetType();
  parsed.input_num = node.GetDataInNum();
  parsed.output_num = node.GetDataOutNum();

  for (size_t i = 0U; i < parsed.input_num; ++i) {
    const auto input_desc = desc->GetInputDescPtr(static_cast<uint32_t>(i));
    if (input_desc != nullptr) {
      parsed.input_descs.emplace_back(*input_desc);
      continue;
    }
    // 空端口用输出端口描述兜底没有依据，直接补一个空描述，由消费方按需判断。
    parsed.input_descs.emplace_back(ge::GeTensorDesc());
  }
  for (size_t i = 0U; i < parsed.output_num; ++i) {
    const auto output_desc = desc->GetOutputDescPtr(static_cast<uint32_t>(i));
    if (output_desc != nullptr) {
      parsed.output_descs.emplace_back(*output_desc);
      continue;
    }
    parsed.output_descs.emplace_back(ge::GeTensorDesc());
  }

  if (source != nullptr) {
    // 来源描述的是算子本身的 IR 端口，不是 kernel 的物理端口。上层的准备链 kernel
    // （TilingParse、PrepareCacheableTilingFwkData、CacheableTiling 等）会引用同一份
    // 算子 CNI，但它们的物理端口更多——CacheableTiling 正是按 CNI 的 IR 输入数读取
    // 前若干个 shape。因此这里只要求物理端口不少于 IR 端口；多余的是 kernel 辅助端口，
    // 不进入 CNI。
    // 准备链里既有端口多于算子（CacheableTiling 8 入）也有端口少于算子
    // （GetPlatformInfo 0 入）的 kernel，二者共用同一份算子 CNI 都是合法的。
    // 端口描述只填充 min(算子实例端口数, 物理端口数) 个，其余由消费方按需忽略。
    GELOGI("[GertGraph] Node %s (type %s) has %zu inputs/%zu outputs while source %s describes %zu/%zu",
           parsed.name.c_str(), parsed.type.c_str(), parsed.input_num, parsed.output_num, source->id.c_str(),
           source->inputs.size(), source->outputs.size());
    GE_ASSERT_TRUE(source->attr_names.size() == source->attrs.size(),
                   "Source %s declares %zu attribute names but %zu values", source->id.c_str(),
                   source->attr_names.size(), source->attrs.size());
    parsed.attrs = source->attrs;
    // 端口描述按算子实例端口数补齐/截断：CNI 的输入输出描述区由实例端口数决定，
    // 而 kernel 的物理端口可能与算子不同（GetPlatformInfo 无输入、CacheableTiling 端口更多）。
    parsed.input_descs.resize(static_cast<size_t>(std::accumulate(source->ir_input_counts.cbegin(),
                                                                  source->ir_input_counts.cend(), int64_t{0})),
                              ge::GeTensorDesc());
    parsed.output_descs.resize(static_cast<size_t>(std::accumulate(source->ir_output_counts.cbegin(),
                                                                   source->ir_output_counts.cend(), int64_t{0})),
                               ge::GeTensorDesc());
  }
  def = std::move(parsed);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus BuildComputeNodeInfoBuffer(const GertComputeNodeDef &def, bg::BufferPool &buffer_pool,
                                           std::unique_ptr<uint8_t[]> &compute_node_info, size_t &total_size) {
  // 无来源的固定 ABI kernel：IR 端口与实际端口一一对应，每个 IR 端口恰好实例化 1 次。
  // 这是唯一有确定规则的推断方式；需要算子语义的 kernel 缺少来源必须在调用方拒绝。
  std::vector<int64_t> ir_input_counts;
  std::vector<int64_t> ir_output_counts;
  // 无来源时 IR 端口与物理端口一一对应；有来源时两者都取自算子描述。
  // 注意 ComputeNodeInfo 的 inputs_num_/outputs_num_ 是「实例端口数」，消费方
  // （如 BuildGeneralTilingCacheKey）按它遍历 shape 输入，因此必须与算子一致，
  // 不能填 kernel 自己的物理端口数。
  size_t ir_input_num = def.input_num;
  size_t ir_output_num = def.output_num;
  size_t instance_input_num = def.input_num;
  size_t instance_output_num = def.output_num;
  if (def.source != nullptr) {
    const auto &source = *def.source;
    ir_input_num = source.ir_input_names.size();
    ir_output_num = source.ir_output_names.size();
    instance_input_num = static_cast<size_t>(
        std::accumulate(source.ir_input_counts.cbegin(), source.ir_input_counts.cend(), int64_t{0}));
    instance_output_num = static_cast<size_t>(
        std::accumulate(source.ir_output_counts.cbegin(), source.ir_output_counts.cend(), int64_t{0}));
    GE_ASSERT_TRUE(source.ir_input_counts.size() == ir_input_num, "Source %s IR input names and counts disagree",
                   source.id.c_str());
    GE_ASSERT_TRUE(source.ir_output_counts.size() == ir_output_num, "Source %s IR output names and counts disagree",
                   source.id.c_str());
    ir_input_counts = source.ir_input_counts;
    ir_output_counts = source.ir_output_counts;
  } else {
    ir_input_counts.assign(def.input_num, 1);
    ir_output_counts.assign(def.output_num, 1);
  }

  std::vector<std::vector<uint8_t>> attr_buffers;
  if (!AppendAttrValues(def.attrs, attr_buffers)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  size_t attr_size = 0U;
  auto attr_holder = bg::CreateAttrBuffer(attr_buffers, attr_size);
  GE_ASSERT_NOTNULL(attr_holder, "Failed to build runtime attribute buffer for node %s", def.name.c_str());

  GE_ASSERT_SUCCESS(
      ComputeNodeInfo::CalcSize(ir_input_num, ir_output_num, instance_input_num, instance_output_num, total_size));
  // CNI 之后紧跟属性缓冲：GetAttrs 按 runtime_attr_size_ 定位，布局由两者共同决定。
  size_t full_size = 0U;
  GE_ASSERT_TRUE(!ge::AddOverflow(total_size, attr_size, full_size), "Compute node info size overflow");
  auto holder = ge::ComGraphMakeUnique<uint8_t[]>(full_size);
  GE_ASSERT_NOTNULL(holder);
  GE_ASSERT_EOK(memset_s(holder.get(), full_size, 0, full_size));

  const auto name_id = buffer_pool.AddStr(def.name.c_str());
  const auto type_id = buffer_pool.AddStr(def.type.c_str());
  auto *info = reinterpret_cast<ComputeNodeInfo *>(holder.get());
  info->Init(ir_input_num, ir_output_num, instance_input_num, instance_output_num, attr_size,
             reinterpret_cast<const char *>(name_id), reinterpret_cast<const char *>(type_id));

  SetInstanceInfo(ir_input_counts, *info, true);
  SetInstanceInfo(ir_output_counts, *info, false);

  for (size_t i = 0U; i < instance_input_num; ++i) {
    auto *td = info->MutableInputTdInfo(i);
    GE_ASSERT_NOTNULL(td);
    GE_ASSERT_SUCCESS(FillTd(def.input_descs[i], *td));
  }
  for (size_t i = 0U; i < instance_output_num; ++i) {
    auto *td = info->MutableOutputTdInfo(i);
    GE_ASSERT_NOTNULL(td);
    GE_ASSERT_SUCCESS(FillTd(def.output_descs[i], *td));
  }
  // 属性区不在缓冲区末尾：布局是 输入实例信息 -> 端口描述 -> 运行时属性 -> 输出实例信息。
  // 必须按 CNI 自己的偏移写入，否则 GetAttrs 读到的是未初始化区域。
  // RuntimeAttrs 不是可平凡拷贝的类型，按字节写入而不是整体赋值。
  if (attr_size != 0U) {
    auto *attr_dst = reinterpret_cast<uint8_t *>(info->MutableAttrs());
    GE_ASSERT_NOTNULL(attr_dst);
    const auto offset = static_cast<size_t>(attr_dst - holder.get());
    GE_ASSERT_TRUE(offset + attr_size <= full_size, "Runtime attribute area exceeds the compute node info buffer");
    for (size_t i = 0U; i < attr_size; ++i) {
      attr_dst[i] = attr_holder[i];
    }
  }
  total_size = full_size;
  compute_node_info = std::move(holder);
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
