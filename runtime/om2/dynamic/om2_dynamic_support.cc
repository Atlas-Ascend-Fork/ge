/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "om2_dynamic_api.h"

#include <cstddef>
#include "common/om2/om2_online_prepare_data.h"
#include "hybrid/executor/hybrid_model_rt_v2_executor.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/mem_manager.h"
#include "graph/manager/active_memory_allocator.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/checker.h"
#include "common/om2/om2_model_api_utils.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/runtime/mem_allocator.h"
#include "framework/runtime/rt_session.h"

namespace {
using namespace gert;
struct Om2LoadContext {
  ge::Status Init(const gert::Om2OnlinePrepareData &data, uint64_t session_id, uint32_t device_id) {
    using namespace ge;
    session.SetSessionId(session_id);
    if (!data.prepare_ge_variables) {
      return SUCCESS;
    }
    const auto var_manager = VarManager::Instance(session_id);
    GE_ASSERT_NOTNULL(var_manager);
    if (!var_manager->HasMemoryManager()) {
      var_manager->SetMemManager(&MemManager::Instance());
    }
    const auto page_size = VarManager::IsVariableUse1gHugePage() ? kDrv1GPageSize : kDrvPageSize;
    auto allocator = SessionMemAllocator<ExpandableActiveMemoryAllocator>::Instance().GetMemAllocator(
        session_id, device_id, RT_MEMORY_HBM, page_size);
    GE_ASSERT_SUCCESS(var_manager->InitExpandableMemoryAllocator(allocator));
    GE_ASSERT_SUCCESS(visitor.Init(var_manager, device_id, session_id, data.graph_id));
    GE_ASSERT_SUCCESS(visitor.AssembleVariables(data.variables));
    GE_ASSERT_SUCCESS(visitor.AssembleSharedConstants(data.shared_constants));
    GE_ASSERT_SUCCESS(visitor.AssembleFileConstants(data.file_constants));
    session.SetVarManager(&visitor);
    return SUCCESS;
  }
  // Host shared constants may borrow the source node weight storage.
  std::shared_ptr<gert::Om2OnlinePrepareData> prepare_data;
  ge::hybrid::GraphVarVisitor visitor;
  gert::RtSession session;
};

struct DynamicContext {
  std::unique_ptr<Om2LoadContext> owned_load_context;
  RtSession *rt_session = nullptr;
  std::shared_ptr<Allocators> allocators;
  std::vector<ge::ReadonlyByteBuffer> host_buffers;
  std::map<uint64_t, void *> host_constants;
  std::vector<GertModelSharedLibrary> descriptions;
  std::vector<const GertModelSharedLibrary *> pointers;
  GertModelSharedLibraries libraries;
  GertModelDynamicLoadConfig load_config;
};

ge::Status PrepareSharedLibraries(const GertModelData &model_data, std::vector<GertModelSharedLibrary> &descriptions,
                                  std::vector<const GertModelSharedLibrary *> &pointers,
                                  GertModelSharedLibraries &libraries) {
  const auto *so_bins = GetGertModelSoBins(model_data);
  if (so_bins == nullptr) {
    return ge::SUCCESS;
  }
  GE_ASSERT_TRUE(so_bins->struct_size >= offsetof(GertModelDataSoBins, libraries) + sizeof(so_bins->libraries));
  descriptions.resize(so_bins->libraries.size());
  pointers.reserve(descriptions.size());
  for (size_t i = 0U; i < descriptions.size(); ++i) {
    const auto &source = so_bins->libraries[i];
    GE_ASSERT_NOTNULL(source);
    GE_ASSERT_TRUE(source->struct_size >= offsetof(GertModelDataSoBin, vendor_name) + sizeof(source->vendor_name));
    GE_ASSERT_NOTNULL(source->binary);
    auto &target = descriptions[i];
    target.so_name = gert::GertGetStr(source->so_name);
    target.so_bin_type = static_cast<GertModelSharedLibraryType>(source->so_bin_type);
    target.vendor_name = gert::GertGetStr(source->vendor_name);
    target.data = source->binary->data.get();
    target.data_size = source->binary->data_size;
    GE_ASSERT_TRUE(gert::om2::IsValidSharedLibrary(target));
    pointers.push_back(&target);
  }
  libraries.library_num = pointers.size();
  libraries.libraries = pointers.data();
  return ge::SUCCESS;
}

ge::Status PrepareHostConstants(const GertModelData &model_data, DynamicContext &context) {
  GE_ASSERT_TRUE(!model_data.models.empty());
  GE_ASSERT_NOTNULL(model_data.models[0]);
  GE_ASSERT_NOTNULL(model_data.models[0]->constants_config);
  const auto &config = *model_data.models[0]->constants_config;
  std::map<std::string, const GertModelDataFile *> files;
  std::map<std::string, uint8_t *> loaded_files;
  bool files_initialized = false;
  for (const auto &meta : config.consts) {
    GE_ASSERT_NOTNULL(meta);
    if (GetGertModelConstPlacement(*meta) != GertModelDataConstPlacement::kHost) {
      continue;
    }
    GE_ASSERT_TRUE(std::string(GertGetStr(meta->type)) == "INTERNAL");
    GE_ASSERT_TRUE(context.host_constants.emplace(meta->index, nullptr).second);
    GE_ASSERT_TRUE((meta->offset >= 0) && (meta->size >= 0));
    if (!files_initialized) {
      GE_ASSERT_NOTNULL(model_data.constants);
      for (const auto &file : model_data.constants->constants_data) {
        if (file != nullptr) {
          GE_ASSERT_TRUE(files.emplace(GertGetStr(file->file_name), file.get()).second,
                         "[OM2] Duplicate Host internal weight file");
        }
      }
      files_initialized = true;
    }
    if (meta->size == 0) {
      continue;
    }
    const std::string name = GertGetStr(meta->file_name);
    const auto file = files.find(name);
    GE_ASSERT_TRUE(file != files.end(), "[OM2] Missing Host internal weight file %s", name.c_str());
    GE_ASSERT_NOTNULL(file->second->data);
    const auto offset = static_cast<uint64_t>(meta->offset);
    GE_ASSERT_TRUE(offset <= file->second->data_size);
    GE_ASSERT_TRUE(static_cast<uint64_t>(meta->size) <= file->second->data_size - offset);
    auto loaded = loaded_files.find(name);
    if (loaded == loaded_files.end()) {
      auto data = GertMakeFileData(file->second->data.get(), file->second->data_size);
      GE_ASSERT_NOTNULL(data);
      loaded = loaded_files.emplace(name, const_cast<uint8_t *>(data.get())).first;
      context.host_buffers.emplace_back(std::move(data));
    }
    context.host_constants[meta->index] = loaded->second + offset;
  }
  return ge::SUCCESS;
}
}  // namespace

static ge::Status DynamicCreate(const Om2DynamicCreateInput *input, Om2DynamicCreateOutput *output) {
  try {
    GE_ASSERT_NOTNULL(input);
    GE_ASSERT_NOTNULL(output);
    GE_ASSERT_TRUE(input->struct_size >= offsetof(Om2DynamicCreateInput, session_id));
    GE_ASSERT_TRUE(output->struct_size >= sizeof(Om2DynamicCreateOutput));
    output->context = nullptr;
    output->external_var_addr = nullptr;
    output->external_var_size = 0;
    GE_ASSERT_NOTNULL(input->model_data);
    auto context = std::make_unique<DynamicContext>();
    context->rt_session = static_cast<gert::RtSession *>(input->rt_session);
    const auto &data = *input->model_data;
    if ((context->rt_session == nullptr) &&
        (data.struct_size >= offsetof(GertModelData, online_prepare_data) + sizeof(data.online_prepare_data)) &&
        (data.online_prepare_data != nullptr)) {
      GE_ASSERT_TRUE(input->struct_size >= sizeof(Om2DynamicCreateInput));
      GE_ASSERT_TRUE(input->runtime_context_valid == 1U);
      context->owned_load_context = std::make_unique<Om2LoadContext>();
      context->owned_load_context->prepare_data = data.online_prepare_data;
      GE_ASSERT_SUCCESS(
          context->owned_load_context->Init(*data.online_prepare_data, input->session_id, input->device_id));
      context->rt_session = &context->owned_load_context->session;
    }
    if (input->external_allocator != nullptr) {
      context->allocators =
          std::make_shared<gert::Allocators>(*static_cast<gert::Allocators *>(input->external_allocator));
    }
    GE_ASSERT_SUCCESS(
        PrepareSharedLibraries(*input->model_data, context->descriptions, context->pointers, context->libraries));
    context->load_config.shared_libraries =
        GetGertModelSoBins(*input->model_data) == nullptr ? nullptr : &context->libraries;
    context->load_config.rt_session = context->rt_session;
    GE_ASSERT_SUCCESS(PrepareHostConstants(*input->model_data, *context));
    if (context->rt_session != nullptr) {
      context->rt_session->GetExternalVar(output->external_var_addr, output->external_var_size);
    }
    output->context = context.release();
    return ge::SUCCESS;
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] Create raised an exception.");
    return ge::FAILED;
  }
}

static ge::Status DynamicPrepareConstants(void *context, void **constants, uint64_t constant_count) {
  try {
    GE_ASSERT_NOTNULL(context);
    const auto &host_constants = static_cast<DynamicContext *>(context)->host_constants;
    if (!host_constants.empty()) {
      GE_ASSERT_NOTNULL(constants);
      GE_ASSERT_TRUE(host_constants.rbegin()->first < constant_count);
    }
    for (const auto &item : host_constants) {
      constants[item.first] = item.second;
    }
    return ge::SUCCESS;
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] PrepareConstants raised an exception.");
    return ge::FAILED;
  }
}

static ge::Status DynamicPrepareLoad(void *context, GertModelLoadConfig *config) {
  try {
    GE_ASSERT_NOTNULL(context);
    GE_ASSERT_NOTNULL(config);
    GE_ASSERT_TRUE(config->struct_size >=
                   offsetof(GertModelLoadConfig, dynamic_config) + sizeof(config->dynamic_config));
    const auto &dynamic = *static_cast<DynamicContext *>(context);
    config->external_allocator = dynamic.allocators.get();
    config->dynamic_config = &dynamic.load_config;
    return ge::SUCCESS;
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] PrepareLoad raised an exception.");
    return ge::FAILED;
  }
}

static ge::Status DynamicPrepareRun(void *context, void *external_allocator, GertModelRunConfig *config) {
  try {
    GE_ASSERT_NOTNULL(context);
    GE_ASSERT_NOTNULL(config);
    GE_ASSERT_TRUE(config->struct_size >=
                   offsetof(GertModelRunConfig, external_allocator) + sizeof(config->external_allocator));
    // A non-null zero-capacity address would prevent runtime output sharing.
    for (uint64_t i = 0U; i < config->output_count; ++i) {
      auto &output = *static_cast<gert::Tensor *>(config->output_data[i]);
      if ((output.GetAddr() != nullptr) && (output.GetSize() == 0U)) {
        const auto placement = output.GetPlacement();
        output.SetData(gert::TensorData(nullptr, nullptr, 0U, placement));
      }
    }
    config->external_allocator = external_allocator;
    return ge::SUCCESS;
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] PrepareRun raised an exception.");
    return ge::FAILED;
  }
}

extern "C" int32_t Om2DynamicCreate(const Om2DynamicCreateInput *input, Om2DynamicCreateOutput *output) {
  return static_cast<int32_t>(DynamicCreate(input, output));
}

extern "C" int32_t Om2DynamicPrepareConstants(void *context, void **constants, uint64_t constant_count) {
  return static_cast<int32_t>(DynamicPrepareConstants(context, constants, constant_count));
}

extern "C" int32_t Om2DynamicPrepareLoad(void *context, GertModelLoadConfig *config) {
  return static_cast<int32_t>(DynamicPrepareLoad(context, config));
}

extern "C" int32_t Om2DynamicPrepareRun(void *context, void *external_allocator, GertModelRunConfig *config) {
  return static_cast<int32_t>(DynamicPrepareRun(context, external_allocator, config));
}

extern "C" void Om2DynamicDestroy(void *context) {
  try {
    delete static_cast<DynamicContext *>(context);
  } catch (...) {
    GELOGE(ge::FAILED, "[OM2][Dynamic] Destroy raised an exception.");
  }
}
