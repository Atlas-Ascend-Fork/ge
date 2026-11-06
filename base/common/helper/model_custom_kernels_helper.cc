/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "framework/common/helper/model_helper.h"

#include <cinttypes>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "common/helper/custom_op_registry_builder.h"
#include "graph/custom_op_registry.h"
#include "common/helper/custom_op_so_loader.h"
#include "common/helper/python_custom_op_package_helper.h"
#include "common/model/ge_root_model.h"
#include "external/graph/custom_op.h"
#include "graph/custom_op/artifact_provider.h"
#include "graph/custom_op/cast.h"
#include "graph/custom_op/framework_hosted_op.h"
#include "graph/custom_op_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/custom_op/model_scope.h"
#include "graph/custom_op/op_proto_ledger.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/ir_definitions_query.h"

namespace ge {
namespace {
Status ValidateCustomOpNodeDeserialized(const NodePtr &node, const CustomOpRegistryPtr &registry) {
  if (node == nullptr) {
    return SUCCESS;
  }

  const std::string op_type = node->GetType();
  const AscendString ascend_op_type(op_type.c_str());
  // 冲突跳过档：Python creator 因与模型 registry 内已有 creator（C++ SO）冲突被跳过时，
  // registry 里仍有那份 C++ creator（收集口径为真），故本判据必须独立于收集口径之前。
  // 图中使用了被跳过的 op_type → Python-C++ 冲突且被图使用 → 点名硬失败（未被图使用时
  // 只在恢复侧 warning，加载不阻断）。纯 C++/无 Python 分区路径集合为空，判据零回归。
  if (registry->IsPythonConflictSkippedOpType(ascend_op_type)) {
    GELOGE(FAILED,
           "[CUSTOM OP] load model failed: op %s is used by the model, but its Python creator was skipped due to a "
           "conflict with an existing creator in the same model registry.",
           op_type.c_str());
    return FAILED;
  }
  if (!registry->IsCustomOpToBeCollected(ascend_op_type)) {
    // 缺失档：OM 携带 PYTHON_CUSTOM_OPS 时，manifest 声明过、被图使用却恢复不出 creator，
    // 说明源码执行失败或与 manifest 不一致——加载期点名失败，不推迟到执行期。
    // 判据用收集口径（IsCustomOpToBeCollected）而非任意注册口径（HasCreator）：Python creator 落
    // (kTop, kCustom) 档位，与 C++ 自定义算子同为收集口径内条目；kBottom 兜底注册（框架内置）
    // 不在收集口径内，不应因 Python manifest 触发加载期失败。
    if (registry->IsPythonManifestOpType(ascend_op_type)) {
      GELOGE(FAILED,
             "[CUSTOM OP] load model failed: op %s is used by the model and declared in the PYTHON_CUSTOM_OPS "
             "manifest, but its creator was not restored. The embedded Python source did not produce a usable "
             "descriptor for this op type (missing, capability-mismatched or failed to initialize).",
             op_type.c_str());
      return FAILED;
    }
    GELOGD("[CUSTOM OP] op %s is not found in registry.", op_type.c_str());
    return SUCCESS;
  }

  if (registry->HasCustomOp(ascend_op_type)) {
    return SUCCESS;
  }
  // 仅实现 PortableOp 能力的算子存在反序列化义务（CUSTOM_OPS 分区数据必须被消费）；
  // 非 PortableOp 算子的 kernel 由算子 so 自行管理，无需反序列化，so 加载并注册 creator
  // 后即完成全部初始化，此处查询会为算子创建实例。
  if (registry->GetCustomOpCommonCapability(ascend_op_type, CustomOpCapability::kPortable) == nullptr) {
    GELOGD("[CUSTOM OP] custom op %s is not PortableOp, no serialized data to consume.", op_type.c_str());
    return SUCCESS;
  }
  GELOGE(FAILED, "[CUSTOM OP] custom op %s is used by model but not deserialized.", op_type.c_str());
  return FAILED;
}

Status ValidateCustomOpsInGraphDeserialized(const ComputeGraphPtr &graph, const CustomOpRegistryPtr &registry,
                                            std::set<ComputeGraph *> &visited_graphs) {
  if (graph == nullptr) {
    return SUCCESS;
  }
  if (!visited_graphs.insert(graph.get()).second) {
    return SUCCESS;
  }

  for (const auto &node : graph->GetAllNodes()) {
    GE_ASSERT_SUCCESS(ValidateCustomOpNodeDeserialized(node, registry));
  }
  return SUCCESS;
}

bool HasNonEmptyPartition(const OmFileLoadHelper &om_load_helper, ModelPartitionType type) {
  ModelPartition partition;
  if (om_load_helper.GetModelPartition(type, partition, 0U) != SUCCESS) {
    return false;
  }
  return (partition.data != nullptr) && (partition.size > 0U);
}

bool HasNonEmptyCustomOpsPartition(const OmFileLoadHelper &om_load_helper) {
  return HasNonEmptyPartition(om_load_helper, ModelPartitionType::CUSTOM_OPS);
}

Status CollectCustomOpTypesFromGraph(const ComputeGraphPtr &graph, const CustomOpRegistryPtr &registry,
                                     std::set<std::string> &used_custom_op_types) {
  if (graph == nullptr) {
    return SUCCESS;
  }
  GE_ASSERT_NOTNULL(registry);
  for (const auto &node : graph->GetAllNodes()) {
    const std::string op_type = node->GetType();
    const AscendString ascend_op_type(op_type.c_str());
    if (registry->IsCustomOpToBeCollected(ascend_op_type)) {
      (void)used_custom_op_types.insert(op_type);
    }
  }
  return SUCCESS;
}

// 判断一个实际使用的自定义算子是否由 Python 实现：只有 Python 实现需要把定义脚本打包进 OM，
// C++ 实现的 creator 由 SO_BINS 恢复。识别方式是查询 GE 内部 CustomOpArtifactProvider，
// 不依赖 op type 命名，也不查询任何进程级注册表。
//
// dynamic_cast 前提：两段式取实例返回的是 most-derived 对象内 BaseCustomOp 子对象的指针；
// BaseCustomOp 自带虚析构属 polymorphic 类型，因此到兄弟基类 FrameworkHostedOp /
// CustomOpArtifactProvider 的转换成立。
//
// 识别判据与 portable 声明正交：
//   - 是否 Python：由 FrameworkHostedOp 标记接口判定（PR 5358），不看 portable、不看 artifact；
//   - 是否有可打包源码：由 CustomOpArtifactProvider::GetDeploymentArtifacts() 是否为空判定
//     （Python 实现注册即冻结源码，为空仅对应源码不可定位的形态）；
//   - 是否有 state 义务：由 CustomOpCast<PortableOp> 是否命中判定。
// 故非 portable 的无状态 Python 算子同样能被正确识别为 kPythonDeploy。
enum class PythonDeployKind { kNotCustomOp, kCppNative, kPythonNoDeploy, kPythonDeploy };

// 一次分类产出保存侧所需的全部信息，供 DetectUsedPythonDeployCustomOp（只用 kind）与
// SaveCustomOpsPartition（用 kind + serializable_op + artifact）共用，避免两处各扫一遍实例。
struct UsedCustomOpInfo {
  PortableOp *serializable_op = nullptr;  // 非空表示有 state 义务，进 CUSTOM_OPS 序列化链
  // 非空表示源码可打包，进 PYTHON_CUSTOM_OPS。分文件原型时含两项：实现 artifact
  // 与承载 @register_op 声明的原型 artifact（只收实现侧会让 proto.py 进不了 OM，
  // 加载后无 infer_func，动态 shape 推导静默缺席）。
  std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> artifacts;
};

PythonDeployKind ClassifyCustomOp(CustomOpRegistry &registry, const std::string &op_type_str, UsedCustomOpInfo &info) {
  info = UsedCustomOpInfo();
  const AscendString op_type(op_type_str.c_str());
  // 取实例分两级：先按 kPortable 取 provider，保证序列化的就是加载侧 DeserializeCustomKernelItem
  // 反序列化的那个实例（它同样只按 kPortable 取）；取不到再兜底 GetAnyCustomOpInstance，
  // 覆盖无状态（未声明 portable）的 Python 算子——它们不产出 CUSTOM_OPS，但仍要打包源码。
  BaseCustomOp *custom_op =
      CustomOpCast<PortableOp>(registry.GetCustomOpCommonCapability(op_type, CustomOpCapability::kPortable));
  if (custom_op == nullptr) {
    custom_op = registry.GetAnyCustomOpInstance(op_type);
  }
  if (custom_op == nullptr) {
    // 取不到实例：保存侧不做检查（fail-open），仅不进 PYTHON_CUSTOM_OPS，恢复失败延后到加载/执行期暴露。
    GELOGW("[CUSTOM OP] op[%s] has creator registered but instance creation failed; skip python packaging.",
           op_type_str.c_str());
    return PythonDeployKind::kNotCustomOp;
  }
  // state 义务与 Python/C++ 识别正交：C++ PortableOp 同样要进 CUSTOM_OPS，故先于 FrameworkHostedOp 判定填充
  info.serializable_op = CustomOpCast<PortableOp>(custom_op);
  if (dynamic_cast<const FrameworkHostedOp *>(custom_op) == nullptr) {
    // 非 FrameworkHostedOp：普通 C++ 自定义算子，SO 与 state 由既有逻辑处理，不参与 Python 采集
    return PythonDeployKind::kCppNative;
  }
  const auto *const provider = dynamic_cast<const CustomOpArtifactProvider *>(custom_op);
  if (provider != nullptr) {
    info.artifacts = provider->GetDeploymentArtifacts();
  }
  if (info.artifacts.empty()) {
    // Python 算子但无可打包源码（定义文件不可定位：exec/动态类/zipimport，或冻结失败）
    return PythonDeployKind::kPythonNoDeploy;
  }
  return PythonDeployKind::kPythonDeploy;
}

// AscendString::GetString() 可返回 nullptr，冻结前判空；失败返回 false 由调用方跳过该字段。
bool CopyIrString(const ge::AscendString &value, std::string &result) {
  const char *value_string = value.GetString();
  if (value_string == nullptr) {
    return false;
  }
  result = value_string;
  return true;
}

// GetRegisteredIrDefFromGraph 返回的 kind 字符串 → bridge 侧位定义（与
// PythonCustomOpProtoInputKind / PythonCustomOpProtoOutputKind 对齐）；未知值返回 false，
// 由调用方跳过整个 descriptor 的 IR 冻结（避免按错误输入类别绑定参数）。
bool ParseIrInputKind(const char *kind, uint32_t &result) {
  const std::string kind_str(kind);
  if (kind_str == "required") {
    result = 0U;
    return true;
  }
  if (kind_str == "optional") {
    result = 1U;
    return true;
  }
  if (kind_str == "dynamic") {
    result = 2U;
    return true;
  }
  return false;
}

bool ParseIrOutputKind(const char *kind, uint32_t &result) {
  const std::string kind_str(kind);
  if (kind_str == "required") {
    result = 0U;
    return true;
  }
  if (kind_str == "dynamic") {
    result = 1U;
    return true;
  }
  return false;
}

// 编译进程把 OperatorFactory 中的 canonical IR 定义冻结进 descriptor 快照。
// 加载态 D4 抑制原型注册，schema-bound execute 的参数绑定在推理进程查不到
// OperatorFactory，必须随 PYTHON_CUSTOM_OPS 分区携带 IR（inputs/attrs/outputs）。
// 查不到（图未引用该原型/注册链路异常）时 IR 字段保持为空，执行期按既有语义点名失败。
std::shared_ptr<const PythonCustomOpDeploymentArtifact> FreezeArtifactIrSnapshots(
    const std::shared_ptr<const PythonCustomOpDeploymentArtifact> &artifact) {
  bool all_frozen = true;
  for (const auto &descriptor : artifact->descriptors) {
    if (descriptor.ir_inputs.empty() && descriptor.ir_attrs.empty() && descriptor.ir_outputs.empty()) {
      all_frozen = false;
      break;
    }
  }
  if (all_frozen) {
    return artifact;  // 已含 IR（重复保存），原样返回
  }
  auto cloned = std::make_shared<PythonCustomOpDeploymentArtifact>(*artifact);
  for (auto &descriptor : cloned->descriptors) {
    if (!descriptor.ir_inputs.empty() || !descriptor.ir_attrs.empty() || !descriptor.ir_outputs.empty()) {
      continue;  // 该 descriptor 已冻结，保留
    }
    std::vector<std::pair<ge::AscendString, ge::AscendString>> inputs;
    std::vector<std::pair<ge::AscendString, ge::AscendString>> outputs;
    std::vector<std::pair<ge::AscendString, ge::AscendString>> attrs;
    if (GetRegisteredIrDefFromGraph(descriptor.op_type.c_str(), inputs, outputs, attrs) != ge::SUCCESS) {
      GELOGW("[PYTHON_CUSTOM_OP][PACKAGE_BUILD] canonical IR not found for op[%s], skip ir snapshot.",
             descriptor.op_type.c_str());
      continue;
    }
    bool frozen = true;
    descriptor.ir_inputs.reserve(inputs.size());
    for (const auto &input : inputs) {
      std::string name;
      uint32_t kind = 0U;
      if (!CopyIrString(input.first, name) || (input.second.GetString() == nullptr) ||
          !ParseIrInputKind(input.second.GetString(), kind)) {
        frozen = false;
        break;
      }
      descriptor.ir_inputs.push_back(PythonIrInputSnapshot{std::move(name), kind});
    }
    if (frozen) {
      descriptor.ir_attrs.reserve(attrs.size());
      for (const auto &attr : attrs) {
        std::string name;
        std::string type;
        if (!CopyIrString(attr.first, name) || !CopyIrString(attr.second, type)) {
          frozen = false;
          break;
        }
        descriptor.ir_attrs.push_back(PythonIrAttrSnapshot{std::move(name), std::move(type)});
      }
    }
    if (frozen) {
      descriptor.ir_outputs.reserve(outputs.size());
      for (const auto &output : outputs) {
        std::string name;
        uint32_t kind = 0U;
        if (!CopyIrString(output.first, name) || (output.second.GetString() == nullptr) ||
            !ParseIrOutputKind(output.second.GetString(), kind)) {
          frozen = false;
          break;
        }
        descriptor.ir_outputs.push_back(PythonIrOutputSnapshot{std::move(name), kind});
      }
    }
    if (!frozen) {
      // 任一字段判空失败或 kind 未知：丢弃半截快照，IR 保持为空（执行期点名失败），
      // 不静默降级为 required（会让加载态按错误输入类别绑定参数）。
      descriptor.ir_inputs.clear();
      descriptor.ir_attrs.clear();
      descriptor.ir_outputs.clear();
      GELOGW("[PYTHON_CUSTOM_OP][PACKAGE_BUILD] invalid canonical IR field for op[%s], skip ir snapshot.",
             descriptor.op_type.c_str());
    }
  }
  return cloned;
}
}  // namespace

Status LoadCustomOpsToRegistry(const uint8_t *data, const size_t len, const CustomOpRegistryPtr &registry) {
  if (registry == nullptr) {
    GELOGE(FAILED, "[CUSTOM OP] model custom op registry is null.");
    return FAILED;
  }
  GE_CHK_STATUS_RET(registry->LoadCustomOpsPartition(data, len),
                    "[CUSTOM OP] Load custom ops partition to model registry failed.");
  return SUCCESS;
}

Status ModelHelper::CollectUsedCustomOpTypes(const GeRootModelPtr &ge_root_model,
                                             std::set<std::string> &used_custom_op_types) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  const auto &registry = ge_root_model->GetCustomOpRegistry();
  GE_ASSERT_NOTNULL(registry);
  GE_ASSERT_SUCCESS(CollectCustomOpTypesFromGraph(ge_root_model->GetRootGraph(), registry, used_custom_op_types));

  // subgraph_instance_name_to_model_ 中的 GeModel 可能持有独立的 ComputeGraph 对象，
  // 这些子图未必通过 AddSubgraph 挂入 root_graph 的子图树，因此无法被上方 GetAllNodes() 遍历到。
  // 典型场景：编译分区后各分区独立持有自己的 ComputeGraph，或反序列化时每个 GeModel 单独还原图对象。
  // 此处需额外遍历，以确保这类游离子图中的自定义算子也被收集到。
  const auto &subgraph_map = ge_root_model->GetSubgraphInstanceNameToModel();
  for (const auto &subgraph_pair : subgraph_map) {
    const auto &ge_model = subgraph_pair.second;
    if (ge_model == nullptr || ge_model->GetGraph() == nullptr) {
      continue;
    }
    const auto &graph = ge_model->GetGraph();
    if (graph == ge_root_model->GetRootGraph()) {
      continue;
    }
    GE_ASSERT_SUCCESS(CollectCustomOpTypesFromGraph(graph, registry, used_custom_op_types));
  }
  return SUCCESS;
}

Status ModelHelper::SerializeCustomOpKernel(PortableOp *serializable_op, const std::string &op_type_str,
                                            std::vector<uint8_t> &merged_buffers) const {
  if (serializable_op == nullptr) {
    GELOGE(FAILED, "[CUSTOM OP] serializable custom op is null, op_type:%s", op_type_str.c_str());
    return FAILED;
  }

  std::vector<uint8_t> buffer;
  const auto ret = serializable_op->Serialize(buffer);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(ret, "[CUSTOM OP] serialize failed, op_type:%s", op_type_str.c_str());
    return ret;
  }

  if (buffer.empty()) {
    GELOGE(FAILED, "[CUSTOM OP] serialized buffer is empty, op_type:%s", op_type_str.c_str());
    return FAILED;
  }

  CustomKernelItemHeader header;
  header.magic = kCustomKernelItemMagic;
  header.name_len = static_cast<uint32_t>(op_type_str.size());
  header.bin_len = static_cast<uint32_t>(buffer.size());

  const auto *header_ptr = reinterpret_cast<const uint8_t *>(&header);
  (void)merged_buffers.insert(merged_buffers.end(), header_ptr, header_ptr + sizeof(header));
  (void)merged_buffers.insert(merged_buffers.end(), op_type_str.begin(), op_type_str.end());
  (void)merged_buffers.insert(merged_buffers.end(), buffer.begin(), buffer.end());
  GELOGD("[CUSTOM OP] Serialized custom op '%s', bin size:%zu", op_type_str.c_str(), buffer.size());
  return SUCCESS;
}

Status ModelHelper::DetectUsedPythonDeployCustomOp(const GeRootModelPtr &ge_root_model,
                                                   std::set<std::string> &used_custom_op_types,
                                                   bool &has_python_deploy_custom_op) const {
  has_python_deploy_custom_op = false;
  used_custom_op_types.clear();
  if (ge_root_model == nullptr) {
    GELOGE(FAILED, "[CUSTOM OP] ge root model is null.");
    return FAILED;
  }
  // registry 为空时不存在任何自定义算子，交由既有 gating 分支处理，不视为错误
  if (ge_root_model->GetCustomOpRegistry() == nullptr) {
    return SUCCESS;
  }
  GE_ASSERT_SUCCESS(CollectUsedCustomOpTypes(ge_root_model, used_custom_op_types));
  auto &registry = *(ge_root_model->GetCustomOpRegistry());
  // 保存侧不做校验：图中使用的 Python 算子无可打包源码（kPythonNoDeploy，源码不可定位）或取实例
  // 失败（kNotCustomOp）都不报错，只是不进 PYTHON_CUSTOM_OPS；恢复失败延后到加载/执行期暴露。
  UsedCustomOpInfo info;
  for (const auto &op_type_str : used_custom_op_types) {
    const auto kind = ClassifyCustomOp(registry, op_type_str, info);
    if (kind == PythonDeployKind::kPythonDeploy) {
      has_python_deploy_custom_op = true;
    }
  }
  return SUCCESS;
}

Status ModelHelper::SaveCustomOpsPartition(std::shared_ptr<OmFileSaveHelper> &om_file_save_helper,
                                           const GeRootModelPtr &ge_root_model) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  if (ge_root_model->GetCustomOpRegistry() == nullptr) {
    GELOGI("[CUSTOM OP] model custom op registry is null, skip saving custom ops partition.");
    return SUCCESS;
  }

  std::set<std::string> used_custom_op_types;
  GE_ASSERT_SUCCESS(CollectUsedCustomOpTypes(ge_root_model, used_custom_op_types));
  return SaveCustomOpsPartition(om_file_save_helper, ge_root_model, used_custom_op_types);
}

Status ModelHelper::SaveCustomOpsPartition(std::shared_ptr<OmFileSaveHelper> &om_file_save_helper,
                                           const GeRootModelPtr &ge_root_model,
                                           const std::set<std::string> &used_custom_op_types) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  const auto &registry = ge_root_model->GetCustomOpRegistry();
  // registry 为空是合法场景（模型不含任何自定义算子），保持与两参重载一致的跳过语义
  if (registry == nullptr) {
    GELOGI("[CUSTOM OP] model custom op registry is null, skip saving custom ops partition.");
    return SUCCESS;
  }

  if (used_custom_op_types.empty()) {
    GELOGI("[CUSTOM OP] No custom ops used in graph, skip saving custom kernels partition.");
    return SUCCESS;
  }

  // 源码打包与 portable 正交：Python 实现的源码（被图使用时）进 PYTHON_CUSTOM_OPS，
  // portable 决定 state 进 CUSTOM_OPS，两个集合可任意组合（含「仅源码无 state」的无状态 Python 算子）。
  // 一次遍历同时产出两样东西，采集独立于 CollectCustomOpSo（后者对 Python 算子 continue 且不置 has_custom_op）。
  std::vector<std::pair<std::string, PortableOp *>> serializable_ops;
  serializable_ops.reserve(used_custom_op_types.size());
  PythonCustomOpPackageBuilder package_builder;
  UsedCustomOpInfo info;
  for (const auto &op_type_str : used_custom_op_types) {
    const auto kind = ClassifyCustomOp(*registry, op_type_str, info);
    // state 义务：仅 PortableOp 进 CUSTOM_OPS 序列化链（非 portable 算子根本不进此链，绕开空 buffer 不一致）
    if (info.serializable_op != nullptr) {
      (void)serializable_ops.emplace_back(op_type_str, info.serializable_op);
    }
    if (kind == PythonDeployKind::kPythonDeploy) {
      // 逐个交出：分文件原型时同一 op_type 会带来实现 artifact 与原型 artifact 两项。
      // AddArtifact 按 artifact_id 去重，多个 op_type 共享同一文件时不会重复打包。
      for (const auto &artifact : info.artifacts) {
        GE_CHK_STATUS_RET(package_builder.AddArtifact(FreezeArtifactIrSnapshots(artifact)),
                          "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] collect deployment artifact failed, op type:%s",
                          op_type_str.c_str());
      }
    }
    // kPythonNoDeploy / kNotCustomOp：保存侧不做检查，仅不进 PYTHON_CUSTOM_OPS（恢复失败延后暴露）
    // kCppNative：普通 C++ 自定义算子，creator 由 SO_BINS 恢复，不参与 Python 采集
  }

  std::vector<uint8_t> merged_buffers;
  for (const auto &serializable_op : serializable_ops) {
    GE_ASSERT_SUCCESS(SerializeCustomOpKernel(serializable_op.second, serializable_op.first, merged_buffers));
  }

  std::vector<uint8_t> python_package_buffer;
  if (!package_builder.Empty()) {
    GE_CHK_STATUS_RET(package_builder.Build(python_package_buffer),
                      "[PYTHON_CUSTOM_OP][PACKAGE_BUILD] build python custom op package failed.");
  }

  // 两个分区各自独立决定是否写出（四组合）：
  //   python 非空、merged 空 -> 只写 PYTHON_CUSTOM_OPS（「仅源码无 state」的无状态 Python 算子，合法）；
  //   merged 非空、python 空 -> 只写 CUSTOM_OPS（纯 C++ PortableOp 既有行为）；
  //   两者非空 -> 都写（有状态的 Python 算子）；两者皆空 -> 都跳过。
  // 「portable 但源码不可定位」形态保存侧不做检查：state 进 CUSTOM_OPS、源码不进
  // PYTHON_CUSTOM_OPS，OM 照常产出，其恢复失败延后到加载/执行期暴露。
  if (!merged_buffers.empty()) {
    GELOGI("[CUSTOM OP] custom ops partition size:%zu", merged_buffers.size());
    GE_CHK_STATUS_RET(
        om_file_save_helper->AddOwnedPartition(ModelPartitionType::CUSTOM_OPS, std::move(merged_buffers), 0U),
        "[Save][CustomOpsPartition] failed.");
  } else {
    GELOGI("[CUSTOM OP] no custom ops serialized, skip saving custom_ops partition.");
  }
  if (python_package_buffer.empty()) {
    // 无 Python deployment 源码：纯 C++ 路径或模型不含 Python 算子，不需要 PYTHON_CUSTOM_OPS 分区
    return SUCCESS;
  }
  GELOGI("[PYTHON_CUSTOM_OP][PACKAGE_BUILD] python custom ops partition size:%zu", python_package_buffer.size());
  return om_file_save_helper->AddOwnedPartition(ModelPartitionType::PYTHON_CUSTOM_OPS, std::move(python_package_buffer),
                                                0U);
}

Status ModelHelper::ValidateCustomOpsDeserialized(const GeRootModelPtr &ge_root_model,
                                                  const CustomOpRegistryPtr &registry) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  GE_ASSERT_NOTNULL(registry);

  std::set<ComputeGraph *> visited_graphs;
  GE_ASSERT_SUCCESS(ValidateCustomOpsInGraphDeserialized(ge_root_model->GetRootGraph(), registry, visited_graphs));

  const auto &subgraph_map = ge_root_model->GetSubgraphInstanceNameToModel();
  for (const auto &subgraph_pair : subgraph_map) {
    const auto &ge_model = subgraph_pair.second;
    if (ge_model == nullptr) {
      continue;
    }
    GE_ASSERT_SUCCESS(ValidateCustomOpsInGraphDeserialized(ge_model->GetGraph(), registry, visited_graphs));
  }
  return SUCCESS;
}

Status ModelHelper::LoadCustomOps(const OmFileLoadHelper &om_load_helper, const CustomOpRegistryPtr &registry) const {
  ModelPartition custom_ops_partition;
  if (om_load_helper.GetModelPartition(ModelPartitionType::CUSTOM_OPS, custom_ops_partition, 0U) != SUCCESS) {
    GELOGI("[CUSTOM OP] custom ops partition not found, skip load.");
    return SUCCESS;
  }

  if ((custom_ops_partition.data == nullptr) || (custom_ops_partition.size == 0U)) {
    GELOGI("[CUSTOM OP] custom ops partition is empty, skip load.");
    return SUCCESS;
  }

  GE_CHK_STATUS_RET(LoadCustomOpsToRegistry(custom_ops_partition.data, custom_ops_partition.size, registry),
                    "[CUSTOM OP] Load custom ops partition to model registry failed.");
  return SUCCESS;
}

Status ModelHelper::LoadCustomOpRegistry(const OmFileLoadHelper &om_load_helper,
                                         const GeRootModelPtr &ge_root_model) const {
  GE_ASSERT_NOTNULL(ge_root_model);
  static std::mutex offline_custom_op_registry_load_mutex;
  // 先声明锁，确保注册阶段异常返回时 registry 在解锁前析构。
  std::unique_lock<std::mutex> custom_op_load_lock;
  auto registry = std::make_shared<CustomOpRegistry>();
  GE_ASSERT_NOTNULL(registry);
  std::vector<OpSoBinPtr> custom_op_so_bins;
  GE_CHK_STATUS_RET(LoadOpSoBin(om_load_helper, ge_root_model, custom_op_so_bins), "[CUSTOM OP] Load so bins failed.");
  const bool has_python_partition = HasNonEmptyPartition(om_load_helper, ModelPartitionType::PYTHON_CUSTOM_OPS);
  if (custom_op_so_bins.empty() && !has_python_partition) {
    GE_ASSERT_TRUE(!HasNonEmptyCustomOpsPartition(om_load_helper),
                   "[CUSTOM OP] custom ops partition exists but no custom op so is loaded.");
    ge_root_model->SetCustomOpRegistry(registry);
    GELOGI("[CUSTOM OP] no custom op so loaded, set empty model custom op registry.");
    return SUCCESS;
  }

  // 仅串行化自定义算子 SO 的加载、静态注册、原型收集和冲突判定，以及 PYTHON_CUSTOM_OPS 恢复段
  // （恢复涉及 GIL、sys.modules 写入与 scope-local capture 重定向，均为进程级状态）。
  custom_op_load_lock = std::unique_lock<std::mutex>(offline_custom_op_registry_load_mutex);
  if (!custom_op_so_bins.empty()) {
    ScopedOpProtoLoadTxn proto_load_txn(registry);
    std::vector<CustomOpSoHandlePtr> loaded_handles;
    GE_CHK_STATUS_RET(LoadCustomOpSoBins(custom_op_so_bins, loaded_handles),
                      "[CUSTOM OP] Load custom op so bins failed.");
    GE_ASSERT_TRUE(!loaded_handles.empty(), "[CUSTOM OP] custom op so bins loaded without handles.");

    GE_CHK_STATUS_RET(CustomOpRegistryBuilder::AddCreatorsFromSoHandles(loaded_handles, registry),
                      "[CUSTOM OP] Build model custom op registry failed.");
    if (!proto_load_txn.GetConflicts().empty()) {
      for (const auto &conflict : proto_load_txn.GetConflicts()) {
        GELOGE(FAILED,
               "[CUSTOM OP] conflicting custom op implementations for op type[%s] map kind[%u]: incumbent so[%s] "
               "fingerprint[%s], "
               "challenger so[%s] fingerprint[%s].",
               conflict.op_type.c_str(), static_cast<uint32_t>(conflict.map_kind), conflict.incumbent_so_name.c_str(),
               conflict.incumbent_fingerprint.c_str(), conflict.challenger_so_name.c_str(),
               conflict.challenger_fingerprint.c_str());
      }
      GELOGE(FAILED,
             "[CUSTOM OP] load model failed: models in one process must use the same implementation for the "
             "same custom op type. Align custom op package versions and retry.");
      return FAILED;
    }
  }
  // Python 恢复段与加载态原型抑制一致，不进入 ScopedOpProtoLoadTxn；事务析构已在上方作用域结束时完成。
  if (has_python_partition) {
    auto *restore_hook = GetPythonCustomOpRestoreHook();
    if (restore_hook == nullptr) {
      GELOGE(FAILED,
             "[CUSTOM OP] load model failed: the model contains a PYTHON_CUSTOM_OPS partition with embedded Python "
             "custom op sources, but no Python custom op restore hook is registered in this process. Load the model "
             "with a runtime that provides Python custom op support, or re-save the model on a matching environment.");
      return FAILED;
    }
    ModelPartition python_partition;
    GE_CHK_STATUS_RET(om_load_helper.GetModelPartition(ModelPartitionType::PYTHON_CUSTOM_OPS, python_partition, 0U),
                      "[CUSTOM OP] Get python custom ops partition failed.");
    GE_CHK_STATUS_RET(restore_hook->Restore(python_partition.data, python_partition.size, registry),
                      "[CUSTOM OP] Restore python custom ops from partition failed.");
  }

  // 事务析构必须发生在解锁之前；后续 CUSTOM_OPS 反序列化和校验只访问模型级 registry。
  custom_op_load_lock.unlock();
  GE_CHK_STATUS_RET(LoadCustomOps(om_load_helper, registry), "[CUSTOM OP] Load custom ops to registry failed.");
  GE_CHK_STATUS_RET(ValidateCustomOpsDeserialized(ge_root_model, registry),
                    "[CUSTOM OP] Validate model custom ops deserialized failed.");
  ge_root_model->SetCustomOpRegistry(registry);
  return SUCCESS;
}

Status ModelHelper::LoadOpSoBin(const OmFileLoadHelper &om_load_helper, const GeRootModelPtr &ge_root_model,
                                std::vector<OpSoBinPtr> &custom_op_so_bins) const {
  ModelPartition partition_kernel_def;
  if (om_load_helper.GetModelPartition(ModelPartitionType::SO_BINS, partition_kernel_def, 0U) != SUCCESS) {
    return SUCCESS;
  }
  GELOGD("Kernels partition size:%" PRIu64 "", partition_kernel_def.size);
  if (ge_root_model->LoadSoBinData(partition_kernel_def.data, partition_kernel_def.size)) {
    auto root_graph = ge_root_model->GetRootGraph();
    GE_ASSERT_NOTNULL(root_graph);
    std::map<std::string, ge::OpSoBinPtr> bin_file_buffer;
    auto all_so_bin = ge_root_model->GetAllSoBin();
    for (const auto &op_so_bin_ptr : all_so_bin) {
      if (op_so_bin_ptr == nullptr) {
        continue;
      }
      if (op_so_bin_ptr->GetSoBinType() == SoBinType::kAutofuse) {
        if (op_so_bin_ptr->GetSoName() == "guard_check.so") {
          std::string guard_data(reinterpret_cast<const char_t *>(op_so_bin_ptr->GetBinData()),
                                 op_so_bin_ptr->GetBinDataSize());
          (void)AttrUtils::SetStr(root_graph, "_guard_check_so_data", guard_data);
          GELOGD("Restored guard check so data to graph attr, size=%zu.", guard_data.size());
        } else {
          std::string so_path = op_so_bin_ptr->GetVendorName() + "/" + op_so_bin_ptr->GetSoName();
          bin_file_buffer[so_path] = op_so_bin_ptr;
          GELOGD("Added autofuse so_path:%s", so_path.c_str());
        }
      } else if (op_so_bin_ptr->GetSoBinType() == SoBinType::kCustomOp) {
        (void)custom_op_so_bins.emplace_back(op_so_bin_ptr);
      }
    }
    if (!bin_file_buffer.empty()) {
      (void)root_graph->SetExtAttr<std::map<std::string, ge::OpSoBinPtr>>("bin_file_buffer", bin_file_buffer);
    }
    SaveOpSoInfo(ge_root_model);
    GELOGD("Load so bin store success");
  } else {
    GELOGW("Load so bin store unsuccessful");
    GE_ASSERT_TRUE(partition_kernel_def.size == 0U,
                   "Load so bin store failed when SO_BINS partition is non-empty, size:%" PRIu64,
                   partition_kernel_def.size);
  }
  return SUCCESS;
}

Status ModelHelper::LoadCustomOpSoBins(const std::vector<OpSoBinPtr> &custom_so_bins,
                                       std::vector<CustomOpSoHandlePtr> &loaded_handles) const {
  if (custom_so_bins.empty()) {
    return SUCCESS;
  }
  GE_ASSERT_SUCCESS(CustomOpSoLoader::GetInstance().LoadCustomOpSoBins(custom_so_bins, loaded_handles),
                    "Load custom op so bins from SO_BINS failed.");
  return SUCCESS;
}

void ModelHelper::SaveOpSoInfo(const GeRootModelPtr &ge_root_model) const {
  SoInOmInfo so_info;
  (void)ge::AttrUtils::GetStr(*(model_.get()), "host_env_os", so_info.os_info);
  (void)ge::AttrUtils::GetStr(*(model_.get()), "host_env_cpu", so_info.cpu_info);
  (void)ge::AttrUtils::GetStr(*(model_.get()), ATTR_MODEL_OPP_VERSION, so_info.opp_version);
  (void)ge::AttrUtils::GetStr(*(model_.get()), ATTR_MODEL_COMPILER_VERSION, so_info.compiler_version);
  GELOGD("Save so info with host_env_os:%s, host_env_cpu:%s, opp_version:%s, compiler_version:%s",
         so_info.os_info.c_str(), so_info.cpu_info.c_str(), so_info.opp_version.c_str(),
         so_info.compiler_version.c_str());
  ge_root_model->SetSoInOmInfo(so_info);
}
}  // namespace ge
