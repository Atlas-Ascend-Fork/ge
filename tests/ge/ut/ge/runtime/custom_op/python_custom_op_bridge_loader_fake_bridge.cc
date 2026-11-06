/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <atomic>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "runtime/custom_op/python_custom_op_bridge_c_api.h"
#include "runtime/custom_op/python_custom_op_bridge_types.h"

namespace ge {
namespace custom_op {
namespace {
constexpr const char *kScenarioEnvName = "GE_PYTHON_CUSTOM_OP_LOADER_UT_SCENARIO";
constexpr const char *kMultiProtoFailure = "multi_proto_failure";
constexpr const char *kAdapterFailure = "adapter_failure";
constexpr const char *kSuccess = "success";
constexpr const char *kDualBackendSuccess = "dual_backend_success";
constexpr const char *kHostCompileSuccess = "host_compile_success";
constexpr const char *kCppProtoImpl = "cpp_proto_impl";

constexpr const char *kMultiProtoOp = "PythonLoaderMultiProtoRollbackUt";
constexpr const char *kAdapterOpA = "PythonLoaderAdapterRollbackAUt";
constexpr const char *kAdapterOpB = "PythonLoaderAdapterRollbackBUt";
constexpr const char *kSuccessOp = "PythonLoaderSuccessUt";
constexpr const char *kDualBackendOp = "PythonLoaderDualBackendUt";
constexpr const char *kHostCompileOp = "PythonLoaderHostCompileUt";
constexpr const char *kCppProtoOp = "PythonLoaderCppProtoOwnershipUt";

constexpr const char *kMultiProtoKey = "loader_ut:multi_proto";
constexpr const char *kAdapterProtoKeyA = "loader_ut:adapter_proto_a";
constexpr const char *kAdapterProtoKeyB = "loader_ut:adapter_proto_b";
constexpr const char *kAdapterImplKeyA = "loader_ut:adapter_impl_a";
constexpr const char *kAdapterImplKeyB = "loader_ut:adapter_impl_b";
constexpr const char *kSuccessProtoKey = "loader_ut:success_proto";
constexpr const char *kSuccessImplKey = "loader_ut:success_impl";
constexpr const char *kDualBackendProtoKey = "loader_ut:dual_backend_proto";
constexpr const char *kDualBackendImplKey = "loader_ut:dual_backend_impl";
constexpr const char *kHostCompileImplKey = "loader_ut:host_compile_impl";
constexpr const char *kCppProtoImplKey = "loader_ut:cpp_proto_impl";

std::atomic<uint32_t> g_register_count{0U};
std::atomic<uint32_t> g_reset_count{0U};
std::atomic<uint32_t> g_collect_artifact_count{0U};
// 分文件原型场景：记录本次 scope 恢复实际收到了哪几个 embedded source。
// fake 不执行 Python，"某个文件被恢复"用"该文件出现在 sources 里"代替。
std::atomic<bool> g_split_proto_source_seen{false};
std::atomic<bool> g_split_impl_source_seen{false};

// 分文件原型场景的入口在文件尾部定义（与 artifact fixture 放在一起），此处先声明。
bool InSplitProtoScenario();
Status RegisterSplitProtoImpl(const PythonCustomOpRegistrar &registrar);
Status CollectSplitProtoArtifacts(const PythonCustomOpDeploymentArtifactSink *sink, void *user_data);
Status RestoreSplitProtoScope(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources, size_t source_count,
                              const PythonCustomOpScopedDescriptorSink *sink, void *user_data);
Status CollectSplitProtoOpTypes(const PythonCustomOpStringSink *sink, void *user_data);
int g_holder = 0;

PythonCustomOpStringView StringView(const char *value) {
  return PythonCustomOpStringView{value, (value == nullptr) ? 0U : std::strlen(value)};
}

graphStatus InferMeta(const PythonCustomOpStringView *, gert::InferShapeContext *,
                      PythonCustomOpInferMetaResultView *) {
  return GRAPH_SUCCESS;
}

PythonCustomOpProtoDescriptorView MakeProto(const char *descriptor_key, const char *op_type) {
  return PythonCustomOpProtoDescriptorView{
      StringView(descriptor_key), StringView(op_type), nullptr, 0U, nullptr, 0U, nullptr, 0U, &InferMeta};
}

PythonCustomOpAdapterDescriptorView MakeAdapterWithCapabilities(const char *op_type, const char *impl_key,
                                                                const CustomOpCapabilityMask capabilities) {
  return PythonCustomOpAdapterDescriptorView{StringView(op_type), StringView(impl_key), capabilities};
}

PythonCustomOpAdapterDescriptorView MakeAdapter(const char *op_type, const char *impl_key) {
  return MakeAdapterWithCapabilities(op_type, impl_key,
                                     static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute));
}

void *CreateImplHolder(const PythonCustomOpAdapterDescriptorView *desc) {
  return (desc == nullptr) ? nullptr : &g_holder;
}

void DestroyImplHolder(void *holder) {
  (void)holder;
}

graphStatus Execute(const void *holder, gert::EagerOpExecutionContext *ctx) {
  (void)ctx;
  return (holder == nullptr) ? GRAPH_FAILED : GRAPH_SUCCESS;
}

graphStatus HostExecute(const void *holder, gert::HostCpuOpExecutionContext *ctx) {
  (void)ctx;
  return (holder == nullptr) ? GRAPH_FAILED : GRAPH_SUCCESS;
}

graphStatus Compile(const void *holder, gert::OpCompileContext *ctx) {
  (void)ctx;
  return (holder == nullptr) ? GRAPH_FAILED : GRAPH_SUCCESS;
}

PythonCustomOpAdapterCallbacks MakeCallbacks(const bool reject) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = &CreateImplHolder;
  callbacks.destroy_impl_holder = &DestroyImplHolder;
  callbacks.execute = &Execute;
  if (reject) {
    callbacks.create_impl_holder = nullptr;
  }
  (void)reject;
  return callbacks;
}

PythonCustomOpAdapterCallbacks MakeDualBackendCallbacks() {
  auto callbacks = MakeCallbacks(false);
  callbacks.host_cpu_execute = &HostExecute;
  return callbacks;
}

PythonCustomOpAdapterCallbacks MakeHostCompileCallbacks() {
  auto callbacks = MakeCallbacks(false);
  callbacks.execute = nullptr;
  callbacks.host_cpu_execute = &HostExecute;
  callbacks.compile_impl = &Compile;
  return callbacks;
}

Status SetArtifactConfig(const PythonCustomOpBridgeArtifactConfig *config) {
  if ((config == nullptr) || (config->artifact_root == nullptr) || (config->native_module_path == nullptr)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  return static_cast<Status>(GRAPH_SUCCESS);
}

Status RegisterMultiProtoFailure(const PythonCustomOpRegistrar &registrar) {
  const auto first = MakeProto(kMultiProtoKey, kMultiProtoOp);
  if (!registrar.register_op_proto(&first)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const auto invalid = MakeProto("loader_ut:invalid_proto", "");
  return static_cast<Status>(registrar.register_op_proto(&invalid) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

Status RegisterAdapterFailure(const PythonCustomOpRegistrar &registrar) {
  const auto proto_a = MakeProto(kAdapterProtoKeyA, kAdapterOpA);
  const auto proto_b = MakeProto(kAdapterProtoKeyB, kAdapterOpB);
  if ((!registrar.register_op_proto(&proto_a)) || (!registrar.register_op_proto(&proto_b))) {
    return static_cast<Status>(GRAPH_FAILED);
  }

  const auto callbacks = MakeCallbacks(false);
  const auto adapter_a = MakeAdapter(kAdapterOpA, kAdapterImplKeyA);
  if (!registrar.register_op_impl(&adapter_a, &callbacks)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const auto rejecting_callbacks = MakeCallbacks(true);
  const auto adapter_b = MakeAdapter(kAdapterOpB, kAdapterImplKeyB);
  return static_cast<Status>(registrar.register_op_impl(&adapter_b, &rejecting_callbacks) ? GRAPH_SUCCESS
                                                                                          : GRAPH_FAILED);
}

Status RegisterSuccess(const PythonCustomOpRegistrar &registrar) {
  const auto proto = MakeProto(kSuccessProtoKey, kSuccessOp);
  if (!registrar.register_op_proto(&proto)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const auto callbacks = MakeCallbacks(false);
  const auto adapter = MakeAdapter(kSuccessOp, kSuccessImplKey);
  return static_cast<Status>(registrar.register_op_impl(&adapter, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

Status RegisterDualBackendSuccess(const PythonCustomOpRegistrar &registrar) {
  const auto proto = MakeProto(kDualBackendProtoKey, kDualBackendOp);
  if (!registrar.register_op_proto(&proto)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const auto callbacks = MakeDualBackendCallbacks();
  auto adapter = MakeAdapter(kDualBackendOp, kDualBackendImplKey);
  AddCustomOpCapability(adapter.capabilities, CustomOpCapability::kHostCpuExecute);
  return static_cast<Status>(registrar.register_op_impl(&adapter, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

Status RegisterHostCompileSuccess(const PythonCustomOpRegistrar &registrar) {
  const auto proto = MakeProto("loader_ut:host_compile_proto", kHostCompileOp);
  if (!registrar.register_op_proto(&proto)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const auto callbacks = MakeHostCompileCallbacks();
  const auto adapter =
      MakeAdapterWithCapabilities(kHostCompileOp, kHostCompileImplKey,
                                  static_cast<CustomOpCapabilityMask>(CustomOpCapability::kHostCpuExecute) |
                                      static_cast<CustomOpCapabilityMask>(CustomOpCapability::kCompilable));
  return static_cast<Status>(registrar.register_op_impl(&adapter, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

Status RegisterCppProtoImpl(const PythonCustomOpRegistrar &registrar) {
  const auto callbacks = MakeCallbacks(false);
  const auto adapter = MakeAdapter(kCppProtoOp, kCppProtoImplKey);
  return static_cast<Status>(registrar.register_op_impl(&adapter, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

Status RegisterCustomOps(const PythonCustomOpRegistrar *registrar) {
  ++g_register_count;
  if ((registrar == nullptr) || (registrar->register_op_proto == nullptr) || (registrar->register_op_impl == nullptr)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  const char *scenario = std::getenv(kScenarioEnvName);
  if (scenario == nullptr) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  if (std::strcmp(scenario, kMultiProtoFailure) == 0) {
    return RegisterMultiProtoFailure(*registrar);
  }
  if (std::strcmp(scenario, kAdapterFailure) == 0) {
    return RegisterAdapterFailure(*registrar);
  }
  if (std::strcmp(scenario, kSuccess) == 0) {
    return RegisterSuccess(*registrar);
  }
  if (std::strcmp(scenario, kDualBackendSuccess) == 0) {
    return RegisterDualBackendSuccess(*registrar);
  }
  if (std::strcmp(scenario, kHostCompileSuccess) == 0) {
    return RegisterHostCompileSuccess(*registrar);
  }
  if (std::strcmp(scenario, kCppProtoImpl) == 0) {
    return RegisterCppProtoImpl(*registrar);
  }
  if (InSplitProtoScenario()) {
    return RegisterSplitProtoImpl(*registrar);
  }
  return static_cast<Status>(GRAPH_FAILED);
}

void ResetBridgeState() {
  ++g_reset_count;
}

void ShutdownBridge() {}

// 默认场景不冻结任何部署 artifact：loader UT 关注注册/回滚时序，
// 只需保证 collect_deployment_artifacts 存在、可调用且返回零个 artifact。
// 分文件原型场景（split_proto_impl）交出 proto.py / impl.py 两个 artifact，
// 用于驱动保存侧「原型 artifact 必须一并进 OM」的整条链路。
Status CollectDeploymentArtifacts(const PythonCustomOpDeploymentArtifactSink *sink, void *user_data) {
  ++g_collect_artifact_count;
  if ((sink == nullptr) || (sink->on_artifact == nullptr)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  if (InSplitProtoScenario()) {
    return CollectSplitProtoArtifacts(sink, user_data);
  }
  return static_cast<Status>(GRAPH_SUCCESS);
}

// 模型 scope 恢复入口：loader UT 场景只需存在、可调用并返回 SUCCESS（IsBridgeApiValid
// 通过即可）；restore hook UT 的 restore_proto_and_impl 场景推入两个 scoped descriptor，
// 供恢复侧验证 manifest 比对与 artifact 交递链路。
constexpr char kRestoreScenario[] = "restore_proto_and_impl";
constexpr char kRestoreWithProtoOp[] = "PythonRestoreHookWithProtoUt";
constexpr char kRestoreNoProtoOp[] = "PythonRestoreHookNoProtoUt";
constexpr char kRestoreWithProtoKey[] = "restore_ut:with_proto";
constexpr char kRestoreNoProtoKey[] = "restore_ut:no_proto";

bool InRestoreScenario() {
  const char *scenario = std::getenv(kScenarioEnvName);
  return (scenario != nullptr) && (std::strcmp(scenario, kRestoreScenario) == 0);
}

Status RestoreModelScope(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources, size_t source_count,
                         const PythonCustomOpScopedDescriptorSink *sink, void *user_data) {
  if ((sink == nullptr) || (sink->on_descriptor == nullptr)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  if (InSplitProtoScenario()) {
    return RestoreSplitProtoScope(scope_id, sources, source_count, sink, user_data);
  }
  if (InRestoreScenario()) {
    auto callbacks = MakeCallbacks(false);
    callbacks.infer_meta = &InferMeta;
    const PythonCustomOpScopedDescriptorView with_proto = {
        StringView(kRestoreWithProtoOp), StringView(kRestoreWithProtoKey), StringView(kRestoreWithProtoOp),
        StringView(kRestoreWithProtoOp), static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute)};
    const PythonCustomOpScopedDescriptorView no_proto = {
        StringView(kRestoreNoProtoOp), StringView(kRestoreNoProtoKey), StringView(kRestoreNoProtoOp),
        StringView(kRestoreNoProtoOp), static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute)};
    if ((!sink->on_descriptor(user_data, &with_proto, &callbacks)) ||
        (!sink->on_descriptor(user_data, &no_proto, &callbacks))) {
      return static_cast<Status>(GRAPH_FAILED);
    }
  }
  return static_cast<Status>(GRAPH_SUCCESS);
}

// 与 RestoreModelScope 同一场景开关：只声明 kRestoreWithProtoOp 的原型，kRestoreNoProtoOp
// 保持无声明（恢复侧据此判定"能力位与实际推导函数严格对应"）。
Status CollectModelScopeProtoOpTypes(uint64_t, const PythonCustomOpStringSink *sink, void *user_data) {
  if ((sink == nullptr) || (sink->on_string == nullptr)) {
    return static_cast<Status>(GRAPH_FAILED);
  }
  if (InSplitProtoScenario()) {
    return CollectSplitProtoOpTypes(sink, user_data);
  }
  if (InRestoreScenario()) {
    const PythonCustomOpStringView with_proto = StringView(kRestoreWithProtoOp);
    if (!sink->on_string(user_data, &with_proto)) {
      return static_cast<Status>(GRAPH_FAILED);
    }
  }
  return static_cast<Status>(GRAPH_SUCCESS);
}

Status CloseModelScope(uint64_t) {
  return static_cast<Status>(GRAPH_SUCCESS);
}

// ---------------------------------------------------------------- 分文件原型 OM 往返场景
//
// 编译期 Python 冻结产出两个 artifact：proto.py 只承载 @register_op 声明（descriptor 的
// capabilities 恰为 kShapeInfer|kInferMeta，即 IsProtoDescriptor 的判定位组合），impl.py
// 承载实现。本场景让 fake bridge 在 collect_deployment_artifacts 里交出这两项、在
// register_custom_ops 里只登记实现，正好驱动保存侧的真实链路：
//   CollectDeploymentArtifactsFromBridge → artifact_by_op_type_/proto_artifacts_by_op_type_
//   → BuildAdapterDescriptor → GetDeploymentArtifacts → SaveCustomOpsPartition → Build
constexpr const char *kSplitProtoScenario = "split_proto_impl";
constexpr const char *kSplitProtoOp = "PythonSplitProtoRoundTripUt";
constexpr const char *kSplitProtoImplKey = "split_proto_ut:impl";

constexpr const char *kSplitProtoSourceName = "split_proto_plugin.py";
constexpr const char *kSplitImplSourceName = "split_impl_plugin.py";
constexpr char kSplitProtoSource[] =
    "from ge.custom_op import register_op\n"
    "\n"
    "\n"
    "@register_op(op_type=\"PythonSplitProtoRoundTripUt\")\n"
    "def infer_meta(x):\n"
    "    return x\n";
constexpr char kSplitImplSource[] =
    "from ge.custom_op import register_op_impl\n"
    "\n"
    "\n"
    "@register_op_impl(op_type=\"PythonSplitProtoRoundTripUt\")\n"
    "class PythonSplitProtoRoundTripUt:\n"
    "    def execute(self, ctx):\n"
    "        _ = ctx\n";

// artifact_id 契约 = source 字节 sha256 的 64 字符小写十六进制。本场景用确定性伪摘要展开到
// 32 字节：fake 不链 OpenSSL，而校验只需「id 与 sha256 字段一致」，不要求真实 sha256。
// 采用 64 位多项式滚动哈希（乘子取小质数，无长数字字面量），逐字节累积后展开成 32 字节。
std::array<uint8_t, 32U> FakeDigest(const char *source) {
  const size_t length = std::strlen(source);
  uint64_t state = 0U;
  for (size_t i = 0U; i < length; ++i) {
    state = (state * 131U) + static_cast<uint64_t>(static_cast<uint8_t>(source[i])) + 1U;
    state ^= state >> 29U;
  }
  std::array<uint8_t, 32U> digest{};
  for (size_t i = 0U; i < digest.size(); ++i) {
    digest[i] = static_cast<uint8_t>((state >> ((i % 8U) * 8U)) ^ (i * 17U));
  }
  return digest;
}

std::string ToHex(const std::array<uint8_t, 32U> &digest) {
  static constexpr char kHexDigits[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(64U);
  for (const auto byte : digest) {
    hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
    hex.push_back(kHexDigits[byte & 0x0FU]);
  }
  return hex;
}

struct SplitArtifactFixture {
  std::array<uint8_t, 32U> sha256;
  std::string artifact_id;
  const char *source_name;
  const char *source;
  std::vector<PythonCustomOpDeploymentDescriptorView> descriptors;
};

void BuildSplitArtifactFixtures(std::vector<SplitArtifactFixture> &fixtures) {
  fixtures.clear();
  fixtures.resize(2U);

  auto &proto = fixtures[0U];
  proto.sha256 = FakeDigest(kSplitProtoSource);
  proto.artifact_id = ToHex(proto.sha256);
  proto.source_name = kSplitProtoSourceName;
  proto.source = kSplitProtoSource;
  proto.descriptors.push_back(
      PythonCustomOpDeploymentDescriptorView{StringView(kSplitProtoOp), StringView("infer_meta"),
                                             static_cast<CustomOpCapabilityMask>(CustomOpCapability::kShapeInfer) |
                                                 static_cast<CustomOpCapabilityMask>(CustomOpCapability::kInferMeta),
                                             nullptr, 0U, nullptr, 0U, nullptr, 0U});

  auto &impl = fixtures[1U];
  impl.sha256 = FakeDigest(kSplitImplSource);
  impl.artifact_id = ToHex(impl.sha256);
  impl.source_name = kSplitImplSourceName;
  impl.source = kSplitImplSource;
  impl.descriptors.push_back(PythonCustomOpDeploymentDescriptorView{
      StringView(kSplitProtoOp), StringView(kSplitProtoOp),
      static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute), nullptr, 0U, nullptr, 0U, nullptr, 0U});
}

bool InSplitProtoScenario() {
  const char *scenario = std::getenv(kScenarioEnvName);
  return (scenario != nullptr) && (std::strcmp(scenario, kSplitProtoScenario) == 0);
}

Status CollectSplitProtoArtifacts(const PythonCustomOpDeploymentArtifactSink *sink, void *user_data) {
  std::vector<SplitArtifactFixture> fixtures;
  BuildSplitArtifactFixtures(fixtures);
  for (const auto &fixture : fixtures) {
    const PythonCustomOpDeploymentArtifactView view = {StringView(fixture.artifact_id.c_str()),
                                                       StringView(fixture.source_name),
                                                       fixture.sha256.data(),
                                                       fixture.sha256.size(),
                                                       reinterpret_cast<const uint8_t *>(fixture.source),
                                                       std::strlen(fixture.source),
                                                       fixture.descriptors.data(),
                                                       fixture.descriptors.size(),
                                                       3U,
                                                       11U};
    if (!sink->on_artifact(user_data, &view)) {
      return static_cast<Status>(GRAPH_FAILED);
    }
  }
  return static_cast<Status>(GRAPH_SUCCESS);
}

// 编译期注册：本场景只登记实现（@register_op_impl 所在文件），原型声明由 artifact 承载。
Status RegisterSplitProtoImpl(const PythonCustomOpRegistrar &registrar) {
  const auto callbacks = MakeCallbacks(false);
  const auto adapter = MakeAdapter(kSplitProtoOp, kSplitProtoImplKey);
  return static_cast<Status>(registrar.register_op_impl(&adapter, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

// 恢复侧：真实 bridge 会执行 embedded source 并回报 scope 内捕获的声明/实现。
// fake 不执行 Python，"哪个文件被恢复"用"该文件是否出现在 sources 里"记录，
// 供 UT 断言 proto.py 确实随 OM 走到了恢复段；descriptor 与声明按来源文件分别回报，
// 从而能区分"声明来自 proto.py、实现来自 impl.py"。
Status RestoreSplitProtoScope(uint64_t scope_id, const PythonCustomOpEmbeddedSourceView *sources, size_t source_count,
                              const PythonCustomOpScopedDescriptorSink *sink, void *user_data) {
  g_split_proto_source_seen.store(false);
  g_split_impl_source_seen.store(false);
  for (size_t i = 0U; i < source_count; ++i) {
    const auto &source_name = sources[i].source_name;
    const std::string name(source_name.data, source_name.size);
    if (name == kSplitProtoSourceName) {
      g_split_proto_source_seen.store(true);
    } else if (name == kSplitImplSourceName) {
      g_split_impl_source_seen.store(true);
    }
  }
  if (!g_split_impl_source_seen.load()) {
    // 实现文件未恢复就没有可注册的 impl descriptor，与真实 bridge 的失败语义一致。
    return static_cast<Status>(GRAPH_FAILED);
  }
  auto callbacks = MakeCallbacks(false);
  callbacks.infer_meta = &InferMeta;
  // 与真实 bridge 同口径：恢复出的 descriptor key 内嵌 scope_id（module_name 的一部分），
  // 因此同进程多次恢复、以及与编译期注册项共存时都不会撞 key。
  const std::string scoped_impl_key = std::string(kSplitProtoImplKey) + ":scope:" + std::to_string(scope_id);
  const PythonCustomOpScopedDescriptorView impl = {
      StringView(kSplitProtoOp), StringView(scoped_impl_key.c_str()), StringView(kSplitProtoOp),
      StringView(kSplitProtoOp), static_cast<CustomOpCapabilityMask>(CustomOpCapability::kEagerExecute)};
  return static_cast<Status>(sink->on_descriptor(user_data, &impl, &callbacks) ? GRAPH_SUCCESS : GRAPH_FAILED);
}

// 声明只可能来自 proto.py：只在该文件被恢复时才回报，UT 据此证明 infer_func 的来源。
Status CollectSplitProtoOpTypes(const PythonCustomOpStringSink *sink, void *user_data) {
  if (!g_split_proto_source_seen.load()) {
    return static_cast<Status>(GRAPH_SUCCESS);
  }
  const PythonCustomOpStringView op_type = StringView(kSplitProtoOp);
  return static_cast<Status>(sink->on_string(user_data, &op_type) ? GRAPH_SUCCESS : GRAPH_FAILED);
}
}  // namespace
}  // namespace custom_op
}  // namespace ge

extern "C" __attribute__((visibility("default"))) uint32_t GePythonCustomOpLoaderUtGetRegisterCount() {
  return ge::custom_op::g_register_count.load();
}

extern "C" __attribute__((visibility("default"))) uint32_t GePythonCustomOpLoaderUtGetResetCount() {
  return ge::custom_op::g_reset_count.load();
}

extern "C" __attribute__((visibility("default"))) uint32_t GePythonCustomOpLoaderUtGetCollectArtifactCount() {
  return ge::custom_op::g_collect_artifact_count.load();
}

extern "C" __attribute__((visibility("default"))) uint32_t GePythonCustomOpLoaderUtGetSplitProtoSourceSeen() {
  return ge::custom_op::g_split_proto_source_seen.load() ? 1U : 0U;
}

extern "C" __attribute__((visibility("default"))) uint32_t GePythonCustomOpLoaderUtGetSplitImplSourceSeen() {
  return ge::custom_op::g_split_impl_source_seen.load() ? 1U : 0U;
}

extern "C" __attribute__((visibility("default"))) void GePythonCustomOpLoaderUtResetCounters() {
  ge::custom_op::g_register_count.store(0U);
  ge::custom_op::g_reset_count.store(0U);
  ge::custom_op::g_collect_artifact_count.store(0U);
  ge::custom_op::g_split_proto_source_seen.store(false);
  ge::custom_op::g_split_impl_source_seen.store(false);
}

extern "C" __attribute__((visibility("default"))) const ge::custom_op::PythonCustomOpBridgeApi *
GeGetPythonCustomOpBridgeApi() {
  static const ge::custom_op::PythonCustomOpBridgeApi kBridgeApi = {
      ge::custom_op::kPythonCustomOpBridgeAbiVersion,
      &ge::custom_op::SetArtifactConfig,
      &ge::custom_op::RegisterCustomOps,
      &ge::custom_op::ResetBridgeState,
      &ge::custom_op::ShutdownBridge,
      &ge::custom_op::CollectDeploymentArtifacts,
      &ge::custom_op::RestoreModelScope,
      &ge::custom_op::CollectModelScopeProtoOpTypes,
      &ge::custom_op::CloseModelScope,
  };
  return &kBridgeApi;
}
