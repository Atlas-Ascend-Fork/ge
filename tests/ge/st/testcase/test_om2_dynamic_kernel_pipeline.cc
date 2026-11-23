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
#include "securec.h"
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "aicpu_task_struct.h"
#include "aicpu_engine_struct.h"
#include "base/registry/op_impl_space_registry_v2.h"
#include "common/share_graph.h"
#include "faker/aicpu_taskdef_faker.h"
#include "faker/fake_value.h"
#include "faker/ge_model_builder.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/common/helper/om2_package_helper.h"
#include "framework/common/taskdown_common.h"
#include "framework/runtime/rt_session.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "stub/gert_runtime_stub.h"
#include "tests/ge/common/om2_generated_model_runner.h"
#include "tests/ge/ut/ge/graph/load/om2_package_test_utils.h"
#define private public
#include "engine/aicpu/kernel/aicpu_resource_manager.h"
#undef private

namespace ge {
namespace {
struct PipelineObservation {
  bool cpu = false;
  size_t elements = 0U;
  size_t launches = 0U;
  size_t host_input_placeholders = 0U;
  size_t parses = 0U;
  size_t tilings = 0U;
  size_t infer_shapes = 0U;
  std::vector<int64_t> ext_shapes;
  std::vector<uint64_t> tiling_keys;
};
PipelineObservation *observation = nullptr;

graphStatus PipelineInfer(gert::InferShapeContext *context) {
  ++observation->infer_shapes;
  *context->GetOutputShape(0U) = *context->GetInputShape(0U);
  return GRAPH_SUCCESS;
}

graphStatus PipelineParse(gert::KernelContext *) {
  ++observation->parses;
  return GRAPH_SUCCESS;
}

graphStatus PipelineTiling(gert::TilingContext *context) {
  ++observation->tilings;
  const uint64_t count = context->GetInputShape(0U)->GetStorageShape().GetShapeSize();
  context->SetBlockDim(1U);
  context->SetTilingKey(count);
  context->GetWorkspaceSizes(1U)[0U] = count * sizeof(float);
  context->GetRawTilingData()->Append(count);
  return GRAPH_SUCCESS;
}

void *CreatePipelineCompileInfo() {
  return new uint64_t(0U);
}

void DeletePipelineCompileInfo(void *value) {
  delete static_cast<uint64_t *>(value);
}

void AddAtDeviceBoundary(const void *arguments) {
  const auto *addresses = static_cast<const uintptr_t *>(arguments);
  ASSERT_NE(addresses[0U], 0U);
  ASSERT_NE(addresses[1U], 0U);
  ASSERT_NE(addresses[2U], 0U);
  const auto *left = reinterpret_cast<const float *>(addresses[0U]);
  const auto *right = reinterpret_cast<const float *>(addresses[1U]);
  auto *output = reinterpret_cast<float *>(addresses[2U]);
  for (size_t i = 0U; i < observation->elements; ++i) {
    output[i] = left[i] + right[i];
  }
  ++observation->launches;
}

void CheckCoreWorkspace(const void *arguments, const size_t argument_size) {
  ASSERT_GE(argument_size, 4U * sizeof(uintptr_t));
  const auto *addresses = static_cast<const uintptr_t *>(arguments);
  ASSERT_NE(addresses[3U], 0U);
  // The scratch extent comes from this run's tiling result, not a fixed graph workspace.
  auto *workspace = reinterpret_cast<float *>(addresses[3U]);
  for (size_t i = 0U; i < observation->elements; ++i) {
    workspace[i] = static_cast<float>(i);
  }
  EXPECT_FLOAT_EQ(workspace[observation->elements - 1U], static_cast<float>(observation->elements - 1U));
}

void CheckCpuExtInfo(const aicpu::AicpuParamHead &head) {
  using ExtInfo = aicpu::FWKAdapter::ExtInfo;
  using ShapeAndType = aicpu::FWKAdapter::ShapeAndType;
  ASSERT_NE(head.extInfoAddr, 0U);
  ASSERT_GT(head.extInfoLength, sizeof(ExtInfo));
  const auto *bytes = reinterpret_cast<const uint8_t *>(head.extInfoAddr);
  bool found_input = false;
  for (size_t offset = 0U; offset + sizeof(ExtInfo) <= head.extInfoLength;) {
    const auto *info = reinterpret_cast<const ExtInfo *>(bytes + offset);
    ASSERT_LE(offset + sizeof(ExtInfo) + info->infoLen, head.extInfoLength);
    if (info->infoType == aicpu::FWKAdapter::FWK_ADPT_EXT_INPUT_SHAPE) {
      ASSERT_EQ(info->infoLen, 2U * sizeof(ShapeAndType));
      const auto *shapes = reinterpret_cast<const ShapeAndType *>(info->infoMsg);
      EXPECT_EQ(shapes[0U].dims[0U], observation->elements);
      EXPECT_EQ(shapes[1U].dims[0U], observation->elements);
      observation->ext_shapes.push_back(shapes[0U].dims[0U]);
      found_input = true;
    }
    offset += sizeof(ExtInfo) + info->infoLen;
  }
  EXPECT_TRUE(found_input);
}

uint32_t ExecuteCpuBoundary(void *arguments) {
  const auto *head = static_cast<const aicpu::AicpuParamHead *>(arguments);
  EXPECT_EQ(head->ioAddrNum, 3U);
  CheckCpuExtInfo(*head);
  AddAtDeviceBoundary(static_cast<const uint8_t *>(arguments) + sizeof(*head));
  return 0U;
}

// Only the hardware launch implementation is replaced. The generated model,
// production KernelRegistry functions, args handlers and shape updates remain real.
class PipelineAclStub : public gert::AclRuntimeStubImpl {
 public:
  aclError aclrtBinaryGetFunctionByEntry(aclrtBinHandle binary, uint64_t key, aclrtFuncHandle *function) override {
    observation->tiling_keys.push_back(key);
    return AclRuntimeStubImpl::aclrtBinaryGetFunctionByEntry(binary, key, function);
  }

  aclError aclrtLaunchKernelWithHostArgs(aclrtFuncHandle, uint32_t, aclrtStream, aclrtLaunchKernelCfg *, void *args,
                                         size_t size, aclrtPlaceHolderInfo *places, size_t place_count) override {
    std::vector<uint8_t> resolved(static_cast<uint8_t *>(args), static_cast<uint8_t *>(args) + size);
    for (size_t i = 0U; i < place_count; ++i) {
      if (places[i].addrOffset < 2U * sizeof(uintptr_t)) {
        ++observation->host_input_placeholders;
      }
      if (places[i].addrOffset > resolved.size() || places[i].dataOffset > resolved.size()) {
        return ACL_ERROR_INVALID_PARAM;
      }
      const uintptr_t address = reinterpret_cast<uintptr_t>(resolved.data() + places[i].dataOffset);
      if (memcpy_s(resolved.data() + places[i].addrOffset, resolved.size() - places[i].addrOffset, &address,
                   sizeof(address)) != EOK) {
        return ACL_ERROR_INVALID_PARAM;
      }
    }
    if (observation->cpu) {
      return ExecuteCpuBoundary(resolved.data());
    }
    CheckCoreWorkspace(resolved.data(), resolved.size());
    AddAtDeviceBoundary(resolved.data());
    return ACL_SUCCESS;
  }
};

class PipelineRtsStub : public gert::RuntimeStubImpl {
 public:
  rtError_t rtAicpuKernelLaunchExWithArgs(uint32_t, const char *, uint32_t, const rtAicpuArgsEx_t *args, rtSmDesc_t *,
                                          rtStream_t, uint32_t) override {
    auto *begin = static_cast<const uint8_t *>(args->args);
    std::vector<uint8_t> resolved(begin, begin + args->argsSize);
    for (size_t i = 0U; i < args->kernelOffsetInfoNum; ++i) {
      const auto &offset = args->kernelOffsetInfoPtr[i];
      if (offset.addrOffset > resolved.size() || offset.dataOffset > resolved.size()) {
        return ACL_ERROR_INVALID_PARAM;
      }
      const uintptr_t address = reinterpret_cast<uintptr_t>(resolved.data() + offset.dataOffset);
      if (memcpy_s(resolved.data() + offset.addrOffset, resolved.size() - offset.addrOffset, &address,
                   sizeof(address)) != EOK) {
        return ACL_ERROR_INVALID_PARAM;
      }
    }
    ExecuteCpuBoundary(resolved.data());
    return RT_ERROR_NONE;
  }
};

class ScopedPipelineHostKernel {
 public:
  explicit ScopedPipelineHostKernel(size_t &calls)
      : manager_(gert::AicpuResourceManager::GetInstance()),
        run_(manager_.run_cpu_kernel_),
        find_(manager_.aicpu_host_find_func_) {
    manager_.run_cpu_kernel_ = [&calls](void *) -> uint32_t {
      ++calls;
      return 1U;
    };
    manager_.aicpu_host_find_func_ = [](std::string) -> gert::AicpuHostProcFunc { return nullptr; };
  }
  ~ScopedPipelineHostKernel() {
    manager_.run_cpu_kernel_ = run_;
    manager_.aicpu_host_find_func_ = find_;
  }

 private:
  gert::AicpuResourceManager &manager_;
  std::function<uint32_t(void *)> run_;
  std::function<gert::AicpuHostProcFunc(std::string)> find_;
};

class Om2DynamicKernelPipelineST : public om2_test::DynamicPackageTest {
 protected:
  void SetUp() override {
    DynamicPackageTest::SetUp();
    GetThreadLocalContext().SetGraphOption(
        {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
    observation = &observed_;
    original_registry_ = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
    auto registry = std::make_shared<gert::OpImplSpaceRegistryV2>();
    auto *impl = registry->CreateOrGetOpImpl("Add");
    impl->infer_shape = PipelineInfer;
    impl->tiling = PipelineTiling;
    impl->tiling_parse = PipelineParse;
    impl->compile_info_creator = CreatePipelineCompileInfo;
    impl->compile_info_deleter = DeletePipelineCompileInfo;
    gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(registry);
  }

  void TearDown() override {
    gert::DefaultOpImplSpaceRegistryV2::GetInstance().SetSpaceRegistry(original_registry_);
    observation = nullptr;
    DynamicPackageTest::TearDown();
  }

  PipelineObservation observed_;
  std::shared_ptr<gert::OpImplSpaceRegistryV2> original_registry_;
};

GeRootModelPtr BuildKernelRoot(const bool cpu, const bool variant) {
  auto graph = gert::ShareGraph::BuildSingleNodeGraph();
  auto desc = graph->FindNode("add1")->GetOpDesc();
  OpDescUtilsEx::SetType(desc, "Add");
  desc->MutableAllInputName() = {{"x1", 0U}, {"x2", 1U}};
  desc->SetWorkspaceBytes({});
  (void)AttrUtils::SetBool(desc, "SmallShapeHostcpu", false);
  graph->SetGraphUnknownFlag(true);
  if (graph->TopologicalSorting() != GRAPH_SUCCESS) {
    return nullptr;
  }
  gert::GeModelBuilder builder(graph);
  if (cpu) {
    desc->SetOpKernelLibName(variant ? kEngineNameHostCpu : kEngineNameAiCpu);
    gert::AiCpuCCTaskDefFaker task;
    task.SetKernelType(static_cast<uint32_t>(variant ? ccKernelType::HOST_CPU : ccKernelType::AI_CPU));
    return builder.AddTaskDef("Add", task).BuildGeRootModel();
  }
  // Reserve one argument slot; each run's tiling still determines its byte extent.
  desc->SetWorkspaceBytes({0});
  gert::AiCoreTaskDefFaker task("om2_pipeline_add");
  task.ArgsFormat("{i0*}{i1*}{o0*}{ws0*}");
  if (variant) {
    task.WithHandle();
  }
  return builder.AddTaskDef("Add", task).FakeTbeBin({"Add"}).BuildGeRootModel();
}

class PipelinePackage {
 public:
  ~PipelinePackage() {
    (void)runner.Unload();
    session.DestroyResources();
  }

  void BuildAndLoad(const GeRootModelPtr &root) {
    gert::GertModelData generated;
    ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, generated), SUCCESS);
    ASSERT_EQ(generated.models.size(), 1U);
    ASSERT_NE(generated.models[0U]->runtime, nullptr);
    ASSERT_FALSE(generated.models[0U]->runtime->source_artifacts.empty());
    ASSERT_EQ(gert::SerializeGertModelData(generated, archive_, false), SUCCESS);
    ASSERT_EQ(
        gert::DeserializeGertModelData(static_cast<const uint8_t *>(archive_.data.get()), archive_.length, &restored_),
        SUCCESS);
    ASSERT_EQ(restored_.models.size(), 1U);
    ASSERT_NE(restored_.models[0U]->runtime, nullptr);
    ASSERT_TRUE(runner.Open(restored_.models[0U]->runtime->so_artifact)) << runner.Error();
    FillBinaries();
    GertModelDynamicLoadConfig dynamic;
    dynamic.rt_session = &session;
    GertModelLoadConfig load;
    load.dynamic_config = &dynamic;
    load.bin_num = names_.size();
    load.bin_files = names_.data();
    load.bin_data = binaries_.data();
    load.bin_size = sizes_.data();
    ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  }

  void FillBinaries() {
    if (restored_.kernels == nullptr) {
      return;
    }
    for (const auto &binary : restored_.kernels->binaries) {
      ASSERT_NE(binary, nullptr);
      names_.push_back(gert::GertGetStr(binary->file_name));
      binaries_.push_back(binary->data.get());
      sizes_.push_back(binary->data_size);
    }
  }

 private:
  ModelBufferData archive_;
  gert::GertModelData restored_;
  std::vector<const char *> names_;
  std::vector<const void *> binaries_;
  std::vector<uint64_t> sizes_;

 public:
  gert::RtSession session;
  om2_test::GeneratedModelRunner runner;
};

void RunPipelineAdd(PipelinePackage &package, const int64_t count, const gert::TensorPlacement placement,
                    bool expect_host_failure = false) {
  observation->elements = static_cast<size_t>(count);
  const size_t launches_before = observation->launches;
  std::vector<float> left(count, 1.25F);
  std::vector<float> right(count, -3.0F);
  left.back() = 9.0F;
  // TensorFaker initializes Value only for host/following placement. Its backing storage is host-accessible.
  auto first = gert::TensorFaker().Shape({count}).DataType(DT_FLOAT).Placement(gert::kOnHost).Value(left).Build();
  auto second = gert::TensorFaker().Shape({count}).DataType(DT_FLOAT).Placement(gert::kOnHost).Value(right).Build();
  first.GetTensor()->SetPlacement(placement);
  second.GetTensor()->SetPlacement(placement);
  const auto output_placement = observation->cpu ? placement : gert::kOnDeviceHbm;
  auto output = gert::TensorFaker()
                    .Shape({count})
                    .DataType(DT_FLOAT)
                    .Placement(gert::kOnHost)
                    .Value(std::vector<float>(count, -777.0F))
                    .Build();
  output.GetTensor()->SetPlacement(output_placement);
  gert::Tensor *inputs[] = {first.GetTensor(), second.GetTensor()};
  gert::Tensor *outputs[] = {output.GetTensor()};
  GertModelRunConfig run;
  run.input_count = 2U;
  run.input_data = inputs;
  run.output_count = 1U;
  run.output_data = outputs;
  if (expect_host_failure) {
    EXPECT_NE(package.runner.Run(run), ACL_SUCCESS);
    return;
  }
  ASSERT_EQ(package.runner.Run(run), ACL_SUCCESS);
  EXPECT_EQ(observation->launches, launches_before + 1U);
  EXPECT_EQ(outputs[0U]->GetOriginShape(), gert::Shape({count}));
  EXPECT_EQ(outputs[0U]->GetStorageShape(), gert::Shape({count}));
  EXPECT_EQ(outputs[0U]->GetDataType(), DT_FLOAT);
  ASSERT_NE(outputs[0U]->GetData<float>(), nullptr);
  for (size_t i = 0U; i < left.size(); ++i) {
    EXPECT_FLOAT_EQ(outputs[0U]->GetData<float>()[i], left[i] + right[i]);
  }
}

/**
 * 用例描述：动态 AICore 模型由实际 lowering/collector/codegen 生成，普通/handle task 各运行两轮。
 * 预置条件：仅替换 Add 算子的 infer/tiling 插件和 ACL 硬件 launch；框架 Kernel 保持真实。
 * 测试步骤：每种 task Build、归档往返、Load 一次，运行长度 3/7 的输入，然后 Unload。
 * 预期结果：真实 infer、tiling parse、tiling 被调用；每轮 launch 地址产生独立 CPU 参考结果，
 *           shape 和数值随本轮输入刷新；一个模型只编译一次，不覆盖 atomic/mix/legacy tiling。
 */
TEST_F(Om2DynamicKernelPipelineST, AicoreTilingWorkspaceAndLaunchSurvivePackageRoundTrip) {
  // slow_test_limit = 5000
  for (const bool with_handle : {false, true}) {
    SCOPED_TRACE(with_handle);
    observed_ = {};
    gert::GertRuntimeStub stub(std::make_unique<PipelineRtsStub>(), true, std::make_unique<PipelineAclStub>());
    const auto root = BuildKernelRoot(false, with_handle);
    ASSERT_NE(root, nullptr);
    PipelinePackage package;
    ASSERT_NO_FATAL_FAILURE(package.BuildAndLoad(root));
    const auto copies_before =
        stub.GetAclRuntimeStub().GetRtMemcpyRecords().size() + stub.GetRtsRuntimeStub().GetRtMemcpyRecords().size();
    ASSERT_NO_FATAL_FAILURE(RunPipelineAdd(package, 3, gert::kOnHost));
    if (observed_.host_input_placeholders != 0U) {
      // Small Host inputs can travel inline with the launch arguments.
      EXPECT_EQ(observed_.host_input_placeholders, 2U);
    } else {
      EXPECT_GT(
          stub.GetAclRuntimeStub().GetRtMemcpyRecords().size() + stub.GetRtsRuntimeStub().GetRtMemcpyRecords().size(),
          copies_before);
    }
    const auto host_input_placeholders = observed_.host_input_placeholders;
    ASSERT_NO_FATAL_FAILURE(RunPipelineAdd(package, 7, gert::kOnDeviceHbm));
    EXPECT_EQ(observed_.host_input_placeholders, host_input_placeholders);
    EXPECT_EQ(observed_.parses, 1U);
    EXPECT_EQ(observed_.tilings, 2U);
    EXPECT_EQ(observed_.infer_shapes, 2U);
    if (with_handle) {
      EXPECT_EQ(observed_.tiling_keys, (std::vector<uint64_t>{3U, 7U}));
    }
    ASSERT_EQ(package.runner.Unload(), ACL_SUCCESS);
  }
}

/**
 * 用例描述：动态 AICPU CC 参数和 extinfo 经模型生成、归档及加载后跨轮刷新，并验证 HostCC 执行失败的传播。
 * 预置条件：CC 仅接管 RTS/ACL launch；HostCC 注册返回错误的库函数桩。
 * 测试步骤：CC 构建并加载一次，运行长度 3/7 的输入，检查 launch 现场，再卸载；构建、加载并运行 HostCC。
 * 预期结果：每轮 ioAddrNum 为 3，extinfo 的两个输入 shape 均为本轮长度；输出等于 CPU Add；
 *           CC 两轮复用初始化资源且 Unload 成功；HostCC 经默认 collector 调到库函数桩并报告失败，
 *           未覆盖 TF、依赖输出值的四类算子及自定义二进制。
 */
TEST_F(Om2DynamicKernelPipelineST, AicpuCcRefreshesArgsAndHostCcPropagatesRunFailure) {
  // slow_test_limit = 5000
  observed_.cpu = true;
  gert::GertRuntimeStub stub(std::make_unique<PipelineRtsStub>(), true, std::make_unique<PipelineAclStub>());
  const auto root = BuildKernelRoot(true, false);
  ASSERT_NE(root, nullptr);
  PipelinePackage package;
  ASSERT_NO_FATAL_FAILURE(package.BuildAndLoad(root));
  ASSERT_NO_FATAL_FAILURE(RunPipelineAdd(package, 3, gert::kOnDeviceHbm));
  ASSERT_NO_FATAL_FAILURE(RunPipelineAdd(package, 7, gert::kOnDeviceHbm));
  EXPECT_EQ(observed_.ext_shapes, (std::vector<int64_t>{3, 7}));
  ASSERT_EQ(package.runner.Unload(), ACL_SUCCESS);
  size_t host_calls = 0U;
  ScopedPipelineHostKernel host_kernel(host_calls);
  const auto host_root = BuildKernelRoot(true, true);
  ASSERT_NE(host_root, nullptr);
  PipelinePackage host_package;
  ASSERT_NO_FATAL_FAILURE(host_package.BuildAndLoad(host_root));
  ASSERT_NO_FATAL_FAILURE(RunPipelineAdd(host_package, 3, gert::kOnHost, true));
  EXPECT_EQ(host_calls, 1U);
}
}  // namespace
}  // namespace ge
