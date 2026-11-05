/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "runtime/custom_op/python_custom_op_bridge_loader.h"

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "securec.h"

#include "common/ge_common/string_util.h"
#include "common/python_runtime/python_artifact_utils.h"
#include "common/python_runtime/python_bridge_loader_utils.h"
#include "common/python_runtime/python_fallback_codegen_helper.h"
#include "framework/common/debug/ge_log.h"
#include "graph/ascend_string.h"
#include "graph/custom_op_factory.h"
#include "common/helper/python_custom_op_package_helper.h"
#include "graph_metadef/graph/utils/file_utils.h"
#include "mmpa/mmpa_api.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "runtime/custom_op/python_custom_op_bridge_c_api.h"
#include "runtime/custom_op/python_custom_op_proto.h"

namespace ge {
namespace custom_op {
namespace {

constexpr const char *kCustomOpArtifactsRelativePath = "custom_op/python_custom_op_artifacts";
constexpr const char *kPythonFileSuffix = ".py";
constexpr const char *kPythonPackageInitFile = "__init__.py";

namespace artifact = ::ge::python_artifact;
namespace bridge_loader = ::ge::python_bridge_loader;
namespace fallback_codegen = ::ge::python_fallback_codegen;

bool IsPythonFile(const std::string &path) {
  return (path.size() > strlen(kPythonFileSuffix)) &&
         (path.compare(path.size() - strlen(kPythonFileSuffix), strlen(kPythonFileSuffix), kPythonFileSuffix) == 0);
}

bool IsSkippedModuleEntry(const char *name) {
  if (name == nullptr) {
    return true;
  }
  return (name[0] == '_') || (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

bool HasPackageInitFile(const std::string &dir) {
  struct stat path_stat{};
  return stat((dir + "/" + kPythonPackageInitFile).c_str(), &path_stat) == 0;
}

bool HasPythonCustomOpEntry(const std::string &path, const struct stat &path_stat) {
  if (S_ISREG(path_stat.st_mode)) {
    return IsPythonFile(path);
  }
  if (!S_ISDIR(path_stat.st_mode)) {
    return false;
  }

  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    GELOGW("Skip scanning python custom op directory[%s] because opendir failed.", path.c_str());
    return false;
  }
  struct dirent *entry = nullptr;
  while ((entry = readdir(dir)) != nullptr) {
    if (IsSkippedModuleEntry(entry->d_name)) {
      continue;
    }
    const std::string entry_path = path + "/" + entry->d_name;
    struct stat child_stat{};
    if (stat(entry_path.c_str(), &child_stat) != 0) {
      GELOGW("Skip scanning python custom op path[%s] because stat failed.", entry_path.c_str());
      continue;
    }
    if (S_ISREG(child_stat.st_mode) && IsPythonFile(entry_path)) {
      (void)closedir(dir);
      return true;
    }
    if (S_ISDIR(child_stat.st_mode) && HasPackageInitFile(entry_path)) {
      (void)closedir(dir);
      return true;
    }
  }
  (void)closedir(dir);
  return false;
}

Status FindPythonCustomOpEntryInEnv(const char *env_value, bool &found) {
  found = false;
  const std::string custom_opp_path = env_value;
  const std::vector<std::string> custom_opp_paths = StringUtils::Split(custom_opp_path, ':');
  if (custom_opp_paths.empty()) {
    return SUCCESS;
  }
  for (auto path : custom_opp_paths) {
    if (StringUtils::Trim(path).empty()) {
      continue;
    }
    struct stat path_stat{};
    if (stat(path.c_str(), &path_stat) != 0) {
      if (IsPythonFile(path)) {
        GELOGE(FAILED, "Python custom op path[%s] does not exist or is inaccessible.", path.c_str());
        return FAILED;
      }
      GELOGW("Skip inaccessible custom op path[%s].", path.c_str());
      continue;
    }
    if (HasPythonCustomOpEntry(path, path_stat)) {
      found = true;
      return SUCCESS;
    }
  }
  return SUCCESS;
}

std::string GetLoaderLibraryPath() {
  Dl_info dl_info{};
  if ((dladdr(reinterpret_cast<void *>(&LoadPythonCustomOps), &dl_info) == 0) || (dl_info.dli_fname == nullptr) ||
      (dl_info.dli_fname[0] == '\0')) {
    return "";
  }
  const auto real_path = RealPath(dl_info.dli_fname);
  return real_path.empty() ? std::string(dl_info.dli_fname) : real_path;
}

// ABI 精确匹配：expected_abi == kPythonCustomOpBridgeAbiVersion 时函数表已含全部
// 模型 scope 恢复入口，因此一并要求非空——缺任一项说明 bridge SO 与加载侧头文件不同步。
bool IsBridgeApiValid(const PythonCustomOpBridgeApi *api, const uint32_t expected_abi) {
  return (api != nullptr) && (api->abi_version == expected_abi) && (api->set_artifact_config != nullptr) &&
         (api->register_custom_ops != nullptr) && (api->reset_bridge_state != nullptr) &&
         (api->shutdown_bridge != nullptr) && (api->collect_deployment_artifacts != nullptr) &&
         (api->restore_model_scope != nullptr) && (api->collect_model_scope_proto_op_types != nullptr) &&
         (api->close_model_scope != nullptr);
}

// "同 artifact_id 必须同内容"的逐字段比较由 package_helper 统一提供（保存侧 AddArtifact
// 与本处 IngestDeploymentArtifact 共用同一不变量），此处不再保留本地副本。
struct PythonCustomOpRegistrationEntry {
  std::string op_type;
  std::string proto_descriptor_key;
  bool has_proto{false};
  PythonCustomOpInferMetaFn infer_meta{nullptr};
  bool has_impl{false};
  PythonCustomOpAdapterDescriptor impl_desc;
  PythonCustomOpAdapterCallbacks callbacks;
};

bridge_loader::BridgeLoadDependencies BuildBridgeLoadDependencies() {
  return bridge_loader::BridgeLoadDependencies{
      &RealPath,
      &dlopen,
      &dlclose,
      &dlsym,
      &artifact::ResolveLoadedPythonRuntimeKey,
      kPythonCustomOpBridgeGetApiSymbol,
      kPythonCustomOpBridgeAbiVersion,
      RTLD_NOW | RTLD_GLOBAL,
  };
}

class PythonCustomOpBridgeLoader {
 public:
  static PythonCustomOpBridgeLoader &GetInstance() {
    static PythonCustomOpBridgeLoader loader;
    return loader;
  }

  Status Load() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto ret = EnsureLoaded();
    if (ret != SUCCESS) {
      return ret;
    }
    // artifact 必须先于 register_custom_ops 收集：creator 闭包会捕获含 artifact 的 descriptor。
    const auto collect_ret = CollectDeploymentArtifactsFromBridge();
    if (collect_ret != SUCCESS) {
      return collect_ret;
    }
    return RegisterCustomOpsFromBridge();
  }

  // 恢复侧（PYTHON_CUSTOM_OPS 分区）模型 scope 会话。与 Load() 的关键差异：
  // 只确保 bridge SO 已加载，不收集编译期 artifact、不把文件系统插件注册进进程级
  // CustomOpFactory（进程级全局注册表零写入是 D3/D4 的前提）。每个调用都持锁执行，
  // api_ 在锁内重新校验，避免与 ShutdownForProcess 竞争。
  // sink 的深拷贝目标经 user_data 直达调用方 vector（R2-3）：整段调用在本函数持有的
  // mutex_ 保护下同步完成，ingest 回调不再经单例暂存，天然可重入。
  Status RestoreModelScope(const uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources,
                           const size_t source_count, std::vector<PythonCustomOpScopedDescriptor> &descriptors) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto ret = EnsureLoaded();
    if (ret != SUCCESS) {
      GELOGE(ret, "Ensure python custom op bridge failed for restoring model scope[%llu].",
             static_cast<unsigned long long>(scope_id));
      return ret;
    }
    descriptors.clear();
    static constexpr PythonCustomOpScopedDescriptorSink kSink = {&IngestScopedDescriptorFromBridge};
    const auto restore_ret = api_->restore_model_scope(scope_id, sources, source_count, &kSink, &descriptors);
    if (restore_ret != SUCCESS) {
      descriptors.clear();
      GELOGE(restore_ret, "Restore model scope[%llu] with %zu embedded source(s) failed.",
             static_cast<unsigned long long>(scope_id), source_count);
      return restore_ret;
    }
    GELOGI("Restored model scope[%llu] with %zu embedded source(s), collected %zu scoped impl descriptor(s).",
           static_cast<unsigned long long>(scope_id), source_count, descriptors.size());
    return SUCCESS;
  }

  Status CollectModelScopeProtoOpTypes(const uint64_t scope_id, std::vector<std::string> &op_types) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto ret = EnsureLoaded();
    if (ret != SUCCESS) {
      GELOGE(ret, "Ensure python custom op bridge failed for suppressed proto op types of model scope[%llu].",
             static_cast<unsigned long long>(scope_id));
      return ret;
    }
    op_types.clear();
    static constexpr PythonCustomOpStringSink kSink = {&IngestProtoOpTypeFromBridge};
    const auto collect_ret = api_->collect_model_scope_proto_op_types(scope_id, &kSink, &op_types);
    if (collect_ret != SUCCESS) {
      op_types.clear();
      GELOGE(collect_ret, "Collect suppressed proto op types of model scope[%llu] failed.",
             static_cast<unsigned long long>(scope_id));
      return collect_ret;
    }
    return SUCCESS;
  }

  // 关闭 scope 是回滚/卸载路径的兜底，best-effort：bridge 未加载说明 Python 侧
  // 从未开过 scope，失败也只告警不升级。解释器已 finalize 时由 bridge 侧
  // CloseModelScope 的 Py_IsInitialized 保护统一跳过，loader 侧不重复判断
  // （custom_op_runtime 不链 Python，跨 SO 调用进入已 finalize 的 bridge 会早退）。
  void CloseModelScope(const uint64_t scope_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (api_ == nullptr) {
      GELOGI("Skip closing python custom op model scope[%llu] because no bridge is loaded.",
             static_cast<unsigned long long>(scope_id));
      return;
    }
    const auto ret = api_->close_model_scope(scope_id);
    if (ret != SUCCESS) {
      GELOGW("Close python custom op model scope[%llu] failed with status[%u].",
             static_cast<unsigned long long>(scope_id), static_cast<uint32_t>(ret));
    }
  }

  void Unload() {
    std::lock_guard<std::mutex> lock(mutex_);
    GELOGI("Unload python custom ops with bridge library[%s].", loaded_path_.c_str());
    ClearPythonCustomOpRegistrations();
    ClearRegisteredState();
    if ((api_ != nullptr) && (api_->reset_bridge_state != nullptr)) {
      api_->reset_bridge_state();
    }
  }

  void ShutdownForProcess() {
    std::lock_guard<std::mutex> lock(mutex_);
    GELOGI("Shutdown python custom op bridge for process, current library[%s].", loaded_path_.c_str());
    if ((api_ != nullptr) && (api_->shutdown_bridge != nullptr)) {
      api_->shutdown_bridge();
    }
    api_ = nullptr;
    if (handle_ != nullptr) {
      if (dlclose(handle_) != 0) {
        GELOGW("Close python custom op bridge library failed: %s", dlerror());
      }
      handle_ = nullptr;
    }
    loaded_path_.clear();
    ClearRegisteredState();
  }

 private:
  void ClearPythonCustomOpRegistrations() {
    std::vector<AscendString> adapter_op_types;
    adapter_op_types.reserve(registered_op_type_to_adapter_.size());
    for (const auto &op_type : registered_op_type_to_adapter_) {
      adapter_op_types.emplace_back(op_type.c_str());
    }
    CustomOpFactory::RemoveCustomOps(adapter_op_types);
    ClearPythonCustomOpRuntimeRegistry();
    std::vector<std::string> proto_op_types;
    proto_op_types.reserve(python_custom_op_registrations_.size());
    for (const auto &item : python_custom_op_registrations_) {
      if (item.second.has_proto) {
        proto_op_types.emplace_back(item.first);
      }
    }
    UnregisterPythonCustomOpProtos(proto_op_types);
  }

  // 从 bridge 拉取编译期冻结的部署 artifact，深拷贝后按 artifact_id 与 op_type 双向索引。
  // view 中的指针只在 sink 回调期间有效，因此必须在此完成拷贝。
  Status CollectDeploymentArtifactsFromBridge() {
    static constexpr PythonCustomOpDeploymentArtifactSink kSink = {&IngestDeploymentArtifactFromBridge};
    const auto ret = api_->collect_deployment_artifacts(&kSink, nullptr);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Collect][PythonCustomOpDeploymentArtifacts] failed with bridge library[%s].", loaded_path_.c_str());
      return ret;
    }
    GELOGI("[Collect][PythonCustomOpDeploymentArtifacts] collected %zu artifact(s) covering %zu op type(s).",
           artifacts_by_id_.size(), artifact_by_op_type_.size());
    return SUCCESS;
  }

  static bool IngestDeploymentArtifactFromBridge(void *user_data, const PythonCustomOpDeploymentArtifactView *view) {
    // user_data 恒为 nullptr：ingest 目标是单例成员 map，不随调用方变化
    (void)user_data;
    if (view == nullptr) {
      return false;
    }
    return GetInstance().IngestDeploymentArtifact(*view);
  }

  bool IngestDeploymentArtifact(const PythonCustomOpDeploymentArtifactView &view) {
    auto artifact = std::make_shared<PythonCustomOpDeploymentArtifact>();
    // context 仅为错误日志拼装上下文（artifact / op_type[...]），先于 artifact_id 拷贝成功
    std::string context = " artifact";
    if (!CopyStringFieldOrLog(view.artifact_id, false, "artifact id", context, artifact->artifact_id)) {
      return false;
    }
    // source_name 只用于诊断，允许为空
    if (!CopyStringFieldOrLog(view.source_name, true, "source name", context, artifact->source_name)) {
      return false;
    }
    if ((view.source_sha256 == nullptr) || (view.source_sha256_size != PythonCustomOpDeploymentArtifact::kSha256Len)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] invalid sha256 length[%zu].", view.source_sha256_size);
      return false;
    }
    (void)memcpy_s(artifact->source_sha256.data(), artifact->source_sha256.size(), view.source_sha256,
                   view.source_sha256_size);
    if ((view.source == nullptr) || (view.source_size == 0U)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] source is empty.",
             artifact->artifact_id.c_str());
      return false;
    }
    if ((view.descriptors == nullptr) || (view.descriptor_count == 0U)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] has no descriptor.",
             artifact->artifact_id.c_str());
      return false;
    }

    artifact->descriptors.reserve(view.descriptor_count);
    for (size_t i = 0U; i < view.descriptor_count; ++i) {
      const auto &descriptor_view = view.descriptors[i];
      PythonOpDescriptorSnapshot snapshot;
      if (!CopyPythonCustomOpStringView(descriptor_view.op_type, false, snapshot.op_type) ||
          !CopyPythonCustomOpStringView(descriptor_view.class_name, false, snapshot.class_name)) {
        GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] has an invalid op type or class name.",
               artifact->artifact_id.c_str());
        return false;
      }
      context = " op_type[" + snapshot.op_type + "]";
      snapshot.capabilities = descriptor_view.capabilities;
      // canonical IR 快照（保存侧冻结）：旧 bridge SO 无该数据时指针为空，保持为空 vector
      if (descriptor_view.ir_input_count > 0U) {
        if (descriptor_view.ir_inputs == nullptr) {
          GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] op type[%s] has null ir inputs.",
                 artifact->artifact_id.c_str(), snapshot.op_type.c_str());
          return false;
        }
        snapshot.ir_inputs.reserve(descriptor_view.ir_input_count);
        for (size_t j = 0U; j < descriptor_view.ir_input_count; ++j) {
          PythonIrInputSnapshot ir_input;
          if (!CopyStringFieldOrLog(descriptor_view.ir_inputs[j].name, false, "ir input name", context,
                                    ir_input.name)) {
            return false;
          }
          ir_input.kind = descriptor_view.ir_inputs[j].kind;
          snapshot.ir_inputs.push_back(std::move(ir_input));
        }
      }
      if (descriptor_view.ir_attr_count > 0U) {
        if (descriptor_view.ir_attrs == nullptr) {
          GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] op type[%s] has null ir attrs.",
                 artifact->artifact_id.c_str(), snapshot.op_type.c_str());
          return false;
        }
        snapshot.ir_attrs.reserve(descriptor_view.ir_attr_count);
        for (size_t j = 0U; j < descriptor_view.ir_attr_count; ++j) {
          PythonIrAttrSnapshot ir_attr;
          if (!CopyStringFieldOrLog(descriptor_view.ir_attrs[j].name, false, "ir attr name", context, ir_attr.name) ||
              !CopyStringFieldOrLog(descriptor_view.ir_attrs[j].type, false, "ir attr type", context, ir_attr.type)) {
            return false;
          }
          snapshot.ir_attrs.push_back(std::move(ir_attr));
        }
      }
      if (descriptor_view.ir_output_count > 0U) {
        if (descriptor_view.ir_outputs == nullptr) {
          GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] op type[%s] has null ir outputs.",
                 artifact->artifact_id.c_str(), snapshot.op_type.c_str());
          return false;
        }
        snapshot.ir_outputs.reserve(descriptor_view.ir_output_count);
        for (size_t j = 0U; j < descriptor_view.ir_output_count; ++j) {
          PythonIrOutputSnapshot ir_output;
          if (!CopyStringFieldOrLog(descriptor_view.ir_outputs[j].name, false, "ir output name", context,
                                    ir_output.name)) {
            return false;
          }
          ir_output.kind = descriptor_view.ir_outputs[j].kind;
          snapshot.ir_outputs.push_back(std::move(ir_output));
        }
      }
      artifact->descriptors.push_back(std::move(snapshot));
    }

    auto source_buffer = std::make_shared<std::vector<uint8_t>>(view.source, view.source + view.source_size);
    artifact->source = std::move(source_buffer);
    // bridge ABI 版本以 loader 侧常量为准，不信任跨 SO 传来的值
    artifact->bridge_abi_version = kPythonCustomOpBridgeAbiVersion;
    artifact->producer_python_major = view.producer_python_major;
    artifact->producer_python_minor = view.producer_python_minor;
    // 逐字段不变量（id↔sha256、source 非空且≤上限、descriptor 列表合法）与保存侧
    // AddArtifact / codec 解析侧共用 ValidateArtifact 一处实现（R2-4）。
    if (PythonCustomOpPackageBuilder::ValidateArtifact(*artifact) != SUCCESS) {
      GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact[%s] failed shared validation.",
             artifact->artifact_id.c_str());
      return false;
    }

    const auto existing = artifacts_by_id_.find(artifact->artifact_id);
    if (existing != artifacts_by_id_.cend()) {
      // 同一 artifact_id 必须对应完全相同的内容，否则说明摘要或冻结逻辑内部不一致
      if (!PythonCustomOpPackageBuilder::IsSameDeploymentArtifact(*(existing->second), *artifact)) {
        GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] artifact id[%s] is reused by inconsistent content.",
               artifact->artifact_id.c_str());
        return false;
      }
      return true;
    }
    for (const auto &descriptor : artifact->descriptors) {
      // 原型声明条目不参与 op_type 覆盖判定：分文件原型下同一 op_type 合法地同时
      // 出现在原型 artifact（声明）与实现 artifact（实现身份）中。原型 artifact 不
      // 携带实现，也不应独占地占住 op_type——实现侧 artifact 才是交递目标，
      // 但原型 artifact 必须另行索引并在交递时随行走（见 proto_artifacts_by_op_type_）。
      if (IsProtoDescriptor(descriptor)) {
        proto_artifacts_by_op_type_[descriptor.op_type] = artifact;
        continue;
      }
      if (!artifact_by_op_type_.emplace(descriptor.op_type, artifact).second) {
        GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact] op type[%s] is covered by more than one artifact.",
               descriptor.op_type.c_str());
        return false;
      }
    }
    (void)artifacts_by_id_.emplace(artifact->artifact_id, std::move(artifact));
    return true;
  }

  static bool IngestScopedDescriptorFromBridge(void *user_data, const PythonCustomOpScopedDescriptorView *view,
                                               const PythonCustomOpAdapterCallbacks *callbacks) {
    auto *descriptors = static_cast<std::vector<PythonCustomOpScopedDescriptor> *>(user_data);
    return IngestScopedDescriptor(descriptors, view, callbacks);
  }

  // view 是同步借用：在此深拷贝进 sink user_data 指向的目标 vector（调用方栈对象）。
  // ingest 运行在 RestoreModelScope 持有的 mutex_ 之内，故不再加锁。callbacks 是稳定
  // 函数指针表，按值拷贝即可（指针指向已加载 bridge SO，模型卸载前有效）。
  static bool IngestScopedDescriptor(std::vector<PythonCustomOpScopedDescriptor> *descriptors,
                                     const PythonCustomOpScopedDescriptorView *view,
                                     const PythonCustomOpAdapterCallbacks *callbacks) {
    if ((view == nullptr) || (descriptors == nullptr)) {
      return false;
    }
    // impl descriptor 必须携带 create_impl_holder 回调，否则恢复出的 adapter 无法建 holder
    if ((callbacks == nullptr) || (callbacks->create_impl_holder == nullptr)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpScopedDescriptor] missing create_impl_holder callback.");
      return false;
    }
    PythonCustomOpScopedDescriptor descriptor;
    if (!CopyPythonCustomOpStringView(view->op_type, false, descriptor.op_type) ||
        !CopyPythonCustomOpStringView(view->impl_descriptor_key, false, descriptor.impl_descriptor_key) ||
        !CopyPythonCustomOpStringView(view->class_name, false, descriptor.class_name) ||
        !CopyPythonCustomOpStringView(view->class_qualname, false, descriptor.class_qualname)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpScopedDescriptor] invalid string field.");
      return false;
    }
    descriptor.capabilities = view->capabilities;
    descriptor.callbacks = *callbacks;
    descriptors->push_back(std::move(descriptor));
    return true;
  }

  static bool IngestProtoOpTypeFromBridge(void *user_data, const PythonCustomOpStringView *view) {
    auto *op_types = static_cast<std::vector<std::string> *>(user_data);
    if ((view == nullptr) || (op_types == nullptr)) {
      return false;
    }
    std::string op_type;
    if (!CopyPythonCustomOpStringView(*view, false, op_type)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpProtoOpType] invalid op type string.");
      return false;
    }
    op_types->push_back(std::move(op_type));
    return true;
  }

  Status RegisterCustomOpsFromBridge() {
    static constexpr PythonCustomOpRegistrar kRegistrar = {
        &RegisterOpProtoFromBridge,
        &RegisterOpImplFromBridge,
    };
    GELOGI("Register python custom ops with bridge library[%s].", loaded_path_.c_str());
    const auto ret = api_->register_custom_ops(&kRegistrar);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Register][PythonCustomOps] failed with bridge library[%s].", loaded_path_.c_str());
      return ret;
    }
    return CommitPythonCustomOpRegistrations();
  }

  static bool RegisterOpProtoFromBridge(const PythonCustomOpProtoDescriptorView *desc) {
    if (desc == nullptr) {
      return false;
    }
    return GetInstance().RegisterOpProto(*desc);
  }

  static bool RegisterOpImplFromBridge(const PythonCustomOpAdapterDescriptorView *desc,
                                       const PythonCustomOpAdapterCallbacks *callbacks) {
    if ((desc == nullptr) || (callbacks == nullptr)) {
      return false;
    }
    return GetInstance().RegisterOpImpl(*desc, *callbacks);
  }

  bool RegisterOpProto(const PythonCustomOpProtoDescriptorView &view) {
    PythonCustomOpProto proto;
    if (ParsePythonCustomOpProto(view, proto) != GRAPH_SUCCESS) {
      GELOGE(FAILED, "[Parse][PythonCustomOpProto] failed.");
      return false;
    }
    auto &registration = python_custom_op_registrations_[proto.op_type];
    if (registration.op_type.empty()) {
      registration.op_type = proto.op_type;
    }
    if (registration.has_proto) {
      if (registration.proto_descriptor_key == proto.descriptor_key) {
        return true;
      }
      GELOGE(FAILED,
             "Python custom op proto conflict, op type[%s], existing source[descriptor key:%s], "
             "current source[descriptor key:%s].",
             proto.op_type.c_str(), registration.proto_descriptor_key.c_str(), proto.descriptor_key.c_str());
      return false;
    }
    if (RegisterPythonCustomOpProto(proto) != GRAPH_SUCCESS) {
      GELOGE(FAILED, "[Register][PythonCustomOpProto] failed, descriptor key[%s], op type[%s].",
             proto.descriptor_key.c_str(), proto.op_type.c_str());
      return false;
    }
    registration.proto_descriptor_key = proto.descriptor_key;
    registration.has_proto = true;
    registration.infer_meta = proto.infer_meta;
    return true;
  }

  // 跨 SO 字段拷贝的统一错误模板（R2-5）：字段名 + 调用方上下文一条 GELOGE，
  // 语义判定（非空/无 \0/size 与 data 配对）由 bridge_types.h 的共享入口保证（R2-4）。
  static bool CopyStringFieldOrLog(const PythonCustomOpStringView &view, const bool allow_empty, const char *field,
                                   const std::string &context, std::string &value) {
    if (!CopyPythonCustomOpStringView(view, allow_empty, value)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpDeploymentArtifact]%s has an invalid %s.", context.c_str(), field);
      return false;
    }
    return true;
  }

  bool ParseAdapterDescriptor(const PythonCustomOpAdapterDescriptorView &view,
                              PythonCustomOpAdapterDescriptor &desc) const {
    if (!CopyPythonCustomOpStringView(view.op_type, false, desc.op_type) ||
        !CopyPythonCustomOpStringView(view.impl_descriptor_key, false, desc.impl_descriptor_key)) {
      return false;
    }
    desc.capabilities = view.capabilities;
    return true;
  }

  bool RegisterOpImpl(const PythonCustomOpAdapterDescriptorView &view,
                      const PythonCustomOpAdapterCallbacks &callbacks) {
    PythonCustomOpAdapterDescriptor desc;
    if (!ParseAdapterDescriptor(view, desc)) {
      GELOGE(FAILED, "[Parse][PythonCustomOpAdapter] failed.");
      return false;
    }
    auto &registration = python_custom_op_registrations_[desc.op_type];
    if (registration.op_type.empty()) {
      registration.op_type = desc.op_type;
    }
    if (registration.has_impl) {
      if (registration.impl_desc.impl_descriptor_key == desc.impl_descriptor_key) {
        return true;
      }
      GELOGE(FAILED,
             "Python custom op adapter conflict, op type[%s], existing source[impl key:%s], "
             "current source[impl key:%s].",
             desc.op_type.c_str(), registration.impl_desc.impl_descriptor_key.c_str(),
             desc.impl_descriptor_key.c_str());
      return false;
    }
    if (CustomOpFactory::IsExistOp(AscendString(desc.op_type.c_str()), OpRegistrationPriority::kTop,
                                   OpEngine::kCustom)) {
      GELOGE(FAILED,
             "[Check][PythonCustomOpAdapter]Op type[%s] conflicts, existing source[CustomOpFactory creator], "
             "current source[Python impl descriptor key:%s].",
             desc.op_type.c_str(), desc.impl_descriptor_key.c_str());
      return false;
    }
    if (!callbacks.IsValid(desc.capabilities)) {
      GELOGE(FAILED, "Invalid python custom op implementation, descriptor key[%s], op type[%s].",
             desc.impl_descriptor_key.c_str(), desc.op_type.c_str());
      return false;
    }
    registration.has_impl = true;
    registration.impl_desc = desc;
    registration.callbacks = callbacks;
    return true;
  }

  Status RegisterPythonCustomOpImpls() {
    for (const auto &item : python_custom_op_registrations_) {
      const auto &registration = item.second;
      if (!registration.has_impl ||
          (registered_op_type_to_adapter_.find(registration.op_type) != registered_op_type_to_adapter_.cend())) {
        continue;
      }
      if (!PythonCustomOpImplRuntimeRegistry::Register(registration.impl_desc, registration.callbacks)) {
        GELOGE(FAILED, "[Register][PythonCustomOpImplRuntimeRegistry] failed, descriptor key[%s], op type[%s].",
               registration.impl_desc.impl_descriptor_key.c_str(), registration.op_type.c_str());
        return FAILED;
      }
    }
    return SUCCESS;
  }

  bool BuildAdapterDescriptor(const PythonCustomOpRegistrationEntry &registration,
                              PythonCustomOpAdapterDescriptor &desc) const {
    desc.op_type = registration.op_type;
    if (registration.has_impl) {
      desc = registration.impl_desc;
    }
    if (registration.has_proto) {
      desc.infer_meta = registration.infer_meta;
      AddCustomOpCapability(desc.capabilities, CustomOpCapability::kShapeInfer);
      AddCustomOpCapability(desc.capabilities, CustomOpCapability::kInferMeta);
    }
    const auto infer_capabilities = static_cast<CustomOpCapabilityMask>(CustomOpCapability::kShapeInfer) |
                                    static_cast<CustomOpCapabilityMask>(CustomOpCapability::kInferMeta);
    const auto impl_capabilities = desc.capabilities & ~infer_capabilities;
    if ((desc.capabilities == 0U) ||
        (HasCustomOpCapability(desc.capabilities, CustomOpCapability::kInferMeta) && (desc.infer_meta == nullptr)) ||
        ((impl_capabilities != 0U) && desc.impl_descriptor_key.empty()) ||
        ((impl_capabilities == 0U) && !desc.impl_descriptor_key.empty())) {
      GELOGE(FAILED, "Invalid python custom op adapter descriptor, op type[%s].", registration.op_type.c_str());
      return false;
    }

    // 冻结出的部署 artifact 随 descriptor 携带，供保存侧打包进 PYTHON_CUSTOM_OPS。
    // 未冻结（含声明 portable 却无 artifact）不在此检查，恢复失败延后到加载/执行期暴露。
    const auto artifact_iter = artifact_by_op_type_.find(desc.op_type);
    if (artifact_iter != artifact_by_op_type_.cend()) {
      desc.deployment_artifacts.push_back(artifact_iter->second);
    }
    // 分文件原型：声明所在的 artifact 与实现 artifact 是两个文件，必须一并交出。
    // 只交实现 artifact 时 proto.py 进不了 OM，加载后 @register_op 从不执行、
    // 声明集合为空，能力位补位分支不进，动态 shape 推导静默缺席（且无日志）。
    const auto proto_iter = proto_artifacts_by_op_type_.find(desc.op_type);
    if (proto_iter != proto_artifacts_by_op_type_.cend()) {
      desc.deployment_artifacts.push_back(proto_iter->second);
    }
    // 打包契约要求 artifact_id 升序，与 PythonPackageBuilder 的 sources 排序一致。
    std::sort(desc.deployment_artifacts.begin(), desc.deployment_artifacts.end(),
              [](const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &lhs,
                 const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &rhs) {
                return lhs->artifact_id < rhs->artifact_id;
              });
    return true;
  }

  Status RegisterPythonCustomOpCreator(const PythonCustomOpAdapterDescriptor &desc, const OpBackend backend) const {
    const auto ret = CustomOpFactory::RegisterCustomOpCreator(
        AscendString(desc.op_type.c_str()), backend, [registered_desc = desc]() -> std::unique_ptr<BaseCustomOp> {
          auto *adapter = new (std::nothrow) PythonCustomOpAdapter(registered_desc);
          if ((adapter == nullptr) || (!adapter->IsValid())) {
            delete adapter;
            return std::unique_ptr<BaseCustomOp>();
          }
          return std::unique_ptr<BaseCustomOp>(adapter);
        });
    if (ret != GRAPH_SUCCESS) {
      GELOGE(ret, "Register python custom op creator failed, op type[%s].", desc.op_type.c_str());
      return FAILED;
    }
    GELOGI("Python custom op creator is registered, op type[%s], backend[%u].", desc.op_type.c_str(),
           static_cast<uint32_t>(backend));
    return SUCCESS;
  }

  Status RegisterPythonCustomOpCreators() {
    for (const auto &item : python_custom_op_registrations_) {
      const auto &registration = item.second;
      const auto &op_type = registration.op_type;
      if (registered_op_type_to_adapter_.find(op_type) != registered_op_type_to_adapter_.cend()) {
        continue;
      }
      if (CustomOpFactory::IsExistOp(AscendString(op_type.c_str()), OpRegistrationPriority::kTop, OpEngine::kCustom)) {
        GELOGE(FAILED, "Python custom op conflict, op type[%s], existing source[CustomOpFactory creator].",
               op_type.c_str());
        return FAILED;
      }

      PythonCustomOpAdapterDescriptor desc;
      if (!BuildAdapterDescriptor(registration, desc)) {
        return FAILED;
      }
      const auto device_capabilities = static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute) |
                                       static_cast<CustomOpCapabilityMask>(CustomOpCapability::kArgsUpdater) |
                                       static_cast<CustomOpCapabilityMask>(CustomOpCapability::kAnnotatedArgs);
      const bool has_device_capability = (desc.capabilities & device_capabilities) != 0U;
      const bool has_host_capability = HasCustomOpCapability(desc.capabilities, CustomOpCapability::kHostCpuExecute);
      if (has_device_capability) {
        if (RegisterPythonCustomOpCreator(desc, OpBackend::kDevice) != SUCCESS) {
          return FAILED;
        }
      }
      if (has_host_capability) {
        if (RegisterPythonCustomOpCreator(desc, OpBackend::kHostCPU) != SUCCESS) {
          if (has_device_capability) {
            // The device creator is not committed to the ownership set
            CustomOpFactory::RemoveCustomOps({AscendString(op_type.c_str())});
          }
          return FAILED;
        }
      }
      // Keep the existing device anchor behavior for descriptors that only provide common capabilities.
      if (!has_device_capability && !has_host_capability && (desc.capabilities != 0U)) {
        if (RegisterPythonCustomOpCreator(desc, OpBackend::kDevice) != SUCCESS) {
          return FAILED;
        }
      }
      registered_op_type_to_adapter_.insert(op_type);
    }
    return SUCCESS;
  }

  Status CommitPythonCustomOpRegistrations() {
    if (RegisterPythonCustomOpImpls() != SUCCESS) {
      return FAILED;
    }
    return RegisterPythonCustomOpCreators();
  }

  void ClearRegisteredState() {
    python_custom_op_registrations_.clear();
    registered_op_type_to_adapter_.clear();
    artifacts_by_id_.clear();
    artifact_by_op_type_.clear();
    proto_artifacts_by_op_type_.clear();
  }

  Status EnsureLoaded() {
    if (api_ != nullptr) {
      GELOGI("Reuse already loaded python custom op bridge library[%s].", loaded_path_.c_str());
      return SUCCESS;
    }
    const auto runtime_key = artifact::ResolveLoadedPythonRuntimeKey();
    if (!runtime_key.has_python_symbols) {
      GELOGE(FAILED, "[Check][PythonRuntime]Python symbols are not loaded before custom op preprocessing, runtime[%s].",
             runtime_key.ToString().c_str());
      return FAILED;
    }
    if (!runtime_key.is_initialized) {
      GELOGE(FAILED,
             "[Check][PythonRuntime]Python interpreter is not initialized before custom op preprocessing, "
             "runtime[%s].",
             runtime_key.ToString().c_str());
      return FAILED;
    }
    GELOGI("Python custom op runtime key before loading bridge: %s.", runtime_key.ToString().c_str());
    if (TryLoadPrebuiltBridge(runtime_key) || TryLoadFallbackBridge(runtime_key)) {
      return SUCCESS;
    }
    GELOGE(FAILED, "Load python custom op bridge library failed.");
    return FAILED;
  }

  bool TryLoadPrebuiltBridge(const artifact::PythonRuntimeKey &runtime_key) {
    const auto candidates = artifact::BuildPrebuiltBridgeLibraryCandidates(
        runtime_key, GetLoaderLibraryPath(), kCustomOpArtifactsRelativePath, kPythonCustomOpBridgeAbiVersion);
    return TryLoadBridgeCandidates(runtime_key, candidates);
  }

  bool TryLoadFallbackBridge(const artifact::PythonRuntimeKey &runtime_key) {
    std::string gen_artifact_root;
    if (!fallback_codegen::RunFallbackCodegenForModule(
            runtime_key, fallback_codegen::BuildFallbackCodegenDependencies(), "ge.custom_op.fallback_runtime",
            "run_fallback_codegen", gen_artifact_root)) {
      return false;
    }
    const auto candidate =
        artifact::LoadBridgeCandidateFromArtifactRoot(gen_artifact_root, runtime_key, kPythonCustomOpBridgeAbiVersion);
    if (candidate.bridge_path.empty()) {
      GELOGE(FAILED, "Python custom op fallback generated an invalid artifact root[%s].", gen_artifact_root.c_str());
      return false;
    }
    return TryLoadBridgeCandidates(runtime_key, {candidate});
  }

  bool TryLoadBridgeCandidates(const artifact::PythonRuntimeKey &runtime_key,
                               const std::vector<artifact::BridgeLibraryCandidate> &candidates) {
    for (const auto &candidate : candidates) {
      bridge_loader::LoadedBridgeCandidate<PythonCustomOpBridgeApi> loaded_bridge;
      const auto deps = BuildBridgeLoadDependencies();
      const auto status =
          bridge_loader::TryLoadBridgeCandidate<PythonCustomOpBridgeApi, PythonCustomOpBridgeArtifactConfig>(
              runtime_key, candidate, deps, &IsBridgeApiValid, loaded_bridge);
      if (status != bridge_loader::BridgeLoadStatus::kSuccess) {
        const auto error_suffix = bridge_loader::BuildBridgeLoadErrorSuffix(status, dlerror());
        GELOGW(
            "Skip python custom op bridge candidate[%s], artifact_root[%s], native_module[%s], "
            "expected bridge ABI[%u], status[%s]%s.",
            candidate.bridge_path.c_str(), candidate.artifact_root.c_str(), candidate.native_module_path.c_str(),
            kPythonCustomOpBridgeAbiVersion, bridge_loader::BridgeLoadStatusToString(status), error_suffix.c_str());
        continue;
      }
      handle_ = loaded_bridge.handle;
      api_ = loaded_bridge.api;
      loaded_path_ = loaded_bridge.real_path;
      GELOGI("Load python custom op bridge from [%s] success.", loaded_path_.c_str());
      return true;
    }
    return false;
  }

  std::mutex mutex_;
  void *handle_{nullptr};
  const PythonCustomOpBridgeApi *api_{nullptr};
  std::string loaded_path_;
  std::map<std::string, PythonCustomOpRegistrationEntry> python_custom_op_registrations_;
  std::set<std::string> registered_op_type_to_adapter_;
  // 编译期冻结的部署 artifact：按 artifact_id 去重持有，并按 op_type 建立反查索引。
  // 两个 map 持有同一批 shared_ptr，artifact 本身 immutable。
  std::map<std::string, std::shared_ptr<const PythonCustomOpDeploymentArtifact>> artifacts_by_id_;
  std::map<std::string, std::shared_ptr<const PythonCustomOpDeploymentArtifact>> artifact_by_op_type_;
  // 分文件原型：op_type → 承载 @register_op 声明的原型 artifact。与 artifact_by_op_type_
  // （实现 artifact）分开索引，交递时两者一并交给 descriptor——只交实现侧会让 proto.py
  // 进不了 OM，加载后无 infer_func，动态 shape 推导静默缺席。
  std::map<std::string, std::shared_ptr<const PythonCustomOpDeploymentArtifact>> proto_artifacts_by_op_type_;
};
}  // namespace

Status CheckNeedLoadPythonCustomOps(bool &need_load) {
  need_load = false;
  const char_t *custom_opp_path_env = nullptr;
  MM_SYS_GET_ENV(MM_ENV_ASCEND_CUSTOM_OPP_PATH, custom_opp_path_env);
  if ((custom_opp_path_env == nullptr) || (custom_opp_path_env[0] == '\0')) {
    GELOGI("Skip loading python custom ops because ASCEND_CUSTOM_OPP_PATH is empty.");
    return SUCCESS;
  }
  const auto ret = FindPythonCustomOpEntryInEnv(custom_opp_path_env, need_load);
  if (ret != SUCCESS) {
    return ret;
  }
  if (!need_load) {
    GELOGI(
        "Skip loading python custom ops because no loadable python custom op entry is found in "
        "ASCEND_CUSTOM_OPP_PATH.");
  }
  return SUCCESS;
}

Status LoadPythonCustomOps() {
  return PythonCustomOpBridgeLoader::GetInstance().Load();
}

Status RestorePythonCustomOpModelScope(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources,
                                       size_t source_count, std::vector<PythonCustomOpScopedDescriptor> &descriptors) {
  return PythonCustomOpBridgeLoader::GetInstance().RestoreModelScope(scope_id, sources, source_count, descriptors);
}

Status CollectPythonCustomOpModelScopeProtoOpTypes(uint64_t scope_id, std::vector<std::string> &op_types) {
  return PythonCustomOpBridgeLoader::GetInstance().CollectModelScopeProtoOpTypes(scope_id, op_types);
}

void ClosePythonCustomOpModelScope(uint64_t scope_id) {
  PythonCustomOpBridgeLoader::GetInstance().CloseModelScope(scope_id);
}

void UnloadPythonCustomOps() {
  PythonCustomOpBridgeLoader::GetInstance().Unload();
}

void ShutdownPythonCustomOpsForProcess() {
  PythonCustomOpBridgeLoader::GetInstance().ShutdownForProcess();
}
}  // namespace custom_op
}  // namespace ge
