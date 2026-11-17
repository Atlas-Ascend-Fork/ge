/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "custom_task_code_builder.h"
#include "common/om2/codegen/om2_model_utils.h"
#include "common/om2/codegen/task_code_builder_factory.h"
#include "common/om2/codegen/task_code_builder/task_code_builder_util.h"
#include "common/om2/codegen/task_args_manager/om2_model_args_manager.h"
#include "common/om2/codegen/task_args_manager/om2_model_args_utils.h"
#include "opskernel/ops_kernel_info_types.h"
#include "graph/utils/args_format_desc_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_tensor.h"
#include "graph/args_format_desc.h"
#include "graph/debug/ge_attr_define.h"
#include "base/attr/attrs_to_buffer.h"
#include "framework/common/debug/ge_log.h"
#include "common/checker.h"

namespace ge {
namespace {
constexpr uint64_t kAddressLen = sizeof(uint64_t);

// 生成代码内部 CustomArgsRefreshStrategy 的枚举值。该枚举是 SO 私有的 2 值版本，
// 与 ge::ArgsRefreshStrategy（3 值，含 kAnnotatedArgs）不是同一类型，故此处按数值映射，
// 必须与 stable_part_provider.cc 中 kOpDefStructs 的字符串模板定义保持一致。
constexpr int64_t kStrategyNone = 0;
constexpr int64_t kStrategyCallback = 1;

// 与 gert::bg::AppendAttrTensor 保持一致：tensor 类型属性需要展开为 Tensor 内存布局后再序列化
bool AppendTensorAttr(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  auto val = attr.Get<ge::GeTensor>();
  GE_ASSERT_NOTNULL(val);
  const auto &tensor_desc = val->GetTensorDesc();
  const auto shape_size = tensor_desc.GetShape().GetShapeSize();
  GE_ASSERT_TRUE(shape_size >= 0, "Failed to append tensor attr, shape size less than 0");
  size_t total_size = 0U;
  const auto data_type = tensor_desc.GetDataType();
  const size_t tensor_size = (data_type == ge::DT_STRING)
                                 ? val->GetData().GetSize()
                                 : static_cast<size_t>(ge::GetSizeInBytes(shape_size, data_type));
  auto tensor_holder = gert::Tensor::CreateFollowing(data_type, tensor_size, total_size);
  GE_ASSERT_NOTNULL(tensor_holder);
  auto tensor = ge::PtrToPtr<uint8_t, gert::Tensor>(tensor_holder.get());
  tensor->MutableStorageShape().SetDimNum(tensor_desc.GetShape().GetDimNum());
  for (size_t i = 0U; i < tensor_desc.GetShape().GetDimNum(); ++i) {
    tensor->MutableStorageShape().SetDim(i, tensor_desc.GetShape().GetDim(i));
  }
  tensor->MutableOriginShape().SetDimNum(tensor_desc.GetOriginShape().GetDimNum());
  for (size_t i = 0U; i < tensor_desc.GetOriginShape().GetDimNum(); ++i) {
    tensor->MutableOriginShape().SetDim(i, tensor_desc.GetOriginShape().GetDim(i));
  }
  tensor->SetOriginFormat(tensor_desc.GetOriginFormat());
  tensor->SetStorageFormat(tensor_desc.GetFormat());
  GE_ASSERT_TRUE(total_size >= sizeof(gert::Tensor), "total_size %zu < size of Tensor %zu", total_size,
                 sizeof(gert::Tensor));
  const auto copy_len = total_size - sizeof(gert::Tensor);
  if (copy_len != 0U) {
    GE_ASSERT_TRUE(val->GetData().size() >= copy_len, "tensor attr data size %zu < copy len %zu", val->GetData().size(),
                   copy_len);
    GE_ASSERT_EOK(memcpy_s(tensor->GetData<uint8_t>(), copy_len, val->GetData().GetData(), copy_len));
  }
  std::vector<uint8_t> buf(total_size);
  GE_ASSERT_EOK(memcpy_s(buf.data(), total_size, tensor_holder.get(), total_size));
  attrs.emplace_back(std::move(buf));
  return true;
}

bool AppendAttrTensor(const ge::AnyValue &attr, std::vector<std::vector<uint8_t>> &attrs) {
  if (attr.GetValueType() == ge::AnyValue::VT_TENSOR) {
    return AppendTensorAttr(attr, attrs);
  }
  return gert::bg::AppendAttr(attr, attrs);
}

// 按 IR 原型定义的顺序序列化算子属性，得到可直接拷入 ComputeNodeInfo 属性区的 RuntimeAttrsDef 缓冲
Status SerializeIrAttrs(const OpDescPtr &op_desc, std::vector<uint8_t> &buffer) {
  GE_ASSERT_NOTNULL(op_desc);
  const auto all_attrs = AttrUtils::GetAllAttrs(op_desc);
  const auto &ir_attr_names = op_desc->GetIrAttrNames();
  std::vector<std::vector<uint8_t>> runtime_attrs;
  for (const auto &attr_name : ir_attr_names) {
    const auto iter = all_attrs.find(attr_name);
    if (iter != all_attrs.cend()) {
      GE_ASSERT_TRUE(AppendAttrTensor(iter->second, runtime_attrs), "Append IR attr %s failed, op %s.",
                     attr_name.c_str(), op_desc->GetNamePtr());
      continue;
    }
    const auto &required_attrs = op_desc->GetRequiredAttrWithType();
    GE_ASSERT_TRUE(required_attrs.find(attr_name) == required_attrs.cend(),
                   "Required IR attr %s is missing for op %s(%s).", attr_name.c_str(), op_desc->GetNamePtr(),
                   op_desc->GetTypePtr());
    break;
  }
  size_t attrs_size = 0U;
  auto attr_holder = gert::bg::CreateAttrBuffer(runtime_attrs, attrs_size);
  GE_ASSERT_NOTNULL(attr_holder);
  buffer.assign(attr_holder.get(), attr_holder.get() + attrs_size);
  return SUCCESS;
}

Arg RenderIrInstanceInfoArray(AstBuildContext &ast, const std::vector<CustomIrInstanceInfoSemantic> &infos) {
  if (infos.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> entries;
  entries.reserve(infos.size());
  for (const auto &info : infos) {
    entries.push_back(ast.DesignatedInit({{"instance_start", static_cast<int64_t>(info.instance_start)},
                                          {"instance_num", static_cast<int64_t>(info.instance_num)}},
                                         true));
  }
  return Arg(ast.CCast("const CustomIrInstanceInfo[]", ast.InitList(entries, true)));
}

Arg RenderAttrsArray(AstBuildContext &ast, const std::vector<uint8_t> &attrs) {
  if (attrs.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> elements;
  elements.reserve(attrs.size());
  for (const auto value : attrs) {
    elements.push_back(Arg(value));
  }
  return Arg(ast.CCast("const uint8_t[]", ast.InitList(elements, true)));
}

// B3 产出的来源描述落为只读静态数组。数组顺序与运行期 io_tensors 下标严格对应，
// 故此处不再做任何过滤或重排。
Arg RenderTensorRefreshInfoArray(AstBuildContext &ast, const std::vector<TensorRefreshInfo> &infos) {
  if (infos.empty()) {
    return Arg(nullptr);
  }
  std::vector<Arg> entries;
  entries.reserve(infos.size());
  for (const auto &info : infos) {
    entries.push_back(ast.DesignatedInit(
        {{"source_type", ast.StaticCast("TensorRefreshSource", static_cast<int64_t>(info.source_type))},
         {"source_index", static_cast<int64_t>(info.source_index)},
         {"offset", static_cast<int64_t>(info.offset)}},
        true));
  }
  return Arg(ast.CCast("const TensorRefreshInfo[]", ast.InitList(entries, true)));
}
}  // namespace

int64_t CustomTaskCodeBuilder::ParseOpIndex(const domi::TaskDef &task_def) {
  const domi::KernelDef &kernel_def = task_def.kernel();
  domi::KernelContext context = kernel_def.context();
  return static_cast<int64_t>(context.op_index());
}

Status CustomTaskCodeBuilder::ResolveArgsRefreshStrategy(const OpDescPtr &op_desc,
                                                         ArgsRefreshStrategy &strategy) const {
  // OM2 无历史包袱：策略仅由 ATTR_NAME_CUSTOM_TASK_ARGS_MODE 显式声明，不按 args_format 推导。
  // 属性缺失或 kUnspecified 视为算子未申报刷新，取 kNone。
  int64_t mode = 0;
  if (!AttrUtils::GetInt(op_desc, ATTR_NAME_CUSTOM_TASK_ARGS_MODE, mode)) {
    strategy = ArgsRefreshStrategy::kNone;
    return SUCCESS;
  }
  switch (static_cast<CustomTaskArgsMode>(mode)) {
    case CustomTaskArgsMode::kUpdateCallback:
      strategy = ArgsRefreshStrategy::kUpdateCallback;
      return SUCCESS;
    case CustomTaskArgsMode::kAnnotatedArgs:
      strategy = ArgsRefreshStrategy::kAnnotatedArgs;
      return SUCCESS;
    case CustomTaskArgsMode::kNone:
    case CustomTaskArgsMode::kUnspecified:
      strategy = ArgsRefreshStrategy::kNone;
      return SUCCESS;
    default:
      GELOGE(PARAM_INVALID, "[OM2] Custom op %s has invalid %s value %" PRId64 ".", op_desc->GetNamePtr(),
             ATTR_NAME_CUSTOM_TASK_ARGS_MODE.c_str(), mode);
      return PARAM_INVALID;
  }
}

Status CustomTaskCodeBuilder::ParseTaskRunParam(const domi::TaskDef &task_def, const om2::RuntimeParam &rts_param,
                                                OpDescPtr op_desc, om2::TaskRunParam &task_run_param) {
  (void)task_def;
  GE_CHECK_NOTNULL(op_desc);
  op_desc_ = op_desc;
  GE_ASSERT_SUCCESS(ResolveArgsRefreshStrategy(op_desc, build_data_.args_refresh_strategy));

  // 用 *DataAddrsValue 而非 *AddrsValue：后者经 GetInputOutputDescAddrs 追加 desc 地址，与 Contribute 期
  // Resolve*Addrs 产出的 ordered_args 条目数不一致，会使 B3 的下标对应关系错位。
  // has_optional_addr 与 Resolve*Addrs 对齐：输入侧 false（两侧同样跳过 null desc），
  // 输出侧 true（两侧同为 always-empty 输出压入占位项）。
  std::vector<uint64_t> input_mem_types;
  const auto input_addrs = om2::ModelUtils::GetInputDataAddrsValue(rts_param, op_desc, input_mem_types, false);
  std::vector<uint64_t> output_mem_types;
  const auto output_addrs = om2::ModelUtils::GetOutputDataAddrsValue(rts_param, op_desc, output_mem_types, true);
  std::vector<uint64_t> workspace_mem_types;
  const auto workspace_addrs = om2::ModelUtils::GetWorkspaceDataAddrsValue(rts_param, op_desc, workspace_mem_types);

  GE_ASSERT_TRUE(input_addrs.size() == input_mem_types.size(), "[OM2] Custom op %s input addr/mem_type size mismatch.",
                 op_desc->GetNamePtr());
  GE_ASSERT_TRUE(output_addrs.size() == output_mem_types.size(),
                 "[OM2] Custom op %s output addr/mem_type size mismatch.", op_desc->GetNamePtr());
  GE_ASSERT_TRUE(workspace_addrs.size() == workspace_mem_types.size(),
                 "[OM2] Custom op %s workspace addr/mem_type size mismatch.", op_desc->GetNamePtr());

  // support_refresh 统一填 true（与既有各 builder 一致）：此处只表示"不否决"，最终值由分类器经
  // ConstructTaskInitParams 覆盖。填 false 会使分类器把地址降级为 fixed addr 并申请固定设备内存。
  for (size_t i = 0UL; i < input_addrs.size(); ++i) {
    task_run_param.parsed_input_addrs.push_back({input_addrs[i], input_mem_types[i], true, {0}});
  }
  for (size_t i = 0UL; i < output_addrs.size(); ++i) {
    task_run_param.parsed_output_addrs.push_back({output_addrs[i], output_mem_types[i], true, {0}});
  }
  for (size_t i = 0UL; i < workspace_addrs.size(); ++i) {
    task_run_param.parsed_workspace_addrs.push_back({workspace_addrs[i], workspace_mem_types[i], true, {0}});
  }

  // B4：仅 Callback 申报 reserved 容量，采用 OpDesc 级 I/O 计数口径（不受可选输入是否传入、
  // 动态输入展开数影响）。**不压入 args_descs** —— 后者会驱动 ModelArgsLayoutPlanner 占用普通区，
  // 与"不在普通 args 区占位"的决策冲突。
  if (build_data_.args_refresh_strategy == ArgsRefreshStrategy::kUpdateCallback) {
    const uint64_t io_count = op_desc->GetInputsSize() + op_desc->GetOutputsSize();
    GE_ASSERT_SUCCESS(CheckUint64AddOverflow(io_count, static_cast<uint64_t>(om2::kArgsReserved)));
    const uint64_t slot_count = io_count + static_cast<uint64_t>(om2::kArgsReserved);
    GE_ASSERT_SUCCESS(CheckUint64MulOverflow(slot_count, static_cast<uint64_t>(om2::kArgsFieldSize)));
    task_run_param.reserved_args_len = slot_count * static_cast<uint64_t>(om2::kArgsFieldSize);
    task_run_param.reserved_args_placement = om2::ArgsPlacement::kArgsPlacementHbm;
    GELOGI("[OM2] Custom op %s(%s) callback reserved_args_len=%" PRIu64 " (io=%" PRIu64 ").", op_desc->GetNamePtr(),
           op_desc->GetTypePtr(), task_run_param.reserved_args_len, io_count);
  }
  return SUCCESS;
}

Status CustomTaskCodeBuilder::Init(const domi::TaskDef &task_def,
                                   std::vector<om2::MemAllocation> &logical_mem_allocations, const om2::PisToArgs &args,
                                   const om2::IowAddrs &iow_addrs) {
  (void)task_def;
  (void)args;
  if (build_data_.args_refresh_strategy != ArgsRefreshStrategy::kUpdateCallback) {
    return SUCCESS;
  }
  // 仅 input + output 进入刷新描述：workspace 不产生 io_tensor，也不参与算子私有参数刷新。
  // 排列顺序与 Contribute 期 ordered_args 的前两段严格一致（全部输入、然后全部输出）。
  callback_io_addrs_.clear();
  callback_io_refreshable_.clear();
  callback_io_addrs_.reserve(iow_addrs.input_logic_addrs.size() + iow_addrs.output_logic_addrs.size());
  callback_io_refreshable_.reserve(callback_io_addrs_.capacity());
  for (const auto &addr : iow_addrs.input_logic_addrs) {
    callback_io_addrs_.push_back(addr.logic_addr);
    callback_io_refreshable_.push_back(addr.support_refresh ? 1U : 0U);
  }
  for (const auto &addr : iow_addrs.output_logic_addrs) {
    callback_io_addrs_.push_back(addr.logic_addr);
    callback_io_refreshable_.push_back(addr.support_refresh ? 1U : 0U);
  }
  if (callback_io_addrs_.empty()) {
    return SUCCESS;
  }
  // 复用既有 ArgsIoAddrsUpdater 完成 allocation 匹配，避免重写 match_id 的两处易漏语义
  // （不可刷新时 id 重定向到末位 ABSOLUTE、type 仍取 id_addr_matched）。
  // 必须用 refreshable_flags 重载：mem_types 重载经 IsSuppoprtAddrRefreshable 只看 MemoryAppType，
  // 无法体现 FM 门控（GetRefreshTypeByLogicalAddr 在 is_fm_refresh_enable_ 为假时返回 0）。
  GE_CHECK_NOTNULL(op_desc_);
  const om2::ArgsIoAddrsUpdater::OpInfo op_info{op_desc_->GetName(), op_desc_->GetType()};
  GE_ASSERT_SUCCESS(
      args_io_addrs_updater_.Init(logical_mem_allocations, callback_io_addrs_, callback_io_refreshable_, op_info));
  return SUCCESS;
}

Status CustomTaskCodeBuilder::BuildTensorRefreshInfos(const TaskSemanticContributeContext &context) {
  build_data_.tensor_refresh_infos.clear();
  if (build_data_.args_refresh_strategy != ArgsRefreshStrategy::kUpdateCallback) {
    return SUCCESS;
  }
  GE_ASSERT_NOTNULL(context.model_io);
  GE_ASSERT_NOTNULL(context.args_table);

  const size_t input_num = build_data_.semantic.input_addrs.size();
  const size_t output_num = build_data_.semantic.output_addrs.size();
  // ordered_args 前 input_num + output_num 项与 callback_io_addrs_ 同序。条目数不等说明
  // ParseTaskRunParam 与 Contribute 两条取址路径对可选/常量输入的处理出现分歧，
  // 必须编译期响亮失败而非静默错位。
  GE_ASSERT_TRUE(callback_io_addrs_.size() == (input_num + output_num),
                 "[OM2] Custom op %s callback I/O count mismatch: ParseTaskRunParam=%zu, Contribute=%zu+%zu.",
                 header_.op_name.c_str(), callback_io_addrs_.size(), input_num, output_num);

  std::vector<om2::MemAllocationAndOffset> mem_allocs;
  args_io_addrs_updater_.GetArgsMemAllocationAndOffset(mem_allocs);
  GE_ASSERT_TRUE(mem_allocs.size() == callback_io_addrs_.size(),
                 "[OM2] Custom op %s mem allocation count %zu != io count %zu.", header_.op_name.c_str(),
                 mem_allocs.size(), callback_io_addrs_.size());

  // allocation id -> ModelIoEntry 反查表。正向映射（用户索引 -> allocation id）由 ModelAdapter 建立，
  // 此处按 update_host_args_index 取正向值后反向建表。
  std::unordered_map<uint32_t, const ModelIoEntry *> alloc_id_to_io_entry;
  for (const auto &entry : context.model_io->entries) {
    const auto &ids = entry.is_input ? context.args_table->input_index_to_allocation_ids
                                     : context.args_table->output_index_to_allocation_ids;
    if (entry.update_host_args_index >= ids.size()) {
      // 越界守卫：异常模型（重复 ATTR_NAME_INDEX 等）下两侧条目数可能不等，跳过而非越界读
      GELOGW("[OM2] Custom op %s model io entry index %u out of range (size %zu), skip.", header_.op_name.c_str(),
             entry.update_host_args_index, ids.size());
      continue;
    }
    const uint32_t alloc_id = ids[entry.update_host_args_index];
    if (alloc_id == UINT32_MAX) {
      continue;  // 非零拷贝 I/O，无对应 allocation
    }
    (void)alloc_id_to_io_entry.emplace(alloc_id, &entry);
  }

  // 多段 FM 的 segment_offset 口径尚未收敛，本期仅支持单段；多段场景保守落 KeepInitial。
  const size_t fm_segment_num = context.args_table->refreshable_fm_index_and_allocation_ids.size();

  for (size_t k = 0UL; k < callback_io_addrs_.size(); ++k) {
    // 与 RenderDispatchLoop 的 switch 保持同一谓词：仅这 4 类参数产生 io_tensor，
    // 故 TensorRefreshInfo 数组与运行期 io_tensors 下标严格对应。
    const auto arg_type = static_cast<OpArgType>(build_data_.ordered_args[k].type);
    if ((arg_type != OP_ARG_INPUT) && (arg_type != OP_ARG_OUTPUT) && (arg_type != OP_ARG_CONST_TENSOR) &&
        (arg_type != OP_ARG_VAR_TENSOR)) {
      continue;
    }
    TensorRefreshInfo info{};
    const auto &alloc = mem_allocs[k];
    if (callback_io_refreshable_[k] == 0U) {
      // 门控优先于来源判断：最终不可刷新一律保留加载期地址，否则运行期会去取本轮用户地址，
      // 而该地址与加载时绑定的并非同一块内存。
      info.source_type = TensorRefreshSource::KeepInitial;
    } else if (alloc.type == om2::MemAllocation::FEATURE_MAP) {
      if (fm_segment_num == 1UL) {
        info.source_type = TensorRefreshSource::FeatureMap;
        info.offset = alloc.offset;  // 单段 FM：segment_offset 恒为 0
      } else {
        GELOGW(
            "[OM2] Custom op %s has %zu FM segments, multi-segment FM refresh unsupported, "
            "io[%zu] falls back to KeepInitial.",
            header_.op_name.c_str(), fm_segment_num, k);
        info.source_type = TensorRefreshSource::KeepInitial;
      }
    } else {
      const auto it = alloc_id_to_io_entry.find(static_cast<uint32_t>(alloc.id));
      if ((it != alloc_id_to_io_entry.end()) && (it->second->addr_offset >= 0)) {
        info.source_type = it->second->is_input ? TensorRefreshSource::ModelInput : TensorRefreshSource::ModelOutput;
        info.source_index = it->second->index;
        info.offset = alloc.offset + static_cast<uint64_t>(it->second->addr_offset);
      } else {
        // 回退 ABSOLUTE / 固定规划区：保留加载期真实承载
        info.source_type = TensorRefreshSource::KeepInitial;
      }
    }
    (void)build_data_.tensor_refresh_infos.emplace_back(info);
  }
  GELOGI("[OM2] Custom op %s built %zu tensor refresh infos (io=%zu, fm_segments=%zu).", header_.op_name.c_str(),
         build_data_.tensor_refresh_infos.size(), callback_io_addrs_.size(), fm_segment_num);
  return SUCCESS;
}

Status CustomTaskCodeBuilder::Contribute(TaskSemanticContributeContext &context) {
  GE_ASSERT_SUCCESS(TaskCodeBuilder::Contribute(context));
  GE_ASSERT_NOTNULL(context.next_args_table_index);
  GE_ASSERT_NOTNULL(context.next_host_args_offset);
  GE_ASSERT_NOTNULL(context.op_desc);

  build_data_.semantic.task_type = context.task_type;
  build_data_.semantic.kernel_type = static_cast<ccKernelType>(context.task_def.kernel().context().kernel_type());
  build_data_.semantic.prof_ge_task_type =
      TaskCodeBuilderUtil::ConvertToProfilingTaskType(context.op_desc, context.task_def);
  build_data_.semantic.op_impl_mode = TaskCodeBuilderUtil::GetOpImplMode(context.op_desc);
  build_data_.task_index = context.task_index;

  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveWorkspaceAddrs(context, build_data_.semantic.workspace_addrs));
  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveInputAddrs(context, build_data_.semantic.input_addrs));
  GE_ASSERT_SUCCESS(Om2ModelUtils::ResolveOutputAddrs(context, true, build_data_.semantic.output_addrs));
  AssignTaskLocalIoNames();
  GE_ASSERT_SUCCESS(BuildCustomKernelIrInfo(context.op_desc));

  dispatch_type_ = OpDispatchType::DISPATCH_CUSTOM_KERNEL;

  // parse args format
  std::vector<ArgDesc> arg_descs;
  domi::KernelContext kernel_context = context.task_def.kernel().context();
  GE_ASSERT_SUCCESS(ArgsFormatDesc::Parse(context.op_desc, kernel_context.args_format(), arg_descs),
                    "[OM2] Formatted args [%s] parsed failed.", kernel_context.args_format().c_str());

  // calc args size
  size_t args_size = 0U;
  for (const auto &arg_desc : arg_descs) {
    // 变长自定义数据的字节存于 kernel_ext_info，需在加载期分配 device 内存并上传，OM2 出图路径
    // 无对应能力。注册 pattern 后 ArgsFormatDesc::Parse 必然成功，若不在此显式拒绝，args_size 会累加
    // CUSTOM_DATA_BLOCK 的 8 字节而 ordered_args 仅由 input/output/workspace 构成，导致静默生成含未初始化槽的 args
    GE_ASSERT_TRUE(arg_desc.addr_type != AddrType::CUSTOM_DATA_BLOCK,
                   "[OM2] Custom op %s(%s) uses variable-length custom data, which is not supported in the OM2 "
                   "codegen path.",
                   context.op_desc->GetNamePtr(), context.op_desc->GetTypePtr());
    (void)ArgsFormatDesc::GetArgSize(context.op_desc, arg_desc, args_size);
  }
  InitArgsTableEntry(context, args_size);

  // update values of context.next_args_table_index and context.next_host_args_offset
  if (build_data_.semantic.args_table_entry.has_value()) {
    ++(*context.next_args_table_index);
    *context.next_host_args_offset +=
        Om2ModelUtils::ArgsSizeAlign8(static_cast<size_t>(build_data_.semantic.args_table_entry->args_size));
  }

  // construct build_data_.ordered_args
  uint64_t current_args_offset = 0U;
  auto append_args = [this, &current_args_offset](const std::vector<AddrSemantic> &addrs) {
    for (const auto &addr : addrs) {
      OpArgDesc arg = TaskCodeBuilderUtil::ConvertAddrDesc(addr);
      arg.args_offset = current_args_offset;
      current_args_offset += kAddressLen;
      build_data_.ordered_args.push_back(std::move(arg));
    }
  };
  append_args(build_data_.semantic.input_addrs);
  append_args(build_data_.semantic.output_addrs);
  append_args(build_data_.semantic.workspace_addrs);

  // B3 来源转换：必须在 ordered_args 构建完成后执行，二者按同一下标空间对齐。
  GE_ASSERT_SUCCESS(BuildTensorRefreshInfos(context));

  return SUCCESS;
}

Status CustomTaskCodeBuilder::RenderDistHelper(std::vector<DeclNode *> &items) {
  auto op = ast_.Var("const TaskDispatchInfo *", "op");
  auto ctx = ast_.Var("const DispatchOpContext &", "ctx");
  GE_ASSERT_SUCCESS(RenderDispatchCustomKernel(op, ctx, items));
  return SUCCESS;
}

Status CustomTaskCodeBuilder::RenderOpDefTableFields(std::vector<std::pair<std::string, Arg>> &fields) {
  GELOGI("[OM2] BuildOpDefTable: op=%s, func_idx=%u", header_.op_name.c_str(),
         build_data_.semantic.launch.func_handle_index);
  fields.push_back({"dispatch_type", ast_.StaticCast("OpDispatchType", static_cast<int64_t>(dispatch_type_))});
  fields.push_back({"op_name", Arg::StringLiteral(header_.op_name)});

  auto custom_fields = std::vector<std::pair<std::string, Arg>>{
      {"args_info", TaskCodeBuilderUtil::RenderOpArgDesc(ast_, build_data_.ordered_args)},
      {"args_info_num", static_cast<int64_t>(build_data_.ordered_args.size())},
      {"op_type", Arg::StringLiteral(header_.op_type)},
      {"args_idx", static_cast<int64_t>(build_data_.semantic.args_table_entry->table_index)},
      {"stream_id", static_cast<uint32_t>(header_.stream_id)},
      {"task_type", static_cast<int64_t>(build_data_.semantic.task_type)},
      {"prof_ge_task_type", static_cast<int64_t>(build_data_.semantic.prof_ge_task_type)},
      {"op_impl_mode", static_cast<int64_t>(build_data_.semantic.op_impl_mode)},
      {"ir_inputs_num", static_cast<int64_t>(build_data_.custom_kernel.ir_inputs_num)},
      {"input_instance_info", RenderIrInstanceInfoArray(ast_, build_data_.custom_kernel.input_instance_infos)},
      {"ir_outputs_num", static_cast<int64_t>(build_data_.custom_kernel.ir_outputs_num)},
      {"output_instance_info", RenderIrInstanceInfoArray(ast_, build_data_.custom_kernel.output_instance_infos)},
      {"attrs", RenderAttrsArray(ast_, build_data_.custom_kernel.attrs)},
      {"attrs_size", static_cast<int64_t>(build_data_.custom_kernel.attrs.size())},
      // Callback 地址刷新：尾部追加，与 CustomDispatchInfo 字符串模板同步（两处漏改任一处会静默
      // 产生缺字段的初始化列表）。非 Callback 任务策略为 None、数组为空指针、长度为 0。
      {"args_refresh_strategy",
       ast_.StaticCast("CustomArgsRefreshStrategy",
                       static_cast<int64_t>(build_data_.args_refresh_strategy == ArgsRefreshStrategy::kUpdateCallback
                                                ? kStrategyCallback
                                                : kStrategyNone))},
      {"task_index", static_cast<int64_t>(build_data_.task_index)},
      {"tensor_refresh_infos", RenderTensorRefreshInfoArray(ast_, build_data_.tensor_refresh_infos)},
      {"tensor_refresh_infos_num", static_cast<int64_t>(build_data_.tensor_refresh_infos.size())},
  };
  auto custom_dispatch = ast_.DesignatedInit({{"custom", ast_.DesignatedInit(custom_fields)}});
  (void)fields.emplace_back("dispatch_info", custom_dispatch);

  return SUCCESS;
}

std::string CustomTaskCodeBuilder::GetFuncName() const {
  return kDispatchFuncName;
}

void CustomTaskCodeBuilder::AssignTaskLocalIoNames() {
  const std::string task_prefix = "op" + std::to_string(header_.op_index);
  for (size_t i = 0U; i < build_data_.semantic.input_addrs.size(); ++i) {
    if (build_data_.semantic.input_addrs[i].tensor_info.has_value()) {
      build_data_.semantic.input_addrs[i].symbol_hint = task_prefix + "_input" + std::to_string(i);
    }
  }
  for (size_t i = 0U; i < build_data_.semantic.output_addrs.size(); ++i) {
    if (build_data_.semantic.output_addrs[i].tensor_info.has_value()) {
      build_data_.semantic.output_addrs[i].symbol_hint = task_prefix + "_output" + std::to_string(i);
    }
  }
}

Status CustomTaskCodeBuilder::BuildCustomKernelIrInfo(const OpDescPtr &op_desc) {
  GE_ASSERT_NOTNULL(op_desc);
  auto &custom_kernel = build_data_.custom_kernel;

  const size_t ir_inputs_num = op_desc->GetIrInputs().size();
  const auto input_ir_map = OpDescUtils::GetInputIrIndexes2InstanceIndexesPairMap(op_desc);
  if ((ir_inputs_num > 0U) && (!input_ir_map.empty())) {
    custom_kernel.ir_inputs_num = static_cast<uint32_t>(ir_inputs_num);
    custom_kernel.input_instance_infos.reserve(ir_inputs_num);
    uint32_t instance_start = 0U;
    for (size_t i = 0U; i < ir_inputs_num; ++i) {
      const auto iter = input_ir_map.find(i);
      const uint32_t instance_num = (iter != input_ir_map.cend()) ? static_cast<uint32_t>(iter->second.second) : 0U;
      custom_kernel.input_instance_infos.push_back({instance_start, instance_num});
      instance_start += instance_num;
    }
  } else {
    // 无 IR 原型信息时回退为“一对一”映射，保证执行期仍可解析输入
    const size_t fallback_num = build_data_.semantic.input_addrs.size();
    custom_kernel.ir_inputs_num = static_cast<uint32_t>(fallback_num);
    custom_kernel.input_instance_infos.reserve(fallback_num);
    for (size_t i = 0U; i < fallback_num; ++i) {
      custom_kernel.input_instance_infos.push_back({static_cast<uint32_t>(i), 1U});
    }
  }

  const size_t ir_outputs_num = op_desc->GetIrOutputs().size();
  const auto output_ir_map = OpDescUtils::GetOutputIrIndexes2InstanceIndexesPairMap(op_desc);
  if ((ir_outputs_num > 0U) && (!output_ir_map.empty())) {
    custom_kernel.ir_outputs_num = static_cast<uint32_t>(ir_outputs_num);
    custom_kernel.output_instance_infos.reserve(ir_outputs_num);
    uint32_t instance_start = 0U;
    for (size_t i = 0U; i < ir_outputs_num; ++i) {
      const auto iter = output_ir_map.find(i);
      const uint32_t instance_num = (iter != output_ir_map.cend()) ? static_cast<uint32_t>(iter->second.second) : 0U;
      custom_kernel.output_instance_infos.push_back({instance_start, instance_num});
      instance_start += instance_num;
    }
  } else {
    const size_t fallback_num = build_data_.semantic.output_addrs.size();
    custom_kernel.ir_outputs_num = static_cast<uint32_t>(fallback_num);
    custom_kernel.output_instance_infos.reserve(fallback_num);
    for (size_t i = 0U; i < fallback_num; ++i) {
      custom_kernel.output_instance_infos.push_back({static_cast<uint32_t>(i), 1U});
    }
  }

  GE_ASSERT_SUCCESS(SerializeIrAttrs(op_desc, custom_kernel.attrs));
  GELOGI("[OM2] Custom kernel IR info: op=%s, ir_inputs_num=%u, ir_outputs_num=%u, attrs_size=%zu",
         header_.op_name.c_str(), custom_kernel.ir_inputs_num, custom_kernel.ir_outputs_num,
         custom_kernel.attrs.size());
  return SUCCESS;
}

void CustomTaskCodeBuilder::InitArgsTableEntry(const TaskSemanticContributeContext &context, const uint64_t args_size) {
  args_table_entry_ =
      TaskCodeBuilderUtil::InitArgsTableEntry(context, build_data_.semantic.args_table_entry, args_size);
}

Status CustomTaskCodeBuilder::RenderDispatchCustomKernel(const VarRef &op, const VarRef &ctx,
                                                         std::vector<DeclNode *> &items) const {
  std::vector<BodyItem> body;
  auto setup = RenderDispatchSetup(op, ctx);
  (void)body.insert(body.end(), setup.begin(), setup.end());
  body.push_back(RenderDispatchLoop(op, ctx));
  auto distribution = RenderDistribution(op, ctx);
  (void)body.insert(body.end(), distribution.begin(), distribution.end());
  auto launch_callback = HandleExecuteCallback(op, ctx);
  (void)body.insert(body.end(), launch_callback.begin(), launch_callback.end());
  return TaskCodeBuilderUtil::RenderDispatchFunc(ast_, "DispatchCustomKernel", body, items);
}

std::vector<BodyItem> CustomTaskCodeBuilder::RenderDispatchSetup(const VarRef &op, const VarRef &ctx) const {
  return {
      ast_.VarDecl(
          ast_.Var("ArgsInfo *", "args_info"),
          ctx.Attr("args_table").Attr("GetArgsInfo")(op.Arrow("dispatch_info").Attr("custom").Attr("args_idx"))),
      ChkNotNull(ast_.Var("", "args_info")),
      // -- 声明 ordered_io_addrs 和 Report IO 向量 --
      ast_.VarDecl(ast_.Var("std::vector<gert::Tensor*>", "input_tensors")),
      ast_.VarDecl(ast_.Var("std::vector<gert::Tensor*>", "output_tensors")),
      ast_.VarDecl(ast_.Var("std::vector<gert::Tensor>", "io_tensors")),
      ast_.Call(
          "",
          {ast_.Var("", "io_tensors").Attr("reserve")(op.Arrow("dispatch_info").Attr("custom").Attr("args_info_num"))}),
      ast_.VarDecl(ast_.Var("std::vector<GertModelTaskIoEntry>", "report_inputs")),
      ast_.VarDecl(ast_.Var("std::vector<GertModelTaskIoEntry>", "report_outputs")),
      ast_.VarDecl(ast_.Var("std::vector<uint64_t>", "report_workspace_addrs")),
      ast_.VarDecl(ast_.Var("std::vector<uint64_t>", "report_workspace_sizes")),
  };
}

BodyItem CustomTaskCodeBuilder::RenderDispatchLoop(const VarRef &op, const VarRef &ctx) const {
  auto a = ast_.Var("const auto &", "a");
  return ast_.For(ast_.VarDecl("uint32_t", "j", ast_.UInt(0)),
                  ast_.Var("", "j") < op.Arrow("dispatch_info").Attr("custom").Attr("args_info_num"),
                  ast_.PostInc(ast_.Var("", "j")),
                  std::initializer_list<BodyItem>{
                      ast_.VarDecl(a, op.Arrow("dispatch_info").Attr("custom").Attr("args_info")[ast_.Var("", "j")]),
                      ast_.VarDecl(ast_.Var("uint64_t", "_addr"), ast_.UInt(0)),
                      ast_.Switch(ast_.Var("", "a").Attr("type"),
                                  std::vector<BodyItem>{
                                      // INPUT / OUTPUT / CONST_TENSOR → 共享 handler（内部根据 a.type 区分）
                                      ast_.Case(ast_.Var("", "OP_ARG_INPUT")),
                                      ast_.Case(ast_.Var("", "OP_ARG_OUTPUT")),
                                      ast_.Case(ast_.Var("", "OP_ARG_CONST_TENSOR")),
                                      ast_.Case(ast_.Var("", "OP_ARG_VAR_TENSOR")),
                                      ast_.Block(HandleInputOutputArg(a, ctx)),
                                      ast_.Case(Arg(nullptr)),
                                      ast_.Block({
                                          ast_.Break(),
                                      }),
                                  }),
                  });
}

std::vector<BodyItem> CustomTaskCodeBuilder::RenderDistribution(const VarRef &op, const VarRef &ctx) const {
  auto custom = op.Arrow("dispatch_info").Attr("custom");
  auto stream = ctx.Attr("stream_list")[custom.Attr("stream_id")];
  auto allocator = ast_.Var("auto", "allocator");
  auto eager_context_holder = ast_.Var("auto", "eager_context_holder");
  auto eager_context = ast_.Var("auto", "eager_context");
  auto custom_op_ptr = ast_.Var("auto", "custom_op_ptr");
  auto eager_execute_op_ptr = ast_.Var("auto", "eager_execute_op_ptr");

  const bool is_callback = (build_data_.args_refresh_strategy == ArgsRefreshStrategy::kUpdateCallback);
  std::vector<BodyItem> items;
  items.push_back(ast_.BlankLine());
  items.push_back(ast_.Comment("construct EagerOpExecutionContext"));

  // workspace 容器与 args handler 的所有权在调用方。非 Callback 任务用分发函数内的栈上局部量，
  // 生命周期覆盖本次 Execute；Callback 任务改由 CustomTaskState 持久持有，供运行期逐轮刷新复用。
  Arg allocator_arg;
  Arg ws_blocks_arg;
  Arg handler_arg;
  if (is_callback) {
    // Handler 必须先于 BuildKernelContextHolder 创建：它是 context 的 kArgsHandler 附加输出槽内容。
    items.push_back(ast_.VarDecl(ast_.Var("void *", "args_handler_raw"), nullptr));
    items.push_back(ast_.Call("OM2_CHK_NOTNULL", {ctx.Attr("create_args_handler")}));
    items.push_back(
        ChkStatus(ast_.Var("", "ctx.create_args_handler(&ctx.args_table, AllocateOm2ArgsBuffer, &args_handler_raw)")));
    items.push_back(ast_.Call("OM2_CHK_NOTNULL", {ast_.Var("", "args_handler_raw")}));
    items.push_back(ast_.VarDecl(ast_.Var("auto", "task_state"), ast_.Call("std::make_unique<CustomTaskState>", {})));
    items.push_back(ast_.Var("",
                             "task_state->allocator = std::make_unique<SinkOnlyAllocator>("
                             "ctx.dev_dynamic_mem_ptrs)"));
    // Deleter 按值缓存 destroy 回调，故不依赖 callbacks_ 存活；失败提前返回时由 unique_ptr 自动销毁 Handler
    items.push_back(ast_.Var("",
                             "task_state->args_handler = Om2ArgsHandlerHandle(args_handler_raw, "
                             "Om2ArgsHandlerDeleter{ctx.destroy_args_handler})"));
    allocator_arg = Arg(ast_.Var("", "task_state->allocator.get()"));
    ws_blocks_arg = Arg(ast_.Var("", "&task_state->workspace_blocks"));
    handler_arg = Arg(ast_.Var("", "args_handler_raw"));
  } else {
    items.push_back(ast_.VarDecl(ast_.Var("std::vector<gert::GertMemBlock *>", "workspace_blocks")));
    items.push_back(
        ast_.VarDecl(allocator, ast_.Call("std::make_unique<SinkOnlyAllocator>", {ctx.Attr("dev_dynamic_mem_ptrs")})));
    allocator_arg = Arg(allocator.Attr("get()"));
    ws_blocks_arg = Arg(ast_.Var("", "&workspace_blocks"));
    handler_arg = Arg(ast_.Var("", "nullptr"));
  }

  items.push_back(ast_.VarDecl(
      eager_context_holder,
      ast_.Call("BuildKernelContextHolder",
                {op.Arrow("op_name"), op.Arrow("dispatch_info").Attr("custom").Addr(), ast_.Var("", "input_tensors"),
                 ast_.Var("", "output_tensors"), allocator_arg, stream, ws_blocks_arg, handler_arg})));
  items.push_back(ast_.VarDecl(
      eager_context, ast_.ReinterpretCast("gert::EagerOpExecutionContext *", eager_context_holder.Attr("context_"))));

  items.push_back(ast_.BlankLine());
  items.push_back(ast_.Comment("construct EagerExecuteOp"));
  items.push_back(ast_.VarDecl(custom_op_ptr, ast_.Call("ge::CustomOpFactory::CreateOrGetCustomOp",
                                                        {op.Arrow("dispatch_info").Attr("custom").Attr("op_type")})));
  items.push_back(ast_.Call("OM2_CHK_NOTNULL", {custom_op_ptr}));
  items.push_back(ast_.VarDecl(eager_execute_op_ptr, ast_.Call("dynamic_cast<ge::EagerExecuteOp *>", {custom_op_ptr})));
  items.push_back(ast_.If(eager_execute_op_ptr == "nullptr",
                          {ast_.Call("OM2_LOGE", {ast_.Str("%s is custom op but did not implement EagerExecuteOp."),
                                                  eager_context.Arrow("GetNodeType()")}),
                           ast_.Return("ACL_ERROR_FAILURE")}));

  if (is_callback) {
    items.push_back(ast_.BlankLine());
    items.push_back(ast_.Comment("persist per-task state for runtime args refresh"));
    // 加载期二次能力校验：编译期属性只保证 ArgsUpdater（GetArgsRefreshStrategy 不校验 EagerExecuteOp），
    // 此处再确认算子确实实现了 ArgsUpdater，否则运行期逐轮刷新无从回调。
    items.push_back(ast_.Var("", "task_state->args_updater = dynamic_cast<ge::ArgsUpdater *>(custom_op_ptr)"));
    items.push_back(ast_.Call("OM2_CHK_NOTNULL", {ast_.Var("", "task_state->args_updater")}));
    // move 转移 buffer 所有权，已有 Tensor 元素地址保持有效，故 context 中指向 io_tensors 元素的
    // 指针、以及 report_inputs/report_outputs 里的 GertModelTaskIoEntry 在 move 后仍然有效。
    items.push_back(ast_.Var("", "task_state->context_holder = std::move(eager_context_holder)"));
    items.push_back(ast_.Var("", "task_state->io_tensors = std::move(io_tensors)"));
    items.push_back(ast_.Call("OM2_CHK_NOTNULL", {ctx.Attr("custom_tasks")}));
    items.push_back(ast_.VarDecl(ast_.Var("const uint32_t", "task_index"),
                                 op.Arrow("dispatch_info").Attr("custom").Attr("task_index")));
    items.push_back(ast_.Call("OM2_CHK_TRUE", {ast_.Var("", "task_index < ctx.custom_tasks->size()")}));
    items.push_back(ast_.Var("",
                             "(*ctx.custom_tasks)[task_index] = CustomTaskStatePtr(task_state.release(), "
                             "CustomTaskStateDeleter{DestroyCustomTaskState})"));
  }

  items.push_back(ast_.BlankLine());
  items.push_back(ast_.Comment("execute custom kernel directly"));
  items.push_back(
      ast_.If(ctx.Attr("launch_func") == "nullptr",
              {
                  ast_.Call("OM2_LOGI", {ast_.Str("DispatchCustomKernel: Start to execute custom kernel directly.")}),
                  ChkStatus(ast_.Call("LaunchEagerExecuteOp", {eager_execute_op_ptr, eager_context})),
                  ast_.Return("ACL_SUCCESS"),
              }));
  return items;
}

std::vector<BodyItem> CustomTaskCodeBuilder::HandleExecuteCallback(const VarRef &op, const VarRef &ctx) const {
  auto custom = op.Arrow("dispatch_info").Attr("custom");
  auto task_type = custom.Attr("task_type");
  auto stream = ctx.Attr("stream_list")[custom.Attr("stream_id")];
  auto launch_params = ast_.Var("GertModelTaskLaunchParams", "launch_params");
  auto eager_context = ast_.Var("auto", "eager_context");
  auto eager_execute_op_ptr = ast_.Var("auto", "eager_execute_op_ptr");

  return {
      ast_.BlankLine(),
      ast_.Comment("execute custom kernel by callback"),
      ast_.VarDecl(ast_.Var("GertModelTaskDesc", "task_info")),
      ChkStatus(ast_.Call("AssembleOm2TaskInfo",
                          {ast_.Var("", "task_info").Addr(),
                           op.Arrow("op_name"),
                           custom.Attr("op_type"),
                           ast_.UInt(0U),
                           custom.Attr("stream_id"),
                           ast_.UInt(0U),  // block-dim
                           ast_.UInt(0U),
                           ast_.ReinterpretCast("uintptr_t", ast_.Var("", "args_info").Arrow("dev_addr")),
                           ast_.Var("", "args_info").Arrow("size"),
                           ast_.Var("", "report_inputs").Data(),
                           ast_.StaticCast("uint64_t", ast_.Var("", "report_inputs").Size()),
                           ast_.Var("", "report_outputs").Data(),
                           ast_.StaticCast("uint32_t", ast_.Var("", "report_outputs").Size()),
                           ast_.Var("", "report_workspace_addrs").Data(),
                           ast_.Var("", "report_workspace_sizes").Data(),
                           ast_.StaticCast("uint32_t", ast_.Var("", "report_workspace_addrs").Size()),
                           task_type,
                           stream,
                           ast_.UInt(0U),
                           ast_.UInt(0U),
                           Arg(nullptr),
                           ast_.ULong(0U),
                           ast_.ULong(0U),
                           ast_.ULong(0U),
                           ast_.ULong(0U),
                           custom.Attr("op_impl_mode"),
                           custom.Attr("prof_ge_task_type"),
                           ast_.UInt(0U)})),
      ChkStatus(ast_.Call("aclrtStreamGetId",
                          {ast_.Var("", "task_info").Attr("stream"),
                           ast_.ReinterpretCast("int32_t *", ast_.Var("", "task_info").Attr("stream_id").Addr())})),
      ast_.Assign(ast_.Var("", "task_info").Attr("task_raw_info"), Arg(nullptr)),

      ast_.VarDecl(launch_params, ast_.InitList({})),
      ast_.Assign(launch_params.Attr("launch_custom_kernel_params").Attr("func_launch_custom_kernel"),
                  ast_.Var("", "LaunchEagerExecuteOp")),
      ast_.Assign(launch_params.Attr("launch_custom_kernel_params").Attr("eager_op"), eager_execute_op_ptr),
      ast_.Assign(launch_params.Attr("launch_custom_kernel_params").Attr("eager_op_context"), eager_context),

      ast_.VarDecl(ast_.Var("GertModelTaskLaunchInfo", "launch_info"),
                   ast_.DesignatedInit({{"launch_type", ast_.Var("", "ACL_RT_LAUNCH_CUSTOM_KERNEL")},
                                        {"task_info", ast_.Var("", "task_info").Addr()},
                                        {"launch_params", launch_params.Addr()}})),
      ast_.Call("OM2_LOGI", {ast_.Str("DispatchCustomKernel: Start to execute custom kernel launch callback.")}),
      ChkStatus(ast_.Call("ctx.launch_func", {ctx.Attr("instance_handle"), ast_.Var("", "launch_info").Addr()})),
  };
}

std::vector<BodyItem> CustomTaskCodeBuilder::HandleInputOutputArg(const VarRef &a, const VarRef &ctx) const {
  return {
      ast_.Assign(
          ast_.Var("", "_addr"),
          ast_.ReinterpretCast("uint64_t",
                               ast_.Call("ResolveOpAddr", {a.Attr("addr").Attr("mem_src"), a.Attr("addr").Attr("index"),
                                                           a.Attr("addr").Attr("offset"), ctx.Attr("total_dev_mem_ptr"),
                                                           ctx.Attr("session_scope_mem_ptr"), ctx.Attr("constants"),
                                                           ctx.Attr("var_addrs")}))),
      ast_.Var("", "io_tensors")
          .PushBack(ast_.Call(
              "BuildTensor",
              {ast_.ReinterpretCast("void *", ast_.Var("", "_addr")), a.Attr("data").Attr("tensor").Attr("size"),
               a.Attr("data").Attr("tensor").Attr("data_type"), a.Attr("data").Attr("tensor").Attr("format"),
               a.Attr("data").Attr("tensor").Attr("shape"), a.Attr("data").Attr("tensor").Attr("shape_dims")})),
      ast_.VarDecl(
          ast_.Var("GertModelTaskIoEntry", "_entry"),
          ast_.InitList({ast_.Var("", "sizeof(GertModelTaskIoEntry)"), ast_.Var("", "io_tensors").Attr("back")().Addr(),
                         a.Attr("data").Attr("tensor").Attr("args_offset")})),
      ast_.If(a.Attr("type") == ast_.Var("", "OP_ARG_INPUT") || a.Attr("type") == ast_.Var("", "OP_ARG_CONST_TENSOR"),
              {
                  ast_.Var("", "report_inputs").PushBack(ast_.Var("", "_entry")),
                  ast_.Var("", "input_tensors").PushBack(ast_.Var("", "io_tensors").Attr("back")().Addr()),
              },
              {
                  ast_.Var("", "report_outputs").PushBack(ast_.Var("", "_entry")),
                  ast_.Var("", "output_tensors").PushBack(ast_.Var("", "io_tensors").Attr("back")().Addr()),
              }),
      ast_.Break(),
  };
}

REGISTER_TASK_CODE_BUILDER(MODEL_TASK_CUSTOM_KERNEL, CustomTaskCodeBuilder);
}  // namespace ge
