/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// restore hook 的端到端 UT：复用 loader UT 的 fake bridge 基建（Py_* 符号覆写 +
// ScopedArtifactTree + PYTHONPATH），经 InstallPythonCustomOpRestoreHook 安装真
// PythonCustomOpRestoreHookImpl，用 PythonCustomOpPackageBuilder 构造真实
// PYTHON_CUSTOM_OPS 分区字节驱动 Restore()。覆盖：
// - 原型声明条目（IsProtoDescriptor）与实现条目同 op_type 共存一个 package；
// - 恢复侧按 scope 原型声明补回 kShapeInfer/kInferMeta 与 infer_meta 回调，
//   无声明的 op_type 保持纯实现位（能力位与实际推导函数严格对应）；
// - 实现条目恢复出的 adapter 携带 deployment_artifacts（「加载→再保存」分区不丢）。

#include <dlfcn.h>
#include <unistd.h>

#include <sys/stat.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/helper/model_parser_base.h"
#include "common/helper/python_custom_op_package_helper.h"
#include "common/model/ge_root_model.h"
#include "common/python_runtime/ge_python_runtime_manager.h"
#include "common/python_runtime/python_artifact_utils.h"
#include "framework/common/helper/model_helper.h"
#include "graph/compute_graph.h"
#include "graph/custom_op/artifact_provider.h"
#include "graph/custom_op/capability.h"
#include "graph/custom_op/cast.h"
#include "graph/custom_op_factory.h"
#include "graph/custom_op/model_scope.h"
#include "graph/custom_op_registry.h"
#include "graph/op_desc.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "runtime/custom_op/python_custom_op_bridge_loader.h"
#include "runtime/custom_op/python_custom_op_restore_hook.h"

#include "gtest/gtest.h"

// 与 loader UT 相同的 Python C API 符号覆写：GePythonRuntimeManager 经
// ResolvePythonCApi(RTLD_DEFAULT) 命中这些桩，不需要真实解释器。
extern "C" int Py_IsInitialized();
extern "C" const char *Py_GetVersion();
extern "C" void Py_Initialize();
extern "C" void Py_Finalize();
extern "C" void PyEval_InitThreads();
extern "C" int PyEval_ThreadsInitialized();
extern "C" void *PyEval_SaveThread();
extern "C" void PyEval_RestoreThread(void *thread_state);
extern "C" int PyGILState_Check();

namespace {
bool g_fake_python_initialized = false;
bool g_fake_python_threads_initialized = false;
}  // namespace

extern "C" int Py_IsInitialized() {
  return g_fake_python_initialized ? 1 : 0;
}

extern "C" const char *Py_GetVersion() {
  return "3.11.0";
}

extern "C" void Py_Initialize() {
  g_fake_python_initialized = true;
}

extern "C" void Py_Finalize() {
  g_fake_python_initialized = false;
  g_fake_python_threads_initialized = false;
}

extern "C" void PyEval_InitThreads() {
  g_fake_python_threads_initialized = true;
}

extern "C" int PyEval_ThreadsInitialized() {
  return g_fake_python_threads_initialized ? 1 : 0;
}

extern "C" void *PyEval_SaveThread() {
  return reinterpret_cast<void *>(0x200);
}

extern "C" void PyEval_RestoreThread(void *thread_state) {
  (void)thread_state;
}

extern "C" int PyGILState_Check() {
  return 1;
}

namespace ge {
namespace custom_op {
namespace {
constexpr char kScenarioEnvName[] = "GE_PYTHON_CUSTOM_OP_LOADER_UT_SCENARIO";
constexpr char kPythonPathEnvName[] = "PYTHONPATH";
constexpr char kRestoreScenario[] = "restore_proto_and_impl";
constexpr char kFakeBridgeArtifactName[] = "libge_python_custom_op_loader_ut_fake.so";
constexpr char kFakeNativeArtifactName[] = "_ge_custom_op_loader_ut_fake.so";
constexpr char kPythonTag[] = "cp311";

constexpr char kWithProtoOp[] = "PythonRestoreHookWithProtoUt";
constexpr char kNoProtoOp[] = "PythonRestoreHookNoProtoUt";

// 分文件原型 OM 往返场景（fake bridge 的 split_proto_impl）：op_type 与两个源文件名
// 必须与 fake bridge 侧常量一致——UT 断言"哪个文件走到了恢复段"就靠这两处对齐。
constexpr char kSplitProtoScenario[] = "split_proto_impl";
constexpr char kSplitProtoOp[] = "PythonSplitProtoRoundTripUt";
constexpr char kSplitProtoSourceName[] = "split_proto_plugin.py";
constexpr char kSplitImplSourceName[] = "split_impl_plugin.py";

constexpr uint32_t kTestBridgeAbi = kPythonCustomOpBridgeAbiVersion;
// 与 fake bridge 的 MakeCallbacks(false) 对齐：impl 侧能力位只有 eager_execute
// （fake 回调表无 serialize/deserialize，portable 会被 callbacks.IsValid 拒绝）。
constexpr uint32_t kImplCapabilities = 1U << 0U;
// 原型声明条目的固定能力位（kShapeInfer | kInferMeta），IsProtoDescriptor 的判定依据。
constexpr uint32_t kProtoCapabilities = (1U << 2U) | (1U << 6U);

class ScopedEnvVar {
 public:
  ScopedEnvVar(const char *name, const std::string &value) : name_(name) {
    const char *old_value = std::getenv(name);
    if (old_value != nullptr) {
      old_value_ = old_value;
      has_old_value_ = true;
    }
    (void)setenv(name_.c_str(), value.c_str(), 1);
  }

  ~ScopedEnvVar() {
    if (has_old_value_) {
      (void)setenv(name_.c_str(), old_value_.c_str(), 1);
    } else {
      (void)unsetenv(name_.c_str());
    }
  }

  ScopedEnvVar(const ScopedEnvVar &) = delete;
  ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

 private:
  std::string name_;
  std::string old_value_;
  bool has_old_value_{false};
};

// 与 loader UT 的 ScopedArtifactTree 相同的目录布局：site-packages/ge/custom_op/
// python_custom_op_artifacts/fake/ 下放 fake bridge SO 符号链接与 manifest.json。
class ScopedArtifactTree {
 public:
  bool Prepare(const std::string &bridge_target) {
    if (bridge_target.empty() || (access(bridge_target.c_str(), R_OK) != 0)) {
      return false;
    }
    char dir_template[] = "/tmp/ge_python_custom_op_restore_hook_ut_XXXXXX";
    const char *created_dir = mkdtemp(dir_template);
    if (created_dir == nullptr) {
      return false;
    }
    root_ = created_dir;
    const std::vector<std::string> dirs = {
        Path("site-packages"), Path("site-packages/ge"), Path("site-packages/ge/custom_op"),
        Path("site-packages/ge/custom_op/python_custom_op_artifacts"), ArtifactRoot()};
    for (const auto &dir : dirs) {
      if ((mkdir(dir.c_str(), 0700) != 0) && (errno != EEXIST)) {
        return false;
      }
      dirs_.emplace_back(dir);
    }

    const auto bridge_path = ArtifactRoot() + "/" + kFakeBridgeArtifactName;
    if (symlink(bridge_target.c_str(), bridge_path.c_str()) != 0) {
      return false;
    }
    files_.emplace_back(bridge_path);
    if (!WriteFile(ArtifactRoot() + "/" + kFakeNativeArtifactName, "fake native artifact")) {
      return false;
    }
    const std::string manifest =
        "{\n"
        "  \"python_tag\": \"" +
        std::string(kPythonTag) +
        "\",\n"
        "  \"platform\": \"" +
        python_artifact::CurrentPlatformTag() +
        "\",\n"
        "  \"bridge_abi\": " +
        std::to_string(kTestBridgeAbi) +
        ",\n"
        "  \"artifacts\": {\n"
        "    \"bridge\": \"" +
        kFakeBridgeArtifactName +
        "\",\n"
        "    \"native\": \"" +
        kFakeNativeArtifactName +
        "\"\n"
        "  }\n"
        "}\n";
    return WriteFile(ArtifactRoot() + "/manifest.json", manifest);
  }

  ~ScopedArtifactTree() {
    for (auto iter = files_.rbegin(); iter != files_.rend(); ++iter) {
      (void)remove(iter->c_str());
    }
    for (auto iter = dirs_.rbegin(); iter != dirs_.rend(); ++iter) {
      (void)rmdir(iter->c_str());
    }
    if (!root_.empty()) {
      (void)rmdir(root_.c_str());
    }
  }

  std::string PythonPath() const {
    return Path("site-packages");
  }

  ScopedArtifactTree(const ScopedArtifactTree &) = delete;
  ScopedArtifactTree &operator=(const ScopedArtifactTree &) = delete;
  ScopedArtifactTree() = default;

 private:
  std::string Path(const std::string &relative_path) const {
    return root_ + "/" + relative_path;
  }

  std::string ArtifactRoot() const {
    return Path("site-packages/ge/custom_op/python_custom_op_artifacts/fake");
  }

  bool WriteFile(const std::string &path, const std::string &content) {
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
      return false;
    }
    files_.emplace_back(path);
    output << content;
    output.close();
    return output.good();
  }

  std::string root_;
  std::vector<std::string> files_;
  std::vector<std::string> dirs_;
};

std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> MakeDeterministicDigest(const uint32_t seed) {
  std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> digest{};
  for (size_t i = 0U; i < digest.size(); ++i) {
    digest[i] = static_cast<uint8_t>((seed >> ((i % 4U) * 8U)) ^ (seed + static_cast<uint32_t>(i)));
  }
  return digest;
}

std::string ToLowerHex(const std::array<uint8_t, PythonCustomOpDeploymentArtifact::kSha256Len> &digest) {
  static constexpr char kHexDigits[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(PythonCustomOpDeploymentArtifact::kArtifactIdLen);
  for (const auto byte : digest) {
    hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
    hex.push_back(kHexDigits[byte & 0x0FU]);
  }
  return hex;
}

PythonOpDescriptorSnapshot MakeImplSnapshot(std::string op_type) {
  PythonOpDescriptorSnapshot snapshot;
  snapshot.op_type = std::move(op_type);
  snapshot.class_name = snapshot.op_type;
  snapshot.capabilities = kImplCapabilities;
  return snapshot;
}

std::shared_ptr<PythonCustomOpDeploymentArtifact> MakeArtifact(
    const uint32_t seed, const std::vector<PythonOpDescriptorSnapshot> &descriptors) {
  auto artifact = std::make_shared<PythonCustomOpDeploymentArtifact>();
  artifact->bridge_abi_version = kTestBridgeAbi;
  artifact->producer_python_major = 3U;
  artifact->producer_python_minor = 11U;
  artifact->source_sha256 = MakeDeterministicDigest(seed);
  artifact->artifact_id = ToLowerHex(artifact->source_sha256);
  artifact->source_name = "plugin" + std::to_string(seed) + ".py";
  const auto source = std::make_shared<std::vector<uint8_t>>(std::vector<uint8_t>{'p', 'a', 's', 's', '\n'});
  artifact->source = source;
  artifact->descriptors = descriptors;
  return artifact;
}

// 保存侧的入口条件：图中确实使用该 op_type，且 GeRootModel 绑定了自定义算子 registry
// （SaveCustomOpsPartition 经它取 creator 与部署 artifact）。
GeRootModelPtr MakeRootModelUsingPythonOp(const std::string &op_type) {
  auto root_graph = std::make_shared<ComputeGraph>("root_graph_for_python_custom_op");
  auto op_desc = std::make_shared<OpDesc>("python_custom_op_node", op_type);
  (void)op_desc->AddDynamicInputDesc("x", 1);
  (void)op_desc->AddDynamicOutputDesc("y", 1);
  if (root_graph->AddNode(op_desc) == nullptr) {
    return nullptr;
  }
  auto ge_root_model = std::make_shared<GeRootModel>();
  if (ge_root_model->Initialize(root_graph) != SUCCESS) {
    return nullptr;
  }
  ge_root_model->SetCustomOpRegistry(CustomOpFactory::GetGlobalRegistryPtr());
  return ge_root_model;
}

// 从 OM 字节里取指定分区：恢复侧拿到的东西必须来自 OM 字节本身，
// 不能是保存侧内存里的中间态（那样验不出分区是否真的写出去了）。
// 指针切分与 ModelHelper::LoadModel 一致：头部单独处理，分区表从头部之后开始。
bool ExtractPartitionFromOmBuffer(const ModelBufferData &model_buffer, ModelPartitionType type,
                                  ModelPartition &partition) {
  if (model_buffer.data == nullptr) {
    return false;
  }
  auto *const file_header = reinterpret_cast<const ModelFileHeader *>(model_buffer.data.get());
  auto *const partitions_begin = model_buffer.data.get() + sizeof(ModelFileHeader);
  OmFileLoadHelper load_helper;
  if (load_helper.Init(partitions_begin, file_header->model_length, file_header) != SUCCESS) {
    return false;
  }
  return load_helper.GetModelPartition(type, partition, 0U) == SUCCESS;
}

class PythonCustomOpRestoreHookTest : public testing::Test {
 protected:
  void SetUp() override {
    InstallPythonCustomOpRestoreHook();
    ShutdownPythonCustomOpsForProcess();
    ClearRuntimeState();
    Py_Initialize();
    ASSERT_TRUE(artifact_tree_.Prepare(PythonCustomOpBridgeLoaderPathForUt()));
    python_path_env_ = std::make_unique<ScopedEnvVar>(kPythonPathEnvName, artifact_tree_.PythonPath());
    scenario_env_ = std::make_unique<ScopedEnvVar>(kScenarioEnvName, kRestoreScenario);
  }

  void TearDown() override {
    scenario_env_.reset();
    python_path_env_.reset();
    ShutdownPythonCustomOpsForProcess();
    ClearRuntimeState();
    Py_Finalize();
  }

  // 恢复进一个全新的模型级 registry；scope 由 registry 持有，registry 析构即清理。
  CustomOpRegistryPtr RestoreIntoFreshRegistry(const std::vector<uint8_t> &package_bytes) {
    auto registry = std::make_shared<CustomOpRegistry>();
    auto *hook = GetPythonCustomOpRestoreHook();
    if (hook == nullptr) {
      return nullptr;
    }
    if (hook->Restore(package_bytes.data(), package_bytes.size(), registry) != GRAPH_SUCCESS) {
      return nullptr;
    }
    return registry;
  }

  void SetScenario(const char *scenario) {
    scenario_env_.reset();
    scenario_env_ = std::make_unique<ScopedEnvVar>(kScenarioEnvName, scenario);
  }

 private:
  static void ClearRuntimeState() {
    CustomOpFactory::RemoveCustomOps(
        {AscendString(kWithProtoOp), AscendString(kNoProtoOp), AscendString(kSplitProtoOp)});
    PythonCustomOpImplRuntimeRegistry::Clear();
    GePythonRuntimeManager::Instance().ShutdownProcess();
  }

  static std::string PythonCustomOpBridgeLoaderPathForUt() {
#ifndef PYTHON_CUSTOM_OP_LOADER_UT_FAKE_BRIDGE_PATH
#define PYTHON_CUSTOM_OP_LOADER_UT_FAKE_BRIDGE_PATH ""
#endif
    return PYTHON_CUSTOM_OP_LOADER_UT_FAKE_BRIDGE_PATH;
  }

  ScopedArtifactTree artifact_tree_;
  std::unique_ptr<ScopedEnvVar> python_path_env_;
  std::unique_ptr<ScopedEnvVar> scenario_env_;
};

// 打包侧：原型声明条目（capabilities 恰为 kShapeInfer|kInferMeta）与实现条目同
// op_type 共存 —— 原型 artifact 携带声明、实现 artifact 携带实现身份。build 与 parse
// 的 op_type 全局唯一性都必须豁免原型条目，否则分文件原型无法打包。
TEST_F(PythonCustomOpRestoreHookTest, PackageBuildAndParseAcceptProtoEntrySharingOpTypeWithImpl) {
  PythonOpDescriptorSnapshot proto_snapshot;
  proto_snapshot.op_type = kWithProtoOp;
  proto_snapshot.class_name = "infer_func";
  proto_snapshot.capabilities = kProtoCapabilities;

  PythonOpDescriptorSnapshot impl_snapshot;
  impl_snapshot.op_type = kWithProtoOp;
  impl_snapshot.class_name = kWithProtoOp;
  impl_snapshot.capabilities = kImplCapabilities;

  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x10U, {proto_snapshot})), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x20U, {impl_snapshot})), SUCCESS);

  std::vector<uint8_t> out;
  ASSERT_EQ(builder.Build(out), SUCCESS);

  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(out.data(), out.size(), parsed), SUCCESS);
  ASSERT_EQ(parsed.artifacts.size(), 2U);
  bool has_proto = false;
  bool has_impl = false;
  for (const auto &artifact : parsed.artifacts) {
    for (const auto &descriptor : artifact->descriptors) {
      EXPECT_EQ(descriptor.op_type, kWithProtoOp);
      if (IsProtoDescriptor(descriptor)) {
        EXPECT_EQ(descriptor.capabilities, kProtoCapabilities);
        has_proto = true;
      } else {
        EXPECT_EQ(descriptor.capabilities, kImplCapabilities);
        has_impl = true;
      }
    }
  }
  EXPECT_TRUE(has_proto);
  EXPECT_TRUE(has_impl);
}

// 打包侧负向：两个实现条目（非原型）op_type 重复仍拒绝 —— 豁免只针对原型条目，
// 实现身份的唯一性不放松。
TEST_F(PythonCustomOpRestoreHookTest, PackageBuildStillRejectsDuplicateImplOpType) {
  PythonOpDescriptorSnapshot first;
  first.op_type = kWithProtoOp;
  first.class_name = "ImplA";
  first.capabilities = kImplCapabilities;
  PythonOpDescriptorSnapshot second = first;
  second.class_name = "ImplB";

  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x10U, {first})), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x20U, {second})), SUCCESS);

  std::vector<uint8_t> out;
  EXPECT_EQ(builder.Build(out), FAILED);
}

// 端到端：manifest 实现条目 + 原型条目共存一个 package；fake bridge 在 scope 恢复时
// 只对 with_proto 声明原型。恢复后：
// - with_proto 的 adapter 补回 kShapeInfer/kInferMeta 与 infer_meta 回调；
// - no_proto 的 adapter 保持纯实现位、无 infer_meta（不谎报能力位）；
// - 两者都携带 deployment_artifacts（再保存时 PYTHON_CUSTOM_OPS 分区不丢）。
TEST_F(PythonCustomOpRestoreHookTest, RestoreBackfillsProtoCapabilitiesAndArtifact) {
  PythonOpDescriptorSnapshot proto_entry;
  proto_entry.op_type = kWithProtoOp;
  proto_entry.class_name = "infer_func";
  proto_entry.capabilities = kProtoCapabilities;

  PythonOpDescriptorSnapshot with_proto_impl;
  with_proto_impl.op_type = kWithProtoOp;
  with_proto_impl.class_name = kWithProtoOp;
  with_proto_impl.capabilities = kImplCapabilities;

  PythonOpDescriptorSnapshot no_proto_impl;
  no_proto_impl.op_type = kNoProtoOp;
  no_proto_impl.class_name = kNoProtoOp;
  no_proto_impl.capabilities = kImplCapabilities;

  PythonCustomOpPackageBuilder builder;
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x10U, {proto_entry})), SUCCESS);
  ASSERT_EQ(builder.AddArtifact(MakeArtifact(0x20U, {with_proto_impl, no_proto_impl})), SUCCESS);
  std::vector<uint8_t> package_bytes;
  ASSERT_EQ(builder.Build(package_bytes), SUCCESS);

  auto registry = RestoreIntoFreshRegistry(package_bytes);
  ASSERT_NE(registry, nullptr);

  // creator 已注册（纯 eager 位无 device 能力 → kDevice 作锚点）
  ASSERT_TRUE(registry->HasCreator(AscendString(kWithProtoOp)));
  ASSERT_TRUE(registry->HasCreator(AscendString(kNoProtoOp)));

  auto *with_proto_op = registry->CreateOrGetCustomOp(AscendString(kWithProtoOp), OpBackend::kDevice);
  auto *no_proto_op = registry->CreateOrGetCustomOp(AscendString(kNoProtoOp), OpBackend::kDevice);
  ASSERT_NE(with_proto_op, nullptr);
  ASSERT_NE(no_proto_op, nullptr);

  // 能力位补位与 scope 内的原型声明严格对应：查询经 registry 的 common capability
  // 通道（与 RT2 的 NeedCustomOpInferShape 同路）。
  EXPECT_NE(registry->GetCustomOpCommonCapability(AscendString(kWithProtoOp), CustomOpCapability::kShapeInfer),
            nullptr);
  EXPECT_NE(registry->GetCustomOpCommonCapability(AscendString(kWithProtoOp), CustomOpCapability::kInferMeta), nullptr);
  EXPECT_EQ(registry->GetCustomOpCommonCapability(AscendString(kNoProtoOp), CustomOpCapability::kShapeInfer), nullptr);
  EXPECT_EQ(registry->GetCustomOpCommonCapability(AscendString(kNoProtoOp), CustomOpCapability::kInferMeta), nullptr);

  // deployment_artifacts 交递：恢复出的 adapter 必须携带与该 op_type 相关的**全部**
  // artifact——分文件原型时声明所在的 artifact 与实现 artifact 是两个文件，只交出实现
  // artifact 会让原型在再保存时丢失，加载后 shape 推导能力静默缺失。
  const auto *artifact_provider = dynamic_cast<const CustomOpArtifactProvider *>(with_proto_op);
  ASSERT_NE(artifact_provider, nullptr);
  const auto artifacts = artifact_provider->GetDeploymentArtifacts();
  ASSERT_EQ(artifacts.size(), 2U);
  bool has_impl_artifact = false;
  bool has_proto_artifact = false;
  for (const auto &artifact : artifacts) {
    ASSERT_NE(artifact, nullptr);
    if (artifact->artifact_id == ToLowerHex(MakeDeterministicDigest(0x20U))) {
      has_impl_artifact = true;
      EXPECT_EQ(artifact->descriptors.size(), 2U);
    } else if (artifact->artifact_id == ToLowerHex(MakeDeterministicDigest(0x10U))) {
      has_proto_artifact = true;
      EXPECT_EQ(artifact->descriptors.size(), 1U);
    }
  }
  EXPECT_TRUE(has_impl_artifact);
  EXPECT_TRUE(has_proto_artifact);
}

TEST_F(PythonCustomOpRestoreHookTest, RestoreRejectsSameOpTypeFromDifferentArtifactAcrossModels) {
  // 两个模型带同一 op_type 但来自不同源文件（artifact_id 不同）：与 C++ 自定义算子的
  // SO 指纹冲突同口径，必须硬失败并点名，而不是双双恢复成功、之后按 op_type
  // 首个命中取到前一个模型的实现（静默算错 shape / 参数错位）。
  PythonCustomOpPackageBuilder first_builder;
  ASSERT_EQ(
      first_builder.AddArtifact(MakeArtifact(0x10U, {MakeImplSnapshot(kWithProtoOp), MakeImplSnapshot(kNoProtoOp)})),
      SUCCESS);
  std::vector<uint8_t> first_bytes;
  ASSERT_EQ(first_builder.Build(first_bytes), SUCCESS);

  PythonCustomOpPackageBuilder second_builder;
  ASSERT_EQ(
      second_builder.AddArtifact(MakeArtifact(0x20U, {MakeImplSnapshot(kWithProtoOp), MakeImplSnapshot(kNoProtoOp)})),
      SUCCESS);
  std::vector<uint8_t> second_bytes;
  ASSERT_EQ(second_builder.Build(second_bytes), SUCCESS);

  auto first = RestoreIntoFreshRegistry(first_bytes);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(RestoreIntoFreshRegistry(second_bytes), nullptr);

  // 冲突不产生副作用：第一个模型仍然完好。
  EXPECT_TRUE(first->HasCreator(AscendString(kWithProtoOp)));
}

TEST_F(PythonCustomOpRestoreHookTest, RestoreAllowsDifferentArtifactAfterModelUnloaded) {
  // 台账随 scope 关闭按引用计数回收：前一个模型卸载后，同名 op_type 的不同实现
  // 可以正常加载，否则一次加载就会永久锁死该 op_type。
  PythonCustomOpPackageBuilder first_builder;
  ASSERT_EQ(
      first_builder.AddArtifact(MakeArtifact(0x10U, {MakeImplSnapshot(kWithProtoOp), MakeImplSnapshot(kNoProtoOp)})),
      SUCCESS);
  std::vector<uint8_t> first_bytes;
  ASSERT_EQ(first_builder.Build(first_bytes), SUCCESS);

  PythonCustomOpPackageBuilder second_builder;
  ASSERT_EQ(
      second_builder.AddArtifact(MakeArtifact(0x20U, {MakeImplSnapshot(kWithProtoOp), MakeImplSnapshot(kNoProtoOp)})),
      SUCCESS);
  std::vector<uint8_t> second_bytes;
  ASSERT_EQ(second_builder.Build(second_bytes), SUCCESS);

  {
    auto first = RestoreIntoFreshRegistry(first_bytes);
    ASSERT_NE(first, nullptr);
  }
  auto second = RestoreIntoFreshRegistry(second_bytes);
  ASSERT_NE(second, nullptr);
  EXPECT_TRUE(second->HasCreator(AscendString(kWithProtoOp)));
}

// 保存侧 → Build() 写出真实 OM 字节 → 从 OM 字节里取 PYTHON_CUSTOM_OPS 分区。
// 后续断言都建立在"OM 里到底写了什么"这个事实上，而不是保存侧内存里的中间态。
static void SaveSplitProtoPackageThroughOmForUt(std::vector<uint8_t> &package_bytes) {
  const auto ge_root_model = MakeRootModelUsingPythonOp(kSplitProtoOp);
  ASSERT_NE(ge_root_model, nullptr);
  std::shared_ptr<OmFileSaveHelper> om_file_save_helper = std::make_shared<OmFileSaveHelper>();
  ModelHelper model_helper;
  ASSERT_EQ(model_helper.SaveCustomOpsPartition(om_file_save_helper, ge_root_model), SUCCESS);
  ModelBufferData model_buffer;
  ASSERT_EQ(om_file_save_helper->SaveModel("unused.om", model_buffer, false), SUCCESS);
  ModelPartition python_partition;
  ASSERT_TRUE(ExtractPartitionFromOmBuffer(model_buffer, ModelPartitionType::PYTHON_CUSTOM_OPS, python_partition));
  ASSERT_NE(python_partition.data, nullptr);
  ASSERT_GT(python_partition.size, 0U);
  uint8_t *partition_bytes = const_cast<uint8_t *>(python_partition.data);
  package_bytes.assign(partition_bytes, partition_bytes + python_partition.size);
}

// 两个源文件都必须进 OM：分文件原型时声明与实现分居两文件，少一个就少一块语义。
static void ExpectSplitProtoSourcesPackedInOm(const std::vector<uint8_t> &package_bytes) {
  ParsedPythonCustomOpPackage parsed;
  ASSERT_EQ(ParseAndValidatePythonCustomOpPackage(package_bytes.data(), package_bytes.size(), parsed), SUCCESS);
  ASSERT_EQ(parsed.artifacts.size(), 2U);
  std::set<std::string> source_names;
  for (const auto &artifact : parsed.artifacts) {
    ASSERT_NE(artifact, nullptr);
    (void)source_names.insert(artifact->source_name);
  }
  EXPECT_EQ(source_names.count(kSplitProtoSourceName), 1U);
  EXPECT_EQ(source_names.count(kSplitImplSourceName), 1U);
}

// 声明来自 proto.py：fake bridge 只在该文件出现在恢复输入里时才回报该 op_type 的声明；
// 声明可达 → kShapeInfer/kInferMeta 被补回，这正是异常形态里缺失的部分。
static void ExpectSplitProtoDeclarationsRestored(const CustomOpRegistryPtr &registry) {
  using GetSeenFn = uint32_t (*)();
  auto *get_proto_seen =
      reinterpret_cast<GetSeenFn>(dlsym(RTLD_DEFAULT, "GePythonCustomOpLoaderUtGetSplitProtoSourceSeen"));
  auto *get_impl_seen =
      reinterpret_cast<GetSeenFn>(dlsym(RTLD_DEFAULT, "GePythonCustomOpLoaderUtGetSplitImplSourceSeen"));
  ASSERT_NE(get_proto_seen, nullptr);
  ASSERT_NE(get_impl_seen, nullptr);
  EXPECT_EQ(get_proto_seen(), 1U);
  EXPECT_EQ(get_impl_seen(), 1U);
  EXPECT_NE(registry->GetCustomOpCommonCapability(AscendString(kSplitProtoOp), CustomOpCapability::kShapeInfer),
            nullptr);
  EXPECT_NE(registry->GetCustomOpCommonCapability(AscendString(kSplitProtoOp), CustomOpCapability::kInferMeta),
            nullptr);
}

// 恢复出的 adapter 携带两个 artifact（实现 + 原型），「加载→再保存」不会丢 proto.py：
// 原样交回打包链后应重建出同一份分区字节（缓存落盘/级联导出路径）。
static void ExpectSplitProtoArtifactsReshipped(const CustomOpArtifactProvider &provider,
                                               const std::vector<uint8_t> &package_bytes) {
  const auto restored_artifacts = provider.GetDeploymentArtifacts();
  ASSERT_EQ(restored_artifacts.size(), 2U);
  std::set<std::string> restored_source_names;
  for (const auto &artifact : restored_artifacts) {
    ASSERT_NE(artifact, nullptr);
    (void)restored_source_names.insert(artifact->source_name);
  }
  EXPECT_EQ(restored_source_names.count(kSplitProtoSourceName), 1U);
  EXPECT_EQ(restored_source_names.count(kSplitImplSourceName), 1U);
  PythonCustomOpPackageBuilder rebuilt_builder;
  for (const auto &artifact : restored_artifacts) {
    ASSERT_EQ(rebuilt_builder.AddArtifact(artifact), SUCCESS);
  }
  std::vector<uint8_t> rebuilt_bytes;
  ASSERT_EQ(rebuilt_builder.Build(rebuilt_bytes), SUCCESS);
  EXPECT_EQ(rebuilt_bytes, package_bytes);
}

// 分文件原型的 OM 字节往返（本特性的核心端到端用例）：
// 两个源文件（proto.py 承载 @register_op 声明、impl.py 承载 @register_op_impl 实现，同
// op_type）→ 冻结 → CollectDeploymentArtifactsFromBridge → BuildAdapterDescriptor →
// SaveCustomOpsPartition → Build() → **OM 字节** → ParseAndValidatePythonCustomOpPackage →
// Restore。断言原型 artifact 与实现 artifact 都在 OM 里，且加载侧据此恢复出 shape 推导能力。
//
// 这条链此前断了在 BuildAdapterDescriptor：artifact_by_op_type_ 只索引实现 artifact，
// 原型 artifact 收进 artifacts_by_id_ 却没有 getter，于是 proto.py 从未进 OM；恢复侧
// 因此拿不到声明，kShapeInfer 静默缺席。用例逐环断言，避免再次"某个中间态看起来对"。
TEST_F(PythonCustomOpRestoreHookTest, SplitProtoRoundTripThroughOmBytesRestoresShapeInfer) {
  SetScenario(kSplitProtoScenario);
  ASSERT_EQ(LoadPythonCustomOps(), SUCCESS);

  std::vector<uint8_t> package_bytes;
  SaveSplitProtoPackageThroughOmForUt(package_bytes);
  ExpectSplitProtoSourcesPackedInOm(package_bytes);

  // 恢复侧：走真 hook（含 bridge 的 scope 会话），进全新模型级 registry。
  auto registry = RestoreIntoFreshRegistry(package_bytes);
  ASSERT_NE(registry, nullptr);
  ExpectSplitProtoDeclarationsRestored(registry);

  auto *custom_op = registry->CreateOrGetCustomOp(AscendString(kSplitProtoOp), OpBackend::kDevice);
  ASSERT_NE(custom_op, nullptr);
  const auto *artifact_provider = dynamic_cast<const CustomOpArtifactProvider *>(custom_op);
  ASSERT_NE(artifact_provider, nullptr);
  ExpectSplitProtoArtifactsReshipped(*artifact_provider, package_bytes);
}
}  // namespace
}  // namespace custom_op
}  // namespace ge
