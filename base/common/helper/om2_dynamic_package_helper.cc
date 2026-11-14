/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/common/helper/om2_package_helper.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "common/checker.h"
#include "common/ge_common/ge_types.h"
#include "common/ge_common/string_util.h"
#include "common/ge_common/util.h"
#include "common/helper/model_save_utils.h"
#include "common/helper/om2/om2_utils.h"
#include "common/om2/om2_online_prepare_data.h"
#include "common/om2/dynamic_codegen/execute_graph_dependencies.h"
#include "common/om2/dynamic_codegen/info_collect/program_info_collector.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/gert_metadata_preparer.h"
#include "common/om2/dynamic_codegen/prepare/gert_resources.h"
#include "common/om2/dynamic_codegen/program_body_codegen.h"
#include "common/op_so_store/op_so_store_utils.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/common/om2_feature_list.h"
#include "framework/omg/omg_inner_types.h"
#include "framework/om2/model_data/om2_package_contants.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_dump_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/tensor_utils.h"
#include "graph_metadef/common/plugin/plugin_manager.h"
#include "lowering/execute_graph_preparer.h"
#include "lowering/model_converter.h"
#include "stage/stage_plan.h"
#include "stage/stage_splitter.h"

namespace ge {
namespace {
struct GraphPostProcessResult {
  gert::GertModelDescInfo model_info;
  gert::GertResourceTable resources;
  gert::GertResourceImages images;
};

Status CopyResource(const std::string &name, const std::vector<uint8_t> &bytes, gert::GertModelDataFile &file) {
  file.file_name = gert::GertMakeStr(name);
  GE_ASSERT_NOTNULL(file.file_name);
  file.data_size = bytes.size();
  file.data = gert::GertMakeFileData(bytes.data(), bytes.size());
  GE_ASSERT_TRUE(bytes.empty() || file.data != nullptr);
  return SUCCESS;
}

Status BuildGraphIo(const std::vector<gert::ModelIoEntry> &entries, std::vector<gert::GertTensorDesc> &descs,
                    std::vector<gert::GertTensorDesc> &descs_v2) {
  for (const auto &entry : entries) {
    auto desc = gert::MakeGertTensorDesc(entry.name, entry.desc.GetDataType(), entry.desc.GetFormat(),
                                         entry.desc.GetShape().GetDims());
    int64_t size = 0;
    const auto &dims = entry.desc.GetShape().GetDims();
    const bool has_dynamic_dim = std::any_of(dims.cbegin(), dims.cend(), [](const int64_t dim) { return dim < 0; });
    if (!has_dynamic_dim) {
      GE_ASSERT_SUCCESS(TensorUtils::GetTensorSizeInBytes(entry.desc, size));
    }
    desc.size = size > 0 ? static_cast<uint64_t>(size) : 0U;
    (void)entry.desc.GetShapeRange(desc.shape_range);
    // V2 是既有 ACL 查询描述，不借它承载另一套 origin/storage 语义。
    descs_v2.emplace_back(gert::MakeGertTensorDesc(desc));
    descs.emplace_back(std::move(desc));
  }
  return SUCCESS;
}

Status ValidateNodeSoReferences(const FastNode &node, const gert::GertResourceImages &images) {
  std::vector<std::string> so_ids;
  GE_ASSERT_SUCCESS(gert::GetNodeSoIds(node, so_ids));
  for (const auto &id : so_ids) {
    GE_ASSERT_TRUE(images.so_ids.count(id) != 0U, "Unknown SO resource %s", id.c_str());
  }
  return SUCCESS;
}

Status ValidateNodeBinaryReference(const FastNode &node, const gert::GertResourceImages &images) {
  std::string binary_id;
  GE_ASSERT_SUCCESS(gert::GetNodeBinaryId(node, binary_id));
  GE_ASSERT_TRUE(binary_id.empty() || images.binary_ids.count(binary_id) != 0U, "Unknown binary resource %s",
                 binary_id.c_str());
  return SUCCESS;
}

// 资源 ID 只在接入时使用，Const 的最终权重编号和偏移使用既有 codegen 协议。
Status BindNodeWeightReference(FastNode &node, const gert::GertResourceImages &images) {
  gert::GertNodeWeightRef ref;
  GE_ASSERT_SUCCESS(gert::GetNodeWeightRef(node, ref));
  if (ref.id.empty()) {
    return SUCCESS;
  }
  GE_ASSERT_TRUE(node.GetDataOutNum() == 1U && std::string(node.GetTypePtr()) == "Const",
                 "Weight reference requires a single-output Tensor Const: %s", node.GetNamePtr());
  gert::GertWeightSlice slice;
  GE_ASSERT_SUCCESS(gert::ResolveWeightSlice(ref, images.weights, slice));
  auto desc = node.GetOpDescPtr();
  auto output_desc = desc->GetOutputDesc(0U);
  GE_ASSERT_TRUE(AttrUtils::SetInt(output_desc, "om2_weight_index", slice.weight_index));
  TensorUtils::SetDataOffset(output_desc, slice.offset);
  GE_ASSERT_TRUE(AttrUtils::SetListInt(output_desc, ATTR_NAME_GRAPH_FLATTEN_OFFSET, {slice.offset, slice.length}));
  GE_ASSERT_SUCCESS(desc->UpdateOutputDesc(0U, output_desc));
  ConstGeTensorPtr tensor;
  if (AttrUtils::GetTensor(desc, "value", tensor) && tensor != nullptr) {
    auto copy = std::make_shared<GeTensor>(*tensor);
    GE_ASSERT_NOTNULL(copy);
    auto &tensor_desc = copy->MutableTensorDesc();
    GE_ASSERT_TRUE(AttrUtils::SetInt(tensor_desc, "om2_weight_index", slice.weight_index));
    TensorUtils::SetDataOffset(tensor_desc, slice.offset);
    GE_ASSERT_TRUE(AttrUtils::SetListInt(tensor_desc, ATTR_NAME_GRAPH_FLATTEN_OFFSET, {slice.offset, slice.length}));
    GE_ASSERT_TRUE(AttrUtils::SetTensor(desc, "value", copy));
  }
  return SUCCESS;
}

Status BindGraphResources(ExecuteGraph &graph, const gert::GertResourceImages &images) {
  for (auto *node : graph.GetAllNodes()) {
    GE_ASSERT_NOTNULL(node);
    // 校验当前节点的 SO 资源引用。
    GE_ASSERT_SUCCESS(ValidateNodeSoReferences(*node, images));
    // 校验当前节点的设备 Kernel 二进制引用。
    GE_ASSERT_SUCCESS(ValidateNodeBinaryReference(*node, images));
    // 解析权重切片并回写当前节点的权重编号及偏移。
    GE_ASSERT_SUCCESS(BindNodeWeightReference(*node, images));
  }
  return SUCCESS;
}

Status BuildGraphConstants(const gert::GertResourceImages &images, gert::GertModelData &built) {
  auto &unit = *built.models[0];
  unit.constants_config = std::make_unique<gert::GertModelDataConstantsConfig>();
  auto &config = *unit.constants_config;
  // om2动态当前限制：临时方案，权重处理方案需要后续单独考虑。动态权重逐块保存为 Host INTERNAL const。
  for (size_t i = 0U; i < images.weights.entries.size(); ++i) {
    const auto &weight = images.weights.entries[i];
    GE_ASSERT_TRUE(!weight.device_placement && weight.size >= 0 &&
                   static_cast<uint64_t>(weight.size) == weight.data.size());
    const std::string name = "constant_" + std::to_string(i);
    auto file = std::make_unique<gert::GertModelDataFile>();
    GE_ASSERT_SUCCESS(CopyResource(name, weight.data, *file));
    built.constants->constants_data.emplace_back(std::move(file));
    auto meta = std::make_unique<gert::GertModelDataConstMeta>();
    meta->index = i;
    meta->type = gert::GertMakeStr("INTERNAL");
    meta->file_name = gert::GertMakeStr(name);
    meta->op_name = gert::GertMakeStr(weight.id);
    meta->placement = gert::GertModelDataConstPlacement::kHost;
    meta->size = weight.size;
    GE_ASSERT_TRUE(static_cast<uint64_t>(weight.size) <=
                   std::numeric_limits<uint64_t>::max() - config.internal_weight_size);
    config.internal_weight_size += static_cast<uint64_t>(weight.size);
    config.consts.emplace_back(std::move(meta));
  }
  return SUCCESS;
}

Status BuildGraphLibraries(const gert::GertResourceTable &table, const gert::GertResourceImages &images,
                           gert::GertModelData &built) {
  // 保留调用方声明顺序；需要前置的依赖必须在表中先声明，避免悄悄改变加载顺序。
  std::set<std::string> loaded_ids;
  std::set<std::string> loaded_names;
  for (const auto &so : table.sos) {
    for (const auto &dependency : so.dependencies) {
      GE_ASSERT_TRUE(loaded_ids.count(dependency) != 0U, "SO %s dependency %s must precede it", so.id.c_str(),
                     dependency.c_str());
    }
    loaded_ids.insert(so.id);
    if (!loaded_names.insert(so.name).second) {
      continue;
    }
    if (so.purpose == gert::GertSoPurpose::kCustom) {
      auto file = std::make_unique<gert::GertModelDataFile>();
      GE_ASSERT_SUCCESS(CopyResource(so.name, so.content, *file));
      built.custom_ops->libraries.emplace_back(std::move(file));
      continue;
    }
    // 当前动态 pbody 仅消费 SpaceRegistry；不能把普通 kernel/tiling SO 冒充另一种 SoBinType。
    GE_ASSERT_TRUE(so.purpose == gert::GertSoPurpose::kSpaceRegistry, "Unsupported shared library purpose for %s",
                   so.name.c_str());
    auto bin = std::make_unique<gert::GertModelDataSoBin>();
    bin->binary = std::make_unique<gert::GertModelDataFile>();
    GE_ASSERT_SUCCESS(CopyResource(so.name, so.content, *bin->binary));
    bin->so_name = gert::GertMakeStr(so.name);
    bin->vendor_name = gert::GertMakeStr(so.vendor);
    bin->so_bin_type = static_cast<uint64_t>(SoBinType::kSpaceRegistry);
    built.so_bins->libraries.emplace_back(std::move(bin));
  }
  for (const auto &binary : images.binaries) {
    auto file = std::make_unique<gert::GertModelDataFile>();
    GE_ASSERT_SUCCESS(CopyResource(binary.name, binary.content, *file));
    built.kernels->binaries.emplace_back(std::move(file));
  }
  return SUCCESS;
}

Status ParseGraphModelInfo(const ExecuteGraph &source_graph, const std::vector<std::pair<FastNode *, int32_t>> &outputs,
                           gert::GertModelDescInfo &model_info) {
  GE_ASSERT_SUCCESS(gert::ParseGertModelDesc(source_graph, model_info));
  GE_ASSERT_TRUE(outputs.size() == model_info.outputs.size(), "Model output descriptions do not match outputs");
  return SUCCESS;
}

Status ValidateGraphResources(const gert::GertResourceTable &resources) {
  for (const auto &weight : resources.weights) {
    GE_ASSERT_TRUE(weight.mode == gert::GertWeightMode::kEmbedded && !weight.device_placement,
                   "Dynamic graph currently requires embedded Host weights");
  }
  return SUCCESS;
}

Status PrepareGraphResources(ExecuteGraph &graph, gert::GertResourceTable &resources,
                             gert::GertResourceImages &images) {
  // 解析传入资源描述及内容。
  GE_ASSERT_SUCCESS(gert::ParseGertResources(graph, resources));
  // 校验当前支持的资源使用方式。
  GE_ASSERT_SUCCESS(ValidateGraphResources(resources));
  // 生成资源镜像及内部编号。
  GE_ASSERT_SUCCESS(gert::BuildGertResourceImages(resources, images));
  // 校验最终节点的资源引用并绑定权重切片。
  GE_ASSERT_SUCCESS(BindGraphResources(graph, images));
  return SUCCESS;
}

Status ValidateStageGraphOutputs(const ExecuteGraph &graph,
                                 const std::vector<std::pair<FastNode *, int32_t>> &outputs) {
  std::set<std::string> names;
  for (auto *node : graph.GetDirectNode()) {
    GE_ASSERT_NOTNULL(node);
    GE_ASSERT_TRUE(names.insert(node->GetName()).second);
  }
  for (const auto &output : outputs) {
    GE_ASSERT_NOTNULL(output.first);
    GE_ASSERT_TRUE(graph.CheckNodeIsInGraph(output.first));
  }
  return SUCCESS;
}

Status ValidateStageInputBindings(const StageSplitter &splitter, const gert::GertModelDescInfo &model_info) {
  GE_ASSERT_TRUE(splitter.GetInputBindings(StageKind::kInit, StageInputClass::kLoad).empty(),
                 "User load-time inputs require a load binding contract");
  GE_ASSERT_TRUE(
      splitter.GetInputBindings(StageKind::kMain, StageInputClass::kExecute).size() == model_info.inputs.size(),
      "Model input descriptions do not match execution inputs");
  return SUCCESS;
}

Status SplitGraphStages(ExecuteGraph &graph, const std::vector<std::pair<FastNode *, int32_t>> &outputs,
                        const gert::GertModelDescInfo &model_info) {
  // 分析节点阶段。
  StagePlan plan;
  GE_ASSERT_SUCCESS(StageAnalyzer(graph).Analyze(plan));
  // 校验节点名称及输出归属。
  GE_ASSERT_SUCCESS(ValidateStageGraphOutputs(graph, outputs));
  // 拆分阶段、桥接跨阶段值并登记收尾目标，内部校验拆分结果。
  StageSplitter splitter(graph);
  GE_ASSERT_SUCCESS(splitter.Split(plan, outputs));
  // 校验加载输入限制及执行输入数量。
  GE_ASSERT_SUCCESS(ValidateStageInputBindings(splitter, model_info));
  return SUCCESS;
}

Status ReorderGraphNodes(ExecuteGraph &graph) {
  for (auto *subgraph : graph.GetAllSubgraphs()) {
    GE_ASSERT_NOTNULL(subgraph);
    subgraph->ReorderByNodeId();
  }
  graph.ReorderByNodeId();
  return SUCCESS;
}

Status ReadComputeNodeTopoIds(const ExecuteGraph &graph, std::vector<int64_t> &topo_ids) {
  if (!graph.HasAttr(gert::attr::kComputeNodeTopoIds)) {
    return SUCCESS;
  }
  GE_ASSERT_TRUE(AttrUtils::GetListInt(graph, gert::attr::kComputeNodeTopoIds, topo_ids),
                 "Attribute %s must be a list of logical compute-node topo IDs", gert::attr::kComputeNodeTopoIds);
  GE_ASSERT_TRUE(!topo_ids.empty(), "Attribute %s must not be empty", gert::attr::kComputeNodeTopoIds);
  for (const auto id : topo_ids) {
    // NodePriorityCalculator computes id * 10 + 9 using int64_t.
    GE_ASSERT_TRUE(id >= 0 && id <= (std::numeric_limits<int64_t>::max() - 9) / 10,
                   "Invalid logical compute-node topo ID %" PRId64, id);
  }
  for (const auto *node : graph.GetAllNodes()) {
    GE_ASSERT_NOTNULL(node);
    const auto *desc = node->GetOpDescBarePtr();
    GE_ASSERT_NOTNULL(desc);
    int64_t index = 0;
    if (AttrUtils::GetInt(desc, gert::kComputeNodeIndex, index)) {
      GE_ASSERT_TRUE(index >= 0 && static_cast<size_t>(index) < topo_ids.size(),
                     "Node %s compute node index %" PRId64 " exceeds explicit topo mapping size %zu",
                     node->GetNamePtr(), index, topo_ids.size());
    }
  }
  GELOGI("Using %zu explicit compute-node topo IDs for execute graph %s", topo_ids.size(), graph.GetName().c_str());
  return SUCCESS;
}

Status PrepareExecuteGraph(ExecuteGraph &graph) {
  // 执行优化、拓扑排序及优先级计算。
  gert::ExecuteGraphPrepareInput input;
  gert::ExecuteGraphPrepareResult prepared;
  GE_ASSERT_SUCCESS(ReadComputeNodeTopoIds(graph, input.indexes_to_compute_node_id));
  GE_ASSERT_SUCCESS(gert::PrepareExecuteGraph(graph, gert::LoweringOption{}, input, prepared));
  // 恢复执行依赖，并在补边后重新排序。
  GE_ASSERT_SUCCESS(gert::om2::RestoreLaunchFreeDependencies(graph));
  // 按最终节点 ID 整理节点顺序。
  GE_ASSERT_SUCCESS(ReorderGraphNodes(graph));
  return SUCCESS;
}

Status PostProcessGraph(const std::shared_ptr<ExecuteGraph> &source_graph, GraphPostProcessResult &result) {
  GE_ASSERT_NOTNULL(source_graph);
  DumpGraph(source_graph.get(), "ExecuteGraphBeforeStageSplit");
  const auto outputs = ExecuteGraphUtils::GetGraphOutputs(source_graph.get());
  // 1. 解析模型 IO 描述，供拆图后的输入数量校验使用。
  GE_ASSERT_SUCCESS(ParseGraphModelInfo(*source_graph, outputs, result.model_info));
  GE_ASSERT_TRUE(source_graph->GetParentNodeBarePtr() == nullptr, "Stage split requires a root execution graph");

  // 2. 三阶段拆分，包含阶段分析与拆分结果校验。
  GE_ASSERT_SUCCESS(SplitGraphStages(*source_graph, outputs, result.model_info));
  DumpGraph(source_graph.get(), "ExecuteGraphAfterStageSplit");

  // 3. 执行图准备，包含优化、依赖整理、排序与优先级计算。
  GE_ASSERT_SUCCESS(PrepareExecuteGraph(*source_graph));

  // 4. 在最终图上解析并绑定传入资源，不再修改图拓扑。
  GE_ASSERT_SUCCESS(PrepareGraphResources(*source_graph, result.resources, result.images));

  // 5. 准备执行图元信息。
  GE_ASSERT_SUCCESS(gert::PrepareExecuteGraphMetadata(*source_graph));
  return SUCCESS;
}

// 外部构图从执行图属性解析结果中获取 IO 和资源，目前保留独立的模型数据装配流程。
// 与 GE 路线 PrepareDynamicModelData 的归一待资源及元信息的统一表示方案明确后处理。
Status PrepareGraphModelData(const std::string &model_name, const GraphPostProcessResult &prepared,
                             gert::GertModelData &model_data) {
  model_data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  // 外部构图不生成 visual json：不挂空的 debug 单元，公共序列化器按 debug 是否为空决定是否
  // 写入该条目，避免用空内容触发归档写入失败。GE 路线仍由 BuildDebugInfo 填入真实内容。
  model_data.models[0]->debug = nullptr;
  // 填写模型名称及输入输出描述。
  auto &meta = *model_data.models[0]->model_meta;
  meta.model_name = gert::GertMakeStr(model_name);
  GE_ASSERT_SUCCESS(BuildGraphIo(prepared.model_info.inputs, meta.input_desc, meta.input_desc_v2));
  GE_ASSERT_SUCCESS(BuildGraphIo(prepared.model_info.outputs, meta.output_desc, meta.output_desc_v2));
  // 装配常量数据及描述。
  GE_ASSERT_SUCCESS(BuildGraphConstants(prepared.images, model_data));
  // 装配共享库和设备 Kernel 二进制。
  GE_ASSERT_SUCCESS(BuildGraphLibraries(prepared.resources, prepared.images, model_data));
  return SUCCESS;
}

Status GenerateDynamicProgramBody(const gert::om2::ProgramInfo &program_info, const std::string &model_name,
                                  gert::om2::ProgramBodyCodegenResult &result) {
  gert::om2::ProgramBodyCodegenOption option;
  option.model_name = model_name;
  option.interface_file_name = model_name + "_interface.h";
  option.source_file_name = model_name + "_program_body.cpp";
  option.makefile_file_name = "Makefile";
  constexpr int32_t kMinMultiThreadCount = 2;
  const char *const thread_num_env = std::getenv("MAX_RUNTIME_CORE_NUMBER");
  if (thread_num_env != nullptr) {
    int32_t thread_num = 0;
    GE_ASSERT_SUCCESS(ConvertToInt32(std::string(thread_num_env), thread_num),
                      "Invalid MAX_RUNTIME_CORE_NUMBER value: %s", thread_num_env);
    if (thread_num >= kMinMultiThreadCount) {
      option.thread_num = static_cast<size_t>(thread_num);
    }
  }
  return gert::om2::ProgramBodyCodegen().Generate(program_info, option, result);
}

Status BuildDynamicSingleOutputDesc(const OpDescPtr &op_desc, const std::vector<std::string> &out_node_name,
                                    const size_t i, const std::string &src_name, const int64_t src_index,
                                    gert::GertModelDataModelMeta &model_meta) {
  const auto out_size = op_desc->GetInputsSize();
  std::string output_name;
  if (out_size == out_node_name.size()) {
    const bool contains_colon = out_node_name[i].find(':') != std::string::npos;
    output_name = contains_colon ? out_node_name[i] : (out_node_name[i] + ":" + std::to_string(src_index));
  } else {
    output_name = std::string("output_") + std::to_string(i) + "_" + src_name + "_" + std::to_string(src_index);
  }

  const auto &tensor_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
  GE_ASSERT_NOTNULL(tensor_desc);

  gert::GertTensorDesc desc = gert::MakeGertTensorDesc(output_name, tensor_desc->GetDataType(),
                                                       tensor_desc->GetFormat(), tensor_desc->GetShape().GetDims());

  int64_t tensor_size = 0;
  if (AttrUtils::GetInt(tensor_desc, ATTR_NAME_SPECIAL_OUTPUT_SIZE, tensor_size) && (tensor_size > 0)) {
    desc.size = static_cast<size_t>(tensor_size);
  } else {
    (void)TensorUtils::GetTensorSizeInBytes(*tensor_desc, tensor_size);
    desc.size = static_cast<size_t>(tensor_size);
  }

  std::vector<std::pair<int64_t, int64_t>> range;
  if (tensor_desc->GetShapeRange(range) == SUCCESS) {
    desc.shape_range = range;
  }

  gert::GertTensorDesc desc_v2 = gert::MakeGertTensorDesc(desc);
  model_meta.output_desc.push_back(std::move(desc));
  model_meta.output_desc_v2.push_back(std::move(desc_v2));
  return SUCCESS;
}
}  // namespace

Status Om2PackageHelper::BuildDynamicOutputDescs(const GeModelPtr &ge_model, const std::vector<OpDescPtr> &output_ops,
                                                 gert::GertModelDataModelMeta &model_meta) {
  std::vector<std::string> out_node_name;
  (void)AttrUtils::GetListStr(ge_model, ATTR_MODEL_OUT_NODES_NAME, out_node_name);

  for (const auto &op_desc : output_ops) {
    const auto out_size = op_desc->GetInputsSize();
    auto src_name = op_desc->GetSrcName();
    auto src_index = op_desc->GetSrcIndex();
    if (src_name.size() < out_size || src_index.size() < out_size) {
      // 未构建根 GeModel 时不会执行 ModelBuilder::AddNodeInputProperty，直接读取根图连边。
      const auto node = ge_model->GetGraph()->FindNode(op_desc->GetName());
      GE_ASSERT_NOTNULL(node);
      src_name.clear();
      src_index.clear();
      for (size_t i = 0U; i < out_size; ++i) {
        const auto anchor = node->GetInDataAnchor(static_cast<int32_t>(i));
        GE_ASSERT_NOTNULL(anchor);
        const auto peer = anchor->GetPeerOutAnchor();
        GE_ASSERT_NOTNULL(peer);
        src_name.emplace_back(peer->GetOwnerNodeBarePtr()->GetNamePtr());
        src_index.emplace_back(peer->GetIdx());
      }
    }

    for (size_t i = 0UL; i < out_size; ++i) {
      GE_CHK_STATUS_RET_NOLOG(
          BuildDynamicSingleOutputDesc(op_desc, out_node_name, i, src_name[i], src_index[i], model_meta));
    }

    std::vector<std::string> shape_info;
    if (AttrUtils::GetListStr(op_desc, ATTR_NAME_DYNAMIC_OUTPUT_DIMS, shape_info)) {
      for (const auto &s : shape_info) {
        model_meta.dynamic_output_shape.emplace_back(gert::GertMakeStr(s));
      }
    }
  }
  return SUCCESS;
}

Status Om2PackageHelper::BuildManifestCommonFields(gert::GertModelData &model_data) {
  auto &manifest = *model_data.manifest;
  manifest.compatibility.compiler_version = gert::GertMakeStr(gert::GERT_EXECUTOR_VERSION);
  manifest.model_num = static_cast<uint64_t>(model_data.models.size());
  manifest.atc_command = gert::GertMakeStr(domi::GetContext().atc_cmdline);
  return SUCCESS;
}

Status Om2PackageHelper::BuildDynamicManifest(gert::GertModelData &model_data, const std::vector<GeModelPtr> &models) {
  GE_ASSERT_SUCCESS(BuildManifestCommonFields(model_data));
  auto &manifest = *model_data.manifest;
  uint32_t max_version = 0U;
  for (const auto &model : models) {
    if (model == nullptr) {
      continue;
    }
    // 探测时保留完整模型及资源，仅替换本轮写入的 manifest；检查返回值前恢复原 manifest。
    auto model_manifest = std::make_unique<gert::GertModelDataManifest>();
    model_data.manifest.swap(model_manifest);
    const auto ret = BuildManifest(model_data, model);
    model_data.manifest.swap(model_manifest);
    GE_ASSERT_SUCCESS(ret);
    for (const auto &feature : model_manifest->compatibility.used_features) {
      (void)manifest.compatibility.used_features.emplace(gert::GertMakeStr(feature.first),
                                                         gert::GertMakeStr(feature.second));
    }
    const uint32_t version =
        gert::ParseVersion(gert::GertGetStr(model_manifest->compatibility.required_executor_version));
    if (version > max_version) {
      max_version = version;
      manifest.compatibility.required_executor_version =
          gert::GertMakeStr(model_manifest->compatibility.required_executor_version);
    }
  }
  return SUCCESS;
}

Status Om2PackageHelper::SerializeModelData(const gert::GertModelData &model_data, const std::string &model_name,
                                            const std::string &output_file, ModelBufferData &model) const {
  const std::string writer_path = (!is_offline_ && !model_name.empty()) ? model_name : output_file;
  return gert::SerializeGertModelData(model_data, model, is_offline_, writer_path);
}

Status Om2PackageHelper::CollectSoBinaries(const GeRootModelPtr &ge_root_model, const GeModelPtr &root_model,
                                           std::vector<OpSoBinPtr> &so_bins) {
  GE_ASSERT_NOTNULL(ge_root_model);
  auto collected_bins = ge_root_model->GetAllSoBin();
  if (OpSoStoreUtils::IsSoBinType(ge_root_model->GetSoInOmFlag(), SoBinType::kSpaceRegistry)) {
    GE_ASSERT_NOTNULL(root_model);
    std::string host_env_os;
    std::string host_env_cpu;
    (void)AttrUtils::GetStr(*root_model, ATTR_MODEL_HOST_ENV_OS, host_env_os);
    (void)AttrUtils::GetStr(*root_model, ATTR_MODEL_HOST_ENV_CPU, host_env_cpu);
    if (host_env_os.empty() || host_env_cpu.empty()) {
      PluginManager::GetCurEnvPackageOsAndCpuType(host_env_os, host_env_cpu);
    }
    GE_ASSERT_TRUE(!host_env_os.empty() && !host_env_cpu.empty(), "[OM2] Target host environment is empty");
    OpSoStore registry_store;
    std::unordered_set<std::string> compiler_versions;
    GE_ASSERT_SUCCESS(CollectSpaceRegistrySoBins(host_env_cpu, host_env_os, registry_store, compiler_versions));
    const auto registry_bins = registry_store.GetSoBin();
    collected_bins.insert(collected_bins.end(), registry_bins.begin(), registry_bins.end());
  }
  so_bins = std::move(collected_bins);
  return SUCCESS;
}

Status Om2PackageHelper::BuildSoBinaries(const std::vector<OpSoBinPtr> &so_bins, gert::GertModelDataSoBins &result) {
  gert::GertModelDataSoBins collected;
  std::unordered_set<std::string> used_file_names{
      std::string(gert::OM2_SO_BINS_JSON_PATH).substr(std::strlen(gert::OM2_SO_BINS_DIR))};
  for (const auto &so_bin : so_bins) {
    GE_ASSERT_NOTNULL(so_bin);
    const auto &so_name = so_bin->GetSoName();
    const auto &vendor_name = so_bin->GetVendorName();
    const auto so_bin_type = static_cast<uint64_t>(so_bin->GetSoBinType());
    const auto *const data = so_bin->GetBinData();
    const size_t data_size = so_bin->GetBinDataSize();
    GE_ASSERT_TRUE(!so_name.empty() && so_name != "." && so_name.find_first_of("/\\") == std::string::npos &&
                       so_name.find("..") == std::string::npos,
                   "[OM2] Invalid SO name: %s", so_name.c_str());
    GE_ASSERT_TRUE(data != nullptr && data_size > 0U, "[OM2] Empty SO binary: %s", so_name.c_str());

    bool duplicate = false;
    for (const auto &existing : collected.libraries) {
      if (so_name == gert::GertGetStr(existing->so_name) && vendor_name == gert::GertGetStr(existing->vendor_name) &&
          so_bin_type == existing->so_bin_type && data_size == existing->binary->data_size &&
          std::memcmp(data, existing->binary->data.get(), data_size) == 0) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    std::string file_name = so_name;
    const bool has_so_suffix = so_name.size() >= 3U && so_name.compare(so_name.size() - 3U, 3U, ".so") == 0;
    const std::string stem = has_so_suffix ? so_name.substr(0U, so_name.size() - 3U) : so_name;
    size_t rename_index = 0U;
    while (!used_file_names.insert(file_name).second) {
      file_name = stem + "_" + std::to_string(++rename_index) + (has_so_suffix ? ".so" : "");
    }
    // 归档名仅用于避免文件覆盖；加载时所需的原始 SO 名称单独保留。
    auto library = std::make_unique<gert::GertModelDataSoBin>();
    library->so_name = gert::GertMakeStr(so_name);
    library->vendor_name = gert::GertMakeStr(vendor_name);
    library->so_bin_type = so_bin_type;
    GE_ASSERT_NOTNULL(library->so_name);
    if (!vendor_name.empty()) {
      GE_ASSERT_NOTNULL(library->vendor_name);
    }
    library->binary = std::make_unique<gert::GertModelDataFile>();
    library->binary->file_name = gert::GertMakeStr(file_name);
    library->binary->data = gert::GertMakeFileData(data, data_size);
    library->binary->data_size = data_size;
    GE_ASSERT_NOTNULL(library->binary->file_name);
    GE_ASSERT_NOTNULL(library->binary->data);
    collected.libraries.emplace_back(std::move(library));
  }
  result.libraries = std::move(collected.libraries);
  return SUCCESS;
}

Status Om2PackageHelper::BuildDynamicConstantsData(const std::vector<GeModelPtr> &models,
                                                   gert::GertModelDataModel &unit,
                                                   gert::GertModelDataConstants &constants) {
  auto config = std::make_unique<gert::GertModelDataConstantsConfig>();
  std::vector<std::unique_ptr<gert::GertModelDataFile>> weight_files;
  // om2动态当前限制：统一动静态常量描述及权重打包；当前保留动态 lowering 编号和 Host 权重块处理。
  // 动态 shape 场景使用 const 描述 GeModel 粒度的权重块。
  // 编号由 lowering 写入 GeModel，与固化到 pbody 的 Const 引用共用，打包不重新分配。
  for (const auto &ge_model : models) {
    GE_ASSERT_NOTNULL(ge_model);
    const size_t weight_size = ge_model->GetWeightSize();
    if (weight_size == 0U) {
      continue;
    }
    int64_t weight_index = -1;
    GE_ASSERT_TRUE(AttrUtils::GetInt(ge_model, "om2_weight_index", weight_index) && (weight_index >= 0),
                   "[OM2] Missing or invalid lowering weight index: %s", ge_model->GetName().c_str());
    const size_t current_index = static_cast<size_t>(weight_index);
    GE_ASSERT_TRUE(weight_size <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    GE_ASSERT_TRUE(weight_size <= std::numeric_limits<uint64_t>::max() - config->internal_weight_size);
    GE_ASSERT_NOTNULL(ge_model->GetWeightData(), "[OM2] Weight data is null: %s", ge_model->GetName().c_str());
    const std::string file_name = gert::FormatOm2Path("%s%zu", gert::OM2_CONSTANTS_FILE_PREFIX, current_index);
    auto weight_file = std::make_unique<gert::GertModelDataFile>();
    weight_file->file_name = gert::GertMakeStr(file_name);
    GE_ASSERT_NOTNULL(weight_file->file_name);
    weight_file->data = gert::GertMakeFileData(ge_model->GetWeightData(), weight_size);
    GE_ASSERT_NOTNULL(weight_file->data);
    weight_file->data_size = weight_size;

    auto meta = std::make_unique<gert::GertModelDataConstMeta>();
    meta->index = current_index;
    meta->type = gert::GertMakeStr("INTERNAL");
    meta->placement = gert::GertModelDataConstPlacement::kHost;
    meta->file_name = gert::GertMakeStr(file_name);
    meta->offset = 0;
    meta->size = static_cast<int64_t>(weight_size);
    GE_ASSERT_NOTNULL(meta->type);
    GE_ASSERT_NOTNULL(meta->file_name);
    config->internal_weight_size += weight_size;
    config->consts.emplace_back(std::move(meta));
    weight_files.emplace_back(std::move(weight_file));
  }
  unit.constants_config = std::move(config);
  constants.constants_data = std::move(weight_files);
  return SUCCESS;
}

Status Om2PackageHelper::BuildCustomOpResources(const GeRootModelPtr &ge_root_model, gert::GertModelData &model_data) {
  GE_ASSERT_SUCCESS(BuildCustomKernelBinaries(ge_root_model, model_data));
  GE_ASSERT_SUCCESS(BuildCustomSharedLibs(ge_root_model, model_data));
  return SUCCESS;
}

Status Om2PackageHelper::GetDynamicGeModel(const GeRootModelPtr &ge_root_model, GeModelPtr &ge_model) {
  GE_ASSERT_NOTNULL(ge_root_model);
  const auto &root_graph = ge_root_model->GetRootGraph();
  GE_ASSERT_NOTNULL(root_graph);
  const auto &models = ge_root_model->GetSubgraphInstanceNameToModel();
  const auto iter = models.find(root_graph->GetName());
  if (iter != models.end()) {
    GE_ASSERT_NOTNULL(iter->second);
    GE_ASSERT_NOTNULL(iter->second->GetGraph());
  }
  GeModelPtr first_ge_model;
  GE_ASSERT_SUCCESS(InitFirstGeModel(ge_root_model, models, first_ge_model));
  if (iter != models.end()) {
    ge_model = std::move(first_ge_model);
    return SUCCESS;
  }

  // 展开后的动态图可以只有子图 GeModel。此对象仅复用打包元数据逻辑，不加入编译结果表，
  // 不承载子模型的 task、权重或 workspace；整图 IO 和变量仍从根 ComputeGraph 读取。
  first_ge_model->SetName(root_graph->GetName());
  int64_t notify_num = 0;
  (void)AttrUtils::GetInt(root_graph, ATTR_MODEL_NOTIFY_NUM, notify_num);
  GE_ASSERT_TRUE(AttrUtils::SetInt(first_ge_model, ATTR_MODEL_NOTIFY_NUM, notify_num));
  // 与 RT2 CreateModelDesc 一致，整图输出名称可保存在子模型上；仅取整图级属性。
  for (const auto &name : {ATTR_MODEL_OUT_NODES_NAME, ATTR_MODEL_HOST_ENV_OS, ATTR_MODEL_HOST_ENV_CPU}) {
    GeAttrValue value;
    if (root_graph->GetAttr(name, value) != GRAPH_SUCCESS) {
      for (const auto &entry : models) {
        GE_ASSERT_NOTNULL(entry.second);
        if (entry.second->GetAttr(name, value) == GRAPH_SUCCESS) {
          break;
        }
      }
    }
    if (!value.IsEmpty()) {
      GE_ASSERT_SUCCESS(first_ge_model->SetAttr(name, value));
    }
  }
  ge_model = std::move(first_ge_model);
  return SUCCESS;
}

Status Om2PackageHelper::BuildDynamicGertModelData(const GeRootModelPtr &ge_root_model,
                                                   gert::GertModelData &model_data) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  const auto &root_graph = ge_root_model->GetRootGraph();
  GE_ASSERT_NOTNULL(root_graph);
  // 1. GE lowering，并恢复 OM2 所需的执行依赖。
  gert::ModelConverter::Args args;
  args.for_om2 = true;
  const auto execute_graph = gert::ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model, args);
  GE_ASSERT_NOTNULL(execute_graph);
  GE_ASSERT_SUCCESS(gert::om2::RestoreLaunchFreeDependencies(*execute_graph));

  // 2. 从 GE 编译产物提取资源和模型元信息，保留 lowering 分配的权重编号。
  gert::GertModelData built;
  gert::InitGertModelData(built);
  gert::InitGertModelDataUnit(built);
  built.models[0]->execution_type = gert::GertModelExecutionType::kDynamic;
  GE_ASSERT_SUCCESS(PrepareDynamicModelData(ge_root_model, built));

  // 3. 共用 OM2 编译打包流程，仅成功时发布结果。
  auto name = ge_root_model->GetModelName();
  if (name.empty()) {
    name = root_graph->GetName().empty() ? "om2_dynamic" : root_graph->GetName();
  }
  GE_ASSERT_SUCCESS(CompileAndPackageOm2(execute_graph, name, root_graph->GetGraphID(), built));
  GE_ASSERT_SUCCESS(BuildDynamicManifest(built, GetModelsToBuild(nullptr, ge_root_model)));
  auto online = std::make_shared<gert::Om2OnlinePrepareData>();
  online->graph_id = root_graph->GetGraphID();
  online->prepare_ge_variables = true;
  std::set<NodePtr> visited;
  const auto collect = [&](const ComputeGraphPtr &graph) -> Status {
    GE_ASSERT_NOTNULL(graph);
    online->graphs.emplace_back(graph);
    for (const auto &node : graph->GetAllNodes()) {
      if (!visited.insert(node).second) {
        continue;
      }
      if (node->GetType() == VARIABLE) {
        online->variables.emplace_back(node);
      } else if (node->GetType() == CONSTANTOP) {
        online->shared_constants.emplace_back(node);
      } else if (node->GetType() == FILECONSTANT) {
        online->file_constants.emplace_back(node);
      }
    }
    return SUCCESS;
  };
  GE_ASSERT_SUCCESS(collect(root_graph));
  for (const auto &entry : ge_root_model->GetSubgraphInstanceNameToModel()) {
    GE_ASSERT_NOTNULL(entry.second);
    GE_ASSERT_SUCCESS(collect(entry.second->GetGraph()));
  }
  built.online_prepare_data = std::move(online);
  model_data = std::move(built);
  return SUCCESS;
}

std::vector<GeModelPtr> Om2PackageHelper::GetModelsToBuild(const GeModelPtr &ge_model,
                                                           const GeRootModelPtr &ge_root_model) {
  if (ge_model != nullptr) {
    return {ge_model};
  }
  std::vector<GeModelPtr> models;
  if (ge_root_model != nullptr) {
    for (const auto &entry : ge_root_model->GetSubgraphInstanceNameToModel()) {
      models.emplace_back(entry.second);
    }
  }
  return models;
}

// GE 路线从 GeRootModel/GeModel 提取模型元信息、权重、Kernel、SO 和定位信息。
// om2动态当前限制：待资源及元信息的统一表示方案明确后，再与 PrepareGraphModelData 归一提取及装配流程。
Status Om2PackageHelper::PrepareDynamicModelData(const GeRootModelPtr &ge_root_model, gert::GertModelData &model_data) {
  model_data.so_bins = std::make_unique<gert::GertModelDataSoBins>();
  const auto models = GetModelsToBuild(nullptr, ge_root_model);
  GeModelPtr ge_model;
  GE_ASSERT_SUCCESS(GetDynamicGeModel(ge_root_model, ge_model));
  auto &unit = *model_data.models[0];
  GE_ASSERT_SUCCESS(BuildCustomOpResources(ge_root_model, model_data));
  for (const auto &model : models) {
    GE_ASSERT_SUCCESS(BuildKernelBinaries(model, model_data));
  }
  if (models.size() > 1U) {
    GE_ASSERT_SUCCESS(DeduplicateKernelBinaries(model_data));
  }
  GE_ASSERT_SUCCESS(BuildModelMeta(ge_model, unit));
  GE_ASSERT_SUCCESS(BuildDynamicConstantsData(models, unit, *model_data.constants));
  GE_ASSERT_SUCCESS(BuildSharedLibraries(ge_root_model, ge_model, model_data.so_bins.get()));
  GE_ASSERT_SUCCESS(BuildDebugInfo(ge_model, unit));
  return SUCCESS;
}

Status Om2PackageHelper::BuildSharedLibraries(const GeRootModelPtr &ge_root_model, const GeModelPtr &ge_model,
                                              gert::GertModelDataSoBins *shared_libraries) {
  if (shared_libraries == nullptr) {
    return SUCCESS;
  }
  std::vector<OpSoBinPtr> so_bins;
  GE_ASSERT_SUCCESS(CollectSoBinaries(ge_root_model, ge_model, so_bins));
  return BuildSoBinaries(so_bins, *shared_libraries);
}

Status Om2PackageHelper::DeduplicateKernelBinaries(gert::GertModelData &model_data) {
  auto &kernels = model_data.kernels->binaries;
  std::unordered_map<std::string, size_t> indices;
  size_t count = 0U;
  for (size_t i = 0U; i < kernels.size(); ++i) {
    auto &kernel = kernels[i];
    const std::string name = gert::GertGetStr(kernel->file_name);
    const auto iter = indices.find(name);
    if (iter != indices.end()) {
      const auto &existing = *kernels[iter->second];
      GE_ASSERT_TRUE(existing.data_size == kernel->data_size, "[OM2] Conflicting kernel size: %s", name.c_str());
      GE_ASSERT_TRUE(
          kernel->data_size == 0U || std::memcmp(existing.data.get(), kernel->data.get(), kernel->data_size) == 0,
          "[OM2] Conflicting kernel content: %s", name.c_str());
      continue;
    }
    indices.emplace(name, count);
    if (count != i) {
      kernels[count] = std::move(kernel);
    }
    ++count;
  }
  kernels.resize(count);
  return SUCCESS;
}

Status Om2PackageHelper::CompileDynamicProgramBody(const std::string &model_name, const uint32_t graph_id,
                                                   std::vector<gert::GertModelDataFile> &&artifacts,
                                                   gert::GertModelDataRuntime &runtime) {
  GE_ASSERT_TRUE(!artifacts.empty(), "[OM2] Dynamic source artifacts are empty");
  runtime.source_artifacts = std::move(artifacts);
  const Om2CompileInfo compile_info{model_name, model_name, graph_id};
  return Om2Utils::CompileGeneratedCppToSo(runtime.source_artifacts, compile_info, runtime.so_artifact, false);
}

// 两路在各自完成模型元信息和资源装配后汇合，此处共用 Codegen 和 SO 编译。
// 模型元信息和资源的提取、装配仍保留在各自的准备函数中。
Status Om2PackageHelper::CompileAndPackageOm2(const std::shared_ptr<ExecuteGraph> &graph, const std::string &model_name,
                                              const uint32_t graph_id, gert::GertModelData &model_data) {
  GE_ASSERT_NOTNULL(graph);
  // 1. 从最终执行图收集代码生成所需的信息。
  gert::om2::ProgramInfo program_info;
  GE_ASSERT_SUCCESS(gert::om2::ProgramInfoCollector().Collect(*graph, program_info));

  // 2. 生成模型源码。
  gert::om2::ProgramBodyCodegenResult result;
  GE_ASSERT_SUCCESS(GenerateDynamicProgramBody(program_info, model_name, result));

  // 3. 编译源码，保留准备阶段填入的资源和模型元信息。
  GE_ASSERT_SUCCESS(
      CompileDynamicProgramBody(model_name, graph_id, std::move(result.artifacts), *model_data.models[0]->runtime));

  return SUCCESS;
}

Status Om2PackageHelper::BuildGertModelDataFromGraph(const Graph &graph, gert::GertModelData &model_data) const {
  const auto execute_graph = GraphUtilsEx::Graph2ExecuteGraph(graph);
  GE_ASSERT_NOTNULL(execute_graph, "Expected a valid execution graph");
  // 1. 图后处理。
  GraphPostProcessResult prepared;
  GE_ASSERT_SUCCESS(PostProcessGraph(execute_graph, prepared));

  // 2. 从执行图提取资源和模型元信息。
  const auto name = execute_graph->GetName().empty() ? "om2_dynamic" : execute_graph->GetName();
  gert::GertModelData built;
  gert::InitGertModelData(built);
  gert::InitGertModelDataUnit(built);
  built.models[0]->execution_type = gert::GertModelExecutionType::kDynamic;
  GE_ASSERT_SUCCESS(PrepareGraphModelData(name, prepared, built));

  // 3. 共用 OM2 编译打包流程，仅成功时发布结果。
  GE_ASSERT_SUCCESS(CompileAndPackageOm2(execute_graph, name, 0U, built));
  GE_ASSERT_SUCCESS(BuildManifestCommonFields(built));
  built.online_prepare_data = std::make_shared<gert::Om2OnlinePrepareData>();
  model_data = std::move(built);
  return SUCCESS;
}

}  // namespace ge
