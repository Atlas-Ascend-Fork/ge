/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "runtime/custom_op/python_custom_op_restore_hook.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/helper/python_custom_op_package_helper.h"
#include "common/python_runtime/ge_python_runtime_manager.h"
#include "framework/common/debug/ge_log.h"
#include "graph/ascend_string.h"
#include "graph/custom_op.h"
#include "graph/custom_op/capability.h"
#include "graph/custom_op/model_scope.h"
#include "graph/custom_op_registry.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "runtime/custom_op/python_custom_op_bridge_c_api.h"
#include "runtime/custom_op/python_custom_op_bridge_loader.h"

namespace ge {
namespace custom_op {
namespace {
// scope_id 权威分配点：进程内 64-bit 单调递增、永不回收。
// 从 1 起（Python 侧拒绝非正 id）；即便反复加载/卸载同一 OM，id 也永不复用，
// 使 module 名 `_ge_om_custom_op_<scope_id>_<artifact_id>` 与 scoped descriptor_key 进程内唯一。
std::atomic<uint64_t> g_next_scope_id{0U};

uint64_t AllocateScopeId() {
  return ++g_next_scope_id;
}

// 进程级 op_type → artifact_id 台账：一个进程内同名自定义算子必须用同一实现。
//
// C++ 自定义算子对这条规则有硬防护（ScopedOpProtoLoadTxn 的 SO 指纹冲突判定 →
// GELOGE FAILED，报错点名 incumbent/challenger），而 Python 恢复段不进入该事务；
// 若不在这里补上，两个模型带同名但实现不同的 Python 算子时会双双恢复成功，
// 之后 find_scoped_proto / find_scoped_ir_meta 按 op_type 在全活 scope 里首个命中
// 即返回，后加载的模型静默用前一个模型的 infer_func / IR schema —— 推导出错误
// shape 或按错误 schema 绑定参数，且零日志。
//
// 判据用 artifact_id（= 定义文件字节的 sha256）：同一 op_type 来自同一源文件即放行，
// 来自不同源文件即冲突。台账只在 Restore 成功时提交，随 scope 关闭按引用计数回收，
// 因此反复加载/卸载不残留。
struct PythonOpTypeLedgerEntry {
  std::string artifact_id;
  size_t ref_count{0U};
};

std::mutex g_python_op_type_ledger_mutex;
std::map<std::string, PythonOpTypeLedgerEntry> g_python_op_type_ledger;

// 一条跨模型冲突记录：本模型要用的 artifact_id 与进程内已占用的不同。
struct PythonOpTypeConflict {
  std::string op_type;
  std::string incumbent_artifact_id;
  std::string challenger_artifact_id;
};

// 判定本模型声明的 op_type 与进程级台账是否冲突；不修改台账。
bool DetectPythonOpTypeConflicts(const std::map<std::string, std::string> &op_type_to_artifact_id,
                                 std::vector<PythonOpTypeConflict> &conflicts) {
  std::lock_guard<std::mutex> lock(g_python_op_type_ledger_mutex);
  for (const auto &item : op_type_to_artifact_id) {
    const auto iter = g_python_op_type_ledger.find(item.first);
    if ((iter != g_python_op_type_ledger.cend()) && (iter->second.artifact_id != item.second)) {
      conflicts.push_back(PythonOpTypeConflict{item.first, iter->second.artifact_id, item.second});
    }
  }
  return conflicts.empty();
}

void CommitPythonOpTypeClaims(const std::map<std::string, std::string> &op_type_to_artifact_id) {
  std::lock_guard<std::mutex> lock(g_python_op_type_ledger_mutex);
  for (const auto &item : op_type_to_artifact_id) {
    auto &entry = g_python_op_type_ledger[item.first];
    entry.artifact_id = item.second;
    ++entry.ref_count;
  }
}

void ReleasePythonOpTypeClaims(const std::map<std::string, std::string> &op_type_to_artifact_id) {
  std::lock_guard<std::mutex> lock(g_python_op_type_ledger_mutex);
  for (const auto &item : op_type_to_artifact_id) {
    const auto iter = g_python_op_type_ledger.find(item.first);
    if (iter == g_python_op_type_ledger.cend()) {
      continue;
    }
    if (iter->second.ref_count <= 1U) {
      (void)g_python_op_type_ledger.erase(iter);
    } else {
      --iter->second.ref_count;
    }
  }
}

// 加载态模型 scope：不持 registry 强引用（registry 持 scope），只承载 scope_id 与恢复期
// 注册进进程级 RuntimeRegistry 的 descriptor_key 列表，供 Close() 精确回收（反复加载/卸载
// 不残留）。Close() 幂等，须在解释器仍存活时调用——registry 析构顺序保证 instance/creator
// 先于 scope 释放（custom_op_registry.cc 显式析构）；解释器已 finalize 的兜底由 bridge 侧
// CloseModelScope 的解释器保护承担。
class PythonCustomOpModelScope : public CustomOpModelScope {
 public:
  explicit PythonCustomOpModelScope(uint64_t scope_id) : scope_id_(scope_id) {}
  ~PythonCustomOpModelScope() override {
    Close();
  }

  PythonCustomOpModelScope(const PythonCustomOpModelScope &) = delete;
  PythonCustomOpModelScope &operator=(const PythonCustomOpModelScope &) = delete;

  void Close() override {
    if (closed_) {
      return;
    }
    closed_ = true;
    // holder 已随 registry 析构（active_adapter_count 归零），此处 Unregister 精确回收
    // 进程级 RuntimeRegistry 条目；best-effort，失败只告警（可能仍被引用则留待进程退出）。
    for (const auto &descriptor_key : registered_descriptor_keys_) {
      if (!PythonCustomOpImplRuntimeRegistry::Unregister(descriptor_key)) {
        GELOGW("[PythonCustomOp][Restore] Unregister runtime descriptor key[%s] of scope[%llu] was not clean.",
               descriptor_key.c_str(), static_cast<unsigned long long>(scope_id_));
      }
    }
    registered_descriptor_keys_.clear();
    // 归还本模型对进程级 op_type 台账的占用；引用计数归零即擦除，反复加载/卸载不残留。
    ReleasePythonOpTypeClaims(op_type_to_artifact_id_);
    op_type_to_artifact_id_.clear();
    // 关闭 Python 侧 scope：移除 sys.modules 中的 `_ge_om_custom_op_*` 模块并清理捕获状态。
    ClosePythonCustomOpModelScope(scope_id_);
    GELOGI("[PythonCustomOp][Restore] Closed model scope[%llu].", static_cast<unsigned long long>(scope_id_));
  }

  void AddRegisteredKey(std::string descriptor_key) {
    registered_descriptor_keys_.push_back(std::move(descriptor_key));
  }

  // 声明本模型占用的 op_type → artifact_id，供 Close() 时精确归还台账。
  void SetOpTypeToArtifactId(std::map<std::string, std::string> op_type_to_artifact_id) {
    op_type_to_artifact_id_ = std::move(op_type_to_artifact_id);
  }

  uint64_t GetScopeId() const {
    return scope_id_;
  }

 private:
  uint64_t scope_id_;
  std::vector<std::string> registered_descriptor_keys_;
  std::map<std::string, std::string> op_type_to_artifact_id_;
  bool closed_{false};
};

// manifest 中一个 op_type 对应的期望 descriptor 身份：(class_qualname, capabilities)。
// manifest 的 class_name 字段存的即 __qualname__（_deployment._build_snapshot），
// capabilities 由 Python capabilities_from_interfaces 产出，与 C++ ParseInterfaces 逐位一致。
struct ManifestDescriptor {
  std::string class_qualname;
  CustomOpCapabilityMask capabilities{0U};
};

// backend 分派镜像编译期 RegisterPythonCustomOpCreators：device 能力（eager/args_updater/
// annotated_args）注册 kDevice，host_cpu_execute 注册 kHostCPU；仅公共能力（如 portable/
// compilable）以 kDevice 作锚点；scope 内声明了同 op_type 原型时 capabilities 还含
// shape_infer/infer_meta，这类只有推导能力的条目同样以 kDevice 作锚点。
// creator 闭包按值捕获 adapter_desc（含 scoped descriptor_key），不持 registry/scope 强引用，
// 不形成生命周期环；holder 在闭包首次运行（Deserialize 或执行）时惰性创建。
Status RegisterModelCreators(const CustomOpRegistryPtr &registry, const PythonCustomOpAdapterDescriptor &adapter_desc) {
  const BaseOpCreator creator = [adapter_desc]() -> std::unique_ptr<BaseCustomOp> {
    auto *adapter = new (std::nothrow) PythonCustomOpAdapter(adapter_desc);
    if ((adapter == nullptr) || (!adapter->IsValid())) {
      delete adapter;
      return std::unique_ptr<BaseCustomOp>();
    }
    return std::unique_ptr<BaseCustomOp>(adapter);
  };
  const AscendString op_type(adapter_desc.op_type.c_str());
  const auto device_capabilities = static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute) |
                                   static_cast<CustomOpCapabilityMask>(CustomOpCapability::kArgsUpdater) |
                                   static_cast<CustomOpCapabilityMask>(CustomOpCapability::kAnnotatedArgs);
  const bool has_device = (adapter_desc.capabilities & device_capabilities) != 0U;
  const bool has_host = HasCustomOpCapability(adapter_desc.capabilities, CustomOpCapability::kHostCpuExecute);

  auto register_one = [&](OpBackend backend) -> Status {
    const auto ret = registry->RegisterCreator(op_type, backend, creator);
    if (ret != GRAPH_SUCCESS) {
      GELOGE(FAILED, "[PythonCustomOp][Restore] Register model creator failed, op type[%s], backend[%u], status[%u].",
             adapter_desc.op_type.c_str(), static_cast<uint32_t>(backend), static_cast<uint32_t>(ret));
      return FAILED;
    }
    return SUCCESS;
  };

  if (has_device) {
    GE_CHK_STATUS_RET(register_one(OpBackend::kDevice), "[PythonCustomOp][Restore] device creator failed.");
  }
  if (has_host) {
    if (register_one(OpBackend::kHostCPU) != SUCCESS) {
      // 已提交的 device creator 需回滚，避免半注册（RemoveCustomOps 清理该 op_type 全部 backend）
      registry->RemoveCustomOps({op_type});
      return FAILED;
    }
  }
  if ((!has_device) && (!has_host) && (adapter_desc.capabilities != 0U)) {
    GE_CHK_STATUS_RET(register_one(OpBackend::kDevice), "[PythonCustomOp][Restore] anchor creator failed.");
  }
  return SUCCESS;
}

class PythonCustomOpRestoreHookImpl : public PythonCustomOpRestoreHook {
 public:
  graphStatus Restore(const uint8_t *data, size_t len, const CustomOpRegistryPtr &registry) override {
    if (registry == nullptr) {
      GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Model registry is null.");
      return GRAPH_FAILED;
    }
    // 1) 结构校验（无副作用，安全边界）：全部校验通过后才准备 runtime、执行任何源码。
    ParsedPythonCustomOpPackage parsed;
    if (ParseAndValidatePythonCustomOpPackage(data, len, parsed) != SUCCESS) {
      GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Parse and validate PYTHON_CUSTOM_OPS partition failed.");
      return GRAPH_FAILED;
    }
    // manifest 索引：op_type 全局唯一（codec 已保证），每个 op_type 对应一个期望身份。
    // artifact 索引同步建立：恢复出的 adapter 携带 deployment_artifacts，使"加载→再保存"
    // （缓存落盘/级联导出）时 ClassifyCustomOp 仍判 kPythonDeploy，PYTHON_CUSTOM_OPS
    // 分区不因中转丢失（parsed.artifacts 是 Build 的逆产物，内容与编译期冻结一致）。
    // 必须收集**全部**与该 op_type 相关的 artifact：分文件原型时声明所在的 artifact
    // 与实现 artifact 是两个文件，只交出实现 artifact 会让 proto.py 在再保存时丢失。
    std::set<std::string> manifest_op_types;
    std::map<std::string, ManifestDescriptor> manifest_by_op_type;
    std::map<std::string, std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>>> artifacts_by_op_type;
    // 本模型对每个 op_type 的实现身份：artifact_id 即定义文件字节的 sha256，
    // 是跨模型冲突判定的指纹（与 C++ 侧 SO 指纹同口径）。
    std::map<std::string, std::string> op_type_to_artifact_id;
    // 原型声明条目（capabilities 恒为 kShapeInfer|kInferMeta，见 package_helper 的
    // IsProtoDescriptor）：只描述声明本身，不参与实现身份比对。
    std::set<std::string> proto_manifest_op_types;
    for (const auto &artifact : parsed.artifacts) {
      for (const auto &snapshot : artifact->descriptors) {
        auto &artifacts = artifacts_by_op_type[snapshot.op_type];
        // 同文件原型时声明与实现在同一个 artifact 里，按 artifact_id 去重。
        const bool already_collected =
            std::any_of(artifacts.cbegin(), artifacts.cend(),
                        [&artifact](const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &collected) {
                          return collected->artifact_id == artifact->artifact_id;
                        });
        if (!already_collected) {
          artifacts.push_back(artifact);
        }
        if (IsProtoDescriptor(snapshot)) {
          (void)proto_manifest_op_types.insert(snapshot.op_type);
          continue;
        }
        (void)manifest_op_types.insert(snapshot.op_type);
        manifest_by_op_type[snapshot.op_type] = ManifestDescriptor{snapshot.class_name, snapshot.capabilities};
        op_type_to_artifact_id[snapshot.op_type] = artifact->artifact_id;
      }
    }
    // 跨模型冲突判定（与 C++ 自定义算子同口径）：一个进程内同名自定义算子必须用同一实现。
    // 放在准备 runtime 与执行任何 embedded source 之前——冲突时本模型既不产生任何副作用，
    // 也不必先恢复再回滚。Python 恢复段不进入 ScopedOpProtoLoadTxn，这里补上同等防护。
    std::vector<PythonOpTypeConflict> conflicts;
    if (!DetectPythonOpTypeConflicts(op_type_to_artifact_id, conflicts)) {
      for (const auto &conflict : conflicts) {
        GELOGE(GRAPH_FAILED,
               "[PythonCustomOp][Restore] conflicting python custom op implementations for op type[%s]: incumbent "
               "artifact id[%s] (already loaded by another model in this process), challenger artifact id[%s].",
               conflict.op_type.c_str(), conflict.incumbent_artifact_id.c_str(),
               conflict.challenger_artifact_id.c_str());
      }
      GELOGE(GRAPH_FAILED,
             "[PythonCustomOp][Restore] load model failed: models in one process must use the same implementation for "
             "the same custom op type. Align custom op package versions and retry.");
      return GRAPH_FAILED;
    }
    // 2) 按需准备 Python runtime（附着已有解释器或加载受支持 libpython）。
    if (GePythonRuntimeManager::Instance().EnsureReady() != SUCCESS) {
      GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Ensure python runtime ready failed.");
      return GRAPH_FAILED;
    }
    // 3) 一次会话恢复整个 scope：open + 逐 artifact 内存初始化 embedded source
    //    （不落盘、不改 sys.path）+ 收集捕获的 impl descriptor。descriptor 级
    //    canonical IR 快照随 embedded source 一并传入 Python scope，供加载态
    //    schema-bound execute/compile 的参数绑定查询（加载态原型未注册）。
    //    视图仅同步借用：字符串与列表底层存储都在 parsed.artifacts 与本函数的
    //    view 存储（本次调用存活）。
    const uint64_t scope_id = AllocateScopeId();
    // 单个 artifact 的 IR 视图存储：view 持有指向这些 vector 堆缓冲的指针，
    // vector 本体 move 进外层存储后堆缓冲地址不变，指针保持有效。
    struct ArtifactIrViewStorage {
      std::vector<PythonCustomOpDeploymentDescriptorView> ir_def_views;
      std::vector<std::vector<PythonCustomOpIrInputDefView>> ir_input_views;
      std::vector<std::vector<PythonCustomOpIrAttrDefView>> ir_attr_views;
      std::vector<std::vector<PythonCustomOpIrOutputDefView>> ir_output_views;
    };
    std::vector<PythonCustomOpEmbeddedSourceView> source_views;
    std::vector<ArtifactIrViewStorage> ir_view_storages;
    source_views.reserve(parsed.artifacts.size());
    ir_view_storages.reserve(parsed.artifacts.size());
    for (const auto &artifact : parsed.artifacts) {
      // source 非空不变量由 codec 侧 ValidateArtifact 保证（ParseAndValidatePythonCustomOpPackage
      // 对每个 artifact 复用保存侧校验），此处不再复校。
      std::vector<PythonCustomOpDeploymentDescriptorView> ir_def_views;
      std::vector<std::vector<PythonCustomOpIrInputDefView>> ir_input_views(artifact->descriptors.size());
      std::vector<std::vector<PythonCustomOpIrAttrDefView>> ir_attr_views(artifact->descriptors.size());
      std::vector<std::vector<PythonCustomOpIrOutputDefView>> ir_output_views(artifact->descriptors.size());
      ir_def_views.reserve(artifact->descriptors.size());
      for (size_t i = 0U; i < artifact->descriptors.size(); ++i) {
        const auto &snapshot = artifact->descriptors[i];
        // 原型条目只描述声明，canonical IR 由实现侧 artifact 携带（同一 op_type 的
        // IR 快照必须全 scope 一致，见 _scope.initialize_source 的一致性校验）。
        if (IsProtoDescriptor(snapshot)) {
          continue;
        }
        auto &input_views = ir_input_views[i];
        auto &attr_views = ir_attr_views[i];
        auto &output_views = ir_output_views[i];
        input_views.reserve(snapshot.ir_inputs.size());
        for (const auto &ir_input : snapshot.ir_inputs) {
          input_views.push_back(PythonCustomOpIrInputDefView{
              PythonCustomOpStringView{ir_input.name.data(), ir_input.name.size()}, ir_input.kind});
        }
        attr_views.reserve(snapshot.ir_attrs.size());
        for (const auto &ir_attr : snapshot.ir_attrs) {
          attr_views.push_back(
              PythonCustomOpIrAttrDefView{PythonCustomOpStringView{ir_attr.name.data(), ir_attr.name.size()},
                                          PythonCustomOpStringView{ir_attr.type.data(), ir_attr.type.size()}});
        }
        output_views.reserve(snapshot.ir_outputs.size());
        for (const auto &ir_output : snapshot.ir_outputs) {
          output_views.push_back(PythonCustomOpIrOutputDefView{
              PythonCustomOpStringView{ir_output.name.data(), ir_output.name.size()}, ir_output.kind});
        }
        PythonCustomOpDeploymentDescriptorView ir_def;
        ir_def.op_type = PythonCustomOpStringView{snapshot.op_type.data(), snapshot.op_type.size()};
        ir_def.class_name = PythonCustomOpStringView{"", 0U};
        ir_def.capabilities = 0U;
        ir_def.ir_inputs = input_views.empty() ? nullptr : input_views.data();
        ir_def.ir_input_count = input_views.size();
        ir_def.ir_attrs = attr_views.empty() ? nullptr : attr_views.data();
        ir_def.ir_attr_count = attr_views.size();
        ir_def.ir_outputs = output_views.empty() ? nullptr : output_views.data();
        ir_def.ir_output_count = output_views.size();
        ir_def_views.push_back(ir_def);
      }
      PythonCustomOpEmbeddedSourceView view;
      view.artifact_id = PythonCustomOpStringView{artifact->artifact_id.data(), artifact->artifact_id.size()};
      view.source_name = PythonCustomOpStringView{artifact->source_name.data(), artifact->source_name.size()};
      view.source = artifact->source->data();
      view.source_size = artifact->source->size();
      view.ir_defs = ir_def_views.empty() ? nullptr : ir_def_views.data();
      view.ir_def_count = ir_def_views.size();
      source_views.push_back(view);
      ir_view_storages.push_back(ArtifactIrViewStorage{std::move(ir_def_views), std::move(ir_input_views),
                                                       std::move(ir_attr_views), std::move(ir_output_views)});
    }
    auto scope = std::make_shared<PythonCustomOpModelScope>(scope_id);
    bool committed = false;
    // 失败自回滚：关闭 Python scope + 回收 RuntimeRegistry 条目（creator 由被丢弃的 registry 析构清理）。
    auto rollback = [&]() {
      if (!committed) {
        scope->Close();
      }
    };
    struct Guard {
      std::function<void()> fn;
      ~Guard() {
        fn();
      }
    } guard{rollback};
    std::vector<PythonCustomOpScopedDescriptor> scoped_descriptors;
    if (RestorePythonCustomOpModelScope(scope_id, source_views.data(), source_views.size(), scoped_descriptors) !=
        SUCCESS) {
      GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Restore model scope[%llu] with %zu embedded source(s) failed.",
             static_cast<unsigned long long>(scope_id), source_views.size());
      return GRAPH_FAILED;
    }
    // 4) 原型声明：加载态不写进程级原型表（D4），但声明本身留在 scope 里，是恢复
    //    infer_func 的唯一来源。分文件原型（proto.py 声明、impl.py 实现）与同文件原型
    //    在这里汇合：两者都由 embedded source 执行时的 register_op 记进 scope。
    //    失败不阻断恢复——该 op 只是没有 shape 推导能力，被图使用时按既有语义点名失败。
    std::set<std::string> proto_declared_op_types;
    {
      std::vector<std::string> declared_proto_op_types;
      if (CollectPythonCustomOpModelScopeProtoOpTypes(scope_id, declared_proto_op_types) == SUCCESS) {
        proto_declared_op_types.insert(declared_proto_op_types.cbegin(), declared_proto_op_types.cend());
      } else {
        GELOGW(
            "[PythonCustomOp][Restore] Collect prototype declarations of scope[%llu] failed, shape inference "
            "capability will not be restored for any op type.",
            static_cast<unsigned long long>(scope_id));
      }
    }
    // manifest 携带了声明条目、源码却没能恢复出同名声明：只告警。声明缺失不影响实现执行，
    // 影响的是 shape 推导；声明与实现的映射由 op_type 决定（见 _deployment 的原型分组）。
    for (const auto &op_type : proto_manifest_op_types) {
      if (proto_declared_op_types.count(op_type) == 0U) {
        GELOGW(
            "[PythonCustomOp][Restore] prototype declaration of op type[%s] was declared in the manifest but not "
            "restored from any embedded source; shape inference capability stays unavailable for it.",
            op_type.c_str());
      }
    }
    // 5) 分级校验 + creator 注册进模型级 registry。
    std::set<std::string> restored_op_types;
    std::set<std::string> conflict_skipped_op_types;
    for (const auto &desc : scoped_descriptors) {
      const auto manifest_iter = manifest_by_op_type.find(desc.op_type);
      const bool in_manifest = (manifest_iter != manifest_by_op_type.cend());
      if (in_manifest) {
        const auto &expected = manifest_iter->second;
        if ((expected.class_qualname != desc.class_qualname) || (expected.capabilities != desc.capabilities)) {
          // 能力/qualname 不匹配：不注册 creator（若被图使用，加载期点名硬失败）。
          GELOGW(
              "[PythonCustomOp][Restore] op type[%s] descriptor mismatch with manifest "
              "(class[%s] vs expected[%s], capabilities[0x%x] vs expected[0x%x]); skip creator registration.",
              desc.op_type.c_str(), desc.class_qualname.c_str(), expected.class_qualname.c_str(), desc.capabilities,
              expected.capabilities);
          continue;
        }
      } else {
        // manifest 之外的额外 descriptor：manifest 与 embedded source 由同一次冻结产出，
        // 两者不一致只可能是包内容自相矛盾，直接拒绝（与实现身份不匹配同等严格）。
        GELOGE(GRAPH_FAILED,
               "[PythonCustomOp][Restore] op type[%s] is an extra descriptor not declared in the manifest.",
               desc.op_type.c_str());
        return GRAPH_FAILED;
      }
      // 模型级冲突检查：与已有 creator（C++ SO）冲突则跳过；被图使用时加载期点名硬失败。
      if (registry->HasCreator(AscendString(desc.op_type.c_str()))) {
        GELOGW(
            "[PythonCustomOp][Restore] op type[%s] conflicts with an existing creator in the model registry; "
            "skip Python creator.",
            desc.op_type.c_str());
        (void)conflict_skipped_op_types.insert(desc.op_type);
        continue;
      }
      // 注册进进程级 RuntimeRegistry（按 descriptor_key 索引；scoped key 含 scope_id 天然唯一）。
      PythonCustomOpAdapterDescriptor adapter_desc;
      adapter_desc.op_type = desc.op_type;
      adapter_desc.impl_descriptor_key = desc.impl_descriptor_key;
      adapter_desc.capabilities = desc.capabilities;
      adapter_desc.infer_meta = nullptr;  // 原型未恢复前不参与 shape 推导
      const auto artifact_iter = artifacts_by_op_type.find(desc.op_type);
      if (artifact_iter != artifacts_by_op_type.cend()) {
        adapter_desc.deployment_artifacts = artifact_iter->second;
      }
      // Register 先于能力回填：RuntimeRegistry 的 callbacks 校验只认实现位，
      // kShapeInfer/kInferMeta 属于原型侧能力，带上会被判非法。
      if (!PythonCustomOpImplRuntimeRegistry::Register(adapter_desc, desc.callbacks)) {
        GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Register runtime descriptor key[%s] failed.",
               desc.impl_descriptor_key.c_str());
        return GRAPH_FAILED;
      }
      // scope 里确实声明了同一 op_type 才补 shape 推导能力：infer_meta trampoline 经
      // _infer_meta.call_infer_meta 落回 scope 内的原型声明取 infer_func。声明缺失时
      // 只保留实现能力——既不谎报能力位（否则 IsValid 会放行一个必崩的 adapter）。
      if (proto_declared_op_types.count(desc.op_type) != 0U) {
        adapter_desc.infer_meta = desc.callbacks.infer_meta;
        AddCustomOpCapability(adapter_desc.capabilities, CustomOpCapability::kShapeInfer);
        AddCustomOpCapability(adapter_desc.capabilities, CustomOpCapability::kInferMeta);
      }
      scope->AddRegisteredKey(desc.impl_descriptor_key);
      if (RegisterModelCreators(registry, adapter_desc) != SUCCESS) {
        GELOGE(GRAPH_FAILED, "[PythonCustomOp][Restore] Register model creators for op type[%s] failed.",
               desc.op_type.c_str());
        return GRAPH_FAILED;
      }
      (void)restored_op_types.insert(desc.op_type);
    }
    // manifest 声明但完全未恢复出的 op_type：warning（未被图使用不阻断；被图使用时加载期点名硬失败）。
    for (const auto &op_type : manifest_op_types) {
      if ((restored_op_types.count(op_type) == 0U) && (conflict_skipped_op_types.count(op_type) == 0U)) {
        GELOGW(
            "[PythonCustomOp][Restore] manifest op type[%s] was not restored (missing from embedded source "
            "execution or capability-mismatched).",
            op_type.c_str());
      }
    }
    // 6) 提交：manifest 交 base 作分级校验判据，scope 交 registry 持有（生命周期绑定模型）。
    CustomOpRegistry::PythonRestoreManifest restore_manifest;
    restore_manifest.manifest_op_types = manifest_op_types;
    restore_manifest.conflict_skipped_op_types = std::move(conflict_skipped_op_types);
    registry->SetPythonRestoreManifest(std::move(restore_manifest));
    // 占用进程级台账（Close 时归还）；必须在 SetModelScope 之前，否则 scope 析构
    // 可能早于占用发生，导致 Release 找不到条目。
    CommitPythonOpTypeClaims(op_type_to_artifact_id);
    scope->SetOpTypeToArtifactId(std::move(op_type_to_artifact_id));
    registry->SetModelScope(scope);
    committed = true;
    GELOGI("[PythonCustomOp][Restore] Restored %zu python creator op type(s) into model scope[%llu].",
           restored_op_types.size(), static_cast<unsigned long long>(scope_id));
    return GRAPH_SUCCESS;
  }
};

PythonCustomOpRestoreHookImpl &GetRestoreHookInstance() {
  static PythonCustomOpRestoreHookImpl hook;
  return hook;
}
}  // namespace

void InstallPythonCustomOpRestoreHook() {
  // 幂等：重复安装同一进程级单例无副作用。
  SetPythonCustomOpRestoreHook(&GetRestoreHookInstance());
}
}  // namespace custom_op
}  // namespace ge
