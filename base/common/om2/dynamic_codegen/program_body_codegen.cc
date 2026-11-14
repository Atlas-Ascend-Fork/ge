/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/dynamic_codegen/program_body_codegen.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "securec.h"
#include "common/checker.h"
#include "base/runtime/runtime_attrs_def.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "exe_graph/runtime/context_extend.h"
#include "exe_graph/runtime/continuous_buffer.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "framework/runtime/model_desc.h"
#include "engine/aicore/launch_kernel/rt_kernel_launch_args_ex.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/gert_model_data_utils.h"
#include "kernel/tensor_attr.h"
#include "common/om2/codegen/emitter/stable_parts/stable_part_provider.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/multithread_program_plan.h"
#include "core/executor/multi_thread_topological/executor/schedule/producer/producers/kernel_tags/kernel_tags.h"

namespace ge {
std::string_view GetOm2ModelApiHeader();
std::string_view GetOm2ModelApiUtilsHeader();
std::string_view GetOm2KernelBinHeader();
std::string_view GetOm2HostResourceHeader();
}  // namespace ge

namespace gert {
namespace om2 {
namespace {
using ge::Arg;
using ge::AstBuildContext;
using ge::AstContext;
using ge::CppEmitter;
using ge::DeclNode;
using ge::Expr;
using ge::FieldDecl;
using ge::FunctionDef;
using ge::IncludeDecl;
using ge::InitListExpr;
using ge::MethodDef;

struct DynamicChainCodegenInfo {
  uint32_t index;
  uint64_t symbol;
  ChainSourceKind source_kind;
  ChainBindingPhase binding_phase;
  ChainBindingKind binding_kind;
  int64_t source_index;
  std::string cpp_symbol;
  std::string resource_key;
};

struct DynamicConstChainCodegenInfo {
  uint32_t chain_index;
  std::string producer_name;
  Om2ConstValue value;
  bool custom = false;
  std::string initializer;
  std::vector<uint8_t> payload;
  bool payload_is_string = true;
};

struct BinaryBindingCodegenInfo {
  uint32_t chain_index;
  std::string key;
  bool is_ge_kernel_bin;
};

struct DynamicKernelCodegenInfo {
  std::string cpp_symbol;
  std::string execution_node_name;
  std::string kernel_type;
  std::vector<KernelPortInfo> inputs;
  std::vector<KernelPortInfo> outputs;
  uint32_t compute_node_index;
  OutputCreatePolicy output_policy;
  KernelImplType impl_type;
  bool require_compute_node_info;
  std::string generated_function;
  std::string generated_outputs_creator_code;
};

struct TensorDescCodegenInfo {
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  uint64_t expand_dims_size;
  uint64_t expand_dims_mask;
  bool exists;
};

struct AnchorCodegenInfo {
  size_t instance_start;
  size_t instance_num;
};

struct ComputeNodeCodegenInfo {
  std::string node_name;
  std::string node_type;
  size_t ir_input_num;
  size_t ir_output_num;
  std::vector<uint8_t> attrs;
  std::vector<AnchorCodegenInfo> input_instances;
  std::vector<AnchorCodegenInfo> output_instances;
  std::vector<TensorDescCodegenInfo> input_descs;
  std::vector<TensorDescCodegenInfo> output_descs;
};

struct KernelExtendCodegenInfo {
  std::string kernel_name;
  std::string kernel_type;
};

struct ShapeCodegenInfo {
  size_t dim_num;
  std::vector<int64_t> dims;
};

struct ShapeRangeCodegenInfo {
  ShapeCodegenInfo min;
  ShapeCodegenInfo max;
};

struct ModelIoCodegenInfo {
  std::string name;
  int32_t data_type;
  int32_t storage_format;
  int32_t origin_format;
  ShapeCodegenInfo storage_shape;
  ShapeCodegenInfo origin_shape;
  ShapeRangeCodegenInfo storage_shape_range;
  ShapeRangeCodegenInfo origin_shape_range;
  ShapeCodegenInfo aipp_shape;
};

struct ModelDescCodegenInfo {
  size_t reusable_stream_num;
  size_t reusable_event_num;
  size_t reusable_notify_num;
  size_t attached_stream_num;
  std::vector<ModelIoCodegenInfo> inputs;
  std::vector<ModelIoCodegenInfo> outputs;
};

struct RawShape {
  size_t dim_num;
  int64_t dims[gert::Shape::kMaxDimNum];
  uint8_t reserved[40];
};

struct RawExpandDimsType {
  uint64_t bits;
  uint8_t reserved[40];
};

struct RawStorageFormat {
  int32_t origin_format;
  int32_t storage_format;
  RawExpandDimsType expand_dims_type;
  uint8_t reserved[40];
};

struct RawStorageShape {
  RawShape origin_shape;
  RawShape storage_shape;
  uint8_t reserved[40];
};

struct RawShapeRange {
  RawShape min;
  RawShape max;
};

struct RawModelIoDesc {
  const ge::char_t *name;
  int32_t data_type;
  RawStorageFormat format;
  RawStorageShape shape;
  RawShapeRange storage_shape_range;
  RawShapeRange origin_shape_range;
  RawShape aipp_shape;
};

struct RawContinuousVector {
  size_t capacity;
  size_t size;
  uint8_t reserved[40];
  uint8_t elements[8];
};

struct RawModelDesc {
  void *space_registries;
  const ge::char_t *file_constant_weight_dir;
  size_t input_num;
  size_t output_num;
  size_t reusable_stream_num;
  size_t reusable_event_num;
  size_t reusable_notify_num;
  size_t attached_stream_num;
  RawContinuousVector model_io_descs;
};

static_assert(sizeof(RawModelIoDesc) == sizeof(gert::ModelIoDesc), "ModelIoDesc layout changed");
static_assert(sizeof(RawModelDesc) == sizeof(gert::ModelDesc), "ModelDesc layout changed");

struct TensorConstPayload {
  uint64_t address;
  uint64_t size;
  int32_t placement;
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  uint64_t expand_dims_size;
  uint64_t expand_dims_mask;
  uint32_t origin_dim_num;
  uint32_t storage_dim_num;
  int64_t origin_dims[gert::Shape::kMaxDimNum];
  int64_t storage_dims[gert::Shape::kMaxDimNum];
  uint64_t weight_index;
  uint64_t has_weight_index;
  uint64_t weight_data_offset;
  uint64_t weight_data_size;
};

static_assert(sizeof(TensorConstPayload) == 488U, "Tensor const payload layout changed");

struct StructuredResourceCodegenInfo {
  std::vector<std::string> buffer_entries;
  std::vector<ComputeNodeCodegenInfo> compute_node_infos;
  std::vector<KernelExtendCodegenInfo> kernel_extend_infos;
  ModelDescCodegenInfo model_desc;
};

// A fused group merges a consecutive run of own (kCodegenImpl) kernels in one region into a
// single generated function.  Chains with a single producer and a single consumer inside the
// group are re-bound to stack-local Chain objects, removing the global chains_[] access for
// those transfers.  Dispatch uses kFusedDispatchBase | group ordinal so the shared
// RunKernelImplDispatch switch can address fused units without occupying kernel indexes.
constexpr uint32_t kFusedDispatchBase = 0x40000000U;

struct FusedGroupInfo {
  // Serial/structured anchor: consecutive steps inside one region (also covers If islands).
  uint32_t region_index = kInvalidProgramIndex;
  size_t step_begin = 0U;  // inclusive, index into region steps
  size_t step_end = 0U;    // exclusive
  // MultiThread anchor: consecutive kernel units inside one worker lane.
  bool from_multithread = false;
  size_t worker_index = 0U;
  size_t unit_begin = 0U;  // inclusive, index into worker units
  size_t unit_end = 0U;    // exclusive
  uint32_t fused_dispatch_index = kInvalidProgramIndex;
  std::set<uint32_t> localized_chains;
  std::string function_symbol;
  std::string function_body;
};

struct DynamicProgramModel {
  std::string model_name;
  std::vector<DynamicChainCodegenInfo> chains;
  std::vector<DynamicConstChainCodegenInfo> const_chains;
  std::vector<DynamicKernelCodegenInfo> init;
  std::vector<DynamicKernelCodegenInfo> main;
  std::vector<DynamicKernelCodegenInfo> deinit;
  ProgramPlanType main_plan_type = ProgramPlanType::kSequential;
  std::vector<ProgramRegion> main_regions;
  uint32_t main_root_region_index = kInvalidProgramIndex;
  size_t thread_num = 1U;
  MultiThreadProgramPlan main_multithread_plan;
  bool main_multithread_enabled = false;
  std::vector<FusedGroupInfo> main_fused_groups;
  std::set<uint32_t> main_fused_member_kernels;
  ModelResourceInfo resources;
  StructuredResourceCodegenInfo structured_resources;
  bool structured_resources_valid = false;
  bool codegen_valid = true;
};

// Compatibility metadata for the original dynamic GE compact-weight route.
struct HostTensorBindingCodegenInfo {
  uint32_t chain_index;
  uint32_t weight_index;
  uint64_t data_offset;
  uint64_t data_size;
};

const gert::ContinuousBuffer *AsContinuousBuffer(const std::vector<uint8_t> &bytes) {
  if (bytes.size() < sizeof(gert::ContinuousBuffer)) {
    return nullptr;
  }
  const auto *buffer = reinterpret_cast<const gert::ContinuousBuffer *>(bytes.data());
  return buffer->GetTotalLength() == bytes.size() ? buffer : nullptr;
}

bool ResolveBufferString(const gert::ContinuousBuffer &buffer, const size_t id, std::string &value) {
  size_t size = 0U;
  const auto *data = buffer.Get<char>(id, size);
  if ((data == nullptr) || (size == 0U) || (data[size - 1U] != '\0')) {
    return false;
  }
  value.assign(data, size - 1U);
  return true;
}

bool ValidateRuntimeAttrs(const uint8_t *const attrs, const size_t attrs_size) {
  if ((attrs == nullptr) || (attrs_size < sizeof(RuntimeAttrsDef))) {
    return false;
  }
  const auto *const attrs_def = reinterpret_cast<const RuntimeAttrsDef *>(attrs);
  size_t offsets_size = 0U;
  if (ge::MulOverflow(attrs_def->attr_num, sizeof(size_t), offsets_size) ||
      ge::AddOverflow(sizeof(RuntimeAttrsDef), offsets_size, offsets_size) || (offsets_size > attrs_size)) {
    return false;
  }
  const size_t payload_start = offsets_size;
  size_t previous_offset = payload_start;
  for (size_t index = 0U; index < attrs_def->attr_num; ++index) {
    const size_t offset = attrs_def->offset[index];
    if ((offset < payload_start) || (offset >= attrs_size) || (offset < previous_offset)) {
      return false;
    }
    previous_offset = offset;
  }
  return true;
}

TensorDescCodegenInfo CollectTensorDesc(const gert::CompileTimeTensorDesc &desc) {
  const auto expand_dims_type = desc.GetExpandDimsType();
  uint64_t mask = 0U;
  for (uint64_t i = 0U; i < expand_dims_type.GetFullSize(); ++i) {
    if (expand_dims_type.IsExpandIndex(i)) {
      mask |= (1ULL << i);
    }
  }
  return {static_cast<int32_t>(desc.GetDataType()),
          static_cast<int32_t>(desc.GetOriginFormat()),
          static_cast<int32_t>(desc.GetStorageFormat()),
          expand_dims_type.GetFullSize(),
          mask,
          desc.IsExist()};
}

ShapeCodegenInfo CollectRawShape(const RawShape &shape) {
  ShapeCodegenInfo info;
  info.dim_num = shape.dim_num;
  if (shape.dim_num <= gert::Shape::kMaxDimNum) {
    info.dims.assign(shape.dims, shape.dims + shape.dim_num);
  } else if (shape.dim_num == std::numeric_limits<size_t>::max()) {
    info.dims.push_back(shape.dims[0]);
  }
  return info;
}

ModelIoCodegenInfo CollectModelIo(const RawModelIoDesc &desc, const gert::ContinuousBuffer &model_desc_buffer) {
  ModelIoCodegenInfo info;
  const auto name_id = reinterpret_cast<size_t>(desc.name);
  (void)ResolveBufferString(model_desc_buffer, name_id, info.name);
  info.data_type = desc.data_type;
  info.storage_format = desc.format.storage_format;
  info.origin_format = desc.format.origin_format;
  info.storage_shape = CollectRawShape(desc.shape.storage_shape);
  info.origin_shape = CollectRawShape(desc.shape.origin_shape);
  info.storage_shape_range.min = CollectRawShape(desc.storage_shape_range.min);
  info.storage_shape_range.max = CollectRawShape(desc.storage_shape_range.max);
  info.origin_shape_range.min = CollectRawShape(desc.origin_shape_range.min);
  info.origin_shape_range.max = CollectRawShape(desc.origin_shape_range.max);
  info.aipp_shape = CollectRawShape(desc.aipp_shape);
  return info;
}

bool BuildStructuredResources(const ModelResourceInfo &resources, StructuredResourceCodegenInfo &output) {
  const auto *buffer = AsContinuousBuffer(resources.buffer);
  const auto *compute_infos = AsContinuousBuffer(resources.compute_node_infos);
  const auto *kernel_infos = AsContinuousBuffer(resources.kernel_extend_infos);
  const auto *model_desc_buffer = AsContinuousBuffer(resources.model_desc);
  if ((buffer == nullptr) || (compute_infos == nullptr) || (kernel_infos == nullptr) ||
      (model_desc_buffer == nullptr) || (model_desc_buffer->GetNum() == 0U)) {
    return false;
  }

  output.buffer_entries.resize(buffer->GetNum());
  for (size_t i = 0U; i < buffer->GetNum(); ++i) {
    if (!ResolveBufferString(*buffer, i, output.buffer_entries[i])) {
      return false;
    }
  }

  output.compute_node_infos.reserve(compute_infos->GetNum());
  for (size_t i = 0U; i < compute_infos->GetNum(); ++i) {
    size_t record_size = 0U;
    const auto *info = compute_infos->Get<gert::ComputeNodeInfo>(i, record_size);
    if (info == nullptr) {
      return false;
    }
    ComputeNodeCodegenInfo codegen_info;
    const auto name_id = reinterpret_cast<size_t>(info->GetNodeName());
    const auto type_id = reinterpret_cast<size_t>(info->GetNodeType());
    if (!ResolveBufferString(*buffer, name_id, codegen_info.node_name) ||
        !ResolveBufferString(*buffer, type_id, codegen_info.node_type)) {
      return false;
    }
    codegen_info.ir_input_num = info->GetIrInputsNum();
    codegen_info.ir_output_num = info->GetIrOutputsNum();
    size_t base_size = 0U;
    if (gert::ComputeNodeInfo::CalcSize(info->GetIrInputsNum(), info->GetIrOutputsNum(), info->GetInputsNum(),
                                        info->GetOutputsNum(), base_size) != ge::GRAPH_SUCCESS ||
        record_size < base_size) {
      return false;
    }
    const size_t attrs_size = record_size - base_size;
    const auto *const attrs = reinterpret_cast<const uint8_t *>(info->GetAttrs());
    if (!ValidateRuntimeAttrs(attrs, attrs_size)) {
      return false;
    }
    codegen_info.attrs.assign(attrs, attrs + attrs_size);
    codegen_info.input_instances.reserve(codegen_info.ir_input_num);
    codegen_info.output_instances.reserve(codegen_info.ir_output_num);
    codegen_info.input_descs.reserve(info->GetInputsNum());
    codegen_info.output_descs.reserve(info->GetOutputsNum());
    for (size_t index = 0U; index < codegen_info.ir_input_num; ++index) {
      const auto *anchor = info->GetInputInstanceInfo(index);
      if (anchor == nullptr) {
        return false;
      }
      codegen_info.input_instances.push_back({anchor->GetInstanceStart(), anchor->GetInstanceNum()});
    }
    for (size_t index = 0U; index < codegen_info.ir_output_num; ++index) {
      const auto *anchor = info->GetOutputInstanceInfo(index);
      if (anchor == nullptr) {
        return false;
      }
      codegen_info.output_instances.push_back({anchor->GetInstanceStart(), anchor->GetInstanceNum()});
    }
    for (size_t index = 0U; index < info->GetInputsNum(); ++index) {
      const auto *desc = info->GetInputTdInfo(index);
      if (desc == nullptr) {
        return false;
      }
      codegen_info.input_descs.push_back(CollectTensorDesc(*desc));
    }
    for (size_t index = 0U; index < info->GetOutputsNum(); ++index) {
      const auto *desc = info->GetOutputTdInfo(index);
      if (desc == nullptr) {
        return false;
      }
      codegen_info.output_descs.push_back(CollectTensorDesc(*desc));
    }
    output.compute_node_infos.emplace_back(std::move(codegen_info));
  }

  output.kernel_extend_infos.reserve(kernel_infos->GetNum());
  for (size_t i = 0U; i < kernel_infos->GetNum(); ++i) {
    const auto *info = kernel_infos->Get<gert::KernelExtendInfo>(i);
    if (info == nullptr) {
      return false;
    }
    KernelExtendCodegenInfo codegen_info;
    if (!ResolveBufferString(*buffer, reinterpret_cast<size_t>(info->GetKernelName()), codegen_info.kernel_name) ||
        !ResolveBufferString(*buffer, reinterpret_cast<size_t>(info->GetKernelType()), codegen_info.kernel_type)) {
      return false;
    }
    output.kernel_extend_infos.emplace_back(std::move(codegen_info));
  }

  const auto *model_desc = model_desc_buffer->Get<RawModelDesc>(model_desc_buffer->GetNum() - 1U);
  if (model_desc == nullptr) {
    return false;
  }
  output.model_desc.reusable_stream_num = model_desc->reusable_stream_num;
  output.model_desc.reusable_event_num = model_desc->reusable_event_num;
  output.model_desc.reusable_notify_num = model_desc->reusable_notify_num;
  output.model_desc.attached_stream_num = model_desc->attached_stream_num;
  const auto *io_descs =
      reinterpret_cast<const RawModelIoDesc *>(reinterpret_cast<const uint8_t *>(model_desc) + sizeof(RawModelDesc));
  output.model_desc.inputs.reserve(model_desc->input_num);
  output.model_desc.outputs.reserve(model_desc->output_num);
  for (size_t i = 0U; i < model_desc->input_num; ++i) {
    output.model_desc.inputs.emplace_back(CollectModelIo(io_descs[i], *model_desc_buffer));
  }
  for (size_t i = 0U; i < model_desc->output_num; ++i) {
    output.model_desc.outputs.emplace_back(CollectModelIo(io_descs[model_desc->input_num + i], *model_desc_buffer));
  }
  return true;
}

std::string MakeStableSymbol(const std::string &prefix, const std::string &name, size_t index) {
  std::string symbol = prefix;
  symbol.push_back('_');
  symbol.append(std::to_string(index));
  symbol.push_back('_');
  for (const char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      symbol.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    } else {
      symbol.push_back('_');
    }
  }
  return symbol;
}

std::string EscapeForStringLiteral(const std::string &value);

std::vector<FusedGroupInfo> AnalyzeFusableGroups(const DynamicProgramModel &model);
void CollectFusedMemberKernels(const std::vector<FusedGroupInfo> &groups, const DynamicProgramModel &model,
                               std::set<uint32_t> &members);

DynamicProgramModel BuildDynamicProgramModel(const ProgramInfo &program_info, const ProgramBodyCodegenOption &option) {
  DynamicProgramModel model;
  model.model_name = option.model_name;
  model.resources = program_info.resources;
  model.structured_resources_valid = BuildStructuredResources(program_info.resources, model.structured_resources);
  model.chains.reserve(program_info.chains.size());
  for (const auto &chain : program_info.chains) {
    const auto binary_key = program_info.binary_resource_keys.find(chain.index);
    model.chains.push_back(
        {chain.index, chain.symbol, chain.source_kind, chain.binding_phase, chain.binding_kind, chain.source_index,
         MakeStableSymbol("chain", std::to_string(chain.index), chain.index),
         binary_key == program_info.binary_resource_keys.cend() ? std::string() : binary_key->second});
  }
  model.const_chains.reserve(program_info.const_chains.size());
  for (const auto &chain : program_info.const_chains) {
    model.const_chains.push_back({chain.chain_index, chain.producer_name, chain.value, false, {}, {}, true});
  }
  for (const auto &chain : program_info.custom_const_chains) {
    Om2ConstValue value;
    value.type = Om2ValueType::kUnknown;
    model.const_chains.push_back({chain.chain_index, chain.producer_name, value, true, chain.initializer, chain.payload,
                                  chain.payload_is_string});
  }
  const auto append_kernels = [&model](const std::string &block_name, const std::vector<KernelProgramInfo> &kernels,
                                       std::vector<DynamicKernelCodegenInfo> &output) {
    output.reserve(output.size() + kernels.size());
    for (size_t index = 0; index < kernels.size(); ++index) {
      const auto &kernel = kernels[index];
      DynamicKernelCodegenInfo codegen_info{MakeStableSymbol(block_name, kernel.kernel_type, index),
                                            kernel.execution_node_name,
                                            kernel.kernel_type,
                                            kernel.inputs,
                                            kernel.outputs,
                                            kernel.compute_node_index,
                                            kernel.output_policy,
                                            kernel.impl_type,
                                            kernel.require_compute_node_info,
                                            {},
                                            {}};
      if (kernel.impl_type == KernelImplType::kCodegenImpl) {
        if ((kernel.code_builder == nullptr) ||
            (kernel.code_builder->GenerateKernelCode(codegen_info.cpp_symbol, codegen_info.generated_function) !=
             ge::GRAPH_SUCCESS)) {
          model.codegen_valid = false;
        }
      }
      if (kernel.output_policy == OutputCreatePolicy::kCodegenImpl) {
        if ((kernel.code_builder == nullptr) ||
            (kernel.code_builder->GenerateOutputsCreatorCode(codegen_info.generated_outputs_creator_code) !=
             ge::GRAPH_SUCCESS) ||
            codegen_info.generated_outputs_creator_code.empty()) {
          model.codegen_valid = false;
        }
      }
      output.emplace_back(std::move(codegen_info));
    }
  };
  append_kernels("init", program_info.init.kernels, model.init);
  append_kernels("main", program_info.main.kernels, model.main);
  append_kernels("deinit", program_info.deinit.kernels, model.deinit);
  model.main_plan_type = program_info.main.plan_type;
  model.main_regions = program_info.main.regions;
  model.main_root_region_index = program_info.main.root_region_index;
  model.thread_num = option.thread_num;
  if (option.thread_num > 1U) {
    std::vector<ExecTaskType> kernel_types;
    kernel_types.reserve(program_info.main.kernels.size());
    for (const auto &kernel : program_info.main.kernels) {
      const auto task_type = ClassifyKernelTask(kernel.kernel_type.c_str(), option.thread_num);
      kernel_types.push_back(task_type);
      GELOGD("[OM2][ProgramBody] kernel_index=%zu kernel_type=%s exec_task_type=%u", kernel_types.size() - 1U,
             kernel.kernel_type.c_str(), static_cast<uint32_t>(task_type));
    }
    model.main_multithread_enabled =
        BuildMultiThreadProgramPlan(program_info.main, kernel_types, option.thread_num, model.main_multithread_plan);
    if (model.main_multithread_enabled) {
      GELOGI(
          "[OM2][ProgramBody] Main execution mode=static_multithread, requested_threads=%zu, worker_count=%zu, "
          "plan_type=%u",
          option.thread_num, model.main_multithread_plan.workers.size(),
          static_cast<uint32_t>(program_info.main.plan_type));
    } else {
      GELOGI("[OM2][ProgramBody] Main execution mode=serial, requested_threads=%zu, plan_type=%u", option.thread_num,
             static_cast<uint32_t>(program_info.main.plan_type));
    }
  } else {
    GELOGI("[OM2][ProgramBody] Main execution mode=serial, requested_threads=%zu, plan_type=%u", option.thread_num,
           static_cast<uint32_t>(program_info.main.plan_type));
  }
  model.main_fused_groups = AnalyzeFusableGroups(model);
  CollectFusedMemberKernels(model.main_fused_groups, model, model.main_fused_member_kernels);
  for (const auto &group : model.main_fused_groups) {
    GELOGI("[OM2][ProgramBody] fused group region=%u steps=[%zu,%zu) chains=%zu symbol=%s", group.region_index,
           group.step_begin, group.step_end, group.localized_chains.size(), group.function_symbol.c_str());
  }
  return model;
}

std::string EscapeForStringLiteral(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size() + 8U);
  for (const char c : value) {
    switch (c) {
      case '\\':
      case '"':
        escaped.push_back('\\');
        escaped.push_back(c);
        break;
      case '\n':
        escaped.append("\\n");
        break;
      case '\r':
        escaped.append("\\r");
        break;
      case '\t':
        escaped.append("\\t");
        break;
      default:
        escaped.push_back(c);
        break;
    }
  }
  return escaped;
}

std::string EmitWrappedStringLiteral(const std::string &value, const std::string &continuation_indent) {
  constexpr size_t kChunkSize = 80U;
  if (value.empty()) {
    return "\"\"";
  }
  std::ostringstream ss;
  for (size_t offset = 0U; offset < value.size(); offset += kChunkSize) {
    if (offset > 0U) {
      ss << "\n" << continuation_indent;
    }
    ss << "\"" << EscapeForStringLiteral(value.substr(offset, kChunkSize)) << "\"";
  }
  return ss.str();
}

std::string EmitStructuredResources(const DynamicProgramModel &program_model,
                                    const std::vector<uint32_t> &model_input_chains,
                                    const std::vector<uint32_t> &model_output_chains,
                                    std::string &metadata_initializer) {
  const auto &resources = program_model.structured_resources;
  std::ostringstream types;
  std::ostringstream ss;
  types << R"(struct GeneratedAnchorDef {
  size_t instance_start;
  size_t instance_num;
};

struct GeneratedTensorDescDef {
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  size_t expand_dims_size;
  uint64_t expand_dims_mask;
  bool exists;
};

struct GeneratedComputeNodeInfoDef {
  const char *name;
  const char *type;
  size_t ir_input_num;
  size_t ir_output_num;
  GeneratedArrayRef<uint8_t> attrs;
  GeneratedArrayRef<GeneratedAnchorDef> input_instances;
  GeneratedArrayRef<GeneratedAnchorDef> output_instances;
  GeneratedArrayRef<GeneratedTensorDescDef> input_descs;
  GeneratedArrayRef<GeneratedTensorDescDef> output_descs;
};

struct GeneratedShapeDef {
  const int64_t *dims;
  size_t stored_dim_num;
  size_t dim_num;
};

struct GeneratedShapeRangeDef {
  GeneratedShapeDef min;
  GeneratedShapeDef max;
};

struct GeneratedModelIoDescDef {
  const char *name;
  int32_t data_type;
  int32_t storage_format;
  int32_t origin_format;
  GeneratedShapeDef storage_shape;
  GeneratedShapeDef origin_shape;
  GeneratedShapeRangeDef storage_range;
  GeneratedShapeRangeDef origin_range;
  GeneratedShapeDef aipp_shape;
};

struct GeneratedModelIoDef {
  GeneratedModelIoDescDef desc;
  uint32_t chain_index;
};

struct GeneratedModelDescDef {
  size_t reusable_stream_num;
  size_t reusable_event_num;
  size_t reusable_notify_num;
  size_t attached_stream_num;
  GeneratedArrayRef<GeneratedModelIoDef> inputs;
  GeneratedArrayRef<GeneratedModelIoDef> outputs;
};

struct GeneratedModelMetadataDef {
  GeneratedArrayRef<GeneratedComputeNodeInfoDef> compute_nodes;
  GeneratedModelDescDef model_desc;
};

)";

  const auto emit_anchor_ref = [](const std::vector<AnchorCodegenInfo> &values) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{(const GeneratedAnchorDef[]){";
    for (size_t i = 0U; i < values.size(); ++i) {
      out << "{" << values[i].instance_start << "U, " << values[i].instance_num << "U}"
          << (i + 1U == values.size() ? "" : ", ");
    }
    out << "}, " << values.size() << "U}";
    return out.str();
  };
  const auto emit_tensor_ref = [](const std::vector<TensorDescCodegenInfo> &values) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{(const GeneratedTensorDescDef[]){";
    for (size_t i = 0U; i < values.size(); ++i) {
      const auto &value = values[i];
      out << "{" << value.data_type << ", " << value.origin_format << ", " << value.storage_format << ", "
          << value.expand_dims_size << "U, " << value.expand_dims_mask << "ULL, " << (value.exists ? "true" : "false")
          << "}" << (i + 1U == values.size() ? "" : ", ");
    }
    out << "}, " << values.size() << "U}";
    return out.str();
  };
  const auto emit_byte_ref = [](const std::vector<uint8_t> &values) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{(const uint8_t[]){";
    for (size_t i = 0U; i < values.size(); ++i) {
      out << static_cast<uint32_t>(values[i]) << "U" << (i + 1U == values.size() ? "" : ", ");
    }
    out << "}, " << values.size() << "U}";
    return out.str();
  };
  ss << "{\n  {\n    ";
  if (resources.compute_node_infos.empty()) {
    ss << "nullptr, 0U";
  } else {
    ss << "(const GeneratedComputeNodeInfoDef[]){\n";
  }
  for (size_t i = 0U; i < resources.compute_node_infos.size(); ++i) {
    const auto &info = resources.compute_node_infos[i];
    ss << "      {\n"
       << "        \"" << EscapeForStringLiteral(info.node_name) << "\",\n"
       << "        \"" << EscapeForStringLiteral(info.node_type) << "\",\n"
       << "        " << info.ir_input_num << "U,\n"
       << "        " << info.ir_output_num << "U,\n"
       << "        " << emit_byte_ref(info.attrs) << ",\n"
       << "        " << emit_anchor_ref(info.input_instances) << ",\n"
       << "        " << emit_anchor_ref(info.output_instances) << ",\n"
       << "        " << emit_tensor_ref(info.input_descs) << ",\n"
       << "        " << emit_tensor_ref(info.output_descs) << "\n"
       << "      }" << (i + 1U == resources.compute_node_infos.size() ? "" : ",") << "\n";
  }
  if (!resources.compute_node_infos.empty()) {
    ss << "    },\n    " << resources.compute_node_infos.size() << "U";
  }
  ss << "\n  },\n  {\n    " << resources.model_desc.reusable_stream_num << "U,\n    "
     << resources.model_desc.reusable_event_num << "U,\n    " << resources.model_desc.reusable_notify_num << "U,\n    "
     << resources.model_desc.attached_stream_num << "U,\n    ";

  const auto emit_shape = [](const ShapeCodegenInfo &shape) {
    std::ostringstream out;
    out << "{";
    if (shape.dims.empty()) {
      out << "nullptr";
    } else {
      out << "(const int64_t[]){";
    }
    for (size_t i = 0U; i < shape.dims.size(); ++i) {
      out << shape.dims[i] << "LL" << (i + 1U == shape.dims.size() ? "" : ", ");
    }
    if (!shape.dims.empty()) {
      out << "}";
    }
    out << ", " << shape.dims.size() << "U, ";
    if (shape.dim_num == std::numeric_limits<size_t>::max()) {
      out << "static_cast<size_t>(-1)";
    } else {
      out << shape.dim_num << "U";
    }
    out << "}";
    return out.str();
  };
  const auto emit_io = [&emit_shape](const ModelIoCodegenInfo &io) {
    std::ostringstream out;
    out << "{\n"
        << "            \"" << EscapeForStringLiteral(io.name) << "\",\n"
        << "            " << io.data_type << ",\n"
        << "            " << io.storage_format << ",\n"
        << "            " << io.origin_format << ",\n"
        << "            " << emit_shape(io.storage_shape) << ",\n"
        << "            " << emit_shape(io.origin_shape) << ",\n"
        << "            {" << emit_shape(io.storage_shape_range.min) << ", " << emit_shape(io.storage_shape_range.max)
        << "},\n"
        << "            {" << emit_shape(io.origin_shape_range.min) << ", " << emit_shape(io.origin_shape_range.max)
        << "},\n"
        << "            " << emit_shape(io.aipp_shape) << "\n"
        << "          }";
    return out.str();
  };

  const auto emit_io_ref = [&emit_io](const std::vector<ModelIoCodegenInfo> &values,
                                      const std::vector<uint32_t> &chain_indices) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{\n      (const GeneratedModelIoDef[]){\n";
    for (size_t i = 0U; i < values.size(); ++i) {
      const auto chain_index = i < chain_indices.size() ? chain_indices[i] : kInvalidProgramIndex;
      out << "        {\n          " << emit_io(values[i]) << ",\n          " << chain_index << "U\n        }"
          << (i + 1U == values.size() ? "" : ",") << "\n";
    }
    out << "      },\n      " << values.size() << "U\n    }";
    return out.str();
  };
  ss << emit_io_ref(resources.model_desc.inputs, model_input_chains) << ",\n    "
     << emit_io_ref(resources.model_desc.outputs, model_output_chains) << "\n  }\n}";
  metadata_initializer = ss.str();
  return types.str();
}

std::string EmitTypedConstChainSupport() {
  return R"(
struct DfxExeArgConstDef {
  bool need_assert;
  bool need_print;
  int64_t buffer_size;
};

struct BuildTensorAttrConstDef {
  int32_t placement;
  int32_t data_type;
  int32_t storage_format;
  int32_t origin_format;
};

struct LaunchKernelAttrConstDef {
  int32_t id;
  uint32_t value;
};

struct ComputeNodeDescConstDef {
  size_t input_num;
  size_t output_num;
  size_t workspace_cap;
  size_t max_tiling_data;
  bool need_shape_buffer;
  bool need_overflow;
  GeneratedArrayRef<uint8_t> compiled_args;
};

struct DynDescConstDef {
  bool is_group_first;
  uint32_t io_group_id;
  uint32_t io_index;
  uint32_t dyn_num;
  uint32_t group_size;
  uint32_t group_offset;
  uint32_t ptr_offset;
};

struct ArgsInfoConstDef {
  int32_t arg_type;
  int32_t arg_format;
  int32_t start_index;
  uint32_t arg_size;
  bool folded_first;
  bool be_folded;
  DynDescConstDef dyn_desc;
  size_t data_size;
};

struct ArgsInfosDescConstDef {
  size_t input_num;
  size_t output_num;
  size_t input_valid_num;
  size_t output_valid_num;
  GeneratedArrayRef<ArgsInfoConstDef> args_infos;
};

struct ContinuousVectorVectorInt32ConstDef {
  GeneratedArrayRef<GeneratedArrayRef<int32_t>> vectors;
};

template <typename T>
struct ContinuousVectorConstDef {
  GeneratedArrayRef<T> values;
};

struct OpExecuteOptionsConstDef {
  GeneratedArrayRef<uint8_t> bytes;
};

struct MixCoreArgsConstDef {
  uint32_t mix_type;
  uint32_t all_core_num;
  uint32_t vec_core_num;
  uint32_t ai_core_num;
};

// Layout-compatible with gert::kernel::MemoryBaseTypeOffset from
// kernel/known_subgraph/davinci_model_kernel.h: enum(4B)+pad, int64 offset, uint64 size.
struct MemoryBaseTypeOffsetConstDef {
  uint32_t base_type;
  int64_t offset;
  uint64_t size;
};

// Layout-compatible with gert::kernel::FileConstantNameAndMem from
// kernel/known_subgraph/davinci_model_kernel.h (kMaxFileNameLen + 1 == 256).
struct GeneratedFileConstantNameAndMem {
  char name[256];
  const void *mem;
  size_t size;
};

struct StorageShapeConstDef {
  uint32_t origin_dim_num;
  uint32_t storage_dim_num;
  int64_t origin_dims[gert::Shape::kMaxDimNum];
  int64_t storage_dims[gert::Shape::kMaxDimNum];
};

struct GertTensorDataConstDef {
  uint64_t address;
  uint64_t size;
  int32_t placement;
  int64_t stream_id;
};

struct TensorDataObjectConstDef {
  uint64_t address;
  uint64_t size;
  int32_t placement;
};

struct TensorConstDef {
  uint64_t address;
  uint64_t size;
  int32_t placement;
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  uint64_t expand_dims_size;
  uint64_t expand_dims_mask;
  uint32_t origin_dim_num;
  uint32_t storage_dim_num;
  int64_t origin_dims[gert::Shape::kMaxDimNum];
  int64_t storage_dims[gert::Shape::kMaxDimNum];
};

template <typename T>
aclError InitInlineConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  static_assert(sizeof(T) <= sizeof(void *), "Chain value is too large");
  void *bits = nullptr;
  if (memcpy_s(&bits, sizeof(bits), definition, sizeof(T)) != EOK) {
    return kFailed;
  }
  chain.Set(bits, nullptr);
  return kOk;
}

template <typename T>
aclError InitOwnedConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  auto *const storage = new (std::nothrow) T(*static_cast<const T *>(definition));
  if (storage == nullptr) {
    return kFailed;
  }
  chain.SetWithDefaultDeleter(storage);
  return kOk;
}

aclError InitNullConst(gert::Chain &chain, const void *) {
  chain.Set(nullptr, nullptr);
  return kOk;
}

aclError InitLaunchKernelAttrsConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const GeneratedArrayRef<LaunchKernelAttrConstDef> *>(definition);
  if ((def.size != 5U) || (def.data == nullptr)) {
    return kFailed;
  }
  auto attrs = std::make_unique<aclrtLaunchKernelAttr[]>(def.size);
  for (size_t i = 0U; i < def.size; ++i) {
    attrs[i].id = static_cast<aclrtLaunchKernelAttrId>(def[i].id);
    switch (attrs[i].id) {
      case ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE:
        if (def[i].value > std::numeric_limits<uint8_t>::max()) {
          return kFailed;
        }
        attrs[i].value.schemMode = static_cast<uint8_t>(def[i].value);
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE:
        attrs[i].value.dynUBufSize = def[i].value;
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE:
        attrs[i].value.engineType = static_cast<aclrtEngineType>(def[i].value);
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET:
        attrs[i].value.blockDimOffset = def[i].value;
        break;
      case ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING:
        attrs[i].value.enableProfiling = def[i].value;
        break;
      default:
        return kFailed;
    }
  }
  chain.SetWithDefaultDeleter<aclrtLaunchKernelAttr[]>(attrs.release());
  return kOk;
}

aclError InitLaunchKernelConfigConst(gert::Chain &chain, const void *definition) {
  if ((definition == nullptr) || (*static_cast<const size_t *>(definition) != 5U)) {
    return kFailed;
  }
  auto *const config = new (std::nothrow) aclrtLaunchKernelCfg{};
  if (config == nullptr) {
    return kFailed;
  }
  config->numAttrs = *static_cast<const size_t *>(definition);
  // PrepareLaunchConfig binds this instance's attribute array before launch.
  chain.SetWithDefaultDeleter(config);
  return kOk;
}

aclError InitStringConst(gert::Chain &chain, const void *definition) {
  chain.Set(const_cast<void *>(definition), nullptr);
  return kOk;
}

aclError InitRawBytesConst(gert::Chain &chain, const void *definition) {
  chain.Set(const_cast<void *>(definition), nullptr);
  return kOk;
}

aclError InitProcessLocalPointerConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  unsigned long long address = 0ULL;
  char trailing = '\0';
  if ((sscanf_s(static_cast<const char *>(definition), "%llu%c", &address, &trailing,
                static_cast<unsigned int>(sizeof(trailing))) != 1) ||
      (address == 0ULL) ||
      (address > static_cast<unsigned long long>(std::numeric_limits<uintptr_t>::max()))) {
    return kFailed;
  }
  chain.Set(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), nullptr);
  return kOk;
}

aclError InitNullPointerConst(gert::Chain &chain, const void *definition) {
  (void)definition;
  chain.Set(nullptr, nullptr);
  return kOk;
}

aclError InitAicpuHostProcFuncConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  auto &resource_manager = gert::AicpuResourceManager::GetInstance();
  if (resource_manager.LoadConstantFoldingLib() != ge::GRAPH_SUCCESS) {
    return kFailed;
  }
  auto finder = resource_manager.GetAicpuHostFindFunc();
  if (finder == nullptr) {
    return kFailed;
  }
  const auto func = finder(static_cast<const char *>(definition));
  if (func == nullptr) {
    return kFailed;
  }
  static_assert(sizeof(func) <= sizeof(void *), "Aicpu host function pointer does not fit in a Chain value");
  void *bits = nullptr;
  if (memcpy_s(&bits, sizeof(bits), &func, sizeof(func)) != EOK) {
    return kFailed;
  }
  chain.Set(bits, nullptr);
  return kOk;
}

aclError InitTensorDataOffsetConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  unsigned long long offset = 0ULL;
  unsigned long long size = 0ULL;
  int placement = 0;
  if (sscanf_s(static_cast<const char *>(definition), "%llu,%llu,%d", &offset, &size, &placement) != 3) {
    return kFailed;
  }
  if ((offset > static_cast<unsigned long long>(std::numeric_limits<uintptr_t>::max())) ||
      (size > static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) ||
      (placement < static_cast<int>(gert::kOnDeviceHbm)) ||
      (placement > static_cast<int>(gert::kTensorPlacementEnd))) {
    return kFailed;
  }
  auto *const storage = new (std::nothrow) gert::TensorData(
      reinterpret_cast<void *>(static_cast<uintptr_t>(offset)), nullptr, static_cast<size_t>(size),
      static_cast<gert::TensorPlacement>(placement));
  if (storage == nullptr) {
    return kFailed;
  }
  chain.SetWithDefaultDeleter(storage);
  return kOk;
}

aclError InitTaskConfigConst(gert::Chain &chain, const void *) {
  const rtTaskCfgInfo_t value{};
  return InitOwnedConst<rtTaskCfgInfo_t>(chain, &value);
}

aclError InitDfxExeArgConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const DfxExeArgConstDef *>(definition);
  const gert::DfxExeArg value{def.need_assert, def.need_print, def.buffer_size};
  return InitOwnedConst<gert::DfxExeArg>(chain, &value);
}

aclError InitBuildTensorAttrConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const BuildTensorAttrConstDef *>(definition);
  gert::kernel::BuildTensorAttr value{};
  value.placement = static_cast<gert::TensorPlacement>(def.placement);
  value.data_type = static_cast<ge::DataType>(def.data_type);
  value.storage_format.SetStorageFormat(static_cast<ge::Format>(def.storage_format));
  value.storage_format.SetOriginFormat(static_cast<ge::Format>(def.origin_format));
  return InitOwnedConst<gert::kernel::BuildTensorAttr>(chain, &value);
}

aclError InitOpExecuteOptionsConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const OpExecuteOptionsConstDef *>(definition);
  if (def.bytes.size != sizeof(gert::OpExecuteOptions)) {
    return kFailed;
  }
  gert::OpExecuteOptions value{};
  if (memcpy_s(&value, sizeof(value), def.bytes.data, def.bytes.size) != EOK) {
    return kFailed;
  }
  return InitOwnedConst<gert::OpExecuteOptions>(chain, &value);
}

aclError InitMixCoreArgsConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const MixCoreArgsConstDef *>(definition);
  if ((def.mix_type != 0U) && (def.mix_type != 1U)) {
    return kFailed;
  }
  gert::kernel::MixCoreArgs value{};
  value.mix_type = static_cast<gert::kernel::MixType>(def.mix_type);
  value.all_core_num = def.all_core_num;
  value.vec_core_num = def.vec_core_num;
  value.ai_core_num = def.ai_core_num;
  return InitOwnedConst<gert::kernel::MixCoreArgs>(chain, &value);
}

aclError InitMemoryBaseTypeOffsetConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const MemoryBaseTypeOffsetConstDef *>(definition);
  auto *const item = new (std::nothrow) MemoryBaseTypeOffsetConstDef{def.base_type, def.offset, def.size};
  if (item == nullptr) {
    return kFailed;
  }
  chain.SetWithDefaultDeleter(item);
  return kOk;
}

aclError InitStorageShapeConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const StorageShapeConstDef *>(definition);
  if ((def.origin_dim_num > gert::Shape::kMaxDimNum) || (def.storage_dim_num > gert::Shape::kMaxDimNum)) {
    return kFailed;
  }
  auto *const shape = new (std::nothrow) gert::StorageShape();
  if (shape == nullptr) {
    return kFailed;
  }
  shape->MutableOriginShape().SetDimNum(def.origin_dim_num);
  shape->MutableStorageShape().SetDimNum(def.storage_dim_num);
  for (size_t i = 0U; i < def.origin_dim_num; ++i) {
    shape->MutableOriginShape().SetDim(i, def.origin_dims[i]);
  }
  for (size_t i = 0U; i < def.storage_dim_num; ++i) {
    shape->MutableStorageShape().SetDim(i, def.storage_dims[i]);
  }
  chain.SetWithDefaultDeleter(shape);
  return kOk;
}

aclError InitGertTensorDataConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const GertTensorDataConstDef *>(definition);
  if ((def.address > static_cast<uint64_t>(std::numeric_limits<uintptr_t>::max())) ||
      (def.size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) ||
      (def.placement < static_cast<int32_t>(gert::kOnDeviceHbm)) ||
      (def.placement >= static_cast<int32_t>(gert::kTensorPlacementEnd))) {
    return kFailed;
  }
  // The runtime keeps GertTensorData's out-of-line constructors hidden.  Use
  // the public inline default constructor and explicitly restore only the
  // non-owning tensor fields; manager/block ownership must remain null after
  // deserialization.
  auto *const storage = new (std::nothrow) gert::GertTensorData();
  if (storage == nullptr) {
    return kFailed;
  }
  if (storage->MutableTensorData().SetAddr(
          reinterpret_cast<void *>(static_cast<uintptr_t>(def.address)), nullptr) != ge::GRAPH_SUCCESS) {
    delete storage;
    return kFailed;
  }
  storage->SetSize(static_cast<size_t>(def.size));
  storage->SetPlacement(static_cast<gert::TensorPlacement>(def.placement));
  chain.SetWithDefaultDeleter(storage);
  return kOk;
}

aclError InitTensorDataObjectConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const TensorDataObjectConstDef *>(definition);
  // A data-less TensorData is a legitimate state, not corruption: TensorData::Release() and the
  // failure path of AddShareCount both leave addr_ = nullptr, size_ = 0 and
  // placement_ = kTensorPlacementEnd.  Shape-only carriers look exactly like that.
  const bool owns_no_data = (def.address == 0ULL) && (def.size == 0ULL);
  const bool placement_valid =
      ((def.placement >= static_cast<int32_t>(gert::kOnDeviceHbm)) &&
       (def.placement < static_cast<int32_t>(gert::kTensorPlacementEnd))) ||
      ((def.placement == static_cast<int32_t>(gert::kTensorPlacementEnd)) && owns_no_data);
  if (!placement_valid) {
    return kFailed;
  }
  // Restore a bare gert::TensorData rather than the GertTensorData wrapper.  Ownership must not
  // survive deserialization, so the manager stays null and the address is treated as borrowed.
  auto *const storage = new (std::nothrow)
      gert::TensorData(reinterpret_cast<gert::TensorAddress>(static_cast<uintptr_t>(def.address)), nullptr,
                       static_cast<size_t>(def.size), static_cast<gert::TensorPlacement>(def.placement));
  if (storage == nullptr) {
    return kFailed;
  }
  chain.SetWithDefaultDeleter(storage);
  return kOk;
}

aclError InitGertTensorConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const TensorConstDef *>(definition);
  // A data-less Tensor legitimately carries kTensorPlacementEnd; the collector normalizes it that way
  // so that op impls see the same state RT2 shows them.  Accept exactly that combination.
  const bool def_owns_no_data = (def.address == 0ULL) && (def.size == 0ULL);
  const bool def_placement_valid =
      ((def.placement >= static_cast<int32_t>(gert::kOnDeviceHbm)) &&
       (def.placement < static_cast<int32_t>(gert::kTensorPlacementEnd))) ||
      ((def.placement == static_cast<int32_t>(gert::kTensorPlacementEnd)) && def_owns_no_data);
  if ((def.origin_dim_num > gert::Shape::kMaxDimNum) || (def.storage_dim_num > gert::Shape::kMaxDimNum) ||
      (def.expand_dims_size > gert::ExpandDimsType::kMaxExpandSize) || !def_placement_valid ||
      (def.address > static_cast<uint64_t>(std::numeric_limits<uintptr_t>::max())) ||
      (def.size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))) {
    return kFailed;
  }
  gert::StorageShape shape;
  shape.MutableOriginShape().SetDimNum(def.origin_dim_num);
  shape.MutableStorageShape().SetDimNum(def.storage_dim_num);
  for (size_t i = 0U; i < def.origin_dim_num; ++i) {
    shape.MutableOriginShape().SetDim(i, def.origin_dims[i]);
  }
  for (size_t i = 0U; i < def.storage_dim_num; ++i) {
    shape.MutableStorageShape().SetDim(i, def.storage_dims[i]);
  }
  const uint64_t expand_bits = (def.expand_dims_size << gert::ExpandDimsType::kMaxExpandSize) |
                               (def.expand_dims_mask & ((uint64_t{1} << 56U) - 1U));
  const gert::StorageFormat format(static_cast<ge::Format>(def.origin_format),
                                   static_cast<ge::Format>(def.storage_format),
                                   gert::ExpandDimsType(static_cast<int64_t>(expand_bits)));
  auto *const tensor = new (std::nothrow) gert::Tensor(
      shape, format, static_cast<gert::TensorPlacement>(def.placement), static_cast<ge::DataType>(def.data_type),
      reinterpret_cast<void *>(static_cast<uintptr_t>(def.address)));
  if (tensor == nullptr) {
    return kFailed;
  }
  tensor->SetSize(static_cast<size_t>(def.size));
  chain.SetWithDefaultDeleter(tensor);
  return kOk;
}

aclError InitComputeNodeDescConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const ComputeNodeDescConstDef *>(definition);
  const size_t total_size = sizeof(gert::RtKernelLaunchArgsEx::ComputeNodeDesc) + def.compiled_args.size;
  auto storage = std::make_unique<uint8_t[]>(total_size);
  auto *desc = reinterpret_cast<gert::RtKernelLaunchArgsEx::ComputeNodeDesc *>(storage.get());
  desc->input_num = def.input_num;
  desc->output_num = def.output_num;
  desc->workspace_cap = def.workspace_cap;
  desc->max_tiling_data = def.max_tiling_data;
  desc->need_shape_buffer = def.need_shape_buffer;
  desc->need_overflow = def.need_overflow;
  desc->compiled_args_size = def.compiled_args.size;
  if (def.compiled_args.size > 0U) {
    if (memcpy_s(desc->compiled_args, def.compiled_args.size,
                 def.compiled_args.data, def.compiled_args.size) != EOK) {
      return kFailed;
    }
  }
  chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
  return kOk;
}

aclError InitArgsInfosDescConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const ArgsInfosDescConstDef *>(definition);
  const size_t total_size = sizeof(gert::ArgsInfosDesc) + def.args_infos.size * sizeof(gert::ArgsInfosDesc::ArgsInfo);
  auto storage = std::make_unique<uint8_t[]>(total_size);
  auto *desc = reinterpret_cast<gert::ArgsInfosDesc *>(storage.get());
  desc->Init(def.input_num, def.output_num, def.input_valid_num, def.output_valid_num);
  auto *infos = reinterpret_cast<gert::ArgsInfosDesc::ArgsInfo *>(desc->MutableArgsInfoBase());
  for (size_t i = 0U; i < def.args_infos.size; ++i) {
    const auto &info = def.args_infos[i];
    infos[i].Init(static_cast<gert::ArgsInfosDesc::ArgsInfo::ArgsInfoType>(info.arg_type),
                  static_cast<gert::ArgsInfosDesc::ArgsInfo::ArgsInfoFormat>(info.arg_format),
                  info.start_index, info.arg_size);
    infos[i].folded_first = info.folded_first;
    infos[i].be_folded = info.be_folded;
    infos[i].dyn_desc = {info.dyn_desc.is_group_first, info.dyn_desc.io_group_id, info.dyn_desc.io_index,
                         info.dyn_desc.dyn_num, info.dyn_desc.group_size, info.dyn_desc.group_offset,
                         info.dyn_desc.ptr_offset};
    infos[i].data_size = info.data_size;
  }
  chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
  return kOk;
}

aclError InitContinuousVectorVectorInt32Const(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const ContinuousVectorVectorInt32ConstDef *>(definition);
  size_t element_num = 0U;
  for (const auto &vector : def.vectors) {
    element_num += vector.size;
  }
  const size_t total_size = gert::ContinuousVectorVector::GetOverHeadLength(def.vectors.size) +
      def.vectors.size * sizeof(gert::ContinuousVector) + element_num * sizeof(int32_t);
  auto storage = std::make_unique<uint8_t[]>(total_size);
  auto *value = new (storage.get()) gert::ContinuousVectorVector();
  value->Init(def.vectors.size);
  for (const auto &vector : def.vectors) {
    auto *output = value->Add<int32_t>(vector.size);
    if (output == nullptr) {
      return kFailed;
    }
    std::copy(vector.begin(), vector.end(), static_cast<int32_t *>(output->MutableData()));
  }
  chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
  return kOk;
}

template <typename T>
aclError InitContinuousVectorConst(gert::Chain &chain, const void *definition) {
  if (definition == nullptr) {
    return kFailed;
  }
  const auto &def = *static_cast<const ContinuousVectorConstDef<T> *>(definition);
  const size_t total_size = sizeof(gert::ContinuousVector) + def.values.size * sizeof(T);
  auto storage = std::make_unique<uint8_t[]>(total_size);
  auto *value = new (storage.get()) gert::ContinuousVector();
  value->Init(def.values.size);
  if (value->SetSize(def.values.size) != ge::GRAPH_SUCCESS) {
    return kFailed;
  }
  if (def.values.size > 0U) {
    std::copy(def.values.begin(), def.values.end(), static_cast<T *>(value->MutableData()));
  }
  chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
  return kOk;
}

)";
}

bool IsPortableConstType(const Om2ValueType type) {
  switch (type) {
    case Om2ValueType::kBool:
    case Om2ValueType::kInt32T:
    case Om2ValueType::kInt64T:
    case Om2ValueType::kUInt32T:
    case Om2ValueType::kUInt64T:
    case Om2ValueType::kSizeT:
    case Om2ValueType::kGeDataType:
    case Om2ValueType::kGertTensorPlacement:
    case Om2ValueType::kGertCoreTypeIndexPtr:
    case Om2ValueType::kGeOppImplVersion:
    case Om2ValueType::kRtTaskCfgInfoTPtr:
    case Om2ValueType::kAclrtLaunchKernelAttrPtr:
    case Om2ValueType::kAclrtLaunchKernelCfgPtr:
    case Om2ValueType::kGertDfxExeArgPtr:
    case Om2ValueType::kGertBuildTensorAttrPtr:
    case Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr:
    case Om2ValueType::kGertArgsInfosDescPtr:
    case Om2ValueType::kGertContinuousVectorVectorInt32Ptr:
    case Om2ValueType::kTypedContinuousVectorInt64Ptr:
    case Om2ValueType::kTypedContinuousVectorSizeTPtr:
    case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr:
    case Om2ValueType::kGertOpExecuteOptionsPtr:
    case Om2ValueType::kGertMixCoreArgsPtr:
    case Om2ValueType::kAclnnDeterministicConfigPtr:
    case Om2ValueType::kAclnnOriginalDeterministicConfigPtr:
    case Om2ValueType::kMemoryBaseTypeOffsetPtr:
    case Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr:
    case Om2ValueType::kGertTensorPtr:
    case Om2ValueType::kCStringPtr:
    case Om2ValueType::kBytePtr:
      return true;
    default:
      return false;
  }
}

std::string EmitTypedConstChainRef(const DynamicProgramModel &program_model) {
  if (program_model.const_chains.empty()) {
    return "{nullptr, 0U}";
  }
  for (const auto &chain : program_model.const_chains) {
    if (chain.custom) {
      if (chain.initializer.empty() || chain.payload.empty() ||
          (chain.payload_is_string && (chain.payload.back() != 0U))) {
        GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 custom const chain %u has invalid initializer payload", chain.chain_index);
        return {};
      }
      continue;
    }
    if (!IsPortableConstType(chain.value.type)) {
      if (chain.value.is_null_pointer) {
        continue;
      }
      GELOGE(ge::GRAPH_PARAM_INVALID,
             "OM2 const chain %u has non-serializable value type %u; runtime bindings must be supplied by Load/Run",
             chain.chain_index, static_cast<uint32_t>(chain.value.type));
      return {};
    }
  }
  const auto emit_byte_ref = [](const std::vector<uint8_t> &values) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{(const uint8_t[]){";
    for (size_t i = 0U; i < values.size(); ++i) {
      out << static_cast<uint32_t>(values[i]) << "U" << (i + 1U == values.size() ? "" : ", ");
    }
    out << "}, " << values.size() << "U}";
    return out.str();
  };
  const auto emit_typed_array_ref = [](const auto &values, const char *type_name) {
    if (values.empty()) {
      return std::string("{nullptr, 0U}");
    }
    std::ostringstream out;
    out << "{(const " << type_name << "[]){";
    for (size_t i = 0U; i < values.size(); ++i) {
      out << values[i] << (i + 1U == values.size() ? "" : ", ");
    }
    out << "}, " << values.size() << "U}";
    return out.str();
  };
  std::ostringstream ss;
  ss << "{(const ConstChainDef[]){\n";
  for (const auto &chain : program_model.const_chains) {
    const auto &value = chain.value;
    ss << "    {\n"
       << "      " << chain.chain_index << "U,\n"
       << "      \"" << EscapeForStringLiteral(chain.producer_name) << "\",\n"
       << "      ";
    if (chain.custom) {
      if (chain.initializer == "storage_shape") {
        constexpr size_t kShapePayloadSize = sizeof(uint32_t) * 2U + sizeof(int64_t) * gert::Shape::kMaxDimNum * 2U;
        if (chain.payload.size() != kShapePayloadSize) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 storage shape chain %u has invalid payload size %zu", chain.chain_index,
                 chain.payload.size());
          return {};
        }
        const auto read_value = [&chain](const size_t offset, auto &value) {
          return memcpy_s(&value, sizeof(value), chain.payload.data() + offset, sizeof(value)) == EOK;
        };
        uint32_t origin_dim_num = 0U;
        uint32_t storage_dim_num = 0U;
        const size_t storage_base = sizeof(uint32_t) + sizeof(int64_t) * gert::Shape::kMaxDimNum;
        if (!read_value(0U, origin_dim_num) || !read_value(storage_base, storage_dim_num)) {
          return {};
        }
        if ((origin_dim_num > gert::Shape::kMaxDimNum) || (storage_dim_num > gert::Shape::kMaxDimNum)) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 storage shape chain %u has invalid dim count", chain.chain_index);
          return {};
        }
        ss << "&InitStorageShapeConst, (const StorageShapeConstDef[]){{" << origin_dim_num << "U, " << storage_dim_num
           << "U, {";
        for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
          int64_t dim = 0;
          if (!read_value(sizeof(uint32_t) + i * sizeof(int64_t), dim)) {
            return {};
          }
          ss << dim << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
        }
        ss << "}, {";
        for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
          int64_t dim = 0;
          if (!read_value(storage_base + sizeof(uint32_t) + i * sizeof(int64_t), dim)) {
            return {};
          }
          ss << dim << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
        }
        ss << "}}}";
      } else if (chain.initializer == "gert_tensor") {
        if (chain.payload.size() != sizeof(TensorConstPayload)) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor chain %u has invalid payload size %zu", chain.chain_index,
                 chain.payload.size());
          return {};
        }
        TensorConstPayload def{};
        if (memcpy_s(&def, sizeof(def), chain.payload.data(), sizeof(def)) != EOK) {
          return {};
        }
        if ((def.origin_dim_num > gert::Shape::kMaxDimNum) || (def.storage_dim_num > gert::Shape::kMaxDimNum) ||
            (def.expand_dims_size > gert::ExpandDimsType::kMaxExpandSize)) {
          GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor chain %u has invalid shape fields", chain.chain_index);
          return {};
        }
        int32_t placement = def.placement;
        // kTensorPlacementEnd is the ABI-valid state of a Tensor that owns no data, which is what the
        // collector normalizes an address-less, zero-sized payload to so that op impls see the same
        // state RT2 shows them.  Clamping it to kOnHost here would silently undo that and make impls
        // such as ReshapeInferShapeImpl report an empty tensor, so only clamp genuinely invalid values.
        const bool payload_owns_no_data = (def.address == 0ULL) && (def.size == 0ULL);
        const bool payload_placement_valid =
            ((placement >= static_cast<int32_t>(gert::kOnDeviceHbm)) &&
             (placement < static_cast<int32_t>(gert::kTensorPlacementEnd))) ||
            ((placement == static_cast<int32_t>(gert::kTensorPlacementEnd)) && payload_owns_no_data);
        if (!payload_placement_valid) {
          placement = static_cast<int32_t>(gert::kOnHost);
        }
        // Model-weight Tensor constants are rebound from the host-weight
        // table during Load. Keep the captured address as an initialization
        // fallback for runtime-created tensors that are not host weights.
        ss << "&InitGertTensorConst, (const TensorConstDef[]){ {" << def.address << "ULL, " << def.size << "ULL, "
           << placement << ", " << def.data_type << ", " << def.origin_format << ", " << def.storage_format << ", "
           << def.expand_dims_size << "ULL, " << def.expand_dims_mask << "ULL, " << def.origin_dim_num << "U, "
           << def.storage_dim_num << "U, {";
        for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
          ss << def.origin_dims[i] << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
        }
        ss << "}, {";
        for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
          ss << def.storage_dims[i] << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
        }
        ss << "} }}";
      } else {
        std::string initializer_ref;
        if (chain.initializer == "aicpu_host_proc_func") {
          initializer_ref = "&InitAicpuHostProcFuncConst, ";
        } else if (chain.initializer == "tensor_data_offset") {
          initializer_ref = "&InitTensorDataOffsetConst, ";
        } else if (chain.initializer == "gert_tensor_data") {
          initializer_ref = "&InitGertTensorDataConst, ";
        } else if (chain.initializer == "gert_tensor_data_object") {
          initializer_ref = "&InitTensorDataObjectConst, ";
        } else if (chain.initializer == "process_local_pointer") {
          initializer_ref = "&InitProcessLocalPointerConst, ";
        } else if (chain.initializer == "null_pointer") {
          initializer_ref = "&InitNullPointerConst, ";
        } else {
          initializer_ref = "&" + chain.initializer + ", ";
        }
        ss << initializer_ref;
        if (chain.initializer == "gert_tensor_data") {
          if (!chain.payload_is_string || chain.payload.empty() || chain.payload.back() != 0U) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 GertTensorData chain %u has invalid payload", chain.chain_index);
            return {};
          }
          unsigned long long address = 0ULL;
          unsigned long long size = 0ULL;
          int placement = 0;
          long long stream_id = 0LL;
          char trailing = '\0';
          const auto *payload = reinterpret_cast<const char *>(chain.payload.data());
          constexpr int kGertTensorDataFieldCount = 4;
          if (sscanf_s(payload, "%llu,%llu,%d,%lld%c", &address, &size, &placement, &stream_id, &trailing,
                       static_cast<unsigned int>(sizeof(trailing))) != kGertTensorDataFieldCount) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 GertTensorData chain %u has malformed payload", chain.chain_index);
            return {};
          }
          if ((address > static_cast<unsigned long long>(std::numeric_limits<uintptr_t>::max())) ||
              (size > static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) ||
              (placement < static_cast<int>(gert::kOnDeviceHbm)) ||
              (placement >= static_cast<int>(gert::kTensorPlacementEnd))) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 GertTensorData chain %u has invalid fields", chain.chain_index);
            return {};
          }
          ss << "(const GertTensorDataConstDef[]){ {" << address << "ULL, " << size << "ULL, " << placement << ", "
             << stream_id << "LL} }";
        } else if (chain.initializer == "gert_tensor_data_object") {
          if (!chain.payload_is_string || chain.payload.empty() || (chain.payload.back() != 0U)) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 TensorData chain %u has invalid payload", chain.chain_index);
            return {};
          }
          unsigned long long address = 0ULL;
          unsigned long long size = 0ULL;
          int placement = 0;
          char trailing = '\0';
          const auto *payload = reinterpret_cast<const char *>(chain.payload.data());
          constexpr int kTensorDataFieldCount = 3;
          if (sscanf_s(payload, "%llu,%llu,%d%c", &address, &size, &placement, &trailing,
                       static_cast<unsigned int>(sizeof(trailing))) != kTensorDataFieldCount) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 TensorData chain %u has malformed payload", chain.chain_index);
            return {};
          }
          // Mirror InitTensorDataObjectConst's contract so a bad chain fails here, with its index in
          // the log, instead of surfacing as an opaque kFailed at load time.
          const bool owns_no_data = (address == 0ULL) && (size == 0ULL);
          const bool placement_valid = ((placement >= static_cast<int>(gert::kOnDeviceHbm)) &&
                                        (placement < static_cast<int>(gert::kTensorPlacementEnd))) ||
                                       ((placement == static_cast<int>(gert::kTensorPlacementEnd)) && owns_no_data);
          if ((address > static_cast<unsigned long long>(std::numeric_limits<uintptr_t>::max())) ||
              (size > static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) || !placement_valid) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 TensorData chain %u has invalid fields: placement=%d size=%llu",
                   chain.chain_index, placement, size);
            return {};
          }
          ss << "(const TensorDataObjectConstDef[]){ {" << address << "ULL, " << size << "ULL, " << placement << "} }";
        } else if (chain.payload_is_string) {
          ss << EmitWrappedStringLiteral(
              std::string(reinterpret_cast<const char *>(chain.payload.data()), chain.payload.size() - 1U), "      ");
        } else {
          ss << "(const uint8_t[]){";
          for (size_t i = 0U; i < chain.payload.size(); ++i) {
            ss << static_cast<uint32_t>(chain.payload[i]) << "U" << (i + 1U == chain.payload.size() ? "" : ", ");
          }
          ss << "}";
        }
      }
    } else if (value.is_null_pointer) {
      ss << "&InitNullConst, nullptr";
    } else {
      switch (value.type) {
        case Om2ValueType::kBool:
          ss << "&InitInlineConst<bool>, (const bool[]){" << (value.unsigned_value != 0U ? "true" : "false") << "}";
          break;
        case Om2ValueType::kInt32T:
          ss << "&InitInlineConst<int32_t>, (const int32_t[]){" << value.signed_value << "}";
          break;
        case Om2ValueType::kInt64T:
          ss << "&InitInlineConst<int64_t>, (const int64_t[]){" << value.signed_value << "}";
          break;
        case Om2ValueType::kUInt32T:
        case Om2ValueType::kGeDataType:
        case Om2ValueType::kGertTensorPlacement:
        case Om2ValueType::kGertCoreTypeIndexPtr:
        case Om2ValueType::kGeOppImplVersion:
          ss << "&InitInlineConst<uint32_t>, (const uint32_t[]){" << value.unsigned_value << "U}";
          break;
        case Om2ValueType::kUInt64T:
          ss << "&InitInlineConst<uint64_t>, (const uint64_t[]){" << value.unsigned_value << "U}";
          break;
        case Om2ValueType::kSizeT:
          ss << "&InitInlineConst<size_t>, (const size_t[]){" << value.unsigned_value << "U}";
          break;
        case Om2ValueType::kCStringPtr:
          ss << "&InitStringConst,\n      " << EmitWrappedStringLiteral(value.string_value, "      ");
          break;
        case Om2ValueType::kBytePtr:
          ss << "&InitRawBytesConst, (const uint8_t[]){";
          if (value.raw_bytes.empty()) {
            ss << "0U";
          } else {
            for (size_t i = 0U; i < value.raw_bytes.size(); ++i) {
              ss << static_cast<uint32_t>(value.raw_bytes[i]) << "U" << (i + 1U == value.raw_bytes.size() ? "" : ", ");
            }
          }
          ss << "}";
          break;
        case Om2ValueType::kRtTaskCfgInfoTPtr:
          ss << "&InitTaskConfigConst, nullptr";
          break;
        case Om2ValueType::kAclrtLaunchKernelAttrPtr:
          ss << "&InitLaunchKernelAttrsConst, (const GeneratedArrayRef<LaunchKernelAttrConstDef>[]){{"
             << "(const LaunchKernelAttrConstDef[]){";
          for (size_t i = 0U; i < value.launch_kernel_attrs.size(); ++i) {
            const auto &attr = value.launch_kernel_attrs[i];
            ss << "{" << attr.id << ", " << attr.value << "U}"
               << (i + 1U == value.launch_kernel_attrs.size() ? "" : ", ");
          }
          ss << "}, " << value.launch_kernel_attrs.size() << "U}}";
          break;
        case Om2ValueType::kAclrtLaunchKernelCfgPtr:
          ss << "&InitLaunchKernelConfigConst, (const size_t[]){" << value.unsigned_value << "U}";
          break;
        case Om2ValueType::kGertDfxExeArgPtr:
          ss << "&InitDfxExeArgConst, (const DfxExeArgConstDef[]){{"
             << (value.dfx_exe_arg.need_assert ? "true" : "false") << ", "
             << (value.dfx_exe_arg.need_print ? "true" : "false") << ", " << value.dfx_exe_arg.buffer_size << "}}";
          break;
        case Om2ValueType::kGertBuildTensorAttrPtr:
          ss << "&InitBuildTensorAttrConst, (const BuildTensorAttrConstDef[]){{" << value.build_tensor_attr.placement
             << ", " << value.build_tensor_attr.data_type << ", " << value.build_tensor_attr.storage_format << ", "
             << value.build_tensor_attr.origin_format << "}}";
          break;
        case Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr: {
          const auto &def = value.compute_node_desc;
          ss << "&InitComputeNodeDescConst, (const ComputeNodeDescConstDef[]){{" << def.input_num << "U, "
             << def.output_num << "U, " << def.workspace_cap << "U, " << def.max_tiling_data << "U, "
             << (def.need_shape_buffer ? "true" : "false") << ", " << (def.need_overflow ? "true" : "false") << ", "
             << emit_byte_ref(def.compiled_args) << "}}";
          break;
        }
        case Om2ValueType::kGertArgsInfosDescPtr: {
          const auto &def = value.args_infos_desc;
          ss << "&InitArgsInfosDescConst,\n"
             << "      (const ArgsInfosDescConstDef[]){{\n"
             << "        " << def.input_args_info_num << "U,\n"
             << "        " << def.output_args_info_num << "U,\n"
             << "        " << def.input_valid_num << "U,\n"
             << "        " << def.output_valid_num << "U,\n"
             << "        ";
          if (def.args_infos.empty()) {
            ss << "{nullptr, 0U}";
          } else {
            ss << "{(const ArgsInfoConstDef[]){\n";
            for (size_t i = 0U; i < def.args_infos.size(); ++i) {
              const auto &info = def.args_infos[i];
              ss << "          {\n"
                 << "            " << info.arg_type << ",\n"
                 << "            " << info.arg_format << ",\n"
                 << "            " << info.start_index << ",\n"
                 << "            " << info.arg_size << "U,\n"
                 << "            " << (info.folded_first ? "true" : "false") << ",\n"
                 << "            " << (info.be_folded ? "true" : "false") << ",\n"
                 << "            {" << (info.dyn_desc.is_group_first ? "true" : "false") << ", "
                 << info.dyn_desc.io_group_id << "U, " << info.dyn_desc.io_index << "U, " << info.dyn_desc.dyn_num
                 << "U, " << info.dyn_desc.group_size << "U, " << info.dyn_desc.group_offset << "U, "
                 << info.dyn_desc.ptr_offset << "U},\n"
                 << "            " << info.data_size << "U\n"
                 << "          }" << (i + 1U == def.args_infos.size() ? "" : ",") << "\n";
            }
            ss << "        }, " << def.args_infos.size() << "U}";
          }
          ss << "\n      }}";
          break;
        }
        case Om2ValueType::kGertContinuousVectorVectorInt32Ptr: {
          const auto &vectors = value.continuous_vector_vector_int32;
          ss << "&InitContinuousVectorVectorInt32Const,\n"
             << "      (const ContinuousVectorVectorInt32ConstDef[]){{\n"
             << "        ";
          if (vectors.empty()) {
            ss << "{nullptr, 0U}";
          } else {
            ss << "{(const GeneratedArrayRef<int32_t>[]){\n";
            for (size_t i = 0U; i < vectors.size(); ++i) {
              if (vectors[i].empty()) {
                ss << "          {nullptr, 0U}";
              } else {
                ss << "          {(const int32_t[]){";
                for (size_t j = 0U; j < vectors[i].size(); ++j) {
                  ss << vectors[i][j] << (j + 1U == vectors[i].size() ? "" : ", ");
                }
                ss << "}, " << vectors[i].size() << "U}";
              }
              ss << (i + 1U == vectors.size() ? "" : ",") << "\n";
            }
            ss << "        }, " << vectors.size() << "U}";
          }
          ss << "\n      }}";
          break;
        }
        case Om2ValueType::kTypedContinuousVectorInt64Ptr: {
          const auto &values = value.continuous_vector_int64;
          ss << "&InitContinuousVectorConst<int64_t>,\n"
             << "      (const ContinuousVectorConstDef<int64_t>[]){" << emit_typed_array_ref(values, "int64_t") << "}";
          break;
        }
        case Om2ValueType::kTypedContinuousVectorSizeTPtr: {
          const auto &values = value.continuous_vector_size_t;
          ss << "&InitContinuousVectorConst<size_t>,\n"
             << "      (const ContinuousVectorConstDef<size_t>[]){" << emit_typed_array_ref(values, "size_t") << "}";
          break;
        }
        case Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr: {
          const auto &values = value.continuous_vector_args_index_to_io_index;
          ss << "&InitContinuousVectorConst<GeneratedArgsIndexToIoIndex>,\n"
             << "      (const ContinuousVectorConstDef<GeneratedArgsIndexToIoIndex>[]){";
          if (values.empty()) {
            ss << "{nullptr, 0U}";
          } else {
            ss << "{(const GeneratedArgsIndexToIoIndex[]){";
            for (size_t i = 0U; i < values.size(); ++i) {
              const auto &item = values[i];
              ss << "{" << item.args_role << ", " << item.args_index << "U, " << item.io_index << "U}"
                 << (i + 1U == values.size() ? "" : ", ");
            }
            ss << "}, " << values.size() << "U}";
          }
          ss << "}";
          break;
        }
        case Om2ValueType::kGertOpExecuteOptionsPtr:
          ss << "&InitOpExecuteOptionsConst, (const OpExecuteOptionsConstDef[]){{"
             << emit_byte_ref(value.op_execute_options) << "}}";
          break;
        case Om2ValueType::kGertMixCoreArgsPtr: {
          const auto &mix_args = value.mix_core_args;
          ss << "&InitMixCoreArgsConst, (const MixCoreArgsConstDef[]){{" << mix_args.mix_type << "U, "
             << mix_args.all_core_num << "U, " << mix_args.vec_core_num << "U, " << mix_args.ai_core_num << "U}}";
          break;
        }
        case Om2ValueType::kAclnnDeterministicConfigPtr: {
          const auto &config = value.aclnn_deterministic_config;
          ss << "&InitOwnedConst<gert::kernel::AclnnDeterministicConfig>, "
             << "(const gert::kernel::AclnnDeterministicConfig[]){{" << (config.has_op_deterministic ? "true" : "false")
             << ", " << config.op_deterministic << ", " << (config.has_op_deterministic_level ? "true" : "false")
             << ", " << config.op_deterministic_level << "}}";
          break;
        }
        case Om2ValueType::kAclnnOriginalDeterministicConfigPtr: {
          const auto &config = value.aclnn_original_deterministic_config;
          ss << "&InitOwnedConst<gert::kernel::AclnnOriginalDeterministicConfig>, "
             << "(const gert::kernel::AclnnOriginalDeterministicConfig[]){{" << config.deterministic << ", "
             << config.deterministic_level << ", {}}}";
          break;
        }
        case Om2ValueType::kMemoryBaseTypeOffsetPtr: {
          const auto &offsets = value.memory_base_type_offsets;
          if (offsets.empty()) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 MemoryBaseTypeOffset chain %u has no items", chain.chain_index);
            return {};
          }
          ss << "&InitMemoryBaseTypeOffsetConst, (const MemoryBaseTypeOffsetConstDef[]){";
          for (size_t i = 0U; i < offsets.size(); ++i) {
            ss << "{" << offsets[i].base_type << "U, " << offsets[i].offset << "LL, " << offsets[i].size << "ULL}"
               << (i + 1U == offsets.size() ? "" : ", ");
          }
          ss << "}";
          break;
        }
        case Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr: {
          const auto &name_and_mems = value.file_constant_name_and_mems;
          ss << "&InitContinuousVectorConst<GeneratedFileConstantNameAndMem>, "
             << "(const ContinuousVectorConstDef<GeneratedFileConstantNameAndMem>[]){";
          if (name_and_mems.empty()) {
            ss << "{nullptr, 0U}";
          } else {
            ss << "{(const GeneratedFileConstantNameAndMem[]){";
            for (size_t i = 0U; i < name_and_mems.size(); ++i) {
              ss << "{\"" << EscapeForStringLiteral(name_and_mems[i].name) << "\", "
                 << "(const void *)(uintptr_t)" << name_and_mems[i].mem << "ULL, " << name_and_mems[i].size << "U}"
                 << (i + 1U == name_and_mems.size() ? "" : ", ");
            }
            ss << "}, " << name_and_mems.size() << "U}";
          }
          ss << "}";
          break;
        }
        case Om2ValueType::kGertTensorPtr: {
          // Tensor constants carry portable metadata; their data address is
          // patched from the OM2 host-weight table during Load. Never emit the
          // process-local pointer captured in the source Tensor object.
          const auto &tensor = value.tensor;
          if ((tensor.origin_shape.size() > gert::Shape::kMaxDimNum) ||
              (tensor.storage_shape.size() > gert::Shape::kMaxDimNum)) {
            GELOGE(ge::GRAPH_PARAM_INVALID, "OM2 Tensor chain %u has invalid shape rank", chain.chain_index);
            return {};
          }
          int32_t placement = tensor.placement;
          if ((placement < static_cast<int32_t>(gert::kOnDeviceHbm)) ||
              (placement >= static_cast<int32_t>(gert::kTensorPlacementEnd))) {
            // FX converter Tensor metadata can carry the kTensorPlacementEnd
            // sentinel even when the payload is backed by host weights.  OM2
            // binds all serialized Tensor payloads from host-weight memory, so
            // normalize that sentinel to the ABI-valid host placement.
            placement = static_cast<int32_t>(gert::kOnHost);
          }
          ss << "&InitGertTensorConst, (const TensorConstDef[]){ {0ULL, 0ULL, " << placement << ", " << tensor.data_type
             << ", " << tensor.origin_format << ", " << tensor.storage_format << ", 0ULL, 0ULL, "
             << tensor.origin_shape.size() << "U, " << tensor.storage_shape.size() << "U, {";
          for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
            ss << (i < tensor.origin_shape.size() ? tensor.origin_shape[i] : 0LL) << "LL"
               << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
          }
          ss << "}, {";
          for (size_t i = 0U; i < gert::Shape::kMaxDimNum; ++i) {
            ss << (i < tensor.storage_shape.size() ? tensor.storage_shape[i] : 0LL) << "LL"
               << (i + 1U == gert::Shape::kMaxDimNum ? "" : ", ");
          }
          ss << "} }}";
          break;
        }
        case Om2ValueType::kVoidPtr:
        case Om2ValueType::kGertTensorDataPtr:
        case Om2ValueType::kGertTensorDataObjectPtr:
        case Om2ValueType::kGertShapePtr:
        case Om2ValueType::kGertStorageShapePtr:
        case Om2ValueType::kGertAllocatorPtr:
        case Om2ValueType::kGeAllocatorPtr:
        case Om2ValueType::kGertAllocatorsPtr:
        case Om2ValueType::kRtStreamTPtr:
        case Om2ValueType::kTypedContinuousVectorRtStreamTPtr:
        case Om2ValueType::kRtNotifyTPtr:
        case Om2ValueType::kTypedContinuousVectorRtNotifyTPtr:
        case Om2ValueType::kGertAllocatorVectorPtr:
        case Om2ValueType::kGertTensorDataVectorPtr:
        case Om2ValueType::kGertModelDescPtr:
        case Om2ValueType::kRtSessionPtr:
        case Om2ValueType::kOuterWeightMemPtr:
        case Om2ValueType::kGertOpImplSpaceRegistryV2Ptr:
        case Om2ValueType::kGertCustomOpRegistryPtr:
        case Om2ValueType::kGertKernelBinDataPtr:
        case Om2ValueType::kGeKernelBinPtr:
        case Om2ValueType::kKernelFuncPtr:
        case Om2ValueType::kGertRtKernelLaunchArgsExPtr:
        case Om2ValueType::kFePlatFormInfosPtr:
        case Om2ValueType::kGertTilingFwkDataPtr:
        case Om2ValueType::kGertCacheableTilingFwkDataPtr:
        case Om2ValueType::kGeOpDescPtr:
        case Om2ValueType::kAicpuArgsHandlerPtr:
        case Om2ValueType::kGertAicpuExtInfoHandlerPtr:
        case Om2ValueType::kGertCoreNumInfosPtr:
        case Om2ValueType::kAicpuHostProcFuncPtr:
        case Om2ValueType::kGeDavinciModelPtr:
        case Om2ValueType::kGertOpImplSpaceRegistryV2ArrayPtr:
          return {};
        case Om2ValueType::kUnknown:
          return {};
      }
    }
    ss << "\n    },\n";
  }
  ss << "  }, " << program_model.const_chains.size() << "U}";
  return ss.str();
}

std::vector<uint32_t> CollectBindingChains(const DynamicProgramModel &program_model,
                                           const ChainBindingKind binding_kind,
                                           const ChainBindingPhase binding_phase = ChainBindingPhase::kLoad) {
  std::vector<std::pair<int64_t, uint32_t>> ordered;
  for (const auto &chain : program_model.chains) {
    if ((chain.binding_kind == binding_kind) && (chain.binding_phase == binding_phase)) {
      ordered.emplace_back(chain.source_index, chain.index);
    }
  }
  std::sort(ordered.begin(), ordered.end(), [](const auto &lhs, const auto &rhs) {
    if (lhs.first != rhs.first) {
      return lhs.first < rhs.first;
    }
    return lhs.second < rhs.second;
  });
  std::vector<uint32_t> indices;
  indices.reserve(ordered.size());
  for (const auto &item : ordered) {
    indices.push_back(item.second);
  }
  return indices;
}

ge::graphStatus CollectHostTensorBindings(const DynamicProgramModel &program_model,
                                          std::vector<HostTensorBindingCodegenInfo> &bindings) {
  for (const auto &chain : program_model.const_chains) {
    if (chain.custom && chain.initializer == "gert_tensor") {
      if (chain.payload.size() != sizeof(TensorConstPayload)) {
        continue;
      }
      TensorConstPayload payload{};
      GE_ASSERT_EOK(memcpy_s(&payload, sizeof(payload), chain.payload.data(), sizeof(payload)));
      if ((payload.has_weight_index != 0U) && (payload.size != 0U)) {
        bindings.push_back({chain.chain_index, static_cast<uint32_t>(payload.weight_index), payload.weight_data_offset,
                            payload.weight_data_size});
      }
      continue;
    }
    if ((chain.value.type != Om2ValueType::kGertTensorPtr) || !chain.value.tensor.has_weight_index ||
        (chain.value.tensor.data_size == 0U)) {
      continue;
    }
    bindings.push_back({chain.chain_index, chain.value.tensor.weight_index, chain.value.tensor.data_offset,
                        chain.value.tensor.data_size});
  }
  return ge::GRAPH_SUCCESS;
}

std::vector<BinaryBindingCodegenInfo> CollectBinaryBindings(const DynamicProgramModel &program_model) {
  std::vector<BinaryBindingCodegenInfo> bindings;
  for (const auto &chain : program_model.chains) {
    if (((chain.binding_kind == ChainBindingKind::kBinary) ||
         (chain.binding_kind == ChainBindingKind::kGeKernelBinary)) &&
        (chain.binding_phase == ChainBindingPhase::kLoad)) {
      bindings.push_back({chain.index, chain.resource_key, chain.binding_kind == ChainBindingKind::kGeKernelBinary});
    }
  }
  return bindings;
}

std::vector<uint32_t> CollectModelIoChains(const DynamicProgramModel &program_model,
                                           const ChainSourceKind source_kind) {
  std::vector<std::pair<int64_t, uint32_t>> ordered;
  for (const auto &chain : program_model.chains) {
    if (chain.source_kind == source_kind) {
      ordered.emplace_back(chain.source_index, chain.index);
    }
  }
  std::sort(ordered.begin(), ordered.end(), [](const auto &lhs, const auto &rhs) {
    if (lhs.first != rhs.first) {
      return lhs.first < rhs.first;
    }
    return lhs.second < rhs.second;
  });
  std::vector<uint32_t> indices;
  indices.reserve(ordered.size());
  for (const auto &item : ordered) {
    indices.push_back(item.second);
  }
  return indices;
}

std::vector<uint32_t> CollectSourceChains(const DynamicProgramModel &program_model, const ChainSourceKind source_kind) {
  std::vector<uint32_t> indices;
  for (const auto &chain : program_model.chains) {
    if (chain.source_kind == source_kind) {
      indices.push_back(chain.index);
    }
  }
  return indices;
}

std::string EmitIndexRef(const std::vector<uint32_t> &indices) {
  if (indices.empty()) {
    return "{nullptr, 0U}";
  }
  std::ostringstream ss;
  ss << "{(const uint32_t[]){";
  for (size_t i = 0U; i < indices.size(); ++i) {
    if (i > 0U) {
      ss << ", ";
    }
    ss << indices[i] << "U";
  }
  ss << "}, " << indices.size() << "U}";
  return ss.str();
}

std::string EmitPortRef(const std::vector<KernelPortInfo> &ports) {
  if (ports.empty()) {
    return "{nullptr, 0U}";
  }
  std::ostringstream ss;
  ss << "{(const KernelPortDef[]){\n";
  for (const auto &port : ports) {
    std::string semantic_name = port.semantic_name;
    std::replace(semantic_name.begin(), semantic_name.end(), '\n', ' ');
    std::replace(semantic_name.begin(), semantic_name.end(), '\r', ' ');
    ss << "          {" << port.chain_index << "U, \"" << EscapeForStringLiteral(semantic_name) << "\", "
       << static_cast<uint32_t>(port.value_type) << "U},\n";
  }
  ss << "        }, " << ports.size() << "U}";
  return ss.str();
}

std::string EmitProgramPhase(const std::vector<DynamicKernelCodegenInfo> &kernels) {
  if (kernels.empty()) {
    return "{nullptr, 0U}";
  }
  std::ostringstream ss;
  ss << "{(const KernelDef[]){\n";
  for (size_t i = 0U; i < kernels.size(); ++i) {
    const auto &kernel = kernels[i];
    ss << "      {\"" << EscapeForStringLiteral(kernel.execution_node_name) << "\", \""
       << EscapeForStringLiteral(kernel.kernel_type) << "\",\n        " << EmitPortRef(kernel.inputs) << ",\n        "
       << EmitPortRef(kernel.outputs) << ",\n        ";
    if (kernel.compute_node_index == std::numeric_limits<uint32_t>::max()) {
      ss << "kNoComputeNodeIndex";
    } else {
      ss << kernel.compute_node_index << "U";
    }
    ss << ", " << static_cast<uint32_t>(kernel.impl_type) << "U, " << static_cast<uint32_t>(kernel.output_policy)
       << "U}";
    if ((i + 1U) < kernels.size()) {
      ss << ",";
    }
    ss << "\n";
  }
  ss << "    }, " << kernels.size() << "U}";
  return ss.str();
}

std::string EmitProgramInitializer(const DynamicProgramModel &program_model) {
  std::ostringstream ss;
  ss << "{\n  " << EmitProgramPhase(program_model.init) << ",\n  " << EmitProgramPhase(program_model.main) << ",\n  "
     << EmitProgramPhase(program_model.deinit) << "\n}";
  return ss.str();
}

std::string EmitRuntimeBindingInitializer(
    const std::vector<uint32_t> &stream_chains, const std::vector<uint32_t> &external_allocator_chains,
    const std::vector<uint32_t> &model_desc_chains, const std::vector<uint32_t> &space_registry_chains,
    const std::vector<uint32_t> &custom_op_registry_chains,
    const std::vector<BinaryBindingCodegenInfo> &binary_bindings, const std::vector<uint32_t> &outer_weight_mem_chains,
    const std::vector<HostTensorBindingCodegenInfo> &host_tensor_bindings,
    const std::vector<uint32_t> &session_id_chains, const std::vector<uint32_t> &notify_chains,
    const std::vector<uint32_t> &rt_session_chains) {
  std::ostringstream ss;
  ss << "{\n"
     << "  " << EmitIndexRef(stream_chains) << ",\n"
     << "  " << EmitIndexRef(external_allocator_chains) << ",\n"
     << "  " << EmitIndexRef(model_desc_chains) << ",\n"
     << "  " << EmitIndexRef(space_registry_chains) << ",\n"
     << "  " << EmitIndexRef(custom_op_registry_chains) << ",\n";
  if (binary_bindings.empty()) {
    ss << "  {nullptr, 0U},\n";
  } else {
    ss << "  {(const RuntimeBindingDef::BinaryBinding[]){\n";
    for (size_t i = 0U; i < binary_bindings.size(); ++i) {
      ss << "    {" << binary_bindings[i].chain_index << "U, \"" << EscapeForStringLiteral(binary_bindings[i].key)
         << "\", " << (binary_bindings[i].is_ge_kernel_bin ? "true" : "false") << "}"
         << ((i + 1U == binary_bindings.size()) ? "" : ",") << "\n";
    }
    ss << "  }, " << binary_bindings.size() << "U},\n";
  }
  ss << "  " << EmitIndexRef(outer_weight_mem_chains) << ",\n";
  if (host_tensor_bindings.empty()) {
    ss << "  {nullptr, 0U},\n";
  } else {
    ss << "  {(const RuntimeBindingDef::HostTensorBinding[]){\n";
    for (size_t i = 0U; i < host_tensor_bindings.size(); ++i) {
      const auto &binding = host_tensor_bindings[i];
      ss << "    {" << binding.chain_index << "U, " << binding.weight_index << "U, " << binding.data_offset << "ULL, "
         << binding.data_size << "ULL}" << ((i + 1U == host_tensor_bindings.size()) ? "" : ",") << "\n";
    }
    ss << "  }, " << host_tensor_bindings.size() << "U},\n";
  }
  ss << "  " << EmitIndexRef(session_id_chains) << ",\n"
     << "  " << EmitIndexRef(notify_chains) << ",\n"
     << "  " << EmitIndexRef(rt_session_chains) << "\n}";
  return ss.str();
}

Expr *BuildChainMetaInit(AstBuildContext &ast, const DynamicChainCodegenInfo &chain) {
  return ast.InitList(
      {ast.UInt(chain.index), ast.ULong(chain.symbol), ast.UInt(static_cast<uint64_t>(chain.source_kind)),
       ast.UInt(static_cast<uint64_t>(chain.binding_phase)), ast.UInt(static_cast<uint64_t>(chain.binding_kind)),
       ast.Str(chain.cpp_symbol)},
      false);
}

Expr *BuildKernelMetaInit(AstBuildContext &ast, const DynamicKernelCodegenInfo &kernel) {
  return ast.InitList(
      {ast.Str(EscapeForStringLiteral(kernel.execution_node_name)), ast.Str(EscapeForStringLiteral(kernel.kernel_type)),
       ast.UInt(kernel.inputs.size()), ast.UInt(kernel.outputs.size()), ast.UInt(kernel.compute_node_index),
       ast.UInt(static_cast<uint64_t>(kernel.output_policy)), ast.UInt(static_cast<uint64_t>(kernel.impl_type)),
       kernel.require_compute_node_info, ast.Str(kernel.cpp_symbol)},
      false);
}

Expr *BuildProgramMetadataInit(AstBuildContext &ast, const DynamicProgramModel &program_model,
                               const ProgramBodyCodegenOption &option) {
  return ast.InitList(
      {ast.Str(EscapeForStringLiteral(option.model_name)), "kChains", ast.ULong(program_model.chains.size()),
       "kInitKernels", ast.ULong(program_model.init.size()), "kMainKernels", ast.ULong(program_model.main.size()),
       "kDeInitKernels", ast.ULong(program_model.deinit.size()), ast.ULong(program_model.resources.buffer.size()),
       ast.ULong(program_model.resources.compute_node_infos.size()),
       ast.ULong(program_model.resources.kernel_extend_infos.size()),
       ast.ULong(program_model.resources.model_desc.size())},
      true);
}

FieldDecl *BuildKernelArrayDecl(AstBuildContext &ast, const std::string &name,
                                const std::vector<DynamicKernelCodegenInfo> &kernels) {
  std::vector<Arg> items;
  items.reserve(kernels.size());
  for (const auto &kernel : kernels) {
    items.emplace_back(BuildKernelMetaInit(ast, kernel));
  }
  return ast.Field("static const GeneratedKernelMeta", name + "[]", ast.InitList(items, true));
}

FieldDecl *BuildChainArrayDecl(AstBuildContext &ast, const std::vector<DynamicChainCodegenInfo> &chains) {
  std::vector<Arg> items;
  items.reserve(chains.size());
  for (const auto &chain : chains) {
    items.emplace_back(BuildChainMetaInit(ast, chain));
  }
  return ast.Field("static const GeneratedChainMeta", "kChains[]", ast.InitList(items, true));
}

FieldDecl *BuildProgramMetadataDecl(AstBuildContext &ast, const DynamicProgramModel &program_model,
                                    const ProgramBodyCodegenOption &option) {
  return ast.Field("static const GeneratedProgramMetadata", "kProgramMetadata",
                   BuildProgramMetadataInit(ast, program_model, option));
}

FieldDecl *BuildKernelSequenceDecl(AstBuildContext &ast, const std::string &name,
                                   const std::vector<DynamicKernelCodegenInfo> &kernels) {
  std::vector<Arg> items;
  items.reserve(kernels.size());
  for (const auto &kernel : kernels) {
    items.emplace_back(ast.Str(kernel.cpp_symbol));
  }
  return ast.Field("static const char *const", name + "[]", ast.InitList(items, true));
}

FieldDecl *BuildChainSymbolSequenceDecl(AstBuildContext &ast, const std::vector<DynamicChainCodegenInfo> &chains) {
  std::vector<Arg> items;
  items.reserve(chains.size());
  for (const auto &chain : chains) {
    items.emplace_back(ast.Str(chain.cpp_symbol));
  }
  return ast.Field("static const char *const", "kChainSymbols[]", ast.InitList(items, true));
}

[[maybe_unused]] std::vector<DeclNode *> BuildSourceTranslationUnit(AstBuildContext &ast,
                                                                    const DynamicProgramModel &program_model,
                                                                    const ProgramBodyCodegenOption &option) {
  return {
      ast.Include(option.interface_file_name, IncludeDecl::Kind::kQuote),
      ast.Include("new", IncludeDecl::Kind::kAngle),
      ast.Space(),
      ast.Namespace("om2", {BuildChainSymbolSequenceDecl(ast, program_model.chains),
                            BuildChainArrayDecl(ast, program_model.chains),
                            BuildKernelSequenceDecl(ast, "kInitKernelSymbols", program_model.init),
                            BuildKernelArrayDecl(ast, "kInitKernels", program_model.init),
                            BuildKernelSequenceDecl(ast, "kMainKernelSymbols", program_model.main),
                            BuildKernelArrayDecl(ast, "kMainKernels", program_model.main),
                            BuildKernelSequenceDecl(ast, "kDeInitKernelSymbols", program_model.deinit),
                            BuildKernelArrayDecl(ast, "kDeInitKernels", program_model.deinit),
                            BuildProgramMetadataDecl(ast, program_model, option)}),
  };
}

// Fused step role for the ops/sequence emitters: 1 = group start (emit one fused row),
// 0 = group member (skip, already covered by the fused row), -1 = not fused.
int FusedStepRole(const DynamicProgramModel &program_model, const uint32_t region_index, const size_t step_index,
                  const FusedGroupInfo **out_group = nullptr) {
  for (const auto &group : program_model.main_fused_groups) {
    if ((group.region_index != region_index) || (step_index < group.step_begin) || (step_index >= group.step_end)) {
      continue;
    }
    const bool is_start = (step_index == group.step_begin);
    if (is_start && (out_group != nullptr)) {
      *out_group = &group;
    }
    return is_start ? 1 : 0;
  }
  return -1;
}

bool EmitRegionRunCalls(const DynamicProgramModel &program_model, const uint32_t region_index, const size_t indent,
                        std::unordered_set<uint32_t> &active_regions, std::ostringstream &calls);

bool EmitIfRunCalls(const DynamicProgramModel &program_model, const ProgramStep &step, const size_t indent,
                    std::unordered_set<uint32_t> &active_regions, std::ostringstream &calls) {
  if ((step.if_info.selector_chain_index >= program_model.chains.size()) ||
      (step.if_info.selector_value_type != Om2ValueType::kUInt32T)) {
    return false;
  }
  const std::string current_indent(indent, ' ');
  const auto condition_region_index = (step.condition_region_index != kInvalidProgramIndex)
                                          ? step.condition_region_index
                                          : step.if_info.condition_region_index;
  if (condition_region_index != kInvalidProgramIndex) {
    if (!EmitRegionRunCalls(program_model, condition_region_index, indent, active_regions, calls)) {
      return false;
    }
  }
  calls << current_indent << "switch (chains_[" << step.if_info.selector_chain_index << "U].GetValue<uint32_t>()) {\n";
  std::unordered_set<uint32_t> selector_values;
  for (const auto &branch : step.if_info.branches) {
    if (!selector_values.insert(branch.selector_value).second) {
      return false;
    }
    calls << current_indent << "  case " << branch.selector_value << "U:\n";
    if (!EmitRegionRunCalls(program_model, branch.region_index, indent + 4U, active_regions, calls)) {
      return false;
    }
    calls << current_indent << "    break;\n";
  }
  calls << current_indent << "  default:\n" << current_indent << "    return kFailed;\n" << current_indent << "}\n";
  return true;
}

bool EmitRegionRunCalls(const DynamicProgramModel &program_model, const uint32_t region_index, const size_t indent,
                        std::unordered_set<uint32_t> &active_regions, std::ostringstream &calls) {
  if ((region_index >= program_model.main_regions.size()) || !active_regions.insert(region_index).second) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] invalid or recursive region index %u (region count %zu)", region_index,
           program_model.main_regions.size());
    return false;
  }
  const auto &region = program_model.main_regions[region_index];
  const std::string current_indent(indent, ' ');
  const auto flush_kernel_run = [&program_model, &calls, &current_indent](
                                    const uint32_t local_region_index, const size_t run_begin, const size_t run_end) {
    const auto &steps = program_model.main_regions[local_region_index].steps;
    if (run_end <= run_begin) {
      return true;
    }
    const std::string ops_name =
        "kMainRunR" + std::to_string(local_region_index) + "_" + std::to_string(run_begin) + "Ops";
    const std::string seq_name = "kMainRunR" + std::to_string(local_region_index) + "_" + std::to_string(run_begin);
    calls << current_indent << "static constexpr uint8_t " << ops_name << "[] = {";
    bool ops_first = true;
    for (size_t index = run_begin; index < run_end; ++index) {
      const auto &step = steps[index];
      if ((step.kind != ProgramStepKind::kKernel) || (step.kernel_index >= program_model.main.size())) {
        return false;
      }
      const FusedGroupInfo *group = nullptr;
      if (FusedStepRole(program_model, local_region_index, index, &group) == 0) {
        continue;  // fused member: covered by the fused row emitted at the group start
      }
      if (!ops_first) {
        calls << ", ";
      }
      ops_first = false;
      if (group != nullptr) {
        calls << "1U";
        continue;
      }
      const auto &kernel = program_model.main[step.kernel_index];
      calls << ((kernel.impl_type == KernelImplType::kCodegenImpl)
                    ? "1U"
                    : ((kernel.impl_type == KernelImplType::kEmptyImpl) ? "2U" : "0U"));
    }
    calls << "};\n" << current_indent << "static constexpr uint32_t " << seq_name << "[] = {";
    bool seq_first = true;
    for (size_t index = run_begin; index < run_end; ++index) {
      const FusedGroupInfo *group = nullptr;
      const int role = FusedStepRole(program_model, local_region_index, index, &group);
      if (role == 0) {
        continue;
      }
      if (!seq_first) {
        calls << ", ";
      }
      seq_first = false;
      calls << ((role == 1) ? group->fused_dispatch_index : steps[index].kernel_index) << "U";
    }
    calls << "};\n"
          << current_indent << "OM2_ASSERT_SUCCESS(RunKernelSequence(kMainKernelPhase, " << ops_name << ", " << seq_name
          << ", ArraySize(" << seq_name << ")));\n";
    return true;
  };
  size_t run_begin = 0U;
  bool in_run = false;
  for (size_t step_index = 0U; step_index < region.steps.size(); ++step_index) {
    const auto &step = region.steps[step_index];
    if (step.kind == ProgramStepKind::kKernel) {
      if (step.kernel_index >= program_model.main.size()) {
        GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] region %u step %zu has invalid kernel index %u (kernel count %zu)",
               region_index, step_index, step.kernel_index, program_model.main.size());
        return false;
      }
      if (!in_run) {
        run_begin = step_index;
        in_run = true;
      }
      continue;
    }
    if (in_run) {
      if (!flush_kernel_run(region_index, run_begin, step_index)) {
        return false;
      }
      in_run = false;
    }
    if (step.kind != ProgramStepKind::kIf) {
      return false;
    }
    if (!EmitIfRunCalls(program_model, step, indent, active_regions, calls)) {
      return false;
    }
  }
  if (in_run) {
    if (!flush_kernel_run(region_index, run_begin, region.steps.size())) {
      return false;
    }
  }
  active_regions.erase(region_index);
  return true;
}

bool EmitSequentialMainCalls(const DynamicProgramModel &program_model, std::string &output) {
  std::ostringstream calls;
  if (program_model.main_root_region_index >= program_model.main_regions.size()) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] invalid main root region index %u (region count %zu)",
           program_model.main_root_region_index, program_model.main_regions.size());
    return false;
  }
  const auto &root_region = program_model.main_regions[program_model.main_root_region_index];
  if (root_region.steps.empty()) {
    output = calls.str();
    return true;
  }
  calls << "  static constexpr uint8_t kMainSequenceOps[] = {";
  bool is_first_op = true;
  for (size_t step_index = 0U; step_index < root_region.steps.size(); ++step_index) {
    const auto &step = root_region.steps[step_index];
    if ((step.kind != ProgramStepKind::kKernel) || (step.kernel_index >= program_model.main.size())) {
      GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] sequential root contains unsupported step kind %u kernel %u",
             static_cast<uint32_t>(step.kind), step.kernel_index);
      return false;
    }
    const FusedGroupInfo *group = nullptr;
    if (FusedStepRole(program_model, program_model.main_root_region_index, step_index, &group) == 0) {
      continue;
    }
    if (!is_first_op) {
      calls << ", ";
    }
    if (group != nullptr) {
      calls << "1U";
    } else {
      calls << ((program_model.main[step.kernel_index].impl_type == KernelImplType::kCodegenImpl)
                    ? "1U"
                    : ((program_model.main[step.kernel_index].impl_type == KernelImplType::kEmptyImpl) ? "2U" : "0U"));
    }
    is_first_op = false;
  }
  calls << "};\n"
        << "  static constexpr uint32_t kMainSequence[] = {";
  bool is_first = true;
  for (size_t step_index = 0U; step_index < root_region.steps.size(); ++step_index) {
    const FusedGroupInfo *group = nullptr;
    const int role = FusedStepRole(program_model, program_model.main_root_region_index, step_index, &group);
    if (role == 0) {
      continue;
    }
    if (!is_first) {
      calls << ", ";
    }
    calls << ((role == 1) ? group->fused_dispatch_index : root_region.steps[step_index].kernel_index) << "U";
    is_first = false;
  }
  calls << "};\n";
  calls << "    OM2_ASSERT_SUCCESS(RunKernelSequence(kMainKernelPhase, kMainSequenceOps, kMainSequence,"
           " ArraySize(kMainSequence)));\n";
  output = calls.str();
  return true;
}

bool EmitStructuredMainCalls(const DynamicProgramModel &program_model, std::string &output) {
  std::ostringstream calls;
  std::unordered_set<uint32_t> active_regions;
  if (!EmitRegionRunCalls(program_model, program_model.main_root_region_index, 4U, active_regions, calls)) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] failed structured main root region %u",
           program_model.main_root_region_index);
    return false;
  }
  output = calls.str();
  return true;
}

bool EmitMainProgramCalls(const DynamicProgramModel &program_model, std::string &output) {
  switch (program_model.main_plan_type) {
    case ProgramPlanType::kSequential:
      return EmitSequentialMainCalls(program_model, output);
    case ProgramPlanType::kStructuredTopological:
    case ProgramPlanType::kStructuredTopologicalPriority:
      return EmitStructuredMainCalls(program_model, output);
  }
  return false;
}

enum class MtRowOp : uint32_t { kWait = 0U, kRunKernel = 1U, kRunIsland = 2U, kRunKernelImpl = 3U };

struct GeneratedMtRow {
  MtRowOp op = MtRowOp::kWait;
  uint32_t a = 0U;
  uint32_t b = 0U;
  uint32_t c = 0U;
};

std::string EmitKernelImplSupport(const DynamicProgramModel &program_model) {
  struct PhaseKernels {
    const char *phase_expr;
    char letter;
    const std::vector<DynamicKernelCodegenInfo> *kernels;
  };
  const PhaseKernels phases[] = {
      {"kInitKernelPhase", 'I', &program_model.init},
      {"kMainKernelPhase", 'M', &program_model.main},
      {"kDeInitKernelPhase", 'D', &program_model.deinit},
  };
  std::vector<std::vector<size_t>> impl_indexes;
  std::ostringstream trampolines;
  for (const auto &phase : phases) {
    std::vector<size_t> indexes;
    for (size_t index = 0U; index < phase.kernels->size(); ++index) {
      const auto &kernel = (*phase.kernels)[index];
      if (kernel.impl_type != KernelImplType::kCodegenImpl) {
        continue;
      }
      if ((phase.kernels == &program_model.main) &&
          (program_model.main_fused_member_kernels.count(static_cast<uint32_t>(index)) != 0U)) {
        continue;  // fused member: emitted once inside its fused function only
      }
      indexes.push_back(index);
      trampolines << "  aclError RunKernelImpl" << phase.letter << index << "() {\n"
                  << "    OM2_ASSERT_SUCCESS(" << kernel.cpp_symbol << "());\n"
                  << "    OM2_LOGI(\"[KernelTrace][" << EscapeForStringLiteral(kernel.execution_node_name) << "]\");\n"
                  << "    return kOk;\n"
                  << "  }\n\n";
    }
    impl_indexes.push_back(std::move(indexes));
  }
  bool has_any = !program_model.main_fused_groups.empty();
  for (const auto &indexes : impl_indexes) {
    has_any = has_any || !indexes.empty();
  }
  if (!has_any) {
    // RunKernelSequence and the multithread schedule interpreter
    // unconditionally reference the dispatch symbol, so a fallback-only model
    // still needs a definition. No ops/mT rows select it in this case; reaching
    // the stub is a generation-time invariant violation.
    return "  aclError RunKernelImplDispatch(const uint32_t, const uint32_t) "
           "{\n"
           "    return kFailed;\n"
           "  }\n\n";
  }
  std::ostringstream out;
  out << trampolines.str();
  for (size_t fused_index = 0U; fused_index < program_model.main_fused_groups.size(); ++fused_index) {
    const auto &group = program_model.main_fused_groups[fused_index];
    out << "  aclError " << group.function_symbol << "() {\n"
        << group.function_body << "    return kOk;\n"
        << "  }\n\n"
        << "  aclError RunKernelImplF" << fused_index << "() {\n"
        << "    OM2_ASSERT_SUCCESS(" << group.function_symbol << "());\n"
        << "    return kOk;\n"
        << "  }\n\n";
  }
  out << "  aclError RunKernelImplDispatch(const uint32_t phase, const uint32_t kernel_index) {\n";
  for (size_t phase_index = 0U; phase_index < 3U; ++phase_index) {
    const bool phase_has_fused = (phase_index == 1U) && !program_model.main_fused_groups.empty();
    if (impl_indexes[phase_index].empty() && !phase_has_fused) {
      continue;
    }
    out << "    if (phase == " << phases[phase_index].phase_expr << ") {\n"
        << "      switch (kernel_index) {\n";
    for (const size_t index : impl_indexes[phase_index]) {
      out << "        case " << index << "U:\n"
          << "          return RunKernelImpl" << phases[phase_index].letter << index << "();\n";
    }
    if (phase_has_fused) {
      for (size_t fused_index = 0U; fused_index < program_model.main_fused_groups.size(); ++fused_index) {
        out << "        case " << program_model.main_fused_groups[fused_index].fused_dispatch_index << "U:\n"
            << "          return RunKernelImplF" << fused_index << "();\n";
      }
    }
    out << "        default:\n"
        << "          break;\n"
        << "      }\n"
        << "    }\n";
  }
  out << "    return kFailed;\n"
      << "  }\n\n";
  return out.str();
}

// Strips the wrapper declaration and closing brace off a builder-produced function
// ("  aclError Symbol() {\n" ... "  }\n") and returns the inner statements.  Returns an empty
// string when the layout is unexpected; the caller then drops the candidate group.
std::string StripGeneratedFunctionBody(const std::string &function) {
  const size_t open = function.find("{\n");
  const size_t close = function.rfind("  }\n");
  if ((open == std::string::npos) || (close == std::string::npos) || (close <= open)) {
    return {};
  }
  return function.substr(open + 2U, close - (open + 2U));
}

void ReplaceChainRef(std::string &body, const uint32_t chain_index, const std::string &local_name) {
  const std::string target = "chains_[" + std::to_string(chain_index) + "U]";
  size_t pos = 0U;
  while ((pos = body.find(target, pos)) != std::string::npos) {
    body.replace(pos, target.size(), local_name);
    pos += local_name.size();
  }
}

// A member body may only contain `return kOk;` / `return kFailed;` returns.  Inside the fused
// function each member is wrapped in do{...}while(0): success returns become `break;` so the
// next member still runs, failure returns leave the fused function immediately.  Any other
// return form rejects the group.
bool RewriteFusedMemberBody(std::string &inner) {
  size_t pos = 0U;
  while ((pos = inner.find("return", pos)) != std::string::npos) {
    size_t after = pos + 6U;
    while ((after < inner.size()) && std::isspace(static_cast<unsigned char>(inner[after]))) {
      ++after;
    }
    if (inner.compare(after, 3U, "kOk") == 0) {
      inner.replace(pos, (after + 3U) - pos, "break");
      pos += 5U;
      continue;
    }
    if (inner.compare(after, 7U, "kFailed") == 0) {
      pos = after + 7U;
      continue;
    }
    return false;
  }
  return true;
}

// Builds the fused function body for one group: one stack-local gert::Chain per localized
// chain, then the stripped bodies of the member kernels (each in its own do/while scope so
// local names never collide and per-member success returns fall through).  Returns false
// when a member body cannot be stripped or rewritten safely.
bool BuildFusedFunctionBody(const DynamicProgramModel &model, const std::vector<uint32_t> &kernel_indexes,
                            const std::set<uint32_t> &localized_chains, std::string &body) {
  std::ostringstream ss;
  for (const auto chain_index : localized_chains) {
    ss << "    gert::Chain fused_chain_" << chain_index << "{};\n";
  }
  for (const auto kernel_index : kernel_indexes) {
    const auto &kernel = model.main[kernel_index];
    std::string inner = StripGeneratedFunctionBody(kernel.generated_function);
    if (inner.empty() || !RewriteFusedMemberBody(inner)) {
      return false;
    }
    for (const auto chain_index : localized_chains) {
      ReplaceChainRef(inner, chain_index, "fused_chain_" + std::to_string(chain_index));
    }
    ss << "    do {\n" << inner << "    } while (false);\n";
  }
  body = ss.str();
  return true;
}

// Detects consecutive own-kernel runs whose intermediate chains have exactly one producer and
// one consumer across the whole model (no fallback/empty readers, no If selector use, no
// model output, chain born as kKernelOutput).  Only the Main block is fused; MultiThread
// plans keep per-kernel units, so fused analysis is skipped when MT is enabled.
std::vector<FusedGroupInfo> AnalyzeFusableGroups(const DynamicProgramModel &model) {
  std::vector<FusedGroupInfo> groups;
  if (model.main_regions.empty()) {
    return groups;
  }
  const auto count_port_uses = [](const std::vector<DynamicKernelCodegenInfo> &kernels,
                                  const std::vector<KernelPortInfo> DynamicKernelCodegenInfo::*ports,
                                  std::unordered_map<uint32_t, uint32_t> &uses) {
    for (const auto &kernel : kernels) {
      for (const auto &port : kernel.*ports) {
        ++uses[port.chain_index];
      }
    }
  };
  std::unordered_map<uint32_t, uint32_t> producer_uses;
  std::unordered_map<uint32_t, uint32_t> consumer_uses;
  count_port_uses(model.init, &DynamicKernelCodegenInfo::outputs, producer_uses);
  count_port_uses(model.main, &DynamicKernelCodegenInfo::outputs, producer_uses);
  count_port_uses(model.deinit, &DynamicKernelCodegenInfo::outputs, producer_uses);
  count_port_uses(model.init, &DynamicKernelCodegenInfo::inputs, consumer_uses);
  count_port_uses(model.main, &DynamicKernelCodegenInfo::inputs, consumer_uses);
  count_port_uses(model.deinit, &DynamicKernelCodegenInfo::inputs, consumer_uses);

  std::set<uint32_t> excluded_chains;
  for (const auto &chain : model.chains) {
    if (chain.source_kind != ChainSourceKind::kKernelOutput) {
      excluded_chains.insert(chain.index);  // covers model I/O, constants, runtime bindings
    }
  }
  // Chains pre-created by a kCodegenImpl outputs creator hold their object on the global
  // chains_[] array (created once at load); localizing them would strand the object behind a
  // stack-local Chain that nobody initialized, so fusion keeps them on the global array.
  const auto exclude_codegen_created_outputs =
      [&excluded_chains](const std::vector<DynamicKernelCodegenInfo> &kernels) {
        for (const auto &kernel : kernels) {
          if (kernel.output_policy != OutputCreatePolicy::kCodegenImpl) {
            continue;
          }
          for (const auto &output : kernel.outputs) {
            excluded_chains.insert(output.chain_index);
          }
        }
      };
  exclude_codegen_created_outputs(model.init);
  exclude_codegen_created_outputs(model.main);
  exclude_codegen_created_outputs(model.deinit);
  for (const auto &region : model.main_regions) {
    for (const auto &step : region.steps) {
      if (step.kind == ProgramStepKind::kIf) {
        excluded_chains.insert(step.if_info.selector_chain_index);
      }
    }
  }
  const auto is_localizable = [&producer_uses, &consumer_uses, &excluded_chains](const uint32_t chain) {
    return (excluded_chains.count(chain) == 0U) && (producer_uses.count(chain) != 0U) &&
           (producer_uses.at(chain) == 1U) && (consumer_uses.count(chain) != 0U) && (consumer_uses.at(chain) == 1U);
  };

  // Under MultiThread the root region's flat kernel runs are consumed by the worker schedule
  // table, so region-based fusion there would race with MT-based fusion; only If sub-regions
  // (executed sequentially inside islands) keep the region-based form.
  std::set<uint32_t> if_sub_regions;
  for (const auto &region : model.main_regions) {
    for (const auto &step : region.steps) {
      if (step.kind != ProgramStepKind::kIf) {
        continue;
      }
      const auto condition_region = (step.condition_region_index != kInvalidProgramIndex)
                                        ? step.condition_region_index
                                        : step.if_info.condition_region_index;
      if (condition_region != kInvalidProgramIndex) {
        if_sub_regions.insert(condition_region);
      }
      for (const auto &branch : step.if_info.branches) {
        if_sub_regions.insert(branch.region_index);
      }
    }
  }
  for (uint32_t region_index = 0U; region_index < model.main_regions.size(); ++region_index) {
    if (model.main_multithread_enabled && (if_sub_regions.count(region_index) == 0U)) {
      continue;
    }
    const auto &region = model.main_regions[region_index];
    size_t run_begin = 0U;
    auto flush_run = [&model, region_index, &groups, &is_localizable](const size_t begin, const size_t end) {
      if (end - begin < 2U) {
        return;
      }
      const auto &region = model.main_regions[region_index];
      // Greedily extend the group while the intermediate chain between consecutive kernels is
      // localizable; a kernel pair joins a group only through its direct successor edge.
      size_t group_begin = begin;
      while (group_begin < end) {
        size_t group_end = group_begin + 1U;
        std::set<uint32_t> localized;
        while (group_end < end) {
          const auto &producer = model.main[region.steps[group_end - 1U].kernel_index];
          const auto &consumer = model.main[region.steps[group_end].kernel_index];
          bool linked = false;
          for (const auto &output : producer.outputs) {
            if (!is_localizable(output.chain_index)) {
              continue;
            }
            for (const auto &input : consumer.inputs) {
              if (input.chain_index == output.chain_index) {
                localized.insert(output.chain_index);
                linked = true;
              }
            }
          }
          if (!linked) {
            break;
          }
          ++group_end;
        }
        if (group_end - group_begin >= 2U) {
          FusedGroupInfo group;
          group.region_index = region_index;
          group.step_begin = group_begin;
          group.step_end = group_end;
          group.localized_chains = std::move(localized);
          groups.push_back(std::move(group));
        }
        group_begin = group_end;
      }
    };
    size_t step = 0U;
    while (step < region.steps.size()) {
      if (region.steps[step].kind != ProgramStepKind::kKernel) {
        flush_run(run_begin, step);
        run_begin = step + 1U;
      }
      ++step;
    }
    flush_run(run_begin, region.steps.size());
  }

  if (model.main_multithread_enabled) {
    // MT fusion works on the scheduled worker units: consecutive own-kernel units on one lane
    // can fuse only when no intermediate progress is observed by another lane (a fused unit
    // publishes only its last member's progress, so any wait on an earlier member would hang).
    std::set<std::pair<size_t, uint32_t>> waited_progress;
    for (const auto &worker : model.main_multithread_plan.workers) {
      for (const auto &unit : worker.units) {
        for (const auto &wait : unit.waits) {
          waited_progress.insert({wait.worker_index, wait.progress});
        }
      }
    }
    for (size_t worker_index = 0U; worker_index < model.main_multithread_plan.workers.size(); ++worker_index) {
      const auto &units = model.main_multithread_plan.workers[worker_index].units;
      size_t group_begin = 0U;
      while (group_begin < units.size()) {
        size_t group_end = group_begin + 1U;
        std::set<uint32_t> localized;
        while (group_end < units.size()) {
          const auto &producer_unit = units[group_end - 1U];
          const auto &consumer_unit = units[group_end];
          bool linked = false;
          if ((producer_unit.kind == ExecutionUnitKind::kKernel) &&
              (consumer_unit.kind == ExecutionUnitKind::kKernel)) {
            const auto &producer = model.main[producer_unit.kernel_index];
            const auto &consumer = model.main[consumer_unit.kernel_index];
            if ((producer.impl_type == KernelImplType::kCodegenImpl) &&
                (consumer.impl_type == KernelImplType::kCodegenImpl)) {
              for (const auto &output : producer.outputs) {
                if (!is_localizable(output.chain_index)) {
                  continue;
                }
                for (const auto &input : consumer.inputs) {
                  if (input.chain_index == output.chain_index) {
                    localized.insert(output.chain_index);
                    linked = true;
                  }
                }
              }
            }
          }
          if (!linked || (waited_progress.count({worker_index, static_cast<uint32_t>(group_end)}) != 0U)) {
            break;  // current tail becomes a non-tail member; its progress must stay observable
          }
          ++group_end;
        }
        if (group_end - group_begin >= 2U) {
          FusedGroupInfo group;
          group.from_multithread = true;
          group.worker_index = worker_index;
          group.unit_begin = group_begin;
          group.unit_end = group_end;
          group.localized_chains = std::move(localized);
          groups.push_back(std::move(group));
        }
        group_begin = group_end;
      }
    }
  }

  std::vector<FusedGroupInfo> valid_groups;
  for (auto &group : groups) {
    std::vector<uint32_t> kernel_indexes;
    if (group.from_multithread) {
      const auto &units = model.main_multithread_plan.workers[group.worker_index].units;
      for (size_t unit = group.unit_begin; unit < group.unit_end; ++unit) {
        kernel_indexes.push_back(units[unit].kernel_index);
      }
      group.function_symbol =
          "main_fused_w" + std::to_string(group.worker_index) + "_u" + std::to_string(group.unit_begin);
    } else {
      const auto &region = model.main_regions[group.region_index];
      for (size_t step = group.step_begin; step < group.step_end; ++step) {
        kernel_indexes.push_back(region.steps[step].kernel_index);
      }
      group.function_symbol =
          "main_fused_r" + std::to_string(group.region_index) + "_s" + std::to_string(group.step_begin);
    }
    if (!BuildFusedFunctionBody(model, kernel_indexes, group.localized_chains, group.function_body)) {
      GELOGW("[OM2][ProgramBody] fused group has an unstrippable body; dropped");
      continue;
    }
    valid_groups.push_back(std::move(group));
  }
  for (size_t index = 0U; index < valid_groups.size(); ++index) {
    valid_groups[index].fused_dispatch_index = kFusedDispatchBase + static_cast<uint32_t>(index);
  }
  return valid_groups;
}

void CollectFusedMemberKernels(const std::vector<FusedGroupInfo> &groups, const DynamicProgramModel &model,
                               std::set<uint32_t> &members) {
  for (const auto &group : groups) {
    if (group.from_multithread) {
      const auto &units = model.main_multithread_plan.workers[group.worker_index].units;
      for (size_t unit = group.unit_begin; unit < group.unit_end; ++unit) {
        members.insert(units[unit].kernel_index);
      }
      continue;
    }
    const auto &region = model.main_regions[group.region_index];
    for (size_t step = group.step_begin; step < group.step_end; ++step) {
      members.insert(region.steps[step].kernel_index);
    }
  }
}

std::string EmitMultiThreadProgramSupport(const DynamicProgramModel &program_model) {
  if (!program_model.main_multithread_enabled) {
    return {};
  }
  std::ostringstream methods;
  methods << "  void SetWorkerError(const aclError error) {\n"
          << "    aclError expected = kOk;\n"
          << "    (void)worker_error_.compare_exchange_strong(expected, error, std::memory_order_acq_rel);\n"
          << "  }\n\n"
          << "  // Shared wait helper: spins until lane reaches progress. unit_id high bit marks islands.\n"
          << "  // Defined once so per-wait call sites stay one line; keeps the generated so small.\n"
          << "  aclError WaitLaneProgress(const size_t lane, const uint32_t progress, const uint32_t unit_id) {\n"
          << "    uint32_t spin = 0U;\n"
          << "    while (worker_progress_[lane].load(std::memory_order_acquire) < progress) {\n"
          << "      const aclError wait_error = worker_error_.load(std::memory_order_acquire);\n"
          << "      if (wait_error != kOk) {\n"
          << "        OM2_LOGI(\"[KernelTrace][WaitAbort][unit=%u][island=%s][waiting_lane=%zu]\"\n"
          << "                 \"[need_progress=%u][got_progress=%u][error=%d]\",\n"
          << "                 unit_id & ~(1U << 31), (unit_id & (1U << 31)) != 0U ? \"yes\" : \"no\",\n"
          << "                 lane, progress, worker_progress_[lane].load(std::memory_order_acquire),\n"
          << "                 static_cast<int32_t>(wait_error));\n"
          << "        return wait_error;\n"
          << "      }\n"
          << "      if ((++spin & ((1U << 26) - 1U)) == 0U) {\n"
          << "        OM2_LOGI(\"[KernelTrace][WaitStuck][unit=%u][island=%s][waiting_lane=%zu]\"\n"
          << "                 \"[need_progress=%u][got_progress=%u]\",\n"
          << "                 unit_id & ~(1U << 31), (unit_id & (1U << 31)) != 0U ? \"yes\" : \"no\",\n"
          << "                 lane, progress, worker_progress_[lane].load(std::memory_order_acquire));\n"
          << "      }\n"
          << "      std::this_thread::yield();\n"
          << "    }\n"
          << "    return kOk;\n"
          << "  }\n\n";
  for (const auto &worker : program_model.main_multithread_plan.workers) {
    for (const auto &unit : worker.units) {
      if (unit.kind != ExecutionUnitKind::kStructuredIsland) {
        continue;
      }
      if (unit.root_step_index >= program_model.main_regions[program_model.main_root_region_index].steps.size()) {
        return {};
      }
      const auto &step = program_model.main_regions[program_model.main_root_region_index].steps[unit.root_step_index];
      if (step.kind != ProgramStepKind::kIf) {
        return {};
      }
      methods << "  aclError RunMainStructuredIsland" << unit.root_step_index << "() {\n";
      std::unordered_set<uint32_t> active_regions;
      if (!EmitIfRunCalls(program_model, step, 4U, active_regions, methods)) {
        return {};
      }
      methods << "    return kOk;\n"
              << "  }\n\n";
    }
  }
  std::vector<GeneratedMtRow> rows;
  std::vector<size_t> worker_offsets;
  std::unordered_set<uint32_t> island_cases;
  std::unordered_set<uint32_t> kernel_impl_cases;
  const auto mt_fused_role = [&program_model](const size_t worker_index, const size_t unit_index,
                                              const FusedGroupInfo **out_group) {
    for (const auto &group : program_model.main_fused_groups) {
      if (!group.from_multithread || (group.worker_index != worker_index) || (unit_index < group.unit_begin) ||
          (unit_index >= group.unit_end)) {
        continue;
      }
      const bool is_start = (unit_index == group.unit_begin);
      if (is_start && (out_group != nullptr)) {
        *out_group = &group;
      }
      return is_start ? 1 : 0;
    }
    return -1;
  };
  for (size_t worker_index = 0U; worker_index < program_model.main_multithread_plan.workers.size(); ++worker_index) {
    const auto &worker = program_model.main_multithread_plan.workers[worker_index];
    worker_offsets.push_back(rows.size());
    for (size_t local_index = 0U; local_index < worker.units.size(); ++local_index) {
      const auto &unit = worker.units[local_index];
      const auto progress = static_cast<uint32_t>(local_index + 1U);
      const uint32_t unit_id =
          (unit.kind == ExecutionUnitKind::kKernel) ? unit.kernel_index : ((1U << 31) | unit.root_step_index);
      const FusedGroupInfo *fused_group = nullptr;
      const int fused_role = mt_fused_role(worker_index, local_index, &fused_group);
      if (fused_role == 0) {
        continue;  // fused member: covered by the fused row emitted at the group start
      }
      if (fused_group != nullptr) {
        // The fused unit is atomic: every member's cross-lane waits must hold before it starts
        // (merged per lane to the max progress), and it publishes only the last member's
        // original progress so other lanes keep working.
        std::map<size_t, uint32_t> merged_waits;
        for (size_t member = fused_group->unit_begin; member < fused_group->unit_end; ++member) {
          for (const auto &wait : worker.units[member].waits) {
            auto &required = merged_waits[wait.worker_index];
            required = std::max(required, wait.progress);
          }
        }
        for (const auto &wait : merged_waits) {
          rows.push_back(GeneratedMtRow{MtRowOp::kWait, static_cast<uint32_t>(wait.first), wait.second, unit_id});
        }
        rows.push_back(GeneratedMtRow{MtRowOp::kRunKernelImpl, fused_group->fused_dispatch_index, 0U,
                                      static_cast<uint32_t>(fused_group->unit_end)});
        continue;
      }
      for (const auto &wait : unit.waits) {
        rows.push_back(
            GeneratedMtRow{MtRowOp::kWait, static_cast<uint32_t>(wait.worker_index), wait.progress, unit_id});
      }
      if (unit.kind == ExecutionUnitKind::kKernel) {
        if (unit.kernel_index >= program_model.main.size()) {
          return {};
        }
        const auto &kernel = program_model.main[unit.kernel_index];
        if (kernel.impl_type == KernelImplType::kRunFuncImpl) {
          rows.push_back(GeneratedMtRow{MtRowOp::kRunKernel, unit.kernel_index, 0U, progress});
        } else if (kernel.impl_type == KernelImplType::kCodegenImpl) {
          kernel_impl_cases.insert(unit.kernel_index);
          rows.push_back(GeneratedMtRow{MtRowOp::kRunKernelImpl, unit.kernel_index, 0U, progress});
        } else {
          return {};
        }
      } else {
        if (unit.root_step_index >= program_model.main_regions[program_model.main_root_region_index].steps.size()) {
          return {};
        }
        island_cases.insert(unit.root_step_index);
        rows.push_back(GeneratedMtRow{MtRowOp::kRunIsland, unit.root_step_index, 0U, progress});
      }
    }
  }
  worker_offsets.push_back(rows.size());

  methods << "  enum class MtOp : uint32_t { kWait = 0U, kRunKernel = 1U, kRunIsland = 2U, kRunKernelImpl = 3U };\n"
          << "  struct MtRow {\n"
          << "    MtOp op;\n"
          << "    uint32_t a;\n"
          << "    uint32_t b;\n"
          << "    uint32_t c;\n"
          << "  };\n"
          << "  static constexpr MtRow kWmtRows[] = {\n";
  for (const auto &row : rows) {
    const char *const op_name =
        (row.op == MtRowOp::kWait)
            ? "kWait"
            : ((row.op == MtRowOp::kRunKernel) ? "kRunKernel"
                                               : ((row.op == MtRowOp::kRunIsland) ? "kRunIsland" : "kRunKernelImpl"));
    methods << "    MtRow{MtOp::" << op_name << ", " << row.a << "U, " << row.b << "U, " << row.c << "U},\n";
  }
  methods << "  };\n"
          << "  static constexpr size_t kWmtWorkerCount = " << program_model.main_multithread_plan.workers.size()
          << "U;\n"
          << "  static constexpr size_t kWmtWorkerOffsets[] = {";
  for (size_t worker_index = 0U; worker_index < worker_offsets.size(); ++worker_index) {
    if (worker_index > 0U) {
      methods << ", ";
    }
    methods << worker_offsets[worker_index] << "U";
  }
  methods << "};\n";
  methods
      << "  aclError RunMainWorkerSteps(const size_t worker_lane, const MtRow *const begin, const MtRow *const end) {\n"
      << "    for (const MtRow *step = begin; step != end; ++step) {\n"
      << "      if (step->op == MtOp::kWait) {\n"
      << "        const aclError wait_ret = WaitLaneProgress(step->a, step->b, step->c);\n"
      << "        if (wait_ret != kOk) {\n"
      << "          return wait_ret;\n"
      << "        }\n"
      << "        continue;\n"
      << "      }\n"
      << "      const aclError prior_error = worker_error_.load(std::memory_order_acquire);\n"
      << "      if (prior_error != kOk) {\n"
      << "        return prior_error;\n"
      << "      }\n"
      << "      aclError kernel_ret = kOk;\n"
      << "      if (step->op == MtOp::kRunKernel) {\n"
      << "        kernel_ret = RunKernel(kMainKernelPhase, step->a);\n"
      << "      } else if (step->op == MtOp::kRunIsland) {\n"
      << "        switch (step->a) {\n";
  for (const uint32_t root_step_index : island_cases) {
    methods << "          case " << root_step_index << "U:\n"
            << "            kernel_ret = RunMainStructuredIsland" << root_step_index << "();\n"
            << "            break;\n";
  }
  methods << "          default:\n"
          << "            kernel_ret = kFailed;\n"
          << "            break;\n"
          << "        }\n";
  const bool mt_has_kernel_impl =
      !kernel_impl_cases.empty() ||
      std::any_of(program_model.main_fused_groups.cbegin(), program_model.main_fused_groups.cend(),
                  [](const FusedGroupInfo &group) { return group.from_multithread; });
  if (mt_has_kernel_impl) {
    methods << "      } else if (step->op == MtOp::kRunKernelImpl) {\n"
            << "        kernel_ret = RunKernelImplDispatch(kMainKernelPhase, step->a);\n";
  }
  methods << "      } else {\n"
          << "        kernel_ret = kFailed;\n"
          << "      }\n"
          << "      if (kernel_ret != kOk) {\n"
          << "        SetWorkerError(kernel_ret);\n"
          << "        return kernel_ret;\n"
          << "      }\n"
          << "      worker_progress_[worker_lane].store(step->c, std::memory_order_release);\n"
          << "    }\n"
          << "    return kOk;\n"
          << "  }\n\n"
          << "  aclError RunMainWorker(const size_t worker_index) {\n"
          << "    if (worker_index >= kWmtWorkerCount) {\n"
          << "      return kFailed;\n"
          << "    }\n"
          << "    return RunMainWorkerSteps(worker_index, kWmtRows + kWmtWorkerOffsets[worker_index],\n"
          << "                              kWmtRows + kWmtWorkerOffsets[worker_index + 1U]);\n"
          << "  }\n\n"
          << "  static uint32_t RunMainWorkerEntry(void *const opaque, const size_t worker_index) {\n"
          << "    if (opaque == nullptr) {\n"
          << "      return static_cast<uint32_t>(kFailed);\n"
          << "    }\n"
          << "    return static_cast<uint32_t>(\n"
          << "        static_cast<Om2DynamicProgramBody *>(opaque)->RunMainWorker(worker_index));\n"
          << "  }\n\n"
          << "  aclError InitMainWorkerPool() {\n"
          << "    if (worker_pool_ != nullptr) {\n"
          << "      return kFailed;\n"
          << "    }\n"
          << "    const uint32_t worker_types[] = {";
  for (size_t worker_index = 0U; worker_index < program_model.main_multithread_plan.workers.size(); ++worker_index) {
    if (worker_index > 0U) {
      methods << ", ";
    }
    methods << static_cast<uint32_t>(program_model.main_multithread_plan.workers[worker_index].type) << "U";
  }
  methods << "};\n"
          << "    worker_pool_ = GeOm2CreateStaticWorkerPool(ArraySize(worker_types), worker_types);\n"
          << "    if (worker_pool_ == nullptr) {\n"
          << "      return kFailed;\n"
          << "    }\n"
          << "    OM2_LOGI(\"[OM2][ProgramBody] Main execution mode=static_multithread, worker_count="
          << program_model.main_multithread_plan.workers.size() << "\");\n"
          << "    return kOk;\n"
          << "  }\n\n"
          << "  void DestroyMainWorkerPool() {\n"
          << "    GeOm2DestroyStaticWorkerPool(worker_pool_);\n"
          << "    worker_pool_ = nullptr;\n"
          << "  }\n\n"
          << "  aclError RunMainWorkers() {\n"
          << "    for (auto &progress : worker_progress_) {\n"
          << "      progress.store(0U, std::memory_order_relaxed);\n"
          << "    }\n"
          << "    worker_error_.store(kOk, std::memory_order_release);\n"
          << "    const aclError pool_ret = static_cast<aclError>(\n"
          << "        GeOm2RunStaticWorkerPool(worker_pool_, run_stream_, &RunMainWorkerEntry, this));\n"
          << "    const aclError worker_ret = worker_error_.load(std::memory_order_acquire);\n"
          << "    const aclError final_ret = worker_ret == kOk ? pool_ret : worker_ret;\n"
          << "    if (final_ret != kOk) {\n"
          << "      OM2_LOGI(\"[KernelTrace][RunMainWorkersFail][ret=%d][worker_error=%d]\",\n"
          << "               static_cast<int32_t>(final_ret), static_cast<int32_t>(worker_ret));\n"
          << "      for (size_t lane = 0U; lane < worker_progress_.size(); ++lane) {\n"
          << "        OM2_LOGI(\"[KernelTrace][LaneState][lane=%zu][progress=%u]\",\n"
          << "                 lane, worker_progress_[lane].load(std::memory_order_acquire));\n"
          << "      }\n"
          << "    }\n"
          << "    return final_ret;\n"
          << "  }\n\n";
  return methods.str();
}

std::string BuildExecutableSource(const DynamicProgramModel &program_model, const ProgramBodyCodegenOption &option) {
  const auto stream_chains = CollectBindingChains(program_model, ChainBindingKind::kStream);
  const auto external_allocator_chains = CollectBindingChains(program_model, ChainBindingKind::kExternalAllocator);
  const auto model_desc_chains = CollectBindingChains(program_model, ChainBindingKind::kModelDesc);
  const auto space_registry_chains = CollectBindingChains(program_model, ChainBindingKind::kSpaceRegistry);
  const auto custom_op_registry_chains = CollectBindingChains(program_model, ChainBindingKind::kCustomOpRegistry);
  const auto outer_weight_mem_chains = CollectBindingChains(program_model, ChainBindingKind::kOuterWeightMem);
  const auto rt_session_chains = CollectBindingChains(program_model, ChainBindingKind::kRtSession);
  std::vector<HostTensorBindingCodegenInfo> host_tensor_bindings;
  if (CollectHostTensorBindings(program_model, host_tensor_bindings) != ge::GRAPH_SUCCESS) {
    return {};
  }
  const auto notify_chains = CollectBindingChains(program_model, ChainBindingKind::kNotifies, ChainBindingPhase::kRun);
  const auto binary_bindings = CollectBinaryBindings(program_model);
  const bool has_ge_kernel_bins = std::any_of(binary_bindings.cbegin(), binary_bindings.cend(),
                                              [](const auto &binding) { return binding.is_ge_kernel_bin; });
  const auto model_input_chains = CollectModelIoChains(program_model, ChainSourceKind::kModelInput);
  const auto model_output_chains = CollectModelIoChains(program_model, ChainSourceKind::kModelOutput);
  if (model_output_chains.size() != program_model.structured_resources.model_desc.outputs.size()) {
    std::ostringstream output_chain_debug;
    for (const auto chain_index : model_output_chains) {
      const auto iter = std::find_if(program_model.chains.cbegin(), program_model.chains.cend(),
                                     [chain_index](const auto &chain) { return chain.index == chain_index; });
      if (iter != program_model.chains.cend()) {
        output_chain_debug << " [" << iter->source_index << ":" << iter->index << "]";
      }
    }
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] model output chains:%s", output_chain_debug.str().c_str());
  }
  if ((model_input_chains.size() != program_model.structured_resources.model_desc.inputs.size()) ||
      (model_output_chains.size() != program_model.structured_resources.model_desc.outputs.size())) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] model I/O chain count mismatch: inputs %zu/%zu outputs %zu/%zu",
           model_input_chains.size(), program_model.structured_resources.model_desc.inputs.size(),
           model_output_chains.size(), program_model.structured_resources.model_desc.outputs.size());
    return {};
  }
  const auto has_expected_source_indices = [&program_model](const std::vector<uint32_t> &chain_indices) {
    for (size_t index = 0U; index < chain_indices.size(); ++index) {
      const auto iter =
          std::find_if(program_model.chains.cbegin(), program_model.chains.cend(),
                       [chain_index = chain_indices[index]](const auto &chain) { return chain.index == chain_index; });
      if ((iter == program_model.chains.cend()) || (iter->source_index != static_cast<int64_t>(index))) {
        return false;
      }
    }
    return true;
  };
  if (!has_expected_source_indices(model_input_chains) || !has_expected_source_indices(model_output_chains)) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] model I/O chain source indices are not contiguous");
    return {};
  }
  const std::string const_chain_ref = EmitTypedConstChainRef(program_model);
  if (!program_model.const_chains.empty() && const_chain_ref.empty()) {
    GELOGE(ge::GRAPH_FAILED, "[OM2][ProgramBody] failed to emit %zu Const chain references",
           program_model.const_chains.size());
    return {};
  }

  std::ostringstream ss;
  const auto emit_run_kernel_calls = [](const std::vector<DynamicKernelCodegenInfo> &kernels,
                                        const char *const sequence_name, const char *const phase) {
    std::ostringstream calls;
    if (kernels.empty()) {
      return calls.str();
    }
    calls << "  static constexpr uint8_t " << sequence_name << "Ops[] = {";
    for (size_t index = 0U; index < kernels.size(); ++index) {
      if (index > 0U) {
        calls << ", ";
      }
      calls << ((kernels[index].impl_type == KernelImplType::kCodegenImpl)
                    ? "1U"
                    : ((kernels[index].impl_type == KernelImplType::kEmptyImpl) ? "2U" : "0U"));
    }
    calls << "};\n"
          << "  static constexpr uint32_t " << sequence_name << "[] = {";
    for (size_t index = 0U; index < kernels.size(); ++index) {
      if (index > 0U) {
        calls << ", ";
      }
      calls << index << "U";
    }
    calls << "};\n"
          << "    OM2_ASSERT_SUCCESS(RunKernelSequence(" << phase << ", " << sequence_name << "Ops, " << sequence_name
          << ", ArraySize(" << sequence_name << ")));\n";
    return calls.str();
  };
  const std::string init_kernel_calls = emit_run_kernel_calls(program_model.init, "kInitSequence", "kInitKernelPhase");
  const std::string deinit_kernel_calls =
      emit_run_kernel_calls(program_model.deinit, "kDeInitSequence", "kDeInitKernelPhase");
  const auto emit_output_creation_calls = [](const std::vector<DynamicKernelCodegenInfo> &kernels,
                                             const char *const phase) {
    std::ostringstream calls;
    for (size_t index = 0U; index < kernels.size(); ++index) {
      const auto &kernel = kernels[index];
      if (kernel.output_policy == OutputCreatePolicy::kOutputsCreator) {
        calls << "    OM2_ASSERT_SUCCESS(CreateContextOutput(kOm2Model.program." << phase << "[" << index << "U]));\n";
      } else if (kernel.output_policy == OutputCreatePolicy::kCodegenImpl) {
        calls << "    OM2_ASSERT_SUCCESS(" << kernel.generated_outputs_creator_code << ");\n";
      }
    }
    return calls.str();
  };
  const std::string init_output_creation_calls = emit_output_creation_calls(program_model.init, "init");
  const std::string main_output_creation_calls = emit_output_creation_calls(program_model.main, "main");
  std::string main_kernel_calls;
  if (program_model.main_multithread_enabled) {
    main_kernel_calls = "    OM2_ASSERT_SUCCESS(RunMainWorkers());\n";
  } else if (!EmitMainProgramCalls(program_model, main_kernel_calls)) {
    GELOGE(ge::GRAPH_FAILED,
           "[OM2][ProgramBody] failed to emit main calls: plan %u root %u regions %zu steps(root) %zu",
           static_cast<uint32_t>(program_model.main_plan_type), program_model.main_root_region_index,
           program_model.main_regions.size(),
           (program_model.main_root_region_index < program_model.main_regions.size())
               ? program_model.main_regions[program_model.main_root_region_index].steps.size()
               : 0U);
    return {};
  }
  const std::string multithread_support = EmitMultiThreadProgramSupport(program_model);
  if (program_model.main_multithread_enabled && multithread_support.empty()) {
    return {};
  }
  const std::string kernel_impl_support = EmitKernelImplSupport(program_model);
  const std::string init_worker_pool_call =
      program_model.main_multithread_enabled ? "    OM2_ASSERT_SUCCESS(InitMainWorkerPool());\n" : std::string();
  const std::string destroy_worker_pool_call =
      program_model.main_multithread_enabled ? "    DestroyMainWorkerPool();\n" : std::string();
  std::string multithread_fields;
  if (program_model.main_multithread_enabled) {
    multithread_fields =
        "  void *worker_pool_{nullptr};\n"
        "  std::array<std::atomic<uint32_t>, " +
        std::to_string(program_model.main_multithread_plan.workers.size()) +
        "U> worker_progress_{};\n"
        "  std::atomic<aclError> worker_error_{kOk};\n";
  }
  std::string codegen_functions;
  const auto append_codegen_functions = [&codegen_functions](const auto &kernels) {
    for (const auto &kernel : kernels) {
      codegen_functions.append(kernel.generated_function);
    }
  };
  append_codegen_functions(program_model.init);
  std::vector<DynamicKernelCodegenInfo> unfused_main_kernels;
  for (size_t index = 0U; index < program_model.main.size(); ++index) {
    if (program_model.main_fused_member_kernels.count(static_cast<uint32_t>(index)) == 0U) {
      unfused_main_kernels.push_back(program_model.main[index]);
    }
  }
  append_codegen_functions(unfused_main_kernels);
  append_codegen_functions(program_model.deinit);
  std::string custom_op_factory_code;
  std::string custom_op_deserializer_code;
  if ((ge::ResolveStablePart(ge::StablePartId::kCreateDynamicClassCustomOpFactory, custom_op_factory_code) !=
       ge::SUCCESS) ||
      (ge::ResolveStablePart(ge::StablePartId::kCustomKernelDeserializer, custom_op_deserializer_code) !=
       ge::SUCCESS)) {
    return {};
  }
  ss << "#include \"" << option.interface_file_name << "\"\n";
  if (program_model.main_multithread_enabled) {
    ss << "#include <atomic>\n#include <thread>\n";
  }
  ss << R"(#include <functional>

#include "securec.h"
#include "om2_model_api.h"
#include "om2_model_api_utils.h"
#include "generated_program_assets.h"
#include "om2_runtime_value_types.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "acl/acl_rt.h"
#include "base/registry/op_impl_space_registry_v2.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "exe_graph/runtime/context_extend.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/gert_mem_allocator.h"
#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "graph/custom_op.h"
#include "graph/error_codes.h"
#include "graph/types.h"
#include "graph/utils/type_utils.h"
#include "base/runtime/runtime_attrs_def.h"
#include "register/kernel_registry.h"
#include "rt_external_base.h"
)";
  ss << "namespace ge {\n" << custom_op_factory_code << "}  // namespace ge\n";
  if (has_ge_kernel_bins) {
    ss << "#include \"graph/op_kernel_bin.h\"\n";
  }
  if (program_model.main_multithread_enabled) {
    ss << R"(extern "C" void *GeOm2CreateStaticWorkerPool(size_t worker_count, const uint32_t *worker_types);
extern "C" uint32_t GeOm2RunStaticWorkerPool(
    void *handle, void *stream, uint32_t (*func)(void *, size_t), void *opaque);
extern "C" void GeOm2DestroyStaticWorkerPool(void *handle);

)";
  }
  ss << R"(

#define OM2_DISMISSABLE_GUARD(var, callback) ScopeGuard scope_guard_##var(callback)
#define OM2_DISMISS_GUARD(var) scope_guard_##var.Dismiss()
#define OM2_ASSERT_SUCCESS(expr)             \
  do {                                       \
    const aclError om2_ret = (expr);         \
    if (om2_ret != ACL_SUCCESS) {            \
      (void)std::fprintf(stderr, "[OM2][ProgramBody] %s failed, ret=%d\\n", #expr, om2_ret); \
      (void)std::fflush(stderr);              \
      return om2_ret;                        \
    }                                        \
  } while (false)

namespace gert {
class RtSession;
using AicpuHostProcFunc = ge::graphStatus (*)(KernelContext *);

struct OuterWeightMem {
  const void *weight_ptr;
  size_t weight_size;
};

namespace kernel {
struct AclnnDeterministicConfig {
  bool has_op_deterministic = false;
  int32_t op_deterministic = 0;
  bool has_op_deterministic_level = false;
  int32_t op_deterministic_level = 0;
};
struct AclnnOriginalDeterministicConfig {
  int32_t deterministic = 0;
  int32_t deterministic_level = 0;
  int8_t reserved[4]{};
};
enum class MixType : uint32_t { MIX_AICORE = 0U, MIX_VECTOR_CORE = 1U };
struct MixCoreArgs {
  MixType mix_type;
  uint32_t all_core_num;
  uint32_t vec_core_num;
  uint32_t ai_core_num;
};
}  // namespace kernel

class AicpuResourceManager {
 public:
  static AicpuResourceManager &GetInstance();
  ge::graphStatus LoadConstantFoldingLib();
  std::function<AicpuHostProcFunc(std::string)> GetAicpuHostFindFunc() const;
};

class ModelDesc {
 public:
  void SetSpaceRegistries(OpImplSpaceRegistryV2Array *space_registries) {
    space_registries_ = space_registries;
  }
  const OpImplSpaceRegistryV2Array *GetSpaceRegistries() const {
    return space_registries_;
  }
  OpImplSpaceRegistryV2Array *GetSpaceRegistries() {
    return space_registries_;
  }
  void SetFileConstantWeightDir(const ge::char_t *file_constant_weight_dir) {
    file_constant_weight_dir_ = file_constant_weight_dir;
  }
  const ge::char_t *GetFileConstantWeightDir() const {
    return file_constant_weight_dir_;
  }

 private:
  OpImplSpaceRegistryV2Array *space_registries_;
  const ge::char_t *file_constant_weight_dir_;
};

class ContinuousBuffer {
 public:
  size_t GetNum() const {
    return num_;
  }
  template <typename T>
  T *Get(size_t index) {
    if (index >= num_) {
      return nullptr;
    }
    return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(this) + offsets_[index]);
  }
  template <typename T>
  const T *Get(size_t index) const {
    if (index >= num_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(reinterpret_cast<const uint8_t *>(this) + offsets_[index]);
  }

 private:
  size_t num_;
  int64_t reserved_;
  size_t offsets_[1];
};
}  // namespace gert

namespace om2 {
namespace {
constexpr aclError kOk = ACL_SUCCESS;
constexpr aclError kFailed = ACL_ERROR_FAILURE;
constexpr uint32_t kAivecElfMagic = 0x41415246U;
constexpr size_t kMaxSoSize = 1024U * 1024U * 1024U;
constexpr uint32_t kNoComputeNodeIndex = 0xFFFFFFFFU;
constexpr uint32_t kRunFuncImpl = 1U;
constexpr uint32_t kOutputsCreator = 1U;
constexpr uint32_t kInitKernelPhase = 0U;
constexpr uint32_t kMainKernelPhase = 1U;
constexpr uint32_t kDeInitKernelPhase = 2U;
static const bool kKernelTraceEnabled = []() {
  const char *const value = std::getenv("GE_OM2_KERNEL_TRACE");
  return (value != nullptr) && (std::strcmp(value, "1") == 0);
}();

class ScopeGuard {
 public:
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;

  explicit ScopeGuard(const std::function<void()> &callback) : callback_(callback) {}

  ~ScopeGuard() {
    if (callback_ != nullptr) {
      callback_();
    }
  }

  void Dismiss() {
    callback_ = nullptr;
  }

 private:
  std::function<void()> callback_;
};

template <typename T>
struct GeneratedArrayRef {
  const T *data;
  size_t size;

  const T *begin() const {
    return data;
  }

  const T *end() const {
    return size == 0U ? data : data + size;
  }

  const T &operator[](const size_t index) const {
    return data[index];
  }
};

struct KernelPortDef {
  uint32_t chain_index;
  const char *semantic_name;
  uint32_t value_type;
};

struct KernelDef {
  const char *execution_node_name;
  const char *kernel_type;
  GeneratedArrayRef<KernelPortDef> inputs;
  GeneratedArrayRef<KernelPortDef> outputs;
  uint32_t compute_node_index;
  uint32_t impl_type;
  uint32_t output_policy;
};

struct ProgramDef {
  GeneratedArrayRef<KernelDef> init;
  GeneratedArrayRef<KernelDef> main;
  GeneratedArrayRef<KernelDef> deinit;
};

struct RuntimeBindingDef {
  GeneratedArrayRef<uint32_t> streams;
  GeneratedArrayRef<uint32_t> external_allocators;
  GeneratedArrayRef<uint32_t> model_descs;
  GeneratedArrayRef<uint32_t> space_registries;
  GeneratedArrayRef<uint32_t> custom_op_registries;
  struct BinaryBinding {
    uint32_t chain_index;
    const char *key;
    bool is_ge_kernel_bin;
  };
  GeneratedArrayRef<BinaryBinding> binaries;
  GeneratedArrayRef<uint32_t> outer_weight_mems;
  struct HostTensorBinding {
    uint32_t chain_index;
    uint32_t weight_index;
    uint64_t data_offset;
    uint64_t data_size;
  };
  GeneratedArrayRef<HostTensorBinding> host_tensors;
  GeneratedArrayRef<uint32_t> session_ids;
  GeneratedArrayRef<uint32_t> notifies;
  GeneratedArrayRef<uint32_t> rt_sessions;
};

struct ConstChainDef {
  uint32_t chain_index;
  const char *producer_name;
  aclError (*initialize)(gert::Chain &chain, const void *definition);
  const void *definition;
};

struct GeneratedBinData {
  uint32_t magic;
  uint32_t version;
  const void *data;
  uint64_t length;
  uint64_t placeholder;
};

// Static and dynamic pbody share the CustomOp deserializer. Keep its input bytes
// separate from the AICore binary wrappers used by RT2 kernel bindings.
struct BinDataInfo {
  const void *data;
  size_t size;
};
)" << custom_op_deserializer_code
     << R"(
struct SharedLibraryView {
  const char *name;
  uint32_t so_bin_type;
  uint32_t opp_impl_version;
  const void *data;
  size_t size;
};

struct KernelBinaryView {
  const char *key;
  const void *data;
  size_t size;
};

struct ParsedLoadConfig {
  std::vector<SharedLibraryView> libraries;
  std::vector<std::string> binary_keys;
  std::vector<KernelBinaryView> binaries;
  void *external_allocators{nullptr};
  uint64_t session_id{0U};
  void **constants{nullptr};
  void *rt_session{nullptr};
};

struct ParsedRunConfig {
  void *stream{nullptr};
  void *external_allocators{nullptr};
  gert::Tensor *const *inputs{nullptr};
  size_t input_count{0U};
  gert::Tensor *const *outputs{nullptr};
  size_t output_count{0U};
  void *notifies{nullptr};
};
)";
  std::string metadata_initializer;
  ss << EmitStructuredResources(program_model, model_input_chains, model_output_chains, metadata_initializer);
  ss << EmitTypedConstChainSupport();
  ss << R"(struct ChainTableDef {
  size_t count;
  GeneratedArrayRef<ConstChainDef> constants;
  RuntimeBindingDef runtime_bindings;
};

struct Om2ModelDef {
  GeneratedModelMetadataDef metadata;
  ChainTableDef chains;
  ProgramDef program;
};
)";
  ss << "static const Om2ModelDef kOm2Model = {\n  " << metadata_initializer << ",\n  {\n    "
     << program_model.chains.size() << "U,\n    " << const_chain_ref << ",\n    "
     << EmitRuntimeBindingInitializer(
            stream_chains, external_allocator_chains, model_desc_chains, space_registry_chains,
            custom_op_registry_chains, binary_bindings, outer_weight_mem_chains, host_tensor_bindings,
            CollectSourceChains(program_model, ChainSourceKind::kSessionId), notify_chains, rt_session_chains)
     << "\n  },\n  " << EmitProgramInitializer(program_model) << "\n};\n";
  ss << R"(

template <typename T, size_t N>
constexpr size_t ArraySize(const T (&)[N]) {
  return N;
}

aclError ToAclStatus(const ge::graphStatus status) {
  return status == ge::GRAPH_SUCCESS ? kOk : kFailed;
}

template <typename T>
void SetReference(gert::Chain &chain, T *value) {
  chain.Set(static_cast<void *>(value), nullptr);
}

void SetReference(gert::Chain &chain, std::nullptr_t) {
  chain.Set(nullptr, nullptr);
}

struct GeneratedShapeRangeStorage {
  gert::Shape min;
  gert::Shape max;
};

// om2动态当前限制：以下存储结构镜像内部 framework/runtime/model_desc.h 的 I/O 和模型描述布局，
// InitStructuredResources 将其转换为 gert::ModelDesc，依赖成员偏移和对象布局保持一致。
// 后续需通过运行时构造接口或明确的稳定描述协议恢复，避免生成模型手工拼装内部对象。
struct GeneratedModelIoDescStorage {
  const ge::char_t *name;
  int32_t data_type;
  gert::StorageFormat format;
  gert::StorageShape shape;
  GeneratedShapeRangeStorage storage_shape_range;
  GeneratedShapeRangeStorage origin_shape_range;
  gert::Shape aipp_shape;
};

struct GeneratedModelDescStorage {
  gert::OpImplSpaceRegistryV2Array *space_registries;
  const ge::char_t *file_constant_weight_dir;
  size_t input_num;
  size_t output_num;
  size_t reusable_stream_num;
  size_t reusable_event_num;
  size_t reusable_notify_num;
  size_t attached_stream_num;
  gert::ContinuousVector model_io_descs;
};

void InitGeneratedShape(const GeneratedShapeDef &def, gert::Shape &shape) {
  shape.SetDimNum(0U);
  for (size_t i = 0U; i < def.stored_dim_num; ++i) {
    shape.SetDim(i, def.dims[i]);
  }
  shape.SetDimNum(def.dim_num);
}

void InitGeneratedTensorDesc(const GeneratedTensorDescDef &def, gert::CompileTimeTensorDesc &desc) {
  desc.SetDataType(static_cast<ge::DataType>(def.data_type));
  desc.SetOriginFormat(static_cast<ge::Format>(def.origin_format));
  desc.SetStorageFormat(static_cast<ge::Format>(def.storage_format));
  gert::ExpandDimsType expand_dims;
  for (size_t i = 0U; i < def.expand_dims_size; ++i) {
    if ((def.expand_dims_mask & (1ULL << i)) != 0U) {
      expand_dims.SetExpandIndex(i);
    }
  }
  desc.SetExpandDimsType(expand_dims);
  desc.SetExist(def.exists);
}

void InitGeneratedModelIo(const GeneratedModelIoDescDef &def, GeneratedModelIoDescStorage &desc) {
  desc.name = def.name;
  desc.data_type = def.data_type;
  desc.format.SetOriginFormat(static_cast<ge::Format>(def.origin_format));
  desc.format.SetStorageFormat(static_cast<ge::Format>(def.storage_format));
  InitGeneratedShape(def.origin_shape, desc.shape.MutableOriginShape());
  InitGeneratedShape(def.storage_shape, desc.shape.MutableStorageShape());
  InitGeneratedShape(def.origin_range.min, desc.origin_shape_range.min);
  InitGeneratedShape(def.origin_range.max, desc.origin_shape_range.max);
  InitGeneratedShape(def.storage_range.min, desc.storage_shape_range.min);
  InitGeneratedShape(def.storage_range.max, desc.storage_shape_range.max);
  InitGeneratedShape(def.aipp_shape, desc.aipp_shape);
}

aclError InitStructuredResources(
    std::vector<std::unique_ptr<uint8_t[]>> &compute_node_info_holders_,
    std::vector<const gert::ComputeNodeInfo *> &compute_node_infos_,
    std::unique_ptr<uint8_t[]> &model_desc_holder_, gert::ModelDesc *&model_desc_) {
  compute_node_info_holders_.clear();
  compute_node_infos_.clear();
  for (size_t i = 0U; i < kOm2Model.metadata.compute_nodes.size; ++i) {
    const auto &def = kOm2Model.metadata.compute_nodes[i];
    size_t size = 0U;
    if (gert::ComputeNodeInfo::CalcSize(def.ir_input_num, def.ir_output_num, def.input_descs.size,
                                        def.output_descs.size, size) != ge::GRAPH_SUCCESS) {
      return kFailed;
    }
    if ((def.attrs.data == nullptr) || (def.attrs.size < sizeof(RuntimeAttrsDef))) {
      return kFailed;
    }
    size += def.attrs.size;
    auto holder = std::make_unique<uint8_t[]>(size);
    if (memset_s(holder.get(), size, 0, size) != EOK) {
      return kFailed;
    }
    auto *info = reinterpret_cast<gert::ComputeNodeInfo *>(holder.get());
    info->Init(def.ir_input_num, def.ir_output_num, def.input_descs.size, def.output_descs.size,
               def.attrs.size, def.name, def.type);
    if (memcpy_s(info->MutableAttrs(), def.attrs.size, def.attrs.data, def.attrs.size) != EOK) {
      return kFailed;
    }
    for (size_t j = 0U; j < def.ir_input_num; ++j) {
      new (info->MutableInputInstanceInfo(j))
          gert::AnchorInstanceInfo(def.input_instances[j].instance_start, def.input_instances[j].instance_num);
    }
    for (size_t j = 0U; j < def.ir_output_num; ++j) {
      new (info->MutableOutputInstanceInfo(j))
          gert::AnchorInstanceInfo(def.output_instances[j].instance_start, def.output_instances[j].instance_num);
    }
    for (size_t j = 0U; j < def.input_descs.size; ++j) {
      new (info->MutableInputTdInfo(j)) gert::CompileTimeTensorDesc();
      InitGeneratedTensorDesc(def.input_descs[j], *info->MutableInputTdInfo(j));
    }
    for (size_t j = 0U; j < def.output_descs.size; ++j) {
      new (info->MutableOutputTdInfo(j)) gert::CompileTimeTensorDesc();
      InitGeneratedTensorDesc(def.output_descs[j], *info->MutableOutputTdInfo(j));
    }
    compute_node_infos_.push_back(info);
    compute_node_info_holders_.emplace_back(std::move(holder));
  }

  const auto &model_def = kOm2Model.metadata.model_desc;
  const size_t io_count = model_def.inputs.size + model_def.outputs.size;
  const size_t model_desc_size = sizeof(GeneratedModelDescStorage) + io_count * sizeof(GeneratedModelIoDescStorage);
  model_desc_holder_ = std::make_unique<uint8_t[]>(model_desc_size);
  if (memset_s(model_desc_holder_.get(), model_desc_size, 0, model_desc_size) != EOK) {
    return kFailed;
  }
  auto *model_desc = reinterpret_cast<GeneratedModelDescStorage *>(model_desc_holder_.get());
  model_desc->input_num = model_def.inputs.size;
  model_desc->output_num = model_def.outputs.size;
  model_desc->reusable_stream_num = model_def.reusable_stream_num;
  model_desc->reusable_event_num = model_def.reusable_event_num;
  model_desc->reusable_notify_num = model_def.reusable_notify_num;
  model_desc->attached_stream_num = model_def.attached_stream_num;
  model_desc->model_io_descs.Init(io_count);
  if (model_desc->model_io_descs.SetSize(io_count) != ge::GRAPH_SUCCESS) {
    return kFailed;
  }
  auto *io_descs = reinterpret_cast<GeneratedModelIoDescStorage *>(model_desc->model_io_descs.MutableData());
  for (size_t i = 0U; i < model_def.inputs.size; ++i) {
    InitGeneratedModelIo(model_def.inputs[i].desc, io_descs[i]);
  }
  for (size_t i = 0U; i < model_def.outputs.size; ++i) {
    InitGeneratedModelIo(model_def.outputs[i].desc, io_descs[model_def.inputs.size + i]);
  }
  model_desc_ = reinterpret_cast<gert::ModelDesc *>(model_desc_holder_.get());
  return kOk;
}

class StreamNotifyResources {
 public:
  ~StreamNotifyResources() {
    (void)Synchronize();
    Release();
  }

  aclError Initialize(aclrtStream load_stream, const size_t stream_num, const size_t notify_num) {
    if (load_stream_ != nullptr) {
      return kFailed;
    }
    // Default load path owns its initialization stream. External load streams retain
    // the legacy borrowed behavior; their full ABI integration is deferred.
    load_stream_ = load_stream;
    if (load_stream_ == nullptr) {
      OM2_ASSERT_SUCCESS(aclrtCreateStreamWithConfig(&load_stream_, 0U, 0U));
      owns_load_stream_ = true;
    }
    streams_ = CreateVector<aclrtStream>(stream_num);
    notifies_ = CreateVector<aclrtNotify>(notify_num);
    if ((streams_ == nullptr) || (notifies_ == nullptr)) {
      return kFailed;
    }
    auto *streams = Streams();
    if (stream_num != 0U) {
      streams->MutableData()[0U] = load_stream_;
    }
    for (size_t i = 1U; i < stream_num; ++i) {
      OM2_ASSERT_SUCCESS(aclrtCreateStreamWithConfig(&streams->MutableData()[i], 0U, 0U));
    }
    auto *notifies = Notifies();
    for (size_t i = 0U; i < notify_num; ++i) {
      // Keep the existing RT2 NotifyAllocator creation flags.
      OM2_ASSERT_SUCCESS(aclrtCreateNotify(&notifies->MutableData()[i], 0U));
    }
    return kOk;
  }

  aclError BindRunStream(aclrtStream stream) {
    if ((stream == nullptr) || (load_stream_ == nullptr)) {
      return kFailed;
    }
    auto *streams = Streams();
    if ((streams != nullptr) && (streams->GetSize() != 0U)) {
      // Match RT2: main graph consumes this invocation's stream in logical slot 0.
      // The caller owns execution streams and waits for queued work before unloading.
      streams->MutableData()[0U] = stream;
    }
    return kOk;
  }

  gert::TypedContinuousVector<aclrtStream> *Streams() const {
    return reinterpret_cast<gert::TypedContinuousVector<aclrtStream> *>(streams_.get());
  }

  gert::TypedContinuousVector<aclrtNotify> *Notifies() const {
    return reinterpret_cast<gert::TypedContinuousVector<aclrtNotify> *>(notifies_.get());
  }

  aclError Synchronize() const {
    aclError result = kOk;
    const auto sync = [&result](aclrtStream stream) {
      if (stream != nullptr) {
        const aclError ret = aclrtSynchronizeStream(stream);
        if (ret != ACL_SUCCESS) {
          OM2_LOGE("Synchronize pbody resource stream failed, ret=%d", ret);
          if (result == ACL_SUCCESS) {
            result = ret;
          }
        }
      }
    };
    sync(load_stream_);
    const auto *streams = Streams();
    if (streams != nullptr) {
      for (size_t i = 1U; i < streams->GetSize(); ++i) {
        sync(streams->GetData()[i]);
      }
    }
    return result;
  }

  // Caller synchronizes before releasing chains that can retain asynchronous task data.
  // Null-initialized slots also make partial creation failures safe to clean up.
  void Release() {
    auto *notifies = Notifies();
    if (notifies != nullptr) {
      for (size_t i = 0U; i < notifies->GetSize(); ++i) {
        if (notifies->GetData()[i] != nullptr) {
          (void)aclrtDestroyNotify(notifies->GetData()[i]);
        }
      }
    }
    auto *streams = Streams();
    if (streams != nullptr) {
      for (size_t i = 1U; i < streams->GetSize(); ++i) {
        if (streams->GetData()[i] != nullptr) {
          (void)aclrtDestroyStream(streams->GetData()[i]);
        }
      }
    }
    notifies_.reset();
    streams_.reset();
    if (owns_load_stream_ && (load_stream_ != nullptr)) {
      (void)aclrtDestroyStream(load_stream_);
    }
    load_stream_ = nullptr;
    owns_load_stream_ = false;
  }

 private:
  template <typename T>
  static std::unique_ptr<uint8_t[]> CreateVector(const size_t count) {
    auto storage = gert::ContinuousVector::Create<T>(count);
    if (storage == nullptr) {
      return nullptr;
    }
    auto *vector = reinterpret_cast<gert::TypedContinuousVector<T> *>(storage.get());
    if (vector->SetSize(count) != ge::GRAPH_SUCCESS) {
      return nullptr;
    }
    std::fill_n(vector->MutableData(), count, nullptr);
    return storage;
  }

  aclrtStream load_stream_{nullptr};
  bool owns_load_stream_{false};
  std::unique_ptr<uint8_t[]> streams_;
  std::unique_ptr<uint8_t[]> notifies_;
};

bool IsSafeSoName(const char *name) {
  if (name == nullptr) {
    return false;
  }
  const std::string file_name(name);
  if ((file_name.empty()) || (file_name.size() > 255U) ||
      (std::filesystem::path(file_name).filename().string() != file_name)) {
    return false;
  }
  return file_name.size() > 3U && file_name.compare(file_name.size() - 3U, 3U, ".so") == 0;
}

aclError MaterializeSoFiles(const std::vector<const SharedLibraryView *> &sos, std::filesystem::path &temp_dir) {
  auto path = std::filesystem::temp_directory_path() / "om2-registry-XXXXXX";
  std::string path_text = path.string();
  std::vector<char> path_buffer(path_text.begin(), path_text.end());
  path_buffer.push_back('\0');
  char *created = mkdtemp(path_buffer.data());
  if (created == nullptr) {
    return kFailed;
  }
  temp_dir = created;
  for (const auto *so : sos) {
    std::ofstream output(temp_dir / so->name, std::ios::binary | std::ios::trunc);
    if (!output.write(static_cast<const char *>(so->data), static_cast<std::streamsize>(so->size))) {
      output.close();
      std::filesystem::remove_all(temp_dir);
      return kFailed;
    }
  }
  return kOk;
}

)" << R"(class Om2DynamicProgramBody {
 public:
  Om2DynamicProgramBody() : chains_(kOm2Model.chains.count) {}

  ~Om2DynamicProgramBody() {
    ReleaseValues();
  }

  aclError Load(const GertModelLoadConfig &config) {
    if (state_ != State::kCreated) {
      return kFailed;
    }
    OM2_LOGI("[KernelTrace] Start model tracing.");
    ParsedLoadConfig parsed;
    OM2_ASSERT_SUCCESS(ParseLoadConfig(config, parsed));
    instance_handle_ = config.instance_handle;
    model_id_ = config.model_id;
    OM2_DISMISSABLE_GUARD(load_failure, [this]() {
      state_ = State::kFailed;
      ReleaseValues();
    });
    OM2_ASSERT_SUCCESS(InitSerializedResources());
    OM2_ASSERT_SUCCESS(BuildSpaceRegistries(parsed.libraries.data(), parsed.libraries.size()));
    OM2_ASSERT_SUCCESS(RestoreCustomOpBinaries(parsed.binaries.data(), parsed.binaries.size()));
    OM2_ASSERT_SUCCESS(BuildBinaries(parsed.binaries.data(), parsed.binaries.size()));
    OM2_ASSERT_SUCCESS(ResolveKernelFuncs(kOm2Model.program.init));
    OM2_ASSERT_SUCCESS(ResolveKernelFuncs(kOm2Model.program.main));
    OM2_ASSERT_SUCCESS(ResolveKernelFuncs(kOm2Model.program.deinit));
    OM2_ASSERT_SUCCESS(InitConstantChains());
    OM2_ASSERT_SUCCESS(BindLoadValues(nullptr, parsed.external_allocators, parsed.session_id,
                                      parsed.constants, parsed.rt_session));
    OM2_ASSERT_SUCCESS(InitContexts(kOm2Model.program.init, kernel_entries_[kInitKernelPhase]));
    OM2_ASSERT_SUCCESS(InitContexts(kOm2Model.program.main, kernel_entries_[kMainKernelPhase]));
    OM2_ASSERT_SUCCESS(InitContexts(kOm2Model.program.deinit, kernel_entries_[kDeInitKernelPhase]));
)" << init_output_creation_calls
     << main_output_creation_calls << R"(    OM2_ASSERT_SUCCESS(RunInitProgram());
    // Main may run on another stream, so initialization must finish before Load returns.
    OM2_ASSERT_SUCCESS(stream_notify_resources_.Synchronize());
)" << init_worker_pool_call
     << R"(    state_ = State::kLoaded;
    OM2_DISMISS_GUARD(load_failure);
    OM2_LOGI("[KernelTrace] End model tracing.");
    return kOk;
  }

  aclError RunAsync(aclrtStream stream, const GertModelRunConfig &config, const bool is_async = true) {
    if (state_ != State::kLoaded) {
      return kFailed;
    }
    ParsedRunConfig parsed;
    OM2_ASSERT_SUCCESS(ParseRunConfig(config, stream, parsed));
    OM2_ASSERT_SUCCESS(stream_notify_resources_.BindRunStream(parsed.stream));
    run_stream_ = parsed.stream;
    OM2_LOGI("[KernelTrace] Start model tracing.");
    const auto *callbacks = config.run_callbacks;
    const GertModelRunReportInfo report{sizeof(GertModelRunReportInfo), model_id_, is_async ? stream : nullptr,
                                       is_async ? 1U : 0U};
    if ((callbacks != nullptr) &&
        (callbacks->struct_size >= offsetof(GertModelRunCallbacks, report_run_info_preprocess) +
                                       sizeof(callbacks->report_run_info_preprocess)) &&
        (callbacks->report_run_info_preprocess != nullptr)) {
      (void)callbacks->report_run_info_preprocess(instance_handle_, &report);
    }
    BindRunValues(parsed);
    aclError run_ret = RunMainProgram();
    if (!is_async) {
      const aclError sync_ret = aclrtSynchronizeStreamWithTimeout(
          stream, static_cast<int32_t>(config.stream_sync_timeout_ms));
      if (run_ret == kOk) {
        run_ret = sync_ret;
      }
    }
    ClearRunValues();
    if ((run_ret == kOk) && (callbacks != nullptr) &&
        (callbacks->struct_size >= offsetof(GertModelRunCallbacks, report_run_info_postprocess) +
                                       sizeof(callbacks->report_run_info_postprocess)) &&
        (callbacks->report_run_info_postprocess != nullptr)) {
      (void)callbacks->report_run_info_postprocess(instance_handle_, &report);
    }
    OM2_LOGI("[KernelTrace] End model tracing.");
    return run_ret;
  }

  aclError Run(const GertModelRunConfig &config) {
    if ((state_ != State::kLoaded) ||
        (config.struct_size < offsetof(GertModelRunConfig, run_callbacks) + sizeof(config.run_callbacks))) {
      return kFailed;
    }
    if (sync_run_stream_ == nullptr) {
      OM2_ASSERT_SUCCESS(aclrtCreateStreamWithConfig(&sync_run_stream_, 0U, 0U));
    }
    return RunAsync(sync_run_stream_, config, false);
  }

  aclError Destroy() {
)" << destroy_worker_pool_call
     << R"(    if (sync_run_stream_ != nullptr) {
      (void)aclrtSynchronizeStream(sync_run_stream_);
    }
    (void)stream_notify_resources_.Synchronize();
    if (state_ == State::kLoaded) {
      (void)RunDeInitProgram();
    }
    state_ = State::kDestroyed;
    ReleaseValues();
    return kOk;
  }

 private:
  enum class State { kCreated, kLoaded, kFailed, kDestroyed };
  struct KernelEntry {
    const gert::KernelRegistry::KernelFuncs *funcs{nullptr};
    gert::KernelContext *context{nullptr};
  };
  // om2动态当前限制：Discuss the final runtime resource ownership design.
  // These pbody records are a temporary codegen adaptation.
  struct CustAicpuBinResource {
    aclrtBinHandle handle{nullptr};
    aclrtContext owner_context{nullptr};
    uint32_t output_chain_index{0U};
  };
)" << codegen_functions
     << multithread_support << R"(
  static aclError ParseLoadConfig(const GertModelLoadConfig &config, ParsedLoadConfig &parsed) {
    if (config.struct_size < offsetof(GertModelLoadConfig, external_notify_num) + sizeof(config.external_notify_num)) {
      return kFailed;
    }
    parsed.external_allocators = gert::om2::GetExternalAllocator(config);
    parsed.rt_session = gert::om2::GetRtSession(config);
    parsed.session_id = config.session_id == nullptr ? 0U : *config.session_id;
    parsed.constants = config.constants;
    if ((kOm2Model.chains.runtime_bindings.host_tensors.size != 0U) && (parsed.constants == nullptr)) {
      return kFailed;
    }
    // om2动态当前限制：默认由 pbody 创建加载流；外部加载流及资源数组的接入后续统一考虑。
    const auto *libraries = gert::om2::GetSharedLibraries(config);
    if (libraries != nullptr) {
      if (!gert::om2::IsValidSharedLibraries(*libraries)) {
        return kFailed;
      }
      for (uint64_t i = 0U; i < libraries->library_num; ++i) {
        const auto &library = *libraries->libraries[i];
        parsed.libraries.push_back({library.so_name, static_cast<uint32_t>(library.so_bin_type),
            static_cast<uint32_t>(gert::OppImplVersionTag::kOpp), library.data, library.data_size});
      }
    }
    if ((config.bin_num != 0U) &&
        ((config.bin_files == nullptr) || (config.bin_data == nullptr) || (config.bin_size == nullptr))) {
      return kFailed;
    }
    parsed.binary_keys.reserve(config.bin_num);
    parsed.binaries.reserve(config.bin_num);
    for (uint64_t i = 0U; i < config.bin_num; ++i) {
      if ((config.bin_files[i] == nullptr) || (config.bin_data[i] == nullptr) || (config.bin_size[i] == 0U)) {
        return kFailed;
      }
      const std::string name = std::filesystem::path(config.bin_files[i]).filename().string();
      // CustomOp keeps its serialization name; RT2 KernelBinData chains use a content key.
      parsed.binary_keys.push_back(IsCustomOpBinary(name.c_str())
          ? name : KernelBinaryKey(config.bin_data[i], config.bin_size[i]));
      parsed.binaries.push_back({parsed.binary_keys.back().c_str(), config.bin_data[i], config.bin_size[i]});
    }
    return kOk;
  }

  static aclError ParseRunConfig(const GertModelRunConfig &config, aclrtStream stream, ParsedRunConfig &parsed) {
    if ((config.struct_size < offsetof(GertModelRunConfig, run_callbacks) + sizeof(config.run_callbacks)) ||
        (stream == nullptr) ||
        ((config.input_count != 0U) && (config.input_data == nullptr)) ||
        ((config.output_count != 0U) && (config.output_data == nullptr)) ||
        (config.input_count != kOm2Model.metadata.model_desc.inputs.size) ||
        (config.output_count != kOm2Model.metadata.model_desc.outputs.size)) {
      return kFailed;
    }
    for (uint64_t i = 0U; i < config.input_count; ++i) {
      if (config.input_data[i] == nullptr) {
        return kFailed;
      }
    }
    for (uint64_t i = 0U; i < config.output_count; ++i) {
      if (config.output_data[i] == nullptr) {
        return kFailed;
      }
    }
    parsed.stream = stream;
    parsed.external_allocators = gert::om2::GetExternalAllocator(config);
    parsed.inputs = config.input_data;
    parsed.outputs = config.output_data;
    parsed.input_count = config.input_count;
    parsed.output_count = config.output_count;
    return kOk;
  }

  // Same FNV-1a content key as common/om2/om2_kernel_binary_utils.h.
  static std::string KernelBinaryKey(const void *data, const size_t size) {
    uint64_t hash = 1469598103934665603ULL;
    const auto *bytes = static_cast<const uint8_t *>(data);
    for (size_t i = 0U; i < size; ++i) {
      hash ^= bytes[i];
      hash *= 1099511628211ULL;
    }
    return std::to_string(hash);
  }

  aclError InitSerializedResources() {
    return InitStructuredResources(compute_node_info_holders_, compute_node_infos_, model_desc_holder_, model_desc_);
  }

  // om2动态当前限制：当前 pbody 直接依赖内部 base/registry/op_impl_space_registry_v2.h，
  // 构造 OpImplSpaceRegistryV2 并加载算子实现 SO；后续需重新设计 registry 的获取边界。
  // SpaceRegistry 加载后续移至 executor，复用 RT2 进程级 SO 内容去重及统一生命周期管理；
  // pbody 使用 executor 准备好的 registry，避免各模型和 pbody 实例重复处理 SO 加载。
  aclError BuildSpaceRegistries(const SharedLibraryView *so_data, const size_t so_count) {
    std::map<uint32_t, std::vector<const SharedLibraryView *>> grouped;
    std::map<uint32_t, std::set<std::string>> names_by_version;
    for (size_t index = 0U; index < so_count; ++index) {
      const auto &so = so_data[index];
      if (!IsSafeSoName(so.name) || (so.data == nullptr) || (so.size == 0U) || (so.size > kMaxSoSize) ||
          (so.so_bin_type > GERT_MODEL_SHARED_LIBRARY_CUSTOM_OP)) {
        return kFailed;
      }
      if (so.so_bin_type == GERT_MODEL_SHARED_LIBRARY_SPACE_REGISTRY) {
        if (!names_by_version[so.opp_impl_version].insert(so.name).second) {
          return kFailed;
        }
        grouped[so.opp_impl_version].push_back(&so);
      }
    }
    for (const auto &version_and_sos : grouped) {
      if (version_and_sos.first >= registries_.size()) {
        return kFailed;
      }
      std::filesystem::path temp_dir;
      OM2_ASSERT_SUCCESS(MaterializeSoFiles(version_and_sos.second, temp_dir));
      auto registry = std::make_shared<gert::OpImplSpaceRegistryV2>();
      if (registry == nullptr) {
        std::filesystem::remove_all(temp_dir);
        return kFailed;
      }
      for (const auto *so : version_and_sos.second) {
        if (std::string(so->name) == "libophost_comm_legacy.so") {
          continue;
        }
        const auto path = (temp_dir / so->name).string();
        gert::OppSoDesc desc({ge::AscendString(path.c_str())}, ge::AscendString(so->name));
        if (registry->AddSoToRegistry(desc) != ge::GRAPH_SUCCESS) {
          std::filesystem::remove_all(temp_dir);
          return kFailed;
        }
      }
      std::filesystem::remove_all(temp_dir);
      registries_[version_and_sos.first] = std::move(registry);
    }
    if (model_desc_ != nullptr) {
      model_desc_->SetSpaceRegistries(&registries_);
      model_desc_->SetFileConstantWeightDir("");
    }
    return kOk;
  }

  GeneratedBinData *BuildBinary(const void *data, const size_t size) {
    auto holder = std::make_unique<uint8_t[]>(sizeof(GeneratedBinData) + size);
    if (holder == nullptr) {
      return nullptr;
    }
    auto *binary = reinterpret_cast<GeneratedBinData *>(holder.get());
    binary->magic = kAivecElfMagic;
    binary->version = 0U;
    binary->data = nullptr;
    binary->length = size;
    if ((size != 0U) && (memcpy_s(&binary->placeholder, size, data, size) != EOK)) {
      return nullptr;
    }
    binary_holders_.emplace_back(std::move(holder));
    return binary;
  }

  static bool IsCustomOpBinary(const char *name) {
    const std::string key(name);
    constexpr char kSuffix[] = "_CustomKernel.bin";
    constexpr size_t kSuffixSize = sizeof(kSuffix) - 1U;
    return (key.size() > kSuffixSize) &&
           (key.compare(key.size() - kSuffixSize, kSuffixSize, kSuffix) == 0);
  }

  static aclError RestoreCustomOpBinaries(const KernelBinaryView *binaries, const size_t binary_count) {
    if ((binary_count != 0U) && (binaries == nullptr)) {
      return kFailed;
    }
    std::unordered_map<std::string, BinDataInfo> custom_binaries;
    for (size_t i = 0U; i < binary_count; ++i) {
      const auto &binary = binaries[i];
      if ((binary.key == nullptr) || (binary.data == nullptr) || (binary.size == 0U)) {
        return kFailed;
      }
      if (!IsCustomOpBinary(binary.key)) {
        continue;
      }
      const auto inserted = custom_binaries.emplace(binary.key, BinDataInfo{binary.data, binary.size});
      if (!inserted.second &&
          ((inserted.first->second.size != binary.size) ||
           (std::memcmp(inserted.first->second.data, binary.data, binary.size) != 0))) {
        return kFailed;
      }
    }
    return DeserializeCustKernelBinaries(custom_binaries);
  }

  aclError BuildBinaries(const KernelBinaryView *binaries, const size_t binary_count) {
    if (binary_count == 0U) {
      binary_holders_.clear();
      binary_data_.clear();
      return kOk;
    }
    if (binaries == nullptr) {
      return kFailed;
    }
    binary_holders_.clear();
    binary_data_.clear();
    for (size_t i = 0U; i < binary_count; ++i) {
      if (binaries[i].key == nullptr || binaries[i].data == nullptr || binaries[i].size == 0U) {
        return kFailed;
      }
      if (IsCustomOpBinary(binaries[i].key)) {
        continue;
      }
      if (binary_data_.find(binaries[i].key) != binary_data_.cend()) {
        continue;
      }
      auto *const binary = BuildBinary(binaries[i].data, binaries[i].size);
      if (binary == nullptr) {
        return kFailed;
      }
      (void)binary_data_.emplace(binaries[i].key, binary);
    }
    return kOk;
  }

  aclError ResolveKernelFuncs(const GeneratedArrayRef<KernelDef> &defs) {
    auto &registry = gert::KernelRegistry::GetInstance();
    for (size_t i = 0U; i < defs.size; ++i) {
      if (defs[i].impl_type != kRunFuncImpl) {
        continue;
      }
      if (kernel_funcs_.find(defs[i].kernel_type) != kernel_funcs_.cend()) {
        continue;
      }
      auto *funcs = registry.FindKernelFuncs(defs[i].kernel_type);
      if ((funcs == nullptr) || (funcs->run_func == nullptr)) {
        return kFailed;
      }
      kernel_funcs_[defs[i].kernel_type] = funcs;
    }
    return kOk;
  }

  aclError InitConstantChains() {
    for (const auto &def : kOm2Model.chains.constants) {
      if ((def.chain_index >= chains_.size()) || (def.initialize == nullptr)) {
        return kFailed;
      }
      const aclError ret = def.initialize(chains_[def.chain_index], def.definition);
      if (ret != ACL_SUCCESS) {
        (void)std::fprintf(stderr, "[OM2][ProgramBody] constant chain %u (%s) initializer failed, ret=%d\\n",
                           def.chain_index, def.producer_name == nullptr ? "" : def.producer_name, ret);
        (void)std::fflush(stderr);
        return ret;
      }
    }
    return kOk;
  }

  aclError BindLoadValues(void *stream, void *external_allocators, const uint64_t session_id,
                          void **constants, void *rt_session = nullptr) {
    session_id_ = session_id;
    rt_session_ = rt_session;
    outer_weight_mem_ = {nullptr, 0U};
    const auto &bindings = kOm2Model.chains.runtime_bindings;
    const auto is_valid_chain = [this](const uint32_t chain_index) {
      return chain_index < chains_.size();
    };
    const auto &resource_desc = kOm2Model.metadata.model_desc;
    if (resource_desc.attached_stream_num >
        std::numeric_limits<size_t>::max() - resource_desc.reusable_stream_num) {
      return kFailed;
    }
    const size_t stream_num = bindings.streams.size == 0U
                                  ? 0U
                                  : std::max<size_t>(1U, resource_desc.reusable_stream_num +
                                                            resource_desc.attached_stream_num);
    const size_t notify_num = bindings.notifies.size == 0U ? 0U : resource_desc.reusable_notify_num;
    OM2_ASSERT_SUCCESS(stream_notify_resources_.Initialize(stream, stream_num, notify_num));
    for (const auto chain_index : bindings.streams) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      chains_[chain_index].Set(stream_notify_resources_.Streams(), nullptr);
    }
    for (const auto chain_index : bindings.notifies) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      chains_[chain_index].Set(stream_notify_resources_.Notifies(), nullptr);
    }
    // om2动态当前限制：临时方案，外置内存 allocator 集合当前以指针透传并绑定到 chain，交由 RT2 kernel 使用。
    // pbody 不构造或销毁该集合；nullptr 保留 SelectL1Allocator 使用图内 allocator 的行为。
    for (const auto chain_index : bindings.external_allocators) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      chains_[chain_index].Set(external_allocators, nullptr);
    }
    for (const auto chain_index : bindings.outer_weight_mems) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      SetReference(chains_[chain_index], &outer_weight_mem_);
    }
    if ((bindings.model_descs.size != 0U) && (model_desc_ == nullptr)) {
      return kFailed;
    }
    for (const auto chain_index : bindings.model_descs) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      SetReference(chains_[chain_index], model_desc_);
    }
    constexpr auto kDefaultOppImplVersion =
        static_cast<size_t>(gert::OppImplVersionTag::kOpp);
    // A model_desc binding also consumes this registry indirectly: GetSpaceRegistry reads
    // model_desc->GetSpaceRegistries(), which is filled from registries_ below, so kernels such
    // as FindInferShapeFunc would otherwise resolve a null registry even though no chain binds
    // one directly. Fill it for either binding kind, but never overwrite an existing registry.
    if (((bindings.space_registries.size != 0U) || (bindings.model_descs.size != 0U)) &&
        (registries_[kDefaultOppImplVersion] == nullptr)) {
      // Dynamic OM2 does not serialize aclnn-only registry binaries. Reuse the process registry
      // populated by the FXRT/GE lowering path in this process.
      registries_[kDefaultOppImplVersion] = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry(
          gert::OppImplVersionTag::kOpp);
    }
    if ((bindings.space_registries.size != 0U) && (registries_[kDefaultOppImplVersion] == nullptr)) {
      // Only a direct registry consumer makes a missing registry a hard Load failure; a model
      // that merely carries model_desc must not gain a new loading requirement.
      return kFailed;
    }
    auto *space_registry = registries_[kDefaultOppImplVersion].get();
    for (const auto chain_index : bindings.space_registries) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      SetReference(chains_[chain_index], space_registry);
    }
    if (bindings.custom_op_registries.size != 0U) {
      // Borrow the global registry used by static pbody after executor loads CustomOp SOs.
      // Do not create or clear a per-model registry.
      custom_op_registry_ = ge::CustomOpFactory::GetGlobalRegistryPtr();
      if (custom_op_registry_ == nullptr) {
        return kFailed;
      }
      for (const auto chain_index : bindings.custom_op_registries) {
        if (!is_valid_chain(chain_index)) {
          return kFailed;
        }
        SetReference(chains_[chain_index], custom_op_registry_.get());
      }
    }
    for (size_t i = 0U; i < bindings.binaries.size; ++i) {
      const auto &binding = bindings.binaries[i];
      if (!is_valid_chain(binding.chain_index) || binding.key == nullptr) {
        return kFailed;
      }
      const auto iter = binary_data_.find(binding.key);
      if (iter == binary_data_.cend()) {
        return kFailed;
      }
)"
     << (has_ge_kernel_bins ? R"(      if (binding.is_ge_kernel_bin) {
        auto holder = ge_kernel_bins_.find(binding.key);
        if (holder == ge_kernel_bins_.cend()) {
          auto *const binary = iter->second;
          if ((binding.key[0] == '\0') || (binary->length == 0U) ||
              (binary->length > std::vector<char>().max_size())) {
            return kFailed;
          }
          std::vector<char> bytes(binary->length);
          if (memcpy_s(bytes.data(), bytes.size(), &binary->placeholder, bytes.size()) != EOK) {
            return kFailed;
          }
          // om2动态当前限制：pbody 构造并持有内部 OpKernelBin，向 RT2 launch kernel 传递
          // shared_ptr<OpKernelBin> 的地址；内嵌头文件仅解决编译依赖，未消除 C++ ABI 耦合。
          // 后续需重新设计二进制资源边界，由运行时恢复并管理该对象，模型只提供二进制描述。
          holder = ge_kernel_bins_.emplace(
              binding.key, std::make_shared<ge::OpKernelBin>(binding.key, std::move(bytes))).first;
        }
        SetReference(chains_[binding.chain_index], &holder->second);
        continue;
      }
)"
                            : "")
     << R"(      SetReference(chains_[binding.chain_index], iter->second);
    }
    for (const auto chain_index : bindings.session_ids) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      chains_[chain_index].Set(&session_id_, nullptr);
    }
    if ((bindings.rt_sessions.size != 0U) && (rt_session_ == nullptr)) {
      (void)std::fprintf(stderr, "[OM2][ProgramBody] model has %zu rt_session chains but none supplied\\n",
                         static_cast<size_t>(bindings.rt_sessions.size));
      (void)std::fflush(stderr);
      return kFailed;
    }
    for (const auto chain_index : bindings.rt_sessions) {
      if (!is_valid_chain(chain_index)) {
        return kFailed;
      }
      // om2动态当前限制：RtSession 已由运行时准备，pbody 仍向 kernel 透传内部 C++ 对象指针；
      // 后续需统一 session 访问边界，不能将该指针视为可落盘或跨版本稳定的模型数据。
      SetReference(chains_[chain_index], rt_session_);
    }
    return BindHostConstants(constants);
  }

  aclError BindHostConstants(void **constants) {
    const auto &bindings = kOm2Model.chains.runtime_bindings.host_tensors;
    // constants uses the same GeModel indices as the generated bindings (including empty slots).
    // Executor validates each source file against constants_config before supplying these addresses.
    for (const auto &binding : bindings) {
      if ((binding.chain_index >= chains_.size()) || (constants == nullptr) ||
          (constants[binding.weight_index] == nullptr) ||
          (binding.data_size > UINT64_MAX - binding.data_offset)) {
        return kFailed;
      }
      auto *const tensor = chains_[binding.chain_index].GetValue<gert::Tensor *>();
      if (tensor == nullptr) {
        return kFailed;
      }
      auto &tensor_data = tensor->MutableTensorData();
      if (tensor_data.SetAddr(static_cast<uint8_t *>(constants[binding.weight_index]) + binding.data_offset,
                              nullptr) != ge::GRAPH_SUCCESS) {
        return kFailed;
      }
      tensor_data.SetSize(binding.data_size);
    }
    return kOk;
  }

  void BindRunValues(const ParsedRunConfig &config) {
    // Rebind graph arguments for every main invocation, as RT2 SpecifyArgsInputs does.
    for (const auto chain_index : kOm2Model.chains.runtime_bindings.external_allocators) {
      chains_[chain_index].Set(config.external_allocators, nullptr);
    }
    for (const auto chain_index : kOm2Model.chains.runtime_bindings.streams) {
      chains_[chain_index].Set(stream_notify_resources_.Streams(), nullptr);
    }
    const auto &model_desc = kOm2Model.metadata.model_desc;
    for (size_t i = 0U; i < model_desc.inputs.size; ++i) {
      chains_[model_desc.inputs[i].chain_index].Set(config.inputs[i], nullptr);
    }
    for (size_t i = 0U; i < model_desc.outputs.size; ++i) {
      chains_[model_desc.outputs[i].chain_index].Set(config.outputs[i], nullptr);
    }
    for (const auto chain_index : kOm2Model.chains.runtime_bindings.notifies) {
      // Legacy callers may still supply a borrowed notify collection. Otherwise use pbody resources.
      chains_[chain_index].Set(config.notifies != nullptr ? config.notifies : stream_notify_resources_.Notifies(),
                               nullptr);
    }
  }

  void ClearRunValues() {
    run_stream_ = nullptr;
    const auto &model_desc = kOm2Model.metadata.model_desc;
    for (const auto &input : model_desc.inputs) {
      chains_[input.chain_index].Set(nullptr, nullptr);
    }
    for (const auto &output : model_desc.outputs) {
      chains_[output.chain_index].Set(nullptr, nullptr);
    }
    for (const auto chain_index : kOm2Model.chains.runtime_bindings.notifies) {
      chains_[chain_index].Set(nullptr, nullptr);
    }
  }

  aclError InitContexts(const GeneratedArrayRef<KernelDef> &defs, std::vector<KernelEntry> &entries) {
    entries.clear();
    entries.resize(defs.size);
    for (size_t i = 0U; i < defs.size; ++i) {
      const auto &def = defs[i];
      if (def.impl_type != kRunFuncImpl) {
        continue;
      }
      const size_t value_count = def.inputs.size + def.outputs.size;
      const size_t context_size = sizeof(KernelRunContext) + sizeof(AsyncAnyValue *) * value_count;
      auto storage = std::make_unique<uint8_t[]>(context_size + sizeof(gert::KernelExtendInfo));
      if (memset_s(storage.get(), context_size + sizeof(gert::KernelExtendInfo), 0,
                   context_size + sizeof(gert::KernelExtendInfo)) != EOK) {
        return kFailed;
      }
      auto *context = reinterpret_cast<gert::KernelContext *>(storage.get());
      auto *run_context = context->GetContext();
      run_context->input_size = def.inputs.size;
      run_context->output_size = def.outputs.size;
      run_context->output_start = &run_context->values[def.inputs.size];
      if (def.compute_node_index != kNoComputeNodeIndex) {
        if (def.compute_node_index >= compute_node_infos_.size()) {
          return kFailed;
        }
        run_context->compute_node_info = const_cast<gert::ComputeNodeInfo *>(compute_node_infos_[def.compute_node_index]);
      }
      auto *kernel_extend_info = reinterpret_cast<gert::KernelExtendInfo *>(storage.get() + context_size);
      kernel_extend_info->SetKernelName(def.execution_node_name);
      kernel_extend_info->SetKernelType(def.kernel_type);
      run_context->kernel_extend_info = kernel_extend_info;
      for (size_t input_index = 0U; input_index < def.inputs.size; ++input_index) {
        run_context->values[input_index] =
            reinterpret_cast<AsyncAnyValue *>(&chains_[def.inputs[input_index].chain_index]);
      }
      for (size_t output_index = 0U; output_index < def.outputs.size; ++output_index) {
        run_context->values[def.inputs.size + output_index] =
            reinterpret_cast<AsyncAnyValue *>(&chains_[def.outputs[output_index].chain_index]);
      }
      KernelEntry entry;
      entry.funcs = kernel_funcs_.at(def.kernel_type);
      entry.context = context;
      contexts_.emplace_back(std::move(storage));
      entries[i] = entry;
      kernels_[def.execution_node_name] = entry;
    }
    return kOk;
  }

  aclError InitInlineGertTensorDataOutput(gert::Chain &chain, const size_t data_size,
                                          const gert::TensorPlacement placement, const int64_t stream_id) {
    if (data_size > std::numeric_limits<size_t>::max() - sizeof(gert::GertTensorData)) {
      return kFailed;
    }
    const size_t storage_size = sizeof(gert::GertTensorData) + data_size;
    auto storage = std::make_unique<uint8_t[]>(storage_size);
    new (storage.get()) gert::GertTensorData(storage.get() + sizeof(gert::GertTensorData), data_size,
                                             placement, stream_id);
    chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
    return kOk;
  }

  aclError CreateContextOutput(const KernelDef &def) {
    if ((def.impl_type != kRunFuncImpl) || (def.output_policy != kOutputsCreator)) {
      return kOk;
    }
    auto &entry = kernels_.at(def.execution_node_name);
    if ((entry.funcs->outputs_creator != nullptr) &&
        (entry.funcs->outputs_creator(nullptr, entry.context) != ge::GRAPH_SUCCESS)) {
      return kFailed;
    }
    return kOk;
  }

  aclError InitCopyFlowSizesOutput(gert::Chain &chain, const size_t count) {
    auto storage = gert::ContinuousVector::Create<size_t>(count);
    if (storage == nullptr) {
      return kFailed;
    }
    chain.SetWithDefaultDeleter<uint8_t[]>(storage.release());
    return kOk;
  }

  aclError InitCopyFlowTensorOutputs(const std::initializer_list<gert::Chain *> outputs) {
    for (auto *const chain : outputs) {
      auto *value = new (std::nothrow) gert::GertTensorData();
      if (value == nullptr) {
        return kFailed;
      }
      value->SetPlacement(gert::kOnDeviceHbm);
      chain->SetWithDefaultDeleter(value);
    }
    return kOk;
  }

  aclError InitCopyFlowAllocOutput(gert::Chain &chain, const size_t count) {
    auto storage = gert::ContinuousVector::Create<gert::GertTensorData *>(count);
    if (storage == nullptr) {
      return kFailed;
    }
    auto *values = reinterpret_cast<gert::TypedContinuousVector<gert::GertTensorData *> *>(storage.get());
    auto **data = values->MutableData();
    for (size_t i = 0U; i < count; ++i) {
      data[i] = new (std::nothrow) gert::GertTensorData();
      if (data[i] == nullptr) {
        for (size_t j = 0U; j < i; ++j) {
          delete data[j];
        }
        return kFailed;
      }
    }
    values->SetSize(count);
    chain.Set(storage.release(), [](void *ptr) {
      auto *items = static_cast<gert::TypedContinuousVector<gert::GertTensorData *> *>(ptr);
      for (size_t i = 0U; i < items->GetSize(); ++i) {
        (void)items->MutableData()[i]->Free();
      }
      for (size_t i = 0U; i < items->GetCapacity(); ++i) {
        delete items->MutableData()[i];
      }
      delete[] static_cast<uint8_t *>(ptr);
    });
    return kOk;
  }

)" << kernel_impl_support
     << R"(  aclError RunKernelSequence(const uint32_t phase, const uint8_t *const ops, const uint32_t *const sequence,
                               const size_t count) {
    for (size_t index = 0U; index < count; ++index) {
      if (ops[index] == 2U) {
        continue;
      }
      const aclError ret = (ops[index] == 0U) ? RunKernel(phase, sequence[index])
                                              : RunKernelImplDispatch(phase, sequence[index]);
      if (ret != kOk) {
        (void)std::fprintf(stderr, "[OM2][ProgramBody] RunKernel failed, phase=%u index=%u ret=%d\\n", phase,
                           sequence[index], static_cast<int32_t>(ret));
        return ret;
      }
    }
    return kOk;
  }

  aclError RunInitProgram() {
)" << init_kernel_calls
     << R"(    return kOk;
  }

  aclError RunMainProgram() {
)" << main_kernel_calls
     << R"(    return kOk;
  }

  aclError RunDeInitProgram() {
)" << deinit_kernel_calls
     << R"(    return kOk;
  }

  aclError RunKernel(const uint32_t phase, const uint32_t kernel_index) {
    const auto &entry = kernel_entries_[phase][kernel_index];
    const auto &program = phase == kInitKernelPhase
                              ? kOm2Model.program.init
                              : (phase == kMainKernelPhase ? kOm2Model.program.main : kOm2Model.program.deinit);
    if (kKernelTraceEnabled) {
      OM2_LOGI("[KernelTrace][Begin][phase=%u][index=%u][%s][func=0x%llx][context=%p]", phase, kernel_index,
               program[kernel_index].execution_node_name,
               static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.funcs->run_func)),
               static_cast<void *>(entry.context));
    }
    const auto ret = ToAclStatus(entry.funcs->run_func(entry.context));
    if ((ret == ACL_SUCCESS) && kKernelTraceEnabled) {
      OM2_LOGI("[KernelTrace][%s]", program[kernel_index].execution_node_name);
    }
    return ret;
  }

  aclError ReleaseCustAicpuBinResource(CustAicpuBinResource &resource) {
    if (resource.handle == nullptr) {
      return kOk;
    }
    aclrtContext current = nullptr;
    const auto context_ret = aclrtGetCurrentContext(&current);
    if ((context_ret != ACL_SUCCESS) && (context_ret != ACL_ERROR_RT_CONTEXT_NULL)) {
      return kFailed;
    }
    if (context_ret == ACL_ERROR_RT_CONTEXT_NULL) {
      current = nullptr;
    }
    const bool switch_context = current != resource.owner_context;
    if (switch_context && (aclrtSetCurrentContext(resource.owner_context) != ACL_SUCCESS)) {
      return kFailed;
    }
    const auto ret = aclrtBinaryUnLoad(resource.handle);
    if (ret == ACL_SUCCESS) {
      resource.handle = nullptr;
      resource.owner_context = nullptr;
      chains_[resource.output_chain_index].Set(nullptr, nullptr);
    }
    // Match the runtime lease: an initially unbound caller keeps the owner's context.
    if (switch_context && (current != nullptr) && (aclrtSetCurrentContext(current) != ACL_SUCCESS)) {
      return kFailed;
    }
    return ret == ACL_SUCCESS ? kOk : kFailed;
  }

  aclError ReleaseCustAicpuBin(gert::Chain &lease_chain) {
    auto *resource = lease_chain.GetValue<CustAicpuBinResource *>();
    if (resource == nullptr) {
      return kOk;
    }
    const auto ret = ReleaseCustAicpuBinResource(*resource);
    if (resource->handle == nullptr) {
      lease_chain.Set(nullptr, nullptr);
    }
    return ret;
  }

  aclError LoadCustAicpuBin(const uint32_t output_index, const uint32_t lease_index, const char *key) {
    const auto binary = binary_data_.find(key);
    if (binary == binary_data_.end()) {
      return kFailed;
    }
    auto &resource = cust_aicpu_bin_resources_[lease_index];
    OM2_ASSERT_SUCCESS(ReleaseCustAicpuBinResource(resource));
    resource.output_chain_index = output_index;
    chains_[output_index].Set(nullptr, nullptr);
    chains_[lease_index].Set(nullptr, nullptr);
    if ((aclrtGetCurrentContext(&resource.owner_context) != ACL_SUCCESS) || (resource.owner_context == nullptr)) {
      return kFailed;
    }
    aclrtBinaryLoadOption option{};
    option.type = ACL_RT_BINARY_LOAD_OPT_CPU_KERNEL_MODE;
    option.value.cpuKernelMode = 2;
    const aclrtBinaryLoadOptions options{&option, 1U};
    const auto ret = aclrtBinaryLoadFromData(&binary->second->placeholder, binary->second->length,
                                            &options, &resource.handle);
    if ((ret != ACL_SUCCESS) || (resource.handle == nullptr)) {
      return kFailed;
    }
    chains_[output_index].Set(resource.handle, nullptr);
    chains_[lease_index].Set(&resource, nullptr);
    return kOk;
  }

  void ReleaseValues() {
)" << destroy_worker_pool_call
     << R"(    if (sync_run_stream_ != nullptr) {
      (void)aclrtSynchronizeStream(sync_run_stream_);
    }
    (void)stream_notify_resources_.Synchronize();
    // Also covers partial Init failures, for which the deinit program did not run.
    for (auto it = cust_aicpu_bin_resources_.begin(); it != cust_aicpu_bin_resources_.end();) {
      (void)ReleaseCustAicpuBinResource(it->second);
      if (it->second.handle == nullptr) {
        it = cust_aicpu_bin_resources_.erase(it);
      } else {
        ++it;
      }
    }
    // Runtime values can retain memory blocks owned by allocator chains.  Chains are
    // allocated in producer order, so release consumers first and allocator owners last.
    for (auto chain = chains_.rbegin(); chain != chains_.rend(); ++chain) {
      chain->Set(nullptr, nullptr);
    }
)" << (has_ge_kernel_bins ? "    ge_kernel_bins_.clear();\n" : "")
     << R"(
    for (const auto &path_and_handle : aicpu_bin_handles_) {
      if (path_and_handle.second != nullptr) {
        (void)aclrtBinaryUnLoad(path_and_handle.second);
      }
    }
    aicpu_bin_handles_.clear();
    custom_op_registry_.reset();
    stream_notify_resources_.Release();
    if (sync_run_stream_ != nullptr) {
      (void)aclrtDestroyStream(sync_run_stream_);
      sync_run_stream_ = nullptr;
    }
  }

  State state_{State::kCreated};
  std::unique_ptr<uint8_t[]> model_desc_holder_;
  std::vector<std::unique_ptr<uint8_t[]>> compute_node_info_holders_;
  std::vector<std::unique_ptr<uint8_t[]>> binary_holders_;
  StreamNotifyResources stream_notify_resources_;
  std::vector<std::unique_ptr<uint8_t[]>> contexts_;
  std::vector<const gert::ComputeNodeInfo *> compute_node_infos_;
  gert::ModelDesc *model_desc_{nullptr};
  void *rt_session_{nullptr};
  void *run_stream_{nullptr};
  aclrtStream sync_run_stream_{nullptr};
  void *instance_handle_{nullptr};
  uint64_t model_id_{0U};
  uint64_t session_id_{0U};
  gert::OuterWeightMem outer_weight_mem_{nullptr, 0U};
  gert::OpImplSpaceRegistryV2Array registries_{};
  std::shared_ptr<ge::CustomOpRegistry> custom_op_registry_;
  std::unordered_map<std::string, GeneratedBinData *> binary_data_;
)" << (has_ge_kernel_bins ? "  std::unordered_map<std::string, ge::OpKernelBinPtr> ge_kernel_bins_;\n" : "")
     << R"(
  std::map<std::string, const gert::KernelRegistry::KernelFuncs *> kernel_funcs_;
  std::map<std::string, KernelEntry> kernels_;
  std::map<std::string, aclrtBinHandle> aicpu_bin_handles_;
  std::map<uint32_t, CustAicpuBinResource> cust_aicpu_bin_resources_;
  std::vector<gert::Chain> chains_;
  std::array<std::vector<KernelEntry>, 3U> kernel_entries_;
)" << multithread_fields
     << R"(};
)" << R"(}  // namespace
}  // namespace om2

extern "C" __attribute__((visibility("default"))) int32_t GertModelLoad(
    const GertModelLoadConfig *config, GertModelHandle *model_handle, GertModelLoadOutput *) try {
  if (model_handle == nullptr) {
    return ACL_ERROR_FAILURE;
  }
  *model_handle = nullptr;
  if (config == nullptr) {
    return ACL_ERROR_FAILURE;
  }
  auto body = std::make_unique<om2::Om2DynamicProgramBody>();
  const aclError ret = body->Load(*config);
  if (ret != ACL_SUCCESS) {
    return ret;
  }
  *model_handle = body.release();
  return ACL_SUCCESS;
} catch (...) {
  return ACL_ERROR_FAILURE;
}

extern "C" __attribute__((visibility("default"))) int32_t GertModelRunAsync(
    GertModelHandle model_handle, aclrtStream stream, const GertModelRunConfig *config, GertModelRunOutput *) try {
  if ((model_handle == nullptr) || (config == nullptr)) {
    return ACL_ERROR_FAILURE;
  }
  return static_cast<om2::Om2DynamicProgramBody *>(model_handle)->RunAsync(stream, *config);
} catch (...) {
  return ACL_ERROR_FAILURE;
}

extern "C" __attribute__((visibility("default"))) int32_t GertModelRun(
    GertModelHandle model_handle, const GertModelRunConfig *config, GertModelRunOutput *) try {
  if ((model_handle == nullptr) || (config == nullptr)) {
    return ACL_ERROR_FAILURE;
  }
  return static_cast<om2::Om2DynamicProgramBody *>(model_handle)->Run(*config);
} catch (...) {
  return ACL_ERROR_FAILURE;
}

extern "C" __attribute__((visibility("default"))) int32_t GertModelUnload(
    GertModelHandle model_handle, const GertModelUnloadConfig *, GertModelUnloadOutput *) try {
  if (model_handle == nullptr) {
    return ACL_ERROR_FAILURE;
  }
  auto body = std::unique_ptr<om2::Om2DynamicProgramBody>(
      static_cast<om2::Om2DynamicProgramBody *>(model_handle));
  return body->Destroy();
} catch (...) {
  return ACL_ERROR_FAILURE;
}

)";
  return ss.str();
}

std::string BuildGeneratedProgramAssetsHeader(const DynamicProgramModel &program_model) {
  std::ostringstream ss;
  ss << "// Generated by ProgramBodyCodegen. Do not edit.\n";
  ss << "#ifndef " << program_model.model_name << "_GENERATED_PROGRAM_ASSETS_H_\n";
  ss << "#define " << program_model.model_name << "_GENERATED_PROGRAM_ASSETS_H_\n\n";
  ss << "#include <cstddef>\n\n";
  ss << "namespace om2::generated {\n";
  ss << "inline constexpr size_t kChainCount = " << program_model.chains.size() << "U;\n";
  ss << "inline constexpr size_t kInitKernelCount = " << program_model.init.size() << "U;\n";
  ss << "inline constexpr size_t kMainKernelCount = " << program_model.main.size() << "U;\n";
  ss << "inline constexpr size_t kDeInitKernelCount = " << program_model.deinit.size() << "U;\n";
  ss << "}  // namespace om2::generated\n\n";
  ss << "#endif  // " << program_model.model_name << "_GENERATED_PROGRAM_ASSETS_H_\n";
  return ss.str();
}

// om2动态当前限制：以下生成头手工重写内部类型，而非稳定的模型 ABI；后续需重新设计参数描述边界。
// DynDesc、ArgsInfosDesc、RtKernelLaunchArgsEx::ComputeNodeDesc 来自
// runtime/v2/engine/aicore/launch_kernel/rt_kernel_launch_args_ex.h；DfxExeArg 来自
// runtime/v2/engine/aicore/fe_rt2_common.h；BuildTensorAttr 来自 runtime/v2/kernel/tensor_attr.h。
// 当前成员类型、顺序和布局必须与运行时一致，不能仅修改这里或原始定义中的一侧。
std::string BuildRuntimeValueTypesHeader() {
  return "#ifndef OM2_RUNTIME_VALUE_TYPES_H_\n"
         "#define OM2_RUNTIME_VALUE_TYPES_H_\n"
         R"(
#include <cstddef>
#include <cstdint>

#include "exe_graph/runtime/runtime_tensor.h"
#include "exe_graph/runtime/op_execute_context.h"
#include "graph/types.h"

namespace gert {
struct DynDesc {
  bool is_group_first{false};
  uint32_t io_group_id{0U};
  uint32_t io_index{0U};
  uint32_t dyn_num{0U};
  uint32_t group_size{0U};
  uint32_t group_offset{0U};
  uint32_t ptr_offset{0U};
};

class ArgsInfosDesc {
 public:
  struct ArgsInfo {
    enum class ArgsInfoType { INPUT = 0, OUTPUT = 1, TYPE_END };
    enum class ArgsInfoFormat { DIRECT_ADDR = 0, FOLDED_DESC_ADDR = 1, FORMAT_END };
    void Init(ArgsInfoType type, ArgsInfoFormat format, int32_t index, uint32_t size) {
      arg_type = type;
      arg_format = format;
      start_index = index;
      arg_size = size;
    }
    ArgsInfoType arg_type;
    ArgsInfoFormat arg_format;
    int32_t start_index;
    uint32_t arg_size;
    bool folded_first{false};
    bool be_folded{false};
    DynDesc dyn_desc;
    size_t data_size{0U};
  };

  void Init(size_t input_num, size_t output_num, size_t valid_input_num, size_t valid_output_num) {
    input_args_info_num_ = input_num;
    output_args_info_num_ = output_num;
    input_valid_num_ = valid_input_num;
    output_valid_num_ = valid_output_num;
    args_infos_size_ = (input_num + output_num) * sizeof(ArgsInfo);
  }
  void *MutableArgsInfoBase() {
    return args_infos_;
  }

 private:
  size_t input_args_info_num_;
  size_t output_args_info_num_;
  size_t input_valid_num_;
  size_t output_valid_num_;
  size_t args_infos_size_;
  uint8_t args_infos_[8];
};

struct RtKernelLaunchArgsEx {
  struct ComputeNodeDesc {
    size_t input_num;
    size_t output_num;
    size_t workspace_cap;
    size_t max_tiling_data;
    bool need_shape_buffer;
    bool need_overflow;
    size_t compiled_args_size;
    uint8_t compiled_args[8];
  };
};

struct DfxExeArg {
  bool need_assert{false};
  bool need_print{false};
  int64_t buffer_size{0};
};

namespace kernel {
struct BuildTensorAttr {
  TensorPlacement placement;
  ge::DataType data_type;
  StorageFormat storage_format;
};
}  // namespace kernel
}  // namespace gert

)"
         "#endif  // OM2_RUNTIME_VALUE_TYPES_H_\n";
}

// om2动态当前限制：为生成源码手工提供内部 register/kernel_registry.h 的精简定义，
// KernelRegistry/KernelRegisterV2 仍依赖运行时 C++ ABI（含虚函数顺序和 KernelFuncs 布局）。
// 后续需重新设计 kernel 查询/注册边界，避免在生成模型中维护内部类定义的副本。
std::string BuildKernelRegistryHeader() {
  return "#ifndef B369E37D560547C2B8DC137404F9713E_H\n"
         "#define B369E37D560547C2B8DC137404F9713E_H\n"
         R"(
#include <memory>
#include <string>
#include <vector>

#include "graph/error_codes.h"
#include "graph/types.h"
#include "exe_graph/runtime/base_type.h"
#include "exe_graph/runtime/kernel_context.h"

namespace ge {
class FastNode;
class Node;
}  // namespace ge

namespace gert {
class ProfilingInfoWrapper;
class DataDumpInfoWrapper;
class ExceptionDumpInfoWrapper;

class KernelRegistry {
 public:
  static KernelRegistry &GetInstance();
  static void ReplaceKernelRegistry(std::shared_ptr<KernelRegistry> registry);

  using CreateOutputsFunc = UINT32 (*)(const ge::FastNode *, KernelContext *);
  using KernelFunc = UINT32 (*)(KernelContext *context);
  using TracePrinter = std::vector<std::string> (*)(const KernelContext *);
  using ProfilingInfoFiller = ge::graphStatus (*)(const KernelContext *, ProfilingInfoWrapper &);
  using DataDumpInfoFiller = ge::graphStatus (*)(const KernelContext *, DataDumpInfoWrapper &);
  using ExceptionDumpInfoFiller = ge::graphStatus (*)(const KernelContext *, ExceptionDumpInfoWrapper &);

  struct KernelFuncs {
    KernelFunc run_func;
    CreateOutputsFunc outputs_creator;
    TracePrinter trace_printer;
    ProfilingInfoFiller profiling_info_filler;
    DataDumpInfoFiller data_dump_info_filler;
    ExceptionDumpInfoFiller exception_dump_info_filler;
  };

  struct KernelInfo {
    KernelFuncs func;
    std::string critical_section;
  };

  virtual ~KernelRegistry() = default;
  virtual const KernelFuncs *FindKernelFuncs(const std::string &kernel_type) const = 0;
  virtual const KernelInfo *FindKernelInfo(const std::string &kernel_type) const = 0;
  virtual void RegisterKernel(std::string kernel_type, KernelInfo kernel_infos) {
    (void)kernel_type;
    (void)kernel_infos;
  };
};

class KernelRegisterData;
class KernelRegisterV2 {
 public:
  explicit KernelRegisterV2(const ge::char_t *kernel_type);
  KernelRegisterV2(const KernelRegisterV2 &other);
  ~KernelRegisterV2();
  KernelRegisterV2 &operator=(const KernelRegisterV2 &other) = delete;
  KernelRegisterV2 &operator=(KernelRegisterV2 &&other) = delete;
  KernelRegisterV2(KernelRegisterV2 &&other) = delete;

  KernelRegisterV2 &RunFunc(KernelRegistry::KernelFunc func);
  KernelRegisterV2 &ConcurrentCriticalSectionKey(const std::string &critical_section_key);

  KernelRegisterV2 &OutputsCreator(KernelRegistry::CreateOutputsFunc func);
  KernelRegisterV2 &TracePrinter(KernelRegistry::TracePrinter func);
  KernelRegisterV2 &ProfilingInfoFiller(KernelRegistry::ProfilingInfoFiller func);
  KernelRegisterV2 &DataDumpInfoFiller(KernelRegistry::DataDumpInfoFiller func);
  KernelRegisterV2 &ExceptionDumpInfoFiller(KernelRegistry::ExceptionDumpInfoFiller func);

 private:
  std::unique_ptr<KernelRegisterData> register_data_;
};
}  // namespace gert

#define REGISTER_KERNEL_COUNTER2(type, counter) static auto g_register_kernel_##counter = gert::KernelRegisterV2(#type)
#define REGISTER_KERNEL_COUNTER(type, counter) REGISTER_KERNEL_COUNTER2(type, counter)
#define REGISTER_KERNEL(type) REGISTER_KERNEL_COUNTER(type, __COUNTER__)

)"
         "#endif  // B369E37D560547C2B8DC137404F9713E_H\n";
}

std::string SanitizePathComponent(const std::string &name) {
  std::string sanitized;
  sanitized.reserve(name.size());
  for (const auto c : name) {
    if (std::isalnum(static_cast<uint8_t>(c)) || (c == '_') || (c == '-') || (c == '.')) {
      sanitized.push_back(c);
    } else {
      sanitized.push_back('_');
    }
  }
  return sanitized.empty() ? "unknown" : sanitized;
}

void MakeDirIfNeeded(const std::string &path) {
  struct stat stat_buf{};
  if ((stat(path.c_str(), &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode)) {
    return;
  }
  (void)mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

ge::Status AddGeneratedArtifact(const std::string &file_name, const std::string &content,
                                std::vector<gert::GertModelDataFile> &artifacts) {
  gert::GertModelDataFile artifact;
  artifact.file_name = gert::GertMakeStr(file_name);
  GE_ASSERT_NOTNULL(artifact.file_name);
  artifact.data = gert::GertMakeFileData(content.data(), content.size());
  if (!content.empty()) {
    GE_ASSERT_NOTNULL(artifact.data);
  }
  artifact.data_size = content.size();
  artifacts.push_back(std::move(artifact));
  return ge::SUCCESS;
}

void DumpGeneratedArtifacts(const std::string &model_name, const std::vector<gert::GertModelDataFile> &artifacts) {
  // The debug dump root can be overridden via environment so multi-rank jobs
  // can direct each rank's diagnostic artifacts to an isolated directory.
  // When unset, the legacy path /tmp/.om2_tmp_workspace is used.
  const char *env_root = std::getenv("GE_OM2_DEBUG_DUMP_ROOT");
  const std::string dump_root =
      (env_root != nullptr && env_root[0] != '\0') ? std::string(env_root) : "/tmp/.om2_tmp_workspace";
  MakeDirIfNeeded("/tmp");
  MakeDirIfNeeded(dump_root);
  const std::string dump_dir = dump_root + "/" + SanitizePathComponent(model_name);
  MakeDirIfNeeded(dump_dir);
  for (const auto &artifact : artifacts) {
    const std::string file_path = dump_dir + "/" + SanitizePathComponent(gert::GertGetStr(artifact.file_name));
    std::ofstream output(file_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
      continue;
    }
    if (artifact.data_size > 0U) {
      output.write(reinterpret_cast<const char *>(artifact.data.get()),
                   static_cast<std::streamsize>(artifact.data_size));
    }
  }
}

}  // namespace

ge::Status ProgramBodyCodegen::Generate(const ProgramInfo &program_info, const ProgramBodyCodegenOption &option,
                                        ProgramBodyCodegenResult &result) const {
  result = {};
  GE_CHK_BOOL_RET_STATUS(!option.model_name.empty(), ge::FAILED, "[OM2][ProgramBody] model_name is empty");
  GE_CHK_BOOL_RET_STATUS(!option.interface_file_name.empty(), ge::FAILED, "[OM2][ProgramBody] interface file empty");
  GE_CHK_BOOL_RET_STATUS(!option.source_file_name.empty(), ge::FAILED, "[OM2][ProgramBody] source file empty");
  GE_CHK_BOOL_RET_STATUS(!option.makefile_file_name.empty(), ge::FAILED, "[OM2][ProgramBody] makefile file empty");

  const DynamicProgramModel program_model = BuildDynamicProgramModel(program_info, option);
  GE_CHK_BOOL_RET_STATUS(program_model.structured_resources_valid, ge::FAILED,
                         "[OM2][ProgramBody] failed to decode serialized model resources");
  GE_CHK_BOOL_RET_STATUS(program_model.codegen_valid, ge::FAILED,
                         "[OM2][ProgramBody] failed to generate a CodegenImpl kernel");

  AstContext ast_ctx;
  AstBuildContext ast(ast_ctx);

  std::vector<DeclNode *> interface_items = {
      ast.Include("cstddef", IncludeDecl::Kind::kAngle),
      ast.Include("cstdint", IncludeDecl::Kind::kAngle),
      ast.Include("cinttypes", IncludeDecl::Kind::kAngle),
      ast.Include("memory", IncludeDecl::Kind::kAngle),
      ast.Include("string", IncludeDecl::Kind::kAngle),
      ast.Include("vector", IncludeDecl::Kind::kAngle),
      ast.Include("dlog_pub.h", IncludeDecl::Kind::kQuote),
      ast.Include("sys/syscall.h", IncludeDecl::Kind::kAngle),
      ast.Include("unistd.h", IncludeDecl::Kind::kAngle),
      ast.Include("om2_model_api.h", IncludeDecl::Kind::kQuote),
      ast.Space(),
      ast.StablePart(ge::StablePartId::kOm2LogMacros, ge::StablePartPlacement::kTranslationUnit),
      ast.Space(),
      ast.Namespace(
          "om2",
          {ast.Struct(
               "GeneratedComputeNodeDesc",
               {ast.Field("size_t", "input_num", ast.ULong(0U)), ast.Field("size_t", "output_num", ast.ULong(0U)),
                ast.Field("size_t", "workspace_cap", ast.ULong(0U)),
                ast.Field("size_t", "max_tiling_data", ast.ULong(0U)), ast.Field("bool", "need_shape_buffer", false),
                ast.Field("bool", "need_overflow", false)}),
           ast.Struct(
               "GeneratedDynDesc",
               {ast.Field("bool", "is_group_first", false), ast.Field("uint32_t", "io_group_id", ast.UInt(0U)),
                ast.Field("uint32_t", "io_index", ast.UInt(0U)), ast.Field("uint32_t", "dyn_num", ast.UInt(0U)),
                ast.Field("uint32_t", "group_size", ast.UInt(0U)), ast.Field("uint32_t", "group_offset", ast.UInt(0U)),
                ast.Field("uint32_t", "ptr_offset", ast.UInt(0U))}),
           ast.Struct(
               "GeneratedArgsInfo",
               {ast.Field("int32_t", "arg_type", ast.UInt(0U)), ast.Field("int32_t", "arg_format", ast.UInt(0U)),
                ast.Field("int32_t", "start_index", ast.UInt(0U)), ast.Field("uint32_t", "arg_size", ast.UInt(0U)),
                ast.Field("bool", "folded_first", false), ast.Field("bool", "be_folded", false),
                ast.Field("GeneratedDynDesc", "dyn_desc", ast.InitList({}, false)),
                ast.Field("size_t", "data_size", ast.ULong(0U))}),
           ast.Struct("GeneratedArgsInfosDesc", {ast.Field("size_t", "input_args_info_num", ast.ULong(0U)),
                                                 ast.Field("size_t", "output_args_info_num", ast.ULong(0U)),
                                                 ast.Field("size_t", "input_valid_num", ast.ULong(0U)),
                                                 ast.Field("size_t", "output_valid_num", ast.ULong(0U)),
                                                 ast.Field("size_t", "args_infos_size", ast.ULong(0U))}),
           ast.Struct("GeneratedArgsIndexToIoIndex", {ast.Field("int32_t", "args_role", ast.UInt(0U)),
                                                      ast.Field("size_t", "args_index", ast.ULong(0U)),
                                                      ast.Field("size_t", "io_index", ast.ULong(0U))}),
           ast.Struct("GeneratedBuildTensorAttr",
                      {ast.Field("int32_t", "placement", ast.UInt(0U)), ast.Field("int32_t", "data_type", ast.UInt(0U)),
                       ast.Field("int32_t", "storage_format", ast.UInt(0U))}),
           ast.Struct("GeneratedDfxExeArg",
                      {ast.Field("bool", "need_assert", false), ast.Field("bool", "need_print", false),
                       ast.Field("int64_t", "buffer_size", ast.ULong(0U))}),
           ast.Struct("GeneratedBinData",
                      {ast.Field("uint32_t", "magic", ast.UInt(0U)), ast.Field("uint32_t", "version", ast.UInt(0U)),
                       ast.Field("const void *", "data", nullptr), ast.Field("uint64_t", "length", ast.ULong(0U)),
                       ast.Field("uint64_t", "placeholder", ast.ULong(0U))}),
           ast.Struct("GeneratedDim3",
                      {ast.Field("uint32_t", "x", ast.UInt(0U)), ast.Field("uint32_t", "y", ast.UInt(0U)),
                       ast.Field("uint32_t", "z", ast.UInt(0U))}),
           ast.Struct("GeneratedChainMeta",
                      {ast.Field("uint32_t", "index", ast.UInt(0U)), ast.Field("uint64_t", "symbol", ast.ULong(0U)),
                       ast.Field("uint32_t", "source_kind", ast.UInt(0U)),
                       ast.Field("uint32_t", "binding_phase", ast.UInt(0U)),
                       ast.Field("uint32_t", "binding_kind", ast.UInt(0U)),
                       ast.Field("const char *", "cpp_symbol", ast.Str(""))}),
           ast.Struct(
               "GeneratedKernelMeta",
               {ast.Field("const char *", "execution_node_name", ast.Str("")),
                ast.Field("const char *", "kernel_type", ast.Str("")),
                ast.Field("uint32_t", "input_count", ast.UInt(0U)), ast.Field("uint32_t", "output_count", ast.UInt(0U)),
                ast.Field("uint32_t", "compute_node_index", ast.UInt(0U)),
                ast.Field("uint32_t", "output_policy", ast.UInt(0U)), ast.Field("uint32_t", "impl_type", ast.UInt(0U)),
                ast.Field("bool", "require_compute_node_info", false),
                ast.Field("const char *", "cpp_symbol", ast.Str(""))}),
           ast.Struct("GeneratedProgramMetadata",
                      {ast.Field("const char *", "model_name", ast.Str(EscapeForStringLiteral(option.model_name))),
                       ast.Field("const GeneratedChainMeta *", "chains", nullptr),
                       ast.Field("size_t", "chain_count", ast.ULong(0U)),
                       ast.Field("const GeneratedKernelMeta *", "init_kernels", nullptr),
                       ast.Field("size_t", "init_kernel_count", ast.ULong(0U)),
                       ast.Field("const GeneratedKernelMeta *", "main_kernels", nullptr),
                       ast.Field("size_t", "main_kernel_count", ast.ULong(0U)),
                       ast.Field("const GeneratedKernelMeta *", "deinit_kernels", nullptr),
                       ast.Field("size_t", "deinit_kernel_count", ast.ULong(0U)),
                       ast.Field("size_t", "buffer_size", ast.ULong(program_info.resources.buffer.size())),
                       ast.Field("size_t", "compute_node_info_size",
                                 ast.ULong(program_info.resources.compute_node_infos.size())),
                       ast.Field("size_t", "kernel_extend_info_size",
                                 ast.ULong(program_info.resources.kernel_extend_infos.size())),
                       ast.Field("size_t", "model_desc_size", ast.ULong(program_info.resources.model_desc.size()))}),
           ast.Class("GeneratedComputeNodeInfo", {ast.Public(), ast.DeclareMethod("Init", {}, "aclError"),
                                                  ast.DeclareMethod("Get", {}, "const void *"), ast.Private(),
                                                  ast.Field("std::unique_ptr<uint8_t[]>", "storage_", nullptr),
                                                  ast.Field("void *", "info_", nullptr)}),
           ast.Class("GeneratedFixedAssets",
                     {ast.Public(),
                      ast.DeclareMethod("Init", {}, "aclError"),
                      ast.DeclareMethod("ComputeNodeInfo", {}, "const void *"),
                      ast.DeclareMethod("NodeDesc", {}, "GeneratedComputeNodeDesc *"),
                      ast.DeclareMethod("ArgsInfoDesc", {}, "GeneratedArgsInfosDesc *"),
                      ast.DeclareMethod("ArgsSizes", {}, "void *"),
                      ast.DeclareMethod("ArgsToIo", {}, "void *"),
                      ast.DeclareMethod("CopyFlowIndexes", {}, "void *"),
                      ast.DeclareMethod("OutputAttr", {}, "GeneratedBuildTensorAttr *"),
                      ast.DeclareMethod("TaskConfig", {}, "void *"),
                      ast.DeclareMethod("DfxArg", {}, "GeneratedDfxExeArg *"),
                      ast.Private(),
                      ast.Field("GeneratedComputeNodeInfo", "compute_node_info_"),
                      ast.Field("GeneratedComputeNodeDesc", "node_desc_"),
                      ast.Field("GeneratedArgsInfosDesc", "args_info_desc_"),
                      ast.Field("std::unique_ptr<uint8_t[]>", "args_sizes_holder_", nullptr),
                      ast.Field("std::unique_ptr<uint8_t[]>", "args_to_io_holder_", nullptr),
                      ast.Field("std::unique_ptr<uint8_t[]>", "copy_flow_indexes_holder_", nullptr),
                      ast.Field("GeneratedBuildTensorAttr", "output_attr_"),
                      ast.Field("GeneratedDfxExeArg", "dfx_arg_")})}),

  };

  CppEmitter emitter;
  std::string interface_source;
  const auto *translation_unit = ast.File(interface_items);
  GE_CHK_BOOL_RET_STATUS(translation_unit != nullptr, ge::FAILED, "[OM2][ProgramBody] interface AST is null");
  GE_CHK_STATUS(translation_unit->Accept(emitter, interface_source));

  const std::string source_source = BuildExecutableSource(program_model, option);
  GE_CHK_BOOL_RET_STATUS(!source_source.empty(), ge::FAILED,
                         "[OM2][ProgramBody] failed to expand static execution regions");

  ProgramBodyCodegenResult local_result;
  GE_ASSERT_SUCCESS(AddGeneratedArtifact(option.interface_file_name, interface_source + "\n", local_result.artifacts));
  GE_ASSERT_SUCCESS(AddGeneratedArtifact(option.source_file_name, source_source + "\n", local_result.artifacts));
  GE_ASSERT_SUCCESS(AddGeneratedArtifact("generated_program_assets.h", BuildGeneratedProgramAssetsHeader(program_model),
                                         local_result.artifacts));
  GE_ASSERT_SUCCESS(
      AddGeneratedArtifact("om2_model_api.h", std::string(ge::GetOm2ModelApiHeader()), local_result.artifacts));
  GE_ASSERT_SUCCESS(AddGeneratedArtifact("om2_model_api_utils.h", std::string(ge::GetOm2ModelApiUtilsHeader()),
                                         local_result.artifacts));
  GE_ASSERT_SUCCESS(
      AddGeneratedArtifact("om2_runtime_value_types.h", BuildRuntimeValueTypesHeader(), local_result.artifacts));
  GE_ASSERT_SUCCESS(
      AddGeneratedArtifact("register/kernel_registry.h", BuildKernelRegistryHeader(), local_result.artifacts));
  const auto aicore_binary_chains = CollectBindingChains(program_model, ChainBindingKind::kGeKernelBinary);
  if (!aicore_binary_chains.empty()) {
    GE_ASSERT_SUCCESS(AddGeneratedArtifact("graph/op_kernel_bin.h", std::string(ge::GetOm2KernelBinHeader()),
                                           local_result.artifacts));
    GE_ASSERT_SUCCESS(AddGeneratedArtifact("graph/host_resource/host_resource.h",
                                           std::string(ge::GetOm2HostResourceHeader()), local_result.artifacts));
  }
  if (option.emit_makefile) {
    const std::string makefile = R"(CANN_ROOT ?= $(ASCEND_HOME_PATH)
USE_STUB_LIB ?= 1
MAKEFLAGS += -j$(shell nproc)

ifeq ($(origin CXX),default)
CXX := c++
endif
CXX ?= c++

TARGET := lib)" + option.model_name +
                                 R"(_program_body.so
SRC_FILES := )" + option.source_file_name +
                                 R"(

ifndef CPPFLAGS
CPPFLAGS := \
  -I$(CANN_ROOT)/include \
  -I$(CANN_ROOT)/pkg_inc \
  -I$(CANN_ROOT)/pkg_inc/base \
  -I$(CANN_ROOT)/pkg_inc/runtime \
  -I$(CANN_ROOT)/pkg_inc/profiling \
  -I$(CURDIR)/include
endif

ifndef CXXFLAGS
CXXFLAGS := -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=0 -O2 -fPIC -fvisibility=hidden
endif

ifeq ($(USE_STUB_LIB),1)
LIB_PATH ?= $(CANN_ROOT)/devlib
else
LIB_PATH ?= $(CANN_ROOT)/lib64
endif

ifndef LDFLAGS
LDFLAGS := -shared -L$(LIB_PATH) -L$(CANN_ROOT)/lib64 -Wl,--no-as-needed
endif
ifndef LDLIBS
LDLIBS := -lge_runner_v2 -lgert -lregister -lopp_registry -lmetadef -lacl_rt -lc_sec -Wl,--as-needed
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC_FILES)
)"
                                 "\t"
                                 R"(@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
)"
                                 "\t"
                                 R"(@rm -f $(TARGET)
)";
    GE_ASSERT_SUCCESS(AddGeneratedArtifact(option.makefile_file_name, makefile + "\n", local_result.artifacts));
  }
  DumpGeneratedArtifacts(option.model_name, local_result.artifacts);
  result = std::move(local_result);
  return ge::SUCCESS;
}
}  // namespace om2
}  // namespace gert
