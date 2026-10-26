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
#include "opskernel/ops_kernel_info_types.h"
#include "graph/utils/args_format_desc_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_tensor.h"
#include "graph/args_format_desc.h"
#include "base/attr/attrs_to_buffer.h"
#include "framework/common/debug/ge_log.h"
#include "common/checker.h"

namespace ge {
namespace {
constexpr uint64_t kAddressLen = sizeof(uint64_t);

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
}  // namespace

int64_t CustomTaskCodeBuilder::ParseOpIndex(const domi::TaskDef &task_def) {
  const domi::KernelDef &kernel_def = task_def.kernel();
  domi::KernelContext context = kernel_def.context();
  return static_cast<int64_t>(context.op_index());
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
  (void)build_data_.semantic.args_table_entry.emplace();
  build_data_.semantic.args_table_entry->table_index = *context.next_args_table_index;
  build_data_.semantic.args_table_entry->args_size = args_size;
  build_data_.semantic.args_table_entry->host_offset = *context.next_host_args_offset;
  args_table_entry_ = &(*build_data_.semantic.args_table_entry);
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

  return {
      ast_.BlankLine(),
      ast_.Comment("construct EagerOpExecutionContext"),
      ast_.VarDecl(allocator, ast_.Call("std::make_shared<AllocatorFaker>", {})),
      ast_.VarDecl(
          eager_context_holder,
          ast_.Call("BuildKernelContextHolder",
                    {op.Arrow("op_name"), op.Arrow("dispatch_info").Attr("custom").Addr(),
                     ast_.Var("", "input_tensors"), ast_.Var("", "output_tensors"), allocator.Attr("get()"), stream})),
      ast_.VarDecl(eager_context,
                   ast_.ReinterpretCast("gert::EagerOpExecutionContext *", eager_context_holder.Attr("context_"))),

      ast_.BlankLine(),
      ast_.Comment("construct EagerExecuteOp"),
      ast_.VarDecl(custom_op_ptr, ast_.Call("ge::CustomOpFactory::CreateOrGetCustomOp",
                                            {op.Arrow("dispatch_info").Attr("custom").Attr("op_type")})),
      ast_.Call("OM2_CHK_NOTNULL", {custom_op_ptr}),
      ast_.VarDecl(eager_execute_op_ptr, ast_.Call("dynamic_cast<ge::EagerExecuteOp *>", {custom_op_ptr})),
      ast_.If(eager_execute_op_ptr == "nullptr",
              {ast_.Call("OM2_LOGE", {ast_.Str("%s is custom op but did not implement EagerExecuteOp."),
                                      eager_context.Arrow("GetNodeType()")}),
               ast_.Return("ACL_ERROR_FAILURE")}),

      ast_.BlankLine(),
      ast_.Comment("execute custom kernel directly"),
      ast_.If(ctx.Attr("launch_func") == "nullptr",
              {
                  ast_.Call("OM2_LOGI", {ast_.Str("DispatchCustomKernel: Start to execute custom kernel directly.")}),
                  ChkStatus(ast_.Call("LaunchEagerExecuteOp", {eager_execute_op_ptr, eager_context})),
                  ast_.Return("ACL_SUCCESS"),
              }),
  };
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
