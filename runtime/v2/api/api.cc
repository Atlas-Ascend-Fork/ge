/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <map>
#include <mutex>
#include "runtime/gert_api.h"
#include "base/err_msg.h"
#include "lowering/model_converter.h"
#include "framework/common/helper/model_helper.h"
#include "common/helper/model_parser_base.h"
#include "common/host_resource_center/host_resource_center.h"
#include "core/debug/kernel_tracing.h"
#include "core/builder/model_v2_executor_builder.h"
#include "engine/aicore/autofuse_dispatcher/dispatcher_table.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_global_options.h"
#include "graph_metadef/common/ge_common/util.h"
#include "exe_graph/runtime/allocator.h"
#include "kernel/memory/host_mem_allocator.h"
#include "kernel/memory/caching_mem_allocator.h"
#include "kernel/memory/external_allocator.h"
#include "graph/utils/graph_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "acl/acl_rt.h"

namespace gert {
namespace {
class DispatcherGuardChecker : public GuardChecker {
 public:
  explicit DispatcherGuardChecker(std::shared_ptr<ge::autofuse_dispatcher::DispatcherTable> table)
      : table_(std::move(table)) {}
  bool Match(const std::vector<Tensor *> &inputs, char_t *reason, size_t reason_size) const override {
    return (table_ != nullptr) && table_->Match(inputs, reason, reason_size);
  }

 private:
  std::shared_ptr<ge::autofuse_dispatcher::DispatcherTable> table_;
};

ge::Status CheckGlobalDeterministicOption(std::map<std::string, std::string> &global_options,
                                          const std::string &option_name, const int32_t model_value) {
  int32_t process_value = 0;
  const auto iter = global_options.find(option_name);
  if (iter == global_options.cend()) {
    global_options.emplace(option_name, std::to_string(model_value));
    return ge::SUCCESS;
  }
  const std::string &process_value_str = iter->second;
  if ((ge::ConvertToInt32(process_value_str, process_value) == ge::SUCCESS) && (process_value == model_value)) {
    return ge::SUCCESS;
  }

  const std::string value = std::to_string(model_value);
  const char *reason = "the offline model configuration differs from the process global configuration.";
  REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"parameter", "value", "reason"}),
                            std::vector<const char *>({option_name.c_str(), value.c_str(), reason}));
  GELOGE(ge::PARAM_INVALID, "The offline model option[%s]=%d conflicts with process option[%s]=%s.",
         option_name.c_str(), model_value, option_name.c_str(), process_value_str.c_str());
  return ge::PARAM_INVALID;
}

ge::Status CheckGlobalDeterministicConfig(const ge::ComputeGraphPtr &root_graph) {
  GE_ASSERT_NOTNULL(root_graph);
  int32_t deterministic = 0;
  int32_t deterministic_level = 0;
  (void)ge::AttrUtils::GetInt(root_graph, ge::DETERMINISTIC, deterministic);
  (void)ge::AttrUtils::GetInt(root_graph, ge::DETERMINISTIC_LEVEL, deterministic_level);

  const std::lock_guard<std::mutex> lock(ge::GetGlobalOptionsMutex());
  auto &global_options = ge::GetMutableGlobalOptions();
  GE_ASSERT_SUCCESS(CheckGlobalDeterministicOption(global_options, ge::DETERMINISTIC, deterministic));
  GE_ASSERT_SUCCESS(CheckGlobalDeterministicOption(global_options, ge::DETERMINISTIC_LEVEL, deterministic_level));
  return ge::SUCCESS;
}

std::map<std::string, ge::GeModelPtr> ExtractFallbackModels(const ge::GeRootModelPtr &root_model) {
  std::map<std::string, ge::GeModelPtr> fallback_models;
  bool dual_body = false;
  if ((root_model->GetRootGraph() == nullptr) ||
      (!ge::AttrUtils::GetBool(root_model->GetRootGraph(), ge::kDualBodyAttr, dual_body)) || (!dual_body)) {
    return fallback_models;
  }
  const std::string fb_root_key = ge::kFallbackModelKey;
  const std::string fb_sub_prefix = ge::kFallbackSubgraphPrefix;
  std::vector<std::string> fb_keys;
  for (const auto &kv : root_model->GetSubgraphInstanceNameToModel()) {
    if ((kv.first == fb_root_key) || (kv.first.compare(0U, fb_sub_prefix.size(), fb_sub_prefix) == 0U)) {
      fb_keys.emplace_back(kv.first);
    }
  }
  for (const auto &key : fb_keys) {
    fallback_models[key] = root_model->GetSubgraphInstanceNameToModel().at(key);
    root_model->RemoveInstanceSubgraphModel(key);
  }
  return fallback_models;
}

size_t SumModelWeightSize(const std::map<std::string, ge::GeModelPtr> &models) {
  size_t total = 0U;
  for (const auto &kv : models) {
    if (kv.second != nullptr) {
      total += kv.second->GetWeightSize();
    }
  }
  return total;
}

void FillDualBodyWeightInfo(const ge::ComputeGraphPtr &root_graph, const ge::GeRootModelPtr &root_model,
                            const std::map<std::string, ge::GeModelPtr> &fallback_models, ExecutorSet &executor_set) {
  const size_t main_weight_size = SumModelWeightSize(root_model->GetSubgraphInstanceNameToModel());
  bool weights_identical = false;
  (void)ge::AttrUtils::GetBool(root_graph, ge::kWeightsIdenticalAttr, weights_identical);
  executor_set.weights_identical = weights_identical && (!fallback_models.empty());
  executor_set.shared_weight_size = executor_set.weights_identical ? main_weight_size : 0U;
  if (fallback_models.empty()) {
    return;
  }
  GELOGI("dual-body weights identical=%d, main_size=%zu.", static_cast<int32_t>(executor_set.weights_identical),
         main_weight_size);
}

ge::graphStatus LoadRootModelForExecutor(const ge::ModelData &model_data, ge::ModelHelper &model_helper,
                                         ge::GeRootModelPtr &root_model, const bool check_global_deterministic_config) {
  const auto ret = model_helper.LoadRootModel(model_data);
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "load root model failed.");
    return ret;
  }
  root_model = model_helper.GetGeRootModel();
  GE_ASSERT_NOTNULL(root_model);
  auto root_graph = root_model->GetRootGraph();
  GE_ASSERT_NOTNULL(root_graph);
  if (check_global_deterministic_config) {
    GE_ASSERT_SUCCESS(CheckGlobalDeterministicConfig(root_graph));
  }
  if (ge::GraphUtils::IsSingleOpScene(root_graph)) {
    auto root_ge_model = model_helper.GetGeModel();
    GE_ASSERT_NOTNULL(root_ge_model);
    root_graph->SetGraphUnknownFlag(true);
    root_model->SetRootGraph(root_graph);
    root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), root_ge_model);
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus AssignModelIdForExecutor(const ge::GeRootModelPtr &root_model) {
  uint32_t model_id = 0U;
  ge::ModelManager::GetInstance().GenModelId(model_id);
  root_model->SetCurModelId(model_id);
  GELOGI("Current id of hybrid model %s is %u.", root_model->GetModelName().c_str(), root_model->GetCurModelId());
  for (auto &named_model : root_model->GetSubgraphInstanceNameToModel()) {
    if (named_model.second == nullptr) {
      GELOGE(ge::FAILED, "Compiled model of graph %s is nullptr", named_model.first.c_str());
      return ge::FAILED;
    }
    named_model.second->SetModelId(model_id);
  }
  return ge::ModelManager::GetInstance().InitOpMasterDeviceSo(model_id, root_model);
}

ge::graphStatus BuildMainExecutor(const ge::ModelData &model_data, const ModelConverter::Args &args,
                                  RtSession *const rt_session, const ge::GeRootModelPtr &root_model,
                                  ExecutorSet &executor_set) {
  auto main_graph = ModelConverter().ConvertGeModelToExecuteGraph(root_model, args);
  if (main_graph == nullptr) {
    GELOGE(ge::FAILED, "lower main execution body failed.");
    return ge::FAILED;
  }
  ModelV2ExecutorBuilder main_builder(rt_session);
  main_builder.ExecuteGraph(main_graph).ModelData(model_data).GeRootModel(root_model);
  executor_set.main = main_builder.Build(ExecutorOption{});
  if (executor_set.main == nullptr) {
    GELOGE(ge::FAILED, "build main executor failed.");
    return ge::FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus BuildFallbackExecutor(const ge::ModelData &model_data, const ModelConverter::Args &args,
                                      RtSession *const rt_session, const ge::GeRootModelPtr &root_model,
                                      const std::map<std::string, ge::GeModelPtr> &fallback_models,
                                      ExecutorSet &executor_set) {
  ge::GeRootModelPtr fallback_root_model = nullptr;
  auto fallback_graph =
      ModelConverter().ConvertFallbackModelToExecuteGraph(root_model, fallback_models, args, fallback_root_model);
  if (fallback_graph == nullptr) {
    GELOGE(ge::FAILED, "lower fallback execution body failed.");
    return ge::FAILED;
  }
  ModelV2ExecutorBuilder fallback_builder(rt_session);
  fallback_builder.ExecuteGraph(fallback_graph).ModelData(model_data).GeRootModel(fallback_root_model);
  executor_set.fallback = fallback_builder.Build(ExecutorOption{});
  if (executor_set.fallback == nullptr) {
    GELOGE(ge::FAILED, "build fallback executor failed.");
    return ge::FAILED;
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus LoadGuardForExecutor(const ge::ComputeGraphPtr &root_graph, ExecutorSet &executor_set) {
  const std::string *guard_so = ge::AttrUtils::GetStr(root_graph, "_guard_check_so_data");
  if ((guard_so == nullptr) || guard_so->empty()) {
    return ge::GRAPH_SUCCESS;
  }
  auto table = std::make_shared<ge::autofuse_dispatcher::DispatcherTable>();
  if (table == nullptr) {
    return ge::FAILED;
  }
  if (table->LoadGuardSo(*guard_so) != ge::SUCCESS) {
    GELOGW("load guard so failed, disable guard and always take fallback body.");
    return ge::GRAPH_SUCCESS;
  }
  executor_set.guard = std::make_shared<DispatcherGuardChecker>(std::move(table));
  GELOGI("guard loaded, guard so size=%zu.", guard_so->size());
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus LoadToModelV2ExecutorBuilder(const ge::ModelData &model_data, const ModelConverter::Args &args,
                                             ModelV2ExecutorBuilder &builder,
                                             const bool check_global_deterministic_config) {
  // todo： 当前单算子是一个临时方案， 后续考虑与图流程归一
  ge::ModelHelper model_helper;
  ge::GeRootModelPtr root_model = nullptr;
  GE_ASSERT_SUCCESS(LoadRootModelForExecutor(model_data, model_helper, root_model, check_global_deterministic_config));

  GE_ASSERT_SUCCESS(AssignModelIdForExecutor(root_model));

  // autofuse 离线双执行体：单执行体接口只返回主执行体，先摘出兜底 sub-model。
  const auto fallback_models = ExtractFallbackModels(root_model);
  if (!fallback_models.empty()) {
    GELOGI("model contains %zu fallback sub model(s), ignored by single-executor API.", fallback_models.size());
  }

  auto graph = ModelConverter().ConvertGeModelToExecuteGraph(root_model, args);
  GE_ASSERT_NOTNULL(graph, "Failed to lowering to execute graph");

  builder.ExecuteGraph(graph).ModelData(model_data).GeRootModel(root_model);

  return ge::GRAPH_SUCCESS;
}
}  // namespace

ge::graphStatus LoadExecutorSetFromModelData(const ge::ModelData &model_data, const LoadExecutorArgs &args,
                                             ExecutorSet &executor_set) {
  ge::ModelHelper model_helper;
  ge::GeRootModelPtr root_model = nullptr;
  GE_ASSERT_SUCCESS(LoadRootModelForExecutor(model_data, model_helper, root_model, true));
  GE_ASSERT_SUCCESS(AssignModelIdForExecutor(root_model));

  const auto fallback_models = ExtractFallbackModels(root_model);
  FillDualBodyWeightInfo(root_model->GetRootGraph(), root_model, fallback_models, executor_set);
  ModelConverter::Args converter_args{{}, nullptr, nullptr, nullptr, &args.file_constant_mems};
  GE_ASSERT_SUCCESS(BuildMainExecutor(model_data, converter_args, args.rt_session, root_model, executor_set));
  if (!fallback_models.empty()) {
    GE_ASSERT_SUCCESS(
        BuildFallbackExecutor(model_data, converter_args, args.rt_session, root_model, fallback_models, executor_set));
  }
  if (executor_set.fallback != nullptr) {
    GE_ASSERT_SUCCESS(LoadGuardForExecutor(root_model->GetRootGraph(), executor_set));
  }
  GELOGI("load executor set success, has_fallback=%d, weights_identical=%d.",
         static_cast<int32_t>(executor_set.fallback != nullptr), static_cast<int32_t>(executor_set.weights_identical));
  return ge::GRAPH_SUCCESS;
}

void FreeModelData(ge::ModelData &model_data) {
  delete[] static_cast<ge::char_t *>(model_data.model_data);
  model_data.model_data = nullptr;
}

ge::graphStatus LoadDataFromFile(const ge::char_t *model_path, ge::ModelData &model_data) {
  const auto ret = ge::ModelParserBase::LoadFromFile(model_path, -1, model_data);
  if (ret != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "Failed to load model data");
  }
  return ret;
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromFile(const ge::char_t *model_path, ge::graphStatus &error_code) {
  ge::ModelData model_data;
  GE_MAKE_GUARD(release_model_data, [&model_data] {
    if (model_data.model_data != nullptr) {
      FreeModelData(model_data);
    }
  });
  error_code = LoadDataFromFile(model_path, model_data);
  if (error_code != ge::GRAPH_SUCCESS) {
    GELOGE(ge::FAILED, "Failed to load model data from model path");
    return nullptr;
  }

  auto ret = LoadExecutorFromModelData(model_data, error_code);
  return ret;
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromModelData(const ge::ModelData &model_data,
                                                           ge::graphStatus &error_code) {
  return LoadExecutorFromModelData(model_data, ExecutorOption{}, error_code);
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromModelDataWithRtSession(const ge::ModelData &model_data,
                                                                        RtSession *const rt_session,
                                                                        ge::graphStatus &error_code) {
  ModelV2ExecutorBuilder builder(rt_session);
  error_code = LoadToModelV2ExecutorBuilder(model_data, {}, builder, false);
  GE_ASSERT_SUCCESS(error_code);
  ExecutorOption option{};
  return builder.Build(option);
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromModelData(const ge::ModelData &model_data,
                                                           const LoadExecutorArgs &args, ge::graphStatus &error_code) {
  ModelV2ExecutorBuilder builder(args.rt_session);
  ModelConverter::Args converter_args{{}, nullptr, nullptr, nullptr, &args.file_constant_mems};
  error_code = LoadToModelV2ExecutorBuilder(model_data, converter_args, builder, true);
  GE_ASSERT_SUCCESS(error_code);
  ExecutorOption option{};
  return builder.Build(option);
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromModelData(const ge::ModelData &model_data,
                                                           const ExecutorOption &executor_option,
                                                           ge::graphStatus &error_code) {
  ModelV2ExecutorBuilder builder;
  error_code = LoadToModelV2ExecutorBuilder(model_data, {}, builder, false);
  GE_ASSERT_SUCCESS(error_code);
  return builder.Build(executor_option);
}

std::unique_ptr<ModelV2Executor> LoadExecutorFromModelData(
    const ge::ModelData &model_data, const ExecutorOption &executor_option, StreamAllocator *const stream_allocator,
    EventAllocator *const event_allocator, NotifyAllocator *const notify_allocator, ge::graphStatus &error_code) {
  ModelV2ExecutorBuilder builder;
  ModelConverter::Args args{{}, stream_allocator, event_allocator, notify_allocator, nullptr};
  error_code = LoadToModelV2ExecutorBuilder(model_data, args, builder, false);
  GE_ASSERT_SUCCESS(error_code);
  return builder.Build(executor_option);
}

std::unique_ptr<StreamExecutor> LoadStreamExecutorFromModelData(const ge::ModelData &model_data,
                                                                const LoweringOption &optimize_option,
                                                                ge::graphStatus &error_code) {
  auto builder = ge::MakeUnique<ModelV2ExecutorBuilder>();
  GE_ASSERT_NOTNULL(builder);
  error_code = LoadToModelV2ExecutorBuilder(
      model_data, ModelConverter::Args{optimize_option, nullptr, nullptr, nullptr, nullptr}, *builder, false);
  GE_ASSERT_SUCCESS(error_code);
  return ge::MakeUnique<StreamExecutor>(builder.release());
}

std::unique_ptr<StreamExecutor> LoadStreamExecutorFromModelData(const ge::ModelData &model_data,
                                                                ge::graphStatus &error_code) {
  LoweringOption option;
  return LoadStreamExecutorFromModelData(model_data, option, error_code);
}

std::unique_ptr<StreamExecutor> LoadStreamExecutorFromModelData(const ge::ModelData &model_data, const void *weight_ptr,
                                                                const size_t weight_size, ge::graphStatus &error_code) {
  (void)weight_ptr;
  (void)weight_size;
  auto builder = ge::MakeUnique<ModelV2ExecutorBuilder>();
  GE_ASSERT_NOTNULL(builder);
  error_code = LoadToModelV2ExecutorBuilder(model_data, {}, *builder, false);
  GE_ASSERT_SUCCESS(error_code);
  return ge::MakeUnique<StreamExecutor>(builder.release());
}

ge::graphStatus IsDynamicModel(const void *const model, size_t model_size, bool &is_dynamic_model) {
  GE_ASSERT_NOTNULL(model, "[Check][Input] failed, model is null.");
  ge::ModelData model_data;
  if (model_size < sizeof(ge::ModelFileHeader)) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID,
           "[Check][Param] Invalid model size. Model data size %zu must be greater than or equal to %zu.", model_size,
           sizeof(ge::ModelFileHeader));
    return ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID;
  }
  const auto file_head = reinterpret_cast<const ge::ModelFileHeader *>(model);
  is_dynamic_model = ge::ModelParserBase::IsDynamicModel(*file_head);
  GELOGD("Parser model[%p], model size[%zu], is_dynamic_model[%d].", model, model_size, is_dynamic_model);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus IsDynamicModel(const ge::char_t *model_path, bool &is_dynamic_model) {
  GE_ASSERT_NOTNULL(model_path, "[Check][ModelPath] failed, model_path is null.");
  ge::ModelData model_data;
  GE_MAKE_GUARD(release_model_data, [&model_data] {
    if (model_data.model_data != nullptr) {
      FreeModelData(model_data);
    }
  });
  ge::graphStatus error_code = ge::ModelParserBase::LoadFromFile(model_path, -1, model_data);
  if (error_code != ge::GRAPH_SUCCESS) {
    GELOGE(error_code, "Failed to load model data from model path[%s]", model_path);
    return error_code;
  }
  const ge::ModelFileHeader *file_head = nullptr;
  GE_CHK_STATUS_RET(ge::ModelHelper::GetModelFileHead(model_data, file_head), "[Get][ModelFileHead] failed.");
  GE_ASSERT_NOTNULL(file_head);
  is_dynamic_model = ge::ModelParserBase::IsDynamicModel(*file_head);
  GELOGD("Parser model path[%s], is_dynamic_model[%d].", model_path, is_dynamic_model);
  return ge::GRAPH_SUCCESS;
}

std::unique_ptr<ge::Allocator> AllocatorFactory::Create(const TensorPlacement &placement) {
  return Create("", placement);
}

std::unique_ptr<ge::Allocator> AllocatorFactory::Create(const std::string &graph_name,
                                                        const TensorPlacement &placement) {
  int32_t device_id = 0;
  switch (placement) {
    case kOnDeviceHbm:
      (void)aclrtGetDevice(&device_id);
      return memory::CachingMemAllocator::GetAllocator(graph_name, device_id, RT_MEMORY_HBM);
    case kOnDeviceP2p:
      (void)aclrtGetDevice(&device_id);
      return memory::CachingMemAllocator::GetAllocator(graph_name, device_id, RT_MEMORY_P2P_DDR);
    case kOnHost:
    case kFollowing:
      return ge::MakeUnique<memory::HostMemAllocator>();
    default:
      GELOGE(ge::PARAM_INVALID, "Unsupported placement %d to create allocator", static_cast<int32_t>(placement));
      return nullptr;
  }
}

std::unique_ptr<ge::Allocator> CreateExternalAllocator(const ge::AllocatorDesc *const allocatorDesc) {
  return ge::MakeUnique<ExternalAllocator>(*allocatorDesc);
}
}  // namespace gert
