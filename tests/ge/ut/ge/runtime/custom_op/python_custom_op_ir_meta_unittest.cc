/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <new>
#include <string>
#include <vector>

#include "common/python_runtime/python_artifact_utils.h"
#include "common/python_runtime/python_bridge_loader_utils.h"
#include "exe_graph/runtime/op_compile_context.h"
#include "runtime/custom_op/python_custom_op_adapter.h"
#include "runtime/custom_op/python_custom_op_bridge_c_api.h"

namespace ge {
namespace custom_op {
namespace {
namespace artifact = ::ge::python_artifact;
namespace bridge_loader = ::ge::python_bridge_loader;

struct MockPythonCustomOpHolder {};

void *CreateMockPythonCustomOpHolder(const PythonCustomOpAdapterDescriptorView *desc) {
  return ((desc == nullptr) || (desc->impl_descriptor_key.data == nullptr)) ? nullptr
                                                                            : new (std::nothrow)
                                                                                  MockPythonCustomOpHolder();
}

void DestroyMockPythonCustomOpHolder(void *holder) {
  delete static_cast<MockPythonCustomOpHolder *>(holder);
}

graphStatus ExecuteMockPythonCustomOp(const void *holder, gert::EagerOpExecutionContext *ctx) {
  (void)ctx;
  EXPECT_NE(holder, nullptr);
  return (holder != nullptr) ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus ExecuteHostMockPythonCustomOp(const void *holder, gert::HostCpuOpExecutionContext *ctx) {
  (void)ctx;
  EXPECT_NE(holder, nullptr);
  return (holder != nullptr) ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus DeclareMockPythonCustomOp(const void *holder, gert::AnnotatedArgsContext *ctx) {
  return ((holder != nullptr) && (ctx != nullptr)) ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus CompileMockPythonCustomOp(const void *holder, gert::OpCompileContext *ctx) {
  return ((holder != nullptr) && (ctx != nullptr)) ? GRAPH_SUCCESS : GRAPH_FAILED;
}

graphStatus SerializeMockPythonCustomOp(const void *holder, std::vector<uint8_t> *output) {
  if ((holder == nullptr) || (output == nullptr)) {
    return GRAPH_FAILED;
  }
  *output = {0x01U, 0x02U, 0x03U};
  return GRAPH_SUCCESS;
}

graphStatus DeserializeMockPythonCustomOp(const void *holder, const uint8_t *data, size_t size) {
  if ((holder == nullptr) || (data == nullptr) || (size == 0U)) {
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}

graphStatus SerializeFailingPythonCustomOp(const void *holder, std::vector<uint8_t> *output) {
  (void)holder;
  (void)output;
  return GRAPH_FAILED;
}

graphStatus DeserializeFailingPythonCustomOp(const void *holder, const uint8_t *data, size_t size) {
  (void)holder;
  (void)data;
  (void)size;
  return GRAPH_FAILED;
}

void *CreateNullPythonCustomOpHolder(const PythonCustomOpAdapterDescriptorView *desc) {
  (void)desc;
  return nullptr;
}

struct MockPythonCustomOpBridgeLoadState {
  PythonCustomOpBridgeApi api{};
  const PythonCustomOpBridgeApi *api_to_return{nullptr};
  uint32_t close_count{0U};
  uint32_t set_config_count{0U};
  uint32_t register_count{0U};
  uint32_t collect_artifact_count{0U};
};

int g_mock_python_custom_op_bridge_handle = 0;
MockPythonCustomOpBridgeLoadState g_mock_python_custom_op_bridge_load_state;

Status SetMockPythonCustomOpArtifactConfig(const PythonCustomOpBridgeArtifactConfig *config) {
  if ((config == nullptr) || (config->artifact_root == nullptr) || (config->native_module_path == nullptr)) {
    return FAILED;
  }
  ++g_mock_python_custom_op_bridge_load_state.set_config_count;
  return SUCCESS;
}

Status RegisterMockPythonCustomOps(const PythonCustomOpRegistrar *registrar) {
  (void)registrar;
  ++g_mock_python_custom_op_bridge_load_state.register_count;
  return SUCCESS;
}

void ResetMockPythonCustomOpBridgeState() {}

void ShutdownMockPythonCustomOpBridge() {}

Status CollectMockPythonCustomOpDeploymentArtifacts(const PythonCustomOpDeploymentArtifactSink *sink, void *user_data) {
  (void)user_data;
  ++g_mock_python_custom_op_bridge_load_state.collect_artifact_count;
  return ((sink == nullptr) || (sink->on_artifact == nullptr)) ? FAILED : SUCCESS;
}

const PythonCustomOpBridgeApi *GetMockPythonCustomOpBridgeApi() {
  return g_mock_python_custom_op_bridge_load_state.api_to_return;
}

std::string ResolveMockPythonCustomOpBridgePath(const char *path) {
  return (path == nullptr) ? std::string() : std::string("/tmp/libge_python_custom_op_bridge.so");
}

void *OpenMockPythonCustomOpBridge(const char *path, int flags) {
  (void)flags;
  return (path == nullptr) ? nullptr : static_cast<void *>(&g_mock_python_custom_op_bridge_handle);
}

int CloseMockPythonCustomOpBridge(void *handle) {
  if (handle != nullptr) {
    ++g_mock_python_custom_op_bridge_load_state.close_count;
  }
  return 0;
}

void *LookupMockPythonCustomOpBridgeSymbol(void *handle, const char *symbol) {
  if ((handle == nullptr) || (symbol == nullptr)) {
    return nullptr;
  }
  return reinterpret_cast<void *>(&GetMockPythonCustomOpBridgeApi);
}

artifact::PythonRuntimeKey ResolveMockPythonRuntimeKey() {
  return {};
}

bool IsMockPythonCustomOpBridgeApiValid(const PythonCustomOpBridgeApi *api, const uint32_t expected_abi) {
  return (api != nullptr) && (api->abi_version == expected_abi) && (api->set_artifact_config != nullptr) &&
         (api->register_custom_ops != nullptr) && (api->reset_bridge_state != nullptr) &&
         (api->shutdown_bridge != nullptr) && (api->collect_deployment_artifacts != nullptr);
}

bridge_loader::BridgeLoadDependencies MakeMockPythonCustomOpBridgeLoadDependencies() {
  return bridge_loader::BridgeLoadDependencies{
      &ResolveMockPythonCustomOpBridgePath, &OpenMockPythonCustomOpBridge,
      &CloseMockPythonCustomOpBridge,       &LookupMockPythonCustomOpBridgeSymbol,
      &ResolveMockPythonRuntimeKey,         kPythonCustomOpBridgeGetApiSymbol,
      kPythonCustomOpBridgeAbiVersion,      0,
  };
}

void ResetMockPythonCustomOpBridgeLoadState(const uint32_t abi_version) {
  g_mock_python_custom_op_bridge_load_state = MockPythonCustomOpBridgeLoadState{};
  g_mock_python_custom_op_bridge_load_state.api = PythonCustomOpBridgeApi{
      abi_version,
      &SetMockPythonCustomOpArtifactConfig,
      &RegisterMockPythonCustomOps,
      &ResetMockPythonCustomOpBridgeState,
      &ShutdownMockPythonCustomOpBridge,
      &CollectMockPythonCustomOpDeploymentArtifacts,
  };
  g_mock_python_custom_op_bridge_load_state.api_to_return = &g_mock_python_custom_op_bridge_load_state.api;
}

bridge_loader::BridgeLoadStatus LoadMockPythonCustomOpBridge(
    bridge_loader::LoadedBridgeCandidate<PythonCustomOpBridgeApi> &loaded_bridge) {
  const artifact::BridgeLibraryCandidate candidate{
      "libge_python_custom_op_bridge.so",
      "/tmp/custom_op/python_custom_op_artifacts/cp311-linux-aarch64",
      "/tmp/custom_op/python_custom_op_artifacts/cp311-linux-aarch64/_ge_custom_op_native.so",
  };
  return bridge_loader::TryLoadBridgeCandidate<PythonCustomOpBridgeApi, PythonCustomOpBridgeArtifactConfig>(
      artifact::PythonRuntimeKey{}, candidate, MakeMockPythonCustomOpBridgeLoadDependencies(),
      &IsMockPythonCustomOpBridgeApiValid, loaded_bridge);
}
}  // namespace

TEST(PythonCustomOpAdapter, forwards_execute_without_ir_meta_pod) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_without_ir_meta";
  desc.op_type = "PythonCustomOpAdapterUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kEagerExecute);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.execute = ExecuteMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());
    EXPECT_EQ(adapter.Execute(static_cast<gert::EagerOpExecutionContext *>(nullptr)), GRAPH_SUCCESS);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, keeps_legacy_execute_without_registered_ir) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_legacy_without_ir";
  desc.op_type = "PythonCustomOpLegacyWithoutIrUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kEagerExecute);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.execute = ExecuteMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());
    EXPECT_EQ(adapter.Execute(static_cast<gert::EagerOpExecutionContext *>(nullptr)), GRAPH_SUCCESS);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, forwards_host_execute_without_ir_meta_pod) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_host_without_ir_meta";
  desc.op_type = "PythonCustomOpHostAdapterUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kHostCpuExecute);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.host_cpu_execute = ExecuteHostMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());
    EXPECT_EQ(adapter.Execute(static_cast<gert::HostCpuOpExecutionContext *>(nullptr)), GRAPH_SUCCESS);
    EXPECT_EQ(adapter.Execute(static_cast<gert::EagerOpExecutionContext *>(nullptr)), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, validates_host_execute_callback_by_capability) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_host_callback";
  desc.op_type = "PythonCustomOpHostCallbackUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kHostCpuExecute);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  EXPECT_FALSE(callbacks.IsValid(desc.capabilities));

  callbacks.host_cpu_execute = ExecuteHostMockPythonCustomOp;
  EXPECT_TRUE(callbacks.IsValid(desc.capabilities));
}

TEST(PythonCustomOpAdapter, validates_annotated_args_callback_by_capability) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_annotated_args_callback";
  desc.op_type = "PythonCustomOpIrMetaUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kAnnotatedArgs);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  EXPECT_FALSE(callbacks.IsValid(desc.capabilities));

  callbacks.declare_launch_args = DeclareMockPythonCustomOp;
  EXPECT_TRUE(callbacks.IsValid(desc.capabilities));
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, validates_and_forwards_compile_callback_by_capability) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_compile_callback";
  desc.op_type = "PythonCustomOpCompileUt";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kCompilable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  EXPECT_FALSE(callbacks.IsValid(desc.capabilities));

  callbacks.compile_impl = CompileMockPythonCustomOp;
  EXPECT_TRUE(callbacks.IsValid(desc.capabilities));
  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());
    gert::OpCompileContext ctx;
    EXPECT_EQ(adapter.Compile(&ctx), GRAPH_SUCCESS);
    EXPECT_EQ(adapter.Compile(nullptr), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpBridgeAbi, rejects_mismatched_abi_before_registration_and_accepts_current) {
  bridge_loader::LoadedBridgeCandidate<PythonCustomOpBridgeApi> loaded_bridge;
  ResetMockPythonCustomOpBridgeLoadState(kPythonCustomOpBridgeAbiVersion + 1U);

  EXPECT_EQ(LoadMockPythonCustomOpBridge(loaded_bridge), bridge_loader::BridgeLoadStatus::kInvalidApi);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.close_count, 1U);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.set_config_count, 0U);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.register_count, 0U);

  ResetMockPythonCustomOpBridgeLoadState(kPythonCustomOpBridgeAbiVersion);
  ASSERT_EQ(LoadMockPythonCustomOpBridge(loaded_bridge), bridge_loader::BridgeLoadStatus::kSuccess);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.close_count, 0U);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.set_config_count, 1U);
  ASSERT_NE(loaded_bridge.api, nullptr);
  EXPECT_EQ(loaded_bridge.api->register_custom_ops(nullptr), SUCCESS);
  EXPECT_EQ(g_mock_python_custom_op_bridge_load_state.register_count, 1U);
}

TEST(PythonCustomOpBridgeAbi, rejects_null_portable_callbacks) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  // serialize and deserialize default to nullptr

  CustomOpCapabilityMask capabilities = 0U;
  AddCustomOpCapability(capabilities, CustomOpCapability::kPortable);

  EXPECT_FALSE(callbacks.IsValid(capabilities));
}

TEST(PythonCustomOpBridgeAbi, rejects_partial_portable_callbacks) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  // Only serialize set, deserialize remains nullptr
  callbacks.serialize = SerializeMockPythonCustomOp;

  CustomOpCapabilityMask capabilities = 0U;
  AddCustomOpCapability(capabilities, CustomOpCapability::kPortable);

  EXPECT_FALSE(callbacks.IsValid(capabilities));
}

TEST(PythonCustomOpBridgeAbi, accepts_portable_with_both_callbacks) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  CustomOpCapabilityMask capabilities = 0U;
  AddCustomOpCapability(capabilities, CustomOpCapability::kPortable);

  EXPECT_TRUE(callbacks.IsValid(capabilities));
}

TEST(PythonCustomOpBridgeAbi, accepts_portable_combined_with_eager_and_annotated) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.execute = ExecuteMockPythonCustomOp;
  callbacks.declare_launch_args = DeclareMockPythonCustomOp;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  CustomOpCapabilityMask capabilities = 0U;
  AddCustomOpCapability(capabilities, CustomOpCapability::kEagerExecute);
  AddCustomOpCapability(capabilities, CustomOpCapability::kAnnotatedArgs);
  AddCustomOpCapability(capabilities, CustomOpCapability::kPortable);

  EXPECT_TRUE(callbacks.IsValid(capabilities));
}

TEST(PythonCustomOpBridgeAbi, rejects_unsupported_capability_still_after_portable_added) {
  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;

  CustomOpCapabilityMask capabilities = 0U;
  // kShapeInfer, kInferMeta and kArgsUpdater remain unsupported at this layer
  AddCustomOpCapability(capabilities, CustomOpCapability::kShapeInfer);

  EXPECT_FALSE(callbacks.IsValid(capabilities));
}

TEST(PythonCustomOpAdapter, serializes_and_deserializes_portable_only) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_portable_only";
  desc.op_type = "PythonCustomOpAdapterPortableOnly";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> buffer;
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_SUCCESS);
    ASSERT_EQ(buffer.size(), 3U);
    EXPECT_EQ(buffer[0], 0x01U);
    EXPECT_EQ(buffer[1], 0x02U);
    EXPECT_EQ(buffer[2], 0x03U);
    EXPECT_EQ(adapter.Deserialize(buffer), GRAPH_SUCCESS);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, serializes_portable_combined_with_eager_and_annotated) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_portable_combined";
  desc.op_type = "PythonCustomOpAdapterPortableCombined";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kEagerExecute);
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kAnnotatedArgs);
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.execute = ExecuteMockPythonCustomOp;
  callbacks.declare_launch_args = DeclareMockPythonCustomOp;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    // Same holder used for all capabilities
    EXPECT_EQ(adapter.Execute(static_cast<gert::EagerOpExecutionContext *>(nullptr)), GRAPH_SUCCESS);

    std::vector<uint8_t> buffer;
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_SUCCESS);
    EXPECT_EQ(buffer.size(), 3U);
    EXPECT_EQ(adapter.Deserialize(buffer), GRAPH_SUCCESS);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, rejects_serialize_without_portable_capability) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_no_portable";
  desc.op_type = "PythonCustomOpAdapterNoPortable";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kEagerExecute);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.execute = ExecuteMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> buffer;
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_FAILED);
    EXPECT_EQ(adapter.Deserialize(buffer), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, rejects_deserialize_empty_buffer) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_deserialize_empty";
  desc.op_type = "PythonCustomOpAdapterDeserializeEmpty";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> empty_buffer;
    EXPECT_EQ(adapter.Deserialize(empty_buffer), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, serialize_fails_when_callback_returns_failure) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_serialize_failure";
  desc.op_type = "PythonCustomOpAdapterSerializeFailure";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeFailingPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> buffer;
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, deserialize_fails_when_callback_returns_failure) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_deserialize_failure";
  desc.op_type = "PythonCustomOpAdapterDeserializeFailure";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeFailingPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> buffer = {0x01U, 0x02U};
    EXPECT_EQ(adapter.Deserialize(buffer), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, serialize_fails_when_holder_create_returns_null) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_holder_create_null";
  desc.op_type = "PythonCustomOpAdapterHolderCreateNull";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateNullPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  // Registration requires valid callbacks struct but holder create returning null means adapter is invalid
  // Note: Register succeeds because callbacks are valid (holder create is non-null),
  // but adapter becomes invalid.
  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    EXPECT_FALSE(adapter.IsValid());

    std::vector<uint8_t> buffer;
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_FAILED);
    EXPECT_EQ(adapter.Deserialize(buffer), GRAPH_FAILED);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, destroy_allows_unregister_after_adapter_scope) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_destroy_cleanup";
  desc.op_type = "PythonCustomOpAdapterDestroyCleanup";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));

  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());
    // Active adapter blocks unregister
    EXPECT_FALSE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
  }

  // After adapter destruction, unregister succeeds
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, serialize_clears_output_before_callback) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_serialize_clears";
  desc.op_type = "PythonCustomOpAdapterSerializeClears";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    std::vector<uint8_t> buffer = {0xFFU, 0xFEU, 0xFDU, 0xFCU, 0xFBU};
    EXPECT_EQ(adapter.Serialize(buffer), GRAPH_SUCCESS);
    // Buffer was cleared then filled by callback with exactly 3 bytes
    ASSERT_EQ(buffer.size(), 3U);
    EXPECT_EQ(buffer[0], 0x01U);
    EXPECT_EQ(buffer[1], 0x02U);
    EXPECT_EQ(buffer[2], 0x03U);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

TEST(PythonCustomOpAdapter, deserialize_rejects_empty_buffer_before_callback) {
  PythonCustomOpAdapterDescriptor desc;
  desc.impl_descriptor_key = "python_adapter_deserialize_reject_empty";
  desc.op_type = "PythonCustomOpAdapterDeserializeRejectEmpty";
  AddCustomOpCapability(desc.capabilities, CustomOpCapability::kPortable);

  PythonCustomOpAdapterCallbacks callbacks;
  callbacks.create_impl_holder = CreateMockPythonCustomOpHolder;
  callbacks.destroy_impl_holder = DestroyMockPythonCustomOpHolder;
  callbacks.serialize = SerializeMockPythonCustomOp;
  callbacks.deserialize = DeserializeMockPythonCustomOp;

  ASSERT_TRUE(PythonCustomOpImplRuntimeRegistry::Register(desc, callbacks));
  {
    PythonCustomOpAdapter adapter(desc);
    ASSERT_TRUE(adapter.IsValid());

    const std::vector<uint8_t> empty_buffer;
    EXPECT_EQ(adapter.Deserialize(empty_buffer), GRAPH_FAILED);

    // But non-empty succeeds
    const std::vector<uint8_t> valid_buffer = {0x01U};
    EXPECT_EQ(adapter.Deserialize(valid_buffer), GRAPH_SUCCESS);
  }
  EXPECT_TRUE(PythonCustomOpImplRuntimeRegistry::Unregister(desc.impl_descriptor_key));
}

}  // namespace custom_op
}  // namespace ge
